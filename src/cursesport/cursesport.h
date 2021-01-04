/************************************************************************
 * cursesport.h   Portability functions to enable curses applications   *
 *                     to be built on Win32 systems                     *
 * Copyright (C)  1998-2021  Ben Webb                                   *
 *                Email: benwebb@users.sf.net                           *
 *                WWW: https://dopewars.sourceforge.io/                 *
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

#ifndef __CURSESPORT_H__
#define __CURSESPORT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#ifdef CYGWIN                   /* Definitions for native Win32 build */
#include <winsock2.h>
#include <windows.h>
#include <string.h>

#include <stdio.h>
#include <conio.h>

extern int COLS, LINES;

#define COLOR_MAGENTA 1
#define COLOR_BLACK   2
#define COLOR_WHITE   3
#define COLOR_BLUE    4
#define COLOR_RED     5

#define COLOR_PAIR(i) ((i) << 16)

#define ACS_VLINE       0x2502
#define ACS_ULCORNER    0x250c
#define ACS_HLINE       0x2500
#define ACS_URCORNER    0x2510
#define ACS_TTEE        0x252c
#define ACS_LLCORNER    0x2514
#define ACS_LRCORNER    0x2518
#define ACS_BTEE        0x2534
#define ACS_LTEE        0x251c
#define ACS_RTEE        0x2524

typedef int SCREEN;

#define stdscr        0
#define curscr        0
#define KEY_ENTER     13
#define KEY_BACKSPACE 8
#define A_BOLD        0

SCREEN *newterm(void *, void *, void *);
void refresh(void);
#define wrefresh(win) refresh()
void start_color(void);
void init_pair(int index, WORD fg, WORD bg);
void cbreak(void);
void noecho(void);
void nodelay(void *, char);
void keypad(void *, char);
void curs_set(BOOL visible);
void endwin(void);
void move(int y, int x);
void attrset(int newAttr);
void addstr(const char *str);
void addch(unsigned ch);
void mvaddstr(int x, int y, const char *str);
void mvaddch(int x, int y, int ch);

#define erase() clear_screen()

void standout(void);
void standend(void);
void endwin(void);

#else /* Definitions for Unix build */
#include <errno.h>

/* Include a suitable curses-type library */
#if HAVE_LIBNCURSES || defined(CURSES_HAVE_NCURSES_H)
#include <ncurses.h>
#elif HAVE_LIBCURSES || defined(CURSES_HAVE_CURSES_H)
#include <curses.h>
#elif defined(CURSES_HAVE_NCURSES_NCURSES_H)
#include <ncurses/ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_CURSES_H)
#include <ncurses/curses.h>
#elif HAVE_LIBCUR_COLR
#include <curses_colr/curses.h>
#endif

#endif /* CYGWIN */

gunichar bgetch(void);

#endif /* __CURSESPORT_H__ */
