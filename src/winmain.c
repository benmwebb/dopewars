/************************************************************************
 * winmain.c      Startup code and support for the Win32 platform       *
 * Copyright (C)  1998-2002  Ben Webb                                   *
 *                Email: ben@bellatrix.pcl.ox.ac.uk                     *
 *                WWW: http://dopewars.sourceforge.net/                 *
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

#ifdef CYGWIN

#include <windows.h>
#include <commctrl.h>
#include <glib.h>
#include <stdlib.h>

#include "dopewars.h"
#include "nls.h"
#include "tstring.h"
#include "util.h"
#include "AIPlayer.h"
#include "message.h"
#include "serverside.h"
#include "winmain.h"

#ifdef CURSES_CLIENT
#include "curses_client/curses_client.h"
#endif

#ifdef GUI_CLIENT
#include "gui_client/gtk_client.h"
#endif

#ifdef GUI_SERVER
#include "gtkport/gtkport.h"
#endif

static void ServerLogMessage(const gchar *log_domain,
                             GLogLevelFlags log_level,
                             const gchar *message, gpointer user_data)
{
  GString *text;

  text = GetLogString(log_level, message);
  if (text) {
    g_string_append(text, "\n");
    g_print(text->str);
    g_string_free(text, TRUE);
  }
}

static void ServerPrintFunc(const gchar *string)
{
  DWORD NumChar;

  WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), string, strlen(string),
               &NumChar, NULL);
}

static void LogMessage(const gchar *log_domain, GLogLevelFlags log_level,
                       const gchar *message, gpointer user_data)
{
  MessageBox(NULL, message, "Error", MB_OK | MB_ICONSTOP);
}

static GString *TextOutput = NULL;

static void WindowPrintStart()
{
  TextOutput = g_string_new("");
}

static void WindowPrintFunc(const gchar *string)
{
  g_string_append(TextOutput, string);
}

static void WindowPrintEnd()
{
  MessageBox(NULL, TextOutput->str, "dopewars",
             MB_OK | MB_ICONINFORMATION);
  g_string_free(TextOutput, TRUE);
  TextOutput = NULL;
}

static FILE *LogFile = NULL;

static void LogFileStart()
{
  LogFile = fopen("dopewars-log.txt", "w");
}

static void LogFilePrintFunc(const gchar *string)
{
  if (LogFile) {
    fprintf(LogFile, string);
    fflush(LogFile);
  }
}

static void LogFileEnd(void)
{
  if (LogFile)
    fclose(LogFile);
}

gchar *GetBinaryDir(void)
{
  gchar *filename = NULL, *lastslash;
  gint filelen = 80;
  DWORD retval;

  while (1) {
    filename = g_realloc(filename, filelen);
    filename[filelen - 1] = '\0';
    retval = GetModuleFileName(NULL, filename, filelen);

    if (retval == 0) {
      g_free(filename);
      filename = NULL;
      break;
    } else if (filename[filelen - 1]) {
      filelen *= 2;
    } else
      break;
  }

  if (filename) {
    lastslash = strrchr(filename, '\\');
    if (lastslash)
      *lastslash = '\0';
  }

  return filename;
}

#ifdef ENABLE_NLS
static gchar *GetWindowsLocale(void)
{
  LCID userID;
  WORD lang, sublang;
  static gchar langenv[30];     /* Should only be 5 chars, so
                                 * this'll be plenty */
  gchar *oldlang;

  langenv[0] = '\0';
  strcpy(langenv, "LANG=");

  oldlang = getenv("LANG");

  if (oldlang) {
    g_print("Started with LANG = %s\n", oldlang);
    return NULL;
  }

  userID = GetUserDefaultLCID();
  lang = PRIMARYLANGID(LANGIDFROMLCID(userID));
  sublang = SUBLANGID(LANGIDFROMLCID(userID));

  switch (lang) {
  case LANG_ENGLISH:
    strcat(langenv, "en");
    if (sublang == SUBLANG_ENGLISH_UK)
      strcat(langenv, "_gb");
    break;
  case LANG_FRENCH:
    strcat(langenv, "fr");
    break;
  case LANG_GERMAN:
    strcat(langenv, "de");
    break;
  case LANG_POLISH:
    strcat(langenv, "pl");
    break;
  case LANG_PORTUGUESE:
    strcat(langenv, "pt");
    if (sublang == SUBLANG_PORTUGUESE_BRAZILIAN)
      strcat(langenv, "_br");
    break;
  }

  if (strlen(langenv) > 5) {
    g_print("Using Windows language %s\n", langenv);
    return langenv;
  } else
    return NULL;
}
#endif /* ENABLE_NLS */

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpszCmdParam, int nCmdShow)
{
  gchar **split;
  int argc;
  gboolean is_service;
  gchar *modpath;

#ifdef ENABLE_NLS
  gchar *winlocale;
#endif

  /* Are we running as an NT service? */
  is_service = (lpszCmdParam && strncmp(lpszCmdParam, "-N", 2) == 0);

  if (is_service) {
    modpath = GetBinaryDir();
    SetCurrentDirectory(modpath);
    g_free(modpath);
  }

  LogFileStart();
  g_set_print_handler(LogFilePrintFunc);

  g_log_set_handler(NULL, LogMask() | G_LOG_LEVEL_MESSAGE,
                    ServerLogMessage, NULL);

  if (!is_service) {
    g_log_set_handler(NULL, G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL,
                      LogMessage, NULL);
  }
#ifdef ENABLE_NLS
  winlocale = GetWindowsLocale();
  if (winlocale)
    putenv(winlocale);

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif

  /* Informational comment placed at the start of the Windows log file
   * (this is used for messages printed during processing of the config
   * files - under Unix these are just printed to stdout) */
  g_print(_("# This is the dopewars startup log, containing any\n"
            "# informative messages resulting from configuration\n"
            "# file processing and the like.\n\n"));

#ifdef HAVE_GLIB2
  split = g_strsplit(lpszCmdParam, " ", 1);
#else
  split = g_strsplit(lpszCmdParam, " ", 0);
#endif
  argc = 0;
  while (split[argc] && split[argc][0])
    argc++;

  GeneralStartup(argc, split);
  OpenLog();
  if (WantVersion || WantHelp) {
    WindowPrintStart();
    g_set_print_handler(WindowPrintFunc);
    HandleHelpTexts();
    WindowPrintEnd();
#ifdef NETWORKING
  } else if (is_service) {
    StartNetworking();
    Network = Server = TRUE;
    win32_init(hInstance, hPrevInstance, "mainicon");
    ServiceMain();
    StopNetworking();
#endif
  } else if (WantConvert) {
    WindowPrintStart();
    g_set_print_handler(WindowPrintFunc);
    ConvertHighScoreFile();
    WindowPrintEnd();
  } else {
#ifdef NETWORKING
    StartNetworking();
#endif
    if (Server) {
#ifdef NETWORKING
#ifdef GUI_SERVER
      g_log_set_handler(NULL, G_LOG_LEVEL_CRITICAL, LogMessage, NULL);
      win32_init(hInstance, hPrevInstance, "mainicon");
      GuiServerLoop(FALSE);
#else
      AllocConsole();
      SetConsoleTitle(_("dopewars server"));
      g_log_set_handler(NULL,
                        LogMask() | G_LOG_LEVEL_MESSAGE |
                        G_LOG_LEVEL_WARNING, ServerLogMessage, NULL);
      g_set_print_handler(ServerPrintFunc);
      newterm(NULL, NULL, NULL);
      ServerLoop();
#endif /* GUI_SERVER */
#else
      WindowPrintStart();
      g_set_print_handler(WindowPrintFunc);
      g_print(_("This binary has been compiled without networking "
                "support, and thus cannot run\nin server mode. "
                "Recompile passing --enable-networking to the "
                "configure script.\n"));
      WindowPrintEnd();
#endif /* NETWORKING */
    } else if (AIPlayer) {
      AllocConsole();

      /* Title of the Windows window used for AI player output */
      SetConsoleTitle(_("dopewars AI"));

      g_log_set_handler(NULL,
                        LogMask() | G_LOG_LEVEL_MESSAGE |
                        G_LOG_LEVEL_WARNING, ServerLogMessage, NULL);
      g_set_print_handler(ServerPrintFunc);
      AIPlayerLoop();
    } else {
      switch (WantedClient) {
      case CLIENT_AUTO:
        if (!GtkLoop(hInstance, hPrevInstance, TRUE)) {
          AllocConsole();
          SetConsoleTitle(_("dopewars"));
          CursesLoop();
        }
        break;
      case CLIENT_WINDOW:
        GtkLoop(hInstance, hPrevInstance, FALSE);
        break;
      case CLIENT_CURSES:
        AllocConsole();
        SetConsoleTitle(_("dopewars"));
        CursesLoop();
        break;
      }
    }
#ifdef NETWORKING
    StopNetworking();
#endif
  }
  CloseLog();
  LogFileEnd();
  g_strfreev(split);
  CloseHighScoreFile();
  g_free(PidFile);
  g_free(Log.File);
  g_free(ConvertFile);
  return 0;
}

#endif /* CYGWIN */
