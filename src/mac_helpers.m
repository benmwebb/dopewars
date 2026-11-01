/************************************************************************
 * mac_helpers.m  Helper functions for Mac builds                       *
 * Copyright (C)  1998-2022  Ben Webb                                   *
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

#import <AppKit/AppKit.h>

#include "mac_helpers.h"

void mac_open_url(const char *url)
{
  NSString *urlstr = [[NSString alloc] initWithUTF8String:url];
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:urlstr]];
}

void mac_activate_app(void)
{
  [NSApp activateIgnoringOtherApps:YES];
}

int mac_dark_mode(void)
{
  NSAppearanceName name = [[NSApp effectiveAppearance]
      bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua,
                                          NSAppearanceNameDarkAqua]];
  return [name isEqualToString:NSAppearanceNameDarkAqua];
}

/* Calls a C function when the application's appearance changes */
@interface DPAppearanceObserver : NSObject {
  void (*func)(void);
}
- (id)initWithFunc:(void (*)(void))f;
@end

@implementation DPAppearanceObserver
- (id)initWithFunc:(void (*)(void))f
{
  if ((self = [super init])) {
    func = f;
  }
  return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object
                        change:(NSDictionary *)change context:(void *)context
{
  func();
}
@end

void mac_on_appearance_changed(void (*func)(void))
{
  /* The observer lives for as long as the application */
  DPAppearanceObserver *observer = [[DPAppearanceObserver alloc]
                                       initWithFunc:func];
  [NSApp addObserver:observer forKeyPath:@"effectiveAppearance"
             options:0 context:NULL];
}
