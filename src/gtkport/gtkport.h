/************************************************************************
 * gtkport.h      Portable "almost-GTK+" for Unix/Win32                 *
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

#ifndef __GTKPORT_H__
#define __GTKPORT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "itemfactory.h"

#ifdef CYGWIN

/* GTK+ emulation prototypes etc. for Win32 platform */

/* Provide prototypes compatible with GTK+3 */
#define GTK_MAJOR_VERSION 3

#include <winsock2.h>
#include <windows.h>
#include <glib.h>
#include <stdarg.h>

#include "gtkenums.h"
#include "gtktypes.h"

#include "clist.h"
#include "treeview.h"

struct _GtkMisc {
  GtkWidget widget;
};

struct _GtkProgressBar {
  GtkWidget widget;
  GtkProgressBarOrientation orient;
  gfloat position;
};

struct _GtkSeparator {
  GtkWidget widget;
};

struct _GtkHSeparator {
  GtkSeparator separator;
};

struct _GtkVSeparator {
  GtkSeparator separator;
};

struct _GtkMenuItem {
  GtkWidget widget;
  GtkMenu *submenu;
  gint ID;
  gint accelind;
  gchar *text;
  gint check:1;
  gint active:1;
};

struct _GtkMenuShell {
  GtkWidget widget;
  HMENU menu;
  GSList *children;
};

struct _GtkMenu {
  GtkMenuShell menushell;
  guint active;
};

struct _GtkMenuBar {
  GtkMenuShell menushell;
  gint LastID;
};

typedef struct _GtkEditable GtkEditable;
typedef struct _GtkEntry GtkEntry;
typedef struct _GtkText GtkText;
typedef struct _GtkTextBuffer GtkTextBuffer;
typedef struct _GtkSpinButton GtkSpinButton;

struct _GtkEditable {
  GtkWidget widget;
  GString *text;
  gint is_editable:1;
};

struct _GtkEntry {
  GtkEditable editable;
  gint is_visible:1;
};

struct _GtkSpinButton {
  GtkEntry entry;
  GtkAdjustment *adj;
  HWND updown;
};

struct _GtkTextBuffer {
  GData *tags;
};

struct _GtkText {
  GtkEditable editable;
  gint word_wrap:1;
  GtkTextBuffer *buffer;
};

typedef struct _GtkLabel GtkLabel;

struct _GtkLabel {
  GtkWidget widget;
  gchar *text;
};

typedef struct _GtkUrl GtkUrl;

struct _GtkUrl {
  GtkLabel label;
  gchar *target;
};

struct _GtkPanedChild {
  GtkWidget *widget;
  gint resize:1;
  gint shrink:1;
};

struct _GtkPaned {
  GtkContainer container;
  GtkPanedChild children[2];
  GtkAllocation true_alloc;
  gint handle_size, gutter_size;
  gint handle_pos;
  gint Tracking:1;
};

struct _GtkVPaned {
  GtkPaned paned;
};

struct _GtkHPaned {
  GtkPaned paned;
};

typedef struct _GtkBox GtkBox;
typedef struct _GtkBoxChild GtkBoxChild;
typedef struct _GtkHBox GtkHBox;
typedef struct _GtkVBox GtkVBox;
typedef struct _GtkNotebookChild GtkNotebookChild;
typedef struct _GtkNotebook GtkNotebook;

struct _GtkBoxChild {
  GtkWidget *widget;
  guint expand:1;
  guint fill:1;
};

struct _GtkBox {
  GtkContainer container;
  GList *children;
  guint16 spacing;
  gint maxreq;
  guint homogeneous:1;
};

struct _GtkHBox {
  GtkBox box;
};

struct _GtkVBox {
  GtkBox box;
};

struct _GtkNotebookChild {
  GtkWidget *child, *tab_label;
  HWND tabpage;
};

struct _GtkNotebook {
  GtkContainer container;
  GSList *children;
  gint selection;
};

typedef struct _GtkBin GtkBin;

struct _GtkBin {
  GtkContainer container;
  GtkWidget *child;
};

typedef struct _GtkFrame GtkFrame;
typedef struct _GtkButton GtkButton;
typedef struct _GtkToggleButton GtkToggleButton;
typedef struct _GtkCheckButton GtkCheckButton;
typedef struct _GtkRadioButton GtkRadioButton;

struct _GtkFrame {
  GtkBin bin;
  gchar *text;
  GtkRequisition label_req;
};

struct _GtkButton {
  GtkWidget widget;
  gchar *text;
};

struct _GtkComboBox {
  GtkWidget widget;
  GtkTreeModel *model;
  gint model_column;
  gint active;
};

struct _GtkToggleButton {
  GtkButton button;
  gboolean toggled;
};

struct _GtkCheckButton {
  GtkToggleButton toggle;
};

struct _GtkRadioButton {
  GtkCheckButton check;
  GSList *group;
};

typedef struct _GtkWindow GtkWindow;

struct _GtkWindow {
  GtkBin bin;
  GtkWindowType type;
  gchar *title;
  gint default_width, default_height;
  GtkMenuBar *menu_bar;
  GtkAccelGroup *accel_group;
  GtkWidget *focus;
  HACCEL hAccel;
  guint modal:1;
  guint allow_shrink:1;
  guint allow_grow:1;
  guint auto_shrink:1;
};

typedef struct _GtkTable GtkTable;
typedef struct _GtkTableChild GtkTableChild;
typedef struct _GtkTableRowCol GtkTableRowCol;

struct _GtkTable {
  GtkContainer container;
  GList *children;
  GtkTableRowCol *rows, *cols;
  guint16 nrows, ncols;
  guint16 column_spacing, row_spacing;
  guint homogeneous:1;
};

struct _GtkTableChild {
  GtkWidget *widget;
  guint16 left_attach, right_attach, top_attach, bottom_attach;
  GtkAttachOptions xoptions, yoptions;
};

struct _GtkTableRowCol {
  guint16 requisition;
  guint16 allocation;
  gint16 spacing;
};

extern GtkClass GtkContainerClass;
extern HFONT defFont;
extern HINSTANCE hInst;

#define G_OBJECT(obj) ((GObject *)(obj))
#define GTK_CONTAINER(obj) ((GtkContainer *)(obj))
#define GTK_PANED(obj) ((GtkPaned *)(obj))
#define GTK_VPANED(obj) ((GtkVPaned *)(obj))
#define GTK_HPANED(obj) ((GtkHPaned *)(obj))
#define GTK_BIN(obj) ((GtkBin *)(obj))
#define GTK_FRAME(obj) ((GtkFrame *)(obj))
#define GTK_BOX(obj) ((GtkBox *)(obj))
#define GTK_HBOX(obj) ((GtkHBox *)(obj))
#define GTK_VBOX(obj) ((GtkVBox *)(obj))
#define GTK_NOTEBOOK(obj) ((GtkNotebook *)(obj))
#define GTK_WIDGET(obj) ((GtkWidget *)(obj))
#define GTK_EDITABLE(obj) ((GtkEditable *)(obj))
#define GTK_ENTRY(obj) ((GtkEntry *)(obj))
#define GTK_SPIN_BUTTON(obj) ((GtkSpinButton *)(obj))
#define GTK_TEXT(obj) ((GtkText *)(obj))
#define GTK_WINDOW(obj) ((GtkWindow *)(obj))
#define GTK_BUTTON(obj) ((GtkButton *)(obj))
#define GTK_COMBO_BOX(obj) ((GtkComboBox *)(obj))
#define GTK_CELL_LAYOUT(obj) ((GtkCellLayout *)(obj))
#define GTK_TOGGLE_BUTTON(obj) ((GtkToggleButton *)(obj))
#define GTK_RADIO_BUTTON(obj) ((GtkRadioButton *)(obj))
#define GTK_CHECK_BUTTON(obj) ((GtkCheckButton *)(obj))
#define GTK_LABEL(obj) ((GtkLabel *)(obj))
#define GTK_URL(obj) ((GtkUrl *)(obj))
#define GTK_TABLE(obj) ((GtkTable *)(obj))
#define GTK_MENU_SHELL(obj) ((GtkMenuShell *)(obj))
#define GTK_MENU_BAR(obj) ((GtkMenuBar *)(obj))
#define GTK_MENU_ITEM(obj) ((GtkMenuItem *)(obj))
#define GTK_CHECK_MENU_ITEM(obj) ((GtkMenuItem *)(obj))
#define GTK_MENU(obj) ((GtkMenu *)(obj))
#define GTK_MISC(obj) ((GtkMisc *)(obj))
#define GTK_PROGRESS_BAR(obj) ((GtkProgressBar *)(obj))
#define G_CALLBACK(f) ((GCallback) (f))

#define GTK_OBJECT_FLAGS(obj) (G_OBJECT(obj)->flags)
#define GTK_WIDGET_FLAGS(wid) (GTK_OBJECT_FLAGS(wid))
#define GTK_WIDGET_REALIZED(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_REALIZED) != 0)
#define GTK_WIDGET_SENSITIVE(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_SENSITIVE) != 0)
#define GTK_WIDGET_CAN_FOCUS(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_CAN_FOCUS) != 0)
#define GTK_WIDGET_HAS_FOCUS(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_HAS_FOCUS) != 0)
#define GTK_WIDGET_SET_FLAGS(wid,flag) (GTK_WIDGET_FLAGS(wid) |= (flag))
#define GTK_WIDGET_UNSET_FLAGS(wid,flag) (GTK_WIDGET_FLAGS(wid) &= ~(flag))

void gtk_widget_set_can_default(GtkWidget *wid, gboolean flag);

typedef int GdkEvent;

gboolean gtk_widget_get_visible(GtkWidget *widget);
void gtk_widget_show(GtkWidget *widget);
void gtk_widget_show_all(GtkWidget *widget);
void gtk_widget_hide_all(GtkWidget *widget);
void gtk_widget_hide(GtkWidget *widget);
void gtk_widget_destroy(GtkWidget *widget);
void gtk_widget_realize(GtkWidget *widget);
void gtk_widget_set_sensitive(GtkWidget *widget, gboolean sensitive);
void gtk_widget_size_request(GtkWidget *widget,
                             GtkRequisition *requisition);
void gtk_widget_set_size(GtkWidget *widget, GtkAllocation *allocation);
GtkWidget *gtk_widget_get_ancestor(GtkWidget *widget, GtkType type);
GtkWidget *gtk_window_new(GtkWindowType type);
void gtk_window_set_title(GtkWindow *window, const gchar *title);
void gtk_window_set_default_size(GtkWindow *window, gint width,
                                 gint height);
void gtk_window_set_transient_for(GtkWindow *window, GtkWindow *parent);
void gtk_window_set_policy(GtkWindow *window, gint allow_shrink,
                           gint allow_grow, gint auto_shrink);
void gtk_container_add(GtkContainer *container, GtkWidget *widget);
void gtk_container_set_border_width(GtkContainer *container,
                                    guint border_width);
GtkWidget *gtk_button_new_with_label(const gchar *label);
GtkWidget *gtk_label_new(const gchar *text);
GtkWidget *gtk_box_new(GtkOrientation orientation, gint spacing);
void gtk_box_set_homogeneous(GtkBox *box, gboolean homogeneous);
GtkWidget *gtk_check_button_new_with_label(const gchar *label);
GtkWidget *gtk_radio_button_new_with_label(GSList *group,
                                           const gchar *label);
GtkWidget *gtk_radio_button_new_with_label_from_widget(GtkRadioButton *group,
                                                       const gchar *label);
GtkWidget *gtk_frame_new(const gchar *text);
void gtk_frame_set_shadow_type(GtkFrame *frame, GtkShadowType type);
GtkWidget *gtk_text_new(GtkAdjustment *hadj, GtkAdjustment *vadj);
GtkWidget *gtk_entry_new();
void gtk_entry_set_visibility(GtkEntry *entry, gboolean visible);
GtkWidget *gtk_table_new(guint rows, guint cols, gboolean homogeneous);
void gtk_table_resize(GtkTable *table, guint rows, guint cols);
GSList *gtk_radio_button_get_group(GtkRadioButton *radio_button);
void gtk_editable_insert_text(GtkEditable *editable, const gchar *new_text,
                              gint new_text_length, gint *position);
void gtk_editable_delete_text(GtkEditable *editable,
                              gint start_pos, gint end_pos);
gchar *gtk_editable_get_chars(GtkEditable *editable,
                              gint start_pos, gint end_pos);
void gtk_editable_set_editable(GtkEditable *editable,
                               gboolean is_editable);
void gtk_editable_set_position(GtkEditable *editable, gint position);
gint gtk_editable_get_position(GtkEditable *editable);
guint gtk_text_get_length(GtkText *text);
void gtk_text_set_editable(GtkText *text, gboolean is_editable);
void gtk_text_set_word_wrap(GtkText *text, gboolean word_wrap);
void gtk_text_freeze(GtkText *text);
void gtk_text_thaw(GtkText *text);
GtkTextBuffer *gtk_text_view_get_buffer(GtkText *text);
void gtk_text_buffer_create_tag(GtkTextBuffer *buffer, const gchar *name, ...);
void gtk_table_attach(GtkTable *table, GtkWidget *widget,
                      guint left_attach, guint right_attach,
                      guint top_attach, guint bottom_attach,
                      GtkAttachOptions xoptions, GtkAttachOptions yoptions,
                      guint xpadding, guint ypadding);
void gtk_table_attach_defaults(GtkTable *table, GtkWidget *widget,
                               guint left_attach, guint right_attach,
                               guint top_attach, guint bottom_attach);
void gtk_table_set_row_spacing(GtkTable *table, guint row, guint spacing);
void gtk_table_set_col_spacing(GtkTable *table, guint column,
                               guint spacing);
void gtk_table_set_row_spacings(GtkTable *table, guint spacing);
void gtk_table_set_col_spacings(GtkTable *table, guint spacing);
void gtk_box_pack_start(GtkBox *box, GtkWidget *child, gboolean Expand,
                        gboolean Fill, gint Padding);
void gtk_toggle_button_toggled(GtkToggleButton *toggle_button);
gboolean gtk_toggle_button_get_active(GtkToggleButton *toggle_button);
void gtk_toggle_button_set_active(GtkToggleButton *toggle_button,
                                  gboolean is_active);
void gtk_main_quit();
void gtk_main();
guint g_signal_connect(GObject *object, const gchar *name,
                       GCallback func, gpointer func_data);
guint g_signal_connect_swapped(GObject *object, const gchar *name,
                               GCallback func,
                               GObject *slot_object);
void gtk_signal_emit(GObject *object, const gchar *name, ...);
void SetCustomWndProc(WNDPROC wndproc);
void win32_init(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                char *MainIcon);
void gtk_menu_shell_insert(GtkMenuShell *menu_shell, GtkWidget *child,
                           gint position);
void gtk_menu_shell_append(GtkMenuShell *menu_shell, GtkWidget *child);
void gtk_menu_shell_prepend(GtkMenuShell *menu_shell, GtkWidget *child);
GtkWidget *gtk_menu_bar_new();
void gtk_menu_bar_insert(GtkMenuBar *menu_bar, GtkWidget *child,
                         gint position);
void gtk_menu_bar_append(GtkMenuBar *menu_bar, GtkWidget *child);
void gtk_menu_bar_prepend(GtkMenuBar *menu_bar, GtkWidget *child);
GtkWidget *gtk_menu_new();
void gtk_menu_insert(GtkMenu *menu, GtkWidget *child, gint position);
void gtk_menu_append(GtkMenu *menu, GtkWidget *child);
void gtk_menu_prepend(GtkMenu *menu, GtkWidget *child);
GtkWidget *gtk_menu_item_new_with_label(const gchar *label);
void gtk_menu_item_set_right_justified(GtkMenuItem *menu_item,
                                       gboolean right_justified);
#define gtk_menu_item_new_with_mnemonic gtk_menu_item_new_with_label
#define gtk_check_menu_item_new_with_mnemonic gtk_menu_item_new_with_label
GtkMenu *gtk_menu_item_get_submenu(GtkMenuItem *menu_item);
void gtk_menu_item_set_submenu(GtkMenuItem *menu_item, GtkWidget *submenu);
void gtk_check_menu_item_set_active(GtkMenuItem *menu_item, gboolean active);
gboolean gtk_check_menu_item_get_active(GtkMenuItem *menu_item);
void gtk_menu_set_active(GtkMenu *menu, guint index);
GtkWidget *gtk_notebook_new();
void gtk_notebook_append_page(GtkNotebook *notebook, GtkWidget *child,
                              GtkWidget *tab_label);
void gtk_notebook_insert_page(GtkNotebook *notebook, GtkWidget *child,
                              GtkWidget *tab_label, gint position);
void gtk_notebook_set_current_page(GtkNotebook *notebook, gint page_num);
gint gtk_notebook_get_current_page(GtkNotebook *notebook);
GObject *gtk_adjustment_new(gfloat value, gfloat lower, gfloat upper,
                            gfloat step_increment, gfloat page_increment,
                            gfloat page_size);
GtkWidget *gtk_spin_button_new(GtkAdjustment *adjustment,
                               gfloat climb_rate, guint digits);

guint dp_g_io_add_watch(GIOChannel *channel, GIOCondition condition,
                        GIOFunc func, gpointer user_data);
guint dp_g_timeout_add(guint interval, GSourceFunc function, gpointer data);
gboolean dp_g_source_remove(guint tag);

GtkWidget *gtk_separator_new(GtkOrientation orientation);
void g_object_set_data(GObject *object, const gchar *key,
                       gpointer data);
gpointer g_object_get_data(GObject *object, const gchar *key);
GtkAccelGroup *gtk_accel_group_new();
void gtk_accel_group_destroy(GtkAccelGroup *accel_group);
gint gtk_accel_group_add(GtkAccelGroup *accel_group,
                         ACCEL *newaccel);
void gtk_widget_grab_default(GtkWidget *widget);
void gtk_widget_grab_focus(GtkWidget *widget);
void gtk_window_set_modal(GtkWindow *window, gboolean modal);
void gtk_window_add_accel_group(GtkWindow *window,
                                GtkAccelGroup *accel_group);
void gtk_entry_set_text(GtkEntry *entry, const gchar *text);
void gtk_widget_add_accelerator(GtkWidget *widget,
                                const gchar *accel_signal,
                                GtkAccelGroup *accel_group,
                                guint accel_key, guint accel_mods,
                                GtkAccelFlags accel_flags);
void gtk_widget_remove_accelerator(GtkWidget *widget,
                                   GtkAccelGroup *accel_group,
                                   guint accel_key, guint accel_mods);
extern const GtkType GTK_TYPE_WINDOW, GTK_TYPE_MENU_BAR;
GtkWidget *gtk_vpaned_new();
GtkWidget *gtk_hpaned_new();
void gtk_paned_add1(GtkPaned *paned, GtkWidget *child);
void gtk_paned_add2(GtkPaned *paned, GtkWidget *child);
void gtk_paned_pack1(GtkPaned *paned, GtkWidget *child, gboolean resize,
                     gboolean shrink);
void gtk_paned_pack2(GtkPaned *paned, GtkWidget *child, gboolean resize,
                     gboolean shrink);
void gtk_paned_set_position(GtkPaned *paned, gint position);

#define gtk_container_border_width gtk_container_set_border_width
GtkWidget *gtk_button_box_new(GtkOrientation orientation);
void gtk_box_set_spacing(GtkBox *box, gint spacing);
#define gtk_button_box_set_layout(box, layout) {}
GtkWidget *gtk_combo_box_new_with_model(GtkTreeModel *model);
void gtk_combo_box_set_model(GtkComboBox *combo_box, GtkTreeModel *model);
void gtk_combo_box_set_active(GtkComboBox *combo_box, gint index);
void gtk_cell_layout_set_attributes(GtkCellLayout *cell_layout,
                                    GtkCellRenderer *cell, ...);
#define gtk_cell_layout_pack_start(layout, renderer, expand) {}
GtkTreeModel *gtk_combo_box_get_model(GtkComboBox *combo_box);
gboolean gtk_combo_box_get_active_iter(GtkComboBox *combo_box,
                                       GtkTreeIter *iter);
void gtk_label_set_text(GtkLabel *label, const gchar *str);
/* Not currently supported */
#define gtk_label_set_text_with_mnemonic gtk_label_set_text
#define gtk_label_set_mnemonic_widget(label, widget) {}
const gchar *gtk_label_get_text(GtkLabel *label);
void gtk_text_set_point(GtkText *text, guint index);
void gtk_widget_set_size_request(GtkWidget *widget, gint width, gint height);
gint gtk_spin_button_get_value_as_int(GtkSpinButton *spin_button);
void gtk_spin_button_set_value(GtkSpinButton *spin_button, gfloat value);
void gtk_spin_button_set_adjustment(GtkSpinButton *spin_button,
                                    GtkAdjustment *adjustment);
void gtk_spin_button_update(GtkSpinButton *spin_button);
void gtk_misc_set_alignment(GtkMisc *misc, gfloat xalign, gfloat yalign);
GtkWidget *gtk_progress_bar_new();
void gtk_progress_bar_set_fraction(GtkProgressBar *pbar, gfloat percentage);
guint gtk_main_level(void);
GObject *GtkNewObject(GtkClass *klass);
BOOL GetTextSize(HWND hWnd, char *text, LPSIZE lpSize, HFONT hFont);
void gtk_container_realize(GtkWidget *widget);
void gtk_set_default_font(HWND hWnd);
HWND gtk_get_parent_hwnd(GtkWidget *widget);
GtkStyle *gtk_style_new(void);
void gtk_widget_set_style(GtkWidget *widget, GtkStyle *style);
void gtk_window_set_type_hint(GtkWindow *window, GdkWindowTypeHint hint);
void gtk_window_set_position(GtkWindow *window, GtkWindowPosition position);

/* Functions for handling emitted signals */
void gtk_marshal_BOOL__GPOIN(GObject *object, GSList *actions,
                             GCallback default_action,
                             va_list args);
void gtk_marshal_BOOL__GINT(GObject *object, GSList *actions,
                            GCallback default_action,
                            va_list args);
void gtk_marshal_VOID__VOID(GObject *object, GSList *actions,
                            GCallback default_action,
                            va_list args);
void gtk_marshal_VOID__BOOL(GObject *object, GSList *actions,
                            GCallback default_action,
                            va_list args);
void gtk_marshal_VOID__GPOIN(GObject *object, GSList *actions,
                             GCallback default_action,
                             va_list args);
void gtk_marshal_VOID__GINT(GObject *object, GSList *actions,
                            GCallback default_action,
                            va_list args);
void gtk_marshal_VOID__GINT_GINT_EVENT(GObject *object, GSList *actions,
                                       GCallback default_action,
                                       va_list args);

/* Private functions */
void gtk_container_set_size(GtkWidget *widget, GtkAllocation *allocation);
void MapWidgetOrigin(GtkWidget *widget, POINT *pt);

#else /* CYGWIN */

/* Include standard GTK+ headers on Unix systems */

/* We'd like to do this under GTK+2, but we can't until we get rid of
   clists, gtk_object, etc.
   #define GTK_DISABLE_DEPRECATED 1
 */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* Provide compatibility functions for GTK+2 so we can use GTK+3 syntax */
#if GTK_MAJOR_VERSION == 2
GtkWidget *gtk_button_box_new(GtkOrientation orientation);
GtkWidget *gtk_box_new(GtkOrientation orientation, gint spacing);
GtkWidget *gtk_separator_new(GtkOrientation orientation);
#endif

/* Defines for GtkMessageBox options */
#define MB_OK     1
#define MB_CANCEL 2
#define MB_YES    4
#define MB_NO     8
#define MB_MAX    4
#define MB_YESNO  (MB_YES|MB_NO)

#define IDOK      GTK_RESPONSE_OK
#define IDCANCEL  GTK_RESPONSE_CANCEL
#define IDYES     GTK_RESPONSE_YES
#define IDNO      GTK_RESPONSE_NO

/* Other flags */
#define MB_IMMRETURN 16

typedef struct _GtkUrl GtkUrl;

struct _GtkUrl {
  GtkLabel *label;
  gchar *target, *bin;
};

#endif /* CYGWIN */

#if CYGWIN
extern const gchar *GTK_STOCK_OK, *GTK_STOCK_CLOSE, *GTK_STOCK_CANCEL, 
                   *GTK_STOCK_REFRESH, *GTK_STOCK_YES, *GTK_STOCK_NO,
                   *GTK_STOCK_HELP;

typedef enum
{
  GTK_MESSAGE_INFO,
  GTK_MESSAGE_WARNING,
  GTK_MESSAGE_QUESTION,
  GTK_MESSAGE_ERROR
} GtkMessageType;

#define gtk_text_view_set_editable(text, edit) gtk_text_set_editable(text, edit)
#define gtk_text_view_set_wrap_mode(text, wrap) gtk_text_set_word_wrap(text, wrap)
#define GTK_WRAP_WORD TRUE
#define GTK_TEXT_VIEW(wid) GTK_TEXT(wid)
#define GtkTextView GtkText
#endif

/* Global functions */
gint GtkMessageBox(GtkWidget *parent, const gchar *Text,
                   const gchar *Title, GtkMessageType type, gint Options);
GtkWidget *gtk_scrolled_clist_new_with_titles(gint columns,
                                              gchar *titles[],
                                              GtkWidget **pack_widg);
guint SetAccelerator(GtkWidget *labelparent, gchar *Text,
                     GtkWidget *sendto, gchar *signal,
                     GtkAccelGroup *accel_group, gboolean needalt);
GtkWidget *gtk_scrolled_text_view_new(GtkWidget **pack_widg);
void TextViewAppend(GtkTextView *textview, const gchar *text,
                    const gchar *tagname, gboolean scroll);
void TextViewClear(GtkTextView *textview);
GtkWidget *gtk_url_new(const gchar *text, const gchar *target,
                       const gchar *bin);
GtkWidget *NewStockButton(const gchar *label, GtkAccelGroup *accel_group);
gchar *GtkGetFile(const GtkWidget *parent, const gchar *oldname,
                  const gchar *title);
void DisplayHTML(GtkWidget *parent, const gchar *bin, const gchar *target);
gboolean HaveUnicodeSupport(void);
GtkWidget *gtk_scrolled_tree_view_new(GtkWidget **pack_widg);

#endif /* __GTKPORT_H__ */
