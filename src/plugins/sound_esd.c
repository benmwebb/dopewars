/************************************************************************
 * sound_esd.c    dopewars sound system (ESD/esound driver)             *
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

#ifdef HAVE_ESD
#include <stdio.h>
#include <string.h>
#include <esd.h>
#include <glib.h>
#include "../sound.h"

#define MAXCACHE 6

struct SoundCache {
  int esdid;
  gchar *name;
} cache[MAXCACHE];
  

static int sock, nextcache;

static gboolean SoundOpen_ESD(void)
{
  int i;

  sock = esd_open_sound(NULL);
  for (i = 0; i < MAXCACHE; i++) {
    cache[i].esdid = -1;
    cache[i].name = NULL;
  }
  nextcache = 0;
  return TRUE;
}

static void SoundClose_ESD(void)
{
  int i;

  for (i = 0; i < MAXCACHE; i++) {
    g_free(cache[i].name);
    if (cache[i].esdid != -1) {
      esd_sample_free(sock, cache[i].esdid);
    }
  }
  esd_close(sock);
}

static void SoundPlay_ESD(const gchar *snd)
{
  int i;

  for (i = 0; i < MAXCACHE; i++) {
    if (cache[i].name && strcmp(cache[i].name, snd) == 0) {
      esd_sample_play(sock, cache[i].esdid);
      return;
    }
  }

  if (cache[nextcache].esdid != -1) {
    esd_sample_free(sock, cache[nextcache].esdid);
    g_free(cache[nextcache].name);
  }
  cache[nextcache].esdid = esd_file_cache(sock, "", snd);
  cache[nextcache].name = g_strdup(snd);
  esd_sample_play(sock, cache[nextcache].esdid);
  nextcache = (nextcache + 1) % MAXCACHE;
}

SoundDriver *sound_esd_init(void)
{
  static SoundDriver driver;

  driver.name = "esd";
  driver.open = SoundOpen_ESD;
  driver.close = SoundClose_ESD;
  driver.play = SoundPlay_ESD;
  return &driver;
}

#endif /* HAVE_ESD */
