#include <windows.h>
#include <stdio.h>
#include <zlib.h>
#include "util.h"

char *read_line(HANDLE hin) {
  char *buf;
  int bufsize=32,strind=0;
  DWORD bytes_read;
  buf = bmalloc(bufsize);

  while (1) {
    if (!ReadFile(hin,&buf[strind],1,&bytes_read,NULL)) {
      printf("Read error\n"); break;
    }
    if (bytes_read==0) { buf[strind]='\0'; break; }
    else if (buf[strind]=='\r') continue;
    else if (buf[strind]=='\n') { buf[strind++]='\0'; break; }
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

InstData *ReadInstallData() {
  HANDLE fin;
  char *line,*line2,*line3;
  InstFiles *lastinst=NULL,*lastextra=NULL;
  InstLink *lastmenu=NULL,*lastdesktop=NULL;
  InstData *idata;
  int i;
  enum {
    S_PRODUCT=0,S_INSTDIR,S_INSTALL,S_EXTRA,S_STARTMENUDIR,
    S_STARTMENU,S_DESKTOP,
    S_NONE
  } section=S_NONE;
  char *titles[S_NONE] = {
    "[product]","[instdir]", "[install]","[extrafiles]","[startmenudir]",
    "[startmenu]","[desktop]"
  };

  idata = bmalloc(sizeof(InstData));
  idata->installdir = idata->startmenudir = NULL;
  idata->instfiles = idata->extrafiles = NULL;
  idata->startmenu = idata->desktop = NULL;

  fin = CreateFile("filelist",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);

  if (fin) {
    while (1) {
      line=read_line(fin);
      if (!line) break;
      if (line[0]=='\0') { bfree(line); continue; }

      for (i=S_PRODUCT;i<S_NONE;i++) {
        if (strcmp(line,titles[i])==0) {
          section=i; break;
        }
      }
      if (i<S_NONE) { bfree(line); continue; }

      switch(section) {
        case S_NONE:
          printf("Bad line %s\n",line); exit(1);
        case S_PRODUCT:
printf("product ID = %s\n",line);
          idata->product = line; break;
        case S_INSTDIR:
printf("install dir = %s\n",line);
          idata->installdir = line; break;
        case S_STARTMENUDIR:
printf("start menu dir = %s\n",line);
          idata->startmenudir = line; break;
        case S_INSTALL:
          AddInstFiles(line,0,&lastinst,&idata->instfiles);
          break;
        case S_EXTRA:
          AddInstFiles(line,0,&lastextra,&idata->extrafiles);
          break;
        case S_STARTMENU:
          line2=read_line(fin); line3=read_line(fin);
printf("start menu entry = %s/%s/%s\n",line,line2,line3);
          AddInstLink(line,line2,line3,&lastmenu,&idata->startmenu);
          break;
        case S_DESKTOP:
          line2=read_line(fin); line3=read_line(fin);
printf("desktop entry = %s/%s/%s\n",line,line2,line3);
          AddInstLink(line,line2,line3,&lastdesktop,&idata->desktop);
          break;
      }
    }
    CloseHandle(fin);
  }

  if (idata->installdir && idata->startmenudir && idata->product) {
    return idata;
  } else {
    printf("No directories specified\n");
    exit(1);
  }
}

#define BUFFER_SIZE (32*1024)
#define COMPRESSION Z_BEST_COMPRESSION

void OpenNextFile(InstFiles *filelist,InstFiles **listpt,HANDLE *fin) {
  if (*fin) CloseHandle(*fin);
  *fin=NULL;

  if (!*listpt) *listpt = filelist;
  else *listpt = (*listpt)->next;

  if (*listpt) *fin = CreateFile((*listpt)->filename,GENERIC_READ,0,NULL,
                                 OPEN_EXISTING,0,NULL);
}

int main() {
  HANDLE fout,fin;
  DWORD bytes_read,bytes_written;
  InstData *idata;
  InstFiles *filept;
  char *inbuf,*outbuf;
  int status,count;
  bstr *str;
  z_stream z;

  idata=ReadInstallData();

  fout = CreateFile("installfiles.gz",GENERIC_WRITE,0,NULL,
                    CREATE_ALWAYS,0,NULL);

  outbuf = bmalloc(BUFFER_SIZE);
  inbuf = bmalloc(BUFFER_SIZE);

  z.zalloc = Z_NULL;
  z.zfree = Z_NULL;
  z.opaque = Z_NULL;
  deflateInit(&z,COMPRESSION);
  z.avail_in=0;
  z.next_out = outbuf;
  z.avail_out = BUFFER_SIZE;

  filept=NULL;
  fin=NULL;
  OpenNextFile(idata->instfiles,&filept,&fin);
  if (!fin) { printf("Cannot open file\n"); return 1; }

  while (fin) {
    if (z.avail_in==0) {
      z.next_in = inbuf;
      bytes_read=0;
      while (!bytes_read && fin) {
        if (!ReadFile(fin,inbuf,BUFFER_SIZE,&bytes_read,NULL)) {
          printf("Read error\n"); break;
        }
        filept->filesize+=bytes_read;
        if (!bytes_read) OpenNextFile(idata->instfiles,&filept,&fin);
      }
      z.avail_in = bytes_read;
    }
    if (z.avail_in==0) {
      status = deflate(&z,Z_FINISH);
      count = BUFFER_SIZE - z.avail_out;
      if (!WriteFile(fout,outbuf,count,&bytes_written,NULL)) {
        printf("Write error\n");
      }
      break;
    }
    status = deflate(&z,Z_NO_FLUSH);
    count = BUFFER_SIZE - z.avail_out;
    if (!WriteFile(fout,outbuf,count,&bytes_written,NULL)) {
      printf("Write error\n");
    }
    z.next_out = outbuf;
    z.avail_out = BUFFER_SIZE;
  }

  printf("Written compressed data: raw %lu, compressed %lu\n",
         z.total_in,z.total_out);
  bytes_written = z.total_out;
  deflateEnd(&z);

  CloseHandle(fout);

  fout = CreateFile("manifest",GENERIC_WRITE,0,NULL,
                    CREATE_ALWAYS,0,NULL);
  if (!fout) printf("Cannot create file list\n");

  str=bstr_new();
  bstr_setlength(str,0);
  bstr_append_long(str,bytes_written);
  if (!WriteFile(fout,str->text,str->length+1,&bytes_written,NULL)) {
    printf("Write error\n");
  }

  if (!WriteFile(fout,idata->product,strlen(idata->product)+1,
                 &bytes_written,NULL)) {
    printf("Write error\n");
  }
  if (!WriteFile(fout,idata->installdir,strlen(idata->installdir)+1,
                 &bytes_written,NULL)) {
    printf("Write error\n");
  }
  if (!WriteFile(fout,idata->startmenudir,strlen(idata->startmenudir)+1,
                 &bytes_written,NULL)) {
    printf("Write error\n");
  }

  WriteFileList(fout,idata->instfiles);
  WriteFileList(fout,idata->extrafiles);

  WriteLinkList(fout,idata->startmenu);
  WriteLinkList(fout,idata->desktop);

  CloseHandle(fout);
  bfree(inbuf);
  bfree(outbuf);

  FreeInstData(idata,TRUE);

  return 0;
}
