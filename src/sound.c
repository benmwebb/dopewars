/************************************************************************
 * sound.c        dopewars sound system                                 *
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

#include <glib.h>
#include <gmodule.h>

#include "sound.h"

static SoundDriver *driver = NULL;
typedef SoundDriver *(*InitFunc)(void);
GModule *soundmodule = NULL;

void SoundInit(void)
{
  InitFunc ifunc;
  gchar *err;
  
  soundmodule = g_module_open("sound.so", G_MODULE_BIND_LAZY);
  if (!soundmodule) {
    g_print("g_module_open failed: %s\n", g_module_error());
    return;
  }
  if (g_module_symbol(soundmodule, "init", (gpointer *)&ifunc)) {
    g_print("module symbol = %p\n", ifunc);
    driver = (*ifunc)();
    if (driver) {
      g_print("Plugin %s init OK\n", driver->name);
    }
  } else {
    g_print("g_module_symbol failed: %s\n", g_module_error());
  }
}

void SoundOpen(gchar *drivername)
{
  if (driver && driver->open) {
    driver->open();
  }
}

void SoundClose(void)
{
  if (driver && driver->close) {
    driver->close();
  }
  if (soundmodule) {
    g_module_close(soundmodule);
  }
}

void SoundPlay(const gchar *snd)
{
  if (driver && driver->play && snd && snd[0]) {
    driver->play(snd);
  }
}
