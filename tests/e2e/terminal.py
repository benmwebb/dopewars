"""Run a program in a pseudo-terminal and look at its screen, for testing
the curses client.

The program runs in a pseudo-terminal, and everything it writes is fed
through pyte, a terminal emulator, so tests can check the rendered screen
rather than the raw escape sequences. The terminal is 80x25.

On Unix, the program runs under pexpect, with TERM=xterm in the C.UTF-8
locale. On Windows, it runs in a Windows pseudo-console (ConPTY), which
turns what it draws on the console into escape sequences.
"""

import codecs
import queue
import subprocess
import sys
import threading
import time

import pyte

ROWS, COLS = 25, 80
IS_WINDOWS = sys.platform == 'win32'


class _UnixProcess(object):
    """A program in a Unix pseudo-terminal"""

    def __init__(self, argv, env, cwd):
        import pexpect
        self._timeout = pexpect.TIMEOUT
        self._eof = pexpect.EOF
        # A UTF-8 locale, as most users have, so the client's UTF-8 code
        # paths are tested
        env = dict(env, TERM='xterm', LINES=str(ROWS), COLUMNS=str(COLS),
                   LC_ALL='C.UTF-8')
        self.child = pexpect.spawn(argv[0], argv[1:], env=env, cwd=cwd,
                                   dimensions=(ROWS, COLS))

    def read(self, timeout):
        """Output within `timeout` (bytes, maybe empty); EOFError at exit"""
        try:
            return self.child.read_nonblocking(65536, timeout)
        except self._timeout:
            return b''
        except self._eof:
            raise EOFError

    def send(self, keys):
        self.child.send(keys)

    def exit_status(self):
        """Wait for the exit status; a program killed by a signal fails"""
        self.child.close()
        if self.child.signalstatus is not None:
            raise AssertionError("program killed by signal %d"
                                 % self.child.signalstatus)
        return self.child.exitstatus

    def isalive(self):
        return self.child.isalive()

    def kill(self):
        self.child.terminate(force=True)


class _WindowsProcess(object):
    """A program in a Windows pseudo-console, using the ConPTY API directly
    (through ctypes, so nothing needs to be installed)"""

    def __init__(self, argv, env, cwd):
        import ctypes
        from ctypes import wintypes
        k32 = ctypes.WinDLL('kernel32', use_last_error=True)
        self._k32, self._ctypes = k32, ctypes
        HANDLE = wintypes.HANDLE

        class COORD(ctypes.Structure):
            _fields_ = [('X', wintypes.SHORT), ('Y', wintypes.SHORT)]

        class STARTUPINFOEXW(ctypes.Structure):
            _fields_ = [('cb', wintypes.DWORD), ('lpReserved', wintypes.LPWSTR),
                        ('lpDesktop', wintypes.LPWSTR),
                        ('lpTitle', wintypes.LPWSTR),
                        ('dwX', wintypes.DWORD), ('dwY', wintypes.DWORD),
                        ('dwXSize', wintypes.DWORD),
                        ('dwYSize', wintypes.DWORD),
                        ('dwXCountChars', wintypes.DWORD),
                        ('dwYCountChars', wintypes.DWORD),
                        ('dwFillAttribute', wintypes.DWORD),
                        ('dwFlags', wintypes.DWORD),
                        ('wShowWindow', wintypes.WORD),
                        ('cbReserved2', wintypes.WORD),
                        ('lpReserved2', ctypes.c_void_p),
                        ('hStdInput', HANDLE), ('hStdOutput', HANDLE),
                        ('hStdError', HANDLE),
                        ('lpAttributeList', ctypes.c_void_p)]

        class PROCESS_INFORMATION(ctypes.Structure):
            _fields_ = [('hProcess', HANDLE), ('hThread', HANDLE),
                        ('dwProcessId', wintypes.DWORD),
                        ('dwThreadId', wintypes.DWORD)]

        k32.CreatePseudoConsole.argtypes = [COORD, HANDLE, HANDLE,
                                            wintypes.DWORD,
                                            ctypes.POINTER(HANDLE)]
        k32.CreatePseudoConsole.restype = ctypes.c_long
        k32.ClosePseudoConsole.argtypes = [HANDLE]
        k32.ClosePseudoConsole.restype = None
        k32.InitializeProcThreadAttributeList.argtypes = [
            ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD,
            ctypes.POINTER(ctypes.c_size_t)]
        k32.UpdateProcThreadAttribute.argtypes = [
            ctypes.c_void_p, wintypes.DWORD, ctypes.c_size_t,
            ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p,
            ctypes.c_void_p]
        k32.CreateProcessW.argtypes = [
            wintypes.LPCWSTR, wintypes.LPWSTR, ctypes.c_void_p,
            ctypes.c_void_p, wintypes.BOOL, wintypes.DWORD, ctypes.c_void_p,
            wintypes.LPCWSTR, ctypes.c_void_p, ctypes.c_void_p]
        k32.ReadFile.argtypes = [HANDLE, ctypes.c_void_p, wintypes.DWORD,
                                 ctypes.POINTER(wintypes.DWORD),
                                 ctypes.c_void_p]
        k32.WriteFile.argtypes = [HANDLE, ctypes.c_void_p, wintypes.DWORD,
                                  ctypes.POINTER(wintypes.DWORD),
                                  ctypes.c_void_p]
        k32.WaitForSingleObject.argtypes = [HANDLE, wintypes.DWORD]
        k32.WaitForSingleObject.restype = wintypes.DWORD
        k32.GetExitCodeProcess.argtypes = [HANDLE,
                                           ctypes.POINTER(wintypes.DWORD)]
        k32.TerminateProcess.argtypes = [HANDLE, wintypes.UINT]
        k32.CloseHandle.argtypes = [HANDLE]

        def check(ok, what):
            if not ok:
                raise ctypes.WinError(ctypes.get_last_error(), what)

        # Pipes to and from the pseudo-console
        in_read, self.in_write = HANDLE(), HANDLE()
        self.out_read, out_write = HANDLE(), HANDLE()
        check(k32.CreatePipe(ctypes.byref(in_read),
                             ctypes.byref(self.in_write), None, 0),
              'CreatePipe')
        check(k32.CreatePipe(ctypes.byref(self.out_read),
                             ctypes.byref(out_write), None, 0), 'CreatePipe')
        self.hpc = HANDLE()
        hr = k32.CreatePseudoConsole(COORD(COLS, ROWS), in_read, out_write,
                                     0, ctypes.byref(self.hpc))
        if hr != 0:
            raise OSError("CreatePseudoConsole failed: 0x%08x"
                          % (hr & 0xffffffff))

        # Start the program attached to it
        size = ctypes.c_size_t()
        k32.InitializeProcThreadAttributeList(None, 1, 0, ctypes.byref(size))
        attrs = ctypes.create_string_buffer(size.value)
        check(k32.InitializeProcThreadAttributeList(attrs, 1, 0,
                                                    ctypes.byref(size)),
              'InitializeProcThreadAttributeList')
        PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE = 0x00020016
        check(k32.UpdateProcThreadAttribute(
                  attrs, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                  self.hpc, ctypes.sizeof(HANDLE), None, None),
              'UpdateProcThreadAttribute')
        si = STARTUPINFOEXW()
        si.cb = ctypes.sizeof(si)
        si.lpAttributeList = ctypes.cast(attrs, ctypes.c_void_p)
        # Don't let the program inherit our (redirected) standard handles
        si.dwFlags = 0x100    # STARTF_USESTDHANDLES
        pi = PROCESS_INFORMATION()
        env_block = ctypes.create_unicode_buffer(
            ''.join('%s=%s\0' % kv for kv in env.items()) + '\0')
        EXTENDED_STARTUPINFO_PRESENT = 0x00080000
        CREATE_UNICODE_ENVIRONMENT = 0x00000400
        cmdline = ctypes.create_unicode_buffer(subprocess.list2cmdline(argv))
        check(k32.CreateProcessW(None, cmdline, None, None, False,
                                 EXTENDED_STARTUPINFO_PRESENT
                                 | CREATE_UNICODE_ENVIRONMENT,
                                 env_block, cwd, ctypes.byref(si),
                                 ctypes.byref(pi)), 'CreateProcessW')
        self.process = pi.hProcess
        k32.CloseHandle(pi.hThread)
        # The pseudo-console has its own copies of these
        k32.CloseHandle(in_read)
        k32.CloseHandle(out_write)

        self.decoder = codecs.getincrementaldecoder('utf-8')('replace')
        self.output = queue.Queue()
        threading.Thread(target=self._reader, daemon=True).start()
        threading.Thread(target=self._closer, daemon=True).start()

    def _reader(self):
        ctypes, wintypes = self._ctypes, self._ctypes.wintypes
        buf = ctypes.create_string_buffer(65536)
        nread = wintypes.DWORD()
        while self._k32.ReadFile(self.out_read, buf, len(buf),
                                 ctypes.byref(nread), None) and nread.value:
            self.output.put(self.decoder.decode(buf.raw[:nread.value]))
        self.output.put(None)

    def _closer(self):
        """Close the pseudo-console once the program exits, which ends its
        output"""
        self._k32.WaitForSingleObject(self.process, 0xffffffff)
        self._k32.ClosePseudoConsole(self.hpc)

    def read(self, timeout):
        try:
            data = self.output.get(timeout=timeout)
        except queue.Empty:
            return ''
        if data is None:
            self.output.put(None)    # so later reads see EOF too
            raise EOFError
        return data

    def send(self, keys):
        ctypes = self._ctypes
        data = keys.encode('utf-8')
        written = ctypes.wintypes.DWORD()
        self._k32.WriteFile(self.in_write, data, len(data),
                            ctypes.byref(written), None)

    def exit_status(self):
        ctypes = self._ctypes
        self._k32.WaitForSingleObject(self.process, 0xffffffff)
        code = ctypes.wintypes.DWORD()
        self._k32.GetExitCodeProcess(self.process, ctypes.byref(code))
        return code.value

    def isalive(self):
        WAIT_TIMEOUT = 0x102
        return self._k32.WaitForSingleObject(self.process, 0) == WAIT_TIMEOUT

    def kill(self):
        self._k32.TerminateProcess(self.process, 1)


class Terminal(object):

    def __init__(self, argv, env, cwd, timeout=10):
        self.timeout = timeout
        self.screen = pyte.Screen(COLS, ROWS)
        if IS_WINDOWS:
            # Decoded from UTF-8 as it is read
            self.stream = pyte.Stream(self.screen)
            self.child = _WindowsProcess(argv, env, cwd)
        else:
            self.stream = pyte.ByteStream(self.screen)
            self.child = _UnixProcess(argv, env, cwd)
        self.exited = False

    # Output

    def _read(self, timeout):
        """Feed any output that arrives within `timeout` to the screen.
        Returns False once the program has exited."""
        if self.exited:
            return False
        try:
            self.stream.feed(self.child.read(timeout))
        except EOFError:
            self.exited = True
            return False
        return True

    @property
    def lines(self):
        return [line.rstrip() for line in self.screen.display]

    @property
    def text(self):
        return '\n'.join(self.lines)

    def __str__(self):
        return '\n'.join('|' + line for line in self.lines)

    def contains(self, pattern):
        """True if the screen contains `pattern` (a str, or a compiled
        regex)"""
        if hasattr(pattern, 'search'):
            return pattern.search(self.text) is not None
        return pattern in self.text

    def wait_for(self, *patterns, timeout=None):
        """Wait until the screen contains any of `patterns`, and return
        the first one found"""
        deadline = time.monotonic() + (timeout or self.timeout)
        while True:
            for p in patterns:
                if self.contains(p):
                    return p
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not self._read(min(remaining, 0.1)):
                if not self.exited or remaining <= 0:
                    raise TimeoutError("none of %r on screen:\n%s"
                                       % (patterns, self))
                raise EOFError("program exited without showing any of "
                               "%r; screen:\n%s" % (patterns, self))

    def settle(self, quiet=0.3):
        """Read output until there is none for `quiet` seconds"""
        last = time.monotonic()
        while time.monotonic() - last < quiet:
            before = self.text
            if not self._read(0.05):
                return
            if self.text != before:
                last = time.monotonic()

    # Input

    def send(self, keys):
        self.child.send(keys)

    def type(self, text):
        """Type `text` and press Enter"""
        self.child.send(text + '\r')

    # Exit

    def wait_exit(self, timeout=None):
        """Wait for the program to exit, and return its exit status. A
        program killed by a signal fails the test."""
        deadline = time.monotonic() + (timeout or self.timeout)
        while self._read(0.1):
            if time.monotonic() > deadline:
                raise TimeoutError("program did not exit; screen:\n%s"
                                   % self)
        try:
            return self.child.exit_status()
        except AssertionError as err:
            raise AssertionError("%s; screen:\n%s" % (err, self))

    def kill(self):
        """Kill the program if it is still running; returns True if it
        was"""
        if self.child.isalive():
            self.child.kill()
            return True
        return False
