/************************************************************************
 * makeinstall.c  Program to create install data for setup.c            *
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
#include "bzlib/bzlib.h"
#include "util.h"

char *read_line(HANDLE hin)
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
    } else if (buf[strind] == '\r')
      continue;
    else if (buf[strind] == '\n') {
      buf[strind++] = '\0';
      break;
    } else {
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

InstData *ReadInstallData()
{
  HANDLE fin;
  char *line, *line2, *line3, *line4;
  InstFiles *lastinst = NULL, *lastextra = NULL, *lastkeep = NULL;
  InstLink *lastmenu = NULL, *lastdesktop = NULL;
  InstData *idata;
  int i;
  enum {
    S_PRODUCT = 0, S_INSTDIR, S_INSTALL, S_EXTRA, S_KEEP, S_STARTMENUDIR,
    S_STARTMENU, S_DESKTOP, S_NTSERVICE, S_NONE
  } section = S_NONE;
  char *titles[S_NONE] = {
    "[product]", "[instdir]", "[install]", "[extrafiles]", "[keepfiles]",
    "[startmenudir]", "[startmenu]", "[desktop]", "[NT Service]"
  };

  idata = bmalloc(sizeof(InstData));
  idata->installdir = idata->startmenudir = NULL;
  idata->instfiles = idata->extrafiles = idata->keepfiles = NULL;
  idata->startmenu = idata->desktop = NULL;
  idata->service = NULL;

  fin = CreateFile("filelist", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

  if (fin != INVALID_HANDLE_VALUE) {
    while (1) {
      line = read_line(fin);
      if (!line)
        break;
      if (line[0] == '\0') {
        bfree(line);
        continue;
      }

      for (i = S_PRODUCT; i < S_NONE; i++) {
        if (strcmp(line, titles[i]) == 0) {
          section = i;
          break;
        }
      }
      if (i < S_NONE) {
        bfree(line);
        continue;
      }

      switch (section) {
      case S_NONE:
        printf("Bad line %s\n", line);
        exit(EXIT_FAILURE);
      case S_PRODUCT:
        printf("product ID = %s\n", line);
        idata->product = line;
        break;
      case S_INSTDIR:
        printf("install dir = %s\n", line);
        idata->installdir = line;
        break;
      case S_STARTMENUDIR:
        printf("start menu dir = %s\n", line);
        idata->startmenudir = line;
        break;
      case S_INSTALL:
        AddInstFiles(line, 0, &lastinst, &idata->instfiles);
        break;
      case S_EXTRA:
        AddInstFiles(line, 0, &lastextra, &idata->extrafiles);
        break;
      case S_KEEP:
        AddInstFiles(line, 0, &lastkeep, &idata->keepfiles);
        break;
      case S_STARTMENU:
        line2 = read_line(fin);
        line3 = read_line(fin);
        printf("start menu entry = %s/%s/%s\n", line, line2, line3);
        AddInstLink(line, line2, line3, &lastmenu, &idata->startmenu);
        break;
      case S_NTSERVICE:
        line2 = read_line(fin);
        line3 = read_line(fin);
        line4 = read_line(fin);
        printf("NT Service = %s/%s/%s/%s\n", line, line2, line3, line4);
        AddServiceDetails(line, line2, line3, line4, &idata->service);
        break;
      case S_DESKTOP:
        line2 = read_line(fin);
        line3 = read_line(fin);
        printf("desktop entry = %s/%s/%s\n", line, line2, line3);
        AddInstLink(line, line2, line3, &lastdesktop, &idata->desktop);
        break;
      }
    }
    CloseHandle(fin);
  }

  if (idata->installdir && idata->startmenudir && idata->product) {
    return idata;
  } else {
    printf("No directories specified\n");
    exit(EXIT_FAILURE);
  }
}

#define BUFFER_SIZE (32*1024)
#define COMPRESSION 9

void OpenNextFile(InstFiles *filelist, InstFiles **listpt, HANDLE *fin)
{
  char *filename, *sep;

  if (*fin)
    CloseHandle(*fin);
  *fin = NULL;

  if (!*listpt)
    *listpt = filelist;
  else
    *listpt = (*listpt)->next;

  if (*listpt) {
    filename = (*listpt)->filename;
    sep = strstr(filename, " -> ");
    if (sep)
      *sep = '\0';
    *fin = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (sep)
      strcpy(filename, sep + 4);
    if (*fin == INVALID_HANDLE_VALUE)
      printf("Cannot open file: %s\n", filename);
  }
}

int main()
{
  HANDLE fout, fin;
  DWORD bytes_read, bytes_written;
  InstData *idata;
  InstFiles *filept;
  char *inbuf, *outbuf;
  int status, count;
  bstr *str;
  bz_stream bz;

  idata = ReadInstallData();

  fout = CreateFile("installfiles.bz2", GENERIC_WRITE, 0, NULL,
                    CREATE_ALWAYS, 0, NULL);

  outbuf = bmalloc(BUFFER_SIZE);
  inbuf = bmalloc(BUFFER_SIZE);

  bz.bzalloc = NULL;
  bz.bzfree = NULL;
  bz.opaque = NULL;
  BZ2_bzCompressInit(&bz, COMPRESSION, 0, 30);
  bz.avail_in = 0;
  bz.next_out = outbuf;
  bz.avail_out = BUFFER_SIZE;

  filept = NULL;
  fin = NULL;
  OpenNextFile(idata->instfiles, &filept, &fin);
  if (fin == INVALID_HANDLE_VALUE) {
    return 1;
  }

  while (fin != INVALID_HANDLE_VALUE) {
    if (bz.avail_in == 0) {
      bz.next_in = inbuf;
      bytes_read = 0;
      while (!bytes_read && fin) {
        if (!ReadFile(fin, inbuf, BUFFER_SIZE, &bytes_read, NULL)) {
          printf("Read error\n");
          break;
        }
        filept->filesize += bytes_read;
        if (!bytes_read)
          OpenNextFile(idata->instfiles, &filept, &fin);
      }
      bz.avail_in = bytes_read;
    }
    status = BZ2_bzCompress(&bz, bz.avail_in == 0 ? BZ_FINISH : BZ_RUN);
    count = BUFFER_SIZE - bz.avail_out;
    if (!WriteFile(fout, outbuf, count, &bytes_written, NULL)) {
      printf("Write error\n");
    }
    bz.next_out = outbuf;
    bz.avail_out = BUFFER_SIZE;
    if (status == BZ_STREAM_END) {
      break;
    } else if (status != BZ_RUN_OK && status != BZ_FINISH_OK) {
      printf("Unexpected bzlib status: %d\n", status);
      break;
    }
  }

  printf("Written compressed data: raw %d, compressed %d\n",
         bz.total_in_lo32, bz.total_out_lo32);
  bytes_written = bz.total_out_lo32;
  BZ2_bzCompressEnd(&bz);

  CloseHandle(fout);

  fout = CreateFile("manifest", GENERIC_WRITE, 0, NULL,
                    CREATE_ALWAYS, 0, NULL);
  if (fout == INVALID_HANDLE_VALUE)
    printf("Cannot create file list\n");

  str = bstr_new();
  bstr_setlength(str, 0);
  bstr_append_long(str, bytes_written);
  if (!WriteFile(fout, str->text, str->length + 1, &bytes_written, NULL)) {
    printf("Write error\n");
  }

  if (!WriteFile(fout, idata->product, strlen(idata->product) + 1,
                 &bytes_written, NULL)) {
    printf("Write error\n");
  }
  if (!WriteFile(fout, idata->installdir, strlen(idata->installdir) + 1,
                 &bytes_written, NULL)) {
    printf("Write error\n");
  }
  if (!WriteFile
      (fout, idata->startmenudir, strlen(idata->startmenudir) + 1,
       &bytes_written, NULL)) {
    printf("Write error\n");
  }

  WriteFileList(fout, idata->instfiles);
  WriteFileList(fout, idata->extrafiles);

  WriteLinkList(fout, idata->startmenu);
  WriteLinkList(fout, idata->desktop);

  WriteServiceDetails(fout, idata->service);
  WriteFileList(fout, idata->keepfiles);

  CloseHandle(fout);
  bfree(inbuf);
  bfree(outbuf);

  FreeInstData(idata, TRUE);

  return 0;
}

void bz_internal_error(int errcode)
{
  printf("bzip error %d\n", errcode);
}
