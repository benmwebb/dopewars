/************************************************************************
 * error.c        Error-handling routines for dopewars                  *
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

#include <glib.h>               /* For GString functions */
#include <string.h>             /* For strerror */

#ifdef CYGWIN
#include <windows.h>            /* For FormatMessage() etc. */
#include <winsock.h>            /* For WSAxxx constants */
#else
#include <netdb.h>              /* For h_errno error codes */
#endif

#include "error.h"
#include "nls.h"

void FreeError(LastError *error)
{
  if (!error)
    return;
  if (error->type && error->type->FreeErrorData) {
    (*error->type->FreeErrorData)(error);
  } else {
    g_free(error->data);
  }
  g_free(error);
}

LastError *NewError(ErrorType *type, gint code, gpointer data)
{
  LastError *error;

  error = g_new0(LastError, 1);

  error->type = type;
  error->code = code;
  error->data = data;

  return error;
}

void SetError(LastError **error, ErrorType *type, gint code, gpointer data)
{
  if (!error)
    return;
  if (*error)
    FreeError(*error);
  *error = NewError(type, code, data);
}

void LookupErrorCode(GString *str, gint code, ErrTable *table,
                     gchar *fallbackstr)
{
  for (; table && table->string; table++) {
    if (code == table->code) {
      g_string_append(str, _(table->string));
      return;
    }
  }
  g_string_sprintfa(str, fallbackstr, code);
}

/* "Custom" error handling */
static ErrTable CustomErrStr[] = {
  {E_FULLBUF, N_("Connection dropped due to full buffer")},
  {0, NULL}
};

void CustomAppendError(GString *str, LastError *error)
{
  LookupErrorCode(str, error->code, CustomErrStr,
                  _("Internal error code %d"));
}

static ErrorType ETCustom = { CustomAppendError, NULL };
ErrorType *ET_CUSTOM = &ETCustom;

/* 
 * "errno" error handling
 */
void ErrnoAppendError(GString *str, LastError *error)
{
  g_string_append(str, strerror(error->code));
}

static ErrorType ETErrno = { ErrnoAppendError, NULL };
ErrorType *ET_ERRNO = &ETErrno;

#ifdef CYGWIN

/* Winsock error handling */
static ErrTable WSAErrStr[] = {
  /* These are the explanations of the various
   * Windows Sockets error codes */
  {WSANOTINITIALISED, N_("WinSock has not been properly initialised")},
  {WSASYSNOTREADY, N_("Network subsystem is not ready")},
  {WSAVERNOTSUPPORTED, N_("WinSock version not supported")},
  {WSAENETDOWN, N_("The network subsystem has failed")},
  {WSAEADDRINUSE, N_("Address already in use")},
  {WSAENETDOWN, N_("Cannot reach the network")},
  {WSAETIMEDOUT, N_("The connection timed out")},
  {WSAEMFILE, N_("Out of file descriptors")},
  {WSAENOBUFS, N_("Out of buffer space")},
  {WSAEOPNOTSUPP, N_("Operation not supported")},
  {WSAECONNABORTED, N_("Connection aborted due to failure")},
  {WSAECONNRESET, N_("Connection reset by remote host")},
  {WSAECONNREFUSED, N_("Connection refused")},
  {WSAEAFNOSUPPORT, N_("Address family not supported")},
  {WSAEPROTONOSUPPORT, N_("Protocol not supported")},
  {WSAESOCKTNOSUPPORT, N_("Socket type not supported")},
  {WSAHOST_NOT_FOUND, N_("Host not found")},
  {WSATRY_AGAIN, N_("Temporary name server error - try again later")},
  {WSANO_RECOVERY, N_("Failed to contact nameserver")},
  {WSANO_DATA, N_("Valid name, but no DNS data record present")},
  {0, NULL}
};

void WinsockAppendError(GString *str, LastError *error)
{
  LookupErrorCode(str, error->code, WSAErrStr, _("Network error code %d"));
}

static ErrorType ETWinsock = { WinsockAppendError, NULL };
ErrorType *ET_WINSOCK = &ETWinsock;

/* 
 * Standard Win32 "GetLastError" handling
 */
void Win32AppendError(GString *str, LastError *error)
{
  LPTSTR lpMsgBuf;

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM, NULL, error->code,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) & lpMsgBuf, 0, NULL);
  g_string_append(str, lpMsgBuf);
  LocalFree(lpMsgBuf);
}

static ErrorType ETWin32 = { Win32AppendError, NULL };
ErrorType *ET_WIN32 = &ETWin32;

#else

/* h_errno error handling */
static ErrTable DNSErrStr[] = {
  /* These are the explanations of the various name server error codes */
  {HOST_NOT_FOUND, N_("Host not found")},
  {TRY_AGAIN, N_("Temporary name server error - try again later")},
  {0, NULL}
};

void HErrnoAppendError(GString *str, LastError *error)
{
  LookupErrorCode(str, error->code, DNSErrStr,
                  _("Name server error code %d"));
}

static ErrorType ETHErrno = { HErrnoAppendError, NULL };
ErrorType *ET_HERRNO = &ETHErrno;

#endif /* CYGWIN */

void g_string_assign_error(GString *str, LastError *error)
{
  g_string_truncate(str, 0);
  g_string_append_error(str, error);
}

void g_string_append_error(GString *str, LastError *error)
{
  if (!error || !error->type)
    return;
  (*error->type->AppendErrorString) (str, error);
}
