"""Whole games played against the server by the built-in AI player
("dopewars -c")"""

import subprocess

import pytest

from dopeproto import *


def run_ai(server, timeout=120):
    proc = subprocess.run(
        server.args('-c', '-o', '127.0.0.1'), env=server.env,
        cwd=server.workdir, stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    return proc.returncode, proc.stdout.decode('utf-8', errors='replace')


@pytest.mark.parametrize('sanitized', ['TRUE', 'FALSE'])
def test_ai_plays_full_game(make_server, sanitized):
    # Unsanitized games include fights with the cops
    server = make_server("NumTurns = 10\nAITurnPause = 0\n"
                         "Sanitized = %s\n" % sanitized)
    watcher = server.player('Watcher')
    status, output = run_ai(server)
    assert status == 0, output
    assert 'Game time is up' in output or 'AI Player killed' in output, \
        output

    # The watcher saw the AI arrive and leave
    join = watcher.wait_for(C_JOIN)
    assert watcher.wait_for(C_LEAVE).data == join.fields[0]


def test_two_ai_players(make_server):
    server = make_server("NumTurns = 10\nAITurnPause = 0\n"
                         "Sanitized = FALSE\n")
    procs = [subprocess.Popen(
        server.args('-c', '-o', '127.0.0.1'), env=server.env,
        cwd=server.workdir, stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for _ in range(2)]
    for p in procs:
        assert p.wait(timeout=120) == 0
