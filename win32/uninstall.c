/************************************************************************
 * uninstall.c    Simple Win32 uninstaller for dopewars                 *
 * Copyright (C)  2001-2002  Ben Webb                                   *
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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contid.h"
#include "guifunc.h"
#include "util.h"

HINSTANCE hInst;
HWND mainDlg;
char *product;

char *GetProduct(void)
{
  char *product;

  product = strrchr(GetCommandLine(), ' ');
  if (product && strlen(product + 1) > 0)
    return bstrdup(++product);
  else {
    DisplayError("This program should be called with a product ID",
                 FALSE, TRUE);
    ExitProcess(1);
  }
}

DWORD WINAPI DoUninstall(LPVOID lpParam)
{
  InstData *idata;
  HANDLE fin;
  HWND delstat;
  bstr *str;
  char *startmenu, *desktop, *installdir;

  installdir = GetInstallDir(product);

  if (!SetCurrentDirectory(installdir)) {
    str = bstr_new();
    bstr_assign(str, "Could not access install directory ");
    bstr_append(str, installdir);
    DisplayError(str->text, TRUE, TRUE);
    /* Pointless to try to free the bstr, since DisplayError ends the
     * process */
  }

  fin = CreateFile("install.log", GENERIC_READ, 0, NULL,
                   OPEN_EXISTING, 0, NULL);

  if (fin) {
    idata = ReadOldInstData(fin, product, installdir);
    CloseHandle(fin);
    DeleteFile("install.log");

    RemoveService(idata->service);

    delstat = GetDlgItem(mainDlg, ST_DELSTAT);
    DeleteFileList(idata->instfiles, delstat, NULL);
    DeleteFileList(idata->extrafiles, delstat, NULL);

    startmenu = GetStartMenuDir(idata->flags & IF_ALLUSERS, idata);
    desktop = GetDesktopDir();
    DeleteLinkList(startmenu, idata->startmenu, delstat);
    DeleteLinkList(desktop, idata->desktop, delstat);

    RemoveUninstall(startmenu, product, TRUE);

    SetCurrentDirectory(desktop);       /* Just make sure we're not in the 
                                         * install directory any more */

    str = bstr_new();
    if (!RemoveWholeDirectory(installdir)) {
      bstr_assign(str, "Could not remove install directory:\n");
      bstr_append(str, installdir);
      bstr_append(str, "\nYou may wish to manually remove it later.");
      DisplayError(str->text, FALSE, FALSE);
    }

    if (!RemoveWholeDirectory(startmenu)) {
      bstr_assign(str, "Could not remove Start Menu directory:\n");
      bstr_append(str, startmenu);
      bstr_append(str, "\nYou may wish to manually remove it later.");
      DisplayError(str->text, FALSE, FALSE);
    }

    bstr_assign(str, UninstallKey);
    bstr_appendpath(str, product);
    RegDeleteKey(HKEY_LOCAL_MACHINE, str->text);
    bstr_free(str, TRUE);

    bfree(startmenu);
    bfree(desktop);
    FreeInstData(idata, TRUE);
  } else {
    bfree(product);
    bfree(installdir);          /* Normally FreeInstData frees these */
    str = bstr_new();
    bstr_assign(str, "Could not read install.log from ");
    bstr_append(str, installdir);
    DisplayError(str->text, TRUE, TRUE);
    /* Pointless to try to free the bstr, since DisplayError ends the
     * process */
  }
  ShowWindow(GetDlgItem(mainDlg, ST_DELDONE), SW_SHOW);
  EnableWindow(GetDlgItem(mainDlg, BT_DELOK), TRUE);
  SetFocus(GetDlgItem(mainDlg, BT_DELOK));
  return 0;
}

BOOL CALLBACK MainDlgProc(HWND hWnd, UINT msg, WPARAM wParam,
                          LPARAM lParam)
{
  HANDLE hThread;
  DWORD threadID;
  static BOOL startedun = FALSE;

  switch (msg) {
  case WM_INITDIALOG:
    return TRUE;
  case WM_SHOWWINDOW:
    if (wParam && !startedun) {
      startedun = TRUE;
      hThread = CreateThread(NULL, 0, DoUninstall, NULL, 0, &threadID);
    }
    return TRUE;
  case WM_COMMAND:
    if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == BT_DELOK
        && lParam) {
      PostQuitMessage(0);
      return TRUE;
    }
    break;
  case WM_CLOSE:
    PostQuitMessage(0);
    return TRUE;
  }
  return FALSE;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpszCmdParam, int nCmdShow)
{
  MSG msg;
  bstr *str;

  product = GetProduct();

  str = bstr_new();
  bstr_assign(str, "Are you sure you want to uninstall ");
  bstr_append(str, product);
  bstr_append(str, " ?");
  if (MessageBox(NULL, str->text, "Uninstall", MB_YESNO) == IDNO)
    return (1);
  bstr_free(str, TRUE);

  hInst = hInstance;
  if (!hPrevInstance)
    RegisterSepClass(hInstance);

  mainDlg = CreateDialog(hInstance, MAKEINTRESOURCE(1), NULL, MainDlgProc);
  SetGuiFont(mainDlg);

  EnableWindow(GetDlgItem(mainDlg, BT_DELOK), FALSE);
  ShowWindow(mainDlg, SW_SHOW);
  ShowWindow(GetDlgItem(mainDlg, ST_DELDONE), SW_HIDE);

  while (GetMessage(&msg, NULL, 0, 0)) {
    if (!IsDialogMessage(mainDlg, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return 0;
}
