/************************************************************************
 * gtk_client.c   dopewars client using the GTK+ toolkit                *
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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "dopewars.h"
#include "gtk_client.h"
#include "message.h"
#include "nls.h"
#include "serverside.h"
#include "tstring.h"
#include "util.h"
#include "gtkport/gtkport.h"
#include "dopewars-pill.xpm"
#include "optdialog.h"
#include "newgamedia.h"

#define BT_BUY  (GINT_TO_POINTER(1))
#define BT_SELL (GINT_TO_POINTER(2))
#define BT_DROP (GINT_TO_POINTER(3))

#define ET_SPY    0
#define ET_TIPOFF 1

struct InventoryWidgets {
  GtkWidget *HereList, *CarriedList;
  GtkWidget *HereFrame, *CarriedFrame;
  GtkWidget *BuyButton, *SellButton, *DropButton;
  GtkWidget *vbbox;
};

struct StatusWidgets {
  GtkWidget *Location, *Date, *SpaceName, *SpaceValue, *CashName;
  GtkWidget *CashValue, *DebtName, *DebtValue, *BankName, *BankValue;
  GtkWidget *GunsName, *GunsValue, *BitchesName, *BitchesValue;
  GtkWidget *HealthName, *HealthValue;
};

struct ClientDataStruct {
  GtkWidget *window, *messages;
  Player *Play;
  GtkItemFactory *Menu;
  struct StatusWidgets Status;
  struct InventoryWidgets Drug, Gun, InvenDrug, InvenGun;
  GtkWidget *JetButton, *vbox, *PlayerList, *TalkList;
  guint JetAccel;
};

GtkWidget *MainWindow = NULL;

static struct ClientDataStruct ClientData;
static gboolean InGame = FALSE;

static GtkWidget *FightDialog = NULL, *SpyReportsDialog;
static gboolean IsShowingPlayerList = FALSE, IsShowingTalkList = FALSE;
static gboolean IsShowingInventory = FALSE, IsShowingGunShop = FALSE;
static gboolean IsShowingDealDrugs = FALSE;

static void display_intro(GtkWidget *widget, gpointer data);
static void QuitGame(GtkWidget *widget, gpointer data);
static void DestroyGtk(GtkWidget *widget, gpointer data);
static void NewGame(GtkWidget *widget, gpointer data);
static void AbandonGame(GtkWidget *widget, gpointer data);
static void ListScores(GtkWidget *widget, gpointer data);
static void ListInventory(GtkWidget *widget, gpointer data);
static void EndGame(void);
static void Jet(GtkWidget *parent);
static void UpdateMenus(void);

#ifdef NETWORKING
static void GetClientMessage(gpointer data, gint socket,
                             GdkInputCondition condition);
void SocketStatus(NetworkBuffer *NetBuf, gboolean Read, gboolean Write,
                  gboolean CallNow);
#endif /* NETWORKING */

static void HandleClientMessage(char *buf, Player *Play);
static void PrepareHighScoreDialog(void);
static void AddScoreToDialog(char *Data);
static void CompleteHighScoreDialog(gboolean AtEnd);
static void PrintMessage(char *Data, char *tagname);
static void DisplayFightMessage(char *Data);
static GtkWidget *CreateStatusWidgets(struct StatusWidgets *Status);
static void DisplayStats(Player *Play, struct StatusWidgets *Status);
static void UpdateStatus(Player *Play);
static void SetJetButtonTitle(GtkAccelGroup *accel_group);
static void UpdateInventory(struct InventoryWidgets *Inven,
                            Inventory *Objects, int NumObjects,
                            gboolean AreDrugs);
static void JetButtonPressed(GtkWidget *widget, gpointer data);
static void DealDrugs(GtkWidget *widget, gpointer data);
static void DealGuns(GtkWidget *widget, gpointer data);
static void QuestionDialog(char *Data, Player *From);
static void TransferDialog(gboolean Debt);
static void ListPlayers(GtkWidget *widget, gpointer data);
static void TalkToAll(GtkWidget *widget, gpointer data);
static void TalkToPlayers(GtkWidget *widget, gpointer data);
static void TalkDialog(gboolean TalkToAll);
static GtkWidget *CreatePlayerList(void);
static void UpdatePlayerList(GtkWidget *clist, gboolean IncludeSelf);
static void TipOff(GtkWidget *widget, gpointer data);
static void SpyOnPlayer(GtkWidget *widget, gpointer data);
static void ErrandDialog(gint ErrandType);
static void SackBitch(GtkWidget *widget, gpointer data);
static void DestroyShowing(GtkWidget *widget, gpointer data);
static void SetShowing(GtkWidget *window, gboolean *showing);
static gint DisallowDelete(GtkWidget *widget, GdkEvent * event,
                           gpointer data);
static void GunShopDialog(void);
static void NewNameDialog(void);
static void UpdatePlayerLists(void);
static void CreateInventory(GtkWidget *hbox, gchar *Objects,
                            GtkAccelGroup *accel_group,
                            gboolean CreateButtons, gboolean CreateHere,
                            struct InventoryWidgets *widgets,
                            GtkSignalFunc CallBack);
static void GetSpyReports(GtkWidget *widget, gpointer data);
static void DisplaySpyReports(Player *Play);

static GtkItemFactoryEntry menu_items[] = {
  /* The names of the the menus and their items in the GTK+ client */
  {N_("/_Game"), NULL, NULL, 0, "<Branch>"},
  {N_("/Game/_New..."), "<control>N", NewGame, 0, NULL},
  {N_("/Game/_Abandon..."), "<control>A", AbandonGame, 0, NULL},
  {N_("/Game/_Options..."), "<control>O", OptDialog, 0, NULL},
  {N_("/Game/_Quit..."), "<control>Q", QuitGame, 0, NULL},
  {N_("/_Talk"), NULL, NULL, 0, "<Branch>"},
  {N_("/Talk/To _All..."), NULL, TalkToAll, 0, NULL},
  {N_("/Talk/To _Player..."), NULL, TalkToPlayers, 0, NULL},
  {N_("/_List"), NULL, NULL, 0, "<Branch>"},
  {N_("/List/_Players..."), NULL, ListPlayers, 0, NULL},
  {N_("/List/_Scores..."), NULL, ListScores, 0, NULL},
  {N_("/List/_Inventory..."), NULL, ListInventory, 0, NULL},
  {N_("/_Errands"), NULL, NULL, 0, "<Branch>"},
  {N_("/Errands/_Spy..."), NULL, SpyOnPlayer, 0, NULL},
  {N_("/Errands/_Tipoff..."), NULL, TipOff, 0, NULL},
  /* N.B. "Sack Bitch" has to be recreated (and thus translated) at the
   * start of each game, below, so is not marked for gettext here */
  {"/Errands/S_ack Bitch...", NULL, SackBitch, 0, NULL},
  {N_("/Errands/_Get spy reports..."), NULL, GetSpyReports, 0, NULL},
  {N_("/_Help"), NULL, NULL, 0, "<LastBranch>"},
  {N_("/Help/_About..."), "F1", display_intro, 0, NULL}
};

static gchar *MenuTranslate(const gchar *path, gpointer func_data)
{
  /* Translate menu items, using gettext */
  return _(path);
}

static void LogMessage(const gchar *log_domain, GLogLevelFlags log_level,
                       const gchar *message, gpointer user_data)
{
  GtkMessageBox(MainWindow, message,
                /* Titles of the message boxes for warnings and errors */
                log_level & G_LOG_LEVEL_WARNING ? _("Warning") :
                log_level & G_LOG_LEVEL_CRITICAL ? _("Error") :
                _("Message"), GTK_MESSAGE_INFO,
                MB_OK | (gtk_main_level() > 0 ? MB_IMMRETURN : 0));
}

/*
 * Creates an hbutton_box widget, and sets a sensible spacing.
 * N.B. Should use gtk_hbutton_box_set_spacing_default() instead, but
 * this doesn't seem to actually work with GTK+2...
 */
GtkWidget *my_hbbox_new(void)
{
  GtkWidget *hbbox = gtk_hbutton_box_new();

#ifdef HAVE_GLIB2
  gtk_box_set_spacing(GTK_BOX(hbbox), 8);
#endif
  return hbbox;
}

void QuitGame(GtkWidget *widget, gpointer data)
{
  if (!InGame || GtkMessageBox(ClientData.window,
                               /* Prompt in 'quit game' dialog */
                               _("Abandon current game?"),
                               /* Title of 'quit game' dialog */
                               _("Quit Game"), GTK_MESSAGE_QUESTION,
                               MB_YESNO) == IDYES) {
    gtk_main_quit();
  }
}

void DestroyGtk(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

gint MainDelete(GtkWidget *widget, GdkEvent * event, gpointer data)
{
  return (InGame
          && GtkMessageBox(ClientData.window, _("Abandon current game?"),
                           _("Quit Game"), GTK_MESSAGE_QUESTION,
                           MB_YESNO) == IDNO);
}


void NewGame(GtkWidget *widget, gpointer data)
{
  if (InGame) {
    if (GtkMessageBox(ClientData.window, _("Abandon current game?"),
                      /* Title of 'stop game to start a new game' dialog */
                      _("Start new game"), GTK_MESSAGE_QUESTION,
                      MB_YESNO) == IDYES)
      EndGame();
    else
      return;
  }

  /* Save the configuration, so we can restore those elements that get
   * overwritten when we connect to a dopewars server */
  BackupConfig();

#ifdef NETWORKING
  NewGameDialog(ClientData.Play, SocketStatus);
#else
  NewGameDialog(ClientData.Play);
#endif
}

void AbandonGame(GtkWidget *widget, gpointer data)
{
  if (InGame && GtkMessageBox(ClientData.window, _("Abandon current game?"),
                              /* Title of 'abandon game' dialog */
                              _("Abandon game"), GTK_MESSAGE_QUESTION,
                              MB_YESNO) == IDYES) {
    EndGame();
  }
}

void ListScores(GtkWidget *widget, gpointer data)
{
  SendClientMessage(ClientData.Play, C_NONE, C_REQUESTSCORE, NULL, NULL);
}

void ListInventory(GtkWidget *widget, gpointer data)
{
  GtkWidget *window, *button, *hsep, *vbox, *hbox, *hbbox;
  GtkAccelGroup *accel_group;

  if (IsShowingInventory)
    return;
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 550, 120);
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

  /* Title of inventory window */
  gtk_window_set_title(GTK_WINDOW(window), _("Inventory"));

  SetShowing(window, &IsShowingInventory);

  gtk_window_set_transient_for(GTK_WINDOW(window),
                               GTK_WINDOW(ClientData.window));
  gtk_container_set_border_width(GTK_CONTAINER(window), 7);

  vbox = gtk_vbox_new(FALSE, 7);

  hbox = gtk_hbox_new(FALSE, 7);
  CreateInventory(hbox, Names.Drugs, accel_group, FALSE, FALSE,
                  &ClientData.InvenDrug, NULL);
  CreateInventory(hbox, Names.Guns, accel_group, FALSE, FALSE,
                  &ClientData.InvenGun, NULL);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();
  button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            (gpointer)window);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);
  gtk_box_pack_start(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(window), vbox);

  UpdateInventory(&ClientData.InvenDrug, ClientData.Play->Drugs, NumDrug,
                  TRUE);
  UpdateInventory(&ClientData.InvenGun, ClientData.Play->Guns, NumGun,
                  FALSE);

  gtk_widget_show_all(window);
}

#ifdef NETWORKING
void GetClientMessage(gpointer data, gint socket,
                      GdkInputCondition condition)
{
  gchar *pt;
  NetworkBuffer *NetBuf;
  gboolean DoneOK, datawaiting;
  NBStatus status, oldstatus;
  NBSocksStatus oldsocks;

  NetBuf = &ClientData.Play->NetBuf;

  oldstatus = NetBuf->status;
  oldsocks = NetBuf->sockstat;

  datawaiting =
      PlayerHandleNetwork(ClientData.Play, condition & GDK_INPUT_READ,
                          condition & GDK_INPUT_WRITE, &DoneOK);

  status = NetBuf->status;

  /* Handle pre-game stuff */
  if (status != NBS_CONNECTED) {
    /* The start game dialog isn't visible once we're connected... */
    DisplayConnectStatus(FALSE, oldstatus, oldsocks);
  }
  if (oldstatus != NBS_CONNECTED && (status == NBS_CONNECTED || !DoneOK)) {
    FinishServerConnect(DoneOK);
  }

  if (status == NBS_CONNECTED && datawaiting) {
    while ((pt = GetWaitingPlayerMessage(ClientData.Play)) != NULL) {
      HandleClientMessage(pt, ClientData.Play);
      g_free(pt);
    }
  }
  if (!DoneOK) {
    if (status == NBS_CONNECTED) {
      /* The network connection to the server was dropped unexpectedly */
      g_warning(_("Connection to server lost - switching to "
                  "single player mode"));
      SwitchToSinglePlayer(ClientData.Play);
      UpdatePlayerLists();
      UpdateMenus();
    } else {
      ShutdownNetworkBuffer(&ClientData.Play->NetBuf);
    }
  }
}

void SocketStatus(NetworkBuffer *NetBuf, gboolean Read, gboolean Write,
                  gboolean CallNow)
{
  if (NetBuf->InputTag)
    gdk_input_remove(NetBuf->InputTag);
  NetBuf->InputTag = 0;
  if (Read || Write) {
    NetBuf->InputTag = gdk_input_add(NetBuf->fd,
                                     (Read ? GDK_INPUT_READ : 0) |
                                     (Write ? GDK_INPUT_WRITE : 0),
                                     GetClientMessage,
                                     NetBuf->CallBackData);
  }
  if (CallNow)
    GetClientMessage(NetBuf->CallBackData, NetBuf->fd, 0);
}
#endif /* NETWORKING */

void HandleClientMessage(char *pt, Player *Play)
{
  char *Data;
  DispMode DisplayMode;
  AICode AI;
  MsgCode Code;
  Player *From, *tmp;
  gchar *text;
  gboolean Handled;
  GtkWidget *MenuItem;
  GSList *list;

  if (ProcessMessage(pt, Play, &From, &AI, &Code,
                     &Data, FirstClient) == -1) {
    return;
  }

  Handled =
      HandleGenericClientMessage(From, AI, Code, Play, Data, &DisplayMode);
  switch (Code) {
  case C_STARTHISCORE:
    PrepareHighScoreDialog();
    break;
  case C_HISCORE:
    AddScoreToDialog(Data);
    break;
  case C_ENDHISCORE:
    CompleteHighScoreDialog((strcmp(Data, "end") == 0));
    break;
  case C_PRINTMESSAGE:
    PrintMessage(Data, NULL);
    break;
  case C_FIGHTPRINT:
    DisplayFightMessage(Data);
    break;
  case C_PUSH:
    /* The server admin has asked us to leave - so warn the user, and do
     * so */
    g_warning(_("You have been pushed from the server.\n"
                "Switching to single player mode."));
    SwitchToSinglePlayer(Play);
    UpdatePlayerLists();
    UpdateMenus();
    break;
  case C_QUIT:
    /* The server has sent us notice that it is shutting down */
    g_warning(_("The server has terminated.\n"
                "Switching to single player mode."));
    SwitchToSinglePlayer(Play);
    UpdatePlayerLists();
    UpdateMenus();
    break;
  case C_NEWNAME:
    NewNameDialog();
    break;
  case C_BANK:
    TransferDialog(FALSE);
    break;
  case C_LOANSHARK:
    TransferDialog(TRUE);
    break;
  case C_GUNSHOP:
    GunShopDialog();
    break;
  case C_MSG:
    text = g_strdup_printf("%s: %s", GetPlayerName(From), Data);
    PrintMessage(text, "talk");
    g_free(text);
    break;
  case C_MSGTO:
    text = g_strdup_printf("%s->%s: %s", GetPlayerName(From),
                           GetPlayerName(Play), Data);
    PrintMessage(text, "page");
    g_free(text);
    break;
  case C_JOIN:
    text = g_strdup_printf(_("%s joins the game!"), Data);
    PrintMessage(text, "join");
    g_free(text);
    UpdatePlayerLists();
    UpdateMenus();
    break;
  case C_LEAVE:
    if (From != &Noone) {
      text = g_strdup_printf(_("%s has left the game."), Data);
      PrintMessage(text, "leave");
      g_free(text);
      UpdatePlayerLists();
      UpdateMenus();
    }
    break;
  case C_QUESTION:
    QuestionDialog(Data, From == &Noone ? NULL : From);
    break;
  case C_SUBWAYFLASH:
    DisplayFightMessage(NULL);
    for (list = FirstClient; list; list = g_slist_next(list)) {
      tmp = (Player *)list->data;
      tmp->Flags &= ~FIGHTING;
    }
    /* Message displayed when the player "jets" to a new location */
    text = dpg_strdup_printf(_("Jetting to %tde"),
                             Location[(int)Play->IsAt].Name);
    PrintMessage(text, "jet");
    g_free(text);
    break;
  case C_ENDLIST:
    MenuItem = gtk_item_factory_get_widget(ClientData.Menu,
                                           "<main>/Errands/Sack Bitch...");

    /* Text for the Errands/Sack Bitch menu item */
    text = dpg_strdup_printf(_("%/Sack Bitch menu item/S_ack %Tde..."),
                             Names.Bitch);
    SetAccelerator(MenuItem, text, NULL, NULL, NULL, FALSE);
    g_free(text);

    MenuItem = gtk_item_factory_get_widget(ClientData.Menu,
                                           "<main>/Errands/Spy...");

    /* Text to update the Errands/Spy menu item with the price for spying */
    text = dpg_strdup_printf(_("_Spy (%P)"), Prices.Spy);
    SetAccelerator(MenuItem, text, NULL, NULL, NULL, FALSE);
    g_free(text);

    /* Text to update the Errands/Tipoff menu item with the price for a
     * tipoff */
    text = dpg_strdup_printf(_("_Tipoff (%P)"), Prices.Tipoff);
    MenuItem = gtk_item_factory_get_widget(ClientData.Menu,
                                           "<main>/Errands/Tipoff...");
    SetAccelerator(MenuItem, text, NULL, NULL, NULL, FALSE);
    g_free(text);
    if (FirstClient->next)
      ListPlayers(NULL, NULL);
    UpdateMenus();
    break;
  case C_UPDATE:
    if (From == &Noone) {
      ReceivePlayerData(Play, Data, Play);
      UpdateStatus(Play);
    } else {
      ReceivePlayerData(Play, Data, From);
      DisplaySpyReports(From);
    }
    break;
  case C_DRUGHERE:
    UpdateInventory(&ClientData.Drug, Play->Drugs, NumDrug, TRUE);
    gtk_clist_sort(GTK_CLIST(ClientData.Drug.HereList));
    if (IsShowingInventory) {
      UpdateInventory(&ClientData.InvenDrug, Play->Drugs, NumDrug, TRUE);
    }
    break;
  default:
    if (!Handled) {
      g_print("Unknown network message received: %s^%c^%s^%s",
              GetPlayerName(From), Code, GetPlayerName(Play), Data);
    }
    break;
  }
}

struct HiScoreDiaStruct {
  GtkWidget *dialog, *table, *vbox;
};
static struct HiScoreDiaStruct HiScoreDialog = { NULL, NULL, NULL };

/* 
 * Creates an empty dialog to display high scores.
 */
void PrepareHighScoreDialog(void)
{
  GtkWidget *dialog, *vbox, *hsep, *table;

  /* Make sure the server doesn't fool us into creating multiple dialogs */
  if (HiScoreDialog.dialog)
    return;

  HiScoreDialog.dialog = dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* Title of the GTK+ high score dialog */
  gtk_window_set_title(GTK_WINDOW(dialog), _("High Scores"));

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               GTK_WINDOW(ClientData.window));

  HiScoreDialog.vbox = vbox = gtk_vbox_new(FALSE, 7);
  HiScoreDialog.table = table = gtk_table_new(NUMHISCORE, 4, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 30);

  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);
  gtk_widget_show_all(dialog);
}

/* 
 * Adds a single high score (coded in "Data", which is the information
 * received in the relevant network message) to the dialog created by
 * PrepareHighScoreDialog(), above.
 */
void AddScoreToDialog(char *Data)
{
  GtkWidget *label;
  char *cp;
  gchar **spl1, **spl2;
  int index, slen;
  gboolean bold;
  GtkStyle *style = NULL;

  if (!HiScoreDialog.dialog)
    return;

  cp = Data;
  index = GetNextInt(&cp, 0);
  if (!cp || strlen(cp) < 3)
    return;

  bold = (*cp == 'B');          /* Is this score "our" score? (Currently
                                 * ignored) */

  /* Step past the 'bold' character, and the initial '>' (if present) */
  cp += 2;
  g_strchug(cp);

  /* Get the first word - the score */
#ifdef HAVE_GLIB2
  spl1 = g_strsplit(cp, " ", 2);
#else
  spl1 = g_strsplit(cp, " ", 1);
#endif
  if (!spl1 || !spl1[0] || !spl1[1]) {
    /* Error - the high score from the server is invalid */
    g_warning(_("Corrupt high score!"));
    g_strfreev(spl1);
    return;
  }
  label = gtk_label_new(spl1[0]);
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(HiScoreDialog.table), label,
                            0, 1, index, index + 1);
  if (bold) {
    GdkColor color;

    color.red = 0;
    color.green = 0;
    color.blue = 0xDDDD;
    style = gtk_style_new();
    style->fg[GTK_STATE_NORMAL] = color;
    gtk_widget_set_style(label, style);
  }
  gtk_widget_show(label);

  /* Remove any leading whitespace from the remainder, since g_strsplit
   * will split at every space character, not at a run of them */
  g_strchug(spl1[1]);

  /* Get the second word - the date */
#ifdef HAVE_GLIB2
  spl2 = g_strsplit(spl1[1], " ", 2);
#else
  spl2 = g_strsplit(spl1[1], " ", 1);
#endif
  if (!spl2 || !spl2[0] || !spl2[1]) {
    g_warning(_("Corrupt high score!"));
    g_strfreev(spl2);
    return;
  }
  label = gtk_label_new(spl2[0]);
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(HiScoreDialog.table), label,
                            1, 2, index, index + 1);
  if (bold)
    gtk_widget_set_style(label, style);
  gtk_widget_show(label);

  /* The remainder is the name, terminated with (R.I.P.) if the player
   * died, and '<' for the 'current' score */
  g_strchug(spl2[1]);

  /* Remove '<' suffix if present */
  slen = strlen(spl2[1]);
  if (slen >= 1 && spl2[1][slen - 1] == '<') {
    spl2[1][slen - 1] = '\0';
  }
  slen--;

  /* Check for (R.I.P.) suffix, and add it to the 4th column if found */
  if (slen > 8 && spl2[1][slen - 1] == ')' && spl2[1][slen - 8] == '(') {
    label = gtk_label_new(&spl2[1][slen - 8]);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(HiScoreDialog.table), label,
                              3, 4, index, index + 1);
    if (bold)
      gtk_widget_set_style(label, style);
    gtk_widget_show(label);
    spl2[1][slen - 8] = '\0';   /* Remove suffix from the player name */
  }

  /* Finally, add in what's left of the player name */
  g_strchomp(spl2[1]);
  label = gtk_label_new(spl2[1]);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(HiScoreDialog.table), label,
                            2, 3, index, index + 1);
  if (bold)
    gtk_widget_set_style(label, style);
  gtk_widget_show(label);

  g_strfreev(spl1);
  g_strfreev(spl2);
}

/* 
 * If the high scores are being displayed at the end of the game,
 * this function is used to end the game when the high score dialog's
 * "OK" button is pressed.
 */
static void EndHighScore(GtkWidget *widget)
{
  EndGame();
}

/* 
 * Called when all high scores have been received. Finishes off the
 * high score dialog by adding an "OK" button. If the game has ended,
 * then "AtEnd" is TRUE, and clicking this button will end the game.
 */
void CompleteHighScoreDialog(gboolean AtEnd)
{
  GtkWidget *button, *dialog, *hbbox;

  dialog = HiScoreDialog.dialog;

  if (!HiScoreDialog.dialog)
    return;

  hbbox = my_hbbox_new();
  button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            (gpointer)dialog);
  if (AtEnd) {
    InGame = FALSE;
    gtk_signal_connect_object(GTK_OBJECT(dialog), "destroy",
                              GTK_SIGNAL_FUNC(EndHighScore), NULL);
  }
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);
  gtk_box_pack_start(GTK_BOX(HiScoreDialog.vbox), hbbox, FALSE, FALSE, 0);

  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(button);
  gtk_widget_show_all(hbbox);

  /* OK, we're done - allow the creation of new high score dialogs */
  HiScoreDialog.dialog = NULL;
}

/* 
 * Prints an information message in the display area of the GTK+ client.
 * This area is used for displaying drug busts, messages from other
 * players, etc. The message is passed in as the string "text".
 */
void PrintMessage(char *text, char *tagname)
{
  GtkTextView *messages = GTK_TEXT_VIEW(ClientData.messages);

  g_strdelimit(text, "^", '\n');
  TextViewAppend(messages, text, tagname, FALSE);
  TextViewAppend(messages, "\n", NULL, TRUE);
}

/* 
 * Called when one of the action buttons in the Fight dialog is clicked.
 * "data" specifies which button (Deal Drugs/Run/Fight/Stand) was pressed.
 */
static void FightCallback(GtkWidget *widget, gpointer data)
{
  gint Answer;
  Player *Play;
  gchar text[4];
  GtkWidget *window;
  gpointer CanRunHere = NULL;

  window = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
  if (window)
    CanRunHere = gtk_object_get_data(GTK_OBJECT(window), "CanRunHere");

  Answer = GPOINTER_TO_INT(data);
  Play = ClientData.Play;
  switch (Answer) {
  case 'D':
    gtk_widget_hide(FightDialog);
    break;
  case 'R':
    if (CanRunHere) {
      SendClientMessage(Play, C_NONE, C_FIGHTACT, NULL, "R");
    } else {
      Jet(FightDialog);
    }
    break;
  case 'F':
  case 'S':
    text[0] = Answer;
    text[1] = '\0';
    SendClientMessage(Play, C_NONE, C_FIGHTACT, NULL, text);
    break;
  }
}

/* 
 * Adds an action button to the hbox at the base of the Fight dialog.
 * The button's caption is given by "Text", and the keyboard shortcut
 * (if any) is added to "accel_group". "Answer" gives the identifier
 * passed to FightCallback, above.
 */
static GtkWidget *AddFightButton(gchar *Text, GtkAccelGroup *accel_group,
                                 GtkBox *box, gint Answer)
{
  GtkWidget *button;

  button = gtk_button_new_with_label("");
  SetAccelerator(button, Text, button, "clicked", accel_group, FALSE);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(FightCallback),
                     GINT_TO_POINTER(Answer));
  gtk_box_pack_start(box, button, TRUE, TRUE, 0);
  return button;
}

/* Data used to keep track of the widgets giving the information about a
 * player/cop involved in a fight */
struct combatant {
  GtkWidget *name, *bitches, *healthprog, *healthlabel;
};

/* 
 * Creates an empty Fight dialog. Usually this only needs to be done once,
 * as when the user "closes" it, it is only hidden, ready to be reshown
 * later. Buttons for all actions are added here, and are hidden/shown
 * as necessary.
 */
static void CreateFightDialog(void)
{
  GtkWidget *dialog, *vbox, *button, *hbox, *hbbox, *hsep, *text, *table;
  GtkAccelGroup *accel_group;
  GArray *combatants;
  gchar *buf;

  FightDialog = dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 300);
  gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
                     GTK_SIGNAL_FUNC(DisallowDelete), NULL);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 240, 130);
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(dialog), accel_group);
  gtk_window_set_title(GTK_WINDOW(dialog), _("Fight"));
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);

  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               GTK_WINDOW(ClientData.window));

  vbox = gtk_vbox_new(FALSE, 7);

  table = gtk_table_new(2, 4, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 7);
  gtk_table_set_col_spacings(GTK_TABLE(table), 10);

  hsep = gtk_hseparator_new();
  gtk_table_attach_defaults(GTK_TABLE(table), hsep, 0, 4, 1, 2);
  gtk_widget_show_all(table);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
  gtk_object_set_data(GTK_OBJECT(dialog), "table", table);

  combatants = g_array_new(FALSE, TRUE, sizeof(struct combatant));
  g_array_set_size(combatants, 1);
  gtk_object_set_data(GTK_OBJECT(dialog), "combatants", combatants);

  text = gtk_scrolled_text_view_new(&hbox);
  gtk_widget_set_usize(text, 150, 120);

  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_object_set_data(GTK_OBJECT(dialog), "text", text);
  gtk_widget_show_all(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);
  gtk_widget_show(hsep);

  hbbox = my_hbbox_new();

  /* Button for closing the "Fight" dialog and going back to dealing drugs
   * (%Tde = "Drugs" by default) */
  buf = dpg_strdup_printf(_("_Deal %Tde"), Names.Drugs);
  button = AddFightButton(buf, accel_group, GTK_BOX(hbbox), 'D');
  gtk_object_set_data(GTK_OBJECT(dialog), "deal", button);
  g_free(buf);

  /* Button for shooting at other players in the "Fight" dialog, or for
   * popping up the "Fight" dialog from the main window */
  button = AddFightButton(_("_Fight"), accel_group, GTK_BOX(hbbox), 'F');
  gtk_object_set_data(GTK_OBJECT(dialog), "fight", button);

  /* Button to stand and take it in the "Fight" dialog */
  button = AddFightButton(_("_Stand"), accel_group, GTK_BOX(hbbox), 'S');
  gtk_object_set_data(GTK_OBJECT(dialog), "stand", button);

  /* Button to run from combat in the "Fight" dialog */
  button = AddFightButton(_("_Run"), accel_group, GTK_BOX(hbbox), 'R');
  gtk_object_set_data(GTK_OBJECT(dialog), "run", button);

  gtk_widget_show(hsep);
  gtk_box_pack_start(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);
  gtk_widget_show(hbbox);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);
  gtk_widget_show(dialog);
}

/* 
 * Updates the display of information for a player/cop in the Fight dialog.
 * If the player's name (DefendName) already exists, updates the display of
 * total health and number of bitches - otherwise, adds a new entry. If
 * DefendBitches is -1, then the player has left.
 */
static void UpdateCombatant(gchar *DefendName, int DefendBitches,
                            gchar *BitchName, int DefendHealth)
{
  guint i, RowIndex;
  gchar *name;
  struct combatant *compt;
  GArray *combatants;
  GtkWidget *table;
  gchar *BitchText, *HealthText;
  gfloat ProgPercent;

  combatants = (GArray *)gtk_object_get_data(GTK_OBJECT(FightDialog),
                                             "combatants");
  table = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog), "table"));
  if (!combatants)
    return;

  if (DefendName[0]) {
    compt = NULL;
    for (i = 1, RowIndex = 2; i < combatants->len; i++, RowIndex++) {
      compt = &g_array_index(combatants, struct combatant, i);

      if (!compt || !compt->name) {
        compt = NULL;
        continue;
      }
      gtk_label_get(GTK_LABEL(compt->name), &name);
      if (name && strcmp(name, DefendName) == 0)
        break;
      compt = NULL;
    }
    if (!compt) {
      i = combatants->len;
      g_array_set_size(combatants, i + 1);
      compt = &g_array_index(combatants, struct combatant, i);

      gtk_table_resize(GTK_TABLE(table), i + 2, 4);
      RowIndex = i + 1;
    }
  } else {
    compt = &g_array_index(combatants, struct combatant, 0);

    RowIndex = 0;
  }

  /* Display of number of bitches or deputies during combat
   * (%tde="bitches" or "deputies" (etc.) by default) */
  BitchText = dpg_strdup_printf(_("%/Combat: Bitches/%d %tde"),
                                DefendBitches, BitchName);

  /* Display of health during combat */
  if (DefendBitches == -1) {
    HealthText = g_strdup(_("(Left)"));
  } else if (DefendHealth == 0 && DefendBitches == 0) {
    HealthText = g_strdup(_("(Dead)"));
  } else {
    HealthText = g_strdup_printf(_("Health: %d"), DefendHealth);
  }

  ProgPercent = (gfloat)DefendHealth / 100.0;

  if (compt->name) {
    if (DefendName[0]) {
      gtk_label_set_text(GTK_LABEL(compt->name), DefendName);
    }
    if (DefendBitches >= 0) {
      gtk_label_set_text(GTK_LABEL(compt->bitches), BitchText);
    }
    gtk_label_set_text(GTK_LABEL(compt->healthlabel), HealthText);
    gtk_progress_bar_update(GTK_PROGRESS_BAR(compt->healthprog),
                            ProgPercent);
  } else {
    /* Display of the current player's name during combat */
    compt->name = gtk_label_new(DefendName[0] ? DefendName : _("You"));

    gtk_table_attach(GTK_TABLE(table), compt->name, 0, 1,
                     RowIndex, RowIndex + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    compt->bitches = gtk_label_new(DefendBitches >= 0 ? BitchText : "");
    gtk_table_attach(GTK_TABLE(table), compt->bitches, 1, 2,
                     RowIndex, RowIndex + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    compt->healthprog = gtk_progress_bar_new();
    gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(compt->healthprog),
                                     GTK_PROGRESS_LEFT_TO_RIGHT);
    gtk_progress_bar_update(GTK_PROGRESS_BAR(compt->healthprog),
                            ProgPercent);
    gtk_table_attach_defaults(GTK_TABLE(table), compt->healthprog, 2, 3,
                              RowIndex, RowIndex + 1);
    compt->healthlabel = gtk_label_new(HealthText);
    gtk_table_attach(GTK_TABLE(table), compt->healthlabel, 3, 4,
                     RowIndex, RowIndex + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    gtk_widget_show(compt->name);
    gtk_widget_show(compt->bitches);
    gtk_widget_show(compt->healthprog);
    gtk_widget_show(compt->healthlabel);
  }

  g_free(BitchText);
  g_free(HealthText);
}

/* 
 * Cleans up the list of all players/cops involved in a fight.
 */
static void FreeCombatants(void)
{
  GArray *combatants;

  combatants = (GArray *)gtk_object_get_data(GTK_OBJECT(FightDialog),
                                             "combatants");
  if (!combatants)
    return;

  g_array_free(combatants, TRUE);
}

/* 
 * Given the network message "Data" concerning some happening during
 * combat, extracts the relevant data and updates the Fight dialog,
 * creating and/or showing it if necessary.
 * If "Data" is NULL, then closes the dialog. If "Data" is a blank
 * string, then just shows the dialog, displaying no new messages.
 */
void DisplayFightMessage(char *Data)
{
  Player *Play;
  GtkAccelGroup *accel_group;
  GtkWidget *Deal, *Fight, *Stand, *Run;
  GtkTextView *textview;
  gchar *AttackName, *DefendName, *BitchName, *Message;
  FightPoint fp;
  int DefendHealth, DefendBitches, BitchesKilled, ArmPercent;
  gboolean CanRunHere, Loot, CanFire;

  if (!Data) {
    if (FightDialog) {
      FreeCombatants();
      gtk_widget_destroy(FightDialog);
      FightDialog = NULL;
    }
    return;
  }
  if (FightDialog) {
    if (!GTK_WIDGET_VISIBLE(FightDialog))
      gtk_widget_show(FightDialog);
  } else {
    CreateFightDialog();
  }
  if (!FightDialog || !Data[0])
    return;

  Deal = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog), "deal"));
  Fight = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog), "fight"));
  Stand = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog), "stand"));
  Run = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog), "run"));
  textview = GTK_TEXT_VIEW(gtk_object_get_data(GTK_OBJECT(FightDialog),
                                               "text"));

  Play = ClientData.Play;

  if (HaveAbility(Play, A_NEWFIGHT)) {
    ReceiveFightMessage(Data, &AttackName, &DefendName, &DefendHealth,
                        &DefendBitches, &BitchName, &BitchesKilled,
                        &ArmPercent, &fp, &CanRunHere, &Loot, &CanFire,
                        &Message);
    Play->Flags |= FIGHTING;
    switch (fp) {
    case F_HIT:
    case F_ARRIVED:
    case F_MISS:
      UpdateCombatant(DefendName, DefendBitches, BitchName, DefendHealth);
      break;
    case F_LEAVE:
      if (AttackName[0]) {
        UpdateCombatant(AttackName, -1, BitchName, 0);
      }
      break;
    case F_LASTLEAVE:
      Play->Flags &= ~FIGHTING;
      break;
    default:
    }
    accel_group = (GtkAccelGroup *)
        gtk_object_get_data(GTK_OBJECT(ClientData.window), "accel_group");
    SetJetButtonTitle(accel_group);
  } else {
    Message = Data;
    if (Play->Flags & FIGHTING)
      fp = F_MSG;
    else
      fp = F_LASTLEAVE;
    CanFire = (Play->Flags & CANSHOOT);
    CanRunHere = FALSE;
  }
  gtk_object_set_data(GTK_OBJECT(FightDialog), "CanRunHere",
                      GINT_TO_POINTER(CanRunHere));

  g_strdelimit(Message, "^", '\n');
  if (strlen(Message) > 0) {
    TextViewAppend(textview, Message, NULL, FALSE);
    TextViewAppend(textview, "\n", NULL, TRUE);
  }

  if (!CanRunHere || fp == F_LASTLEAVE)
    gtk_widget_show(Deal);
  else
    gtk_widget_hide(Deal);
  if (CanFire && TotalGunsCarried(Play) > 0)
    gtk_widget_show(Fight);
  else
    gtk_widget_hide(Fight);
  if (CanFire && TotalGunsCarried(Play) == 0)
    gtk_widget_show(Stand);
  else
    gtk_widget_hide(Stand);
  if (fp != F_LASTLEAVE)
    gtk_widget_show(Run);
  else
    gtk_widget_hide(Run);
}

/* 
 * Updates the display of pertinent data about player "Play" (location,
 * health, etc. in the status widgets given by "Status". This can point
 * to the widgets at the top of the main window, or those in a Spy
 * Reports dialog.
 */
void DisplayStats(Player *Play, struct StatusWidgets *Status)
{
  gchar *prstr;
  GString *text;

  text = g_string_new(NULL);

  gtk_label_set_text(GTK_LABEL(Status->Location),
                     Location[(int)Play->IsAt].Name);

  g_string_sprintf(text, "%s%02d%s", Names.Month, Play->Turn, Names.Year);
  gtk_label_set_text(GTK_LABEL(Status->Date), text->str);

  g_string_sprintf(text, "%d", Play->CoatSize);
  gtk_label_set_text(GTK_LABEL(Status->SpaceValue), text->str);

  prstr = FormatPrice(Play->Cash);
  gtk_label_set_text(GTK_LABEL(Status->CashValue), prstr);
  g_free(prstr);

  prstr = FormatPrice(Play->Bank);
  gtk_label_set_text(GTK_LABEL(Status->BankValue), prstr);
  g_free(prstr);

  prstr = FormatPrice(Play->Debt);
  gtk_label_set_text(GTK_LABEL(Status->DebtValue), prstr);
  g_free(prstr);

  /* Display of carried guns in GTK+ client status window (%Tde="Guns" by
   * default) */
  dpg_string_sprintf(text, _("%/GTK Stats: Guns/%Tde"), Names.Guns);
  gtk_label_set_text(GTK_LABEL(Status->GunsName), text->str);
  g_string_sprintf(text, "%d", TotalGunsCarried(Play));
  gtk_label_set_text(GTK_LABEL(Status->GunsValue), text->str);

  if (!WantAntique) {
    /* Display of number of bitches in GTK+ client status window
     * (%Tde="Bitches" by default) */
    dpg_string_sprintf(text, _("%/GTK Stats: Bitches/%Tde"),
                       Names.Bitches);
    gtk_label_set_text(GTK_LABEL(Status->BitchesName), text->str);
    g_string_sprintf(text, "%d", Play->Bitches.Carried);
    gtk_label_set_text(GTK_LABEL(Status->BitchesValue), text->str);
  } else {
    gtk_label_set_text(GTK_LABEL(Status->BitchesName), NULL);
    gtk_label_set_text(GTK_LABEL(Status->BitchesValue), NULL);
  }

  g_string_sprintf(text, "%d", Play->Health);
  gtk_label_set_text(GTK_LABEL(Status->HealthValue), text->str);

  g_string_free(text, TRUE);
}

/* 
 * Updates all of the player status in response to a message from the
 * server. This includes the main window display, the gun shop (if
 * displayed) and the inventory (if displayed).
 */
void UpdateStatus(Player *Play)
{
  GtkAccelGroup *accel_group;

  DisplayStats(Play, &ClientData.Status);
  UpdateInventory(&ClientData.Drug, ClientData.Play->Drugs, NumDrug, TRUE);
  gtk_clist_sort(GTK_CLIST(ClientData.Drug.HereList));
  accel_group = (GtkAccelGroup *)
      gtk_object_get_data(GTK_OBJECT(ClientData.window), "accel_group");
  SetJetButtonTitle(accel_group);
  if (IsShowingGunShop) {
    UpdateInventory(&ClientData.Gun, ClientData.Play->Guns, NumGun, FALSE);
  }
  if (IsShowingInventory) {
    UpdateInventory(&ClientData.InvenDrug, ClientData.Play->Drugs,
                    NumDrug, TRUE);
    UpdateInventory(&ClientData.InvenGun, ClientData.Play->Guns,
                    NumGun, FALSE);
  }
}

void UpdateInventory(struct InventoryWidgets *Inven,
                     Inventory *Objects, int NumObjects, gboolean AreDrugs)
{
  GtkWidget *herelist, *carrylist;
  Player *Play;
  gint i, row, selectrow[2];
  gpointer rowdata;
  price_t price;
  gchar *titles[2];
  gboolean CanBuy = FALSE, CanSell = FALSE, CanDrop = FALSE;
  GList *glist[2], *selection;
  GtkCList *clist[2];
  int numlist;

  Play = ClientData.Play;
  herelist = Inven->HereList;
  carrylist = Inven->CarriedList;

  if (herelist)
    numlist = 2;
  else
    numlist = 1;

  /* Make lists of the current selections */
  clist[0] = GTK_CLIST(carrylist);
  if (herelist)
    clist[1] = GTK_CLIST(herelist);
  else
    clist[1] = NULL;

  for (i = 0; i < numlist; i++) {
    glist[i] = NULL;
    selectrow[i] = -1;
    for (selection = clist[i]->selection; selection;
         selection = g_list_next(selection)) {
      row = GPOINTER_TO_INT(selection->data);
      rowdata = gtk_clist_get_row_data(clist[i], row);
      glist[i] = g_list_append(glist[i], rowdata);
    }
  }

  gtk_clist_freeze(GTK_CLIST(carrylist));
  gtk_clist_clear(GTK_CLIST(carrylist));

  if (herelist) {
    gtk_clist_freeze(GTK_CLIST(herelist));
    gtk_clist_clear(GTK_CLIST(herelist));
  }

  for (i = 0; i < NumObjects; i++) {
    if (AreDrugs) {
      titles[0] = Drug[i].Name;
      price = Objects[i].Price;
    } else {
      titles[0] = Gun[i].Name;
      price = Gun[i].Price;
    }

    if (herelist && price > 0) {
      CanBuy = TRUE;
      titles[1] = FormatPrice(price);
      row = gtk_clist_append(GTK_CLIST(herelist), titles);
      g_free(titles[1]);
      gtk_clist_set_row_data(GTK_CLIST(herelist), row, GINT_TO_POINTER(i));
      if (g_list_find(glist[1], GINT_TO_POINTER(i))) {
        selectrow[1] = row;
        gtk_clist_select_row(GTK_CLIST(herelist), row, 0);
      }
    }

    if (Objects[i].Carried > 0) {
      if (price > 0)
        CanSell = TRUE;
      else
        CanDrop = TRUE;
      if (HaveAbility(ClientData.Play, A_DRUGVALUE) && AreDrugs) {
        titles[1] = dpg_strdup_printf("%d @ %P", Objects[i].Carried,
                                      Objects[i].TotalValue /
                                      Objects[i].Carried);
      } else {
        titles[1] = g_strdup_printf("%d", Objects[i].Carried);
      }
      row = gtk_clist_append(GTK_CLIST(carrylist), titles);
      g_free(titles[1]);
      gtk_clist_set_row_data(GTK_CLIST(carrylist), row,
                             GINT_TO_POINTER(i));
      if (g_list_find(glist[0], GINT_TO_POINTER(i))) {
        selectrow[0] = row;
        gtk_clist_select_row(GTK_CLIST(carrylist), row, 0);
      }
    }
  }

  for (i = 0; i < numlist; i++) {
    if (selectrow[i] != -1 && gtk_clist_row_is_visible(clist[i],
                                                       selectrow[i]) !=
        GTK_VISIBILITY_FULL) {
      gtk_clist_moveto(clist[i], selectrow[i], 0, 0.0, 0.0);
    }
    g_list_free(glist[i]);
  }

  gtk_clist_thaw(GTK_CLIST(carrylist));
  if (herelist)
    gtk_clist_thaw(GTK_CLIST(herelist));

  if (Inven->vbbox) {
    gtk_widget_set_sensitive(Inven->BuyButton, CanBuy);
    gtk_widget_set_sensitive(Inven->SellButton, CanSell);
    gtk_widget_set_sensitive(Inven->DropButton, CanDrop);
  }
}

static void JetCallback(GtkWidget *widget, gpointer data)
{
  int NewLocation;
  gchar *text;
  GtkWidget *JetDialog;

  JetDialog = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(widget), "dialog"));
  NewLocation = GPOINTER_TO_INT(data);
  gtk_widget_destroy(JetDialog);
  text = g_strdup_printf("%d", NewLocation);
  SendClientMessage(ClientData.Play, C_NONE, C_REQUESTJET, NULL, text);
  g_free(text);
}

void JetButtonPressed(GtkWidget *widget, gpointer data)
{
  if (InGame) {
    if (ClientData.Play->Flags & FIGHTING) {
      DisplayFightMessage(NULL);
    } else {
      Jet(NULL);
    }
  }
}

void Jet(GtkWidget *parent)
{
  GtkWidget *dialog, *table, *button, *label, *vbox;
  GtkAccelGroup *accel_group;
  gint boxsize, i, row, col;
  gchar *name, AccelChar;

  accel_group = gtk_accel_group_new();

  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  /* Title of 'Jet' dialog */
  gtk_window_set_title(GTK_WINDOW(dialog), _("Jet to location"));

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);
  gtk_window_add_accel_group(GTK_WINDOW(dialog), accel_group);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               parent ? GTK_WINDOW(parent)
                               : GTK_WINDOW(ClientData.window));

  vbox = gtk_vbox_new(FALSE, 7);

  /* Prompt in 'Jet' dialog */
  label = gtk_label_new(_("Where to, dude ? "));
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  /* Generate a square box of buttons for all locations */
  boxsize = 1;
  while (boxsize * boxsize < NumLocation)
    boxsize++;
  col = boxsize;
  row = 1;

  /* Avoid creating a box with an entire row empty at the bottom */
  while (row * col < NumLocation)
    row++;

  table = gtk_table_new(row, col, TRUE);

  for (i = 0; i < NumLocation; i++) {
    if (i < 9)
      AccelChar = '1' + i;
    else if (i < 35)
      AccelChar = 'A' + i - 9;
    else
      AccelChar = '\0';

    row = i / boxsize;
    col = i % boxsize;
    if (AccelChar == '\0') {
      button = gtk_button_new_with_label(Location[i].Name);
    } else {
      button = gtk_button_new_with_label("");

      /* Display of locations in 'Jet' window (%tde="The Bronx" etc. by
       * default) */
      name = dpg_strdup_printf(_("_%c. %tde"), AccelChar, Location[i].Name);
      SetAccelerator(button, name, button, "clicked", accel_group, FALSE);
      /* Add keypad shortcuts as well */
      if (i < 9) {
        gtk_widget_add_accelerator(button, "clicked", accel_group,
                                   GDK_KP_1 + i, 0,
                                   GTK_ACCEL_VISIBLE);
      }
      g_free(name);
    }
    gtk_widget_set_sensitive(button, i != ClientData.Play->IsAt);
    gtk_object_set_data(GTK_OBJECT(button), "dialog", dialog);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(JetCallback), GINT_TO_POINTER(i));
    gtk_table_attach_defaults(GTK_TABLE(table), button, col, col + 1, row,
                              row + 1);
  }
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

  gtk_container_add(GTK_CONTAINER(dialog), vbox);
  gtk_widget_show_all(dialog);
}

struct DealDiaStruct {
  GtkWidget *dialog, *cost, *carrying, *space, *afford, *amount;
  gint DrugInd;
  gpointer Type;
};
static struct DealDiaStruct DealDialog;

static void UpdateDealDialog(void)
{
  GString *text;
  GtkAdjustment *spin_adj;
  gint DrugInd, CanDrop, CanCarry, CanAfford, MaxDrug;
  Player *Play;

  text = g_string_new(NULL);
  DrugInd = DealDialog.DrugInd;
  Play = ClientData.Play;

  /* Display of the current price of the selected drug in 'Deal Drugs'
   * dialog */
  dpg_string_sprintf(text, _("at %P"), Play->Drugs[DrugInd].Price);
  gtk_label_set_text(GTK_LABEL(DealDialog.cost), text->str);

  CanDrop = Play->Drugs[DrugInd].Carried;

  /* Display of current inventory of the selected drug in 'Deal Drugs'
   * dialog (%tde="Opium" etc. by default) */
  dpg_string_sprintf(text, _("You are currently carrying %d %tde"),
                     CanDrop, Drug[DrugInd].Name);
  gtk_label_set_text(GTK_LABEL(DealDialog.carrying), text->str);

  CanCarry = Play->CoatSize;

  /* Available space for drugs in 'Deal Drugs' dialog */
  g_string_sprintf(text, _("Available space: %d"), CanCarry);
  gtk_label_set_text(GTK_LABEL(DealDialog.space), text->str);

  if (DealDialog.Type == BT_BUY) {
    CanAfford = Play->Cash / Play->Drugs[DrugInd].Price;

    /* Number of the selected drug that you can afford in 'Deal Drugs'
     * dialog */
    g_string_sprintf(text, _("You can afford %d"), CanAfford);
    gtk_label_set_text(GTK_LABEL(DealDialog.afford), text->str);
    MaxDrug = MIN(CanCarry, CanAfford);
  } else
    MaxDrug = CanDrop;

  spin_adj = (GtkAdjustment *)gtk_adjustment_new(MaxDrug, 1.0, MaxDrug,
                                                 1.0, 10.0, 10.0);
  gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(DealDialog.amount),
                                 spin_adj);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(DealDialog.amount), MaxDrug);

  g_string_free(text, TRUE);
}

static void DealSelectCallback(GtkWidget *widget, gpointer data)
{
  DealDialog.DrugInd = GPOINTER_TO_INT(data);
  UpdateDealDialog();
}

static void DealOKCallback(GtkWidget *widget, gpointer data)
{
  GtkWidget *spinner;
  gint amount;
  gchar *text;

  spinner = DealDialog.amount;

  gtk_spin_button_update(GTK_SPIN_BUTTON(spinner));
  amount = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinner));

  text = g_strdup_printf("drug^%d^%d", DealDialog.DrugInd,
                         data == BT_BUY ? amount : -amount);
  SendClientMessage(ClientData.Play, C_NONE, C_BUYOBJECT, NULL, text);
  g_free(text);

  gtk_widget_destroy(DealDialog.dialog);
}

void DealDrugs(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog, *label, *hbox, *hbbox, *button, *spinner, *menu,
      *optionmenu, *menuitem, *vbox, *hsep, *defbutton;
  GtkAdjustment *spin_adj;
  GtkAccelGroup *accel_group;
  GtkWidget *clist;
  gchar *Action;
  GString *text;
  GList *selection;
  gint row;
  Player *Play;
  gint DrugInd, i, SelIndex, FirstInd;
  gboolean DrugIndOK;

  g_assert(!IsShowingDealDrugs);

  /* Action in 'Deal Drugs' dialog - "Buy/Sell/Drop Drugs" */
  if (data == BT_BUY)
    Action = _("Buy");
  else if (data == BT_SELL)
    Action = _("Sell");
  else if (data == BT_DROP)
    Action = _("Drop");
  else {
    g_warning("Bad DealDrug type");
    return;
  }

  DealDialog.Type = data;
  Play = ClientData.Play;

  if (data == BT_BUY)
    clist = ClientData.Drug.HereList;
  else
    clist = ClientData.Drug.CarriedList;
  selection = GTK_CLIST(clist)->selection;
  if (selection) {
    row = GPOINTER_TO_INT(selection->data);
    DrugInd =
        GPOINTER_TO_INT(gtk_clist_get_row_data(GTK_CLIST(clist), row));
  } else
    DrugInd = -1;

  DrugIndOK = FALSE;
  FirstInd = -1;
  for (i = 0; i < NumDrug; i++) {
    if ((data == BT_DROP && Play->Drugs[i].Carried > 0
         && Play->Drugs[i].Price == 0)
        || (data == BT_SELL && Play->Drugs[i].Carried > 0
         && Play->Drugs[i].Price != 0)
        || (data == BT_BUY && Play->Drugs[i].Price != 0)) {
      if (FirstInd == -1)
        FirstInd = i;
      if (DrugInd == i)
        DrugIndOK = TRUE;
    }
  }
  if (!DrugIndOK) {
    if (FirstInd == -1)
      return;
    else
      DrugInd = FirstInd;
  }

  text = g_string_new(NULL);
  accel_group = gtk_accel_group_new();

  dialog = DealDialog.dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dialog), Action);
  gtk_window_add_accel_group(GTK_WINDOW(dialog), accel_group);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               GTK_WINDOW(ClientData.window));
  SetShowing(dialog, &IsShowingDealDrugs);

  vbox = gtk_vbox_new(FALSE, 7);

  hbox = gtk_hbox_new(FALSE, 7);

  label = gtk_label_new(Action);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  optionmenu = gtk_option_menu_new();
  menu = gtk_menu_new();
  SelIndex = -1;
  for (i = 0; i < NumDrug; i++) {
    if ((data == BT_DROP && Play->Drugs[i].Carried > 0
         && Play->Drugs[i].Price == 0)
        || (data == BT_SELL && Play->Drugs[i].Carried > 0
         && Play->Drugs[i].Price != 0)
        || (data == BT_BUY && Play->Drugs[i].Price != 0)) {
      menuitem = gtk_menu_item_new_with_label(Drug[i].Name);
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
                         GTK_SIGNAL_FUNC(DealSelectCallback),
                         GINT_TO_POINTER(i));
      gtk_menu_append(GTK_MENU(menu), menuitem);
      if (DrugInd >= i)
        SelIndex++;
    }
  }
  gtk_menu_set_active(GTK_MENU(menu), SelIndex);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_box_pack_start(GTK_BOX(hbox), optionmenu, TRUE, TRUE, 0);

  DealDialog.DrugInd = DrugInd;

  label = DealDialog.cost = gtk_label_new(NULL);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  label = DealDialog.carrying = gtk_label_new(NULL);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  label = DealDialog.space = gtk_label_new(NULL);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  if (data == BT_BUY) {
    label = DealDialog.afford = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  }
  hbox = gtk_hbox_new(FALSE, 7);
  if (data == BT_BUY) {
    /* Prompts for action in the "deal drugs" dialog */
    g_string_sprintf(text, _("Buy how many?"));
  } else if (data == BT_SELL) {
    g_string_sprintf(text, _("Sell how many?"));
  } else {
    g_string_sprintf(text, _("Drop how many?"));
  }
  label = gtk_label_new(text->str);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  spin_adj = (GtkAdjustment *)gtk_adjustment_new(1.0, 1.0, 2.0,
                                                 1.0, 10.0, 10.0);
  spinner = DealDialog.amount = gtk_spin_button_new(spin_adj, 1.0, 0);
  gtk_signal_connect(GTK_OBJECT(spinner), "activate",
                     GTK_SIGNAL_FUNC(DealOKCallback), data);
  gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();
  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(DealOKCallback), data);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  defbutton = button;
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            (gpointer)dialog);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  gtk_box_pack_start(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);

  g_string_free(text, TRUE);
  UpdateDealDialog();

  gtk_widget_show_all(dialog);
  gtk_widget_grab_default(defbutton);
}

void DealGuns(GtkWidget *widget, gpointer data)
{
  GtkWidget *clist, *dialog;
  GList *selection;
  gint row, GunInd;
  gchar *Action, *Title;
  GString *text;

  dialog = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
  if (data == BT_BUY)
    Action = _("Buy");
  else if (data == BT_SELL)
    Action = _("Sell");
  else
    Action = _("Drop");

  if (data == BT_BUY)
    clist = ClientData.Gun.HereList;
  else
    clist = ClientData.Gun.CarriedList;
  selection = GTK_CLIST(clist)->selection;
  if (selection) {
    row = GPOINTER_TO_INT(selection->data);
    GunInd =
        GPOINTER_TO_INT(gtk_clist_get_row_data(GTK_CLIST(clist), row));
  } else
    return;


  /* Title of 'gun shop' dialog (%tde="guns" by default) "Buy/Sell/Drop
   * Guns" */
  if (data == BT_BUY)
    Title = dpg_strdup_printf(_("Buy %tde"), Names.Guns);
  else if (data == BT_SELL)
    Title = dpg_strdup_printf(_("Sell %tde"), Names.Guns);
  else
    Title = dpg_strdup_printf(_("Drop %tde"), Names.Guns);

  text = g_string_new("");

  if (data != BT_BUY && TotalGunsCarried(ClientData.Play) == 0) {
    dpg_string_sprintf(text, _("You don't have any %tde to sell!"),
                       Names.Guns);
    GtkMessageBox(dialog, text->str, Title, GTK_MESSAGE_WARNING, MB_OK);
  } else if (data == BT_BUY && TotalGunsCarried(ClientData.Play) >=
             ClientData.Play->Bitches.Carried + 2) {
    dpg_string_sprintf(text,
                       _("You'll need more %tde to carry any more %tde!"),
                       Names.Bitches, Names.Guns);
    GtkMessageBox(dialog, text->str, Title, GTK_MESSAGE_WARNING, MB_OK);
  } else if (data == BT_BUY
             && Gun[GunInd].Space > ClientData.Play->CoatSize) {
    dpg_string_sprintf(text,
                       _("You don't have enough space to carry that %tde!"),
                       Names.Gun);
    GtkMessageBox(dialog, text->str, Title, GTK_MESSAGE_WARNING, MB_OK);
  } else if (data == BT_BUY && Gun[GunInd].Price > ClientData.Play->Cash) {
    dpg_string_sprintf(text,
                       _("You don't have enough cash to buy that %tde!"),
                       Names.Gun);
    GtkMessageBox(dialog, text->str, Title, GTK_MESSAGE_WARNING, MB_OK);
  } else if (data == BT_SELL && ClientData.Play->Guns[GunInd].Carried == 0) {
    GtkMessageBox(dialog, _("You don't have any to sell!"), Title, 
                  GTK_MESSAGE_WARNING, MB_OK);
  } else {
    g_string_sprintf(text, "gun^%d^%d", GunInd, data == BT_BUY ? 1 : -1);
    SendClientMessage(ClientData.Play, C_NONE, C_BUYOBJECT, NULL,
                      text->str);
  }
  g_free(Title);
  g_string_free(text, TRUE);
}

static void QuestionCallback(GtkWidget *widget, gpointer data)
{
  gint Answer;
  gchar text[5];
  GtkWidget *dialog;
  Player *To;

  dialog = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(widget), "dialog"));
  To = (Player *)gtk_object_get_data(GTK_OBJECT(dialog), "From");
  Answer = GPOINTER_TO_INT(data);

  text[0] = (gchar)Answer;
  text[1] = '\0';
  SendClientMessage(ClientData.Play, C_NONE, C_ANSWER, To, text);

  gtk_widget_destroy(dialog);
}

void QuestionDialog(char *Data, Player *From)
{
  GtkWidget *dialog, *label, *vbox, *hsep, *hbbox, *button;
  GtkAccelGroup *accel_group;
  gchar *Responses, **split, *LabelText, *trword, *underline;

  /* Button titles that correspond to the single-keypress options provided
   * by the curses client (e.g. _Yes corresponds to 'Y' etc.) */
  gchar *Words[] = { N_("_Yes"), N_("_No"), N_("_Run"),
    N_("_Fight"), N_("_Attack"), N_("_Evade")
  };
  guint numWords = sizeof(Words) / sizeof(Words[0]);
  guint i, j;

#ifdef HAVE_GLIB2
  split = g_strsplit(Data, "^", 2);
#else
  split = g_strsplit(Data, "^", 1);
#endif
  if (!split[0] || !split[1]) {
    g_warning("Bad QUESTION message %s", Data);
    return;
  }

  g_strdelimit(split[1], "^", '\n');

  Responses = split[0];
  LabelText = split[1];

  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  accel_group = gtk_accel_group_new();
  gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
                     GTK_SIGNAL_FUNC(DisallowDelete), NULL);
  gtk_object_set_data(GTK_OBJECT(dialog), "From", (gpointer)From);

  /* Title of the 'ask player a question' dialog */
  gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));

  gtk_window_add_accel_group(GTK_WINDOW(dialog), accel_group);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               GTK_WINDOW(ClientData.window));

  vbox = gtk_vbox_new(FALSE, 7);
  while (*LabelText == '\n')
    LabelText++;
  label = gtk_label_new(LabelText);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();

  for (i = 0; i < strlen(Responses); i++) {
    switch (Responses[i]) {
    case 'Y':
      button = gtk_button_new_from_stock(GTK_STOCK_YES);
      break;
    case 'N':
      button = gtk_button_new_from_stock(GTK_STOCK_NO);
      break;
    default:
      for (j = 0, trword = NULL; j < numWords && !trword; j++) {
        underline = strchr(Words[j], '_');
        if (underline && toupper(underline[1]) == Responses[i]) {
          trword = _(Words[j]);
        }
      }
      button = gtk_button_new_with_label("");
      if (trword) {
        SetAccelerator(button, trword, button, "clicked", accel_group, FALSE);
      } else {
        trword = g_strdup_printf("_%c", Responses[i]);
        SetAccelerator(button, trword, button, "clicked", accel_group, FALSE);
        g_free(trword);
      }
      break;
    }
    gtk_object_set_data(GTK_OBJECT(button), "dialog", (gpointer)dialog);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(QuestionCallback),
                       GINT_TO_POINTER((gint)Responses[i]));
    gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);
  }
  gtk_box_pack_start(GTK_BOX(vbox), hbbox, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);
  gtk_widget_show_all(dialog);

  g_strfreev(split);
}

void GuiStartGame(void)
{
  Player *Play = ClientData.Play;

  InitAbilities(Play);
  SendAbilities(Play);
  SendNullClientMessage(Play, C_NONE, C_NAME, NULL, GetPlayerName(Play));
  InGame = TRUE;
  UpdateMenus();
  gtk_widget_show_all(ClientData.vbox);
  UpdatePlayerLists();
}

void EndGame(void)
{
  DisplayFightMessage(NULL);
  gtk_widget_hide_all(ClientData.vbox);
  TextViewClear(GTK_TEXT_VIEW(ClientData.messages));
  ShutdownNetwork(ClientData.Play);
  UpdatePlayerLists();
  CleanUpServer();
  RestoreConfig();
  InGame = FALSE;
  UpdateMenus();
}

static void ChangeDrugSort(GtkCList *clist, gint column,
                           gpointer user_data)
{
  if (column == 0) {
    DrugSortMethod = (DrugSortMethod == DS_ATOZ ? DS_ZTOA : DS_ATOZ);
  } else {
    DrugSortMethod = (DrugSortMethod == DS_CHEAPFIRST ? DS_CHEAPLAST :
                      DS_CHEAPFIRST);
  }
  gtk_clist_sort(clist);
}

static gint DrugSortFunc(GtkCList *clist, gconstpointer ptr1,
                         gconstpointer ptr2)
{
  int index1, index2;
  price_t pricediff;

  index1 = GPOINTER_TO_INT(((const GtkCListRow *)ptr1)->data);
  index2 = GPOINTER_TO_INT(((const GtkCListRow *)ptr2)->data);
  if (index1 < 0 || index1 >= NumDrug || index2 < 0 || index2 >= NumDrug)
    return 0;

  switch (DrugSortMethod) {
  case DS_ATOZ:
    return g_strcasecmp(Drug[index1].Name, Drug[index2].Name);
  case DS_ZTOA:
    return g_strcasecmp(Drug[index2].Name, Drug[index1].Name);
  case DS_CHEAPFIRST:
    pricediff = ClientData.Play->Drugs[index1].Price -
                ClientData.Play->Drugs[index2].Price;
    return pricediff == 0 ? 0 : pricediff < 0 ? -1 : 1;
  case DS_CHEAPLAST:
    pricediff = ClientData.Play->Drugs[index2].Price -
                ClientData.Play->Drugs[index1].Price;
    return pricediff == 0 ? 0 : pricediff < 0 ? -1 : 1;
  }
  return 0;
}

void UpdateMenus(void)
{
  gboolean MultiPlayer;
  gint Bitches;

  MultiPlayer = (FirstClient && FirstClient->next != NULL);
  Bitches = InGame
      && ClientData.Play ? ClientData.Play->Bitches.Carried : 0;

  gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                                                       "<main>/Talk"),
                           InGame && Network);
  gtk_widget_set_sensitive(gtk_item_factory_get_widget
                           (ClientData.Menu, "<main>/Game/Options..."),
                           !InGame);
  gtk_widget_set_sensitive(gtk_item_factory_get_widget
                           (ClientData.Menu, "<main>/Game/Abandon..."),
                           InGame);
  gtk_widget_set_sensitive(gtk_item_factory_get_widget
                           (ClientData.Menu, "<main>/List"), InGame);
  gtk_widget_set_sensitive(gtk_item_factory_get_widget
                           (ClientData.Menu, "<main>/List/Players..."),
                           InGame && Network);
  gtk_widget_set_sensitive(gtk_item_factory_get_widget
                           (ClientData.Menu, "<main>/Errands"), InGame);
  gtk_widget_set_sensitive(gtk_item_factory_get_widget
                           (ClientData.Menu, "<main>/Errands/Spy..."),
                           InGame && MultiPlayer);
  gtk_widget_set_sensitive(gtk_item_factory_get_widget
                           (ClientData.Menu, "<main>/Errands/Tipoff..."),
                           InGame && MultiPlayer);
  gtk_widget_set_sensitive(gtk_item_factory_get_widget
                           (ClientData.Menu,
                            "<main>/Errands/Sack Bitch..."), Bitches > 0);
  gtk_widget_set_sensitive(gtk_item_factory_get_widget
                           (ClientData.Menu,
                            "<main>/Errands/Get spy reports..."), InGame
                           && MultiPlayer);
}

GtkWidget *CreateStatusWidgets(struct StatusWidgets *Status)
{
  GtkWidget *table, *label;

  table = gtk_table_new(3, 6, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 3);
  gtk_table_set_col_spacings(GTK_TABLE(table), 3);

  label = Status->Location = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 2, 0, 1);

  label = Status->Date = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 4, 0, 1);

  /* Available space label in GTK+ client status display */
  label = Status->SpaceName = gtk_label_new(_("Space"));

  gtk_table_attach_defaults(GTK_TABLE(table), label, 4, 5, 0, 1);
  label = Status->SpaceValue = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 5, 6, 0, 1);

  /* Player's cash label in GTK+ client status display */
  label = Status->CashName = gtk_label_new(_("Cash"));

  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
  label = Status->CashValue = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 1, 2);

  /* Player's debt label in GTK+ client status display */
  label = Status->DebtName = gtk_label_new(_("Debt"));

  gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 1, 2);
  label = Status->DebtValue = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 3, 4, 1, 2);

  /* Player's bank balance label in GTK+ client status display */
  label = Status->BankName = gtk_label_new(_("Bank"));

  gtk_table_attach_defaults(GTK_TABLE(table), label, 4, 5, 1, 2);
  label = Status->BankValue = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 5, 6, 1, 2);

  label = Status->GunsName = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
  label = Status->GunsValue = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 2, 3);

  label = Status->BitchesName = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 2, 3);
  label = Status->BitchesValue = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 3, 4, 2, 3);

  /* Player's health label in GTK+ client status display */
  label = Status->HealthName = gtk_label_new(_("Health"));

  gtk_table_attach_defaults(GTK_TABLE(table), label, 4, 5, 2, 3);
  label = Status->HealthValue = gtk_label_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 5, 6, 2, 3);
  return table;
}

void SetJetButtonTitle(GtkAccelGroup *accel_group)
{
  GtkWidget *button;
  guint accel_key;
  gchar *caption;

  button = ClientData.JetButton;
  accel_key = ClientData.JetAccel;

  if (accel_key) {
    gtk_widget_remove_accelerator(button, accel_group, accel_key, 0);
  }

  if (ClientData.Play && ClientData.Play->Flags & FIGHTING) {
    caption = _("_Fight");
  } else {
    /* Caption of 'Jet' button in main window */
    caption = _("_Jet");
  }
  ClientData.JetAccel = SetAccelerator(button, caption, button,
                                       "clicked", accel_group, FALSE);
}

static void SetIcon(GtkWidget *window, gchar **xpmdata)
{
#ifndef CYGWIN
  GdkBitmap *mask;
  GdkPixmap *icon;
  GtkStyle *style;

  style = gtk_widget_get_style(window);
  icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                      &style->bg[GTK_STATE_NORMAL],
                                      xpmdata);
  gdk_window_set_icon(window->window, NULL, icon, mask);
#endif
}

static void make_tags(GtkTextView *textview)
{
#ifdef HAVE_GLIB2
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);

  gtk_text_buffer_create_tag(buffer, "jet", "foreground", "blue", NULL);
  gtk_text_buffer_create_tag(buffer, "talk", "foreground", "red", NULL);
  gtk_text_buffer_create_tag(buffer, "page", "foreground", "magenta", NULL);
  gtk_text_buffer_create_tag(buffer, "join", "foreground", "darkblue", NULL);
  gtk_text_buffer_create_tag(buffer, "leave", "foreground", "darkred", NULL);
#endif
}

#ifdef CYGWIN
gboolean GtkLoop(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                 gboolean ReturnOnFail)
#else
gboolean GtkLoop(int *argc, char **argv[], gboolean ReturnOnFail)
#endif
{
  GtkWidget *window, *vbox, *vbox2, *hbox, *frame, *table, *menubar, *text,
      *vpaned, *button, *clist;
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

#ifdef CYGWIN
  win32_init(hInstance, hPrevInstance, "mainicon");
#else
  gtk_set_locale();
  if (ReturnOnFail && !gtk_init_check(argc, argv))
    return FALSE;
  else if (!ReturnOnFail)
    gtk_init(argc, argv);
#endif

#ifdef HAVE_GLIB2
  /* GTK+2 expects all strings to be UTF-8, so we force gettext to return
   * all translations in this encoding here. */
  bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif

  /* Set up message handlers */
  ClientMessageHandlerPt = HandleClientMessage;

  /* Have the GLib log messages pop up in a nice dialog box */
  g_log_set_handler(NULL,
                    LogMask() | G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_WARNING |
                    G_LOG_LEVEL_CRITICAL, LogMessage, NULL);

  if (!CheckHighScoreFileConfig())
    return TRUE;

  /* Create the main player */
  ClientData.Play = g_new(Player, 1);
  FirstClient = AddPlayer(0, ClientData.Play, FirstClient);

  gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
  gtk_vbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);

#ifndef HAVE_GLIB2
  gtk_hbutton_box_set_spacing_default(8);
#endif

  window = MainWindow = ClientData.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* Title of main window in GTK+ client */
  gtk_window_set_title(GTK_WINDOW(window), _("dopewars"));
  gtk_window_set_default_size(GTK_WINDOW(window), 450, 390);
  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
                     GTK_SIGNAL_FUNC(MainDelete), NULL);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
                     GTK_SIGNAL_FUNC(DestroyGtk), NULL);

  accel_group = gtk_accel_group_new();
  gtk_object_set_data(GTK_OBJECT(window), "accel_group", accel_group);
  item_factory = ClientData.Menu = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
                                                        "<main>",
                                                        accel_group);
  gtk_item_factory_set_translate_func(item_factory, MenuTranslate, NULL,
                                      NULL);

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items,
                                NULL);
  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  vbox2 = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox2), menubar, FALSE, FALSE, 0);
  gtk_widget_show_all(menubar);
  UpdateMenus();

  vbox = ClientData.vbox = gtk_vbox_new(FALSE, 5);
  frame = gtk_frame_new(_("Stats"));

  table = CreateStatusWidgets(&ClientData.Status);

  gtk_container_add(GTK_CONTAINER(frame), table);

  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

  vpaned = gtk_vpaned_new();

  text = ClientData.messages = gtk_scrolled_text_view_new(&hbox);
  make_tags(GTK_TEXT_VIEW(text));
  gtk_widget_set_usize(text, 100, 80);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_paned_pack1(GTK_PANED(vpaned), hbox, TRUE, TRUE);

  hbox = gtk_hbox_new(FALSE, 7);
  CreateInventory(hbox, Names.Drugs, accel_group, TRUE, TRUE,
                  &ClientData.Drug, GTK_SIGNAL_FUNC(DealDrugs));
  clist = ClientData.Drug.HereList;
  gtk_clist_column_titles_active(GTK_CLIST(clist));
  gtk_clist_set_compare_func(GTK_CLIST(clist), DrugSortFunc);
  gtk_signal_connect(GTK_OBJECT(clist), "click-column",
                     GTK_SIGNAL_FUNC(ChangeDrugSort), NULL);

  button = ClientData.JetButton = gtk_button_new_with_label("");
  ClientData.JetAccel = 0;
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(JetButtonPressed), NULL);
  gtk_box_pack_start(GTK_BOX(ClientData.Drug.vbbox), button, TRUE, TRUE, 0);
  SetJetButtonTitle(accel_group);

  gtk_paned_pack2(GTK_PANED(vpaned), hbox, TRUE, TRUE);

  gtk_box_pack_start(GTK_BOX(vbox), vpaned, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(vbox2), vbox, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox2);

  /* Just show the window, not the vbox - we'll do that when the game
   * starts */
  gtk_widget_show(vbox2);
  gtk_widget_show(window);

  gtk_widget_realize(window);

  SetIcon(window, dopewars_pill_xpm);

  gtk_main();

  /* Free the main player */
  FirstClient = RemovePlayer(ClientData.Play, FirstClient);

  return TRUE;
}

static void PackCentredURL(GtkWidget *vbox, gchar *title, gchar *target,
                           gchar *browser)
{
  GtkWidget *hbox, *label, *url;

  /* There must surely be a nicer way of making the URL centred - but I
   * can't think of one... */
  hbox = gtk_hbox_new(FALSE, 0);
  label = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

  url = gtk_url_new(title, target, browser);
  gtk_box_pack_start(GTK_BOX(hbox), url, FALSE, FALSE, 0);

  label = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
}

void display_intro(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog, *label, *table, *OKButton, *vbox, *hsep, *hbbox;
  gchar *VersionStr, *docindex;
  const int rows = 6, cols = 3;
  int i, j;
  gchar *table_data[6][3] = {
    /* Credits labels in GTK+ 'about' dialog */
    {N_("Icons and graphics"), "Ocelot Mantis", NULL},
    {N_("Drug Dealing and Research"), "Dan Wolf", NULL},
    {N_("Play Testing"), "Phil Davis", "Owen Walsh"},
    {N_("Extensive Play Testing"), "Katherine Holt",
     "Caroline Moore"},
    {N_("Constructive Criticism"), "Andrea Elliot-Smith",
     "Pete Winn"},
    {N_("Unconstructive Criticism"), "James Matthews", NULL}
  };

  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* Title of GTK+ 'about' dialog */
  gtk_window_set_title(GTK_WINDOW(dialog), _("About dopewars"));

  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               GTK_WINDOW(ClientData.window));
  gtk_container_border_width(GTK_CONTAINER(dialog), 10);

  vbox = gtk_vbox_new(FALSE, 5);

  /* Main content of GTK+ 'about' dialog */
  label = gtk_label_new(_("Based on John E. Dell's old Drug Wars game, "
                          "dopewars is a simulation of an\nimaginary drug "
                          "market.  dopewars is an All-American game which "
                          "features\nbuying, selling, and trying to get "
                          "past the cops!\n\nThe first thing you need to "
                          "do is pay off your debt to the Loan Shark. "
                          "After\nthat, your goal is to make as much "
                          "money as possible (and stay alive)! You\n"
                          "have one month of game time to make "
                          "your fortune.\n"));
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  /* Version and copyright notice in GTK+ 'about' dialog */
  VersionStr = g_strdup_printf(_("Version %s     "
                                 "Copyright (C) 1998-2002  "
                                 "Ben Webb ben@bellatrix.pcl.ox.ac.uk\n"
                                 "dopewars is released under the "
                                 "GNU General Public Licence\n"), VERSION);
  label = gtk_label_new(VersionStr);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  g_free(VersionStr);

  table = gtk_table_new(rows, cols, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 3);
  gtk_table_set_col_spacings(GTK_TABLE(table), 3);
  for (i = 0; i < rows; i++)
    for (j = 0; j < cols; j++)
      if (table_data[i][j]) {
        if (j == 0)
          label = gtk_label_new(_(table_data[i][j]));
        else
          label = gtk_label_new(table_data[i][j]);
        gtk_table_attach_defaults(GTK_TABLE(table), label, j, j + 1, i,
                                  i + 1);
      }
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

  /* Label at the bottom of GTK+ 'about' dialog */
  label = gtk_label_new(_("\nFor information on the command line "
                          "options, type dopewars -h at your\n"
                          "Unix prompt. This will display a help "
                          "screen, listing the available options.\n"));
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  docindex = GetDocIndex();
  PackCentredURL(vbox, "Local HTML documentation", docindex, WebBrowser);
  g_free(docindex);

  PackCentredURL(vbox, "http://dopewars.sourceforge.net/",
                 "http://dopewars.sourceforge.net/", WebBrowser);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();
  OKButton = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_signal_connect_object(GTK_OBJECT(OKButton), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            (gpointer)dialog);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), OKButton);

  gtk_box_pack_start(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);

  GTK_WIDGET_SET_FLAGS(OKButton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(OKButton);

  gtk_widget_show_all(dialog);
}

static void SendDoneMessage(GtkWidget *widget, gpointer data)
{
  SendClientMessage(ClientData.Play, C_NONE, C_DONE, NULL, NULL);
}

static void TransferPayAll(GtkWidget *widget, GtkWidget *dialog)
{
  gchar *text;

  text = pricetostr(ClientData.Play->Debt);
  SendClientMessage(ClientData.Play, C_NONE, C_PAYLOAN, NULL, text);
  g_free(text);
  gtk_widget_destroy(dialog);
}

static void TransferOK(GtkWidget *widget, GtkWidget *dialog)
{
  gpointer Debt;
  GtkWidget *deposit, *entry;
  gchar *text, *title;
  price_t money;
  gboolean withdraw = FALSE;

  Debt = gtk_object_get_data(GTK_OBJECT(dialog), "debt");
  entry = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(dialog), "entry"));
  text = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
  money = strtoprice(text);
  g_free(text);

  if (Debt) {
    /* Title of loan shark dialog - (%Tde="The Loan Shark" by default) */
    title = dpg_strdup_printf(_("%/LoanShark window title/%Tde"),
                              Names.LoanSharkName);
    if (money > ClientData.Play->Debt)
      money = ClientData.Play->Debt;
  } else {
    /* Title of bank dialog - (%Tde="The Bank" by default) */
    title = dpg_strdup_printf(_("%/BankName window title/%Tde"),
                              Names.BankName);
    deposit = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(dialog), "deposit"));
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(deposit))) {
      withdraw = TRUE;
    }
  }

  if (money < 0) {
    GtkMessageBox(dialog, _("You must enter a positive amount of money!"),
                  title, GTK_MESSAGE_WARNING, MB_OK);
  } else if (!Debt && withdraw && money > ClientData.Play->Bank) {
    GtkMessageBox(dialog, _("There isn't that much money available..."),
                  title, GTK_MESSAGE_WARNING, MB_OK);
  } else if (!withdraw && money > ClientData.Play->Cash) {
    GtkMessageBox(dialog, _("You don't have that much money!"),
                  title, GTK_MESSAGE_WARNING, MB_OK);
  } else {
    text = pricetostr(withdraw ? -money : money);
    SendClientMessage(ClientData.Play, C_NONE,
                      Debt ? C_PAYLOAN : C_DEPOSIT, NULL, text);
    g_free(text);
    gtk_widget_destroy(dialog);
  }
  g_free(title);
}

void TransferDialog(gboolean Debt)
{
  GtkWidget *dialog, *button, *label, *radio, *table, *vbox;
  GtkWidget *hbbox, *hsep, *entry;
  GSList *group;
  GString *text;

  text = g_string_new("");

  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
                     GTK_SIGNAL_FUNC(SendDoneMessage), NULL);
  if (Debt) {
    /* Title of loan shark dialog - (%Tde="The Loan Shark" by default) */
    dpg_string_sprintf(text, _("%/LoanShark window title/%Tde"),
                       Names.LoanSharkName);
  } else {
    /* Title of bank dialog - (%Tde="The Bank" by default) */
    dpg_string_sprintf(text, _("%/BankName window title/%Tde"),
                       Names.BankName);
  }
  gtk_window_set_title(GTK_WINDOW(dialog), text->str);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               GTK_WINDOW(ClientData.window));

  vbox = gtk_vbox_new(FALSE, 7);
  table = gtk_table_new(4, 3, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  gtk_table_set_col_spacings(GTK_TABLE(table), 4);

  /* Display of player's cash in bank or loan shark dialog */
  dpg_string_sprintf(text, _("Cash: %P"), ClientData.Play->Cash);
  label = gtk_label_new(text->str);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 3, 0, 1);

  if (Debt) {
    /* Display of player's debt in loan shark dialog */
    dpg_string_sprintf(text, _("Debt: %P"), ClientData.Play->Debt);
  } else {
    /* Display of player's bank balance in bank dialog */
    dpg_string_sprintf(text, _("Bank: %P"), ClientData.Play->Bank);
  }
  label = gtk_label_new(text->str);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 3, 1, 2);

  gtk_object_set_data(GTK_OBJECT(dialog), "debt", GINT_TO_POINTER(Debt));
  if (Debt) {
    /* Prompt for paying back a loan */
    label = gtk_label_new(_("Pay back:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 4);
  } else {
    /* Radio button selected if you want to pay money into the bank */
    radio = gtk_radio_button_new_with_label(NULL, _("Deposit"));
    gtk_object_set_data(GTK_OBJECT(dialog), "deposit", radio);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio));
    gtk_table_attach_defaults(GTK_TABLE(table), radio, 0, 1, 2, 3);

    /* Radio button selected if you want to withdraw money from the bank */
    radio = gtk_radio_button_new_with_label(group, _("Withdraw"));
    gtk_table_attach_defaults(GTK_TABLE(table), radio, 0, 1, 3, 4);
  }
  label = gtk_label_new(Currency.Symbol);
  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), "0");
  gtk_object_set_data(GTK_OBJECT(dialog), "entry", entry);
  gtk_signal_connect(GTK_OBJECT(entry), "activate",
                     GTK_SIGNAL_FUNC(TransferOK), dialog);

  if (Currency.Prefix) {
    gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 2, 4);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 2, 3, 2, 4);
  } else {
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 2, 4);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 2, 4);
  }

  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();
  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(TransferOK), dialog);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  if (Debt && ClientData.Play->Cash >= ClientData.Play->Debt) {
    /* Button to pay back the entire loan/debt */
    button = gtk_button_new_with_label(_("Pay all"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(TransferPayAll), dialog);
    gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);
  }
  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            (gpointer)dialog);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);
  gtk_box_pack_start(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(dialog), vbox);

  gtk_widget_show_all(dialog);

  g_string_free(text, TRUE);
}

void ListPlayers(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog, *clist, *button, *vbox, *hsep, *hbbox;

  if (IsShowingPlayerList)
    return;
  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* Title of player list dialog */
  gtk_window_set_title(GTK_WINDOW(dialog), _("Player List"));

  gtk_window_set_default_size(GTK_WINDOW(dialog), 200, 180);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);

  gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               GTK_WINDOW(ClientData.window));
  SetShowing(dialog, &IsShowingPlayerList);

  vbox = gtk_vbox_new(FALSE, 7);

  clist = ClientData.PlayerList = CreatePlayerList();
  UpdatePlayerList(clist, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), clist, TRUE, TRUE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();
  button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            (gpointer)dialog);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  gtk_box_pack_start(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);
  gtk_widget_show_all(dialog);
}

struct TalkStruct {
  GtkWidget *dialog, *clist, *entry, *checkbutton;
};

static void TalkSend(GtkWidget *widget, struct TalkStruct *TalkData)
{
  gboolean AllPlayers;
  gchar *text;
  GString *msg;
  GList *selection;
  gint row;
  Player *Play;

  AllPlayers =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                   (TalkData->checkbutton));
  text = gtk_editable_get_chars(GTK_EDITABLE(TalkData->entry), 0, -1);
  gtk_editable_delete_text(GTK_EDITABLE(TalkData->entry), 0, -1);
  if (!text)
    return;

  msg = g_string_new("");

  if (AllPlayers) {
    SendClientMessage(ClientData.Play, C_NONE, C_MSG, NULL, text);
    g_string_sprintf(msg, "%s: %s", GetPlayerName(ClientData.Play), text);
    PrintMessage(msg->str, "talk");
  } else {
    for (selection = GTK_CLIST(TalkData->clist)->selection; selection;
         selection = g_list_next(selection)) {
      row = GPOINTER_TO_INT(selection->data);
      Play =
          (Player *)gtk_clist_get_row_data(GTK_CLIST(TalkData->clist),
                                           row);
      if (Play) {
        SendClientMessage(ClientData.Play, C_NONE, C_MSGTO, Play, text);
        g_string_sprintf(msg, "%s->%s: %s", GetPlayerName(ClientData.Play),
                         GetPlayerName(Play), text);
        PrintMessage(msg->str, "page");
      }
    }
  }
  g_free(text);
  g_string_free(msg, TRUE);
}

void TalkToAll(GtkWidget *widget, gpointer data)
{
  TalkDialog(TRUE);
}

void TalkToPlayers(GtkWidget *widget, gpointer data)
{
  TalkDialog(FALSE);
}

void TalkDialog(gboolean TalkToAll)
{
  GtkWidget *dialog, *clist, *button, *entry, *label, *vbox, *hsep,
      *checkbutton, *hbbox;
  static struct TalkStruct TalkData;

  if (IsShowingTalkList)
    return;
  dialog = TalkData.dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* Title of talk dialog */
  gtk_window_set_title(GTK_WINDOW(dialog), _("Talk to player(s)"));

  gtk_window_set_default_size(GTK_WINDOW(dialog), 200, 190);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);

  gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               GTK_WINDOW(ClientData.window));
  SetShowing(dialog, &IsShowingTalkList);

  vbox = gtk_vbox_new(FALSE, 7);

  clist = TalkData.clist = ClientData.TalkList = CreatePlayerList();
  UpdatePlayerList(clist, FALSE);
  gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_MULTIPLE);
  gtk_box_pack_start(GTK_BOX(vbox), clist, TRUE, TRUE, 0);

  checkbutton = TalkData.checkbutton =
      /* Checkbutton set if you want to talk to all players */
      gtk_check_button_new_with_label(_("Talk to all players"));

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), TalkToAll);
  gtk_box_pack_start(GTK_BOX(vbox), checkbutton, FALSE, FALSE, 0);

  /* Prompt for you to enter the message to be sent to other players */
  label = gtk_label_new(_("Message:-"));

  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  entry = TalkData.entry = gtk_entry_new();
  gtk_signal_connect(GTK_OBJECT(entry), "activate",
                     GTK_SIGNAL_FUNC(TalkSend), (gpointer)&TalkData);
  gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();

  /* Button to send a message to other players */
  button = gtk_button_new_with_label(_("Send"));

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(TalkSend), (gpointer)&TalkData);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            (gpointer)dialog);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  gtk_box_pack_start(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(dialog), vbox);
  gtk_widget_show_all(dialog);
}

GtkWidget *CreatePlayerList(void)
{
  GtkWidget *clist;
  gchar *text[1];

  text[0] = "Name";
  clist = gtk_clist_new_with_titles(1, text);
  gtk_clist_column_titles_passive(GTK_CLIST(clist));
  gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
  return clist;
}

void UpdatePlayerList(GtkWidget *clist, gboolean IncludeSelf)
{
  GSList *list;
  gchar *text[1];
  gint row;
  Player *Play;

  gtk_clist_freeze(GTK_CLIST(clist));
  gtk_clist_clear(GTK_CLIST(clist));
  for (list = FirstClient; list; list = g_slist_next(list)) {
    Play = (Player *)list->data;
    if (IncludeSelf || Play != ClientData.Play) {
      text[0] = GetPlayerName(Play);
      row = gtk_clist_append(GTK_CLIST(clist), text);
      gtk_clist_set_row_data(GTK_CLIST(clist), row, Play);
    }
  }
  gtk_clist_thaw(GTK_CLIST(clist));
}

static void ErrandOK(GtkWidget *widget, GtkWidget *clist)
{
  GList *selection;
  Player *Play;
  gint row;
  GtkWidget *dialog;
  gint ErrandType;

  dialog = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(widget), "dialog"));
  ErrandType = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),
                                                   "errandtype"));
  selection = GTK_CLIST(clist)->selection;
  if (selection) {
    row = GPOINTER_TO_INT(selection->data);
    Play = (Player *)gtk_clist_get_row_data(GTK_CLIST(clist), row);
    if (ErrandType == ET_SPY) {
      SendClientMessage(ClientData.Play, C_NONE, C_SPYON, Play, NULL);
    } else {
      SendClientMessage(ClientData.Play, C_NONE, C_TIPOFF, Play, NULL);
    }
    gtk_widget_destroy(dialog);
  }
}

void SpyOnPlayer(GtkWidget *widget, gpointer data)
{
  ErrandDialog(ET_SPY);
}

void TipOff(GtkWidget *widget, gpointer data)
{
  ErrandDialog(ET_TIPOFF);
}

void ErrandDialog(gint ErrandType)
{
  GtkWidget *dialog, *clist, *button, *vbox, *hbbox, *hsep, *label;
  gchar *text;

  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);

  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               GTK_WINDOW(ClientData.window));

  vbox = gtk_vbox_new(FALSE, 7);

  if (ErrandType == ET_SPY) {
    /* Title of dialog to select a player to spy on */
    gtk_window_set_title(GTK_WINDOW(dialog), _("Spy On Player"));

    /* Informative text for "spy on player" dialog. (%tde = "bitch",
     * "bitch", "guns", "drugs", respectively, by default) */
    text = dpg_strdup_printf(_("Please choose the player to spy on. "
                               "Your %tde will\nthen offer his "
                               "services to the player, and if "
                               "successful,\nyou will be able to "
                               "view the player's stats with the\n"
                               "\"Get spy reports\" menu. Remember "
                               "that the %tde will leave\nyou, so "
                               "any %tde or %tde that he's "
                               "carrying may be lost!"), Names.Bitch,
                             Names.Bitch, Names.Guns, Names.Drugs);
    label = gtk_label_new(text);
    g_free(text);
  } else {

    /* Title of dialog to select a player to tip the cops off to */
    gtk_window_set_title(GTK_WINDOW(dialog), _("Tip Off The Cops"));

    /* Informative text for "tip off cops" dialog. (%tde = "bitch",
     * "bitch", "guns", "drugs", respectively, by default) */
    text = dpg_strdup_printf(_("Please choose the player to tip off "
                               "the cops to. Your %tde will\nhelp "
                               "the cops to attack that player, "
                               "and then report back to you\non "
                               "the encounter. Remember that the "
                               "%tde will leave you temporarily,\n"
                               "so any %tde or %tde that he's "
                               "carrying may be lost!"), Names.Bitch,
                             Names.Bitch, Names.Guns, Names.Drugs);
    label = gtk_label_new(text);
    g_free(text);
  }

  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  clist = ClientData.PlayerList = CreatePlayerList();
  UpdatePlayerList(clist, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), clist, TRUE, TRUE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();
  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_object_set_data(GTK_OBJECT(button), "dialog", dialog);
  gtk_object_set_data(GTK_OBJECT(button), "errandtype",
                      GINT_TO_POINTER(ErrandType));
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(ErrandOK), (gpointer)clist);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);
  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            (gpointer)dialog);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  gtk_box_pack_start(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);
  gtk_widget_show_all(dialog);
}

void SackBitch(GtkWidget *widget, gpointer data)
{
  char *title, *text;

  /* Cannot sack bitches if you don't have any! */
  if (ClientData.Play->Bitches.Carried <= 0)
    return;

  /* Title of dialog to sack a bitch (%Tde = "Bitch" by default) */
  title = dpg_strdup_printf(_("%/Sack Bitch dialog title/Sack %Tde"),
                            Names.Bitch);

  /* Confirmation message for sacking a bitch. (%tde = "guns", "drugs",
   * "bitch", respectively, by default) */
  text = dpg_strdup_printf(_("Are you sure? (Any %tde or %tde carried\n"
                             "by this %tde may be lost!)"), Names.Guns,
                           Names.Drugs, Names.Bitch);

  if (GtkMessageBox(ClientData.window, text, title, GTK_MESSAGE_QUESTION,
                    MB_YESNO) == IDYES) {
    ClientData.Play->Bitches.Carried--;
    UpdateMenus();
    SendClientMessage(ClientData.Play, C_NONE, C_SACKBITCH, NULL, NULL);
  }
  g_free(text);
  g_free(title);
}

void CreateInventory(GtkWidget *hbox, gchar *Objects,
                     GtkAccelGroup *accel_group, gboolean CreateButtons,
                     gboolean CreateHere, struct InventoryWidgets *widgets,
                     GtkSignalFunc CallBack)
{
  GtkWidget *scrollwin, *clist, *vbbox, *frame[2], *button[3];
  gint i, mini;
  GString *text;
  gchar *titles[2][2];
  gchar *button_text[3];
  gpointer button_type[3] = { BT_BUY, BT_SELL, BT_DROP };

  /* Column titles for display of drugs/guns carried or available for
   * purchase */
  titles[0][0] = titles[1][0] = _("Name");
  titles[0][1] = _("Price");
  titles[1][1] = _("Number");

  /* Button titles for buying/selling/dropping guns or drugs */
  button_text[0] = _("_Buy ->");
  button_text[1] = _("<- _Sell");
  button_text[2] = _("_Drop <-");

  text = g_string_new("");

  if (CreateHere) {
    /* Title of the display of available drugs/guns (%Tde = "Guns" or
     * "Drugs" by default) */
    dpg_string_sprintf(text, _("%Tde here"), Objects);
    widgets->HereFrame = frame[0] = gtk_frame_new(text->str);
  }

  /* Title of the display of carried drugs/guns (%Tde = "Guns" or "Drugs"
   * by default) */
  dpg_string_sprintf(text, _("%Tde carried"), Objects);

  widgets->CarriedFrame = frame[1] = gtk_frame_new(text->str);

  widgets->HereList = widgets->CarriedList = NULL;
  if (CreateHere)
    mini = 0;
  else
    mini = 1;
  for (i = mini; i < 2; i++) {
    gtk_container_set_border_width(GTK_CONTAINER(frame[i]), 5);

    clist = gtk_scrolled_clist_new_with_titles(2, titles[i], &scrollwin);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);
    gtk_clist_column_titles_passive(GTK_CLIST(clist));
    gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_SINGLE);
    gtk_clist_set_auto_sort(GTK_CLIST(clist), FALSE);
    gtk_container_add(GTK_CONTAINER(frame[i]), scrollwin);
    if (i == 0)
      widgets->HereList = clist;
    else
      widgets->CarriedList = clist;
  }
  if (CreateHere)
    gtk_box_pack_start(GTK_BOX(hbox), frame[0], TRUE, TRUE, 0);

  if (CreateButtons) {
    widgets->vbbox = vbbox = gtk_vbutton_box_new();

    for (i = 0; i < 3; i++) {
      button[i] = gtk_button_new_with_label("");
      SetAccelerator(button[i], _(button_text[i]), button[i],
                     "clicked", accel_group, FALSE);
      if (CallBack)
        gtk_signal_connect(GTK_OBJECT(button[i]), "clicked",
                           GTK_SIGNAL_FUNC(CallBack), button_type[i]);
      gtk_box_pack_start(GTK_BOX(vbbox), button[i], TRUE, TRUE, 0);
    }
    widgets->BuyButton = button[0];
    widgets->SellButton = button[1];
    widgets->DropButton = button[2];
    gtk_box_pack_start(GTK_BOX(hbox), vbbox, FALSE, FALSE, 0);
  } else
    widgets->vbbox = NULL;

  gtk_box_pack_start(GTK_BOX(hbox), frame[1], TRUE, TRUE, 0);
  g_string_free(text, TRUE);
}

void SetShowing(GtkWidget *window, gboolean *showing)
{
  g_assert(showing);

  *showing = TRUE;
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
                     GTK_SIGNAL_FUNC(DestroyShowing), (gpointer)showing);
}

void DestroyShowing(GtkWidget *widget, gpointer data)
{
  gboolean *IsShowing = (gboolean *)data;

  if (IsShowing)
    *IsShowing = FALSE;
}

static void NewNameOK(GtkWidget *widget, GtkWidget *window)
{
  GtkWidget *entry;
  gchar *text;

  entry = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(window), "entry"));
  text = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
  if (text[0]) {
    SetPlayerName(ClientData.Play, text);
    SendNullClientMessage(ClientData.Play, C_NONE, C_NAME, NULL, text);
    gtk_widget_destroy(window);
  }
  g_free(text);
}

void NewNameDialog(void)
{
  GtkWidget *window, *button, *hsep, *vbox, *label, *entry;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* Title of dialog for changing a player's name */
  gtk_window_set_title(GTK_WINDOW(window), _("Change Name"));

  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(window),
                               GTK_WINDOW(ClientData.window));
  gtk_container_set_border_width(GTK_CONTAINER(window), 7);
  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
                     GTK_SIGNAL_FUNC(DisallowDelete), NULL);

  vbox = gtk_vbox_new(FALSE, 7);

  /* Informational text to prompt the player to change his/her name */
  label = gtk_label_new(_("Unfortunately, somebody else is already "
                          "using \"your\" name. Please change it:-"));
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  entry = gtk_entry_new();
  gtk_object_set_data(GTK_OBJECT(window), "entry", entry);
  gtk_signal_connect(GTK_OBJECT(entry), "activate",
                     GTK_SIGNAL_FUNC(NewNameOK), window);
  gtk_entry_set_text(GTK_ENTRY(entry), GetPlayerName(ClientData.Play));
  gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(NewNameOK), window);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(button);

  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show_all(window);
}

gint DisallowDelete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  return (TRUE);
}

void GunShopDialog(void)
{
  GtkWidget *window, *button, *hsep, *vbox, *hbox, *hbbox;
  GtkAccelGroup *accel_group;
  gchar *text;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 600, 190);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
                     GTK_SIGNAL_FUNC(SendDoneMessage), NULL);
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

  /* Title of 'gun shop' dialog in GTK+ client (%Tde="Dan's House of Guns"
   * by default) */
  text = dpg_strdup_printf(_("%/GTK GunShop window title/%Tde"),
                           Names.GunShopName);
  gtk_window_set_title(GTK_WINDOW(window), text);
  g_free(text);
  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(window),
                               GTK_WINDOW(ClientData.window));
  gtk_container_set_border_width(GTK_CONTAINER(window), 7);
  SetShowing(window, &IsShowingGunShop);

  vbox = gtk_vbox_new(FALSE, 7);

  hbox = gtk_hbox_new(FALSE, 7);
  CreateInventory(hbox, Names.Guns, accel_group, TRUE, TRUE,
                  &ClientData.Gun, GTK_SIGNAL_FUNC(DealGuns));

  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

  hbbox = my_hbbox_new();
  button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            (gpointer)window);
  gtk_box_pack_start_defaults(GTK_BOX(hbbox), button);

  gtk_box_pack_start(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  UpdateInventory(&ClientData.Gun, ClientData.Play->Guns, NumGun, FALSE);
  gtk_widget_show_all(window);
}

void UpdatePlayerLists(void)
{
  if (IsShowingPlayerList)
    UpdatePlayerList(ClientData.PlayerList, FALSE);
  if (IsShowingTalkList)
    UpdatePlayerList(ClientData.TalkList, FALSE);
}

void GetSpyReports(GtkWidget *Widget, gpointer data)
{
  SendClientMessage(ClientData.Play, C_NONE, C_CONTACTSPY, NULL, NULL);
}

static void DestroySpyReports(GtkWidget *widget, gpointer data)
{
  SpyReportsDialog = NULL;
}

static void CreateSpyReports(void)
{
  GtkWidget *window, *button, *vbox, *notebook;
  GtkAccelGroup *accel_group;

  SpyReportsDialog = window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  accel_group = gtk_accel_group_new();
  gtk_object_set_data(GTK_OBJECT(window), "accel_group", accel_group);
  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

  /* Title of window to display reports from spies with other players */
  gtk_window_set_title(GTK_WINDOW(window), _("Spy reports"));

  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(window),
                               GTK_WINDOW(ClientData.window));
  gtk_container_set_border_width(GTK_CONTAINER(window), 7);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
                     GTK_SIGNAL_FUNC(DestroySpyReports), NULL);

  vbox = gtk_vbox_new(FALSE, 5);
  notebook = gtk_notebook_new();
  gtk_object_set_data(GTK_OBJECT(window), "notebook", notebook);

  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

  button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            (gpointer)window);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(window), vbox);

  gtk_widget_show_all(window);
}

void DisplaySpyReports(Player *Play)
{
  GtkWidget *dialog, *notebook, *vbox, *hbox, *frame, *label, *table;
  GtkAccelGroup *accel_group;
  struct StatusWidgets Status;
  struct InventoryWidgets SpyDrugs, SpyGuns;

  if (!SpyReportsDialog)
    CreateSpyReports();
  dialog = SpyReportsDialog;
  notebook =
      GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(dialog), "notebook"));
  accel_group =
      (GtkAccelGroup
       *)(gtk_object_get_data(GTK_OBJECT(dialog), "accel_group"));
  vbox = gtk_vbox_new(FALSE, 5);
  frame = gtk_frame_new("Stats");
  gtk_container_set_border_width(GTK_CONTAINER(frame), 4);
  table = CreateStatusWidgets(&Status);
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  CreateInventory(hbox, Names.Drugs, accel_group, FALSE, FALSE, &SpyDrugs,
                  NULL);
  CreateInventory(hbox, Names.Guns, accel_group, FALSE, FALSE, &SpyGuns,
                  NULL);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  label = gtk_label_new(GetPlayerName(Play));

  DisplayStats(Play, &Status);
  UpdateInventory(&SpyDrugs, Play->Drugs, NumDrug, TRUE);
  UpdateInventory(&SpyGuns, Play->Guns, NumGun, FALSE);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

  gtk_widget_show_all(notebook);
}
