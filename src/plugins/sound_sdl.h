/************************************************************************
 * sound_sdl.h    Header file for dopewars sound system (SDL driver)    *
 * Copyright (C)  1998-2003  Ben Webb                                   *
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

#ifndef __DP_SOUND_SDL_H__
#define __DP_SOUND_SDL_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sound.h"

#ifdef HAVE_SDL_MIXER
SoundDriver *sound_sdl_init(void);
#endif /* HAVE_SDL_MIXER */

#endif /* __DP_SOUND_SDL_H__ */
