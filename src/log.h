/************************************************************************
 * log.h          Logging functions for dopewars                        *
 * Copyright (C)  1998-2003  Ben Webb                                   *
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

#ifndef __DP_LOG_H__
#define __DP_LOG_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

typedef enum {
    LF_SERVER = (1 << 0)
} LogFlags;

void dopelog(const int loglevel, const LogFlags flags,
                 const gchar *format, ...);
GLogLevelFlags LogMask(void);
GString *GetLogString(GLogLevelFlags log_level, const gchar *message);
void OpenLog(void);
void CloseLog(void);

#endif /* __DP_LOG_H__ */
