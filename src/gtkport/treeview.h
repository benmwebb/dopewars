/************************************************************************
 * treeview.h     GtkTreeView (and friends) implementation for gtkport  *
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

#ifndef __TREEVIEW_H__
#define __TREEVIEW_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef CYGWIN

#include <glib.h>
#include "gtkenums.h"

typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreeViewColumn GtkTreeViewColumn;
typedef struct _GtkListStore GtkListStore;
/* ListStore is the only model we provide here, so they can be synonyms */
typedef struct _GtkListStore GtkTreeModel;
typedef struct _GtkCellRenderer GtkCellRenderer;

/* Our TreeModel is sortable, so this can be a synonym */
typedef struct _GtkListStore GtkTreeSortable;

/* We only support one selection per tree view, so make them synonyms */
typedef struct _GtkTreeView GtkTreeSelection;

/* A list row is just a list of pointers to column data */
typedef struct _GtkListStoreRow GtkListStoreRow;

/* Tree iterators and paths are each just a row index */
typedef guint GtkTreeIter;
typedef guint GtkTreePath;

typedef gint (*GtkTreeIterCompareFunc) (GtkTreeModel *model,
                                        GtkTreeIter *a, GtkTreeIter *b,
                                        gpointer user_data);

struct _GtkTreeView {
  GtkContainer container;
  HWND header, scrollwin;
  GtkTreeModel *model;
  int scrollpos;
  gint16 header_size;
  GtkSelectionMode mode;
  GSList *columns; /* List of GtkTreeViewColumn objects */
  GList *selection;
  gint headers_clickable:1;
};

struct _GtkTreeViewColumn {
  gchar *title;       /* header title */
  int model_column;   /* the index of the column in the GtkTreeModel */
  gint sort_column_id; /* what to sort by when this column is selected */
  gint width;
  gint optimal_width;
  gfloat xalign; /* 0.0 for left, 0.5 for center, 1.0 for right align */
  guint visible:1;
  guint resizeable:1;
  guint auto_resize:1;
  guint expand:1;         /* should the column take up available space? */
};

struct _GtkListStoreRow {
  gpointer *data; /* Data for each column */
};

struct _GtkListStore {
  GObject object;
  int ncols;         /* Number of columns */
  int *coltype;      /* Type of each column (e.g. G_TYPE_STRING) */
  GArray *rows;      /* All rows in the list as GtkListStoreRow */
  GtkTreeView *view; /* The currently connected view (only one supported) */
  gint sort_column_id; /* what to sort by, if >= 0 */
  GtkSortType sort_order;
  GArray *sort_func;   /* callback functions for sorting, by sort_column_id */
  gint need_sort:1;  /* set once data is added/changed */
};

/* Empty struct; we don't support customizing the renderer */
struct _GtkCellRenderer {
};

typedef void (*GtkTreeSelectionForeachFunc) (GtkTreeModel *model,
                GtkTreePath *path, GtkTreeIter *iter, gpointer data);

#define GTK_TREE_VIEW(obj) ((GtkTreeView *)(obj))
#define GTK_TREE_MODEL(obj) ((GtkTreeModel *)(obj))
#define GTK_TREE_SORTABLE(obj) ((GtkTreeSortable *)(obj))
#define GTK_LIST_STORE(obj) ((GtkListStore *)(obj))

#define GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID (-1)
#define GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID (-2)

GtkListStore *gtk_list_store_new(gint n_columns, ...);
void gtk_list_store_clear(GtkListStore *list_store);
void gtk_list_store_insert(GtkListStore *list_store, GtkTreeIter *iter,
                           gint position);
void gtk_list_store_append(GtkListStore *list_store, GtkTreeIter *iter);
gboolean gtk_list_store_remove(GtkListStore *list_store, GtkTreeIter *iter);
void gtk_list_store_set(GtkListStore *list_store, GtkTreeIter *iter, ...);
void gtk_list_store_swap(GtkListStore *store, GtkTreeIter *a, GtkTreeIter *b);

void gtk_tree_model_get(GtkTreeModel *tree_model, GtkTreeIter *iter, ...);
gboolean gtk_tree_model_iter_nth_child(GtkTreeModel *tree_model,
                                       GtkTreeIter *iter,
                                       GtkTreeIter *parent, gint n);
gint gtk_tree_model_iter_n_children(GtkTreeModel *tree_model,
                                    GtkTreeIter *iter);

GtkWidget *gtk_tree_view_new(void);
GtkTreeSelection *gtk_tree_view_get_selection(GtkTreeView *tree_view);
void gtk_tree_view_set_model(GtkTreeView *tree_view, GtkTreeModel *model);
GtkTreeModel *gtk_tree_view_get_model(GtkTreeView *tree_view);
void gtk_tree_view_set_headers_clickable(GtkTreeView *tree_view,
                                         gboolean setting);
gint gtk_tree_view_insert_column_with_attributes
            (GtkTreeView *tree_view, gint position, const gchar *title,
             GtkCellRenderer *cell, ...);
void gtk_tree_view_scroll_to_cell(GtkTreeView *tree_view,
                                  GtkTreePath *path,
                                  GtkTreeViewColumn *column,
                                  gboolean use_align, gfloat row_align,
                                  gfloat col_align);
void gtk_tree_selection_selected_foreach(GtkTreeSelection *selection,
                                         GtkTreeSelectionForeachFunc func,
                                         gpointer data);
void gtk_tree_selection_set_mode(GtkTreeSelection *selection,
                                 GtkSelectionMode type);
gboolean gtk_tree_selection_get_selected(GtkTreeSelection *selection,
                                 GtkTreeModel **model,
                                 GtkTreeIter *iter);
gint gtk_tree_selection_count_selected_rows(GtkTreeSelection *selection);
void gtk_tree_selection_select_path(GtkTreeSelection *selection,
                                    GtkTreePath *path);
void gtk_tree_selection_unselect_path(GtkTreeSelection *selection,
                                      GtkTreePath *path);
void gtk_tree_selection_unselect_all(GtkTreeSelection *selection);
#define gtk_tree_selection_select_iter(sel, iter) gtk_tree_selection_select_path(sel, iter)
#define gtk_tree_selection_unselect_iter(sel, iter) gtk_tree_selection_unselect_path(sel, iter)
GList *gtk_tree_selection_get_selected_rows(GtkTreeSelection *selection,
                                            GtkTreeModel **model);
#define gtk_tree_path_free g_free
gint *gtk_tree_path_get_indices_with_depth(GtkTreePath *path, gint *depth);
GtkTreeViewColumn *gtk_tree_view_column_new_with_attributes
                   (const gchar *title, GtkCellRenderer *cell, ...);
void gtk_tree_view_column_set_resizable(GtkTreeViewColumn *tree_column,
                                        gboolean resizable);
void gtk_tree_view_column_set_expand(GtkTreeViewColumn *tree_column,
                                     gboolean expand);
void gtk_tree_view_column_set_sort_column_id(GtkTreeViewColumn *tree_column,
                                             gint sort_column_id);
/* We treat this as the alignment of all cells in this column, not just the
   alignment of the header */
void gtk_tree_view_column_set_alignment(GtkTreeViewColumn *tree_column,
                                        gfloat xalign);
/* This is only used to set the xalign property, so make it a noop */
#define g_object_set(object, name, value, end) {}
gint gtk_tree_view_insert_column(GtkTreeView *tree_view,
                                 GtkTreeViewColumn *column,
                                 gint position);
GtkTreeViewColumn *gtk_tree_view_get_column(GtkTreeView *tree_view, gint n);

/* Force a sort of the view */
void gtk_tree_view_sort(GtkTreeView *tv);

void gtk_tree_sortable_set_sort_func(GtkTreeSortable *sortable,
                                     gint sort_column_id,
                                     GtkTreeIterCompareFunc sort_func,
                                     gpointer user_data,
                                     GDestroyNotify destroy);
void gtk_tree_sortable_set_sort_column_id(GtkTreeSortable *sortable,
                                          gint sort_column_id,
                                          GtkSortType order);
GtkCellRenderer *gtk_cell_renderer_text_new(void);

void g_object_unref(gpointer object);
gpointer g_object_ref(gpointer object);

/* Private functions */
void InitTreeViewClass(HINSTANCE hInstance);
void gtk_tree_model_free(GtkTreeModel *model);
#endif /* CYGWIN */

#endif
