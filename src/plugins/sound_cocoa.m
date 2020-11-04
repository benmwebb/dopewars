/************************************************************************
 * sound_cocoa.m  Implementation of dopewars sound system (Cocoa driver)*
 * Copyright (C)  1998-2020  Ben Webb                                   *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_COCOA
#include <glib.h>
#include "../sound.h"

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

/* Cache player by name of sound file */
NSMutableDictionary *play_by_name;

static gboolean SoundOpen_Cocoa(void)
{
  play_by_name = [[NSMutableDictionary alloc] init];
  return TRUE;
}

static void SoundClose_Cocoa(void)
{
  [play_by_name release];
}

static void SoundPlay_Cocoa(const gchar *snd)
{
  NSString *sound = [[NSString alloc] initWithUTF8String:snd];
  NSSound *p;
  p = [play_by_name objectForKey:sound];
  if (!p) {
    p = [[NSSound alloc] initWithContentsOfFile:sound byReference:YES];
    [play_by_name setObject:p forKey:sound];
  }
  [p play];
}

SoundDriver *sound_cocoa_init(void)
{
  static SoundDriver driver;

  driver.name = "cocoa";
  driver.open = SoundOpen_Cocoa;
  driver.close = SoundClose_Cocoa;
  driver.play = SoundPlay_Cocoa;
  return &driver;
}
#endif
