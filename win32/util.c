/************************************************************************
 * util.c         Shared functions for Win32 installer programs         *
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
#include <string.h>
#include <shlobj.h>
#include "util.h"

const char *UninstallKey =
    "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
const char *UninstallEXE = "bw-uninstall.exe";

static void bstr_append_dir(bstr *str, BOOL windir);

void *bmalloc(UINT numbytes)
{
  HLOCAL localpt;

  if (numbytes <= 0)
    numbytes = 1;

  localpt = LocalAlloc(LMEM_FIXED, numbytes);
  if (localpt)
    return (void *)localpt;
  else {
    DisplayError("Could not allocate memory: ", TRUE, TRUE);
    ExitProcess(1);
  }
}

void bfree(void *pt)
{
  if (!pt)
    return;
  if (LocalFree((HLOCAL) pt)) {
    DisplayError("Could not free memory: ", TRUE, TRUE);
  }
}

void *brealloc(void *pt, UINT numbytes)
{
  UINT numcp;
  void *newpt;

  if (!pt && numbytes)
    return bmalloc(numbytes);
  else if (pt && !numbytes) {
    bfree(pt);
  } else if (pt && numbytes) {
    newpt = bmalloc(numbytes);
    memset(newpt, 0, numbytes);
    numcp = LocalSize((HLOCAL) pt);
    if (numbytes < numcp)
      numcp = numbytes;
    memcpy((char *)newpt, (char *)pt, numcp);
    bfree(pt);
    return newpt;
  }
  return NULL;
}

char *bstrdup(char *str)
{
  char *newstr;

  if (str) {
    newstr = bmalloc(strlen(str) + 1);
    strcpy(newstr, str);
  } else {
    newstr = bmalloc(1);
    newstr[0] = '\0';
  }
  return newstr;
}

bstr *bstr_new(void)
{
  bstr *str;
  str = bmalloc(sizeof(bstr));
  str->bufsiz = 64;
  str->length = 0;
  str->text = bmalloc(str->bufsiz);
  str->text[0] = '\0';
  return str;
}

void bstr_free(bstr *str, BOOL free_text)
{
  if (free_text)
    bfree(str->text);
  bfree(str);
}

void bstr_expandby(bstr *str, unsigned extralength)
{
  if (str->bufsiz >= str->length + 1 + extralength)
    return;

  while (str->bufsiz < str->length + 1 + extralength)
    str->bufsiz *= 2;
  str->text = brealloc(str->text, str->bufsiz);
}

void bstr_setlength(bstr *str, unsigned length)
{
  if (length <= str->length) {
    str->length = length;
    str->text[length] = '\0';
  } else {
    bstr_expandby(str, length - str->length);
  }
}

void bstr_assign(bstr *str, const char *text)
{
  if (text) {
    bstr_setlength(str, strlen(text));
    strcpy(str->text, text);
    str->length = strlen(text);
  } else {
    bstr_setlength(str, 0);
  }
}

void bstr_append(bstr *str, const char *text)
{
  if (!text)
    return;
  bstr_expandby(str, strlen(text));
  strcat(str->text, text);
  str->length += strlen(text);
}

void bstr_appendpath(bstr *str, const char *text)
{
  if (str->length && str->text[str->length - 1] != '\\') {
    bstr_append_c(str, '\\');
  }
  bstr_append(str, text);
}

void bstr_append_c(bstr *str, char ch)
{
  bstr_expandby(str, 1);
  str->text[str->length] = ch;
  str->length++;
  str->text[str->length] = '\0';
}

/* We can be pretty confident that this is enough space for an integer */
#define MAX_LENGTH_INT 80

void bstr_append_long(bstr *str, long val)
{
  char tmpbuf[MAX_LENGTH_INT];

  sprintf(tmpbuf, "%ld", val);
  bstr_append(str, tmpbuf);
}

void bstr_append_windir(bstr *str)
{
  bstr_append_dir(str, TRUE);
}

void bstr_append_curdir(bstr *str)
{
  bstr_append_dir(str, FALSE);
}

void bstr_assign_windir(bstr *str)
{
  bstr_setlength(str, 0);
  bstr_append_windir(str);
}

void bstr_assign_curdir(bstr *str)
{
  bstr_setlength(str, 0);
  bstr_append_curdir(str);
}

void bstr_append_dir(bstr *str, BOOL windir)
{
  unsigned spaceleft;
  UINT retval;
  int tries;

  spaceleft = str->bufsiz - str->length;        /* spaceleft includes the
                                                 * null */

  for (tries = 0; tries < 5; tries++) {
    if (windir) {
      retval = GetWindowsDirectory(str->text + str->length, spaceleft);
    } else {
      retval = GetCurrentDirectory(spaceleft, str->text + str->length);
    }
    if (retval == 0)
      DisplayError("Cannot get directory: ", TRUE, TRUE);
    if (retval <= spaceleft - 1) {
      str->length += retval;
      break;
    }
    bstr_expandby(str, retval); /* Let's err on the side of caution */
    spaceleft = str->bufsiz - str->length;
  }
  if (tries >= 5)
    DisplayError("Cannot get directory: ", TRUE, TRUE);
}

void DisplayError(const char *errtext, BOOL addsyserr, BOOL fatal)
{
  DWORD syserr;
  bstr *str;
  LPTSTR lpMsgBuf;

  syserr = GetLastError();

  str = bstr_new();
  bstr_assign(str, errtext);

  if (addsyserr) {
    bstr_append(str, "; ");
    bstr_append_long(str, syserr);
    bstr_append(str, ": ");
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM, NULL, syserr,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf, 0, NULL);
    bstr_append(str, lpMsgBuf);
    LocalFree(lpMsgBuf);
  }

  MessageBox(NULL, str->text, fatal ? "Fatal Error" : "Error",
             MB_OK | MB_ICONSTOP);
  if (fatal)
    ExitProcess(1);
}

void AddInstFiles(char *filename, DWORD filesize, InstFiles **lastpt,
                  InstFiles **firstpt)
{
  InstFiles *newpt;

  newpt = bmalloc(sizeof(InstFiles));
  if (*lastpt) {
    (*lastpt)->next = newpt;
  } else {
    *firstpt = newpt;
  }
  *lastpt = newpt;
  newpt->next = NULL;
  newpt->filename = filename;
  newpt->filesize = filesize;
}

void AddInstLink(char *linkfile, char *origfile, char *args,
                 InstLink **lastpt, InstLink **firstpt)
{
  InstLink *newpt;

  newpt = bmalloc(sizeof(InstLink));
  if (*lastpt) {
    (*lastpt)->next = newpt;
  } else {
    *firstpt = newpt;
  }
  *lastpt = newpt;
  newpt->next = NULL;
  newpt->linkfile = linkfile;
  newpt->origfile = origfile;
  newpt->args = args;
}

void FreeLinkList(InstLink *linklist, BOOL freepts)
{
  InstLink *thispt, *nextpt;

  thispt = linklist;
  while (thispt) {
    nextpt = thispt->next;

    if (freepts) {
      bfree(thispt->linkfile);
      bfree(thispt->origfile);
      bfree(thispt->args);
    }
    bfree(thispt);

    thispt = nextpt;
  }
}

void FreeFileList(InstFiles *filelist, BOOL freepts)
{
  InstFiles *thispt, *nextpt;

  thispt = filelist;
  while (thispt) {
    nextpt = thispt->next;

    if (freepts)
      bfree(thispt->filename);
    bfree(thispt);

    thispt = nextpt;
  }
}

void AddServiceDetails(char *servicename, char *servicedisp,
                       char *servicedesc, char *serviceexe,
                       NTService **service)
{
  *service = bmalloc(sizeof(NTService));
  (*service)->name = servicename;
  (*service)->display = servicedisp;
  (*service)->description = servicedesc;
  (*service)->exe = serviceexe;
}

void FreeServiceDetails(NTService *service, BOOL freepts)
{
  if (!service)
    return;

  if (freepts) {
    bfree(service->name);
    bfree(service->display);
    bfree(service->description);
    bfree(service->exe);
  }
  bfree(service);
}

void FreeInstData(InstData *idata, BOOL freepts)
{
  FreeFileList(idata->instfiles, freepts);
  FreeFileList(idata->extrafiles, freepts);
  FreeFileList(idata->keepfiles, freepts);

  FreeLinkList(idata->startmenu, freepts);
  FreeLinkList(idata->desktop, freepts);

  FreeServiceDetails(idata->service, freepts);

  bfree(idata->product);
  bfree(idata->installdir);
  bfree(idata->startmenudir);

  bfree(idata);
}

void WriteInstFlags(HANDLE fout, InstFlags flags)
{
  DWORD bytes_written;
  char str[3];

  str[0] = (char)flags;
  if (!WriteFile(fout, str, 1, &bytes_written, NULL)) {
    printf("Write error\n");
  }
}

void WriteServiceDetails(HANDLE fout, NTService *service)
{
  DWORD bytes_written;
  char str[] = "";

  if (!service) {
    if (!WriteFile(fout, str, strlen(str) + 1, &bytes_written, NULL)) {
      printf("Write error\n");
    }
  } else {
    if (!WriteFile(fout, service->name, strlen(service->name) + 1,
                   &bytes_written, NULL)) {
      printf("Write error\n");
    }
    if (!WriteFile(fout, service->display, strlen(service->display) + 1,
                   &bytes_written, NULL)) {
      printf("Write error\n");
    }
    if (!WriteFile
        (fout, service->description, strlen(service->description) + 1,
         &bytes_written, NULL)) {
      printf("Write error\n");
    }
    if (!WriteFile(fout, service->exe, strlen(service->exe) + 1,
                   &bytes_written, NULL)) {
      printf("Write error\n");
    }
  }
}

void WriteLinkList(HANDLE fout, InstLink *listpt)
{
  char str[] = "";
  DWORD bytes_written;

  for (; listpt; listpt = listpt->next) {
    if (!WriteFile(fout, listpt->linkfile, strlen(listpt->linkfile) + 1,
                   &bytes_written, NULL)) {
      printf("Write error\n");
    }
    if (!WriteFile(fout, listpt->origfile, strlen(listpt->origfile) + 1,
                   &bytes_written, NULL)) {
      printf("Write error\n");
    }
    if (!WriteFile(fout, listpt->args, strlen(listpt->args) + 1,
                   &bytes_written, NULL)) {
      printf("Write error\n");
    }
  }
  if (!WriteFile(fout, str, strlen(str) + 1, &bytes_written, NULL)) {
    printf("Write error\n");
  }
}

void WriteFileList(HANDLE fout, InstFiles *listpt)
{
  bstr *str;
  DWORD bytes_written;

  str = bstr_new();

  for (; listpt; listpt = listpt->next) {
    if (!WriteFile(fout, listpt->filename, strlen(listpt->filename) + 1,
                   &bytes_written, NULL)) {
      printf("Write error\n");
    }
    bstr_setlength(str, 0);
    bstr_append_long(str, listpt->filesize);
    if (!WriteFile(fout, str->text, str->length + 1, &bytes_written, NULL)) {
      printf("Write error\n");
    }
  }
  bstr_assign(str, "");
  if (!WriteFile(fout, str->text, str->length + 1, &bytes_written, NULL)) {
    printf("Write error\n");
  }
  bstr_free(str, TRUE);
}

static char *GetSpecialDir(int dirtype)
{
  LPITEMIDLIST pidl;
  LPMALLOC pmalloc;
  char szDir[MAX_PATH];
  BOOL doneOK = FALSE;

  if (SUCCEEDED(SHGetMalloc(&pmalloc))) {
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, dirtype, &pidl))) {
      if (SHGetPathFromIDList(pidl, szDir))
        doneOK = TRUE;
      pmalloc->lpVtbl->Free(pmalloc, pidl);
    }
    pmalloc->lpVtbl->Release(pmalloc);
  }
  return (doneOK ? bstrdup(szDir) : NULL);
}

char *GetStartMenuTopDir(BOOL AllUsers)
{
  return GetSpecialDir(AllUsers ? CSIDL_COMMON_STARTMENU :
                       CSIDL_STARTMENU);
}

char *GetDesktopDir(void)
{
  return GetSpecialDir(CSIDL_DESKTOPDIRECTORY);
}

char *GetStartMenuDir(BOOL AllUsers, InstData *idata)
{
  bstr *str;
  char *topdir, *retval;

  topdir = GetStartMenuTopDir(AllUsers);

  str = bstr_new();

  bstr_assign(str, topdir);
  bfree(topdir);

  bstr_appendpath(str, "Programs");
  bstr_appendpath(str, idata->startmenudir);

  retval = str->text;
  bstr_free(str, FALSE);
  return retval;
}

BOOL CreateWholeDirectory(char *path)
{
  char *pt;

  if (!path)
    return FALSE;

  /* We may as well try the easy way first */
  if (CreateDirectory(path, NULL))
    return TRUE;

  /* \\machine\share notation */
  if (strlen(path) > 2 && path[0] == '\\' && path[1] == '\\') {
    pt = &path[2];              /* Skip initial "\\" */
    while (*pt && *pt != '\\')
      pt++;                     /* Skip the machine name */
    /* X: notation */
  } else if (strlen(path) > 2 && path[1] == ':') {
    pt = &path[2];              /* Skip the X: part */
  } else {
    pt = path;
  }


  while (*pt) {
    pt++;
    if (*pt == '\\') {
      *pt = '\0';
      if (!CreateDirectory(path, NULL)
          && GetLastError() != ERROR_ALREADY_EXISTS) {
        *pt = '\\';
        return FALSE;
      }
      *pt = '\\';
    }
  }
  return (CreateDirectory(path, NULL)
          || GetLastError() == ERROR_ALREADY_EXISTS);
}

BOOL RemoveWholeDirectory(char *path)
{
  char *pt;
  BOOL retval;

  if (!path || !RemoveDirectory(path))
    return FALSE;

  for (pt = &path[strlen(path) - 2]; pt > path; pt--) {
    if (*pt == '\\') {
      *pt = '\0';
      retval = RemoveDirectory(path);
      *pt = '\\';
      if (!retval)
        break;
    }
  }
  return TRUE;
}

void RemoveService(NTService *service)
{
  SC_HANDLE scManager, scService;
  SERVICE_STATUS status;

  if (!service)
    return;

  scManager = OpenSCManager(NULL, NULL, GENERIC_READ);

  if (!scManager) {
    DisplayError("Cannot connect to service manager", TRUE, FALSE);
    return;
  }

  scService = OpenService(scManager, service->name, DELETE | SERVICE_STOP);
  if (!scService) {
    DisplayError("Cannot open service", TRUE, FALSE);
  } else {
    if (!ControlService(scService, SERVICE_CONTROL_STOP, &status) &&
        GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
      DisplayError("Cannot stop service", TRUE, FALSE);
    }
    if (!DeleteService(scService)) {
      DisplayError("Cannot delete service", TRUE, FALSE);
    }
    CloseServiceHandle(scService);
  }

  CloseServiceHandle(scManager);
}

char *read_line0(HANDLE hin)
{
  char *buf;
  int bufsize = 32, strind = 0;
  DWORD bytes_read;

  buf = bmalloc(bufsize);

  while (1) {
    if (!ReadFile(hin, &buf[strind], 1, &bytes_read, NULL)) {
      printf("Read error\n");
      break;
    }
    if (bytes_read == 0) {
      buf[strind] = '\0';
      break;
    } else if (buf[strind] == '\0')
      break;
    else {
      strind++;
      if (strind >= bufsize) {
        bufsize *= 2;
        buf = brealloc(buf, bufsize);
      }
    }
  }
  if (strind == 0) {
    bfree(buf);
    return NULL;
  } else
    return buf;
}

InstLink *ReadLinkList(HANDLE fin)
{
  InstLink *first = NULL, *listpt = NULL, *newpt;
  char *linkfile, *origfile, *args;

  while (1) {
    linkfile = read_line0(fin);
    if (!linkfile)
      break;
    origfile = read_line0(fin);
    args = read_line0(fin);
    if (!origfile)
      DisplayError("Corrupt install.log", FALSE, TRUE);
    newpt = bmalloc(sizeof(InstLink));
    if (listpt)
      listpt->next = newpt;
    else
      first = newpt;
    listpt = newpt;
    newpt->next = NULL;
    newpt->linkfile = linkfile;
    newpt->origfile = origfile;
    newpt->args = args;
  }
  return first;
}

InstFlags ReadInstFlags(HANDLE fin)
{
  DWORD bytes_read;
  char buf[3];

  buf[0] = 0;
  if (!ReadFile(fin, buf, 1, &bytes_read, NULL)) {
    printf("Read error\n");
  }

  return (InstFlags)buf[0];
}

NTService *ReadServiceDetails(HANDLE fin)
{
  NTService *service = NULL;
  char *name, *disp, *desc, *exe;

  name = read_line0(fin);
  if (name) {
    disp = read_line0(fin);
    desc = read_line0(fin);
    exe = read_line0(fin);
    if (!disp || !desc || !exe) {
      DisplayError("Corrupt install.log", FALSE, TRUE);
    } else {
      AddServiceDetails(name, disp, desc, exe, &service);
    }
  }

  return service;
}

InstFiles *ReadFileList(HANDLE fin)
{
  InstFiles *first = NULL, *listpt = NULL, *newpt;
  char *filename, *filesize;

  while (1) {
    filename = read_line0(fin);
    if (!filename)
      break;
    filesize = read_line0(fin);
    if (!filesize)
      DisplayError("Corrupt install.log", FALSE, TRUE);
    newpt = bmalloc(sizeof(InstFiles));
    if (listpt)
      listpt->next = newpt;
    else
      first = newpt;
    listpt = newpt;

    newpt->next = NULL;
    newpt->filename = filename;
    newpt->filesize = atol(filesize);
    bfree(filesize);
  }
  return first;
}

char *GetInstallDir(char *product)
{
  HKEY key;
  bstr *str;
  DWORD keytype, keylen;
  char *installdir;

  str = bstr_new();
  bstr_assign(str, UninstallKey);
  bstr_appendpath(str, product);
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, str->text, 0,
                   KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
    DisplayError("Could not open registry", FALSE, TRUE);
  }

  if (RegQueryValueEx(key, "InstallDirectory", NULL,
                      &keytype, NULL, &keylen) != ERROR_SUCCESS ||
      keytype != REG_SZ) {
    DisplayError("Could not query registry key", FALSE, TRUE);
  }

  installdir = bmalloc(keylen);
  if (RegQueryValueEx(key, "InstallDirectory", NULL,
                      &keytype, installdir, &keylen) != ERROR_SUCCESS) {
    DisplayError("Could not get registry key value", FALSE, TRUE);
  }

  bstr_free(str, TRUE);
  return installdir;
}

InstData *ReadOldInstData(HANDLE fin, char *product, char *installdir)
{
  InstData *idata;

  idata = bmalloc(sizeof(InstData));

  idata->product = product;
  idata->installdir = installdir;
  idata->startmenudir = read_line0(fin);

  idata->instfiles = ReadFileList(fin);
  idata->extrafiles = ReadFileList(fin);
  idata->keepfiles = NULL;

  idata->startmenu = ReadLinkList(fin);
  idata->desktop = ReadLinkList(fin);

  idata->service = ReadServiceDetails(fin);

  idata->flags = ReadInstFlags(fin);

  return idata;
}

void DeleteFileList(InstFiles *listpt, HWND hwnd, InstFiles *keepfiles)
{
  bstr *str;
  char *sep;
  InstFiles *keeppt;

  str = bstr_new();
  for (; listpt; listpt = listpt->next) {
    keeppt = keepfiles;
    while (keeppt && strcmp(keeppt->filename, listpt->filename) != 0) {
      keeppt = keeppt->next;
    }
    if (keeppt) {
      keeppt->filesize = 1;     /* Mark that this file is already
                                 * installed */
    } else {
      bstr_assign(str, "Deleting file: ");
      bstr_append(str, listpt->filename);
      SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)str->text);
      DeleteFile(listpt->filename);
      sep = strrchr(listpt->filename, '\\');
      if (sep) {
        *sep = '\0';
        RemoveWholeDirectory(listpt->filename);
        *sep = '\\';
      }
    }
  }
  bstr_free(str, TRUE);
}

void DeleteLinkList(char *dir, InstLink *listpt, HWND hwnd)
{
  bstr *str;

  str = bstr_new();
  if (SetCurrentDirectory(dir)) {
    for (; listpt; listpt = listpt->next) {
      bstr_assign(str, "Deleting shortcut: ");
      bstr_append(str, listpt->linkfile);
      SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)str->text);
      DeleteFile(listpt->linkfile);
    }
  } else {
    bstr_assign(str, "Could not find shortcut directory ");
    bstr_append(str, dir);
    DisplayError(str->text, TRUE, FALSE);
  }
  bstr_free(str, TRUE);
}

void RemoveUninstall(char *startmenu, char *product, BOOL delexe)
{
  bstr *inipath, *uninstpath, *uninstlink;

  inipath = bstr_new();
  uninstpath = bstr_new();
  uninstlink = bstr_new();

  bstr_assign(uninstlink, startmenu);
  bstr_appendpath(uninstlink, "Uninstall ");
  bstr_append(uninstlink, product);
  bstr_append(uninstlink, ".LNK");
  DeleteFile(uninstlink->text);

  if (delexe) {
    bstr_assign_windir(inipath);
    bstr_assign(uninstpath, inipath->text);

    bstr_appendpath(inipath, "wininit.ini");
    bstr_appendpath(uninstpath, UninstallEXE);

    if (!WritePrivateProfileString("Renane", "NUL", uninstpath->text,
                                   inipath->text)) {
      DisplayError("Cannot write to wininit.ini: ", TRUE, FALSE);
    }
  }

  bstr_free(uninstlink, TRUE);
  bstr_free(uninstpath, TRUE);
  bstr_free(inipath, TRUE);
}
