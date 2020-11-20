/************************************************************************
 * itemfactory.c  GtkItemFactory and friends for Unix/Win32             *
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

#include "gtkport.h"

/* No need to reimplement functions if we have GTK+2 */
#if defined(CYGWIN) || GTK_MAJOR_VERSION > 2

#ifdef CYGWIN

#define OurAccel ACCEL

#else
typedef struct _OurAccel OurAccel;
struct _OurAccel {
  guint key;
  GdkModifierType mods;
  GtkAccelFlags flags;
};
#endif

typedef struct _DPGtkItemFactoryChild DPGtkItemFactoryChild;

struct _DPGtkItemFactoryChild {
  gchar *path;
  GtkWidget *widget;
};

DPGtkItemFactory *dp_gtk_item_factory_new(GtkType container_type,
                                          const gchar *path,
                                          GtkAccelGroup *accel_group)
{
  DPGtkItemFactory *new_fac;

  new_fac = g_new0(DPGtkItemFactory, 1);
  new_fac->path = g_strdup(path);
  new_fac->accel_group = accel_group;
  new_fac->top_widget = gtk_menu_bar_new();
  new_fac->translate_func = NULL;
  return new_fac;
}

static gint PathCmp(const gchar *path1, const gchar *path2)
{
  gint Match = 1;

  if (!path1 || !path2)
    return 0;

  while (*path1 && *path2 && Match) {
    while (*path1 == '_')
      path1++;
    while (*path2 == '_')
      path2++;
    if (*path1 == *path2) {
      path1++;
      path2++;
    } else
      Match = 0;
  }
  if (*path1 || *path2)
    Match = 0;
  return Match;
}

static gchar *TransPath(DPGtkItemFactory *ifactory, gchar *path)
{
  gchar *transpath = NULL;

  if (ifactory->translate_func) {
    transpath =
        (*ifactory->translate_func) (path, ifactory->translate_data);
  }

  return transpath ? transpath : path;
}

static void gtk_item_factory_parse_path(DPGtkItemFactory *ifactory,
                                        gchar *path,
                                        DPGtkItemFactoryChild **parent,
                                        GString *menu_title)
{
  GSList *list;
  DPGtkItemFactoryChild *child;
  gchar *root, *pt, *transpath;

  transpath = TransPath(ifactory, path);
  pt = strrchr(transpath, '/');
  if (pt)
    g_string_assign(menu_title, pt + 1);

  pt = strrchr(path, '/');
  if (!pt)
    return;
  root = g_strdup(path);
  root[pt - path] = '\0';

  for (list = ifactory->children; list; list = g_slist_next(list)) {
    child = (DPGtkItemFactoryChild *)list->data;
    if (PathCmp(child->path, root) == 1) {
      *parent = child;
      break;
    }
  }
  g_free(root);
}

static gboolean gtk_item_factory_parse_accel(DPGtkItemFactory *ifactory,
                                             gchar *accelerator,
                                             GString *menu_title,
                                             OurAccel *accel)
{
  gchar *apt;

  if (!accelerator)
    return FALSE;

  apt = accelerator;
#ifdef CYGWIN
  accel->fVirt = 0;
  accel->key = 0;
  accel->cmd = 0;
  g_string_append(menu_title, "\t");
#else
  accel->key = 0;
  accel->mods = 0;
  accel->flags = GTK_ACCEL_VISIBLE;
#endif

  if (strncmp(apt, "<control>", 9) == 0) {
#ifdef CYGWIN
    accel->fVirt |= FCONTROL;
    g_string_append(menu_title, "Ctrl+");
#else
    accel->mods |= GDK_CONTROL_MASK;
#endif
    apt += 9;
  }

  if (strlen(apt) == 1) {
    accel->key = *apt;
#ifdef CYGWIN
    g_string_append_c(menu_title, *apt);
    accel->fVirt |= FVIRTKEY;
#endif
  } else if (strcmp(apt, "F1") == 0) {
#ifdef CYGWIN
    g_string_append(menu_title, apt);
    accel->fVirt |= FVIRTKEY;
    accel->key = VK_F1;
#else
    accel->key = GDK_KEY_F1;
#endif
  }
  return (accel->key != 0);
}

void dp_gtk_item_factory_create_item(DPGtkItemFactory *ifactory,
                                     DPGtkItemFactoryEntry *entry,
                                     gpointer callback_data,
                                     guint callback_type)
{
  DPGtkItemFactoryChild *new_child, *parent = NULL;
  GString *menu_title;
  GtkWidget *menu_item, *menu;
  OurAccel accel;
  gboolean haveaccel;

  new_child = g_new0(DPGtkItemFactoryChild, 1);

  new_child->path = g_strdup(entry->path);

  menu_title = g_string_new("");

  gtk_item_factory_parse_path(ifactory, new_child->path, &parent,
                              menu_title);
  haveaccel =
      gtk_item_factory_parse_accel(ifactory, entry->accelerator,
                                   menu_title, &accel);

  if (entry->item_type && strcmp(entry->item_type, "<CheckItem>") == 0) {
    menu_item = gtk_check_menu_item_new_with_mnemonic(menu_title->str);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
  } else {
    menu_item = gtk_menu_item_new_with_mnemonic(menu_title->str);
    if (entry->item_type && strcmp(entry->item_type, "<LastBranch>") == 0) {
      gtk_menu_item_set_right_justified(GTK_MENU_ITEM(menu_item), TRUE);
    }
  }
  new_child->widget = menu_item;
  if (entry->callback) {
    gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
                       entry->callback, callback_data);
  }

  if (parent) {
    menu = GTK_WIDGET(GTK_MENU_ITEM(parent->widget)->submenu);
    if (!menu) {
      menu = gtk_menu_new();
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent->widget), menu);
    }
    gtk_menu_append(GTK_MENU(menu), menu_item);
  } else {
    gtk_menu_bar_append(GTK_MENU_BAR(ifactory->top_widget), menu_item);
  }

  if (haveaccel && ifactory->accel_group) {
#ifdef CYGWIN
    GTK_MENU_ITEM(menu_item)->accelind =
        gtk_accel_group_add(ifactory->accel_group, &accel);
#else
    gtk_widget_add_accelerator(menu_item, "activate", ifactory->accel_group,
		               accel.key, accel.mods, accel.flags);
#endif
  }

  g_string_free(menu_title, TRUE);

  ifactory->children = g_slist_append(ifactory->children, new_child);
}

void dp_gtk_item_factory_create_items(DPGtkItemFactory *ifactory,
                                      guint n_entries,
                                      DPGtkItemFactoryEntry *entries,
                                      gpointer callback_data)
{
  gint i;

  for (i = 0; i < n_entries; i++) {
    dp_gtk_item_factory_create_item(ifactory, &entries[i], callback_data, 0);
  }
}

GtkWidget *dp_gtk_item_factory_get_widget(DPGtkItemFactory *ifactory,
                                          const gchar *path)
{
  gint root_len;
  GSList *list;
  DPGtkItemFactoryChild *child;

  root_len = strlen(ifactory->path);
  if (!path || strlen(path) < root_len)
    return NULL;

  if (strncmp(ifactory->path, path, root_len) != 0)
    return NULL;
  if (strlen(path) == root_len)
    return ifactory->top_widget;

  for (list = ifactory->children; list; list = g_slist_next(list)) {
    child = (DPGtkItemFactoryChild *)list->data;
    if (PathCmp(child->path, &path[root_len]) == 1)
      return child->widget;
  }
  return NULL;
}

void dp_gtk_item_factory_set_translate_func(DPGtkItemFactory *ifactory,
                                            DPGtkTranslateFunc func,
                                            gpointer data,
                                            GtkDestroyNotify notify)
{
  ifactory->translate_func = func;
  ifactory->translate_data = data;
}

#endif   /* defined(CYGWIN) || GTK_MAJOR_VERSION > 2 */
