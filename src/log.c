/************************************************************************
 * log.c          dopewars - logging functions                          *
 * Copyright (C)  1998-2005  Ben Webb                                   *
 *                Email: benwebb@users.sf.net                           *
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

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <glib.h>
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#include "dopewars.h"
#include "log.h"

/* 
 * General logging function. All messages should be given a loglevel,
 * from 0 to 5 (0=vital, 2=normal, 5=maximum debugging output). This
 * is essentially just a wrapper around the GLib g_log function.
 */
void dopelog(const int loglevel, const LogFlags flags,
             const gchar *format, ...)
{
  va_list args;

  /* Don't print server log messages when running standalone */
  if (flags & LF_SERVER && !Network)
    return;

  va_start(args, format);
  g_logv(G_LOG_DOMAIN, 1 << (loglevel + G_LOG_LEVEL_USER_SHIFT), format, args);
  va_end(args);

#ifdef HAVE_SYSLOG_H
  if (loglevel <= Log.Level) {
    va_start(args, format);
    vsyslog(LOG_INFO, format, args);
    va_end(args);
  }
#endif
}

/* 
 * Returns the bitmask necessary to catch all custom log messages.
 */
GLogLevelFlags LogMask()
{
  return ((1 << (MAXLOG)) - 1) << G_LOG_LEVEL_USER_SHIFT;
}

/* 
 * Returns the text to be displayed in a log message, if any.
 */
GString *GetLogString(GLogLevelFlags log_level, const gchar *message)
{
  GString *text;
  gchar TimeBuf[80];
  gint i;
  time_t tim;
  struct tm *timep;

  text = g_string_new("");
  if (Log.Timestamp) {
    tim = time(NULL);
    timep = localtime(&tim);
    strftime(TimeBuf, 80, Log.Timestamp, timep);
    TimeBuf[79] = '\0';
    g_string_append(text, TimeBuf);
  }

  for (i = 0; i < MAXLOG; i++)
    if (log_level & (1 << (G_LOG_LEVEL_USER_SHIFT + i))) {
      if (i > Log.Level) {
        g_string_free(text, TRUE);
        return NULL;
      }
      g_string_sprintfa(text, "%d: ", i);
    }
  g_string_append(text, message);
  return text;
}

void OpenLog(void)
{
  CloseLog();
#ifdef HAVE_SYSLOG_H
  openlog(PACKAGE, LOG_PID, LOG_USER);
#endif
  if (Log.File[0] == '\0')
    return;
  Log.fp = fopen(Log.File, "a");
  if (Log.fp) {
#ifdef SETVBUF_REVERSED         /* 2nd and 3rd arguments are reversed on
                                 * some systems */
    setvbuf(Log.fp, _IOLBF, (char *)NULL, 0);
#else
    setvbuf(Log.fp, (char *)NULL, _IOLBF, 0);
#endif
  }
}

void CloseLog(void)
{
  if (Log.fp)
    fclose(Log.fp);
  Log.fp = NULL;
}
