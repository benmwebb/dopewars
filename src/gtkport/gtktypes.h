/************************************************************************
 * gtktypes.h     Custom types for gtkport code                         *
 * Copyright (C)  2002-2020  Ben Webb                                   *
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

#ifndef __GTKTYPES_H__
#define __GTKTYPES_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef CYGWIN

#define MB_IMMRETURN 0

#define MYWM_SOCKETDATA (WM_USER+100)
#define MYWM_TASKBAR    (WM_USER+101)
#define MYWM_SERVICE    (WM_USER+102)

#define GDK_MOD1_MASK 0

extern HICON mainIcon;

#define GDK_KP_0 0xFFB0
#define GDK_KP_1 0xFFB1
#define GDK_KP_2 0xFFB2
#define GDK_KP_3 0xFFB3
#define GDK_KP_4 0xFFB4
#define GDK_KP_5 0xFFB5
#define GDK_KP_6 0xFFB6
#define GDK_KP_7 0xFFB7
#define GDK_KP_8 0xFFB8
#define GDK_KP_9 0xFFB9

typedef gint (*GtkFunction) (gpointer data);
typedef void (*GtkDestroyNotify) (gpointer data);

typedef struct _GtkClass GtkClass;
typedef struct _GtkObject GtkObject;

typedef struct _GtkRequisition GtkRequisition;
typedef struct _GtkAllocation GtkAllocation;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkSignalType GtkSignalType;
typedef struct _GtkContainer GtkContainer;

typedef void (*GtkSignalFunc) ();
typedef void (*GtkSignalMarshaller) (GtkObject *object, GSList *actions,
                                     GtkSignalFunc default_action,
                                     va_list args);

typedef struct _GdkColor GdkColor;
typedef struct _GtkStyle GtkStyle;
typedef struct _GtkMenuShell GtkMenuShell;
typedef struct _GtkMenuBar GtkMenuBar;
typedef struct _GtkMenuItem GtkMenuItem;
typedef struct _GtkMenu GtkMenu;
typedef struct _GtkAdjustment GtkAdjustment;
typedef struct _GtkSeparator GtkSeparator;
typedef struct _GtkMisc GtkMisc;
typedef struct _GtkProgressBar GtkProgressBar;
typedef struct _GtkHSeparator GtkHSeparator;
typedef struct _GtkVSeparator GtkVSeparator;
typedef struct _GtkAccelGroup GtkAccelGroup;
typedef struct _GtkPanedChild GtkPanedChild;
typedef struct _GtkPaned GtkPaned;
typedef struct _GtkVPaned GtkVPaned;
typedef struct _GtkHPaned GtkHPaned;
typedef struct _GtkOptionMenu GtkOptionMenu;

struct _GtkAccelGroup {
  ACCEL *accel;                 /* list of ACCEL structures */
  gint numaccel;
};

struct _GtkSignalType {
  gchar *name;
  GtkSignalMarshaller marshaller;
  GtkSignalFunc default_action;
};

struct _GdkColor {
  gulong  pixel;
  gushort red;
  gushort green;
  gushort blue;
};

struct _GtkStyle {
  GdkColor fg[5];
  GdkColor bg[5];
};

typedef gboolean (*GtkWndProc) (GtkWidget *widget, UINT msg,
                                WPARAM wParam, LPARAM lParam, gboolean *dodef);

struct _GtkClass {
  gchar *Name;
  GtkClass *parent;
  gint Size;
  GtkSignalType *signals;
  GtkWndProc wndproc;
};

typedef GtkClass *GtkType;

struct _GtkObject {
  GtkClass *klass;
  GData *object_data;
  GData *signals;
  guint32 flags;
};

struct _GtkAdjustment {
  GtkObject object;
  gfloat value, lower, upper;
  gfloat step_increment, page_increment, page_size;
};

struct _GtkRequisition {
  gint16 width, height;
};

struct _GtkAllocation {
  gint16 x, y, width, height;
};

struct _GtkWidget {
  GtkObject object;
  HWND hWnd;
  GtkRequisition requisition;
  GtkAllocation allocation;
  GtkRequisition usize;
  GtkWidget *parent;
};

struct _GtkContainer {
  GtkWidget widget;
  GtkWidget *child;
  guint border_width:16;
};

#endif /* CYGWIN */

#endif /* __GTKTYPES_H__ */
