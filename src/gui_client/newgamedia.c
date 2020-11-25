/************************************************************************
 * newgamedia.c   New game dialog                                       *
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

#include <string.h>
#include <stdlib.h>              /* For atoi */
#include <glib.h>

#include "dopewars.h"
#include "network.h"
#include "message.h"
#include "nls.h"
#include "gtkport/gtkport.h"
#include "gtk_client.h"
#include "newgamedia.h"

struct StartGameStruct {
  GtkWidget *dialog, *name, *hostname, *port, *antique, *status,
            *metaserv, *notebook;
  Player *play;
#ifdef NETWORKING
  CurlConnection *MetaConn;
  GSList *NewMetaList;
  NBCallBack sockstat;
#endif
};

static struct StartGameStruct stgam;

#ifdef NETWORKING
static void SocksAuthDialog(NetworkBuffer *netbuf, gpointer data);
static void FillMetaServerList(gboolean UseNewList);

/* List of servers on the metaserver */
static GSList *MetaList = NULL;

#endif /* NETWORKING */

/* Which notebook page to display in the New Game dialog */
static gint NewGameType = 0;

static gboolean GetStartGamePlayerName(gchar **PlayerName)
{
  g_free(*PlayerName);
  *PlayerName = gtk_editable_get_chars(GTK_EDITABLE(stgam.name), 0, -1);
  if (*PlayerName && (*PlayerName)[0])
    return TRUE;
  else {
    GtkMessageBox(stgam.dialog,
                  _("You can't start the game without giving a name first!"),
                  _("New Game"), GTK_MESSAGE_WARNING, MB_OK);
    return FALSE;
  }
}

static void SetStartGameStatus(gchar *msg)
{
  gtk_label_set_text(GTK_LABEL(stgam.status),
                     msg ? msg : _("Status: Waiting for user input"));
}

#ifdef NETWORKING


static void ReportMetaConnectError(GError *err)
{
  char *str = g_strdup_printf(_("Status: ERROR: %s"), err->message);
  SetStartGameStatus(str);
  g_free(str);
}

/* Called by glib when we get action on a multi socket */
static gboolean glib_socket(GIOChannel *ch, GIOCondition condition,
                            gpointer data)
{
  CurlConnection *g = (CurlConnection*) data;
  int still_running;
  GError *err = NULL;
  int fd = g_io_channel_unix_get_fd(ch);
  int action =
    ((condition & G_IO_IN) ? CURL_CSELECT_IN : 0) |
    ((condition & G_IO_OUT) ? CURL_CSELECT_OUT : 0);

  CurlConnectionSocketAction(g, fd, action, &still_running, &err);
  if (!err && still_running) {
    return TRUE;
  } else {
    if (g->timer_event) {
      dp_g_source_remove(g->timer_event);
      g->timer_event = 0;
    }
    if (!err)
      HandleWaitingMetaServerData(stgam.MetaConn, &stgam.NewMetaList, &err);
    if (err) {
      ReportMetaConnectError(err);
      g_error_free(err);
    } else {
      SetStartGameStatus(NULL);
    }

    CloseCurlConnection(stgam.MetaConn);
    FillMetaServerList(TRUE);
    return FALSE;
  }
}

static gboolean glib_timeout(gpointer userp)
{
  CurlConnection *g = userp;
  GError *err = NULL;
  int still_running;
  if (!CurlConnectionSocketAction(g, CURL_SOCKET_TIMEOUT, 0, &still_running,
                                  &err)) {
    ReportMetaConnectError(err);
    g_error_free(err);
  }
  g->timer_event = 0;
  return G_SOURCE_REMOVE;
}

static void ConnectError(void)
{
  GString *neterr;
  gchar *text;
  LastError *error;

  error = stgam.play->NetBuf.error;

  neterr = g_string_new("");

  if (error) {
    g_string_assign_error(neterr, error);
  } else {
    g_string_assign(neterr, _("Connection closed by remote host"));
  }

  /* Error: GTK+ client could not connect to the given dopewars server */
  text = g_strdup_printf(_("Status: Could not connect (%s)"), neterr->str);

  SetStartGameStatus(text);
  g_free(text);
  g_string_free(neterr, TRUE);
}

void FinishServerConnect(gboolean ConnectOK)
{
  if (ConnectOK) {
    Client = Network = TRUE;
    gtk_widget_destroy(stgam.dialog);
    GuiStartGame();
  } else {
    ConnectError();
  }
}

static void DoConnect(void)
{
  gchar *text;
  NetworkBuffer *NetBuf;
  NBStatus oldstatus;
  NBSocksStatus oldsocks;

  NetBuf = &stgam.play->NetBuf;

  /* Message displayed during the attempted connect to a dopewars server */
  text = g_strdup_printf(_("Status: Attempting to contact %s..."),
                         ServerName);
  SetStartGameStatus(text);
  g_free(text);

  /* Terminate any existing connection attempts */
  ShutdownNetworkBuffer(NetBuf);
  if (stgam.MetaConn->running) {
    CloseCurlConnection(stgam.MetaConn);
  }

  oldstatus = NetBuf->status;
  oldsocks = NetBuf->sockstat;
  if (StartNetworkBufferConnect(NetBuf, NULL, ServerName, Port)) {
    DisplayConnectStatus(oldstatus, oldsocks);
    SetNetworkBufferUserPasswdFunc(NetBuf, SocksAuthDialog, NULL);
    SetNetworkBufferCallBack(NetBuf, stgam.sockstat, NULL);
  } else {
    ConnectError();
  }
}

static void ConnectToServer(GtkWidget *widget, gpointer data)
{
  gchar *text;

  g_free(ServerName);
  ServerName = gtk_editable_get_chars(GTK_EDITABLE(stgam.hostname),
                                      0, -1);
  text = gtk_editable_get_chars(GTK_EDITABLE(stgam.port), 0, -1);
  Port = atoi(text);
  g_free(text);

  if (GetStartGamePlayerName(&stgam.play->Name)) {
    DoConnect();
  }
}

/* Columns in metaserver list */
enum {
  META_COL_SERVER = 0,
  META_COL_PORT,
  META_COL_VERSION,
  META_COL_PLAYERS,
  META_COL_COMMENT,
  META_NUM_COLS
};

static void FillMetaServerList(gboolean UseNewList)
{
  GtkWidget *metaserv;
  GtkListStore *store;
  ServerData *ThisServer;
  GtkTreeIter iter;
  GSList *ListPt;

  if (UseNewList && !stgam.NewMetaList)
    return;

  metaserv = stgam.metaserv;
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(metaserv)));

  gtk_list_store_clear(store);

  if (UseNewList) {
    ClearServerList(&MetaList);
    MetaList = stgam.NewMetaList;
    stgam.NewMetaList = NULL;
  }

  for (ListPt = MetaList; ListPt; ListPt = g_slist_next(ListPt)) {
    char *players;
    ThisServer = (ServerData *)(ListPt->data);
    if (ThisServer->CurPlayers == -1) {
      /* Displayed if we don't know how many players are logged on to a
         server */
      players = _("Unknown");
    } else {
      /* e.g. "5 of 20" means 5 players are logged on to a server, out of
         a maximum of 20 */
      players = g_strdup_printf(_("%d of %d"), ThisServer->CurPlayers,
                                  ThisServer->MaxPlayers);
    }
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, META_COL_SERVER, ThisServer->Name,
		       META_COL_PORT, ThisServer->Port,
		       META_COL_VERSION, ThisServer->Version,
		       META_COL_PLAYERS, players,
		       META_COL_COMMENT, ThisServer->Comment, -1);
    if (ThisServer->CurPlayers != -1)
      g_free(players);
  }
}

void DisplayConnectStatus(NBStatus oldstatus, NBSocksStatus oldsocks)
{
  NBStatus status;
  NBSocksStatus sockstat;
  gchar *text;

  status = stgam.play->NetBuf.status;
  sockstat = stgam.play->NetBuf.sockstat;
  if (oldstatus == status && sockstat == oldsocks)
    return;

  switch (status) {
  case NBS_PRECONNECT:
    break;
  case NBS_SOCKSCONNECT:
    switch (sockstat) {
    case NBSS_METHODS:
      /* Tell the user that we've successfully connected to a SOCKS server,
         and are now ready to tell it to initiate the "real" connection */
      text = g_strdup_printf(_("Status: Connected to SOCKS server %s..."),
                             Socks.name);
      SetStartGameStatus(text);
      g_free(text);
      break;
    case NBSS_USERPASSWD:
      /* Tell the user that the SOCKS server is asking us for a username
         and password */
      SetStartGameStatus(_("Status: Authenticating with SOCKS server"));
      break;
    case NBSS_CONNECT:
      text =
      /* Tell the user that all necessary SOCKS authentication has been
         completed, and now we're going to try to have it connect to
         the final destination */
          g_strdup_printf(_("Status: Asking SOCKS for connect to %s..."),
                          ServerName);
      SetStartGameStatus(text);
      g_free(text);
      break;
    }
    break;
  case NBS_CONNECTED:
    break;
  }
}

static void UpdateMetaServerList(GtkWidget *widget)
{
  gchar *text;
  GError *tmp_error = NULL;

  /* Terminate any existing connection attempts */
  ShutdownNetworkBuffer(&stgam.play->NetBuf);
  if (stgam.MetaConn->running) {
    CloseCurlConnection(stgam.MetaConn);
  }

  ClearServerList(&stgam.NewMetaList);

  /* Message displayed during the attempted connect to the metaserver */
  text = g_strdup_printf(_("Status: Attempting to contact %s..."),
                         MetaServer.URL);
  SetStartGameStatus(text);
  g_free(text);

  if (!OpenMetaHttpConnection(stgam.MetaConn, &tmp_error)) {
    text = g_strdup_printf(_("Status: ERROR: %s"), tmp_error->message);
    g_error_free(tmp_error);
    SetStartGameStatus(text);
    g_free(text);
  }
}

static void MetaServerConnect(GtkWidget *widget, gpointer data)
{
  GtkTreeSelection *treesel;
  GtkTreeModel *model;
  GtkTreeIter iter;

  treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(stgam.metaserv));

  if (gtk_tree_selection_get_selected(treesel, &model, &iter)) {
    gchar *name;
    gtk_tree_model_get(model, &iter, META_COL_SERVER, &name,
                       META_COL_PORT, &Port, -1);
    AssignName(&ServerName, name);
    g_free(name);

    if (GetStartGamePlayerName(&stgam.play->Name)) {
      DoConnect();
    }
  }
}
#endif /* NETWORKING */

static void StartSinglePlayer(GtkWidget *widget, gpointer data)
{
  WantAntique =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(stgam.antique));
  if (!GetStartGamePlayerName(&stgam.play->Name))
    return;
  GuiStartGame();
  gtk_widget_destroy(stgam.dialog);
}

static void CloseNewGameDia(GtkWidget *widget, gpointer data)
{
#ifdef NETWORKING
  /* Terminate any existing connection attempts */
  if (stgam.play->NetBuf.status != NBS_CONNECTED) {
    ShutdownNetworkBuffer(&stgam.play->NetBuf);
  }
  if (stgam.MetaConn) {
    CloseCurlConnection(stgam.MetaConn);
    stgam.MetaConn = NULL;
  }
  ClearServerList(&stgam.NewMetaList);
#endif

  /* Remember which tab we chose for the next time we use this dialog */
  NewGameType = gtk_notebook_get_current_page(GTK_NOTEBOOK(stgam.notebook));
}

#ifdef NETWORKING
static void metalist_changed(GtkTreeSelection *sel, GtkWidget *conn_button)
{
  gtk_widget_set_sensitive(conn_button,
                           gtk_tree_selection_count_selected_rows(sel) > 0);
}
#endif

#ifdef NETWORKING
static GtkTreeModel *create_metaserver_model(void)
{
  GtkListStore *store;

  store = gtk_list_store_new(META_NUM_COLS, G_TYPE_STRING, G_TYPE_UINT,
                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  return GTK_TREE_MODEL(store);
}

static GtkWidget *create_metaserver_view(GtkWidget **pack_widg)
{
  int i;
  GtkWidget *view;
  GtkTreeModel *model;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  gchar *server_titles[META_NUM_COLS];
  gboolean expand[META_NUM_COLS];

  /* Column titles of metaserver information */
  server_titles[0] = _("Server");  expand[0] = TRUE;
  server_titles[1] = _("Port");    expand[1] = FALSE;
  server_titles[2] = _("Version"); expand[2] = FALSE;
  server_titles[3] = _("Players"); expand[3] = FALSE;
  server_titles[4] = _("Comment"); expand[4] = TRUE;

  view = gtk_scrolled_tree_view_new(pack_widg);
  renderer = gtk_cell_renderer_text_new();
  for (i = 0; i < META_NUM_COLS; ++i) {
    col = gtk_tree_view_column_new_with_attributes(
              server_titles[i], renderer, "text", i, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_expand(col, expand[i]);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);
  }
  model = create_metaserver_model();
  gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
  /* Tree view keeps a reference, so we can drop ours */
  g_object_unref(model);
  return view;
}
#endif

#ifdef NETWORKING
void NewGameDialog(Player *play, NBCallBack sockstat, CurlConnection *MetaConn)
#else
void NewGameDialog(Player *play)
#endif
{
  GtkWidget *vbox, *vbox2, *hbox, *label, *entry, *notebook;
  GtkWidget *frame, *button, *dialog;
  GtkAccelGroup *accel_group;
#if GTK_MAJOR_VERSION == 2
  guint AccelKey;
#endif

#ifdef NETWORKING
  GtkWidget *clist, *scrollwin, *table, *hbbox, *defbutton;
  GtkTreeSelection *treesel;
  gchar *ServerEntry, *text;
  gboolean UpdateMeta = FALSE;
  SetCurlCallback(MetaConn, glib_timeout, glib_socket);

  stgam.MetaConn = MetaConn;
  stgam.NewMetaList = NULL;
  stgam.sockstat = sockstat;

#endif /* NETWORKING */

  stgam.play = play;
  stgam.dialog = dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(G_OBJECT(dialog), "destroy",
                   G_CALLBACK(CloseNewGameDia), NULL);

  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(MainWindow));
#ifdef NETWORKING
  gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 300);
#endif
  accel_group = gtk_accel_group_new();

  /* Title of 'New Game' dialog */
  gtk_window_set_title(GTK_WINDOW(dialog), _("New Game"));
  my_set_dialog_position(GTK_WINDOW(dialog));
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);
  gtk_window_add_accel_group(GTK_WINDOW(dialog), accel_group);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 7);

  label = gtk_label_new("");

#if GTK_MAJOR_VERSION == 2
  AccelKey = gtk_label_parse_uline(GTK_LABEL(label),
#else
  gtk_label_set_text_with_mnemonic(GTK_LABEL(label),
#endif
                                   /* Prompt for player's name in 'New
                                      Game' dialog */
                                   _("Hey dude, what's your _name?"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  entry = stgam.name = gtk_entry_new();
#if GTK_MAJOR_VERSION == 2
  gtk_widget_add_accelerator(entry, "grab-focus", accel_group, AccelKey,
                             GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
#else
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
#endif
  gtk_entry_set_text(GTK_ENTRY(entry), GetPlayerName(stgam.play));
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  notebook = stgam.notebook = gtk_notebook_new();

#ifdef NETWORKING
  frame = gtk_frame_new(_("Server"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 4);
  vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
  gtk_container_set_border_width(GTK_CONTAINER(vbox2), 4);
  table = gtk_table_new(2, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  gtk_table_set_col_spacings(GTK_TABLE(table), 4);

  /* Prompt for hostname to connect to in GTK+ new game dialog */
  label = gtk_label_new(_("Host name"));

  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                   GTK_SHRINK, GTK_SHRINK, 0, 0);
  entry = stgam.hostname = gtk_entry_new();

  ServerEntry = "localhost";
  if (g_ascii_strncasecmp(ServerName, SN_META, strlen(SN_META)) == 0) {
    NewGameType = 2;
    UpdateMeta = TRUE;
  } else if (g_ascii_strncasecmp(ServerName, SN_PROMPT, strlen(SN_PROMPT)) == 0)
    NewGameType = 0;
  else if (g_ascii_strncasecmp(ServerName, SN_SINGLE, strlen(SN_SINGLE)) == 0)
    NewGameType = 1;
  else
    ServerEntry = ServerName;

  gtk_entry_set_text(GTK_ENTRY(entry), ServerEntry);
  gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 0, 1);
  label = gtk_label_new(_("Port"));
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                   GTK_SHRINK, GTK_SHRINK, 0, 0);
  entry = stgam.port = gtk_entry_new();
  text = g_strdup_printf("%d", Port);
  gtk_entry_set_text(GTK_ENTRY(entry), text);
  g_free(text);
  gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 1, 2);

  gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);

  button = gtk_button_new_with_label("");
  /* Button to connect to a named dopewars server */
  SetAccelerator(button, _("_Connect"), button, "clicked", accel_group, TRUE);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(ConnectToServer), NULL);
  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);
  gtk_widget_set_can_default(button, TRUE);
  defbutton = button;
  
  label = gtk_label_new(_("Server"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);
#endif /* NETWORKING */

  /* Title of 'New Game' dialog notebook tab for single-player mode */
  frame = gtk_frame_new(_("Single player"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 4);
  vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
  gtk_container_set_border_width(GTK_CONTAINER(vbox2), 4);
  stgam.antique = gtk_check_button_new_with_label("");

  /* Checkbox to activate 'antique mode' in single-player games */
  SetAccelerator(stgam.antique, _("_Antique mode"), stgam.antique,
                 "clicked", accel_group, TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stgam.antique),
                               WantAntique);
  gtk_box_pack_start(GTK_BOX(vbox2), stgam.antique, FALSE, FALSE, 0);
  button = gtk_button_new_with_label("");

  /* Button to start a new single-player (standalone, non-network) game */
  SetAccelerator(button, _("_Start single-player game"), button,
                 "clicked", accel_group, TRUE);

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(StartSinglePlayer), NULL);
  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);
  label = gtk_label_new(_("Single player"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

#ifdef NETWORKING
  /* Title of Metaserver frame in New Game dialog */
  frame = gtk_frame_new(_("Metaserver"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 4);

  vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
  gtk_container_set_border_width(GTK_CONTAINER(vbox2), 4);

  clist = stgam.metaserv = create_metaserver_view(&scrollwin);
  gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(clist), FALSE);
  gtk_tree_selection_set_mode(
       gtk_tree_view_get_selection(GTK_TREE_VIEW(clist)), GTK_SELECTION_SINGLE);

  gtk_box_pack_start(GTK_BOX(vbox2), scrollwin, TRUE, TRUE, 0);

  hbbox = my_hbbox_new();

  /* Button to update metaserver information */
  button = NewStockButton(GTK_STOCK_REFRESH, accel_group);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(UpdateMetaServerList), NULL);
  my_gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  button = gtk_button_new_with_label("");
  SetAccelerator(button, _("_Connect"), button, "clicked", accel_group, TRUE);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(MetaServerConnect), NULL);
  gtk_widget_set_sensitive(button, FALSE);
  treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(clist));
  g_signal_connect(treesel, "changed", G_CALLBACK(metalist_changed), button);
  my_gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  gtk_box_pack_start(GTK_BOX(vbox2), hbbox, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);

  label = gtk_label_new(_("Metaserver"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);
#endif /* NETWORKING */

  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

  /* Caption of status label in New Game dialog before anything has
   * happened */
  label = stgam.status = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(stgam.dialog), vbox);

  gtk_widget_grab_focus(stgam.name);
#ifdef NETWORKING
  if (UpdateMeta) {
    UpdateMetaServerList(NULL);
  } else {
    FillMetaServerList(FALSE);
  }
#endif

  SetStartGameStatus(NULL);
  gtk_widget_show_all(dialog);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), NewGameType);
#ifdef NETWORKING
  gtk_widget_grab_default(defbutton);
#endif
}

#ifdef NETWORKING
static void OKSocksAuth(GtkWidget *widget, GtkWidget *window)
{
  g_object_set_data(G_OBJECT(window), "authok", GINT_TO_POINTER(TRUE));
  gtk_widget_destroy(window);
}

static void DestroySocksAuth(GtkWidget *window, gpointer data)
{
  GtkWidget *userentry, *passwdentry;
  gchar *username = NULL, *password = NULL;
  gpointer authok;
  NetworkBuffer *netbuf;

  authok = g_object_get_data(G_OBJECT(window), "authok");
  userentry =
      (GtkWidget *)g_object_get_data(G_OBJECT(window), "username");
  passwdentry =
      (GtkWidget *)g_object_get_data(G_OBJECT(window), "password");
  netbuf =
      (NetworkBuffer *)g_object_get_data(G_OBJECT(window), "netbuf");

  g_assert(userentry && passwdentry && netbuf);

  if (authok) {
    username = gtk_editable_get_chars(GTK_EDITABLE(userentry), 0, -1);
    password = gtk_editable_get_chars(GTK_EDITABLE(passwdentry), 0, -1);
  }

  SendSocks5UserPasswd(netbuf, username, password);
  g_free(username);
  g_free(password);
}

static void SocksAuthDialog(NetworkBuffer *netbuf, gpointer data)
{
  GtkWidget *window, *button, *hsep, *vbox, *label, *entry, *table, *hbbox;
  GtkAccelGroup *accel_group;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

  g_signal_connect(G_OBJECT(window), "destroy",
                   G_CALLBACK(DestroySocksAuth), NULL);
  g_object_set_data(G_OBJECT(window), "netbuf", (gpointer)netbuf);

  /* Title of dialog for authenticating with a SOCKS server */
  gtk_window_set_title(GTK_WINDOW(window),
                       _("SOCKS Authentication Required"));
  my_set_dialog_position(GTK_WINDOW(window));

  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(window),
                               GTK_WINDOW(MainWindow));
  gtk_container_set_border_width(GTK_CONTAINER(window), 7);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);

  table = gtk_table_new(2, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 10);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);

  label = gtk_label_new("User name:");
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

  entry = gtk_entry_new();
  g_object_set_data(G_OBJECT(window), "username", (gpointer)entry);
  gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 0, 1);

  label = gtk_label_new("Password:");
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

  entry = gtk_entry_new();
  g_object_set_data(G_OBJECT(window), "password", (gpointer)entry);

#ifdef HAVE_FIXED_GTK
  /* GTK+ versions earlier than 1.2.10 do bad things with this */
  gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
#endif

  gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 1, 2);

  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

  hsep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();

  button = NewStockButton(GTK_STOCK_OK, accel_group);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(OKSocksAuth), (gpointer)window);
  my_gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  button = NewStockButton(GTK_STOCK_CANCEL, accel_group);
  g_signal_connect_swapped(G_OBJECT(button), "clicked",
                           G_CALLBACK(gtk_widget_destroy),
                           G_OBJECT(window));
  my_gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  gtk_box_pack_start(GTK_BOX(vbox), hbbox, TRUE, TRUE, 0);

  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show_all(window);
}

#endif /* NETWORKING */
