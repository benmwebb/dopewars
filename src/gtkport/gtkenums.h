/************************************************************************
 * gtkenums.h     Enumerated types for gtkport code                     *
 * Copyright (C)  2002-2003  Ben Webb                                   *
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

#ifndef __GTKENUMS_H__
#define __GTKENUMS_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef CYGWIN

typedef enum {
  GTK_WINDOW_TOPLEVEL, GTK_WINDOW_DIALOG, GTK_WINDOW_POPUP
} GtkWindowType;

typedef enum {
  GTK_ACCEL_VISIBLE        = 1 << 0,
  GTK_ACCEL_SIGNAL_VISIBLE = 1 << 1
} GtkAccelFlags;

typedef enum {
  GTK_BUTTONBOX_SPREAD,
  GTK_BUTTONBOX_EDGE,
  GTK_BUTTONBOX_START,
  GTK_BUTTONBOX_END
} GtkButtonBoxStyle;

typedef enum {
  GTK_STATE_NORMAL,
  GTK_STATE_ACTIVE,
  GTK_STATE_PRELIGHT,
  GTK_STATE_SELECTED,
  GTK_STATE_INSENSITIVE
} GtkStateType;

typedef enum {
  GTK_VISIBILITY_NONE,
  GTK_VISIBILITY_PARTIAL,
  GTK_VISIBILITY_FULL
} GtkVisibility;

typedef enum {
  GTK_PROGRESS_LEFT_TO_RIGHT,
  GTK_PROGRESS_RIGHT_TO_LEFT,
  GTK_PROGRESS_BOTTOM_TO_TOP,
  GTK_PROGRESS_TOP_TO_BOTTOM
} GtkProgressBarOrientation;

typedef enum {
  GTK_EXPAND = 1 << 0,
  GTK_SHRINK = 1 << 1,
  GTK_FILL   = 1 << 2
} GtkAttachOptions;

typedef enum {
  GTK_SELECTION_SINGLE,
  GTK_SELECTION_BROWSE,
  GTK_SELECTION_MULTIPLE,
  GTK_SELECTION_EXTENDED
} GtkSelectionMode;

typedef enum {
  GDK_INPUT_READ      = 1 << 0,
  GDK_INPUT_WRITE     = 1 << 1,
  GDK_INPUT_EXCEPTION = 1 << 2
} GdkInputCondition;

typedef enum {
  GTK_SHADOW_NONE,
  GTK_SHADOW_IN,
  GTK_SHADOW_OUT,
  GTK_SHADOW_ETCHED_IN,
  GTK_SHADOW_ETCHED_OUT
} GtkShadowType;

typedef enum {
  GTK_JUSTIFY_LEFT,
  GTK_JUSTIFY_RIGHT,
  GTK_JUSTIFY_CENTER,
  GTK_JUSTIFY_FILL
} GtkJustification;

typedef enum {
  GTK_REALIZED    = 1 << 6,
  GTK_VISIBLE     = 1 << 8,
  GTK_SENSITIVE   = 1 << 10,
  GTK_CAN_FOCUS   = 1 << 11,
  GTK_HAS_FOCUS   = 1 << 12,
  GTK_CAN_DEFAULT = 1 << 13,
  GTK_IS_DEFAULT  = 1 << 14
} GtkWidgetFlags;

typedef enum
{
  GDK_WINDOW_TYPE_HINT_NORMAL,
  GDK_WINDOW_TYPE_HINT_DIALOG,
  GDK_WINDOW_TYPE_HINT_MENU,
  GDK_WINDOW_TYPE_HINT_TOOLBAR
} GdkWindowTypeHint;

typedef enum
{
  GTK_WIN_POS_NONE,
  GTK_WIN_POS_CENTER,
  GTK_WIN_POS_MOUSE,
  GTK_WIN_POS_CENTER_ALWAYS,
  GTK_WIN_POS_CENTER_ON_PARENT
} GtkWindowPosition;

#endif /* CYGWIN */

#endif /* __GTKENUMS_H__ */
