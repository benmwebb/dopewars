"""The GUI client against a fake server (see fakeserver.py): checks that
it shows what the server sends, sends the right messages back, and
survives messages that a real server would never send. Needs a build
with --enable-test-hooks, and a display."""

import pytest

from dopeproto import *
from fakeserver import HOSTILE_MESSAGES
from test_gui import connect, in_game, stat


@pytest.fixture
def client(fake_server, gui_client):
    """A GUI client that has joined the fake server's game, with Fakeium
    and Mockaine (but not Stubweed) for sale"""
    c = gui_client(fake_server.port)
    connect(c)
    # The client is now connecting; the fake server accepts it
    fake_server.join(prices=[123, 4567, 0])
    c.wait_for(in_game)
    return c


def end_game(fake_server, client):
    """End the game, and quit once the high scores are shown"""
    fake_server.end_game()
    client.dismiss_until(lambda s: s.window('High Scores'))
    client.click('High Scores|Close')
    client.quit()


def test_shows_server_data(fake_server, client):
    assert fake_server.client_name == 'Alice'
    screen = client.wait_for(lambda s: len(s.rows('here')) == 2)
    assert screen.rows('here') == [['Fakeium', '$123'],
                                   ['Mockaine', '$4,567']]
    assert stat(screen, 'location') == 'Testville'
    assert screen.contains('Minions')
    end_game(fake_server, client)


def test_buy(fake_server, client):
    client.select('here', 'Fakeium')
    client.click('Buy ->')
    screen = client.wait_for_window('Buy')
    assert screen.contains('You can afford 16')    # $2,000 / $123
    client.set_text('amount', '2')
    client.click('OK')
    assert fake_server.wait_for(C_BUYOBJECT).data == 'drug^0^2'
    end_game(fake_server, client)


def test_jet(fake_server, client):
    client.click('Jet!')
    screen = client.wait_for_window('Jet to location')
    assert screen.contains('4. Fakeham')
    client.click('3. Stubford')
    assert fake_server.wait_for(C_REQUESTJET).data == '2'
    end_game(fake_server, client)


def test_question(fake_server, client):
    fake_server.send(C_QUESTION, 'YN^Do you like tests?')
    client.wait_for('Do you like tests?')
    client.click('Question|Yes')
    assert fake_server.wait_for(C_ANSWER).data == 'Y'
    end_game(fake_server, client)


@pytest.mark.parametrize('code,title,close', [
    (C_BANK, 'The Fake Bank', 'Cancel'),
    (C_LOANSHARK, 'The Fake Shark', 'Cancel'),
    (C_GUNSHOP, 'Fake Guns', 'Close'),
])
def test_dialog_only_once(fake_server, client, code, title, close):
    """A second bank, loan shark or gun shop message is ignored while
    that dialog is open"""
    fake_server.send(code)
    client.wait_for_window(title)
    fake_server.send(code)
    # Messages are handled in order, so once this is shown the second
    # message has been too
    fake_server.print_message('After the second message')
    screen = client.wait_for('After the second message')
    assert screen.titles.count(title) == 1
    client.click(title + '|' + close)
    fake_server.wait_for(C_DONE)
    # The dialog can be opened again once it is closed
    fake_server.send(code)
    client.wait_for_window(title)
    client.click(title + '|' + close)
    fake_server.wait_for(C_DONE)
    end_game(fake_server, client)


def test_killed_in_fight(fake_server, client):
    """Dying in a fight shows the high scores over the Fight dialog;
    closing them must leave the main window usable (issue #68: the
    Win32 client left it disabled)"""
    fight = 'Officer Hardass^^100^5^deputy^0^0^%s^%s'
    fake_server.send(C_FIGHTPRINT, fight % ('A001', 'Hardass arrives'))
    client.wait_for_window('Fight')
    fake_server.send(C_FIGHTPRINT, fight % ('H001', "You've been killed"))
    fake_server.end_game()
    client.wait_for_window('High Scores')
    client.click('High Scores|Close')
    screen = client.wait_for(lambda s: s.titles == ['dopewars']
                             and s.contains('New Game...'))
    assert not screen.window('dopewars').get('insensitive')
    client.quit()


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
    client.dismiss_until(text)
    client.quit()


def test_connection_lost(fake_server, client):
    fake_server.close()
    client.dismiss_until('Connection to server lost')
    client.quit()


def test_oversized_line(fake_server, client):
    """A line longer than the client's read buffer drops the connection"""
    fake_server.send_oversized_line()
    client.dismiss_until('Connection to server lost')
    client.quit()


@pytest.mark.parametrize('line', [m[1] for m in HOSTILE_MESSAGES],
                         ids=[m[0] for m in HOSTILE_MESSAGES])
def test_hostile_message(fake_server, client, line):
    """The client survives the message, and still responds"""
    fake_server.send_raw(line)
    fake_server.print_message('Still here')
    client.dismiss_until('Still here')
    end_game(fake_server, client)
