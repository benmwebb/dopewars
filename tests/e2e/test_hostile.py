"""Malformed, malicious or badly-timed input must not crash or hang the
server, or affect other players. Each test finishes by checking that a
new player can still join, and the fixture checks that the server then
shuts down cleanly."""

import socket

import pytest

from dopeproto import *


def assert_server_ok(server):
    assert server.is_running()
    probe = server.player('Probe%d' % len(server.clients))
    assert probe.player is not None
    probe.close()


GARBAGE = [
    b'',
    b'^',
    b'^^',
    b'^^^^^^^^^^^^^^^^^^^^',
    b'x',
    b'AA',
    b'^A',
    b'\x00\x00\x00',
    b'\xff\xfe\xfd invalid UTF-8',
    b'^^' + b'\xc3' * 100,
    b'99999999999999999999^AEhello',
    b'-1^AEhello',
    b'^ZZ',
    b'^A\x7f',
    b'%s%s%s%n%n%n',
]


@pytest.mark.parametrize('line', GARBAGE, ids=repr)
@pytest.mark.parametrize('joined', [False, True], ids=['before', 'after'])
def test_garbage(server, line, joined):
    c = server.client()
    if joined:
        c.join('Alice')
    c.send_raw(line)
    c.send_raw(line + b'\r')
    assert_server_ok(server)


@pytest.mark.parametrize('code,data', [
    (C_BUYOBJECT, 'drug^0^99999999999999999999'),
    (C_BUYOBJECT, 'drug^0^-99999999999999999999'),
    (C_BUYOBJECT, 'drug^99999999999^1'),
    (C_BUYOBJECT, 'bitch^0^2147483647'),
    (C_BUYOBJECT, ''),
    (C_BUYOBJECT, '^^^^'),
    (C_PAYLOAN, '-1000000'),
    (C_PAYLOAN, '99999999999999999999999'),
    (C_DEPOSIT, '-99999999999999999999999'),
    (C_REQUESTJET, ''),
    (C_FIGHTACT, 'F'),
    (C_FIGHTACT, 'R'),
    (C_FIGHTACT, ''),
    (C_ANSWER, 'Y'),
    (C_ANSWER, ''),
    (C_DONE, ''),
    (C_SPYON, ''),
    (C_TIPOFF, ''),
    (C_SACKBITCH, ''),
    (C_CONTACTSPY, ''),
    (C_ABILITIES, ''),
    (C_ABILITIES, '1' * 10000),
    (C_TRADE, 'x'),
    (C_KILL, 'Alice'),
    (C_PUSH, ''),
    (C_QUIT, ''),
    (C_INIT, '1^-5^-5^-5'),
    (C_UPDATE, '1^2^3'),
], ids=lambda x: repr(x))
def test_bad_or_untimely_command(server, code, data):
    """Commands with silly data, or sent when they make no sense"""
    alice = server.player('Alice')
    start = alice.player
    alice.send(code, data)
    for m in alice.drain():
        if m.code == C_UPDATE and m.sender is None:
            p = alice.player
            # Nothing should have made us richer
            assert p.cash + p.bank - p.debt <= start.cash + start.bank \
                - start.debt
    assert_server_ok(server)


def test_long_name_truncated(server):
    alice = server.player('Alice')
    server.client().join('X' * 1000)
    name = alice.wait_for(C_JOIN).fields[0]
    assert 0 < len(name) < 1000
    assert_server_ok(server)


def test_spy_and_tipoff_on_self(server):
    alice = server.player('Alice')
    start = alice.player
    alice.send(C_SPYON, to=alice.id)
    alice.send(C_TIPOFF, to=alice.id)
    alice.send(C_SPYON, to=12345)
    assert not [m for m in alice.drain() if m.code == C_UPDATE]
    assert alice.player.cash == start.cash
    assert_server_ok(server)


def test_huge_line(server):
    """A line too long for the server's buffer"""
    c = server.client()
    try:
        c.sock.sendall(b'^^A' + b'x' * (4 * 1024 * 1024) + b'\n')
    except (BrokenPipeError, ConnectionResetError):
        pass    # The server may well drop us part way through
    assert_server_ok(server)


def test_many_messages_at_once(server):
    alice = server.player('Alice')
    alice.sock.sendall(b''.join(
        b'^AE' + (b'spam %d' % i) + b'\n' for i in range(5000)))
    assert_server_ok(server)


def test_partial_line_then_disconnect(server):
    c = server.client()
    c.sock.sendall(b'^^Ac')        # start of a C_NAME, with no newline
    c.close()
    assert_server_ok(server)


def test_connect_and_disconnect(server):
    for _ in range(20):
        socket.create_connection(('127.0.0.1', server.port)).close()
    assert_server_ok(server)


def test_idle_connection_doesnt_block_others(server):
    idle = server.client()          # never sends anything
    idle.sock.sendall(b'^^A')       # nor finishes a line
    alice = server.player('Alice')
    alice.send(C_MSG, 'hi')
    assert_server_ok(server)


def test_disconnect_mid_question(make_server):
    """Disconnect while the server is waiting for an answer"""
    server = make_server()
    alice = server.player('Alice')
    alice.jet_and_arrive(1)
    alice.answers[C_ASKLOAN] = None     # don't answer
    alice.jet(0)
    alice.wait_for(C_QUESTION)
    alice.close()
    assert_server_ok(server)


def test_many_players(make_server):
    server = make_server("MaxClients = 30\n")
    players = [server.player('P%d' % i) for i in range(25)]
    players[0].send(C_MSG, 'hello')
    for p in players[1:]:
        p.wait_for(C_MSG, match=lambda m: m.data == 'hello')
    for p in players:
        p.close()
    assert_server_ok(server)
