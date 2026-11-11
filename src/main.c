/************************************************************************
 * main.c         dopewars - program entry point (non-Win32)            *
 * Copyright (C)  1998-2022  Ben Webb                                   *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <glib.h>

#include "dopewars.h"
#include "admin.h"
#include "error.h"
#include "log.h"
#include "message.h"
#include "network.h"
#include "nls.h"
#include "serverside.h"
#include "sound.h"
#include "AIPlayer.h"

#ifdef CURSES_CLIENT
#include "curses_client/curses_client.h"
#endif

#ifdef GUI_CLIENT
#include "gui_client/gtk_client.h"
#endif

#ifdef GUI_SERVER
#include "gtkport/gtkport.h"
#endif

#ifndef CYGWIN

#if defined(NETWORKING) && !defined(GUI_SERVER)
static void ServerLogMessage(const gchar *log_domain,
                             GLogLevelFlags log_level,
                             const gchar *message, gpointer user_data)
{
  GString *text;

  text = GetLogString(log_level, message);
  if (text) {
    fprintf(Log.fp ? Log.fp : stdout, "%s\n", text->str);
    g_string_free(text, TRUE);
  }
}
#endif

#ifndef CURSES_CLIENT
/*
 * Stub function to report an error if the Curses client is requested and
 * it isn't compiled in.
 */
void CursesLoop(struct CMDLINE *cmdline)
{
  g_print(_("No curses client available - rebuild the binary passing the\n"
            "--enable-curses-client option to configure, or use a windowed\n"
            "client (if available) instead!\n"));
}
#endif

#ifndef GUI_CLIENT
/*
 * Stub function to report an error if the GTK+ client is requested and
 * it isn't compiled in.
 */
#ifdef CYGWIN
gboolean GtkLoop(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                 struct CMDLINE *cmdline, gboolean ReturnOnFail)
#else
gboolean GtkLoop(int *argc, char **argv[], struct CMDLINE *cmdline,
                 gboolean ReturnOnFail)
#endif
{
  if (!ReturnOnFail) {
    g_print(_("No graphical client available - rebuild the binary\n"
              "passing the --enable-gui-client option to configure, or\n"
              "use the curses client (if available) instead!\n"));
  }
  return FALSE;
}
#endif

static void DefaultLogMessage(const gchar *log_domain,
                              GLogLevelFlags log_level,
                              const gchar *message, gpointer user_data)
{
  GString *text;

  text = GetLogString(log_level, message);
  if (text) {
    g_print("%s\n", text->str);
    g_string_free(text, TRUE);
  }
}

/* 
 * Standard program entry - Win32 uses WinMain() instead, in winmain.c
 */
int main(int argc, char *argv[])
{
  struct CMDLINE *cmdline;
#ifdef ENABLE_NLS
  const char *charset;
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
  LocaleIsUTF8 = g_get_charset(&charset);
#endif
  WantUTF8Errors(FALSE);
  g_log_set_handler(NULL, LogMask(), DefaultLogMessage, NULL);
  cmdline = GeneralStartup(argc, argv);
  if (cmdline->logfile) {
    AssignName(&Log.File, cmdline->logfile);
  }
  OpenLog();
  SoundInit();
  if (cmdline->version || cmdline->help) {
    HandleHelpTexts(cmdline->help);
  } else if (cmdline->admin) {
#ifdef NETWORKING
    AdminServer(cmdline);
#else
    g_print(_("This binary has been compiled without networking "
              "support, and thus cannot run\nin admin mode. "
              "Recompile passing --enable-networking to the "
              "configure script.\n"));
#endif
  } else if (cmdline->convert) {
    ConvertHighScoreFile(cmdline->convertfile);
  } else {
    InitNetwork();
    if (cmdline->server) {
#ifdef NETWORKING
#ifdef GUI_SERVER
      Server = TRUE;
#if GTK_MAJOR_VERSION >= 4
      gtk_init();
#else
#if GTK_MAJOR_VERSION == 2
      gtk_set_locale();
#endif
      gtk_init(&argc, &argv);
#endif
      GuiServerLoop(cmdline, FALSE);
#else
      g_log_set_handler(NULL, LogMask(), ServerLogMessage, NULL);
      ServerLoop(cmdline);
#endif /* GUI_SERVER */
#else
      g_print(_("This binary has been compiled without networking "
                "support, and thus cannot run\nin server mode. "
                "Recompile passing --enable-networking to the "
                "configure script.\n"));
#endif /* NETWORKING */
    } else if (cmdline->ai) {
      AIPlayerLoop(cmdline);
    } else
      switch (cmdline->client) {
      case CLIENT_AUTO:
        if (!GtkLoop(&argc, &argv, cmdline, TRUE))
          CursesLoop(cmdline);
        break;
      case CLIENT_WINDOW:
        GtkLoop(&argc, &argv, cmdline, FALSE);
        break;
      case CLIENT_CURSES:
        CursesLoop(cmdline);
        break;
      }
#ifdef NETWORKING
    StopNetworking();
#endif
  }
  FreeCmdLine(cmdline);
  CloseLog();
  CloseHighScoreFile();
  g_free(PidFile);
  g_free(Log.File);
  SoundClose();
  return 0;
}

#endif /* CYGWIN */
