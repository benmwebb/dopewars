/************************************************************************
 * util.h         Shared functions for Win32 installer programs         *
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

typedef struct _bstr {
  char *text;
  unsigned length;              /* Length of current text, NOT including
                                 * terminating null */
  unsigned bufsiz;              /* Size of the allocated memory buffer */
} bstr;

typedef struct _InstFiles {
  char *filename;
  DWORD filesize;
  struct _InstFiles *next;
} InstFiles;

typedef struct _InstLink {
  char *linkfile;
  char *origfile;
  char *args;
  struct _InstLink *next;
} InstLink;

typedef struct _NTService {
  char *name;
  char *display;
  char *description;
  char *exe;
} NTService;

typedef enum {
  IF_ALLUSERS = 1
} InstFlags;

typedef struct _InstData {
  char *product;
  char *installdir, *startmenudir;
  DWORD totalsize;
  NTService *service;
  InstFiles *instfiles;
  InstFiles *extrafiles;
  InstFiles *keepfiles;
  InstLink *startmenu;
  InstLink *desktop;
  InstFlags flags;
} InstData;

extern const char *UninstallKey;
extern const char *UninstallEXE;

void *bmalloc(UINT numbytes);
void bfree(void *pt);
void *brealloc(void *pt, UINT numbytes);
char *bstrdup(char *str);

bstr *bstr_new(void);
void bstr_free(bstr *str, BOOL free_text);
void bstr_expandby(bstr *str, unsigned extralength);
void bstr_setlength(bstr *str, unsigned length);
void bstr_assign(bstr *str, const char *text);
void bstr_append(bstr *str, const char *text);
void bstr_appendpath(bstr *str, const char *text);
void bstr_append_c(bstr *str, const char ch);
void bstr_append_long(bstr *str, const long val);
void bstr_append_windir(bstr *str);
void bstr_append_curdir(bstr *str);
void bstr_assign_windir(bstr *str);
void bstr_assign_curdir(bstr *str);

void DisplayError(const char *errtext, BOOL addsyserr, BOOL fatal);
void AddInstFiles(char *filename, DWORD filesize, InstFiles **lastpt,
                  InstFiles **firstpt);
void AddInstLink(char *linkfile, char *origfile, char *args,
                 InstLink **lastpt, InstLink **firstpt);
void FreeLinkList(InstLink *linklist, BOOL freepts);
void FreeFileList(InstFiles *filelist, BOOL freepts);
void FreeInstData(InstData *idata, BOOL freepts);
void AddServiceDetails(char *servicename, char *servicedisp,
                       char *servicedesc, char *serviceexe,
                       NTService **service);
void FreeServiceDetails(NTService *service, BOOL freepts);
void WriteServiceDetails(HANDLE fout, NTService *service);
void WriteInstFlags(HANDLE fout, InstFlags flags);
void WriteLinkList(HANDLE fout, InstLink *listpt);
void WriteFileList(HANDLE fout, InstFiles *listpt);
char *GetStartMenuTopDir(BOOL AllUsers);
char *GetStartMenuDir(BOOL AllUsers, InstData *idata);
char *GetDesktopDir(void);
BOOL CreateWholeDirectory(char *path);
BOOL RemoveWholeDirectory(char *path);
void DeleteLinkList(char *dir, InstLink *listpt, HWND hwnd);
void DeleteFileList(InstFiles *listpt, HWND hwnd, InstFiles *keepfiles);
InstData *ReadOldInstData(HANDLE fin, char *product, char *installdir);
char *GetInstallDir(char *product);
void RemoveService(NTService *service);
void RemoveUninstall(char *startmenu, char *product, BOOL delexe);
