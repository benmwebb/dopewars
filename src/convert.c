/************************************************************************
 * convert.c      Codeset conversion functions                          *
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <glib.h>

#include "convert.h"

static gchar *int_codeset = NULL;

static const gchar *FixedCodeset(const gchar *codeset)
{
  if (strcmp(codeset, "ANSI_X3.4-1968") == 0
      || strcmp(codeset, "ASCII") == 0) {
    return "ISO-8859-1";
  } else {
    return codeset;
  }
}

void Conv_SetInternalCodeset(const gchar *codeset)
{
  g_free(int_codeset);
  int_codeset = g_strdup(FixedCodeset(codeset));
}

static const gchar *GetLocaleCodeset(void)
{
#ifdef HAVE_GLIB2
  const gchar *codeset;

  g_get_charset(&codeset);
  return FixedCodeset(codeset);
#else
  return "ISO-8859-1";
#endif
}

Converter *Conv_New(void)
{
  Converter *conv;

  conv = g_new(Converter, 1);
  conv->ext_codeset = g_strdup(GetLocaleCodeset());
  if (!int_codeset) {
    int_codeset = g_strdup(GetLocaleCodeset());
  }
  return conv;
}

void Conv_Free(Converter *conv)
{
  g_free(conv->ext_codeset);
  g_free(conv);
}

void Conv_SetCodeset(Converter *conv, const gchar *codeset)
{
  g_free(conv->ext_codeset);
  conv->ext_codeset = g_strdup(FixedCodeset(codeset));
}

gboolean Conv_Needed(Converter *conv)
{
#ifdef HAVE_GLIB2
  return (strcmp(conv->ext_codeset, int_codeset) != 0
          || strcmp(int_codeset, "UTF-8") == 0);
#else
  return FALSE;
#endif
}

static gchar *do_convert(const gchar *from_codeset, const gchar *to_codeset,
                         const gchar *from_str, int from_len)
{
#ifdef HAVE_GLIB2
  gchar *to_str;

  if (strcmp(to_codeset, "UTF-8") == 0 && strcmp(from_codeset, "UTF-8") == 0) {
    const gchar *start, *end;

    if (from_len == -1) {
      to_str = g_strdup(from_str);
    } else {
      to_str = g_strndup(from_str, from_len);
    }
    start = to_str;
    while (start && *start && !g_utf8_validate(start, -1, &end)
	   && end && *end) {
      *((gchar *)end) = '?';
      start = ++end;
    }
    return to_str;
  } else {
    to_str = g_convert_with_fallback(from_str, from_len, to_codeset,
                                     from_codeset, "?", NULL, NULL, NULL);
    if (to_str) {
      return to_str;
    } else {
      return g_strdup("[?]");
    }
  }
#else
  if (from_len == -1) {
    return g_strdup(from_str);
  } else {
    return g_strndup(from_str, from_len);
  }
#endif
}

gchar *Conv_ToExternal(Converter *conv, const gchar *int_str, int len)
{
  return do_convert(int_codeset, conv->ext_codeset, int_str, len);
}

gchar *Conv_ToInternal(Converter *conv, const gchar *ext_str, int len)
{
  return do_convert(conv->ext_codeset, int_codeset, ext_str, len);
}
