/************************************************************************
 * tstring.c      "Translated string" wrappers for dopewars             *
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include "dopewars.h"
#include "message.h"
#include "tstring.h"

typedef struct _FmtData {
  union {
    int IntVal;
    price_t PriceVal;
    char CharVal;
    char *StrVal;
  } data;
  char Type;
} FmtData;

/* Largest argument number, width or precision allowed in a format string */
#define MAXFMTNUM 1000

gchar *GetDefaultTString(gchar *tstring)
{
  gchar *dstr, *pt;

  dstr = g_strdup(tstring);
  pt = strchr(dstr, '_');
  if (pt)
    *pt = '\0';
  return dstr;
}

gchar *GetTranslatedString(gchar *str, gchar *code, gboolean Caps)
{
  gchar *dstr, *pt, *tstr, *Default, *tcode;

  dstr = g_strdup(str);
  g_strdelimit(dstr, "_", '^');
  pt = dstr;
  Default = GetNextWord(&pt, "");
  tstr = NULL;

  while (1) {
    tcode = GetNextWord(&pt, NULL);
    tstr = GetNextWord(&pt, "");
    if (!tcode) {
      tstr = NULL;
      break;
    }
    if (strcmp(tcode, code) == 0) {
      break;
    } else
      tstr = NULL;
  }

  if (tstr) {
    if (Caps)
      tstr = InitialCaps(tstr);
    else
      tstr = g_strdup(tstr);
  } else {
    if (Caps)
      tstr = InitialCaps(Default);
    else
      tstr = g_strdup(Default);
  }

  g_free(dstr);
  return tstr;
}

/*
 * Adds the decimal digit "digit" to "num", stopping just above MAXFMTNUM
 * so that huge numbers can't overflow
 */
static int AddFmtDigit(int num, char digit)
{
  return MIN(num * 10 + digit - '0', MAXFMTNUM + 1);
}

void GetNextFormat(guint *Index, gchar *str, int *StartPos,
                   int *EndPos, int *FmtPos, gchar *Type, int *ArgNum,
                   int *Wid, int *Prec, char *Code)
{
  int anum, wid, prec;
  guint i;
  gchar type;

  *StartPos = -1;
  *EndPos = *FmtPos = *ArgNum = *Wid = *Prec = 0;
  *Type = 0;
  Code[0] = 0;
  anum = wid = prec = 0;
  i = *Index;
  while (str[i]) {
    if (str[i] == '%') {
      *StartPos = *EndPos = i++;
      while (str[i] && strchr("#0- +'", str[i]))
        i++;                    /* Skip flag characters */
      while (str[i] >= '0' && str[i] <= '9')
        wid = AddFmtDigit(wid, str[i++]);
      if (str[i] == '$') {
        *EndPos = i;
        i++;
        anum = wid;
        wid = 0;
        while (str[i] && strchr("#0- +'", str[i]))
          i++;                  /* Skip flag characters */
        while (str[i] >= '0' && str[i] <= '9')
          wid = AddFmtDigit(wid, str[i++]);
      }
      if (str[i] == '.') {
        i++;
        while (str[i] >= '0' && str[i] <= '9')
          prec = AddFmtDigit(prec, str[i++]);
      }
      *FmtPos = i;
      type = str[i];
      if ((type == 'T' || type == 't') && i + 2 < strlen(str)) {
        Code[0] = str[i + 1];
        Code[1] = str[i + 2];
        Code[2] = 0;
        i += 3;
      } else if (type == '/') {
        i++;
        while (str[i] != '\0' && str[i] != '/')
          i++;
        if (str[i] == '/')
          i++;
      } else
        i++;
      *ArgNum = anum;
      *Wid = wid;
      *Prec = prec;
      *Index = i;
      *Type = type;
      return;
    } else
      i++;
  }
  *Index = i;
}

/*
 * Fills "arr" with the type of each argument used by the format string
 * "format". Returns FALSE if the format string is invalid: it has an
 * unknown conversion, a number that is too large, an argument that is
 * never used or is used with two different types.
 */
static gboolean GetFormatArgs(gchar *format, GArray *arr)
{
  int StartPos, EndPos, FmtPos, Wid, Prec, ArgNum, DefaultArgNum;
  guint i;
  char Code[3], Type;
  FmtData *fdat;

  i = DefaultArgNum = 0;
  while (i < strlen(format)) {
    GetNextFormat(&i, format, &StartPos, &EndPos, &FmtPos, &Type, &ArgNum,
                  &Wid, &Prec, Code);
    if (StartPos == -1)
      break;
    if (Wid > MAXFMTNUM || Prec > MAXFMTNUM || ArgNum > MAXFMTNUM)
      return FALSE;
    /* %% and %/.../ don't use an argument */
    if (Type == '%' || Type == '/')
      continue;
    if (Type == '\0' || !strchr("dPcstT", Type))
      return FALSE;
    if (ArgNum == 0)
      ArgNum = ++DefaultArgNum;
    if (ArgNum > arr->len) {
      g_array_set_size(arr, ArgNum);
    }
    fdat = &g_array_index(arr, FmtData, ArgNum - 1);
    if (fdat->Type != '\0' && fdat->Type != Type)
      return FALSE;
    fdat->Type = Type;
  }
  for (i = 0; i < arr->len; i++) {
    if (g_array_index(arr, FmtData, i).Type == '\0')
      return FALSE;
  }
  return TRUE;
}

/*
 * Returns TRUE if "format" is a valid format string for the dpg_*printf
 * functions, using arguments of at most the types given in "argtypes",
 * one character per argument: 'd' (int), 'c' (char), 'P' (price_t) or
 * 's' (a string, used by %s, %t or %T). The format string need not use
 * every argument. Use this to check format strings that come from
 * config files or other untrusted sources.
 */
gboolean IsValidTFmt(gchar *format, const gchar *argtypes)
{
  GArray *arr;
  gboolean valid;
  guint i;
  char Type;

  arr = g_array_new(FALSE, TRUE, sizeof(FmtData));
  valid = GetFormatArgs(format, arr) && arr->len <= strlen(argtypes);
  for (i = 0; valid && i < arr->len; i++) {
    Type = g_array_index(arr, FmtData, i).Type;
    if (Type == 't' || Type == 'T')
      Type = 's';
    valid = (Type == argtypes[i]);
  }
  g_array_free(arr, TRUE);
  return valid;
}

gchar *HandleTFmt(gchar *format, va_list va)
{
  int StrInd, StartPos, EndPos, FmtPos, Wid, Prec, ArgNum, DefaultArgNum;
  guint i;
  char Code[3], Type;
  gchar *fstr;
  GString *string, *tmpfmt;
  GArray *arr;
  FmtData *fdat;

  arr = g_array_new(FALSE, TRUE, sizeof(FmtData));
  if (!GetFormatArgs(format, arr)) {
    /* Show the format string as-is rather than guess at the arguments */
    g_warning("Invalid format string: %s", format);
    g_array_free(arr, TRUE);
    return g_strdup(format);
  }

  string = g_string_new("");
  tmpfmt = g_string_new("");

  for (i = 0; i < arr->len; i++) {
    fdat = &g_array_index(arr, FmtData, i);

    switch (fdat->Type) {
    case 'd':
      fdat->data.IntVal = va_arg(va, int);
      break;
    case 'P':
      fdat->data.PriceVal = va_arg(va, price_t);
      break;
    case 'c':
      fdat->data.CharVal = (char)va_arg(va, int);
      break;
    case 's':
    case 't':
    case 'T':
      fdat->data.StrVal = va_arg(va, char *);
      break;
    }
  }
  i = DefaultArgNum = 0;
  while (i < strlen(format)) {
    StrInd = i;
    GetNextFormat(&i, format, &StartPos, &EndPos, &FmtPos, &Type, &ArgNum,
                  &Wid, &Prec, Code);
    if (StartPos == -1) {
      g_string_append(string, &format[StrInd]);
      break;
    }
    while (StrInd < StartPos)
      g_string_append_c(string, format[StrInd++]);
    if (Type == '%') {
      g_string_append_c(string, '%');
      continue;
    } else if (Type == '/') {
      continue;                 /* %/.../ is a comment for translators */
    }
    if (ArgNum == 0)
      ArgNum = ++DefaultArgNum;
    g_string_assign(tmpfmt, "%");
    EndPos++;
    while (EndPos < FmtPos)
      g_string_append_c(tmpfmt, format[EndPos++]);
    if (Type == 'T' || Type == 't' || Type == 'P')
      g_string_append_c(tmpfmt, 's');
    else
      g_string_append_c(tmpfmt, Type);
    fdat = &g_array_index(arr, FmtData, ArgNum - 1);

    switch (Type) {
    case 'd':
      g_string_append_printf(string, tmpfmt->str, fdat->data.IntVal);
      break;
    case 'c':
      g_string_append_printf(string, tmpfmt->str, fdat->data.CharVal);
      break;
    case 'P':
      fstr = FormatPrice(fdat->data.PriceVal);
      g_string_append_printf(string, tmpfmt->str, fstr);
      g_free(fstr);
      break;
    case 't':
    case 'T':
      fstr = GetTranslatedString(fdat->data.StrVal, Code, Type == 'T');
      g_string_append_printf(string, tmpfmt->str, fstr);
      g_free(fstr);
      break;
    case 's':
      g_string_append_printf(string, tmpfmt->str, fdat->data.StrVal);
      break;
    }
  }
  g_array_free(arr, TRUE);
  g_string_free(tmpfmt, TRUE);
  return g_string_free(string, FALSE);
}

void dpg_print(gchar *format, ...)
{
  va_list ap;
  gchar *retstr;

  va_start(ap, format);
  retstr = HandleTFmt(format, ap);
  va_end(ap);
  g_print("%s", retstr);
  g_free(retstr);
}

gchar *dpg_strdup_printf(gchar *format, ...)
{
  va_list ap;
  gchar *retstr;

  va_start(ap, format);
  retstr = HandleTFmt(format, ap);
  va_end(ap);
  return retstr;
}

void dpg_string_printf(GString *string, gchar *format, ...)
{
  va_list ap;
  gchar *newstr;

  va_start(ap, format);
  newstr = HandleTFmt(format, ap);
  g_string_assign(string, newstr);
  g_free(newstr);
  va_end(ap);
}

void dpg_string_append_printf(GString *string, gchar *format, ...)
{
  va_list ap;
  gchar *newstr;

  va_start(ap, format);
  newstr = HandleTFmt(format, ap);
  g_string_append(string, newstr);
  g_free(newstr);
  va_end(ap);
}
