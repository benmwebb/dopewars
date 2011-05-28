/************************************************************************
 * sound_sdl.c    dopewars sound system (SDL driver)                    *
 * Copyright (C)  1998-2011  Ben Webb                                   *
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

#ifdef HAVE_SDL_MIXER
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <glib.h>
#include "../sound.h"

struct ChannelStruct {
  Mix_Chunk *chunk;
  gchar *name;
} channel[MIX_CHANNELS];
  
static gboolean SoundOpen_SDL(void)
{
  const int audio_rate = MIX_DEFAULT_FREQUENCY;
  const int audio_format = MIX_DEFAULT_FORMAT;
  const int audio_channels = 2;
  int i;

  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    return FALSE;
  }

  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, 4096) < 0) {
    SDL_Quit();
    return FALSE;
  }
  Mix_AllocateChannels(MIX_CHANNELS);

  for (i = 0; i < MIX_CHANNELS; i++) {
    channel[i].chunk = NULL;
    channel[i].name = NULL;
  }
  return TRUE;
}

static void SoundClose_SDL(void)
{
  int i;

  for (i = 0; i < MIX_CHANNELS; i++) {
    g_free(channel[i].name);
    if (channel[i].chunk) {
      Mix_FreeChunk(channel[i].chunk);
    }
  }
  Mix_CloseAudio();
  SDL_Quit();
}

static void SoundPlay_SDL(const gchar *snd)
{
  int i, chan_num;
  Mix_Chunk *chunk;

  for (i = 0; i < MIX_CHANNELS; i++) {
    if (channel[i].name && strcmp(channel[i].name, snd) == 0) {
      Mix_PlayChannel(-1, channel[i].chunk, 0);
      return;
    }
  }

  chunk = Mix_LoadWAV(snd);
  if (!chunk) {
    return;
  }

  chan_num = Mix_PlayChannel(-1, chunk, 0);
  if (chan_num < 0) {
    Mix_FreeChunk(chunk);
    return;
  }

  if (channel[chan_num].chunk) {
    Mix_FreeChunk(channel[chan_num].chunk);
    g_free(channel[chan_num].name);
  }

  channel[chan_num].chunk = chunk;
  channel[chan_num].name = g_strdup(snd);
}

SoundDriver *sound_sdl_init(void)
{
  static SoundDriver driver;

  driver.name = "sdl";
  driver.open = SoundOpen_SDL;
  driver.close = SoundClose_SDL;
  driver.play = SoundPlay_SDL;
  return &driver;
}

#endif /* HAVE_SDL_MIXER */
