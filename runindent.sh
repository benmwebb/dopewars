#!/bin/sh -f

indent -kr -i2 -cp1 -ncs -bad -nut \
       -fc1 -fca -sc \
\
       -T SCREEN \
\
       -T ssize_t -T fd_set -T FILE \
\
       -T price_t -T AbilType -T Abilities -T ClientType -T DispMode \
       -T EventCode -T PlayerFlags -T DrugIndex -T Inventory -T Player \
       -T DopeEntry -T DopeList -T ServerData -T ErrorType -T LastError \
       -T CustomErrorCode -T ErrTable -T MetaErrorCode -T MsgCode -T AICode \
       -T FightPoint -T SocksMethods -T SocksErrorCode -T HttpErrorCode \
       -T ConnBuf -T NetworkBuffer -T NBCallBack -T NBUserPasswd \
       -T SocksServer -T NBStatus -T NBSocksStatus -T HttpStatus \
       -T HttpConnection -T HCAuthFunc -T OfferForce -T FmtData \
\
       -T TCHAR -T DWORD -T HANDLE -T ACCEL -T RECT -T COLORREF \
       -T LPARAM -T WPARAM -T LPMINMAXINFO -T HWND -T LPMEASUREITEMSTRUCT \
       -T HD_NOTIFY -T NMHDR -T UINT -T SOCKET -T HBRUSH -T LONG -T LPTSTR \
       -T HFONT -T BOOL \
\
       -T GString -T gpointer -T gconstpointer -T gchar -T guchar -T gint \
       -T guint -T GList -T GSList -T gboolean -T GScannerConfig -T GScanner \
       -T GPtrArray -T GArray -T gfloat -T gint16 \
\
       -T GtkObject -T GtkCList -T GtkWidget -T GtkRequisition \
       -T GtkAllocation -T GtkWindow -T GtkContainer -T GtkRadioButton \
       -T GtkAdjustment -T GtkEntry -T GtkAccelGroup -T GtkTable \
       -T GtkItemFactory -T GtkItemFactoryEntry -T GtkEditable -T GtkText \
       -T GtkBox -T GtkToggleButton -T GtkMenuShell -T GtkMenuBar -T GtkMenu \
       -T GtkNotebook -T GtkMenuItem -T GtkPaned -T GtkOptionMenu -T GtkLabel \
       -T GtkSpinButton -T GtkMisc -T GtkProgressBar -T GdkEvent -T GtkButton \
       -T GtkClass -T GdkInput -T GtkBoxChild -T GtkTableChild -T GtkSignal \
       -T GtkGIntSignalFunc -T GtkItemFactoryChild -T GtkNotebookChild \
       -T GtkCListRow -T GdkEventButton -T GtkCheckButton -T GtkTimeout \
\
       -T NTService -T InstFiles -T InstLink -T InstData -T bstr -T InstFlags
