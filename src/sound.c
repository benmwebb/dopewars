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

#ifdef PLUGINS
#include <dlfcn.h>
#else
#include "plugins/sound_sdl.h"
#include "plugins/sound_esd.h"
#include "plugins/sound_winmm.h"
#endif

#include "sound.h"

static SoundDriver *driver = NULL;
typedef SoundDriver *(*InitFunc)(void);
void *soundmodule = NULL;

static void AddPlugin(InitFunc ifunc)
{
  driver = (*ifunc)();
  if (driver) {
    g_print("%s sound plugin init OK\n", driver->name);
  }
}

void SoundInit(void)
{
#ifdef PLUGINS
  InitFunc ifunc;
  
  soundmodule = dlopen("libsound_esd.so", RTLD_NOW);
  if (!soundmodule) {
    /* FIXME: using dlerror() here causes a segfault later in the program */
    g_print("dlopen failed\n");
    return;
  }
  ifunc = dlsym(soundmodule, "sound_esd_init");
  if (ifunc) {
    AddPlugin(ifunc);
  } else {
    g_print("dlsym failed: %s\n", dlerror());
  }
#else
#ifdef HAVE_ESD
  AddPlugin(sound_esd_init);
#endif
#ifdef HAVE_SDL_MIXER
  AddPlugin(sound_sdl_init);
#endif
#ifdef HAVE_WINMM
  AddPlugin(sound_winmm_init);
#endif
#endif
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
#ifdef PLUGINS
  if (soundmodule) {
    dlclose(soundmodule);
  }
#endif
}

void SoundPlay(const gchar *snd)
{
  if (driver && driver->play && snd && snd[0]) {
    driver->play(snd);
  }
}
