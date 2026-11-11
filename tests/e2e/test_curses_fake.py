"""The curses client against a fake server (see fakeserver.py): checks
that it shows what the server sends, sends the right messages back, and
survives messages that a real server would never send. Unix only (see
terminal.py)."""

import pytest

from dopeproto import *
from fakeserver import HOSTILE_MESSAGES


@pytest.fixture
def client(fake_server, curses_client):
    """A curses client that has joined the fake server's game, with
    Fakeium and Mockaine (but not Stubweed) for sale"""
    c = curses_client(fake_server.port)
    c.start()
    c.connect()
    # The client is now connecting; the fake server accepts it
    fake_server.join(prices=[123, 4567, 0])
    c.to_street()
    return c


def end_game(fake_server, client):
    fake_server.end_game()
    client.finish()


def test_shows_server_data(fake_server, client):
    assert fake_server.client_name == 'Alice'
    assert 'Testville' in client.lines[0]
    assert 'Hey dude, the prices of things here are:' in client.text
    assert 'A. Fakeium' in client.text and '$123' in client.text
    assert 'B. Mockaine' in client.text and '$4,567' in client.text
    assert 'Stubweed' not in client.text
    # The name for bitches comes from the server too
    assert 'Minions' in client.lines[0] or 'minions' in client.lines[0]
    end_game(fake_server, client)


def test_buy(fake_server, client):
    client.send('B')
    client.wait_for_prompt('What do you wish to buy?')
    client.send('B')
    client.wait_for_prompt('How many do you buy?')
    client.send('2\r')
    assert fake_server.wait_for(C_BUYOBJECT).data == 'drug^1^2'
    end_game(fake_server, client)


def test_jet(fake_server, client):
    client.send('J')
    client.wait_for_prompt('Where to, dude ?')
    assert '4. Fakeham' in client.text
    client.send('3')
    assert fake_server.wait_for(C_REQUESTJET).data == '2'
    end_game(fake_server, client)


def test_question(fake_server, client):
    fake_server.send(C_QUESTION, 'YN^Do you like tests?')
    client.wait_for_prompt('[Yes/No]')
    assert 'Do you like tests?' in client.cursor_line()
    client.send('Y')
    assert fake_server.wait_for(C_ANSWER).data == 'Y'
    end_game(fake_server, client)


def test_messages(fake_server, client):
    fake_server.send(C_JOIN, 'Bob^2')
    client.wait_for('Bob joins the game!')
    fake_server.send(C_MSG, 'hi Alice', sender=2)
    client.wait_for('Bob: hi Alice')
    fake_server.send(C_LEAVE, 'Bob', sender=2)
    client.wait_for('Bob has left the game.')
    fake_server.print_message('A message from the server')
    client.wait_for('A message from the server')
    end_game(fake_server, client)


@pytest.mark.parametrize('code,text', [
    (C_QUIT, 'The server has terminated'),
    (C_PUSH, 'You have been pushed from the server'),
])
def test_server_ends_connection(fake_server, client, code, text):
    """The game carries on in single-player mode"""
    fake_server.send(code)
    client.wait_for(text)
    client.to_street()
    assert 'T>alk' not in client.cursor_line()
    client.quit()


def test_connection_lost(fake_server, client):
    fake_server.close()
    client.wait_for('Connection to server lost!')
    client.to_street()
    assert 'T>alk' not in client.cursor_line()
    client.quit()


def test_oversized_line(fake_server, client):
    """A line longer than the client's read buffer drops the connection"""
    fake_server.send_oversized_line()
    client.wait_for('Connection to server lost!')
    client.to_street()
    client.quit()


@pytest.mark.parametrize('line', [m[1] for m in HOSTILE_MESSAGES],
                         ids=[m[0] for m in HOSTILE_MESSAGES])
def test_hostile_message(fake_server, client, line):
    """The client survives the message, and still responds"""
    fake_server.send_raw(line)
    fake_server.print_message('Still here')
    client.dismiss_until('Still here')
    end_game(fake_server, client)
