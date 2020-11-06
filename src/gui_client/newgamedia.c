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

CurlGlobalData global_data;

/* Called by glib when we get action on a multi socket */
static gboolean glib_socket(GIOChannel *ch, GIOCondition condition,
                            gpointer data)
{
  CurlGlobalData *g = (CurlGlobalData*) data;
  CURLMcode rc;
  int fd = g_io_channel_unix_get_fd(ch);
  fprintf(stderr, "bw> glib socket\n");
  int action =
    ((condition & G_IO_IN) ? CURL_CSELECT_IN : 0) |
    ((condition & G_IO_OUT) ? CURL_CSELECT_OUT : 0);

  rc = curl_multi_socket_action(g->multi, fd, action, &g->still_running);
  if (rc != CURLM_OK) fprintf(stderr, "action %d %s\n", rc, curl_multi_strerror(rc));
  if (g->still_running) {
    return TRUE;
  } else {
    GError *tmp_error = NULL;
    fprintf(stderr, "got data %s\n", stgam.MetaConn->data);
    fprintf(stderr, "last transfer done, kill timeout\n");
    if (g->timer_event) {
      g_source_remove(g->timer_event);
      g->timer_event = 0;
    }
    if (!HandleWaitingMetaServerData(stgam.MetaConn, &stgam.NewMetaList,
                                     &tmp_error)) {
      char *str = g_strdup_printf(_("Status: ERROR: %s"), tmp_error->message);
      SetStartGameStatus(str);
      g_free(str);
      g_error_free(tmp_error);
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
  CurlGlobalData *g = userp;
  CURLMcode rc;
  fprintf(stderr, "bw> glib_timeout\n");
  rc = curl_multi_socket_action(g->multi, CURL_SOCKET_TIMEOUT, 0, &g->still_running);
  if (rc != CURLM_OK) fprintf(stderr, "action %d %s\n", rc, curl_multi_strerror(rc));
  g->timer_event = 0;
  return G_SOURCE_REMOVE;
}

static void ConnectError(gboolean meta)
{
  GString *neterr;
  gchar *text;
  LastError *error;

/*if (meta)
    error = stgam.MetaConn->NetBuf.error;
  else*/
    error = stgam.play->NetBuf.error;

  neterr = g_string_new("");

  if (error) {
    g_string_assign_error(neterr, error);
  } else {
    g_string_assign(neterr, _("Connection closed by remote host"));
  }

  if (meta) {
    /* Error: GTK+ client could not connect to the metaserver */
    text =
        g_strdup_printf(_("Status: Could not connect to metaserver (%s)"),
                        neterr->str);
  } else {
    /* Error: GTK+ client could not connect to the given dopewars server */
    text =
        g_strdup_printf(_("Status: Could not connect (%s)"), neterr->str);
  }

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
    ConnectError(FALSE);
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
    ConnectError(FALSE);
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

static void FillMetaServerList(gboolean UseNewList)
{
  GtkWidget *metaserv;
  ServerData *ThisServer;
  gchar *titles[5];
  GSList *ListPt;
  gint row, width;

  if (UseNewList && !stgam.NewMetaList)
    return;

  metaserv = stgam.metaserv;
  gtk_clist_freeze(GTK_CLIST(metaserv));
  gtk_clist_clear(GTK_CLIST(metaserv));

  if (UseNewList) {
    ClearServerList(&MetaList);
    MetaList = stgam.NewMetaList;
    stgam.NewMetaList = NULL;
  }

  for (ListPt = MetaList; ListPt; ListPt = g_slist_next(ListPt)) {
    ThisServer = (ServerData *)(ListPt->data);
    titles[0] = ThisServer->Name;
    titles[1] = g_strdup_printf("%d", ThisServer->Port);
    titles[2] = ThisServer->Version;
    if (ThisServer->CurPlayers == -1) {
      /* Displayed if we don't know how many players are logged on to a
       * server */
      titles[3] = _("Unknown");
    } else {
      /* e.g. "5 of 20" means 5 players are logged on to a server, out of
       * a maximum of 20 */
      titles[3] = g_strdup_printf(_("%d of %d"), ThisServer->CurPlayers,
                                  ThisServer->MaxPlayers);
    }
    titles[4] = ThisServer->Comment;
    row = gtk_clist_append(GTK_CLIST(metaserv), titles);
    gtk_clist_set_row_data(GTK_CLIST(metaserv), row, (gpointer)ThisServer);
    g_free(titles[1]);
    if (ThisServer->CurPlayers != -1)
      g_free(titles[3]);
  }
  if (MetaList) {
    width = gtk_clist_optimal_column_width(GTK_CLIST(metaserv), 4);
    gtk_clist_set_column_width(GTK_CLIST(metaserv), 4, width);
    width = gtk_clist_optimal_column_width(GTK_CLIST(metaserv), 3);
    gtk_clist_set_column_width(GTK_CLIST(metaserv), 3, width);
    width = gtk_clist_optimal_column_width(GTK_CLIST(metaserv), 0);
    gtk_clist_set_column_width(GTK_CLIST(metaserv), 0, width);
  }
  gtk_clist_thaw(GTK_CLIST(metaserv));
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
       * and are now ready to tell it to initiate the "real" connection */
      text = g_strdup_printf(_("Status: Connected to SOCKS server %s..."),
                             Socks.name);
      SetStartGameStatus(text);
      g_free(text);
      break;
    case NBSS_USERPASSWD:
      /* Tell the user that the SOCKS server is asking us for a username
       * and password */
      SetStartGameStatus(_("Status: Authenticating with SOCKS server"));
      break;
    case NBSS_CONNECT:
      text =
      /* Tell the user that all necessary SOCKS authentication has been
       * completed, and now we're going to try to have it connect to
       * the final destination */
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
  GList *selection;
  gint row;
  GtkWidget *clist;
  ServerData *ThisServer;

  clist = stgam.metaserv;
  selection = GTK_CLIST(clist)->selection;
  if (selection) {
    row = GPOINTER_TO_INT(selection->data);
    ThisServer = (ServerData *)gtk_clist_get_row_data(GTK_CLIST(clist), row);
    AssignName(&ServerName, ThisServer->Name);
    Port = ThisServer->Port;

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

static void metalist_row_select(GtkWidget *clist, gint row, gint column,
                                GdkEvent *event, GtkWidget *conn_button)
{
  gtk_widget_set_sensitive(conn_button, TRUE);
}

static void metalist_row_unselect(GtkWidget *clist, gint row, gint column,
                                  GdkEvent *event, GtkWidget *conn_button)
{
  gtk_widget_set_sensitive(conn_button, FALSE);
}

#ifdef NETWORKING
void NewGameDialog(Player *play, NBCallBack sockstat, CurlConnection *MetaConn)
#else
void NewGameDialog(Player *play)
#endif
{
  GtkWidget *vbox, *vbox2, *hbox, *label, *entry, *notebook;
  GtkWidget *frame, *button, *dialog, *defbutton;
  GtkAccelGroup *accel_group;
  guint AccelKey;

#ifdef NETWORKING
  GtkWidget *clist, *scrollwin, *table, *hbbox;
  gchar *server_titles[5], *ServerEntry, *text;
  gboolean UpdateMeta = FALSE;
  SetCurlCallback(MetaConn, &global_data, glib_timeout, glib_socket);

  /* Column titles of metaserver information */
  server_titles[0] = _("Server");
  server_titles[1] = _("Port");
  server_titles[2] = _("Version");
  server_titles[3] = _("Players");
  server_titles[4] = _("Comment");

  stgam.MetaConn = MetaConn;
  stgam.NewMetaList = NULL;
  stgam.sockstat = sockstat;

#endif /* NETWORKING */

  stgam.play = play;
  stgam.dialog = dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
                     GTK_SIGNAL_FUNC(CloseNewGameDia), NULL);

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

  vbox = gtk_vbox_new(FALSE, 7);
  hbox = gtk_hbox_new(FALSE, 7);

  label = gtk_label_new("");

  AccelKey = gtk_label_parse_uline(GTK_LABEL(label),
                                   /* Prompt for player's name in 'New
                                    * Game' dialog */
                                   _("Hey dude, what's your _name?"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  entry = stgam.name = gtk_entry_new();
  gtk_widget_add_accelerator(entry, "grab-focus", accel_group, AccelKey,
                             GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
  gtk_entry_set_text(GTK_ENTRY(entry), GetPlayerName(stgam.play));
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  notebook = stgam.notebook = gtk_notebook_new();

#ifdef NETWORKING
  frame = gtk_frame_new(_("Server"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 4);
  vbox2 = gtk_vbox_new(FALSE, 7);
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
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(ConnectToServer), NULL);
  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  defbutton = button;
  
  label = gtk_label_new(_("Server"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);
#endif /* NETWORKING */

  /* Title of 'New Game' dialog notebook tab for single-player mode */
  frame = gtk_frame_new(_("Single player"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 4);
  vbox2 = gtk_vbox_new(FALSE, 7);
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

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(StartSinglePlayer), NULL);
  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);
  label = gtk_label_new(_("Single player"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

#ifdef NETWORKING
  /* Title of Metaserver frame in New Game dialog */
  frame = gtk_frame_new(_("Metaserver"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 4);

  vbox2 = gtk_vbox_new(FALSE, 7);
  gtk_container_set_border_width(GTK_CONTAINER(vbox2), 4);

  clist = stgam.metaserv =
      gtk_scrolled_clist_new_with_titles(5, server_titles, &scrollwin);
  gtk_clist_column_titles_passive(GTK_CLIST(clist));
  gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_SINGLE);
  gtk_clist_set_column_width(GTK_CLIST(clist), 0, 130);
  gtk_clist_set_column_width(GTK_CLIST(clist), 1, 35);

  gtk_box_pack_start(GTK_BOX(vbox2), scrollwin, TRUE, TRUE, 0);

  hbbox = my_hbbox_new();

  /* Button to update metaserver information */
  button = NewStockButton(GTK_STOCK_REFRESH, accel_group);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(UpdateMetaServerList), NULL);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  button = gtk_button_new_with_label("");
  SetAccelerator(button, _("_Connect"), button, "clicked", accel_group, TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(MetaServerConnect), NULL);
  gtk_widget_set_sensitive(button, FALSE);
  gtk_signal_connect(GTK_OBJECT(clist), "select_row",
                     GTK_SIGNAL_FUNC(metalist_row_select), button);
  gtk_signal_connect(GTK_OBJECT(clist), "unselect_row",
                     GTK_SIGNAL_FUNC(metalist_row_unselect), button);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

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
  gtk_notebook_set_page(GTK_NOTEBOOK(notebook), NewGameType);
#ifdef NETWORKING
  gtk_widget_grab_default(defbutton);
#endif
}

#ifdef NETWORKING
static void OKSocksAuth(GtkWidget *widget, GtkWidget *window)
{
  gtk_object_set_data(GTK_OBJECT(window), "authok", GINT_TO_POINTER(TRUE));
  gtk_widget_destroy(window);
}

static void DestroySocksAuth(GtkWidget *window, gpointer data)
{
  GtkWidget *userentry, *passwdentry;
  gchar *username = NULL, *password = NULL;
  gpointer authok, meta;
  NetworkBuffer *netbuf;

  authok = gtk_object_get_data(GTK_OBJECT(window), "authok");
  meta = gtk_object_get_data(GTK_OBJECT(window), "meta");
  userentry =
      (GtkWidget *)gtk_object_get_data(GTK_OBJECT(window), "username");
  passwdentry =
      (GtkWidget *)gtk_object_get_data(GTK_OBJECT(window), "password");
  netbuf =
      (NetworkBuffer *)gtk_object_get_data(GTK_OBJECT(window), "netbuf");

  g_assert(userentry && passwdentry && netbuf);

  if (authok) {
    username = gtk_editable_get_chars(GTK_EDITABLE(userentry), 0, -1);
    password = gtk_editable_get_chars(GTK_EDITABLE(passwdentry), 0, -1);
  }

  SendSocks5UserPasswd(netbuf, username, password);
  g_free(username);
  g_free(password);
}

static void RealSocksAuthDialog(NetworkBuffer *netbuf, gboolean meta,
                                gpointer data)
{
  GtkWidget *window, *button, *hsep, *vbox, *label, *entry, *table, *hbbox;
  GtkAccelGroup *accel_group;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

  gtk_signal_connect(GTK_OBJECT(window), "destroy",
                     GTK_SIGNAL_FUNC(DestroySocksAuth), NULL);
  gtk_object_set_data(GTK_OBJECT(window), "netbuf", (gpointer)netbuf);
  gtk_object_set_data(GTK_OBJECT(window), "meta", GINT_TO_POINTER(meta));

  /* Title of dialog for authenticating with a SOCKS server */
  gtk_window_set_title(GTK_WINDOW(window),
                       _("SOCKS Authentication Required"));
  my_set_dialog_position(GTK_WINDOW(window));

  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(window),
                               GTK_WINDOW(MainWindow));
  gtk_container_set_border_width(GTK_CONTAINER(window), 7);

  vbox = gtk_vbox_new(FALSE, 7);

  table = gtk_table_new(2, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 10);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);

  label = gtk_label_new("User name:");
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

  entry = gtk_entry_new();
  gtk_object_set_data(GTK_OBJECT(window), "username", (gpointer)entry);
  gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 0, 1);

  label = gtk_label_new("Password:");
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

  entry = gtk_entry_new();
  gtk_object_set_data(GTK_OBJECT(window), "password", (gpointer)entry);

#ifdef HAVE_FIXED_GTK
  /* GTK+ versions earlier than 1.2.10 do bad things with this */
  gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
#endif

  gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 1, 2);

  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();

  button = NewStockButton(GTK_STOCK_OK, accel_group);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(OKSocksAuth), (gpointer)window);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  button = NewStockButton(GTK_STOCK_CANCEL, accel_group);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            GTK_OBJECT(window));
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  gtk_box_pack_start(GTK_BOX(vbox), hbbox, TRUE, TRUE, 0);

  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show_all(window);
}

void SocksAuthDialog(NetworkBuffer *netbuf, gpointer data)
{
  RealSocksAuthDialog(netbuf, FALSE, data);
}

#endif /* NETWORKING */
