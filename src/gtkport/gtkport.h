/************************************************************************
 * gtkport.h      Portable "almost-GTK+" for Unix/Win32                 *
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

#ifndef __GTKPORT_H__
#define __GTKPORT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef CYGWIN

/* GTK+ emulation prototypes etc. for Win32 platform */

#include <windows.h>
#include <glib.h>
#include <stdarg.h>

#define MB_IMMRETURN 0

#define MYWM_SOCKETDATA (WM_USER+100)
#define MYWM_TASKBAR    (WM_USER+101)
#define MYWM_SERVICE    (WM_USER+102)

#define GDK_MOD1_MASK 0

extern HICON mainIcon;

typedef enum {
  GTK_WINDOW_TOPLEVEL, GTK_WINDOW_DIALOG, GTK_WINDOW_POPUP
} GtkWindowType;

typedef enum {
  GTK_ACCEL_VISIBLE        = 1 << 0,
  GTK_ACCEL_SIGNAL_VISIBLE = 1 << 1
} GtkAccelFlags;

typedef enum {
  GTK_BUTTONBOX_SPREAD,
  GTK_BUTTONBOX_EDGE,
  GTK_BUTTONBOX_START,
  GTK_BUTTONBOX_END
} GtkButtonBoxStyle;

typedef enum {
  GTK_STATE_NORMAL,
  GTK_STATE_ACTIVE,
  GTK_STATE_PRELIGHT,
  GTK_STATE_SELECTED,
  GTK_STATE_INSENSITIVE
} GtkStateType;

typedef enum {
  GTK_VISIBILITY_NONE,
  GTK_VISIBILITY_PARTIAL,
  GTK_VISIBILITY_FULL
} GtkVisibility;

typedef enum {
  GTK_PROGRESS_LEFT_TO_RIGHT,
  GTK_PROGRESS_RIGHT_TO_LEFT,
  GTK_PROGRESS_BOTTOM_TO_TOP,
  GTK_PROGRESS_TOP_TO_BOTTOM
} GtkProgressBarOrientation;

typedef enum {
  GTK_EXPAND = 1 << 0,
  GTK_SHRINK = 1 << 1,
  GTK_FILL   = 1 << 2
} GtkAttachOptions;

typedef enum {
  GTK_SELECTION_SINGLE,
  GTK_SELECTION_BROWSE,
  GTK_SELECTION_MULTIPLE,
  GTK_SELECTION_EXTENDED
} GtkSelectionMode;

typedef enum {
  GDK_INPUT_READ      = 1 << 0,
  GDK_INPUT_WRITE     = 1 << 1,
  GDK_INPUT_EXCEPTION = 1 << 2
} GdkInputCondition;

typedef enum
{
  GTK_SHADOW_NONE,
  GTK_SHADOW_IN,
  GTK_SHADOW_OUT,
  GTK_SHADOW_ETCHED_IN,
  GTK_SHADOW_ETCHED_OUT
} GtkShadowType;

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
typedef void (*GdkInputFunction) (gpointer data, gint source,
                                  GdkInputCondition condition);
typedef gchar *(*GtkTranslateFunc) (const gchar *path, gpointer func_data);
typedef void (*GtkDestroyNotify) (gpointer data);

typedef enum {
  GTK_REALIZED    = 1 << 6,
  GTK_VISIBLE     = 1 << 8,
  GTK_SENSITIVE   = 1 << 10,
  GTK_CAN_FOCUS   = 1 << 11,
  GTK_HAS_FOCUS   = 1 << 12,
  GTK_CAN_DEFAULT = 1 << 13,
  GTK_IS_DEFAULT  = 1 << 14
} GtkWidgetFlags;

#define GTK_VISIBLE 1

typedef struct _GtkClass GtkClass;
typedef struct _GtkObject GtkObject;

typedef struct _GtkRequisition GtkRequisition;
typedef struct _GtkAllocation GtkAllocation;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkSignalType GtkSignalType;
typedef struct _GtkContainer GtkContainer;

typedef void (*GtkSignalFunc) ();
typedef void (*GtkItemFactoryCallback) ();
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

#include "clist.h"

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

struct _GtkText {
  GtkEditable editable;
  gint word_wrap:1;
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
typedef struct _GtkItemFactoryEntry GtkItemFactoryEntry;
typedef struct _GtkItemFactory GtkItemFactory;

struct _GtkItemFactoryEntry {
  gchar *path;
  gchar *accelerator;
  GtkItemFactoryCallback callback;
  guint callback_action;
  gchar *item_type;
};

struct _GtkItemFactory {
  GtkObject object;
  GSList *children;
  gchar *path;
  GtkAccelGroup *accel_group;
  GtkWidget *top_widget;
  GtkTranslateFunc translate_func;
  gpointer translate_data;
};

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

struct _GtkOptionMenu {
  GtkButton button;
  GtkWidget *menu;
  guint selection;
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

#define GTK_OBJECT(obj) ((GtkObject *)(obj))
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
#define GTK_OPTION_MENU(obj) ((GtkOptionMenu *)(obj))
#define GTK_TOGGLE_BUTTON(obj) ((GtkToggleButton *)(obj))
#define GTK_RADIO_BUTTON(obj) ((GtkRadioButton *)(obj))
#define GTK_CHECK_BUTTON(obj) ((GtkCheckButton *)(obj))
#define GTK_LABEL(obj) ((GtkLabel *)(obj))
#define GTK_URL(obj) ((GtkUrl *)(obj))
#define GTK_TABLE(obj) ((GtkTable *)(obj))
#define GTK_MENU_SHELL(obj) ((GtkMenuShell *)(obj))
#define GTK_MENU_BAR(obj) ((GtkMenuBar *)(obj))
#define GTK_MENU_ITEM(obj) ((GtkMenuItem *)(obj))
#define GTK_MENU(obj) ((GtkMenu *)(obj))
#define GTK_MISC(obj) ((GtkMisc *)(obj))
#define GTK_PROGRESS_BAR(obj) ((GtkProgressBar *)(obj))
#define GTK_SIGNAL_FUNC(f) ((GtkSignalFunc) f)

#define GTK_OBJECT_FLAGS(obj) (GTK_OBJECT(obj)->flags)
#define GTK_WIDGET_FLAGS(wid) (GTK_OBJECT_FLAGS(wid))
#define GTK_WIDGET_REALIZED(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_REALIZED) != 0)
#define GTK_WIDGET_VISIBLE(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_VISIBLE) != 0)
#define GTK_WIDGET_SENSITIVE(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_SENSITIVE) != 0)
#define GTK_WIDGET_CAN_FOCUS(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_CAN_FOCUS) != 0)
#define GTK_WIDGET_HAS_FOCUS(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_HAS_FOCUS) != 0)
#define GTK_WIDGET_SET_FLAGS(wid,flag) (GTK_WIDGET_FLAGS(wid) |= (flag))
#define GTK_WIDGET_UNSET_FLAGS(wid,flag) (GTK_WIDGET_FLAGS(wid) &= ~(flag))

typedef int GdkEvent;

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
GtkWidget *gtk_hbox_new(gboolean homogeneous, gint spacing);
GtkWidget *gtk_vbox_new(gboolean homogeneous, gint spacing);
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
GtkItemFactory *gtk_item_factory_new(GtkType container_type,
                                     const gchar *path,
                                     GtkAccelGroup *accel_group);
void gtk_item_factory_create_item(GtkItemFactory *ifactory,
                                  GtkItemFactoryEntry *entry,
                                  gpointer callback_data,
                                  guint callback_type);
void gtk_item_factory_create_items(GtkItemFactory *ifactory,
                                   guint n_entries,
                                   GtkItemFactoryEntry *entries,
                                   gpointer callback_data);
GtkWidget *gtk_item_factory_get_widget(GtkItemFactory *ifactory,
                                       const gchar *path);
GSList *gtk_radio_button_group(GtkRadioButton *radio_button);
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
void gtk_box_pack_start_defaults(GtkBox *box, GtkWidget *child);
void gtk_toggle_button_toggled(GtkToggleButton *toggle_button);
gboolean gtk_toggle_button_get_active(GtkToggleButton *toggle_button);
void gtk_toggle_button_set_active(GtkToggleButton *toggle_button,
                                  gboolean is_active);
void gtk_main_quit();
void gtk_main();
guint gtk_signal_connect(GtkObject *object, const gchar *name,
                         GtkSignalFunc func, gpointer func_data);
guint gtk_signal_connect_object(GtkObject *object, const gchar *name,
                                GtkSignalFunc func,
                                GtkObject *slot_object);
void gtk_signal_emit(GtkObject *object, const gchar *name, ...);
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
void gtk_menu_item_set_submenu(GtkMenuItem *menu_item, GtkWidget *submenu);
void gtk_menu_set_active(GtkMenu *menu, guint index);
GtkWidget *gtk_notebook_new();
void gtk_notebook_append_page(GtkNotebook *notebook, GtkWidget *child,
                              GtkWidget *tab_label);
void gtk_notebook_insert_page(GtkNotebook *notebook, GtkWidget *child,
                              GtkWidget *tab_label, gint position);
void gtk_notebook_set_page(GtkNotebook *notebook, gint page_num);
GtkObject *gtk_adjustment_new(gfloat value, gfloat lower, gfloat upper,
                              gfloat step_increment, gfloat page_increment,
                              gfloat page_size);
GtkWidget *gtk_spin_button_new(GtkAdjustment *adjustment,
                               gfloat climb_rate, guint digits);
void gdk_input_remove(gint tag);
gint gdk_input_add(gint source, GdkInputCondition condition,
                   GdkInputFunction function, gpointer data);
GtkWidget *gtk_hseparator_new();
GtkWidget *gtk_vseparator_new();
void gtk_object_set_data(GtkObject *object, const gchar *key,
                         gpointer data);
gpointer gtk_object_get_data(GtkObject *object, const gchar *key);
GtkAccelGroup *gtk_accel_group_new();
void gtk_accel_group_destroy(GtkAccelGroup *accel_group);
void gtk_item_factory_set_translate_func(GtkItemFactory *ifactory,
                                         GtkTranslateFunc func,
                                         gpointer data,
                                         GtkDestroyNotify notify);
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
GtkWidget *gtk_hbutton_box_new();
void gtk_hbutton_box_set_spacing_default(gint spacing);
#define gtk_vbutton_box_new() gtk_vbox_new(TRUE, 5)
#define gtk_hbutton_box_set_layout_default(layout) {}
#define gtk_vbutton_box_set_spacing_default(spacing) {}
#define gtk_vbutton_box_set_layout_default(layout) {}
GtkWidget *gtk_option_menu_new(void);
GtkWidget *gtk_option_menu_get_menu(GtkOptionMenu *option_menu);
void gtk_option_menu_set_menu(GtkOptionMenu *option_menu, GtkWidget *menu);
void gtk_option_menu_set_history(GtkOptionMenu *option_menu, guint index);
void gtk_label_set_text(GtkLabel *label, const gchar *str);
guint gtk_label_parse_uline(GtkLabel *label, const gchar *str);
void gtk_label_get(GtkLabel *label, gchar **str);
void gtk_text_set_point(GtkText *text, guint index);
void gtk_widget_set_usize(GtkWidget *widget, gint width, gint height);
gint gtk_spin_button_get_value_as_int(GtkSpinButton *spin_button);
void gtk_spin_button_set_value(GtkSpinButton *spin_button, gfloat value);
void gtk_spin_button_set_adjustment(GtkSpinButton *spin_button,
                                    GtkAdjustment *adjustment);
void gtk_spin_button_update(GtkSpinButton *spin_button);
void gtk_misc_set_alignment(GtkMisc *misc, gfloat xalign, gfloat yalign);
GtkWidget *gtk_progress_bar_new();
void gtk_progress_bar_set_orientation(GtkProgressBar *pbar,
                                      GtkProgressBarOrientation orientation);
void gtk_progress_bar_update(GtkProgressBar *pbar, gfloat percentage);
guint gtk_timeout_add(guint32 interval, GtkFunction function,
                      gpointer data);
void gtk_timeout_remove(guint timeout_handler_id);
guint gtk_main_level(void);
GtkObject *GtkNewObject(GtkClass *klass);
BOOL GetTextSize(HWND hWnd, char *text, LPSIZE lpSize, HFONT hFont);
void gtk_container_realize(GtkWidget *widget);
void gtk_set_default_font(HWND hWnd);
HWND gtk_get_parent_hwnd(GtkWidget *widget);
GtkStyle *gtk_style_new(void);
void gtk_widget_set_style(GtkWidget *widget, GtkStyle *style);

/* Functions for handling emitted signals */
void gtk_marshal_BOOL__GPOIN(GtkObject *object, GSList *actions,
                             GtkSignalFunc default_action,
                             va_list args);
void gtk_marshal_BOOL__GINT(GtkObject *object, GSList *actions,
                            GtkSignalFunc default_action,
                            va_list args);
void gtk_marshal_VOID__VOID(GtkObject *object, GSList *actions,
                            GtkSignalFunc default_action,
                            va_list args);
void gtk_marshal_VOID__BOOL(GtkObject *object, GSList *actions,
                            GtkSignalFunc default_action,
                            va_list args);
void gtk_marshal_VOID__GPOIN(GtkObject *object, GSList *actions,
                             GtkSignalFunc default_action,
                             va_list args);
void gtk_marshal_VOID__GINT(GtkObject *object, GSList *actions,
                            GtkSignalFunc default_action,
                            va_list args);
void gtk_marshal_VOID__GINT_GINT_EVENT(GtkObject *object, GSList *actions,
                                       GtkSignalFunc default_action,
                                       va_list args);

/* Private functions */
void gtk_container_set_size(GtkWidget *widget, GtkAllocation *allocation);

#else /* CYGWIN */

/* Include standard GTK+ headers on Unix systems */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* Defines for GtkMessageBox options */
#define MB_OK     1
#define MB_CANCEL 2
#define MB_YES    4
#define MB_NO     8
#define MB_MAX    4
#define MB_YESNO  (MB_YES|MB_NO)

#ifdef HAVE_GLIB2
#define IDOK      GTK_RESPONSE_OK
#define IDCANCEL  GTK_RESPONSE_CANCEL
#define IDYES     GTK_RESPONSE_YES
#define IDNO      GTK_RESPONSE_NO
#else
#define IDOK      1
#define IDCANCEL  2
#define IDYES     4
#define IDNO      8
#endif

/* Other flags */
#define MB_IMMRETURN 16

typedef struct _GtkUrl GtkUrl;

struct _GtkUrl {
  GtkLabel *label;
  gchar *target, *bin;
};

#endif /* CYGWIN */

#if CYGWIN || !HAVE_GLIB2
extern const gchar *GTK_STOCK_OK, *GTK_STOCK_CLOSE, *GTK_STOCK_CANCEL, 
                   *GTK_STOCK_REFRESH, *GTK_STOCK_YES, *GTK_STOCK_NO;

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

#endif /* __GTKPORT_H__ */
