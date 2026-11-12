"""Joining and leaving the game, and talking to other players"""

import pytest

from dopeproto import *


def test_join(server):
    alice = server.player('Alice')
    assert alice.use_ids
    assert alice.num_locations == 8
    assert alice.num_drugs == 12
    assert alice.num_guns == 4
    p = alice.player
    assert (p.cash, p.debt, p.bank) == (2000, 5500, 0)
    assert p.is_at == 0 and p.turn == 1
    assert p.date == (1, 12, 1984)
    assert len(alice.prices) == 12
    assert any(alice.prices)


def test_start_cash_from_config(make_server):
    server = make_server("StartCash = 12345\nStartDebt = 0\n")
    p = server.player('Alice').player
    assert (p.cash, p.debt) == (12345, 0)


def test_second_player_sees_first(server):
    alice = server.player('Alice')
    bob = server.client()
    bob.send(C_ABILITIES, '1' * A_NUM)
    bob.send(C_NAME, 'Bob')
    lst = bob.wait_for(C_LIST)
    assert lst.fields[0] == 'Alice'
    bob.wait_for(C_ENDLIST)

    join = alice.wait_for(C_JOIN)
    assert join.fields[0] == 'Bob'


def test_duplicate_name(server):
    server.player('Alice')
    imposter = server.client()
    imposter.send(C_ABILITIES, '1' * A_NUM)
    imposter.send(C_NAME, 'Alice')
    imposter.wait_for(C_NEWNAME)
    # Picking another name works
    imposter.send(C_NAME, 'Mallory')
    imposter.wait_for(C_INIT)


def test_empty_name(server):
    c = server.client()
    c.send(C_ABILITIES, '1' * A_NUM)
    c.send(C_NAME, '')
    c.wait_for(C_NEWNAME)


def test_name_control_characters_replaced(server):
    alice = server.player('Alice')
    server.client().join('Bad\x01Name^x')
    join = alice.wait_for(C_JOIN)
    # '^' is a field separator, so StripTerminators replaces it
    assert join.fields[0] == 'Bad?Name~x'


def test_chat(server):
    alice = server.player('Alice')
    bob = server.player('Bob')
    carol = server.player('Carol')
    alice.drain()
    bob.drain()

    carol.send(C_MSG, 'hello everyone')
    for c in (alice, bob):
        msg = c.wait_for(C_MSG)
        assert msg.data == 'hello everyone'
        assert msg.sender == carol.id


def test_private_message(server):
    alice = server.player('Alice')
    bob = server.player('Bob')
    carol = server.player('Carol')
    alice.drain()
    bob.drain()

    carol.send(C_MSGTO, 'psst', to=bob.id)
    msg = bob.wait_for(C_MSGTO)
    assert msg.data == 'psst'
    # Nobody else sees it
    assert not [m for m in alice.drain() if m.code == C_MSGTO]


def test_leave(server):
    alice = server.player('Alice')
    bob = server.player('Bob')
    alice.wait_for(C_JOIN)
    bob.close()
    msg = alice.wait_for(C_LEAVE)
    assert msg.data == 'Bob'


def test_quit_game(server):
    alice = server.player('Alice')
    bob = server.player('Bob')
    alice.wait_for(C_JOIN)
    bob.send(C_WANTQUIT)
    bob.wait_for(C_ENDHISCORE)
    assert alice.wait_for(C_LEAVE).data == 'Bob'


def test_max_clients(make_server):
    server = make_server("MaxClients = 2\n")
    server.player('Alice')
    server.player('Bob')
    carol = server.client()
    carol.send(C_ABILITIES, '1' * A_NUM)
    carol.send(C_NAME, 'Carol')
    msg = carol.wait_for(C_PRINTMESSAGE)
    assert 'limit of 2 players' in msg.data
    # Carol is not in the game
    assert not [m for m in carol.drain() if m.code == C_INIT]


def test_commands_ignored_before_join(server):
    c = server.client()
    c.send(C_ABILITIES, '1' * A_NUM)
    # Not joined yet, so these must be ignored rather than acted on
    c.send(C_MSG, 'spam')
    c.send(C_REQUESTJET, '3')
    c.send(C_BUYOBJECT, 'drug^0^1')
    alice = server.player('Alice')
    assert not [m for m in alice.drain() if m.code == C_MSG]
    server.assert_logged('Ignoring pre-join command')
