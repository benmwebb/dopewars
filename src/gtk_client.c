/* gtk_client.c  dopewars client using the GTK+ toolkit                 */
/* Copyright (C)  1998-2000  Ben Webb                                   */
/*                Email: ben@bellatrix.pcl.ox.ac.uk                     */
/*                WWW: http://bellatrix.pcl.ox.ac.uk/~ben/dopewars/     */

/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU General Public License          */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */

/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */

/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston,               */
/*                   MA  02111-1307, USA.                               */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef GTK_CLIENT

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "dopeos.h"
#include "dopewars.h"
#include "gtk_client.h"
#include "message.h"
#include "serverside.h"

#define BT_BUY  (GINT_TO_POINTER(1))
#define BT_SELL (GINT_TO_POINTER(2))
#define BT_DROP (GINT_TO_POINTER(3))

#define MB_OK     1
#define MB_CANCEL 2
#define MB_YES    4
#define MB_NO     8
#define MB_MAX    4

#define ET_SPY    0
#define ET_TIPOFF 1

struct InventoryWidgets {
   GtkWidget *HereList,*CarriedList;
   GtkWidget *HereFrame,*CarriedFrame;
   GtkWidget *BuyButton,*SellButton,*DropButton;
   GtkWidget *vbbox;
};

struct StatusWidgets {
   GtkWidget *Location,*Date,*SpaceName,*SpaceValue,*CashName,*CashValue,
             *DebtName,*DebtValue,*BankName,*BankValue,
             *GunsName,*GunsValue,*BitchesName,*BitchesValue,
             *HealthName,*HealthValue;
};

struct ClientDataStruct {
   GtkWidget *window,*messages;
   gchar *PlayerName;
   Player *Play;
   GtkItemFactory *Menu;
   struct StatusWidgets Status;
   struct InventoryWidgets Drug,Gun,InvenDrug,InvenGun;
   GtkWidget *JetButton,*vbox,*PlayerList,*TalkList;
   guint JetAccel;
   gint GdkInputTag;
};

static struct ClientDataStruct ClientData;
static gboolean InGame=FALSE,MetaServerRead=FALSE;
static GtkWidget *FightDialog=NULL,*SpyReportsDialog;
static gboolean IsShowingPlayerList=FALSE,IsShowingTalkList=FALSE,
                IsShowingInventory=FALSE,IsShowingGunShop=FALSE;

static void display_intro(GtkWidget *widget,gpointer data);
static void QuitGame(GtkWidget *widget,gpointer data);
static void DestroyGtk(GtkWidget *widget,gpointer data);
static void NewGame(GtkWidget *widget,gpointer data);
static void ListScores(GtkWidget *widget,gpointer data);
static void ListInventory(GtkWidget *widget,gpointer data);
static void NewGameDialog();
static void StartGame();
static void EndGame();
static void UpdateMenus();
static void GetClientMessage(gpointer data,gint socket,
                             GdkInputCondition condition);
static void SetSocketWriteTest(Player *Play,gboolean WriteTest);
static void HandleClientMessage(char *buf,Player *Play);
static void PrepareHighScoreDialog();
static void AddScoreToDialog(char *Data);
static void CompleteHighScoreDialog();
static void PrintMessage(char *Data);
static void DisplayFightMessage(char *Data);
static GtkWidget *CreateStatusWidgets(struct StatusWidgets *Status);
static void DisplayStats(Player *Play,struct StatusWidgets *Status);
static void UpdateStatus(Player *Play,gboolean DisplayDrugs);
static void SetJetButtonTitle(GtkAccelGroup *accel_group);
static void UpdateInventory(struct InventoryWidgets *Inven,
                            Inventory *Objects,int NumObjects,
                            gboolean AreDrugs);
static void JetButtonPressed(GtkWidget *widget,gpointer data);
static void Jet();
static void DealDrugs(GtkWidget *widget,gpointer data);
static void DealGuns(GtkWidget *widget,gpointer data);
static void QuestionDialog(char *Data,Player *From);
static gint MessageBox(GtkWidget *parent,const gchar *Title,
                       const gchar *Text,gint Options);
static void TransferDialog(gboolean Debt);
static void ListPlayers(GtkWidget *widget,gpointer data);
static void TalkToAll(GtkWidget *widget,gpointer data);
static void TalkToPlayers(GtkWidget *widget,gpointer data);
static void TalkDialog(gboolean TalkToAll);
static GtkWidget *CreatePlayerList();
static void UpdatePlayerList(GtkWidget *clist,gboolean IncludeSelf);
static void TipOff(GtkWidget *widget,gpointer data);
static void SpyOnPlayer(GtkWidget *widget,gpointer data);
static void ErrandDialog(gint ErrandType);
static void SackBitch(GtkWidget *widget,gpointer data);
static void DestroyShowing(GtkWidget *widget,gpointer data);
static gint DisallowDelete(GtkWidget *widget,GdkEvent *event,gpointer data);
static void GunShopDialog();
static void NewNameDialog();
static void UpdatePlayerLists();
static void CreateInventory(GtkWidget *hbox,gchar *Objects,
                            GtkAccelGroup *accel_group,
                            gboolean CreateButtons,gboolean CreateHere,
                            struct InventoryWidgets *widgets,
                            GtkSignalFunc CallBack);
static void GetSpyReports(GtkWidget *widget,gpointer data);
static void DisplaySpyReports(Player *Play);

static GtkItemFactoryEntry menu_items[] = {
   { N_("/_Game"),NULL,NULL,0,"<Branch>" },
   { N_("/Game/_New"),"<control>N",NewGame,0,NULL },
   { N_("/Game/_Quit"),"<control>Q",QuitGame,0,NULL },
   { N_("/_Talk"),NULL,NULL,0,"<Branch>" },
   { N_("/Talk/To _All"),NULL,TalkToAll,0,NULL },
   { N_("/Talk/To _Player"),NULL,TalkToPlayers,0,NULL },
   { N_("/_List"),NULL,NULL,0,"<Branch>" },
   { N_("/List/_Players"),NULL,ListPlayers,0,NULL },
   { N_("/List/_Scores"),NULL,ListScores,0,NULL },
   { N_("/List/_Inventory"),NULL,ListInventory,0,NULL },
   { N_("/_Errands"),NULL,NULL,0,"<Branch>" },
   { N_("/Errands/_Spy"),NULL,SpyOnPlayer,0,NULL },
   { N_("/Errands/_Tipoff"),NULL,TipOff,0,NULL },
   { N_("/Errands/Sack _Bitch"),NULL,SackBitch,0,NULL },
   { N_("/Errands/_Get spy reports"),NULL,GetSpyReports,0,NULL },
   { N_("/_Help"),NULL,NULL,0,"<LastBranch>" },
   { N_("/Help/_About"),"F1",display_intro,0,NULL }
};

static void LogMessage(const gchar *log_domain,GLogLevelFlags log_level,
                       const gchar *message,gpointer user_data) {
   MessageBox(NULL,log_level&G_LOG_LEVEL_WARNING ? _("Warning") : _("Message"),
              message,MB_OK);
}

void QuitGame(GtkWidget *widget,gpointer data) {
   if (!InGame ||
       MessageBox(ClientData.window,_("Quit Game"),_("Abandon current game?"),
                  MB_YES|MB_NO)==MB_YES) {
      gtk_main_quit();
   }
}

void DestroyGtk(GtkWidget *widget,gpointer data) {
   gtk_main_quit();
}

gint MainDelete(GtkWidget *widget,GdkEvent *event,gpointer data) {
   return (InGame && MessageBox(ClientData.window,_("Quit Game"),
           _("Abandon current game?"),MB_YES|MB_NO)==MB_NO);
}


void NewGame(GtkWidget *widget,gpointer data) {
   if (InGame) {
      if (MessageBox(ClientData.window,_("Start new game"),
          _("Abandon current game?"),MB_YES|MB_NO)==MB_YES) EndGame();
      else return;
   }
   NewGameDialog();
}

void ListScores(GtkWidget *widget,gpointer data) {
   SendClientMessage(ClientData.Play,C_NONE,C_REQUESTSCORE,NULL,NULL,
                     ClientData.Play);
}

void ListInventory(GtkWidget *widget,gpointer data) {
   GtkWidget *window,*button,*hsep,*vbox,*hbox;
   GtkAccelGroup *accel_group;
   gchar *caps;

   if (IsShowingInventory) return;
   window=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_default_size(GTK_WINDOW(window),550,120);
   accel_group=gtk_accel_group_new();
   gtk_window_add_accel_group(GTK_WINDOW(window),accel_group);
   gtk_window_set_title(GTK_WINDOW(window),_("Inventory"));

   IsShowingInventory=TRUE;
   gtk_window_set_modal(GTK_WINDOW(window),FALSE);
   gtk_object_set_data(GTK_OBJECT(window),"IsShowing",
                       (gpointer)&IsShowingInventory);
   gtk_signal_connect(GTK_OBJECT(window),"destroy",
                      GTK_SIGNAL_FUNC(DestroyShowing),NULL);

   gtk_window_set_transient_for(GTK_WINDOW(window),
                                GTK_WINDOW(ClientData.window));
   gtk_container_set_border_width(GTK_CONTAINER(window),7);

   vbox=gtk_vbox_new(FALSE,7);

   hbox=gtk_hbox_new(FALSE,7);
   caps=InitialCaps(Names.Drugs);
   CreateInventory(hbox,caps,accel_group,FALSE,FALSE,
                   &ClientData.InvenDrug,NULL); g_free(caps);
   caps=InitialCaps(Names.Guns);
   CreateInventory(hbox,caps,accel_group,FALSE,FALSE,
                   &ClientData.InvenGun,NULL); g_free(caps);

   gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   button=gtk_button_new_with_label(_("Close"));
   gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)window);
   gtk_box_pack_start(GTK_BOX(vbox),button,FALSE,FALSE,0);

   gtk_container_add(GTK_CONTAINER(window),vbox);

   UpdateInventory(&ClientData.InvenDrug,ClientData.Play->Drugs,NumDrug,TRUE);
   UpdateInventory(&ClientData.InvenGun,ClientData.Play->Guns,NumGun,FALSE);

   gtk_widget_show_all(window);
}

void GetClientMessage(gpointer data,gint socket,
                      GdkInputCondition condition) {
   gchar *pt;
   if (condition&GDK_INPUT_WRITE) {
      WriteConnectionBufferToWire(ClientData.Play);
      if (ClientData.Play->WriteBuf.DataPresent==0) {
         SetSocketWriteTest(ClientData.Play,FALSE);
      }
   }
   if (condition&GDK_INPUT_READ) {
      if (ReadConnectionBufferFromWire(ClientData.Play)) {
         while ((pt=ReadFromConnectionBuffer(ClientData.Play))!=NULL) {
            HandleClientMessage(pt,ClientData.Play); g_free(pt);
         }
      } else {
         if (Network) gdk_input_remove(ClientData.GdkInputTag);
         g_warning(_("Connection to server lost - switching to "
                   "single player mode"));
         SwitchToSinglePlayer(ClientData.Play);
      }
   }
}

void SetSocketWriteTest(Player *Play,gboolean WriteTest) {
   if (Network) {
      if (ClientData.GdkInputTag) gdk_input_remove(ClientData.GdkInputTag);
      ClientData.GdkInputTag=gdk_input_add(Play->fd,
                             GDK_INPUT_READ|(WriteTest ? GDK_INPUT_WRITE : 0),
                             GetClientMessage,NULL);
   }
}

void HandleClientMessage(char *pt,Player *Play) {
   char *Data,Code,AICode,DisplayMode;
   Player *From,*tmp;
   gchar *text,*prstr;
   gboolean Handled;
   GtkWidget *MenuItem;
   GSList *list;

/* Ignore To: field as all messages should be for "Play" */
   if (ProcessMessage(pt,Play,&From,&AICode,&Code,NULL,&Data,FirstClient)==-1) {
      return;
   }

   Handled=HandleGenericClientMessage(From,AICode,Code,Play,Data,&DisplayMode);
   switch(Code) {
      case C_STARTHISCORE:
         PrepareHighScoreDialog(); break;
      case C_HISCORE:
         AddScoreToDialog(Data); break;
      case C_ENDHISCORE:
         CompleteHighScoreDialog();
         if (strcmp(Data,"end")==0) EndGame();
         break;
      case C_PRINTMESSAGE:
         PrintMessage(Data); break;
      case C_FIGHTPRINT:
         DisplayFightMessage(Data); break;
      case C_PUSH:
         if (Network) gdk_input_remove(ClientData.GdkInputTag);
         g_warning(_("You have been pushed from the server."));
         SwitchToSinglePlayer(Play);
         break;
      case C_QUIT:
         if (Network) gdk_input_remove(ClientData.GdkInputTag);
         g_warning(_("The server has terminated."));
         SwitchToSinglePlayer(Play);
         break;
      case C_NEWNAME:
         NewNameDialog(); break;
      case C_BANK:
         TransferDialog(FALSE); break;
      case C_LOANSHARK:
         TransferDialog(TRUE); break;
      case C_GUNSHOP:
         GunShopDialog(); break;
      case C_MSG:
         text=g_strdup_printf("%s: %s",GetPlayerName(From),Data);
         PrintMessage(text); g_free(text);
         break;
      case C_MSGTO:
         text=g_strdup_printf("%s->%s: %s",GetPlayerName(From),
                              GetPlayerName(Play),Data);
         PrintMessage(text); g_free(text);
         break;
      case C_JOIN:
         text=g_strdup_printf(_("%s joins the game!"),Data);
         PrintMessage(text); g_free(text);
         UpdatePlayerLists();
         break;
      case C_LEAVE:
         if (From!=&Noone) {
            text=g_strdup_printf(_("%s has left the game."),Data);
            PrintMessage(text); g_free(text);
            UpdatePlayerLists();
         }
         break;
      case C_QUESTION:
         QuestionDialog(Data,From==&Noone ? NULL : From); break;
      case C_SUBWAYFLASH:
         DisplayFightMessage(NULL);
         for (list=FirstClient;list;list=g_slist_next(list)) {
            tmp=(Player *)list->data;
            tmp->Flags &= ~FIGHTING;
         }
         text=g_strdup_printf(_("Jetting to %s"),
                              Location[(int)Play->IsAt].Name);
         PrintMessage(text); g_free(text);
         break;
      case C_ENDLIST:
         MenuItem=gtk_item_factory_get_widget(ClientData.Menu,
                                              _("<main>/Errands/Spy"));
         prstr=FormatPrice(Prices.Spy);
         text=g_strdup_printf(_("_Spy\t(%s)"),prstr);
         gtk_label_parse_uline(GTK_LABEL(GTK_BIN(MenuItem)->child),text);
         g_free(text); g_free(prstr);
         prstr=FormatPrice(Prices.Tipoff);
         text=g_strdup_printf(_("_Tipoff\t(%s)"),prstr);
         MenuItem=gtk_item_factory_get_widget(ClientData.Menu,
                                              _("<main>/Errands/Tipoff"));
         gtk_label_parse_uline(GTK_LABEL(GTK_BIN(MenuItem)->child),text);
         g_free(text); g_free(prstr);
         break;
      case C_UPDATE:
         if (From==&Noone) {
            ReceivePlayerData(Data,Play);
            UpdateStatus(Play,TRUE);
         } else {
            ReceivePlayerData(Data,From);
            DisplaySpyReports(From);
         }
         break;
      case C_DRUGHERE:
         UpdateInventory(&ClientData.Drug,Play->Drugs,NumDrug,TRUE);
         gtk_clist_sort(GTK_CLIST(ClientData.Drug.HereList));
         if (IsShowingInventory) {
            UpdateInventory(&ClientData.InvenDrug,Play->Drugs,NumDrug,TRUE);
         }
         break;
   }
}

struct HiScoreDiaStruct {
   GtkWidget *dialog,*table,*vbox;
};
static struct HiScoreDiaStruct HiScoreDialog;

void PrepareHighScoreDialog() {
   GtkWidget *dialog,*vbox,*hsep,*table;

   HiScoreDialog.dialog=dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_title(GTK_WINDOW(dialog),_("High Scores"));
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);
   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   HiScoreDialog.vbox=vbox=gtk_vbox_new(FALSE,7);
   HiScoreDialog.table=table=gtk_table_new(NUMHISCORE,1,FALSE);
   gtk_table_set_row_spacings(GTK_TABLE(table),5);
   gtk_table_set_col_spacings(GTK_TABLE(table),5);

   gtk_box_pack_start(GTK_BOX(vbox),table,TRUE,TRUE,0);
   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);
   gtk_container_add(GTK_CONTAINER(dialog),vbox);
   gtk_widget_show_all(dialog);
}

void AddScoreToDialog(char *Data) {
   GtkWidget *label;
   char *cp;
   int index;
   cp=Data;
   index=GetNextInt(&cp,0);
   if (!cp || strlen(cp)<2) return;
   label=gtk_label_new(&cp[1]);
   gtk_table_attach_defaults(GTK_TABLE(HiScoreDialog.table),label,
                             0,1,index,index+1);
   gtk_widget_show(label);
}

void CompleteHighScoreDialog() {
   GtkWidget *OKButton,*dialog;
   dialog=HiScoreDialog.dialog;
   OKButton=gtk_button_new_with_label(_("OK"));
   gtk_signal_connect_object(GTK_OBJECT(OKButton),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)dialog);
   gtk_box_pack_start(GTK_BOX(HiScoreDialog.vbox),OKButton,TRUE,TRUE,0);
 
   GTK_WIDGET_SET_FLAGS(OKButton,GTK_CAN_DEFAULT);
   gtk_widget_grab_default(OKButton);
   gtk_widget_show(OKButton);
}

void PrintMessage(char *text) {
   gint EditPos;
   char *cr="\n";
   GtkEditable *messages;

   messages=GTK_EDITABLE(ClientData.messages);

   gtk_text_freeze(GTK_TEXT(messages));
   g_strdelimit(text,"^",'\n');
   EditPos=gtk_text_get_length(GTK_TEXT(ClientData.messages));
   while (*text=='\n') text++;
   gtk_editable_insert_text(messages,text,strlen(text),&EditPos);
   if (text[strlen(text)-1]!='\n') {
      gtk_editable_insert_text(messages,cr,1,&EditPos);
   }
   gtk_text_thaw(GTK_TEXT(messages));
   gtk_editable_set_position(messages,EditPos);
}

static void FightCallback(GtkWidget *widget,gpointer data) {
   gint Answer;
   Player *Play;
   gchar *text;
   Answer=GPOINTER_TO_INT(data);
   Play=ClientData.Play;
   switch(Answer) {
      case 'D':
         gtk_widget_hide(FightDialog); break;
      case 'R':
         gtk_widget_hide(FightDialog);
         Jet(); break;
      case 'F': case 'S':
         if (Play->Flags&CANSHOOT &&
             ((Answer=='F' && TotalGunsCarried(Play)>0) ||
              (Answer=='S' && TotalGunsCarried(Play)==0))) {
            Play->Flags &= ~CANSHOOT;
            text=g_strdup_printf("%c",Answer);
            SendClientMessage(Play,C_NONE,C_FIGHTACT,NULL,text,Play);
            g_free(text);
         }
         break;
   }
}

static GtkWidget *AddFightButton(gchar *Text,GtkAccelGroup *accel_group,
                                 GtkBox *box,gint Answer) {
   GtkWidget *button;
   guint AccelKey;
   button=gtk_button_new_with_label("");
   AccelKey=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),Text);
   gtk_widget_add_accelerator(button,"clicked",accel_group,AccelKey,0,
                              GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(FightCallback),
                      GINT_TO_POINTER(Answer));
   gtk_box_pack_start(box,button,TRUE,TRUE,0);
   return button;
}

static void CreateFightDialog() {
   GtkWidget *dialog,*vbox,*button,*hbox,*hbbox,*hsep,*text,*vscroll;
   GtkAdjustment *adj;
   GtkAccelGroup *accel_group;
   gchar *buf;

   FightDialog=dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_signal_connect(GTK_OBJECT(dialog),"delete_event",
                      GTK_SIGNAL_FUNC(DisallowDelete),NULL);
   gtk_window_set_default_size(GTK_WINDOW(dialog),240,130);
   accel_group=gtk_accel_group_new();
   gtk_window_add_accel_group(GTK_WINDOW(dialog),accel_group);
   gtk_window_set_title(GTK_WINDOW(dialog),_("Fight"));
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);

   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   vbox=gtk_vbox_new(FALSE,7);

   hbox=gtk_hbox_new(FALSE,0);
   adj=(GtkAdjustment *)gtk_adjustment_new(0.0,0.0,100.0,1.0,10.0,10.0);
   text=gtk_text_new(NULL,adj);
   gtk_object_set_data(GTK_OBJECT(dialog),"text",text);
   gtk_text_set_editable(GTK_TEXT(text),FALSE);
   gtk_text_set_word_wrap(GTK_TEXT(text),TRUE);
   gtk_object_set_data(GTK_OBJECT(dialog),"text",text);
   gtk_box_pack_start(GTK_BOX(hbox),text,TRUE,TRUE,0);
   vscroll=gtk_vscrollbar_new(adj);
   gtk_box_pack_start(GTK_BOX(hbox),vscroll,FALSE,FALSE,0);
   gtk_widget_show_all(hbox);
   gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);
   gtk_widget_show(hsep);

   hbbox=gtk_hbutton_box_new();
   buf=g_strdup_printf(_("_Deal %s"),Names.Drugs);
   button=AddFightButton(buf,accel_group,GTK_BOX(hbbox),'D');
   gtk_widget_show(button);

   button=AddFightButton(_("_Fight"),accel_group,GTK_BOX(hbbox),'F');
   gtk_object_set_data(GTK_OBJECT(dialog),"fight",button);

   button=AddFightButton(_("_Stand"),accel_group,GTK_BOX(hbbox),'S');
   gtk_object_set_data(GTK_OBJECT(dialog),"stand",button);

   button=AddFightButton(_("_Run"),accel_group,GTK_BOX(hbbox),'R');
   gtk_object_set_data(GTK_OBJECT(dialog),"run",button);

   gtk_widget_show(hsep);
   gtk_box_pack_start(GTK_BOX(vbox),hbbox,FALSE,FALSE,0);
   gtk_widget_show(hbbox);
   gtk_widget_show(vbox);
   gtk_container_add(GTK_CONTAINER(dialog),vbox);
   gtk_widget_show(dialog);
}

void DisplayFightMessage(char *Data) {
   Player *Play;
   gint EditPos;
   GtkWidget *Fight,*Stand,*Run,*Text;
   char cr[] = "\n";

   if (!Data) {
      if (FightDialog) {
         gtk_widget_destroy(FightDialog); FightDialog=NULL;
      }
      return;
   }
   if (FightDialog) {
      if (!GTK_WIDGET_VISIBLE(FightDialog)) gtk_widget_show(FightDialog);
   } else {
      CreateFightDialog();
   }
   if (!FightDialog) return;

   Fight=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog),"fight"));
   Stand=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog),"stand"));
   Run=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog),"run"));
   Text=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog),"text"));

   Play=ClientData.Play;

   g_strdelimit(Data,"^",'\n');
   if (strlen(Data)>0) {
      EditPos=gtk_text_get_length(GTK_TEXT(Text));
      gtk_editable_insert_text(GTK_EDITABLE(Text),Data,strlen(Data),&EditPos);
      gtk_editable_insert_text(GTK_EDITABLE(Text),cr,strlen(cr),&EditPos);
   }

   if (Play->Flags&CANSHOOT && TotalGunsCarried(Play)>0)
      gtk_widget_show(Fight); else gtk_widget_hide(Fight);
   if (Play->Flags&CANSHOOT && TotalGunsCarried(Play)==0)
      gtk_widget_show(Stand); else gtk_widget_hide(Stand);
   if (Play->Flags&FIGHTING)
      gtk_widget_show(Run); else gtk_widget_hide(Run);
}

void DisplayStats(Player *Play,struct StatusWidgets *Status) {
   gchar *prstr,*caps;
   GString *text;

   text=g_string_new(NULL);

   gtk_label_set_text(GTK_LABEL(Status->Location),
                      Location[(int)Play->IsAt].Name);

   g_string_sprintf(text,"%s%02d%s",Names.Month,Play->Turn,Names.Year);
   gtk_label_set_text(GTK_LABEL(Status->Date),text->str);

   g_string_sprintf(text,"%d",Play->CoatSize);
   gtk_label_set_text(GTK_LABEL(Status->SpaceValue),text->str);
   
   prstr=FormatPrice(Play->Cash);
   gtk_label_set_text(GTK_LABEL(Status->CashValue),prstr);
   g_free(prstr);

   prstr=FormatPrice(Play->Bank);
   gtk_label_set_text(GTK_LABEL(Status->BankValue),prstr);
   g_free(prstr);

   prstr=FormatPrice(Play->Debt);
   gtk_label_set_text(GTK_LABEL(Status->DebtValue),prstr);
   g_free(prstr);

   caps=InitialCaps(Names.Guns);
   gtk_label_set_text(GTK_LABEL(Status->GunsName),caps);
   g_free(caps);
   g_string_sprintf(text,"%d",TotalGunsCarried(Play));
   gtk_label_set_text(GTK_LABEL(Status->GunsValue),text->str);

   if (!WantAntique) {
      caps=InitialCaps(Names.Bitches);
      gtk_label_set_text(GTK_LABEL(Status->BitchesName),caps);
      g_free(caps);
      g_string_sprintf(text,"%d",Play->Bitches.Carried);
      gtk_label_set_text(GTK_LABEL(Status->BitchesValue),text->str);
   } else {
      gtk_label_set_text(GTK_LABEL(Status->BitchesName),NULL);
      gtk_label_set_text(GTK_LABEL(Status->BitchesValue),NULL);
   }

   g_string_sprintf(text,"%d",Play->Health);
   gtk_label_set_text(GTK_LABEL(Status->HealthValue),text->str);

   g_string_free(text,TRUE);
}

void UpdateStatus(Player *Play,gboolean DisplayDrugs) {
   GtkAccelGroup *accel_group;
   DisplayStats(Play,&ClientData.Status);
   UpdateInventory(&ClientData.Drug,ClientData.Play->Drugs,NumDrug,TRUE);
   gtk_clist_sort(GTK_CLIST(ClientData.Drug.HereList));
   accel_group=(GtkAccelGroup *)
             gtk_object_get_data(GTK_OBJECT(ClientData.window),"accel_group");
   SetJetButtonTitle(accel_group);
   if (IsShowingGunShop) {
      UpdateInventory(&ClientData.Gun,ClientData.Play->Guns,NumGun,FALSE);
   }
   if (IsShowingInventory) {
      UpdateInventory(&ClientData.InvenDrug,ClientData.Play->Drugs,
                      NumDrug,TRUE);
      UpdateInventory(&ClientData.InvenGun,ClientData.Play->Guns,
                      NumGun,FALSE);
   }
}

void UpdateInventory(struct InventoryWidgets *Inven,
                     Inventory *Objects,int NumObjects,gboolean AreDrugs) {
   GtkWidget *herelist,*carrylist;
   Player *Play;
   gint i,row,selectrow[2];
   gpointer rowdata;
   price_t price;
   gchar *titles[2];
   gboolean CanBuy=FALSE,CanSell=FALSE,CanDrop=FALSE;
   GList *glist[2],*selection;
   GtkCList *clist[2];
   int numlist;

   Play=ClientData.Play;
   herelist=Inven->HereList;
   carrylist=Inven->CarriedList;

   if (herelist) numlist=2; else numlist=1;

/* Make lists of the current selections */
   clist[0]=GTK_CLIST(carrylist);
   if (herelist) clist[1]=GTK_CLIST(herelist); else clist[1]=NULL;

   for (i=0;i<numlist;i++) {
      glist[i]=NULL;
      selectrow[i]=-1;
      for (selection=clist[i]->selection;selection;
           selection=g_list_next(selection)) {
         row=GPOINTER_TO_INT(selection->data);
         rowdata=gtk_clist_get_row_data(clist[i],row);
         glist[i]=g_list_append(glist[i],rowdata);
      }
   }

   gtk_clist_freeze(GTK_CLIST(carrylist));
   gtk_clist_clear(GTK_CLIST(carrylist));

   if (herelist) {
      gtk_clist_freeze(GTK_CLIST(herelist));
      gtk_clist_clear(GTK_CLIST(herelist));
   }

   for (i=0;i<NumObjects;i++) {
      if (AreDrugs) {
         titles[0] = Drug[i].Name; price=Objects[i].Price;
      } else {
         titles[0]=Gun[i].Name; price=Gun[i].Price;
      }

      if (herelist && price > 0) {
         CanBuy=TRUE;
         titles[1] = FormatPrice(price);
         row=gtk_clist_append(GTK_CLIST(herelist),titles); g_free(titles[1]);
         gtk_clist_set_row_data(GTK_CLIST(herelist),row,GINT_TO_POINTER(i));
         if (g_list_find(glist[1],GINT_TO_POINTER(i))) {
            selectrow[1]=row;
            gtk_clist_select_row(GTK_CLIST(herelist),row,0);
         }
      }

      if (Objects[i].Carried > 0) {
         if (price>0) CanSell=TRUE; else CanDrop=TRUE;
         titles[1] = g_strdup_printf("%d",Objects[i].Carried);
         row=gtk_clist_append(GTK_CLIST(carrylist),titles); g_free(titles[1]);
         gtk_clist_set_row_data(GTK_CLIST(carrylist),row,GINT_TO_POINTER(i));
         if (g_list_find(glist[0],GINT_TO_POINTER(i))) {
            selectrow[0]=row;
            gtk_clist_select_row(GTK_CLIST(carrylist),row,0);
         }
      }
   }

   for (i=0;i<numlist;i++) {
      if (selectrow[i]!=-1 && gtk_clist_row_is_visible(clist[i],
                                selectrow[i])!=GTK_VISIBILITY_FULL) {
         gtk_clist_moveto(clist[i],selectrow[i],0,0.0,0.0);
      }
      g_list_free(glist[i]);
   }

   gtk_clist_thaw(GTK_CLIST(carrylist));
   if (herelist) gtk_clist_thaw(GTK_CLIST(herelist));

   if (Inven->vbbox) {
      gtk_widget_set_sensitive(Inven->BuyButton,CanBuy);
      gtk_widget_set_sensitive(Inven->SellButton,CanSell);
      gtk_widget_set_sensitive(Inven->DropButton,CanDrop);
   }
}

static void JetCallback(GtkWidget *widget,gpointer data) {
   int NewLocation;
   gchar *text;
   GtkWidget *JetDialog;

   JetDialog = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(widget),"dialog"));
   NewLocation = GPOINTER_TO_INT(data);
   gtk_widget_destroy(JetDialog);
   text=g_strdup_printf("%d",NewLocation);
   SendClientMessage(ClientData.Play,C_NONE,C_REQUESTJET,NULL,
                     text,ClientData.Play);
   g_free(text);
}

void JetButtonPressed(GtkWidget *widget,gpointer data) {
   if (ClientData.Play->Flags & FIGHTING) {
      DisplayFightMessage("");
   } else {
      Jet();
   }
}

void Jet() {
   GtkWidget *dialog,*table,*button,*label,*vbox;
   GtkAccelGroup *accel_group;
   gint boxsize,i,row,col;
   gchar *name,AccelChar;
   guint AccelKey;

   accel_group=gtk_accel_group_new();

   dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_title(GTK_WINDOW(dialog),_("Jet to location"));
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);
   gtk_window_add_accel_group(GTK_WINDOW(dialog),accel_group);
   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   vbox=gtk_vbox_new(FALSE,7);

   label=gtk_label_new(_("Where to, dude ? "));
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

   /* Generate a square box of buttons for all locations */
   boxsize=1;
   while (boxsize*boxsize < NumLocation) boxsize++;
   col=boxsize; row=1;

   /* Avoid creating a box with an entire row empty at the bottom */
   while (row*col < NumLocation) row++;

   table=gtk_table_new(row,col,TRUE);

   for (i=0;i<NumLocation;i++) {
      if (i<9) AccelChar='1'+i;
      else if (i<35) AccelChar='A'+i-9;
      else AccelChar='\0';

      row=i/boxsize; col=i%boxsize;
      if (AccelChar=='\0') {
         button=gtk_button_new_with_label(Location[i].Name);
      } else {
         button=gtk_button_new_with_label("");
         name=g_strdup_printf("_%c. %s",AccelChar,Location[i].Name);
         AccelKey=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),name);
         gtk_widget_add_accelerator(button,"clicked",accel_group,AccelKey,0,
                        GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
         g_free(name);
      }
      gtk_widget_set_sensitive(button,i != ClientData.Play->IsAt);
      gtk_object_set_data(GTK_OBJECT(button),"dialog",dialog);
      gtk_signal_connect(GTK_OBJECT(button),"clicked",
                         GTK_SIGNAL_FUNC(JetCallback),GINT_TO_POINTER(i));
      gtk_table_attach_defaults(GTK_TABLE(table),button,col,col+1,row,row+1);
   }
   gtk_box_pack_start(GTK_BOX(vbox),table,TRUE,TRUE,0);

   gtk_container_add(GTK_CONTAINER(dialog),vbox);
   gtk_widget_show_all(dialog);
}

struct DealDiaStruct {
   GtkWidget *dialog,*cost,*carrying,*space,*afford,*amount;
   gint DrugInd;
   gpointer Type;
};
static struct DealDiaStruct DealDialog;

static void UpdateDealDialog() {
   GString *text;
   gchar *prstr;
   GtkAdjustment *spin_adj;
   gint DrugInd,CanDrop,CanCarry,CanAfford,MaxDrug;
   Player *Play;

   text=g_string_new(NULL);
   DrugInd=DealDialog.DrugInd;
   Play=ClientData.Play;

   prstr=FormatPrice(Play->Drugs[DrugInd].Price);
   g_string_sprintf(text,_("at %s"),prstr);
   g_free(prstr);
   gtk_label_set_text(GTK_LABEL(DealDialog.cost),text->str);

   CanDrop=Play->Drugs[DrugInd].Carried;
   g_string_sprintf(text,_("You are currently carrying %d %s"),
                    CanDrop,Drug[DrugInd].Name);
   gtk_label_set_text(GTK_LABEL(DealDialog.carrying),text->str);

   CanCarry=Play->CoatSize;
   g_string_sprintf(text,_("Available space: %d"),CanCarry);
   gtk_label_set_text(GTK_LABEL(DealDialog.space),text->str);

   if (DealDialog.Type==BT_BUY) {
      CanAfford=Play->Cash/Play->Drugs[DrugInd].Price;
      g_string_sprintf(text,_("You can afford %d"),CanAfford);
      gtk_label_set_text(GTK_LABEL(DealDialog.afford),text->str);
      MaxDrug=MIN(CanCarry,CanAfford);
   } else MaxDrug=CanDrop;

   spin_adj=(GtkAdjustment *)gtk_adjustment_new(MaxDrug,1.0,MaxDrug,
                                                1.0,10.0,10.0);
   gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(DealDialog.amount),spin_adj);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(DealDialog.amount),MaxDrug);

   g_string_free(text,TRUE);
}

static void DealSelectCallback(GtkWidget *widget,gpointer data) {
   DealDialog.DrugInd=GPOINTER_TO_INT(data);
   UpdateDealDialog();
}

static void DealOKCallback(GtkWidget *widget,gpointer data) {
   GtkWidget *spinner;
   gint amount;
   gchar *text;

   spinner=DealDialog.amount;

   amount=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinner));

   text=g_strdup_printf("drug^%d^%d",DealDialog.DrugInd,
                        data==BT_BUY ? amount : -amount);
   SendClientMessage(ClientData.Play,C_NONE,C_BUYOBJECT,NULL,
                     text,ClientData.Play);
   g_free(text);

   gtk_widget_destroy(DealDialog.dialog);
}

void DealDrugs(GtkWidget *widget,gpointer data) {
   GtkWidget *dialog,*label,*hbox,*hbbox,*button,*spinner,*menu,
             *optionmenu,*menuitem,*vbox,*hsep;
   GtkAdjustment *spin_adj;
   GtkAccelGroup *accel_group;
   GtkWidget *clist;
   gchar *Action;
   GString *text;
   GList *selection;
   gint row;
   Player *Play;
   gint DrugInd,i,SelIndex,FirstInd;
   gboolean DrugIndOK;

   if (data==BT_BUY) Action=_("Buy");
   else if (data==BT_SELL) Action=_("Sell");
   else if (data==BT_DROP) Action=_("Drop");
   else {
      g_warning("Bad DealDrug type"); return;
   }

   DealDialog.Type=data;
   Play=ClientData.Play;

   if (data==BT_BUY) clist=ClientData.Drug.HereList;
   else clist=ClientData.Drug.CarriedList;
   selection=GTK_CLIST(clist)->selection;
   if (selection) {
      row=GPOINTER_TO_INT(selection->data);
      DrugInd=GPOINTER_TO_INT(gtk_clist_get_row_data(GTK_CLIST(clist),row));
   } else DrugInd=-1;

   DrugIndOK=FALSE;
   FirstInd=-1;
   for (i=0;i<NumDrug;i++) {
      if ((data==BT_DROP && Play->Drugs[i].Carried>0 &&
           Play->Drugs[i].Price==0) ||
          (data==BT_SELL && Play->Drugs[i].Carried>0 &&
           Play->Drugs[i].Price!=0) ||
          (data==BT_BUY && Play->Drugs[i].Price!=0)) {
         if (FirstInd==-1) FirstInd=i;
         if (DrugInd==i) DrugIndOK=TRUE;
      }
   }
   if (!DrugIndOK) {
      if (FirstInd==-1) return;
      else DrugInd=FirstInd;
   }

   text=g_string_new(NULL);
   accel_group=gtk_accel_group_new();
   dialog=DealDialog.dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_title(GTK_WINDOW(dialog),Action);
   gtk_window_add_accel_group(GTK_WINDOW(dialog),accel_group);
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);
   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   vbox=gtk_vbox_new(FALSE,7);

   hbox=gtk_hbox_new(FALSE,7);

   label=gtk_label_new(Action);
   gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);

   optionmenu=gtk_option_menu_new();
   menu=gtk_menu_new();
   SelIndex=-1;
   for (i=0;i<NumDrug;i++) {
      if ((data==BT_DROP && Play->Drugs[i].Carried>0 &&
           Play->Drugs[i].Price==0) ||
          (data==BT_SELL && Play->Drugs[i].Carried>0 &&
           Play->Drugs[i].Price!=0) ||
          (data==BT_BUY && Play->Drugs[i].Price!=0)) {
         menuitem=gtk_menu_item_new_with_label(Drug[i].Name);
         gtk_signal_connect(GTK_OBJECT(menuitem),"activate",
                            GTK_SIGNAL_FUNC(DealSelectCallback),
                            GINT_TO_POINTER(i));
         gtk_menu_append(GTK_MENU(menu),menuitem);
         if (DrugInd>=i) SelIndex++;
      }
   }
   gtk_menu_set_active(GTK_MENU(menu),SelIndex);
   gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu),menu);
   gtk_box_pack_start(GTK_BOX(hbox),optionmenu,TRUE,TRUE,0);

   DealDialog.DrugInd=DrugInd;

   label=DealDialog.cost=gtk_label_new(NULL);
   gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
   gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

   label=DealDialog.carrying=gtk_label_new(NULL);
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

   label=DealDialog.space=gtk_label_new(NULL);
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

   if (data==BT_BUY) {
      label=DealDialog.afford=gtk_label_new(NULL);
      gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);
   }
   hbox=gtk_hbox_new(FALSE,7);
   g_string_sprintf(text,_("%s how many?"),Action);
   label=gtk_label_new(text->str);
   gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
   spin_adj=(GtkAdjustment *)gtk_adjustment_new(1.0,1.0,2.0,1.0,10.0,10.0);
   spinner=DealDialog.amount=gtk_spin_button_new(spin_adj,1.0,0);
   gtk_box_pack_start(GTK_BOX(hbox),spinner,FALSE,FALSE,0);
   gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   hbbox=gtk_hbutton_box_new();
   button=gtk_button_new_with_label(_("OK"));
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(DealOKCallback),data);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_grab_default(button);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);   
   button=gtk_button_new_with_label(_("Cancel"));
   gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)dialog);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);   

   gtk_box_pack_start(GTK_BOX(vbox),hbbox,FALSE,FALSE,0);
   gtk_container_add(GTK_CONTAINER(dialog),vbox);

   g_string_free(text,TRUE);
   UpdateDealDialog();

   gtk_widget_show_all(dialog);
}

void DealGuns(GtkWidget *widget,gpointer data) {
   GtkWidget *clist,*dialog;
   GList *selection;
   gint row,GunInd;
   gchar *Action,*Title;
   GString *text;

   dialog=gtk_widget_get_ancestor(widget,GTK_TYPE_WINDOW);
   if (data==BT_BUY) Action=_("Buy");
   else if (data==BT_SELL) Action=_("Sell");
   else Action=_("Drop");

   if (data==BT_BUY) clist=ClientData.Gun.HereList;
   else clist=ClientData.Gun.CarriedList;
   selection=GTK_CLIST(clist)->selection;
   if (selection) {
      row=GPOINTER_TO_INT(selection->data);
      GunInd=GPOINTER_TO_INT(gtk_clist_get_row_data(GTK_CLIST(clist),row));
   } else return;

   Title=g_strdup_printf("%s %s",Action,Names.Guns);
   text=g_string_new("");

   if (data!=BT_BUY && TotalGunsCarried(ClientData.Play)==0) {
      g_string_sprintf(text,_("You don't have any %s!"),Names.Guns);
      MessageBox(dialog,Title,text->str,MB_OK);
   } else if (data==BT_BUY && TotalGunsCarried(ClientData.Play) >=
                              ClientData.Play->Bitches.Carried+2) { 
      g_string_sprintf(text,_("You'll need more %s to carry any more %s!"),
                           Names.Bitches,Names.Guns);
      MessageBox(dialog,Title,text->str,MB_OK);
   } else if (data==BT_BUY && Gun[GunInd].Space > ClientData.Play->CoatSize) {
      g_string_sprintf(text,_("You don't have enough space to carry that %s!"),
                           Names.Gun);
      MessageBox(dialog,Title,text->str,MB_OK);
   } else if (data==BT_BUY && Gun[GunInd].Price > ClientData.Play->Cash) {
      g_string_sprintf(text,_("You don't have enough cash to buy that %s!"),
                           Names.Gun);
      MessageBox(dialog,Title,text->str,MB_OK);
   } else if (data==BT_SELL && ClientData.Play->Guns[GunInd].Carried == 0) {
      MessageBox(dialog,Title,_("You don't have any to sell!"),MB_OK);
   } else {
      g_string_sprintf(text,"gun^%d^%d",GunInd,data==BT_BUY ? 1 : -1);
      SendClientMessage(ClientData.Play,C_NONE,C_BUYOBJECT,NULL,text->str,
                        ClientData.Play);
   }
   g_free(Title);
   g_string_free(text,TRUE);
}

static void QuestionCallback(GtkWidget *widget,gpointer data) {
   gint Answer;
   gchar text[5];
   GtkWidget *dialog;
   Player *To;

   dialog = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(widget),"dialog"));
   To = (Player *)gtk_object_get_data(GTK_OBJECT(dialog),"From");
   Answer = GPOINTER_TO_INT(data);

   text[0]=(gchar)Answer; text[1]='\0';
   SendClientMessage(ClientData.Play,C_NONE,C_ANSWER,To,text,ClientData.Play);

   gtk_widget_destroy(dialog);
}

void QuestionDialog(char *Data,Player *From) {
   GtkWidget *dialog,*label,*vbox,*hsep,*hbbox,*button;
   GtkAccelGroup *accel_group;
   guint AccelKey;
   gchar *Responses,**split,*LabelText;
   gchar *Words[] = { N_("_Yes"), N_("_No"), N_("_Run"),
                      N_("_Fight"), N_("_Attack"), N_("_Evade") };
   gint numWords = sizeof(Words) / sizeof(Words[0]);
   gint i,Answer;

   split=g_strsplit(Data,"^",1);
   if (!split[0] || !split[1]) {
      g_warning("Bad QUESTION message %s",Data); return;
   }

   g_strdelimit(split[1],"^",'\n');

   Responses=split[0]; LabelText=split[1];

   dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   accel_group=gtk_accel_group_new();
   gtk_signal_connect(GTK_OBJECT(dialog),"delete_event",
                      GTK_SIGNAL_FUNC(DisallowDelete),NULL);
   gtk_object_set_data(GTK_OBJECT(dialog),"From",(gpointer)From);
   gtk_window_set_title(GTK_WINDOW(dialog),_("Question"));
   gtk_window_add_accel_group(GTK_WINDOW(dialog),accel_group);
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);
   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   vbox=gtk_vbox_new(FALSE,7);
   while (*LabelText=='\n') LabelText++;
   label=gtk_label_new(LabelText);
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   hbbox=gtk_hbutton_box_new();

   for (i=0;i<numWords;i++) {
      Answer=(gint)Words[i][0];
      if (Answer=='_' && strlen(Words[i])>=2) Answer=(gint)Words[i][1];
      if (strchr(Responses,Answer)) {
         button=gtk_button_new_with_label("");
         AccelKey=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                                        _(Words[i]));
         gtk_widget_add_accelerator(button,"clicked",accel_group,AccelKey,0,
                              GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
         gtk_object_set_data(GTK_OBJECT(button),"dialog",(gpointer)dialog);
         gtk_signal_connect(GTK_OBJECT(button),"clicked",
                            GTK_SIGNAL_FUNC(QuestionCallback),
                            GINT_TO_POINTER(Answer));
         gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);
      }
   }
   gtk_box_pack_start(GTK_BOX(vbox),hbbox,TRUE,TRUE,0);
   gtk_container_add(GTK_CONTAINER(dialog),vbox);
   gtk_widget_show_all(dialog);
   
   g_strfreev(split);
}

void StartGame() {
   Player *Play;
   Play=ClientData.Play=g_new(Player,1);
   FirstClient=AddPlayer(0,Play,FirstClient);
   Play->fd=ClientSock;
   SendAbilities(Play);
   SetPlayerName(Play,ClientData.PlayerName);
   SendClientMessage(NULL,C_NONE,C_NAME,NULL,ClientData.PlayerName,Play);
   InGame=TRUE;
   UpdateMenus();
   if (Network) {
      SetSocketWriteTest(Play,TRUE);
   }
   gtk_widget_show_all(ClientData.vbox);
   UpdatePlayerLists();
}

void EndGame() {
   DisplayFightMessage(NULL);
   gtk_widget_hide_all(ClientData.vbox);
   gtk_editable_delete_text(GTK_EDITABLE(ClientData.messages),0,-1);
   g_free(ClientData.PlayerName);
   ClientData.PlayerName=g_strdup(GetPlayerName(ClientData.Play));
   if (Network) gdk_input_remove(ClientData.GdkInputTag);
   ShutdownNetwork();
   UpdatePlayerLists();
   CleanUpServer();
   InGame=FALSE;
   UpdateMenus();
}

static void ChangeDrugSort(GtkCList *clist,gint column,gpointer user_data) {
   if (column==0) {
      DrugSortMethod=(DrugSortMethod==DS_ATOZ ? DS_ZTOA : DS_ATOZ);
   } else {
      DrugSortMethod=(DrugSortMethod==DS_CHEAPFIRST ? DS_CHEAPLAST :
                                                      DS_CHEAPFIRST);
   }
   gtk_clist_sort(clist);
}

static gint DrugSortFunc(GtkCList *clist,gconstpointer ptr1,
                         gconstpointer ptr2) {
   int index1,index2;
   price_t pricediff;

   index1=GPOINTER_TO_INT(((GtkCListRow *)ptr1)->data);
   index2=GPOINTER_TO_INT(((GtkCListRow *)ptr2)->data);
   if (index1<0 || index1>=NumDrug || index2<0 || index2>=NumDrug) return 0;

   switch(DrugSortMethod) {
      case DS_ATOZ:
         return strcasecmp(Drug[index1].Name,Drug[index2].Name);
      case DS_ZTOA:
         return strcasecmp(Drug[index2].Name,Drug[index1].Name);
      case DS_CHEAPFIRST:
         pricediff=ClientData.Play->Drugs[index1].Price-
                   ClientData.Play->Drugs[index2].Price;
         return pricediff==0 ? 0 : pricediff<0 ? -1 : 1;
      case DS_CHEAPLAST:
         pricediff=ClientData.Play->Drugs[index2].Price-
                   ClientData.Play->Drugs[index1].Price;
         return pricediff==0 ? 0 : pricediff<0 ? -1 : 1;
   }
   return 0;
}

void UpdateMenus() {
   gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                                                   _("<main>/Talk")),InGame);
   gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                                                   _("<main>/List")),InGame);
   gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                                                   _("<main>/Errands")),InGame);
}

GtkWidget *CreateStatusWidgets(struct StatusWidgets *Status) {
   GtkWidget *table,*label;

   table = gtk_table_new(3,6,FALSE);
   gtk_table_set_row_spacings(GTK_TABLE(table),3);
   gtk_table_set_col_spacings(GTK_TABLE(table),3);

   label=Status->Location = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,0,2,0,1);

   label=Status->Date = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,2,4,0,1);

   label=Status->SpaceName = gtk_label_new(_("Space"));
   gtk_table_attach_defaults(GTK_TABLE(table),label,4,5,0,1);
   label=Status->SpaceValue = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,5,6,0,1);

   label=Status->CashName = gtk_label_new(_("Cash"));
   gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,1,2);
   label=Status->CashValue = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,1,2,1,2);

   label=Status->DebtName = gtk_label_new(_("Debt"));
   gtk_table_attach_defaults(GTK_TABLE(table),label,2,3,1,2);
   label=Status->DebtValue = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,3,4,1,2);

   label=Status->BankName = gtk_label_new(_("Bank"));
   gtk_table_attach_defaults(GTK_TABLE(table),label,4,5,1,2);
   label=Status->BankValue = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,5,6,1,2);

   label=Status->GunsName = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,2,3);
   label=Status->GunsValue = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,1,2,2,3);

   label=Status->BitchesName = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,2,3,2,3);
   label=Status->BitchesValue = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,3,4,2,3);

   label=Status->HealthName = gtk_label_new(_("Health"));
   gtk_table_attach_defaults(GTK_TABLE(table),label,4,5,2,3);
   label=Status->HealthValue = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,5,6,2,3);
   return table;
}

void SetJetButtonTitle(GtkAccelGroup *accel_group) {
   GtkWidget *button;
   guint accel_key;

   button=ClientData.JetButton;
   accel_key=ClientData.JetAccel;

   if (accel_key) {
      gtk_widget_remove_accelerator(button,accel_group,accel_key,0);
   }
   
   accel_key=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                (ClientData.Play && ClientData.Play->Flags & FIGHTING) ?
                _("_Fight") : _("_Jet!"));
   gtk_widget_add_accelerator(button,"clicked",accel_group,accel_key,0,
                              GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
   ClientData.JetAccel=accel_key;
}

char GtkLoop(int *argc,char **argv[],char ReturnOnFail) {
   GtkWidget *window,*vbox,*vbox2,*hbox,*frame,*table,*menubar,*text,
             *vpaned,*button,*vscroll,*clist;
   GtkAccelGroup *accel_group;
   GtkItemFactory *item_factory;
   GtkAdjustment *adj;
   gchar *buf;
   int i;
   gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

   if (ReturnOnFail && !gtk_init_check(argc,argv)) return FALSE;
   else if (!ReturnOnFail) gtk_init(argc,argv);

/* Set up message handlers */
   ClientMessageHandlerPt = HandleClientMessage;
   ClientData.GdkInputTag=0;
   SocketWriteTestPt = SetSocketWriteTest;

/* Have the GLib log messages pop up in a nice dialog box */
   g_log_set_handler(NULL,G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_WARNING,
                     LogMessage,NULL);

   ClientData.PlayerName=NULL;
   ClientData.Play=NULL;
   window=ClientData.window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(window),_("dopewars"));
   gtk_window_set_default_size(GTK_WINDOW(window),450,390);
   gtk_signal_connect(GTK_OBJECT(window),"delete_event",
                      GTK_SIGNAL_FUNC(MainDelete),NULL);
   gtk_signal_connect(GTK_OBJECT(window),"destroy",
                      GTK_SIGNAL_FUNC(DestroyGtk),NULL);

   accel_group = gtk_accel_group_new();
   gtk_object_set_data(GTK_OBJECT(window),"accel_group",accel_group);
   item_factory = ClientData.Menu = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
                                                         "<main>",accel_group);
   /* Translate the paths of the menu items, in place */
   for (i=0;i<nmenu_items;i++) {
      menu_items[i].path=_(menu_items[i].path);
   }

   gtk_item_factory_create_items(item_factory,nmenu_items,menu_items,NULL);
   gtk_window_add_accel_group(GTK_WINDOW(window),accel_group);
   menubar = gtk_item_factory_get_widget(item_factory,"<main>");

   vbox2=gtk_vbox_new(FALSE,0);
   gtk_box_pack_start(GTK_BOX(vbox2),menubar,FALSE,FALSE,0);
   gtk_widget_show_all(menubar);
   UpdateMenus();

   vbox=ClientData.vbox=gtk_vbox_new(FALSE,5);
   frame=gtk_frame_new(_("Stats"));

   table = CreateStatusWidgets(&ClientData.Status);

   gtk_container_add(GTK_CONTAINER(frame),table);

   gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);

   vpaned=gtk_vpaned_new();

   hbox=gtk_hbox_new(FALSE,0);
   adj=(GtkAdjustment *)gtk_adjustment_new(0.0,0.0,100.0,1.0,10.0,10.0);
   text=ClientData.messages=gtk_text_new(NULL,adj);
   gtk_widget_set_usize(text,100,80);
   gtk_text_set_point(GTK_TEXT(text),0);
   gtk_text_set_editable(GTK_TEXT(text),FALSE);
   gtk_text_set_word_wrap(GTK_TEXT(text),TRUE);
   gtk_box_pack_start(GTK_BOX(hbox),text,TRUE,TRUE,0);
   vscroll=gtk_vscrollbar_new(adj);
   gtk_box_pack_start(GTK_BOX(hbox),vscroll,FALSE,FALSE,0);
   gtk_paned_pack1(GTK_PANED(vpaned),hbox,TRUE,TRUE);

   hbox=gtk_hbox_new(FALSE,7);
   buf=InitialCaps(Names.Drugs);
   CreateInventory(hbox,buf,accel_group,TRUE,TRUE,&ClientData.Drug,
                   DealDrugs); g_free(buf);
   clist=ClientData.Drug.HereList;
   gtk_clist_column_titles_active(GTK_CLIST(clist));
   gtk_clist_set_compare_func(GTK_CLIST(clist),DrugSortFunc);
   gtk_signal_connect(GTK_OBJECT(clist),"click-column",
                      GTK_SIGNAL_FUNC(ChangeDrugSort),NULL);

   button=ClientData.JetButton=gtk_button_new_with_label("");
   ClientData.JetAccel=0;
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(JetButtonPressed),NULL);
   gtk_box_pack_start(GTK_BOX(ClientData.Drug.vbbox),button,TRUE,TRUE,0);
   SetJetButtonTitle(accel_group);

   gtk_paned_pack2(GTK_PANED(vpaned),hbox,TRUE,TRUE);

   gtk_box_pack_start(GTK_BOX(vbox),vpaned,TRUE,TRUE,0);

   gtk_box_pack_start(GTK_BOX(vbox2),vbox,TRUE,TRUE,0);
   gtk_container_add(GTK_CONTAINER(window),vbox2);

/* Just show the window, not the vbox - we'll do that when the game starts */
   gtk_widget_show(vbox2);
   gtk_widget_show(window);

   gtk_main();
   return TRUE;
}

void display_intro(GtkWidget *widget,gpointer data) {
   GtkWidget *dialog,*label,*table,*OKButton,*vbox,*hsep;
   gchar *VersionStr;
   const int rows=5,cols=3;
   int i,j;
   gchar *table_data[5][3] = {
      { N_("Drug Dealing and Research"), "Dan Wolf", NULL },
      { N_("Play Testing"), "Phil Davis", "Owen Walsh" },
      { N_("Extensive Play Testing"), "Katherine Holt",
        "Caroline Moore" },
      { N_("Constructive Criticism"), "Andrea Elliot-Smith",
        "Pete Winn" },
      { N_("Unconstructive Criticism"), "James Matthews", NULL }
   };

   dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_title(GTK_WINDOW(dialog),_("About dopewars"));
   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));
   gtk_container_border_width(GTK_CONTAINER(dialog),10);

   vbox=gtk_vbox_new(FALSE,5);

   label=gtk_label_new(
_("Based on John E. Dell's old Drug Wars game, dopewars is a simulation of an\n"
"imaginary drug market.  dopewars is an All-American game which features\n"
"buying, selling, and trying to get past the cops!\n\n"
"The first thing you need to do is pay off your debt to the Loan Shark. After\n"
"that, your goal is to make as much money as possible (and stay alive)! You\n"
"have one month of game time to make your fortune.\n"));
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

   VersionStr=g_strdup_printf(_("Version %s     "
"Copyright (C) 1998-2000  Ben Webb ben@bellatrix.pcl.ox.ac.uk\n"
"dopewars is released under the GNU General Public Licence\n"),VERSION);
   label=gtk_label_new(VersionStr);
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);
   g_free(VersionStr);

   table = gtk_table_new(rows,cols,FALSE);
   gtk_table_set_row_spacings(GTK_TABLE(table),3);
   gtk_table_set_col_spacings(GTK_TABLE(table),3);
   for (i=0;i<rows;i++) for (j=0;j<cols;j++) if (table_data[i][j]) {
      if (j==0) label=gtk_label_new(_(table_data[i][j]));
      else label=gtk_label_new(table_data[i][j]);
      gtk_table_attach_defaults(GTK_TABLE(table),label,j,j+1,i,i+1);
   }
   gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,FALSE,0);

   label=gtk_label_new(
_("\nFor information on the command line options, type dopewars -h at your\n"
"Unix prompt. This will display a help screen, listing the available"
"options."));
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   OKButton=gtk_button_new_with_label(_("OK"));
   gtk_signal_connect_object(GTK_OBJECT(OKButton),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)dialog);

   gtk_box_pack_start(GTK_BOX(vbox),OKButton,FALSE,FALSE,0);
   gtk_container_add(GTK_CONTAINER(dialog),vbox);

   GTK_WIDGET_SET_FLAGS(OKButton,GTK_CAN_DEFAULT);
   gtk_widget_grab_default(OKButton);

   gtk_widget_show_all(dialog);
}

struct StartGameStruct {
   GtkWidget *dialog,*name,*hostname,*port,*antique,*status,*metaserv;
   gint ConnectTag;
};

static void FinishConnect(gpointer data,gint socket,
                          GdkInputCondition condition) {
   gchar *text,*NetworkError;
   struct StartGameStruct *widgets;

   widgets=(struct StartGameStruct *)data;

   gdk_input_remove(widgets->ConnectTag);
   widgets->ConnectTag=0;
   NetworkError=FinishSetupNetwork();
   if (NetworkError) {
      text=g_strdup_printf(_("Status: Could not connect (%s)"),NetworkError);
      gtk_label_set_text(GTK_LABEL(widgets->status),text);
      g_free(text);
   } else {
      gtk_widget_destroy(widgets->dialog);
      StartGame();
   } 
}

static void DoConnect(struct StartGameStruct *widgets) {
   gchar *text,*NetworkError;
   text=g_strdup_printf(_("Status: Attempting to contact %s..."),ServerName);
   gtk_label_set_text(GTK_LABEL(widgets->status),text); g_free(text);

   if (widgets->ConnectTag!=0) {
      gdk_input_remove(widgets->ConnectTag); CloseSocket(ClientSock);
      widgets->ConnectTag=0;
   }
   NetworkError=SetupNetwork(TRUE);
   if (!NetworkError) {
      widgets->ConnectTag=gdk_input_add(ClientSock,GDK_INPUT_WRITE,
                                        FinishConnect,(gpointer)widgets);
   } else {
      text=g_strdup_printf(_("Status: Could not connect (%s)"),NetworkError);
      gtk_label_set_text(GTK_LABEL(widgets->status),text);
      g_free(text);
   }
}

static void ConnectToServer(GtkWidget *widget,struct StartGameStruct *widgets) {
   gchar *text;
   g_free(ServerName);
   ServerName=gtk_editable_get_chars(GTK_EDITABLE(widgets->hostname),0,-1);
   text=gtk_editable_get_chars(GTK_EDITABLE(widgets->port),0,-1);
   Port=atoi(text);
   g_free(text);
   g_free(ClientData.PlayerName);
   ClientData.PlayerName=gtk_editable_get_chars(GTK_EDITABLE(widgets->name),
                                                0,-1);
   if (!ClientData.PlayerName || !ClientData.PlayerName[0]) return;
   DoConnect(widgets);
}

static void StartSinglePlayer(GtkWidget *widget,
                              struct StartGameStruct *widgets) {
   WantAntique=
          gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->antique));
   g_free(ClientData.PlayerName);
   ClientData.PlayerName=gtk_editable_get_chars(GTK_EDITABLE(widgets->name),
                                                0,-1);
   StartGame();
   gtk_widget_destroy(widgets->dialog);
}

static void FillMetaServerList(struct StartGameStruct *widgets) {
   GtkWidget *metaserv;
   ServerData *ThisServer;
   gchar *titles[5];
   GSList *ListPt;
   gint row;

   metaserv=widgets->metaserv;
   gtk_clist_freeze(GTK_CLIST(metaserv));
   gtk_clist_clear(GTK_CLIST(metaserv));


   for (ListPt=ServerList;ListPt;ListPt=g_slist_next(ListPt)) {
      ThisServer=(ServerData *)(ListPt->data);
      titles[0]=ThisServer->Name;
      titles[1]=g_strdup_printf("%d",ThisServer->Port);
      titles[2]=ThisServer->Version;
      titles[3]=g_strdup_printf(_("%d of %d"),ThisServer->CurPlayers,
                                           ThisServer->MaxPlayers);
      titles[4]=ThisServer->Comment;
      row=gtk_clist_append(GTK_CLIST(metaserv),titles);
      gtk_clist_set_row_data(GTK_CLIST(metaserv),row,(gpointer)ThisServer);
      g_free(titles[1]); g_free(titles[3]);
   }
   gtk_clist_thaw(GTK_CLIST(metaserv));
}

static void UpdateMetaServerList(GtkWidget *widget,
                                 struct StartGameStruct *widgets) {
   char *MetaError;
   int HttpSock;
   MetaError=OpenMetaServerConnection(&HttpSock);

   if (MetaError) {
      return;
   }
   ReadMetaServerData(HttpSock);
   CloseMetaServerConnection(HttpSock);
   MetaServerRead=TRUE;
   FillMetaServerList(widgets);
}

static void MetaServerConnect(GtkWidget *widget,
                              struct StartGameStruct *widgets) {
   GList *selection;
   gint row;
   GtkWidget *clist;
   ServerData *ThisServer;

   clist=widgets->metaserv;
   selection=GTK_CLIST(clist)->selection;
   if (selection) {
      row=GPOINTER_TO_INT(selection->data);
      ThisServer=(ServerData *)gtk_clist_get_row_data(GTK_CLIST(clist),row);
      AssignName(&ServerName,ThisServer->Name);
      Port=ThisServer->Port;
      g_free(ClientData.PlayerName);
      ClientData.PlayerName=gtk_editable_get_chars(GTK_EDITABLE(widgets->name),
                                                   0,-1);
      if (!ClientData.PlayerName || !ClientData.PlayerName[0]) return;
      DoConnect(widgets);
   }
}

static void CloseNewGameDia(GtkWidget *widget,
                            struct StartGameStruct *widgets) {
   if (widgets->ConnectTag!=0) {
      gdk_input_remove(widgets->ConnectTag); CloseSocket(ClientSock);
      widgets->ConnectTag=0;
   }
}

void NewGameDialog() {
   GtkWidget *vbox,*vbox2,*hbox,*label,*entry,*notebook,*frame,*button;
   GtkWidget *table,*clist,*scrollwin,*dialog,*hbbox;
   GtkAccelGroup *accel_group;
   guint AccelKey;
   gchar *text;
   gchar *server_titles[5];
   static struct StartGameStruct widgets;

   server_titles[0]=_("Server");
   server_titles[1]=_("Port");
   server_titles[2]=_("Version");
   server_titles[3]=_("Players");
   server_titles[4]=_("Comment");

   widgets.ConnectTag=0;
   widgets.dialog=dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_signal_connect(GTK_OBJECT(dialog),"destroy",
                      GTK_SIGNAL_FUNC(CloseNewGameDia),
                      (gpointer)&widgets);
   
   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));
   accel_group=gtk_accel_group_new();

   gtk_window_set_title(GTK_WINDOW(widgets.dialog),_("New Game"));
   gtk_container_set_border_width(GTK_CONTAINER(widgets.dialog),7);
   gtk_window_add_accel_group(GTK_WINDOW(widgets.dialog),accel_group);

   vbox=gtk_vbox_new(FALSE,7);
   hbox=gtk_hbox_new(FALSE,7);

   label=gtk_label_new("");
   AccelKey=gtk_label_parse_uline(GTK_LABEL(label),
                                  _("Hey dude, what's your _name?"));
   gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);

   entry=widgets.name=gtk_entry_new();
   gtk_widget_add_accelerator(entry,"grab-focus",accel_group,AccelKey,0,
                           GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
   if (ClientData.PlayerName) {
      gtk_entry_set_text(GTK_ENTRY(entry),ClientData.PlayerName);
   }
   gtk_box_pack_start(GTK_BOX(hbox),entry,TRUE,TRUE,0);
   
   gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

   notebook=gtk_notebook_new();

   frame=gtk_frame_new(_("Server"));
   gtk_container_set_border_width(GTK_CONTAINER(frame),4);
   vbox2=gtk_vbox_new(FALSE,7);
   gtk_container_set_border_width(GTK_CONTAINER(vbox2),4);
   table=gtk_table_new(2,2,FALSE);
   gtk_table_set_row_spacings(GTK_TABLE(table),4);
   gtk_table_set_col_spacings(GTK_TABLE(table),4);
   label=gtk_label_new(_("Host name"));
   gtk_table_attach(GTK_TABLE(table),label,0,1,0,1,
                    GTK_SHRINK,GTK_SHRINK,0,0);
   entry=widgets.hostname=gtk_entry_new();
   gtk_entry_set_text(GTK_ENTRY(entry),ServerName);
   gtk_table_attach(GTK_TABLE(table),entry,1,2,0,1,
                    GTK_EXPAND|GTK_SHRINK|GTK_FILL,
                    GTK_EXPAND|GTK_SHRINK|GTK_FILL,0,0);
   label=gtk_label_new(_("Port"));
   gtk_table_attach(GTK_TABLE(table),label,0,1,1,2,
                    GTK_SHRINK,GTK_SHRINK,0,0);
   entry=widgets.port=gtk_entry_new();
   text=g_strdup_printf("%d",Port);
   gtk_entry_set_text(GTK_ENTRY(entry),text);
   g_free(text);
   gtk_table_attach(GTK_TABLE(table),entry,1,2,1,2,
                    GTK_EXPAND|GTK_SHRINK|GTK_FILL,
                    GTK_EXPAND|GTK_SHRINK|GTK_FILL,0,0);

   gtk_box_pack_start(GTK_BOX(vbox2),table,FALSE,FALSE,0);

   button=gtk_button_new_with_label("");
   AccelKey=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                                  _("_Connect"));
   gtk_widget_add_accelerator(button,"clicked",accel_group,AccelKey,0,
                              GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(ConnectToServer),
                      (gpointer)&widgets);
   gtk_box_pack_start(GTK_BOX(vbox2),button,FALSE,FALSE,0);
   gtk_container_add(GTK_CONTAINER(frame),vbox2);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_grab_default(button);

   label=gtk_label_new(_("Server"));
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook),frame,label);
   frame=gtk_frame_new(_("Single player"));
   gtk_container_set_border_width(GTK_CONTAINER(frame),4);
   vbox2=gtk_vbox_new(FALSE,7);
   gtk_container_set_border_width(GTK_CONTAINER(vbox2),4);
   widgets.antique=gtk_check_button_new_with_label("");
   AccelKey=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(widgets.antique)->child),
                                 _("_Antique mode"));
   gtk_widget_add_accelerator(widgets.antique,"clicked",accel_group,AccelKey,0,
                              GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.antique),WantAntique);
   gtk_box_pack_start(GTK_BOX(vbox2),widgets.antique,FALSE,FALSE,0);
   button=gtk_button_new_with_label("");
   AccelKey=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                                  _("_Start single-player game"));
   gtk_widget_add_accelerator(button,"clicked",accel_group,AccelKey,0,
                              GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(StartSinglePlayer),
                      (gpointer)&widgets);
   gtk_box_pack_start(GTK_BOX(vbox2),button,FALSE,FALSE,0);
   gtk_container_add(GTK_CONTAINER(frame),vbox2);
   label=gtk_label_new(_("Single player"));
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook),frame,label);
   frame=gtk_frame_new(_("Metaserver"));
   gtk_container_set_border_width(GTK_CONTAINER(frame),4);

   vbox2=gtk_vbox_new(FALSE,7);
   gtk_container_set_border_width(GTK_CONTAINER(vbox2),4);
   scrollwin=gtk_scrolled_window_new(NULL,NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                  GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
   clist=widgets.metaserv=gtk_clist_new_with_titles(5,server_titles);
   gtk_clist_column_titles_passive(GTK_CLIST(clist));
   gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
   gtk_container_add(GTK_CONTAINER(scrollwin),clist);
   gtk_box_pack_start(GTK_BOX(vbox2),scrollwin,TRUE,TRUE,0);

   hbbox=gtk_hbutton_box_new();
   button=gtk_button_new_with_label("");
   AccelKey=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                                  _("_Update"));
   gtk_widget_add_accelerator(button,"clicked",accel_group,AccelKey,0,
                              GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(UpdateMetaServerList),
                      (gpointer)&widgets);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);

   button=gtk_button_new_with_label("");
   AccelKey=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                                  _("_Connect"));
   gtk_widget_add_accelerator(button,"clicked",accel_group,AccelKey,0,
                              GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(MetaServerConnect),
                      (gpointer)&widgets);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);

   gtk_box_pack_start(GTK_BOX(vbox2),hbbox,FALSE,FALSE,0);
   gtk_container_add(GTK_CONTAINER(frame),vbox2);

   label=gtk_label_new(_("Metaserver"));
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook),frame,label);
   gtk_box_pack_start(GTK_BOX(vbox),notebook,TRUE,TRUE,0);

   label=widgets.status=gtk_label_new(_("Status: Waiting for user input"));
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

   gtk_container_add(GTK_CONTAINER(widgets.dialog),vbox);

   gtk_widget_grab_focus(widgets.name);
   FillMetaServerList(&widgets);

   gtk_widget_show_all(widgets.dialog);
}

static void DestroyMessageBox(GtkWidget *widget,gpointer data) {
   gtk_main_quit();
}

static void MessageBoxCallback(GtkWidget *widget,gpointer data) {
   gint *retval;
   GtkWidget *dialog;
   dialog=gtk_widget_get_ancestor(widget,GTK_TYPE_WINDOW);
   retval=(gint *)gtk_object_get_data(GTK_OBJECT(widget),"retval");
   if (retval) *retval=GPOINTER_TO_INT(data);
   gtk_widget_destroy(dialog);
}

gint MessageBox(GtkWidget *parent,const gchar *Title,
                const gchar *Text,gint Options) {
   GtkWidget *dialog,*button,*label,*vbox,*hbbox,*hsep;
   GtkAccelGroup *accel_group;
   gint i;
   static gint retval;
   guint AccelKey;
   gchar *ButtonData[MB_MAX] = { N_("OK"), N_("Cancel"),
                                 N_("_Yes"), N_("_No") };

   dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   accel_group=gtk_accel_group_new();
   gtk_window_add_accel_group(GTK_WINDOW(dialog),accel_group);
   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);
   if (parent) gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                            GTK_WINDOW(parent));
   gtk_signal_connect(GTK_OBJECT(dialog),"destroy",
                      GTK_SIGNAL_FUNC(DestroyMessageBox),NULL);
   if (Title) gtk_window_set_title(GTK_WINDOW(dialog),Title);

   vbox=gtk_vbox_new(FALSE,7);

   if (Text) {
      label=gtk_label_new(Text);
      gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);
   }

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   retval=MB_CANCEL;

   hbbox=gtk_hbutton_box_new();
   for (i=0;i<MB_MAX;i++) {
      if (Options & (1<<i)) {
         button=gtk_button_new_with_label("");
         AccelKey=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                                        _(ButtonData[i]));
         if (AccelKey) gtk_widget_add_accelerator(button,"clicked",
                                  accel_group,AccelKey,0,
                                  GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
         gtk_object_set_data(GTK_OBJECT(button),"retval",&retval);
         gtk_signal_connect(GTK_OBJECT(button),"clicked",
                            GTK_SIGNAL_FUNC(MessageBoxCallback),
                            GINT_TO_POINTER(1<<i));
         gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);
      }
   }
   gtk_box_pack_start(GTK_BOX(vbox),hbbox,TRUE,TRUE,0);
   gtk_container_add(GTK_CONTAINER(dialog),vbox);
   gtk_widget_show_all(dialog);
   gtk_main();
   return retval;
}

static void SendDoneMessage(GtkWidget *widget,gpointer data) {
   SendClientMessage(ClientData.Play,C_NONE,C_DONE,NULL,NULL,ClientData.Play);
}

static void TransferPayAll(GtkWidget *widget,GtkWidget *dialog) {
   gchar *text;
   text=pricetostr(ClientData.Play->Debt);
   SendClientMessage(ClientData.Play,C_NONE,C_PAYLOAN,NULL,
                     text,ClientData.Play);
   g_free(text);
   gtk_widget_destroy(dialog);
}

static void TransferOK(GtkWidget *widget,GtkWidget *dialog) {
   gpointer Debt;
   GtkWidget *deposit,*entry;
   gchar *text;
   price_t money;

   Debt=gtk_object_get_data(GTK_OBJECT(dialog),"debt");
   entry=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(dialog),"entry"));
   text=gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
   money=strtoprice(text);
   g_free(text);

   if (money<0) money=0;
   if (Debt) {
      if (money>ClientData.Play->Debt) money=ClientData.Play->Debt;
   } else {
      deposit=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(dialog),"deposit"));
      if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(deposit))) {
         money=-money;
      }
      if (-money>ClientData.Play->Bank) {
         MessageBox(dialog,"Bank",
                    _("There isn't that much money in the bank..."),
                    MB_OK);
         return;
      }
   }
   if (money>ClientData.Play->Cash) {
      MessageBox(dialog,Debt ? "Pay loan" : "Bank",
                 _("You don't have that much money!"),MB_OK);
      return;
   }
   text=pricetostr(money);
   SendClientMessage(ClientData.Play,C_NONE,
                     Debt ? C_PAYLOAN : C_DEPOSIT,NULL,text,ClientData.Play);
   g_free(text);
   gtk_widget_destroy(dialog);
}

void TransferDialog(gboolean Debt) {
   GtkWidget *dialog,*button,*label,*radio,*table,*vbox,*hbbox,*hsep,*entry;
   gchar *text,*prstr;
   GSList *group;

   dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_signal_connect(GTK_OBJECT(dialog),"destroy",
                      GTK_SIGNAL_FUNC(SendDoneMessage),NULL);
   gtk_window_set_title(GTK_WINDOW(dialog),Debt ? Names.LoanSharkName :
                                                  Names.BankName);
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);
   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   vbox=gtk_vbox_new(FALSE,7);
   table=gtk_table_new(4,3,FALSE);
   gtk_table_set_row_spacings(GTK_TABLE(table),4);
   gtk_table_set_col_spacings(GTK_TABLE(table),4);

   prstr=FormatPrice(ClientData.Play->Cash);
   text=g_strdup_printf(_("Cash: %s"),prstr);
   label=gtk_label_new(text);
   g_free(text); g_free(prstr);
   gtk_table_attach_defaults(GTK_TABLE(table),label,0,3,0,1);

   if (Debt) {
      prstr=FormatPrice(ClientData.Play->Debt);
      text=g_strdup_printf(_("Debt: %s"),prstr);
   } else {
      prstr=FormatPrice(ClientData.Play->Bank);
      text=g_strdup_printf(_("Bank: %s"),prstr);
   }
   label=gtk_label_new(text);
   g_free(text); g_free(prstr);
   gtk_table_attach_defaults(GTK_TABLE(table),label,0,3,1,2);

   gtk_object_set_data(GTK_OBJECT(dialog),"debt",GINT_TO_POINTER(Debt));
   if (Debt) {
      label=gtk_label_new(_("Pay back:"));
      gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,2,4);
   } else {
      radio=gtk_radio_button_new_with_label(NULL,_("Deposit"));
      gtk_object_set_data(GTK_OBJECT(dialog),"deposit",radio);
      group=gtk_radio_button_group(GTK_RADIO_BUTTON(radio));
      gtk_table_attach_defaults(GTK_TABLE(table),radio,0,1,2,3);
      radio=gtk_radio_button_new_with_label(group,_("Withdraw"));
      gtk_table_attach_defaults(GTK_TABLE(table),radio,0,1,3,4);
   }
   label=gtk_label_new("$");
   gtk_table_attach_defaults(GTK_TABLE(table),label,1,2,2,4);
   entry=gtk_entry_new();
   gtk_entry_set_text(GTK_ENTRY(entry),"0");
   gtk_object_set_data(GTK_OBJECT(dialog),"entry",entry);
   gtk_signal_connect(GTK_OBJECT(entry),"activate",
                      GTK_SIGNAL_FUNC(TransferOK),dialog);
   gtk_table_attach_defaults(GTK_TABLE(table),entry,2,3,2,4);

   gtk_box_pack_start(GTK_BOX(vbox),table,TRUE,TRUE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   hbbox=gtk_hbutton_box_new();
   button=gtk_button_new_with_label(_("OK"));
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(TransferOK),dialog);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);

   if (Debt && ClientData.Play->Cash>=ClientData.Play->Debt) {
      button=gtk_button_new_with_label(_("Pay all"));
      gtk_signal_connect(GTK_OBJECT(button),"clicked",
                         GTK_SIGNAL_FUNC(TransferPayAll),dialog);
      gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);
   }
   button=gtk_button_new_with_label(_("Cancel"));
   gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)dialog);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);
   gtk_box_pack_start(GTK_BOX(vbox),hbbox,FALSE,FALSE,0);

   gtk_container_add(GTK_CONTAINER(dialog),vbox);

   gtk_widget_show_all(dialog);
}

void ListPlayers(GtkWidget *widget,gpointer data) {
   GtkWidget *dialog,*clist,*button,*vbox,*hsep;

   if (IsShowingPlayerList) return;
   dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_title(GTK_WINDOW(dialog),_("Player List"));
   gtk_window_set_default_size(GTK_WINDOW(dialog),200,180);
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);

   IsShowingPlayerList=TRUE;
   gtk_window_set_modal(GTK_WINDOW(dialog),FALSE);
   gtk_object_set_data(GTK_OBJECT(dialog),"IsShowing",
                       (gpointer)&IsShowingPlayerList);
   gtk_signal_connect(GTK_OBJECT(dialog),"destroy",
                      GTK_SIGNAL_FUNC(DestroyShowing),NULL);

   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   vbox=gtk_vbox_new(FALSE,7);

   clist=ClientData.PlayerList=CreatePlayerList();
   UpdatePlayerList(clist,FALSE);
   gtk_box_pack_start(GTK_BOX(vbox),clist,TRUE,TRUE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   button=gtk_button_new_with_label("OK");
   gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)dialog);
   gtk_box_pack_start(GTK_BOX(vbox),button,FALSE,FALSE,0);
   gtk_container_add(GTK_CONTAINER(dialog),vbox);
   gtk_widget_show_all(dialog);
}

struct TalkStruct {
   GtkWidget *dialog,*clist,*entry,*checkbutton;
};

static void TalkSend(GtkWidget *widget,struct TalkStruct *TalkData) {
   gboolean AllPlayers;
   gchar *text;
   GString *msg;
   GList *selection;
   gint row;
   Player *Play;

   AllPlayers=
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(TalkData->checkbutton));
   text=gtk_editable_get_chars(GTK_EDITABLE(TalkData->entry),0,-1);
   gtk_editable_delete_text(GTK_EDITABLE(TalkData->entry),0,-1);
   if (!text) return;

   msg=g_string_new("");

   if (AllPlayers) {
      SendClientMessage(ClientData.Play,C_NONE,C_MSG,NULL,text,ClientData.Play);
      g_string_sprintf(msg,"%s: %s",GetPlayerName(ClientData.Play),text);
      PrintMessage(msg->str);
   } else {
      for(selection=GTK_CLIST(TalkData->clist)->selection;selection;
          selection=g_list_next(selection)) {
         row=GPOINTER_TO_INT(selection->data);
         Play=(Player *)gtk_clist_get_row_data(GTK_CLIST(TalkData->clist),row);
         if (Play) {
            SendClientMessage(ClientData.Play,C_NONE,C_MSGTO,Play,
                              text,ClientData.Play);
            g_string_sprintf(msg,"%s->%s: %s",GetPlayerName(ClientData.Play),
                             GetPlayerName(Play),text);
            PrintMessage(msg->str);
         }
      }
   }
   g_free(text);
   g_string_free(msg,TRUE);
}

void TalkToAll(GtkWidget *widget,gpointer data) {
   TalkDialog(TRUE);
}

void TalkToPlayers(GtkWidget *widget,gpointer data) {
   TalkDialog(FALSE);
}

void TalkDialog(gboolean TalkToAll) {
   GtkWidget *dialog,*clist,*button,*entry,*label,*vbox,*hsep,
             *checkbutton,*hbbox;
   static struct TalkStruct TalkData;

   if (IsShowingTalkList) return;
   dialog=TalkData.dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_title(GTK_WINDOW(dialog),_("Talk to player(s)"));
   gtk_window_set_default_size(GTK_WINDOW(dialog),200,190);
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);

   IsShowingTalkList=TRUE;
   gtk_window_set_modal(GTK_WINDOW(dialog),FALSE);
   gtk_object_set_data(GTK_OBJECT(dialog),"IsShowing",
                       (gpointer)&IsShowingTalkList);
   gtk_signal_connect(GTK_OBJECT(dialog),"destroy",
                      GTK_SIGNAL_FUNC(DestroyShowing),NULL);

   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   vbox=gtk_vbox_new(FALSE,7);

   clist=TalkData.clist=ClientData.TalkList=CreatePlayerList();
   UpdatePlayerList(clist,FALSE);
   gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_MULTIPLE);
   gtk_box_pack_start(GTK_BOX(vbox),clist,TRUE,TRUE,0);

   checkbutton=TalkData.checkbutton=
               gtk_check_button_new_with_label(_("Talk to all players"));
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),TalkToAll);
   gtk_box_pack_start(GTK_BOX(vbox),checkbutton,FALSE,FALSE,0);

   label=gtk_label_new(_("Message:-"));
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

   entry=TalkData.entry=gtk_entry_new();
   gtk_signal_connect(GTK_OBJECT(entry),"activate",
                      GTK_SIGNAL_FUNC(TalkSend),
                      (gpointer)&TalkData);
   gtk_box_pack_start(GTK_BOX(vbox),entry,FALSE,FALSE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   hbbox=gtk_hbutton_box_new();
   button=gtk_button_new_with_label(_("Send"));
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(TalkSend),
                      (gpointer)&TalkData);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);

   button=gtk_button_new_with_label(_("Close"));
   gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)dialog);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);

   gtk_box_pack_start(GTK_BOX(vbox),hbbox,FALSE,FALSE,0);

   gtk_container_add(GTK_CONTAINER(dialog),vbox);
   gtk_widget_show_all(dialog);
}

GtkWidget *CreatePlayerList() {
   GtkWidget *clist;
   gchar *text[1];

   text[0]="Name";
   clist=gtk_clist_new_with_titles(1,text);
   gtk_clist_column_titles_passive(GTK_CLIST(clist));
   gtk_clist_set_column_auto_resize(GTK_CLIST(clist),0,TRUE);
   return clist;
}

void UpdatePlayerList(GtkWidget *clist,gboolean IncludeSelf) {
   GSList *list;
   gchar *text[1];
   gint row;
   Player *Play;
   gtk_clist_freeze(GTK_CLIST(clist));
   gtk_clist_clear(GTK_CLIST(clist));
   for (list=FirstClient;list;list=g_slist_next(list)) {
      Play=(Player *)list->data;
      if (IncludeSelf || Play!=ClientData.Play) {
         text[0]=GetPlayerName(Play);
         row=gtk_clist_append(GTK_CLIST(clist),text);
         gtk_clist_set_row_data(GTK_CLIST(clist),row,Play);
      }
   }
   gtk_clist_thaw(GTK_CLIST(clist));
}

static void ErrandOK(GtkWidget *widget,GtkWidget *clist) {
   GList *selection;
   Player *Play;
   gint row;
   GtkWidget *dialog;
   gint ErrandType;
   dialog=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(widget),"dialog"));
   ErrandType=GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),
                                                  "errandtype"));
   selection=GTK_CLIST(clist)->selection;
   if (selection) {
      row=GPOINTER_TO_INT(selection->data);
      Play=(Player *)gtk_clist_get_row_data(GTK_CLIST(clist),row);
      if (ErrandType==ET_SPY) {
         SendClientMessage(ClientData.Play,C_NONE,C_SPYON,Play,
                           NULL,ClientData.Play);
      } else {
         SendClientMessage(ClientData.Play,C_NONE,C_TIPOFF,Play,
                           NULL,ClientData.Play);
      }
      gtk_widget_destroy(dialog);
   }
}

void SpyOnPlayer(GtkWidget *widget,gpointer data) {
   ErrandDialog(ET_SPY);
}

void TipOff(GtkWidget *widget,gpointer data) {
   ErrandDialog(ET_TIPOFF);
}

void ErrandDialog(gint ErrandType) {
   GtkWidget *dialog,*clist,*button,*vbox,*hbbox,*hsep,*label;
   gchar *text;

   dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);

   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   vbox=gtk_vbox_new(FALSE,7);

   if (ErrandType==ET_SPY) {
      gtk_window_set_title(GTK_WINDOW(dialog),_("Spy On Player"));
      text=g_strdup_printf(
_("Please choose the player to spy on. Your %s will\n"
"then offer his services to the player, and if successful,\n"
"you will be able to view the player's stats with the\n"
"\"Get spy reports\" menu. Remember that the %s will leave\n"
"you, so any %s or %s that he's carrying may be lost!"),
Names.Bitch,Names.Bitch,Names.Guns,Names.Drugs);
      label=gtk_label_new(text); g_free(text);
   } else {
      gtk_window_set_title(GTK_WINDOW(dialog),_("Tip Off The Cops"));
      text=g_strdup_printf(
_("Please choose the player to tip off the cops to. Your %s will\n"
"help the cops to attack that player, and then report back to you\n"
"on the encounter. Remember that the %s will leave you temporarily,\n"
"so any %s or %s that he's carrying may be lost!"),
Names.Bitch,Names.Bitch,Names.Guns,Names.Drugs);
      label=gtk_label_new(text); g_free(text);
   }

   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

   clist=ClientData.PlayerList=CreatePlayerList();
   UpdatePlayerList(clist,FALSE);
   gtk_box_pack_start(GTK_BOX(vbox),clist,TRUE,TRUE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   hbbox=gtk_hbutton_box_new();
   button=gtk_button_new_with_label(_("OK"));
   gtk_object_set_data(GTK_OBJECT(button),"dialog",dialog);
   gtk_object_set_data(GTK_OBJECT(button),"errandtype",
                       GINT_TO_POINTER(ErrandType));
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(ErrandOK),
                      (gpointer)clist);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);
   button=gtk_button_new_with_label(_("Cancel"));
   gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)dialog);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);

   gtk_box_pack_start(GTK_BOX(vbox),hbbox,FALSE,FALSE,0);
   gtk_container_add(GTK_CONTAINER(dialog),vbox);
   gtk_widget_show_all(dialog);
}

void SackBitch(GtkWidget *widget,gpointer data) {
   char *caps,*title,*text;
   caps=InitialCaps(Names.Bitch);
   title=g_strdup_printf(_("Sack %s"),caps);
   text=g_strdup_printf(_("Are you sure? (Any %s or %s carried\n"
                        "by this %s may be lost!)"),Names.Guns,
                        Names.Drugs,Names.Bitch);
   if (MessageBox(ClientData.window,title,text,MB_YES|MB_NO)==MB_YES) {
      SendClientMessage(ClientData.Play,C_NONE,C_SACKBITCH,NULL,NULL,
                        ClientData.Play);
   }
   g_free(caps); g_free(text); g_free(title);
}

void CreateInventory(GtkWidget *hbox,gchar *Objects,GtkAccelGroup *accel_group,
                     gboolean CreateButtons,gboolean CreateHere,
                     struct InventoryWidgets *widgets,GtkSignalFunc CallBack) {
   GtkWidget *scrollwin,*clist,*vbbox,*frame[2],*button[3];
   gint i,mini;
   guint accel_key;
   GString *text;
   gchar *titles[2][2];
   gchar *button_text[3];
   gpointer button_type[3]  = { BT_BUY, BT_SELL, BT_DROP };

   titles[0][0]=titles[1][0]=_("Name");
   titles[0][1]=_("Price");
   titles[1][1]=_("Number");

   button_text[0]=_("_Buy ->");
   button_text[1]=_("<- _Sell");
   button_text[2]=_("_Drop <-");

   text=g_string_new("");

   if (CreateHere) {
      g_string_sprintf(text,_("%s here"),Objects);
      widgets->HereFrame=frame[0]=gtk_frame_new(text->str);
   }
   g_string_sprintf(text,_("%s carried"),Objects);
   widgets->CarriedFrame=frame[1]=gtk_frame_new(text->str);

   widgets->HereList=widgets->CarriedList=NULL;
   if (CreateHere) mini=0; else mini=1;
   for (i=mini;i<2;i++) {
      gtk_container_set_border_width(GTK_CONTAINER(frame[i]),5);

      scrollwin=gtk_scrolled_window_new(NULL,NULL);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                     GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
      clist=gtk_clist_new_with_titles(2,titles[i]);
      gtk_clist_set_column_auto_resize(GTK_CLIST(clist),0,TRUE);
      gtk_clist_set_column_auto_resize(GTK_CLIST(clist),1,TRUE);
      gtk_clist_column_titles_passive(GTK_CLIST(clist));
      gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
      gtk_clist_set_auto_sort(GTK_CLIST(clist),FALSE);
      gtk_container_add(GTK_CONTAINER(scrollwin),clist);
      gtk_container_add(GTK_CONTAINER(frame[i]),scrollwin);
      if (i==0) widgets->HereList=clist; else widgets->CarriedList=clist;
   }
   if (CreateHere) gtk_box_pack_start(GTK_BOX(hbox),frame[0],TRUE,TRUE,0);

   if (CreateButtons) {
      widgets->vbbox=vbbox=gtk_vbutton_box_new();

      for (i=0;i<3;i++) {
         button[i]=gtk_button_new_with_label("");
         accel_key=gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button[i])->child),
                                         button_text[i]);
         gtk_widget_add_accelerator(button[i],"clicked",accel_group,accel_key,0,
                                  GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
         if (CallBack) gtk_signal_connect(GTK_OBJECT(button[i]),"clicked",
                                          GTK_SIGNAL_FUNC(CallBack),
                                          button_type[i]);
         gtk_box_pack_start(GTK_BOX(vbbox),button[i],TRUE,TRUE,0);
      }
      widgets->BuyButton=button[0];
      widgets->SellButton=button[1];
      widgets->DropButton=button[2];
      gtk_box_pack_start(GTK_BOX(hbox),vbbox,FALSE,FALSE,0);
   } else widgets->vbbox=NULL;

   gtk_box_pack_start(GTK_BOX(hbox),frame[1],TRUE,TRUE,0);
   g_string_free(text,TRUE);
}

void DestroyShowing(GtkWidget *widget,gpointer data) {
   gboolean *IsShowing;

   IsShowing=(gboolean *)gtk_object_get_data(GTK_OBJECT(widget),"IsShowing");
   if (IsShowing) *IsShowing=FALSE;
}

gint DisallowDelete(GtkWidget *widget,GdkEvent *event,gpointer data) {
   return(TRUE);
}

static void NewNameOK(GtkWidget *widget,GtkWidget *window) {
   GtkWidget *entry;
   gchar *text;

   entry=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(window),"entry"));
   text=gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
   if (text[0]) {
      SetPlayerName(ClientData.Play,text);
      SendClientMessage(NULL,C_NONE,C_NAME,NULL,text,ClientData.Play);
      gtk_widget_destroy(window);
   }
   g_free(text);
}

void NewNameDialog() {
   GtkWidget *window,*button,*hsep,*vbox,*label,*entry;

   window=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_title(GTK_WINDOW(window),_("Change Name"));
   gtk_window_set_modal(GTK_WINDOW(window),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(window),
                                GTK_WINDOW(ClientData.window));
   gtk_container_set_border_width(GTK_CONTAINER(window),7);
   gtk_signal_connect(GTK_OBJECT(window),"delete_event",
                      GTK_SIGNAL_FUNC(DisallowDelete),NULL);

   vbox=gtk_vbox_new(FALSE,7);

   label=gtk_label_new(_("Unfortunately, somebody else is already "
                       "using \"your\" name. Please change it:-"));
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

   entry=gtk_entry_new();
   gtk_object_set_data(GTK_OBJECT(window),"entry",entry);
   gtk_signal_connect(GTK_OBJECT(entry),"activate",
                      GTK_SIGNAL_FUNC(NewNameOK),window);
   gtk_entry_set_text(GTK_ENTRY(entry),GetPlayerName(ClientData.Play));
   gtk_box_pack_start(GTK_BOX(vbox),entry,FALSE,FALSE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   button=gtk_button_new_with_label(_("OK"));
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(NewNameOK),window);
   gtk_box_pack_start(GTK_BOX(vbox),button,FALSE,FALSE,0);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_grab_default(button);
   
   gtk_container_add(GTK_CONTAINER(window),vbox);
   gtk_widget_show_all(window);
}

void GunShopDialog() {
   GtkWidget *window,*button,*hsep,*vbox,*hbox;
   GtkAccelGroup *accel_group;
   gchar *text;

   window=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_default_size(GTK_WINDOW(window),600,190);
   gtk_signal_connect(GTK_OBJECT(window),"destroy",
                      GTK_SIGNAL_FUNC(SendDoneMessage),NULL);
   accel_group=gtk_accel_group_new();
   gtk_window_add_accel_group(GTK_WINDOW(window),accel_group);
   gtk_window_set_title(GTK_WINDOW(window),Names.GunShopName);
   gtk_window_set_modal(GTK_WINDOW(window),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(window),
                                GTK_WINDOW(ClientData.window));
   gtk_container_set_border_width(GTK_CONTAINER(window),7);
   IsShowingGunShop=TRUE;
   gtk_object_set_data(GTK_OBJECT(window),"IsShowing",
                       (gpointer)&IsShowingGunShop);
   gtk_signal_connect(GTK_OBJECT(window),"destroy",
                      GTK_SIGNAL_FUNC(DestroyShowing),NULL);

   vbox=gtk_vbox_new(FALSE,7);

   hbox=gtk_hbox_new(FALSE,7);
   text=InitialCaps(Names.Guns);
   CreateInventory(hbox,text,accel_group,TRUE,TRUE,&ClientData.Gun,
                   DealGuns); g_free(text);

   gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

   button=gtk_button_new_with_label(_("Done"));
   gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)window);
   gtk_box_pack_start(GTK_BOX(vbox),button,FALSE,FALSE,0);

   gtk_container_add(GTK_CONTAINER(window),vbox);

   UpdateInventory(&ClientData.Gun,ClientData.Play->Guns,NumGun,FALSE);
   gtk_widget_show_all(window);
}

void UpdatePlayerLists() {
   if (IsShowingPlayerList) UpdatePlayerList(ClientData.PlayerList,FALSE);
   if (IsShowingTalkList) UpdatePlayerList(ClientData.TalkList,FALSE);
}

void GetSpyReports(GtkWidget *Widget,gpointer data) {
   SendClientMessage(ClientData.Play,C_NONE,C_CONTACTSPY,NULL,NULL,
                     ClientData.Play);
}

static void DestroySpyReports(GtkWidget *widget,gpointer data) {
   SpyReportsDialog=NULL;
}

static void CreateSpyReports() {
   GtkWidget *window,*button,*vbox,*notebook;
   GtkAccelGroup *accel_group;

   SpyReportsDialog=window=gtk_window_new(GTK_WINDOW_DIALOG);
   accel_group=gtk_accel_group_new();
   gtk_object_set_data(GTK_OBJECT(window),"accel_group",accel_group);
   gtk_window_add_accel_group(GTK_WINDOW(window),accel_group);
   gtk_window_set_title(GTK_WINDOW(window),_("Spy reports"));
   gtk_window_set_modal(GTK_WINDOW(window),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(window),
                                GTK_WINDOW(ClientData.window));
   gtk_container_set_border_width(GTK_CONTAINER(window),7);
   gtk_signal_connect(GTK_OBJECT(window),"destroy",
                      GTK_SIGNAL_FUNC(DestroySpyReports),NULL);

   vbox=gtk_vbox_new(FALSE,5);
   notebook=gtk_notebook_new();
   gtk_object_set_data(GTK_OBJECT(window),"notebook",notebook);

   gtk_box_pack_start(GTK_BOX(vbox),notebook,TRUE,TRUE,0);

   button=gtk_button_new_with_label(_("Close"));
   gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)window);
   gtk_box_pack_start(GTK_BOX(vbox),button,FALSE,FALSE,0);

   gtk_container_add(GTK_CONTAINER(window),vbox);

   gtk_widget_show_all(window);
}

void DisplaySpyReports(Player *Play) {
   GtkWidget *dialog,*notebook,*vbox,*hbox,*frame,*label,*table;
   GtkAccelGroup *accel_group;
   gchar *caps;
   struct StatusWidgets Status;
   struct InventoryWidgets SpyDrugs,SpyGuns;

   if (!SpyReportsDialog) CreateSpyReports();
   dialog=SpyReportsDialog;
   notebook=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(dialog),"notebook"));
   accel_group=(GtkAccelGroup *)(gtk_object_get_data(GTK_OBJECT(dialog),
                                                     "accel_group"));
   vbox=gtk_vbox_new(FALSE,5);
   frame=gtk_frame_new("Stats");
   gtk_container_set_border_width(GTK_CONTAINER(frame),4);
   table=CreateStatusWidgets(&Status);
   gtk_container_add(GTK_CONTAINER(frame),table);
   gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);

   hbox=gtk_hbox_new(FALSE,5);
   caps=InitialCaps(Names.Drugs);
   CreateInventory(hbox,caps,accel_group,FALSE,FALSE,&SpyDrugs,NULL);
   g_free(caps);
   caps=InitialCaps(Names.Guns);
   CreateInventory(hbox,caps,accel_group,FALSE,FALSE,&SpyGuns,NULL);
   g_free(caps);

   gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);
   label=gtk_label_new(GetPlayerName(Play));

   DisplayStats(Play,&Status);
   UpdateInventory(&SpyDrugs,Play->Drugs,NumDrug,TRUE);
   UpdateInventory(&SpyGuns,Play->Guns,NumGun,FALSE);

   gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,label);

   gtk_widget_show_all(notebook);
}

#else

#include <glib.h>
#include "dopewars.h"

char GtkLoop(int *argc,char **argv[],char ReturnOnFail) {
   if (!ReturnOnFail) {
      g_print(_("No GTK+ client available - rebuild the binary passing the\n"
              "--enable-gtk-client option to configure, or use the curses\n"
              "client (if available) instead!\n"));
   }
   return FALSE;
}

#endif /* GTK_CLIENT */
