/************************************************************************
 * common.h       Shared setup code for the dopewars unit tests         *
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

#ifndef __DP_TEST_COMMON_H__
#define __DP_TEST_COMMON_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include "dopewars.h"
#include "message.h"

/* Initializes GLib testing and loads the default game configuration,
 * ignoring any user configuration file. Call once from main(). */
void dptest_init(int *argc, char ***argv);

/* Resets the game configuration to the defaults (e.g. after a test has
 * resized the drug or location lists) */
void dptest_reset_config(void);

/* Creates a player in a local (non-networked) game and adds it to the
 * server's player list, plus a matching client-side player so that
 * messages sent by the server can be captured */
Player *dptest_new_server_player(void);

/* Removes all players created by dptest_new_server_player() */
void dptest_remove_players(void);

/* Messages sent by the server to the client since the last call to
 * dptest_clear_messages(), as an array of strings */
extern GPtrArray *dptest_messages;
void dptest_clear_messages(void);

/* Returns the most recent captured message with the given code, or NULL.
 * The returned pointer is to the data following the message code. */
const gchar *dptest_find_message(MsgCode code);

#endif /* __DP_TEST_COMMON_H__ */
