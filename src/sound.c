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
#include <dlfcn.h>

#include "sound.h"

static SoundDriver *driver = NULL;
typedef SoundDriver *(*InitFunc)(void);
void *soundmodule = NULL;

void SoundInit(void)
{
  InitFunc ifunc;
  
  soundmodule = dlopen("sound.so", RTLD_NOW);
  if (!soundmodule) {
    /* FIXME: using dlerror() here causes a segfault later in the program */
    g_print("dlopen failed\n");
    return;
  }
  ifunc = dlsym(soundmodule, "init");
  if (ifunc) {
    driver = (*ifunc)();
    if (driver) {
      g_print("%s sound plugin init OK\n", driver->name);
    }
  } else {
    g_print("dlsym failed: %s\n", dlerror());
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
    dlclose(soundmodule);
  }
}

void SoundPlay(const gchar *snd)
{
  if (driver && driver->play && snd && snd[0]) {
    driver->play(snd);
  }
}
