"""Drive the dopewars curses client ("dopewars -t") through its screens.

Much of the client's output is pop-ups that wait for a key ("Press any
key...") or for an answer to a question ("[Yes/No]"), which can appear
whenever the server sends something. These stay on screen after they
have been answered, so the driver looks at where the cursor is: curses
leaves it just after whatever is currently waiting for input.
"""

import os
import time

from conftest import dopewars_binary, dopewars_env
from terminal import Terminal

PRESS_ANY_KEY = 'Press any key...'
STREET_PROMPT = 'Will you B>uy'


class CursesClient(Terminal):

    def __init__(self, workdir, port=None, name='Alice', args=(),
                 config='', timeout=10):
        workdir = str(workdir)
        config_file = os.path.join(workdir, 'client.cfg')
        with open(config_file, 'w') as fh:
            fh.write("MetaServer.Active = FALSE\n" + config)
        argv = [dopewars_binary(), '-t', '-g', config_file,
                '-f', os.path.join(workdir, 'client.sco')]
        self.port = port
        if port is not None:
            argv += ['-o', '127.0.0.1', '-p', str(port)]
        if name:
            argv += ['-P', name]
        Terminal.__init__(self, argv + list(args), dopewars_env(workdir),
                          workdir, timeout)

    # What is waiting for input

    def before_cursor(self):
        """The text on the cursor's line, up to the cursor"""
        cursor = self.screen.cursor
        return self.screen.display[cursor.y][:cursor.x].rstrip()

    def cursor_line(self):
        return self.screen.display[self.screen.cursor.y].rstrip()

    def waiting_for(self, *endings):
        """If the text before the cursor ends with one of `endings`,
        return it; otherwise None"""
        before = self.before_cursor()
        for e in endings:
            if before.endswith(e):
                return e
        return None

    def wait_for_prompt(self, *endings, timeout=None):
        """Wait until the client is waiting for input just after one of
        `endings`, and return that ending"""
        deadline = time.monotonic() + (timeout or self.timeout)
        while True:
            self.settle(0.15)
            found = self.waiting_for(*endings)
            if found:
                return found
            if self.exited:
                raise EOFError("client exited while waiting for %r; "
                               "screen:\n%s" % (endings, self))
            if time.monotonic() > deadline:
                raise TimeoutError("client not waiting for %r; cursor "
                                   "after %r; screen:\n%s"
                                   % (endings, self.before_cursor(), self))

    # Getting about

    def start(self):
        """Get past the introduction screen"""
        self.wait_for_prompt(PRESS_ANY_KEY)
        self.send(' ')

    def connect(self):
        """Pick "Connect" on the server menu, accepting the host and port
        given on the command line"""
        self.wait_for_prompt('or P>lay single-player ?')
        self.send('C')
        self.wait_for_prompt('Hostname: 127.0.0.1')
        self.send('\r')
        self.wait_for_prompt('Port: %d' % self.port)
        self.send('\r')

    def to_street(self, answers=None, timeout=None):
        """Dismiss pop-ups until the main "Will you B>uy..." prompt is
        waiting. Questions are answered from `answers`, a dict mapping
        text in the question to the key to press (by default, N)."""
        answers = answers or {}
        deadline = time.monotonic() + (timeout or self.timeout)
        while True:
            found = self.wait_for_prompt(
                PRESS_ANY_KEY, ']', 'Q>uit?',
                timeout=max(deadline - time.monotonic(), 0.1))
            if found == 'Q>uit?' and STREET_PROMPT in self.cursor_line():
                return
            elif found == PRESS_ANY_KEY:
                self.send(' ')
            elif found == ']':
                line = self.cursor_line()
                key = 'N'
                for text, answer in answers.items():
                    if text in line:
                        key = answer
                self.send(key)
            else:
                raise AssertionError("unexpected prompt %r; screen:\n%s"
                                     % (self.cursor_line(), self))
            # Wait for the key to be acted on, so the same prompt isn't
            # answered twice
            self.settle()
            if time.monotonic() > deadline:
                raise TimeoutError("never got to the main prompt; "
                                   "screen:\n%s" % self)

    def dismiss_until(self, text, timeout=None):
        """Dismiss pop-ups, answering any question with the first key it
        offers, until `text` is on screen"""
        deadline = time.monotonic() + (timeout or self.timeout)
        while text not in self.text:
            self.settle(0.15)
            if self.waiting_for(PRESS_ANY_KEY):
                self.send(' ')
            elif self.waiting_for(']'):
                # The list of keys may have wrapped onto several lines
                y = self.screen.cursor.y
                before = ''.join(self.screen.display[:y]) + self.before_cursor()
                self.send(before[before.rindex('[') + 1])
            if self.exited:
                raise EOFError("client exited before showing %r; "
                               "screen:\n%s" % (text, self))
            if time.monotonic() > deadline:
                raise TimeoutError("%r never shown; screen:\n%s"
                                   % (text, self))

    def join(self):
        """Start the client and join the game"""
        self.start()
        self.connect()
        self.to_street()

    def quit(self, timeout=None):
        """Quit from the main prompt, decline to play again, and check
        that the client exits normally"""
        self.to_street()
        self.send('Q')
        self.wait_for_prompt('Are you sure you want to quit? [Y/N]')
        self.send('Y')
        self.finish(timeout)

    def finish(self, timeout=None):
        """Get through the end of the game (the high scores, then "Play
        again?"), and check that the client exits normally"""
        while True:
            found = self.wait_for_prompt(PRESS_ANY_KEY, 'Play again? [Y/N]',
                                         timeout=timeout)
            if found == PRESS_ANY_KEY:
                self.send(' ')
                self.settle()
            else:
                break
        self.send('N')
        status = self.wait_exit(timeout)
        assert status == 0, "client exited with status %r" % status
