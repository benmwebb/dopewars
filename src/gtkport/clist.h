/************************************************************************
 * clist.h        GtkCList implementation for gtkport                   *
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

#ifndef __CLIST_H__
#define __CLIST_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef CYGWIN

#include <glib.h>

typedef struct _GtkCList GtkCList;
typedef struct _GtkCListRow GtkCListRow;
typedef struct _GtkCListColumn GtkCListColumn;

typedef gint (*GtkCListCompareFunc) (GtkCList *clist, gconstpointer ptr1,
                                     gconstpointer ptr2);

struct _GtkCListColumn {
  gchar *title;
  gint width;
  guint visible:1;
  guint resizeable:1;
  guint auto_resize:1;
  guint button_passive:1;
};

struct _GtkCListRow {
  gpointer data;
  gchar **text;
};

struct _GtkCList {
  GtkContainer container;
  gint cols, rows;
  HWND header;
  gint16 header_size;
  GSList *rowdata;
  GtkCListColumn *coldata;
  GList *selection;
  GtkSelectionMode mode;
  GtkCListCompareFunc cmp_func;
  gint auto_sort:1;
};

#define GTK_CLIST(obj) ((GtkCList *)(obj))

GtkWidget *gtk_clist_new(gint columns);
GtkWidget *gtk_clist_new_with_titles(gint columns, gchar *titles[]);
gint gtk_clist_append(GtkCList *clist, gchar *text[]);
void gtk_clist_remove(GtkCList *clist, gint row);
void gtk_clist_set_column_title(GtkCList *clist, gint column,
                                const gchar *title);
gint gtk_clist_insert(GtkCList *clist, gint row, gchar *text[]);
gint gtk_clist_set_text(GtkCList *clist, gint row, gint col, gchar *text);
void gtk_clist_set_column_width(GtkCList *clist, gint column, gint width);
void gtk_clist_column_title_passive(GtkCList *clist, gint column);
void gtk_clist_column_titles_passive(GtkCList *clist);
void gtk_clist_column_title_active(GtkCList *clist, gint column);
void gtk_clist_column_titles_active(GtkCList *clist);
void gtk_clist_set_selection_mode(GtkCList *clist, GtkSelectionMode mode);
void gtk_clist_sort(GtkCList *clist);
void gtk_clist_freeze(GtkCList *clist);
void gtk_clist_thaw(GtkCList *clist);
void gtk_clist_clear(GtkCList *clist);
void gtk_clist_set_row_data(GtkCList *clist, gint row, gpointer data);
gpointer gtk_clist_get_row_data(GtkCList *clist, gint row);
void gtk_clist_set_auto_sort(GtkCList *clist, gboolean auto_sort);
void gtk_clist_columns_autosize(GtkCList *clist);
void gtk_clist_select_row(GtkCList *clist, gint row, gint column);
void gtk_clist_unselect_row(GtkCList *clist, gint row, gint column);
GtkVisibility gtk_clist_row_is_visible(GtkCList *clist, gint row);
void gtk_clist_moveto(GtkCList *clist, gint row, gint column,
                      gfloat row_align, gfloat col_align);
void gtk_clist_set_compare_func(GtkCList *clist,
                                GtkCListCompareFunc cmp_func);
void gtk_clist_set_column_auto_resize(GtkCList *clist, gint column,
                                      gboolean auto_resize);
#endif /* CYGWIN */

#endif
