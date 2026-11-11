/************************************************************************
 * test-message.c Unit tests for network message parsing                *
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

#include <string.h>

#include "common.h"
#include "message.h"

static void test_get_next_word(void)
{
  gchar buf[] = "one^two^^three";
  gchar *pt = buf;

  g_assert_cmpstr(GetNextWord(&pt, "def"), ==, "one");
  g_assert_cmpstr(GetNextWord(&pt, "def"), ==, "two");
  g_assert_cmpstr(GetNextWord(&pt, "def"), ==, "");
  g_assert_cmpstr(GetNextWord(&pt, "def"), ==, "three");
  /* End of data returns the default */
  g_assert_cmpstr(GetNextWord(&pt, "def"), ==, "def");
  g_assert_null(GetNextWord(&pt, NULL));

  pt = NULL;
  g_assert_cmpstr(GetNextWord(&pt, "def"), ==, "def");
}

static void test_get_next_int(void)
{
  gchar buf[] = "42^-7^junk";
  gchar *pt = buf;

  g_assert_cmpint(GetNextInt(&pt, 99), ==, 42);
  g_assert_cmpint(GetNextInt(&pt, 99), ==, -7);
  g_assert_cmpint(GetNextInt(&pt, 99), ==, 0);
  g_assert_cmpint(GetNextInt(&pt, 99), ==, 99);
}

static void test_get_next_price(void)
{
  gchar buf[] = "1234567890123^-5";
  gchar *pt = buf;

  g_assert_cmpint(GetNextPrice(&pt, 1), ==, (price_t)1234567890123LL);
  g_assert_cmpint(GetNextPrice(&pt, 1), ==, -5);
  g_assert_cmpint(GetNextPrice(&pt, 1), ==, 1);
}

static void test_process_message(void)
{
  Player *play, *other;
  GSList *list = NULL;
  AICode ai;
  MsgCode code;
  gchar *data;
  gchar msg1[] = "0^AAhello";
  gchar *msg2 = g_strdup_printf("^%c%c", C_NONE, C_JOIN);
  gchar msg3[] = "0^A";
  gchar msg4[] = "57^AAhello";

  play = g_new0(Player, 1);
  list = AddPlayer(0, play, list);
  play->ID = 0;

  g_assert_cmpint(ProcessMessage(msg1, play, &other, &ai, &code, &data,
                                 list), ==, 0);
  g_assert_true(other == play);
  g_assert_cmpint(ai, ==, C_NONE);
  g_assert_cmpint(code, ==, C_PRINTMESSAGE);
  g_assert_cmpstr(data, ==, "hello");

  /* A missing ID means the message is from nobody in particular */
  g_assert_cmpint(ProcessMessage(msg2, play, &other, &ai, &code, &data,
                                 list), ==, 0);
  g_assert_true(other == &Noone);
  g_assert_cmpint(code, ==, C_JOIN);
  g_assert_cmpstr(data, ==, "");

  /* Too short to contain an AI code and a message code */
  g_assert_cmpint(ProcessMessage(msg3, play, &other, &ai, &code, &data,
                                 list), ==, -1);

  /* Unknown player ID */
  g_assert_cmpint(ProcessMessage(msg4, play, &other, &ai, &code, &data,
                                 list), ==, -1);

  /* No player list */
  g_assert_cmpint(ProcessMessage(msg1, play, &other, &ai, &code, &data,
                                 NULL), ==, -1);

  g_free(msg2);
  list = RemovePlayer(play, list);
}

/*
 * C_INIT from a hostile server must not resize the game data to zero or
 * negative sizes, or to sizes large enough to exhaust memory.
 */
static void test_initial_data_bounds(void)
{
  Player *client;
  gchar small[] = "1.6^-5^-5^-5^bitch^bitches^gun^guns^drug^drugs^date^0";
  gchar zero[] = "1.6^0^0^0^bitch^bitches^gun^guns^drug^drugs^date^0";
  gchar *huge = g_strdup("1.6^99999999^99999999^99999999^bitch^bitches"
                         "^gun^guns^drug^drugs^date^0");

  client = g_new0(Player, 1);
  FirstClient = AddPlayer(0, client, FirstClient);

  ReceiveInitialData(client, small);
  g_assert_cmpint(NumLocation, ==, 1);
  g_assert_cmpint(NumGun, ==, 0);
  g_assert_cmpint(NumDrug, ==, 1);

  ReceiveInitialData(client, zero);
  g_assert_cmpint(NumLocation, ==, 1);
  g_assert_cmpint(NumGun, ==, 0);
  g_assert_cmpint(NumDrug, ==, 1);

  ReceiveInitialData(client, huge);
  g_assert_cmpint(NumLocation, ==, MAX_NET_ITEMS);
  g_assert_cmpint(NumGun, ==, MAX_NET_ITEMS);
  g_assert_cmpint(NumDrug, ==, MAX_NET_ITEMS);

  /* The existing player's inventory must have grown to match, with the
   * new entries zeroed */
  g_assert_cmpint(client->NumDrugsAllocated, ==, MAX_NET_ITEMS);
  g_assert_cmpint(client->NumGunsAllocated, ==, MAX_NET_ITEMS);
  g_assert_cmpint(client->Drugs[MAX_NET_ITEMS - 1].Carried, ==, 0);
  g_assert_cmpint(client->Guns[MAX_NET_ITEMS - 1].Carried, ==, 0);

  g_free(huge);
  FirstClient = RemovePlayer(client, FirstClient);
  dptest_reset_config();
}

static void test_initial_data_names(void)
{
  Player *client;
  gchar data[] = "1.6^8^4^12^hoe^hoes^piece^pieces^dope^dopes^%T"
                 "^3^Shark^Bank^Guns^Pub^1EUR";

  client = g_new0(Player, 1);
  FirstClient = AddPlayer(0, client, FirstClient);

  ReceiveInitialData(client, data);
  g_assert_cmpint(NumLocation, ==, 8);
  g_assert_cmpint(NumGun, ==, 4);
  g_assert_cmpint(NumDrug, ==, 12);
  g_assert_cmpstr(Names.Bitch, ==, "hoe");
  g_assert_cmpstr(Names.Guns, ==, "pieces");
  g_assert_cmpstr(Names.Drugs, ==, "dopes");
  g_assert_cmpint(client->ID, ==, 3);
  g_assert_cmpstr(Names.LoanSharkName, ==, "Shark");
  g_assert_cmpstr(Names.RoughPubName, ==, "Pub");
  g_assert_true(Currency.Prefix);
  g_assert_cmpstr(Currency.Symbol, ==, "EUR");

  FirstClient = RemovePlayer(client, FirstClient);
  dptest_reset_config();
}

/*
 * C_UPDATE with an out-of-range location or an invalid date must be
 * ignored rather than stored.
 */
static void test_player_data_bounds(void)
{
  Player *client;
  gchar badloc[] = "100^0^0^100^100^9999^5^0^31^2^1985";
  gchar negloc[] = "100^0^0^100^100^-1^5^0^1^1^1985";

  client = g_new0(Player, 1);
  FirstClient = AddPlayer(0, client, FirstClient);

  ReceivePlayerData(client, badloc, client);
  g_assert_cmpint(client->IsAt, ==, 0);
  /* 31st February is not a date; the default start date is kept */
  g_assert_true(g_date_valid(client->date));
  g_assert_cmpint(g_date_get_month(client->date), !=, 2);

  ReceivePlayerData(client, negloc, client);
  g_assert_cmpint(client->IsAt, ==, 0);
  g_assert_cmpint(g_date_get_day(client->date), ==, 1);
  g_assert_cmpint(g_date_get_month(client->date), ==, 1);
  g_assert_cmpint(g_date_get_year(client->date), ==, 1985);

  FirstClient = RemovePlayer(client, FirstClient);
}

/*
 * Player data sent by the server should be read back unchanged by the
 * client.
 */
static void test_player_data_round_trip(void)
{
  Player *play, *client;
  const gchar *data;
  gchar *copy;

  play = dptest_new_server_player();
  play->Cash = 123456;
  play->Debt = 5500;
  play->Bank = 42;
  play->Health = 77;
  play->CoatSize = 60;
  play->IsAt = 3;
  play->Turn = 9;
  play->Drugs[2].Carried = 15;
  play->Guns[1].Carried = 2;
  play->Bitches.Carried = 4;

  dptest_clear_messages();
  SendPlayerData(play);
  data = dptest_find_message(C_UPDATE);
  g_assert_nonnull(data);

  client = (Player *)FirstClient->data;
  copy = g_strdup(data);
  ReceivePlayerData(client, copy, client);
  g_free(copy);

  g_assert_cmpint(client->Cash, ==, 123456);
  g_assert_cmpint(client->Debt, ==, 5500);
  g_assert_cmpint(client->Bank, ==, 42);
  g_assert_cmpint(client->Health, ==, 77);
  g_assert_cmpint(client->CoatSize, ==, 60);
  g_assert_cmpint(client->IsAt, ==, 3);
  g_assert_cmpint(client->Turn, ==, 9);
  g_assert_cmpint(client->Drugs[2].Carried, ==, 15);
  g_assert_cmpint(client->Guns[1].Carried, ==, 2);
  g_assert_cmpint(client->Bitches.Carried, ==, 4);

  dptest_remove_players();
}

static void test_drugs_here(void)
{
  Player *client;
  gchar data[] = "100^^300";
  int i;

  client = g_new0(Player, 1);
  FirstClient = AddPlayer(0, client, FirstClient);

  ReceiveDrugsHere(data, client);
  g_assert_cmpint(client->EventNum, ==, E_ARRIVE);
  g_assert_cmpint(client->Drugs[0].Price, ==, 100);
  g_assert_cmpint(client->Drugs[1].Price, ==, 0);
  g_assert_cmpint(client->Drugs[2].Price, ==, 300);
  /* Prices not sent default to zero (not for sale) */
  for (i = 3; i < NumDrug; i++) {
    g_assert_cmpint(client->Drugs[i].Price, ==, 0);
  }

  FirstClient = RemovePlayer(client, FirstClient);
}

int main(int argc, char *argv[])
{
  dptest_init(&argc, &argv);

  g_test_add_func("/message/get-next-word", test_get_next_word);
  g_test_add_func("/message/get-next-int", test_get_next_int);
  g_test_add_func("/message/get-next-price", test_get_next_price);
  g_test_add_func("/message/process-message", test_process_message);
  g_test_add_func("/message/initial-data/bounds", test_initial_data_bounds);
  g_test_add_func("/message/initial-data/names", test_initial_data_names);
  g_test_add_func("/message/player-data/bounds", test_player_data_bounds);
  g_test_add_func("/message/player-data/round-trip",
                  test_player_data_round_trip);
  g_test_add_func("/message/drugs-here", test_drugs_here);

  return g_test_run();
}
