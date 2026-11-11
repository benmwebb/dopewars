"""Run a program in a pseudo-terminal and look at its screen, for testing
the curses client.

The program runs under pexpect, and everything it writes is fed through
pyte, a terminal emulator, so tests can check the rendered screen rather
than the raw escape sequences. The terminal is 80x25 with TERM=xterm, in
the C.UTF-8 locale.

This needs Unix pseudo-terminals, so isn't available on Windows.
"""

import time

import pexpect
import pyte

ROWS, COLS = 25, 80


class Terminal(object):

    def __init__(self, argv, env, cwd, timeout=10):
        # A UTF-8 locale, as most users have, so the client's UTF-8 code
        # paths are tested
        env = dict(env, TERM='xterm', LINES=str(ROWS), COLUMNS=str(COLS),
                   LC_ALL='C.UTF-8')
        self.timeout = timeout
        self.screen = pyte.Screen(COLS, ROWS)
        self.stream = pyte.ByteStream(self.screen)
        self.child = pexpect.spawn(argv[0], argv[1:], env=env, cwd=cwd,
                                   dimensions=(ROWS, COLS))
        self.exited = False

    # Output

    def _read(self, timeout):
        """Feed any output that arrives within `timeout` to the screen.
        Returns False once the program has exited."""
        if self.exited:
            return False
        try:
            self.stream.feed(self.child.read_nonblocking(65536, timeout))
        except pexpect.TIMEOUT:
            pass
        except pexpect.EOF:
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
        self.child.close()
        if self.child.signalstatus is not None:
            raise AssertionError("program killed by signal %d; screen:\n%s"
                                 % (self.child.signalstatus, self))
        return self.child.exitstatus

    def kill(self):
        """Kill the program if it is still running; returns True if it
        was"""
        if self.child.isalive():
            self.child.terminate(force=True)
            return True
        return False
