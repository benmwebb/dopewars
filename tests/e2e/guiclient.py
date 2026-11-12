"""Drive the dopewars GUI client ("dopewars -w") through its test hook.

The client must be built with --enable-test-hooks. It then connects to
the port named by DOPEWARS_TEST_CTL, and takes commands from there (see
src/gtkport/testhook.c): it can describe every window on screen, and
click buttons, fill in entries, pick list rows and notebook tabs, and
pick menu items. This works the same with GTK+2, GTK+3, GTK4 and the
native Win32 client.

Widgets are found by the name given them in the client (e.g. "here", the
list of drugs for sale) or, for buttons, by their label. Either can be
preceded by "TITLE|" to look only in the window with that title.
"""

import json
import os
import socket
import subprocess
import time

from conftest import IS_WINDOWS, dopewars_binary, dopewars_env

_has_hooks = None

# Buttons that close a dialog (the first of them, in a question)
DIALOG_BUTTONS = ('OK', 'Yes', 'Close')


def has_test_hooks():
    """True if the dopewars binary was built with --enable-test-hooks"""
    global _has_hooks
    if _has_hooks is None:
        with open(dopewars_binary(), 'rb') as fh:
            _has_hooks = b'DOPEWARS_TEST_CTL' in fh.read()
    return _has_hooks


def walk(nodes):
    """Every widget in `nodes` (from GUIClient.dump()), and inside them"""
    for node in nodes:
        yield node
        yield from walk(node.get('children', []))


class Screen(object):
    """What was on screen, from GUIClient.dump()"""

    def __init__(self, windows):
        self.windows = windows

    def window(self, title):
        """The window with the given title, or None"""
        for w in self.windows:
            if w.get('text') == title:
                return w
        return None

    @property
    def titles(self):
        return [w.get('text') for w in self.windows]

    def widget(self, name):
        """The widget with the given name, or None"""
        for node in walk(self.windows):
            if node.get('name') == name:
                return node
        return None

    def rows(self, name):
        """The rows of the named list, each a list of strings"""
        return self.widget(name)['rows']

    def texts(self):
        """All text on screen, including list rows"""
        for node in walk(self.windows):
            if node.get('text'):
                yield node['text']
            for row in node.get('rows', []):
                yield from row

    def contains(self, text):
        return any(text in t for t in self.texts())

    def __str__(self):
        return json.dumps(self.windows, indent=1)


class GUIClient(object):

    def __init__(self, workdir, port=None, name='Alice', args=(),
                 config='', timeout=15):
        workdir = str(workdir)
        self.timeout = timeout
        config_file = os.path.join(workdir, 'client.cfg')
        with open(config_file, 'w') as fh:
            fh.write("MetaServer.Active = FALSE\n" + config)
        argv = [dopewars_binary(), '-w', '-g', config_file,
                '-f', os.path.join(workdir, 'client.sco')]
        if port is not None:
            argv += ['-o', '127.0.0.1', '-p', str(port)]
        if name:
            argv += ['-P', name]
        argv += list(args)

        self.listener = socket.socket()
        self.listener.bind(('127.0.0.1', 0))
        self.listener.listen(1)
        env = dopewars_env(workdir)
        env['DOPEWARS_TEST_CTL'] = str(self.listener.getsockname()[1])
        # Software rendering (for Xvfb), and no accessibility bus
        env['GSK_RENDERER'] = 'cairo'
        env['NO_AT_BRIDGE'] = '1'
        env['GTK_A11Y'] = 'none'
        self.output_file = os.path.join(workdir, 'gui.out')
        with open(self.output_file, 'w') as out:
            self.proc = subprocess.Popen(
                argv, env=env, cwd=workdir, stdin=subprocess.DEVNULL,
                stdout=out, stderr=subprocess.STDOUT)
        self.sock = None
        self._accept()

    def _accept(self):
        deadline = time.monotonic() + self.timeout
        self.listener.settimeout(0.2)
        while self.sock is None:
            try:
                self.sock, _ = self.listener.accept()
            except socket.timeout:
                if self.proc.poll() is not None:
                    raise EOFError("GUI client exited with status %d before "
                                   "connecting to the test:\n%s"
                                   % (self.proc.returncode, self.output))
                if time.monotonic() > deadline:
                    raise TimeoutError("GUI client never connected to the "
                                       "test:\n%s" % self.output)
        self.listener.close()
        self.sock.settimeout(self.timeout)
        self.reader = self.sock.makefile('r', encoding='utf-8',
                                         errors='replace', newline='\n')

    @property
    def output(self):
        """What the client printed (GTK+ warnings, for example)"""
        with open(self.output_file, errors='replace') as fh:
            return fh.read()

    # Commands

    def command(self, *args):
        """Send a command, and return the reply"""
        line = '\t'.join(args) + '\n'
        try:
            self.sock.sendall(line.encode('utf-8'))
            reply = self.reader.readline()
        except OSError as err:
            raise EOFError("lost the GUI client (%s); status %r:\n%s"
                           % (err, self.proc.poll(), self.output))
        if not reply:
            raise EOFError("GUI client closed the connection; status %r:\n%s"
                           % (self.proc.poll(), self.output))
        return reply.rstrip('\n')

    def dump(self):
        """Everything on screen, as a Screen"""
        return Screen(json.loads(self.command('dump')))

    def _retry(self, *args, timeout=None):
        """Run a command, retrying until it works (e.g. the widget it acts
        on appears) or the timeout"""
        deadline = time.monotonic() + (timeout or self.timeout)
        while True:
            reply = self.command(*args)
            if reply == 'ok':
                return
            if time.monotonic() > deadline:
                raise AssertionError("%s: %s; screen:\n%s"
                                     % (' '.join(args), reply, self.dump()))
            time.sleep(0.05)

    def click(self, selector, timeout=None):
        self._retry('click', selector, timeout=timeout)

    def set_text(self, name, text, timeout=None):
        self._retry('set-text', name, text, timeout=timeout)

    def select(self, name, row, timeout=None):
        """Select the row containing `row` in a list or combo box, or the
        tab `row` of a notebook"""
        self._retry('select', name, row, timeout=timeout)

    def menu(self, path, timeout=None):
        """Pick a menu item, e.g. "Game/Quit..." """
        self._retry('menu', '<main>/' + path, timeout=timeout)

    def close(self, title, timeout=None):
        """Close a window, as its close button would"""
        self._retry('close', title, timeout=timeout)

    # Waiting

    def wait_for(self, condition, timeout=None):
        """Wait until `condition` (text that should be on screen, or a
        function of the Screen) holds, and return the Screen"""
        if isinstance(condition, str):
            text = condition
            condition = lambda screen: screen.contains(text)
        deadline = time.monotonic() + (timeout or self.timeout)
        while True:
            screen = self.dump()
            if condition(screen):
                return screen
            if time.monotonic() > deadline:
                raise TimeoutError("timed out waiting; screen:\n%s" % screen)
            time.sleep(0.05)

    def wait_for_window(self, title, timeout=None):
        return self.wait_for(lambda s: s.window(title), timeout)

    def dismiss_until(self, condition, timeout=None):
        """Close message boxes and answer questions (with the first
        button) until `condition` holds"""
        if isinstance(condition, str):
            text = condition
            condition = lambda screen: screen.contains(text)
        deadline = time.monotonic() + (timeout or self.timeout)
        while True:
            screen = self.dump()
            if condition(screen):
                return screen
            button = self._dialog_button(screen)
            if button:
                self.command('click', button)
            if time.monotonic() > deadline:
                raise TimeoutError("timed out dismissing dialogs; screen:\n%s"
                                   % screen)
            time.sleep(0.05)

    @staticmethod
    def _dialog_button(screen):
        """A button to press to get rid of a dialog (as "TITLE|LABEL"), if
        there is one. Questions get their first answer."""
        for window in screen.windows:
            title = window.get('text') or ''
            if title in ('dopewars', 'New Game'):
                continue
            buttons = [n['text'] for n in walk([window])
                       if n['type'] in ('GtkButton', 'button', 'Button')
                       and n.get('text') and not n.get('insensitive')]
            for b in buttons:
                if b in DIALOG_BUTTONS:
                    return title + '|' + b
            if title == 'Question' and buttons:
                return title + '|' + buttons[0]
        return None

    # Exit

    def wait_exit(self, timeout=None):
        """Wait for the client to exit, and check that it exited normally"""
        status = self.proc.wait(timeout=timeout or self.timeout)
        assert status == 0, ("GUI client exited with status %d:\n%s"
                             % (status, self.output))

    def quit(self, timeout=None):
        """Quit from the menu (confirming if a game is in progress), and
        check that the client exits normally"""
        self.menu('Game/Quit...')
        deadline = time.monotonic() + (timeout or self.timeout)
        while self.proc.poll() is None:
            try:
                screen = self.dump()
            except EOFError:
                break
            if screen.contains('Abandon current game?'):
                self.command('click', 'Yes')
            if time.monotonic() > deadline:
                raise TimeoutError("GUI client did not quit:\n%s" % screen)
            time.sleep(0.05)
        self.wait_exit(timeout)

    def kill(self):
        """Kill the client if it is still running; returns True if it was"""
        if self.proc.poll() is None:
            self.proc.kill()
            self.proc.wait()
            return True
        return False
