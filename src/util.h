/************************************************************************
 * util.h         Miscellaneous utility and portability functions       *
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

#ifndef __UTIL_H__
#define __UTIL_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#ifdef CYGWIN                   /* Definitions for native Win32 build */
#include <windows.h>
#include <string.h>

#define SIGWINCH      0
#define SIGPIPE       0
#define SIG_BLOCK     0
#define SIG_UNBLOCK   0

typedef int ssize_t;

struct sigaction {
  void *sa_handler;
  int sa_flags;
  int sa_mask;
};

void sigemptyset(int *mask);
void sigaddset(int *mask, int sig);
int sigaction(int sig, struct sigaction *sact, char *pt);
void sigprocmask(int flag, int *mask, char *pt);
int bselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfs,
            struct timeval *tm);
#else /* Definitions for Unix build */
#define bselect select
#endif /* CYGWIN */

#ifndef HAVE_GETOPT
int getopt(int argc, char *const argv[], const char *str);
extern char *optarg;
#endif

void MicroSleep(int microsec);

int ReadLock(FILE *fp);
int WriteLock(FILE *fp);
void ReleaseLock(FILE *fp);

/* Now make definitions if they haven't been done properly */
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif

#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#endif /* __UTIL_H__ */
