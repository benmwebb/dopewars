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

BOOL mySetWindowText(HWND hWnd, LPCTSTR lpString)
{
  gunichar2 *text;
  BOOL retval;

  if (!(text = g_utf8_to_utf16(lpString, -1, NULL, NULL, NULL))) {
    retval = SetWindowTextA(hWnd, lpString);
  } else {
    retval = SetWindowTextW(hWnd, text);
    g_free(text);
  }
  return retval;
}

HWND myCreateWindow(LPCTSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
                    int x, int y, int nWidth, int nHeight, HWND hwndParent,
                    HMENU hMenu, HANDLE hInstance, LPVOID lpParam)
{
  gunichar2 *classname, *winname;
  HWND retval;

  classname = g_utf8_to_utf16(lpClassName, -1, NULL, NULL, NULL);
  winname = g_utf8_to_utf16(lpWindowName, -1, NULL, NULL, NULL);
  if (!classname || !winname) {
    retval = CreateWindowA(lpClassName, lpWindowName, dwStyle, x, y, nWidth,
                           nHeight, hwndParent, hMenu, hInstance, lpParam);
  } else {
    retval = CreateWindowW(classname, winname, dwStyle, x, y, nWidth,
                           nHeight, hwndParent, hMenu, hInstance, lpParam);
  }
  g_free(classname);
  g_free(winname);
  return retval;
}

HWND myCreateWindowEx(DWORD dwExStyle, LPCTSTR lpClassName,
                      LPCSTR lpWindowName, DWORD dwStyle, int x, int y,
                      int nWidth, int nHeight, HWND hwndParent, HMENU hMenu,
                      HANDLE hInstance, LPVOID lpParam)
{
  gunichar2 *classname, *winname;
  HWND retval;

  classname = g_utf8_to_utf16(lpClassName, -1, NULL, NULL, NULL);
  winname = g_utf8_to_utf16(lpWindowName, -1, NULL, NULL, NULL);
  if (!classname || !winname) {
    retval = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle,
                             x, y, nWidth, nHeight, hwndParent, hMenu,
                             hInstance, lpParam);
  } else {
    retval = CreateWindowExW(dwExStyle, classname, winname, dwStyle, x, y,
                             nWidth, nHeight, hwndParent, hMenu, hInstance,
                             lpParam);
  }
  g_free(classname);
  g_free(winname);
  return retval;
}

#endif /* CYGWIN */
