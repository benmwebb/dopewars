/************************************************************************
 * error.h        Header file for dopewars error-handling routines      *
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

#ifndef __ERROR_H__
#define __ERROR_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

struct _LastError;
typedef struct _ErrorType {
  void (*AppendErrorString) (GString *str, struct _LastError *error);
  void (*FreeErrorData) (struct _LastError *error);
} ErrorType;

typedef struct _LastError {
  gint code;
  ErrorType *type;
  gpointer data;
} LastError;

extern ErrorType *ET_CUSTOM, *ET_ERRNO;

#ifdef CYGWIN
extern ErrorType *ET_WIN32, *ET_WINSOCK;
#else
extern ErrorType *ET_HERRNO;
#endif

typedef enum {
  E_FULLBUF
} CustomErrorCode;

typedef struct _ErrTable {
  gint code;
  gchar *string;
} ErrTable;

void FreeError(LastError *error);
LastError *NewError(ErrorType *type, gint code, gpointer data);
void SetError(LastError **error, ErrorType *type, gint code,
              gpointer data);
void LookupErrorCode(GString *str, gint code, ErrTable *table,
                     gchar *fallbackstr);
void g_string_assign_error(GString *str, LastError *error);
void g_string_append_error(GString *str, LastError *error);

#endif /* __ERROR_H__ */
