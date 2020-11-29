/************************************************************************
 * gtkport.c      Portable "almost-GTK+" for Unix/Win32                 *
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

#ifndef CYGWIN
#include <sys/types.h>          /* For pid_t (fork) */
#include <sys/wait.h>           /* For wait */
#ifdef HAVE_UNISTD_H
#include <unistd.h>             /* For fork and execv */
#endif
#endif /* !CYGWIN */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <glib.h>

#include "gtkport.h"
#include "nls.h"

#ifdef APPLE
#include "mac_helpers.h"
#endif

#if CYGWIN
#include "unicodewrap.h"
const gchar *GTK_STOCK_OK = N_("_OK");
const gchar *GTK_STOCK_CLOSE = N_("_Close");
const gchar *GTK_STOCK_CANCEL = N_("_Cancel");
const gchar *GTK_STOCK_REFRESH = N_("_Refresh");
const gchar *GTK_STOCK_YES = N_("_Yes");
const gchar *GTK_STOCK_NO = N_("_No");
const gchar *GTK_STOCK_HELP = N_("_Help");
#endif

#ifdef CYGWIN

#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <uxtheme.h>
#include <shlwapi.h>

#define LISTITEMVPACK  0

#define PANED_STARTPOS 200

HICON mainIcon = NULL;
static WNDPROC customWndProc = NULL;
static gboolean HaveRichEdit = FALSE;
static gchar *RichEditClass = NULL;
static gboolean HaveXPControls = FALSE;

static guint RecurseLevel = 0;

static const gchar *WC_GTKSEP    = "WC_GTKSEP";
static const gchar *WC_GTKVPANED = "WC_GTKVPANED";
static const gchar *WC_GTKHPANED = "WC_GTKHPANED";
static const gchar *WC_GTKDIALOG = "WC_GTKDIALOG";
static const gchar *WC_GTKURL    = "WC_GTKURL";

static void gtk_button_size_request(GtkWidget *widget,
                                    GtkRequisition *requisition);
static void gtk_entry_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition);
static void gtk_entry_set_size(GtkWidget *widget,
                               GtkAllocation *allocation);
static void gtk_text_size_request(GtkWidget *widget,
                                  GtkRequisition *requisition);
static void gtk_text_destroy(GtkWidget *widget);
static void gtk_button_destroy(GtkWidget *widget);
static void gtk_check_button_size_request(GtkWidget *widget,
                                          GtkRequisition *requisition);
static void gtk_check_button_toggled(GtkCheckButton *check_button,
                                     gpointer data);
static void gtk_radio_button_clicked(GtkRadioButton *radio_button,
                                     gpointer data);
static void gtk_radio_button_toggled(GtkRadioButton *radio_button,
                                     gpointer data);
static void gtk_container_destroy(GtkWidget *widget);
static void gtk_container_size_request(GtkWidget *widget,
                                       GtkRequisition *requisition);
static void gtk_container_show_all(GtkWidget *widget, gboolean hWndOnly);
static void gtk_window_size_request(GtkWidget *widget,
                                    GtkRequisition *requisition);
static void gtk_window_set_size(GtkWidget *widget,
                                GtkAllocation *allocation);
static void gtk_window_destroy(GtkWidget *widget);
static void gtk_window_set_menu(GtkWindow *window, GtkMenuBar *menu_bar);
static GtkWidget *gtk_window_get_menu_ID(GtkWindow *window, gint ID);
static gboolean gtk_window_wndproc(GtkWidget *widget, UINT msg, WPARAM wParam,
                                   LPARAM lParam, gboolean *dodef);
static void gtk_table_destroy(GtkWidget *widget);
static void gtk_table_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition);
static void gtk_table_set_size(GtkWidget *widget,
                               GtkAllocation *allocation);
static void gtk_table_realize(GtkWidget *widget);
static void gtk_box_destroy(GtkWidget *widget);
static void gtk_hbox_size_request(GtkWidget *widget,
                                  GtkRequisition *requisition);
static void gtk_hbox_set_size(GtkWidget *widget,
                              GtkAllocation *allocation);
static void gtk_vbox_size_request(GtkWidget *widget,
                                  GtkRequisition *requisition);
static void gtk_vbox_set_size(GtkWidget *widget,
                              GtkAllocation *allocation);
static gint gtk_window_delete_event(GtkWidget *widget, GdkEvent *event);
static void gtk_window_realize(GtkWidget *widget);
static void gtk_window_show(GtkWidget *widget);
static void gtk_window_hide(GtkWidget *widget);
static void gtk_button_realize(GtkWidget *widget);
static void gtk_entry_realize(GtkWidget *widget);
static void gtk_text_realize(GtkWidget *widget);
static void gtk_check_button_realize(GtkWidget *widget);
static void gtk_radio_button_realize(GtkWidget *widget);
static void gtk_radio_button_destroy(GtkWidget *widget);
static void gtk_box_realize(GtkWidget *widget);

static void gtk_label_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition);
static void gtk_label_set_size(GtkWidget *widget,
                               GtkAllocation *allocation);
static void gtk_url_size_request(GtkWidget *widget,
                                 GtkRequisition *requisition);
static void gtk_label_destroy(GtkWidget *widget);
static void gtk_url_destroy(GtkWidget *widget);
static void gtk_label_realize(GtkWidget *widget);
static void gtk_url_realize(GtkWidget *widget);
static void gtk_frame_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition);
static void gtk_frame_set_size(GtkWidget *widget,
                               GtkAllocation *allocation);
static void gtk_frame_destroy(GtkWidget *widget);
static void gtk_frame_realize(GtkWidget *widget);
static void gtk_box_show_all(GtkWidget *widget, gboolean hWndOnly);
static void gtk_table_show_all(GtkWidget *widget, gboolean hWndOnly);
static void gtk_widget_show_all_full(GtkWidget *widget, gboolean hWndOnly);
static void gtk_widget_show_full(GtkWidget *widget, gboolean recurse);
static void gtk_widget_update(GtkWidget *widget, gboolean ForceResize);
static void gtk_container_hide_all(GtkWidget *widget, gboolean hWndOnly);
static void gtk_box_hide_all(GtkWidget *widget, gboolean hWndOnly);
static void gtk_table_hide_all(GtkWidget *widget, gboolean hWndOnly);
static void gtk_widget_hide_all_full(GtkWidget *widget, gboolean hWndOnly);
static void gtk_widget_hide_full(GtkWidget *widget, gboolean recurse);

static void gtk_menu_bar_realize(GtkWidget *widget);
static void gtk_menu_item_realize(GtkWidget *widget);
static void gtk_menu_item_enable(GtkWidget *widget);
static void gtk_menu_item_disable(GtkWidget *widget);
static void gtk_menu_realize(GtkWidget *widget);
static void gtk_menu_shell_realize(GtkWidget *widget);
static GtkWidget *gtk_menu_shell_get_menu_ID(GtkMenuShell *menu_shell,
                                             gint ID);
static void gtk_widget_create(GtkWidget *widget);
static void gtk_notebook_realize(GtkWidget *widget);
static void gtk_notebook_destroy(GtkWidget *widget);
static void gtk_notebook_set_size(GtkWidget *widget,
                                  GtkAllocation *allocation);
static void gtk_notebook_size_request(GtkWidget *widget,
                                      GtkRequisition *requisition);
static void gtk_notebook_show_all(GtkWidget *widget, gboolean hWndOnly);
static void gtk_notebook_hide_all(GtkWidget *widget, gboolean hWndOnly);
static void gtk_spin_button_size_request(GtkWidget *widget,
                                         GtkRequisition *requisition);
static void gtk_spin_button_set_size(GtkWidget *widget,
                                     GtkAllocation *allocation);
static void gtk_spin_button_realize(GtkWidget *widget);
static void gtk_spin_button_destroy(GtkWidget *widget);
static void gtk_spin_button_show(GtkWidget *widget);
static void gtk_spin_button_hide(GtkWidget *widget);
static void gtk_separator_size_request(GtkWidget *widget,
                                       GtkRequisition *requisition);
static void gtk_separator_realize(GtkWidget *widget);
static void gtk_paned_show_all(GtkWidget *widget, gboolean hWndOnly);
static void gtk_paned_hide_all(GtkWidget *widget, gboolean hWndOnly);
static void gtk_paned_realize(GtkWidget *widget);
static void gtk_vpaned_realize(GtkWidget *widget);
static void gtk_hpaned_realize(GtkWidget *widget);
static void gtk_vpaned_size_request(GtkWidget *widget,
                                    GtkRequisition *requisition);
static void gtk_hpaned_size_request(GtkWidget *widget,
                                    GtkRequisition *requisition);
static void gtk_vpaned_set_size(GtkWidget *widget,
                                GtkAllocation *allocation);
static void gtk_hpaned_set_size(GtkWidget *widget,
                                GtkAllocation *allocation);
static void gtk_combo_box_size_request(GtkWidget *widget,
                                       GtkRequisition *requisition);
static void gtk_combo_box_set_size(GtkWidget *widget,
                                   GtkAllocation *allocation);
static void gtk_combo_box_realize(GtkWidget *widget);
static void gtk_combo_box_destroy(GtkWidget *widget);
static void gtk_button_set_text(GtkButton *button, gchar *text);
static void gtk_menu_item_set_text(GtkMenuItem *menuitem, gchar *text);
static void gtk_editable_create(GtkWidget *widget);
static void gtk_combo_box_update_selection(GtkWidget *widget);
static void gtk_widget_set_focus(GtkWidget *widget);
static void gtk_widget_lose_focus(GtkWidget *widget);
static void gtk_window_update_focus(GtkWindow *window);
static void gtk_window_set_focus(GtkWindow *window);
static void gtk_window_handle_user_size(GtkWindow *window,
                                        GtkAllocation *allocation);
static void gtk_window_handle_minmax_size(GtkWindow *window,
                                          LPMINMAXINFO minmax);
static void gtk_window_handle_auto_size(GtkWindow *window,
                                        GtkAllocation *allocation);
static void gtk_window_set_initial_position(GtkWindow *window,
                                            GtkAllocation *allocation);
static void gtk_progress_bar_size_request(GtkWidget *widget,
                                          GtkRequisition *requisition);
static void gtk_progress_bar_realize(GtkWidget *widget);
static void gtk_accel_group_set_id(GtkAccelGroup *accel_group, gint ind,
                                   gint ID);
static void EnableParent(GtkWindow *window);

struct _OurSource {
  guint id;     /* Unique identifier */

  /* Used for timeouts (dp_g_timeout_add()) */
  GSourceFunc timeout_function;

  /* Used for IO channels (dp_g_io_watch()) */
  GIOChannel *io_channel;
  GIOFunc io_function;
  int socket;

  gpointer data;    /* Data passed to callback function */
};
typedef struct _OurSource OurSource;

static GtkSignalType GtkObjectSignals[] = {
  {"create", gtk_marshal_VOID__VOID, NULL},
  {"", NULL, NULL}
};

static GtkClass GtkObjectClass = {
  "object", NULL, sizeof(GObject), GtkObjectSignals, NULL
};

static GtkClass GtkAdjustmentClass = {
  "adjustment", &GtkObjectClass, sizeof(GtkAdjustment), NULL, NULL
};

static GtkSignalType GtkWidgetSignals[] = {
  {"create", gtk_marshal_VOID__VOID, gtk_widget_create},
  {"size_request", gtk_marshal_VOID__GPOIN, NULL},
  {"set_size", gtk_marshal_VOID__GPOIN, NULL},
  {"realize", gtk_marshal_VOID__VOID, NULL},
  {"destroy", gtk_marshal_VOID__VOID, NULL},
  {"show", gtk_marshal_VOID__VOID, NULL},
  {"hide", gtk_marshal_VOID__VOID, NULL},
  {"show_all", gtk_marshal_VOID__BOOL, NULL},
  {"hide_all", gtk_marshal_VOID__BOOL, NULL},
  {"enable", gtk_marshal_VOID__VOID, NULL},
  {"disable", gtk_marshal_VOID__VOID, NULL},
  {"", NULL, NULL}
};

static GtkClass GtkWidgetClass = {
  "widget", &GtkObjectClass, sizeof(GtkWidget), GtkWidgetSignals, NULL
};

static GtkSignalType GtkSeparatorSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_separator_size_request},
  {"realize", gtk_marshal_VOID__VOID, gtk_separator_realize},
  {"", NULL, NULL}
};

static GtkSignalType GtkProgressBarSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_progress_bar_size_request},
  {"realize", gtk_marshal_VOID__VOID, gtk_progress_bar_realize},
  {"", NULL, NULL}
};

static GtkClass GtkProgressBarClass = {
  "progressbar", &GtkWidgetClass, sizeof(GtkProgressBar),
      GtkProgressBarSignals, NULL
};

static GtkClass GtkSeparatorClass = {
  "separator", &GtkWidgetClass, sizeof(GtkSeparator), GtkSeparatorSignals, NULL
};

static GtkClass GtkHSeparatorClass = {
  "hseparator", &GtkSeparatorClass, sizeof(GtkHSeparator), NULL, NULL
};

static GtkClass GtkVSeparatorClass = {
  "vseparator", &GtkSeparatorClass, sizeof(GtkVSeparator), NULL, NULL
};

static GtkClass GtkMenuShellClass = {
  "menushell", &GtkWidgetClass, sizeof(GtkMenuShell), NULL, NULL
};

static GtkSignalType GtkMenuBarSignals[] = {
  {"realize", gtk_marshal_VOID__VOID, gtk_menu_bar_realize},
  {"", NULL, NULL}
};

static GtkClass GtkMenuBarClass = {
  "menubar", &GtkMenuShellClass, sizeof(GtkMenuBar), GtkMenuBarSignals, NULL
};

static GtkSignalType GtkMenuItemSignals[] = {
  {"realize", gtk_marshal_VOID__VOID, gtk_menu_item_realize},
  {"set_text", gtk_marshal_VOID__GPOIN, gtk_menu_item_set_text},
  {"activate", gtk_marshal_VOID__VOID, NULL},
  {"enable", gtk_marshal_VOID__VOID, gtk_menu_item_enable},
  {"disable", gtk_marshal_VOID__VOID, gtk_menu_item_disable},
  {"", NULL, NULL}
};

static GtkClass GtkMenuItemClass = {
  "menuitem", &GtkWidgetClass, sizeof(GtkMenuItem), GtkMenuItemSignals, NULL
};

static GtkSignalType GtkMenuSignals[] = {
  {"realize", gtk_marshal_VOID__VOID, gtk_menu_realize},
  {"", NULL, NULL}
};

static GtkClass GtkMenuClass = {
  "menu", &GtkMenuShellClass, sizeof(GtkMenu), GtkMenuSignals, NULL
};

static GtkSignalType GtkEditableSignals[] = {
  {"create", gtk_marshal_VOID__VOID, gtk_editable_create},
  {"activate", gtk_marshal_VOID__VOID, NULL},
  {"", NULL, NULL}
};

static GtkClass GtkEditableClass = {
  "editable", &GtkWidgetClass, sizeof(GtkEditable), GtkEditableSignals, NULL
};

static GtkSignalType GtkEntrySignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_entry_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_entry_set_size},
  {"realize", gtk_marshal_VOID__VOID, gtk_entry_realize},
  {"", NULL, NULL}
};

static GtkClass GtkEntryClass = {
  "entry", &GtkEditableClass, sizeof(GtkEntry), GtkEntrySignals, NULL
};

static GtkSignalType GtkSpinButtonSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_spin_button_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_spin_button_set_size},
  {"realize", gtk_marshal_VOID__VOID, gtk_spin_button_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_spin_button_destroy},
  {"hide", gtk_marshal_VOID__VOID, gtk_spin_button_hide},
  {"show", gtk_marshal_VOID__VOID, gtk_spin_button_show},
  {"", NULL, NULL}
};

static GtkClass GtkSpinButtonClass = {
  "spinbutton", &GtkEntryClass, sizeof(GtkSpinButton),
  GtkSpinButtonSignals, NULL
};

static GtkSignalType GtkTextSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_text_size_request},
  {"realize", gtk_marshal_VOID__VOID, gtk_text_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_text_destroy},
  {"", NULL, NULL}
};

static GtkClass GtkTextClass = {
  "text", &GtkEditableClass, sizeof(GtkText), GtkTextSignals, NULL
};

static GtkSignalType GtkLabelSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_label_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_label_set_size},
  {"set_text", gtk_marshal_VOID__GPOIN, gtk_label_set_text},
  {"realize", gtk_marshal_VOID__VOID, gtk_label_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_label_destroy},
  {"", NULL, NULL}
};

static GtkClass GtkLabelClass = {
  "label", &GtkWidgetClass, sizeof(GtkLabel), GtkLabelSignals, NULL
};

static GtkSignalType GtkUrlSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_url_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_label_set_size},
  {"realize", gtk_marshal_VOID__VOID, gtk_url_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_url_destroy},
  {"", NULL, NULL}
};

static GtkClass GtkUrlClass = {
  "URL", &GtkLabelClass, sizeof(GtkUrl), GtkUrlSignals, NULL
};

static GtkSignalType GtkButtonSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_button_size_request},
  {"set_text", gtk_marshal_VOID__GPOIN, gtk_button_set_text},
  {"realize", gtk_marshal_VOID__VOID, gtk_button_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_button_destroy},
  {"clicked", gtk_marshal_VOID__VOID, NULL},
  {"", NULL, NULL}
};

static GtkClass GtkButtonClass = {
  "button", &GtkWidgetClass, sizeof(GtkButton), GtkButtonSignals, NULL
};

static GtkSignalType GtkComboBoxSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_combo_box_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_combo_box_set_size},
  {"realize", gtk_marshal_VOID__VOID, gtk_combo_box_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_combo_box_destroy},
  {"changed", gtk_marshal_VOID__GPOIN, NULL},
  {"", NULL, NULL}
};

static GtkClass GtkComboBoxClass = {
  "combobox", &GtkWidgetClass, sizeof(GtkComboBox),
  GtkComboBoxSignals, NULL
};

static GtkSignalType GtkToggleButtonSignals[] = {
  {"toggled", gtk_marshal_VOID__VOID, NULL},
  {"", NULL, NULL}
};

static GtkClass GtkToggleButtonClass = {
  "toggle", &GtkButtonClass, sizeof(GtkToggleButton),
  GtkToggleButtonSignals, NULL
};

static GtkSignalType GtkCheckButtonSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_check_button_size_request},
  {"realize", gtk_marshal_VOID__VOID, gtk_check_button_realize},
  {"", NULL, NULL}
};

static GtkClass GtkCheckButtonClass = {
  "check", &GtkToggleButtonClass, sizeof(GtkCheckButton),
  GtkCheckButtonSignals, NULL
};

static GtkSignalType GtkRadioButtonSignals[] = {
  {"realize", gtk_marshal_VOID__VOID, gtk_radio_button_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_radio_button_destroy},
  {"", NULL, NULL}
};

static GtkClass GtkRadioButtonClass = {
  "radio", &GtkCheckButtonClass, sizeof(GtkRadioButton),
  GtkRadioButtonSignals, NULL
};

static GtkSignalType GtkContainerSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_container_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_container_set_size},
  {"realize", gtk_marshal_VOID__VOID, gtk_container_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_container_destroy},
  {"show_all", gtk_marshal_VOID__BOOL, gtk_container_show_all},
  {"hide_all", gtk_marshal_VOID__BOOL, gtk_container_hide_all},
  {"", NULL, NULL}
};

GtkClass GtkContainerClass = {
  "container", &GtkWidgetClass, sizeof(GtkContainer), GtkContainerSignals, NULL
};

static GtkSignalType GtkPanedSignals[] = {
  {"show_all", gtk_marshal_VOID__BOOL, gtk_paned_show_all},
  {"hide_all", gtk_marshal_VOID__BOOL, gtk_paned_hide_all},
  {"", NULL, NULL}
};

static GtkClass GtkPanedClass = {
  "paned", &GtkContainerClass, sizeof(GtkPaned), GtkPanedSignals, NULL
};

static GtkSignalType GtkVPanedSignals[] = {
  {"realize", gtk_marshal_VOID__VOID, gtk_vpaned_realize},
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_vpaned_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_vpaned_set_size},
  {"", NULL, NULL}
};

static GtkClass GtkVPanedClass = {
  "vpaned", &GtkPanedClass, sizeof(GtkVPaned), GtkVPanedSignals, NULL
};

static GtkSignalType GtkHPanedSignals[] = {
  {"realize", gtk_marshal_VOID__VOID, gtk_hpaned_realize},
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_hpaned_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_hpaned_set_size},
  {"", NULL, NULL}
};

static GtkClass GtkHPanedClass = {
  "hpaned", &GtkPanedClass, sizeof(GtkHPaned), GtkHPanedSignals, NULL
};

static GtkSignalType GtkBoxSignals[] = {
  {"realize", gtk_marshal_VOID__VOID, gtk_box_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_box_destroy},
  {"show_all", gtk_marshal_VOID__BOOL, gtk_box_show_all},
  {"hide_all", gtk_marshal_VOID__BOOL, gtk_box_hide_all},
  {"", NULL, NULL}
};

static GtkClass GtkBoxClass = {
  "box", &GtkContainerClass, sizeof(GtkBox), GtkBoxSignals, NULL
};

static GtkSignalType GtkNotebookSignals[] = {
  {"realize", gtk_marshal_VOID__VOID, gtk_notebook_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_notebook_destroy},
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_notebook_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_notebook_set_size},
  {"show_all", gtk_marshal_VOID__BOOL, gtk_notebook_show_all},
  {"hide_all", gtk_marshal_VOID__BOOL, gtk_notebook_hide_all},
  {"", NULL, NULL}
};

static GtkClass GtkNotebookClass = {
  "notebook", &GtkContainerClass, sizeof(GtkNotebook), GtkNotebookSignals, NULL
};

static GtkSignalType GtkTableSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_table_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_table_set_size},
  {"realize", gtk_marshal_VOID__VOID, gtk_table_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_table_destroy},
  {"show_all", gtk_marshal_VOID__BOOL, gtk_table_show_all},
  {"hide_all", gtk_marshal_VOID__BOOL, gtk_table_hide_all},
  {"", NULL, NULL}
};

static GtkClass GtkTableClass = {
  "table", &GtkContainerClass, sizeof(GtkTable), GtkTableSignals, NULL
};

static GtkSignalType GtkHBoxSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_hbox_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_hbox_set_size},
  {"", NULL, NULL}
};

static GtkClass GtkHBoxClass = {
  "hbox", &GtkBoxClass, sizeof(GtkHBox), GtkHBoxSignals, NULL
};

static GtkSignalType GtkVBoxSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_vbox_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_vbox_set_size},
  {"", NULL, NULL}
};

static GtkClass GtkVBoxClass = {
  "vbox", &GtkBoxClass, sizeof(GtkVBox), GtkVBoxSignals, NULL
};

static GtkClass GtkBinClass = {
  "bin", &GtkContainerClass, sizeof(GtkBin), NULL, NULL
};

static GtkSignalType GtkFrameSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_frame_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_frame_set_size},
  {"realize", gtk_marshal_VOID__VOID, gtk_frame_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_frame_destroy},
  {"", NULL, NULL}
};

static GtkClass GtkFrameClass = {
  "frame", &GtkBinClass, sizeof(GtkFrame), GtkFrameSignals, NULL
};

static GtkSignalType GtkWindowSignals[] = {
  {"size_request", gtk_marshal_VOID__GPOIN, gtk_window_size_request},
  {"set_size", gtk_marshal_VOID__GPOIN, gtk_window_set_size},
  {"realize", gtk_marshal_VOID__VOID, gtk_window_realize},
  {"destroy", gtk_marshal_VOID__VOID, gtk_window_destroy},
  {"show", gtk_marshal_VOID__VOID, gtk_window_show},
  {"hide", gtk_marshal_VOID__VOID, gtk_window_hide},
  {"delete_event", gtk_marshal_BOOL__GPOIN,
   G_CALLBACK(gtk_window_delete_event)},
  {"", NULL, NULL}
};

static GtkClass GtkWindowClass = {
  "window", &GtkBinClass, sizeof(GtkWindow), GtkWindowSignals,
  gtk_window_wndproc
};

const GtkType GTK_TYPE_WINDOW = &GtkWindowClass;
const GtkType GTK_TYPE_MENU_BAR = &GtkMenuBarClass;
const GtkType GTK_TYPE_NOTEBOOK = &GtkNotebookClass;

HINSTANCE hInst;
HFONT defFont;
static HFONT urlFont;
static GSList *WindowList = NULL;
static GSList *OurSources = NULL;
static HWND TopLevel = NULL;

static WNDPROC wpOrigEntryProc, wpOrigTextProc;

void gtk_set_default_font(HWND hWnd)
{
  mySendMessage(hWnd, WM_SETFONT, (WPARAM)defFont, MAKELPARAM(FALSE, 0));
}

GObject *GtkNewObject(GtkClass *klass)
{
  GObject *newObj;

  newObj = g_malloc0(klass->Size);
  newObj->klass = klass;
  gtk_signal_emit(newObj, "create");

  return newObj;
}

static void DispatchSocketEvent(SOCKET sock, long event)
{
  GSList *list;
  OurSource *s;

  for (list = OurSources; list; list = g_slist_next(list)) {
    s = (OurSource *)(list->data);
    if (s->socket == sock) {
      (*s->io_function) (s->io_channel,
                          (event & (FD_READ | FD_CLOSE | FD_ACCEPT) ?
                           G_IO_IN : 0) |
                          (event & (FD_WRITE | FD_CONNECT) ?
                           G_IO_OUT : 0), s->data);
      break;
    }
  }
}

static void DispatchTimeoutEvent(UINT id)
{
  GSList *list;
  OurSource *s;

  for (list = OurSources; list; list = g_slist_next(list)) {
    s = (OurSource *)list->data;
    if (s->id == id) {
      if (s->timeout_function) {
        if (!(*s->timeout_function) (s->data)) {
          dp_g_source_remove(s->id);
        }
      }
      break;
    }
  }
}

static HWND gtk_get_window_hwnd(GtkWidget *widget)
{
  widget = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
  if (widget) {
    return widget->hWnd;
  } else {
    return NULL;
  }
}

HWND gtk_get_parent_hwnd(GtkWidget *widget)
{
  GtkWidget *child = NULL;
  while (widget && G_OBJECT(widget)->klass != GTK_TYPE_WINDOW
         && G_OBJECT(widget)->klass != GTK_TYPE_NOTEBOOK) {
    child = widget;
    widget = widget->parent;
  }
  if (widget) {
    if (G_OBJECT(widget)->klass == GTK_TYPE_NOTEBOOK) {
      GSList *children;
      for (children = GTK_NOTEBOOK(widget)->children; children;
           children = g_slist_next(children)) {
        GtkNotebookChild *note_child;
        note_child = (GtkNotebookChild *)(children->data);
        if (note_child && note_child->child == child) {
          return note_child->tabpage;
        }
      }
      return NULL;
    } else {
      return widget->hWnd;
    }
  } else {
    return NULL;
  }
}

static void UpdatePanedGhostRect(GtkPaned *paned, RECT *OldRect,
                                 RECT *NewRect, gint16 x, gint16 y)
{
  HWND hWnd, parent;
  RECT rect, clrect;
  POINT MouseCoord;
  HDC hDC;
  GtkWidget *widget = GTK_WIDGET(paned);

  if (!OldRect && !NewRect)
    return;
  parent = gtk_get_parent_hwnd(widget);
  hWnd = widget->hWnd;
  if (!parent || !hWnd)
    return;

  MouseCoord.x = x;
  MouseCoord.y = y;
  MapWindowPoints(hWnd, parent, &MouseCoord, 1);

  rect.left = paned->true_alloc.x;
  rect.top = paned->true_alloc.y;
  GetClientRect(hWnd, &clrect);
  if (clrect.right > clrect.bottom) {
    rect.right = paned->true_alloc.x + paned->true_alloc.width;
    rect.bottom = MouseCoord.y;
  } else {
    rect.bottom = paned->true_alloc.y + paned->true_alloc.height;
    rect.right = MouseCoord.x;
  }

  if (OldRect && NewRect && OldRect->right == rect.right &&
      OldRect->bottom == rect.bottom)
    return;

  hDC = GetDC(parent);

  if (OldRect)
    DrawFocusRect(hDC, OldRect);
  if (NewRect) {
    CopyRect(NewRect, &rect);
    DrawFocusRect(hDC, NewRect);
  }
  ReleaseDC(parent, hDC);
}

LRESULT CALLBACK GtkPanedProc(HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam)
{
  PAINTSTRUCT ps;
  HPEN oldpen, dkpen, ltpen;
  RECT rect;
  static RECT GhostRect;
  HDC hDC;
  gint newpos;
  GtkPaned *paned;

  paned = GTK_PANED(myGetWindowLong(hwnd, GWLP_USERDATA));
  switch (msg) {
  case WM_PAINT:
    if (GetUpdateRect(hwnd, NULL, TRUE)) {
      BeginPaint(hwnd, &ps);
      GetClientRect(hwnd, &rect);
      hDC = ps.hdc;
      ltpen =
          CreatePen(PS_SOLID, 0, (COLORREF)GetSysColor(COLOR_3DHILIGHT));
      dkpen =
          CreatePen(PS_SOLID, 0, (COLORREF)GetSysColor(COLOR_3DSHADOW));

      if (rect.right > rect.bottom) {
        oldpen = SelectObject(hDC, ltpen);
        MoveToEx(hDC, rect.left, rect.top, NULL);
        LineTo(hDC, rect.right, rect.top);

        SelectObject(hDC, dkpen);
        MoveToEx(hDC, rect.left, rect.bottom - 1, NULL);
        LineTo(hDC, rect.right, rect.bottom - 1);
      } else {
        oldpen = SelectObject(hDC, ltpen);
        MoveToEx(hDC, rect.left, rect.top, NULL);
        LineTo(hDC, rect.left, rect.bottom);

        SelectObject(hDC, ltpen);
        MoveToEx(hDC, rect.right - 1, rect.top, NULL);
        LineTo(hDC, rect.right - 1, rect.bottom);
      }

      SelectObject(hDC, oldpen);
      DeleteObject(ltpen);
      DeleteObject(dkpen);
      EndPaint(hwnd, &ps);
    }
    return TRUE;
  case WM_LBUTTONDOWN:
    if (!paned)
      break;
    SetCapture(hwnd);
    paned->Tracking = TRUE;
    UpdatePanedGhostRect(paned, NULL, &GhostRect,
                         LOWORD(lParam), HIWORD(lParam));
    return TRUE;
  case WM_MOUSEMOVE:
    if (!paned || !paned->Tracking)
      break;
    UpdatePanedGhostRect(paned, &GhostRect, &GhostRect,
                         LOWORD(lParam), HIWORD(lParam));
    return TRUE;
  case WM_LBUTTONUP:
    if (!paned || !paned->Tracking)
      break;
    ReleaseCapture();
    paned->Tracking = FALSE;
    UpdatePanedGhostRect(paned, &GhostRect, NULL,
                         LOWORD(lParam), HIWORD(lParam));
    GetClientRect(hwnd, &rect);
    if (rect.right > rect.bottom) {
      newpos = ((gint16)HIWORD(lParam) + GTK_WIDGET(paned)->allocation.y -
                paned->true_alloc.y) * 100 / paned->true_alloc.height;
    } else {
      newpos = ((gint16)LOWORD(lParam) + GTK_WIDGET(paned)->allocation.x -
                paned->true_alloc.x) * 100 / paned->true_alloc.width;
    }
    gtk_paned_set_position(paned, newpos);
    return TRUE;
  default:
    return myDefWindowProc(hwnd, msg, wParam, lParam);
  }
  return FALSE;
}

void DisplayHTML(GtkWidget *parent, const gchar *bin, const gchar *target)
{
  ShellExecute(parent->hWnd, "open", target, NULL, NULL, 0);
}

LRESULT CALLBACK GtkUrlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  GtkWidget *widget;

  if (msg == WM_PAINT) {
    gchar *text;
    RECT wndrect;
    PAINTSTRUCT ps;
    HDC hDC;
    HFONT oldFont;

    widget = GTK_WIDGET(myGetWindowLong(hwnd, GWLP_USERDATA));
    text = GTK_LABEL(widget)->text;
    if (text && BeginPaint(hwnd, &ps)) {
      hDC = ps.hdc;
      oldFont = SelectObject(hDC, urlFont);
      SetTextColor(hDC, RGB(0, 0, 0xCC));
      SetBkMode(hDC, TRANSPARENT);
      GetClientRect(hwnd, &wndrect);
      myDrawText(hDC, text, -1, &wndrect,
                 DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);
      SelectObject(hDC, oldFont);
      EndPaint(hwnd, &ps);
    }
    return TRUE;
  } else if (msg == WM_LBUTTONUP) {
    widget = GTK_WIDGET(myGetWindowLong(hwnd, GWLP_USERDATA));

    DisplayHTML(widget, NULL, GTK_URL(widget)->target);
    return FALSE;
  } else
    return myDefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK GtkSepProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HPEN oldpen, dkpen, ltpen;
  RECT rect;
  HDC hDC;

  if (msg == WM_PAINT) {
    if (GetUpdateRect(hwnd, NULL, TRUE)) {
      BeginPaint(hwnd, &ps);
      GetClientRect(hwnd, &rect);
      hDC = ps.hdc;
      ltpen =
          CreatePen(PS_SOLID, 0, (COLORREF)GetSysColor(COLOR_3DHILIGHT));
      dkpen =
          CreatePen(PS_SOLID, 0, (COLORREF)GetSysColor(COLOR_3DSHADOW));

      if (rect.right > rect.bottom) {
        oldpen = SelectObject(hDC, dkpen);
        MoveToEx(hDC, rect.left, rect.top, NULL);
        LineTo(hDC, rect.right, rect.top);

        SelectObject(hDC, ltpen);
        MoveToEx(hDC, rect.left, rect.top + 1, NULL);
        LineTo(hDC, rect.right, rect.top + 1);
      } else {
        oldpen = SelectObject(hDC, dkpen);
        MoveToEx(hDC, rect.left, rect.top, NULL);
        LineTo(hDC, rect.left, rect.bottom);

        SelectObject(hDC, ltpen);
        MoveToEx(hDC, rect.left + 1, rect.top, NULL);
        LineTo(hDC, rect.left + 1, rect.bottom);
      }

      SelectObject(hDC, oldpen);
      DeleteObject(ltpen);
      DeleteObject(dkpen);
      EndPaint(hwnd, &ps);
    }
    return TRUE;
  } else
    return myDefWindowProc(hwnd, msg, wParam, lParam);
}

gboolean gtk_window_wndproc(GtkWidget *widget, UINT msg, WPARAM wParam,
                            LPARAM lParam, gboolean *dodef)
{
  RECT rect;
  GtkAllocation alloc;
  GtkWidget *menu;
  gboolean signal_return;
  GdkEvent event = 0;

  switch(msg) {
  case WM_SIZE:
    GetWindowRect(widget->hWnd, &rect);
    alloc.x = rect.left;
    alloc.y = rect.top;
    alloc.width = rect.right - rect.left;
    alloc.height = rect.bottom - rect.top;
    gtk_window_handle_user_size(GTK_WINDOW(widget), &alloc);
    gtk_widget_set_size(widget, &alloc);
    InvalidateRect(widget->hWnd, NULL, TRUE);
    UpdateWindow(widget->hWnd);
    return FALSE;
  case WM_GETMINMAXINFO:
    gtk_window_handle_minmax_size(GTK_WINDOW(widget), (LPMINMAXINFO)lParam);
    return FALSE;
  case WM_ACTIVATE:
  case WM_ACTIVATEAPP:
    if ((msg == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE)
        || (msg == WM_ACTIVATEAPP && wParam)) {
      if (GTK_WINDOW(widget)->focus) {
        gtk_widget_set_focus(GTK_WINDOW(widget)->focus);
      }
      if (!GTK_WINDOW(widget)->focus) {
        gtk_window_set_focus(GTK_WINDOW(widget));
      }
    } else if (msg == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) {
      gtk_window_update_focus(GTK_WINDOW(widget));
    }
    return FALSE;
  case WM_CLOSE:
    gtk_signal_emit(G_OBJECT(widget), "delete_event",
                    &event, &signal_return);
    *dodef = FALSE;
    return TRUE;
  case WM_COMMAND:
    if (HIWORD(wParam) == 0 || HIWORD(wParam) == 1) {
      menu = gtk_window_get_menu_ID(GTK_WINDOW(widget), LOWORD(wParam));
      if (menu) {
        if (GTK_MENU_ITEM(menu)->check) {
          gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu),
                                         !GTK_CHECK_MENU_ITEM(menu)->active);
        }
        gtk_signal_emit(G_OBJECT(menu), "activate");
        return FALSE;
      }
    }
    break;
  }

  return FALSE;
}

static BOOL HandleWinMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                             gboolean *dodef)
{
  GtkWidget *widget;
  GtkClass *klass;
  LPMEASUREITEMSTRUCT lpmis;
  HDC hDC;
  TEXTMETRIC tm;
  LPDRAWITEMSTRUCT lpdis;
  NMHDR *nmhdr;
  gboolean retval = FALSE;

  *dodef = TRUE;

  if (customWndProc
      && myCallWindowProc(customWndProc, hwnd, msg, wParam, lParam))
    return TRUE;

  widget = GTK_WIDGET(myGetWindowLong(hwnd, GWLP_USERDATA));
  if (widget && (klass = G_OBJECT(widget)->klass)
      && klass->wndproc) {
    retval = klass->wndproc(widget, msg, wParam, lParam, dodef);
  }

  switch (msg) {
  case WM_DRAWITEM:
    if ((lpdis = (LPDRAWITEMSTRUCT)lParam)
        && (widget = GTK_WIDGET(myGetWindowLong(lpdis->hwndItem, GWLP_USERDATA)))
        && (klass = G_OBJECT(widget)->klass)
        && klass->wndproc) {
      retval = klass->wndproc(widget, msg, wParam, lParam, dodef);
    }
    break;
  case WM_MEASUREITEM:
    lpmis = (LPMEASUREITEMSTRUCT)lParam;
    hDC = GetDC(hwnd);
    if (!GetTextMetrics(hDC, &tm))
      g_warning("GetTextMetrics failed");
    ReleaseDC(hwnd, hDC);
    if (lpmis) {
      lpmis->itemHeight = tm.tmHeight + LISTITEMVPACK * 2;
      return TRUE;
    }
    break;
  case WM_COMMAND:
    widget = GTK_WIDGET(myGetWindowLong((HWND)lParam, GWLP_USERDATA));
    klass = NULL;
    if (widget && (klass = G_OBJECT(widget)->klass)
        && klass->wndproc) {
      retval = klass->wndproc(widget, msg, wParam, lParam, dodef);
    }

    if (lParam && klass == &GtkComboBoxClass &&
               HIWORD(wParam) == CBN_SELENDOK) {
      gtk_combo_box_update_selection(widget);
    } else if (lParam && HIWORD(wParam) == BN_CLICKED) {
      gtk_signal_emit(G_OBJECT(widget), "clicked");
    } else
      return TRUE;
    break;
  case WM_NOTIFY:
    nmhdr = (NMHDR *)lParam;
    if (!nmhdr)
      break;

    widget = GTK_WIDGET(myGetWindowLong(nmhdr->hwndFrom, GWLP_USERDATA));
    if (widget && (klass = G_OBJECT(widget)->klass)
        && klass->wndproc) {
      retval = klass->wndproc(widget, msg, wParam, lParam, dodef);
    }

    if (widget && nmhdr->code == TCN_SELCHANGE) {
      gtk_notebook_set_current_page(GTK_NOTEBOOK(widget),
                                    TabCtrl_GetCurSel(nmhdr->hwndFrom));
      return FALSE;
    }
    break;
  case MYWM_SOCKETDATA:
    /* Ignore network messages if in recursive message loops, to avoid
     * dodgy race conditions */
    if (RecurseLevel <= 1) {
      /* Make any error available by the usual WSAGetLastError() mechanism */
      WSASetLastError(WSAGETSELECTERROR(lParam));
      DispatchSocketEvent((SOCKET)wParam, WSAGETSELECTEVENT(lParam));
    }
    break;
  case WM_TIMER:
    DispatchTimeoutEvent((UINT)wParam);
    return FALSE;
  }

  return retval;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  gboolean retval, dodef = TRUE;

  retval = HandleWinMessage(hwnd, msg, wParam, lParam, &dodef);
  if (dodef) {
    return myDefWindowProc(hwnd, msg, wParam, lParam);
  } else {
    return retval;
  }
}

INT_PTR APIENTRY MainDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  gboolean dodef;

  if (msg == WM_INITDIALOG) {
    return TRUE;
  } else {
    return HandleWinMessage(hwnd, msg, wParam, lParam, &dodef);
  }
}

LRESULT APIENTRY EntryWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam)
{
  GtkWidget *widget;

  if (msg == WM_KEYUP && wParam == VK_RETURN) {
    widget = GTK_WIDGET(myGetWindowLong(hwnd, GWLP_USERDATA));
    if (widget)
      gtk_signal_emit(G_OBJECT(widget), "activate");
    return FALSE;
  }
  return myCallWindowProc(wpOrigEntryProc, hwnd, msg, wParam, lParam);
}

LRESULT APIENTRY TextWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                             LPARAM lParam)
{
  GtkWidget *widget;

  if (msg == WM_GETDLGCODE) {
    widget = GTK_WIDGET(myGetWindowLong(hwnd, GWLP_USERDATA));
    if (!GTK_EDITABLE(widget)->is_editable) {
      return DLGC_HASSETSEL | DLGC_WANTARROWS;
    }
  }
  return myCallWindowProc(wpOrigTextProc, hwnd, msg, wParam, lParam);
}

void SetCustomWndProc(WNDPROC wndproc)
{
  customWndProc = wndproc;
}

/*
 * Returns TRUE if we are using version 6 of the Common Controls library
 * (as shipped with Windows XP) and thus need to worry about visual themes
 */
static gboolean CheckForXPControls(void)
{
  HINSTANCE hDll;
  BOOL retval = FALSE;

  hDll = LoadLibrary("COMCTL32.DLL");
  if (hDll) {
    DLLGETVERSIONPROC getverproc;
    getverproc = (DLLGETVERSIONPROC)GetProcAddress(hDll, "DllGetVersion");
    if (getverproc) {
      DLLVERSIONINFO dvi;
      HRESULT hr;
      ZeroMemory(&dvi, sizeof(dvi));
      dvi.cbSize = sizeof(dvi);
      hr = getverproc(&dvi);
      if (SUCCEEDED(hr) && dvi.dwMajorVersion >= 6) {
        retval = TRUE;
      }
    }
    FreeLibrary(hDll);
  }
  return retval;
}

void win32_init(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                char *MainIcon)
{
  WNDCLASS wc;

  hInst = hInstance;
  InitUnicodeSupport();
  defFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
  urlFont = CreateFont(14, 0, 0, 0, FW_SEMIBOLD, FALSE, TRUE, FALSE,
                       ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                       FF_SWISS | DEFAULT_PITCH, NULL);
  WindowList = NULL;
  RecurseLevel = 0;
  customWndProc = NULL;
  if (MainIcon) {
    mainIcon = LoadIcon(hInstance, MainIcon);
  } else {
    mainIcon = LoadIcon(NULL, IDI_APPLICATION);
  }

  InitCommonControls();
  LoadLibrary("RICHED20.DLL");

  /* Rich Edit controls have two different class names, depending on whether
   * we want ANSI or Unicode - argh! */
  if (HaveUnicodeSupport()) {
    RichEditClass = "RichEdit20W";
  } else {
    RichEditClass = "RichEdit20A";
  }
  HaveRichEdit = GetClassInfo(hInstance, RichEditClass, &wc);

  HaveXPControls = CheckForXPControls();

  if (!hPrevInstance) {
    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = mainIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "mainwin";
    myRegisterClass(&wc);

    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WC_GTKDIALOG;
    myRegisterClass(&wc);

    wc.style = 0;
    wc.lpfnWndProc = GtkPanedProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_SIZEWE);
    wc.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WC_GTKHPANED;
    myRegisterClass(&wc);

    wc.style = 0;
    wc.lpfnWndProc = GtkPanedProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_SIZENS);
    wc.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WC_GTKVPANED;
    myRegisterClass(&wc);

    wc.style = 0;
    wc.lpfnWndProc = GtkSepProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WC_GTKSEP;
    myRegisterClass(&wc);

    wc.style = 0;
    wc.lpfnWndProc = GtkUrlProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_HAND);
    wc.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WC_GTKURL;
    myRegisterClass(&wc);

    InitCListClass(hInstance);
    InitTreeViewClass(hInstance);
  }
}

guint gtk_main_level(void)
{
  return RecurseLevel;
}

void gtk_widget_update(GtkWidget *widget, gboolean ForceResize)
{
  GtkRequisition req;
  GtkWidget *window;
  GtkAllocation alloc;

  if (!GTK_WIDGET_REALIZED(widget))
    return;

  gtk_widget_size_request(widget, &req);
  window = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
  if (window) {
    alloc.x = alloc.y = 0;
    alloc.width = window->requisition.width;
    alloc.height = window->requisition.height;
    gtk_window_handle_auto_size(GTK_WINDOW(window), &alloc);
    if (alloc.width != window->allocation.width
        || alloc.height != window->allocation.height || ForceResize) {
      gtk_widget_set_size(window, &alloc);
    }
  }
}

void gtk_widget_show(GtkWidget *widget)
{
  gtk_widget_show_full(widget, TRUE);
}

gboolean gtk_widget_get_visible(GtkWidget *widget)
{
  return (GTK_WIDGET_FLAGS(widget) & GTK_VISIBLE) != 0;
}

void gtk_widget_show_full(GtkWidget *widget, gboolean recurse)
{
  GtkAllocation alloc;

  if (gtk_widget_get_visible(widget))
    return;
  GTK_WIDGET_SET_FLAGS(widget, GTK_VISIBLE);

  if (recurse)
    gtk_widget_show_all_full(widget, TRUE);
  else
    gtk_signal_emit(G_OBJECT(widget), "show");

  if (!GTK_WIDGET_REALIZED(widget)
      && G_OBJECT(widget)->klass == &GtkWindowClass) {
    gtk_widget_realize(widget);
    alloc.x = alloc.y = 0;
    alloc.width = widget->requisition.width;
    alloc.height = widget->requisition.height;
    gtk_window_set_initial_position(GTK_WINDOW(widget), &alloc);
    gtk_widget_set_size(widget, &alloc);
    ShowWindow(widget->hWnd, SW_SHOWNORMAL);
    UpdateWindow(widget->hWnd);
  } else if (GTK_WIDGET_REALIZED(widget)
             && G_OBJECT(widget)->klass != &GtkWindowClass) {
    gtk_widget_update(widget, TRUE);
    if (!recurse)
      ShowWindow(widget->hWnd, SW_SHOWNORMAL);
  }
  // g_print("widget show - set focus\n");
  gtk_widget_set_focus(widget);
}

void gtk_widget_hide(GtkWidget *widget)
{
  gtk_widget_hide_full(widget, TRUE);
}

void gtk_widget_hide_full(GtkWidget *widget, gboolean recurse)
{
  GtkAllocation alloc;
  GtkRequisition req;
  GtkWidget *window;

  if (!gtk_widget_get_visible(widget))
    return;

  if (recurse)
    gtk_widget_hide_all_full(widget, TRUE);
  else {
    gtk_signal_emit(G_OBJECT(widget), "hide");
    if (widget->hWnd)
      ShowWindow(widget->hWnd, SW_HIDE);
  }

  GTK_WIDGET_UNSET_FLAGS(widget, GTK_VISIBLE);

  gtk_widget_lose_focus(widget);

  gtk_widget_size_request(widget, &req);
  if (GTK_WIDGET_REALIZED(widget)) {
    window = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
    if (window) {
      alloc.x = alloc.y = 0;
      alloc.width = window->requisition.width;
      alloc.height = window->requisition.height;
      gtk_window_handle_auto_size(GTK_WINDOW(window), &alloc);
      gtk_widget_set_size(window, &alloc);
    }
  }
}

void gtk_widget_set_focus(GtkWidget *widget)
{
  GtkWidget *window;

  if (!widget || !GTK_WIDGET_CAN_FOCUS(widget)
      || !GTK_WIDGET_SENSITIVE(widget) || !gtk_widget_get_visible(widget))
    return;
  window = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
  gtk_window_update_focus(GTK_WINDOW(window));
  if (!window || GTK_WINDOW(window)->focus)
    return;

  // g_print("Window %p focus set to widget %p
  // (%s)\n",window,widget,G_OBJECT(widget)->klass->Name);
  GTK_WINDOW(window)->focus = widget;
  if (widget->hWnd) {
    // if (!SetFocus(widget->hWnd)) g_print("SetFocus failed on widget
    // %p\n",widget);
    SetFocus(widget->hWnd);
  }
  // else g_print("Cannot call SetFocus - no hWnd\n");
}

static BOOL CALLBACK SetFocusEnum(HWND hWnd, LPARAM data)
{
  GtkWidget *widget;
  GtkWindow *window = GTK_WINDOW(data);

  widget = GTK_WIDGET(myGetWindowLong(hWnd, GWLP_USERDATA));
  if (!widget || !GTK_WIDGET_CAN_FOCUS(widget) ||
      !GTK_WIDGET_SENSITIVE(widget) || !gtk_widget_get_visible(widget) ||
      window->focus == widget) {
    return TRUE;
  } else {
    // g_print("gtk_window_set_focus: focus set to widget %p\n",widget);
    window->focus = widget;
    SetFocus(widget->hWnd);
    return FALSE;
  }
}

void gtk_window_set_focus(GtkWindow *window)
{
  if (!window || !GTK_WIDGET(window)->hWnd)
    return;
  EnumChildWindows(GTK_WIDGET(window)->hWnd, SetFocusEnum, (LPARAM)window);
}

void gtk_widget_lose_focus(GtkWidget *widget)
{
  GtkWidget *window;

  if (!widget || !GTK_WIDGET_CAN_FOCUS(widget))
    return;
  window = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
  gtk_window_update_focus(GTK_WINDOW(window));
  if (GTK_WINDOW(window)->focus == widget) {
    gtk_window_set_focus(GTK_WINDOW(window));
  }
}

void gtk_window_update_focus(GtkWindow *window)
{
  GtkWidget *widget;
  HWND FocusWnd;

  if (GTK_WIDGET(window)->hWnd != GetActiveWindow())
    return;
  FocusWnd = GetFocus();
  window->focus = NULL;
  if (FocusWnd) {
    widget = GTK_WIDGET(myGetWindowLong(FocusWnd, GWLP_USERDATA));
    if (widget && GTK_WIDGET(window)->hWnd &&
        IsChild(GTK_WIDGET(window)->hWnd, FocusWnd)) {
      window->focus = widget;
    }
  }
}

void gtk_widget_realize(GtkWidget *widget)
{
  GtkRequisition req;

  if (GTK_WIDGET_REALIZED(widget))
    return;
  gtk_signal_emit(G_OBJECT(widget), "realize", &req);
  if (widget->hWnd)
    mySetWindowLong(widget->hWnd, GWLP_USERDATA, (LONG_PTR)widget);
  GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
  gtk_widget_set_sensitive(widget, GTK_WIDGET_SENSITIVE(widget));

  gtk_widget_size_request(widget, &req);
}

void gtk_widget_create(GtkWidget *widget)
{
  GTK_WIDGET_SET_FLAGS(widget, GTK_SENSITIVE);
  widget->usize.width = 0;
  widget->usize.height = 0;
}

void gtk_widget_destroy(GtkWidget *widget)
{
  if (!widget)
    return;

  /* If we're closing a modal window, reactivate the parent _before_
   * calling DestroyWindow, to avoid annoying flicker caused if Windows
   * chooses to reactivate another application when we close the modal
   * dialog */
  if (G_OBJECT(widget)->klass == &GtkWindowClass) {
    EnableParent(GTK_WINDOW(widget));
  }

  gtk_widget_lose_focus(widget);
  if (widget->hWnd)
    DestroyWindow(widget->hWnd);
  widget->hWnd = NULL;
  gtk_signal_emit(G_OBJECT(widget), "destroy");
  g_free(widget);
}

void gtk_widget_set_sensitive(GtkWidget *widget, gboolean sensitive)
{
  if (sensitive) {
    GTK_WIDGET_SET_FLAGS(widget, GTK_SENSITIVE);
    if (widget->hWnd)
      EnableWindow(widget->hWnd, sensitive);
    gtk_widget_set_focus(widget);
  } else {
    GTK_WIDGET_UNSET_FLAGS(widget, GTK_SENSITIVE);
    gtk_widget_lose_focus(widget);
    if (widget->hWnd)
      EnableWindow(widget->hWnd, sensitive);
  }

  gtk_signal_emit(G_OBJECT(widget), sensitive ? "enable" : "disable");
  if (sensitive && widget->hWnd
      && G_OBJECT(widget)->klass == &GtkWindowClass)
    SetActiveWindow(widget->hWnd);
}

void gtk_widget_size_request(GtkWidget *widget,
                             GtkRequisition *requisition)
{
  GtkRequisition req;

  requisition->width = requisition->height = 0;
  if (gtk_widget_get_visible(widget)) {
    gtk_signal_emit(G_OBJECT(widget), "size_request", requisition);
  }
  if (requisition->width < widget->usize.width)
    requisition->width = widget->usize.width;
  if (requisition->height < widget->usize.height)
    requisition->height = widget->usize.height;
  if (requisition->width != widget->requisition.width ||
      requisition->height != widget->requisition.height) {
    memcpy(&widget->requisition, requisition, sizeof(GtkRequisition));
    if (widget->parent)
      gtk_widget_size_request(widget->parent, &req);
  }
}

void MapWidgetOrigin(GtkWidget *widget, POINT *pt)
{
  HWND imm_parent, window_parent;

  imm_parent = GetParent(widget->hWnd);
  window_parent = gtk_get_window_hwnd(widget);
  if (imm_parent && window_parent && imm_parent != window_parent) {
    MapWindowPoints(window_parent, imm_parent, pt, 1);
  }
}

void gtk_widget_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  gtk_signal_emit(G_OBJECT(widget), "set_size", allocation);
  memcpy(&widget->allocation, allocation, sizeof(GtkAllocation));
  if (widget->hWnd) {
    POINT pt;

    pt.x = allocation->x;
    pt.y = allocation->y;
    MapWidgetOrigin(widget, &pt);
    SetWindowPos(widget->hWnd, HWND_TOP, pt.x, pt.y,
                 allocation->width, allocation->height,
                 SWP_NOZORDER |
                 (G_OBJECT(widget)->klass ==
                  &GtkWindowClass ? SWP_NOMOVE : 0));
  }
}

GtkWidget *gtk_window_new(GtkWindowType type)
{
  GtkWindow *win;

  win = GTK_WINDOW(GtkNewObject(&GtkWindowClass));

  win->title = g_strdup("");
  win->type = type;
  win->allow_grow = TRUE;

  return GTK_WIDGET(win);
}

void gtk_window_set_title(GtkWindow *window, const gchar *title)
{
  g_free(window->title);
  window->title = g_strdup(title);
}

gint gtk_window_delete_event(GtkWidget *widget, GdkEvent *event)
{
  gtk_widget_destroy(widget);
  return TRUE;
}

void gtk_window_set_default_size(GtkWindow *window, gint width,
                                 gint height)
{
  window->default_width = width;
  window->default_height = height;
}

void gtk_window_set_transient_for(GtkWindow *window, GtkWindow *parent)
{
  if (window && parent) {
    GTK_WIDGET(window)->parent = GTK_WIDGET(parent);
    if (GTK_WIDGET(window)->hWnd && GTK_WIDGET(parent)->hWnd) {
      SetParent(GTK_WIDGET(window)->hWnd, GTK_WIDGET(parent)->hWnd);
    }
  }
}

void gtk_window_set_policy(GtkWindow *window, gint allow_shrink,
                           gint allow_grow, gint auto_shrink)
{
  window->allow_shrink = allow_shrink;
  window->allow_grow = allow_grow;
  window->auto_shrink = auto_shrink;
}

void gtk_window_set_menu(GtkWindow *window, GtkMenuBar *menu_bar)
{
  HWND hWnd;
  HMENU hMenu;

  hWnd = GTK_WIDGET(window)->hWnd;
  hMenu = GTK_MENU_SHELL(menu_bar)->menu;

  if (hWnd && hMenu)
    SetMenu(hWnd, hMenu);
  window->menu_bar = menu_bar;
}

void gtk_container_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkContainer *container;
  GtkAllocation child_alloc;

  container = GTK_CONTAINER(widget);
  if (container->child) {
    child_alloc.x = allocation->x + container->border_width;
    child_alloc.y = allocation->y + container->border_width;
    child_alloc.width = allocation->width - container->border_width * 2;
    child_alloc.height = allocation->height - container->border_width * 2;
    gtk_widget_set_size(container->child, &child_alloc);
  }
}

void gtk_frame_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkFrame *frame;
  GtkContainer *container;

  frame = GTK_FRAME(widget);
  container = GTK_CONTAINER(widget);
  allocation->x += container->border_width;
  allocation->y += container->border_width;
  allocation->width -= container->border_width * 2;
  allocation->height -= container->border_width * 2;

  if (container->child) {
    GtkAllocation child_alloc;

    child_alloc.x = allocation->x + 3;
    child_alloc.y = allocation->y + 3 + frame->label_req.height;
    child_alloc.width = allocation->width - 6;
    child_alloc.height = allocation->height - frame->label_req.height - 6;
    gtk_widget_set_size(container->child, &child_alloc);
  }
}

void gtk_frame_set_shadow_type(GtkFrame *frame, GtkShadowType type)
{
}

void gtk_container_size_request(GtkWidget *widget,
                                GtkRequisition *requisition)
{
  GtkContainer *container;

  container = GTK_CONTAINER(widget);
  if (container->child) {
    requisition->width = container->child->requisition.width +
        container->border_width * 2;
    requisition->height = container->child->requisition.height +
        container->border_width * 2;
  }
}

void gtk_window_size_request(GtkWidget *widget,
                             GtkRequisition *requisition)
{
  gtk_container_size_request(widget, requisition);
  requisition->width += GetSystemMetrics(SM_CXSIZEFRAME) * 2;
  requisition->height += GetSystemMetrics(SM_CYSIZEFRAME) * 2 +
      GetSystemMetrics(SM_CYCAPTION);
  if (GTK_WINDOW(widget)->menu_bar) {
    requisition->height += GetSystemMetrics(SM_CYMENU);
  }
}

void gtk_window_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkAllocation child_alloc;
  GtkWindow *window = GTK_WINDOW(widget);

  child_alloc.x = child_alloc.y = 0;
  child_alloc.width =
      allocation->width - GetSystemMetrics(SM_CXSIZEFRAME) * 2;
  child_alloc.height =
      allocation->height - GetSystemMetrics(SM_CYSIZEFRAME) * 2 -
      GetSystemMetrics(SM_CYCAPTION);
  if (window->menu_bar) {
    child_alloc.height -= GetSystemMetrics(SM_CYMENU);
  }
  gtk_container_set_size(widget, &child_alloc);
}

void gtk_button_size_request(GtkWidget *widget,
                             GtkRequisition *requisition)
{
  SIZE size, minsize;
  GtkButton *but = GTK_BUTTON(widget);

  gtk_container_size_request(widget, requisition);

  /* Without minsize, an unexpanded "OK" button looks silly... */
  if (GetTextSize(widget->hWnd, but->text, &size, defFont)
      && GetTextSize(widget->hWnd, "Cancel", &minsize, defFont)) {
    size.cx = MAX(size.cx, minsize.cx);
    size.cy = MAX(size.cy, minsize.cy);
    requisition->width = size.cx + 15;
    requisition->height = size.cy + 10;
  }
}

BOOL GetTextSize(HWND hWnd, char *text, LPSIZE lpSize, HFONT hFont)
{
  HDC hDC;
  BOOL RetVal = 0;
  SIZE LineSize;
  HFONT oldFont;
  char *endpt, *startpt;

  hDC = GetDC(hWnd);
  oldFont = SelectObject(hDC, hFont);

  startpt = text;
  lpSize->cx = lpSize->cy = 0;

  while (startpt) {
    endpt = startpt;
    while (endpt && *endpt != '\n' && *endpt)
      endpt++;
    if (endpt) {
      if ((endpt == startpt
           && GetTextExtentPoint32(hDC, "W", 1, &LineSize))
          || (endpt != startpt
              && GetTextExtentPoint32(hDC, startpt, endpt - startpt,
                                      &LineSize))) {
        RetVal = 1;
        if (LineSize.cx > lpSize->cx)
          lpSize->cx = LineSize.cx;
        lpSize->cy += LineSize.cy;
      }
      if (*endpt == '\0')
        break;
      startpt = endpt + 1;
    } else
      break;
  }
  SelectObject(hDC, oldFont);
  ReleaseDC(hWnd, hDC);
  return RetVal;
}

void gtk_entry_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  SIZE size;

  if (GetTextSize(widget->hWnd, "text", &size, defFont)) {
    requisition->width = size.cx * 6;
    requisition->height = size.cy + 8;
  }
}

void gtk_text_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  SIZE size;

  if (GetTextSize(widget->hWnd, "Sample text", &size, defFont)) {
    requisition->width = size.cx;
    requisition->height = size.cy * 2 + 8;
  }
}

void gtk_frame_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  SIZE size;
  GtkFrame *frame = GTK_FRAME(widget);

  gtk_container_size_request(widget, requisition);

  if (GetTextSize(widget->hWnd, frame->text, &size, defFont)) {
    frame->label_req.width = size.cx;
    frame->label_req.height = size.cy;
    if (size.cx > requisition->width)
      requisition->width = size.cx;
    requisition->width += 6;
    requisition->height += size.cy + 6;
  }
}


void gtk_check_button_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition)
{
  gtk_button_size_request(widget, requisition);
  requisition->width += 10;
}

GtkWidget *gtk_button_new_with_label(const gchar *label)
{
  GtkButton *but;

  but = GTK_BUTTON(GtkNewObject(&GtkButtonClass));
  but->text = g_strdup(label);
  g_strdelimit(but->text, "_", '&');

  return GTK_WIDGET(but);
}

GtkWidget *gtk_check_button_new_with_label(const gchar *label)
{
  GtkButton *but;

  but = GTK_BUTTON(GtkNewObject(&GtkCheckButtonClass));
  but->text = g_strdup(label);
  g_strdelimit(but->text, "_", '&');

  return GTK_WIDGET(but);
}

GtkWidget *gtk_radio_button_new_with_label_from_widget(GtkRadioButton *group,
                                                       const gchar *label)
{
  GSList *list;

  list = gtk_radio_button_get_group(group);
  return (gtk_radio_button_new_with_label(list, label));
}

GtkWidget *gtk_radio_button_new_with_label(GSList *group,
                                           const gchar *label)
{
  GtkButton *but;
  GtkRadioButton *radio;
  GSList *listpt;

  but = GTK_BUTTON(GtkNewObject(&GtkRadioButtonClass));
  but->text = g_strdup(label);
  g_strdelimit(but->text, "_", '&');

  if (group == NULL)
    GTK_TOGGLE_BUTTON(but)->toggled = TRUE;

  group = g_slist_append(group, GTK_RADIO_BUTTON(but));
  for (listpt = group; listpt; listpt = g_slist_next(listpt)) {
    radio = GTK_RADIO_BUTTON(listpt->data);
    radio->group = group;
  }

  return GTK_WIDGET(but);
}

GtkWidget *gtk_label_new(const gchar *text)
{
  GtkLabel *label;

  label = GTK_LABEL(GtkNewObject(&GtkLabelClass));
  gtk_label_set_text(label, text);

  return GTK_WIDGET(label);
}

GtkWidget *gtk_url_new(const gchar *text, const gchar *target,
                       const gchar *bin)
{
  GtkUrl *url;

  url = GTK_URL(GtkNewObject(&GtkUrlClass));

  GTK_LABEL(url)->text = g_strdup(text);
  url->target = g_strdup(target);

  /* N.B. "bin" argument is ignored under Win32 */

  return GTK_WIDGET(url);
}

GtkWidget *gtk_box_new(GtkOrientation orientation, gint spacing)
{
  GtkBox *box;

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    box = GTK_BOX(GtkNewObject(&GtkHBoxClass));
  } else {
    box = GTK_BOX(GtkNewObject(&GtkVBoxClass));
  }

  box->spacing = spacing;
  box->homogeneous = FALSE;
  return GTK_WIDGET(box);
}

void gtk_box_set_homogeneous(GtkBox *box, gboolean homogeneous)
{
  box->homogeneous = homogeneous;
}

GtkWidget *gtk_frame_new(const gchar *text)
{
  GtkFrame *frame;

  frame = GTK_FRAME(GtkNewObject(&GtkFrameClass));
  frame->text = g_strdup(text);

  return GTK_WIDGET(frame);
}

GtkWidget *gtk_text_new(GtkAdjustment *hadj, GtkAdjustment *vadj)
{
  GtkText *text;

  text = GTK_TEXT(GtkNewObject(&GtkTextClass));
  text->buffer = g_new(GtkTextBuffer, 1);
  g_datalist_init(&text->buffer->tags);
  return GTK_WIDGET(text);
}

GtkWidget *gtk_scrolled_text_view_new(GtkWidget **pack_widg)
{
  GtkWidget *text;

  text = gtk_text_new(NULL, NULL);
  *pack_widg = text;
  return text;
}

GtkWidget *gtk_entry_new()
{
  GtkEntry *entry;

  entry = GTK_ENTRY(GtkNewObject(&GtkEntryClass));
  entry->is_visible = TRUE;

  return GTK_WIDGET(entry);
}

GSList *gtk_radio_button_get_group(GtkRadioButton *radio_button)
{
  return radio_button->group;
}

static void gtk_editable_sync_text(GtkEditable *editable)
{
  HWND hWnd;
  gchar *buffer;

  hWnd = GTK_WIDGET(editable)->hWnd;
  if (!hWnd) {
    return;
  }

  buffer = myGetWindowText(hWnd);
  g_string_assign(editable->text, buffer);
  g_free(buffer);
}

void gtk_editable_insert_text(GtkEditable *editable, const gchar *new_text,
                              gint new_text_length, gint *position)
{
  GtkWidget *widget = GTK_WIDGET(editable);
  HWND hWnd;
  gint i;
  GString *newstr;

  gtk_editable_sync_text(editable);

  /* Convert Unix-style lone '\n' to Windows-style '\r\n' */
  newstr = g_string_new("");
  for (i = 0; i < new_text_length && new_text[i]; i++) {
    if (new_text[i] == '\n' && (i == 0 || new_text[i - 1] != '\r')) {
      g_string_append_c(newstr, '\r');
    }
    g_string_append_c(newstr, new_text[i]);
  }
  g_string_insert(editable->text, *position, newstr->str);

  hWnd = widget->hWnd;
  if (hWnd) {
    mySendMessage(hWnd, EM_SETSEL, (WPARAM)*position, (LPARAM)*position);
    myEditReplaceSel(hWnd, FALSE, newstr->str);
    *position += newstr->len;
    gtk_editable_set_position(editable, *position);
  }
  g_string_free(newstr, TRUE);
}

static void gtk_text_view_set_format(GtkTextView *textview,
                                     const gchar *tagname, guint len,
                                     gint endpos)
{
  CHARFORMAT *cf, cfdef;
  GtkWidget *widget = GTK_WIDGET(textview);
  GtkTextBuffer *buffer;
  DWORD st, end;

  if (!GTK_WIDGET_REALIZED(widget)) {
    return;
  }

  /* RichEdit controls do something odd behind the scenes with line ends,
   * so we use our guess for the last character position, and then read
   * back the resultant selection to get the true "length" of the control */
  mySendMessage(widget->hWnd, EM_SETSEL, (WPARAM)endpos, (LPARAM)endpos);
  mySendMessage(widget->hWnd, EM_GETSEL, (WPARAM)&st, (LPARAM)&end);
  endpos = (gint)end;

  mySendMessage(widget->hWnd, EM_SETSEL, (WPARAM)(endpos - len),
                (LPARAM)endpos);

  if (!tagname || !(buffer = gtk_text_view_get_buffer(textview))
      || !(cf = g_datalist_get_data(&buffer->tags, tagname))) {
    cfdef.cbSize = sizeof(CHARFORMAT);
    cfdef.dwMask = CFM_COLOR;
    cfdef.dwEffects = CFE_AUTOCOLOR;
    cf = &cfdef;
  }
  mySendMessage(widget->hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)cf);
}

void gtk_editable_delete_text(GtkEditable *editable,
                              gint start_pos, gint end_pos)
{
  GtkWidget *widget = GTK_WIDGET(editable);
  HWND hWnd;

  gtk_editable_sync_text(editable);
  if (end_pos < 0 || end_pos >= editable->text->len)
    end_pos = editable->text->len;
  g_string_erase(editable->text, start_pos, end_pos - start_pos);

  hWnd = widget->hWnd;
  if (hWnd) {
    mySetWindowText(hWnd, editable->text->str);
  }
}

gchar *gtk_editable_get_chars(GtkEditable *editable,
                              gint start_pos, gint end_pos)
{
  gchar *retbuf;
  gint copylen;

  gtk_editable_sync_text(editable);
  if (end_pos < 0 || end_pos >= editable->text->len)
    end_pos = editable->text->len;
  copylen = end_pos - start_pos;
  retbuf = g_new(gchar, copylen + 1);

  memcpy(retbuf, &editable->text->str[start_pos], copylen);
  retbuf[copylen] = '\0';
  return retbuf;
}

void gtk_editable_set_editable(GtkEditable *editable, gboolean is_editable)
{
  GtkWidget *widget = GTK_WIDGET(editable);
  HWND hWnd;

  editable->is_editable = is_editable;
  hWnd = widget->hWnd;
  if (hWnd)
    mySendMessage(hWnd, EM_SETREADONLY, (WPARAM)(!is_editable), (LPARAM)0);
}

void gtk_editable_set_position(GtkEditable *editable, gint position)
{
  GtkWidget *widget = GTK_WIDGET(editable);
  HWND hWnd;

  if (!GTK_WIDGET_REALIZED(widget))
    return;
  hWnd = widget->hWnd;

  /* RichEdit 2.0 doesn't appear to show the caret unless we show the
   * selection */
  mySendMessage(hWnd, EM_HIDESELECTION, 0, 0);
  mySendMessage(hWnd, EM_SETSEL, (WPARAM)position, (LPARAM)position);
  mySendMessage(hWnd, EM_SCROLLCARET, 0, 0);
  mySendMessage(hWnd, EM_HIDESELECTION, 1, 0);
}

gint gtk_editable_get_position(GtkEditable *editable)
{
  GtkWidget *widget = GTK_WIDGET(editable);
  HWND hWnd;
  DWORD EndPos;

  if (!GTK_WIDGET_REALIZED(widget))
    return 0;
  hWnd = widget->hWnd;
  mySendMessage(hWnd, EM_GETSEL, (WPARAM)NULL, (LPARAM)&EndPos);
  return (gint)EndPos;
}

guint gtk_text_get_length(GtkText *text)
{
  return GTK_EDITABLE(text)->text->len;
}

void gtk_box_pack_start(GtkBox *box, GtkWidget *child, gboolean Expand,
                        gboolean Fill, gint Padding)
{
  GtkBoxChild *newChild;

  newChild = g_new0(GtkBoxChild, 1);

  newChild->widget = child;
  newChild->expand = Expand;
  newChild->fill = Fill;

  box->children = g_list_append(box->children, (gpointer)newChild);
  child->parent = GTK_WIDGET(box);
  if (GTK_WIDGET_REALIZED(GTK_WIDGET(box))) {
    gtk_widget_realize(child);
    gtk_widget_update(GTK_WIDGET(box), TRUE);
  }
}

void gtk_button_destroy(GtkWidget *widget)
{
  g_free(GTK_BUTTON(widget)->text);
}

void gtk_frame_destroy(GtkWidget *widget)
{
  gtk_container_destroy(widget);
  g_free(GTK_FRAME(widget)->text);
}

void gtk_container_destroy(GtkWidget *widget)
{
  GtkWidget *child = GTK_CONTAINER(widget)->child;

  if (child)
    gtk_widget_destroy(child);
}

void gtk_box_destroy(GtkWidget *widget)
{
  GtkBoxChild *child;
  GList *children;

  gtk_container_destroy(widget);

  for (children = GTK_BOX(widget)->children; children;
       children = g_list_next(children)) {
    child = (GtkBoxChild *)(children->data);
    if (child && child->widget)
      gtk_widget_destroy(child->widget);
    g_free(child);
  }
  g_list_free(GTK_BOX(widget)->children);
}

static void gtk_text_destroy(GtkWidget *widget)
{
  GtkText *text = GTK_TEXT(widget);
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(text);

  if (buffer) {
    g_datalist_clear(&buffer->tags);
    g_free(buffer);
  }
}

static void EnableParent(GtkWindow *window)
{
  GtkWidget *parent;

  parent = GTK_WIDGET(window)->parent;

  if (window->modal && parent) {
    GSList *list;
    GtkWindow *listwin;
    HWND ourhWnd, parenthWnd;

    ourhWnd = GTK_WIDGET(window)->hWnd;
    parenthWnd = parent->hWnd;
    for (list = WindowList; list; list = g_slist_next(list)) {
      listwin = GTK_WINDOW(list->data);
      if (listwin != window && listwin->modal
          && gtk_widget_get_visible(GTK_WIDGET(listwin))
          && GTK_WIDGET(listwin)->parent == parent)
        return;
    }
    gtk_widget_set_sensitive(parent, TRUE);

    if (ourhWnd && parenthWnd && ourhWnd == GetActiveWindow()) {
      SetActiveWindow(parenthWnd);
    }
  }
}

void gtk_window_destroy(GtkWidget *widget)
{
  GtkWindow *window = GTK_WINDOW(widget);

  WindowList = g_slist_remove(WindowList, (gpointer)window);
  gtk_container_destroy(widget);
  if (window->accel_group)
    gtk_accel_group_destroy(window->accel_group);
  if (window->hAccel)
    DestroyAcceleratorTable(window->hAccel);
  g_free(window->title);
}

void gtk_window_show(GtkWidget *widget)
{
  GtkWindow *window = GTK_WINDOW(widget);

  if (window->modal && widget->parent)
    gtk_widget_set_sensitive(widget->parent, FALSE);
}

void gtk_window_hide(GtkWidget *widget)
{
  GtkWindow *window = GTK_WINDOW(widget);

  EnableParent(window);
}

void gtk_hbox_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  GtkBoxChild *child;
  GList *children;
  GtkRequisition *child_req;
  gint spacing = GTK_BOX(widget)->spacing, numchildren = 0;
  gint maxreq = 0;
  gboolean homogeneous = GTK_BOX(widget)->homogeneous;
  gboolean first = TRUE;

  for (children = GTK_BOX(widget)->children; children;
       children = g_list_next(children)) {
    child = (GtkBoxChild *)(children->data);
    if (child && child->widget && gtk_widget_get_visible(child->widget)) {
      if (first) {
        first = FALSE;
      } else {
        requisition->width += spacing;
      }
      child_req = &child->widget->requisition;
      if (homogeneous) {
        numchildren++;
        if (child_req->width > maxreq)
          maxreq = child_req->width;
      } else {
        requisition->width += child_req->width;
      }
      if (child_req->height > requisition->height)
        requisition->height = child_req->height;
    }
  }
  if (homogeneous)
    requisition->width += numchildren * maxreq;
  GTK_BOX(widget)->maxreq = maxreq;
  requisition->width += 2 * GTK_CONTAINER(widget)->border_width;
  requisition->height += 2 * GTK_CONTAINER(widget)->border_width;
}

void gtk_vbox_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  GtkBoxChild *child;
  GList *children;
  GtkRequisition *child_req;
  gint spacing = GTK_BOX(widget)->spacing, numchildren = 0;
  gint maxreq = 0;
  gboolean homogeneous = GTK_BOX(widget)->homogeneous;
  gboolean first = TRUE;

  for (children = GTK_BOX(widget)->children; children;
       children = g_list_next(children)) {
    child = (GtkBoxChild *)(children->data);
    if (child && child->widget && gtk_widget_get_visible(child->widget)) {
      if (first) {
        first = FALSE;
      } else {
        requisition->height += spacing;
      }
      child_req = &child->widget->requisition;
      child_req = &child->widget->requisition;
      if (homogeneous) {
        numchildren++;
        if (child_req->height > maxreq)
          maxreq = child_req->height;
      } else {
        requisition->height += child_req->height;
      }
      if (child_req->width > requisition->width)
        requisition->width = child_req->width;
    }
  }
  if (homogeneous)
    requisition->height += numchildren * maxreq;
  GTK_BOX(widget)->maxreq = maxreq;
  requisition->width += 2 * GTK_CONTAINER(widget)->border_width;
  requisition->height += 2 * GTK_CONTAINER(widget)->border_width;
}

static void gtk_box_count_children(GtkBox *box, gint16 allocation,
                                   gint16 requisition, gint *extra)
{
  GtkBoxChild *child;
  GList *children;
  gint NumCanExpand = 0;

  for (children = box->children; children;
       children = g_list_next(children)) {
    child = (GtkBoxChild *)(children->data);
    if (child && child->widget && gtk_widget_get_visible(child->widget) &&
        child->expand)
      NumCanExpand++;
  }

  *extra = allocation - requisition;
  if (NumCanExpand > 0)
    *extra /= NumCanExpand;
}

static void gtk_box_size_child(GtkBox *box, GtkBoxChild *child,
                               gint extra, gint16 maxpos,
                               gint16 requisition, gint16 *pos,
                               gint16 *size, GList *listpt, gint16 *curpos)
{
  gboolean TooSmall = FALSE;

  *pos = *curpos;
  if (extra < 0) {
    extra = 0;
    TooSmall = TRUE;
  }
  if (child->expand && child->fill) {
    *size = requisition + extra;
    *curpos += requisition + extra;
  } else if (child->expand) {
    *size = requisition;
    *pos += extra / 2;
    *curpos += requisition + extra;
  } else {
    *size = requisition;
    *curpos += requisition;
  }
  *curpos += box->spacing;
  if (TooSmall) {
    if (*pos >= maxpos) {
      *pos = *size = 0;
    } else if (*pos + *size > maxpos) {
      *size = maxpos - *pos;
    }
  }
}

void gtk_hbox_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_alloc;
  gint extra;
  gint16 curpos;
  gint maxpos, height, border_width;

  border_width = GTK_CONTAINER(widget)->border_width;
  maxpos = allocation->x + allocation->width - border_width;
  height = allocation->height - 2 * border_width;

  box = GTK_BOX(widget);

  curpos = allocation->x + border_width;
  gtk_box_count_children(box, allocation->width, widget->requisition.width,
                         &extra);

  for (children = box->children; children;
       children = g_list_next(children)) {
    child = (GtkBoxChild *)(children->data);
    if (child && child->widget && gtk_widget_get_visible(child->widget)) {
      gtk_box_size_child(box, child, extra, maxpos,
                         box->homogeneous ? box->maxreq :
                         child->widget->requisition.width,
                         &child_alloc.x, &child_alloc.width,
                         children, &curpos);
      child_alloc.y = allocation->y + border_width;
      child_alloc.height = height;
      gtk_widget_set_size(child->widget, &child_alloc);
    }
  }
}

void gtk_vbox_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_alloc;
  gint extra;
  gint16 curpos;
  gint width, maxpos, border_width;

  border_width = GTK_CONTAINER(widget)->border_width;
  width = allocation->width - 2 * border_width;
  maxpos = allocation->y + allocation->height - border_width;

  box = GTK_BOX(widget);

  curpos = allocation->y + border_width;
  gtk_box_count_children(box, allocation->height,
                         widget->requisition.height, &extra);

  for (children = box->children; children;
       children = g_list_next(children)) {
    child = (GtkBoxChild *)(children->data);
    if (child && child->widget && gtk_widget_get_visible(child->widget)) {
      gtk_box_size_child(box, child, extra, maxpos,
                         box->homogeneous ? box->maxreq :
                         child->widget->requisition.height,
                         &child_alloc.y, &child_alloc.height,
                         children, &curpos);
      child_alloc.x = allocation->x + border_width;
      child_alloc.width = width;
      gtk_widget_set_size(child->widget, &child_alloc);
    }
  }
}

void gtk_window_realize(GtkWidget *widget)
{
  GtkWindow *win = GTK_WINDOW(widget);
  HWND Parent;
  DWORD resize = 0;

  if (win->allow_shrink || win->allow_grow)
    resize = WS_SIZEBOX;

  Parent = gtk_get_parent_hwnd(widget->parent);
  if (Parent) {
    widget->hWnd = myCreateDialog(hInst, "gtkdialog", Parent, MainDlgProc);
    mySetWindowText(widget->hWnd, win->title);
  } else {
    widget->hWnd = myCreateWindow("mainwin", win->title,
                                  WS_OVERLAPPEDWINDOW | CS_HREDRAW |
                                  CS_VREDRAW | resize, CW_USEDEFAULT, 0, 0,
                                  0, Parent, NULL, hInst, NULL);
    if (!TopLevel)
      TopLevel = widget->hWnd;
  }
  WindowList = g_slist_append(WindowList, (gpointer)win);
  gtk_set_default_font(widget->hWnd);
  gtk_container_realize(widget);

  if (win->accel_group && win->accel_group->numaccel) {
    win->hAccel = CreateAcceleratorTable(win->accel_group->accel,
                                         win->accel_group->numaccel);
  }
}

void gtk_container_realize(GtkWidget *widget)
{
  GtkWidget *child = GTK_CONTAINER(widget)->child;

  if (child)
    gtk_widget_realize(child);
}

void gtk_box_realize(GtkWidget *widget)
{
  GtkBoxChild *child;
  GList *children;

  gtk_container_realize(widget);

  for (children = GTK_BOX(widget)->children; children;
       children = g_list_next(children)) {
    child = (GtkBoxChild *)(children->data);
    if (child && child->widget)
      gtk_widget_realize(child->widget);
  }
}

void gtk_button_realize(GtkWidget *widget)
{
  GtkButton *but = GTK_BUTTON(widget);
  HWND Parent;

  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  Parent = gtk_get_parent_hwnd(widget);
  widget->hWnd = myCreateWindow("BUTTON", but->text,
                                WS_CHILD | WS_TABSTOP |
                                (GTK_WIDGET_FLAGS(widget) & GTK_IS_DEFAULT ?
                                 BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
                                widget->allocation.x, widget->allocation.y,
                                widget->allocation.width,
                                widget->allocation.height, Parent, NULL,
                                hInst, NULL);
  gtk_set_default_font(widget->hWnd);
}

void gtk_entry_realize(GtkWidget *widget)
{
  HWND Parent;

  Parent = gtk_get_parent_hwnd(widget);
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  widget->hWnd = myCreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
                                  WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                  widget->allocation.x, widget->allocation.y,
                                  widget->allocation.width,
                                  widget->allocation.height, Parent, NULL,
                                  hInst, NULL);
  /* Subclass the window (we assume that all edit boxes have the same
   * window procedure) */
  wpOrigEntryProc = (WNDPROC)mySetWindowLong(widget->hWnd,
                                             GWLP_WNDPROC,
                                             (LONG_PTR)EntryWndProc);
  gtk_set_default_font(widget->hWnd);
  gtk_editable_set_editable(GTK_EDITABLE(widget),
                            GTK_EDITABLE(widget)->is_editable);
  gtk_entry_set_visibility(GTK_ENTRY(widget),
                           GTK_ENTRY(widget)->is_visible);
  mySetWindowText(widget->hWnd, GTK_EDITABLE(widget)->text->str);
}

void gtk_text_realize(GtkWidget *widget)
{
  HWND Parent;
  gboolean editable;

  Parent = gtk_get_parent_hwnd(widget);
  editable = GTK_EDITABLE(widget)->is_editable;
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  widget->hWnd = myCreateWindowEx(WS_EX_CLIENTEDGE,
                                  HaveRichEdit ? RichEditClass : "EDIT", "",
                                  WS_CHILD | (editable ? WS_TABSTOP : 0) |
                                  ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL |
                                  (GTK_TEXT(widget)->word_wrap ?
                                   0 : ES_AUTOHSCROLL), 0, 0, 0,
                                  0, Parent, NULL, hInst, NULL);
  /* Subclass the window (we assume that all multiline edit boxes have the 
   * same window procedure) */
  wpOrigTextProc = (WNDPROC)mySetWindowLong(widget->hWnd,
                                            GWLP_WNDPROC,
                                            (LONG_PTR)TextWndProc);
  gtk_set_default_font(widget->hWnd);
  gtk_editable_set_editable(GTK_EDITABLE(widget),
                            GTK_EDITABLE(widget)->is_editable);
  mySetWindowText(widget->hWnd, GTK_EDITABLE(widget)->text->str);
}

void gtk_frame_realize(GtkWidget *widget)
{
  HWND Parent;
  GtkFrame *frame = GTK_FRAME(widget);

  Parent = gtk_get_parent_hwnd(widget);
  widget->hWnd = myCreateWindow("BUTTON", frame->text,
                                WS_CHILD | BS_GROUPBOX,
                                widget->allocation.x, widget->allocation.y,
                                widget->allocation.width,
                                widget->allocation.height,
                                Parent, NULL, hInst, NULL);
  gtk_set_default_font(widget->hWnd);
  gtk_container_realize(widget);
}

void gtk_check_button_realize(GtkWidget *widget)
{
  GtkButton *but = GTK_BUTTON(widget);
  HWND Parent;
  gboolean toggled;

  Parent = gtk_get_parent_hwnd(widget);
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  widget->hWnd = myCreateWindow("BUTTON", but->text,
                                WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                                widget->allocation.x, widget->allocation.y,
                                widget->allocation.width,
                                widget->allocation.height, Parent, NULL,
                                hInst, NULL);
  gtk_set_default_font(widget->hWnd);
  g_signal_connect(G_OBJECT(widget), "clicked",
                   gtk_toggle_button_toggled, NULL);
  g_signal_connect(G_OBJECT(widget), "toggled",
                   gtk_check_button_toggled, NULL);
  toggled = GTK_TOGGLE_BUTTON(widget)->toggled;
  GTK_TOGGLE_BUTTON(widget)->toggled = !toggled;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), toggled);
}

void gtk_radio_button_realize(GtkWidget *widget)
{
  GtkButton *but = GTK_BUTTON(widget);
  HWND Parent;
  gboolean toggled;

  Parent = gtk_get_parent_hwnd(widget);
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  widget->hWnd = myCreateWindow("BUTTON", but->text,
                                WS_CHILD | WS_TABSTOP | BS_RADIOBUTTON,
                                widget->allocation.x, widget->allocation.y,
                                widget->allocation.width,
                                widget->allocation.height, Parent, NULL,
                                hInst, NULL);
  gtk_set_default_font(widget->hWnd);
  g_signal_connect(G_OBJECT(widget), "clicked",
                   gtk_radio_button_clicked, NULL);
  g_signal_connect(G_OBJECT(widget), "toggled",
                   gtk_radio_button_toggled, NULL);
  toggled = GTK_TOGGLE_BUTTON(widget)->toggled;
  GTK_TOGGLE_BUTTON(widget)->toggled = !toggled;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), toggled);
}

void gtk_radio_button_destroy(GtkWidget *widget)
{
  GSList *group, *listpt;
  GtkRadioButton *radio;

  gtk_button_destroy(widget);
  group = GTK_RADIO_BUTTON(widget)->group;
  group = g_slist_remove(group, GTK_RADIO_BUTTON(widget));
  for (listpt = group; listpt; listpt = g_slist_next(listpt)) {
    radio = GTK_RADIO_BUTTON(listpt->data);
    radio->group = group;
  }
}

void gtk_container_show_all(GtkWidget *widget, gboolean hWndOnly)
{
  GtkContainer *container = GTK_CONTAINER(widget);

  if (container->child)
    gtk_widget_show_all_full(container->child, hWndOnly);
}

void gtk_container_hide_all(GtkWidget *widget, gboolean hWndOnly)
{
  GtkContainer *container = GTK_CONTAINER(widget);

  if (container->child)
    gtk_widget_hide_all_full(container->child, hWndOnly);
}

void gtk_box_show_all(GtkWidget *widget, gboolean hWndOnly)
{
  GtkBoxChild *child;
  GList *children;

  gtk_container_show_all(widget, hWndOnly);

  for (children = GTK_BOX(widget)->children; children;
       children = g_list_next(children)) {
    child = (GtkBoxChild *)(children->data);
    if (child && child->widget)
      gtk_widget_show_all_full(child->widget, hWndOnly);
  }
}

void gtk_box_hide_all(GtkWidget *widget, gboolean hWndOnly)
{
  GtkBoxChild *child;
  GList *children;

  gtk_container_hide_all(widget, hWndOnly);

  for (children = GTK_BOX(widget)->children; children;
       children = g_list_next(children)) {
    child = (GtkBoxChild *)(children->data);
    if (child && child->widget)
      gtk_widget_hide_all_full(child->widget, hWndOnly);
  }
}

void gtk_table_show_all(GtkWidget *widget, gboolean hWndOnly)
{
  GList *children;
  GtkTableChild *child;

  gtk_container_show_all(widget, hWndOnly);
  for (children = GTK_TABLE(widget)->children; children;
       children = g_list_next(children)) {
    child = (GtkTableChild *)(children->data);
    if (child && child->widget)
      gtk_widget_show_all_full(child->widget, hWndOnly);
  }
}

void gtk_table_hide_all(GtkWidget *widget, gboolean hWndOnly)
{
  GList *children;
  GtkTableChild *child;

  gtk_container_hide_all(widget, hWndOnly);
  for (children = GTK_TABLE(widget)->children; children;
       children = g_list_next(children)) {
    child = (GtkTableChild *)(children->data);
    if (child && child->widget)
      gtk_widget_hide_all_full(child->widget, hWndOnly);
  }
}

void gtk_widget_hide_all(GtkWidget *widget)
{
  gtk_widget_hide_all_full(widget, FALSE);
}

void gtk_widget_hide_all_full(GtkWidget *widget, gboolean hWndOnly)
{
  gtk_signal_emit(G_OBJECT(widget), "hide_all", hWndOnly);
  if (hWndOnly) {
    gtk_signal_emit(G_OBJECT(widget), "hide");
    if (widget->hWnd)
      ShowWindow(widget->hWnd, SW_HIDE);
  } else
    gtk_widget_hide_full(widget, FALSE);
}

void gtk_widget_show_all(GtkWidget *widget)
{
  gtk_widget_show_all_full(widget, FALSE);
}

void gtk_widget_show_all_full(GtkWidget *widget, gboolean hWndOnly)
{
  GtkAllocation alloc;
  gboolean InitWindow = FALSE;

  if (!GTK_WIDGET_REALIZED(widget) &&
      G_OBJECT(widget)->klass == &GtkWindowClass)
    InitWindow = TRUE;

  if (InitWindow)
    gtk_widget_realize(widget);

  gtk_signal_emit(G_OBJECT(widget), "show_all", hWndOnly);

  if (hWndOnly) {
    if (gtk_widget_get_visible(widget)) {
      gtk_signal_emit(G_OBJECT(widget), "show");
      if (widget->hWnd)
        ShowWindow(widget->hWnd, SW_SHOWNORMAL);
    }
  } else {
    gtk_widget_show_full(widget, FALSE);
  }

  if (InitWindow) {
    gtk_widget_update(widget, TRUE);
    alloc.x = alloc.y = 0;
    alloc.width = widget->requisition.width;
    alloc.height = widget->requisition.height;
    gtk_window_set_initial_position(GTK_WINDOW(widget), &alloc);
    ShowWindow(widget->hWnd, SW_SHOWNORMAL);
    UpdateWindow(widget->hWnd);
  }
}

GtkWidget *gtk_widget_get_ancestor(GtkWidget *widget, GtkType type)
{
  if (!widget)
    return NULL;
  while (widget && G_OBJECT(widget)->klass != type) {
    widget = widget->parent;
  }
  return widget;
}

void gtk_label_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  SIZE size;
  GtkLabel *label = GTK_LABEL(widget);

  if (GetTextSize(widget->hWnd, label->text, &size, defFont)) {
    requisition->width = size.cx;
    requisition->height = size.cy;
  }
}

void gtk_url_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  SIZE size;
  GtkLabel *label = GTK_LABEL(widget);

  if (GetTextSize(widget->hWnd, label->text, &size, urlFont)) {
    requisition->width = size.cx;
    requisition->height = size.cy;
  }
}

void gtk_label_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  gint yexcess;

  yexcess = allocation->height - widget->requisition.height;
  if (yexcess > 0) {
    allocation->y += yexcess / 2;
    allocation->height -= yexcess;
  }
}

void gtk_entry_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  gint yexcess;

  yexcess = allocation->height - widget->requisition.height;
  if (yexcess > 0) {
    allocation->y += yexcess / 2;
    allocation->height -= yexcess;
  }
}

void gtk_label_destroy(GtkWidget *widget)
{
  g_free(GTK_LABEL(widget)->text);
}

void gtk_url_destroy(GtkWidget *widget)
{
  g_free(GTK_LABEL(widget)->text);
  g_free(GTK_URL(widget)->target);
}

void gtk_label_realize(GtkWidget *widget)
{
  GtkLabel *label = GTK_LABEL(widget);
  HWND Parent;

  Parent = gtk_get_parent_hwnd(widget);
  widget->hWnd = myCreateWindow("STATIC", label->text,
                                WS_CHILD | SS_CENTER,
                                widget->allocation.x, widget->allocation.y,
                                widget->allocation.width,
                                widget->allocation.height, Parent, NULL,
                                hInst, NULL);
  gtk_set_default_font(widget->hWnd);
}

void gtk_url_realize(GtkWidget *widget)
{
  HWND Parent;

  Parent = gtk_get_parent_hwnd(widget);
  widget->hWnd = myCreateWindow(WC_GTKURL, GTK_LABEL(widget)->text,
                                WS_CHILD,
                                widget->allocation.x, widget->allocation.y,
                                widget->allocation.width,
                                widget->allocation.height, Parent, NULL,
                                hInst, NULL);
}

void gtk_container_add(GtkContainer *container, GtkWidget *widget)
{
  container->child = widget;
  widget->parent = GTK_WIDGET(container);
  if (GTK_WIDGET_REALIZED(GTK_WIDGET(container))) {
    gtk_widget_realize(widget);
    gtk_widget_update(GTK_WIDGET(container), TRUE);
  }
}

void gtk_container_set_border_width(GtkContainer *container,
                                    guint border_width)
{
  container->border_width = border_width;
}

GtkWidget *gtk_table_new(guint rows, guint cols, gboolean homogeneous)
{
  GtkTable *table;

  table = GTK_TABLE(GtkNewObject(&GtkTableClass));

  table->nrows = rows;
  table->ncols = cols;
  table->homogeneous = homogeneous;

  table->rows = g_new0(GtkTableRowCol, rows);
  table->cols = g_new0(GtkTableRowCol, cols);

  return GTK_WIDGET(table);
}

void gtk_table_resize(GtkTable *table, guint rows, guint cols)
{
  gint i;

  table->rows = g_realloc(table->rows, sizeof(GtkTableRowCol) * rows);
  table->cols = g_realloc(table->cols, sizeof(GtkTableRowCol) * cols);

  for (i = table->nrows; i < rows; i++) {
    table->rows[i].requisition = 0;
    table->rows[i].allocation = 0;
    table->rows[i].spacing = table->row_spacing;
  }
  for (i = table->ncols; i < cols; i++) {
    table->cols[i].requisition = 0;
    table->cols[i].allocation = 0;
    table->cols[i].spacing = table->column_spacing;
  }
  table->nrows = rows;
  table->ncols = cols;
  gtk_widget_update(GTK_WIDGET(table), FALSE);
}

void gtk_table_attach_defaults(GtkTable *table, GtkWidget *widget,
                               guint left_attach, guint right_attach,
                               guint top_attach, guint bottom_attach)
{
  gtk_table_attach(table, widget, left_attach, right_attach, top_attach,
                   bottom_attach, GTK_EXPAND, GTK_EXPAND, 0, 0);
}

void gtk_table_attach(GtkTable *table, GtkWidget *widget,
                      guint left_attach, guint right_attach,
                      guint top_attach, guint bottom_attach,
                      GtkAttachOptions xoptions, GtkAttachOptions yoptions,
                      guint xpadding, guint ypadding)
{
  GtkTableChild *newChild;

  newChild = g_new0(GtkTableChild, 1);

  newChild->widget = widget;
  newChild->left_attach = left_attach;
  newChild->right_attach = right_attach;
  newChild->top_attach = top_attach;
  newChild->bottom_attach = bottom_attach;
  newChild->xoptions = xoptions;
  newChild->yoptions = yoptions;

  table->children = g_list_append(table->children, (gpointer)newChild);
  widget->parent = GTK_WIDGET(table);
  if (GTK_WIDGET_REALIZED(GTK_WIDGET(table))) {
    gtk_widget_realize(widget);
    gtk_widget_update(GTK_WIDGET(table), TRUE);
  }
}

void gtk_table_destroy(GtkWidget *widget)
{
  GList *children;
  GtkTableChild *child;

  gtk_container_destroy(widget);
  for (children = GTK_TABLE(widget)->children; children;
       children = g_list_next(children)) {
    child = (GtkTableChild *)(children->data);
    if (child->widget)
      gtk_widget_destroy(child->widget);
    g_free(child);
  }
  g_list_free(GTK_TABLE(widget)->children);
}

static void gtk_table_get_size_children(GtkTable *table,
                                        GtkAttachOptions colopts,
                                        GtkAttachOptions *cols)
{
  GList *children;

  for (children = table->children; children;
       children = g_list_next(children)) {
    GtkRequisition child_req;
    GtkWidget *child_wid;
    int i;
    GtkTableChild *child = (GtkTableChild *)(children->data);

    if (!child)
      continue;
    if (colopts) {
      if (child->xoptions & colopts) {
        for (i = child->left_attach; i < child->right_attach; i++) {
          cols[i] = colopts;
        }
      } else {
        continue;
      }
    } else if (child->xoptions & GTK_SHRINK) {
      continue;
    }
    child_wid = child->widget;
    if (child_wid && child->left_attach < child->right_attach
        && child->top_attach < child->bottom_attach
        && gtk_widget_get_visible(child_wid)) {
      child_req.width = child_wid->requisition.width;
      child_req.height = child_wid->requisition.height;
      child_req.height /= (child->bottom_attach - child->top_attach);
      if (colopts) {
        child_req.width /= (child->right_attach - child->left_attach);
        for (i = child->left_attach; i < child->right_attach; i++) {
          if (child_req.width > table->cols[i].requisition)
            table->cols[i].requisition = child_req.width;
        }
      } else {
        for (i = child->left_attach; i < child->right_attach; i++) {
          child_req.width -= table->cols[i].requisition;
        }
        if (child_req.width > 0) {
          int nexpand = 0, ntotal = child->right_attach - child->left_attach;
          for (i = child->left_attach; i < child->right_attach; i++) {
            if (cols[i] == 0) {
              nexpand++;
            }
          }
          if (nexpand) {
            for (i = child->left_attach; i < child->right_attach; i++) {
              if (cols[i] == 0) {
                table->cols[i].requisition += child_req.width / nexpand;
              }
            }
          } else {
            for (i = child->left_attach; i < child->right_attach; i++) {
              table->cols[i].requisition += child_req.width / ntotal;
            }
          }
        }
      }
      for (i = child->top_attach; i < child->bottom_attach; i++) {
        if (child_req.height > table->rows[i].requisition)
          table->rows[i].requisition = child_req.height;
      }
    }
  }
}

void gtk_table_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  GtkTable *table;
  gint16 MaxReq;
  int i;
  GtkAttachOptions *cols;

  table = GTK_TABLE(widget);
  for (i = 0; i < table->ncols; i++)
    table->cols[i].requisition = 0;
  for (i = 0; i < table->nrows; i++)
    table->rows[i].requisition = 0;

  gtk_container_size_request(widget, requisition);
  cols = g_new0(GtkAttachOptions, table->ncols);
  if (!table->homogeneous) {
    gtk_table_get_size_children(table, GTK_SHRINK, cols);
  }
  gtk_table_get_size_children(table, 0, cols);
  g_free(cols);

  if (table->homogeneous) {
    MaxReq = 0;
    for (i = 0; i < table->ncols; i++)
      if (table->cols[i].requisition > MaxReq) {
        MaxReq = table->cols[i].requisition;
      }
    for (i = 0; i < table->ncols; i++)
      table->cols[i].requisition = MaxReq;

    MaxReq = 0;
    for (i = 0; i < table->nrows; i++)
      if (table->rows[i].requisition > MaxReq) {
        MaxReq = table->rows[i].requisition;
      }
    for (i = 0; i < table->nrows; i++)
      table->rows[i].requisition = MaxReq;
  }

  requisition->width = requisition->height =
      2 * GTK_CONTAINER(widget)->border_width;

  for (i = 0; i < table->ncols; i++)
    requisition->width += table->cols[i].requisition;
  for (i = 0; i < table->ncols - 1; i++)
    requisition->width += table->cols[i].spacing;
  for (i = 0; i < table->nrows; i++)
    requisition->height += table->rows[i].requisition;
  for (i = 0; i < table->nrows - 1; i++)
    requisition->height += table->rows[i].spacing;

}

void gtk_table_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkTable *table;
  gint row_extra = 0, col_extra = 0, i, row_expand = 0, col_expand = 0;
  GtkAllocation child_alloc;
  GList *children;
  GtkTableChild *child;
  gint border_width;
  GtkAttachOptions *rowopt, *colopt;

  table = GTK_TABLE(widget);
  border_width = GTK_CONTAINER(widget)->border_width;

  rowopt = g_new0(GtkAttachOptions, table->nrows);
  colopt = g_new0(GtkAttachOptions, table->ncols);
  for (children = table->children; children;
       children = g_list_next(children)) {
    GtkTableChild *child = (GtkTableChild *)(children->data);
    GtkWidget *child_wid;

    if (!child)
      continue;
    child_wid = child->widget;
    if (child_wid && child->left_attach < child->right_attach &&
        child->top_attach < child->bottom_attach &&
        gtk_widget_get_visible(child_wid)) {
      if (child->xoptions & GTK_SHRINK) {
        for (i = child->left_attach; i < child->right_attach; i++) {
          colopt[i] |= GTK_SHRINK;
        }
      }
      if (child->yoptions & GTK_EXPAND) {
        for (i = child->top_attach; i < child->bottom_attach; i++) {
          rowopt[i] |= GTK_EXPAND;
        }
      }
    }
  }

  for (i = 0; i < table->ncols; i++) {
    if (!(colopt[i] & GTK_SHRINK)) {
      col_expand++;
    }
  }
  for (i = 0; i < table->nrows; i++) {
    if (rowopt[i] & GTK_EXPAND) {
      row_expand++;
    }
  }

  if (col_expand) {
    col_extra =
        (allocation->width - widget->requisition.width) / col_expand;
  }
  if (row_expand) {
    row_extra =
        (allocation->height - widget->requisition.height) / row_expand;
  }
  for (i = 0; i < table->ncols; i++) {
    table->cols[i].allocation = table->cols[i].requisition +
                                (colopt[i] & GTK_SHRINK ? 0 : col_extra);
  }
  for (i = 0; i < table->nrows; i++) {
    table->rows[i].allocation = table->rows[i].requisition +
                                (rowopt[i] & GTK_SHRINK ? 0 : row_extra);
  }
  g_free(rowopt);
  g_free(colopt);
  for (children = table->children; children;
       children = g_list_next(children)) {
    child = (GtkTableChild *)(children->data);
    if (!child || !child->widget || !gtk_widget_get_visible(child->widget))
      continue;
    child_alloc.x = allocation->x + border_width;
    child_alloc.y = allocation->y + border_width;
    child_alloc.width = child_alloc.height = 0;
    for (i = 0; i < child->left_attach; i++) {
      child_alloc.x += table->cols[i].allocation + table->cols[i].spacing;
    }
    for (i = 0; i < child->top_attach; i++) {
      child_alloc.y += table->rows[i].allocation + table->rows[i].spacing;
    }
    for (i = child->left_attach; i < child->right_attach; i++) {
      child_alloc.width += table->cols[i].allocation;
      if (i < child->right_attach - 1) {
        child_alloc.width += table->cols[i].spacing;
      }
    }
    for (i = child->top_attach; i < child->bottom_attach; i++) {
      child_alloc.height += table->rows[i].allocation;
      if (i < child->bottom_attach - 1) {
        child_alloc.height += table->rows[i].spacing;
      }
    }
    gtk_widget_set_size(child->widget, &child_alloc);
  }
}

void gtk_table_realize(GtkWidget *widget)
{
  GList *children;
  GtkTableChild *child;

  gtk_container_realize(widget);
  for (children = GTK_TABLE(widget)->children; children;
       children = g_list_next(children)) {
    child = (GtkTableChild *)(children->data);
    if (child->widget)
      gtk_widget_realize(child->widget);
  }
}

void gtk_table_set_row_spacing(GtkTable *table, guint row, guint spacing)
{
  if (table && row >= 0 && row < table->nrows) {
    table->rows[row].spacing = spacing;
  }
}

void gtk_table_set_col_spacing(GtkTable *table, guint column,
                               guint spacing)
{
  if (table && column >= 0 && column < table->ncols) {
    table->cols[column].spacing = spacing;
  }
}

void gtk_table_set_row_spacings(GtkTable *table, guint spacing)
{
  int i;

  table->row_spacing = spacing;
  for (i = 0; i < table->nrows; i++)
    table->rows[i].spacing = spacing;
}

void gtk_table_set_col_spacings(GtkTable *table, guint spacing)
{
  int i;

  table->column_spacing = spacing;
  for (i = 0; i < table->ncols; i++)
    table->cols[i].spacing = spacing;
}

void gtk_toggle_button_toggled(GtkToggleButton *toggle_button)
{
  toggle_button->toggled = !toggle_button->toggled;
  gtk_signal_emit(G_OBJECT(toggle_button), "toggled");
}

void gtk_check_button_toggled(GtkCheckButton *check_button, gpointer data)
{
  HWND hWnd;
  gboolean is_active = GTK_TOGGLE_BUTTON(check_button)->toggled;

  hWnd = GTK_WIDGET(check_button)->hWnd;
  if (hWnd) {
    mySendMessage(hWnd, BM_SETCHECK,
                  is_active ? BST_CHECKED : BST_UNCHECKED, 0);
  }
}

void gtk_radio_button_clicked(GtkRadioButton *radio_button, gpointer data)
{
  GtkToggleButton *toggle = GTK_TOGGLE_BUTTON(radio_button);

  if (toggle->toggled)
    return;
  else
    gtk_toggle_button_toggled(toggle);
}

void gtk_radio_button_toggled(GtkRadioButton *radio_button, gpointer data)
{
  HWND hWnd;
  GSList *group;
  GtkRadioButton *radio;
  gboolean is_active = GTK_TOGGLE_BUTTON(radio_button)->toggled;

  hWnd = GTK_WIDGET(radio_button)->hWnd;
  if (hWnd) {
    mySendMessage(hWnd, BM_SETCHECK,
                  is_active ? BST_CHECKED : BST_UNCHECKED, 0);
  }
  if (is_active) {
    for (group = radio_button->group; group; group = g_slist_next(group)) {
      radio = GTK_RADIO_BUTTON(group->data);
      if (radio && radio != radio_button) {
        GTK_TOGGLE_BUTTON(radio)->toggled = FALSE;
        hWnd = GTK_WIDGET(radio)->hWnd;
        if (hWnd)
          mySendMessage(hWnd, BM_SETCHECK, BST_UNCHECKED, 0);
      }
    }
  }
}

gboolean gtk_toggle_button_get_active(GtkToggleButton *toggle_button)
{
  return (toggle_button->toggled);
}

void gtk_toggle_button_set_active(GtkToggleButton *toggle_button,
                                  gboolean is_active)
{
  if (toggle_button->toggled == is_active)
    return;
  else
    gtk_toggle_button_toggled(toggle_button);
}

void gtk_main_quit()
{
  PostQuitMessage(0);
}

void gtk_main()
{
  MSG msg;
  GSList *list;
  BOOL MsgDone;
  GtkWidget *widget, *window;
  HACCEL hAccel;

  RecurseLevel++;

  while (myGetMessage(&msg, NULL, 0, 0)) {
    MsgDone = FALSE;
    widget = GTK_WIDGET(myGetWindowLong(msg.hwnd, GWLP_USERDATA));
    window = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
    if (window) {
      hAccel = GTK_WINDOW(window)->hAccel;
      if (hAccel) {
        MsgDone = TranslateAccelerator(window->hWnd, hAccel, &msg);
      }
    }
    if (!MsgDone)
      for (list = WindowList; list && !MsgDone; list = g_slist_next(list)) {
        widget = GTK_WIDGET(list->data);
        if (widget && widget->hWnd
            && (MsgDone = myIsDialogMessage(widget->hWnd, &msg)) == TRUE)
          break;
      }
    if (!MsgDone) {
      TranslateMessage(&msg);
      myDispatchMessage(&msg);
    }
  }
  RecurseLevel--;
}

typedef struct _GtkSignal GtkSignal;

struct _GtkSignal {
  GCallback func;
  GObject *slot_object;
  gpointer func_data;
};

typedef gint (*GtkGIntSignalFunc) ();

void gtk_marshal_BOOL__GINT(GObject *object, GSList *actions,
                            GCallback default_action, va_list args)
{
  gboolean *retval;
  gint arg1;
  GtkSignal *signal;
  GtkGIntSignalFunc sigfunc;

  arg1 = va_arg(args, gint);
  retval = va_arg(args, gboolean *);

  if (!retval) {
    g_warning("gtk_marshal_BOOL__GINT: retval NULL");
    return;
  }

  while (actions) {
    signal = (GtkSignal *)actions->data;
    sigfunc = (GtkGIntSignalFunc)signal->func;
    if (signal->slot_object) {
      *retval = (*sigfunc) (signal->slot_object, arg1);
    } else
      *retval = (*sigfunc) (object, arg1, signal->func_data);
    if (*retval)
      return;
    actions = g_slist_next(actions);
  }
  sigfunc = (GtkGIntSignalFunc)default_action;
  if (sigfunc)
    *retval = (*sigfunc) (object, arg1);
}

void gtk_marshal_BOOL__GPOIN(GObject *object, GSList *actions,
                             GCallback default_action, va_list args)
{
  gboolean *retval;
  gpointer arg1;
  GtkSignal *signal;
  GtkGIntSignalFunc sigfunc;

  arg1 = va_arg(args, gpointer);
  retval = va_arg(args, gboolean *);

  if (!retval) {
    g_warning("gtk_marshal_BOOL__GPOIN: retval NULL");
    return;
  }

  while (actions) {
    signal = (GtkSignal *)actions->data;
    sigfunc = (GtkGIntSignalFunc)signal->func;
    if (signal->slot_object) {
      *retval = (*sigfunc) (signal->slot_object, arg1);
    } else
      *retval = (*sigfunc) (object, arg1, signal->func_data);
    if (*retval)
      return;
    actions = g_slist_next(actions);
  }
  sigfunc = (GtkGIntSignalFunc)default_action;
  if (sigfunc)
    *retval = (*sigfunc) (object, arg1);
}

void gtk_marshal_VOID__VOID(GObject *object, GSList *actions,
                            GCallback default_action, va_list args)
{
  GtkSignal *signal;

  while (actions) {
    signal = (GtkSignal *)actions->data;
    if (signal->slot_object) {
      (*signal->func) (signal->slot_object);
    } else
      (*signal->func) (object, signal->func_data);
    actions = g_slist_next(actions);
  }
  if (default_action)
    (*default_action) (object);
}

void gtk_marshal_VOID__GINT(GObject *object, GSList *actions,
                            GCallback default_action, va_list args)
{
  gint arg1;
  GtkSignal *signal;

  arg1 = va_arg(args, gint);

  while (actions) {
    signal = (GtkSignal *)actions->data;
    if (signal->slot_object) {
      (*signal->func) (signal->slot_object, arg1);
    } else
      (*signal->func) (object, arg1, signal->func_data);
    actions = g_slist_next(actions);
  }
  if (default_action)
    (*default_action) (object, arg1);
}

void gtk_marshal_VOID__GPOIN(GObject *object, GSList *actions,
                             GCallback default_action, va_list args)
{
  gpointer arg1;
  GtkSignal *signal;

  arg1 = va_arg(args, gpointer);

  while (actions) {
    signal = (GtkSignal *)actions->data;
    if (signal->slot_object) {
      (*signal->func) (signal->slot_object, arg1);
    } else
      (*signal->func) (object, arg1, signal->func_data);
    actions = g_slist_next(actions);
  }
  if (default_action)
    (*default_action) (object, arg1);
}

void gtk_marshal_VOID__BOOL(GObject *object, GSList *actions,
                            GCallback default_action, va_list args)
{
  gboolean arg1;
  GtkSignal *signal;

  arg1 = va_arg(args, gboolean);

  while (actions) {
    signal = (GtkSignal *)actions->data;
    if (signal->slot_object) {
      (*signal->func) (signal->slot_object, arg1);
    } else
      (*signal->func) (object, arg1, signal->func_data);
    actions = g_slist_next(actions);
  }
  if (default_action)
    (*default_action) (object, arg1);
}

void gtk_marshal_VOID__GINT_GINT_EVENT(GObject *object, GSList *actions,
                                       GCallback default_action,
                                       va_list args)
{
  gint arg1, arg2;
  GdkEvent *arg3;
  GtkSignal *signal;

  arg1 = va_arg(args, gint);
  arg2 = va_arg(args, gint);
  arg3 = va_arg(args, GdkEvent *);

  while (actions) {
    signal = (GtkSignal *)actions->data;
    if (signal->slot_object) {
      (*signal->func) (signal->slot_object, arg1, arg2, arg3);
    } else
      (*signal->func) (object, arg1, arg2, arg3, signal->func_data);
    actions = g_slist_next(actions);
  }
  if (default_action)
    (*default_action) (object, arg1, arg2, arg3);
}

static GtkSignalType *gtk_get_signal_type(GObject *object,
                                          const gchar *name)
{
  GtkClass *klass;
  GtkSignalType *signals;

  for (klass = object->klass; klass; klass = klass->parent) {
    for (signals = klass->signals; signals && signals->name[0]; signals++) {
      if (strcmp(signals->name, name) == 0)
        return signals;
    }
  }
  return NULL;
}

void gtk_signal_emit(GObject *object, const gchar *name, ...)
{
  GSList *signal_list;
  GtkSignalType *signal_type;
  va_list ap;

  if (!object)
    return;

  va_start(ap, name);
  signal_list = (GSList *)g_datalist_get_data(&object->signals, name);
  signal_type = gtk_get_signal_type(object, name);
  if (signal_type && signal_type->marshaller) {
    (*signal_type->marshaller) (object, signal_list,
                                signal_type->default_action, ap);
  }
  va_end(ap);
  if (!signal_type)
    g_warning("gtk_signal_emit: unknown signal %s", name);
}

guint g_signal_connect(GObject *object, const gchar *name,
                       GCallback func, gpointer func_data)
{
  GtkSignal *signal;
  GtkSignalType *signal_type;
  GSList *signal_list;

  if (!object)
    return 0;
  signal_type = gtk_get_signal_type(object, name);
  if (!signal_type) {
    g_warning("g_signal_connect: unknown signal %s", name);
    return 0;
  }
  signal_list = (GSList *)g_datalist_get_data(&object->signals, name);
  signal = g_new0(GtkSignal, 1);

  signal->func = func;
  signal->func_data = func_data;
  signal_list = g_slist_append(signal_list, signal);
  g_datalist_set_data(&object->signals, name, signal_list);
  return 0;
}

guint g_signal_connect_swapped(GObject *object, const gchar *name,
                               GCallback func, GObject *slot_object)
{
  GtkSignal *signal;
  GtkSignalType *signal_type;
  GSList *signal_list;

  if (!object)
    return 0;
  signal_type = gtk_get_signal_type(object, name);
  if (!signal_type) {
    g_warning("g_signal_connect_swapped: unknown signal %s", name);
    return 0;
  }
  signal_list = (GSList *)g_datalist_get_data(&object->signals, name);
  signal = g_new0(GtkSignal, 1);

  signal->func = func;
  signal->slot_object = slot_object;
  signal_list = g_slist_append(signal_list, signal);
  g_datalist_set_data(&object->signals, name, signal_list);
  return 0;
}

void gtk_menu_shell_insert(GtkMenuShell *menu_shell, GtkWidget *child,
                           gint position)
{
  menu_shell->children =
      g_slist_insert(menu_shell->children, (gpointer)child, position);
  child->parent = GTK_WIDGET(menu_shell);
}

void gtk_menu_shell_append(GtkMenuShell *menu_shell, GtkWidget *child)
{
  gtk_menu_shell_insert(menu_shell, child, -1);
}

void gtk_menu_shell_prepend(GtkMenuShell *menu_shell, GtkWidget *child)
{
  gtk_menu_shell_insert(menu_shell, child, 0);
}

GtkWidget *gtk_menu_bar_new()
{
  GtkMenuBar *menu_bar;

  menu_bar = GTK_MENU_BAR(GtkNewObject(&GtkMenuBarClass));
  return GTK_WIDGET(menu_bar);
}

void gtk_menu_bar_insert(GtkMenuBar *menu_bar, GtkWidget *child,
                         gint position)
{
  gtk_menu_shell_insert(GTK_MENU_SHELL(menu_bar), child, position);
}

void gtk_menu_bar_append(GtkMenuBar *menu_bar, GtkWidget *child)
{
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), child);
}

void gtk_menu_bar_prepend(GtkMenuBar *menu_bar, GtkWidget *child)
{
  gtk_menu_shell_prepend(GTK_MENU_SHELL(menu_bar), child);
}

GtkWidget *gtk_menu_new()
{
  GtkMenu *menu;

  menu = GTK_MENU(GtkNewObject(&GtkMenuClass));
  return GTK_WIDGET(menu);
}

void gtk_menu_insert(GtkMenu *menu, GtkWidget *child, gint position)
{
  gtk_menu_shell_insert(GTK_MENU_SHELL(menu), child, position);
}

void gtk_menu_append(GtkMenu *menu, GtkWidget *child)
{
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), child);
}

void gtk_menu_prepend(GtkMenu *menu, GtkWidget *child)
{
  gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), child);
}

GtkWidget *gtk_menu_item_new_with_label(const gchar *label)
{
  GtkMenuItem *menu_item;

  menu_item = GTK_MENU_ITEM(GtkNewObject(&GtkMenuItemClass));
  menu_item->accelind = -1;
  menu_item->text = g_strdup(label);
  menu_item->check = 0;
  menu_item->active = 0;
  g_strdelimit(menu_item->text, "_", '&');
  return GTK_WIDGET(menu_item);
}

void gtk_menu_item_set_right_justified(GtkMenuItem *menu_item,
                                       gboolean right_justified)
{
  /* noop */
}

void gtk_menu_item_set_submenu(GtkMenuItem *menu_item, GtkWidget *submenu)
{
  menu_item->submenu = GTK_MENU(submenu);
  submenu->parent = GTK_WIDGET(menu_item);
}

GtkMenu *gtk_menu_item_get_submenu(GtkMenuItem *menu_item)
{
  return menu_item->submenu;
}

gboolean gtk_check_menu_item_get_active(GtkMenuItem *menu_item)
{
  return menu_item->active;
}

void gtk_check_menu_item_set_active(GtkMenuItem *menu_item, gboolean active)
{
  GtkWidget *widget = GTK_WIDGET(menu_item);
  menu_item->active = active;

  if (GTK_WIDGET_REALIZED(widget)) {
    MENUITEMINFO mii;
    HMENU parent_menu;

    parent_menu = GTK_MENU_SHELL(widget->parent)->menu;
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;
    mii.fState = active ? MFS_CHECKED : MFS_UNCHECKED;
    mySetMenuItemInfo(parent_menu, menu_item->ID, FALSE, &mii);
  }
}

static GtkWidget *gtk_menu_item_get_menu_ID(GtkMenuItem *menu_item,
                                            gint ID)
{
  if (menu_item->ID == ID) {
    return GTK_WIDGET(menu_item);
  } else if (menu_item->submenu) {
    return gtk_menu_shell_get_menu_ID(GTK_MENU_SHELL(menu_item->submenu),
                                      ID);
  } else
    return NULL;
}

GtkWidget *gtk_menu_shell_get_menu_ID(GtkMenuShell *menu_shell, gint ID)
{
  GSList *list;
  GtkWidget *menu_item;

  for (list = menu_shell->children; list; list = list->next) {
    menu_item = gtk_menu_item_get_menu_ID(GTK_MENU_ITEM(list->data), ID);
    if (menu_item)
      return menu_item;
  }
  return NULL;
}

GtkWidget *gtk_window_get_menu_ID(GtkWindow *window, gint ID)
{
  if (window->menu_bar) {
    return gtk_menu_shell_get_menu_ID(GTK_MENU_SHELL(window->menu_bar),
                                      ID);
  } else
    return NULL;
}

void gtk_menu_bar_realize(GtkWidget *widget)
{
  GtkMenuBar *menu_bar = GTK_MENU_BAR(widget);
  GtkWidget *window;

  GTK_MENU_SHELL(widget)->menu = CreateMenu();
  menu_bar->LastID = 1000;

  gtk_menu_shell_realize(widget);

  window = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
  gtk_window_set_menu(GTK_WINDOW(window), menu_bar);
}

void gtk_menu_item_realize(GtkWidget *widget)
{
  GtkMenuItem *menu_item = GTK_MENU_ITEM(widget);
  MENUITEMINFO mii;
  GtkWidget *menu_bar, *window;
  HMENU parent_menu;
  gint pos;

  menu_bar = gtk_widget_get_ancestor(widget, GTK_TYPE_MENU_BAR);
  if (menu_bar)
    menu_item->ID = GTK_MENU_BAR(menu_bar)->LastID++;

  if (menu_item->accelind >= 0) {
    window = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
    if (window && GTK_WINDOW(window)->accel_group) {
      gtk_accel_group_set_id(GTK_WINDOW(window)->accel_group,
                             menu_item->accelind, menu_item->ID);
    }
  }

  if (menu_item->submenu)
    gtk_widget_realize(GTK_WIDGET(menu_item->submenu));

  parent_menu = GTK_MENU_SHELL(widget->parent)->menu;
  pos = g_slist_index(GTK_MENU_SHELL(widget->parent)->children, widget);

  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
  if (menu_item->submenu) {
    mii.fMask |= MIIM_SUBMENU;
    mii.hSubMenu = GTK_MENU_SHELL(menu_item->submenu)->menu;
  }
  mii.fType = MFT_STRING;
  if (GTK_WIDGET_SENSITIVE(widget)) {
    mii.fState = MFS_ENABLED;
  } else {
    mii.fState = MFS_GRAYED;
  }
  if (menu_item->check) {
    mii.fState |= (menu_item->active ? MFS_CHECKED : MFS_UNCHECKED);
  }
  mii.wID = menu_item->ID;
  mii.dwTypeData = (LPTSTR)menu_item->text;
  mii.cch = myw32strlen(menu_item->text);
  myInsertMenuItem(parent_menu, pos, TRUE, &mii);
}

void gtk_menu_realize(GtkWidget *widget)
{
  GTK_MENU_SHELL(widget)->menu = CreatePopupMenu();
  gtk_menu_shell_realize(widget);
}

void gtk_menu_set_active(GtkMenu *menu, guint index)
{
  menu->active = index;
}

void gtk_menu_shell_realize(GtkWidget *widget)
{
  GSList *children;
  GtkMenuShell *menu = GTK_MENU_SHELL(widget);

  for (children = menu->children; children;
       children = g_slist_next(children)) {
    gtk_widget_realize(GTK_WIDGET(children->data));
  }
}

void gtk_menu_item_enable(GtkWidget *widget)
{
  GtkWidget *parent;
  HMENU hMenu;
  HWND hWnd;

  parent = widget->parent;
  if (!parent)
    return;
  hMenu = GTK_MENU_SHELL(parent)->menu;
  if (hMenu)
    EnableMenuItem(hMenu, GTK_MENU_ITEM(widget)->ID,
                   MF_BYCOMMAND | MF_ENABLED);
  hWnd = gtk_get_parent_hwnd(widget);
  if (hWnd)
    DrawMenuBar(hWnd);
}

void gtk_menu_item_disable(GtkWidget *widget)
{
  GtkWidget *parent;
  HMENU hMenu;
  HWND hWnd;

  parent = widget->parent;
  if (!parent)
    return;
  hMenu = GTK_MENU_SHELL(parent)->menu;
  if (hMenu)
    EnableMenuItem(hMenu, GTK_MENU_ITEM(widget)->ID,
                   MF_BYCOMMAND | MF_GRAYED);
  hWnd = gtk_get_parent_hwnd(widget);
  if (hWnd)
    DrawMenuBar(hWnd);
}

GtkWidget *gtk_notebook_new()
{
  GtkNotebook *notebook;

  notebook = GTK_NOTEBOOK(GtkNewObject(&GtkNotebookClass));
  return GTK_WIDGET(notebook);
}

void gtk_notebook_append_page(GtkNotebook *notebook, GtkWidget *child,
                              GtkWidget *tab_label)
{
  gtk_notebook_insert_page(notebook, child, tab_label, -1);
}

void gtk_notebook_insert_page(GtkNotebook *notebook, GtkWidget *child,
                              GtkWidget *tab_label, gint position)
{
  GtkNotebookChild *note_child;
  GtkWidget *widget = GTK_WIDGET(notebook);

  note_child = g_new0(GtkNotebookChild, 1);

  note_child->child = child;
  note_child->tab_label = tab_label;
  note_child->tabpage = myCreateDialog(hInst, "tabpage",
                                       gtk_get_parent_hwnd(widget->parent),
                                       MainDlgProc);
  EnableThemeDialogTexture(note_child->tabpage, ETDT_ENABLETAB);
  notebook->children =
      g_slist_insert(notebook->children, note_child, position);
  child->parent = GTK_WIDGET(notebook);
}

void gtk_notebook_set_current_page(GtkNotebook *notebook, gint page_num)
{
  GSList *children;
  GtkNotebookChild *note_child;
  GtkWidget *widget = GTK_WIDGET(notebook);
  gint pos = 0;

  if (page_num < 0)
    page_num = g_slist_length(notebook->children) - 1;
  notebook->selection = page_num;

  if (GTK_WIDGET_REALIZED(widget)) {
    if (widget->hWnd)
      TabCtrl_SetCurSel(widget->hWnd, page_num);
    for (children = notebook->children; children;
         children = g_slist_next(children)) {
      note_child = (GtkNotebookChild *)(children->data);
      if (note_child && note_child->child) {
        if (pos == page_num) {
          ShowWindow(note_child->tabpage, SW_SHOW);
        } else {
          ShowWindow(note_child->tabpage, SW_HIDE);
        }
        pos++;
      }
    }
  }
}

gint gtk_notebook_get_current_page(GtkNotebook *notebook)
{
  return notebook->selection;
}

void gtk_notebook_realize(GtkWidget *widget)
{
  GSList *children;
  GtkNotebookChild *note_child;
  HWND Parent;
  gint tab_pos = 0;
  TC_ITEM tie;

  Parent = gtk_get_parent_hwnd(widget->parent);
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  widget->hWnd = myCreateWindow(WC_TABCONTROL, "",
                                WS_CHILD | WS_TABSTOP, 0, 0, 0, 0,
                                Parent, NULL, hInst, NULL);
  if (widget->hWnd == NULL)
    g_print("Error creating window!\n");
  gtk_set_default_font(widget->hWnd);

  tie.mask = TCIF_TEXT | TCIF_IMAGE;
  tie.iImage = -1;

  for (children = GTK_NOTEBOOK(widget)->children; children;
       children = g_slist_next(children)) {
    note_child = (GtkNotebookChild *)(children->data);
    if (note_child) {
      if (note_child->tab_label)
        tie.pszText = GTK_LABEL(note_child->tab_label)->text;
      else
        tie.pszText = "No label";
      myTabCtrl_InsertItem(widget->hWnd, tab_pos++, &tie);
      note_child->tabpage = myCreateDialog(hInst, "tabpage",
                                           gtk_get_parent_hwnd(widget->parent),
                                           MainDlgProc);
      EnableThemeDialogTexture(note_child->tabpage, ETDT_ENABLETAB);
      if (note_child->child) {
        gtk_widget_realize(note_child->child);
      }
    }
  }
  gtk_notebook_set_current_page(GTK_NOTEBOOK(widget),
                                GTK_NOTEBOOK(widget)->selection);
}

void gtk_notebook_show_all(GtkWidget *widget, gboolean hWndOnly)
{
  GSList *children;
  GtkNotebookChild *note_child;

  if (!hWndOnly)
    for (children = GTK_NOTEBOOK(widget)->children; children;
         children = g_slist_next(children)) {
      note_child = (GtkNotebookChild *)(children->data);
      if (note_child && note_child->child)
        gtk_widget_show_all_full(note_child->child, hWndOnly);
    }
  gtk_notebook_set_current_page(GTK_NOTEBOOK(widget),
                                GTK_NOTEBOOK(widget)->selection);
}

void gtk_notebook_hide_all(GtkWidget *widget, gboolean hWndOnly)
{
  GSList *children;
  GtkNotebookChild *note_child;

  for (children = GTK_NOTEBOOK(widget)->children; children;
       children = g_slist_next(children)) {
    note_child = (GtkNotebookChild *)(children->data);
    if (note_child && note_child->child)
      gtk_widget_hide_all_full(note_child->child, hWndOnly);
    ShowWindow(note_child->tabpage, SW_HIDE);
  }
}

void gtk_notebook_destroy(GtkWidget *widget)
{
  GSList *children;
  GtkNotebookChild *note_child;

  for (children = GTK_NOTEBOOK(widget)->children; children;
       children = g_slist_next(children)) {
    note_child = (GtkNotebookChild *)(children->data);
    if (note_child) {
      gtk_widget_destroy(note_child->child);
      gtk_widget_destroy(note_child->tab_label);
      DestroyWindow(note_child->tabpage);
    }
    g_free(note_child);
  }
  g_slist_free(GTK_NOTEBOOK(widget)->children);
}

void gtk_notebook_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GSList *children;
  RECT rect;

  gtk_container_set_size(widget, allocation);
  rect.left = allocation->x;
  rect.top = allocation->y;
  rect.right = allocation->x + allocation->width;
  rect.bottom = allocation->y + allocation->height;
  TabCtrl_AdjustRect(widget->hWnd, FALSE, &rect);

  for (children = GTK_NOTEBOOK(widget)->children; children;
       children = g_slist_next(children)) {
    GtkNotebookChild *note_child = (GtkNotebookChild *)(children->data);
    if (note_child && note_child->child) {
      GtkAllocation child_alloc;

      child_alloc.x = rect.left + GTK_CONTAINER(widget)->border_width;
      child_alloc.y = rect.top + GTK_CONTAINER(widget)->border_width;
      child_alloc.width = rect.right - rect.left
                          - 2 * GTK_CONTAINER(widget)->border_width;
      child_alloc.height = rect.bottom - rect.top
                           - 2 * GTK_CONTAINER(widget)->border_width;
      SetWindowPos(note_child->tabpage, HWND_TOP,
                   rect.left, rect.top, rect.right - rect.left,
                   rect.bottom - rect.top, SWP_NOZORDER);
      gtk_widget_set_size(note_child->child, &child_alloc);
    }
  }
}

void gtk_notebook_size_request(GtkWidget *widget,
                               GtkRequisition *requisition)
{
  GSList *children;
  GtkNotebookChild *note_child;
  GtkRequisition *child_req;
  RECT rect;

  requisition->width = requisition->height = 0;
  for (children = GTK_NOTEBOOK(widget)->children; children;
       children = g_slist_next(children)) {
    note_child = (GtkNotebookChild *)(children->data);
    if (note_child && note_child->child &&
        gtk_widget_get_visible(note_child->child)) {
      child_req = &note_child->child->requisition;
      if (child_req->width > requisition->width)
        requisition->width = child_req->width;
      if (child_req->height > requisition->height)
        requisition->height = child_req->height;
    }
  }
  requisition->width += GTK_CONTAINER(widget)->border_width * 2;
  requisition->height += GTK_CONTAINER(widget)->border_width * 2;
  rect.left = rect.top = 0;
  rect.right = requisition->width;
  rect.bottom = requisition->height;
  TabCtrl_AdjustRect(widget->hWnd, TRUE, &rect);
  requisition->width = rect.right - rect.left;
  requisition->height = rect.bottom - rect.top;
}

GObject *gtk_adjustment_new(gfloat value, gfloat lower, gfloat upper,
                            gfloat step_increment, gfloat page_increment,
                            gfloat page_size)
{
  GtkAdjustment *adj;

  adj = (GtkAdjustment *)(GtkNewObject(&GtkAdjustmentClass));

  adj->value = value;
  adj->lower = lower;
  adj->upper = upper;
  adj->step_increment = step_increment;
  adj->page_increment = page_increment;
  adj->page_size = page_size;

  return G_OBJECT(adj);
}

GtkWidget *gtk_spin_button_new(GtkAdjustment *adjustment,
                               gfloat climb_rate, guint digits)
{
  GtkSpinButton *spin;

  spin = GTK_SPIN_BUTTON(GtkNewObject(&GtkSpinButtonClass));
  GTK_ENTRY(spin)->is_visible = TRUE;

  gtk_spin_button_set_adjustment(spin, adjustment);

  return GTK_WIDGET(spin);
}

void gtk_spin_button_size_request(GtkWidget *widget,
                                  GtkRequisition *requisition)
{
  gtk_entry_size_request(widget, requisition);
}

void gtk_spin_button_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  int udwidth;
  HWND updown;

  udwidth = GetSystemMetrics(SM_CXVSCROLL);
  allocation->width -= udwidth;

  updown = GTK_SPIN_BUTTON(widget)->updown;
  if (updown) {
    POINT pt;
    pt.x = allocation->x;
    pt.y = allocation->y;
    MapWidgetOrigin(widget, &pt);
    SetWindowPos(updown, HWND_TOP, pt.x + allocation->width, pt.y,
                 udwidth, allocation->height, SWP_NOZORDER);
  }
}

gint gtk_spin_button_get_value_as_int(GtkSpinButton *spin_button)
{
  HWND hWnd;
  LRESULT lres;

  hWnd = spin_button->updown;
  if (hWnd) {
    lres = mySendMessage(hWnd, UDM_GETPOS, 0, 0);
    if (HIWORD(lres) != 0)
      return 0;
    else
      return (gint)LOWORD(lres);
  } else
    return (gint)spin_button->adj->value;
}

void gtk_spin_button_set_value(GtkSpinButton *spin_button, gfloat value)
{
  HWND hWnd;

  spin_button->adj->value = value;
  hWnd = spin_button->updown;
  if (hWnd)
    mySendMessage(hWnd, UDM_SETPOS, 0, (LPARAM)MAKELONG((short)value, 0));
}

void gtk_spin_button_set_adjustment(GtkSpinButton *spin_button,
                                    GtkAdjustment *adjustment)
{
  HWND hWnd;

  spin_button->adj = adjustment;
  hWnd = spin_button->updown;
  if (hWnd) {
    mySendMessage(hWnd, UDM_SETRANGE, 0,
                  (LPARAM)MAKELONG((short)adjustment->upper,
                                   (short)adjustment->lower));
    mySendMessage(hWnd, UDM_SETPOS, 0,
                  (LPARAM)MAKELONG((short)adjustment->value, 0));
  }
}

void gtk_spin_button_realize(GtkWidget *widget)
{
  GtkSpinButton *spin = GTK_SPIN_BUTTON(widget);
  HWND Parent;

  gtk_entry_realize(widget);

  Parent = gtk_get_parent_hwnd(widget->parent);
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  spin->updown = CreateUpDownControl(WS_CHILD | WS_BORDER | WS_TABSTOP |
                                     UDS_SETBUDDYINT | UDS_NOTHOUSANDS |
                                     UDS_ARROWKEYS, 0, 0, 0, 0, Parent, 0,
                                     hInst, widget->hWnd, 20, 10, 15);
  gtk_set_default_font(spin->updown);
  gtk_spin_button_set_adjustment(spin, spin->adj);
}

void gtk_spin_button_destroy(GtkWidget *widget)
{
  g_free(GTK_SPIN_BUTTON(widget)->adj);
}

void gtk_spin_button_show(GtkWidget *widget)
{
  HWND updown;

  updown = GTK_SPIN_BUTTON(widget)->updown;
  if (updown)
    ShowWindow(updown, SW_SHOWNORMAL);
}

void gtk_spin_button_hide(GtkWidget *widget)
{
  HWND updown;

  updown = GTK_SPIN_BUTTON(widget)->updown;
  if (updown)
    ShowWindow(updown, SW_HIDE);
}

void gtk_spin_button_update(GtkSpinButton *spin_button)
{
}

GtkWidget *gtk_separator_new(GtkOrientation orientation)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    return GTK_WIDGET(GtkNewObject(&GtkHSeparatorClass));
  } else {
    return GTK_WIDGET(GtkNewObject(&GtkVSeparatorClass));
  }
}

void gtk_separator_size_request(GtkWidget *widget,
                                GtkRequisition *requisition)
{
  requisition->height = requisition->width = 2;
}

void gtk_separator_realize(GtkWidget *widget)
{
  HWND Parent;

  Parent = gtk_get_parent_hwnd(widget);
  widget->hWnd = myCreateWindow(WC_GTKSEP, "", WS_CHILD,
                                0, 0, 0, 0, Parent, NULL, hInst, NULL);
}

void g_object_set_data(GObject *object, const gchar *key,
                       gpointer data)
{
  g_datalist_set_data(&object->object_data, key, data);
}

gpointer g_object_get_data(GObject *object, const gchar *key)
{
  return g_datalist_get_data(&object->object_data, key);
}

GtkAccelGroup *gtk_accel_group_new()
{
  GtkAccelGroup *new_accel;

  new_accel = g_new0(GtkAccelGroup, 1);
  new_accel->accel = NULL;
  new_accel->numaccel = 0;
  return new_accel;
}

gint gtk_accel_group_add(GtkAccelGroup *accel_group, ACCEL *newaccel)
{
  accel_group->numaccel++;
  accel_group->accel = g_realloc(accel_group->accel,
                                 accel_group->numaccel * sizeof(ACCEL));
  memcpy(&accel_group->accel[accel_group->numaccel - 1], newaccel,
         sizeof(ACCEL));
  return (accel_group->numaccel - 1);
}

void gtk_accel_group_set_id(GtkAccelGroup *accel_group, gint ind, gint ID)
{
  if (ind < accel_group->numaccel)
    accel_group->accel[ind].cmd = ID;
}

void gtk_accel_group_destroy(GtkAccelGroup *accel_group)
{
  g_free(accel_group->accel);
  g_free(accel_group);
}

void gtk_widget_grab_default(GtkWidget *widget)
{
  GTK_WIDGET_SET_FLAGS(widget, GTK_IS_DEFAULT);
}

void gtk_widget_grab_focus(GtkWidget *widget)
{
  if (widget->hWnd && GTK_WIDGET_CAN_FOCUS(widget)) {
    SetFocus(widget->hWnd);
  }
}

void gtk_window_set_modal(GtkWindow *window, gboolean modal)
{
  window->modal = modal;
}

void gtk_window_add_accel_group(GtkWindow *window,
                                GtkAccelGroup *accel_group)
{
  window->accel_group = accel_group;
}

void gtk_entry_set_text(GtkEntry *entry, const gchar *text)
{
  int pos = 0;

  gtk_editable_delete_text(GTK_EDITABLE(entry), 0, -1);
  gtk_editable_insert_text(GTK_EDITABLE(entry), text, strlen(text), &pos);
}

void gtk_entry_set_visibility(GtkEntry *entry, gboolean visible)
{
  HWND hWnd;

  entry->is_visible = visible;
  hWnd = GTK_WIDGET(entry)->hWnd;
  if (hWnd)
    mySendMessage(hWnd, EM_SETPASSWORDCHAR, visible ? 0 : (WPARAM)'*', 0);
}

guint SetAccelerator(GtkWidget *labelparent, gchar *Text,
                     GtkWidget *sendto, gchar *signal,
                     GtkAccelGroup *accel_group, gboolean needalt)
{
  gtk_signal_emit(G_OBJECT(labelparent), "set_text", Text);
  return 0;
}

void gtk_widget_add_accelerator(GtkWidget *widget,
                                const gchar *accel_signal,
                                GtkAccelGroup *accel_group,
                                guint accel_key, guint accel_mods,
                                GtkAccelFlags accel_flags)
{
}

void gtk_widget_remove_accelerator(GtkWidget *widget,
                                   GtkAccelGroup *accel_group,
                                   guint accel_key, guint accel_mods)
{
}

GtkWidget *gtk_vpaned_new()
{
  GtkVPaned *vpaned;

  vpaned = GTK_VPANED(GtkNewObject(&GtkVPanedClass));
  GTK_PANED(vpaned)->handle_size = 5;
  GTK_PANED(vpaned)->handle_pos = PANED_STARTPOS;
  return GTK_WIDGET(vpaned);
}

GtkWidget *gtk_hpaned_new()
{
  GtkHPaned *hpaned;

  hpaned = GTK_HPANED(GtkNewObject(&GtkHPanedClass));
  GTK_PANED(hpaned)->handle_size = 5;
  GTK_PANED(hpaned)->handle_pos = PANED_STARTPOS;
  return GTK_WIDGET(hpaned);
}

static void gtk_paned_pack(GtkPaned *paned, gint pos, GtkWidget *child,
                           gboolean resize, gboolean shrink)
{
  paned->children[pos].widget = child;
  paned->children[pos].resize = resize;
  paned->children[pos].shrink = shrink;
  child->parent = GTK_WIDGET(paned);
}

void gtk_paned_pack1(GtkPaned *paned, GtkWidget *child, gboolean resize,
                     gboolean shrink)
{
  gtk_paned_pack(paned, 0, child, resize, shrink);
}

void gtk_paned_pack2(GtkPaned *paned, GtkWidget *child, gboolean resize,
                     gboolean shrink)
{
  gtk_paned_pack(paned, 1, child, resize, shrink);
}

void gtk_paned_add1(GtkPaned *paned, GtkWidget *child)
{
  gtk_paned_pack1(paned, child, FALSE, TRUE);
}

void gtk_paned_add2(GtkPaned *paned, GtkWidget *child)
{
  gtk_paned_pack2(paned, child, FALSE, TRUE);
}

void gtk_paned_show_all(GtkWidget *widget, gboolean hWndOnly)
{
  GtkPaned *paned = GTK_PANED(widget);
  gint i;

  for (i = 0; i < 2; i++)
    if (paned->children[i].widget) {
      gtk_widget_show_all_full(paned->children[i].widget, hWndOnly);
    }
}

void gtk_paned_hide_all(GtkWidget *widget, gboolean hWndOnly)
{
  GtkPaned *paned = GTK_PANED(widget);
  gint i;

  for (i = 0; i < 2; i++)
    if (paned->children[i].widget)
      gtk_widget_hide_all_full(paned->children[i].widget, hWndOnly);
}

void gtk_paned_realize(GtkWidget *widget)
{
  GtkPaned *paned = GTK_PANED(widget);
  gint i;

  for (i = 0; i < 2; i++)
    if (paned->children[i].widget) {
      gtk_widget_realize(paned->children[i].widget);
    }
}

void gtk_vpaned_realize(GtkWidget *widget)
{
  HWND Parent;

  gtk_paned_realize(widget);
  Parent = gtk_get_parent_hwnd(widget);
  widget->hWnd = myCreateWindow(WC_GTKVPANED, "", WS_CHILD,
                                0, 0, 0, 0, Parent, NULL, hInst, NULL);
}

void gtk_hpaned_realize(GtkWidget *widget)
{
  HWND Parent;

  gtk_paned_realize(widget);
  Parent = gtk_get_parent_hwnd(widget);
  widget->hWnd = myCreateWindow(WC_GTKHPANED, "", WS_CHILD,
                                0, 0, 0, 0, Parent, NULL, hInst, NULL);
}

static void gtk_paned_set_handle_percent(GtkPaned *paned, gint16 req[2])
{
  if (req[0] + req[1])
    paned->handle_pos = req[0] * 100 / (req[0] + req[1]);
  else
    paned->handle_pos = 0;
}

void gtk_vpaned_size_request(GtkWidget *widget,
                             GtkRequisition *requisition)
{
  GtkPaned *paned = GTK_PANED(widget);
  gint i;
  gint16 req[2] = { 0, 0 };

  requisition->width = requisition->height = 0;
  for (i = 0; i < 2; i++)
    if (paned->children[i].widget) {
      if (paned->children[i].widget->requisition.width >
          requisition->width)
        requisition->width = paned->children[i].widget->requisition.width;
      req[i] = paned->children[i].widget->requisition.height;
      requisition->height += req[i];
    }
  requisition->height += paned->handle_size;
  gtk_paned_set_handle_percent(paned, req);
}

void gtk_hpaned_size_request(GtkWidget *widget,
                             GtkRequisition *requisition)
{
  GtkPaned *paned = GTK_PANED(widget);
  gint i;
  gint16 req[2] = { 0, 0 };

  requisition->width = requisition->height = 0;
  for (i = 0; i < 2; i++)
    if (paned->children[i].widget) {
      if (paned->children[i].widget->requisition.height >
          requisition->height)
        requisition->height =
            paned->children[i].widget->requisition.height;
      req[i] = paned->children[i].widget->requisition.width;
      requisition->width += req[i];
    }
  requisition->width += paned->handle_size;
  gtk_paned_set_handle_percent(paned, req);
}

void gtk_vpaned_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkPaned *paned = GTK_PANED(widget);
  GtkWidget *child;
  gint16 alloc;
  GtkAllocation child_alloc;

  memcpy(&paned->true_alloc, allocation, sizeof(GtkAllocation));

  alloc = allocation->height - paned->handle_size;

  child = paned->children[0].widget;
  if (child) {
    child_alloc.x = allocation->x;
    child_alloc.y = allocation->y;
    child_alloc.width = allocation->width;
    child_alloc.height = alloc * paned->handle_pos / 100;
    gtk_widget_set_size(child, &child_alloc);
  }

  child = paned->children[1].widget;
  if (child) {
    child_alloc.x = allocation->x;
    child_alloc.width = allocation->width;
    child_alloc.height = alloc * (100 - paned->handle_pos) / 100;
    child_alloc.y =
        allocation->y + allocation->height - child_alloc.height;
    gtk_widget_set_size(child, &child_alloc);
  }

  allocation->y += alloc * paned->handle_pos / 100;
  allocation->height = paned->handle_size;
}

void gtk_hpaned_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkPaned *paned = GTK_PANED(widget);
  GtkWidget *child;
  gint16 alloc;
  GtkAllocation child_alloc;

  memcpy(&paned->true_alloc, allocation, sizeof(GtkAllocation));
  alloc = allocation->width - paned->handle_size;

  child = paned->children[0].widget;
  if (child) {
    child_alloc.x = allocation->x;
    child_alloc.y = allocation->y;
    child_alloc.height = allocation->height;
    child_alloc.width = alloc * paned->handle_pos / 100;
    gtk_widget_set_size(child, &child_alloc);
  }

  child = paned->children[1].widget;
  if (child) {
    child_alloc.x = allocation->x;
    child_alloc.height = allocation->height;
    child_alloc.width = alloc * (100 - paned->handle_pos) / 100;
    child_alloc.x = allocation->x + allocation->width - child_alloc.width;
    gtk_widget_set_size(child, &child_alloc);
  }

  allocation->x += alloc * paned->handle_pos / 100;
  allocation->width = paned->handle_size;
}

void gtk_text_set_editable(GtkText *text, gboolean is_editable)
{
  gtk_editable_set_editable(GTK_EDITABLE(text), is_editable);
}

void gtk_text_set_word_wrap(GtkText *text, gboolean word_wrap)
{
  text->word_wrap = word_wrap;
}

void gtk_text_freeze(GtkText *text)
{
}

void gtk_text_thaw(GtkText *text)
{
}

GtkTextBuffer *gtk_text_view_get_buffer(GtkText *text)
{
  return text->buffer;
}

static int parse_colcomp(const gchar *hexval)
{
  int col = 0, i, ch;

  for (i = 0; i < 4; ++i) {
    col <<= 4;
    ch = toupper(hexval[i]);
    if (ch >= '0' && ch <= '9') {
      col |= (ch - '0');
    } else if (ch >= 'A' && ch <= 'F') {
      col |= (ch - 'A' + 10);
    }
  }
  return col / 257; /* Scale from 0-65535 to 0-255 */
}

static gboolean parse_color(const gchar *name, COLORREF *color)
{
  int red, green, blue, i, ch;

  if (!name || strlen(name) != 13 || name[0] != '#') {
    return FALSE;
  }
  for (i = strlen(name) - 1; i >= 1; --i) {
    ch = toupper(name[i]);
    if (!((ch >= '0' && ch <= '9')
          || (ch >= 'A' && ch <= 'F'))) {
      return FALSE;
    }
  }
  red = parse_colcomp(&name[1]);
  green = parse_colcomp(&name[5]);
  blue = parse_colcomp(&name[9]);
  if (color) {
    *color = RGB(red, green, blue);
  }
  return TRUE;
}

void gtk_text_buffer_create_tag(GtkTextBuffer *buffer, const gchar *name, ...)
{
  va_list ap;
  gchar *attr, *data;
  CHARFORMAT *cf;

  va_start(ap, name);
  cf = g_new(CHARFORMAT, 1);
  cf->cbSize = sizeof(CHARFORMAT);
  cf->dwMask = 0;
  do {
    attr = va_arg(ap, gchar *);
    if (attr) {
      if (strcmp(attr, "foreground") == 0) {
	data = va_arg(ap, gchar *);
	cf->dwMask |= CFM_COLOR;
	cf->dwEffects = 0;
	if (!parse_color(data, &cf->crTextColor)) {
	  cf->dwEffects = CFE_AUTOCOLOR;
	}
      }
    }
  } while (attr);

  g_datalist_set_data_full(&buffer->tags, name, cf, g_free);
  va_end(ap);
}

GtkWidget *gtk_combo_box_new_with_model(GtkTreeModel *model)
{
  GtkComboBox *combo_box;

  combo_box = GTK_COMBO_BOX(GtkNewObject(&GtkComboBoxClass));
  combo_box->active = -1;
  combo_box->model_column = -1;
  gtk_combo_box_set_model(combo_box, model);
  return GTK_WIDGET(combo_box);
}

void gtk_cell_layout_set_attributes(GtkCellLayout *cell_layout,
                                    GtkCellRenderer *cell, ...)
{
  const char *name;
  va_list args;

  va_start(args, cell);
  /* Currently we only support the "text" attribute to point to the
     ListStore column */
  while ((name = va_arg(args, const char *)) != NULL) {
    if (strcmp(name, "text") == 0) {
      cell_layout->model_column = va_arg(args, int);
    }
  }
  va_end(args);
}

void gtk_combo_box_set_model(GtkComboBox *combo_box, GtkTreeModel *model)
{
  HWND hWnd;

  if (!model)
    return;
  combo_box->model = model;
  hWnd = GTK_WIDGET(combo_box)->hWnd;

  /* For now we work only with a single column of string type */
  if (hWnd) {
    mySendMessage(hWnd, CB_RESETCONTENT, 0, 0);
  }
  if (hWnd && combo_box->model_column >= 0) {
    int nrow;
    int col = combo_box->model_column;
    g_assert(model->coltype[col] == G_TYPE_STRING);
    for (nrow = 0; nrow < combo_box->model->rows->len; ++nrow) {
      GtkListStoreRow *row = &g_array_index(combo_box->model->rows,
                                            GtkListStoreRow, nrow);
      myComboBox_AddString(hWnd, row->data[col]);
    }
    mySendMessage(hWnd, CB_SETCURSEL, (WPARAM)combo_box->active, 0);
  }
}

GtkTreeModel *gtk_combo_box_get_model(GtkComboBox *combo_box)
{
  return combo_box->model;
}

void gtk_combo_box_set_active(GtkComboBox *combo_box, gint index)
{
  combo_box->active = index;
}

gboolean gtk_combo_box_get_active_iter(GtkComboBox *combo_box,
                                       GtkTreeIter *iter)
{
  if (combo_box->active >= 0) {
    *iter = combo_box->active;
    return TRUE;
  } else {
    return FALSE;
  }
}

void gtk_combo_box_size_request(GtkWidget *widget,
                                GtkRequisition *requisition)
{
  SIZE size;

  if (GetTextSize(widget->hWnd, "Sample text", &size, defFont)) {
    requisition->width = size.cx + 40;
    requisition->height = size.cy + 4;
  }
}

void gtk_combo_box_set_size(GtkWidget *widget, GtkAllocation *allocation)
{
  allocation->height *= 6;
}

void gtk_combo_box_realize(GtkWidget *widget)
{
  HWND Parent;
  GtkComboBox *combo_box = GTK_COMBO_BOX(widget);

  Parent = gtk_get_parent_hwnd(widget);
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  widget->hWnd = myCreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", "",
                                  WS_CHILD | WS_TABSTOP | WS_VSCROLL |
                                  CBS_HASSTRINGS | CBS_DROPDOWNLIST,
                                  0, 0, 0, 0, Parent, NULL, hInst, NULL);
  gtk_set_default_font(widget->hWnd);
  gtk_combo_box_set_model(combo_box, combo_box->model);
}

void gtk_combo_box_destroy(GtkWidget *widget)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX(widget);
  if (combo_box->model) {
    gtk_tree_model_free(combo_box->model);
  }
  combo_box->model = NULL;
}

void gtk_label_set_text(GtkLabel *label, const gchar *str)
{
  HWND hWnd;

  g_free(label->text);
  label->text = g_strdup(str ? str : "");
  g_strdelimit(label->text, "_", '&');
  hWnd = GTK_WIDGET(label)->hWnd;
  if (hWnd) {
    gtk_widget_update(GTK_WIDGET(label), FALSE);
    mySetWindowText(hWnd, label->text);
  }
}

void gtk_button_set_text(GtkButton *button, gchar *text)
{
  HWND hWnd;

  g_free(button->text);
  button->text = g_strdup(text ? text : "");
  g_strdelimit(button->text, "_", '&');
  hWnd = GTK_WIDGET(button)->hWnd;
  if (hWnd) {
    gtk_widget_update(GTK_WIDGET(button), FALSE);
    mySetWindowText(hWnd, button->text);
  }
}

static void gtk_menu_item_set_text(GtkMenuItem *menuitem, gchar *text)
{
  GtkWidget *widget = GTK_WIDGET(menuitem);

  g_free(menuitem->text);
  menuitem->text = g_strdup(text ? text : "");
  g_strdelimit(menuitem->text, "_", '&');

  if (GTK_WIDGET_REALIZED(widget)) {
    MENUITEMINFO mii;
    HMENU parent_menu;

    parent_menu = GTK_MENU_SHELL(widget->parent)->menu;
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.dwTypeData = (LPTSTR)menuitem->text;
    mii.cch = myw32strlen(menuitem->text);
    mySetMenuItemInfo(parent_menu, menuitem->ID, FALSE, &mii);
  }
}

const char *gtk_label_get_text(GtkLabel *label)
{
  return label->text;
}

void gtk_text_set_point(GtkText *text, guint index)
{
  gtk_editable_set_position(GTK_EDITABLE(text), index);
}

void gtk_widget_set_size_request(GtkWidget *widget, gint width, gint height)
{
  widget->usize.width = width;
  widget->usize.height = height;
}

void gtk_editable_create(GtkWidget *widget)
{
  GtkEditable *editable = GTK_EDITABLE(widget);

  gtk_widget_create(widget);
  editable->is_editable = TRUE;
  editable->text = g_string_new("");
}

void gtk_combo_box_update_selection(GtkWidget *widget)
{
  LRESULT lres;

  if (widget->hWnd == NULL)
    return;
  lres = mySendMessage(widget->hWnd, CB_GETCURSEL, 0, 0);
  if (lres == CB_ERR)
    return;

  GTK_COMBO_BOX(widget)->active = lres;
  gtk_signal_emit(G_OBJECT(widget), "changed", NULL);
}

void gtk_window_handle_user_size(GtkWindow *window,
                                 GtkAllocation *allocation)
{
}

void gtk_window_handle_minmax_size(GtkWindow *window, LPMINMAXINFO minmax)
{
  GtkRequisition *req;

  req = &GTK_WIDGET(window)->requisition;
  if (!window->allow_shrink) {
    minmax->ptMinTrackSize.x = req->width;
    minmax->ptMinTrackSize.y = req->height;
  }
  if (!window->allow_grow) {
    minmax->ptMaxTrackSize.x = req->width;
    minmax->ptMaxTrackSize.y = req->height;
  }
}

void gtk_window_set_initial_position(GtkWindow *window,
                                     GtkAllocation *allocation)
{
  RECT rect;
  GtkWidget *widget = GTK_WIDGET(window);

  GetWindowRect(GetDesktopWindow(), &rect);

  if (widget->parent) {
    allocation->x =
        rect.left + (rect.right - rect.left - allocation->width) / 2;
    allocation->y =
        rect.top + (rect.bottom - rect.top - allocation->height) / 2;
    if (allocation->x < 0)
      allocation->x = 0;
    if (allocation->y < 0)
      allocation->y = 0;
    if (widget->hWnd)
      SetWindowPos(widget->hWnd, HWND_TOP, allocation->x,
                   allocation->y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
  }
}

void gtk_window_handle_auto_size(GtkWindow *window,
                                 GtkAllocation *allocation)
{
  GtkWidget *widget = GTK_WIDGET(window);

  if (allocation->width < window->default_width) {
    allocation->width = window->default_width;
  }
  if (allocation->height < window->default_height) {
    allocation->height = window->default_height;
  }
  if (allocation->width < widget->allocation.width) {
    allocation->width = widget->allocation.width;
  }
  if (allocation->height < widget->allocation.height) {
    allocation->height = widget->allocation.height;
  }
}

void gtk_paned_set_position(GtkPaned *paned, gint position)
{
  GtkAllocation allocation;

  if (position < 0)
    position = 0;
  if (position > 100)
    position = 100;
  paned->handle_pos = position;
  memcpy(&allocation, &paned->true_alloc, sizeof(GtkAllocation));
  gtk_widget_set_size(GTK_WIDGET(paned), &allocation);
}

void gtk_misc_set_alignment(GtkMisc *misc, gfloat xalign, gfloat yalign)
{
}

GtkWidget *gtk_progress_bar_new()
{
  GtkProgressBar *prog;

  prog = GTK_PROGRESS_BAR(GtkNewObject(&GtkProgressBarClass));
  prog->orient = GTK_PROGRESS_LEFT_TO_RIGHT;
  prog->position = 0;

  return GTK_WIDGET(prog);
}

void gtk_progress_bar_set_fraction(GtkProgressBar *pbar, gfloat percentage)
{
  GtkWidget *widget;

  widget = GTK_WIDGET(pbar);
  pbar->position = percentage;
  if (GTK_WIDGET_REALIZED(widget)) {
    mySendMessage(widget->hWnd, PBM_SETPOS,
                  (WPARAM)(10000.0 * pbar->position), 0);
  }
}

void gtk_progress_bar_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition)
{
  SIZE size;

  if (GetTextSize(widget->hWnd, "Sample", &size, defFont)) {
    requisition->width = size.cx;
    requisition->height = size.cy;
  }
}

void gtk_progress_bar_realize(GtkWidget *widget)
{
  HWND Parent;
  GtkProgressBar *prog;

  prog = GTK_PROGRESS_BAR(widget);
  Parent = gtk_get_parent_hwnd(widget);
  widget->hWnd = myCreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, "",
                                  WS_CHILD,
                                  widget->allocation.x, widget->allocation.y,
                                  widget->allocation.width,
                                  widget->allocation.height, Parent, NULL,
                                  hInst, NULL);
  gtk_set_default_font(widget->hWnd);
  mySendMessage(widget->hWnd, PBM_SETRANGE, 0, MAKELPARAM(0, 10000));
  mySendMessage(widget->hWnd, PBM_SETPOS, (WPARAM)(10000.0 * prog->position),
                0);
}

gint GtkMessageBox(GtkWidget *parent, const gchar *Text,
                   const gchar *Title, GtkMessageType type, gint Options)
{
  gint retval;

  RecurseLevel++;

  switch(type) {
  case GTK_MESSAGE_WARNING:
    Options |= MB_ICONWARNING; break;
  case GTK_MESSAGE_ERROR:
    Options |= MB_ICONERROR; break;
  case GTK_MESSAGE_QUESTION:
    Options |= MB_ICONQUESTION; break;
  default: break;
  }

  retval = myMessageBox(parent && parent->hWnd ? parent->hWnd : NULL,
                        Text, Title, Options);
  RecurseLevel--;

  return retval;
}


/* Add a new source and return a unique ID */
static guint add_our_source(OurSource *source)
{
  GSList *list;
  guint id = 1;

  /* Get an unused ID */
  list = OurSources;
  while (list) {
    OurSource *s = (OurSource *)list->data;
    if (s->id == id) {
      id++;
      list = OurSources;  /* Back to the start */
    } else {
      list = g_slist_next(list);
    }
  }

  source->id = id;
  OurSources = g_slist_append(OurSources, source);
  return source->id;
}

/* Like g_timeout_add(), but uses the Windows event loop */
guint dp_g_timeout_add(guint interval, GSourceFunc function, gpointer data)
{
  guint id;
  OurSource *source = g_new0(OurSource, 1);
  source->timeout_function = function;
  source->data = data;
  id = add_our_source(source);
  if (SetTimer(TopLevel, id, interval, NULL) == 0) {
    g_warning("Failed to create timer!");
  }
  return id;
}

/* Like g_io_add_watch(), but uses the Windows event loop */
guint dp_g_io_add_watch(GIOChannel *channel, GIOCondition condition,
                        GIOFunc func, gpointer user_data)
{
  OurSource *source = g_new0(OurSource, 1);
  source->io_channel = channel;
  source->io_function = func;
  source->socket = g_io_channel_unix_get_fd(channel);
  source->data = user_data;
  WSAAsyncSelect(source->socket, TopLevel, MYWM_SOCKETDATA,
                 (condition & G_IO_IN ? FD_READ | FD_CLOSE | FD_ACCEPT : 0)
                 | (condition & G_IO_OUT ? FD_WRITE | FD_CONNECT : 0));
  return add_our_source(source);
}

/* Like g_source_remove(), but uses the Windows event loop */
gboolean dp_g_source_remove(guint tag)
{
  GSList *list;
  OurSource *s;
  for (list = OurSources; list; list = g_slist_next(list)) {
    s = (OurSource *)list->data;
    if (s->id == tag) {
      if (s->timeout_function) {
        if (KillTimer(TopLevel, s->id) == 0) {
          g_warning("Failed to kill timer!");
        }
      } else {
        WSAAsyncSelect(s->socket, TopLevel, 0, 0);
      }
      OurSources = g_slist_remove(OurSources, s);
      g_free(s);
      break;
    }
  }
  return TRUE;
}

GtkWidget *NewStockButton(const gchar *label, GtkAccelGroup *accel_group)
{
  return gtk_button_new_with_label(_(label));
}

/* We don't really handle styles, so these are just placeholder functions */
static GtkStyle statstyle;
GtkStyle *gtk_style_new(void)
{
  return &statstyle;
}

void gtk_widget_set_style(GtkWidget *widget, GtkStyle *style)
{
}

GtkWidget *gtk_button_box_new(GtkOrientation orientation)
{
  GtkWidget *bbox, *spacer;

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    bbox = gtk_box_new(orientation, 0);
  } else {
    bbox = gtk_box_new(orientation, 5);
  }
  gtk_box_set_homogeneous(GTK_BOX(bbox), TRUE);
  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    /* Add a spacer so that all hboxes are right-aligned */
    spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(bbox), spacer, TRUE, TRUE, 0);
  }
  return bbox;
}

void gtk_box_set_spacing(GtkBox *box, gint spacing)
{
  box->spacing = spacing;
}

/*
 * Win32 doesn't use a window manager, so this is just a placeholder.
 */
void gtk_window_set_type_hint(GtkWindow *window, GdkWindowTypeHint hint)
{
}

void gtk_window_set_position(GtkWindow *window, GtkWindowPosition position)
{
}

gchar *GtkGetFile(const GtkWidget *parent, const gchar *oldname,
                  const gchar *title)
{
  OPENFILENAME ofn;
  char file[800];

  if (oldname) {
    strncpy(file, oldname, sizeof(file));
    file[sizeof(file) - 1] = '\0';
  } else {
    file[0] = '\0';
  }
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = parent ? parent->hWnd : NULL;
  ofn.hInstance = NULL;
  ofn.lpstrFilter = NULL;
  ofn.lpstrCustomFilter = NULL;
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = file;
  ofn.nMaxFile = sizeof(file);
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = title;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
  ofn.lpstrDefExt = NULL;
  if (GetOpenFileName(&ofn)) {
    return g_strdup(file);
  } else {
    return NULL;
  }
}

void gtk_widget_set_can_default(GtkWidget *wid, gboolean flag)
{
  if (flag) {
    GTK_WIDGET_SET_FLAGS(wid, GTK_CAN_DEFAULT);
  } else {
    GTK_WIDGET_UNSET_FLAGS(wid, GTK_CAN_DEFAULT);
  }
}

#else /* CYGWIN */

#if GTK_MAJOR_VERSION == 2
GtkWidget *gtk_button_box_new(GtkOrientation orientation)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    return gtk_hbutton_box_new();
  } else {
    return gtk_vbutton_box_new();
  }
}

GtkWidget *gtk_box_new(GtkOrientation orientation, gint spacing)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    return gtk_hbox_new(FALSE, spacing);
  } else {
    return gtk_vbox_new(FALSE, spacing);
  }
}

GtkWidget *gtk_separator_new(GtkOrientation orientation)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    return gtk_hseparator_new();
  } else {
    return gtk_vseparator_new();
  }
}

#endif

guint SetAccelerator(GtkWidget *labelparent, gchar *Text,
                     GtkWidget *sendto, gchar *signal,
                     GtkAccelGroup *accel_group, gboolean needalt)
{
#if GTK_MAJOR_VERSION == 2
  guint AccelKey;

  AccelKey =
      gtk_label_parse_uline(GTK_LABEL(GTK_BIN(labelparent)->child), Text);
  if (sendto && AccelKey) {
    gtk_widget_add_accelerator(sendto, signal, accel_group, AccelKey,
                               needalt ? GDK_MOD1_MASK : 0,
                               GTK_ACCEL_VISIBLE);
  }
  return AccelKey;
#else
  gtk_label_set_text_with_mnemonic(
                   GTK_LABEL(gtk_bin_get_child(GTK_BIN(labelparent))), Text);
  if (sendto) {
    gtk_label_set_mnemonic_widget(
                   GTK_LABEL(gtk_bin_get_child(GTK_BIN(labelparent))), sendto);
  }
  return 0;
#endif
}

GtkWidget *gtk_scrolled_text_view_new(GtkWidget **pack_widg)
{
  GtkWidget *textview, *scrollwin, *frame;

  textview = gtk_text_view_new();

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

  scrollwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrollwin), textview);
  gtk_container_add(GTK_CONTAINER(frame), scrollwin);

  *pack_widg = frame;
  return textview;
}

void TextViewAppend(GtkTextView *textview, const gchar *text,
                    const gchar *tagname, gboolean scroll)
{
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  GtkTextMark *insert;

  buffer = gtk_text_view_get_buffer(textview);

  gtk_text_buffer_get_end_iter(buffer, &iter);
  gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, text, -1,
                                           tagname, NULL);
  if (scroll) {
    gtk_text_buffer_place_cursor(buffer, &iter);
    insert = gtk_text_buffer_get_mark(buffer, "insert");
    gtk_text_view_scroll_mark_onscreen(textview, insert);
  }
}

void TextViewClear(GtkTextView *textview)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);

  gtk_text_buffer_set_text(buffer, "", -1);
}

static void DestroyGtkMessageBox(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

static void GtkMessageBoxCallback(GtkWidget *widget, gpointer data)
{
  gint *retval;
  GtkWidget *dialog;

  dialog = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
  retval = (gint *)g_object_get_data(G_OBJECT(widget), "retval");
  if (retval)
    *retval = GPOINTER_TO_INT(data);
  gtk_widget_destroy(dialog);
}

gint OldGtkMessageBox(GtkWidget *parent, const gchar *Text,
                      const gchar *Title, gint Options)
{
  GtkWidget *dialog, *button, *label, *vbox, *hbbox, *hsep;
  GtkAccelGroup *accel_group;
  gint i;
  static gint retval;
  gboolean imm_return;
  const gchar *ButtonData[MB_MAX] = {
    GTK_STOCK_OK, GTK_STOCK_CANCEL, GTK_STOCK_YES, GTK_STOCK_NO
  };

  imm_return = Options & MB_IMMRETURN;
  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(dialog), accel_group);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);
  if (parent)
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
  if (!imm_return) {
    g_signal_connect(G_OBJECT(dialog), "destroy",
                     G_CALLBACK(DestroyGtkMessageBox), NULL);
  }
  if (Title)
    gtk_window_set_title(GTK_WINDOW(dialog), Title);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);

  if (Text) {
    label = gtk_label_new(Text);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  }

  hsep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  retval = MB_CANCEL;

  hbbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  for (i = 0; i < MB_MAX; i++) {
    if (Options & (1 << i)) {
      button = NewStockButton(ButtonData[i], accel_group);
      if (!imm_return) {
        g_object_set_data(G_OBJECT(button), "retval", &retval);
      }
      g_signal_connect(G_OBJECT(button), "clicked",
                       G_CALLBACK(GtkMessageBoxCallback),
                       GINT_TO_POINTER(1 << i));
      gtk_box_pack_start(GTK_BOX(hbbox), button, TRUE, TRUE, 0);
    }
  }
  gtk_box_pack_start(GTK_BOX(vbox), hbbox, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);
  gtk_widget_show_all(dialog);
  if (!imm_return)
    gtk_main();
  return retval;
}

GtkWidget *NewStockButton(const gchar *label, GtkAccelGroup *accel_group)
{
  return gtk_button_new_from_stock(label);
}

gint GtkMessageBox(GtkWidget *parent, const gchar *Text,
                   const gchar *Title, GtkMessageType type, gint Options)
{
  GtkWidget *dialog;
  gint retval;
  GtkButtonsType buttons = GTK_BUTTONS_NONE;

  if (Options & MB_IMMRETURN || !parent) {
    return OldGtkMessageBox(parent, Text, Title, Options);
  }

  if (Options & MB_CANCEL) buttons = GTK_BUTTONS_OK_CANCEL;
  else if (Options & MB_OK) buttons = GTK_BUTTONS_OK;
  else if (Options & MB_YESNO) buttons = GTK_BUTTONS_YES_NO;

  dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                  GTK_DIALOG_MODAL,
                                  type, buttons, "%s", Text);
  if (Title) gtk_window_set_title(GTK_WINDOW(dialog), Title);

  retval = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  return retval;
}

static void gtk_url_set_cursor(GtkWidget *widget, GtkWidget *label)
{
  GdkCursor *cursor;

  cursor = gdk_cursor_new(GDK_HAND2);
  gdk_window_set_cursor(gtk_widget_get_window(label), cursor);
  gdk_cursor_unref(cursor);
}

void DisplayHTML(GtkWidget *parent, const gchar *bin, const gchar *target)
{
#ifdef APPLE
  mac_open_url(target);
#elif defined(HAVE_FORK)
  char *args[3];
  pid_t pid;
  int status;

  if (target && target[0] && bin && bin[0]) {
    args[0] = (char *)bin;
    args[1] = (char *)target;
    args[2] = NULL;
    /* Fork twice so that the spawned process gets init as its parent */
    pid = fork();
    if (pid > 0) {
      wait(&status);
    } else if (pid == 0) {
      pid = fork();
      if (pid == 0) {
        execv(bin, args);
        g_print("dopewars: cannot execute %s\n", bin);
        _exit(EXIT_FAILURE);
      } else {
        _exit(EXIT_SUCCESS);
      }
    }
  }
#endif
}

static gboolean gtk_url_triggered(GtkWidget *widget, GdkEventButton *event,
                                  gpointer data)
{
  gchar *bin, *target;

  bin = (gchar *)g_object_get_data(G_OBJECT(widget), "bin");
  target = (gchar *)g_object_get_data(G_OBJECT(widget), "target");
  DisplayHTML(widget, bin, target);
  return TRUE;
}

GtkWidget *gtk_url_new(const gchar *text, const gchar *target,
                       const gchar *bin)
{
  GtkWidget *label, *eventbox;
  gchar *pattern;
  GtkStyle *style;
  GdkColor color;

  color.red = 0;
  color.green = 0;
  color.blue = 0xDDDD;

  label = gtk_label_new(text);

  /* Set the text colour */
  style = gtk_rc_get_style(label);
  if (!style) {
    style = gtk_widget_get_style(label);
  }
  style = gtk_style_copy(style);
  style->fg[GTK_STATE_NORMAL] = color;
  gtk_widget_set_style(label, style);
  g_object_unref(G_OBJECT(style));

  /* Make the text underlined */
  pattern = g_strnfill(strlen(text), '_');
  gtk_label_set_pattern(GTK_LABEL(label), pattern);
  g_free(pattern);

  /* We cannot set the cursor until the widget is realized, so set up a
   * handler to do this later */
  g_signal_connect(G_OBJECT(label), "realize",
                   G_CALLBACK(gtk_url_set_cursor), label);

  eventbox = gtk_event_box_new();
  g_object_set_data_full(G_OBJECT(eventbox), "target",
                           g_strdup(target), g_free);
  g_object_set_data_full(G_OBJECT(eventbox), "bin",
                           g_strdup(bin), g_free);
  g_signal_connect(G_OBJECT(eventbox), "button-release-event",
                   G_CALLBACK(gtk_url_triggered), NULL);

  gtk_container_add(GTK_CONTAINER(eventbox), label);

  return eventbox;
}

gchar *GtkGetFile(const GtkWidget *parent, const gchar *oldname,
                  const gchar *title)
{
  GtkWidget *dialog;
  GtkFileChooser *chooser;
  gint res;
  gchar *ret;

  dialog = gtk_file_chooser_dialog_new(
                 title, GTK_WINDOW(parent), GTK_FILE_CHOOSER_ACTION_OPEN,
                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                 _("_Select"), GTK_RESPONSE_ACCEPT, NULL);
  chooser = GTK_FILE_CHOOSER(dialog);
  gtk_file_chooser_set_filename(chooser, oldname);
  res = gtk_dialog_run(GTK_DIALOG(dialog));
  ret = NULL;
  if (res == GTK_RESPONSE_ACCEPT) {
    ret = gtk_file_chooser_get_filename(chooser);
  }
  gtk_widget_destroy(dialog);
  return ret;
}

gboolean HaveUnicodeSupport(void)
{
  return TRUE;
}

#endif /* CYGWIN */

#if CYGWIN
void TextViewAppend(GtkTextView *textview, const gchar *text,
                    const gchar *tagname, gboolean scroll)
{
  gint editpos;

  editpos = gtk_text_get_length(GTK_TEXT(textview));
  gtk_editable_insert_text(GTK_EDITABLE(textview), text, strlen(text),
                           &editpos);
#ifdef CYGWIN
  gtk_text_view_set_format(textview, tagname, myw32strlen(text), editpos);
#endif
  if (scroll) {
    gtk_editable_set_position(GTK_EDITABLE(textview), editpos);
  }
}

void TextViewClear(GtkTextView *textview)
{
  gtk_editable_delete_text(GTK_EDITABLE(textview), 0, -1);
}
#endif
