/* dopeos.h         dopewars - operating system-specific function       */
/*                             definitions                              */
/* Copyright (C)  1998-2001  Ben Webb                                   */
/*                Email: ben@bellatrix.pcl.ox.ac.uk                     */
/*                WWW: http://bellatrix.pcl.ox.ac.uk/~ben/dopewars/     */

/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU General Public License          */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */

/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */

/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston,               */
/*                   MA  02111-1307, USA.                               */

#ifndef __DOPEOS_H__
#define __DOPEOS_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef CYGWIN   /* Definitions for native Win32 build */
#include <windows.h>
#include <string.h>

#if NETWORKING
#include <winsock.h>
#endif

#include <stdio.h>

void refresh();
HANDLE WINAPI GetConHandle (TCHAR * pszName);
extern WORD TextAttr,PromptAttr,TitleAttr,LocationAttr,StatsAttr,DebtAttr;
extern int Width,Depth;

#define COLOR_MAGENTA 1
#define COLOR_BLACK   2
#define COLOR_WHITE   3
#define COLOR_BLUE    4
#define COLOR_RED     5

#define SIGWINCH 0
#define SIGPIPE  0
#define SIG_BLOCK 0
#define SIG_UNBLOCK 0

struct sigaction {
	void *sa_handler;
	int sa_flags;
	int sa_mask;
};

void sigemptyset(int *mask);
void sigaddset(int *mask,int sig);
int sigaction(int sig,struct sigaction *sact,char *pt);
void sigprocmask(int flag,int *mask,char *pt);

#define COLS  Width
#define LINES Depth

#define ACS_VLINE		179
#define ACS_ULCORNER	218
#define ACS_HLINE		196
#define ACS_URCORNER	191
#define ACS_TTEE		194
#define ACS_LLCORNER	192
#define ACS_LRCORNER	217
#define ACS_BTEE		193
#define ACS_LTEE		195
#define ACS_RTEE		180

typedef int SCREEN;
#define stdscr 0
#define curscr 0
#define KEY_ENTER     13
#define KEY_BACKSPACE 8
#define A_BOLD        0

SCREEN *newterm(void *,void *,void *);
void start_color();
void init_pair(int index,WORD fg,WORD bg);
void cbreak();
void noecho();
void nodelay(void *,char);
void keypad(void *,char);
void curs_set(BOOL visible);
void endwin();
void move(int y,int x);
void attrset(WORD newAttr);
void addstr(char *str);
void addch(int ch);
void mvaddstr(int x,int y,char *str);
void mvaddch(int x,int y,int ch);
int bgetch();
#define erase() clear_screen()
char *index(char *str,char ch);
int getopt(int argc,char *argv[],char *str);
extern char *optarg;
#define F_SETFL 0
#define O_NONBLOCK FIONBIO

typedef int ssize_t;
void gettimeofday(void *pt,void *pt2);
void standout();
void standend();
void endwin();
int bselect(int nfds,fd_set *readfds,fd_set *writefds,fd_set *exceptfs,
		struct timeval *tm);

#if NETWORKING
int GetSocketError();
void fcntl(SOCKET s,int fsetfl,long cmd);
#define CloseSocket(sock) closesocket(sock)
void StartNetworking();
void StopNetworking();
void SetReuse(SOCKET sock);
#endif

#else /* Definitions for Unix build */

#include <sys/types.h>

#include <stdio.h>

#ifdef NETWORKING
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif /* NETWORKING */

#include <errno.h>

/* Only include sys/wait.h on those systems which support it */
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

/* Include a suitable curses-type library */
#if HAVE_LIBNCURSES
#include <ncurses.h>
#elif HAVE_LIBCURSES
#include <curses.h>
#elif HAVE_LIBCUR_COLR
#include <curses_colr/curses.h>
#endif

extern int Width,Depth;

#define PromptAttr   (COLOR_PAIR(1))
#define TextAttr     (COLOR_PAIR(2))
#define LocationAttr (COLOR_PAIR(3)|A_BOLD)
#define TitleAttr    (COLOR_PAIR(4))
#define StatsAttr    (COLOR_PAIR(5))
#define DebtAttr     (COLOR_PAIR(6))

#ifdef CURSES_CLIENT
int bgetch(void);
#else
/* When not using curses, fall back to stdio's getchar() function */
#define bgetch getchar
#endif

#define bselect select

#if NETWORKING
#define CloseSocket(sock) close(sock)
int GetSocketError(void);
void StartNetworking(void);
void StopNetworking(void);
void SetReuse(int sock);
#endif /* NETWORKING */

#endif /* CYGWIN */

void MicroSleep(int microsec);

int ReadLock(FILE *fp); 
int WriteLock(FILE *fp); 
void ReleaseLock(FILE *fp);

/* Now make definitions if they haven't been done properly */
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif

#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#endif /* __DOPEOS_H__ */
