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
static gunichar2 *strtow32(const char *instr, int len)
{
  gunichar2 *outstr;
  if (!instr) {
    return NULL;
  }
  outstr = g_utf8_to_utf16(instr, len, NULL, NULL, NULL);
  if (!outstr) {
    outstr = g_utf8_to_utf16("[?]", len, NULL, NULL, NULL);
  }
  return outstr;
}

static gchar *w32tostr(const gunichar2 *instr, int len)
{
  gchar *outstr;
  if (!instr) {
    return NULL;
  }
  outstr = g_utf16_to_utf8(instr, len, NULL, NULL, NULL);
  if (!outstr) {
    outstr = g_strdup("[?]");
  }
  return outstr;
}

BOOL mySetWindowText(HWND hWnd, LPCTSTR lpString)
{
  BOOL retval;

  if (unicode_support) {
    gunichar2 *text;
    text = strtow32(lpString, -1);
    retval = SetWindowTextW(hWnd, text);
    g_free(text);
  } else {
    retval = SetWindowTextA(hWnd, lpString);
  }

  return retval;
}

HWND myCreateWindow(LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle,
                    int x, int y, int nWidth, int nHeight, HWND hwndParent,
                    HMENU hMenu, HANDLE hInstance, LPVOID lpParam)
{
  HWND retval;

  if (unicode_support) {
    gunichar2 *classname, *winname;
    classname = strtow32(lpClassName, -1);
    winname = strtow32(lpWindowName, -1);
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
                      LPCTSTR lpWindowName, DWORD dwStyle, int x, int y,
                      int nWidth, int nHeight, HWND hwndParent, HMENU hMenu,
                      HANDLE hInstance, LPVOID lpParam)
{
  HWND retval;

  if (unicode_support) {
    gunichar2 *classname, *winname;
    classname = strtow32(lpClassName, -1);
    winname = strtow32(lpWindowName, -1);
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

gchar *myGetWindowText(HWND hWnd)
{
  gint textlen;

  textlen = SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
  if (unicode_support) {
    gunichar2 *buffer;
    gchar *retstr;

    buffer = g_new0(gunichar2, textlen + 1);
    GetWindowTextW(hWnd, buffer, textlen + 1);
    buffer[textlen] = '\0';
    retstr = w32tostr(buffer, textlen);
    g_free(buffer);
    return retstr;
  } else {
    gchar *buffer;

    buffer = g_new0(gchar, textlen + 1);
    GetWindowTextA(hWnd, buffer, textlen + 1);
    buffer[textlen] = '\0';
    return buffer;
  }
}

int myDrawText(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect,
               UINT uFormat)
{
  int retval;

  if (unicode_support) {
    gunichar2 *text;

    text = strtow32(lpString, nCount);
    retval = DrawTextW(hDC, text, -1, lpRect, uFormat);
    g_free(text);
  } else {
    retval = DrawTextA(hDC, lpString, nCount, lpRect, uFormat);
  }
  return retval;
}

static BOOL makeMenuItemInfoW(LPMENUITEMINFOW lpmiiw, LPMENUITEMINFO lpmii)
{
  BOOL strdata;
  strdata = (lpmii->fMask & MIIM_TYPE)
            && !(lpmii->fType & (MFT_BITMAP | MFT_SEPARATOR));

  lpmiiw->cbSize = sizeof(MENUITEMINFOW);
  lpmiiw->fMask = lpmii->fMask;
  lpmiiw->fType = lpmii->fType;
  lpmiiw->fState = lpmii->fState;
  lpmiiw->wID = lpmii->wID;
  lpmiiw->hSubMenu = lpmii->hSubMenu;
  lpmiiw->hbmpChecked = lpmii->hbmpChecked;
  lpmiiw->hbmpUnchecked = lpmii->hbmpUnchecked;
  lpmiiw->dwItemData = lpmii->dwItemData;
  lpmiiw->dwTypeData = strdata ? strtow32(lpmii->dwTypeData, -1)
                               : (LPWSTR)lpmii->dwTypeData;
  lpmiiw->cch = lpmii->cch;
  return strdata;
}

BOOL WINAPI mySetMenuItemInfo(HMENU hMenu, UINT uItem, BOOL fByPosition,
                              LPMENUITEMINFO lpmii)
{
  BOOL retval;

  if (unicode_support) {
    MENUITEMINFOW miiw;
    BOOL strdata;

    strdata = makeMenuItemInfoW(&miiw, lpmii);
    retval = SetMenuItemInfoW(hMenu, uItem, fByPosition, &miiw);
    if (strdata) {
      g_free(miiw.dwTypeData);
    }
  } else {
    retval = SetMenuItemInfoA(hMenu, uItem, fByPosition, lpmii);
  }
  return retval;
}

BOOL WINAPI myInsertMenuItem(HMENU hMenu, UINT uItem, BOOL fByPosition,
                             LPMENUITEMINFO lpmii)
{
  BOOL retval;

  if (unicode_support) {
    MENUITEMINFOW miiw;
    BOOL strdata;

    strdata = makeMenuItemInfoW(&miiw, lpmii);
    retval = InsertMenuItemW(hMenu, uItem, fByPosition, &miiw);
    if (strdata) {
      g_free(miiw.dwTypeData);
    }
  } else {
    retval = InsertMenuItemA(hMenu, uItem, fByPosition, lpmii);
  }
  return retval;
}

static BOOL makeHeaderItemW(HD_ITEMW *phdiw, const HD_ITEM *phdi)
{
  BOOL strdata;

  strdata = phdi->mask & HDI_TEXT;

  phdiw->mask = phdi->mask;
  phdiw->cxy = phdi->cxy;
  phdiw->pszText = strdata ? strtow32(phdi->pszText, -1)
                           : (LPWSTR)phdi->pszText;
  phdiw->hbm = phdi->hbm;
  phdiw->cchTextMax = phdi->cchTextMax;
  phdiw->fmt = phdi->fmt;
  phdiw->lParam = phdi->lParam;
  return strdata;
}

static BOOL makeTabItemW(TC_ITEMW *tiew, const TC_ITEM *tie)
{
  BOOL strdata;

  strdata = tie->mask & TCIF_TEXT;
  tiew->mask = tie->mask;
  tiew->pszText = strdata ? strtow32(tie->pszText, -1)
                          : (LPWSTR)tie->pszText;
  tiew->cchTextMax = tie->cchTextMax;
  tiew->iImage = tie->iImage;
  tiew->lParam = tie->lParam;
  return strdata;
}

int myHeader_InsertItem(HWND hWnd, int index, const HD_ITEM *phdi)
{
  int retval;
  if (unicode_support && IsWindowUnicode(hWnd)) {
    HD_ITEMW hdiw;
    BOOL strdata;

    strdata = makeHeaderItemW(&hdiw, phdi);
    retval = (int)SendMessageW(hWnd, HDM_INSERTITEMW, (WPARAM)index,
                               (LPARAM)&hdiw);
    if (strdata) {
      g_free(hdiw.pszText);
    }
  } else {
    retval = (int)SendMessageA(hWnd, HDM_INSERTITEM, (WPARAM)index,
                               (LPARAM)phdi);
  }
  return retval;
}

int myTabCtrl_InsertItem(HWND hWnd, int index, const TC_ITEM *pitem)
{
  int retval;
  if (unicode_support) {
    TC_ITEMW tiew;
    BOOL strdata;
    strdata = makeTabItemW(&tiew, pitem);
    retval = (int)SendMessageW(hWnd, TCM_INSERTITEMW, (WPARAM)index,
                               (LPARAM)&tiew);
    if (strdata) {
      g_free(tiew.pszText);
    }
  } else {
    retval = (int)SendMessageA(hWnd, TCM_INSERTITEMW, (WPARAM)index,
                               (LPARAM)pitem);
  }
  return retval;
}

ATOM myRegisterClass(CONST WNDCLASS *lpWndClass)
{
  ATOM retval;

  if (unicode_support) {
    WNDCLASSW wcw;

    wcw.style = lpWndClass->style;
    wcw.lpfnWndProc = lpWndClass->lpfnWndProc;
    wcw.cbClsExtra = lpWndClass->cbClsExtra;
    wcw.cbWndExtra = lpWndClass->cbWndExtra;
    wcw.hInstance = lpWndClass->hInstance;
    wcw.hIcon = lpWndClass->hIcon;
    wcw.hCursor = lpWndClass->hCursor;
    wcw.hbrBackground = lpWndClass->hbrBackground;
    wcw.lpszMenuName = strtow32(lpWndClass->lpszMenuName, -1);
    wcw.lpszClassName = strtow32(lpWndClass->lpszClassName, -1);
    retval = RegisterClassW(&wcw);
    g_free((LPWSTR)wcw.lpszMenuName);
    g_free((LPWSTR)wcw.lpszClassName);
  } else {
    retval = RegisterClassA(lpWndClass);
  }
  return retval;
}

HWND myCreateDialog(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent,
                    DLGPROC lpDialogFunc)
{
  HWND retval;

  if (unicode_support) {
    gunichar2 *text;
    text = strtow32(lpTemplate, -1);
    retval = CreateDialogW(hInstance, text, hWndParent, lpDialogFunc);
    g_free(text);
  } else {
    retval = CreateDialogA(hInstance, lpTemplate, hWndParent, lpDialogFunc);
  }
  return retval;
}

LRESULT mySendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  if (unicode_support) {
    return SendMessageW(hWnd, Msg, wParam, lParam);
  } else {
    return SendMessageA(hWnd, Msg, wParam, lParam);
  }
}

void myEditReplaceSel(HWND hWnd, BOOL fCanUndo, LPCSTR lParam)
{
  if (unicode_support) {
    gunichar2 *text;
    text = strtow32(lParam, -1);
    SendMessageW(hWnd, EM_REPLACESEL, (WPARAM)fCanUndo, (LPARAM)text);
    g_free(text);
  } else {
    SendMessageA(hWnd, EM_REPLACESEL, (WPARAM)fCanUndo, (LPARAM)lParam);
  }
}

LONG mySetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong)
{
  if (unicode_support) {
    return SetWindowLongW(hWnd, nIndex, dwNewLong);
  } else {
    return SetWindowLongA(hWnd, nIndex, dwNewLong);
  }
}

LONG myGetWindowLong(HWND hWnd, int nIndex)
{
  if (unicode_support) {
    return GetWindowLongW(hWnd, nIndex);
  } else {
    return GetWindowLongA(hWnd, nIndex);
  }
}

LRESULT myCallWindowProc(WNDPROC lpPrevWndProc, HWND hWnd, UINT Msg,
                         WPARAM wParam, LPARAM lParam)
{
  if (unicode_support) {
    return CallWindowProcW(lpPrevWndProc, hWnd, Msg, wParam, lParam);
  } else {
    return CallWindowProcA(lpPrevWndProc, hWnd, Msg, wParam, lParam);
  }
}

LRESULT myDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  if (unicode_support) {
    return DefWindowProcW(hWnd, Msg, wParam, lParam);
  } else {
    return DefWindowProcA(hWnd, Msg, wParam, lParam);
  }
}

int myMessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
  int retval;

  if (unicode_support) {
    gunichar2 *text, *caption;
    text = strtow32(lpText, -1);
    caption = strtow32(lpCaption, -1);
    retval = MessageBoxW(hWnd, text, caption, uType);
    g_free(text);
    g_free(caption);
  } else {
    retval = MessageBoxA(hWnd, lpText, lpCaption, uType);
  }
  return retval;
}

BOOL myGetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin,
                  UINT wMsgFilterMax)
{
  if (unicode_support) {
    return GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
  } else {
    return GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
  }
}

LONG myDispatchMessage(CONST MSG *lpmsg)
{
  if (unicode_support) {
    return DispatchMessageW(lpmsg);
  } else {
    return DispatchMessageA(lpmsg);
  }
}

BOOL myIsDialogMessage(HWND hDlg, LPMSG lpMsg)
{
  if (unicode_support) {
    return IsDialogMessageW(hDlg, lpMsg);
  } else {
    return IsDialogMessageA(hDlg, lpMsg);
  }
}

size_t myw32strlen(const char *str)
{
  if (unicode_support) {
    return g_utf8_strlen(str, -1);
  } else {
    return strlen(str);
  }
}

LRESULT myComboBox_AddString(HWND hWnd, LPCTSTR text)
{
  LRESULT retval;
  if (unicode_support) {
    gunichar2 *w32text;
    w32text = strtow32(text, -1);
    retval = SendMessageW(hWnd, CB_ADDSTRING, 0, (LPARAM)w32text);
    g_free(w32text);
  } else {
    retval = SendMessageA(hWnd, CB_ADDSTRING, 0, (LPARAM)text);
  }
  return retval;
}

#endif /* CYGWIN */
