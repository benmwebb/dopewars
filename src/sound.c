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
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <dlfcn.h>
#else
#include "plugins/sound_sdl.h"
#include "plugins/sound_esd.h"
#include "plugins/sound_winmm.h"
#endif

#include "sound.h"

static SoundDriver *driver = NULL;
static GSList *driverlist = NULL;
typedef SoundDriver *(*InitFunc)(void);
static gboolean module_open = FALSE;

const gchar *GetPluginName(GSList **listpt)
{
  if (!*listpt) {
    *listpt = driverlist;
  } else {
    *listpt = g_slist_next(*listpt);
  }
  if (*listpt) {
    SoundDriver *drivpt = (SoundDriver *)(*listpt)->data;

    if (drivpt) {
      return drivpt->name;
    }
  }
  return NULL;
}

static void AddPlugin(InitFunc ifunc, void *module)
{
  driver = (*ifunc)();
  if (driver) {
    g_print("%s sound plugin init OK\n", driver->name);
    driver->module = module;
    driverlist = g_slist_append(driverlist, driver);
  }
}

#ifdef PLUGINS
static void OpenModule(const gchar *modname, const gchar *fullname)
{
  InitFunc ifunc;
  gint len = strlen(modname);
  void *soundmodule;

  if (len > 6 && strncmp(modname, "lib", 3) == 0
      && strcmp(modname + len - 3, ".so") == 0) {
    GString *funcname;

    soundmodule = dlopen(fullname, RTLD_NOW);
    if (!soundmodule) {
      /* FIXME: using dlerror() here causes a segfault later in the program */
      g_print("dlopen of %s failed: %s\n", fullname, dlerror());
      return;
    }

    funcname = g_string_new(modname);
    g_string_truncate(funcname, len - 3);
    g_string_erase(funcname, 0, 3);
    g_string_append(funcname, "_init");
    ifunc = dlsym(soundmodule, funcname->str);
    if (ifunc) {
      AddPlugin(ifunc, soundmodule);
    } else {
      g_print("dlsym (%s) failed: %s\n", funcname->str, dlerror());
      dlclose(soundmodule);
    }
    g_string_free(funcname, TRUE);
  }
}

static void ScanPluginDir(const gchar *dirname)
{
  DIR *dir;

  dir = opendir(dirname);
  if (dir) {
    struct dirent *fileinfo;
    GString *modname;

    modname = g_string_new("");
    do {
      fileinfo = readdir(dir);
      if (fileinfo) {
        g_string_assign(modname, dirname);
	g_string_append_c(modname, G_DIR_SEPARATOR);
	g_string_append(modname, fileinfo->d_name);
        OpenModule(fileinfo->d_name, modname->str);
      }
    } while (fileinfo);
    g_string_free(modname, TRUE);
    closedir(dir);
  } else {
    g_print("Cannot open dir %s\n", dirname);
  }
}
#endif

void SoundInit(void)
{
#ifdef PLUGINS
  ScanPluginDir(PLUGINDIR);
  ScanPluginDir("src/plugins/.libs");
  ScanPluginDir("plugins/.libs");
#else
#ifdef HAVE_ESD
  AddPlugin(sound_esd_init, NULL);
#endif
#ifdef HAVE_SDL_MIXER
  AddPlugin(sound_sdl_init, NULL);
#endif
#ifdef HAVE_WINMM
  AddPlugin(sound_winmm_init, NULL);
#endif
#endif
  module_open = FALSE;
}

void SoundOpen(gchar *drivername)
{
  if (driver && driver->open && !module_open) {
    driver->open();
    module_open = TRUE;
  }
}

void SoundClose(void)
{
#ifdef PLUGINS
  GSList *listpt;
  SoundDriver *listdriv;
#endif

  if (driver && driver->close && module_open) {
    driver->close();
    module_open = FALSE;
  }
#ifdef PLUGINS
  for (listpt = driverlist; listpt; listpt = g_slist_next(listpt)) {
    listdriv = (SoundDriver *)listpt->data;
    if (listdriv && listdriv->module) {
      dlclose(listdriv->module);
    }
  }
#endif
  g_slist_free(driverlist);
  driverlist = NULL;
}

void SoundPlay(const gchar *snd)
{
  if (driver && driver->play && snd && snd[0]) {
    driver->play(snd);
  }
}
