#include <string.h>
#include <windows.h>
#include <winsock.h>
#include <commctrl.h>
#include <stdarg.h>
#include "gtk.h"

#define LISTITEMVPACK  0
#define LISTITEMHPACK  3
#define LISTHEADERPACK 6

#define PANED_STARTPOS 200

#define WM_SOCKETDATA (WM_USER+100)

static const gchar *WC_GTKSEP    = "WC_GTKSEP";
static const gchar *WC_GTKVPANED = "WC_GTKVPANED";
static const gchar *WC_GTKHPANED = "WC_GTKHPANED";
static const gchar *WC_GTKDIALOG = "WC_GTKDIALOG";

static BOOL GetTextSize(HWND hWnd,char *text,LPSIZE lpSize);
static void gtk_button_size_request(GtkWidget *widget,
                                    GtkRequisition *requisition);
static void gtk_entry_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition);
static void gtk_entry_set_size(GtkWidget *widget,
                               GtkAllocation *allocation);
static void gtk_text_size_request(GtkWidget *widget,
                                  GtkRequisition *requisition);
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
static void gtk_container_set_size(GtkWidget *widget,GtkAllocation *allocation);
static void gtk_window_size_request(GtkWidget *widget,
                                    GtkRequisition *requisition);
static void gtk_window_set_size(GtkWidget *widget,GtkAllocation *allocation);
static void gtk_window_destroy(GtkWidget *widget);
static void gtk_window_set_menu(GtkWindow *window,GtkMenuBar *menu_bar);
static GtkWidget *gtk_window_get_menu_ID(GtkWindow *window,gint ID);
static void gtk_table_destroy(GtkWidget *widget);
static void gtk_table_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition);
static void gtk_table_set_size(GtkWidget *widget,GtkAllocation *allocation);
static void gtk_table_realize(GtkWidget *widget);
static void gtk_box_destroy(GtkWidget *widget);
static void gtk_hbox_size_request(GtkWidget *widget,
                                  GtkRequisition *requisition);
static void gtk_hbox_set_size(GtkWidget *widget,GtkAllocation *allocation);
static void gtk_vbox_size_request(GtkWidget *widget,
                                  GtkRequisition *requisition);
static void gtk_vbox_set_size(GtkWidget *widget,GtkAllocation *allocation);
static gint gtk_window_delete_event(GtkWidget *widget,GdkEvent *event);
static void gtk_window_realize(GtkWidget *widget);
static void gtk_window_show(GtkWidget *widget);
static void gtk_window_hide(GtkWidget *widget);
static void gtk_container_realize(GtkWidget *widget);
static void gtk_button_realize(GtkWidget *widget);
static void gtk_entry_realize(GtkWidget *widget);
static void gtk_text_realize(GtkWidget *widget);
static void gtk_check_button_realize(GtkWidget *widget);
static void gtk_radio_button_realize(GtkWidget *widget);
static void gtk_radio_button_destroy(GtkWidget *widget);
static void gtk_box_realize(GtkWidget *widget);

static HWND gtk_get_parent_hwnd(GtkWidget *widget);
static void gtk_label_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition);
static void gtk_label_set_size(GtkWidget *widget,
                               GtkAllocation *allocation);
static void gtk_label_destroy(GtkWidget *widget);
static void gtk_label_realize(GtkWidget *widget);
static void gtk_frame_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition);
static void gtk_frame_set_size(GtkWidget *widget,GtkAllocation *allocation);
static void gtk_frame_destroy(GtkWidget *widget);
static void gtk_frame_realize(GtkWidget *widget);
static void gtk_clist_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition);
static void gtk_clist_set_size(GtkWidget *widget,GtkAllocation *allocation);
static void gtk_clist_realize(GtkWidget *widget);
static void gtk_clist_show(GtkWidget *widget);
static void gtk_clist_hide(GtkWidget *widget);
static void gtk_clist_draw_row(GtkCList *clist,LPDRAWITEMSTRUCT lpdis);
static void gtk_container_show_all(GtkWidget *widget,gboolean hWndOnly);
static void gtk_box_show_all(GtkWidget *widget,gboolean hWndOnly);
static void gtk_table_show_all(GtkWidget *widget,gboolean hWndOnly);
static void gtk_widget_show_all_full(GtkWidget *widget,gboolean hWndOnly);
static void gtk_widget_show_full(GtkWidget *widget,gboolean recurse);
static void gtk_widget_update(GtkWidget *widget,gboolean ForceResize);
static void gtk_container_hide_all(GtkWidget *widget,gboolean hWndOnly);
static void gtk_box_hide_all(GtkWidget *widget,gboolean hWndOnly);
static void gtk_table_hide_all(GtkWidget *widget,gboolean hWndOnly);
static void gtk_widget_hide_all_full(GtkWidget *widget,gboolean hWndOnly);
static void gtk_widget_hide_full(GtkWidget *widget,gboolean recurse);

static void gtk_marshal_BOOL__GPOIN(GtkObject *object,GSList *actions,
                                    GtkSignalFunc default_action,
                                    va_list args);
static void gtk_marshal_BOOL__GINT(GtkObject *object,GSList *actions,
                                   GtkSignalFunc default_action,
                                   va_list args);
static void gtk_marshal_VOID__VOID(GtkObject *object,GSList *actions,
                                   GtkSignalFunc default_action,
                                   va_list args);
static void gtk_marshal_VOID__BOOL(GtkObject *object,GSList *actions,
                                   GtkSignalFunc default_action,
                                   va_list args);
static void gtk_marshal_VOID__GPOIN(GtkObject *object,GSList *actions,
                                    GtkSignalFunc default_action,
                                    va_list args);
static void gtk_menu_bar_realize(GtkWidget *widget);
static void gtk_menu_item_realize(GtkWidget *widget);
static void gtk_menu_item_enable(GtkWidget *widget);
static void gtk_menu_item_disable(GtkWidget *widget);
static void gtk_menu_realize(GtkWidget *widget);
static void gtk_menu_shell_realize(GtkWidget *widget);
static GtkWidget *gtk_menu_shell_get_menu_ID(GtkMenuShell *menu_shell,gint ID);
static void gtk_widget_create(GtkWidget *widget);
static void gtk_notebook_realize(GtkWidget *widget);
static void gtk_notebook_destroy(GtkWidget *widget);
static void gtk_notebook_set_size(GtkWidget *widget,
                                  GtkAllocation *allocation);
static void gtk_notebook_size_request(GtkWidget *widget,
                                      GtkRequisition *requisition);
static void gtk_notebook_show_all(GtkWidget *widget,gboolean hWndOnly);
static void gtk_notebook_hide_all(GtkWidget *widget,gboolean hWndOnly);
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
static void gtk_paned_show_all(GtkWidget *widget,gboolean hWndOnly);
static void gtk_paned_hide_all(GtkWidget *widget,gboolean hWndOnly);
static void gtk_paned_realize(GtkWidget *widget);
static void gtk_vpaned_realize(GtkWidget *widget);
static void gtk_hpaned_realize(GtkWidget *widget);
static void gtk_vpaned_size_request(GtkWidget *widget,
                                    GtkRequisition *requisition);
static void gtk_hpaned_size_request(GtkWidget *widget,
                                    GtkRequisition *requisition);
static void gtk_vpaned_set_size(GtkWidget *widget,GtkAllocation *allocation);
static void gtk_hpaned_set_size(GtkWidget *widget,GtkAllocation *allocation);
static void gtk_option_menu_size_request(GtkWidget *widget,
                                         GtkRequisition *requisition);
static void gtk_option_menu_set_size(GtkWidget *widget,
                                     GtkAllocation *allocation);
static void gtk_option_menu_realize(GtkWidget *widget);
static void gtk_clist_update_selection(GtkWidget *widget);
static void gtk_button_set_text(GtkButton *button,gchar *text);
static void gtk_menu_item_set_text(GtkMenuItem *menuitem,gchar *text);
static void gtk_editable_create(GtkWidget *widget);
static void gtk_option_menu_update_selection(GtkWidget *widget);
static void gtk_clist_update_widths(GtkCList *clist,gchar *text[]);
static void gtk_clist_update_all_widths(GtkCList *clist);
static void gtk_clist_do_auto_resize(GtkCList *clist);
static void gtk_clist_set_column_width_full(GtkCList *clist,gint column,
                                            gint width,gboolean ResizeHeader);
static void gtk_widget_set_focus(GtkWidget *widget);
static void gtk_widget_lose_focus(GtkWidget *widget);
static void gtk_window_update_focus(GtkWindow *window);
static void gtk_window_set_focus(GtkWindow *window);
static void gtk_window_handle_user_size(GtkWindow *window,
                                        GtkAllocation *allocation);
static void gtk_window_handle_auto_size(GtkWindow *window,
                                        GtkAllocation *allocation);
static void gtk_window_set_initial_position(GtkWindow *window,
                                            GtkAllocation *allocation);

typedef struct _GdkInput GdkInput;

struct _GdkInput {
   gint source;
   GdkInputCondition condition;
   GdkInputFunction function;
   gpointer data;
};

typedef struct _GtkItemFactoryChild GtkItemFactoryChild;

struct _GtkItemFactoryChild {
   gchar *path;
   GtkWidget *widget;
};

static GtkSignalType GtkObjectSignals[] = {
   { "create",gtk_marshal_VOID__VOID,NULL },
   { "",NULL,NULL }
};

static GtkClass GtkObjectClass = {
   "object",NULL,sizeof(GtkObject),GtkObjectSignals
};

static GtkClass GtkAdjustmentClass = {
   "adjustment",&GtkObjectClass,sizeof(GtkAdjustment),NULL
};

static GtkClass GtkItemFactoryClass = {
   "itemfactory",&GtkObjectClass,sizeof(GtkItemFactory),NULL
};

static GtkSignalType GtkWidgetSignals[] = {
   { "create",gtk_marshal_VOID__VOID,gtk_widget_create },
   { "size_request",gtk_marshal_VOID__GPOIN,NULL },
   { "set_size",gtk_marshal_VOID__GPOIN,NULL },
   { "realize",gtk_marshal_VOID__VOID,NULL },
   { "destroy",gtk_marshal_VOID__VOID,NULL },
   { "show",gtk_marshal_VOID__VOID,NULL },
   { "hide",gtk_marshal_VOID__VOID,NULL },
   { "show_all",gtk_marshal_VOID__BOOL,NULL },
   { "hide_all",gtk_marshal_VOID__BOOL,NULL },
   { "enable",gtk_marshal_VOID__VOID,NULL },
   { "disable",gtk_marshal_VOID__VOID,NULL },
   { "",NULL,NULL }
};

static GtkClass GtkWidgetClass = {
   "widget",&GtkObjectClass,sizeof(GtkWidget),GtkWidgetSignals
};

static GtkSignalType GtkSeparatorSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_separator_size_request },
   { "realize",gtk_marshal_VOID__VOID,gtk_separator_realize },
   { "",NULL,NULL }
};

static GtkClass GtkSeparatorClass = {
   "separator",&GtkWidgetClass,sizeof(GtkSeparator),GtkSeparatorSignals
};

static GtkClass GtkHSeparatorClass = {
   "hseparator",&GtkSeparatorClass,sizeof(GtkHSeparator),NULL
};

static GtkClass GtkVSeparatorClass = {
   "vseparator",&GtkSeparatorClass,sizeof(GtkVSeparator),NULL
};

static GtkClass GtkMenuShellClass = {
   "menushell",&GtkWidgetClass,sizeof(GtkMenuShell),NULL
};

static GtkSignalType GtkMenuBarSignals[] = {
   { "realize",gtk_marshal_VOID__VOID,gtk_menu_bar_realize },
   { "",NULL,NULL }
};

static GtkClass GtkMenuBarClass = {
   "menubar",&GtkMenuShellClass,sizeof(GtkMenuBar),GtkMenuBarSignals
};

static GtkSignalType GtkMenuItemSignals[] = {
   { "realize",gtk_marshal_VOID__VOID,gtk_menu_item_realize },
   { "set_text",gtk_marshal_VOID__GPOIN,gtk_menu_item_set_text },
   { "activate",gtk_marshal_VOID__VOID,NULL },
   { "enable",gtk_marshal_VOID__VOID,gtk_menu_item_enable },
   { "disable",gtk_marshal_VOID__VOID,gtk_menu_item_disable },
   { "",NULL,NULL }
};

static GtkClass GtkMenuItemClass = {
   "menuitem",&GtkWidgetClass,sizeof(GtkMenuItem),GtkMenuItemSignals
};

static GtkSignalType GtkMenuSignals[] = {
   { "realize",gtk_marshal_VOID__VOID,gtk_menu_realize },
   { "",NULL,NULL }
};

static GtkClass GtkMenuClass = {
   "menu",&GtkMenuShellClass,sizeof(GtkMenu),GtkMenuSignals
};

static GtkSignalType GtkEditableSignals[] = {
   { "create",gtk_marshal_VOID__VOID,gtk_editable_create },
   { "activate",gtk_marshal_VOID__VOID,NULL },
   { "",NULL,NULL }
};

static GtkClass GtkEditableClass = {
   "editable",&GtkWidgetClass,sizeof(GtkEditable),GtkEditableSignals
};

static GtkSignalType GtkEntrySignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_entry_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_entry_set_size },
   { "realize",gtk_marshal_VOID__VOID,gtk_entry_realize },
   { "",NULL,NULL }
};

static GtkClass GtkEntryClass = {
   "entry",&GtkEditableClass,sizeof(GtkEntry),GtkEntrySignals
};

static GtkSignalType GtkSpinButtonSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_spin_button_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_spin_button_set_size },
   { "realize",gtk_marshal_VOID__VOID,gtk_spin_button_realize },
   { "destroy",gtk_marshal_VOID__VOID,gtk_spin_button_destroy },
   { "hide",gtk_marshal_VOID__VOID,gtk_spin_button_hide },
   { "show",gtk_marshal_VOID__VOID,gtk_spin_button_show },
   { "",NULL,NULL }
};

static GtkClass GtkSpinButtonClass = {
   "spinbutton",&GtkEntryClass,sizeof(GtkSpinButton),GtkSpinButtonSignals
};

static GtkSignalType GtkTextSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_text_size_request },
   { "realize",gtk_marshal_VOID__VOID,gtk_text_realize },
   { "",NULL,NULL }
};

static GtkClass GtkTextClass = {
   "text",&GtkEditableClass,sizeof(GtkText),GtkTextSignals
};

static GtkSignalType GtkLabelSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_label_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_label_set_size },
   { "set_text",gtk_marshal_VOID__GPOIN,gtk_label_set_text },
   { "realize",gtk_marshal_VOID__VOID,gtk_label_realize },
   { "destroy",gtk_marshal_VOID__VOID,gtk_label_destroy },
   { "",NULL,NULL }
};

static GtkClass GtkLabelClass = {
   "label",&GtkWidgetClass,sizeof(GtkLabel),GtkLabelSignals
};

static GtkSignalType GtkButtonSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_button_size_request },
   { "set_text",gtk_marshal_VOID__GPOIN,gtk_button_set_text },
   { "realize",gtk_marshal_VOID__VOID,gtk_button_realize },
   { "destroy",gtk_marshal_VOID__VOID,gtk_button_destroy },
   { "clicked",gtk_marshal_VOID__VOID,NULL },
   { "",NULL,NULL }
};

static GtkClass GtkButtonClass = {
   "button",&GtkWidgetClass,sizeof(GtkButton),GtkButtonSignals
};

static GtkSignalType GtkOptionMenuSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_option_menu_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_option_menu_set_size },
   { "realize",gtk_marshal_VOID__VOID,gtk_option_menu_realize },
   { "",NULL,NULL }
};

static GtkClass GtkOptionMenuClass = {
   "optionmenu",&GtkButtonClass,sizeof(GtkOptionMenu),GtkOptionMenuSignals
};

static GtkSignalType GtkToggleButtonSignals[] = {
   { "toggled",gtk_marshal_VOID__VOID,NULL },
   { "",NULL,NULL }
};

static GtkClass GtkToggleButtonClass = {
   "toggle",&GtkButtonClass,sizeof(GtkToggleButton),GtkToggleButtonSignals
};

static GtkSignalType GtkCheckButtonSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_check_button_size_request },
   { "realize",gtk_marshal_VOID__VOID,gtk_check_button_realize },
   { "",NULL,NULL }
};

static GtkClass GtkCheckButtonClass = {
   "check",&GtkToggleButtonClass,sizeof(GtkCheckButton),GtkCheckButtonSignals
};

static GtkSignalType GtkRadioButtonSignals[] = {
   { "realize",gtk_marshal_VOID__VOID,gtk_radio_button_realize },
   { "destroy",gtk_marshal_VOID__VOID,gtk_radio_button_destroy },
   { "",NULL,NULL }
};

static GtkClass GtkRadioButtonClass = {
   "radio",&GtkCheckButtonClass,sizeof(GtkRadioButton),GtkRadioButtonSignals
};

static GtkSignalType GtkContainerSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_container_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_container_set_size },
   { "realize",gtk_marshal_VOID__VOID,gtk_container_realize },
   { "destroy",gtk_marshal_VOID__VOID,gtk_container_destroy },
   { "show_all",gtk_marshal_VOID__BOOL,gtk_container_show_all },
   { "hide_all",gtk_marshal_VOID__BOOL,gtk_container_hide_all },
   { "",NULL,NULL }
};

static GtkClass GtkContainerClass = {
   "container",&GtkWidgetClass,sizeof(GtkContainer),GtkContainerSignals
};

static GtkSignalType GtkPanedSignals[] = {
   { "show_all",gtk_marshal_VOID__BOOL,gtk_paned_show_all },
   { "hide_all",gtk_marshal_VOID__BOOL,gtk_paned_hide_all },
   { "",NULL,NULL }
};
static GtkClass GtkPanedClass = {
   "paned",&GtkContainerClass,sizeof(GtkPaned),GtkPanedSignals
};

static GtkSignalType GtkVPanedSignals[] = {
   { "realize",gtk_marshal_VOID__VOID,gtk_vpaned_realize },
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_vpaned_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_vpaned_set_size },
   { "",NULL,NULL }
};

static GtkClass GtkVPanedClass = {
   "vpaned",&GtkPanedClass,sizeof(GtkVPaned),GtkVPanedSignals
};

static GtkSignalType GtkHPanedSignals[] = {
   { "realize",gtk_marshal_VOID__VOID,gtk_hpaned_realize },
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_hpaned_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_hpaned_set_size },
   { "",NULL,NULL }
};

static GtkClass GtkHPanedClass = {
   "hpaned",&GtkPanedClass,sizeof(GtkHPaned),GtkHPanedSignals
};

static GtkSignalType GtkBoxSignals[] = {
   { "realize",gtk_marshal_VOID__VOID,gtk_box_realize },
   { "destroy",gtk_marshal_VOID__VOID,gtk_box_destroy },
   { "show_all",gtk_marshal_VOID__BOOL,gtk_box_show_all },
   { "hide_all",gtk_marshal_VOID__BOOL,gtk_box_hide_all },
   { "",NULL,NULL }
};

static GtkClass GtkBoxClass = {
   "box",&GtkContainerClass,sizeof(GtkBox),GtkBoxSignals
};

static GtkSignalType GtkNotebookSignals[] = {
   { "realize",gtk_marshal_VOID__VOID,gtk_notebook_realize },
   { "destroy",gtk_marshal_VOID__VOID,gtk_notebook_destroy },
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_notebook_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_notebook_set_size },
   { "show_all",gtk_marshal_VOID__BOOL,gtk_notebook_show_all },
   { "hide_all",gtk_marshal_VOID__BOOL,gtk_notebook_hide_all },
   { "",NULL,NULL }
};

static GtkClass GtkNotebookClass = {
   "notebook",&GtkContainerClass,sizeof(GtkNotebook),GtkNotebookSignals
};

static GtkSignalType GtkTableSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_table_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_table_set_size },
   { "realize",gtk_marshal_VOID__VOID,gtk_table_realize },
   { "destroy",gtk_marshal_VOID__VOID,gtk_table_destroy },
   { "show_all",gtk_marshal_VOID__BOOL,gtk_table_show_all },
   { "hide_all",gtk_marshal_VOID__BOOL,gtk_table_hide_all },
   { "",NULL,NULL }
};

static GtkClass GtkTableClass = {
   "table",&GtkContainerClass,sizeof(GtkTable),GtkTableSignals
};

static GtkSignalType GtkCListSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_clist_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_clist_set_size },
   { "realize",gtk_marshal_VOID__VOID,gtk_clist_realize },
   { "click-column",gtk_marshal_VOID__GPOIN,NULL },
   { "show",gtk_marshal_VOID__VOID,gtk_clist_show },
   { "hide",gtk_marshal_VOID__VOID,gtk_clist_hide },
   { "",NULL,NULL }
};

static GtkClass GtkCListClass = {
   "clist",&GtkContainerClass,sizeof(GtkCList),GtkCListSignals
};

static GtkSignalType GtkHBoxSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_hbox_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_hbox_set_size },
   { "",NULL,NULL }
};

static GtkClass GtkHBoxClass = {
   "hbox",&GtkBoxClass,sizeof(GtkHBox),GtkHBoxSignals
};

static GtkSignalType GtkVBoxSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_vbox_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_vbox_set_size },
   { "",NULL,NULL }
};

static GtkClass GtkVBoxClass = {
   "vbox",&GtkBoxClass,sizeof(GtkVBox),GtkVBoxSignals
};

static GtkClass GtkBinClass = {
   "bin",&GtkContainerClass,sizeof(GtkBin),NULL
};

static GtkSignalType GtkFrameSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_frame_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_frame_set_size },
   { "realize",gtk_marshal_VOID__VOID,gtk_frame_realize },
   { "destroy",gtk_marshal_VOID__VOID,gtk_frame_destroy },
   { "",NULL,NULL }
};

static GtkClass GtkFrameClass = {
   "frame",&GtkBinClass,sizeof(GtkFrame),GtkFrameSignals
};

static GtkSignalType GtkWindowSignals[] = {
   { "size_request",gtk_marshal_VOID__GPOIN,gtk_window_size_request },
   { "set_size",gtk_marshal_VOID__GPOIN,gtk_window_set_size },
   { "realize",gtk_marshal_VOID__VOID,gtk_window_realize },
   { "destroy",gtk_marshal_VOID__VOID,gtk_window_destroy },
   { "show",gtk_marshal_VOID__VOID,gtk_window_show },
   { "hide",gtk_marshal_VOID__VOID,gtk_window_hide },
   { "delete_event",gtk_marshal_BOOL__GPOIN,
                    GTK_SIGNAL_FUNC(gtk_window_delete_event) },
   { "",NULL,NULL }
};

static GtkClass GtkWindowClass = {
   "window",&GtkBinClass,sizeof(GtkWindow),GtkWindowSignals
};

const GtkType GTK_TYPE_WINDOW=&GtkWindowClass;
const GtkType GTK_TYPE_MENU_BAR=&GtkMenuBarClass;

static HINSTANCE hInst;
static HFONT hFont;
static GSList *WindowList=NULL;
static GSList *GdkInputs=NULL;
static HWND TopLevel=NULL;
long AsyncSocketError=0;

static void gtk_set_default_font(HWND hWnd) {
   SendMessage(hWnd,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(FALSE,0));
}

static GtkObject *GtkNewObject(GtkClass *klass) {
   GtkObject *newObj;

   newObj=g_malloc0(klass->Size);
   newObj->klass=klass;
   gtk_signal_emit(newObj,"create");

   return newObj;
}

static void DispatchSocketEvent(SOCKET sock,long event) {
   GSList *list;
   GdkInput *input;
   for (list=GdkInputs;list;list=g_slist_next(list)) {
      input=(GdkInput *)(list->data);
      if (input->source==sock) {
         (*input->function)(input->data,input->source,
                            (event&(FD_READ|FD_CLOSE) ? GDK_INPUT_READ:0) |
                            (event&(FD_WRITE|FD_CONNECT) ? GDK_INPUT_WRITE:0));
         break;
      }
   }
}

LRESULT CALLBACK GtkPanedProc(HWND hwnd,UINT msg,UINT wParam,LONG lParam) {
   PAINTSTRUCT ps;
   HPEN oldpen,dkpen,ltpen;
   RECT rect;
   HDC hDC;
   GtkPaned *paned;
   paned=GTK_PANED(GetWindowLong(hwnd,GWL_USERDATA));
   switch(msg) {
      case WM_PAINT:
         if (GetUpdateRect(hwnd,NULL,TRUE)) {
            BeginPaint(hwnd,&ps);
            GetClientRect(hwnd,&rect);
            hDC=ps.hdc;
            ltpen=CreatePen(PS_SOLID,0,(COLORREF)GetSysColor(COLOR_3DHILIGHT));
            dkpen=CreatePen(PS_SOLID,0,(COLORREF)GetSysColor(COLOR_3DSHADOW));

            if (rect.right > rect.bottom) {
               oldpen=SelectObject(hDC,ltpen);
               MoveToEx(hDC,rect.left,rect.top,NULL);
               LineTo(hDC,rect.right,rect.top);

               SelectObject(hDC,dkpen);
               MoveToEx(hDC,rect.left,rect.bottom-1,NULL);
               LineTo(hDC,rect.right,rect.bottom-1);
            } else {
               oldpen=SelectObject(hDC,ltpen);
               MoveToEx(hDC,rect.left,rect.top,NULL);
               LineTo(hDC,rect.left,rect.bottom);

               SelectObject(hDC,ltpen);
               MoveToEx(hDC,rect.right-1,rect.top,NULL);
               LineTo(hDC,rect.right-1,rect.bottom);
            }

            SelectObject(hDC,oldpen);
            DeleteObject(ltpen); DeleteObject(dkpen);
            EndPaint(hwnd,&ps);
         }
         return TRUE;
      case WM_LBUTTONDOWN:
         if (!paned) break;
         SetCapture(hwnd);
         paned->Tracking=TRUE;
         return TRUE;
      case WM_MOUSEMOVE:
         if (!paned||!paned->Tracking) break;
         return TRUE;
      case WM_LBUTTONUP:
         if (!paned||!paned->Tracking) break;
         ReleaseCapture();
         paned->Tracking=FALSE;
         return TRUE;
      default:
         return DefWindowProc(hwnd,msg,wParam,lParam);
   }
   return FALSE;
}

LRESULT CALLBACK GtkSepProc(HWND hwnd,UINT msg,UINT wParam,LONG lParam) {
   PAINTSTRUCT ps;
   HPEN oldpen,dkpen,ltpen;
   RECT rect;
   HDC hDC;
   if (msg==WM_PAINT) {
      if (GetUpdateRect(hwnd,NULL,TRUE)) {
         BeginPaint(hwnd,&ps);
         GetClientRect(hwnd,&rect);
         hDC=ps.hdc;
         ltpen=CreatePen(PS_SOLID,0,(COLORREF)GetSysColor(COLOR_3DHILIGHT));
         dkpen=CreatePen(PS_SOLID,0,(COLORREF)GetSysColor(COLOR_3DSHADOW));

         if (rect.right > rect.bottom) {
            oldpen=SelectObject(hDC,dkpen);
            MoveToEx(hDC,rect.left,rect.top,NULL);
            LineTo(hDC,rect.right,rect.top);

            SelectObject(hDC,ltpen);
            MoveToEx(hDC,rect.left,rect.top+1,NULL);
            LineTo(hDC,rect.right,rect.top+1);
         } else {
            oldpen=SelectObject(hDC,dkpen);
            MoveToEx(hDC,rect.left,rect.top,NULL);
            LineTo(hDC,rect.left,rect.bottom);

            SelectObject(hDC,ltpen);
            MoveToEx(hDC,rect.left+1,rect.top,NULL);
            LineTo(hDC,rect.left+1,rect.bottom);
         }

         SelectObject(hDC,oldpen);
         DeleteObject(ltpen); DeleteObject(dkpen);
         EndPaint(hwnd,&ps);
      }
      return TRUE;
   } else return DefWindowProc(hwnd,msg,wParam,lParam);
}

LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg,UINT wParam,LONG lParam) {
   GtkWidget *window,*widget;
   GtkClass *klass;
   RECT rect;
   GtkAllocation alloc;
   gboolean signal_return;
   GdkEvent event=0;
   LPMEASUREITEMSTRUCT lpmis;
   HDC hDC;
   TEXTMETRIC tm;
   LPDRAWITEMSTRUCT lpdis;
   HD_NOTIFY FAR* phdr;
   NMHDR *nmhdr;
   switch(msg) {
      case WM_SIZE:
         window=GTK_WIDGET(GetWindowLong(hwnd,GWL_USERDATA));
         GetWindowRect(hwnd,&rect);
         alloc.x=rect.left; alloc.y=rect.top;
         alloc.width=rect.right-rect.left;
         alloc.height=rect.bottom-rect.top;
         gtk_window_handle_user_size(GTK_WINDOW(window),&alloc);
         gtk_widget_set_size(window,&alloc);
         break;
      case WM_ACTIVATE:
      case WM_ACTIVATEAPP:
         widget=GTK_WIDGET(GetWindowLong(hwnd,GWL_USERDATA));
         if (widget) klass=GTK_OBJECT(widget)->klass; else klass=NULL;
         if (klass==&GtkWindowClass) {
             if ((msg==WM_ACTIVATE && LOWORD(wParam)!=WA_INACTIVE)||
                 (msg==WM_ACTIVATEAPP && wParam)) {
               if (GTK_WINDOW(widget)->focus) {
                  gtk_widget_set_focus(GTK_WINDOW(widget)->focus);
               }
               if (!GTK_WINDOW(widget)->focus) {
                  gtk_window_set_focus(GTK_WINDOW(widget));
               }
            } else if (msg==WM_ACTIVATE && LOWORD(wParam)==WA_INACTIVE) {
               gtk_window_update_focus(GTK_WINDOW(widget));
            }
         }
         break;
      case WM_COMMAND:
         widget=GTK_WIDGET(GetWindowLong((HWND)lParam,GWL_USERDATA));
         if (widget) klass=GTK_OBJECT(widget)->klass; else klass=NULL;

         if (lParam && klass==&GtkCListClass && HIWORD(wParam)==LBN_SELCHANGE) {
            gtk_clist_update_selection(widget);
         } else if (lParam && klass==&GtkOptionMenuClass &&
                    HIWORD(wParam)==CBN_SELENDOK) {
            gtk_option_menu_update_selection(widget);
         } else if (lParam && HIWORD(wParam)==BN_CLICKED) {
            gtk_signal_emit(GTK_OBJECT(widget),"clicked");
         } else if (HIWORD(wParam)==0) {
            window=GTK_WIDGET(GetWindowLong(hwnd,GWL_USERDATA));
            widget=gtk_window_get_menu_ID(GTK_WINDOW(window),LOWORD(wParam));
            if (widget) gtk_signal_emit(GTK_OBJECT(widget),"activate");
         } else return TRUE;
         break;
      case WM_CLOSE:
         gtk_signal_emit(GTK_OBJECT(GetWindowLong(hwnd,GWL_USERDATA)),
                         "delete_event",&event,&signal_return);
         return TRUE;
         break;
      case WM_DRAWITEM:
         lpdis=(LPDRAWITEMSTRUCT)lParam;
         if (lpdis) {
            gtk_clist_draw_row(GTK_CLIST(GetWindowLong(lpdis->hwndItem,
                                         GWL_USERDATA)),lpdis);
            return TRUE;
         }
         break;
      case WM_MEASUREITEM:
         lpmis=(LPMEASUREITEMSTRUCT)lParam;
         hDC=GetDC(hwnd);
         if (!GetTextMetrics(hDC,&tm)) g_warning("GetTextMetrics failed");
         ReleaseDC(hwnd,hDC);
         if (lpmis) {
            lpmis->itemHeight = tm.tmHeight + LISTITEMVPACK*2;
            return TRUE;
         }
         break;
      case WM_NOTIFY:
         phdr=(HD_NOTIFY FAR *)lParam;
         nmhdr=(NMHDR *)lParam;
         if (!nmhdr) break;
         if (nmhdr->code==HDN_ITEMCHANGED) {
            gtk_clist_set_column_width_full(
                    GTK_CLIST(GetWindowLong(nmhdr->hwndFrom,GWL_USERDATA)),
                    phdr->iItem,phdr->pitem->cxy,FALSE);
            return FALSE;
         } else if (nmhdr->code==TCN_SELCHANGE) {
            gtk_notebook_set_page(
                    GTK_NOTEBOOK(GetWindowLong(nmhdr->hwndFrom,GWL_USERDATA)),
                    TabCtrl_GetCurSel(nmhdr->hwndFrom));
            return FALSE;
         }
         break;
      case WM_SOCKETDATA:
         AsyncSocketError=WSAGETSELECTERROR(lParam);
         DispatchSocketEvent((SOCKET)wParam,WSAGETSELECTEVENT(lParam));
         AsyncSocketError=0;
         break;
      default:
         return DefWindowProc(hwnd,msg,wParam,lParam);
   }
   return FALSE;
}

void win32_init(HINSTANCE hInstance,HINSTANCE hPrevInstance) {
   WNDCLASS wc;
   hInst=hInstance;
   hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
   WindowList=NULL;
   AsyncSocketError=0;
   if (!hPrevInstance) {
      wc.style		= CS_HREDRAW|CS_VREDRAW;
      wc.lpfnWndProc	= MainWndProc;
      wc.cbClsExtra	= 0;
      wc.cbWndExtra	= 0;
      wc.hInstance	= hInstance;
      wc.hIcon		= LoadIcon(NULL,IDI_APPLICATION);
      wc.hCursor	= LoadCursor(NULL,IDC_ARROW);
      wc.hbrBackground	= (HBRUSH)(1+COLOR_BTNFACE);
      wc.lpszMenuName	= NULL;
      wc.lpszClassName	= "mainwin";
      RegisterClass(&wc);

      wc.style		= CS_HREDRAW|CS_VREDRAW;
      wc.lpfnWndProc	= MainWndProc;
      wc.cbClsExtra	= 0;
      wc.cbWndExtra	= 0;
      wc.hInstance	= hInstance;
      wc.hIcon		= NULL;
      wc.hCursor	= LoadCursor(NULL,IDC_ARROW);
      wc.hbrBackground	= (HBRUSH)(1+COLOR_BTNFACE);
      wc.lpszMenuName	= NULL;
      wc.lpszClassName	= WC_GTKDIALOG;
      RegisterClass(&wc);

      wc.style		= CS_HREDRAW|CS_VREDRAW;
      wc.lpfnWndProc	= GtkPanedProc;
      wc.cbClsExtra	= 0;
      wc.cbWndExtra	= 0;
      wc.hInstance	= hInstance;
      wc.hIcon		= NULL;
      wc.hCursor	= LoadCursor(NULL,IDC_SIZEWE);
      wc.hbrBackground	= (HBRUSH)(1+COLOR_BTNFACE);
      wc.lpszMenuName	= NULL;
      wc.lpszClassName	= WC_GTKHPANED;
      RegisterClass(&wc);

      wc.style		= CS_HREDRAW|CS_VREDRAW;
      wc.lpfnWndProc	= GtkPanedProc;
      wc.cbClsExtra	= 0;
      wc.cbWndExtra	= 0;
      wc.hInstance	= hInstance;
      wc.hIcon		= NULL;
      wc.hCursor	= LoadCursor(NULL,IDC_SIZENS);
      wc.hbrBackground	= (HBRUSH)(1+COLOR_BTNFACE);
      wc.lpszMenuName	= NULL;
      wc.lpszClassName	= WC_GTKVPANED;
      RegisterClass(&wc);

      wc.style		= CS_HREDRAW|CS_VREDRAW;
      wc.lpfnWndProc	= GtkSepProc;
      wc.cbClsExtra	= 0;
      wc.cbWndExtra	= 0;
      wc.hInstance	= hInstance;
      wc.hIcon		= NULL;
      wc.hCursor	= LoadCursor(NULL,IDC_ARROW);
      wc.hbrBackground	= (HBRUSH)(1+COLOR_BTNFACE);
      wc.lpszMenuName	= NULL;
      wc.lpszClassName	= WC_GTKSEP;
      RegisterClass(&wc);
   }

   InitCommonControls();
}

void gtk_widget_update(GtkWidget *widget,gboolean ForceResize) {
   GtkRequisition req;
   GtkWidget *window;
   GtkAllocation alloc;

   if (!GTK_WIDGET_REALIZED(widget)) return;

   gtk_widget_size_request(widget,&req);
   window=gtk_widget_get_ancestor(widget,GTK_TYPE_WINDOW);
   if (window) {
      alloc.x=alloc.y=0;
      alloc.width=window->requisition.width;
      alloc.height=window->requisition.height;
      gtk_window_handle_auto_size(GTK_WINDOW(window),&alloc);
      if (alloc.width!=window->allocation.width ||
          alloc.height!=window->allocation.height || ForceResize) {
         gtk_widget_set_size(window,&alloc);
      }
   }
}

void gtk_widget_show(GtkWidget *widget) {
   gtk_widget_show_full(widget,TRUE);
}

void gtk_widget_show_full(GtkWidget *widget,gboolean recurse) {
   GtkAllocation alloc;

   if (GTK_WIDGET_VISIBLE(widget)) return;
   GTK_WIDGET_SET_FLAGS(widget,GTK_VISIBLE);

   if (recurse) gtk_widget_show_all_full(widget,TRUE);
   else gtk_signal_emit(GTK_OBJECT(widget),"show");

   if (!GTK_WIDGET_REALIZED(widget) &&
       GTK_OBJECT(widget)->klass==&GtkWindowClass) {
      gtk_widget_realize(widget);
      alloc.x=alloc.y=0;
      alloc.width=widget->requisition.width;
      alloc.height=widget->requisition.height;
      gtk_window_set_initial_position(GTK_WINDOW(widget),&alloc);
      gtk_widget_set_size(widget,&alloc);
      ShowWindow(widget->hWnd,SW_SHOWNORMAL);
      UpdateWindow(widget->hWnd);
   } else if (GTK_WIDGET_REALIZED(widget) &&
             GTK_OBJECT(widget)->klass!=&GtkWindowClass) {
      gtk_widget_update(widget,TRUE);
      if (!recurse) ShowWindow(widget->hWnd,SW_SHOWNORMAL);
   }

// g_print("widget show - set focus\n");
   gtk_widget_set_focus(widget);
}

void gtk_widget_hide(GtkWidget *widget) {
   gtk_widget_hide_full(widget,TRUE);
}

void gtk_widget_hide_full(GtkWidget *widget,gboolean recurse) {
   GtkAllocation alloc;
   GtkRequisition req;
   GtkWidget *window;

   if (!GTK_WIDGET_VISIBLE(widget)) return;

   if (recurse) gtk_widget_hide_all_full(widget,TRUE);
   else {
      gtk_signal_emit(GTK_OBJECT(widget),"hide");
      if (widget->hWnd) ShowWindow(widget->hWnd,SW_HIDE);
   }

   GTK_WIDGET_UNSET_FLAGS(widget,GTK_VISIBLE);

   gtk_widget_lose_focus(widget);

   gtk_widget_size_request(widget,&req);
   if (GTK_WIDGET_REALIZED(widget)) {
      window=gtk_widget_get_ancestor(widget,GTK_TYPE_WINDOW);
      if (window) {
         alloc.x=alloc.y=0;
         alloc.width=window->requisition.width;
         alloc.height=window->requisition.height;
         gtk_window_handle_auto_size(GTK_WINDOW(window),&alloc);
         gtk_widget_set_size(window,&alloc);
      }
   }
}

void gtk_widget_set_focus(GtkWidget *widget) {
   GtkWidget *window;
   if (!widget || !GTK_WIDGET_CAN_FOCUS(widget) ||
       !GTK_WIDGET_SENSITIVE(widget) || !GTK_WIDGET_VISIBLE(widget)) return;
   window=gtk_widget_get_ancestor(widget,GTK_TYPE_WINDOW);
   gtk_window_update_focus(GTK_WINDOW(window));
   if (!window || GTK_WINDOW(window)->focus) return;

// g_print("Window %p focus set to widget %p (%s)\n",window,widget,GTK_OBJECT(widget)->klass->Name);
   GTK_WINDOW(window)->focus=widget;
   if (widget->hWnd) {
//    if (!SetFocus(widget->hWnd)) g_print("SetFocus failed on widget %p\n",widget);
      SetFocus(widget->hWnd);
   }
// else g_print("Cannot call SetFocus - no hWnd\n");
}

static BOOL CALLBACK SetFocusEnum(HWND hWnd,LPARAM data) {
   GtkWidget *widget;
   GtkWindow *window=GTK_WINDOW(data);
   widget=GTK_WIDGET(GetWindowLong(hWnd,GWL_USERDATA));
   if (!widget || !GTK_WIDGET_CAN_FOCUS(widget) ||
       !GTK_WIDGET_SENSITIVE(widget) || !GTK_WIDGET_VISIBLE(widget) ||
       window->focus==widget) {
      return TRUE;
   } else {
//g_print("gtk_window_set_focus: focus set to widget %p\n",widget);
      window->focus=widget;
      SetFocus(widget->hWnd);
      return FALSE;
   }
}

void gtk_window_set_focus(GtkWindow *window) {
   if (!window||!GTK_WIDGET(window)->hWnd) return;
   EnumChildWindows(GTK_WIDGET(window)->hWnd,SetFocusEnum,(LPARAM)window);
}

void gtk_widget_lose_focus(GtkWidget *widget) {
   GtkWidget *window;
   if (!widget || !GTK_WIDGET_CAN_FOCUS(widget)) return;
   window=gtk_widget_get_ancestor(widget,GTK_TYPE_WINDOW);
   gtk_window_update_focus(GTK_WINDOW(window));
   if (GTK_WINDOW(window)->focus==widget) {
      gtk_window_set_focus(GTK_WINDOW(window));
   }
}

void gtk_window_update_focus(GtkWindow *window) {
   GtkWidget *widget;
   HWND FocusWnd;
   if (GTK_WIDGET(window)->hWnd != GetActiveWindow()) return;
   FocusWnd=GetFocus();
   window->focus=NULL;
   if (FocusWnd) {
      widget=GTK_WIDGET(GetWindowLong(FocusWnd,GWL_USERDATA));
      if (widget && GTK_WIDGET(window)->hWnd &&
          GetParent(FocusWnd)==GTK_WIDGET(window)->hWnd) {
         window->focus=widget;
      } /*else g_print("Widget %p is not child of window %p\n",widget,window);*/
   }// else g_print("GetFocus returned NULL\n");
}

void gtk_widget_realize(GtkWidget *widget) {
   GtkRequisition req;
   if (GTK_WIDGET_REALIZED(widget)) return;
/* g_print("Realizing widget %p of class %s\n",widget,GTK_OBJECT(widget)->klass->Name);*/
   gtk_signal_emit(GTK_OBJECT(widget),"realize",&req);
   if (widget->hWnd) SetWindowLong(widget->hWnd,GWL_USERDATA,(LONG)widget);
   GTK_WIDGET_SET_FLAGS(widget,GTK_REALIZED);
   gtk_widget_set_sensitive(widget,GTK_WIDGET_SENSITIVE(widget));

   gtk_widget_size_request(widget,&req);
}

void gtk_widget_create(GtkWidget *widget) {
   GTK_WIDGET_SET_FLAGS(widget,GTK_SENSITIVE);
}

void gtk_widget_destroy(GtkWidget *widget) {
   if (!widget) return;
   gtk_widget_lose_focus(widget);
// g_print("gtk_widget_destroy on widget %p\n",widget);
   gtk_signal_emit(GTK_OBJECT(widget),"destroy");
// g_print("Freeing widget\n");
   g_free(widget);
// g_print("Widget freed\n");
}

void gtk_widget_set_sensitive(GtkWidget *widget,gboolean sensitive) {
   if (sensitive) {
      GTK_WIDGET_SET_FLAGS(widget,GTK_SENSITIVE);
      if (widget->hWnd) EnableWindow(widget->hWnd,sensitive);
      gtk_widget_set_focus(widget);
   } else {
      GTK_WIDGET_UNSET_FLAGS(widget,GTK_SENSITIVE);
      gtk_widget_lose_focus(widget);
      if (widget->hWnd) EnableWindow(widget->hWnd,sensitive);
   }

   gtk_signal_emit(GTK_OBJECT(widget),sensitive ? "enable" : "disable");
   if (sensitive && widget->hWnd && GTK_OBJECT(widget)->klass==&GtkWindowClass)
      SetActiveWindow(widget->hWnd);
}

void gtk_widget_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   GtkRequisition req;
   requisition->width=requisition->height=0;
   if (GTK_WIDGET_VISIBLE(widget)) {
      gtk_signal_emit(GTK_OBJECT(widget),"size_request",requisition);
   }
   if (requisition->width != widget->requisition.width ||
       requisition->height != widget->requisition.height) {
      memcpy(&widget->requisition,requisition,sizeof(GtkRequisition));
      if (widget->parent) gtk_widget_size_request(widget->parent,&req);
   }
}

void gtk_widget_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   gtk_signal_emit(GTK_OBJECT(widget),"set_size",allocation);
   memcpy(&widget->allocation,allocation,sizeof(GtkAllocation));
/* g_print("Widget pos set to %d,%d, size %d,%d\n",
           allocation->x,allocation->y,
           allocation->width,allocation->height);*/
   if (widget->hWnd) {
      SetWindowPos(widget->hWnd,HWND_TOP,
                 allocation->x,allocation->y,
                 allocation->width,allocation->height,
                 SWP_NOZORDER |
                 (GTK_OBJECT(widget)->klass==&GtkWindowClass ? SWP_NOMOVE :
                                                               0));
   }
}

GtkWidget *gtk_window_new(GtkWindowType type) {
   GtkWindow *win;

   win=GTK_WINDOW(GtkNewObject(&GtkWindowClass));
// g_print("New window %p created\n",win);

   win->title=g_strdup("");
   win->type=type;
   
   return GTK_WIDGET(win);
}

void gtk_window_set_title(GtkWindow *window,const gchar *title) {
   g_free(window->title);
   window->title=g_strdup(title);
}

gint gtk_window_delete_event(GtkWidget *widget,GdkEvent *event) {
   gtk_widget_destroy(widget);
   return TRUE;
}

void gtk_window_set_default_size(GtkWindow *window,gint width,gint height) {
   window->default_width = width;
   window->default_height = height;
}

void gtk_window_set_transient_for(GtkWindow *window,GtkWindow *parent) {
   if (window && parent) {
      GTK_WIDGET(window)->parent = GTK_WIDGET(parent);
      if (GTK_WIDGET(window)->hWnd && GTK_WIDGET(parent)->hWnd) {
         SetParent(GTK_WIDGET(window)->hWnd,GTK_WIDGET(parent)->hWnd);
      }
   }
}

void gtk_window_set_menu(GtkWindow *window,GtkMenuBar *menu_bar) {
   HWND hWnd;
   HMENU hMenu;

   hWnd=GTK_WIDGET(window)->hWnd;
   hMenu=GTK_MENU_SHELL(menu_bar)->menu;

   if (hWnd && hMenu) SetMenu(hWnd,hMenu);
   window->menu_bar=menu_bar;
}

void gtk_container_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   GtkContainer *container;
   GtkAllocation child_alloc;
   container=GTK_CONTAINER(widget);
   if (container->child) {
      child_alloc.x=allocation->x+container->border_width;
      child_alloc.y=allocation->y+container->border_width;
      child_alloc.width=allocation->width-container->border_width*2;
      child_alloc.height=allocation->height-container->border_width*2;
      gtk_widget_set_size(container->child,&child_alloc);
   }
}

void gtk_frame_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   GtkFrame *frame;
   GtkAllocation child_alloc;

   frame=GTK_FRAME(widget);
   child_alloc.x = allocation->x+3;
   child_alloc.y = allocation->y + 3 + frame->label_req.height;
   child_alloc.width = allocation->width - 6;
   child_alloc.height = allocation->height - frame->label_req.height - 6;
   gtk_container_set_size(widget,&child_alloc);
}

void gtk_container_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   GtkContainer *container;
/*g_print("gtk_container_size_request\n");*/
   container=GTK_CONTAINER(widget);
   if (container->child) {
/*g_print("requesting size of child\n");*/
      requisition->width=container->child->requisition.width+
                         container->border_width*2;
      requisition->height=container->child->requisition.height+
                          container->border_width*2;
/*    g_print("Container requesting size %d by %d\n",requisition->width,
              requisition->height);*/
   }
}

void gtk_window_size_request(GtkWidget *widget,GtkRequisition *requisition) {
/*g_print("gtk_window_size_request\n");*/
   gtk_container_size_request(widget,requisition);
   requisition->width+=GetSystemMetrics(SM_CXSIZEFRAME)*2;
   requisition->height+=GetSystemMetrics(SM_CYSIZEFRAME)*2+
                        GetSystemMetrics(SM_CYCAPTION);
   if (GTK_WINDOW(widget)->menu_bar) {
      requisition->height+=GetSystemMetrics(SM_CYMENU);
   }
}

void gtk_window_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   GtkAllocation child_alloc;
   GtkWindow *window=GTK_WINDOW(widget);

   child_alloc.x=child_alloc.y=0;
   child_alloc.width=allocation->width-GetSystemMetrics(SM_CXSIZEFRAME)*2;
   child_alloc.height=allocation->height-GetSystemMetrics(SM_CYSIZEFRAME)*2
                                        -GetSystemMetrics(SM_CYCAPTION);
   if (window->menu_bar) {
      child_alloc.height-=GetSystemMetrics(SM_CYMENU);
   }
   gtk_container_set_size(widget,&child_alloc);
}

void gtk_button_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   SIZE size;
   GtkButton *but=GTK_BUTTON(widget);

   gtk_container_size_request(widget,requisition);

   if (GetTextSize(widget->hWnd,but->text,&size)) {
      requisition->width = size.cx + 15;
      requisition->height = size.cy + 10;
   }
/* g_print("Button %p requesting size %d by %d\n",widget->hWnd,
           requisition->width,requisition->height);*/
}

BOOL GetTextSize(HWND hWnd,char *text,LPSIZE lpSize) {
   HDC hDC;
   BOOL RetVal=0;
   SIZE LineSize;
   HFONT oldFont;
   char *endpt,*startpt;
   hDC=GetDC(hWnd);
   oldFont=SelectObject(hDC,hFont);

   startpt=text;
   lpSize->cx=lpSize->cy=0;

   while (startpt) {
      endpt=startpt;
      while (endpt && *endpt!='\n' && *endpt) endpt++;
      if (endpt) {
         if ((endpt==startpt && GetTextExtentPoint32(hDC,"W",1,&LineSize)) ||
             (endpt!=startpt && GetTextExtentPoint32(hDC,startpt,
                                       endpt-startpt,&LineSize))) {
            RetVal=1;
            if (LineSize.cx > lpSize->cx) lpSize->cx=LineSize.cx;
            lpSize->cy+=LineSize.cy;
         }
         if (*endpt=='\0') break;
         startpt=endpt+1;
      } else break;
   }
   SelectObject(hDC,oldFont);
   ReleaseDC(hWnd,hDC);
   return RetVal;
}

void gtk_entry_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   SIZE size;
   if (GetTextSize(widget->hWnd,"Sample text",&size)) {
      requisition->width = size.cx;
      requisition->height = size.cy+8;
   }
}

void gtk_text_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   SIZE size;
   if (GetTextSize(widget->hWnd,"Sample text",&size)) {
      requisition->width = size.cx;
      requisition->height = size.cy*2+8;
   }
}

void gtk_frame_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   SIZE size;
   GtkFrame *frame=GTK_FRAME(widget);

   gtk_container_size_request(widget,requisition);

   if (GetTextSize(widget->hWnd,frame->text,&size)) {
      frame->label_req.width = size.cx;
      frame->label_req.height = size.cy;
      if (size.cx > requisition->width) requisition->width=size.cx;
      requisition->width += 6;
      requisition->height += size.cy+6;
   }
/* g_print("Frame requesting size %d by %d\n",requisition->width,
           requisition->height);*/
}


void gtk_check_button_size_request(GtkWidget *widget,
                                   GtkRequisition *requisition) {
   gtk_button_size_request(widget,requisition);
   requisition->width += 10;
}

GtkWidget *gtk_button_new_with_label(const gchar *label) {
   GtkButton *but;
   gint i;

   but=GTK_BUTTON(GtkNewObject(&GtkButtonClass));
   but->text = g_strdup(label);
   for (i=0;i<strlen(but->text);i++) {
      if (but->text[i]=='_') but->text[i]='&';
   }

   return GTK_WIDGET(but);
}

GtkWidget *gtk_check_button_new_with_label(const gchar *label) {
   GtkButton *but;
   gint i;

   but=GTK_BUTTON(GtkNewObject(&GtkCheckButtonClass));
   but->text = g_strdup(label);
   for (i=0;i<strlen(but->text);i++) {
      if (but->text[i]=='_') but->text[i]='&';
   }

   return GTK_WIDGET(but);
}

GtkWidget *gtk_radio_button_new_with_label_from_widget(GtkRadioButton *group,
                                                       const gchar *label) {
   GSList *list;

   list=gtk_radio_button_group(group);
   return (gtk_radio_button_new_with_label(list,label));
}

GtkWidget *gtk_radio_button_new_with_label(GSList *group,const gchar *label) {
   GtkButton *but;
   GtkRadioButton *radio;
   GSList *listpt;
   gint i;

   but=GTK_BUTTON(GtkNewObject(&GtkRadioButtonClass));
   but->text = g_strdup(label);
   for (i=0;i<strlen(but->text);i++) {
      if (but->text[i]=='_') but->text[i]='&';
   }

   if (group==NULL) GTK_TOGGLE_BUTTON(but)->toggled=TRUE;

   group=g_slist_append(group,GTK_RADIO_BUTTON(but));
   for (listpt=group;listpt;listpt=g_slist_next(listpt)) {
      radio=GTK_RADIO_BUTTON(listpt->data);
      radio->group = group;
   }

   return GTK_WIDGET(but);
}

GtkWidget *gtk_label_new(const gchar *text) {
   GtkLabel *label;

   label=GTK_LABEL(GtkNewObject(&GtkLabelClass));
   gtk_label_set_text(label,text);

   return GTK_WIDGET(label);
}

GtkWidget *gtk_hbox_new(gboolean homogeneous,gint spacing) {
   GtkBox *hbox;

   hbox=GTK_BOX(GtkNewObject(&GtkHBoxClass));

   hbox->spacing=spacing;
   hbox->homogeneous=homogeneous;
   return GTK_WIDGET(hbox);
}

GtkWidget *gtk_vbox_new(gboolean homogeneous,gint spacing) {
   GtkBox *vbox;

   vbox=GTK_BOX(GtkNewObject(&GtkVBoxClass));

   vbox->spacing=spacing;
   vbox->homogeneous=homogeneous;
   return GTK_WIDGET(vbox);
}

GtkWidget *gtk_frame_new(const gchar *text) {
   GtkFrame *frame;

   frame=GTK_FRAME(GtkNewObject(&GtkFrameClass));
   frame->text = g_strdup(text);
/*g_print("Frame created with caption %s\n",frame->text);*/

   return GTK_WIDGET(frame);
}

GtkWidget *gtk_text_new(GtkAdjustment *hadj,GtkAdjustment *vadj) {
   return GTK_WIDGET(GtkNewObject(&GtkTextClass));
}

GtkWidget *gtk_scrolled_text_new(GtkAdjustment *hadj,GtkAdjustment *vadj,
                                 GtkWidget **pack_widg) {
   GtkWidget *text;
   text=gtk_text_new(hadj,vadj);
   *pack_widg=text;
   return text;
}

GtkWidget *gtk_entry_new() {
   return GTK_WIDGET(GtkNewObject(&GtkEntryClass));
}

GtkWidget *gtk_clist_new(gint columns) {
   GtkCList *clist;
   int i;

   clist=GTK_CLIST(GtkNewObject(&GtkCListClass));
   clist->ncols=columns;
   clist->cols=g_new0(GtkCListColumn,columns);
   for (i=0;i<columns;i++) {
      clist->cols[i].width=50;
      clist->cols[i].visible=TRUE;
      clist->cols[i].resizeable=TRUE;
   }

   return GTK_WIDGET(clist);
}

GSList *gtk_radio_button_group(GtkRadioButton *radio_button) {
   return radio_button->group;
}

static void gtk_editable_sync_text(GtkEditable *editable) {
   HWND hWnd;
   gint textlen;
   gchar *buffer;

   hWnd=GTK_WIDGET(editable)->hWnd;
   if (!hWnd) return;

   textlen=SendMessage(hWnd,WM_GETTEXTLENGTH,0,0);
   buffer=g_new(gchar,textlen+1);
   SendMessage(hWnd,WM_GETTEXT,(WPARAM)(textlen+1),(LPARAM)buffer);
   g_string_assign(editable->text,buffer);
   g_free(buffer);
}

void gtk_editable_insert_text(GtkEditable *editable,const gchar *new_text,
                              gint new_text_length,gint *position) {
   GtkWidget *widget=GTK_WIDGET(editable);
   HWND hWnd;
   gint i;

   gtk_editable_sync_text(editable);
   g_string_insert(editable->text,*position,new_text);
   for (i=*position;i<*position+strlen(new_text);i++) {
      if (editable->text->str[i]=='\r' &&
          editable->text->str[i+1]=='\n') {
         i++;
      } else if (editable->text->str[i]=='\n') {
         g_string_insert_c(editable->text,i,'\r');
         i++;
         (*position)++;
      }
   }

   hWnd=widget->hWnd;
   if (hWnd) {
      SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)editable->text->str);
      *position+=strlen(new_text);
      gtk_editable_set_position(editable,*position);
   }
}

void gtk_editable_delete_text(GtkEditable *editable,
                              gint start_pos,gint end_pos) {
   GtkWidget *widget=GTK_WIDGET(editable);
   HWND hWnd;

   gtk_editable_sync_text(editable);
   if (end_pos < 0 || end_pos >= editable->text->len)
      end_pos=editable->text->len;
   g_string_erase(editable->text,start_pos,end_pos-start_pos);

   hWnd=widget->hWnd;
   if (hWnd) {
      SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)editable->text->str);
   }
}

gchar *gtk_editable_get_chars(GtkEditable *editable,
                              gint start_pos,gint end_pos) {
   gchar *retbuf;
   gint copylen;
   gtk_editable_sync_text(editable);
   if (end_pos < 0 || end_pos >= editable->text->len)
      end_pos=editable->text->len;
   copylen=end_pos-start_pos+1;
   retbuf=g_new(gchar,copylen);
   memcpy(retbuf,&editable->text->str[start_pos],copylen);
   retbuf[copylen]='\0';
   return retbuf;
}

void gtk_editable_set_editable(GtkEditable *editable,gboolean is_editable) {
   GtkWidget *widget=GTK_WIDGET(editable);
   HWND hWnd;
   editable->is_editable=is_editable;
   hWnd=widget->hWnd;
   if (hWnd) SendMessage(hWnd,EM_SETREADONLY,(WPARAM)(!is_editable),(LPARAM)0);
}

void gtk_editable_set_position(GtkEditable *editable,gint position) {
   GtkWidget *widget=GTK_WIDGET(editable);
   HWND hWnd;
   if (!GTK_WIDGET_REALIZED(widget)) return;
   hWnd=widget->hWnd;
   SendMessage(hWnd,EM_SETSEL,(WPARAM)-1,(LPARAM)position);
   SendMessage(hWnd,EM_SCROLLCARET,0,0);
   SendMessage(hWnd,EM_LINESCROLL,0,(LPARAM)1000);
}

gint gtk_editable_get_position(GtkEditable *editable) {
   GtkWidget *widget=GTK_WIDGET(editable);
   HWND hWnd;
   DWORD EndPos;
   if (!GTK_WIDGET_REALIZED(widget)) return 0;
   hWnd=widget->hWnd;
   SendMessage(hWnd,EM_GETSEL,(WPARAM)NULL,(LPARAM)&EndPos);
   return (gint)EndPos;
}

guint gtk_text_get_length(GtkText *text) {
   return GTK_EDITABLE(text)->text->len;
}

void gtk_box_pack_start(GtkBox *box,GtkWidget *child,gboolean Expand,
                        gboolean Fill,gint Padding) {
   GtkBoxChild *newChild;

   newChild=g_new0(GtkBoxChild,1);
   newChild->widget=child;
   newChild->expand=Expand;
   newChild->fill=Fill;

   box->children = g_list_append(box->children,(gpointer)newChild);
   child->parent = GTK_WIDGET(box);
   if (GTK_WIDGET_REALIZED(GTK_WIDGET(box))) {
      gtk_widget_realize(child);
      gtk_widget_update(GTK_WIDGET(box),TRUE);
   }
}

void gtk_button_destroy(GtkWidget *widget) {
/* g_print("Destroying button %p\n",widget);*/
   g_free(GTK_BUTTON(widget)->text);
}

void gtk_frame_destroy(GtkWidget *widget) {
   gtk_container_destroy(widget);
   g_free(GTK_FRAME(widget)->text);
}

void gtk_container_destroy(GtkWidget *widget) {
   GtkWidget *child=GTK_CONTAINER(widget)->child;
/* g_print("Destroying container %p\n",widget);
   if (child) g_print("Destroying container child %p\n",child);*/
   if (child) gtk_widget_destroy(child);
}

void gtk_box_destroy(GtkWidget *widget) {
   GtkBoxChild *child;
   GList *children;
   gtk_container_destroy(widget);

   for (children=GTK_BOX(widget)->children;children;
        children=g_list_next(children)) {
      child=(GtkBoxChild *)(children->data);
/*    if (child && child->widget) g_print("Destroying box child %p\n",child->widget);*/
      if (child && child->widget) gtk_widget_destroy(child->widget);
      g_free(child);
   }
   g_list_free(GTK_BOX(widget)->children);
}

static void EnableParent(GtkWindow *window) {
   GSList *list;
   GtkWidget *parent;
   GtkWindow *listwin;

   parent=GTK_WIDGET(window)->parent;
   if (window->modal && parent) {
      for (list=WindowList;list;list=g_slist_next(list)) {
         listwin=GTK_WINDOW(list->data);
         if (listwin!=window && listwin->modal &&
             GTK_WIDGET_VISIBLE(GTK_WIDGET(listwin)) &&
             GTK_WIDGET(listwin)->parent==parent) return;
      }
      gtk_widget_set_sensitive(parent,TRUE);
   }
}

void gtk_window_destroy(GtkWidget *widget) {
   GtkWindow *window=GTK_WINDOW(widget);
// g_print("gtk_window_destroy on widget %p\n",widget);
   WindowList=g_slist_remove(WindowList,(gpointer)window);
   gtk_container_destroy(widget);
   g_free(GTK_WINDOW(widget)->title);
   EnableParent(window);
   if (widget->hWnd) DestroyWindow(widget->hWnd);
   widget->hWnd=NULL;
}

void gtk_window_show(GtkWidget *widget) {
   GtkWindow *window=GTK_WINDOW(widget);
   if (window->modal && widget->parent)
      gtk_widget_set_sensitive(widget->parent,FALSE);
}

void gtk_window_hide(GtkWidget *widget) {
   GtkWindow *window=GTK_WINDOW(widget);
   EnableParent(window);
}

void gtk_hbox_size_request(GtkWidget *widget,
                           GtkRequisition *requisition) {
   GtkBoxChild *child;
   GList *children;
   GtkRequisition *child_req;
   gint spacing=GTK_BOX(widget)->spacing,numchildren=0;
   gint maxreq=0;
   gboolean homogeneous=GTK_BOX(widget)->homogeneous;

   for (children=GTK_BOX(widget)->children;children;
        children=g_list_next(children)) {
      child=(GtkBoxChild *)(children->data);
      if (child && child->widget && GTK_WIDGET_VISIBLE(child->widget)) {
         child_req=&child->widget->requisition;
         if (homogeneous) {
            numchildren++;
            if (child_req->width > maxreq) maxreq=child_req->width;
         } else {
            requisition->width+=child_req->width;
         }
         if (g_list_next(children)) requisition->width+=spacing;
         if (child_req->height > requisition->height)
            requisition->height=child_req->height;
      }
   }
   if (homogeneous) requisition->width+=numchildren*maxreq;
   GTK_BOX(widget)->maxreq=maxreq;
   requisition->width+=2*GTK_CONTAINER(widget)->border_width;
   requisition->height+=2*GTK_CONTAINER(widget)->border_width;
}

void gtk_vbox_size_request(GtkWidget *widget,
                           GtkRequisition *requisition) {
   GtkBoxChild *child;
   GList *children;
   GtkRequisition *child_req;
   gint spacing=GTK_BOX(widget)->spacing,numchildren=0;
   gint maxreq=0;
   gboolean homogeneous=GTK_BOX(widget)->homogeneous;

   for (children=GTK_BOX(widget)->children;children;
        children=g_list_next(children)) {
      child=(GtkBoxChild *)(children->data);
      if (child && child->widget && GTK_WIDGET_VISIBLE(child->widget)) {
         child_req=&child->widget->requisition;
         if (homogeneous) {
            numchildren++;
            if (child_req->height > maxreq) maxreq=child_req->height;
         } else {
            requisition->height+=child_req->height;
         }
         if (g_list_next(children)) requisition->height+=spacing;
         if (child_req->width > requisition->width)
            requisition->width=child_req->width;
      }
   }
   if (homogeneous) requisition->height+=numchildren*maxreq;
   GTK_BOX(widget)->maxreq=maxreq;
   requisition->width+=2*GTK_CONTAINER(widget)->border_width;
   requisition->height+=2*GTK_CONTAINER(widget)->border_width;
}

static void gtk_box_count_children(GtkBox *box,gint16 allocation,
                                   gint16 requisition,gint *extra) {
   GtkBoxChild *child;
   GList *children;
   gint NumCanExpand=0;

   for (children=box->children;children;children=g_list_next(children)) {
      child=(GtkBoxChild *)(children->data);
      if (child && child->widget && GTK_WIDGET_VISIBLE(child->widget) &&
          child->expand) NumCanExpand++;
   }

   *extra = allocation-requisition;
   if (NumCanExpand>0) *extra /= NumCanExpand;
}

static void gtk_box_size_child(GtkBox *box,GtkBoxChild *child,
                               gint extra,gint16 maxpos,gint16 requisition,
                               gint16 *pos,gint16 *size,
                               GList *listpt,gint16 *curpos) {
   gboolean TooSmall=FALSE;
   *pos=*curpos;
   if (extra<0) {
      extra=0;
      TooSmall=TRUE;
   }
   if (child->expand && child->fill) {
      *size=requisition+extra;
      *curpos+=requisition+extra;
   } else if (child->expand) {
      *size=requisition;
      *pos+=extra/2;
      *curpos+=requisition+extra;
   } else {
      *size=requisition;
      *curpos+=requisition;
   }
   if (g_list_next(listpt)) *curpos+=box->spacing;
   if (TooSmall) {
      if (*pos >= maxpos) { *pos=*size=0; }
      else if (*pos+*size > maxpos) { *size=maxpos-*pos; }
   }
}

void gtk_hbox_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   GtkBox *box;
   GtkBoxChild *child;
   GList *children;
   GtkAllocation child_alloc;
   gint extra;
   gint16 curpos;
   gint maxpos,height,border_width;

   border_width=GTK_CONTAINER(widget)->border_width;
   maxpos=allocation->x+allocation->width-border_width;
   height=allocation->height-2*border_width;

   box=GTK_BOX(widget);

   curpos = allocation->x+border_width;
   gtk_box_count_children(box,allocation->width,widget->requisition.width,
                          &extra);

   for (children=box->children;children;children=g_list_next(children)) {
      child=(GtkBoxChild *)(children->data);
      if (child && child->widget && GTK_WIDGET_VISIBLE(child->widget)) {
         gtk_box_size_child(box,child,extra,maxpos,
                            box->homogeneous ? box->maxreq :
                                child->widget->requisition.width,
                            &child_alloc.x,&child_alloc.width,
                            children,&curpos);
         child_alloc.y = allocation->y+border_width;
         child_alloc.height = height;
         gtk_widget_set_size(child->widget,&child_alloc);
      }
   }
}

void gtk_vbox_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   GtkBox *box;
   GtkBoxChild *child;
   GList *children;
   GtkAllocation child_alloc;
   gint extra;
   gint16 curpos;
   gint width,maxpos,border_width;

   border_width=GTK_CONTAINER(widget)->border_width;
   width=allocation->width-2*border_width;
   maxpos=allocation->y+allocation->height-border_width;

   box=GTK_BOX(widget);

   curpos = allocation->y+border_width;
   gtk_box_count_children(box,allocation->height,widget->requisition.height,
                          &extra);

   for (children=box->children;children;children=g_list_next(children)) {
      child=(GtkBoxChild *)(children->data);
      if (child && child->widget && GTK_WIDGET_VISIBLE(child->widget)) {
         gtk_box_size_child(box,child,extra,maxpos,
                            box->homogeneous ? box->maxreq :
                                child->widget->requisition.height,
                            &child_alloc.y,&child_alloc.height,
                            children,&curpos);
         child_alloc.x = allocation->x+border_width;
         child_alloc.width = width;
         gtk_widget_set_size(child->widget,&child_alloc);
      }
   }
}

void gtk_window_realize(GtkWidget *widget) {
   GtkWindow *win=GTK_WINDOW(widget);
   HWND Parent;
   Parent=gtk_get_parent_hwnd(widget->parent);
   if (win->type==GTK_WINDOW_TOPLEVEL) {
      widget->hWnd = CreateWindow("mainwin",win->title,
                        WS_OVERLAPPEDWINDOW|CS_HREDRAW|CS_VREDRAW|WS_SIZEBOX,
                        CW_USEDEFAULT,0,0,0,Parent,NULL,hInst,NULL);
      if (!TopLevel) TopLevel=widget->hWnd;
   } else {
      widget->hWnd = CreateWindow(WC_GTKDIALOG,win->title,
                        WS_CAPTION|WS_SYSMENU|CS_HREDRAW|CS_VREDRAW|WS_SIZEBOX,
                        CW_USEDEFAULT,0,0,0,Parent,NULL,hInst,NULL);
   }
   WindowList=g_slist_append(WindowList,(gpointer)win);
   gtk_set_default_font(widget->hWnd);
/* g_print("Window window %p created\n",widget->hWnd);*/
   gtk_container_realize(widget);
// if (win->focus && win->focus->hWnd) SetFocus(win->focus->hWnd);
}

void gtk_container_realize(GtkWidget *widget) {
   GtkWidget *child=GTK_CONTAINER(widget)->child;
/*g_print("Realizing container\n");
   if (child) g_print("Realizing container widget %p\n",child);*/
   if (child) gtk_widget_realize(child);
}

void gtk_box_realize(GtkWidget *widget) {
   GtkBoxChild *child;
   GList *children;

/* g_print("Realizing box...\n");*/
   gtk_container_realize(widget);

   for (children=GTK_BOX(widget)->children;children;
        children=g_list_next(children)) {
     child=(GtkBoxChild *)(children->data);
/*   if (child) g_print("Realizing box widget %p\n",child);*/
     if (child && child->widget) gtk_widget_realize(child->widget);
   }
}

HWND gtk_get_parent_hwnd(GtkWidget *widget) {
   widget=gtk_widget_get_ancestor(widget,GTK_TYPE_WINDOW);
   if (widget) return widget->hWnd; else return NULL;
}

void gtk_button_realize(GtkWidget *widget) {
   GtkButton *but=GTK_BUTTON(widget);
   HWND Parent;
   GTK_WIDGET_SET_FLAGS(widget,GTK_CAN_FOCUS);
   Parent=gtk_get_parent_hwnd(widget);
   widget->hWnd = CreateWindow("BUTTON",but->text,
                            WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,
                            widget->allocation.x,widget->allocation.y,
                            widget->allocation.width,widget->allocation.height,
                            Parent,NULL,hInst,NULL);
   gtk_set_default_font(widget->hWnd);
}

void gtk_entry_realize(GtkWidget *widget) {
   HWND Parent;
   Parent=gtk_get_parent_hwnd(widget);
   GTK_WIDGET_SET_FLAGS(widget,GTK_CAN_FOCUS);
   widget->hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
                            WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,
                            widget->allocation.x,widget->allocation.y,
                            widget->allocation.width,widget->allocation.height,
                            Parent,NULL,hInst,NULL);
   gtk_set_default_font(widget->hWnd);
   gtk_editable_set_editable(GTK_EDITABLE(widget),
                             GTK_EDITABLE(widget)->is_editable);
   SendMessage(widget->hWnd,WM_SETTEXT,0,
               (LPARAM)GTK_EDITABLE(widget)->text->str);
}

void gtk_text_realize(GtkWidget *widget) {
   HWND Parent;
   Parent=gtk_get_parent_hwnd(widget);
   GTK_WIDGET_SET_FLAGS(widget,GTK_CAN_FOCUS);
   widget->hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
                            WS_CHILD|WS_TABSTOP|
                            ES_MULTILINE|ES_WANTRETURN|WS_VSCROLL|
                            (GTK_TEXT(widget)->word_wrap ? 0 : ES_AUTOHSCROLL),
                            0,0,0,0,Parent,NULL,hInst,NULL);
   gtk_set_default_font(widget->hWnd);
   gtk_editable_set_editable(GTK_EDITABLE(widget),
                             GTK_EDITABLE(widget)->is_editable);
   SendMessage(widget->hWnd,WM_SETTEXT,0,
               (LPARAM)GTK_EDITABLE(widget)->text->str);
}

void gtk_frame_realize(GtkWidget *widget) {
   GtkFrame *frame=GTK_FRAME(widget);
   HWND Parent;
   gtk_container_realize(widget);
   Parent=gtk_get_parent_hwnd(widget);
/*g_print("Creating frame window with caption %s\n",frame->text);*/
   widget->hWnd = CreateWindow("BUTTON",frame->text,
                            WS_CHILD|BS_GROUPBOX,
                            widget->allocation.x,widget->allocation.y,
                            widget->allocation.width,widget->allocation.height,
                            Parent,NULL,hInst,NULL);
   gtk_set_default_font(widget->hWnd);
}

void gtk_check_button_realize(GtkWidget *widget) {
   GtkButton *but=GTK_BUTTON(widget);
   HWND Parent;
   gboolean toggled;
   Parent=gtk_get_parent_hwnd(widget);
   GTK_WIDGET_SET_FLAGS(widget,GTK_CAN_FOCUS);
   widget->hWnd = CreateWindow("BUTTON",but->text,
                            WS_CHILD|WS_TABSTOP|BS_CHECKBOX,
                            widget->allocation.x,widget->allocation.y,
                            widget->allocation.width,widget->allocation.height,
                            Parent,NULL,hInst,NULL);
   gtk_set_default_font(widget->hWnd);
   gtk_signal_connect(GTK_OBJECT(widget),"clicked",
                      gtk_toggle_button_toggled,NULL);
   gtk_signal_connect(GTK_OBJECT(widget),"toggled",
                      gtk_check_button_toggled,NULL);
   toggled=GTK_TOGGLE_BUTTON(widget)->toggled;
   GTK_TOGGLE_BUTTON(widget)->toggled = !toggled;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),toggled);
}

void gtk_radio_button_realize(GtkWidget *widget) {
   GtkButton *but=GTK_BUTTON(widget);
   HWND Parent;
   gboolean toggled;
   Parent=gtk_get_parent_hwnd(widget);
   GTK_WIDGET_SET_FLAGS(widget,GTK_CAN_FOCUS);
   widget->hWnd = CreateWindow("BUTTON",but->text,
                            WS_CHILD|WS_TABSTOP|BS_RADIOBUTTON,
                            widget->allocation.x,widget->allocation.y,
                            widget->allocation.width,widget->allocation.height,
                            Parent,NULL,hInst,NULL);
   gtk_set_default_font(widget->hWnd);
   gtk_signal_connect(GTK_OBJECT(widget),"clicked",
                      gtk_radio_button_clicked,NULL);
   gtk_signal_connect(GTK_OBJECT(widget),"toggled",
                      gtk_radio_button_toggled,NULL);
   toggled=GTK_TOGGLE_BUTTON(widget)->toggled;
   GTK_TOGGLE_BUTTON(widget)->toggled = !toggled;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),toggled);
}

void gtk_radio_button_destroy(GtkWidget *widget) {
   GSList *group,*listpt;
   GtkRadioButton *radio;

   gtk_button_destroy(widget);
   group=GTK_RADIO_BUTTON(widget)->group;
   group=g_slist_remove(group,GTK_RADIO_BUTTON(widget));
   for (listpt=group;listpt;listpt=g_slist_next(listpt)) {
      radio=GTK_RADIO_BUTTON(listpt->data);
      radio->group = group;
   }
}


void gtk_clist_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   SIZE size;

   if (GetTextSize(widget->hWnd,"Sample text",&size)) {
      requisition->width = size.cx;
      requisition->height = size.cy*6+12;
   }
}

void gtk_clist_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   GtkCList *clist=GTK_CLIST(widget);
   gtk_container_set_size(widget,allocation);
   if (clist->header) {
      SetWindowPos(clist->header,HWND_TOP,
                   allocation->x,allocation->y,
                   allocation->width,clist->header_size,
                   SWP_NOZORDER);
      allocation->y+=clist->header_size-1;
      allocation->height-=clist->header_size-1;
   }
}

void gtk_clist_realize(GtkWidget *widget) {
   HWND Parent,header;
   HD_LAYOUT hdl;
   HD_ITEM hdi;
   RECT rcParent;
   WINDOWPOS wp;
   GtkCList *clist=GTK_CLIST(widget);
   GSList *rows;
   GtkCListRow *row;
   gint i;

   gtk_container_realize(widget);
   Parent=gtk_get_parent_hwnd(widget);
   GTK_WIDGET_SET_FLAGS(widget,GTK_CAN_FOCUS);
   rcParent.left = rcParent.top = 0;
   rcParent.right = rcParent.bottom = 800;
   header=CreateWindowEx(0,WC_HEADER,NULL,
                         WS_CHILD|WS_BORDER|HDS_HORZ
                         | (GTK_CLIST(widget)->cols[0].button_passive ?
                            0 : HDS_BUTTONS),
                         0,0,0,0,Parent,NULL,hInst,NULL);
   SetWindowLong(header,GWL_USERDATA,(LONG)widget);
   GTK_CLIST(widget)->header=header;
   gtk_set_default_font(header);
   hdl.prc = &rcParent;
   hdl.pwpos = &wp;
   SendMessage(header,HDM_LAYOUT,0,(LPARAM)&hdl);
   clist->header_size=wp.cy;
/* g_print("Header %p, size %d\n",header,wp.cy);*/
   widget->hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,"LISTBOX","",
                                 WS_CHILD|WS_TABSTOP|LBS_DISABLENOSCROLL|
                                 WS_VSCROLL|LBS_USETABSTOPS|
                                 LBS_OWNERDRAWFIXED|LBS_NOTIFY,
                                 0,0,0,0,Parent,NULL,hInst,NULL);
   gtk_set_default_font(widget->hWnd);

   gtk_clist_update_all_widths(clist);
   for (rows=clist->rows;rows;rows=g_slist_next(rows)) {
      row=(GtkCListRow *)rows->data;
      if (row) SendMessage(widget->hWnd,LB_ADDSTRING,0,(LPARAM)row->data);
   }

   for (i=0;i<clist->ncols;i++) {
      hdi.mask = HDI_TEXT|HDI_FORMAT|HDI_WIDTH;
      hdi.pszText=clist->cols[i].title;
      if (hdi.pszText) {
         hdi.cxy=clist->cols[i].width;
         hdi.cchTextMax=strlen(hdi.pszText);
         hdi.fmt = HDF_LEFT|HDF_STRING;
         SendMessage(header,HDM_INSERTITEM,i+1,(LPARAM)&hdi);
      }
   }
}

void gtk_clist_show(GtkWidget *widget) {
   if (GTK_WIDGET_REALIZED(widget)) {
      ShowWindow(GTK_CLIST(widget)->header,SW_SHOWNORMAL);
   }
}

void gtk_clist_hide(GtkWidget *widget) {
   if (GTK_WIDGET_REALIZED(widget)) {
      ShowWindow(GTK_CLIST(widget)->header,SW_HIDE);
   }
}

void gtk_clist_draw_row(GtkCList *clist,LPDRAWITEMSTRUCT lpdis) {
   HBRUSH bkgrnd;
   COLORREF textcol,oldtextcol;
   RECT rcCol;
   gint i,CurrentX;
   GtkCListRow *row;

   if (lpdis->itemState&ODS_SELECTED) {
      bkgrnd=(HBRUSH)(1+COLOR_HIGHLIGHT);
      textcol=(COLORREF)GetSysColor(COLOR_HIGHLIGHTTEXT);
   } else {
      bkgrnd=(HBRUSH)(1+COLOR_WINDOW);
      textcol=(COLORREF)GetSysColor(COLOR_WINDOWTEXT);
   }
   oldtextcol=SetTextColor(lpdis->hDC,textcol);
   SetBkMode(lpdis->hDC,TRANSPARENT);
   FillRect(lpdis->hDC,&lpdis->rcItem,bkgrnd);

   if (lpdis->itemID >= 0 && lpdis->itemID < g_slist_length(clist->rows)) {
      row=(GtkCListRow *)g_slist_nth_data(clist->rows,lpdis->itemID);
      CurrentX=lpdis->rcItem.left;
      rcCol.top=lpdis->rcItem.top;
      rcCol.bottom=lpdis->rcItem.bottom;
      if (row->text) for (i=0;i<clist->ncols;i++) {
         rcCol.left=CurrentX+LISTITEMHPACK;
         CurrentX+=clist->cols[i].width;
         rcCol.right=CurrentX-LISTITEMHPACK;
         if (rcCol.left > lpdis->rcItem.right) rcCol.left=lpdis->rcItem.right;
         if (rcCol.right > lpdis->rcItem.right) rcCol.right=lpdis->rcItem.right;
         if (row->text[i]) DrawText(lpdis->hDC,row->text[i],-1,&rcCol,
                                    DT_LEFT|DT_SINGLELINE|DT_VCENTER);
      }
   }

   SetTextColor(lpdis->hDC,oldtextcol);
   SetBkMode(lpdis->hDC,OPAQUE);
   if (lpdis->itemState&ODS_FOCUS) DrawFocusRect(lpdis->hDC,&lpdis->rcItem);
}

void gtk_clist_do_auto_resize(GtkCList *clist) {
   gint i;
   for (i=0;i<clist->ncols;i++) if (clist->cols[i].auto_resize) {
      gtk_clist_set_column_width(clist,i,clist->cols[i].width);
   }
}

void gtk_clist_update_all_widths(GtkCList *clist) {
   GSList *list;
   GtkCListRow *row;
   gint i;
   SIZE size;
   HWND header;

   for (i=0;i<clist->ncols;i++) clist->cols[i].width=0;
   header=clist->header;
   if (header) for (i=0;i<clist->ncols;i++) {
      if (GetTextSize(header,clist->cols[i].title,&size)) {
         clist->cols[i].width=size.cx+2*LISTHEADERPACK;
      }
   }
   
   for (list=clist->rows;list;list=g_slist_next(list)) {
      row=(GtkCListRow *)list->data;
      if (row && row->text) gtk_clist_update_widths(clist,row->text);
   }
}

void gtk_clist_update_widths(GtkCList *clist,gchar *text[]) {
   gint i;
   SIZE size;
   HWND hWnd;
   hWnd=GTK_WIDGET(clist)->hWnd;
   if (!hWnd) return;
   for (i=0;i<clist->ncols;i++) {
      if (clist->cols[i].auto_resize && GetTextSize(hWnd,text[i],&size) &&
          size.cx+2*LISTITEMHPACK > clist->cols[i].width) {
         clist->cols[i].width = size.cx+2*LISTITEMHPACK;
      }
   }
}

gint gtk_clist_insert(GtkCList *clist,gint row,gchar *text[]) {
   GtkWidget *widget=GTK_WIDGET(clist);
   HWND hWnd;
   GtkCListRow *new_row;
   gint i;

   if (row<0) row=g_slist_length(clist->rows);

   new_row=g_new0(GtkCListRow,1);
   new_row->text=g_new0(gchar *,clist->ncols);
   for (i=0;i<clist->ncols;i++) {
      new_row->text[i]=g_strdup(text[i]);
   }
   gtk_clist_update_widths(clist,new_row->text);
   gtk_clist_do_auto_resize(clist);
   clist->rows=g_slist_insert(clist->rows,(gpointer)new_row,row);

   if (GTK_WIDGET_REALIZED(widget)) {
      hWnd=widget->hWnd;
      SendMessage(hWnd,LB_INSERTSTRING,(WPARAM)row,(LPARAM)new_row->data);
   }

   return row;
}

GtkWidget *gtk_clist_new_with_titles(gint columns,gchar *titles[]) {
   GtkWidget *widget;
   GtkCList *clist;
   gint i;

   widget=gtk_clist_new(columns);
   clist=GTK_CLIST(widget);
   for (i=0;i<clist->ncols;i++) {
      gtk_clist_set_column_title(clist,i,titles[i]);
   }
   return widget;
}

GtkWidget *gtk_scrolled_clist_new_with_titles(gint columns,gchar *titles[],
                                              GtkWidget **pack_widg) {
   GtkWidget *widget;
   widget=gtk_clist_new_with_titles(columns,titles);
   *pack_widg=widget;
   return widget;
}

gint gtk_clist_append(GtkCList *clist,gchar *text[]) {
   return gtk_clist_insert(clist,-1,text);
}

void gtk_clist_set_column_title(GtkCList *clist,gint column,
                                const gchar *title) {
   HWND hWnd;
   if (column<0 || column>=clist->ncols) return;
   g_free(clist->cols[column].title);
   clist->cols[column].title=g_strdup(title);
   if (GTK_WIDGET_REALIZED(GTK_WIDGET(clist))) {
      hWnd=GTK_WIDGET(clist)->hWnd;
      InvalidateRect(hWnd,NULL,FALSE);
      UpdateWindow(hWnd);
   }
}

void gtk_clist_column_title_passive(GtkCList *clist,gint column) {
   if (column>=0 && column<clist->ncols)
      clist->cols[column].button_passive=TRUE;
}

void gtk_clist_column_titles_passive(GtkCList *clist) {
   gint i;
   for (i=0;i<clist->ncols;i++) {
      gtk_clist_column_title_passive(clist,i);
   }
}

void gtk_clist_column_title_active(GtkCList *clist,gint column) {
   if (column>=0 && column<clist->ncols)
      clist->cols[column].button_passive=FALSE;
}

void gtk_clist_column_titles_active(GtkCList *clist) {
   gint i;
   for (i=0;i<clist->ncols;i++) {
      gtk_clist_column_title_active(clist,i);
   }
}

void gtk_clist_set_column_width(GtkCList *clist,gint column,gint width) {
   gtk_clist_set_column_width_full(clist,column,width,TRUE);
}

void gtk_clist_set_column_width_full(GtkCList *clist,gint column,gint width,
                                     gboolean ResizeHeader) {
   HWND hWnd,header;
   HD_ITEM hdi;

   if (column<0 || column>=clist->ncols) return;

   clist->cols[column].width=width;
   if (GTK_WIDGET_REALIZED(GTK_WIDGET(clist))) {
      header=clist->header;
      if (ResizeHeader && header) {
         hdi.mask=HDI_WIDTH;
         hdi.cxy=width;
         if (SendMessage(header,HDM_GETITEM,(WPARAM)column,(LPARAM)&hdi) &&
             hdi.cxy!=width) {
            hdi.mask=HDI_WIDTH;
            hdi.cxy=width;
            SendMessage(header,HDM_SETITEM,(WPARAM)column,(LPARAM)&hdi);
         }
      }
      hWnd=GTK_WIDGET(clist)->hWnd;
      if (hWnd /*&& clist->cols[column].width!=width*/)
         InvalidateRect(hWnd,NULL,FALSE);
   }
}

void gtk_clist_set_selection_mode(GtkCList *clist,GtkSelectionMode mode) {
   clist->mode=mode;
}

void gtk_container_show_all(GtkWidget *widget,gboolean hWndOnly) {
   GtkContainer *container=GTK_CONTAINER(widget);

   if (container->child) gtk_widget_show_all_full(container->child,hWndOnly);
}

void gtk_container_hide_all(GtkWidget *widget,gboolean hWndOnly) {
   GtkContainer *container=GTK_CONTAINER(widget);

   if (container->child) gtk_widget_hide_all_full(container->child,hWndOnly);
}

void gtk_box_show_all(GtkWidget *widget,gboolean hWndOnly) {
   GtkBoxChild *child;
   GList *children;

   gtk_container_show_all(widget,hWndOnly);

   for (children=GTK_BOX(widget)->children;children;
        children=g_list_next(children)) {
     child=(GtkBoxChild *)(children->data);
     if (child && child->widget)
        gtk_widget_show_all_full(child->widget,hWndOnly);
   }
}

void gtk_box_hide_all(GtkWidget *widget,gboolean hWndOnly) {
   GtkBoxChild *child;
   GList *children;

   gtk_container_hide_all(widget,hWndOnly);

   for (children=GTK_BOX(widget)->children;children;
        children=g_list_next(children)) {
     child=(GtkBoxChild *)(children->data);
     if (child && child->widget)
        gtk_widget_hide_all_full(child->widget,hWndOnly);
   }
}

void gtk_table_show_all(GtkWidget *widget,gboolean hWndOnly) {
   GList *children;
   GtkTableChild *child;

   gtk_container_show_all(widget,hWndOnly);
   for (children=GTK_TABLE(widget)->children;children;
        children=g_list_next(children)) {
      child=(GtkTableChild *)(children->data);
      if (child && child->widget)
         gtk_widget_show_all_full(child->widget,hWndOnly);
   }
}

void gtk_table_hide_all(GtkWidget *widget,gboolean hWndOnly) {
   GList *children;
   GtkTableChild *child;

   gtk_container_hide_all(widget,hWndOnly);
   for (children=GTK_TABLE(widget)->children;children;
        children=g_list_next(children)) {
      child=(GtkTableChild *)(children->data);
      if (child && child->widget)
         gtk_widget_hide_all_full(child->widget,hWndOnly);
   }
}

void gtk_widget_hide_all(GtkWidget *widget) {
   gtk_widget_hide_all_full(widget,FALSE);
}

void gtk_widget_hide_all_full(GtkWidget *widget,gboolean hWndOnly) {
   gtk_signal_emit(GTK_OBJECT(widget),"hide_all",hWndOnly);
   if (hWndOnly) {
      gtk_signal_emit(GTK_OBJECT(widget),"hide");
      if (widget->hWnd) ShowWindow(widget->hWnd,SW_HIDE);
   } else gtk_widget_hide_full(widget,FALSE);
}

void gtk_widget_show_all(GtkWidget *widget) {
   gtk_widget_show_all_full(widget,FALSE);
}

void gtk_widget_show_all_full(GtkWidget *widget,gboolean hWndOnly) {
   GtkAllocation alloc;
   gboolean InitWindow=FALSE;

   if (!GTK_WIDGET_REALIZED(widget) &&
       GTK_OBJECT(widget)->klass==&GtkWindowClass) InitWindow=TRUE;

   if (InitWindow) gtk_widget_realize(widget);
  
   gtk_signal_emit(GTK_OBJECT(widget),"show_all",hWndOnly);

   if (hWndOnly) {
      if (GTK_WIDGET_VISIBLE(widget)) {
         gtk_signal_emit(GTK_OBJECT(widget),"show");
         if (widget->hWnd) ShowWindow(widget->hWnd,SW_SHOWNORMAL);
      }
   } else gtk_widget_show_full(widget,FALSE);

   if (InitWindow) {
      gtk_widget_update(widget,TRUE);
      alloc.x=alloc.y=0;
      alloc.width=widget->requisition.width;
      alloc.height=widget->requisition.height;
      gtk_window_set_initial_position(GTK_WINDOW(widget),&alloc);
//    gtk_widget_set_size(widget,&alloc);
      ShowWindow(widget->hWnd,SW_SHOWNORMAL);
      UpdateWindow(widget->hWnd);
   }

}

GtkWidget *gtk_widget_get_ancestor(GtkWidget *widget,GtkType type) {
   if (!widget) return NULL;
   while (widget && GTK_OBJECT(widget)->klass!=type) {
      widget=widget->parent;
   }
   return widget;
}

void gtk_label_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   SIZE size;
   GtkLabel *label=GTK_LABEL(widget);

   if (GetTextSize(widget->hWnd,label->text,&size)) {
      requisition->width = size.cx;
      requisition->height = size.cy;
   }
/* g_print("Label requesting size %d by %d\n",requisition->width,
           requisition->height);*/
}

void gtk_label_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   gint yexcess;
   yexcess=allocation->height-widget->requisition.height;
   if (yexcess > 0) {
      allocation->y += yexcess/2;
      allocation->height -= yexcess;
   }
}

void gtk_entry_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   gint yexcess;
   yexcess=allocation->height-widget->requisition.height;
   if (yexcess > 0) {
      allocation->y += yexcess/2;
      allocation->height -= yexcess;
   }
}

void gtk_label_destroy(GtkWidget *widget) {
   g_free(GTK_LABEL(widget)->text);
}

void gtk_label_realize(GtkWidget *widget) {
   GtkLabel *label=GTK_LABEL(widget);
   HWND Parent;
   Parent=gtk_get_parent_hwnd(widget);
   widget->hWnd = CreateWindow("STATIC",label->text,
                            WS_CHILD|SS_CENTER,
                            widget->allocation.x,widget->allocation.y,
                            widget->allocation.width,widget->allocation.height,
                            Parent,NULL,hInst,NULL);
   gtk_set_default_font(widget->hWnd);
}

void gtk_container_add(GtkContainer *container,GtkWidget *widget) {
   container->child=widget;
   widget->parent=GTK_WIDGET(container);
   if (GTK_WIDGET_REALIZED(GTK_WIDGET(container))) {
      gtk_widget_realize(widget);
      gtk_widget_update(GTK_WIDGET(container),TRUE);
   }
}

void gtk_container_set_border_width(GtkContainer *container,
                                    guint border_width) {
   container->border_width = border_width;
}

GtkWidget *gtk_table_new(guint rows,guint cols,gboolean homogeneous) {
   GtkTable *table;

   table=GTK_TABLE(GtkNewObject(&GtkTableClass));

   table->nrows=rows;
   table->ncols=cols;
   table->homogeneous=homogeneous;

   table->rows = g_new0(GtkTableRowCol,rows);
   table->cols = g_new0(GtkTableRowCol,cols);

   return GTK_WIDGET(table);
}

void gtk_table_attach_defaults(GtkTable *table,GtkWidget *widget,
                               guint left_attach,guint right_attach,
                               guint top_attach,guint bottom_attach) {
   gtk_table_attach(table,widget,left_attach,right_attach,
                    top_attach,bottom_attach,GTK_EXPAND,GTK_EXPAND,0,0);
}

void gtk_table_attach(GtkTable *table,GtkWidget *widget,
                      guint left_attach,guint right_attach,
                      guint top_attach,guint bottom_attach,
                      GtkAttachOptions xoptions,GtkAttachOptions yoptions,
                      guint xpadding,guint ypadding) {
   GtkTableChild *newChild;

   newChild=g_new0(GtkTableChild,1);
   newChild->widget=widget;
   newChild->left_attach=left_attach;
   newChild->right_attach=right_attach;
   newChild->top_attach=top_attach;
   newChild->bottom_attach=bottom_attach;

   table->children=g_list_append(table->children,(gpointer)newChild);
   widget->parent = GTK_WIDGET(table);
   if (GTK_WIDGET_REALIZED(GTK_WIDGET(table))) {
      gtk_widget_realize(widget);
      gtk_widget_update(GTK_WIDGET(table),TRUE);
   }
}

void gtk_table_destroy(GtkWidget *widget) {
   GList *children;
   GtkTableChild *child;

   gtk_container_destroy(widget);
   for (children=GTK_TABLE(widget)->children;children;
        children=g_list_next(children)) {
      child=(GtkTableChild *)(children->data);
      if (child->widget) gtk_widget_destroy(child->widget);
      g_free(child);
   }
   g_list_free(GTK_TABLE(widget)->children);
}

void gtk_table_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   GList *children;
   GtkTableChild *child;
   GtkWidget *child_wid;
   GtkRequisition child_req;
   GtkTable *table;
   gint16 MaxReq;
   int i;

   table=GTK_TABLE(widget);
   for (i=0;i<table->ncols;i++) table->cols[i].requisition=0;
   for (i=0;i<table->nrows;i++) table->rows[i].requisition=0;

   gtk_container_size_request(widget,requisition);
   for (children=table->children;children;
        children=g_list_next(children)) {
      child=(GtkTableChild *)(children->data);
      if (!child) continue;
      child_wid=child->widget;
      if (child_wid && child->left_attach<child->right_attach &&
          child->top_attach<child->bottom_attach &&
          GTK_WIDGET_VISIBLE(child_wid)) {
         child_req.width=child_wid->requisition.width;
         child_req.height=child_wid->requisition.height;
         child_req.width /=  (child->right_attach-child->left_attach);
         child_req.height /= (child->bottom_attach-child->top_attach);
         for (i=child->left_attach;i<child->right_attach;i++) {
            if (child_req.width > table->cols[i].requisition)
               table->cols[i].requisition = child_req.width;
         }
         for (i=child->top_attach;i<child->bottom_attach;i++) {
            if (child_req.height > table->rows[i].requisition)
               table->rows[i].requisition = child_req.height;
         }
      }
   }

   if (table->homogeneous) {
      MaxReq=0;
      for (i=0;i<table->ncols;i++) if (table->cols[i].requisition>MaxReq) {
         MaxReq=table->cols[i].requisition;
      }
      for (i=0;i<table->ncols;i++) table->cols[i].requisition=MaxReq;

      MaxReq=0;
      for (i=0;i<table->nrows;i++) if (table->rows[i].requisition>MaxReq) {
         MaxReq=table->rows[i].requisition;
      }
      for (i=0;i<table->nrows;i++) table->rows[i].requisition=MaxReq;
   }

   requisition->width=requisition->height=2*GTK_CONTAINER(widget)->border_width;

   for (i=0;i<table->ncols;i++) requisition->width+=table->cols[i].requisition;
   for (i=0;i<table->ncols-1;i++) requisition->width+=table->cols[i].spacing;
   for (i=0;i<table->nrows;i++) requisition->height+=table->rows[i].requisition;
   for (i=0;i<table->nrows-1;i++) requisition->height+=table->rows[i].spacing;

}

void gtk_table_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   GtkTable *table;
   gint row_extra=0,col_extra=0,i;
   GtkAllocation child_alloc;
   GList *children;
   GtkTableChild *child;
   gint border_width;

   table=GTK_TABLE(widget);
   border_width=GTK_CONTAINER(widget)->border_width;

   if (table->ncols) {
      col_extra = (allocation->width-widget->requisition.width)/table->ncols;
   }
   if (table->nrows) {
      row_extra = (allocation->height-widget->requisition.height)/table->nrows;
   }
   for (i=0;i<table->ncols;i++) {
      table->cols[i].allocation = table->cols[i].requisition+col_extra;
   }
   for (i=0;i<table->nrows;i++) {
      table->rows[i].allocation = table->rows[i].requisition+row_extra;
   }
   for (children=table->children;children;children=g_list_next(children)) {
      child=(GtkTableChild *)(children->data);
      if (!child || !child->widget ||
          !GTK_WIDGET_VISIBLE(child->widget)) continue;
      child_alloc.x = allocation->x+border_width;
      child_alloc.y = allocation->y+border_width;
      child_alloc.width = child_alloc.height = 0;
      for (i=0;i<child->left_attach;i++) {
         child_alloc.x+=table->cols[i].allocation+table->cols[i].spacing;
      }
      for (i=0;i<child->top_attach;i++) {
         child_alloc.y+=table->rows[i].allocation+table->rows[i].spacing;
      }
      for (i=child->left_attach;i<child->right_attach;i++) {
         child_alloc.width+=table->cols[i].allocation;
      }
      for (i=child->top_attach;i<child->bottom_attach;i++) {
         child_alloc.height+=table->rows[i].allocation;
      }
      gtk_widget_set_size(child->widget,&child_alloc);
   }
}

void gtk_table_realize(GtkWidget *widget) {
   GList *children;
   GtkTableChild *child;

   gtk_container_realize(widget);
   for (children=GTK_TABLE(widget)->children;children;
        children=g_list_next(children)) {
      child=(GtkTableChild *)(children->data);
      if (child->widget) gtk_widget_realize(child->widget);
   }
}

void gtk_table_set_row_spacing(GtkTable *table,guint row,guint spacing) {
   if (table && row>=0 && row<table->nrows) {
      table->rows[row].spacing=spacing;
   }
}

void gtk_table_set_col_spacing(GtkTable *table,guint column,guint spacing) {
   if (table && column>=0 && column<table->ncols) {
      table->cols[column].spacing=spacing;
   }
}

void gtk_table_set_row_spacings(GtkTable *table,guint spacing) {
   int i;
   table->row_spacing=spacing;
   for (i=0;i<table->nrows;i++) table->rows[i].spacing=spacing;
}

void gtk_table_set_col_spacings(GtkTable *table,guint spacing) {
   int i;
   table->column_spacing=spacing;
   for (i=0;i<table->ncols;i++) table->cols[i].spacing=spacing;
}

void gtk_toggle_button_toggled(GtkToggleButton *toggle_button) {
   toggle_button->toggled = !toggle_button->toggled;
   gtk_signal_emit(GTK_OBJECT(toggle_button),"toggled");
}

void gtk_check_button_toggled(GtkCheckButton *check_button,gpointer data) {
   HWND hWnd;
   gboolean is_active = GTK_TOGGLE_BUTTON(check_button)->toggled;
   hWnd=GTK_WIDGET(check_button)->hWnd;
   if (hWnd) {
      SendMessage(hWnd,BM_SETCHECK,is_active ? BST_CHECKED : BST_UNCHECKED,0);
   }
}

void gtk_radio_button_clicked(GtkRadioButton *radio_button,gpointer data) {
   GtkToggleButton *toggle=GTK_TOGGLE_BUTTON(radio_button);

   if (toggle->toggled) return;
   else gtk_toggle_button_toggled(toggle);
}

void gtk_radio_button_toggled(GtkRadioButton *radio_button,gpointer data) {
   HWND hWnd;
   GSList *group;
   GtkRadioButton *radio;
   gboolean is_active = GTK_TOGGLE_BUTTON(radio_button)->toggled;
   hWnd=GTK_WIDGET(radio_button)->hWnd;
   if (hWnd) {
      SendMessage(hWnd,BM_SETCHECK,is_active ? BST_CHECKED : BST_UNCHECKED,0);
   }
   if (is_active) {
      for (group=radio_button->group;group;group=g_slist_next(group)) {
         radio=GTK_RADIO_BUTTON(group->data);
         if (radio && radio!=radio_button) {
            GTK_TOGGLE_BUTTON(radio)->toggled=FALSE;
            hWnd=GTK_WIDGET(radio)->hWnd;
            if (hWnd) SendMessage(hWnd,BM_SETCHECK,BST_UNCHECKED,0);
         }
      }
   }
}

gboolean gtk_toggle_button_get_active(GtkToggleButton *toggle_button) {
   return (toggle_button->toggled);
}

void gtk_toggle_button_set_active(GtkToggleButton *toggle_button,
                                  gboolean is_active) {
   if (toggle_button->toggled == is_active) return;
   else gtk_toggle_button_toggled(toggle_button);
}

void gtk_main_quit() {
   PostQuitMessage(0);
}

void gtk_main() {
   MSG msg;
   GSList *list;
   BOOL MsgDone;
   GtkWidget *widget;
   HACCEL hAccel;

   while (GetMessage(&msg,NULL,0,0)) {
      MsgDone=FALSE;
      for (list=WindowList;list && !MsgDone;list=g_slist_next(list)) {
         widget=GTK_WIDGET(list->data);
         if (widget && widget->hWnd &&
             (MsgDone=IsDialogMessage(widget->hWnd,&msg))==TRUE) break;
      }
      widget=GTK_WIDGET(GetWindowLong(msg.hwnd,GWL_USERDATA));
      if (!MsgDone && widget && GTK_OBJECT(widget)->klass==&GtkWindowClass) {
         hAccel=GTK_WINDOW(widget)->hAccel;
         if (hAccel) MsgDone=TranslateAccelerator(widget->hWnd,hAccel,&msg);
      }
      if (!MsgDone) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }
}

typedef struct _GtkSignal GtkSignal;

struct _GtkSignal {
   GtkSignalFunc func;
   GtkObject *slot_object;
   gpointer func_data;
};

typedef gint (*GtkGIntSignalFunc) ();

void gtk_marshal_BOOL__GINT(GtkObject *object,GSList *actions,
                            GtkSignalFunc default_action,
                            va_list args) {
   gboolean *retval;
   gint arg1;
   GtkSignal *signal;
   GtkGIntSignalFunc sigfunc;

   arg1=va_arg(args,gint);
   retval=va_arg(args,gboolean *);
   if (!retval) { g_warning("gtk_marshal_BOOL__GINT: retval NULL"); return; }

   while (actions) {
      signal=(GtkSignal*)actions->data;
      sigfunc = (GtkGIntSignalFunc)signal->func;
      if (signal->slot_object) {
         *retval = (*sigfunc)(signal->slot_object,arg1);
      } else *retval = (*sigfunc)(object,arg1,signal->func_data);
      if (*retval) return;
      actions=g_slist_next(actions);
   }
   sigfunc=(GtkGIntSignalFunc)default_action;
   if (sigfunc) *retval = (*sigfunc)(object,arg1);
}

void gtk_marshal_BOOL__GPOIN(GtkObject *object,GSList *actions,
                             GtkSignalFunc default_action,
                             va_list args) {
   gboolean *retval;
   gpointer arg1;
   GtkSignal *signal;
   GtkGIntSignalFunc sigfunc;

   arg1=va_arg(args,gpointer);
   retval=va_arg(args,gboolean *);
   if (!retval) { g_warning("gtk_marshal_BOOL__GPOIN: retval NULL"); return; }

   while (actions) {
      signal=(GtkSignal*)actions->data;
      sigfunc = (GtkGIntSignalFunc)signal->func;
      if (signal->slot_object) {
         *retval = (*sigfunc)(signal->slot_object,arg1);
      } else *retval = (*sigfunc)(object,arg1,signal->func_data);
      if (*retval) return;
      actions=g_slist_next(actions);
   }
   sigfunc=(GtkGIntSignalFunc)default_action;
   if (sigfunc) *retval = (*sigfunc)(object,arg1);
}

void gtk_marshal_VOID__VOID(GtkObject *object,GSList *actions,
                            GtkSignalFunc default_action,
                            va_list args) {
   GtkSignal *signal;

   while (actions) {
//    g_print("processing action...\n");
      signal=(GtkSignal*)actions->data;
      if (signal->slot_object) {
         (*signal->func)(signal->slot_object);
      } else (*signal->func)(object,signal->func_data);
      actions=g_slist_next(actions);
   }
   if (default_action) (*default_action)(object);
}

void gtk_marshal_VOID__GPOIN(GtkObject *object,GSList *actions,
                             GtkSignalFunc default_action,
                             va_list args) {
   gpointer arg1;
   GtkSignal *signal;

   arg1=va_arg(args,gpointer);

   while (actions) {
      signal=(GtkSignal*)actions->data;
      if (signal->slot_object) {
         (*signal->func)(signal->slot_object,arg1);
      } else (*signal->func)(object,arg1,signal->func_data);
      actions=g_slist_next(actions);
   }
   if (default_action) (*default_action)(object,arg1);
}

void gtk_marshal_VOID__BOOL(GtkObject *object,GSList *actions,
                             GtkSignalFunc default_action,
                             va_list args) {
   gboolean arg1;
   GtkSignal *signal;

   arg1=va_arg(args,gboolean);

   while (actions) {
      signal=(GtkSignal*)actions->data;
      if (signal->slot_object) {
         (*signal->func)(signal->slot_object,arg1);
      } else (*signal->func)(object,arg1,signal->func_data);
      actions=g_slist_next(actions);
   }
   if (default_action) (*default_action)(object,arg1);
}

static GtkSignalType *gtk_get_signal_type(GtkObject *object,
                                          const gchar *name) {
   GtkClass *klass;
   GtkSignalType *signals;

   for (klass=object->klass;klass;klass=klass->parent) {
/*    g_print("Searching for signal %s in class %s\n",name,klass->Name);*/
      for (signals=klass->signals;signals && signals->name[0];signals++) {
         if (strcmp(signals->name,name)==0) return signals;
      }
   }
   return NULL;
}

void gtk_signal_emit(GtkObject *object,const gchar *name, ...) {
   GSList *signal_list;
   GtkSignalType *signal_type;
   va_list ap;

/* g_print("gtk_signal_emit of signal %s on object %p, type %p\n",
           name,object,object->klass);*/

   if (!object) return;

// g_print("Initialising va_list\n");
   va_start(ap,name);
// g_print("Getting signal list\n");
   signal_list=(GSList *)g_datalist_get_data(&object->signals,name);
// g_print("Getting signal list %p\n",signal_list);
   signal_type=gtk_get_signal_type(object,name);
// g_print("Getting signal type %p\n",signal_type);
   if (signal_type && signal_type->marshaller) {
/*    g_print("Calling marshaller %p with signal_list %p\n",
              signal_type->marshaller,signal_list);*/
      (*signal_type->marshaller)(object,signal_list,
                                 signal_type->default_action,ap);
   }
   va_end(ap);
   if (!signal_type) g_warning("gtk_signal_emit: unknown signal %s",name);
// g_print("Signal emission done\n");
}

guint gtk_signal_connect(GtkObject *object,const gchar *name,
                         GtkSignalFunc func,gpointer func_data) {
   GtkSignal *signal;
   GtkSignalType *signal_type;
   GSList *signal_list;

// g_print("Attempting to connect signal %s to object %p\n",name,object);
   if (!object) return 0;
   signal_type=gtk_get_signal_type(object,name);
   if (!signal_type) {
      g_warning("gtk_signal_connect: unknown signal %s",name);
      return 0;
   }
   signal_list=(GSList *)g_datalist_get_data(&object->signals,name);
   signal=g_new0(GtkSignal,1);
   signal->func=func; signal->func_data=func_data;
   signal_list=g_slist_append(signal_list,signal);
   g_datalist_set_data(&object->signals,name,signal_list);
   return 0;
}

guint gtk_signal_connect_object(GtkObject *object,const gchar *name,
                                GtkSignalFunc func,GtkObject *slot_object) {
   GtkSignal *signal;
   GtkSignalType *signal_type;
   GSList *signal_list;

   if (!object) return 0;
   signal_type=gtk_get_signal_type(object,name);
   if (!signal_type) {
      g_warning("gtk_signal_connect_object: unknown signal %s",name);
      return 0;
   }
   signal_list=(GSList *)g_datalist_get_data(&object->signals,name);
   signal=g_new0(GtkSignal,1);
   signal->func=func; signal->slot_object=slot_object;
   signal_list=g_slist_append(signal_list,signal);
   g_datalist_set_data(&object->signals,name,signal_list);
   return 0;
}

GtkItemFactory *gtk_item_factory_new(GtkType container_type,
                                     const gchar *path,
                                     GtkAccelGroup *accel_group) {
   GtkItemFactory *new_fac;
   new_fac=(GtkItemFactory *)GtkNewObject(&GtkItemFactoryClass);
   new_fac->path=g_strdup(path);
   new_fac->top_widget=gtk_menu_bar_new();
   return new_fac;
}

static gint PathCmp(const gchar *path1,const gchar *path2) {
   gint Match=1;

   if (!path1 || !path2) return 0;

   while (*path1 && *path2 && Match) {
      while (*path1=='_') path1++;
      while (*path2=='_') path2++;
      if (*path1==*path2) { path1++; path2++; }
      else Match=0;
   }
   if (*path1 || *path2) Match=0;
   return Match;
}

static void gtk_item_factory_parse_path(GtkItemFactory *ifactory,
                                        gchar *path,
                                        GtkItemFactoryChild **parent,
                                        gchar **menu_title) {
   GSList *list;
   GtkItemFactoryChild *child;
   gchar *root,*pt,*title;

   pt=strrchr(path,'/');
   if (!pt) return;
   title=g_strdup(pt+1);
   root=g_strdup(path);
   root[pt-path]='\0';


   for (list=ifactory->children;list;list=g_slist_next(list)) {
      child=(GtkItemFactoryChild *)list->data;
      if (PathCmp(child->path,root)==1) { *parent=child; break; }
   }
   *menu_title=title;
   g_free(root);
}

void gtk_item_factory_create_item(GtkItemFactory *ifactory,
                                  GtkItemFactoryEntry *entry,
                                  gpointer callback_data,guint callback_type) {
   GtkItemFactoryChild *new_child,*parent=NULL;
   gchar *menu_title=NULL;
   GtkWidget *menu_item,*menu;

   new_child=g_new0(GtkItemFactoryChild,1);
   new_child->path=g_strdup(entry->path);

   gtk_item_factory_parse_path(ifactory,new_child->path,&parent,&menu_title);
   menu_item=gtk_menu_item_new_with_label(menu_title);
   new_child->widget=menu_item;
   if (entry->callback) {
      gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
                         entry->callback,callback_data);
   }

   if (parent) {
      menu=GTK_WIDGET(GTK_MENU_ITEM(parent->widget)->submenu);
      if (!menu) {
         menu=gtk_menu_new();
         gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent->widget),menu);
      } 
      gtk_menu_append(GTK_MENU(menu),menu_item);
   } else {
      gtk_menu_bar_append(GTK_MENU_BAR(ifactory->top_widget),menu_item);
   }

   g_free(menu_title);

   ifactory->children=g_slist_append(ifactory->children,new_child);
}

void gtk_item_factory_create_items(GtkItemFactory *ifactory,guint n_entries,
                                   GtkItemFactoryEntry *entries,
                                   gpointer callback_data) {
   gint i;
   for (i=0;i<n_entries;i++) {
      gtk_item_factory_create_item(ifactory,&entries[i],callback_data,0);
   }
}

GtkWidget *gtk_item_factory_get_widget(GtkItemFactory *ifactory,
                                       const gchar *path) {
   gint root_len;
   GSList *list;
   GtkItemFactoryChild *child;

   root_len=strlen(ifactory->path);
   if (!path || strlen(path)<root_len) return NULL;

   if (strncmp(ifactory->path,path,root_len)!=0) return NULL;
   if (strlen(path)==root_len) return ifactory->top_widget;

   for (list=ifactory->children;list;list=g_slist_next(list)) {
      child=(GtkItemFactoryChild *)list->data;
      if (PathCmp(child->path,&path[root_len])==1) return child->widget;
   }
   return NULL;
}

void gtk_menu_shell_insert(GtkMenuShell *menu_shell,GtkWidget *child,
                           gint position) {
   menu_shell->children=g_slist_insert(menu_shell->children,(gpointer)child,
                                       position);
   child->parent=GTK_WIDGET(menu_shell);
}

void gtk_menu_shell_append(GtkMenuShell *menu_shell,GtkWidget *child) {
   gtk_menu_shell_insert(menu_shell,child,-1);
}

void gtk_menu_shell_prepend(GtkMenuShell *menu_shell,GtkWidget *child) {
   gtk_menu_shell_insert(menu_shell,child,0);
}

GtkWidget *gtk_menu_bar_new() {
   GtkMenuBar *menu_bar;
   menu_bar=GTK_MENU_BAR(GtkNewObject(&GtkMenuBarClass));
   return GTK_WIDGET(menu_bar);
}

void gtk_menu_bar_insert(GtkMenuBar *menu_bar,GtkWidget *child,gint position) {
   gtk_menu_shell_insert(GTK_MENU_SHELL(menu_bar),child,position);
}

void gtk_menu_bar_append(GtkMenuBar *menu_bar,GtkWidget *child) {
   gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar),child);
}

void gtk_menu_bar_prepend(GtkMenuBar *menu_bar,GtkWidget *child) {
   gtk_menu_shell_prepend(GTK_MENU_SHELL(menu_bar),child);
}

GtkWidget *gtk_menu_new() {
   GtkMenu *menu;
   menu=GTK_MENU(GtkNewObject(&GtkMenuClass));
   return GTK_WIDGET(menu);
}

void gtk_menu_insert(GtkMenu *menu,GtkWidget *child,gint position) {
   gtk_menu_shell_insert(GTK_MENU_SHELL(menu),child,position);
}

void gtk_menu_append(GtkMenu *menu,GtkWidget *child) {
   gtk_menu_shell_append(GTK_MENU_SHELL(menu),child);
}

void gtk_menu_prepend(GtkMenu *menu,GtkWidget *child) {
   gtk_menu_shell_prepend(GTK_MENU_SHELL(menu),child);
}

GtkWidget *gtk_menu_item_new_with_label(const gchar *label) {
   GtkMenuItem *menu_item;
   gint i;

   menu_item=GTK_MENU_ITEM(GtkNewObject(&GtkMenuItemClass));
   menu_item->text=g_strdup(label);
   for (i=0;i<strlen(menu_item->text);i++) {
      if (menu_item->text[i]=='_') menu_item->text[i]='&';
   }
   return GTK_WIDGET(menu_item);
}

void gtk_menu_item_set_submenu(GtkMenuItem *menu_item,GtkWidget *submenu) {
   menu_item->submenu=GTK_MENU(submenu);
   submenu->parent=GTK_WIDGET(menu_item);
}

static GtkWidget *gtk_menu_item_get_menu_ID(GtkMenuItem *menu_item,gint ID) {
   if (menu_item->ID==ID) {
      return GTK_WIDGET(menu_item);
   } else if (menu_item->submenu) {
      return gtk_menu_shell_get_menu_ID(GTK_MENU_SHELL(menu_item->submenu),ID);
   } else return NULL;
}

GtkWidget *gtk_menu_shell_get_menu_ID(GtkMenuShell *menu_shell,gint ID) {
   GSList *list;
   GtkWidget *menu_item;
   for (list=menu_shell->children;list;list=list->next) {
      menu_item=gtk_menu_item_get_menu_ID(GTK_MENU_ITEM(list->data),ID);
      if (menu_item) return menu_item;
   }
   return NULL;
}

GtkWidget *gtk_window_get_menu_ID(GtkWindow *window,gint ID) {
   if (window->menu_bar) {
      return gtk_menu_shell_get_menu_ID(GTK_MENU_SHELL(window->menu_bar),ID);
   } else return NULL;
}

void gtk_menu_bar_realize(GtkWidget *widget) {
   GtkMenuBar *menu_bar=GTK_MENU_BAR(widget);
   GtkWidget *window;
   HMENU hMenu;

   hMenu=GTK_MENU_SHELL(widget)->menu=CreateMenu();
   menu_bar->LastID=1000;

   gtk_menu_shell_realize(widget);

   window=gtk_widget_get_ancestor(widget,GTK_TYPE_WINDOW);
   gtk_window_set_menu(GTK_WINDOW(window),menu_bar);
}

void gtk_menu_item_realize(GtkWidget *widget) {
   GtkMenuItem *menu_item=GTK_MENU_ITEM(widget);
   MENUITEMINFO mii;
   GtkWidget *menu_bar;
   HMENU parent_menu;
   gint pos;

   menu_bar=gtk_widget_get_ancestor(widget,GTK_TYPE_MENU_BAR);
   if (menu_bar) menu_item->ID=GTK_MENU_BAR(menu_bar)->LastID++;

   if (menu_item->submenu) gtk_widget_realize(GTK_WIDGET(menu_item->submenu));

   parent_menu=GTK_MENU_SHELL(widget->parent)->menu;
   pos=g_slist_index(GTK_MENU_SHELL(widget->parent)->children,widget);

   mii.cbSize=sizeof(MENUITEMINFO);
   mii.fMask=MIIM_TYPE|MIIM_ID|MIIM_STATE;
   if (menu_item->submenu) {
      mii.fMask|=MIIM_SUBMENU;
      mii.hSubMenu=GTK_MENU_SHELL(menu_item->submenu)->menu;
   }
   mii.fType=MFT_STRING;
   if (GTK_WIDGET_SENSITIVE(widget)) mii.fState=MFS_ENABLED;
   else mii.fState=MFS_GRAYED;
   mii.wID=menu_item->ID;
   mii.dwTypeData=(LPTSTR)menu_item->text;
   mii.cch=strlen(menu_item->text);
   InsertMenuItem(parent_menu,pos,TRUE,&mii);
}

void gtk_menu_realize(GtkWidget *widget) {
   GTK_MENU_SHELL(widget)->menu=CreatePopupMenu();
   gtk_menu_shell_realize(widget);
}

void gtk_menu_set_active(GtkMenu *menu,guint index) {
   menu->active=index;
}

void gtk_menu_shell_realize(GtkWidget *widget) {
   GSList *children;
   GtkMenuShell *menu=GTK_MENU_SHELL(widget);
   for (children=menu->children;children;children=g_slist_next(children)) {
      gtk_widget_realize(GTK_WIDGET(children->data));
   }
}

void gtk_menu_item_enable(GtkWidget *widget) {
   GtkWidget *parent;
   HMENU hMenu;
   HWND hWnd;
   parent=widget->parent;
   if (!parent) return;
   hMenu=GTK_MENU_SHELL(parent)->menu;
   if (hMenu) EnableMenuItem(hMenu,GTK_MENU_ITEM(widget)->ID,
                             MF_BYCOMMAND|MF_ENABLED);
   hWnd=gtk_get_parent_hwnd(widget);
   if (hWnd) DrawMenuBar(hWnd);
}

void gtk_menu_item_disable(GtkWidget *widget) {
   GtkWidget *parent;
   HMENU hMenu;
   HWND hWnd;
   parent=widget->parent;
   if (!parent) return;
   hMenu=GTK_MENU_SHELL(parent)->menu;
   if (hMenu) EnableMenuItem(hMenu,GTK_MENU_ITEM(widget)->ID,
                             MF_BYCOMMAND|MF_GRAYED);
   hWnd=gtk_get_parent_hwnd(widget);
   if (hWnd) DrawMenuBar(hWnd);
}

GtkWidget *gtk_notebook_new() {
   GtkNotebook *notebook;

   notebook=GTK_NOTEBOOK(GtkNewObject(&GtkNotebookClass));
   return GTK_WIDGET(notebook);
}

void gtk_notebook_append_page(GtkNotebook *notebook,GtkWidget *child,
                              GtkWidget *tab_label) {
   gtk_notebook_insert_page(notebook,child,tab_label,-1);
}

void gtk_notebook_insert_page(GtkNotebook *notebook,GtkWidget *child,
                              GtkWidget *tab_label,gint position) {
   GtkNotebookChild *note_child;
   note_child=g_new0(GtkNotebookChild,1);
   note_child->child=child;
   note_child->tab_label=tab_label;
   notebook->children=g_slist_insert(notebook->children,note_child,position);
   child->parent=GTK_WIDGET(notebook);
}

void gtk_notebook_set_page(GtkNotebook *notebook,gint page_num) {
   GSList *children;
   GtkNotebookChild *note_child;
   GtkWidget *widget=GTK_WIDGET(notebook);
   gint pos=0;

   if (page_num<0) page_num=g_slist_length(notebook->children)-1;
   notebook->selection=page_num;

   if (GTK_WIDGET_REALIZED(widget)) {
      if (widget->hWnd) TabCtrl_SetCurSel(widget->hWnd,page_num);
      for (children=notebook->children;children;
           children=g_slist_next(children)) {
         note_child=(GtkNotebookChild *)(children->data);
         if (note_child && note_child->child) {
            if (pos==page_num) gtk_widget_show_all_full(note_child->child,TRUE);
            else gtk_widget_hide_all_full(note_child->child,TRUE);
            pos++;
         }
      }
   }
}

void gtk_notebook_realize(GtkWidget *widget) {
   GSList *children;
   GtkNotebookChild *note_child;
   HWND Parent;
   gint tab_pos=0;
   TC_ITEM tie;

   Parent=gtk_get_parent_hwnd(widget);
   GTK_WIDGET_SET_FLAGS(widget,GTK_CAN_FOCUS);
   widget->hWnd = CreateWindow(WC_TABCONTROL,"",
                               WS_CHILD|WS_TABSTOP,0,0,0,0,
                               Parent,NULL,hInst,NULL);
   if (widget->hWnd==NULL) g_print("Error creating window!\n");
   gtk_set_default_font(widget->hWnd);

   tie.mask = TCIF_TEXT | TCIF_IMAGE;
   tie.iImage = -1;

   for (children=GTK_NOTEBOOK(widget)->children;children;
        children=g_slist_next(children)) {
      note_child=(GtkNotebookChild *)(children->data);
      if (note_child) {
         if (note_child->tab_label)
            tie.pszText=GTK_LABEL(note_child->tab_label)->text;
         else tie.pszText="No label";
         TabCtrl_InsertItem(widget->hWnd,tab_pos++,&tie);
         if (note_child->child) {
            gtk_widget_realize(note_child->child);
         }
      }
   }
   gtk_notebook_set_page(GTK_NOTEBOOK(widget),GTK_NOTEBOOK(widget)->selection);
}

void gtk_notebook_show_all(GtkWidget *widget,gboolean hWndOnly) {
   GSList *children;
   GtkNotebookChild *note_child;

   if (!hWndOnly) for (children=GTK_NOTEBOOK(widget)->children;children;
        children=g_slist_next(children)) {
      note_child=(GtkNotebookChild *)(children->data);
      if (note_child && note_child->child) 
         gtk_widget_show_all_full(note_child->child,hWndOnly);
   }
   gtk_notebook_set_page(GTK_NOTEBOOK(widget),GTK_NOTEBOOK(widget)->selection);
}

void gtk_notebook_hide_all(GtkWidget *widget,gboolean hWndOnly) {
   GSList *children;
   GtkNotebookChild *note_child;

   for (children=GTK_NOTEBOOK(widget)->children;children;
        children=g_slist_next(children)) {
      note_child=(GtkNotebookChild *)(children->data);
      if (note_child && note_child->child) 
         gtk_widget_hide_all_full(note_child->child,hWndOnly);
   }
}

void gtk_notebook_destroy(GtkWidget *widget) {
   GSList *children;
   GtkNotebookChild *note_child;

   for (children=GTK_NOTEBOOK(widget)->children;children;
        children=g_slist_next(children)) {
      note_child=(GtkNotebookChild *)(children->data);
      if (note_child) {
         gtk_widget_destroy(note_child->child);
         gtk_widget_destroy(note_child->tab_label);
      }
      g_free(note_child);
   }
   g_slist_free(GTK_NOTEBOOK(widget)->children);
}

void gtk_notebook_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   GSList *children;
   GtkNotebookChild *note_child;
   RECT rect;
   GtkAllocation child_alloc;

   gtk_container_set_size(widget,allocation);
   rect.left=allocation->x;
   rect.top=allocation->y;
   rect.right=allocation->x+allocation->width;
   rect.bottom=allocation->y+allocation->height;
   TabCtrl_AdjustRect(widget->hWnd,FALSE,&rect);
   child_alloc.x=rect.left+GTK_CONTAINER(widget)->border_width;
   child_alloc.y=rect.top+GTK_CONTAINER(widget)->border_width;
   child_alloc.width=rect.right-rect.left
                    -2*GTK_CONTAINER(widget)->border_width;
   child_alloc.height=rect.bottom-rect.top
                     -2*GTK_CONTAINER(widget)->border_width;

   for (children=GTK_NOTEBOOK(widget)->children;children;
        children=g_slist_next(children)) {
      note_child=(GtkNotebookChild *)(children->data);
      if (note_child && note_child->child) {
         gtk_widget_set_size(note_child->child,&child_alloc);
      }
   }
}

void gtk_notebook_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   GSList *children;
   GtkNotebookChild *note_child;
   GtkRequisition *child_req;
   RECT rect;

   requisition->width=requisition->height=0;
   for (children=GTK_NOTEBOOK(widget)->children;children;
        children=g_slist_next(children)) {
      note_child=(GtkNotebookChild *)(children->data);
      if (note_child && note_child->child &&
          GTK_WIDGET_VISIBLE(note_child->child)) {
         child_req=&note_child->child->requisition;
         if (child_req->width > requisition->width)
            requisition->width=child_req->width;
         if (child_req->height > requisition->height)
            requisition->height=child_req->height;
      }
   }
   requisition->width+=GTK_CONTAINER(widget)->border_width*2;
   requisition->height+=GTK_CONTAINER(widget)->border_width*2;
   rect.left=rect.top=0;
   rect.right=requisition->width;
   rect.bottom=requisition->height;
   TabCtrl_AdjustRect(widget->hWnd,TRUE,&rect);
   requisition->width=rect.right-rect.left;
   requisition->height=rect.bottom-rect.top;
}

GtkObject *gtk_adjustment_new(gfloat value,gfloat lower,gfloat upper,
                              gfloat step_increment,gfloat page_increment,
                              gfloat page_size) {
   GtkAdjustment *adj;

   adj=(GtkAdjustment *)(GtkNewObject(&GtkAdjustmentClass));

   adj->value=value;
   adj->lower=lower;
   adj->upper=upper;
   adj->step_increment=step_increment;
   adj->page_increment=page_increment;
   adj->page_size=page_size;

   return GTK_OBJECT(adj);
}

GtkWidget *gtk_spin_button_new(GtkAdjustment *adjustment,gfloat climb_rate,
                               guint digits) {
   GtkSpinButton *spin;

   spin=GTK_SPIN_BUTTON(GtkNewObject(&GtkSpinButtonClass));
   gtk_spin_button_set_adjustment(spin,adjustment);

   return GTK_WIDGET(spin);
}

void gtk_spin_button_size_request(GtkWidget *widget,
                                  GtkRequisition *requisition) {
   gtk_entry_size_request(widget,requisition);
// requisition->width += GetSystemMetrics(SM_CXVSCROLL);
}

void gtk_spin_button_set_size(GtkWidget *widget,
                              GtkAllocation *allocation) {
   int width=allocation->width,udwidth;
   HWND updown;

   udwidth=GetSystemMetrics(SM_CXVSCROLL);
   width=allocation->width;
   allocation->width-=udwidth;

   updown=GTK_SPIN_BUTTON(widget)->updown;
   if (updown) {
      SetWindowPos(updown,HWND_TOP,
                 allocation->x+allocation->width,allocation->y,
                 udwidth,allocation->height,
                 SWP_NOZORDER);
   }
}

gint gtk_spin_button_get_value_as_int(GtkSpinButton *spin_button) {
   HWND hWnd;
   LRESULT lres;
   hWnd=spin_button->updown;
   if (hWnd) {
      lres=SendMessage(hWnd,UDM_GETPOS,0,0);
      if (HIWORD(lres) != 0) return 0;
      else return (gint)LOWORD(lres);
   } else return (gint)spin_button->adj->value;
}

void gtk_spin_button_set_value(GtkSpinButton *spin_button,gfloat value) {
   HWND hWnd;
   spin_button->adj->value=value;
   hWnd=spin_button->updown;
   if (hWnd) SendMessage(hWnd,UDM_SETPOS,0,(LPARAM)MAKELONG((short)value,0));
}

void gtk_spin_button_set_adjustment(GtkSpinButton *spin_button,
                                    GtkAdjustment *adjustment) {
   HWND hWnd;
   spin_button->adj=adjustment;
   hWnd=spin_button->updown;
   if (hWnd) {
      SendMessage(hWnd,UDM_SETRANGE,0,
          (LPARAM)MAKELONG((short)adjustment->upper,(short)adjustment->lower));
      SendMessage(hWnd,UDM_SETPOS,0,
          (LPARAM)MAKELONG((short)adjustment->value,0));
   }
}

void gtk_spin_button_realize(GtkWidget *widget) {
   GtkSpinButton *spin=GTK_SPIN_BUTTON(widget);
   HWND Parent;

   gtk_entry_realize(widget);

   Parent=gtk_get_parent_hwnd(widget->parent);
   GTK_WIDGET_SET_FLAGS(widget,GTK_CAN_FOCUS);
   spin->updown=CreateUpDownControl(WS_CHILD|WS_BORDER|WS_TABSTOP|
                        UDS_SETBUDDYINT|UDS_NOTHOUSANDS|UDS_ARROWKEYS,
                        0,0,0,0,Parent,0,hInst,widget->hWnd,20,10,15);
   gtk_set_default_font(spin->updown);
   gtk_spin_button_set_adjustment(spin,spin->adj);
}

void gtk_spin_button_destroy(GtkWidget *widget) {
   g_free(GTK_SPIN_BUTTON(widget)->adj);
}

void gtk_spin_button_show(GtkWidget *widget) {
   HWND updown;
   updown=GTK_SPIN_BUTTON(widget)->updown;
   if (updown) ShowWindow(updown,SW_SHOWNORMAL);
}

void gtk_spin_button_hide(GtkWidget *widget) {
   HWND updown;
   updown=GTK_SPIN_BUTTON(widget)->updown;
   if (updown) ShowWindow(updown,SW_HIDE);
}

void gdk_input_remove(gint tag) {
   GSList *list;
   GdkInput *input;
   for (list=GdkInputs;list;list=g_slist_next(list)) {
      input=(GdkInput *)list->data;
      if (input->source==tag) {
         WSAAsyncSelect(input->source,TopLevel,0,0);
         GdkInputs=g_slist_remove(GdkInputs,input);
         g_free(input);
         break;
      }
   }
}

gint gdk_input_add(gint source,GdkInputCondition condition,
                   GdkInputFunction function,gpointer data) {
   GdkInput *input;
   int rc;
   input=g_new(GdkInput,1);
   input->source=source;
   input->condition=condition;
   input->function=function;
   input->data=data;
   rc=WSAAsyncSelect(source,TopLevel,WM_SOCKETDATA,
                  (condition&GDK_INPUT_READ ? FD_READ|FD_CLOSE:0) |
                  (condition&GDK_INPUT_WRITE ? FD_WRITE|FD_CONNECT:0));
   GdkInputs=g_slist_append(GdkInputs,input);
   return source;
}

GtkWidget *gtk_hseparator_new() {
   return GTK_WIDGET(GtkNewObject(&GtkHSeparatorClass));
}

GtkWidget *gtk_vseparator_new() {
   return GTK_WIDGET(GtkNewObject(&GtkVSeparatorClass));
}

void gtk_separator_size_request(GtkWidget *widget,
                                GtkRequisition *requisition) {
   requisition->height=requisition->width=2;
}

void gtk_separator_realize(GtkWidget *widget) {
   HWND Parent;
   Parent=gtk_get_parent_hwnd(widget);
   widget->hWnd = CreateWindow(WC_GTKSEP,"",WS_CHILD,
                               0,0,0,0,Parent,NULL,hInst,NULL);
}

void gtk_object_set_data(GtkObject *object,const gchar *key,gpointer data) {
   g_datalist_set_data(&object->object_data,key,data);
}

gpointer gtk_object_get_data(GtkObject *object,const gchar *key) {
   return g_datalist_get_data(&object->object_data,key);
}

GtkAccelGroup *gtk_accel_group_new() {
   GtkAccelGroup *new_accel;
   new_accel=g_new0(GtkAccelGroup,1);
   return new_accel;
}

void gtk_item_factory_set_translate_func(GtkItemFactory *ifactory,
                                         GtkTranslateFunc func,
                                         gpointer data,
                                         GtkDestroyNotify notify) {
}

void gtk_widget_grab_default(GtkWidget *widget) {
}

void gtk_widget_grab_focus(GtkWidget *widget) {
   if (widget->hWnd && GTK_WIDGET_CAN_FOCUS(widget)) {
      SetFocus(widget->hWnd);
   }
}

void gtk_window_set_modal(GtkWindow *window,gboolean modal) {
   window->modal=modal;
}

void gtk_window_add_accel_group(GtkWindow *window,GtkAccelGroup *accel_group) {
   window->accel_group=accel_group;
}

void gtk_entry_set_text(GtkEntry *entry,const gchar *text) {
   int pos=0;
   gtk_editable_insert_text(GTK_EDITABLE(entry),text,strlen(text),&pos);
}

guint SetAccelerator(GtkWidget *labelparent,gchar *Text,
                     GtkWidget *sendto,gchar *signal,
                     GtkAccelGroup *accel_group) {
   gtk_signal_emit(GTK_OBJECT(labelparent),"set_text",Text);
   return 0;
}

void gtk_widget_add_accelerator(GtkWidget *widget,
                                const gchar *accel_signal,
                                GtkAccelGroup *accel_group,
                                guint accel_key,guint accel_mods,
                                GtkAccelFlags accel_flags) {
}

void gtk_widget_remove_accelerator(GtkWidget *widget,
                                   GtkAccelGroup *accel_group,
                                   guint accel_key,guint accel_mods) {
}

GtkWidget *gtk_vpaned_new() {
   GtkVPaned *vpaned;
   vpaned=GTK_VPANED(GtkNewObject(&GtkVPanedClass));
   GTK_PANED(vpaned)->handle_size=5;
   GTK_PANED(vpaned)->handle_pos=PANED_STARTPOS;
   return GTK_WIDGET(vpaned);
}

GtkWidget *gtk_hpaned_new() {
   GtkHPaned *hpaned;
   hpaned=GTK_HPANED(GtkNewObject(&GtkHPanedClass));
   GTK_PANED(hpaned)->handle_size=5;
   GTK_PANED(hpaned)->handle_pos=PANED_STARTPOS;
   return GTK_WIDGET(hpaned);
}

static void gtk_paned_pack(GtkPaned *paned,gint pos,GtkWidget *child,
                           gboolean resize,gboolean shrink) {
   paned->children[pos].widget=child;
   paned->children[pos].resize=resize;
   paned->children[pos].shrink=shrink;
   child->parent=GTK_WIDGET(paned);
}

void gtk_paned_pack1(GtkPaned *paned,GtkWidget *child,gboolean resize,
                     gboolean shrink) {
   gtk_paned_pack(paned,0,child,resize,shrink);
}

void gtk_paned_pack2(GtkPaned *paned,GtkWidget *child,gboolean resize,
                     gboolean shrink) {
   gtk_paned_pack(paned,1,child,resize,shrink);
}

void gtk_paned_add1(GtkPaned *paned,GtkWidget *child) {
   gtk_paned_pack1(paned,child,FALSE,TRUE);
}

void gtk_paned_add2(GtkPaned *paned,GtkWidget *child) {
   gtk_paned_pack2(paned,child,FALSE,TRUE);
}

void gtk_paned_show_all(GtkWidget *widget,gboolean hWndOnly) {
   GtkPaned *paned=GTK_PANED(widget);
   gint i;
   for (i=0;i<2;i++) if (paned->children[i].widget) {
      gtk_widget_show_all_full(paned->children[i].widget,hWndOnly);
   }
}

void gtk_paned_hide_all(GtkWidget *widget,gboolean hWndOnly) {
   GtkPaned *paned=GTK_PANED(widget);
   gint i;
   for (i=0;i<2;i++) if (paned->children[i].widget)
      gtk_widget_hide_all_full(paned->children[i].widget,hWndOnly);
}

void gtk_paned_realize(GtkWidget *widget) {
   GtkPaned *paned=GTK_PANED(widget);
   gint i;
   for (i=0;i<2;i++) if (paned->children[i].widget) {
      gtk_widget_realize(paned->children[i].widget);
   }
}

void gtk_vpaned_realize(GtkWidget *widget) {
   HWND Parent;
   gtk_paned_realize(widget);
   Parent=gtk_get_parent_hwnd(widget);
   widget->hWnd = CreateWindow(WC_GTKVPANED,"",WS_CHILD,
                               0,0,0,0,Parent,NULL,hInst,NULL);
}

void gtk_hpaned_realize(GtkWidget *widget) {
   HWND Parent;
   gtk_paned_realize(widget);
   Parent=gtk_get_parent_hwnd(widget);
   widget->hWnd = CreateWindow(WC_GTKHPANED,"",WS_CHILD,
                               0,0,0,0,Parent,NULL,hInst,NULL);
}

static void gtk_paned_set_handle_percent(GtkPaned *paned,gint16 req[2]) {
   if (req[0]+req[1]) paned->handle_pos=req[0]*100/(req[0]+req[1]);
   else paned->handle_pos=0;
}

void gtk_vpaned_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   GtkPaned *paned=GTK_PANED(widget);
   gint i;
   gint16 req[2] = { 0,0 };
   requisition->width=requisition->height=0;
   for (i=0;i<2;i++) if (paned->children[i].widget) {
      if (paned->children[i].widget->requisition.width > requisition->width)
         requisition->width = paned->children[i].widget->requisition.width;
      req[i]=paned->children[i].widget->requisition.height;
      requisition->height += req[i];
   }
   requisition->height += paned->handle_size;
   gtk_paned_set_handle_percent(paned,req);
}

void gtk_hpaned_size_request(GtkWidget *widget,GtkRequisition *requisition) {
   GtkPaned *paned=GTK_PANED(widget);
   gint i;
   gint16 req[2] = { 0,0 };
   requisition->width=requisition->height=0;
   for (i=0;i<2;i++) if (paned->children[i].widget) {
      if (paned->children[i].widget->requisition.height > requisition->height)
         requisition->height = paned->children[i].widget->requisition.height;
      req[i]=paned->children[i].widget->requisition.width;
      requisition->width += req[i];
   }
   requisition->width += paned->handle_size;
   gtk_paned_set_handle_percent(paned,req);
}

void gtk_vpaned_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   GtkPaned *paned=GTK_PANED(widget);
   GtkWidget *child;
   gint16 alloc;
   GtkAllocation child_alloc;

   alloc=allocation->height-paned->handle_size;

   child=paned->children[0].widget;
   if (child) {
      child_alloc.x=allocation->x;
      child_alloc.y=allocation->y;
      child_alloc.width=allocation->width;
      child_alloc.height=alloc*paned->handle_pos/100;
      gtk_widget_set_size(child,&child_alloc);
   }

   child=paned->children[1].widget;
   if (child) {
      child_alloc.x=allocation->x;
      child_alloc.width=allocation->width;
      child_alloc.height=alloc*(100-paned->handle_pos)/100;
      child_alloc.y=allocation->y+allocation->height-child_alloc.height;
      gtk_widget_set_size(child,&child_alloc);
   }

   allocation->y += alloc*paned->handle_pos/100;
   allocation->height = paned->handle_size;
}

void gtk_hpaned_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   GtkPaned *paned=GTK_PANED(widget);
   GtkWidget *child;
   gint16 alloc;
   GtkAllocation child_alloc;

   alloc=allocation->width-paned->handle_size;

   child=paned->children[0].widget;
   if (child) {
      child_alloc.x=allocation->x;
      child_alloc.y=allocation->y;
      child_alloc.height=allocation->height;
      child_alloc.width=alloc*paned->handle_pos/100;
      gtk_widget_set_size(child,&child_alloc);
   }

   child=paned->children[1].widget;
   if (child) {
      child_alloc.x=allocation->x;
      child_alloc.height=allocation->height;
      child_alloc.width=alloc*(100-paned->handle_pos)/100;
      child_alloc.x=allocation->x+allocation->width-child_alloc.width;
      gtk_widget_set_size(child,&child_alloc);
   }

   allocation->x += alloc*paned->handle_pos/100;
   allocation->width = paned->handle_size;
}

void gtk_text_set_editable(GtkText *text,gboolean is_editable) {
   gtk_editable_set_editable(GTK_EDITABLE(text),is_editable);
}

void gtk_text_set_word_wrap(GtkText *text,gboolean word_wrap) {
   text->word_wrap=word_wrap;
}

void gtk_text_freeze(GtkText *text) {
}

void gtk_text_thaw(GtkText *text) {
}

void gtk_clist_sort(GtkCList *clist) {
}

void gtk_clist_freeze(GtkCList *clist) {
}

void gtk_clist_thaw(GtkCList *clist) {
}

void gtk_clist_clear(GtkCList *clist) {
   GtkCListRow *row;
   GSList *list;
   gint i;
   HWND hWnd;

   for (list=clist->rows;list;list=g_slist_next(list)) {
      row=(GtkCListRow *)list->data;
      for (i=0;i<clist->ncols;i++) {
         g_free(row->text[i]);
      }
      g_free(row);
   }
   g_slist_free(clist->rows);
   clist->rows=NULL;

   gtk_clist_update_all_widths(clist);
   hWnd=GTK_WIDGET(clist)->hWnd;
   if (hWnd) {
      SendMessage(hWnd,LB_RESETCONTENT,0,0);
   }
}

GtkWidget *gtk_option_menu_new() {
   GtkOptionMenu *option_menu;
   option_menu=GTK_OPTION_MENU(GtkNewObject(&GtkOptionMenuClass));
   return GTK_WIDGET(option_menu);
}

GtkWidget *gtk_option_menu_get_menu(GtkOptionMenu *option_menu) {
   return option_menu->menu;
}

void gtk_option_menu_set_menu(GtkOptionMenu *option_menu,GtkWidget *menu) {
   GSList *list;
   GtkMenuItem *menu_item;
   HWND hWnd;

   if (!menu) return;
   option_menu->menu=menu;
   hWnd=GTK_WIDGET(option_menu)->hWnd;

   if (hWnd) {
      SendMessage(hWnd,CB_RESETCONTENT,0,0);
      for (list=GTK_MENU_SHELL(menu)->children;list;list=g_slist_next(list)) {
         menu_item=GTK_MENU_ITEM(list->data);
         if (menu_item && menu_item->text)
            SendMessage(hWnd,CB_ADDSTRING,0,(LPARAM)menu_item->text);
      }
      SendMessage(hWnd,CB_SETCURSEL,(WPARAM)GTK_MENU(menu)->active,0);
   }
}

void gtk_option_menu_set_history(GtkOptionMenu *option_menu,guint index) {
   GtkWidget *menu;
   menu=gtk_option_menu_get_menu(option_menu);
   if (menu) gtk_menu_set_active(GTK_MENU(menu),index);
}

void gtk_option_menu_size_request(GtkWidget *widget,
                                  GtkRequisition *requisition) {
   SIZE size;

   if (GetTextSize(widget->hWnd,"Sample text",&size)) {
      requisition->width = size.cx+40;
      requisition->height = size.cy+4;
   }
}

void gtk_option_menu_set_size(GtkWidget *widget,GtkAllocation *allocation) {
   allocation->height *= 6;
}

void gtk_option_menu_realize(GtkWidget *widget) {
   HWND Parent;
   GtkOptionMenu *option_menu=GTK_OPTION_MENU(widget);

   Parent=gtk_get_parent_hwnd(widget);
   GTK_WIDGET_SET_FLAGS(widget,GTK_CAN_FOCUS);
   widget->hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,"COMBOBOX","",
                                 WS_CHILD|WS_TABSTOP|WS_VSCROLL|
                                 CBS_HASSTRINGS|CBS_DROPDOWNLIST,
                                 0,0,0,0,Parent,NULL,hInst,NULL);
   gtk_set_default_font(widget->hWnd);
   gtk_option_menu_set_menu(option_menu,option_menu->menu);
}

void gtk_label_set_text(GtkLabel *label,const gchar *str) {
   gint i;
   HWND hWnd;
   g_free(label->text);
   label->text = g_strdup(str ? str : "");
   for (i=0;i<strlen(label->text);i++) {
      if (label->text[i]=='_') label->text[i]='&';
   }
   hWnd=GTK_WIDGET(label)->hWnd;
   if (hWnd) {
      gtk_widget_update(GTK_WIDGET(label),FALSE);
      SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)label->text);
   }
}

void gtk_button_set_text(GtkButton *button,gchar *text) {
   gint i;
   HWND hWnd;
   g_free(button->text);
   button->text = g_strdup(text ? text : "");
   for (i=0;i<strlen(button->text);i++) {
      if (button->text[i]=='_') button->text[i]='&';
   }
   hWnd=GTK_WIDGET(button)->hWnd;
   if (hWnd) {
      gtk_widget_update(GTK_WIDGET(button),FALSE);
      SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)button->text);
   }
}

static void gtk_menu_item_set_text(GtkMenuItem *menuitem,gchar *text) {
   gint i;
   g_free(menuitem->text);
   menuitem->text = g_strdup(text ? text : "");
   for (i=0;i<strlen(menuitem->text);i++) {
      if (menuitem->text[i]=='_') menuitem->text[i]='&';
   }
}

guint gtk_label_parse_uline(GtkLabel *label,const gchar *str) {
   gint i;
   gtk_label_set_text(label,str);
   if (str) for (i=0;i<strlen(str);i++) {
      if (str[i]=='_') return str[i+1];
   }
   return 0;
}

void gtk_clist_set_row_data(GtkCList *clist,gint row,gpointer data) {
   GtkCListRow *list_row;
   if (row>=0 && row<g_slist_length(clist->rows)) {
      list_row=(GtkCListRow *)g_slist_nth_data(clist->rows,row);
      if (list_row) list_row->data=data;
   }
}

gpointer gtk_clist_get_row_data(GtkCList *clist,gint row) {
   GtkCListRow *list_row;
   if (row>=0 && row<g_slist_length(clist->rows)) {
      list_row=(GtkCListRow *)g_slist_nth_data(clist->rows,row);
      if (list_row) return list_row->data;
   }
   return NULL;
}

void gtk_clist_set_auto_sort(GtkCList *clist,gboolean auto_sort) {
   clist->auto_sort=auto_sort;
}

void gtk_clist_columns_autosize(GtkCList *clist) {
}

void gtk_text_set_point(GtkText *text,guint index) {
   gtk_editable_set_position(GTK_EDITABLE(text),index);
}

void gtk_widget_set_usize(GtkWidget *widget,gint width,gint height) {
}

void gtk_clist_select_row(GtkCList *clist,gint row,gint column) {
   HWND hWnd;
   hWnd=GTK_WIDGET(clist)->hWnd;
   if (hWnd) {
      if (clist->mode==GTK_SELECTION_SINGLE) {
         SendMessage(hWnd,LB_SETCURSEL,(WPARAM)row,0);
      } else {
         SendMessage(hWnd,LB_SETSEL,(WPARAM)TRUE,(LPARAM)row);
      }
      gtk_clist_update_selection(GTK_WIDGET(clist));
   }
}

GtkVisibility gtk_clist_row_is_visible(GtkCList *clist,gint row) {
   return GTK_VISIBILITY_FULL;
}

void gtk_clist_moveto(GtkCList *clist,gint row,gint column,
                      gfloat row_align,gfloat col_align) {
}

void gtk_clist_set_compare_func(GtkCList *clist,GtkCListCompareFunc cmp_func) {
}

void gtk_clist_set_column_auto_resize(GtkCList *clist,gint column,
                                      gboolean auto_resize) {
   if (clist && column>=0 && column<clist->ncols) {
      clist->cols[column].auto_resize=auto_resize;
   }
}

void gtk_clist_update_selection(GtkWidget *widget) {
   GtkCList *clist=GTK_CLIST(widget);
   gint i;
   g_list_free(clist->selection);
   clist->selection=NULL;
   if (widget->hWnd==NULL) return;
   for (i=0;i<g_slist_length(clist->rows);i++) {
      if (SendMessage(widget->hWnd,LB_GETSEL,(WPARAM)i,0) > 0) {
         clist->selection=g_list_append(clist->selection,GINT_TO_POINTER(i));
      }
   }
}

void gtk_editable_create(GtkWidget *widget) {
   GtkEditable *editable=GTK_EDITABLE(widget);
   gtk_widget_create(widget);
   editable->is_editable=TRUE;
   editable->text=g_string_new("");
}

void gtk_option_menu_update_selection(GtkWidget *widget) {
   LRESULT lres;
   GtkMenuShell *menu;
   GtkWidget *menu_item;

   if (widget->hWnd==NULL) return;
   lres=SendMessage(widget->hWnd,CB_GETCURSEL,0,0);
   if (lres==CB_ERR) return;

   menu=GTK_MENU_SHELL(gtk_option_menu_get_menu(GTK_OPTION_MENU(widget)));
   if (menu) {
      menu_item=GTK_WIDGET(g_slist_nth_data(menu->children,lres));
      if (menu_item) gtk_signal_emit(GTK_OBJECT(menu_item),"activate");
   }
}

void gtk_window_handle_user_size(GtkWindow *window,
                                 GtkAllocation *allocation) {
}

void gtk_window_set_initial_position(GtkWindow *window,
                                     GtkAllocation *allocation) {
   RECT rect;
   GtkWidget *widget=GTK_WIDGET(window);
   GetWindowRect(GetDesktopWindow(),&rect);

   if (widget->parent) {
      allocation->x = rect.left+(rect.right-rect.left-allocation->width)/2;
      allocation->y = rect.top+(rect.bottom-rect.top-allocation->height)/2;
      if (allocation->x < 0) allocation->x = 0;
      if (allocation->y < 0) allocation->y = 0;
      if (widget->hWnd) SetWindowPos(widget->hWnd,HWND_TOP,allocation->x,
                                     allocation->y,0,0,SWP_NOSIZE|SWP_NOZORDER);
   }
}

void gtk_window_handle_auto_size(GtkWindow *window,
                                 GtkAllocation *allocation) {
   GtkWidget *widget=GTK_WIDGET(window);
   if (allocation->width < window->default_width) {
      allocation->width=window->default_width;
   }
   if (allocation->height < window->default_height) {
      allocation->height=window->default_height;
   }
   if (allocation->width < widget->allocation.width) {
      allocation->width=widget->allocation.width;
   }
   if (allocation->height < widget->allocation.height) {
      allocation->height=widget->allocation.height;
   }
}
