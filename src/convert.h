/************************************************************************
 * convert.h      Header file for codeset conversion functions          *
 * Copyright (C)  2002  Ben Webb                                        *
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

#ifndef __DP_CONVERT_H__
#define __DP_CONVERT_H__

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>

typedef struct _Converter Converter;
struct _Converter {
  gchar *ext_codeset;
};

void Conv_SetInternalCodeset(const gchar *codeset);
Converter *Conv_New(void);
void Conv_SetCodeset(Converter *conv, const gchar *codeset);
gboolean Conv_Needed(Converter *conv);
gchar *Conv_ToExternal(Converter *conv, const gchar *int_str, size_t len);
gchar *Conv_ToInternal(Converter *conv, const gchar *ext_str, size_t len);
void Conv_Free(Converter *conv);

#endif /* __DP_CONVERT_H__ */
