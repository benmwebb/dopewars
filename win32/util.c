#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <shlobj.h>
#include "util.h"

const char *UninstallKey=
  "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
const char *UninstallEXE="bw-uninstall.exe";

static void bstr_append_dir(bstr *str,BOOL windir);

void *bmalloc(UINT numbytes) {
  HLOCAL localpt;

  if (numbytes<=0) numbytes=1;

  localpt = LocalAlloc(LMEM_FIXED,numbytes);
  if (localpt) return (void *)localpt;
  else {
    DisplayError("Could not allocate memory: ",TRUE,TRUE);
    ExitProcess(1);
  }
}

void bfree(void *pt) {
  if (!pt) return;
  if (LocalFree((HLOCAL)pt)) {
    DisplayError("Could not free memory: ",TRUE,TRUE);
  }
}

void *brealloc(void *pt,UINT numbytes) {
  HLOCAL localpt;
  UINT numcp;
  void *newpt;
  if (!pt && numbytes) return bmalloc(numbytes);
  else if (pt && !numbytes) {
    bfree(pt);
  } else if (pt && numbytes) {
    newpt=bmalloc(numbytes);
    memset(newpt,0,numbytes);
    numcp = LocalSize((HLOCAL)pt);
    if (numbytes < numcp) numcp = numbytes;
    memcpy((char *)newpt,(char *)pt,numcp);
    bfree(pt);
    return newpt;
  }
  return NULL;
}

char *bstrdup(char *str) {
  char *newstr;
  if (str) {
    newstr = bmalloc(strlen(str)+1);
    strcpy(newstr,str);
  } else {
    newstr = bmalloc(1);
    newstr[0]='\0';
  }
  return newstr;
}

bstr *bstr_new(void) {
  bstr *str;
  str = bmalloc(sizeof(bstr));
  str->bufsiz=64;
  str->length=0;
  str->text= bmalloc(str->bufsiz);
  str->text[0]='\0';
  return str;
}

void bstr_free(bstr *str,BOOL free_text) {
  if (free_text) bfree(str->text);
  bfree(str);
}

void bstr_expandby(bstr *str,unsigned extralength) {
  if (str->bufsiz >= str->length+1+extralength) return;

  while (str->bufsiz < str->length+1+extralength) str->bufsiz*=2;
  str->text = brealloc(str->text,str->bufsiz);
}

void bstr_setlength(bstr *str,unsigned length) {
  if (length <= str->length) {
    str->length = length;
    str->text[length]='\0';
  } else {
    bstr_expandby(str,length-str->length);
  }
}

void bstr_assign(bstr *str,const char *text) {
  if (text) {
    bstr_setlength(str,strlen(text));
    strcpy(str->text,text);
    str->length=strlen(text);
  } else {
    bstr_setlength(str,0);
  }
}

void bstr_append(bstr *str,const char *text) {
  if (!text) return;
  bstr_expandby(str,strlen(text));
  strcat(str->text,text);
  str->length+=strlen(text);
}

void bstr_appendpath(bstr *str,const char *text) {
  if (str->length && str->text[str->length-1]!='\\') {
    bstr_append_c(str,'\\');
  }
  bstr_append(str,text);
}

void bstr_append_c(bstr *str,char ch) {
  bstr_expandby(str,1);
  str->text[str->length]=ch;
  str->length++;
  str->text[str->length]='\0';
}

/* We can be pretty confident that this is enough space for an integer */
#define MAX_LENGTH_INT 80

void bstr_append_long(bstr *str,long val) {
  char tmpbuf[MAX_LENGTH_INT];
  sprintf(tmpbuf,"%ld",val);
  bstr_append(str,tmpbuf);
}

void bstr_append_windir(bstr *str) {
  bstr_append_dir(str,TRUE);
}

void bstr_append_curdir(bstr *str) {
  bstr_append_dir(str,FALSE);
}

void bstr_assign_windir(bstr *str) {
  bstr_setlength(str,0);
  bstr_append_windir(str);
}

void bstr_assign_curdir(bstr *str) {
  bstr_setlength(str,0);
  bstr_append_curdir(str);
}

void bstr_append_dir(bstr *str,BOOL windir) {
  unsigned spaceleft;
  UINT retval;
  int tries;

  spaceleft = str->bufsiz-str->length; /* spaceleft includes the null */

  for (tries=0;tries<5;tries++) {
    if (windir) {
      retval = GetWindowsDirectory(str->text+str->length,spaceleft);
    } else {
      retval = GetCurrentDirectory(spaceleft,str->text+str->length);
    }
    if (retval==0) DisplayError("Cannot get directory: ",TRUE,TRUE);
    if (retval <= spaceleft-1) {
      str->length += retval;
      break;
    }
    bstr_expandby(str,retval); /* Let's err on the side of caution */
    spaceleft = str->bufsiz-str->length;
  }
  if (tries>=5) DisplayError("Cannot get directory: ",TRUE,TRUE);
}

void DisplayError(const char *errtext,BOOL addsyserr,BOOL fatal) {
  DWORD syserr;
  bstr *str;
  LPTSTR lpMsgBuf;

  syserr=GetLastError();

  str=bstr_new();
  bstr_assign(str,errtext);

  if (addsyserr) {
    bstr_append(str,"; ");
    bstr_append_long(str,syserr);
    bstr_append(str,": ");
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,syserr,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf,0,NULL);
    bstr_append(str,lpMsgBuf);
    LocalFree(lpMsgBuf);
  }

  MessageBox(NULL,str->text,fatal ? "Fatal Error" : "Error",
             MB_OK | MB_ICONSTOP);
  if (fatal) ExitProcess(1);
}

void AddInstFiles(char *filename,DWORD filesize,InstFiles **lastpt,
                  InstFiles **firstpt) {
  InstFiles *newpt;

  newpt = bmalloc(sizeof(InstFiles));
  if (*lastpt) {
    (*lastpt)->next = newpt;
  } else {
    *firstpt = newpt;
  }
  *lastpt = newpt;
  newpt->next=NULL;
  newpt->filename=filename;
  newpt->filesize=filesize;
}

void AddInstLink(char *linkfile,char *origfile,char *args,InstLink **lastpt,
                 InstLink **firstpt) {
  InstLink *newpt;

  newpt = bmalloc(sizeof(InstLink));
  if (*lastpt) {
    (*lastpt)->next = newpt;
  } else {
    *firstpt = newpt;
  }
  *lastpt = newpt;
  newpt->next=NULL;
  newpt->linkfile=linkfile;
  newpt->origfile=origfile;
  newpt->args=args;
}

void FreeLinkList(InstLink *linklist,BOOL freepts) {
  InstLink *thispt,*nextpt;

  thispt=linklist;
  while (thispt) {
    nextpt=thispt->next;

    if (freepts) {
      bfree(thispt->linkfile);
      bfree(thispt->origfile);
      bfree(thispt->args);
    }
    bfree(thispt);

    thispt=nextpt;
  }
}

void FreeFileList(InstFiles *filelist,BOOL freepts) {
  InstFiles *thispt,*nextpt;

  thispt=filelist;
  while (thispt) {
    nextpt=thispt->next;

    if (freepts) bfree(thispt->filename);
    bfree(thispt);

    thispt=nextpt;
  }
}

void FreeInstData(InstData *idata,BOOL freepts) {
  FreeFileList(idata->instfiles,freepts);
  FreeFileList(idata->extrafiles,freepts);

  FreeLinkList(idata->startmenu,freepts);
  FreeLinkList(idata->desktop,freepts);

  bfree(idata->product);
  bfree(idata->installdir);
  bfree(idata->startmenudir);

  bfree(idata);
}

void WriteLinkList(HANDLE fout,InstLink *listpt) {
  char str[]="";
  DWORD bytes_written;
  for (;listpt;listpt=listpt->next) {
    if (!WriteFile(fout,listpt->linkfile,strlen(listpt->linkfile)+1,
                   &bytes_written,NULL)) {
      printf("Write error\n");
    }
    if (!WriteFile(fout,listpt->origfile,strlen(listpt->origfile)+1,
                   &bytes_written,NULL)) {
      printf("Write error\n");
    }
    if (!WriteFile(fout,listpt->args,strlen(listpt->args)+1,
                   &bytes_written,NULL)) {
      printf("Write error\n");
    }
  }
  if (!WriteFile(fout,str,strlen(str)+1,&bytes_written,NULL)) {
    printf("Write error\n");
  }
}

void WriteFileList(HANDLE fout,InstFiles *listpt) {
  bstr *str;
  DWORD bytes_written;
  str=bstr_new();

  for (;listpt;listpt=listpt->next) {
    if (!WriteFile(fout,listpt->filename,strlen(listpt->filename)+1,
                   &bytes_written,NULL)) {
      printf("Write error\n");
    }
    bstr_setlength(str,0);
    bstr_append_long(str,listpt->filesize);
    if (!WriteFile(fout,str->text,str->length+1,&bytes_written,NULL)) {
      printf("Write error\n");
    }
  }
  bstr_assign(str,"");
  if (!WriteFile(fout,str->text,str->length+1,&bytes_written,NULL)) {
    printf("Write error\n");
  }
  bstr_free(str,TRUE);
}

static char *GetSpecialDir(int dirtype) {
  LPITEMIDLIST pidl;
  LPMALLOC pmalloc;
  char szDir[MAX_PATH];
  BOOL doneOK=FALSE;

  if (SUCCEEDED(SHGetMalloc(&pmalloc))) {
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL,dirtype,&pidl))) {
      if (SHGetPathFromIDList(pidl,szDir)) doneOK=TRUE;
      pmalloc->lpVtbl->Free(pmalloc,pidl);
    }
    pmalloc->lpVtbl->Release(pmalloc);
  }
  return (doneOK ? bstrdup(szDir) : NULL);
}

char *GetStartMenuTopDir(void) {
  return GetSpecialDir(CSIDL_STARTMENU);
}

char *GetDesktopDir(void) {
  return GetSpecialDir(CSIDL_DESKTOPDIRECTORY);
}

char *GetStartMenuDir(InstData *idata) {
  bstr *str;
  char *topdir,*retval;

  topdir=GetStartMenuTopDir();

  str = bstr_new();
  
  bstr_assign(str,topdir);
  bfree(topdir);

  bstr_appendpath(str,"Programs");
  bstr_appendpath(str,idata->startmenudir);

  retval = str->text;
  bstr_free(str,FALSE);
  return retval;
}

BOOL CreateWholeDirectory(char *path) {
  char *pt;
  if (!path) return FALSE;

/* We may as well try the easy way first */
  if (CreateDirectory(path,NULL)) return TRUE;

  /* \\machine\share notation */
  if (strlen(path)>2 && path[0]=='\\' && path[1]=='\\') {
    pt=&path[2]; /* Skip initial "\\" */
  } else {
    pt=path;
  }

  while (*pt && *pt!='\\') pt++;  /* Skip the first (root) '\' */

  while (*pt) {
    pt++;
    if (*pt=='\\') {
      *pt='\0';
      if (!CreateDirectory(path,NULL)) {
        *pt='\\'; return FALSE;
      }
      *pt='\\';
    }
  }
  return CreateDirectory(path,NULL);
}

BOOL RemoveWholeDirectory(char *path) {
  char *pt;
  BOOL retval;
  if (!path || !RemoveDirectory(path)) return FALSE;

  for (pt=&path[strlen(path)-2];pt>path;pt--) {
    if (*pt=='\\') {
      *pt='\0';
      retval=RemoveDirectory(path);
      *pt='\\';
      if (!retval) break;
    }
  }
  return TRUE;
}
