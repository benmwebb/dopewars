/************************************************************************
 * unicodewrap.h  Unicode wrapper functions for Win32                   *
 * Copyright (C)  2002-2003  Ben Webb                                   *
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

#ifndef __UNICODEWRAP_H__
#define __UNICODEWRAP_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef CYGWIN
#include <windows.h>
#include <commctrl.h>

void InitUnicodeSupport(void);

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
LRESULT mySendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
void myEditReplaceSel(HWND hWnd, BOOL fCanUndo, LPCSTR lParam);
LONG mySetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong);
LONG myGetWindowLong(HWND hWnd, int nIndex);
LRESULT myCallWindowProc(WNDPROC lpPrevWndProc, HWND hWnd, UINT Msg,
                         WPARAM wParam, LPARAM lParam);
LRESULT myDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int myMessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);
BOOL myGetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin,
                  UINT wMsgFilterMax);
LONG myDispatchMessage(CONST MSG *lpmsg);
BOOL myIsDialogMessage(HWND hDlg, LPMSG lpMsg);
size_t myw32strlen(const char *str);
LRESULT myComboBox_AddString(HWND hWnd, LPCTSTR text);

#endif /* CYGWIN */

#endif /* __UNICODEWRAP_H__ */
