/* winmain.c      Startup code for dopewars on the Win32 platform       */
/* Copyright (C)  1998-2001  Ben Webb                                   */
/*                Email: ben@bellatrix.pcl.ox.ac.uk                     */
/*                WWW: http://bellatrix.pcl.ox.ac.uk/~ben/dopewars/     */

/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU General Public License          */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */

/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */

/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston,               */
/*                   MA  02111-1307, USA.                               */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef CYGWIN

#include <windows.h>
#include <commctrl.h>
#include <glib.h>
#include <stdlib.h>
#include "dopeid.h"

#include "dopeos.h"
#include "dopewars.h"
#include "tstring.h"
#include "curses_client.h"
#include "gtk_client.h"
#include "message.h"
#include "serverside.h"
#include "gtkport.h"

static void ServerLogMessage(const gchar *log_domain,GLogLevelFlags log_level,
                             const gchar *message,gpointer user_data) {
   DWORD NumChar;
   gchar *text;
   text=g_strdup_printf("%s\n",message);
   WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),text,strlen(text),
                &NumChar,NULL);
   g_free(text);
}

static void Win32PrintFunc(const gchar *string) {
   DWORD NumChar;
   WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),string,strlen(string),
                &NumChar,NULL);
}

int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
                     LPSTR lpszCmdParam,int nCmdShow) {
   gchar **split;
   int argc;
#ifdef ENABLE_NLS
   setlocale(LC_ALL,"");
   bindtextdomain(PACKAGE,LOCALEDIR);
   textdomain(PACKAGE);
#endif
   SetupParameters();
   split=g_strsplit(lpszCmdParam," ",0);
   argc=0;
   while (split[argc]) argc++;
// g_set_print_handler(Win32PrintFunc);
   HandleCmdLine(argc,split);
   g_strfreev(split);
   if (WantVersion || WantHelp) {
      AllocConsole();
      HandleHelpTexts();
      newterm(NULL,NULL,NULL);
      bgetch();
   } else {
      StartNetworking();
      if (Server) {
         AllocConsole();
         SetConsoleTitle(_("dopewars server"));
         g_log_set_handler(NULL,G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING,
                           ServerLogMessage,NULL);
         newterm(NULL,NULL,NULL);
         ServerLoop();
      } else if (WantedClient==CLIENT_CURSES) {
         AllocConsole();
         SetConsoleTitle(_("dopewars"));
         CursesLoop();
      } else {
#if GUI_CLIENT
         GtkLoop(hInstance,hPrevInstance);
#else
         g_print(_("No graphical client available - rebuild the binary\n"
                 "passing the --enable-gui-client option to configure, or\n"
                 "use the curses client (if available) instead!\n"));
#endif
      }
      StopNetworking();
   }
   if (PidFile) g_free(PidFile);
   return 0;
}

#endif /* CYGWIN */
