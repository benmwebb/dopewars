/************************************************************************
 * configfile.c   Functions for dealing with dopewars config files      *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>             /* For memcmp etc. */
#include <stdio.h>              /* For fgetc etc. */
#include <stdlib.h>             /* For atoi */
#include <errno.h>              /* For errno */
#include <ctype.h>              /* For isprint */
#include <glib.h>

#include "dopewars.h"           /* For struct GLOBALS etc. */
#include "nls.h"                /* For _ function */
#include "error.h"              /* For ErrStrFromErrno */

/*
 * Prints the given string to a file, converting control characters
 * and escaping other special characters.
 */
static void PrintEscaped(FILE *fp, gchar *str)
{
  guint i;

  for (i = 0; i < strlen(str); i++) {
    switch(str[i]) {
    case '"':
    case '\'':
    case '\\':
      fputc('\\', fp);
      fputc(str[i], fp);
      break;
    case '\n':
      fputs("\\n", fp);
      break;
    case '\t':
      fputs("\\t", fp);
      break;
    case '\r':
      fputs("\\r", fp);
      break;
    case '\b':
      fputs("\\b", fp);
      break;
    case '\f':
      fputs("\\f", fp);
      break;
    default:
      if (isprint(str[i])) {
        fputc(str[i], fp);
      } else {
        fprintf(fp, "\\%o", (int)(guchar)str[i]);
      }
    }
  }
}

/*
 *  * Writes a single configuration file variable (identified by GlobalIndex
 *   * and StructIndex) to the specified file, in a format suitable for reading
 *    * back in (via. ParseNextConfig and friends).
 *     */
static void WriteConfigValue(FILE *fp, int GlobalIndex, int StructIndex)
{
  gchar *GlobalName;

  if (Globals[GlobalIndex].NameStruct[0]) {
    GlobalName =
        g_strdup_printf("%s[%d].%s", Globals[GlobalIndex].NameStruct,
                        StructIndex, Globals[GlobalIndex].Name);
  } else {
    GlobalName = Globals[GlobalIndex].Name;
  }

  if (Globals[GlobalIndex].IntVal) {
    fprintf(fp, "%s = %d\n", GlobalName,
            *GetGlobalInt(GlobalIndex, StructIndex));
  } else if (Globals[GlobalIndex].BoolVal) {
    fprintf(fp, "%s = %s\n", GlobalName,
            *GetGlobalBoolean(GlobalIndex, StructIndex) ?
            _("TRUE") : _("FALSE"));
  } else if (Globals[GlobalIndex].PriceVal) {
    gchar *prstr = pricetostr(*GetGlobalPrice(GlobalIndex, StructIndex));

    fprintf(fp, "%s = %s\n", GlobalName, prstr);
    g_free(prstr);
  } else if (Globals[GlobalIndex].StringVal) {
    fprintf(fp, "%s = \"", GlobalName);
    PrintEscaped(fp, *GetGlobalString(GlobalIndex, StructIndex));
    fprintf(fp, "\"\n");
  } else if (Globals[GlobalIndex].StringList) {
    int i;

    fprintf(fp, "%s = { ", GlobalName);
    for (i = 0; i < *Globals[GlobalIndex].MaxIndex; i++) {
      if (i > 0)
        fprintf(fp, ", ");
      fputc('"', fp);
      PrintEscaped(fp, (*Globals[GlobalIndex].StringList)[i]);
      fputc('"', fp);
    }
    fprintf(fp, " }\n");
  }

  if (Globals[GlobalIndex].NameStruct[0])
    g_free(GlobalName);
}


static void ReadFileToString(FILE *fp, gchar *str, int matchlen)
{
  int len, mpos, ch;
  gchar *match;
  GString *file;

  file = g_string_new("");
  len = strlen(str);
  if (matchlen > 0) {
    len = MIN(len, matchlen);
  }
  match = g_new(gchar, len);
  mpos = 0;

  while (mpos < len && (ch = fgetc(fp)) != EOF) {
    g_string_append_c(file, ch);
    match[mpos++] = ch;
    if (ch != str[mpos - 1]) {
      int start;
      gboolean shortmatch = FALSE;

      for (start = 1; start < mpos; start++) {
        if (memcmp(str, &match[start], mpos - start) == 0) {
          mpos -= start;
          memmove(match, &match[start], mpos);
          shortmatch = TRUE;
          break;
        }
      }
      if (!shortmatch)
        mpos = 0;
    }
  }
  g_string_truncate(file, file->len - mpos);

  g_free(match);

  rewind(fp);
  ftruncate(fileno(fp), 0);
  fprintf(fp, file->str);

  fprintf(fp, str);

  g_string_free(file, TRUE);
}

/*
 * Writes all of the configuration file variables that have changed
 * (together with their values) to the given file.
 */
static void WriteConfigFile(FILE *fp)
{
  int i, j;

  for (i = 0; i < NUMGLOB; i++) {
    if (Globals[i].Modified) {
      if (Globals[i].NameStruct[0]) {
        for (j = 1; j <= *Globals[i].MaxIndex; j++) {
          WriteConfigValue(fp, i, j);
        }
      } else {
        WriteConfigValue(fp, i, 0);
      }
    }
  }
}

void UpdateConfigFile(gchar *cfgfile)
{
  FILE *fp;
  static gchar *header =
      "\n### Everything from here on is written automatically by\n"
      "### the dopewars program; you can edit it manually, but any\n"
      "### formatting (comments, etc.) will be lost at the next rewrite.\n\n";

  if (!cfgfile) {
    cfgfile = GetLocalConfigFile();
    if (!cfgfile) {
      g_warning(_("Could not determine local config file to write to"));
      return;
    }
  }

  fp = fopen(cfgfile, "r+");
  if (!fp) {
    fp = fopen(cfgfile, "w+");
  }

  if (!fp) {
    gchar *errstr = ErrStrFromErrno(errno);
    g_warning(_("Could not open file %s: %s"), cfgfile, errstr);
    g_free(errstr);
    g_free(cfgfile);
    return;
  }

  ReadFileToString(fp, header, 50);
  WriteConfigFile(fp);

  fclose(fp);
  g_free(cfgfile);
}
