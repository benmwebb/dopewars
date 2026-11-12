"""Server administration with "dopewars -A", which talks to the server
over a Unix domain socket (so is not available on Windows)"""

import select
import subprocess
import sys
import time

import pytest

from dopeproto import *

pytestmark = pytest.mark.skipif(sys.platform == 'win32',
                                reason="admin mode needs Unix sockets")


class Admin(object):
    def __init__(self, server):
        self.proc = subprocess.Popen(
            server.args('-A'), env=server.env, cwd=server.workdir,
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)
        self.output = b''
        self.read_until('ready for admin commands')

    def read_until(self, text, timeout=10):
        """Read output until `text` appears, and return everything read"""
        deadline = time.monotonic() + timeout
        text = text.encode()
        while text not in self.output:
            remaining = deadline - time.monotonic()
            ready, _, _ = select.select([self.proc.stdout], [], [],
                                        max(remaining, 0))
            if not ready:
                raise TimeoutError("%r not in admin output %r"
                                   % (text, self.output))
            chunk = self.proc.stdout.read1(65536)
            if not chunk:
                raise EOFError("admin exited; output %r" % self.output)
            self.output += chunk
        return self.output.decode()

    def command(self, cmd, expect):
        self.proc.stdin.write(cmd.encode() + b'\n')
        self.proc.stdin.flush()
        return self.read_until(expect)

    def close(self):
        self.proc.stdin.close()
        assert self.proc.wait(timeout=10) == 0
        self.proc.stdout.close()


@pytest.fixture
def admin(server):
    a = Admin(server)
    yield a
    a.close()


def test_list(server, admin):
    admin.command('list', 'No users currently logged on')
    server.player('Alice')
    server.player('Bob')
    out = admin.command('list', 'Bob')
    assert 'Alice' in out


def test_message(server, admin):
    alice = server.player('Alice')
    admin.command('msg:Server going down', '')
    assert alice.wait_for(C_MSG).data == 'Server going down'


def test_push(server, admin):
    alice = server.player('Alice')
    admin.command('push Alice', 'Pushing Alice')
    alice.wait_for(C_PUSH)
    admin.command('push Nobody', 'No such user')


def test_kill(server, admin):
    alice = server.player('Alice')
    bob = server.player('Bob')
    alice.wait_for(C_JOIN)
    admin.command('kill Bob', 'Bob killed')
    alice.wait_for(C_KILL)
    assert alice.wait_for(C_LEAVE).data == 'Bob'


def test_help_and_unknown(admin):
    admin.command('help', 'quit')
    admin.command('frobnicate', 'Unknown command')


def test_set_config(server, admin):
    admin.command('StartCash = 54321', 'StartCash is')
    assert server.player('Alice').player.cash == 54321


def test_commands_sent_together(admin):
    """Several commands arriving at once (e.g. pasted) are all sent"""
    admin.command('help\nlist\nfrobnicate', 'Unknown command')
    assert 'No users currently logged on' in admin.output.decode()


def test_quit(server, admin):
    alice = server.player('Alice')
    admin.command('quit', '')
    alice.wait_for(C_QUIT)
    alice.close()
    server.proc.wait(timeout=10)
    assert server.proc.returncode == 0
