/************************************************************************
 * itemfactory.h  GtkItemFactory and friends for Unix/Win32             *
 * Copyright (C)  1998-2020  Ben Webb                                   *
 *                Email: benwebb@users.sf.net                           *
 *                WWW: https://dopewars.sourceforge.io/                 *
 *                                                                      *
 * When using GTK+3, which has removed GtkItemFactory, or Win32,        *
 * provide our own implementation; on GTK+2, use the implementation in  *
 * GTK+ itself.                                                         *
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

#ifndef __ITEM_FACTORY_H__
#define __ITEM_FACTORY_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

/* Use GTK+2's own implementation of these functions */
#if GTK_MAJOR_VERSION == 2
#define DPGtkTranslateFunc GtkTranslateFunc
#define DPGtkItemFactoryCallback GtkItemFactoryCallback
#define DPGtkItemFactoryEntry GtkItemFactoryEntry
#define DPGtkItemFactory GtkItemFactory

GtkItemFactory *dp_gtk_item_factory_new(const gchar *path,
                                        GtkAccelGroup *accel_group);

#define dp_gtk_item_factory_create_items gtk_item_factory_create_items
#define dp_gtk_item_factory_create_item gtk_item_factory_create_item
#define dp_gtk_item_factory_get_widget gtk_item_factory_get_widget
#define dp_gtk_item_factory_set_translate_func gtk_item_factory_set_translate_func
#else

typedef gchar *(*DPGtkTranslateFunc) (const gchar *path, gpointer func_data);

typedef void (*DPGtkItemFactoryCallback) ();

typedef struct _DPGtkItemFactoryEntry DPGtkItemFactoryEntry;
typedef struct _DPGtkItemFactory DPGtkItemFactory;

struct _DPGtkItemFactoryEntry {
  gchar *path;
  gchar *accelerator;
  DPGtkItemFactoryCallback callback;
  guint callback_action;
  gchar *item_type;
};

struct _DPGtkItemFactory {
  GSList *children;
  gchar *path;
  GtkAccelGroup *accel_group;
  GtkWidget *top_widget;
  DPGtkTranslateFunc translate_func;
  gpointer translate_data;
};

DPGtkItemFactory *dp_gtk_item_factory_new(const gchar *path,
                                          GtkAccelGroup *accel_group);
void dp_gtk_item_factory_create_item(DPGtkItemFactory *ifactory,
                                     DPGtkItemFactoryEntry *entry,
                                     gpointer callback_data,
                                     guint callback_type);
void dp_gtk_item_factory_create_items(DPGtkItemFactory *ifactory,
                                      guint n_entries,
                                      DPGtkItemFactoryEntry *entries,
                                      gpointer callback_data);
GtkWidget *dp_gtk_item_factory_get_widget(DPGtkItemFactory *ifactory,
                                          const gchar *path);
void dp_gtk_item_factory_set_translate_func(DPGtkItemFactory *ifactory,
                                            DPGtkTranslateFunc func,
                                            gpointer data,
                                            GDestroyNotify notify);
#endif /* GTK+2 */

#endif /* __ITEM_FACTORY_H__ */
