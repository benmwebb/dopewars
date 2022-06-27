/************************************************************************
 * unicodewrap.h  Unicode wrapper functions for Win32                   *
 * Copyright (C)  2002-2004  Ben Webb                                   *
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

#ifndef __UNICODEWRAP_H__
#define __UNICODEWRAP_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef CYGWIN
#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>

BOOL mySetWindowText(HWND hWnd, LPCTSTR lpString);
HWND myCreateWindow(LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle,
                    int x, int y, int nWidth, int nHeight, HWND hwndParent,
                    HMENU hMenu, HANDLE hInstance, LPVOID lpParam);
HWND myCreateWindowEx(DWORD dwExStyle, LPCTSTR lpClassName,
                      LPCTSTR lpWindowName, DWORD dwStyle, int x, int y,
                      int nWidth, int nHeight, HWND hwndParent, HMENU hMenu,
                      HANDLE hInstance, LPVOID lpParam);
gchar *myGetWindowText(HWND hWnd);
int myDrawText(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect,
               UINT uFormat);
BOOL WINAPI mySetMenuItemInfo(HMENU hMenu, UINT uItem, BOOL fByPosition,
                              LPMENUITEMINFO lpmii);
BOOL WINAPI myInsertMenuItem(HMENU hMenu, UINT uItem, BOOL fByPosition,
                             LPMENUITEMINFO lpmii);
int myHeader_InsertItem(HWND hWnd, int index, const HD_ITEM *phdi);
int myTabCtrl_InsertItem(HWND hWnd, int index, const TC_ITEM *pitem);
ATOM myRegisterClass(CONST WNDCLASS *lpWndClass);
HWND myCreateDialog(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent,
                    DLGPROC lpDialogFunc);
void myEditReplaceSel(HWND hWnd, BOOL fCanUndo, LPCSTR lParam);
int myMessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);
size_t myw32strlen(const char *str);
LRESULT myComboBox_AddString(HWND hWnd, LPCTSTR text);
gchar *w32tostr(const gunichar2 *instr, int len);
gunichar2 *strtow32(const char *instr, int len);

#endif /* CYGWIN */

#endif /* __UNICODEWRAP_H__ */
