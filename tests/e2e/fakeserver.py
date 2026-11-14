"""A scripted stand-in for the dopewars server, for testing clients.

A FakeServer listens on a free port and accepts one client. The test
then decides exactly what the client is sent, including malformed
messages that a real server would never send, and checks what the
client sends back.

FakeServer.join() sends the same sequence of messages that a real server
sends to a new player (see SendInitialData(), SendMiscData() and
SendPlayerData() in message.c), using the game described by a Game.
"""

import socket
import time

from dopeproto import *


class Game(object):
    """The game data a fake server sends. The names differ from the
    dopewars defaults, so tests can tell that the client is showing what
    the server sent rather than its own settings."""

    def __init__(self):
        self.locations = ['Testville', 'Mockton', 'Stubford', 'Fakeham']
        # (name, minimum price, maximum price)
        self.drugs = [('Fakeium', 100, 500), ('Mockaine', 1000, 5000),
                      ('Stubweed', 10, 50)]
        # (name, price, space, damage)
        self.guns = [('Pea Shooter', 500, 4, 2), ('Water Pistol', 300, 4, 1)]
        # Bitch, Bitches, Gun, Guns, Drug, Drugs, Date
        self.names = ['minion', 'minions', 'gun', 'guns', 'thing', 'things',
                      '%m-%d-%Y']
        # Loan shark, bank, gun shop, pub
        self.places = ['the Fake Shark', 'the Fake Bank', 'Fake Guns',
                       'the Fake Pub']
        self.currency = '1$'     # '1' = symbol before the amount
        self.spy_price, self.tipoff_price = 20000, 10000


class PlayerState(object):
    """The fields of a C_UPDATE message (see SendSpyReport())"""

    def __init__(self, game):
        self.cash, self.debt, self.bank = 2000, 5500, 0
        self.health, self.coat_size, self.is_at = 100, 100, 0
        self.turn, self.flags = 1, 0
        self.date = (1, 12, 1984)
        self.guns = [0] * len(game.guns)
        self.drugs = [0] * len(game.drugs)
        self.drug_values = [0] * len(game.drugs)
        self.bitches = 8

    def encode(self):
        fields = [self.cash, self.debt, self.bank, self.health,
                  self.coat_size, self.is_at, self.turn, self.flags]
        fields += list(self.date) + self.guns + self.drugs
        fields += self.drug_values + [self.bitches]
        return '^'.join(str(f) for f in fields)


class FakeServer(object):
    """A listening socket that plays the part of the server for one
    client"""

    def __init__(self, game=None, timeout=10):
        self.game = game or Game()
        self.player = PlayerState(self.game)
        self.timeout = timeout
        self.listener = socket.socket()
        self.listener.bind(('127.0.0.1', 0))
        self.listener.listen(1)
        self.port = self.listener.getsockname()[1]
        self.conn = None
        self.use_ids = False
        self.client_name = None
        self.client_abilities = None
        self.received = []      # every message received, for debugging

    def close(self):
        if self.conn:
            self.conn.close()
            self.conn = None
        if self.listener:
            self.listener.close()
            self.listener = None

    def accept(self):
        """Wait for the client to connect"""
        self.listener.settimeout(self.timeout)
        sock, _ = self.listener.accept()
        self.conn = LineConnection(sock, self.timeout)

    # Sending

    def send(self, code, data='', ai=None, sender=''):
        """Send a message, from the server (or from player ID `sender`)"""
        if ai is None:
            ai = C_NONE
        header = '%s^' % sender if self.use_ids else '%s^^' % sender
        self.conn.send_raw(header + ai + code + data)

    def send_raw(self, line):
        self.conn.send_raw(line)

    def send_oversized_line(self, length=100000):
        """Send a line longer than the client's read buffer (MAXREADBUF
        in network.c). The client drops the connection once its buffer is
        full, which may be before the whole line has been sent."""
        try:
            self.print_message('x' * length)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def print_message(self, text):
        self.send(C_PRINTMESSAGE, text)

    def send_update(self):
        """Send the client its player data, from self.player"""
        self.send(C_UPDATE, self.player.encode())

    def send_prices(self, prices=None):
        """Send the drug prices here (by default, every drug's minimum)"""
        if prices is None:
            prices = [d[1] for d in self.game.drugs]
        self.send(C_DRUGHERE, ''.join('%s^' % p for p in prices))

    # Receiving

    def _parse(self, line):
        # Clients send "to^" with IDs, or "from^to^" before then
        if self.use_ids:
            to, _, rest = line.partition('^')
        else:
            _, _, rest = line.partition('^')
            to, _, rest = rest.partition('^')
        if len(rest) < 2:
            raise ProtocolError("message too short: %r" % line)
        return Message(to or None, rest[0], rest[1], rest[2:])

    def receive(self, timeout=None):
        """Return the next message from the client, or None on timeout.
        (Message.sender is the player the message is addressed to.)"""
        deadline = time.monotonic() + (timeout or self.timeout)
        line = self.conn._read_line(deadline)
        if line is None:
            return None
        msg = self._parse(line)
        self.received.append(msg)
        return msg

    def wait_for(self, code, timeout=None):
        """Read messages until one with the given code arrives"""
        deadline = time.monotonic() + (timeout or self.timeout)
        while True:
            msg = self.receive(max(deadline - time.monotonic(), 0.001))
            if msg is None:
                raise TimeoutError("client sent no %r message; last "
                                   "received: %r"
                                   % (code, self.received[-5:]))
            if msg.code == code:
                return msg

    def expect_closed(self, timeout=None):
        """Wait for the client to close the connection"""
        deadline = time.monotonic() + (timeout or self.timeout)
        try:
            while time.monotonic() < deadline:
                self.receive(max(deadline - time.monotonic(), 0.001))
        except (EOFError, ConnectionResetError):
            return
        raise TimeoutError("client did not close the connection")

    # The start of a game

    def handshake(self):
        """Accept the client, and read its abilities and name. Returns
        the name."""
        self.accept()
        self.client_abilities = self.wait_for(C_ABILITIES).data
        self.client_name = self.wait_for(C_NAME).data
        # Agree on every ability; from now on both ends use player IDs
        self.send(C_ABILITIES, '1' * A_NUM)
        self.use_ids = True
        return self.client_name

    def send_init(self, player_id=1):
        g = self.game
        fields = ['fake-1.0', len(g.locations), len(g.guns), len(g.drugs)]
        fields += g.names + [player_id] + g.places + [g.currency]
        self.send(C_INIT, ''.join('%s^' % f for f in fields))

    def send_data(self):
        g = self.game
        self.send(C_DATA, '0^%s%d^%d^' % (DT_PRICES, g.spy_price,
                                          g.tipoff_price))
        for i, (name, price, space, damage) in enumerate(g.guns):
            self.send(C_DATA, '%d^%s%s^%d^%d^%d^'
                      % (i, DT_GUN, name, price, space, damage))
        for i, (name, low, high) in enumerate(g.drugs):
            self.send(C_DATA, '%d^%s%s^%d^%d^' % (i, DT_DRUG, name, low, high))
        for i, name in enumerate(g.locations):
            self.send(C_DATA, '%d^%s%s^' % (i, DT_LOCATION, name))

    def join(self, prices=None, others=()):
        """Accept the client and start a game, as a real server would.
        `others` is a list of (name, ID) for other players already in the
        game. Returns the client's name."""
        name = self.handshake()
        self.send_init()
        self.send_data()
        for other_name, other_id in others:
            self.send(C_LIST, '%s^%d' % (other_name, other_id))
        self.send(C_ENDLIST)
        self.send_update()
        self.send_prices(prices)
        return name

    def end_game(self):
        """Send an empty high score table, which ends the client's game"""
        self.send(C_STARTHISCORE)
        self.send(C_ENDHISCORE, 'end')


# Messages a malicious or broken server might send, to check that a
# client survives them. Each is (description, raw line), and is sent
# after the client has joined (so with player IDs in use).
HOSTILE_MESSAGES = [
    ('empty line', ''),
    ('header only', '^'),
    ('no message code', '^' + C_NONE),
    ('unknown code', '^' + C_NONE + '~data'),
    ('unknown sender', '9999^' + C_NONE + C_MSG + 'hello'),
    ('negative sender', '-1^' + C_NONE + C_MSG + 'hello'),
    ('huge sender', '99999999999999999999^' + C_NONE + C_MSG + 'hello'),
    ('init with huge counts',
     '^' + C_NONE + C_INIT + 'x^999999999^999999999^999999999^'),
    ('init with negative counts', '^' + C_NONE + C_INIT + 'x^-5^-5^-5^'),
    ('init with no fields', '^' + C_NONE + C_INIT),
    ('data with negative index', '^' + C_NONE + C_DATA + '-1^BEvil^1^2^'),
    ('data with huge index',
     '^' + C_NONE + C_DATA + '99999999999^ANowhere^'),
    ('data with no type', '^' + C_NONE + C_DATA + '0^^'),
    ('data with huge prices',
     '^' + C_NONE + C_DATA + '0^BFakeium^%s^-%s^' % ('9' * 40, '9' * 40)),
    ('update with bad location',
     '^' + C_NONE + C_UPDATE + '1^1^1^100^100^999999^1^0^1^12^1984^'),
    ('update with bad date',
     '^' + C_NONE + C_UPDATE + '1^1^1^100^100^0^1^0^99^99^-5^'),
    ('update with junk', '^' + C_NONE + C_UPDATE + '1^2^3^100^'
     + 'x^y^z^' * 50),
    ('too many prices', '^' + C_NONE + C_DRUGHERE + '1^' * 1000),
    ('negative prices', '^' + C_NONE + C_DRUGHERE + '-100^-1^-99999^'),
    ('player list entry with huge ID',
     '^' + C_NONE + C_LIST + 'Mallory^99999999999'),
    ('join with no name', '^' + C_NONE + C_JOIN),
    ('leave from unknown player', '12345^' + C_NONE + C_LEAVE + 'Ghost'),
    ('high score with bad index',
     '^' + C_NONE + C_HISCORE + '99999^B^someone^1^'),
    ('high score with negative index',
     '^' + C_NONE + C_HISCORE + '-1^B^someone^1^'),
    ('format string in message', '^' + C_NONE + C_PRINTMESSAGE
     + '%s%s%s%n%n%n'),
    ('invalid UTF-8', '^' + C_NONE + C_MSG + '\udcff\udcfe'),
    # (Just under MAXREADBUF; longer lines drop the connection)
    ('long message', '^' + C_NONE + C_PRINTMESSAGE + 'x' * 30000),
    ('fight message with junk', '^' + C_NONE + C_FIGHTPRINT
     + '^^^^^99999^-1^x'),
    ('spy report from unknown player', '777^' + C_NONE + C_UPDATE + '1^2^3'),
    ('abilities resent', '^' + C_NONE + C_ABILITIES + '1' * 1000),
    # The first field of a question is the keys that answer it
    ('question with no answers', '^' + C_NONE + C_QUESTION + '^Pick one'),
    ('question with too many answers', '^' + C_NONE + C_QUESTION
     + 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789^Pick one'),
    ('question with non-ASCII answers', '^' + C_NONE + C_QUESTION
     + 'ÄÖ^Pick one'),
]
