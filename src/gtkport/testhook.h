/************************************************************************
 * testhook.h     Let tests drive the GTK+ client                       *
 * Copyright (C)  1998-2026  Ben Webb                                   *
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

#ifndef __TESTHOOK_H__
#define __TESTHOOK_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef TEST_HOOKS
#include "gtkport.h"

/* If the DOPEWARS_TEST_CTL environment variable names a port, connect to
   it on 127.0.0.1 and take commands from the test listening there. `menu`
   is the main menu, for the "menu" command. */
void dp_test_hook_start(DPGtkItemFactory *menu);
#endif

#endif /* __TESTHOOK_H__ */
