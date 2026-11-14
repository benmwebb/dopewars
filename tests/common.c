/************************************************************************
 * common.c       Shared setup code for the dopewars unit tests         *
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

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "message.h"
#include "serverside.h"

GPtrArray *dptest_messages = NULL;

/* Client-side players, one per server-side player */
static GSList *client_players = NULL;

/*
 * Stands in for the client: records every message the (local) server
 * sends, instead of acting on it.
 */
static void CaptureMessage(char *text, Player *To)
{
  g_ptr_array_add(dptest_messages, g_strdup(text));
}

void dptest_reset_config(void)
{
  struct CMDLINE cmdline;

  memset(&cmdline, 0, sizeof(cmdline));
  /* Pretend we were run with --version, so that no high score file is
   * opened */
  cmdline.version = TRUE;
  InitConfiguration(&cmdline);
}

void dptest_init(int *argc, char ***argv)
{
  gchar *statedir;

  g_test_init(argc, argv, NULL);

  /* Point the user config file somewhere that doesn't exist, so that the
   * tester's own settings can't change the results */
  statedir = g_build_filename(g_get_tmp_dir(), "dopewars-test-nonexistent",
                              NULL);
  g_setenv("XDG_STATE_HOME", statedir, TRUE);
  g_free(statedir);

  srand(1);
  Network = Client = Server = FALSE;
  dptest_reset_config();

  dptest_messages = g_ptr_array_new_with_free_func(g_free);
  ClientMessageHandlerPt = CaptureMessage;
}

Player *dptest_new_server_player(void)
{
  Player *play, *client;

  client = g_new0(Player, 1);
  FirstClient = AddPlayer(0, client, FirstClient);
  client_players = g_slist_append(client_players, client);

  play = g_new0(Player, 1);
  FirstServer = AddPlayer(0, play, FirstServer);
  SetPlayerName(play, "Tester");
  return play;
}

void dptest_remove_players(void)
{
  GSList *list;

  while (FirstServer) {
    FirstServer = RemovePlayer((Player *)FirstServer->data, FirstServer);
  }
  for (list = client_players; list; list = g_slist_next(list)) {
    FirstClient = RemovePlayer((Player *)list->data, FirstClient);
  }
  g_slist_free(client_players);
  client_players = NULL;
}

void dptest_clear_messages(void)
{
  g_ptr_array_set_size(dptest_messages, 0);
}

const gchar *dptest_find_message(MsgCode code)
{
  guint i;

  for (i = dptest_messages->len; i > 0; i--) {
    const gchar *msg = g_ptr_array_index(dptest_messages, i - 1);
    const gchar *pt = strchr(msg, '^');

    /* Messages are "ID^" followed by the AI code, the message code,
     * and the data */
    if (pt && strlen(pt) >= 3 && pt[2] == code) {
      return pt + 3;
    }
  }
  return NULL;
}
