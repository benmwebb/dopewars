/************************************************************************
 * cursesport.c   Portability functions to enable curses applications   *
 *                     to be built on Win32 systems                     *
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

#include "cursesport.h"

#ifdef CYGWIN                   /* Code for native Win32 build under Cygwin */

int COLS, LINES;

static int Width, Depth;
static CHAR_INFO RealScreen[25][80], VirtualScreen[25][80];
static HANDLE hOut, hIn;
static WORD CurAttr = 0;
static int CurX, CurY;
static WORD Attr[10];

void refresh(void)
{
  int y;
  COORD size, offset;
  SMALL_RECT screenpos;

  for (y = 0; y < Depth; y++) {
    if (memcmp(&RealScreen[y][0], &VirtualScreen[y][0],
               sizeof(CHAR_INFO) * Width) != 0) {
      memcpy(&RealScreen[y][0], &VirtualScreen[y][0],
             Width * sizeof(CHAR_INFO));
      size.X = Width;
      size.Y = 1;
      offset.X = offset.Y = 0;
      screenpos.Left = 0;
      screenpos.Top = y;
      screenpos.Right = Width - 1;
      screenpos.Bottom = y;
      WriteConsoleOutput(hOut, &VirtualScreen[y][0], size,
                         offset, &screenpos);
    }
  }
}

static HANDLE WINAPI GetConHandle(TCHAR *pszName)
{
  SECURITY_ATTRIBUTES sa;

  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;
  return CreateFile(pszName, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    &sa, OPEN_EXISTING, (DWORD)0, (HANDLE)0);
}

SCREEN *newterm(void *a, void *b, void *c)
{
  COORD coord;
  int i;

  coord.X = 80;
  coord.Y = 25;
  Width = COLS = 80;
  Depth = LINES = 25;
  CurAttr = 1 << 8;
  CurX = 0;
  CurY = 0;
  for (i = 0; i < 10; i++)
    Attr[i] = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
  hOut = GetConHandle("CONOUT$");
  hIn = GetConHandle("CONIN$");
  SetConsoleMode(hIn, 0);
  return NULL;
}

void start_color(void)
{
}

void init_pair(int index, WORD fg, WORD bg)
{
  if (index >= 0 && index < 10) {
    Attr[index] = 0;
    switch (fg) {
    case COLOR_MAGENTA:
      Attr[index] |= (FOREGROUND_RED + FOREGROUND_BLUE);
      break;
    case COLOR_BLUE:
      Attr[index] |= FOREGROUND_BLUE;
      break;
    case COLOR_RED:
      Attr[index] |= FOREGROUND_RED;
      break;
    case COLOR_WHITE:
      Attr[index] |= (FOREGROUND_RED + FOREGROUND_BLUE + FOREGROUND_GREEN);
      break;
    }
    switch (bg) {
    case COLOR_MAGENTA:
      Attr[index] |= (BACKGROUND_RED + BACKGROUND_BLUE);
      break;
    case COLOR_BLUE:
      Attr[index] |= BACKGROUND_BLUE;
      break;
    case COLOR_RED:
      Attr[index] |= BACKGROUND_RED;
      break;
    case COLOR_WHITE:
      Attr[index] |= (BACKGROUND_RED + BACKGROUND_BLUE + BACKGROUND_GREEN);
      break;
    }
  }
}

void cbreak(void)
{
}

void noecho(void)
{
}

void nodelay(void *a, char b)
{
}

void keypad(void *a, char b)
{
}

void curs_set(BOOL visible)
{
  CONSOLE_CURSOR_INFO ConCurInfo;

  move(CurY, CurX);
  ConCurInfo.dwSize = 10;
  ConCurInfo.bVisible = visible;
  SetConsoleCursorInfo(hOut, &ConCurInfo);
}

void endwin(void)
{
  CurAttr = 0;
  refresh();
  curs_set(1);
}

void move(int y, int x)
{
  COORD coord;

  CurX = x;
  CurY = y;
  coord.X = x;
  coord.Y = y;
  SetConsoleCursorPosition(hOut, coord);
}

void attrset(WORD newAttr)
{
  CurAttr = newAttr;
}

void addstr(char *str)
{
  int i;

  for (i = 0; i < strlen(str); i++)
    addch(str[i]);
  move(CurY, CurX);
}

void addch(int ch)
{
  int attr;

  VirtualScreen[CurY][CurX].Char.AsciiChar = ch % 256;
  attr = ch >> 8;
  if (attr > 0)
    VirtualScreen[CurY][CurX].Attributes = Attr[attr];
  else
    VirtualScreen[CurY][CurX].Attributes = Attr[CurAttr >> 8];
  if (++CurX >= Width) {
    CurX = 0;
    if (++CurY >= Depth)
      CurY = 0;
  }
}

void mvaddstr(int y, int x, char *str)
{
  move(y, x);
  addstr(str);
}

void mvaddch(int y, int x, int ch)
{
  move(y, x);
  addch(ch);
}

/* 
 * Waits for the user to press a key.
 */
int bgetch(void)
{
  DWORD NumRead;
  char Buffer[10];

  refresh();
  ReadConsole(hIn, Buffer, 1, &NumRead, NULL);
  return (int)(Buffer[0]);
}

void standout(void)
{
}

void standend(void)
{
}

#else /* Code for Unix build */

/* 
 * Calls the curses getch() function; if the key pressed is Ctrl-L
 * then automatically clears and redraws the screen, otherwise
 * passes the key back to the calling routine.
 */
int bgetch()
{
  int c;

  c = getch();
  while (c == '\f') {
    wrefresh(curscr);
    c = getch();
  }
  return c;
}

#endif /* CYGWIN */
