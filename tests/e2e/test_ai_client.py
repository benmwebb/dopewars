"""The AI client ("dopewars -c") against a fake server (see fakeserver.py),
including messages that a real server would never send. These run on
every platform."""

import os
import subprocess

import pytest

from conftest import IS_WINDOWS, dopewars_binary, dopewars_env
from dopeproto import *
from fakeserver import HOSTILE_MESSAGES


class AIClient(object):

    def __init__(self, workdir, port):
        config = os.path.join(str(workdir), 'ai.cfg')
        with open(config, 'w') as fh:
            fh.write("AITurnPause = 0\nMetaServer.Active = FALSE\n")
        self.output_file = os.path.join(str(workdir), 'ai.out')
        with open(self.output_file, 'w') as out:
            self.proc = subprocess.Popen(
                [dopewars_binary(), '-c', '-o', '127.0.0.1', '-p', str(port),
                 '-g', config], env=dopewars_env(workdir), cwd=str(workdir),
                stdin=subprocess.DEVNULL, stdout=out, stderr=subprocess.STDOUT)

    @property
    def output(self):
        with open(self.output_file, errors='replace') as fh:
            return fh.read()

    def wait_exit(self, timeout=10):
        """Wait for the client to exit, and check that it exited normally.
        Returns its output (which, on Windows, goes to its own console
        window instead, so is empty)."""
        status = self.proc.wait(timeout=timeout)
        assert status == 0, ("AI client exited with status %d:\n%s"
                             % (status, self.output))
        return self.output


@pytest.fixture
def ai_client(tmp_path, fake_server):
    """An AI client connecting to the fake server"""
    client = AIClient(tmp_path, fake_server.port)
    yield client
    if client.proc.poll() is None:
        client.proc.kill()
        client.proc.wait()
        pytest.fail("AI client still running at the end of the test:\n"
                    + client.output)


def start_turn(fake_server):
    """Send prices, and wait for the AI to finish dealing and jet off"""
    fake_server.send_prices()
    # (Any location; a hostile C_INIT may have changed how many there are)
    assert int(fake_server.wait_for(C_REQUESTJET).data) >= 0


def test_join(fake_server, ai_client):
    name = fake_server.join()
    assert name.startswith('AI) ')
    assert set(fake_server.client_abilities) <= {'0', '1'}
    jet = fake_server.wait_for(C_REQUESTJET)
    assert 0 <= int(jet.data) < len(fake_server.game.locations)
    fake_server.send(C_PUSH)
    output = ai_client.wait_exit()
    if not IS_WINDOWS:
        assert 'AI Player pushed from the server' in output


def test_game_over(fake_server, ai_client):
    fake_server.join()
    fake_server.wait_for(C_REQUESTJET)
    fake_server.end_game()
    output = ai_client.wait_exit()
    if not IS_WINDOWS:
        assert 'Game time is up' in output


def test_server_quits(fake_server, ai_client):
    fake_server.join()
    fake_server.send(C_QUIT)
    output = ai_client.wait_exit()
    if not IS_WINDOWS:
        assert 'The server has terminated' in output


def test_connection_lost(fake_server, ai_client):
    fake_server.join()
    fake_server.wait_for(C_REQUESTJET)
    fake_server.close()
    output = ai_client.wait_exit()
    if not IS_WINDOWS:
        assert 'Connection to server lost' in output


def test_oversized_line(fake_server, ai_client):
    """A line longer than the client's read buffer (MAXREADBUF) drops
    the connection, rather than being read"""
    fake_server.join()
    fake_server.wait_for(C_REQUESTJET)
    fake_server.send(C_PRINTMESSAGE, 'x' * 100000)
    output = ai_client.wait_exit()
    if not IS_WINDOWS:
        assert 'Connection to server lost' in output


def test_name_taken(fake_server, ai_client):
    fake_server.handshake()
    # The AI picks another name when told its name is taken
    fake_server.send(C_NEWNAME)
    assert fake_server.wait_for(C_NAME).data.startswith('AI) ')
    fake_server.send(C_PUSH)
    ai_client.wait_exit()


def test_killed(fake_server, ai_client):
    fake_server.join()
    fake_server.wait_for(C_REQUESTJET)
    fake_server.player.health = 0
    fake_server.send_update()
    output = ai_client.wait_exit()
    if not IS_WINDOWS:
        assert 'AI Player killed' in output


@pytest.mark.parametrize('line', [m[1] for m in HOSTILE_MESSAGES],
                         ids=[m[0] for m in HOSTILE_MESSAGES])
def test_hostile_message(fake_server, ai_client, line):
    """The AI survives the message, and still plays"""
    fake_server.join()
    fake_server.wait_for(C_REQUESTJET)
    fake_server.send_raw(line)
    start_turn(fake_server)
    fake_server.send(C_PUSH)
    ai_client.wait_exit()
