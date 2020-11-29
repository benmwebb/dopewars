/************************************************************************
 * treeview.c     GtkTreeView (and friends) implementation for gtkport  *
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

#ifdef CYGWIN

#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>

#include "unicodewrap.h"

#define LISTITEMHPACK  3
#define LISTHEADERPACK 6

static const gchar *WC_GTKTREEVIEWHDR = "WC_GTKTREEVIEWHDR";

static WNDPROC wpOrigListProc;

static void gtk_tree_view_size_request(GtkWidget *widget,
                                       GtkRequisition *requisition);
static void gtk_tree_view_set_size(GtkWidget *widget,
                                    GtkAllocation *allocation);
static gboolean gtk_tree_view_wndproc(GtkWidget *widget, UINT msg,
                WPARAM wParam, LPARAM lParam, gboolean *dodef);
static void gtk_tree_view_realize(GtkWidget *widget);
static void gtk_tree_view_destroy(GtkWidget *widget);
static void gtk_tree_view_show(GtkWidget *widget);
static void gtk_tree_view_hide(GtkWidget *widget);
static void gtk_tree_view_draw_row(GtkTreeView *tv, LPDRAWITEMSTRUCT lpdis);
static void gtk_tree_view_update_selection(GtkWidget *widget);
static void gtk_tree_view_update_widths(GtkTreeView *tv, GtkTreeModel *model,
                                        GtkListStoreRow *row);
static void gtk_tree_view_update_all_widths(GtkTreeView *tv);
static void gtk_tree_view_do_auto_resize(GtkTreeView *tv);
static void gtk_tree_view_set_column_width(GtkTreeView *tv, gint column,
                                           gint width);
static void gtk_tree_view_set_column_width_full(GtkTreeView *tv, gint column,
                                                gint width,
                                                gboolean ResizeHeader);

static GtkSignalType GtkTreeViewSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_tree_view_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_tree_view_set_size},
  {"realize", gtk_marshal_VOID__VOID, gtk_tree_view_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_tree_view_destroy},
  {"click-column", gtk_marshal_VOID__GINT, NULL},
  {"changed", gtk_marshal_VOID__VOID, NULL},
  {"show", gtk_marshal_VOID__VOID, gtk_tree_view_show},
  {"hide", gtk_marshal_VOID__VOID, gtk_tree_view_hide},
  {"", NULL, NULL}
};

static GtkClass GtkTreeViewClass = {
  "tree_view", &GtkContainerClass, sizeof(GtkTreeView), GtkTreeViewSignals,
  gtk_tree_view_wndproc
};

static GtkClass GtkListStoreClass = {
  "list_store", &GtkObjectClass, sizeof(GtkListStore), NULL, NULL
};

static void SetTreeViewHeaderSize(GtkTreeView *clist)
{
  RECT rc;
  HWND hWnd;
  int width;

  hWnd = GTK_WIDGET(clist)->hWnd;
  clist->scrollpos = GetScrollPos(hWnd, SB_HORZ);

  GetWindowRect(hWnd, &rc);
  width = (int)mySendMessage(hWnd, LB_GETHORIZONTALEXTENT, 0, 0);
  width = MAX(width, rc.right - rc.left) + 100;

  SetWindowPos(clist->header, HWND_TOP, -clist->scrollpos, 0,
               width, clist->header_size, SWP_NOZORDER);
}

static LRESULT APIENTRY ListWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam)
{
  LRESULT retval;
  GtkWidget *widget;

  widget = GTK_WIDGET(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  retval = myCallWindowProc(wpOrigListProc, hwnd, msg, wParam, lParam);

  if (msg == WM_HSCROLL && widget) {
    GtkTreeView *clist = GTK_TREE_VIEW(widget);
    SetTreeViewHeaderSize(clist);
  }

  return retval;
}

gboolean gtk_tree_view_wndproc(GtkWidget *widget, UINT msg, WPARAM wParam,
                               LPARAM lParam, gboolean *dodef)
{
  LPDRAWITEMSTRUCT lpdis;
  HD_NOTIFYA FAR *phdr;
  HD_NOTIFYW FAR *phdrw;
  NMHDR *nmhdr;

  switch(msg) {
  case WM_COMMAND:
    if (lParam && HIWORD(wParam) == LBN_SELCHANGE) {
      gtk_tree_view_update_selection(widget);
      return FALSE;
    }
    break;
  case WM_DRAWITEM:
    lpdis = (LPDRAWITEMSTRUCT)lParam;
    if (lpdis) {
      gtk_tree_view_draw_row(GTK_TREE_VIEW(widget), lpdis);
      *dodef = FALSE;
      return TRUE;
    }
    break;
  case WM_NOTIFY:
    nmhdr = (NMHDR *)lParam;
    if (nmhdr) {
      switch(nmhdr->code) {
      case HDN_ENDTRACKA:
        phdr = (HD_NOTIFYA FAR *)lParam;
        gtk_tree_view_set_column_width_full(GTK_TREE_VIEW(widget), phdr->iItem,
                                            phdr->pitem->cxy, FALSE);
        return FALSE;
      case HDN_ENDTRACKW:
        phdrw = (HD_NOTIFYW FAR *)lParam;
        gtk_tree_view_set_column_width_full(GTK_TREE_VIEW(widget), phdrw->iItem,
                                            phdrw->pitem->cxy, FALSE);
        return FALSE;
      case HDN_ITEMCLICKA:
        phdr = (HD_NOTIFYA FAR *)lParam;
        gtk_signal_emit(G_OBJECT(widget), "click-column", (gint)phdr->iItem);
        return FALSE;
      case HDN_ITEMCLICKW:
        phdrw = (HD_NOTIFYW FAR *)lParam;
        gtk_signal_emit(G_OBJECT(widget), "click-column", (gint)phdrw->iItem);
        return FALSE;
      default:
        break;
      }
    }
    break;
  }

  return FALSE;
}

static void gtk_tree_view_set_extent(GtkTreeView *tv)
{
  HWND hWnd;

  hWnd = GTK_WIDGET(tv)->hWnd;
  if (hWnd) {
    GSList *colpt;
    int width = 0;

    for (colpt = tv->columns; colpt; colpt = g_slist_next(colpt)) {
      GtkTreeViewColumn *col = colpt->data;
      width += col->width;
    }
    mySendMessage(hWnd, LB_SETHORIZONTALEXTENT, (WPARAM)width, 0);
    SetTreeViewHeaderSize(tv);
  }
}

void gtk_tree_view_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkTreeView *clist = GTK_TREE_VIEW(widget);

  gtk_container_set_size(widget, allocation);
  if (clist->header) {
    POINT pt;
    pt.x = allocation->x;
    pt.y = allocation->y;
    MapWidgetOrigin(widget, &pt);
    SetWindowPos(clist->scrollwin, HWND_TOP, pt.x, pt.y,
                 allocation->width, clist->header_size, SWP_NOZORDER);
    allocation->y += clist->header_size - 1;
    allocation->height -= clist->header_size - 1;
  }
  gtk_tree_view_set_extent(clist);
}

GtkWidget *gtk_tree_view_new(void)
{
  GtkTreeView *view;

  view = GTK_TREE_VIEW(GtkNewObject(&GtkTreeViewClass));
  view->model = NULL;
  view->scrollpos = 0;
  view->columns = NULL;
  view->headers_clickable = TRUE;
  view->mode = GTK_SELECTION_SINGLE;
  view->selection = NULL;
  return GTK_WIDGET(view);
}

GtkTreeSelection *gtk_tree_view_get_selection(GtkTreeView *tree_view)
{
  /* The selection *is* the tree view */
  return tree_view;
}

void gtk_tree_view_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  SIZE size;

  if (GetTextSize(widget->hWnd, "Sample text", &size, defFont)) {
    requisition->width = size.cx;
    requisition->height = size.cy * 6 + 12;
  }
}

void gtk_tree_view_realize(GtkWidget *widget)
{
  HWND Parent, header, scrollwin;
  HD_LAYOUT hdl;
  HD_ITEM hdi;
  RECT rcParent;
  WINDOWPOS wp;
  GtkTreeView *tv = GTK_TREE_VIEW(widget);
  GSList *colpt;
  gint i;

  gtk_container_realize(widget);
  Parent = gtk_get_parent_hwnd(widget);
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  rcParent.left = rcParent.top = 0;
  rcParent.right = rcParent.bottom = 800;
  scrollwin = myCreateWindow(WC_GTKTREEVIEWHDR, NULL, WS_CHILD | WS_BORDER,
                             0, 0, 0, 0, Parent, NULL, hInst, NULL);
  SetWindowLongPtr(scrollwin, GWLP_USERDATA, (LONG_PTR)widget);
  header = myCreateWindowEx(0, WC_HEADER, NULL,
                            WS_CHILD | HDS_HORZ | WS_VISIBLE
                            | (tv->headers_clickable ? HDS_BUTTONS : 0),
                            0, 0, 0, 0, scrollwin, NULL, hInst, NULL);
  SetWindowLongPtr(header, GWLP_USERDATA, (LONG_PTR)widget);
  tv->header = header;
  tv->scrollwin = scrollwin;
  gtk_set_default_font(header);
  hdl.prc = &rcParent;
  hdl.pwpos = &wp;
  mySendMessage(header, HDM_LAYOUT, 0, (LPARAM)&hdl);
  tv->header_size = wp.cy;
  widget->hWnd = myCreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "",
                                  WS_CHILD | WS_TABSTOP | WS_VSCROLL
                                  | WS_HSCROLL | LBS_OWNERDRAWFIXED
                                  | LBS_NOTIFY, 0, 0, 0, 0, Parent, NULL,
                                  hInst, NULL);
  /* Subclass the window */
  wpOrigListProc = (WNDPROC)mySetWindowLong(widget->hWnd, GWLP_WNDPROC,
                                            (LONG_PTR)ListWndProc);
  gtk_set_default_font(widget->hWnd);

  if (tv->model) {
    for (i = 0; i < tv->model->rows->len; ++i) {
      mySendMessage(widget->hWnd, LB_ADDSTRING, 0, 1);
    }
  }
  gtk_tree_view_update_all_widths(tv);

  for (colpt = tv->columns, i = 0; colpt; colpt = g_slist_next(colpt), ++i) {
    GtkTreeViewColumn *col = colpt->data;
    if (col->auto_resize) {
      col->width = col->optimal_width;
    }
    hdi.mask = HDI_TEXT | HDI_FORMAT | HDI_WIDTH;
    hdi.pszText = col->title;
    if (hdi.pszText) {
      if (!g_slist_next(colpt))
        hdi.cxy = 9000;
      else
        hdi.cxy = col->width;
      hdi.cchTextMax = strlen(hdi.pszText);
      hdi.fmt = HDF_LEFT | HDF_STRING;
      myHeader_InsertItem(header, i + 1, &hdi);
    }
  }
}

static void gtk_list_store_row_free(GtkListStoreRow *row, GtkListStore *store)
{
  int i;
  for (i = 0; i < store->ncols; ++i) {
    if (store->coltype[i] == G_TYPE_STRING) {
      g_free(row->data[i]);
    }
  }
}

void gtk_list_store_clear(GtkListStore *list_store)
{
  guint i;
  for (i = 0; i < list_store->rows->len; ++i) {
    GtkListStoreRow *row = &g_array_index(list_store->rows, GtkListStoreRow, i);
    gtk_list_store_row_free(row, list_store);
  }
  g_array_set_size(list_store->rows, 0);
  list_store->need_sort = FALSE;  /* an empty store is sorted */

  if (list_store->view) {
    HWND hWnd;
    gtk_tree_view_update_all_widths(list_store->view);
    hWnd = GTK_WIDGET(list_store->view)->hWnd;
    if (hWnd) {
      mySendMessage(hWnd, LB_RESETCONTENT, 0, 0);
    }
  }
}

void gtk_list_store_insert(GtkListStore *list_store, GtkTreeIter *iter,
                           gint position)
{
  GtkListStoreRow row;
  /* Add a new empty row to the store and return a pointer to it */
  row.data = g_new0(gpointer, list_store->ncols);
  if (position < 0) {
    g_array_append_val(list_store->rows, row);
    *iter = list_store->rows->len - 1;
  } else {
    g_array_insert_val(list_store->rows, position, row);
    *iter = position;
  }
}

void gtk_list_store_append(GtkListStore *list_store, GtkTreeIter *iter)
{
  gtk_list_store_insert(list_store, iter, -1);
}

void gtk_list_store_set(GtkListStore *list_store, GtkTreeIter *iter, ...)
{
  va_list ap;
  int colind;
  GtkListStoreRow *row = &g_array_index(list_store->rows, GtkListStoreRow,
                                        *iter);
  list_store->need_sort = TRUE;

  va_start(ap, iter);
  while ((colind = va_arg(ap, int)) >= 0) {
    switch(list_store->coltype[colind]) {
    case G_TYPE_STRING:
      g_free(row->data[colind]);  /* Free any existing string */
      row->data[colind] = g_strdup(va_arg(ap, const char*));
      break;
    case G_TYPE_UINT:
      row->data[colind] = GUINT_TO_POINTER(va_arg(ap, unsigned));
      break;
    case G_TYPE_INT:
      row->data[colind] = GINT_TO_POINTER(va_arg(ap, int));
      break;
    case G_TYPE_POINTER:
      row->data[colind] = va_arg(ap, gpointer);
      break;
    }
  }
  va_end(ap);

  if (list_store->view) {
    GtkWidget *widget = GTK_WIDGET(list_store->view);

    gtk_tree_view_update_widths(list_store->view, list_store, row);
    gtk_tree_view_do_auto_resize(list_store->view);

    if (GTK_WIDGET_REALIZED(widget)) {
      HWND hWnd = widget->hWnd;
      mySendMessage(hWnd, LB_INSERTSTRING, (WPARAM)*iter, 1);
    }
  }
}

void gtk_list_store_swap(GtkListStore *store, GtkTreeIter *a, GtkTreeIter *b)
{
  GtkListStoreRow rowa = g_array_index(store->rows, GtkListStoreRow, *a);
  GtkListStoreRow rowb = g_array_index(store->rows, GtkListStoreRow, *b);

  g_array_index(store->rows, GtkListStoreRow, *a) = rowb;
  g_array_index(store->rows, GtkListStoreRow, *b) = rowa;
  store->need_sort = TRUE;
}

void gtk_tree_model_get(GtkTreeModel *tree_model, GtkTreeIter *iter, ...)
{
  va_list ap;
  char **strpt;
  unsigned *uintpt;
  int *intpt;
  gpointer *ptpt;
  int colind;
  GtkListStoreRow *row = &g_array_index(tree_model->rows, GtkListStoreRow,
                                        *iter);

  va_start(ap, iter);
  while ((colind = va_arg(ap, int)) >= 0) {
    switch(tree_model->coltype[colind]) {
    case G_TYPE_STRING:
      strpt = va_arg(ap, char **);
      *strpt = g_strdup(row->data[colind]);
      break;
    case G_TYPE_UINT:
      uintpt = va_arg(ap, unsigned *);
      *uintpt = GPOINTER_TO_UINT(row->data[colind]);
      break;
    case G_TYPE_INT:
      intpt = va_arg(ap, int *);
      *intpt = GPOINTER_TO_INT(row->data[colind]);
      break;
    case G_TYPE_POINTER:
      ptpt = va_arg(ap, gpointer *);
      *ptpt = row->data[colind];
      break;
    }
  }
  va_end(ap);
}

gboolean gtk_tree_model_iter_nth_child(GtkTreeModel *tree_model,
                                       GtkTreeIter *iter,
                                       GtkTreeIter *parent, gint n)
{
  /* We only work with one level (lists) for now */
  g_assert(parent == NULL);
  *iter = n;
  return TRUE;
}

gint gtk_tree_model_iter_n_children(GtkTreeModel *tree_model,
                                    GtkTreeIter *iter)
{
  /* We only work with one level (lists) for now */
  if (iter) {
    return 1;
  } else {
    return tree_model->rows->len;
  }
}


static void gtk_tree_view_column_free(gpointer data)
{
  GtkTreeViewColumn *col = data;
  g_free(col->title);
  g_free(col);
}

void gtk_tree_model_free(GtkTreeModel *model)
{
  gtk_list_store_clear(model);  /* Remove all rows */
  g_array_free(model->rows, TRUE);
  g_array_free(model->sort_func, TRUE);
  g_free(model->coltype);
  g_free(model);
}

void gtk_tree_view_destroy(GtkWidget *widget)
{
  GtkTreeView *view = GTK_TREE_VIEW(widget);
  g_slist_free_full(view->columns, gtk_tree_view_column_free);
  view->columns = NULL;
  if (view->model) {
      gtk_tree_model_free(view->model);
  }
  view->model = NULL;
}

void gtk_tree_view_show(GtkWidget *widget)
{
  if (GTK_WIDGET_REALIZED(widget)) {
    ShowWindow(GTK_TREE_VIEW(widget)->scrollwin, SW_SHOWNORMAL);
  }
}

void gtk_tree_view_hide(GtkWidget *widget)
{
  if (GTK_WIDGET_REALIZED(widget)) {
    ShowWindow(GTK_TREE_VIEW(widget)->scrollwin, SW_HIDE);
  }
}

/* Draw an individual cell (row+column) */
static void draw_cell_text(GtkTreeViewColumn *col, GtkTreeModel *model,
                           LPDRAWITEMSTRUCT lpdis, GtkListStoreRow *row,
                           RECT *rcCol)
{
  UINT align;
  char *val;
  int modcol = col->model_column;
  /* Convert float 0.0, 0.5 or 1.0 into int, allow for some rounding error */
  switch((int)(col->xalign * 10. + 0.1)) {
  case 10:
    align = DT_RIGHT;
    break;
  case 5:
    align = DT_CENTER;
    break;
  default:
    align = DT_LEFT;
    break;
  }
  align |= DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;

  switch(model->coltype[modcol]) {
  case G_TYPE_STRING:
    if (row->data[modcol]) {
      myDrawText(lpdis->hDC, row->data[modcol], -1, rcCol, align);
    }
    break;
  case G_TYPE_UINT:
    val = g_strdup_printf("%u", GPOINTER_TO_UINT(row->data[modcol]));
    myDrawText(lpdis->hDC, val, -1, rcCol, align);
    g_free(val);
    break;
  case G_TYPE_INT:
    val = g_strdup_printf("%d", GPOINTER_TO_INT(row->data[modcol]));
    myDrawText(lpdis->hDC, val, -1, rcCol, align);
    g_free(val);
    break;
  }
}

void gtk_tree_view_draw_row(GtkTreeView *tv, LPDRAWITEMSTRUCT lpdis)
{
  HBRUSH bkgrnd;
  COLORREF textcol, oldtextcol;
  RECT rcCol;
  int oldbkmode;
  guint nrows;
  gint CurrentX, right;
  GtkListStoreRow *row;

  if (lpdis->itemState & ODS_SELECTED) {
    bkgrnd = (HBRUSH)(1 + COLOR_HIGHLIGHT);
    textcol = (COLORREF)GetSysColor(COLOR_HIGHLIGHTTEXT);
  } else {
    bkgrnd = (HBRUSH)(1 + COLOR_WINDOW);
    textcol = (COLORREF)GetSysColor(COLOR_WINDOWTEXT);
  }
  oldtextcol = SetTextColor(lpdis->hDC, textcol);
  oldbkmode = SetBkMode(lpdis->hDC, TRANSPARENT);
  FillRect(lpdis->hDC, &lpdis->rcItem, bkgrnd);

  nrows = tv->model ? tv->model->rows->len : 0;
  if (lpdis->itemID >= 0 && lpdis->itemID < nrows) {
    int width;
    GSList *colpt;
    row = &g_array_index(tv->model->rows, GtkListStoreRow, lpdis->itemID);
    width = CurrentX = 0;
    for (colpt = tv->columns; colpt; colpt = g_slist_next(colpt)) {
      GtkTreeViewColumn *col = colpt->data;
      width += col->width;
    }
    right = MAX(lpdis->rcItem.right, width);
    rcCol.top = lpdis->rcItem.top;
    rcCol.bottom = lpdis->rcItem.bottom;
    if (row->data)
      for (colpt = tv->columns; colpt; colpt = g_slist_next(colpt)) {
        GtkTreeViewColumn *col = colpt->data;
        rcCol.left = CurrentX + LISTITEMHPACK;
        CurrentX += col->width;
        rcCol.right = CurrentX - LISTITEMHPACK;
        if (rcCol.left > right)
          rcCol.left = right;
        if (rcCol.right > right - LISTITEMHPACK)
          rcCol.right = right - LISTITEMHPACK;
        if (!g_slist_next(colpt))
          rcCol.right = right - LISTITEMHPACK;
        draw_cell_text(col, tv->model, lpdis, row, &rcCol);
      }
  }

  SetTextColor(lpdis->hDC, oldtextcol);
  SetBkMode(lpdis->hDC, oldbkmode);
  if (lpdis->itemState & ODS_FOCUS) {
    DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
  }
}

void gtk_tree_view_do_auto_resize(GtkTreeView *tv)
{
  GSList *colpt;
  gint i;

  for (colpt = tv->columns, i = 0; colpt; colpt = g_slist_next(colpt), i++) {
    GtkTreeViewColumn *col = colpt->data;
    if (col->auto_resize) {
      gtk_tree_view_set_column_width(tv, i, col->optimal_width);
    }
  }
}

gint gtk_tree_view_optimal_column_width(GtkTreeView *tv, gint column)
{
  GtkTreeViewColumn *col = g_slist_nth_data(tv->columns, column);
  return col->optimal_width;
}

void gtk_tree_view_update_all_widths(GtkTreeView *tv)
{
  SIZE size;
  HWND header;
  gint i;

  header = tv->header;
  if (header) {
    GSList *colpt;
    for (colpt = tv->columns, i = 0; colpt; colpt = g_slist_next(colpt), i++) {
      GtkTreeViewColumn *col = colpt->data;
      if (GetTextSize(header, col->title, &size, defFont)) {
        int new_width = size.cx + 4 + 2 * LISTHEADERPACK;
        col->width = MAX(col->width, new_width);
        col->optimal_width = MAX(col->optimal_width, new_width);
      }
    }
  }

  if (tv->model) {
    for (i = 0; i < tv->model->rows->len; ++i) {
      GtkListStoreRow *row = &g_array_index(tv->model->rows,
                                            GtkListStoreRow, i);
      gtk_tree_view_update_widths(tv, tv->model, row);
    }
  }

  gtk_tree_view_set_extent(tv);
}

void gtk_tree_view_update_widths(GtkTreeView *tv, GtkTreeModel *model,
                                 GtkListStoreRow *row)
{
  SIZE size;
  GSList *colpt;
  HWND hWnd;

  hWnd = GTK_WIDGET(tv)->hWnd;
  if (!hWnd)
    return;
  for (colpt = tv->columns; colpt; colpt = g_slist_next(colpt)) {
    GtkTreeViewColumn *col = colpt->data;
    int modcol = col->model_column;
    char *text;
    switch (model->coltype[modcol]) {
    case G_TYPE_STRING:
      text = row->data[modcol];
      break;
    case G_TYPE_UINT:
    case G_TYPE_INT:
      text = "9999"; /* hack */
      break;
    default:
      text = NULL;
    }
    if (text && GetTextSize(hWnd, text, &size, defFont)) {
      int new_width = size.cx + 4 + 2 * LISTITEMHPACK;
      col->optimal_width = MAX(col->optimal_width, new_width);
    }
  }
}

gboolean gtk_list_store_remove(GtkListStore *list_store, GtkTreeIter *iter)
{
  gint rowind = *iter;
  if (rowind >= 0 && rowind < list_store->rows->len) {
    GtkListStoreRow *row = &g_array_index(list_store->rows,
                                          GtkListStoreRow, rowind);
    gtk_list_store_row_free(row, list_store);
    g_array_remove_index(list_store->rows, rowind);

    if (list_store->view && GTK_WIDGET_REALIZED(GTK_WIDGET(list_store->view))) {
      HWND hWnd = GTK_WIDGET(list_store->view)->hWnd;

      mySendMessage(hWnd, LB_DELETESTRING, (WPARAM)rowind, 0);
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

GtkWidget *gtk_scrolled_tree_view_new(GtkWidget **pack_widg)
{
  GtkWidget *widget;

  widget = gtk_tree_view_new();
  *pack_widg = widget;
  return widget;
}

void gtk_tree_view_set_column_width(GtkTreeView *tv, gint column, gint width)
{
  gtk_tree_view_set_column_width_full(tv, column, width, TRUE);
}

void gtk_tree_view_set_column_width_full(GtkTreeView *tv, gint column,
                                         gint width, gboolean ResizeHeader)
{
  int ncols;
  GtkTreeViewColumn *col;
  HWND hWnd, header;
  HD_ITEM hdi;

  ncols = g_slist_length(tv->columns);
  if (column < 0 || column >= ncols)
    return;
  col = g_slist_nth_data(tv->columns, column);

  col->width = width;
  if (GTK_WIDGET_REALIZED(GTK_WIDGET(tv))) {
    header = tv->header;
    if (ResizeHeader && header) {
      hdi.mask = HDI_WIDTH;
      if (column == ncols - 1)
        width = 9000;
      hdi.cxy = width;
      if (mySendMessage(header, HDM_GETITEM, (WPARAM)column, (LPARAM)&hdi)
          && hdi.cxy != width) {
        hdi.mask = HDI_WIDTH;
        hdi.cxy = width;
        mySendMessage(header, HDM_SETITEM, (WPARAM)column, (LPARAM)&hdi);
      }
    }
    gtk_tree_view_set_extent(tv);
    hWnd = GTK_WIDGET(tv)->hWnd;
    if (hWnd)
      InvalidateRect(hWnd, NULL, FALSE);
  }
}

void gtk_tree_selection_set_mode(GtkTreeSelection *selection,
                                 GtkSelectionMode type)
{
  selection->mode = type;
}

void gtk_tree_selection_select_path(GtkTreeSelection *selection,
                                    GtkTreePath *path)
{
  HWND hWnd;
  guint row = *path;

  hWnd = GTK_WIDGET(selection)->hWnd;
  if (hWnd) {
    if (selection->mode == GTK_SELECTION_SINGLE) {
      mySendMessage(hWnd, LB_SETCURSEL, (WPARAM)row, 0);
    } else {
      mySendMessage(hWnd, LB_SETSEL, (WPARAM)TRUE, (LPARAM)row);
    }
    gtk_tree_view_update_selection(GTK_WIDGET(selection));
  }
}

void gtk_tree_selection_unselect_all(GtkTreeSelection *selection)
{
  GList *sel;
  for (sel = selection->selection; sel; sel = g_list_next(sel)) {
    guint row = GPOINTER_TO_UINT(sel->data);
    gtk_tree_selection_unselect_path(selection, &row);
  }
}

GList *gtk_tree_selection_get_selected_rows(GtkTreeSelection *selection,
                                            GtkTreeModel **model)
{
  GList *sel, *pathsel = NULL;
  for (sel = selection->selection; sel; sel = g_list_next(sel)) {
    guint row = GPOINTER_TO_UINT(sel->data);
    GtkTreePath *path = g_new(GtkTreePath, 1);
    *path = row;
    pathsel = g_list_append(pathsel, path);
  }
  if (model) {
    *model = selection->model;
  }
  return pathsel;
}

gint *gtk_tree_path_get_indices_with_depth(GtkTreePath *path, gint *depth)
{
  /* Only one level; path *is* the row index */
  *depth = 1;
  return path;
}

void gtk_tree_selection_unselect_path(GtkTreeSelection *selection,
                                      GtkTreePath *path)
{
  HWND hWnd;
  guint row = *path;

  hWnd = GTK_WIDGET(selection)->hWnd;
  if (hWnd) {
    if (selection->mode == GTK_SELECTION_SINGLE) {
      mySendMessage(hWnd, LB_SETCURSEL, (WPARAM)(-1), 0);
    } else {
      mySendMessage(hWnd, LB_SETSEL, (WPARAM)FALSE, (LPARAM)row);
    }
    gtk_tree_view_update_selection(GTK_WIDGET(selection));
  }
}

gint gtk_tree_selection_count_selected_rows(GtkTreeSelection *selection)
{
  return g_list_length(selection->selection);
}

gboolean gtk_tree_selection_get_selected(GtkTreeSelection *selection,
                                         GtkTreeModel **model,
                                         GtkTreeIter *iter)
{
  if (model) {
    *model = selection->model;
  }

  /* Just return the first selected row */
  if (selection->selection) {
    if (iter) {
      int row = GPOINTER_TO_INT(g_list_nth_data(selection->selection, 0));
      *iter = row;
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

void gtk_tree_selection_selected_foreach(GtkTreeSelection *selection,
                                         GtkTreeSelectionForeachFunc func,
                                         gpointer data)
{
  GList *sel;
  for (sel = selection->selection; sel; sel = g_list_next(sel)) {
    guint row = GPOINTER_TO_UINT(sel->data);
    func(selection->model, &row, &row, data);
  }
}

void gtk_tree_view_update_selection(GtkWidget *widget)
{
  GtkTreeView *tv = GTK_TREE_VIEW(widget);
  gint i;

  g_list_free(tv->selection);
  tv->selection = NULL;
  if (widget->hWnd) {
    if (tv->model) for (i = 0; i < tv->model->rows->len; i++) {
      if (mySendMessage(widget->hWnd, LB_GETSEL, (WPARAM)i, 0) > 0) {
        tv->selection = g_list_append(tv->selection, GINT_TO_POINTER(i));
      }
    }

    gtk_signal_emit(G_OBJECT(widget), "changed");
  }
}

static LRESULT CALLBACK TreeViewHdrWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                           LPARAM lParam)
{
  GtkWidget *widget;
  gboolean retval = FALSE, dodef = TRUE;

  widget = GTK_WIDGET(GetWindowLongPtr(hwnd, GWLP_USERDATA));

  if (widget) {
    retval = gtk_tree_view_wndproc(widget, msg, wParam, lParam, &dodef);
  }

  if (dodef) {
    return myDefWindowProc(hwnd, msg, wParam, lParam);
  } else {
    return retval;
  }
}

void InitTreeViewClass(HINSTANCE hInstance)
{
  WNDCLASS wc;

  wc.style = 0;
  wc.lpfnWndProc = TreeViewHdrWndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = NULL;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = WC_GTKTREEVIEWHDR;
  myRegisterClass(&wc);
}

/* Make a new GtkListStore and fill in the column types */
GtkListStore *gtk_list_store_new(gint n_columns, ...)
{
  GtkListStore *store;
  int i;

  va_list ap;
  va_start(ap, n_columns);

  store = GTK_LIST_STORE(GtkNewObject(&GtkListStoreClass));
  store->view = NULL;
  store->ncols = n_columns;
  store->coltype = g_new(int, n_columns);
  store->rows = g_array_new(FALSE, FALSE, sizeof(GtkListStoreRow));
  store->sort_column_id = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
  store->sort_func = g_array_new(FALSE, TRUE, sizeof(gpointer));
  store->need_sort = FALSE;
  for (i = 0; i < n_columns; ++i) {
    store->coltype[i] = va_arg(ap, int);
  }
  va_end(ap);
  return store;
}

void gtk_tree_sortable_set_sort_func(GtkTreeSortable *sortable,
                                     gint sort_column_id,
                                     GtkTreeIterCompareFunc sort_func,
                                     gpointer user_data,
                                     GDestroyNotify destroy)
{
  /* We don't currently support user_data */
  if (sort_column_id >= sortable->sort_func->len) {
    g_array_set_size(sortable->sort_func, sort_column_id+1);
  }
  g_array_index(sortable->sort_func, gpointer, sort_column_id) = sort_func;
}

void gtk_tree_sortable_set_sort_column_id(GtkTreeSortable *sortable,
                                          gint sort_column_id,
                                          GtkSortType order)
{
  if (sortable->sort_column_id != sort_column_id
      || sortable->sort_order != order) {
    sortable->sort_column_id = sort_column_id;
    sortable->sort_order = order;
    sortable->need_sort = TRUE;
  }
}

/* We don't support customizing renderers right now */
GtkCellRenderer *gtk_cell_renderer_text_new(void)
{
  return NULL;
}

static GtkTreeViewColumn *new_column_internal(const char *title, va_list args)
{
  GtkTreeViewColumn *col;
  const char *name;

  col = g_new0(GtkTreeViewColumn, 1);
  col->title = g_strdup(title);
  col->resizeable = FALSE;
  col->expand = FALSE;
  col->auto_resize = TRUE;
  col->sort_column_id = -1;
  col->model_column = -1;
  col->xalign = 0.0;   /* left align by default */

  /* Currently we only support the "text" attribute to point to the
     ListStore column */
  while ((name = va_arg(args, const char *)) != NULL) {
    if (strcmp(name, "text") == 0) {
      col->model_column = va_arg(args, int);
    }
  }
  return col;
}

GtkTreeViewColumn *gtk_tree_view_column_new_with_attributes
                   (const gchar *title, GtkCellRenderer *cell, ...)
{
  GtkTreeViewColumn *col;
  va_list args;

  va_start(args, cell);
  col = new_column_internal(title, args);
  va_end(args);
  return col;
}

gint gtk_tree_view_insert_column_with_attributes
            (GtkTreeView *tree_view, gint position, const gchar *title,
             GtkCellRenderer *cell, ...)
{
  GtkTreeViewColumn *col;
  va_list args;

  va_start(args, cell);
  col = new_column_internal(title, args);
  va_end(args);
  return gtk_tree_view_insert_column(tree_view, col, position);
}

void gtk_tree_view_scroll_to_cell(GtkTreeView *tree_view,
                                  GtkTreePath *path,
                                  GtkTreeViewColumn *column,
                                  gboolean use_align, gfloat row_align,
                                  gfloat col_align)
{
  /* not implemented */
}

void gtk_tree_view_column_set_resizable(GtkTreeViewColumn *tree_column,
                                        gboolean resizable)
{
  tree_column->resizeable = resizable;
}

void gtk_tree_view_column_set_expand(GtkTreeViewColumn *tree_column,
                                     gboolean expand)
{
  tree_column->expand = expand;
}

void gtk_tree_view_column_set_sort_column_id(GtkTreeViewColumn *tree_column,
                                             gint sort_column_id)
{
  tree_column->sort_column_id = sort_column_id;
}

void gtk_tree_view_column_set_alignment(GtkTreeViewColumn *tree_column,
                                        gfloat xalign)
{
  tree_column->xalign = xalign;
}

gint gtk_tree_view_insert_column(GtkTreeView *tree_view,
                                 GtkTreeViewColumn *column,
                                 gint position)
{
  tree_view->columns = g_slist_insert(tree_view->columns, column, position);
  return g_slist_length(tree_view->columns);
}

GtkTreeViewColumn *gtk_tree_view_get_column(GtkTreeView *tree_view, gint n)
{
  return g_slist_nth_data(tree_view->columns, n);
}

void gtk_tree_view_set_model(GtkTreeView *tree_view, GtkTreeModel *model)
{
  /* We only support a single model per view, so ignore attempts to remove it */
  if (model) {
    tree_view->model = model;
    model->view = tree_view;
    /* todo: update view if necessary */
  }
}

GtkTreeModel *gtk_tree_view_get_model(GtkTreeView *tree_view)
{
  return tree_view->model;
}

void gtk_tree_view_set_headers_clickable(GtkTreeView *tree_view,
                                         gboolean setting)
{
  tree_view->headers_clickable = setting;
}

/* These are noops; we only use these for GtkListStore, which should always
   be owned (and thus freed) by our GtkTreeView */
void g_object_unref(gpointer object)
{
}

gpointer g_object_ref(gpointer object)
{
  return object;
}

#else /* for systems with GTK+ */

GtkWidget *gtk_scrolled_tree_view_new(GtkWidget **pack_widg)
{
  GtkWidget *scrollwin, *clist;

  clist = gtk_tree_view_new();
  scrollwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrollwin), clist);
  *pack_widg = scrollwin;
  return clist;
}

#endif
