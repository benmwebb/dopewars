/************************************************************************
 * sound.c        dopewars sound system                                 *
 * Copyright (C)  1998-2004  Ben Webb                                   *
 *                Email: benwebb@users.sf.net                           *
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
#include <string.h>

#ifdef PLUGINS
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>
#else
#include "plugins/sound_sdl.h"
#include "plugins/sound_esd.h"
#include "plugins/sound_winmm.h"
#endif

#include "dopewars.h"
#include "log.h"
#include "nls.h"
#include "sound.h"

#define NOPLUGIN "none"

static SoundDriver *driver = NULL;
static GSList *driverlist = NULL;
typedef SoundDriver *(*InitFunc)(void);
static gboolean sound_enabled = TRUE;

gchar *GetPluginList(void)
{
  GSList *listpt;
  GString *plugins;
  gchar *retstr;

  plugins = g_string_new("\""NOPLUGIN"\"");
  for (listpt = driverlist; listpt; listpt = g_slist_next(listpt)) {
    SoundDriver *drivpt = (SoundDriver *)listpt->data;

    if (drivpt && drivpt->name) {
      g_string_sprintfa(plugins, ", \"%s\"", drivpt->name);
    }
  }
  retstr = plugins->str;
  g_string_free(plugins, FALSE);
  return retstr;
}

static void AddPlugin(InitFunc ifunc, void *module)
{
  SoundDriver *newdriver = (*ifunc)();

  if (newdriver) {
    dopelog(5, 0, "%s sound plugin init OK", newdriver->name);
    newdriver->module = module;
    driverlist = g_slist_append(driverlist, newdriver);
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
      dopelog(3, 0, "dlopen of %s failed: %s", fullname, dlerror());
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
      dopelog(3, 0, "dlsym (%s) failed: %s", funcname->str, dlerror());
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
    dopelog(5, 0, "Cannot open dir %s", dirname);
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
  driver = NULL;
}

static SoundDriver *GetPlugin(gchar *drivername)
{
  GSList *listpt;

  for (listpt = driverlist; listpt; listpt = g_slist_next(listpt)) {
    SoundDriver *drivpt = (SoundDriver *)listpt->data;

    if (drivpt && drivpt->name
        && (!drivername || strcmp(drivpt->name, drivername) == 0)) {
      return drivpt;
    }
  }
  return NULL;
}

void SoundOpen(gchar *drivername)
{
  if (!drivername || strcmp(drivername, NOPLUGIN) != 0) {
    driver = GetPlugin(drivername);
    if (driver) {
      if (driver->open) {
        dopelog(3, 0, "Using plugin %s", driver->name);
        driver->open();
      }
    } else if (drivername) {
      gchar *plugins, *err;

      plugins = GetPluginList();
      err = g_strdup_printf(_("Invalid plugin \"%s\" selected.\n"
                              "(%s available; now using \"%s\".)"),
                            drivername, plugins, NOPLUGIN);
      g_log(NULL, G_LOG_LEVEL_CRITICAL, err);
      g_free(plugins);
      g_free(err);
    }
  }
  sound_enabled = TRUE;
}

void SoundClose(void)
{
#ifdef PLUGINS
  GSList *listpt;
  SoundDriver *listdriv;
#endif

  if (driver && driver->close) {
    driver->close();
    driver = NULL;
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
  if (sound_enabled && driver && driver->play && snd && snd[0]) {
    driver->play(snd);
  }
}

void SoundEnable(gboolean enable)
{
  sound_enabled = enable;
}

gboolean IsSoundEnabled(void)
{
  return sound_enabled;
}
