/* dopeos.c          dopewars - operating-system-specific functions     */
/* Copyright (C)  1998-2001  Ben Webb                                   */
/*                Email: ben@bellatrix.pcl.ox.ac.uk                     */
/*                WWW: http://dopewars.sourceforge.net/                 */

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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dopeos.h"
#include "dopewars.h"

#ifdef CYGWIN /* Code for native Win32 build under Cygwin */

#include <conio.h>
#ifdef GUI_CLIENT
#include "gtkport.h"
#endif

CHAR_INFO RealScreen[25][80],VirtualScreen[25][80];
HANDLE hOut,hIn;

void refresh() {
   int y;
   COORD size,offset;
   SMALL_RECT screenpos;
   for (y=0;y<Depth;y++) {
      if (memcmp(&RealScreen[y][0],&VirtualScreen[y][0],
                 sizeof(CHAR_INFO)*Width)!=0) {
         memcpy(&RealScreen[y][0],&VirtualScreen[y][0],
                Width*sizeof(CHAR_INFO));
         size.X=Width; size.Y=1;
         offset.X=offset.Y=0;
         screenpos.Left=0; screenpos.Top=y;
         screenpos.Right=Width-1; screenpos.Bottom=y;
         WriteConsoleOutput(hOut,&VirtualScreen[y][0],size,
                            offset,&screenpos);
      }
   }
}

HANDLE WINAPI GetConHandle(TCHAR *pszName) {
   SECURITY_ATTRIBUTES sa;
   sa.nLength = sizeof(sa);
   sa.lpSecurityDescriptor = NULL;
   sa.bInheritHandle = TRUE;
   return CreateFile(pszName,GENERIC_READ|GENERIC_WRITE,
                     FILE_SHARE_READ|FILE_SHARE_WRITE,
                     &sa,OPEN_EXISTING,(DWORD)0,(HANDLE)0);
}

WORD CurAttr=0,TextAttr=2<<8,PromptAttr=1<<8,TitleAttr=4<<8;
WORD LocationAttr=3<<8,StatsAttr=5<<8,DebtAttr=6<<8;
int Width,Depth,CurX,CurY;
char *optarg;
WORD Attr[10];
HWND hwndMain;

SCREEN *newterm(void *a,void *b,void *c) {
   COORD coord;
   int i;
   coord.X=80; coord.Y=25;
   Width=80; Depth=25; CurAttr=TextAttr; CurX=0; CurY=0;
   for (i=0;i<10;i++)
      Attr[i]=FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN;
   hOut=GetConHandle("CONOUT$");
   hIn=GetConHandle("CONIN$");
   SetConsoleMode(hIn,0);
/* SetConsoleScreenBufferSize(hOut,coord);*/
/* clear_screen();*/
   return NULL;
}

void start_color() {}
void init_pair(int index,WORD fg,WORD bg) {
   if (index>=0 && index<10) {
      Attr[index]=0;
      switch(fg) {
         case COLOR_MAGENTA: Attr[index]|=(FOREGROUND_RED+FOREGROUND_BLUE);
                             break;
         case COLOR_BLUE:    Attr[index]|=FOREGROUND_BLUE;
                             break;
         case COLOR_RED:     Attr[index]|=FOREGROUND_RED;
                             break;
         case COLOR_WHITE:   Attr[index]|=(FOREGROUND_RED+FOREGROUND_BLUE+
                                           FOREGROUND_GREEN);
                             break;
      }
      switch(bg) {
         case COLOR_MAGENTA: Attr[index]|=(BACKGROUND_RED+BACKGROUND_BLUE);
                             break;
         case COLOR_BLUE:    Attr[index]|=BACKGROUND_BLUE;
                             break;
         case COLOR_RED:     Attr[index]|=BACKGROUND_RED;
                             break;
         case COLOR_WHITE:   Attr[index]|=(BACKGROUND_RED+BACKGROUND_BLUE+
                                           BACKGROUND_GREEN);
                             break;
      }
   }
}

void cbreak() {}
void noecho() {}
void nodelay(void *a,char b) {}

void keypad(void *a,char b) {}
void curs_set(BOOL visible) {
   CONSOLE_CURSOR_INFO ConCurInfo;
   move(CurY,CurX);
   ConCurInfo.dwSize=10;
   ConCurInfo.bVisible=visible;
   SetConsoleCursorInfo(hOut,&ConCurInfo);
}

void endwin() {
   CurAttr=0;
/* clear_screen(); */
   refresh();
   curs_set(1);
/* CloseHandle(hIn);
   CloseHandle(hOut);*/
}

void move(int y,int x) {
   COORD coord;
   CurX=x; CurY=y;
   coord.X=x; coord.Y=y;
   SetConsoleCursorPosition(hOut,coord);
}

void attrset(WORD newAttr) {
   CurAttr=newAttr;
}

void addstr(char *str) {
   int i;
   for (i=0;i<strlen(str);i++) addch(str[i]);
   move(CurY,CurX);
}

void addch(int ch) {
   int attr;
   VirtualScreen[CurY][CurX].Char.AsciiChar=ch%256;
   attr=ch>>8;
   if (attr>0) VirtualScreen[CurY][CurX].Attributes=Attr[attr];
   else VirtualScreen[CurY][CurX].Attributes=Attr[CurAttr>>8];
   if (++CurX>=Width) {
      CurX=0;
      if (++CurY>=Depth) CurY=0;
   }
}

void mvaddstr(int y,int x,char *str) {
   move(y,x); addstr(str);
}

void mvaddch(int y,int x,int ch) {
   move(y,x); addch(ch);
}

int bgetch() {
/* Waits for the user to press a key */
   DWORD NumRead;
   char Buffer[10];
   refresh();
   ReadConsole(hIn,Buffer,1,&NumRead,NULL);
   return (int)(Buffer[0]);
}

char *index(const char *str,int ch) {
   int i;
   for (i=0;i<strlen(str);i++) { if (str[i]==ch) return str+i; }
   return NULL;
}

int apos=0;
int getopt(int argc,char * const argv[],const char *str) {
   int i,c;
   char *pt;
   while (apos<argc && argv[apos]) {
      if (argv[apos][0]!='-') { apos++; return 0; }
      for (i=1;i<strlen(argv[apos]);i++) {
         c=argv[apos][i];
         pt=index(str,c);
         if (pt) {
            argv[apos][i]='-';
            if (*(pt+1)==':') {
               if (apos+1<argc && i==strlen(argv[apos])-1) {
                  apos++;
                  optarg=argv[apos];
                  apos++;
               } else return 0;
            }
            return c;
         }
      }
      apos++;
   }
   return EOF;
}

void sigemptyset(int *mask) {}
void sigaddset(int *mask,int sig) {}
int sigaction(int sig,struct sigaction *sact,char *pt) { return 0; }
void sigprocmask(int flag,int *mask,char *pt) {}
void standout() {}
void standend() {}

gboolean IsKeyPressed() {
  INPUT_RECORD ConsoleIn;
  DWORD NumConsoleIn;
  while (PeekConsoleInput(hIn,&ConsoleIn,1,&NumConsoleIn) && NumConsoleIn==1) {
    if (ConsoleIn.EventType==KEY_EVENT && ConsoleIn.Event.KeyEvent.bKeyDown) {
      return TRUE;
    } else {
      ReadConsoleInput(hIn,&ConsoleIn,1,&NumConsoleIn);
    }
  }
  return FALSE;
}

int bselect(int nfds,fd_set *readfds,fd_set *writefds,fd_set *exceptfds,
            struct timeval *tm) {
   int retval;
   struct timeval tv,*tp;
   fd_set localread,localexcept;
   char CheckKbHit=0,KeyRead;
   if (nfds==0 && tm) { Sleep(tm->tv_sec*1000+tm->tv_usec/1000); return 0; }
   if (FD_ISSET(0,readfds)) {
      if (nfds==1) return 1;
      tp=&tv;
      CheckKbHit=1;
      FD_CLR(0,readfds);
   } else tp=tm;
   KeyRead=0;
   while (1) {
      tv.tv_sec=0;
      tv.tv_usec=250000;
      
      if (readfds) memcpy(&localread,readfds,sizeof(fd_set));
      if (exceptfds) memcpy(&localexcept,exceptfds,sizeof(fd_set));
      if (CheckKbHit && IsKeyPressed()) tv.tv_usec=0;
      retval=select(nfds,readfds,writefds,exceptfds,tp);
      if (retval==SOCKET_ERROR) return retval;
      if (CheckKbHit && IsKeyPressed()) {
         retval++; FD_SET(0,readfds);
      }
      if (retval>0 || !CheckKbHit) break;
      if (CheckKbHit && tm) {
         if (tm->tv_usec >= 250000) tm->tv_usec-=250000;
         else if (tm->tv_sec) {
            tm->tv_usec+=750000;
            tm->tv_sec--;
         } else break;
      }
      if (readfds) memcpy(readfds,&localread,sizeof(fd_set));
      if (exceptfds) memcpy(exceptfds,&localexcept,sizeof(fd_set));
   }
   return retval;
}

/* We don't do locking under Win32 right now */
int ReadLock(FILE *fp) { return 0; }
int WriteLock(FILE *fp) { return 0; }
void ReleaseLock(FILE *fp) { }

#else /* Code for Unix build */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

int Width,Depth;

#ifdef CURSES_CLIENT
int bgetch() {
/* Calls the curses getch() function; if the key pressed is Ctrl-L */
/* then automatically clears and redraws the screen, otherwise     */
/* passes the key back to the calling routine                      */
   int c;
   c=getch();
   while (c=='\f') {
      wrefresh(curscr);
      c=getch();
   }
   return c;
}
#endif

static int DoLock(FILE *fp,int l_type) {
   struct flock lk;

   lk.l_type = l_type;
   lk.l_whence = lk.l_start = lk.l_len = 0;
   lk.l_pid = 0;

   while(1) {
      if (fcntl(fileno(fp),F_SETLKW,&lk)==0) return 0;
      else if (errno!=EINTR) return 1;
   }
   return 1;
}

int ReadLock(FILE *fp) { return DoLock(fp,F_RDLCK); }
int WriteLock(FILE *fp) { return DoLock(fp,F_WRLCK); }
void ReleaseLock(FILE *fp) { DoLock(fp,F_UNLCK); }

#endif /* CYGWIN */

void MicroSleep(int microsec) {
/* On systems with select, sleep for "microsec" microseconds */
#if HAVE_SELECT
   struct timeval tv;
   tv.tv_sec=0;
   tv.tv_usec=microsec;
   bselect(0,NULL,NULL,NULL,&tv);
#endif
}

