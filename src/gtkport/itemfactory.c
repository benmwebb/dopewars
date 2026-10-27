/************************************************************************
 * itemfactory.c  GtkItemFactory and friends for Unix/Win32             *
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

#include "gtkport.h"

/* No need to reimplement functions if we have GTK+2 */
#if GTK_MAJOR_VERSION == 2
GtkItemFactory *dp_gtk_item_factory_new(const gchar *path,
                                        GtkAccelGroup *accel_group)
{
  return gtk_item_factory_new(GTK_TYPE_MENU_BAR, path, accel_group);
}
#endif

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
#if !defined(CYGWIN) && GTK_MAJOR_VERSION >= 4
  DPGtkItemFactory *ifactory;
  DPGtkItemFactoryChild *parent;
  GMenu *submenu;         /* Only for branches */
  GMenu *parent_menu;     /* Menu that contains this item... */
  gint position;          /* ...and the item's index in that menu */
  GSimpleAction *action;  /* Only for non-branches */
  gchar *detailed_action, *accel;
  gboolean sensitive;
  DPGtkItemFactoryCallback callback;
  gpointer callback_data;
#endif
};

DPGtkItemFactory *dp_gtk_item_factory_new(const gchar *path,
                                          GtkAccelGroup *accel_group)
{
  DPGtkItemFactory *new_fac;

  new_fac = g_new0(DPGtkItemFactory, 1);
  new_fac->path = g_strdup(path);
  new_fac->accel_group = accel_group;
#if !defined(CYGWIN) && GTK_MAJOR_VERSION >= 4
  new_fac->top_menu = g_menu_new();
  new_fac->action_group = g_simple_action_group_new();
  new_fac->top_widget = gtk_popover_menu_bar_new_from_model(
                                   G_MENU_MODEL(new_fac->top_menu));
  gtk_widget_insert_action_group(new_fac->top_widget, "menu",
                                 G_ACTION_GROUP(new_fac->action_group));
#else
  new_fac->top_widget = gtk_menu_bar_new();
#endif
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

#if !defined(CYGWIN) && GTK_MAJOR_VERSION >= 4
static GMenuItem *new_menu_item(DPGtkItemFactoryChild *child,
                                const gchar *label)
{
  GMenuItem *item = g_menu_item_new(label, child->detailed_action);

  if (child->accel) {
    g_menu_item_set_attribute(item, "accel", "s", child->accel);
  }
  return item;
}

static void MenuItemActivate(GSimpleAction *action, GVariant *parameter,
                             gpointer data)
{
  DPGtkItemFactoryChild *child = data;
  GVariant *state = g_action_get_state(G_ACTION(action));

  /* Toggle check items */
  if (state) {
    g_simple_action_set_state(action,
                    g_variant_new_boolean(!g_variant_get_boolean(state)));
    g_variant_unref(state);
  }
  if (child->callback) {
    /* Callbacks are generic G_CALLBACKs, so cast to the real signature */
    ((void (*)(GtkWidget *, gpointer))child->callback)(
                     child->ifactory->top_widget, child->callback_data);
  }
}

static gboolean MenuAccelActivate(GtkWidget *widget, GVariant *args,
                                  gpointer data)
{
  GAction *action = G_ACTION(data);

  if (g_action_get_enabled(action)) {
    g_action_activate(action, NULL);
    return TRUE;
  }
  return FALSE;
}

void dp_gtk_item_factory_create_item(DPGtkItemFactory *ifactory,
                                     DPGtkItemFactoryEntry *entry,
                                     gpointer callback_data,
                                     guint callback_type)
{
  DPGtkItemFactoryChild *new_child, *parent = NULL;
  GString *menu_title;
  GMenuItem *item;
  OurAccel accel;
  gboolean haveaccel;

  new_child = g_new0(DPGtkItemFactoryChild, 1);
  new_child->path = g_strdup(entry->path);
  new_child->ifactory = ifactory;
  new_child->sensitive = TRUE;

  menu_title = g_string_new("");

  gtk_item_factory_parse_path(ifactory, new_child->path, &parent,
                              menu_title);
  haveaccel =
      gtk_item_factory_parse_accel(ifactory, entry->accelerator,
                                   menu_title, &accel);

  new_child->parent = parent;
  new_child->parent_menu = parent ? parent->submenu : ifactory->top_menu;
  if (!new_child->parent_menu) {
    g_warning("Menu item %s has no parent branch", entry->path);
    g_free(new_child->path);
    g_free(new_child);
    g_string_free(menu_title, TRUE);
    return;
  }

  if (entry->item_type && strcmp(entry->item_type, "<Branch>") == 0) {
    new_child->submenu = g_menu_new();
    item = g_menu_item_new_submenu(menu_title->str,
                                   G_MENU_MODEL(new_child->submenu));
  } else {
    gchar *name = g_strdup_printf("item%u",
                                  g_slist_length(ifactory->children));

    if (entry->item_type && strcmp(entry->item_type, "<CheckItem>") == 0) {
      new_child->action = g_simple_action_new_stateful(
                              name, NULL, g_variant_new_boolean(TRUE));
    } else {
      new_child->action = g_simple_action_new(name, NULL);
    }
    new_child->detailed_action = g_strdup_printf("menu.%s", name);
    g_free(name);

    new_child->callback = entry->callback;
    new_child->callback_data = callback_data;
    g_signal_connect(G_OBJECT(new_child->action), "activate",
                     G_CALLBACK(MenuItemActivate), new_child);
    g_action_map_add_action(G_ACTION_MAP(ifactory->action_group),
                            G_ACTION(new_child->action));

    if (haveaccel) {
      accel.key = gdk_keyval_to_lower(accel.key);
      new_child->accel = gtk_accelerator_name(accel.key, accel.mods);
      if (ifactory->accel_group) {
        GtkShortcut *shortcut = gtk_shortcut_new(
                gtk_keyval_trigger_new(accel.key, accel.mods),
                gtk_callback_action_new(MenuAccelActivate,
                                        g_object_ref(new_child->action),
                                        g_object_unref));
        gtk_shortcut_controller_add_shortcut(ifactory->accel_group,
                                             shortcut);
      }
    }
    item = new_menu_item(new_child, menu_title->str);
  }

  new_child->position =
      g_menu_model_get_n_items(G_MENU_MODEL(new_child->parent_menu));
  g_menu_append_item(new_child->parent_menu, item);
  g_object_unref(item);

  g_string_free(menu_title, TRUE);

  ifactory->children = g_slist_append(ifactory->children, new_child);
}

#else
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
  }
  new_child->widget = menu_item;
  if (entry->callback) {
    g_signal_connect(G_OBJECT(menu_item), "activate",
                     entry->callback, callback_data);
  }

  if (parent) {
    menu = GTK_WIDGET(gtk_menu_item_get_submenu(GTK_MENU_ITEM(parent->widget)));
    if (!menu) {
      menu = gtk_menu_new();
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent->widget), menu);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  } else {
    gtk_menu_shell_append(GTK_MENU_SHELL(ifactory->top_widget), menu_item);
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
#endif /* GTK4 */

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

/* Get the item with the given path (which includes the factory's root
   path), or NULL if there is no such item */
static DPGtkItemFactoryChild *FindChild(DPGtkItemFactory *ifactory,
                                        const gchar *path)
{
  gint root_len;
  GSList *list;
  DPGtkItemFactoryChild *child;

  root_len = strlen(ifactory->path);
  if (!path || strlen(path) <= root_len)
    return NULL;

  if (strncmp(ifactory->path, path, root_len) != 0)
    return NULL;

  for (list = ifactory->children; list; list = g_slist_next(list)) {
    child = (DPGtkItemFactoryChild *)list->data;
    if (PathCmp(child->path, &path[root_len]) == 1)
      return child;
  }
  return NULL;
}

GtkWidget *dp_gtk_item_factory_get_widget(DPGtkItemFactory *ifactory,
                                          const gchar *path)
{
  DPGtkItemFactoryChild *child;

  if (path && strcmp(path, ifactory->path) == 0)
    return ifactory->top_widget;

  child = FindChild(ifactory, path);
  return child ? child->widget : NULL;
}

void dp_gtk_item_factory_set_translate_func(DPGtkItemFactory *ifactory,
                                            DPGtkTranslateFunc func,
                                            gpointer data,
                                            GDestroyNotify notify)
{
  ifactory->translate_func = func;
  ifactory->translate_data = data;
}

#endif   /* defined(CYGWIN) || GTK_MAJOR_VERSION > 2 */

#if !defined(CYGWIN) && GTK_MAJOR_VERSION >= 4
/* Enable each action only if it and all of its parent branches are
   sensitive, since GTK4 submenus cannot be made insensitive themselves */
static void UpdateEnabled(DPGtkItemFactory *ifactory)
{
  GSList *list;

  for (list = ifactory->children; list; list = g_slist_next(list)) {
    DPGtkItemFactoryChild *child = (DPGtkItemFactoryChild *)list->data;
    DPGtkItemFactoryChild *pt;
    gboolean enabled = TRUE;

    if (!child->action)
      continue;
    for (pt = child; pt; pt = pt->parent) {
      enabled = enabled && pt->sensitive;
    }
    g_simple_action_set_enabled(child->action, enabled);
  }
}

void dp_gtk_item_factory_set_sensitive(DPGtkItemFactory *ifactory,
                                       const gchar *path,
                                       gboolean sensitive)
{
  DPGtkItemFactoryChild *child = FindChild(ifactory, path);

  if (child) {
    child->sensitive = sensitive;
    UpdateEnabled(ifactory);
  }
}

gboolean dp_gtk_item_factory_get_active(DPGtkItemFactory *ifactory,
                                        const gchar *path)
{
  DPGtkItemFactoryChild *child = FindChild(ifactory, path);
  GVariant *state;
  gboolean active = FALSE;

  if (child && child->action) {
    state = g_action_get_state(G_ACTION(child->action));
    if (state) {
      active = g_variant_get_boolean(state);
      g_variant_unref(state);
    }
  }
  return active;
}

void dp_gtk_item_factory_set_active(DPGtkItemFactory *ifactory,
                                    const gchar *path, gboolean active)
{
  DPGtkItemFactoryChild *child = FindChild(ifactory, path);

  if (child && child->action) {
    g_simple_action_set_state(child->action, g_variant_new_boolean(active));
  }
}

void dp_gtk_item_factory_set_label(DPGtkItemFactory *ifactory,
                                   const gchar *path, gchar *label)
{
  DPGtkItemFactoryChild *child = FindChild(ifactory, path);
  GMenuItem *item;

  /* GMenu items cannot be changed, so replace the item with a new one */
  if (child && child->action) {
    item = new_menu_item(child, label);
    g_menu_remove(child->parent_menu, child->position);
    g_menu_insert_item(child->parent_menu, child->position, item);
    g_object_unref(item);
  }
}

#else
void dp_gtk_item_factory_set_sensitive(DPGtkItemFactory *ifactory,
                                       const gchar *path,
                                       gboolean sensitive)
{
  GtkWidget *widget = dp_gtk_item_factory_get_widget(ifactory, path);

  if (widget) {
    gtk_widget_set_sensitive(widget, sensitive);
  }
}

gboolean dp_gtk_item_factory_get_active(DPGtkItemFactory *ifactory,
                                        const gchar *path)
{
  GtkWidget *widget = dp_gtk_item_factory_get_widget(ifactory, path);

  return widget
         && gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
}

void dp_gtk_item_factory_set_active(DPGtkItemFactory *ifactory,
                                    const gchar *path, gboolean active)
{
  GtkWidget *widget = dp_gtk_item_factory_get_widget(ifactory, path);

  if (widget) {
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), active);
  }
}

void dp_gtk_item_factory_set_label(DPGtkItemFactory *ifactory,
                                   const gchar *path, gchar *label)
{
  GtkWidget *widget = dp_gtk_item_factory_get_widget(ifactory, path);

  if (widget) {
    SetAccelerator(widget, label, NULL, NULL, NULL, FALSE);
  }
}
#endif /* GTK4 */
