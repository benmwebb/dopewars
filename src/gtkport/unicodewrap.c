/************************************************************************
 * unicodewrap.c  Unicode wrapper functions for Win32                   *
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

#ifdef CYGWIN
#include <windows.h>
#include <glib.h>

#include "unicodewrap.h"

static gboolean unicode_support = FALSE;

/*
 * Sets the global variable unicode_support to reflect whether this version
 * of Windows understands Unicode. (WinNT/2000/XP do, 95/98/ME do not.)
 * This is done by calling the Unicode version of GetVersionEx, which should
 * have no undesirable side effects. On non-Unicode systems, this is just
 * a stub function that returns an error.
 */
void InitUnicodeSupport(void)
{
  OSVERSIONINFOW verinfo;

  verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);

  unicode_support =
    (GetVersionExW(&verinfo) || GetLastError() != ERROR_CALL_NOT_IMPLEMENTED);
}

gboolean HaveUnicodeSupport(void)
{
  return unicode_support;
}

/*
 * Converts a string from our internal representation (UTF-8) to a form
 * suitable for Windows Unicode-aware functions (i.e. UTF-16). This
 * returned string must be g_free'd when no longer needed.
 */
static gunichar2 *strtow32(const char *instr)
{
  gunichar2 *outstr;
  outstr = g_utf8_to_utf16(instr, -1, NULL, NULL, NULL);
  if (!outstr) {
    outstr = g_utf8_to_utf16("[?]", -1, NULL, NULL, NULL);
  }
}

BOOL mySetWindowText(HWND hWnd, LPCTSTR lpString)
{
  BOOL retval;

  if (unicode_support) {
    gunichar2 *text;
    text = strtow32(lpString);
    retval = SetWindowTextW(hWnd, text);
    g_free(text);
  } else {
    retval = SetWindowTextA(hWnd, lpString);
  }

  return retval;
}

HWND myCreateWindow(LPCTSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
                    int x, int y, int nWidth, int nHeight, HWND hwndParent,
                    HMENU hMenu, HANDLE hInstance, LPVOID lpParam)
{
  HWND retval;

  if (unicode_support) {
    gunichar2 *classname, *winname;
    classname = strtow32(lpClassName);
    winname = strtow32(lpWindowName);
    retval = CreateWindowW(classname, winname, dwStyle, x, y, nWidth,
                           nHeight, hwndParent, hMenu, hInstance, lpParam);
    g_free(classname);
    g_free(winname);
  } else {
    retval = CreateWindowA(lpClassName, lpWindowName, dwStyle, x, y, nWidth,
                           nHeight, hwndParent, hMenu, hInstance, lpParam);
  }
  return retval;
}

HWND myCreateWindowEx(DWORD dwExStyle, LPCTSTR lpClassName,
                      LPCSTR lpWindowName, DWORD dwStyle, int x, int y,
                      int nWidth, int nHeight, HWND hwndParent, HMENU hMenu,
                      HANDLE hInstance, LPVOID lpParam)
{
  HWND retval;

  if (unicode_support) {
    gunichar2 *classname, *winname;
    classname = strtow32(lpClassName);
    winname = strtow32(lpWindowName);
    retval = CreateWindowExW(dwExStyle, classname, winname, dwStyle, x, y,
                             nWidth, nHeight, hwndParent, hMenu, hInstance,
                             lpParam);
    g_free(classname);
    g_free(winname);
  } else {
    retval = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle,
                             x, y, nWidth, nHeight, hwndParent, hMenu,
                             hInstance, lpParam);
  }
  return retval;
}

#endif /* CYGWIN */
