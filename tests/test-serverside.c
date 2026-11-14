/************************************************************************
 * test-serverside.c  Unit tests for server game logic                  *
 * Copyright (C)  2026  Ben Webb                                        *
 *                Email: benwebb@users.sf.net                           *
 *                WWW: https://dopewars.sourceforge.io/                 *
 *                                                                      *
 * This program is free software; you can redistribute it and/or        *
 * modify it under the terms of the GNU General Public License          *
 * as published by the Free Software Foundation; either version 2       *
 * of the License, or (at your option) any later version.               *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, write to the Free Software          *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,               *
 *                   MA  02111-1307, USA.                               *
 ************************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "message.h"
#include "serverside.h"

/* Runs BuyObject with a message built from a format string */
static void Buy(Player *play, const gchar *fmt, ...) G_GNUC_PRINTF(2, 3);

static void Buy(Player *play, const gchar *fmt, ...)
{
  va_list ap;
  gchar *data;

  va_start(ap, fmt);
  data = g_strdup_vprintf(fmt, ap);
  va_end(ap);
  BuyObject(play, data);
  g_free(data);
}

static Player *NewBuyer(void)
{
  Player *play = dptest_new_server_player();

  /* No cops, so that dropping drugs never starts a fight */
  Sanitized = TRUE;
  play->Cash = 10000;
  play->CoatSize = 100;
  play->Drugs[0].Price = 100;
  play->Guns[0].Price = 500;
  return play;
}

static void test_buy_drug(void)
{
  Player *play = NewBuyer();

  dptest_clear_messages();
  Buy(play, "drug^0^10");
  g_assert_cmpint(play->Drugs[0].Carried, ==, 10);
  g_assert_cmpint(play->CoatSize, ==, 90);
  g_assert_cmpint(play->Cash, ==, 9000);
  g_assert_cmpint(play->Drugs[0].TotalValue, ==, 1000);
  /* The client is told about the change */
  g_assert_nonnull(dptest_find_message(C_UPDATE));

  /* Selling half returns half the total value */
  Buy(play, "drug^0^-5");
  g_assert_cmpint(play->Drugs[0].Carried, ==, 5);
  g_assert_cmpint(play->CoatSize, ==, 95);
  g_assert_cmpint(play->Cash, ==, 9500);
  g_assert_cmpint(play->Drugs[0].TotalValue, ==, 500);

  dptest_remove_players();
}

static void test_buy_drug_rejected(void)
{
  Player *play = NewBuyer();

  dptest_clear_messages();
  Buy(play, "drug^0^0");                /* Nothing to buy */
  Buy(play, "drug^0^101");              /* Not enough space */
  Buy(play, "drug^0^-1");               /* Nothing to sell */
  Buy(play, "drug^0^200");              /* Not enough cash */
  Buy(play, "drug^-1^1");               /* Bad index */
  Buy(play, "drug^%d^1", NumDrug);      /* Bad index */
  Buy(play, "drug^1^1");                /* Not for sale here */
  Buy(play, "spaceship^0^1");           /* Unknown type */

  g_assert_cmpint(play->Drugs[0].Carried, ==, 0);
  g_assert_cmpint(play->CoatSize, ==, 100);
  g_assert_cmpint(play->Cash, ==, 10000);
  g_assert_cmpuint(dptest_messages->len, ==, 0);

  dptest_remove_players();
}

/*
 * Amounts large enough to overflow int arithmetic must be rejected, not
 * wrap around into a "cheap" purchase or a coat with negative space.
 */
static void test_buy_drug_overflow(void)
{
  Player *play = NewBuyer();

  play->Cash = G_MAXINT64 / 2;
  play->CoatSize = G_MAXINT;
  play->Drugs[0].Carried = G_MAXINT - 5;

  Buy(play, "drug^0^10");
  g_assert_cmpint(play->Drugs[0].Carried, ==, G_MAXINT - 5);
  g_assert_cmpint(play->CoatSize, ==, G_MAXINT);

  play->Drugs[0].Carried = 0;
  Buy(play, "drug^0^%d", G_MININT);
  g_assert_cmpint(play->Drugs[0].Carried, ==, 0);
  g_assert_cmpint(play->CoatSize, ==, G_MAXINT);

  /* amount * price wraps around to a negative number, which must not
   * make the drugs free (or pay the player to take them) */
  play->Cash = 1000;
  play->CoatSize = G_MAXINT;
  play->Drugs[0].Price = (price_t)1 << 33;
  Buy(play, "drug^0^%d", G_MAXINT);
  g_assert_cmpint(play->Drugs[0].Carried, ==, 0);
  g_assert_cmpint(play->Cash, ==, 1000);

  dptest_remove_players();
}

static void test_buy_gun(void)
{
  Player *play = NewBuyer();
  int space = Gun[0].Space;

  play->Bitches.Carried = 0;            /* So we can carry 2 guns */

  Buy(play, "gun^0^1");
  g_assert_cmpint(play->Guns[0].Carried, ==, 1);
  g_assert_cmpint(play->CoatSize, ==, 100 - space);
  g_assert_cmpint(play->Cash, ==, 9500);

  Buy(play, "gun^0^2");                 /* Too many guns */
  g_assert_cmpint(play->Guns[0].Carried, ==, 1);

  Buy(play, "gun^0^-1");
  g_assert_cmpint(play->Guns[0].Carried, ==, 0);
  g_assert_cmpint(play->CoatSize, ==, 100);

  Buy(play, "gun^0^%d", G_MININT);
  g_assert_cmpint(play->Guns[0].Carried, ==, 0);
  g_assert_cmpint(play->CoatSize, ==, 100);

  dptest_remove_players();
}

static void test_buy_bitch(void)
{
  Player *play = NewBuyer();
  int carried = play->Bitches.Carried;

  play->Bitches.Price = 2000;
  Buy(play, "bitch^0^2");               /* Only one at a time */
  g_assert_cmpint(play->Bitches.Carried, ==, carried);

  Buy(play, "bitch^0^1");
  g_assert_cmpint(play->Bitches.Carried, ==, carried + 1);
  g_assert_cmpint(play->Cash, ==, 8000);

  /* The offer is used up */
  Buy(play, "bitch^0^1");
  g_assert_cmpint(play->Bitches.Carried, ==, carried + 1);

  dptest_remove_players();
}

/*
 * RandomOffer must not index past the end of the drug list when the
 * server is configured with fewer drugs than the hard-coded WEED and
 * HASHISH indices.
 */
static void test_random_offer_few_drugs(void)
{
  Player *play;
  int i;

  Sanitized = FALSE;
  ResizeDrugs(1);
  play = dptest_new_server_player();
  play->Drugs[0].Carried = 10;
  play->CoatSize = 100;

  /* Seed once rather than on every call, since with some C libraries
   * the first random number after seeding barely depends on the seed */
  srand(12345);
  for (i = 0; i < 500; i++) {
    RandomOffer(play);
    g_assert_cmpint(play->Drugs[0].Carried, >=, 0);
  }

  dptest_remove_players();
  dptest_reset_config();
}

/*
 * A fight message from a player with no guns of any damage must not
 * divide by zero when working out how well armed they are.
 */
static void test_fight_message_no_damage(void)
{
  Player *attacker, *defender;
  int i;

  for (i = 0; i < NumGun; i++) {
    Gun[i].Damage = 0;
  }
  attacker = dptest_new_server_player();
  defender = dptest_new_server_player();
  attacker->FightArray = g_ptr_array_new();
  g_ptr_array_add(attacker->FightArray, attacker);

  dptest_clear_messages();
  SendFightMessage(attacker, defender, 0, F_ARRIVED, 0, FALSE, NULL);
  g_assert_cmpuint(dptest_messages->len, >, 0);

  g_ptr_array_free(attacker->FightArray, TRUE);
  attacker->FightArray = NULL;
  dptest_remove_players();
  dptest_reset_config();
}

int main(int argc, char *argv[])
{
  dptest_init(&argc, &argv);

  g_test_add_func("/serverside/buy/drug", test_buy_drug);
  g_test_add_func("/serverside/buy/drug-rejected", test_buy_drug_rejected);
  g_test_add_func("/serverside/buy/drug-overflow", test_buy_drug_overflow);
  g_test_add_func("/serverside/buy/gun", test_buy_gun);
  g_test_add_func("/serverside/buy/bitch", test_buy_bitch);
  g_test_add_func("/serverside/random-offer/few-drugs",
                  test_random_offer_few_drugs);
  g_test_add_func("/serverside/fight-message/no-damage",
                  test_fight_message_no_damage);

  return g_test_run();
}
