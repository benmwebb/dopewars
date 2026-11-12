"""The curses client ("dopewars -t") playing against a real server,
checked through what it shows on screen. Unix only (see terminal.py)."""

import re
import signal

import pytest

from conftest import free_port
from dopeproto import *

PRESS_ANY_KEY = 'Press any key...'


def money(screen_text, label):
    """The amount shown next to `label` ("Cash", "Bank", "Debt") in the
    stats panel"""
    m = re.search(label + r'\s+\$([\d,]+)', screen_text)
    assert m, "no %s on screen:\n%s" % (label, screen_text)
    return int(m.group(1).replace(',', ''))


def test_intro_and_quit_from_menu(curses_client):
    c = curses_client(free_port())
    c.wait_for_prompt(PRESS_ANY_KEY)
    assert 'D O P E W A R S' in c.text
    assert 'Version' in c.text
    c.send(' ')
    c.wait_for_prompt('or P>lay single-player ?')
    c.send('Q')
    # Quitting from the server menu exits with status 1
    assert c.wait_exit() == 1


def test_connection_refused(curses_client):
    c = curses_client(free_port())    # nothing is listening
    c.start()
    c.connect()
    c.wait_for_prompt('or P>lay single-player ?')
    assert 'Could not start multiplayer dopewars' in c.text
    c.send('Q')
    assert c.wait_exit() == 1


@pytest.mark.windows
def test_single_player(curses_client):
    c = curses_client(None, args=['-n'])
    c.start()
    c.to_street()
    assert 'Hey dude, the prices of drugs here are:' in c.text
    assert 'Bronx' in c.text
    # No talking to other players in single-player mode
    assert 'T>alk' not in c.cursor_line()
    c.quit()


@pytest.mark.windows
def test_join(server, curses_client):
    watcher = server.player('Watcher')
    c = curses_client(server.port)
    c.join()
    assert 'Hey dude, the prices of drugs here are:' in c.text
    assert 'Bronx' in c.text
    assert money(c.text, 'Cash') == 2000
    assert money(c.text, 'Debt') == 5500
    assert 'T>alk, P>age' in c.cursor_line()
    assert watcher.wait_for(C_JOIN).fields[0] == 'Alice'
    c.quit()
    assert watcher.wait_for(C_LEAVE).data == 'Alice'


def test_name_prompt(server, curses_client):
    watcher = server.player('Watcher')
    c = curses_client(server.port, name=None)
    c.start()
    c.wait_for_prompt("Hey dude, what's your name?")
    c.send('Carol\r')
    c.connect()
    c.to_street()
    assert watcher.wait_for(C_JOIN).fields[0] == 'Carol'
    c.quit()


def test_name_taken(server, curses_client):
    server.player('Alice')
    c = curses_client(server.port, name='Alice')
    c.start()
    c.connect()
    c.wait_for_prompt('Name:', "What's your name?", 'name?', ':',
                      timeout=10)
    assert 'somebody else is already using "your" name' in c.text
    c.send('Alicia\r')
    c.to_street()
    c.quit()


def test_buy_and_sell(server, curses_client):
    c = curses_client(server.port)
    c.join()
    cash = money(c.text, 'Cash')
    # Ludes are cheap enough to afford with the starting cash
    m = re.search(r'([A-Z])\. Ludes\s+\$([\d,]+)', c.text)
    assert m, c.text
    key, price = m.group(1), int(m.group(2).replace(',', ''))

    c.send('B')
    c.wait_for_prompt('What do you wish to buy?')
    c.send(key)
    c.wait_for_prompt('How many do you buy?')
    assert 'You can afford' in c.text
    c.send('3\r')
    c.to_street()
    assert money(c.text, 'Cash') == cash - 3 * price
    assert re.search(r'Ludes\s+3', c.text), c.text
    assert 'S>ell' in c.cursor_line()

    c.send('S')
    c.wait_for_prompt('What do you wish to sell?')
    c.send(key)
    c.wait_for_prompt('How many do you sell?')
    assert 'You have 3.' in c.text
    c.send('3\r')
    c.to_street()
    assert money(c.text, 'Cash') == cash
    assert 'S>ell' not in c.cursor_line()
    c.quit()


def test_jet(server, curses_client):
    c = curses_client(server.port)
    c.join()
    c.send('J')
    c.wait_for_prompt('Where to, dude ?')
    assert '2. Ghetto' in c.text
    c.send('2')
    c.to_street()
    assert 'Ghetto' in c.lines[0]
    assert '12-02-1984' in c.lines[0]
    # A day's interest on the debt
    assert money(c.text, 'Debt') > 5500
    c.quit()


def test_loan_shark_and_bank(server, curses_client):
    c = curses_client(server.port)
    c.join()
    # The loan shark and bank are both in the Bronx, so go away and back
    c.send('J')
    c.wait_for_prompt('Where to, dude ?')
    c.send('2')
    c.to_street()
    c.send('J')
    c.wait_for_prompt('Where to, dude ?')
    c.send('1')

    c.wait_for_prompt('[Yes/No]')
    assert 'Loan Shark' in c.cursor_line()
    c.send('Y')
    c.wait_for_prompt('How much money do you pay back?')
    debt = money(c.text, 'Debt')
    c.send('500\r')

    c.wait_for_prompt('[Yes/No]')
    assert 'Bank' in c.cursor_line()
    assert money(c.text, 'Debt') == debt - 500
    c.send('Y')
    c.wait_for_prompt('L>eave ?')
    c.send('D')
    c.wait_for_prompt('How much money?')
    c.send('1000\r')
    # One transaction, then the bank visit is over
    c.to_street()
    assert money(c.text, 'Bank') == 1000
    c.quit()


def test_talk(server, curses_client):
    bob = server.player('Bob')
    c = curses_client(server.port)
    c.join()
    bob.wait_for(C_JOIN)

    c.send('T')
    c.wait_for_prompt('Talk:')
    c.send('hello Bob\r')
    msg = bob.wait_for(C_MSG)
    assert msg.data == 'hello Bob'

    bob.send(C_MSG, 'hi Alice')
    c.wait_for('Bob: hi Alice')
    c.to_street()
    c.quit()


def test_page(server, curses_client):
    bob = server.player('Bob')
    c = curses_client(server.port)
    c.join()
    c.send('P')
    c.wait_for_prompt('(talk privately to) ?')
    assert 'Bob' in c.text
    # Players are listed with letters, like drugs
    m = re.search(r'([A-Z])\. Bob', c.text)
    assert m, c.text
    c.send(m.group(1))
    c.wait_for_prompt('Talk:')
    c.send('psst\r')
    assert bob.wait_for(C_MSGTO).data == 'psst'
    c.to_street()
    c.quit()


def test_list_players(server, curses_client):
    server.player('Bob')
    c = curses_client(server.port)
    c.join()
    c.send('L')
    c.wait_for_prompt('P>layers or S>cores?')
    c.send('P')
    c.wait_for('Bob')
    c.to_street()
    c.quit()


def test_whole_game(make_server, curses_client):
    server = make_server("NumTurns = 3\n")
    c = curses_client(server.port)
    c.join()
    for dest in '2', '3':
        c.send('J')
        c.wait_for_prompt('Where to, dude ?')
        c.send(dest)
        c.to_street()
    c.send('J')
    c.wait_for_prompt('Where to, dude ?')
    c.send('4')
    c.wait_for_prompt(PRESS_ANY_KEY)
    assert 'Your dealing time is up' in c.text
    c.send(' ')
    # Then the high score table, where we have the only (negative) score
    c.wait_for_prompt('Play again? [Y/N]')
    assert 'H I G H   S C O R E S' in c.text
    assert re.search(r'\$-?[\d,]+ +\d\d-\d\d-\d{4} +Alice', c.text), c.text
    c.finish()


def test_server_terminates(server, curses_client):
    c = curses_client(server.port)
    c.join()
    server.proc.send_signal(signal.SIGTERM)
    c.wait_for('The server has terminated')
    c.wait_for_prompt(PRESS_ANY_KEY)
    c.send(' ')
    # The client leaves, so the server can finish shutting down
    assert server.proc.wait(timeout=20) == 0
    # The game carries on in single-player mode
    c.to_street()
    assert 'T>alk' not in c.cursor_line()
    c.quit()
