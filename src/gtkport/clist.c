/************************************************************************
 * clist.c        GtkCList implementation for gtkport                   *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gtkport.h"

#ifdef CYGWIN

#include <windows.h>
#include <commctrl.h>

#define LISTITEMHPACK  3
#define LISTHEADERPACK 6

static void gtk_clist_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition);
static void gtk_clist_set_size(GtkWidget *widget,
                               GtkAllocation *allocation);
static gboolean gtk_clist_wndproc(GtkWidget *widget, UINT msg, WPARAM wParam,
                                  LPARAM lParam, gboolean *dodef);
static void gtk_clist_realize(GtkWidget *widget);
static void gtk_clist_show(GtkWidget *widget);
static void gtk_clist_hide(GtkWidget *widget);
static void gtk_clist_draw_row(GtkCList *clist, LPDRAWITEMSTRUCT lpdis);
static void gtk_clist_update_selection(GtkWidget *widget);
static void gtk_clist_update_widths(GtkCList *clist, gchar *text[]);
static void gtk_clist_update_all_widths(GtkCList *clist);
static void gtk_clist_do_auto_resize(GtkCList *clist);
static void gtk_clist_set_column_width_full(GtkCList *clist, gint column,
                                            gint width,
                                            gboolean ResizeHeader);

static GtkSignalType GtkCListSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_clist_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_clist_set_size},
  {"realize", gtk_marshal_VOID__VOID, gtk_clist_realize},
  {"click-column", gtk_marshal_VOID__GINT, NULL},
  {"select_row", gtk_marshal_VOID__GINT_GINT_EVENT, NULL},
  {"unselect_row", gtk_marshal_VOID__GINT_GINT_EVENT, NULL},
  {"show", gtk_marshal_VOID__VOID, gtk_clist_show},
  {"hide", gtk_marshal_VOID__VOID, gtk_clist_hide},
  {"", NULL, NULL}
};

static GtkClass GtkCListClass = {
  "clist", &GtkContainerClass, sizeof(GtkCList), GtkCListSignals,
  gtk_clist_wndproc
};

gboolean gtk_clist_wndproc(GtkWidget *widget, UINT msg, WPARAM wParam,
                           LPARAM lParam, gboolean *dodef)
{
  LPDRAWITEMSTRUCT lpdis;
  HD_NOTIFY FAR *phdr;
  NMHDR *nmhdr;

  switch(msg) {
  case WM_COMMAND:
    if (lParam && HIWORD(wParam) == LBN_SELCHANGE) {
      gtk_clist_update_selection(widget);
      return FALSE;
    }
    break;
  case WM_DRAWITEM:
    lpdis = (LPDRAWITEMSTRUCT)lParam;
    if (lpdis) {
      gtk_clist_draw_row(GTK_CLIST(widget), lpdis);
      return FALSE;
    }
    break;
  case WM_NOTIFY:
    nmhdr = (NMHDR *)lParam;
    phdr = (HD_NOTIFY FAR *)lParam;
    if (nmhdr && nmhdr->code == HDN_ENDTRACK) {
      gtk_clist_set_column_width_full(GTK_CLIST(widget), phdr->iItem,
                                      phdr->pitem->cxy, FALSE);
      return FALSE;
    } else if (nmhdr && nmhdr->code == HDN_ITEMCLICK) {
      gtk_signal_emit(GTK_OBJECT(widget), "click-column", (gint)phdr->iItem);
      return FALSE;
    }
    break;
  }

  return FALSE;
}

void gtk_clist_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkCList *clist = GTK_CLIST(widget);

  gtk_container_set_size(widget, allocation);
  if (clist->header) {
    SetWindowPos(clist->header, HWND_TOP,
                 allocation->x, allocation->y,
                 allocation->width, clist->header_size, SWP_NOZORDER);
    allocation->y += clist->header_size - 1;
    allocation->height -= clist->header_size - 1;
  }
}

GtkWidget *gtk_clist_new(gint columns)
{
  GtkCList *clist;
  int i;

  clist = GTK_CLIST(GtkNewObject(&GtkCListClass));
  clist->cols = columns;
  clist->coldata = g_new0(GtkCListColumn, columns);
  clist->rows = 0;
  clist->rowdata = NULL;
  for (i = 0; i < columns; i++) {
    clist->coldata[i].width = 0;
    clist->coldata[i].visible = TRUE;
    clist->coldata[i].resizeable = TRUE;
  }

  return GTK_WIDGET(clist);
}

void gtk_clist_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  SIZE size;

  if (GetTextSize(widget->hWnd, "Sample text", &size, defFont)) {
    requisition->width = size.cx;
    requisition->height = size.cy * 6 + 12;
  }
}

void gtk_clist_realize(GtkWidget *widget)
{
  HWND Parent, header;
  HD_LAYOUT hdl;
  HD_ITEM hdi;
  RECT rcParent;
  WINDOWPOS wp;
  GtkCList *clist = GTK_CLIST(widget);
  GSList *rows;
  GtkCListRow *row;
  gint i;

  gtk_container_realize(widget);
  Parent = gtk_get_parent_hwnd(widget);
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  rcParent.left = rcParent.top = 0;
  rcParent.right = rcParent.bottom = 800;
  header = CreateWindowEx(0, WC_HEADER, NULL,
                          WS_CHILD | WS_BORDER | HDS_HORZ
                          | (GTK_CLIST(widget)->coldata[0].button_passive ?
                             0 : HDS_BUTTONS),
                          0, 0, 0, 0, Parent, NULL, hInst, NULL);
  SetWindowLong(header, GWL_USERDATA, (LONG)widget);
  GTK_CLIST(widget)->header = header;
  gtk_set_default_font(header);
  hdl.prc = &rcParent;
  hdl.pwpos = &wp;
  SendMessage(header, HDM_LAYOUT, 0, (LPARAM)&hdl);
  clist->header_size = wp.cy;
  widget->hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "",
                                WS_CHILD | WS_TABSTOP | LBS_DISABLENOSCROLL
                                | WS_VSCROLL | LBS_OWNERDRAWFIXED |
                                LBS_NOTIFY, 0, 0, 0, 0, Parent, NULL,
                                hInst, NULL);
  gtk_set_default_font(widget->hWnd);

  gtk_clist_update_all_widths(clist);
  for (rows = clist->rowdata; rows; rows = g_slist_next(rows)) {
    row = (GtkCListRow *)rows->data;
    if (row)
      SendMessage(widget->hWnd, LB_ADDSTRING, 0, (LPARAM)row->data);
  }

  for (i = 0; i < clist->cols; i++) {
    hdi.mask = HDI_TEXT | HDI_FORMAT | HDI_WIDTH;
    hdi.pszText = clist->coldata[i].title;
    if (hdi.pszText) {
      if (i == clist->cols - 1)
        hdi.cxy = 9000;
      else
        hdi.cxy = clist->coldata[i].width;
      hdi.cchTextMax = strlen(hdi.pszText);
      hdi.fmt = HDF_LEFT | HDF_STRING;
      SendMessage(header, HDM_INSERTITEM, i + 1, (LPARAM)&hdi);
    }
  }
}

void gtk_clist_show(GtkWidget *widget)
{
  if (GTK_WIDGET_REALIZED(widget)) {
    ShowWindow(GTK_CLIST(widget)->header, SW_SHOWNORMAL);
  }
}

void gtk_clist_hide(GtkWidget *widget)
{
  if (GTK_WIDGET_REALIZED(widget)) {
    ShowWindow(GTK_CLIST(widget)->header, SW_HIDE);
  }
}

void gtk_clist_draw_row(GtkCList *clist, LPDRAWITEMSTRUCT lpdis)
{
  HBRUSH bkgrnd;
  COLORREF textcol, oldtextcol;
  RECT rcCol;
  gint i, CurrentX;
  GtkCListRow *row;

  if (lpdis->itemState & ODS_SELECTED) {
    bkgrnd = (HBRUSH)(1 + COLOR_HIGHLIGHT);
    textcol = (COLORREF)GetSysColor(COLOR_HIGHLIGHTTEXT);
  } else {
    bkgrnd = (HBRUSH)(1 + COLOR_WINDOW);
    textcol = (COLORREF)GetSysColor(COLOR_WINDOWTEXT);
  }
  oldtextcol = SetTextColor(lpdis->hDC, textcol);
  SetBkMode(lpdis->hDC, TRANSPARENT);
  FillRect(lpdis->hDC, &lpdis->rcItem, bkgrnd);

  if (lpdis->itemID >= 0 && lpdis->itemID < clist->rows) {
    row = (GtkCListRow *)g_slist_nth_data(clist->rowdata, lpdis->itemID);
    CurrentX = lpdis->rcItem.left;
    rcCol.top = lpdis->rcItem.top;
    rcCol.bottom = lpdis->rcItem.bottom;
    if (row->text)
      for (i = 0; i < clist->cols; i++) {
        rcCol.left = CurrentX + LISTITEMHPACK;
        CurrentX += clist->coldata[i].width;
        rcCol.right = CurrentX - LISTITEMHPACK;
        if (rcCol.left > lpdis->rcItem.right)
          rcCol.left = lpdis->rcItem.right;
        if (rcCol.right > lpdis->rcItem.right)
          rcCol.right = lpdis->rcItem.right;
        if (i == clist->cols - 1)
          rcCol.right = lpdis->rcItem.right;
        if (row->text[i]) {
          DrawText(lpdis->hDC, row->text[i], -1, &rcCol,
                   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        }
      }
  }

  SetTextColor(lpdis->hDC, oldtextcol);
  SetBkMode(lpdis->hDC, OPAQUE);
  if (lpdis->itemState & ODS_FOCUS)
    DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
}

void gtk_clist_do_auto_resize(GtkCList *clist)
{
  gint i;

  for (i = 0; i < clist->cols; i++)
    if (clist->coldata[i].auto_resize) {
      gtk_clist_set_column_width(clist, i, clist->coldata[i].width);
    }
}

void gtk_clist_update_all_widths(GtkCList *clist)
{
  GSList *list;
  GtkCListRow *row;
  gint i;
  SIZE size;
  HWND header;

  header = clist->header;
  if (header)
    for (i = 0; i < clist->cols; i++) {
      if (GetTextSize(header, clist->coldata[i].title, &size, defFont) &&
          clist->coldata[i].width < size.cx + 2 * LISTHEADERPACK) {
        clist->coldata[i].width = size.cx + 2 * LISTHEADERPACK;
      }
    }

  for (list = clist->rowdata; list; list = g_slist_next(list)) {
    row = (GtkCListRow *)list->data;
    if (row && row->text)
      gtk_clist_update_widths(clist, row->text);
  }
}

void gtk_clist_update_widths(GtkCList *clist, gchar *text[])
{
  gint i;
  SIZE size;
  HWND hWnd;

  hWnd = GTK_WIDGET(clist)->hWnd;
  if (!hWnd)
    return;
  for (i = 0; i < clist->cols; i++) {
    if (clist->coldata[i].auto_resize
        && GetTextSize(hWnd, text[i], &size, defFont)
        && size.cx + 2 * LISTITEMHPACK > clist->coldata[i].width) {
      clist->coldata[i].width = size.cx + 2 * LISTITEMHPACK;
    }
  }
}

gint gtk_clist_insert(GtkCList *clist, gint row, gchar *text[])
{
  GtkWidget *widget = GTK_WIDGET(clist);
  HWND hWnd;
  GtkCListRow *new_row;
  gint i;

  if (row < 0)
    row = clist->rows;

  new_row = g_new0(GtkCListRow, 1);
  new_row->text = g_new0(gchar *, clist->cols);

  for (i = 0; i < clist->cols; i++) {
    new_row->text[i] = g_strdup(text[i]);
  }
  gtk_clist_update_widths(clist, new_row->text);
  gtk_clist_do_auto_resize(clist);
  clist->rowdata = g_slist_insert(clist->rowdata, (gpointer)new_row, row);
  clist->rows = g_slist_length(clist->rowdata);

  if (GTK_WIDGET_REALIZED(widget)) {
    hWnd = widget->hWnd;
    SendMessage(hWnd, LB_INSERTSTRING, (WPARAM)row, (LPARAM)NULL);
  }

  return row;
}

gint gtk_clist_set_text(GtkCList *clist, gint row, gint col, gchar *text)
{
  GtkCListRow *list_row;

  if (row < 0 || row >= clist->rows || col < 0 || col >= clist->cols)
    return -1;

  list_row = (GtkCListRow *)g_slist_nth_data(clist->rowdata, row);
  g_free(list_row->text[col]);
  list_row->text[col] = g_strdup(text);

  if (GTK_WIDGET_REALIZED(GTK_WIDGET(clist))) {
    HWND hWnd = GTK_WIDGET(clist)->hWnd;

    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
  }
  return row;
}

void gtk_clist_remove(GtkCList *clist, gint row)
{
  if (row >= 0 && row < clist->rows) {
    GSList *dellink;
    GtkCListRow *delrow;
    int i;

    gtk_clist_unselect_row(clist, row, 0);
    dellink = g_slist_nth(clist->rowdata, row);
    delrow = (GtkCListRow *)dellink->data;
    for (i = 0; i < clist->cols; i++) {
      g_free(delrow->text[i]);
    }
    g_free(delrow->text);
    clist->rowdata = g_slist_remove_link(clist->rowdata, dellink);
    g_free(dellink);

    clist->rows = g_slist_length(clist->rowdata);

    if (GTK_WIDGET_REALIZED(GTK_WIDGET(clist))) {
      HWND hWnd = GTK_WIDGET(clist)->hWnd;

      SendMessage(hWnd, LB_DELETESTRING, (WPARAM)row, 0);
    }
  }
}

GtkWidget *gtk_clist_new_with_titles(gint columns, gchar *titles[])
{
  GtkWidget *widget;
  GtkCList *clist;
  gint i;

  widget = gtk_clist_new(columns);
  clist = GTK_CLIST(widget);
  for (i = 0; i < clist->cols; i++) {
    gtk_clist_set_column_title(clist, i, titles[i]);
  }
  return widget;
}

GtkWidget *gtk_scrolled_clist_new_with_titles(gint columns,
                                              gchar *titles[],
                                              GtkWidget **pack_widg)
{
  GtkWidget *widget;

  widget = gtk_clist_new_with_titles(columns, titles);
  *pack_widg = widget;
  return widget;
}

gint gtk_clist_append(GtkCList *clist, gchar *text[])
{
  return gtk_clist_insert(clist, -1, text);
}

void gtk_clist_set_column_title(GtkCList *clist, gint column,
                                const gchar *title)
{
  HWND hWnd;

  if (column < 0 || column >= clist->cols)
    return;
  g_free(clist->coldata[column].title);
  clist->coldata[column].title = g_strdup(title);
  if (GTK_WIDGET_REALIZED(GTK_WIDGET(clist))) {
    hWnd = GTK_WIDGET(clist)->hWnd;
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
  }
}

void gtk_clist_column_title_passive(GtkCList *clist, gint column)
{
  if (column >= 0 && column < clist->cols)
    clist->coldata[column].button_passive = TRUE;
}

void gtk_clist_column_titles_passive(GtkCList *clist)
{
  gint i;

  for (i = 0; i < clist->cols; i++) {
    gtk_clist_column_title_passive(clist, i);
  }
}

void gtk_clist_column_title_active(GtkCList *clist, gint column)
{
  if (column >= 0 && column < clist->cols)
    clist->coldata[column].button_passive = FALSE;
}

void gtk_clist_column_titles_active(GtkCList *clist)
{
  gint i;

  for (i = 0; i < clist->cols; i++) {
    gtk_clist_column_title_active(clist, i);
  }
}

void gtk_clist_set_column_width(GtkCList *clist, gint column, gint width)
{
  gtk_clist_set_column_width_full(clist, column, width, TRUE);
}

void gtk_clist_set_column_width_full(GtkCList *clist, gint column,
                                     gint width, gboolean ResizeHeader)
{
  HWND hWnd, header;
  HD_ITEM hdi;

  if (column < 0 || column >= clist->cols)
    return;

  clist->coldata[column].width = width;
  if (GTK_WIDGET_REALIZED(GTK_WIDGET(clist))) {
    header = clist->header;
    if (ResizeHeader && header) {
      hdi.mask = HDI_WIDTH;
      if (column == clist->cols - 1)
        width = 9000;
      hdi.cxy = width;
      if (SendMessage(header, HDM_GETITEM, (WPARAM)column, (LPARAM)&hdi) &&
          hdi.cxy != width) {
        hdi.mask = HDI_WIDTH;
        hdi.cxy = width;
        SendMessage(header, HDM_SETITEM, (WPARAM)column, (LPARAM)&hdi);
      }
    }
    hWnd = GTK_WIDGET(clist)->hWnd;
    if (hWnd)
      InvalidateRect(hWnd, NULL, FALSE);
  }
}

void gtk_clist_set_selection_mode(GtkCList *clist, GtkSelectionMode mode)
{
  clist->mode = mode;
}

static GtkCList *sorting_clist;
static gint gtk_clist_sort_func(gconstpointer a, gconstpointer b)
{
  return (*sorting_clist->cmp_func) (sorting_clist, a, b);
}

void gtk_clist_sort(GtkCList *clist)
{
  HWND hWnd;
  gint rowind;
  GList *sel;
  GSList *rowpt;

  sorting_clist = clist;
  if (clist && clist->cmp_func && clist->rows) {
    /* Since the order of the list may change, we need to change the
     * selection as well. Do this by converting the row indices into
     * GSList pointers (which are invariant to the sort) and then convert
     * back afterwards */
    for (sel = clist->selection; sel; sel = g_list_next(sel)) {
      rowind = GPOINTER_TO_INT(sel->data);
      sel->data = (gpointer)g_slist_nth(clist->rowdata, rowind);
    }
    clist->rowdata = g_slist_sort(clist->rowdata, gtk_clist_sort_func);
    for (sel = clist->selection; sel; sel = g_list_next(sel)) {
      rowpt = (GSList *)(sel->data);
      sel->data = GINT_TO_POINTER(g_slist_position(clist->rowdata, rowpt));
    }
    if (GTK_WIDGET_REALIZED(GTK_WIDGET(clist))) {
      hWnd = GTK_WIDGET(clist)->hWnd;
      if (clist->mode == GTK_SELECTION_SINGLE) {
        sel = clist->selection;
        if (sel)
          rowind = GPOINTER_TO_INT(sel->data);
        else
          rowind = -1;
        SendMessage(hWnd, LB_SETCURSEL, (WPARAM)rowind, 0);
      } else {
        for (rowind = 0; rowind < clist->rows; rowind++) {
          SendMessage(hWnd, LB_SETSEL, (WPARAM)FALSE, (LPARAM)rowind);
        }
        for (sel = clist->selection; sel; sel = g_list_next(sel)) {
          rowind = GPOINTER_TO_INT(sel->data);
          SendMessage(hWnd, LB_SETSEL, (WPARAM)TRUE, (LPARAM)rowind);
        }
      }
      InvalidateRect(hWnd, NULL, FALSE);
      UpdateWindow(hWnd);
    }
  }
}

void gtk_clist_freeze(GtkCList *clist)
{
}

void gtk_clist_thaw(GtkCList *clist)
{
}

void gtk_clist_clear(GtkCList *clist)
{
  GtkCListRow *row;
  GSList *list;
  gint i;
  HWND hWnd;

  for (list = clist->rowdata; list; list = g_slist_next(list)) {
    row = (GtkCListRow *)list->data;
    for (i = 0; i < clist->cols; i++) {
      g_free(row->text[i]);
    }
    g_free(row);
  }
  g_slist_free(clist->rowdata);
  clist->rowdata = NULL;
  clist->rows = 0;

  gtk_clist_update_all_widths(clist);
  hWnd = GTK_WIDGET(clist)->hWnd;
  if (hWnd) {
    SendMessage(hWnd, LB_RESETCONTENT, 0, 0);
  }
}

void gtk_clist_set_row_data(GtkCList *clist, gint row, gpointer data)
{
  GtkCListRow *list_row;

  if (row >= 0 && row < clist->rows) {
    list_row = (GtkCListRow *)g_slist_nth_data(clist->rowdata, row);
    if (list_row)
      list_row->data = data;
  }
}

gpointer gtk_clist_get_row_data(GtkCList *clist, gint row)
{
  GtkCListRow *list_row;

  if (row >= 0 && row < clist->rows) {
    list_row = (GtkCListRow *)g_slist_nth_data(clist->rowdata, row);
    if (list_row)
      return list_row->data;
  }
  return NULL;
}

void gtk_clist_set_auto_sort(GtkCList *clist, gboolean auto_sort)
{
  clist->auto_sort = auto_sort;
}

void gtk_clist_columns_autosize(GtkCList *clist)
{
}

void gtk_clist_select_row(GtkCList *clist, gint row, gint column)
{
  HWND hWnd;

  hWnd = GTK_WIDGET(clist)->hWnd;
  if (hWnd) {
    if (clist->mode == GTK_SELECTION_SINGLE) {
      SendMessage(hWnd, LB_SETCURSEL, (WPARAM)row, 0);
    } else {
      SendMessage(hWnd, LB_SETSEL, (WPARAM)TRUE, (LPARAM)row);
    }
    gtk_clist_update_selection(GTK_WIDGET(clist));
  }
}

void gtk_clist_unselect_row(GtkCList *clist, gint row, gint column)
{
  HWND hWnd;

  hWnd = GTK_WIDGET(clist)->hWnd;
  if (hWnd) {
    if (clist->mode == GTK_SELECTION_SINGLE) {
      SendMessage(hWnd, LB_SETCURSEL, (WPARAM)(-1), 0);
    } else {
      SendMessage(hWnd, LB_SETSEL, (WPARAM)FALSE, (LPARAM)row);
    }
    gtk_clist_update_selection(GTK_WIDGET(clist));
  }
}

GtkVisibility gtk_clist_row_is_visible(GtkCList *clist, gint row)
{
  return GTK_VISIBILITY_FULL;
}

void gtk_clist_moveto(GtkCList *clist, gint row, gint column,
                      gfloat row_align, gfloat col_align)
{
}

void gtk_clist_set_compare_func(GtkCList *clist,
                                GtkCListCompareFunc cmp_func)
{
  if (clist)
    clist->cmp_func = cmp_func;
}

void gtk_clist_set_column_auto_resize(GtkCList *clist, gint column,
                                      gboolean auto_resize)
{
  if (clist && column >= 0 && column < clist->cols) {
    clist->coldata[column].auto_resize = auto_resize;
  }
}

void gtk_clist_update_selection(GtkWidget *widget)
{
  GtkCList *clist = GTK_CLIST(widget);
  GList *oldsel, *selpt;
  gint i;

  oldsel = clist->selection;
  clist->selection = NULL;
  if (widget->hWnd) {
    for (i = 0; i < clist->rows; i++) {
      if (SendMessage(widget->hWnd, LB_GETSEL, (WPARAM)i, 0) > 0) {
        clist->selection = g_list_append(clist->selection, GINT_TO_POINTER(i));
      }
    }

    for (selpt = oldsel; selpt; selpt = g_list_next(selpt)) {
      gint row = GPOINTER_TO_INT(selpt->data);

      if (!g_list_find(clist->selection, GINT_TO_POINTER(row))) {
        gtk_signal_emit(GTK_OBJECT(widget), "unselect_row", row, 0, NULL);
      }
    }

    for (selpt = clist->selection; selpt; selpt = g_list_next(selpt)) {
      gint row = GPOINTER_TO_INT(selpt->data);

      if (!g_list_find(oldsel, GINT_TO_POINTER(row))) {
        gtk_signal_emit(GTK_OBJECT(widget), "select_row", row, 0, NULL);
      }
    }
  }
  g_list_free(oldsel);
}

#else /* for systems with GTK+ */

GtkWidget *gtk_scrolled_clist_new_with_titles(gint columns,
                                              gchar *titles[],
                                              GtkWidget **pack_widg)
{
  GtkWidget *scrollwin, *clist;

  clist = gtk_clist_new_with_titles(columns, titles);
  scrollwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrollwin), clist);
  *pack_widg = scrollwin;
  return clist;
}

#endif
