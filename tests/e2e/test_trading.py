"""Buying and selling, moving around, and the loan shark, bank and gun
shop. Locations are 0-based here; the default config puts the loan shark
and bank at location 0 and the gun shop and pub at location 1."""

import os

import pytest

from dopeproto import *


def cheapest_drug(client):
    """Index of the cheapest drug on sale here"""
    on_sale = [(p, i) for i, p in enumerate(client.prices) if p > 0]
    return min(on_sale)[1]


def test_buy_and_sell_drug(server):
    alice = server.player('Alice')
    start = alice.player
    drug = cheapest_drug(alice)
    price = alice.prices[drug]
    amount = min(start.cash // price, start.coat_size, 10)
    assert amount > 0

    alice.buy('drug', drug, amount)
    p = alice.update()
    assert p.drugs[drug] == amount
    assert p.cash == start.cash - amount * price
    assert p.coat_size == start.coat_size - amount
    assert p.drug_values[drug] == amount * price

    alice.buy('drug', drug, -amount)
    p = alice.update()
    assert p.drugs[drug] == 0
    assert p.cash == start.cash
    assert p.coat_size == start.coat_size


@pytest.mark.parametrize('order', [
    'drug^{unpriced}^1',          # not on sale here
    'drug^{cheap}^1000000',       # can't afford, or no space
    'drug^{cheap}^-1',            # nothing to sell
    'drug^-1^1',                  # bad index
    'drug^99^1',                  # bad index
    'gun^0^1',                    # not at the gun shop
    'drug^{cheap}^0',             # nothing at all
])
def test_bad_purchase_ignored(server, order):
    alice = server.player('Alice')
    unpriced = alice.prices.index(0)
    alice.send(C_BUYOBJECT, order.format(unpriced=unpriced,
                                         cheap=cheapest_drug(alice)))
    assert not [m for m in alice.drain() if m.code == C_UPDATE]


def test_jet(server):
    alice = server.player('Alice')
    start = alice.player
    p = alice.jet_and_arrive(3)
    assert p.is_at == 3
    assert p.turn == start.turn + 1
    assert p.date == (2, 12, 1984)
    # Debt grows with interest
    assert p.debt > start.debt
    assert C_SUBWAYFLASH in [m.code for m in alice.received]


@pytest.mark.parametrize('location', ['0', '-1', '8', '99999', 'nowhere'])
def test_bad_jet_denied(server, location):
    # 0 is where we already are; the rest don't exist ("nowhere" is
    # read by atoi() as 0)
    alice = server.player('Alice')
    alice.send(C_REQUESTJET, location)
    msgs = alice.drain()
    assert not [m for m in msgs if m.code == C_SUBWAYFLASH]
    assert 'DENIED jet' in server.read_log()


def test_loan_shark(server):
    alice = server.player('Alice')
    alice.jet_and_arrive(1)
    alice.answers[C_ASKLOAN] = 'Y'
    alice.jet(0)
    alice.wait_for(C_LOANSHARK)
    before = alice.player
    alice.send(C_PAYLOAN, '1000')
    p = alice.update()
    assert p.debt == before.debt - 1000
    assert p.cash == before.cash - 1000

    # Can't pay more than we have
    alice.send(C_PAYLOAN, str(p.cash + 1))
    assert not [m for m in alice.drain() if m.code == C_UPDATE]

    # Leaving the loan shark carries on with the arrival
    alice.send(C_DONE)
    alice.wait_for(C_DRUGHERE)


def test_bank(server):
    alice = server.player('Alice')
    alice.jet_and_arrive(1)
    alice.answers[C_ASKBANK] = 'Y'
    alice.jet(0)
    alice.wait_for(C_BANK)
    before = alice.player
    alice.send(C_DEPOSIT, '500')
    p = alice.update()
    assert (p.bank, p.cash) == (500, before.cash - 500)

    alice.send(C_DEPOSIT, '-200')          # withdraw
    p = alice.update()
    assert (p.bank, p.cash) == (300, before.cash - 300)

    for bad in ('-301', str(p.cash + 1)):
        alice.send(C_DEPOSIT, bad)
    assert not [m for m in alice.drain() if m.code == C_UPDATE]

    alice.send(C_DONE)
    alice.wait_for(C_DRUGHERE)


def test_bank_only_when_visiting(server):
    alice = server.player('Alice')
    alice.send(C_DEPOSIT, '500')
    alice.send(C_PAYLOAN, '500')
    assert not [m for m in alice.drain() if m.code == C_UPDATE]


def test_gun_shop(make_server):
    # The gun shop is closed in sanitized mode
    server = make_server("StartCash = 100000\nSanitized = FALSE\n")
    alice = server.player('Alice')
    name, price, space, damage = alice.guns[0]
    alice.answers[C_ASKGUNSHOP] = 'Y'
    alice.jet(1)
    alice.wait_for(C_GUNSHOP)
    before = alice.player

    alice.buy('gun', 0, 1)
    p = alice.update()
    assert p.guns[0] == 1
    assert p.cash == before.cash - price
    assert p.coat_size == before.coat_size - space

    # We can only carry (bitches + 2) guns
    alice.buy('gun', 0, p.bitches + 2)
    assert not [m for m in alice.drain() if m.code == C_UPDATE]

    alice.buy('gun', 0, -1)
    p = alice.update()
    assert p.guns[0] == 0

    alice.send(C_DONE)
    alice.wait_for(C_DRUGHERE)


def test_game_ends_after_num_turns(make_server):
    server = make_server("NumTurns = 3\n")
    alice = server.player('Alice')
    alice.jet_and_arrive(1)
    alice.jet_and_arrive(2)
    alice.jet(3)
    msg = alice.wait_for(C_PRINTMESSAGE,
                         match=lambda m: 'dealing time is up' in m.data)
    alice.wait_for(C_ENDHISCORE)
    with open(server.score_file, 'rb') as fh:
        assert b'Alice' in fh.read()


def test_high_scores(server):
    alice = server.player('Alice')
    alice.send(C_REQUESTSCORE)
    alice.wait_for(C_STARTHISCORE)
    alice.wait_for(C_ENDHISCORE)


def test_two_players_trade_independently(server):
    alice = server.player('Alice')
    bob = server.player('Bob')
    drug = cheapest_drug(alice)
    alice.buy('drug', drug, 1)
    assert alice.update().drugs[drug] == 1
    bob.jet_and_arrive(5)
    assert bob.player.drugs[drug] == 0
    assert bob.player.is_at == 5
    assert alice.player.is_at == 0
