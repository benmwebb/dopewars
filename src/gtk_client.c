/* gtk_client.c  dopewars client using the GTK+ toolkit                 */
/* Copyright (C)  1998-2001  Ben Webb                                   */
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

#ifdef GUI_CLIENT

#include <stdlib.h>
#include <string.h>

#include "dopeos.h"
#include "dopewars.h"
#include "gtk_client.h"
#include "message.h"
#include "serverside.h"
#include "tstring.h"
#include "gtkport.h"

#define BT_BUY  (GINT_TO_POINTER(1))
#define BT_SELL (GINT_TO_POINTER(2))
#define BT_DROP (GINT_TO_POINTER(3))

#define ET_SPY    0
#define ET_TIPOFF 1

/* Which notebook page to display in the New Game dialog */
static gint NewGameType=0;

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
static void CompleteHighScoreDialog(gboolean AtEnd);
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
/* The names of the the menus and their items in the GTK+ client */
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

static gchar *MenuTranslate(const gchar *path,gpointer func_data) {
/* Translate menu items, using gettext */
   return _(path);
}

static void LogMessage(const gchar *log_domain,GLogLevelFlags log_level,
                       const gchar *message,gpointer user_data) {
   GtkMessageBox(NULL,message,
/* Titles of the message boxes for warnings and errors */
                 log_level&G_LOG_LEVEL_WARNING ? _("Warning") : _("Message"),
                 MB_OK);
}

void QuitGame(GtkWidget *widget,gpointer data) {
   if (!InGame ||
      GtkMessageBox(ClientData.window,
/* Prompt in 'quit game' dialog */
                    _("Abandon current game?"),
/* Title of 'quit game' dialog */
                    _("Quit Game"),MB_YESNO)==IDYES) {
      gtk_main_quit();
   }
}

void DestroyGtk(GtkWidget *widget,gpointer data) {
   gtk_main_quit();
}

gint MainDelete(GtkWidget *widget,GdkEvent *event,gpointer data) {
   return (InGame && GtkMessageBox(ClientData.window,_("Abandon current game?"),
                                   _("Quit Game"),MB_YESNO)==IDNO);
}


void NewGame(GtkWidget *widget,gpointer data) {
   if (InGame) {
      if (GtkMessageBox(ClientData.window,
                        _("Abandon current game?"),
/* Title of 'stop game to start a new game' dialog */
                        _("Start new game"),MB_YESNO)==IDYES) EndGame();
      else return;
   }
   NewGameDialog();
}

void ListScores(GtkWidget *widget,gpointer data) {
   SendClientMessage(ClientData.Play,C_NONE,C_REQUESTSCORE,NULL,NULL);
}

void ListInventory(GtkWidget *widget,gpointer data) {
   GtkWidget *window,*button,*hsep,*vbox,*hbox;
   GtkAccelGroup *accel_group;

   if (IsShowingInventory) return;
   window=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_default_size(GTK_WINDOW(window),550,120);
   accel_group=gtk_accel_group_new();
   gtk_window_add_accel_group(GTK_WINDOW(window),accel_group);

/* Title of inventory window */
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
   CreateInventory(hbox,Names.Drugs,accel_group,FALSE,FALSE,
                   &ClientData.InvenDrug,NULL);
   CreateInventory(hbox,Names.Guns,accel_group,FALSE,FALSE,
                   &ClientData.InvenGun,NULL);

   gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);

/* Caption of the button to close a dialog */
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
   gboolean ReadOK;
   if (condition&GDK_INPUT_WRITE) {
      WriteConnectionBufferToWire(ClientData.Play);
      if (ClientData.Play->WriteBuf.DataPresent==0) {
         SetSocketWriteTest(ClientData.Play,FALSE);
      }
   }
   if (condition&GDK_INPUT_READ) {
      ReadOK=ReadConnectionBufferFromWire(ClientData.Play);
      while ((pt=ReadFromConnectionBuffer(ClientData.Play))!=NULL) {
         HandleClientMessage(pt,ClientData.Play); g_free(pt);
      }
      if (!ReadOK) {
         if (Network) gdk_input_remove(ClientData.GdkInputTag);
         if (InGame) {
            g_warning(_("Connection to server lost - switching to "
                      "single player mode"));
            SwitchToSinglePlayer(ClientData.Play);
         }
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
   gchar *text;
   gboolean Handled;
   GtkWidget *MenuItem;
   GSList *list;

   if (ProcessMessage(pt,Play,&From,&AICode,&Code,&Data,FirstClient)==-1) {
      return;
   }

   Handled=HandleGenericClientMessage(From,AICode,Code,Play,Data,&DisplayMode);
   switch(Code) {
      case C_STARTHISCORE:
         PrepareHighScoreDialog(); break;
      case C_HISCORE:
         AddScoreToDialog(Data); break;
      case C_ENDHISCORE:
         CompleteHighScoreDialog((strcmp(Data,"end")==0));
         break;
      case C_PRINTMESSAGE:
         PrintMessage(Data);
         break;
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
         UpdateMenus();
         break;
      case C_LEAVE:
         if (From!=&Noone) {
            text=g_strdup_printf(_("%s has left the game."),Data);
            PrintMessage(text); g_free(text);
            UpdatePlayerLists();
            UpdateMenus();
         }
         break;
      case C_QUESTION:
         QuestionDialog(Data,From==&Noone ? NULL : From);
         break;
      case C_SUBWAYFLASH:
         DisplayFightMessage(NULL);
         for (list=FirstClient;list;list=g_slist_next(list)) {
            tmp=(Player *)list->data;
            tmp->Flags &= ~FIGHTING;
         }
/* Message displayed when the player "jets" to a new location */
         text=dpg_strdup_printf(_("Jetting to %tde"),
                                Location[(int)Play->IsAt].Name);
         PrintMessage(text); g_free(text);
         break;
      case C_ENDLIST:
         MenuItem=gtk_item_factory_get_widget(ClientData.Menu,
                                              "<main>/Errands/Spy");

/* Text to update the Errands/Spy menu item with the price for spying */
         text=dpg_strdup_printf(_("_Spy\t(%P)"),Prices.Spy);
         SetAccelerator(MenuItem,text,NULL,NULL,NULL);
         g_free(text);

/* Text to update the Errands/Tipoff menu item with the price for a tipoff */
         text=dpg_strdup_printf(_("_Tipoff\t(%P)"),Prices.Tipoff);
         MenuItem=gtk_item_factory_get_widget(ClientData.Menu,
                                              "<main>/Errands/Tipoff");
         SetAccelerator(MenuItem,text,NULL,NULL,NULL);
         g_free(text);
         if (FirstClient->next) ListPlayers(NULL,NULL);
         UpdateMenus();
         break;
      case C_UPDATE:
         if (From==&Noone) {
            ReceivePlayerData(Play,Data,Play);
            UpdateStatus(Play,TRUE);
         } else {
            ReceivePlayerData(Play,Data,From);
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
   HiScoreDialog.table=table=gtk_table_new(NUMHISCORE,4,FALSE);
   gtk_table_set_row_spacings(GTK_TABLE(table),5);
   gtk_table_set_col_spacings(GTK_TABLE(table),30);

   gtk_box_pack_start(GTK_BOX(vbox),table,TRUE,TRUE,0);
   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);
   gtk_container_add(GTK_CONTAINER(dialog),vbox);
   gtk_widget_show_all(dialog);
}

void AddScoreToDialog(char *Data) {
   GtkWidget *label;
   char *cp;
   gchar **spl1,**spl2;
   int index,slen;
   gboolean bold;

   cp=Data;
   index=GetNextInt(&cp,0);
   if (!cp || strlen(cp)<3) return;

   bold = (*cp=='B');   /* Is this score "our" score? (Currently ignored) */

/* Step past the 'bold' character, and the initial '>' (if present) */
   cp+=2;
   g_strchug(cp);

/* Get the first word - the score */
   spl1 = g_strsplit(cp," ",1);
   if (!spl1 || !spl1[0] || !spl1[1]) {
/* Error - the high score from the server is invalid */
      g_warning(_("Corrupt high score!"));
      g_strfreev(spl1);
      return;
   }
   label=gtk_label_new(spl1[0]);
   gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
   gtk_table_attach_defaults(GTK_TABLE(HiScoreDialog.table),label,
                             0,1,index,index+1);
   gtk_widget_show(label);

/* Remove any leading whitespace from the remainder, since g_strsplit
   will split at every space character, not at a run of them */
   g_strchug(spl1[1]);

/* Get the second word - the date */
   spl2 = g_strsplit(spl1[1]," ",1);
   if (!spl2 || !spl2[0] || !spl2[1]) {
      g_warning(_("Corrupt high score!"));
      g_strfreev(spl2);
      return;
   }
   label=gtk_label_new(spl2[0]);
   gtk_misc_set_alignment(GTK_MISC(label),0.5,0.5);
   gtk_table_attach_defaults(GTK_TABLE(HiScoreDialog.table),label,
                             1,2,index,index+1);
   gtk_widget_show(label);

/* The remainder is the name, terminated with (R.I.P.) if the player died,
   and '<' for the 'current' score */
   g_strchug(spl2[1]);

/* Remove '<' suffix if present */
   slen=strlen(spl2[1]);
   if (slen>=1 && spl2[1][slen-1]=='<') {
     spl2[1][slen-1]='\0';
   }
   slen--;

/* Check for (R.I.P.) suffix, and add it to the 4th column if found */
   if (slen>8 && spl2[1][slen-1]==')' && spl2[1][slen-8]=='(') {
      label=gtk_label_new(&spl2[1][slen-8]);
      gtk_misc_set_alignment(GTK_MISC(label),0.5,0.5);
      gtk_table_attach_defaults(GTK_TABLE(HiScoreDialog.table),label,
                                3,4,index,index+1);
      gtk_widget_show(label);
      spl2[1][slen-8]='\0';  /* Remove suffix from the player name */
   }

/* Finally, add in what's left of the player name */
   g_strchomp(spl2[1]);
   label=gtk_label_new(spl2[1]);
   gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
   gtk_table_attach_defaults(GTK_TABLE(HiScoreDialog.table),label,
                             2,3,index,index+1);
   gtk_widget_show(label);

   g_strfreev(spl1); g_strfreev(spl2);
}

static void EndHighScore(GtkWidget *widget) {
/* gtk_widget_destroy(widget);*/
   EndGame();
}

void CompleteHighScoreDialog(gboolean AtEnd) {
   GtkWidget *OKButton,*dialog;
   dialog=HiScoreDialog.dialog;

/* Caption of the "OK" button in dialogs */
   OKButton=gtk_button_new_with_label(_("OK"));
   gtk_signal_connect_object(GTK_OBJECT(OKButton),"clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             (gpointer)dialog);
   if (AtEnd) {
      InGame=FALSE;
      gtk_signal_connect_object(GTK_OBJECT(dialog),"destroy",
                                GTK_SIGNAL_FUNC(EndHighScore),NULL);
   }
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
   gchar text[4];
   GtkWidget *window;
   gpointer CanRunHere=NULL;

   window=gtk_widget_get_ancestor(widget,GTK_TYPE_WINDOW);
   if (window) CanRunHere=gtk_object_get_data(GTK_OBJECT(window),"CanRunHere");

   Answer=GPOINTER_TO_INT(data);
   Play=ClientData.Play;
   switch(Answer) {
      case 'D':
         gtk_widget_hide(FightDialog); break;
      case 'R':
         if (CanRunHere) {
            SendClientMessage(Play,C_NONE,C_FIGHTACT,NULL,"R");
         } else {
            gtk_widget_hide(FightDialog);
            Jet();
         }
         break;
      case 'F': case 'S':
         text[0]=Answer; text[1]='\0';
         SendClientMessage(Play,C_NONE,C_FIGHTACT,NULL,text);
         break;
   }
}

static GtkWidget *AddFightButton(gchar *Text,GtkAccelGroup *accel_group,
                                 GtkBox *box,gint Answer) {
   GtkWidget *button;
   button=gtk_button_new_with_label("");
   SetAccelerator(button,Text,button,"clicked",accel_group);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(FightCallback),
                      GINT_TO_POINTER(Answer));
   gtk_box_pack_start(box,button,TRUE,TRUE,0);
   return button;
}

struct combatant {
   GtkWidget *name,*bitches,*healthprog,*healthlabel;
};

static void CreateFightDialog() {
   GtkWidget *dialog,*vbox,*button,*hbox,*hbbox,*hsep,*text,*table;
   GtkAccelGroup *accel_group;
   GArray *combatants;
   gchar *buf;

   FightDialog=dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_default_size(GTK_WINDOW(dialog),500,300);
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

   table=gtk_table_new(2,4,FALSE);
   gtk_table_set_row_spacings(GTK_TABLE(table),5);
   gtk_table_set_col_spacings(GTK_TABLE(table),5);

   hsep=gtk_hseparator_new();
   gtk_table_attach_defaults(GTK_TABLE(table),hsep,0,4,1,2);
   gtk_widget_show_all(table);
   gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,FALSE,0);
   gtk_object_set_data(GTK_OBJECT(dialog),"table",table);

   combatants = g_array_new(FALSE,TRUE,sizeof(struct combatant));
   g_array_set_size(combatants,1);
   gtk_object_set_data(GTK_OBJECT(dialog),"combatants",combatants);

   text=gtk_scrolled_text_new(NULL,NULL,&hbox);
   gtk_widget_set_usize(text,150,120);

   gtk_text_set_editable(GTK_TEXT(text),FALSE);
   gtk_text_set_word_wrap(GTK_TEXT(text),TRUE);
   gtk_object_set_data(GTK_OBJECT(dialog),"text",text);
   gtk_widget_show_all(hbox);
   gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);

   hsep=gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox),hsep,FALSE,FALSE,0);
   gtk_widget_show(hsep);

   hbbox=gtk_hbutton_box_new();

/* Button for closing the "Fight" dialog and going back to dealing drugs
   (%Tde = "Drugs" by default) */
   buf=dpg_strdup_printf(_("_Deal %Tde"),Names.Drugs);
   button=AddFightButton(buf,accel_group,GTK_BOX(hbbox),'D');
   gtk_object_set_data(GTK_OBJECT(dialog),"deal",button);
   g_free(buf);

/* Button for shooting at other players in the "Fight" dialog, or for
   popping up the "Fight" dialog from the main window */
   button=AddFightButton(_("_Fight"),accel_group,GTK_BOX(hbbox),'F');
   gtk_object_set_data(GTK_OBJECT(dialog),"fight",button);

/* Button to stand and take it in the "Fight" dialog */
   button=AddFightButton(_("_Stand"),accel_group,GTK_BOX(hbbox),'S');
   gtk_object_set_data(GTK_OBJECT(dialog),"stand",button);

/* Button to run from combat in the "Fight" dialog */
   button=AddFightButton(_("_Run"),accel_group,GTK_BOX(hbbox),'R');
   gtk_object_set_data(GTK_OBJECT(dialog),"run",button);

   gtk_widget_show(hsep);
   gtk_box_pack_start(GTK_BOX(vbox),hbbox,FALSE,FALSE,0);
   gtk_widget_show(hbbox);
   gtk_widget_show(vbox);
   gtk_container_add(GTK_CONTAINER(dialog),vbox);
   gtk_widget_show(dialog);
}

static void UpdateCombatant(gchar *DefendName,int DefendBitches,
                            gchar *BitchName,int DefendHealth) {
   gint i,RowIndex;
   gchar *name;
   struct combatant *compt;
   GArray *combatants;
   GtkWidget *table;
   gchar *BitchText,*HealthText;
   gfloat ProgPercent;

   combatants=(GArray *)gtk_object_get_data(GTK_OBJECT(FightDialog),
                                            "combatants");
   table=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog),"table"));
   if (!combatants) return;

   if (DefendName[0]) {
      compt=NULL;
      for (i=1,RowIndex=2;i<combatants->len;i++,RowIndex++) {
         compt=&g_array_index(combatants,struct combatant,i);
         if (!compt || !compt->name) { compt=NULL; continue; }
         gtk_label_get(GTK_LABEL(compt->name),&name);
         if (name && strcmp(name,DefendName)==0) break;
         compt=NULL;
      }
      if (!compt) {
         i=combatants->len;
         g_array_set_size(combatants,i+1);
         compt=&g_array_index(combatants,struct combatant,i);
         gtk_table_resize(GTK_TABLE(table),i+2,4);
         RowIndex=i+1;
      }
   } else {
      compt=&g_array_index(combatants,struct combatant,0);
      RowIndex=0;
   }

/* Display of number of bitches or deputies during combat (%tde="bitches"
   or "deputies" (etc.) by default) */
   BitchText=dpg_strdup_printf(_("%/Combat: Bitches/%d %tde"),
                               DefendBitches,BitchName);

/* Display of health during combat */
   HealthText=g_strdup_printf(_("Health: %d"),DefendHealth);

   ProgPercent=(gfloat)DefendHealth/100.0;

   if (compt->name) {
      if (DefendName[0]) gtk_label_set_text(GTK_LABEL(compt->name),DefendName);
      gtk_label_set_text(GTK_LABEL(compt->bitches),BitchText);
      gtk_label_set_text(GTK_LABEL(compt->healthlabel),HealthText);
      gtk_progress_bar_update(GTK_PROGRESS_BAR(compt->healthprog),
                              ProgPercent);
   } else {
/* Display of the current player's name during combat */
      compt->name = gtk_label_new(DefendName[0] ? DefendName : _("You"));

      gtk_table_attach_defaults(GTK_TABLE(table),compt->name,0,1,
                                RowIndex,RowIndex+1);
      compt->bitches = gtk_label_new(BitchText);
      gtk_table_attach_defaults(GTK_TABLE(table),compt->bitches,1,2,
                                RowIndex,RowIndex+1);
      compt->healthprog = gtk_progress_bar_new();
      gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(compt->healthprog),
                                    GTK_PROGRESS_LEFT_TO_RIGHT);
      gtk_progress_bar_update(GTK_PROGRESS_BAR(compt->healthprog),
                              ProgPercent);
      gtk_table_attach_defaults(GTK_TABLE(table),compt->healthprog,2,3,
                                RowIndex,RowIndex+1);
      compt->healthlabel = gtk_label_new(HealthText);
      gtk_table_attach_defaults(GTK_TABLE(table),compt->healthlabel,3,4,
                                RowIndex,RowIndex+1);
      gtk_widget_show(compt->name);
      gtk_widget_show(compt->bitches);
      gtk_widget_show(compt->healthprog);
      gtk_widget_show(compt->healthlabel);
   }

   g_free(BitchText); g_free(HealthText);
}

static void FreeCombatants() {
   GArray *combatants;
   combatants=(GArray *)gtk_object_get_data(GTK_OBJECT(FightDialog),
                                            "combatants");
   if (!combatants) return;

   g_array_free(combatants,TRUE);
}

void DisplayFightMessage(char *Data) {
   Player *Play;
   gint EditPos;
   GtkAccelGroup *accel_group;
   GtkWidget *Deal,*Fight,*Stand,*Run,*Text;
   char cr[] = "\n";
   gchar *AttackName,*DefendName,*BitchName,FightPoint,*Message;
   int DefendHealth,DefendBitches,BitchesKilled,ArmPercent;
   gboolean CanRunHere,Loot,CanFire;

   if (!Data) {
      if (FightDialog) {
         FreeCombatants();
         gtk_widget_destroy(FightDialog); FightDialog=NULL;
      }
      return;
   }
   if (FightDialog) {
      if (!GTK_WIDGET_VISIBLE(FightDialog)) gtk_widget_show(FightDialog);
   } else {
      CreateFightDialog();
   }
   if (!FightDialog || !Data[0]) return;

   Deal=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog),"deal"));
   Fight=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog),"fight"));
   Stand=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog),"stand"));
   Run=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog),"run"));
   Text=GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(FightDialog),"text"));

   Play=ClientData.Play;

   if (HaveAbility(Play,A_NEWFIGHT)) {
      ReceiveFightMessage(Data,&AttackName,&DefendName,&DefendHealth,
                          &DefendBitches,&BitchName,&BitchesKilled,&ArmPercent,
                          &FightPoint,&CanRunHere,&Loot,&CanFire,&Message);
      if (FightPoint==F_HIT || FightPoint==F_ARRIVED || FightPoint==F_MISS) {
         UpdateCombatant(DefendName,DefendBitches,BitchName,DefendHealth);
      }
      if (FightPoint==F_LASTLEAVE) {
         Play->Flags&= ~FIGHTING;
      } else {
         Play->Flags|=FIGHTING;
      }
      accel_group=(GtkAccelGroup *)
             gtk_object_get_data(GTK_OBJECT(ClientData.window),"accel_group");
      SetJetButtonTitle(accel_group);
   } else {
      Message=Data;
      if (Play->Flags&FIGHTING) FightPoint=F_MSG; else FightPoint=F_LASTLEAVE;
      CanFire = (Play->Flags&CANSHOOT);
      CanRunHere=FALSE;
   }
   gtk_object_set_data(GTK_OBJECT(FightDialog),"CanRunHere",
                       GINT_TO_POINTER(CanRunHere));

   g_strdelimit(Message,"^",'\n');
   if (strlen(Message)>0) {
      EditPos=gtk_text_get_length(GTK_TEXT(Text));
      gtk_editable_insert_text(GTK_EDITABLE(Text),Message,
                               strlen(Message),&EditPos);
      gtk_editable_insert_text(GTK_EDITABLE(Text),cr,strlen(cr),&EditPos);
   }

   if (!CanRunHere || FightPoint==F_LASTLEAVE)
      gtk_widget_show(Deal); else gtk_widget_hide(Deal);
   if (CanFire && TotalGunsCarried(Play)>0)
      gtk_widget_show(Fight); else gtk_widget_hide(Fight);
   if (CanFire && TotalGunsCarried(Play)==0)
      gtk_widget_show(Stand); else gtk_widget_hide(Stand);
   if (FightPoint!=F_LASTLEAVE)
      gtk_widget_show(Run); else gtk_widget_hide(Run);
}

void DisplayStats(Player *Play,struct StatusWidgets *Status) {
   gchar *prstr;
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

/* Display of carried guns in GTK+ client status window (%Tde="Guns"
   by default) */
   dpg_string_sprintf(text,_("%/GTK Stats: Guns/%Tde"),Names.Guns);
   gtk_label_set_text(GTK_LABEL(Status->GunsName),text->str);
   g_string_sprintf(text,"%d",TotalGunsCarried(Play));
   gtk_label_set_text(GTK_LABEL(Status->GunsValue),text->str);

   if (!WantAntique) {
/* Display of number of bitches in GTK+ client status window (%Tde="Bitches"
   by default) */
      dpg_string_sprintf(text,_("%/GTK Stats: Bitches/%Tde"),Names.Bitches);
      gtk_label_set_text(GTK_LABEL(Status->BitchesName),text->str);
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
         if (HaveAbility(ClientData.Play,A_DRUGVALUE) && AreDrugs) {
            titles[1] = dpg_strdup_printf("%d @ %P",Objects[i].Carried,
                              Objects[i].TotalValue/Objects[i].Carried);
         } else {
            titles[1] = g_strdup_printf("%d",Objects[i].Carried);
         }
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
   SendClientMessage(ClientData.Play,C_NONE,C_REQUESTJET,NULL,text);
   g_free(text);
}

void JetButtonPressed(GtkWidget *widget,gpointer data) {
   if (ClientData.Play->Flags & FIGHTING) {
      DisplayFightMessage(NULL);
   } else {
      Jet();
   }
}

void Jet() {
   GtkWidget *dialog,*table,*button,*label,*vbox;
   GtkAccelGroup *accel_group;
   gint boxsize,i,row,col;
   gchar *name,AccelChar;

   accel_group=gtk_accel_group_new();

   dialog=gtk_window_new(GTK_WINDOW_DIALOG);
/* Title of 'Jet' dialog */
   gtk_window_set_title(GTK_WINDOW(dialog),_("Jet to location"));

   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);
   gtk_window_add_accel_group(GTK_WINDOW(dialog),accel_group);
   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   vbox=gtk_vbox_new(FALSE,7);

/* Prompt in 'Jet' dialog */
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

/* Display of locations in 'Jet' window (%tde="The Bronx" etc. by default) */
         name=dpg_strdup_printf(_("_%c. %tde"),AccelChar,Location[i].Name);
         SetAccelerator(button,name,button,"clicked",accel_group);
/* Add keypad shortcuts as well */
         if (i<9) {
            gtk_widget_add_accelerator(button,"clicked",accel_group,GDK_KP_1+i,
                              0,GTK_ACCEL_VISIBLE | GTK_ACCEL_SIGNAL_VISIBLE);
          }
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
   GtkAdjustment *spin_adj;
   gint DrugInd,CanDrop,CanCarry,CanAfford,MaxDrug;
   Player *Play;

   text=g_string_new(NULL);
   DrugInd=DealDialog.DrugInd;
   Play=ClientData.Play;

/* Display of the current price of the selected drug in 'Deal Drugs' dialog */
   dpg_string_sprintf(text,_("at %P"),Play->Drugs[DrugInd].Price);
   gtk_label_set_text(GTK_LABEL(DealDialog.cost),text->str);

   CanDrop=Play->Drugs[DrugInd].Carried;

/* Display of current inventory of the selected drug in 'Deal Drugs' dialog
   (%tde="Opium" etc. by default) */
   dpg_string_sprintf(text,_("You are currently carrying %d %tde"),
                      CanDrop,Drug[DrugInd].Name);
   gtk_label_set_text(GTK_LABEL(DealDialog.carrying),text->str);

   CanCarry=Play->CoatSize;

/* Available space for drugs in 'Deal Drugs' dialog */
   g_string_sprintf(text,_("Available space: %d"),CanCarry);
   gtk_label_set_text(GTK_LABEL(DealDialog.space),text->str);

   if (DealDialog.Type==BT_BUY) {
      CanAfford=Play->Cash/Play->Drugs[DrugInd].Price;

/* Number of the selected drug that you can afford in 'Deal Drugs' dialog */
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
   SendClientMessage(ClientData.Play,C_NONE,C_BUYOBJECT,NULL,text);
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

/* Action in 'Deal Drugs' dialog - "Buy/Sell/Drop Drugs" */
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
   if (data==BT_BUY) {
      g_string_sprintf(text,_("Buy how many?"));
   } else if (data==BT_SELL) {
      g_string_sprintf(text,_("Sell how many?"));
   } else {
      g_string_sprintf(text,_("Drop how many?"));
   }
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

/* Caption of "Cancel" button for GTK+ client dialogs */
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

   
/* Title of 'gun shop' dialog (%tde="guns" by default) */
   if (data==BT_BUY) Title=dpg_strdup_printf(_("Buy %tde"),Names.Guns);
   else if (data==BT_SELL) Title=dpg_strdup_printf(_("Sell %tde"),Names.Guns);
   else Title=dpg_strdup_printf(_("Drop %tde"),Names.Guns);

   text=g_string_new("");

   if (data!=BT_BUY && TotalGunsCarried(ClientData.Play)==0) {
      dpg_string_sprintf(text,_("You don't have any %tde to sell!"),Names.Guns);
      GtkMessageBox(dialog,text->str,Title,MB_OK);
   } else if (data==BT_BUY && TotalGunsCarried(ClientData.Play) >=
                              ClientData.Play->Bitches.Carried+2) { 
      dpg_string_sprintf(text,
                         _("You'll need more %tde to carry any more %tde!"),
                         Names.Bitches,Names.Guns);
      GtkMessageBox(dialog,text->str,Title,MB_OK);
   } else if (data==BT_BUY && Gun[GunInd].Space > ClientData.Play->CoatSize) {
      dpg_string_sprintf(text,
                         _("You don't have enough space to carry that %tde!"),
                         Names.Gun);
      GtkMessageBox(dialog,text->str,Title,MB_OK);
   } else if (data==BT_BUY && Gun[GunInd].Price > ClientData.Play->Cash) {
      dpg_string_sprintf(text,_("You don't have enough cash to buy that %tde!"),
                         Names.Gun);
      GtkMessageBox(dialog,text->str,Title,MB_OK);
   } else if (data==BT_SELL && ClientData.Play->Guns[GunInd].Carried == 0) {
      GtkMessageBox(dialog,_("You don't have any to sell!"),Title,MB_OK);
   } else {
      g_string_sprintf(text,"gun^%d^%d",GunInd,data==BT_BUY ? 1 : -1);
      SendClientMessage(ClientData.Play,C_NONE,C_BUYOBJECT,NULL,text->str);
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
   SendClientMessage(ClientData.Play,C_NONE,C_ANSWER,To,text);

   gtk_widget_destroy(dialog);
}

void QuestionDialog(char *Data,Player *From) {
   GtkWidget *dialog,*label,*vbox,*hsep,*hbbox,*button;
   GtkAccelGroup *accel_group;
   gchar *Responses,**split,*LabelText;

/* Button titles that correspond to the single-keypress options provided
   by the curses client (e.g. _Yes corresponds to 'Y' etc.) */
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

/* Title of the 'ask player a question' dialog */
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
         SetAccelerator(button,_(Words[i]),button,"clicked",accel_group);
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
   InitAbilities(Play);
   SendAbilities(Play);
   SetPlayerName(Play,ClientData.PlayerName);
   SendNullClientMessage(Play,C_NONE,C_NAME,NULL,ClientData.PlayerName);
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
   gboolean MultiPlayer;

   MultiPlayer = (FirstClient && FirstClient->next!=NULL);

   gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                            "<main>/Talk"),InGame && Network);
   gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                            "<main>/List"),InGame);
   gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                            "<main>/List/Players"),InGame && Network);
   gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                            "<main>/Errands"),InGame);
   gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                            "<main>/Errands/Spy"),InGame && MultiPlayer);
   gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                            "<main>/Errands/Tipoff"),InGame && MultiPlayer);
   gtk_widget_set_sensitive(gtk_item_factory_get_widget(ClientData.Menu,
                            "<main>/Errands/Get spy reports"),
                            InGame && MultiPlayer);
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

/* Available space label in GTK+ client status display */
   label=Status->SpaceName = gtk_label_new(_("Space"));

   gtk_table_attach_defaults(GTK_TABLE(table),label,4,5,0,1);
   label=Status->SpaceValue = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,5,6,0,1);

/* Player's cash label in GTK+ client status display */
   label=Status->CashName = gtk_label_new(_("Cash"));

   gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,1,2);
   label=Status->CashValue = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,1,2,1,2);

/* Player's debt label in GTK+ client status display */
   label=Status->DebtName = gtk_label_new(_("Debt"));

   gtk_table_attach_defaults(GTK_TABLE(table),label,2,3,1,2);
   label=Status->DebtValue = gtk_label_new(NULL);
   gtk_table_attach_defaults(GTK_TABLE(table),label,3,4,1,2);

/* Player's bank balance label in GTK+ client status display */
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

/* Player's health label in GTK+ client status display */
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
   
   ClientData.JetAccel=SetAccelerator(button,
                (ClientData.Play && ClientData.Play->Flags & FIGHTING) ?
                _("_Fight") :
/* Caption of 'Jet' button in main window */
                _("_Jet!"),button,"clicked",accel_group);
}

#ifdef CYGWIN
char GtkLoop(HINSTANCE hInstance,HINSTANCE hPrevInstance) {
#else
char GtkLoop(int *argc,char **argv[],char ReturnOnFail) {
#endif
   GtkWidget *window,*vbox,*vbox2,*hbox,*frame,*table,*menubar,*text,
             *vpaned,*button,*clist;
   GtkAccelGroup *accel_group;
   GtkItemFactory *item_factory;
   GtkAdjustment *adj;
   gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

#ifdef CYGWIN
   win32_init(hInstance,hPrevInstance);
#else
   gtk_set_locale();
   if (ReturnOnFail && !gtk_init_check(argc,argv)) return FALSE;
   else if (!ReturnOnFail) gtk_init(argc,argv);
#endif

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

/* Title of main window in GTK+ client */
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
   gtk_item_factory_set_translate_func(item_factory,MenuTranslate,NULL,NULL);

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

   adj=(GtkAdjustment *)gtk_adjustment_new(0.0,0.0,100.0,1.0,10.0,10.0);
   text=ClientData.messages=gtk_scrolled_text_new(NULL,adj,&hbox);
   gtk_widget_set_usize(text,100,80);
   gtk_text_set_point(GTK_TEXT(text),0);
   gtk_text_set_editable(GTK_TEXT(text),FALSE);
   gtk_text_set_word_wrap(GTK_TEXT(text),TRUE);
   gtk_paned_pack1(GTK_PANED(vpaned),hbox,TRUE,TRUE);

   hbox=gtk_hbox_new(FALSE,7);
   CreateInventory(hbox,Names.Drugs,accel_group,TRUE,TRUE,&ClientData.Drug,
                   DealDrugs);
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
/* Credits labels in GTK+ 'about' dialog */
      { N_("Drug Dealing and Research"), "Dan Wolf", NULL },
      { N_("Play Testing"), "Phil Davis", "Owen Walsh" },
      { N_("Extensive Play Testing"), "Katherine Holt",
        "Caroline Moore" },
      { N_("Constructive Criticism"), "Andrea Elliot-Smith",
        "Pete Winn" },
      { N_("Unconstructive Criticism"), "James Matthews", NULL }
   };

   dialog=gtk_window_new(GTK_WINDOW_DIALOG);

/* Title of GTK+ 'about' dialog */
   gtk_window_set_title(GTK_WINDOW(dialog),_("About dopewars"));

   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));
   gtk_container_border_width(GTK_CONTAINER(dialog),10);

   vbox=gtk_vbox_new(FALSE,5);

   label=gtk_label_new(
/* Main content of GTK+ 'about' dialog */
_("Based on John E. Dell's old Drug Wars game, dopewars is a simulation of an\n"
"imaginary drug market.  dopewars is an All-American game which features\n"
"buying, selling, and trying to get past the cops!\n\n"
"The first thing you need to do is pay off your debt to the Loan Shark. After\n"
"that, your goal is to make as much money as possible (and stay alive)! You\n"
"have one month of game time to make your fortune.\n"));
   gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

/* Version and copyright notice in GTK+ 'about' dialog */
   VersionStr=g_strdup_printf(_("Version %s     "
"Copyright (C) 1998-2001  Ben Webb ben@bellatrix.pcl.ox.ac.uk\n"
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
/* Label at the bottom of GTK+ 'about' dialog */
_("\nFor information on the command line options, type dopewars -h at your\n"
"Unix prompt. This will display a help screen, listing the available "
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
/* Error: GTK+ client could not connect to the given dopewars server */
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
/* Message displayed during the attempted connect to a dopewars server */
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
      if (ThisServer->CurPlayers==-1) {
/* Displayed if we don't know how many players are logged on to a server */
         titles[3]=_("Unknown");
      } else {
/* e.g. "5 of 20" means 5 players are logged on to a server, out of a
   maximum of 20 */
         titles[3]=g_strdup_printf(_("%d of %d"),ThisServer->CurPlayers,
                                                 ThisServer->MaxPlayers);
      }
      titles[4]=ThisServer->Comment;
      row=gtk_clist_append(GTK_CLIST(metaserv),titles);
      gtk_clist_set_row_data(GTK_CLIST(metaserv),row,(gpointer)ThisServer);
      g_free(titles[1]);
      if (ThisServer->CurPlayers!=-1) g_free(titles[3]);
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
   gchar *text;
   gchar *server_titles[5],*ServerEntry;
   static struct StartGameStruct widgets;
   guint AccelKey;
   gboolean UpdateMeta=FALSE;

/* Column titles of metaserver information */
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
   gtk_window_set_default_size(GTK_WINDOW(dialog),400,250);
   accel_group=gtk_accel_group_new();

/* Title of 'New Game' dialog */
   gtk_window_set_title(GTK_WINDOW(widgets.dialog),_("New Game"));
   gtk_container_set_border_width(GTK_CONTAINER(widgets.dialog),7);
   gtk_window_add_accel_group(GTK_WINDOW(widgets.dialog),accel_group);

   vbox=gtk_vbox_new(FALSE,7);
   hbox=gtk_hbox_new(FALSE,7);

   label=gtk_label_new("");

   AccelKey=gtk_label_parse_uline(GTK_LABEL(label),
/* Prompt for player's name in 'New Game' dialog */
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

   ServerEntry = "localhost";
   if (strcasecmp(ServerName,SN_META)==0) {
      NewGameType=2;
      UpdateMeta=TRUE;
   } else if (strcasecmp(ServerName,SN_PROMPT)==0) NewGameType=0;
   else if (strcasecmp(ServerName,SN_SINGLE)==0)   NewGameType=1;
   else ServerEntry = ServerName;

   gtk_entry_set_text(GTK_ENTRY(entry),ServerEntry);
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
/* Button to connect to a named dopewars server */
   SetAccelerator(button,_("_Connect"),button,"clicked",accel_group);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(ConnectToServer),
                      (gpointer)&widgets);
   gtk_box_pack_start(GTK_BOX(vbox2),button,FALSE,FALSE,0);
   gtk_container_add(GTK_CONTAINER(frame),vbox2);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_grab_default(button);

   label=gtk_label_new(_("Server"));
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook),frame,label);

/* Title of 'New Game' dialog notebook tab for single-player mode */
   frame=gtk_frame_new(_("Single player"));
   gtk_container_set_border_width(GTK_CONTAINER(frame),4);
   vbox2=gtk_vbox_new(FALSE,7);
   gtk_container_set_border_width(GTK_CONTAINER(vbox2),4);
   widgets.antique=gtk_check_button_new_with_label("");

/* Checkbox to activate 'antique mode' in single-player games */
   SetAccelerator(widgets.antique,_("_Antique mode"),widgets.antique,
                  "clicked",accel_group);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.antique),WantAntique);
   gtk_box_pack_start(GTK_BOX(vbox2),widgets.antique,FALSE,FALSE,0);
   button=gtk_button_new_with_label("");
   SetAccelerator(button,_("_Start single-player game"),button,
                  "clicked",accel_group);
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

   clist=widgets.metaserv=gtk_scrolled_clist_new_with_titles(5,server_titles,
                                                             &scrollwin);
   gtk_clist_column_titles_passive(GTK_CLIST(clist));
   gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
   gtk_clist_set_column_width(GTK_CLIST(clist),0,100);
   gtk_clist_set_column_width(GTK_CLIST(clist),1,35);

   gtk_box_pack_start(GTK_BOX(vbox2),scrollwin,TRUE,TRUE,0);

   hbbox=gtk_hbutton_box_new();
   button=gtk_button_new_with_label("");

/* Button to update metaserver information */
   SetAccelerator(button,_("_Update"),button,
                  "clicked",accel_group);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
                      GTK_SIGNAL_FUNC(UpdateMetaServerList),
                      (gpointer)&widgets);
   gtk_box_pack_start(GTK_BOX(hbbox),button,TRUE,TRUE,0);

   button=gtk_button_new_with_label("");
   SetAccelerator(button,_("_Connect"),button,
                  "clicked",accel_group);
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
   if (UpdateMeta) {
      UpdateMetaServerList(NULL,&widgets);
   } else {
      FillMetaServerList(&widgets);
   }

   gtk_widget_show_all(widgets.dialog);
   gtk_notebook_set_page(GTK_NOTEBOOK(notebook),NewGameType);
}

static void SendDoneMessage(GtkWidget *widget,gpointer data) {
   SendClientMessage(ClientData.Play,C_NONE,C_DONE,NULL,NULL);
}

static void TransferPayAll(GtkWidget *widget,GtkWidget *dialog) {
   gchar *text;
   text=pricetostr(ClientData.Play->Debt);
   SendClientMessage(ClientData.Play,C_NONE,C_PAYLOAN,NULL,text);
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
         GtkMessageBox(dialog,_("There isn't that much money in the bank..."),
                    "Bank",MB_OK);
         return;
      }
   }
   if (money>ClientData.Play->Cash) {
      GtkMessageBox(dialog,_("You don't have that much money!"),
                 Debt ? "Pay loan" : "Bank",MB_OK);
      return;
   }
   text=pricetostr(money);
   SendClientMessage(ClientData.Play,C_NONE,
                     Debt ? C_PAYLOAN : C_DEPOSIT,NULL,text);
   g_free(text);
   gtk_widget_destroy(dialog);
}

void TransferDialog(gboolean Debt) {
   GtkWidget *dialog,*button,*label,*radio,*table,*vbox,*hbbox,*hsep,*entry;
   GSList *group;
   GString *text;

   text=g_string_new("");

   dialog=gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_signal_connect(GTK_OBJECT(dialog),"destroy",
                      GTK_SIGNAL_FUNC(SendDoneMessage),NULL);
   if (Debt) {
/* Title of loan shark dialog - (%Tde="The Loan Shark" by default) */
      dpg_string_sprintf(text,_("%/LoanShark window title/%Tde"),
                         Names.LoanSharkName);
   } else {
/* Title of bank dialog - (%Tde="The Bank" by default) */
      dpg_string_sprintf(text,_("%/BankName window title/%Tde"),
                         Names.LoanSharkName);
   }
   gtk_window_set_title(GTK_WINDOW(dialog),text->str);
   gtk_container_set_border_width(GTK_CONTAINER(dialog),7);
   gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(ClientData.window));

   vbox=gtk_vbox_new(FALSE,7);
   table=gtk_table_new(4,3,FALSE);
   gtk_table_set_row_spacings(GTK_TABLE(table),4);
   gtk_table_set_col_spacings(GTK_TABLE(table),4);

/* Display of player's cash in bank or loan shark dialog */
   dpg_string_sprintf(text,_("Cash: %P"),ClientData.Play->Cash);
   label=gtk_label_new(text->str);
   gtk_table_attach_defaults(GTK_TABLE(table),label,0,3,0,1);

   if (Debt) {
/* Display of player's debt in loan shark dialog */
      dpg_string_sprintf(text,_("Debt: %P"),ClientData.Play->Debt);
   } else {
/* Display of player's bank balance in bank dialog */
      dpg_string_sprintf(text,_("Bank: %P"),ClientData.Play->Bank);
   }
   label=gtk_label_new(text->str);
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

   g_string_free(text,TRUE);
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
      SendClientMessage(ClientData.Play,C_NONE,C_MSG,NULL,text);
      g_string_sprintf(msg,"%s: %s",GetPlayerName(ClientData.Play),text);
      PrintMessage(msg->str);
   } else {
      for(selection=GTK_CLIST(TalkData->clist)->selection;selection;
          selection=g_list_next(selection)) {
         row=GPOINTER_TO_INT(selection->data);
         Play=(Player *)gtk_clist_get_row_data(GTK_CLIST(TalkData->clist),row);
         if (Play) {
            SendClientMessage(ClientData.Play,C_NONE,C_MSGTO,Play,text);
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
         SendClientMessage(ClientData.Play,C_NONE,C_SPYON,Play,NULL);
      } else {
         SendClientMessage(ClientData.Play,C_NONE,C_TIPOFF,Play,NULL);
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
      text=dpg_strdup_printf(
_("Please choose the player to spy on. Your %tde will\n"
"then offer his services to the player, and if successful,\n"
"you will be able to view the player's stats with the\n"
"\"Get spy reports\" menu. Remember that the %tde will leave\n"
"you, so any %tde or %tde that he's carrying may be lost!"),
Names.Bitch,Names.Bitch,Names.Guns,Names.Drugs);
      label=gtk_label_new(text); g_free(text);
   } else {
      gtk_window_set_title(GTK_WINDOW(dialog),_("Tip Off The Cops"));
      text=dpg_strdup_printf(
_("Please choose the player to tip off the cops to. Your %tde will\n"
"help the cops to attack that player, and then report back to you\n"
"on the encounter. Remember that the %tde will leave you temporarily,\n"
"so any %tde or %tde that he's carrying may be lost!"),
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
   char *title,*text;
   title=dpg_strdup_printf(_("Sack %Tde"),Names.Bitch);
   text=dpg_strdup_printf(_("Are you sure? (Any %tde or %tde carried\n"
                          "by this %tde may be lost!)"),Names.Guns,
                          Names.Drugs,Names.Bitch);
   if (GtkMessageBox(ClientData.window,text,title,MB_YESNO)==IDYES) {
      SendClientMessage(ClientData.Play,C_NONE,C_SACKBITCH,NULL,NULL);
   }
   g_free(text); g_free(title);
}

void CreateInventory(GtkWidget *hbox,gchar *Objects,GtkAccelGroup *accel_group,
                     gboolean CreateButtons,gboolean CreateHere,
                     struct InventoryWidgets *widgets,GtkSignalFunc CallBack) {
   GtkWidget *scrollwin,*clist,*vbbox,*frame[2],*button[3];
   gint i,mini;
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
      dpg_string_sprintf(text,_("%Tde here"),Objects);
      widgets->HereFrame=frame[0]=gtk_frame_new(text->str);
   }
   dpg_string_sprintf(text,_("%Tde carried"),Objects);
   widgets->CarriedFrame=frame[1]=gtk_frame_new(text->str);

   widgets->HereList=widgets->CarriedList=NULL;
   if (CreateHere) mini=0; else mini=1;
   for (i=mini;i<2;i++) {
      gtk_container_set_border_width(GTK_CONTAINER(frame[i]),5);

      clist=gtk_scrolled_clist_new_with_titles(2,titles[i],&scrollwin);
      gtk_clist_set_column_auto_resize(GTK_CLIST(clist),0,TRUE);
      gtk_clist_set_column_auto_resize(GTK_CLIST(clist),1,TRUE);
      gtk_clist_column_titles_passive(GTK_CLIST(clist));
      gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
      gtk_clist_set_auto_sort(GTK_CLIST(clist),FALSE);
      gtk_container_add(GTK_CONTAINER(frame[i]),scrollwin);
      if (i==0) widgets->HereList=clist; else widgets->CarriedList=clist;
   }
   if (CreateHere) gtk_box_pack_start(GTK_BOX(hbox),frame[0],TRUE,TRUE,0);

   if (CreateButtons) {
      widgets->vbbox=vbbox=gtk_vbutton_box_new();

      for (i=0;i<3;i++) {
         button[i]=gtk_button_new_with_label("");
         SetAccelerator(button[i],_(button_text[i]),button[i],
                        "clicked",accel_group);
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
      SendNullClientMessage(ClientData.Play,C_NONE,C_NAME,NULL,text);
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

/* Title of 'gun shop' dialog in GTK+ client (%Tde="Dan's House of Guns"
   by default) */
   text=dpg_strdup_printf(_("%/GTK GunShop window title/%Tde"),
                          Names.GunShopName);
   gtk_window_set_title(GTK_WINDOW(window),text);
   g_free(text);
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
   CreateInventory(hbox,Names.Guns,accel_group,TRUE,TRUE,&ClientData.Gun,
                   DealGuns);

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
   SendClientMessage(ClientData.Play,C_NONE,C_CONTACTSPY,NULL,NULL);
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
   CreateInventory(hbox,Names.Drugs,accel_group,FALSE,FALSE,&SpyDrugs,NULL);
   CreateInventory(hbox,Names.Guns,accel_group,FALSE,FALSE,&SpyGuns,NULL);

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
#include "dopewars.h" /* We need this for the definition of '_' */

char GtkLoop(int *argc,char **argv[],char ReturnOnFail) {
   if (!ReturnOnFail) {
      g_print(_("No graphical client available - rebuild the binary\n"
              "passing the --enable-gui-client option to configure, or\n"
              "use the curses client (if available) instead!\n"));
   }
   return FALSE;
}

#endif /* GUI_CLIENT */
