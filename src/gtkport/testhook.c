/************************************************************************
 * testhook.c     Let tests drive the GTK+ client                       *
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

/*
 * Built only with --enable-test-hooks. The client connects to a test
 * listening on the port named by DOPEWARS_TEST_CTL, which sends commands,
 * one per line, with tab-separated arguments:
 *
 *   dump                     Reply with every window on screen, and the
 *                            widgets in it, as a JSON list
 *   click SEL                Click a button
 *   set-text SEL TEXT        Set the text of an entry or spin button
 *   select SEL ROW           Select the row containing the text ROW in a
 *                            list or combo box, or the tab ROW of a
 *                            notebook
 *   menu PATH                Pick a menu item, e.g. "<main>/Game/Quit..."
 *   close TITLE              Close the window with the given title, as
 *                            its close button would
 *
 * SEL is a widget's name (see gtk_widget_set_name()) or, for buttons,
 * its label without mnemonics, optionally preceded by "TITLE|" to look only
 * in the window with that title. It must match exactly one widget on
 * screen. Each command gets the reply "ok" or "error <message>" (or, for
 * "dump", the JSON). Clicks, menu items and closes act just after the reply,
 * since they may open a modal dialog that doesn't return until closed;
 * commands are still read while it is open.
 *
 * This works the same way with GTK+2, GTK+3, GTK4 and our Win32 emulation
 * of GTK+, so one set of tests covers all of them. On Windows, native
 * message boxes (see GtkMessageBox()) are included as well.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef TEST_HOOKS

#include <string.h>
#include <stdlib.h>
#ifdef CYGWIN
#include <winsock2.h>
#include <windows.h>
#include <wchar.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif
#include <glib.h>

#include "gtkport.h"
#include "testhook.h"

#ifdef CYGWIN
typedef SOCKET HookSocket;
#define BAD_SOCKET INVALID_SOCKET
#define WOULD_BLOCK (WSAGetLastError() == WSAEWOULDBLOCK)
#define close_socket closesocket
#else
typedef int HookSocket;
#define BAD_SOCKET (-1)
#define WOULD_BLOCK (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
#define close_socket close
#define dp_g_timeout_add g_timeout_add
#define dp_g_source_remove g_source_remove
#endif

/* How often to look for commands, in ms. (Win32 ignores socket events in
   modal message boxes, so poll rather than watch the socket.) */
#define POLL_INTERVAL 20

static HookSocket ctl_socket = BAD_SOCKET;
static GString *ctl_input = NULL;
static DPGtkItemFactory *ctl_menu = NULL;

/* What a command can act on */
typedef enum {
  KIND_OTHER, KIND_BUTTON, KIND_ENTRY, KIND_LIST, KIND_COMBO, KIND_NOTEBOOK
} WidgetKind;

/* A widget or (on Windows) a native control in a message box */
typedef struct _Target {
  GtkWidget *widget;
#ifdef CYGWIN
  HWND hWnd;
#endif
} Target;

/* A click, menu item or close, to act on once the command has been
   replied to */
typedef struct _Action {
  Target target;
  gchar *menu_path;
  gboolean close;
} Action;

static GQueue actions = G_QUEUE_INIT;

typedef void (*ChildFunc) (GtkWidget *child, gpointer data);

/* Copy of `text` without mnemonic markers ("_" in GTK+, "&" in Win32) */
static gchar *strip_mnemonics(const gchar *text)
{
  GString *str = g_string_new(NULL);
  const gchar *pt;

  for (pt = text; pt && *pt; pt++) {
    if ((*pt == '_' || *pt == '&') && pt[1]) {
      pt++;
    }
    g_string_append_c(str, *pt);
  }
  return g_string_free(str, FALSE);
}

static void append_json_string(GString *out, const gchar *text)
{
  const guchar *pt;

  g_string_append_c(out, '"');
  for (pt = (const guchar *)text; pt && *pt; pt++) {
    switch (*pt) {
    case '"':
      g_string_append(out, "\\\"");
      break;
    case '\\':
      g_string_append(out, "\\\\");
      break;
    case '\n':
      g_string_append(out, "\\n");
      break;
    case '\r':
      g_string_append(out, "\\r");
      break;
    case '\t':
      g_string_append(out, "\\t");
      break;
    default:
      if (*pt < 0x20) {
        g_string_append_printf(out, "\\u%04x", *pt);
      } else {
        g_string_append_c(out, *pt);
      }
    }
  }
  g_string_append_c(out, '"');
}

/* Widget introspection, which differs between GTK+ versions and Win32 */

#ifdef CYGWIN
static gboolean is_a(GtkWidget *widget, const gchar *class_name)
{
  GtkClass *klass;

  for (klass = G_OBJECT(widget)->klass; klass; klass = klass->parent) {
    if (strcmp(klass->Name, class_name) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}
#endif

static const gchar *widget_type(GtkWidget *widget)
{
#ifdef CYGWIN
  return G_OBJECT(widget)->klass->Name;
#else
  return G_OBJECT_TYPE_NAME(widget);
#endif
}

/* The name given with gtk_widget_set_name(), or NULL */
static const gchar *widget_name(GtkWidget *widget)
{
  const gchar *name = gtk_widget_get_name(widget);

#ifndef CYGWIN
  /* GTK+ uses the type name for widgets without a name */
  if (name && strcmp(name, G_OBJECT_TYPE_NAME(widget)) == 0) {
    return NULL;
  }
#endif
  return name && name[0] ? name : NULL;
}

static WidgetKind widget_kind(GtkWidget *widget)
{
#ifdef CYGWIN
  if (is_a(widget, "button")) {
    return KIND_BUTTON;
  } else if (is_a(widget, "entry")) {
    return KIND_ENTRY;
  } else if (is_a(widget, "tree_view")) {
    return KIND_LIST;
  } else if (is_a(widget, "combobox")) {
    return KIND_COMBO;
  } else if (is_a(widget, "notebook")) {
    return KIND_NOTEBOOK;
  }
#else
  if (GTK_IS_BUTTON(widget)) {
    return KIND_BUTTON;
#if GTK_MAJOR_VERSION >= 4
  } else if (GTK_IS_CHECK_BUTTON(widget)) {
    return KIND_BUTTON;
  } else if (GTK_IS_ENTRY(widget) || GTK_IS_SPIN_BUTTON(widget)) {
    return KIND_ENTRY;
#else
  } else if (GTK_IS_ENTRY(widget)) {
    return KIND_ENTRY;
#endif
  } else if (GTK_IS_TREE_VIEW(widget)) {
    return KIND_LIST;
  } else if (GTK_IS_COMBO_BOX(widget)) {
    return KIND_COMBO;
  } else if (GTK_IS_NOTEBOOK(widget)) {
    return KIND_NOTEBOOK;
  }
#endif
  return KIND_OTHER;
}

/* TRUE if the widget is on screen (although maybe covered up) */
static gboolean widget_showing(GtkWidget *widget)
{
#ifdef CYGWIN
  if (widget->hWnd) {
    return IsWindowVisible(widget->hWnd);
  } else {
    return gtk_widget_get_visible(widget);
  }
#else
  return gtk_widget_get_mapped(widget);
#endif
}

static gboolean widget_sensitive(GtkWidget *widget)
{
#ifdef CYGWIN
  return GTK_WIDGET_SENSITIVE(widget);
#else
  return gtk_widget_is_sensitive(widget);
#endif
}

/* TRUE for widgets that are described as a whole, without children */
static gboolean is_leaf(GtkWidget *widget)
{
  WidgetKind kind = widget_kind(widget);

  return kind != KIND_OTHER && kind != KIND_NOTEBOOK;
}

/* Call `func` for each child of `widget`, including GTK+'s internal ones
   (but not those of buttons, entries, lists and combo boxes) */
static void foreach_child(GtkWidget *widget, ChildFunc func, gpointer data)
{
#ifdef CYGWIN
  GList *glist;
  GSList *slist;
  gint i;

  if (is_leaf(widget)) {
    return;
  } else if (is_a(widget, "box")) {
    for (glist = GTK_BOX(widget)->children; glist; glist = g_list_next(glist)) {
      func(((GtkBoxChild *)glist->data)->widget, data);
    }
  } else if (is_a(widget, "table")) {
    for (glist = GTK_TABLE(widget)->children; glist;
         glist = g_list_next(glist)) {
      func(((GtkTableChild *)glist->data)->widget, data);
    }
  } else if (is_a(widget, "notebook")) {
    for (slist = GTK_NOTEBOOK(widget)->children; slist;
         slist = g_slist_next(slist)) {
      func(((GtkNotebookChild *)slist->data)->child, data);
    }
  } else if (is_a(widget, "paned")) {
    for (i = 0; i < 2; i++) {
      if (GTK_PANED(widget)->children[i].widget) {
        func(GTK_PANED(widget)->children[i].widget, data);
      }
    }
  } else if (is_a(widget, "container") && GTK_CONTAINER(widget)->child) {
    func(GTK_CONTAINER(widget)->child, data);
  }
#elif GTK_MAJOR_VERSION >= 4
  GtkWidget *child;

  if (is_leaf(widget) || GTK_IS_TEXT_VIEW(widget)) {
    return;
  }
  for (child = gtk_widget_get_first_child(widget); child;
       child = gtk_widget_get_next_sibling(child)) {
    func(child, data);
  }
#else
  if (!is_leaf(widget) && GTK_IS_CONTAINER(widget)
      && !GTK_IS_TEXT_VIEW(widget)) {
    gtk_container_forall(GTK_CONTAINER(widget), (GtkCallback)func, data);
  }
#endif
}

#ifndef CYGWIN
static void find_label(GtkWidget *widget, gpointer data)
{
  GtkWidget **label = data;

  if (!*label && GTK_IS_LABEL(widget)) {
    *label = widget;
  } else if (!*label) {
#if GTK_MAJOR_VERSION >= 4
    GtkWidget *child;
    for (child = gtk_widget_get_first_child(widget); child;
         child = gtk_widget_get_next_sibling(child)) {
      find_label(child, data);
    }
#else
    if (GTK_IS_CONTAINER(widget)) {
      gtk_container_forall(GTK_CONTAINER(widget), find_label, data);
    }
#endif
  }
}

/* The text shown on a button: that of the first label inside it */
static gchar *button_text(GtkWidget *button)
{
  GtkWidget *label = NULL;

#if GTK_MAJOR_VERSION >= 4
  if (GTK_IS_CHECK_BUTTON(button)) {
    return strip_mnemonics(gtk_check_button_get_label(GTK_CHECK_BUTTON(button)));
  }
  find_label(button, &label);
#else
  gtk_container_forall(GTK_CONTAINER(button), find_label, &label);
#endif
  if (label) {
    return g_strdup(gtk_label_get_text(GTK_LABEL(label)));
  } else {
    return strip_mnemonics(gtk_button_get_label(GTK_BUTTON(button)));
  }
}
#endif

static gchar *row_text(GtkTreeModel *model, GtkTreeIter *iter, gint column);

/* The text the widget shows (a window's title, a label's text, and so
   on), or NULL; free with g_free() */
static gchar *widget_text(GtkWidget *widget)
{
  GtkTreeIter iter;
  WidgetKind kind = widget_kind(widget);

#ifdef CYGWIN
  if (kind == KIND_BUTTON) {
    return strip_mnemonics(GTK_BUTTON(widget)->text);
  } else if (is_a(widget, "editable")) {
    return gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  } else if (is_a(widget, "window")) {
    return g_strdup(GTK_WINDOW(widget)->title);
  } else if (is_a(widget, "frame")) {
    return strip_mnemonics(GTK_FRAME(widget)->text);
  } else if (is_a(widget, "label")) {
    return g_strdup(gtk_label_get_text(GTK_LABEL(widget)));
  }
#else
  if (kind == KIND_BUTTON) {
    return button_text(widget);
  } else if (kind == KIND_ENTRY) {
#if GTK_MAJOR_VERSION >= 4
    return g_strdup(gtk_editable_get_text(GTK_EDITABLE(widget)));
#else
    return g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
#endif
  } else if (GTK_IS_WINDOW(widget)) {
    return g_strdup(gtk_window_get_title(GTK_WINDOW(widget)));
  } else if (GTK_IS_FRAME(widget)) {
    return g_strdup(gtk_frame_get_label(GTK_FRAME(widget)));
  } else if (GTK_IS_LABEL(widget)) {
    return g_strdup(gtk_label_get_text(GTK_LABEL(widget)));
  } else if (GTK_IS_TEXT_VIEW(widget)) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
  }
#endif
  if (kind == KIND_COMBO
      && gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
    return row_text(gtk_combo_box_get_model(GTK_COMBO_BOX(widget)), &iter,
                    0);
  }
  return NULL;
}

/* Rows of lists and combo boxes */

static GtkTreeModel *widget_model(GtkWidget *widget)
{
  if (widget_kind(widget) == KIND_LIST) {
    return gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
  } else {
    return gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
  }
}

static gint model_columns(GtkTreeModel *model)
{
#ifdef CYGWIN
  return model->ncols;
#else
  return gtk_tree_model_get_n_columns(model);
#endif
}

static gboolean is_string_column(GtkTreeModel *model, gint column)
{
#ifdef CYGWIN
  return model->coltype[column] == G_TYPE_STRING;
#else
  return gtk_tree_model_get_column_type(model, column) == G_TYPE_STRING;
#endif
}

/* Text in the given column of a row, or NULL if it isn't a string */
static gchar *row_text(GtkTreeModel *model, GtkTreeIter *iter, gint column)
{
  gchar *text = NULL;

  if (is_string_column(model, column)) {
    gtk_tree_model_get(model, iter, column, &text, -1);
  }
  return text;
}

static gboolean row_selected(GtkWidget *widget, gint row)
{
#ifdef CYGWIN
  if (widget_kind(widget) == KIND_LIST) {
    return g_list_find(GTK_TREE_VIEW(widget)->selection,
                       GUINT_TO_POINTER(row)) != NULL;
  } else {
    return GTK_COMBO_BOX(widget)->active == row;
  }
#else
  if (widget_kind(widget) == KIND_LIST) {
    GtkTreePath *path = gtk_tree_path_new_from_indices(row, -1);
    gboolean selected = gtk_tree_selection_path_is_selected(
           gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), path);
    gtk_tree_path_free(path);
    return selected;
  } else {
    return gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == row;
  }
#endif
}

/* The index of the first row with a string column of `text`, or -1 */
static gint find_row(GtkWidget *widget, const gchar *text)
{
  GtkTreeModel *model = widget_model(widget);
  GtkTreeIter iter;
  gint row, col, nrows, ncols;
  gboolean found = FALSE;

  nrows = gtk_tree_model_iter_n_children(model, NULL);
  ncols = model_columns(model);
  for (row = 0; row < nrows && !found; row++) {
    gtk_tree_model_iter_nth_child(model, &iter, NULL, row);
    for (col = 0; col < ncols && !found; col++) {
      gchar *cell = row_text(model, &iter, col);
      found = cell && strcmp(cell, text) == 0;
      g_free(cell);
    }
  }
  return found ? row - 1 : -1;
}

/* Notebook tabs */

static gint notebook_pages(GtkWidget *widget)
{
#ifdef CYGWIN
  return g_slist_length(GTK_NOTEBOOK(widget)->children);
#else
  return gtk_notebook_get_n_pages(GTK_NOTEBOOK(widget));
#endif
}

static const gchar *notebook_tab(GtkWidget *widget, gint page)
{
#ifdef CYGWIN
  GtkNotebookChild *child = g_slist_nth_data(GTK_NOTEBOOK(widget)->children,
                                             page);
  return child && child->tab_label
         ? gtk_label_get_text(GTK_LABEL(child->tab_label)) : NULL;
#else
  return gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(widget),
                     gtk_notebook_get_nth_page(GTK_NOTEBOOK(widget), page));
#endif
}

static gint find_tab(GtkWidget *widget, const gchar *text)
{
  gint page;

  for (page = 0; page < notebook_pages(widget); page++) {
    const gchar *tab = notebook_tab(widget, page);
    if (tab && strcmp(tab, text) == 0) {
      return page;
    }
  }
  return -1;
}

static void dump_tabs(GtkWidget *widget, GString *out)
{
  gint page;

  g_string_append(out, ",\"tabs\":[");
  for (page = 0; page < notebook_pages(widget); page++) {
    if (page > 0) {
      g_string_append_c(out, ',');
    }
    append_json_string(out, notebook_tab(widget, page));
  }
  g_string_append_printf(out, "],\"selected\":[%d]",
                         gtk_notebook_get_current_page(GTK_NOTEBOOK(widget)));
}

/* Describe a widget, and those inside it, as JSON */

typedef struct _DumpData {
  GString *out;
  gboolean first;
} DumpData;

static void dump_widget(GtkWidget *widget, gpointer data);

static void dump_rows(GtkWidget *widget, GString *out)
{
  GtkTreeModel *model = widget_model(widget);
  GtkTreeIter iter;
  gint row, col, nrows, ncols;
  gboolean first = TRUE;

  if (!model) {
    return;
  }
  nrows = gtk_tree_model_iter_n_children(model, NULL);
  ncols = model_columns(model);
  g_string_append(out, ",\"rows\":[");
  for (row = 0; row < nrows; row++) {
    gboolean first_col = TRUE;
    gtk_tree_model_iter_nth_child(model, &iter, NULL, row);
    g_string_append(out, row > 0 ? ",[" : "[");
    for (col = 0; col < ncols; col++) {
      gchar *cell = row_text(model, &iter, col);
      if (cell) {
        if (!first_col) {
          g_string_append_c(out, ',');
        }
        append_json_string(out, cell);
        first_col = FALSE;
        g_free(cell);
      }
    }
    g_string_append_c(out, ']');
  }
  g_string_append(out, "],\"selected\":[");
  for (row = 0; row < nrows; row++) {
    if (row_selected(widget, row)) {
      g_string_append_printf(out, first ? "%d" : ",%d", row);
      first = FALSE;
    }
  }
  g_string_append_c(out, ']');
}

static gboolean toggle_active(GtkWidget *widget, gboolean *active)
{
#ifdef CYGWIN
  if (is_a(widget, "toggle")) {
    *active = GTK_TOGGLE_BUTTON(widget)->toggled;
    return TRUE;
  }
#else
  if (GTK_IS_TOGGLE_BUTTON(widget)) {
    *active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    return TRUE;
  }
#if GTK_MAJOR_VERSION >= 4
  if (GTK_IS_CHECK_BUTTON(widget)) {
    *active = gtk_check_button_get_active(GTK_CHECK_BUTTON(widget));
    return TRUE;
  }
#endif
#endif
  return FALSE;
}

static void dump_widget(GtkWidget *widget, gpointer data)
{
  DumpData *dump = data, children;
  GString *out = dump->out;
  const gchar *name;
  gchar *text;
  gboolean active;
  WidgetKind kind;

  if (!widget_showing(widget)) {
    return;
  }
  if (!dump->first) {
    g_string_append_c(out, ',');
  }
  dump->first = FALSE;

  g_string_append(out, "{\"type\":");
  append_json_string(out, widget_type(widget));
  if ((name = widget_name(widget))) {
    g_string_append(out, ",\"name\":");
    append_json_string(out, name);
  }
  if ((text = widget_text(widget))) {
    g_string_append(out, ",\"text\":");
    append_json_string(out, text);
    g_free(text);
  }
  if (!widget_sensitive(widget)) {
    g_string_append(out, ",\"insensitive\":true");
  }
  if (toggle_active(widget, &active)) {
    g_string_append_printf(out, ",\"active\":%s", active ? "true" : "false");
  }
  kind = widget_kind(widget);
  if (kind == KIND_LIST || kind == KIND_COMBO) {
    dump_rows(widget, out);
  } else if (kind == KIND_NOTEBOOK) {
    dump_tabs(widget, out);
  }

  children.out = out;
  children.first = TRUE;
  g_string_append(out, ",\"children\":[");
  foreach_child(widget, dump_widget, &children);
  g_string_append(out, "]}");
}

/* Top-level windows */

#ifdef CYGWIN
/* Native message boxes, which aren't GTK+ widgets */

static BOOL CALLBACK add_message_box(HWND hWnd, LPARAM lParam)
{
  GSList **boxes = (GSList **)lParam;
  WCHAR class_name[16];

  if (IsWindowVisible(hWnd)
      && GetClassNameW(hWnd, class_name, G_N_ELEMENTS(class_name))
      && wcscmp(class_name, L"#32770") == 0) {
    *boxes = g_slist_append(*boxes, hWnd);
  }
  return TRUE;
}

static GSList *message_boxes(void)
{
  GSList *boxes = NULL;

  EnumThreadWindows(GetCurrentThreadId(), add_message_box, (LPARAM)&boxes);
  return boxes;
}

static gchar *hwnd_text(HWND hWnd)
{
  WCHAR text[1024];
  gchar *utf8;

  text[0] = 0;
  GetWindowTextW(hWnd, text, G_N_ELEMENTS(text));
  utf8 = g_utf16_to_utf8((gunichar2 *)text, -1, NULL, NULL, NULL);
  return utf8 ? utf8 : g_strdup("");
}

static gboolean hwnd_is_button(HWND hWnd)
{
  WCHAR class_name[16];

  return GetClassNameW(hWnd, class_name, G_N_ELEMENTS(class_name))
         && _wcsicmp(class_name, L"Button") == 0;
}

static BOOL CALLBACK add_control(HWND hWnd, LPARAM lParam)
{
  GSList **controls = (GSList **)lParam;

  if (IsWindowVisible(hWnd)) {
    *controls = g_slist_append(*controls, hWnd);
  }
  return TRUE;
}

static GSList *message_box_controls(HWND box)
{
  GSList *controls = NULL;

  EnumChildWindows(box, add_control, (LPARAM)&controls);
  return controls;
}

static void dump_message_box(HWND box, DumpData *dump)
{
  GString *out = dump->out;
  GSList *controls, *list;
  gchar *text;

  if (!dump->first) {
    g_string_append_c(out, ',');
  }
  dump->first = FALSE;
  text = hwnd_text(box);
  g_string_append(out, "{\"type\":\"MessageBox\",\"text\":");
  append_json_string(out, text);
  g_free(text);
  g_string_append(out, ",\"children\":[");
  controls = message_box_controls(box);
  for (list = controls; list; list = g_slist_next(list)) {
    HWND hWnd = list->data;
    gchar *ctext = hwnd_text(hWnd), *stripped = strip_mnemonics(ctext);
    if (list != controls) {
      g_string_append_c(out, ',');
    }
    g_string_append_printf(out, "{\"type\":\"%s\",\"text\":",
                           hwnd_is_button(hWnd) ? "Button" : "Static");
    append_json_string(out, stripped);
    g_string_append(out, ",\"children\":[]}");
    g_free(ctext);
    g_free(stripped);
  }
  g_slist_free(controls);
  g_string_append(out, "]}");
}
#endif

/* All top-level windows on screen; free with g_list_free() */
static GList *toplevel_windows(void)
{
  GList *windows = NULL;
#ifdef CYGWIN
  GSList *list;

  for (list = dp_gtk_window_list(); list; list = g_slist_next(list)) {
    windows = g_list_append(windows, list->data);
  }
#elif GTK_MAJOR_VERSION >= 4
  GListModel *model = gtk_window_get_toplevels();
  guint i, n = g_list_model_get_n_items(model);

  for (i = 0; i < n; i++) {
    GtkWidget *window = g_list_model_get_item(model, i);
    windows = g_list_append(windows, window);
    g_object_unref(window);
  }
#else
  windows = gtk_window_list_toplevels();
#endif
  return windows;
}

static gchar *dump_all(void)
{
  DumpData dump;
  GList *windows, *list;

  dump.out = g_string_new("[");
  dump.first = TRUE;
  windows = toplevel_windows();
  for (list = windows; list; list = g_list_next(list)) {
    dump_widget(GTK_WIDGET(list->data), &dump);
  }
  g_list_free(windows);
#ifdef CYGWIN
  {
    GSList *boxes = message_boxes(), *slist;
    for (slist = boxes; slist; slist = g_slist_next(slist)) {
      dump_message_box(slist->data, &dump);
    }
    g_slist_free(boxes);
  }
#endif
  g_string_append_c(dump.out, ']');
  return g_string_free(dump.out, FALSE);
}

/* Find the one widget on screen matching a selector */

typedef struct _FindData {
  const gchar *selector;
  WidgetKind kind;
  GSList *found;   /* of Target */
} FindData;

static void find_widget(GtkWidget *widget, gpointer data)
{
  FindData *find = data;
  const gchar *name;

  if (!widget_showing(widget)) {
    return;
  }
  if (widget_kind(widget) == find->kind) {
    gboolean match = (name = widget_name(widget))
                     && strcmp(name, find->selector) == 0;
    if (!match && find->kind == KIND_BUTTON) {
      gchar *text = widget_text(widget);
      match = text && strcmp(text, find->selector) == 0;
      g_free(text);
    }
    if (match) {
      Target *target = g_new0(Target, 1);
      target->widget = widget;
      find->found = g_slist_append(find->found, target);
    }
  }
  foreach_child(widget, find_widget, find);
}

/* TRUE if `title` is NULL (any window) or matches the title of `window` */
static gboolean title_matches(const gchar *title, gchar *window_title)
{
  gboolean match = !title || (window_title
                              && strcmp(title, window_title) == 0);
  g_free(window_title);
  return match;
}

/* Find the target, or return an error message */
static gchar *find_target(const gchar *full_selector, WidgetKind kind,
                          Target *target)
{
  FindData find;
  GList *windows, *list;
  gint count;
  gchar *err = NULL, *title = NULL;
  const gchar *selector = full_selector, *bar;

  if ((bar = strchr(full_selector, '|'))) {
    title = g_strndup(full_selector, bar - full_selector);
    selector = bar + 1;
  }
  find.selector = selector;
  find.kind = kind;
  find.found = NULL;
  windows = toplevel_windows();
  for (list = windows; list; list = g_list_next(list)) {
    if (title_matches(title, widget_text(GTK_WIDGET(list->data)))) {
      find_widget(GTK_WIDGET(list->data), &find);
    }
  }
  g_list_free(windows);
#ifdef CYGWIN
  if (kind == KIND_BUTTON) {
    GSList *boxes = message_boxes(), *slist, *controls, *clist;
    for (slist = boxes; slist; slist = g_slist_next(slist)) {
      if (!title_matches(title, hwnd_text(slist->data))) {
        continue;
      }
      controls = message_box_controls(slist->data);
      for (clist = controls; clist; clist = g_slist_next(clist)) {
        gchar *text = hwnd_text(clist->data);
        gchar *stripped = strip_mnemonics(text);
        if (hwnd_is_button(clist->data) && strcmp(stripped, selector) == 0) {
          Target *t = g_new0(Target, 1);
          t->hWnd = clist->data;
          find.found = g_slist_append(find.found, t);
        }
        g_free(text);
        g_free(stripped);
      }
      g_slist_free(controls);
    }
    g_slist_free(boxes);
  }
#endif

  count = g_slist_length(find.found);
  if (count == 0) {
    err = g_strdup_printf("error no widget \"%s\" on screen", full_selector);
  } else if (count > 1) {
    err = g_strdup_printf("error %d widgets match \"%s\"", count,
                          full_selector);
  } else {
    *target = *(Target *)find.found->data;
  }
  g_slist_free_full(find.found, g_free);
  g_free(title);
  return err;
}

/* Clicks and menu items, done after the reply */

static void click(Target *target)
{
#ifdef CYGWIN
  if (target->hWnd) {
    /* A button in a message box */
    PostMessageW(GetParent(target->hWnd), WM_COMMAND,
                 MAKEWPARAM(GetDlgCtrlID(target->hWnd), BN_CLICKED),
                 (LPARAM)target->hWnd);
  } else if (target->widget->hWnd) {
    /* As the button would tell its parent window when clicked */
    SendMessageW(GetParent(target->widget->hWnd), WM_COMMAND,
                 MAKEWPARAM(GetDlgCtrlID(target->widget->hWnd), BN_CLICKED),
                 (LPARAM)target->widget->hWnd);
  }
#elif GTK_MAJOR_VERSION >= 4
  if (GTK_IS_CHECK_BUTTON(target->widget)) {
    GtkCheckButton *check = GTK_CHECK_BUTTON(target->widget);
    gtk_check_button_set_active(check, !gtk_check_button_get_active(check));
  } else {
    g_signal_emit_by_name(target->widget, "clicked");
  }
#else
  gtk_button_clicked(GTK_BUTTON(target->widget));
#endif
}

/* Ask a window to close, as the window manager would */
static void close_window(GtkWidget *window)
{
#ifdef CYGWIN
  SendMessageW(window->hWnd, WM_CLOSE, 0, 0);
#elif GTK_MAJOR_VERSION == 2
  GdkEvent *event = gdk_event_new(GDK_DELETE);

  event->any.window = g_object_ref(gtk_widget_get_window(window));
  event->any.send_event = TRUE;
  gtk_main_do_event(event);
  gdk_event_free(event);
#else
  gtk_window_close(GTK_WINDOW(window));
#endif
}

static void do_action(Action *action)
{
  if (action->close) {
    close_window(action->target.widget);
  } else if (action->menu_path) {
    dp_gtk_item_factory_activate(ctl_menu, action->menu_path);
    g_free(action->menu_path);
  } else {
    click(&action->target);
  }
#ifndef CYGWIN
  if (action->target.widget) {
    g_object_unref(action->target.widget);
  }
#endif
  g_free(action);
}

static void queue_action(Action *action)
{
#ifndef CYGWIN
  if (action->target.widget) {
    g_object_ref(action->target.widget);
  }
#endif
  g_queue_push_tail(&actions, action);
}

/* Commands */

static gchar *cmd_click(gchar **args)
{
  Action *action;
  Target target;
  gchar *err;

  if (!args[1]) {
    return g_strdup("error usage: click SEL");
  }
  if ((err = find_target(args[1], KIND_BUTTON, &target))) {
    return err;
  }
  if (target.widget && !widget_sensitive(target.widget)) {
    return g_strdup_printf("error \"%s\" is insensitive", args[1]);
  }
  action = g_new0(Action, 1);
  action->target = target;
  queue_action(action);
  return NULL;
}

static gchar *cmd_set_text(gchar **args)
{
  Target target;
  gchar *err;

  if (!args[1] || !args[2]) {
    return g_strdup("error usage: set-text SEL TEXT");
  }
  if ((err = find_target(args[1], KIND_ENTRY, &target))) {
    return err;
  }
  gtk_editable_set_text(GTK_EDITABLE(target.widget), args[2]);
#ifdef CYGWIN
  if (is_a(target.widget, "spinbutton")) {
#else
  if (GTK_IS_SPIN_BUTTON(target.widget)) {
#endif
    gtk_spin_button_update(GTK_SPIN_BUTTON(target.widget));
  }
  return NULL;
}

static gchar *cmd_select(gchar **args)
{
  Target target;
  gchar *err;
  gint row;

  if (!args[1] || !args[2]) {
    return g_strdup("error usage: select SEL ROW");
  }
  if ((err = find_target(args[1], KIND_LIST, &target))
      && (g_free(err), err = find_target(args[1], KIND_COMBO, &target))
      && (g_free(err), err = find_target(args[1], KIND_NOTEBOOK, &target))) {
    return err;
  }
  if (widget_kind(target.widget) == KIND_NOTEBOOK) {
    row = find_tab(target.widget, args[2]);
    if (row < 0) {
      return g_strdup_printf("error no tab \"%s\" in \"%s\"", args[2],
                             args[1]);
    }
    gtk_notebook_set_current_page(GTK_NOTEBOOK(target.widget), row);
    return NULL;
  }
  row = find_row(target.widget, args[2]);
  if (row < 0) {
    return g_strdup_printf("error no row \"%s\" in \"%s\"", args[2], args[1]);
  }
  if (widget_kind(target.widget) == KIND_LIST) {
    GtkTreeSelection *sel =
           gtk_tree_view_get_selection(GTK_TREE_VIEW(target.widget));
#ifdef CYGWIN
    GtkTreePath path = row;
    gtk_tree_selection_select_path(sel, &path);
#else
    GtkTreePath *path = gtk_tree_path_new_from_indices(row, -1);
    gtk_tree_selection_select_path(sel, path);
    gtk_tree_path_free(path);
#endif
  } else {
#ifdef CYGWIN
    /* As the combo box would tell its parent window when changed */
    HWND hWnd = target.widget->hWnd;
    SendMessageW(hWnd, CB_SETCURSEL, (WPARAM)row, 0);
    SendMessageW(GetParent(hWnd), WM_COMMAND,
                 MAKEWPARAM(GetDlgCtrlID(hWnd), CBN_SELENDOK), (LPARAM)hWnd);
#else
    gtk_combo_box_set_active(GTK_COMBO_BOX(target.widget), row);
#endif
  }
  return NULL;
}

static gchar *cmd_menu(gchar **args)
{
  Action *action;

  if (!args[1]) {
    return g_strdup("error usage: menu PATH");
  }
  if (!ctl_menu || !dp_gtk_item_factory_get_enabled(ctl_menu, args[1])) {
    return g_strdup_printf("error no menu item \"%s\", or it is insensitive",
                           args[1]);
  }
  action = g_new0(Action, 1);
  action->menu_path = g_strdup(args[1]);
  queue_action(action);
  return NULL;
}

static gchar *cmd_close(gchar **args)
{
  Action *action;
  GList *windows, *list;
  GtkWidget *found = NULL;

  if (!args[1]) {
    return g_strdup("error usage: close TITLE");
  }
  windows = toplevel_windows();
  for (list = windows; list; list = g_list_next(list)) {
    GtkWidget *window = GTK_WIDGET(list->data);
    gchar *title = widget_text(window);
    if (widget_showing(window) && title && strcmp(title, args[1]) == 0) {
      found = window;
    }
    g_free(title);
  }
  g_list_free(windows);
  if (!found) {
    return g_strdup_printf("error no window \"%s\" on screen", args[1]);
  }
  action = g_new0(Action, 1);
  action->target.widget = found;
  action->close = TRUE;
  queue_action(action);
  return NULL;
}

/* Run one command, and return the reply */
static gchar *run_command(const gchar *line)
{
  gchar **args = g_strsplit(line, "\t", 3);
  gchar *reply = NULL;

  if (!args[0]) {
    reply = g_strdup("error empty command");
  } else if (strcmp(args[0], "dump") == 0) {
    reply = dump_all();
  } else if (strcmp(args[0], "click") == 0) {
    reply = cmd_click(args);
  } else if (strcmp(args[0], "set-text") == 0) {
    reply = cmd_set_text(args);
  } else if (strcmp(args[0], "select") == 0) {
    reply = cmd_select(args);
  } else if (strcmp(args[0], "menu") == 0) {
    reply = cmd_menu(args);
  } else if (strcmp(args[0], "close") == 0) {
    reply = cmd_close(args);
  } else {
    reply = g_strdup_printf("error unknown command \"%s\"", args[0]);
  }
  g_strfreev(args);
  return reply ? reply : g_strdup("ok");
}

/* The connection to the test */

static void ctl_close(void)
{
  close_socket(ctl_socket);
  ctl_socket = BAD_SOCKET;
}

static void ctl_send(const gchar *data, gsize len)
{
  while (len > 0 && ctl_socket != BAD_SOCKET) {
    int sent = send(ctl_socket, data, len, 0);
    if (sent > 0) {
      data += sent;
      len -= sent;
    } else if (sent < 0 && WOULD_BLOCK) {
      fd_set writefds;
      FD_ZERO(&writefds);
      FD_SET(ctl_socket, &writefds);
      select(ctl_socket + 1, NULL, &writefds, NULL, NULL);
    } else {
      ctl_close();
    }
  }
}

/* Read and run commands. This is called again from any nested main loop
   that an action runs (e.g. for a modal dialog), so that the test can
   carry on. */
static gboolean ctl_poll(gpointer data)
{
  char buf[4096];
  int len;
  gchar *newline;
  Action *action;

  if (ctl_socket == BAD_SOCKET) {
    return TRUE;
  }
  while ((len = recv(ctl_socket, buf, sizeof(buf), 0)) > 0) {
    g_string_append_len(ctl_input, buf, len);
  }
  if (len == 0 || !WOULD_BLOCK) {
    /* The test has finished with us */
    ctl_close();
  }

  while ((newline = memchr(ctl_input->str, '\n', ctl_input->len))) {
    gchar *line = g_strndup(ctl_input->str, newline - ctl_input->str);
    gchar *reply;

    g_string_erase(ctl_input, 0, newline - ctl_input->str + 1);
    g_strchomp(line);
    reply = run_command(line);
    ctl_send(reply, strlen(reply));
    ctl_send("\n", 1);
    g_free(reply);
    g_free(line);
  }

  while ((action = g_queue_pop_head(&actions))) {
    do_action(action);
  }
  /* Never remove this source, since that isn't safe while it is running
     in a nested main loop on Win32 */
  return TRUE;
}

void dp_test_hook_start(DPGtkItemFactory *menu)
{
  const gchar *port = g_getenv("DOPEWARS_TEST_CTL");
  struct sockaddr_in addr;
  guint tag;
#ifdef CYGWIN
  WSADATA wsaData;
  u_long nonblocking = 1;

  WSAStartup(MAKEWORD(2, 0), &wsaData);
#endif

  if (!port || !port[0]) {
    return;
  }
  ctl_menu = menu;
  ctl_input = g_string_new(NULL);
  ctl_socket = socket(AF_INET, SOCK_STREAM, 0);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(port));
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (ctl_socket == BAD_SOCKET
      || connect(ctl_socket, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    g_error("cannot connect to test on port %s", port);
  }
#ifdef CYGWIN
  ioctlsocket(ctl_socket, FIONBIO, &nonblocking);
#else
  fcntl(ctl_socket, F_SETFL, fcntl(ctl_socket, F_GETFL) | O_NONBLOCK);
#endif
  tag = dp_g_timeout_add(POLL_INTERVAL, ctl_poll, NULL);
#ifndef CYGWIN
  /* (Win32 timers already work this way) */
  g_source_set_can_recurse(g_main_context_find_source_by_id(NULL, tag), TRUE);
#endif
}

#endif /* TEST_HOOKS */
