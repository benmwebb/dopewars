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

char *read_line0(HANDLE hin) {
  char *buf;
  int bufsize=32,strind=0;
  DWORD bytes_read;
  buf = bmalloc(bufsize);

  while (1) {
    if (!ReadFile(hin,&buf[strind],1,&bytes_read,NULL)) {
      printf("Read error\n"); break;
    }
    if (bytes_read==0) { buf[strind]='\0'; break; } 
    else if (buf[strind]=='\0') break;
    else {
      strind++;
      if (strind>=bufsize) {
        bufsize*=2;
        buf = brealloc(buf,bufsize);
      }
    }
  }
  if (strind==0) { bfree(buf); return NULL; }
  else return buf;
}

InstLink *ReadLinkList(HANDLE fin) {
  InstLink *first=NULL,*listpt=NULL,*newpt;
  char *linkfile,*origfile,*args;

  while (1) {
    linkfile=read_line0(fin);
    if (!linkfile) break;
    origfile=read_line0(fin);
    args=read_line0(fin);
    if (!origfile) DisplayError("Corrupt install.log",FALSE,TRUE);
    newpt = bmalloc(sizeof(InstLink));
    if (listpt) listpt->next = newpt;
    else first = newpt;
    listpt = newpt;
    newpt->next=NULL;
    newpt->linkfile=linkfile;
    newpt->origfile=origfile;
    newpt->args=args;
  }
  return first;
}

InstFiles *ReadFileList(HANDLE fin) {
  InstFiles *first=NULL,*listpt=NULL,*newpt;
  char *filename,*filesize;

  while (1) {
    filename=read_line0(fin);
    if (!filename) break;
    filesize=read_line0(fin);
    if (!filesize) DisplayError("Corrupt install.log",FALSE,TRUE);
    newpt = bmalloc(sizeof(InstFiles));
    if (listpt) listpt->next = newpt;
    else first = newpt;
    listpt = newpt;

    newpt->next=NULL;
    newpt->filename=filename;
    newpt->filesize=atol(filesize);
    bfree(filesize);
  }
  return first;
}

char *GetProduct(void) {
  char *product;
  product = strrchr(GetCommandLine(),' ');
  if (product && strlen(product+1)>0) return bstrdup(++product);
  else {
    DisplayError("This program should be called with a product ID",FALSE,TRUE);
    ExitProcess(1);
  }
}

char *GetInstallDir(char *product) {
  HKEY key;
  bstr *str;
  DWORD keytype,keylen;
  char *installdir;

  str=bstr_new();
  bstr_assign(str,UninstallKey);
  bstr_appendpath(str,product);
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,str->text,0,
                   KEY_ALL_ACCESS,&key)!=ERROR_SUCCESS) {
    DisplayError("Could not open registry",FALSE,TRUE);
  }

  if (RegQueryValueEx(key,"InstallDirectory",NULL,
                      &keytype,NULL,&keylen)!=ERROR_SUCCESS ||
      keytype!=REG_SZ) {
    DisplayError("Could not query registry key",FALSE,TRUE);
  }

  installdir = bmalloc(keylen);
  if (RegQueryValueEx(key,"InstallDirectory",NULL,
                      &keytype,installdir,&keylen)!=ERROR_SUCCESS) {
    DisplayError("Could not get registry key value",FALSE,TRUE);
  }

  bstr_free(str,TRUE);
  return installdir;
}

InstData *ReadInstData(HANDLE fin,char *product,char *installdir) {
  InstData *idata;

  idata=bmalloc(sizeof(InstData));

  idata->product=product;
  idata->installdir=installdir;
  idata->startmenudir=read_line0(fin);

  idata->instfiles = ReadFileList(fin);
  idata->extrafiles = ReadFileList(fin);

  idata->startmenu = ReadLinkList(fin);
  idata->desktop = ReadLinkList(fin);
  return idata;
}

void DeleteFileList(InstFiles *listpt) {
  bstr *str;
  str=bstr_new();
  for (;listpt;listpt=listpt->next) {
    bstr_assign(str,"Deleting file: ");
    bstr_append(str,listpt->filename);
    SendDlgItemMessage(mainDlg,ST_DELSTAT,WM_SETTEXT,0,(LPARAM)str->text);
    DeleteFile(listpt->filename);
  }
  bstr_free(str,TRUE);
}

void DeleteLinkList(char *dir,InstLink *listpt) {
  bstr *str;
  str=bstr_new();
  if (SetCurrentDirectory(dir)) {
    for (;listpt;listpt=listpt->next) {
      bstr_assign(str,"Deleting shortcut: ");
      bstr_append(str,listpt->linkfile);
      SendDlgItemMessage(mainDlg,ST_DELSTAT,WM_SETTEXT,0,(LPARAM)str->text);
      DeleteFile(listpt->linkfile);
    }
  } else {
    bstr_assign(str,"Could not find shortcut directory ");
    bstr_append(str,dir);
    DisplayError(str->text,TRUE,FALSE);
  }
  bstr_free(str,TRUE);
}

void RemoveUninstall(char *startmenu,char *product) {
  bstr *inipath,*uninstpath,*uninstlink;

  inipath=bstr_new();
  uninstpath=bstr_new();
  uninstlink=bstr_new();

  bstr_assign(uninstlink,startmenu);
  bstr_appendpath(uninstlink,"Uninstall ");
  bstr_append(uninstlink,product);
  bstr_append(uninstlink,".LNK");
  DeleteFile(uninstlink->text);

  bstr_assign_windir(inipath);
  bstr_assign(uninstpath,inipath->text);

  bstr_appendpath(inipath,"wininit.ini");
  bstr_appendpath(uninstpath,UninstallEXE);

  if (!WritePrivateProfileString("Renane","NUL",uninstpath->text,
                                 inipath->text)) {
    DisplayError("Cannot write to wininit.ini: ",TRUE,FALSE);
  }

  bstr_free(uninstlink,TRUE);
  bstr_free(uninstpath,TRUE);
  bstr_free(inipath,TRUE);
}

DWORD WINAPI DoUninstall(LPVOID lpParam) {
  InstData *idata;
  HANDLE fin;
  bstr *str;
  char *startmenu,*desktop,*installdir;

  installdir=GetInstallDir(product);

  if (!SetCurrentDirectory(installdir)) {
    str=bstr_new();
    bstr_assign(str,"Could not access install directory ");
    bstr_append(str,installdir);
    DisplayError(str->text,TRUE,TRUE);
/* Pointless to try to free the bstr, since DisplayError ends the process */
  }

  fin = CreateFile("install.log",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);

  if (fin) {
    idata = ReadInstData(fin,product,installdir);
    CloseHandle(fin);
    DeleteFile("install.log");
    DeleteFileList(idata->instfiles);
    DeleteFileList(idata->extrafiles);

    startmenu = GetStartMenuDir(idata);
    desktop = GetDesktopDir();
    DeleteLinkList(startmenu,idata->startmenu);
    DeleteLinkList(desktop,idata->desktop);

    RemoveUninstall(startmenu,product);

    SetCurrentDirectory(desktop); /* Just make sure we're not in the install
                                     directory any more */

    str=bstr_new();
    if (!RemoveWholeDirectory(installdir)) {
      bstr_assign(str,"Could not remove install directory:\n");
      bstr_append(str,installdir);
      bstr_append(str,"\nYou may wish to manually remove it later.");
      DisplayError(str->text,FALSE,FALSE);
    }

    if (!RemoveWholeDirectory(startmenu)) {
      bstr_assign(str,"Could not remove Start Menu directory:\n");
      bstr_append(str,startmenu);
      bstr_append(str,"\nYou may wish to manually remove it later.");
      DisplayError(str->text,FALSE,FALSE);
    }

    bstr_assign(str,UninstallKey);
    bstr_appendpath(str,product);
    RegDeleteKey(HKEY_LOCAL_MACHINE,str->text);
    bstr_free(str,TRUE);

    bfree(startmenu); bfree(desktop);
    FreeInstData(idata,TRUE);
  } else {
    bfree(product); bfree(installdir); /* Normally FreeInstData frees these */
    str=bstr_new();
    bstr_assign(str,"Could not read install.log from ");
    bstr_append(str,installdir);
    DisplayError(str->text,TRUE,TRUE);
/* Pointless to try to free the bstr, since DisplayError ends the process */
  }
  ShowWindow(GetDlgItem(mainDlg,ST_DELDONE),SW_SHOW);
  EnableWindow(GetDlgItem(mainDlg,BT_DELOK),TRUE);
  SetFocus(GetDlgItem(mainDlg,BT_DELOK));
  return 0;
}

BOOL CALLBACK MainDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) {
  HANDLE hThread;
  DWORD threadID;
  static BOOL startedun=FALSE;
  switch(msg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_SHOWWINDOW:
      if (wParam && !startedun) {
        startedun=TRUE;
        hThread = CreateThread(NULL,0,DoUninstall,NULL,0,&threadID);
      }
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==BT_DELOK && lParam) {
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

int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
                     LPSTR lpszCmdParam,int nCmdShow) {
  MSG msg;
  bstr *str;

  product=GetProduct();

  str=bstr_new();
  bstr_assign(str,"Are you sure you want to uninstall ");
  bstr_append(str,product);
  bstr_append(str," ?");
  if (MessageBox(NULL,str->text,"Uninstall",MB_YESNO)==IDNO) return(1);
  bstr_free(str,TRUE);

  hInst = hInstance;
  if (!hPrevInstance) RegisterSepClass(hInstance);

  mainDlg = CreateDialog(hInstance,MAKEINTRESOURCE(1),NULL,MainDlgProc);
  SetGuiFont(mainDlg);

  EnableWindow(GetDlgItem(mainDlg,BT_DELOK),FALSE);
  ShowWindow(mainDlg,SW_SHOW);
  ShowWindow(GetDlgItem(mainDlg,ST_DELDONE),SW_HIDE);

  while (GetMessage(&msg,NULL,0,0)) {
    if (!IsDialogMessage(mainDlg,&msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return 0;
}
