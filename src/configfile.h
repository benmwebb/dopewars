/************************************************************************
 * configfile.h   Functions for dealing with dopewars config files      *
 * Copyright (C)  2002-2004  Ben Webb                                   *
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

#ifndef __DP_CONFIGFILE_H__
#define __DP_CONFIGFILE_H__

#include <glib.h>

extern gchar *LocalCfgEncoding;
gboolean UpdateConfigFile(const gchar *cfgfile, gboolean ForceUTF8);
gboolean IsConfigFileUTF8(void);

#endif /* __DP_CONFIGFILE_H__ */
