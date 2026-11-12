"""pytest fixtures that run a real dopewars server for end-to-end tests.

The binary under test is src/dopewars in the source tree, or the path
in the DOPEWARS_BINARY environment variable.

The server is always shut down cleanly (SIGTERM on Unix, closing its
window on Windows) so that code coverage data is written. A server that
doesn't shut down cleanly fails the test.
"""

import os
import re
import signal
import socket
import subprocess
import sys
import time

import pytest

import dopeproto
from fakeserver import FakeServer

TOPDIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                      os.pardir, os.pardir))
IS_WINDOWS = sys.platform == 'win32'

# Settings used by every test server, on top of the game defaults
# (Sanitized mode stops cops attacking, and drug-related random events,
# so that games are predictable. Tests can override any of these.)
BASE_CONFIG = """
Daemonize = FALSE
MetaServer.Active = FALSE
Sanitized = TRUE
Log.Level = 4
"""


def _native_path(path):
    """Convert an MSYS2-style path (/c/foo) to a Windows one (c:/foo)"""
    m = re.match(r'^/([a-zA-Z])(/.*)?$', path)
    if IS_WINDOWS and m:
        return m.group(1) + ':' + (m.group(2) or '/')
    return path


def dopewars_binary():
    binary = os.environ.get('DOPEWARS_BINARY')
    if binary:
        binary = _native_path(binary)
    else:
        binary = os.path.join(TOPDIR, 'src', 'dopewars')
        if IS_WINDOWS:
            binary += '.exe'
    if not os.path.exists(binary):
        pytest.exit("dopewars binary not found at %s; build it first, or "
                    "set DOPEWARS_BINARY" % binary, returncode=2)
    return binary


def free_port():
    with socket.socket() as s:
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]


def dopewars_env(workdir):
    """Environment for running dopewars in directory `workdir`"""
    env = os.environ.copy()
    # Make the game repeatable, and keep the tester's config out of it
    env['DOPEWARS_SEED'] = '1'
    env['HOME'] = str(workdir)
    env['XDG_STATE_HOME'] = str(workdir)
    return env


def _close_windows(pid):
    """Ask every visible window of process `pid` to close (Windows only)"""
    import ctypes
    from ctypes import wintypes
    user32 = ctypes.windll.user32
    WM_CLOSE = 0x0010
    found = []

    @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    def callback(hwnd, lparam):
        window_pid = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(window_pid))
        if window_pid.value == pid and user32.IsWindowVisible(hwnd):
            found.append(hwnd)
        return True

    user32.EnumWindows(callback, 0)
    for hwnd in found:
        user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
    return len(found) > 0


class Server(object):
    """A dopewars server running in its own temporary directory"""

    def __init__(self, workdir, config=''):
        self.workdir = str(workdir)
        self.config = BASE_CONFIG + config
        self.port = None
        self.proc = None
        self.clients = []
        self.config_file = os.path.join(self.workdir, 'test.cfg')
        self.log_file = os.path.join(self.workdir, 'server.log')
        self.score_file = os.path.join(self.workdir, 'dopewars.sco')
        self.output_file = os.path.join(self.workdir, 'server.out')

    @property
    def env(self):
        return dopewars_env(self.workdir)

    def args(self, *extra):
        """Command line for running dopewars against this server"""
        return [dopewars_binary(), '-p', str(self.port), '-g',
                self.config_file] + list(extra)

    def start(self):
        self.port = free_port()
        with open(self.config_file, 'w') as fh:
            fh.write(self.config)
        self.output = open(self.output_file, 'w')
        self.proc = subprocess.Popen(
            self.args('-S', '-f', self.score_file, '-l', self.log_file),
            cwd=self.workdir, env=self.env, stdin=subprocess.DEVNULL,
            stdout=self.output, stderr=subprocess.STDOUT)
        deadline = time.monotonic() + 15
        while True:
            if self.proc.poll() is not None:
                raise RuntimeError("server exited with status %d:\n%s"
                                   % (self.proc.returncode, self.read_output()))
            try:
                socket.create_connection(('127.0.0.1', self.port),
                                         timeout=1).close()
                return
            except OSError:
                if time.monotonic() > deadline:
                    raise RuntimeError("server did not start listening")
                time.sleep(0.1)

    def client(self):
        """Return a new connection to this server (not yet joined)"""
        c = dopeproto.Client(self.port)
        self.clients.append(c)
        return c

    def player(self, name):
        """Return a new connection that has joined the game as `name`"""
        return self.client().join(name)

    def is_running(self):
        return self.proc.poll() is None

    def read_output(self):
        with open(self.output_file) as fh:
            return fh.read()

    def read_log(self):
        if not os.path.exists(self.log_file):
            return ''
        with open(self.log_file) as fh:
            return fh.read()

    def assert_logged(self, text):
        """Check that the server logged a message containing `text`. The
        Windows (GUI) server shows its log in its window rather than
        writing it to the log file, so this check is skipped there."""
        if not IS_WINDOWS:
            assert text in self.read_log()

    def stop(self):
        """Shut down the server cleanly, after disconnecting all clients
        (the server only exits once every player has gone)"""
        for c in self.clients:
            c.close()
        self.clients = []
        if self.proc.poll() is None:
            if IS_WINDOWS:
                # The server may not have created its window yet
                deadline = time.monotonic() + 10
                while (not _close_windows(self.proc.pid)
                       and time.monotonic() < deadline):
                    time.sleep(0.1)
            else:
                # Exactly one signal: a second one would kill the server
                # without a clean shutdown
                self.proc.send_signal(signal.SIGTERM)
            try:
                self.proc.wait(timeout=20)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()
                self.output.close()
                raise AssertionError("server did not shut down cleanly:\n"
                                     + self.read_output())
        self.output.close()
        if self.proc.returncode != 0:
            raise AssertionError("server exited with status %d:\n%s"
                                 % (self.proc.returncode, self.read_output()))


@pytest.fixture
def make_server(tmp_path):
    """Factory for servers; call with extra config file text"""
    servers = []

    def _make(config=''):
        workdir = tmp_path / ('server%d' % len(servers))
        workdir.mkdir()
        s = Server(workdir, config)
        servers.append(s)
        s.start()
        return s

    yield _make
    errors = []
    for s in servers:
        try:
            s.stop()
        except AssertionError as e:
            errors.append(str(e))
    if errors:
        pytest.fail('\n'.join(errors))


@pytest.fixture
def server(make_server):
    """A server with the default game settings"""
    return make_server()


@pytest.fixture
def fake_server():
    """A fake server (see fakeserver.py) for testing a client"""
    s = FakeServer()
    yield s
    s.close()


@pytest.fixture
def curses_client(request, tmp_path):
    """Factory for curses clients (see cursesclient.py), running in a
    pseudo-terminal. Call with the server's port (or None), and any
    CursesClient keyword arguments. Each test must quit its clients
    itself; a client still running at the end fails the test.

    On Windows, only tests marked "windows" run: a smoke test of the
    native console code, since the game logic is the same as on Unix."""
    if IS_WINDOWS:
        from guiclient import has_test_hooks
        if not request.node.get_closest_marker('windows'):
            pytest.skip("only the curses smoke tests run on Windows")
        if not has_test_hooks():
            pytest.skip("curses tests on Windows need a build with "
                        "--enable-test-hooks")
    else:
        pytest.importorskip('pexpect')
    pytest.importorskip('pyte')
    from cursesclient import CursesClient
    clients = []

    def _make(port=None, **kwargs):
        workdir = tmp_path / ('client%d' % len(clients))
        workdir.mkdir()
        c = CursesClient(workdir, port, **kwargs)
        clients.append(c)
        return c

    yield _make
    running = [c for c in clients if c.kill()]
    if running:
        pytest.fail("curses client still running at the end of the test; "
                    "screen:\n%s" % running[0])


@pytest.fixture
def gui_client(tmp_path):
    """Factory for GUI clients (see guiclient.py), driven through their
    test hook. Call with the server's port (or None), and any GUIClient
    keyword arguments. Each test must quit its clients itself; a client
    still running at the end fails the test."""
    from guiclient import GUIClient, has_test_hooks
    if not has_test_hooks():
        pytest.skip("GUI tests need a build with --enable-test-hooks")
    if (sys.platform.startswith('linux') and not os.environ.get('DISPLAY')
            and not os.environ.get('WAYLAND_DISPLAY')):
        pytest.skip("GUI tests need a display (try xvfb-run)")
    clients = []

    def _make(port=None, **kwargs):
        workdir = tmp_path / ('gui%d' % len(clients))
        workdir.mkdir()
        c = GUIClient(workdir, port, **kwargs)
        clients.append(c)
        return c

    yield _make
    running = [c for c in clients if c.kill()]
    if running:
        pytest.fail("GUI client still running at the end of the test:\n"
                    + running[0].output)
    # GTK+ criticals are usually bugs, such as using a destroyed widget
    for c in clients:
        if '-CRITICAL **' in c.output:
            pytest.fail("GUI client logged a critical error:\n" + c.output)
