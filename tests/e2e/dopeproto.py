"""A minimal client for the dopewars network protocol, for testing.

Messages are single lines terminated by '\\n'. Each message has a header
identifying the sender and/or recipient, a one-character "AI code", a
one-character message code, and data whose fields are separated by '^'.

Until both ends have agreed to use numeric player IDs (the A_PLAYERID
ability), the header is "from_name^to_name^"; afterwards it is "id^".

Message codes are read from the C headers, so they always match the
code under test.
"""

import os
import re
import select
import socket
import time

SRCDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      os.pardir, os.pardir, 'src')


def _parse_enum(text, name):
    """Return a dict of the members of the C enum typedef'd as `name`,
    with their values (as str for character constants, else int)."""
    m = re.search(r'typedef\s+enum\s*\{([^}]*)\}\s*' + name + r'\s*;', text)
    if not m:
        raise ValueError("enum %s not found" % name)
    body = re.sub(r'/\*.*?\*/', '', m.group(1), flags=re.S)
    values = {}
    current = -1
    for item in body.split(','):
        item = item.strip()
        if not item:
            continue
        if '=' in item:
            key, val = (s.strip() for s in item.split('=', 1))
            if val.startswith("'"):
                current = ord(val[1])
            else:
                current = int(val, 0)
        else:
            key = item
            current += 1
        values[key] = current
    return values


def _load_constants():
    with open(os.path.join(SRCDIR, 'message.h')) as fh:
        message_h = fh.read()
    with open(os.path.join(SRCDIR, 'dopewars.h')) as fh:
        dopewars_h = fh.read()
    consts = {}
    for enum in ('MsgCode', 'AICode', 'FightPoint'):
        consts.update({k: chr(v)
                       for k, v in _parse_enum(message_h, enum).items()})
    consts.update(_parse_enum(dopewars_h, 'AbilType'))
    consts.update(_parse_enum(dopewars_h, 'EventCode'))
    return consts


globals().update(_load_constants())

# Types of C_DATA message (#defines in message.h)
DT_LOCATION, DT_DRUG, DT_GUN, DT_PRICES = 'A', 'B', 'C', 'D'


class ProtocolError(Exception):
    pass


class Message(object):
    """A message received from the server"""

    def __init__(self, sender, ai, code, data):
        self.sender = sender     # player ID (int), name (str), or None
        self.ai = ai
        self.code = code
        self.data = data

    @property
    def fields(self):
        return self.data.split('^')

    def __repr__(self):
        return "Message(%r, %r, %r, %r)" % (self.sender, self.ai,
                                            self.code, self.data)


class PlayerData(object):
    """Contents of a C_UPDATE message (see SendPlayerData() in message.c)"""

    def __init__(self, data, num_guns, num_drugs, drug_value=True):
        f = data.split('^')
        f.reverse()

        def nxt(default=0):
            # Like atoi(): empty fields (e.g. zero prices) are 0
            return int(f.pop() or 0) if f else default

        self.cash, self.debt, self.bank = nxt(), nxt(), nxt()
        self.health, self.coat_size, self.is_at = nxt(100), nxt(), nxt()
        self.turn, self.flags = nxt(), nxt()
        self.date = (nxt(1), nxt(1), nxt(1980))
        self.guns = [nxt() for _ in range(num_guns)]
        self.drugs = [nxt() for _ in range(num_drugs)]
        if drug_value:
            self.drug_values = [nxt() for _ in range(num_drugs)]
        self.bitches = nxt()


class Client(object):
    """A connection to a dopewars server, acting as a player"""

    def __init__(self, port, host='127.0.0.1', timeout=10):
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.timeout = timeout
        self.buf = b''
        self.use_ids = False
        self.id = None
        self.num_guns = self.num_drugs = self.num_locations = 0
        self.player = None
        self.prices = []
        self.location_names = {}
        self.drug_names = {}
        self.guns = {}          # index -> (name, price, space, damage)
        self.received = []      # every message received, for debugging
        self.answers = {}       # AI code -> answer to send automatically

    def close(self):
        if self.sock:
            self.sock.close()
            self.sock = None

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    # Sending

    def send_raw(self, line):
        """Send a line (without terminator) exactly as given"""
        if isinstance(line, str):
            line = line.encode('utf-8')
        self.sock.sendall(line + b'\n')

    def send(self, code, data='', ai=None, to=''):
        """Send a message to the server (or to player `to`, which is an ID
        once IDs are in use and a name before then)"""
        if ai is None:
            ai = C_NONE
        if self.use_ids:
            header = '%s^' % to
        else:
            header = '^%s^' % to
        self.send_raw(header + ai + code + data)

    # Receiving

    def _read_line(self, deadline):
        while b'\n' not in self.buf:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            ready, _, _ = select.select([self.sock], [], [], remaining)
            if not ready:
                return None
            chunk = self.sock.recv(65536)
            if not chunk:
                raise EOFError("server closed the connection")
            self.buf += chunk
        line, self.buf = self.buf.split(b'\n', 1)
        return line.rstrip(b'\r').decode('utf-8', errors='replace')

    def _parse(self, line):
        if self.use_ids:
            sender, _, rest = line.partition('^')
            sender = int(sender) if sender else None
        else:
            sender, _, rest = line.partition('^')
            _, _, rest = rest.partition('^')
            sender = sender or None
        if len(rest) < 2:
            raise ProtocolError("message too short: %r" % line)
        return Message(sender, rest[0], rest[1], rest[2:])

    def _track(self, msg):
        """Update our view of the game from a server message"""
        if msg.code == C_ABILITIES:
            # The server supports IDs; we always claim to as well
            self.use_ids = msg.data[A_PLAYERID:A_PLAYERID + 1] == '1'
        elif msg.code == C_INIT:
            f = msg.fields
            self.num_locations = int(f[1])
            self.num_guns = int(f[2])
            self.num_drugs = int(f[3])
            self.id = int(f[11])
        elif msg.code == C_DATA:
            # "index^<type><name>^..." (see SendMiscData() in message.c)
            f = msg.fields
            if len(f) >= 2 and f[1]:
                index, kind, name = int(f[0]), f[1][0], f[1][1:]
                if kind == DT_LOCATION:
                    self.location_names[index] = name
                elif kind == DT_DRUG:
                    self.drug_names[index] = name
                elif kind == DT_GUN:
                    self.guns[index] = (name, int(f[2]), int(f[3]),
                                        int(f[4]))
        elif msg.code == C_UPDATE and msg.sender is None:
            self.player = PlayerData(msg.data, self.num_guns, self.num_drugs)
        elif msg.code == C_DRUGHERE:
            fields = msg.fields[:self.num_drugs]
            fields += [''] * (self.num_drugs - len(fields))
            self.prices = [int(p or 0) for p in fields]
        elif msg.code == C_QUESTION:
            answer = self.answers.get(msg.ai, 'N')
            if answer is not None:
                self.send(C_ANSWER, answer,
                          to='' if msg.sender is None else msg.sender)

    def receive(self, timeout=None):
        """Return the next message, or None on timeout"""
        deadline = time.monotonic() + (timeout or self.timeout)
        line = self._read_line(deadline)
        if line is None:
            return None
        msg = self._parse(line)
        self.received.append(msg)
        self._track(msg)
        return msg

    def wait_for(self, code, ai=None, timeout=None, match=None):
        """Read messages until one with the given code (and AI code, and
        for which match(msg) is true, if given) arrives, and return it"""
        deadline = time.monotonic() + (timeout or self.timeout)
        while True:
            msg = self.receive(max(deadline - time.monotonic(), 0.001))
            if msg is None:
                raise TimeoutError(
                    "no %r message; last received: %r"
                    % (code, self.received[-5:]))
            if (msg.code == code and (ai is None or msg.ai == ai)
                    and (match is None or match(msg))):
                return msg

    def drain(self, timeout=0.5):
        """Read and return all messages until none arrive for `timeout`"""
        msgs = []
        while True:
            msg = self.receive(timeout)
            if msg is None:
                return msgs
            msgs.append(msg)

    def expect_closed(self, timeout=None):
        """Wait for the server to close the connection"""
        deadline = time.monotonic() + (timeout or self.timeout)
        try:
            while time.monotonic() < deadline:
                self.receive(max(deadline - time.monotonic(), 0.001))
        except (EOFError, ConnectionResetError):
            return
        raise TimeoutError("server did not close the connection")

    # Game actions

    def join(self, name):
        """Join the game; returns once the player has arrived and been
        sent the drug prices"""
        self.send(C_ABILITIES, '1' * A_NUM)
        self.send(C_NAME, name)
        self.wait_for(C_INIT)
        self.wait_for(C_ENDLIST)
        self.wait_for(C_DRUGHERE)
        return self

    def update(self, timeout=None):
        """Wait for the next C_UPDATE about us and return our data"""
        self.wait_for(C_UPDATE, timeout=timeout,
                      match=lambda m: m.sender is None)
        return self.player

    def buy(self, kind, index, amount):
        self.send(C_BUYOBJECT, '%s^%d^%d' % (kind, index, amount))

    def jet(self, location):
        self.send(C_REQUESTJET, str(location))

    def jet_and_arrive(self, location):
        """Jet to a location, and wait until the drug prices there arrive
        (any questions on the way are answered from self.answers)"""
        self.jet(location)
        self.wait_for(C_DRUGHERE)
        return self.player
