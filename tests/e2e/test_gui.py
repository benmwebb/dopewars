"""The GUI client ("dopewars -w"), driven through its test hook (see
guiclient.py), playing single-player and against a real server. Needs a
build with --enable-test-hooks, and a display."""

from conftest import free_port
from dopeproto import *


def stat(screen, name):
    """A value in the main window's stats panel ("cash", "location"...)"""
    return screen.widget(name)['text']


def money(text):
    return int(text.lstrip('$').replace(',', ''))


def in_game(screen):
    return screen.widget('here') and screen.rows('here')


def start_single_player(c):
    c.click('New Game...')
    c.select('game-type', 'Single player')
    c.click('Start single-player game')
    return c.wait_for(in_game)


def connect(c):
    """Connect to the server given on the command line"""
    c.click('New Game...')
    screen = c.wait_for_window('New Game')
    assert screen.widget('hostname')['text'] == '127.0.0.1'
    c.click('connect')


def carrying(screen, drug):
    return any(row[0] == drug for row in screen.rows('carried'))


def buy(c, drug, amount):
    c.select('here', drug)
    c.click('Buy ->')
    c.wait_for_window('Buy')
    c.set_text('amount', str(amount))
    c.click('OK')
    return c.wait_for(lambda s: not s.window('Buy') and carrying(s, drug))


def jet(c, location):
    c.click('Jet!')
    c.wait_for_window('Jet to location')
    c.click(location)


def test_start_and_quit(gui_client):
    c = gui_client(free_port())
    screen = c.wait_for_window('dopewars')
    assert screen.contains('New Game...')
    assert not screen.widget('here')
    c.quit()


def test_connection_refused(gui_client):
    c = gui_client(free_port())    # nothing is listening
    connect(c)
    c.wait_for('Status: Could not connect (Connection refused)')
    c.close('New Game')
    c.wait_for(lambda s: s.titles == ['dopewars'])
    c.quit()


def test_single_player(gui_client):
    c = gui_client(None, config='Sanitized = TRUE\n')
    screen = start_single_player(c)
    assert stat(screen, 'cash') == '$2,000'
    assert stat(screen, 'debt') == '$5,500'
    assert stat(screen, 'location') == 'Bronx'
    assert stat(screen, 'date') == '12-01-1984'
    assert screen.rows('carried') == []

    # Buy 3 of the cheapest drug
    drug, price = min(screen.rows('here'), key=lambda row: money(row[1]))
    screen = buy(c, drug, 3)
    assert screen.rows('carried') == [[drug, '3 @ %s' % price]]
    assert money(stat(screen, 'cash')) == 2000 - 3 * money(price)
    assert stat(screen, 'space') == '97'

    jet(c, '2. Ghetto')
    screen = c.dismiss_until(lambda s: stat(s, 'location') == 'Ghetto'
                             and s.titles == ['dopewars'])
    assert stat(screen, 'date') == '12-02-1984'
    c.quit()


def test_whole_game(gui_client):
    c = gui_client(None, config='Sanitized = TRUE\nNumTurns = 2\n')
    start_single_player(c)
    jet(c, '2. Ghetto')
    c.dismiss_until(lambda s: stat(s, 'location') == 'Ghetto'
                    and s.titles == ['dopewars'])
    jet(c, '3. Central Park')
    screen = c.wait_for_window('High Scores')
    assert screen.contains('Alice')
    c.click('Close')
    # Back to the start, ready for a new game
    c.wait_for(lambda s: s.titles == ['dopewars']
               and s.contains('New Game...'))
    c.quit()


def test_join(server, gui_client):
    watcher = server.player('Watcher')
    c = gui_client(server.port)
    connect(c)
    screen = c.wait_for(in_game)
    assert stat(screen, 'cash') == '$2,000'
    assert watcher.wait_for(C_JOIN).fields[0] == 'Alice'
    c.quit()
    assert watcher.wait_for(C_LEAVE).data == 'Alice'


def test_name_taken(server, gui_client):
    server.player('Alice')
    watcher = server.player('Watcher')
    c = gui_client(server.port)
    connect(c)
    c.wait_for_window('Change Name')
    c.set_text('name', 'Bob')
    c.click('OK')
    c.wait_for(in_game)
    assert watcher.wait_for(C_JOIN).fields[0] == 'Bob'
    c.quit()


def test_talk(server, gui_client):
    watcher = server.player('Watcher')
    c = gui_client(server.port)
    connect(c)
    c.wait_for(in_game)
    c.menu('Talk/To All...')
    c.set_text('message', 'hello from the GUI')
    c.click('Send')
    msg = watcher.wait_for(C_MSG)
    assert msg.data == 'hello from the GUI'
    c.click('Talk to player(s)|Close')
    watcher.send(C_MSG, 'hello yourself')
    c.wait_for('Watcher: hello yourself')
    c.quit()


def test_trade_with_server(server, gui_client):
    c = gui_client(server.port)
    connect(c)
    screen = c.wait_for(in_game)
    drug, price = screen.rows('here')[0]
    screen = buy(c, drug, 1)
    assert screen.rows('carried') == [[drug, '1 @ %s' % price]]
    c.select('carried', drug)
    c.click('<- Sell')
    c.wait_for_window('Sell')
    c.click('OK')
    c.wait_for(lambda s: not s.window('Sell') and s.rows('carried') == [])
    c.quit()


def test_server_terminates(make_server, gui_client):
    server = make_server()
    c = gui_client(server.port)
    connect(c)
    c.wait_for(in_game)
    server.stop()
    # The game carries on in single-player mode
    c.dismiss_until('The server has terminated')
    c.quit()
