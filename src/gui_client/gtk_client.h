/************************************************************************
 * gtk_client.h   dopewars client using the GTK+ toolkit                *
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

#ifndef __GTK_CLIENT_H__
#define __GTK_CLIENT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gtkport/gtkport.h"

extern GtkWidget *MainWindow;

#ifdef CYGWIN
gboolean GtkLoop(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                 gboolean ReturnOnFail);
#else
gboolean GtkLoop(int *argc, char **argv[], gboolean ReturnOnFail);
#endif

void GuiStartGame(void);
GtkWidget *my_hbbox_new(void);

#endif
