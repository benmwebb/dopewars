/************************************************************************
 * util.c         Miscellaneous utility and portability functions       *
 * Copyright (C)  1998-2002  Ben Webb                                   *
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef CYGWIN
#include <conio.h>
#endif

#include "util.h"
#include "dopewars.h"

#ifndef HAVE_GETOPT
char *optarg;

static int apos = 0;

int getopt(int argc, char *const argv[], const char *str)
{
  int i, c;
  char *pt;

  while (apos < argc && argv[apos]) {
    if (argv[apos][0] != '-') {
      apos++;
      return 0;
    }
    for (i = 1; i < strlen(argv[apos]); i++) {
      c = argv[apos][i];
      pt = strchr(str, c);
      if (pt) {
        argv[apos][i] = '-';
        if (*(pt + 1) == ':') {
          if (apos + 1 < argc && i == strlen(argv[apos]) - 1) {
            apos++;
            optarg = argv[apos];
            apos++;
          } else
            return 0;
        }
        return c;
      }
    }
    apos++;
  }
  return EOF;
}
#endif /* HAVE_GETOPT */


#ifdef CYGWIN                   /* Code for native Win32 build under Cygwin */

void sigemptyset(int *mask)
{
}

void sigaddset(int *mask, int sig)
{
}

int sigaction(int sig, struct sigaction *sact, char *pt)
{
  return 0;
}

void sigprocmask(int flag, int *mask, char *pt)
{
}

/*static gboolean IsKeyPressed()
{
  INPUT_RECORD ConsoleIn;
  DWORD NumConsoleIn;

  while (PeekConsoleInput(hIn, &ConsoleIn, 1, &NumConsoleIn)
         && NumConsoleIn == 1) {
    if (ConsoleIn.EventType == KEY_EVENT
        && ConsoleIn.Event.KeyEvent.bKeyDown) {
      return TRUE;
    } else {
      ReadConsoleInput(hIn, &ConsoleIn, 1, &NumConsoleIn);
    }
  }
  return FALSE;
}*/

int bselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
            struct timeval *tm)
{
  int retval;
  struct timeval tv, *tp;
  fd_set localread, localexcept;
  char CheckKbHit = 0, KeyRead;

  if (nfds == 0 && tm) {
    Sleep(tm->tv_sec * 1000 + tm->tv_usec / 1000);
    return 0;
  }
  if (FD_ISSET(0, readfds)) {
    if (nfds == 1)
      return 1;
    tp = &tv;
    CheckKbHit = 1;
    FD_CLR(0, readfds);
  } else
    tp = tm;
  KeyRead = 0;
  while (1) {
    tv.tv_sec = 0;
    tv.tv_usec = 250000;

    if (readfds)
      memcpy(&localread, readfds, sizeof(fd_set));
    if (exceptfds)
      memcpy(&localexcept, exceptfds, sizeof(fd_set));
    if (CheckKbHit && kbhit())
      tv.tv_usec = 0;
    retval = select(nfds, readfds, writefds, exceptfds, tp);
    if (retval == SOCKET_ERROR)
      return retval;
    if (CheckKbHit && kbhit()) {
      retval++;
      FD_SET(0, readfds);
    }
    if (retval > 0 || !CheckKbHit)
      break;
    if (CheckKbHit && tm) {
      if (tm->tv_usec >= 250000)
        tm->tv_usec -= 250000;
      else if (tm->tv_sec) {
        tm->tv_usec += 750000;
        tm->tv_sec--;
      } else
        break;
    }
    if (readfds)
      memcpy(readfds, &localread, sizeof(fd_set));
    if (exceptfds)
      memcpy(exceptfds, &localexcept, sizeof(fd_set));
  }
  return retval;
}

/* We don't do locking under Win32 right now */
int ReadLock(FILE * fp)
{
  return 0;
}

int WriteLock(FILE * fp)
{
  return 0;
}

void ReleaseLock(FILE * fp)
{
}

#else /* Code for Unix build */

#include <errno.h>

static int DoLock(FILE * fp, int l_type)
{
  struct flock lk;

  lk.l_type = l_type;
  lk.l_whence = lk.l_start = lk.l_len = 0;
  lk.l_pid = 0;

  while (1) {
    if (fcntl(fileno(fp), F_SETLKW, &lk) == 0)
      return 0;
    else if (errno != EINTR)
      return 1;
  }
  return 1;
}

int ReadLock(FILE * fp)
{
  return DoLock(fp, F_RDLCK);
}

int WriteLock(FILE * fp)
{
  return DoLock(fp, F_WRLCK);
}

void ReleaseLock(FILE * fp)
{
  DoLock(fp, F_UNLCK);
}

#endif /* CYGWIN */

/* 
 * On systems with select, sleep for "microsec" microseconds.
 */
void MicroSleep(int microsec)
{
#if HAVE_SELECT || CYGWIN
  struct timeval tv;

  tv.tv_sec = 0;
  tv.tv_usec = microsec;
  bselect(0, NULL, NULL, NULL, &tv);
#endif
}
