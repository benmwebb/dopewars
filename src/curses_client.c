/* curses_client.c  dopewars client using the (n)curses console library */
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

#ifdef CURSES_CLIENT

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <glib.h>
#include "dopeos.h"
#include "curses_client.h"
#include "serverside.h"
#include "dopewars.h"
#include "message.h"

static void PrepareHighScoreScreen();
static void PrintHighScore(char *Data);

static int ResizedFlag;
static SCREEN *cur_screen;
static char ConnectMethod=CM_SERVER;

/* Function definitions; make them static so as not to clash with functions
   of the same name in different clients */
static void display_intro();
static void ResizeHandle(int sig);
static void CheckForResize(Player *Play);
static int GetKey(char *allowed,char *orig_allowed,gboolean AllowOther,
                  gboolean PrintAllowed);
static void clear_bottom(), clear_screen();
static void clear_line(int line), clear_exceptfor(int skip);
static void nice_wait();
static void DisplayFightMessage(char *text);
static void DisplaySpyReports(char *Data,Player *From,Player *To);
static void display_message(char *buf);
static void print_location(char *text);
static void print_status(Player *Play,char DispDrug);
static char *nice_input(char *prompt,int sy,int sx,char digitsonly,
                        char *displaystr);
static Player *ListPlayers(Player *Play,char Select,char *Prompt);
static void HandleClientMessage(char *buf,Player *Play);
static void PrintMessage(const gchar *text);
static void GunShop(Player *Play);
static void LoanShark(Player *Play);
static void Bank(Player *Play);

static char DisplayMode,QuitRequest;

static void start_curses() {
/* Initialises the curses library for accessing the screen */
   cur_screen=newterm(NULL,stdout,stdin);
   if (WantColour) {
      start_color();
      init_pair(1,COLOR_MAGENTA,COLOR_WHITE);
      init_pair(2,COLOR_BLACK,COLOR_WHITE);
      init_pair(3,COLOR_BLACK,COLOR_WHITE);
      init_pair(4,COLOR_BLUE,COLOR_WHITE);
      init_pair(5,COLOR_WHITE,COLOR_BLUE);
      init_pair(6,COLOR_RED,COLOR_WHITE);
   }
   cbreak();
   noecho();
   nodelay(stdscr,FALSE);
   keypad(stdscr,TRUE);
   curs_set(0);
}

static void end_curses() {
/* Shuts down the curses screen library */
   keypad(stdscr,FALSE);
   curs_set(1);
   erase();
   refresh();
   endwin();
}

void ResizeHandle(int sig) {
/* Handles a SIGWINCH signal, which is sent to indicate that the   */
/* size of the curses screen has changed.                          */
   ResizedFlag=1;
}

void CheckForResize(Player *Play) {
/* Checks to see if the curses window needs to be resized - i.e. if a */
/* SIGWINCH signal has been received                                  */
   sigset_t sigset;
   sigemptyset(&sigset);
   sigaddset(&sigset,SIGWINCH);
   sigprocmask(SIG_BLOCK,&sigset,NULL);
   if (ResizedFlag) {
      ResizedFlag=0;
      end_curses(); start_curses();
      Width=COLS; Depth=LINES;
      attrset(TextAttr); clear_screen();
      display_message("");
      DisplayFightMessage(NULL);
      print_status(Play,1);
   }
   sigprocmask(SIG_UNBLOCK,&sigset,NULL);
}

static void LogMessage(const gchar *log_domain,GLogLevelFlags log_level,
                       const gchar *message,gpointer user_data) {
   attrset(TextAttr); clear_bottom();
   PrintMessage(message);
   nice_wait();
   attrset(TextAttr); clear_bottom();
}

void display_intro() {
/* Displays a dopewars introduction screen */
   GString *text;
   attrset(TextAttr);
   clear_screen();
   attrset(TitleAttr);

   text=g_string_new(_("D O P E W A R S"));
   mvaddstr(1,(Width-text->len)/2,text->str);

   attrset(TextAttr);

   mvaddstr(3,1,_("Based on John E. Dell's old Drug Wars game, dopewars "
                  "is a simulation of an"));
   mvaddstr(4,1,_("imaginary drug market.  dopewars is an All-American "
                  "game which features"));
   mvaddstr(5,1,_("buying, selling, and trying to get past the cops!"));

   mvaddstr(7,1,_("The first thing you need to do is pay off your "
                  "debt to the Loan Shark. After"));
   mvaddstr(8,1,_("that, your goal is to make as much money as "
                  "possible (and stay alive)! You"));
   mvaddstr(9,1,_("have one month of game time to make your fortune."));

   mvaddstr(11,18,_("Copyright (C) 1998-2000  Ben Webb "
                    "ben@bellatrix.pcl.ox.ac.uk"));
   g_string_sprintf(text,_("Version %s"),VERSION);
   mvaddstr(11,2,text->str);
   g_string_assign(text,
           _("dopewars is released under the GNU General Public Licence"));
   mvaddstr(12,(Width-text->len)/2,text->str);

   mvaddstr(14,7,_("Drug Dealing and Research     Dan Wolf"));
   mvaddstr(15,7,_("Play Testing                  Phil Davis           "
                   "Owen Walsh"));
   mvaddstr(16,7,_("Extensive Play Testing        Katherine Holt       "
                   "Caroline Moore"));
   mvaddstr(17,7,_("Constructive Criticism        Andrea Elliot-Smith  "
                   "Pete Winn"));
   mvaddstr(18,7,_("Unconstructive Criticism      James Matthews"));

   mvaddstr(20,3,_("For information on the command line options, type "
                   "dopewars -h at your"));
   mvaddstr(21,1,_("Unix prompt. This will display a help screen, listing "
                   "the available options."));

   g_string_free(text,TRUE);
   nice_wait();
   attrset(TextAttr); clear_screen(); refresh();
}

#ifdef NETWORKING
static void SelectServerManually() {
/* Prompts the user to enter a server name and port to connect to */
   gchar *text,*PortText;
   if (ServerName[0]=='(') AssignName(&ServerName,"localhost");
   attrset(TextAttr);
   clear_bottom();
   mvaddstr(17,1,
            _("Please enter the hostname and port of a dopewars server:-"));
   text=nice_input(_("Hostname: "),18,1,0,ServerName);
   AssignName(&ServerName,text); g_free(text);
   PortText=g_strdup_printf("%d",Port);
   text=nice_input(_("Port: "),19,1,1,PortText);
   Port=atoi(text);
   g_free(text); g_free(PortText);
}

static char *SelectServerFromMetaServer() {
/* Contacts the dopewars metaserver, and obtains a list of valid */
/* server/port pairs, one of which the user should select.       */
/* Returns a pointer to a static string containing an error      */
/* message if the connection failed, otherwise NULL.             */
   char *MetaError;
   int HttpSock,c;
   GSList *ListPt;
   ServerData *ThisServer;
   GString *text;
   gint index;
   static char NoServers[] = N_("No servers listed on metaserver");

   attrset(TextAttr);
   clear_bottom();
   mvaddstr(17,1,_("Please wait... attempting to contact metaserver..."));
   refresh();

   MetaError=OpenMetaServerConnection(&HttpSock);
   if (MetaError) return MetaError;

   clear_line(17);
   mvaddstr(17,1,
          _("Connection to metaserver established. Obtaining server list..."));
   refresh();

   ReadMetaServerData(HttpSock);
   CloseMetaServerConnection(HttpSock);

   text=g_string_new("");

   ListPt=ServerList;
   while (ListPt) {
      ThisServer=(ServerData *)(ListPt->data);
      attrset(TextAttr);
      clear_bottom();
      g_string_sprintf(text,_("Server : %s"),ThisServer->Name);
      mvaddstr(17,1,text->str);
      g_string_sprintf(text,_("Port   : %d"),ThisServer->Port);
      mvaddstr(18,1,text->str);
      g_string_sprintf(text,_("Version    : %s"),ThisServer->Version);
      mvaddstr(18,40,text->str);
      if (ThisServer->CurPlayers==-1) {
         g_string_sprintf(text,_("Players: -unknown- (maximum %d)"),
                          ThisServer->MaxPlayers);
      } else {
         g_string_sprintf(text,_("Players: %d (maximum %d)"),
                          ThisServer->CurPlayers,ThisServer->MaxPlayers);
      }
      mvaddstr(19,1,text->str);
      g_string_sprintf(text,_("Up since   : %s"),ThisServer->UpSince);
      mvaddstr(19,40,text->str);
      g_string_sprintf(text,_("Comment: %s"),ThisServer->Comment);
      mvaddstr(20,1,text->str);
      attrset(PromptAttr);
      mvaddstr(23,1,
               _("N>ext server; P>revious server; S>elect this server... "));
      c=GetKey(_("NPS"),"NPS",FALSE,FALSE);
      switch(c) {
         case 'S': AssignName(&ServerName,ThisServer->Name);
                   Port=ThisServer->Port;
                   ListPt=NULL;
                   break;
         case 'N': ListPt=g_slist_next(ListPt);
                   if (!ListPt) ListPt=ServerList;
                   break;
         case 'P': index=g_slist_position(ServerList,ListPt)-1;
                   if (index>=0) ListPt=g_slist_nth(ServerList,(guint)index);
                   else ListPt=g_slist_last(ListPt);
                   break;
      }
   }
   if (!ServerList) return NoServers;
   clear_line(17);
   refresh();
   g_string_free(text,TRUE);
   return NULL;
}

static char ConnectToServer(Player *Play) {
/* Connects to a dopewars server. Prompts the user to select a server */
/* if necessary. Returns TRUE, unless the user elected to quit the    */
/* program rather than choose a valid server.                         */
   char *pt=NULL,*MetaError=NULL;
   gchar *text;
   int c;
   if (strcasecmp(ServerName,"(MetaServer)")==0 || ConnectMethod==CM_META) {
      ConnectMethod=CM_META;
      MetaError=SelectServerFromMetaServer();
   } else if (strcasecmp(ServerName,"(Prompt)")==0 ||
              ConnectMethod==CM_PROMPT) {
      ConnectMethod=CM_PROMPT;
      SelectServerManually();
   } else if (strcasecmp(ServerName,"(Single)")==0 ||
              ConnectMethod==CM_SINGLE) {
      ConnectMethod=CM_SINGLE;
      return TRUE;
   }
   while (1) {
      attrset(TextAttr);
      clear_bottom();
      if (!MetaError) {
         mvaddstr(17,1,
                  _("Please wait... attempting to contact dopewars server..."));
         refresh();
         pt=SetupNetwork(FALSE);
      }
      if (pt || MetaError) {
         clear_line(17);
         if (MetaError) {
            text=g_strdup_printf(_("Error: %s"),_(MetaError));
            mvaddstr(17,1,text); g_free(text);
         } else {
            mvaddstr(16,1,_("Could not start multiplayer dopewars"));
            text=g_strdup_printf("   (%s)",_(pt));
            mvaddstr(17,1,text); g_free(text);
         }
         pt=MetaError=NULL;
         attrset(PromptAttr);
         mvaddstr(18,1,
                  _("Will you... C>onnect to a different host and/or port"));
         mvaddstr(19,1,
                  _("            L>ist the servers on the metaserver, and "
                    "select one"));
         mvaddstr(20,1,
                  _("            Q>uit (where you can start a server by "
                    "typing "));
         mvaddstr(21,1,
                  _("                   dopewars -s < /dev/null & )"));
         mvaddstr(22,1,_("         or P>lay single-player ? "));
         attrset(TextAttr);
         c=GetKey(_("CLQP"),"CLQP",FALSE,FALSE);
         switch(c) {
            case 'Q': return FALSE;
            case 'P': ConnectMethod=CM_SINGLE;
                      return TRUE;
            case 'L': ConnectMethod=CM_META;
                      MetaError=SelectServerFromMetaServer();
                      break;
            case 'C': ConnectMethod=CM_PROMPT;
                      SelectServerManually();
                      break;
         }
      } else break;
   }
   return TRUE;
}
#endif /* NETWORKING */

static int jet(Player *Play,char AllowReturn) {
/* Displays the list of locations and prompts the user to select one. */
/* If "AllowReturn" is TRUE, then if the current location is selected */
/* simply drop back to the main game loop, otherwise send a request   */
/* to the server to move to the new location. If FALSE, the user MUST */
/* choose a new location to move to. The active client player is      */
/* passed in "Play"                                                   */
/* N.B. May set the global variable DisplayMode                       */
/* Returns: 1 if the user chose to jet to a new location,             */
/*          0 if the action was cancelled instead.                    */
   int i,c;
   char text[80];
   attrset(TextAttr);
   clear_bottom();
   for (i=0;i<NumLocation;i++) {
      sprintf(text,"%d. %s",i+1,Location[i].Name);
      mvaddstr(17+i/3,(i%3)*20+12,text);
   }
   attrset(PromptAttr);
   mvaddstr(22,22,_("Where to, dude ? "));
   attrset(TextAttr);
   curs_set(1);
   while (1) {
      c=bgetch();
      if (c>='1' && c<'1'+NumLocation) {
         addstr(Location[c-'1'].Name);
         if (Play->IsAt != c-'1') {
            curs_set(0);
            sprintf(text,"%d",c-'1');
            DisplayMode=DM_NONE;
            SendClientMessage(Play,C_NONE,C_REQUESTJET,NULL,text);
            return 1;
         }
      }
      if (AllowReturn) break;
   }
   curs_set(0);
   return 0;
}

static void DropDrugs(Player *Play) {
/* Prompts the user "Play" to drop some of the currently carried drugs  */
   int i,c,NumDrugs;
   GString *text;
   gchar *buf;
   attrset(TextAttr);
   clear_bottom();
   text=g_string_new("");
   g_string_sprintf(text,
                    _("You can\'t get any cash for the following carried %s :"),
                    Names.Drugs);
   mvaddstr(16,1,text->str);
   NumDrugs=0;
   for (i=0;i<NumDrug;i++) {
      if (Play->Drugs[i].Carried>0 && Play->Drugs[i].Price==0) {
         g_string_sprintf(text,"%c. %-10s %-8d",NumDrugs+'A',Drug[i].Name,
                              Play->Drugs[i].Carried);
         mvaddstr(17+NumDrugs/3,(NumDrugs%3)*25+4,text->str);
         NumDrugs++;
      }
   }
   attrset(PromptAttr);
   mvaddstr(22,20,_("What do you want to drop? "));
   curs_set(1);
   attrset(TextAttr);
   c=bgetch();
   c=toupper(c);
   if (c>='A' && c<'A'+NumDrugs) {
      for (i=0;i<NumDrug;i++) if (Play->Drugs[i].Carried>0 &&
                                  Play->Drugs[i].Price==0) {
         c--;
         if (c<'A') {
            addstr(Drug[i].Name);
            buf=nice_input(_("How many do you drop? "),23,8,1,NULL);
            c=atoi(buf); g_free(buf);
            if (c>0) {
               g_string_sprintf(text,"drug^%d^%d",i,-c);
               SendClientMessage(Play,C_NONE,C_BUYOBJECT,NULL,text->str);
            }
            break;
         }
      }
   }
   g_string_free(text,TRUE);
}

static void DealDrugs(Player *Play,char Buy) {
/* Prompts the user (i.e. the owner of client "Play") to buy drugs if   */
/* "Buy" is TRUE, or to sell drugs otherwise. A list of available drugs */
/* is displayed, and on receiving the selection, the user is prompted   */
/* for the number of drugs desired. Finally a message is sent to the    */
/* server to buy or sell the required quantity.                         */
   int i,c,NumDrugsHere;
   gchar *text,*input;
   int DrugNum,CanCarry,CanAfford;

   NumDrugsHere=0;
   for (c=0;c<NumDrug;c++) if (Play->Drugs[c].Price>0) NumDrugsHere++;

   clear_line(22);
   attrset(PromptAttr);
   if (Buy) {
      mvaddstr(22,20,_("What do you wish to buy? "));
   } else {
      mvaddstr(22,20,_("What do you wish to sell? "));
   }
   curs_set(1);
   attrset(TextAttr);
   c=bgetch();
   c=toupper(c);
   if (c>='A' && c<'A'+NumDrugsHere) {
      DrugNum=-1;
      for (i=0;i<NumDrug;i++) {
         DrugNum=GetNextDrugIndex(DrugNum,Play);
         if (--c<'A') break;
      }
      addstr(Drug[DrugNum].Name);
      CanCarry=Play->CoatSize;
      CanAfford=Play->Cash/Play->Drugs[DrugNum].Price;

      if (Buy) {
         text=g_strdup_printf(_("You can afford %d, and can carry %d. "),
                              CanAfford,CanCarry);
         mvaddstr(23,2,text);
         input=nice_input(_("How many do you buy? "),23,2+strlen(text),1,NULL);
         c=atoi(input); g_free(input); g_free(text);
         if (c>=0) {
            text=g_strdup_printf("drug^%d^%d",DrugNum,c);
            SendClientMessage(Play,C_NONE,C_BUYOBJECT,NULL,text);
            g_free(text);
         }
      } else {
         text=g_strdup_printf(_("You have %d. "),Play->Drugs[DrugNum].Carried);
         mvaddstr(23,2,text);
         input=nice_input(_("How many do you sell? "),23,2+strlen(text),1,NULL);
         c=atoi(input); g_free(input); g_free(text);
         if (c>=0) {
            text=g_strdup_printf("drug^%d^%d",DrugNum,-c);
            SendClientMessage(Play,C_NONE,C_BUYOBJECT,NULL,text);
            g_free(text);
         }
      }
   }
   curs_set(0);
}

static void GiveErrand(Player *Play) {
/* Prompts the user (player "Play") to give an errand to one of his/her */
/* bitches. The decision is relayed to the server for implementation.   */
   int c,y;
   gchar *prstr;
   GString *text;
   Player *To;
   text=g_string_new("");
   attrset(TextAttr);
   clear_bottom();
   y=17;
   g_string_sprintf(text,_("Choose an errand to give one of your %s..."),
                    Names.Bitches);
   mvaddstr(y++,1,text->str);
   attrset(PromptAttr);
   if (Play->Bitches.Carried>0) {
      g_string_sprintf(text,
                    _("   S>py on another dealer                  (cost: %s)"),
                      prstr=FormatPrice(Prices.Spy));
      mvaddstr(y++,2,text->str); g_free(prstr);
      g_string_sprintf(text,
                    _("   T>ip off the cops to another dealer     (cost: %s)"),
                      prstr=FormatPrice(Prices.Tipoff));
      mvaddstr(y++,2,text->str); g_free(prstr);
      mvaddstr(y++,2,_("   G>et stuffed"));
   }
   if (Play->Flags&SPYINGON) {
      mvaddstr(y++,2,_("or C>ontact your spies and receive reports"));
   }
   mvaddstr(y++,2,_("or N>o errand ? "));
   curs_set(1);
   attrset(TextAttr);
   c=GetKey(_("STGCN"),"STGCN",TRUE,FALSE);
   if (Play->Bitches.Carried>0 || c=='C') switch (c) {
      case 'S':
         To=ListPlayers(Play,TRUE,_("Whom do you want to spy on? "));
         if (To) SendClientMessage(Play,C_NONE,C_SPYON,To,NULL);
         break;
      case 'T':
         To=ListPlayers(Play,TRUE,
                        _("Whom do you want to tip the cops off to? "));
         if (To) SendClientMessage(Play,C_NONE,C_TIPOFF,To,NULL);
         break;
      case 'G':
         attrset(PromptAttr);
         addstr(_(" Are you sure? "));
         c=GetKey(_("YN"),"YN",FALSE,TRUE);
         if (c=='Y') SendClientMessage(Play,C_NONE,C_SACKBITCH,NULL,NULL);
         break;
      case 'C':
         if (Play->Flags & SPYINGON) {
            SendClientMessage(Play,C_NONE,C_CONTACTSPY,NULL,NULL);
         }
         break;
   }
}

static int want_to_quit() {
/* Asks the user if he/she _really_ wants to quit dopewars */
   attrset(TextAttr);
   clear_line(22);
   attrset(PromptAttr);
   mvaddstr(22,1,_("Are you sure you want to quit? "));
   attrset(TextAttr);
   return (GetKey(_("YN"),"YN",FALSE,TRUE)!='N');
}

static void change_name(Player *Play,char nullname) {
/* Prompts the user to change his or her name, and notifies the server */
   gchar *NewName;
   NewName=nice_input(_("New name: "),23,0,0,NULL);
   if (NewName[0]) {
      if (nullname) {
         SendNullClientMessage(Play,C_NONE,C_NAME,NULL,NewName);
      } else {
         SendClientMessage(Play,C_NONE,C_NAME,NULL,NewName);
      }
      SetPlayerName(Play,NewName);
   }
   g_free(NewName);
}

void HandleClientMessage(char *Message,Player *Play) {
/* Given a message "Message" coming in for player "Play", performs          */
/* processing and reacts properly; if a message indicates the end of the    */
/* game, the global variable QuitRequest is set. The global variable        */
/* DisplayMode may also be changed by this routine as a result of network   */
/* traffic.                                                                 */
   char *pt,*Data,Code,*wrd;
   char AICode;
   Player *From,*tmp;
   GSList *list;
   gchar *text;
   int i;
   gboolean Handled;

/* Ignore To: field - all messages will be for Player "Play" */
   if (ProcessMessage(Message,Play,&From,&AICode,&Code,&Data,FirstClient)==-1) {
      return;
   }

   Handled=HandleGenericClientMessage(From,AICode,Code,Play,Data,&DisplayMode);
   switch(Code) {
      case C_ENDLIST:
         if (FirstClient && g_slist_next(FirstClient)) {
            ListPlayers(Play,FALSE,NULL);
         }
         break;
      case C_STARTHISCORE:
         PrepareHighScoreScreen(); break;
      case C_HISCORE:
         PrintHighScore(Data); break;
      case C_ENDHISCORE:
         if (strcmp(Data,"end")==0) {
            QuitRequest=TRUE;
         } else {
            nice_wait();
            clear_screen();
            display_message("");
            print_status(Play,1);
            refresh();
         }
         break;
      case C_PUSH:
         attrset(TextAttr);
         clear_line(22);
         mvaddstr(22,0,_("You have been pushed from the server. "
                         "Reverting to single player mode."));
         nice_wait();
         SwitchToSinglePlayer(Play);
         print_status(Play,TRUE);
         break;
      case C_QUIT:
         attrset(TextAttr);
         clear_line(22);
         mvaddstr(22,0,
            _("The server has terminated. Reverting to single player mode."));
         nice_wait();
         SwitchToSinglePlayer(Play);
         print_status(Play,TRUE);
         break;
      case C_MSG:
         text=g_strdup_printf("%s: %s",GetPlayerName(From),Data);
         display_message(text); g_free(text);
         break;
      case C_MSGTO:
         text=g_strdup_printf("%s->%s: %s",GetPlayerName(From),
                              GetPlayerName(Play),Data);
         display_message(text); g_free(text);
         break;
      case C_JOIN:
         text=g_strdup_printf(_("%s joins the game!"),Data);
         display_message(text); g_free(text);
         break;
      case C_LEAVE:
         if (From!=&Noone) {
            text=g_strdup_printf(_("%s has left the game."),Data);
            display_message(text); g_free(text);
         }
         break;
      case C_RENAME:
         text=g_strdup_printf(_("%s will now be known as %s."),
                              GetPlayerName(From),Data);
         SetPlayerName(From,Data);
         mvaddstr(22,0,text); g_free(text); nice_wait();
         break;
      case C_PRINTMESSAGE:
         PrintMessage(Data);
         nice_wait();
         break;
      case C_FIGHTPRINT:
         pt=Data;
         wrd=GetNextWord(&pt,NULL);
         while (wrd) {
            DisplayFightMessage(wrd);
            wrd=GetNextWord(&pt,NULL);
         }
         break;
      case C_SUBWAYFLASH:
         DisplayFightMessage(NULL);
         for (list=FirstClient;list;list=g_slist_next(list)) {
            tmp=(Player *)list->data;
            tmp->Flags &= ~FIGHTING;
         }
         for (i=0;i<4;i++) {
            print_location(_("S U B W A Y"));
            refresh();
            MicroSleep(100000);
            print_location("");
            refresh();
            MicroSleep(100000);
         }
         print_location(Location[(int)Play->IsAt].Name);
         break;
      case C_QUESTION:
         pt=Data;
         wrd=GetNextWord(&pt,"");
         PrintMessage(pt);
         addch(' ');
         i=GetKey(wrd,wrd,FALSE,TRUE);
         wrd=g_strdup_printf("%c",i);
         SendClientMessage(Play,C_NONE,C_ANSWER,
                           From==&Noone ? NULL : From,wrd);
         g_free(wrd);
         break;
      case C_LOANSHARK:
         LoanShark(Play);
         SendClientMessage(Play,C_NONE,C_DONE,NULL,NULL);
         break;
      case C_BANK:
         Bank(Play);
         SendClientMessage(Play,C_NONE,C_DONE,NULL,NULL);
         break;
      case C_GUNSHOP:
         GunShop(Play);
         SendClientMessage(Play,C_NONE,C_DONE,NULL,NULL);
         break;
      case C_UPDATE:
         if (From==&Noone) {
            ReceivePlayerData(Data,Play);
            print_status(Play,1); refresh();
         } else {
            DisplaySpyReports(Data,From,Play);
         }
         break;
      case C_NEWNAME:
         clear_line(22); clear_line(23);
         attrset(TextAttr);
         mvaddstr(22,0,_("Unfortunately, somebody else is already "
                         "using \"your\" name. Please change it."));
         change_name(Play,1);
         break;
      default:
         if (!Handled) {
            text=g_strdup_printf("%s^%c^%s^%s",GetPlayerName(From),Code,
                                 GetPlayerName(Play),Data);
            mvaddstr(22,0,text); g_free(text); nice_wait();
         }
         break;
   }
}

void PrepareHighScoreScreen() {
/* Responds to a "starthiscore" message by clearing the screen and */
/* displaying the title for the high scores screen                 */
   char *text;
   attrset(TextAttr);
   clear_screen();
   attrset(TitleAttr);
   text=_("H I G H   S C O R E S");
   mvaddstr(0,(Width-strlen(text))/2,text);
   attrset(TextAttr);
}

void PrintHighScore(char *Data) {
/* Prints a high score coded in "Data"; first word is the index of the  */
/* score (i.e. y screen coordinate), second word is the text, the first */
/* letter of which identifies whether it's to be printed bold or not.   */
   char *cp;
   int index;
   cp=Data;
   index=GetNextInt(&cp,0);
   if (!cp || strlen(cp)<2) return;
   move(index+2,0);
   attrset(TextAttr);
   if (cp[0]=='B') standout();
   addstr(&cp[1]); 
   if (cp[0]=='B') standend();
}

void PrintMessage(const gchar *text) {
/* Prints a message "text" received via. a "printmessage" message in the */
/* bottom part of the screen.                                            */
   int i,line;
   attrset(TextAttr);
   clear_line(16);
   for (i=0;i<strlen(text);i++) {
      if (text[i]!='^' || text[i]=='\n') {
         clear_exceptfor(i+1);
         break;
      }
   }
   line=17; move(line,1);
   for (i=0;i<strlen(text);i++) {
      if (text[i]=='^' || text[i]=='\n') {
         line++; move(line,1);
      } else if (text[i]!='\r') addch(text[i]);
   }
}

void GunShop(Player *Play) {
/* Allows player "Play" to buy and sell guns interactively. Passes the */
/* decisions on to the server for sanity checking and implementation.  */
   int i,c,c2;
   gchar *text,*prstr;
   print_status(Play,0);
   attrset(TextAttr);
   clear_bottom();
   for (i=0;i<NumGun;i++) {
      text=g_strdup_printf("%c. %-22s %12s",'A'+i,Gun[i].Name,
                           prstr=FormatPrice(Gun[i].Price));
      mvaddstr(17+i/2,(i%2)*40+1,text);
      g_free(prstr); g_free(text);
   }
   while (1) {
      text=_("Will you B>uy, S>ell, or L>eave? ");
      attrset(PromptAttr);
      clear_line(22);
      mvaddstr(22,40-strlen(text)/2,text);
      attrset(TextAttr);
      c=GetKey(_("BSL"),"BSL",FALSE,FALSE);
      if (c=='L') break;
      if (c=='S' || c=='B') {
         clear_line(22);
         if (c=='S' && TotalGunsCarried(Play)==0) {
            text=g_strdup_printf(_("You don't have any %s to sell!"),
                                 Names.Guns);
            mvaddstr(22,(Width-strlen(text))/2,text); g_free(text);
            nice_wait();
            clear_line(23);
            continue;
         } else if (c=='B' && TotalGunsCarried(Play)>=Play->Bitches.Carried+2) {
            text=g_strdup_printf(_("You'll need more %s to carry any more %s!"),
                                 Names.Bitches,Names.Guns);
            mvaddstr(22,(Width-strlen(text))/2,text); g_free(text);
            nice_wait();
            clear_line(23);
            continue;
         }
         attrset(PromptAttr);
         if (c=='B') {
            mvaddstr(22,20,_("What do you wish to buy? "));
         } else {
            mvaddstr(22,20,_("What do you wish to sell? "));
         }
         curs_set(1);
         attrset(TextAttr);
         c2=bgetch(); c2=toupper(c2);
         if (c2>='A' && c2<'A'+NumGun) {
            c2-='A';
            addstr(Gun[c2].Name);
            if (c=='B') {
               if (Gun[c2].Space > Play->CoatSize) {
                  clear_line(22);
                  text=g_strdup_printf(_("You don't have enough space to "
                                         "carry that %s!"),Names.Gun);
                  mvaddstr(22,(Width-strlen(text))/2,text); g_free(text);
                  nice_wait();
                  clear_line(23);
                  continue;
               } else if (Gun[c2].Price > Play->Cash) {
                  clear_line(22);
                  text=g_strdup_printf(_("You don't have enough cash to buy "
                                         "that %s!"),Names.Gun);
                  mvaddstr(22,(Width-strlen(text))/2,text); g_free(text);
                  nice_wait();
                  clear_line(23);
                  continue;
               }
               Play->Cash -= Gun[c2].Price;
               Play->CoatSize -= Gun[c2].Space;
               Play->Guns[c2].Carried++;
            } else if (c=='S') {
               if (Play->Guns[c2].Carried == 0) {
                  clear_line(22);
                  mvaddstr(22,10,_("You don't have any to sell!"));
                  nice_wait(); clear_line(23); continue;
               }
               Play->Cash += Gun[c2].Price;
               Play->CoatSize += Gun[c2].Space;
               Play->Guns[c2].Carried--;
            }
            text=g_strdup_printf("gun^%d^%d",c2,c=='B' ? 1 : -1);
            SendClientMessage(Play,C_NONE,C_BUYOBJECT,NULL,text);
            g_free(text);
            print_status(Play,0);
         }
      }
   }
   print_status(Play,1);
}

void LoanShark(Player *Play) {
/* Allows player "Play" to pay off loans interactively. */
   gchar *text,*prstr;
   price_t money;
   while (1) {
      clear_bottom();
      attrset(PromptAttr);
      text=nice_input(_("How much money do you pay back? "),19,1,1,NULL);
      attrset(TextAttr);
      money=strtoprice(text); g_free(text);
      if (money<0) money=0;
      if (money>Play->Debt) money=Play->Debt;
      if (money>Play->Cash) {
         mvaddstr(20,1,_("You don't have that much money!"));
         nice_wait();
      } else {
         SendClientMessage(Play,C_NONE,C_PAYLOAN,NULL,
                           (prstr=pricetostr(money)));
         g_free(prstr);
         break;
      }
   }
}

void Bank(Player *Play) {
/* Allows player "Play" to pay in or withdraw money from the bank */
/* interactively.                                                 */
   gchar *text,*prstr;
   price_t money;
   int c;
   while (1) {
      clear_bottom();
      attrset(PromptAttr);
      mvaddstr(18,1,_("Do you want to D>eposit money, W>ithdraw money, "
                      "or L>eave ? "));
      attrset(TextAttr);
      c=GetKey(_("DWL"),"DWL",FALSE,FALSE);
      if (c=='L') return;
      text=nice_input(_("How much money? "),19,1,1,NULL);
      money=strtoprice(text); g_free(text);
      if (money<0) money=0;
      if (c=='W') money=-money;
      if (money>Play->Cash) {
         mvaddstr(20,1,_("You don't have that much money!"));
         nice_wait();
      } else if (-money > Play->Bank) {
         mvaddstr(20,1,_("There isn't that much money in the bank..."));
         nice_wait();
      } else if (money==0) {
         break;
      } else {
         SendClientMessage(Play,C_NONE,C_DEPOSIT,NULL,
                           (prstr=pricetostr(money)));
         g_free(prstr);
         break;
      }
   }
}

int GetKey(char *allowed,char *orig_allowed,gboolean AllowOther,
           gboolean PrintAllowed) {
/* Waits for keyboard input; will only accept a key listed in the */
/* "allowed" string. This string may have been translated; thus   */
/* the "orig_allowed" string contains the untranslated keys.      */
/* Returns the untranslated key corresponding to the key pressed  */
/* (e.g. if allowed[2] is pressed, orig_allowed[2] is returned)   */
/* Case insensitive. If "AllowOther" is TRUE, keys other than the */
/* given selection are allowed, and cause a zero return value.    */
   int i,c;
   curs_set(1);
   c=0;
   if (!allowed || strlen(allowed)==0) return 0;
   if (PrintAllowed) {
      addch('[' | TextAttr);
      for (i=0;i<strlen(allowed);i++) {
         if (i>0) addch('/' | TextAttr);
         addch(allowed[i] | TextAttr);
      }
      addch(']' | TextAttr);
      addch(' ' | TextAttr);
   }
   while (1) {
      c=bgetch(); c=toupper(c);
      for (i=0;i<strlen(allowed);i++) if (allowed[i]==c) {
         addch(c | TextAttr);
         curs_set(0); return orig_allowed[i];
      }
      if (AllowOther) break;
   }
   curs_set(0);
   return 0;
}

void clear_line(int line) {
/* Clears one whole line on the curses screen */
   int i;
   move(line,0);
   for (i=0;i<Width;i++) addch(' ');
}

void clear_exceptfor(int skip) {
/* Clears the bottom of the screen (i.e. from line 16 to line 23) */
/* except for the top "skip" lines                                */
   int i;
   for (i=16+skip;i<=23;i++) clear_line(i);
}


void clear_bottom() {
/* Clears screen lines 16 to 23 */
   int i;
   for (i=16;i<=23;i++) clear_line(i);
}

void clear_screen() {
/* Clears the entire screen; 24 lines of 80 characters each */
   int i;
   for (i=0;i<Depth;i++) clear_line(i);
}

void nice_wait() {
/* Displays a prompt on the bottom screen line and waits for the user */
/* to press a key                                                     */
   gchar *text;
   attrset(PromptAttr);
   text=_("Press any key...");
   mvaddstr(23,(Width-strlen(text))/2,text);
   bgetch();
   attrset(TextAttr);
}

void DisplayFightMessage(char *text) {
/* Handles the display of messages pertaining to player-player fights   */
/* in the lower part of screen (fighting sub-screen). Adds the new line */
/* of text in "text" and scrolls up previous messages if necessary      */
/* If "text" is NULL, initialises the area                              */
/* If "text" is a blank string, redisplays the message area             */
/* Messages are displayed from lines 16 to 20; line 22 is used for      */
/* the prompt for the user                                              */
   static char Messages[5][79];
   static int x,y;
   char *textpt;
   int i;
   if (text==NULL) {
      x=0; y=15;
      for (i=0;i<5;i++) Messages[i][0]='\0';
      return;
   }
   textpt=text;
   if (!textpt[0]) {
      attrset(TextAttr);
      clear_bottom();
      for (i=16;i<=20;i++) {
         mvaddstr(i,1,Messages[i-16]);
      }
   } else while(textpt[0]) {
      if (y==20) for (i=0;i<4;i++) {
         strcpy(Messages[i],Messages[i+1]);
      }
      if (y<20) y++;
      strncpy(Messages[y-16],textpt,78); Messages[y-16][78]='\0';
      if (strlen(textpt)<=78) break;
      textpt+=78;
   }
}

void display_message(char *buf) {
/* Displays a network message "buf" in the message area (lines   */
/* 10 to 14) scrolling previous messages up                      */
/* If "buf" is NULL, clears the message area                     */
/* If "buf" is a blank string, redisplays the message area       */
   int x,y;
   int wid;
   static char Messages[5][200];
   char *bufpt;
   wid = Width-4 < 200 ? Width-4 : 200;
   if (!buf) {
      for (y=0;y<5;y++) {
         memset(Messages[y],' ',200);
         if (Network) {
            mvaddch(y+10,0,' ' | TextAttr);
            addch(ACS_VLINE | StatsAttr);
            for (x=0;x<wid;x++) {
               addch(' ' | StatsAttr);
            }
            addch(ACS_VLINE | StatsAttr);
         }
      }
      return;
   }
   if (!Network) return;
   bufpt=buf;
   while (bufpt[0]!=0) {
      memmove(Messages[0],Messages[1],200*4);
      memset(Messages[4],' ',200);
      memcpy(Messages[4],bufpt,strlen(bufpt)>wid ? wid : strlen(bufpt));
      if (strlen(bufpt)<=wid) break;
      bufpt+=wid;
   }
   for (y=0;y<5;y++) for (x=0;x<wid;x++) {
      mvaddch(y+10,x+2,Messages[y][x] | StatsAttr);
   }
   refresh();
}

void print_location(char *text) {
/* Displays the string "text" at the top of the screen. Usually used for */
/* displaying the current location or the "Subway" flash.                */
   int i;
   if (!text) return;
   attrset(LocationAttr);
   move(0,Width/2-9);
   for (i=0;i<18;i++) addch(' ');
   mvaddstr(0,(Width-strlen(text))/2,text);
   attrset(TextAttr);
}

void print_status(Player *Play,char DispDrug) {
/* Displays the status of player "Play" - i.e. the current turn, the   */
/* location, bitches, available space, cash, guns, health and bank     */
/* details. If "DispDrugs" is TRUE, displays the carried drugs on the  */
/* right hand side of the screen; if FALSE, displays the carried guns. */
   int i,c;
   gchar *prstr,*caps;
   GString *text;

   text=g_string_new(NULL);
   attrset(TitleAttr);
   g_string_sprintf(text,"%s%02d%s",Names.Month,Play->Turn,Names.Year);
   mvaddstr(0,3,text->str);

   attrset(StatsAttr);
   for (i=2;i<=14;i++) {
      mvaddch(i,1,ACS_VLINE);
      mvaddch(i,Width-2,ACS_VLINE);
   }
   mvaddch(1,1,ACS_ULCORNER);
   for (i=0;i<Width-4;i++) addch(ACS_HLINE);
   addch(ACS_URCORNER);

   mvaddch(1,Width/2,ACS_TTEE);
   for (i=2;i<=(Network ? 8 : 13);i++) {
      move(i,2);
      for (c=2;c<Width/2;c++) addch(' ');
      addch(ACS_VLINE);
      for (c=Width/2+1;c<Width-2;c++) addch(' ');
   }
   if (!Network) {
      mvaddch(14,1,ACS_LLCORNER);
      for (i=0;i<Width-4;i++) addch(ACS_HLINE);
      addch(ACS_LRCORNER);
      mvaddch(14,Width/2,ACS_BTEE);
   } else {
      mvaddch(9,1,ACS_LTEE);
      for (i=0;i<Width-4;i++) addch(ACS_HLINE);
      addch(ACS_RTEE);
      mvaddstr(9,15,_("Messages"));
      mvaddch(9,Width/2,ACS_BTEE);
      mvaddch(15,1,ACS_LLCORNER);
      for (i=0;i<Width-4;i++) addch(ACS_HLINE);
      addch(ACS_LRCORNER);
   }

   mvaddstr(1,Width/4-2,_("Stats"));

   attrset(StatsAttr);
   g_string_sprintf(text,_("Cash %17s"),prstr=FormatPrice(Play->Cash));
   g_free(prstr);
   mvaddstr(3,9,text->str);
   g_string_sprintf(text,"%-19s%3d",caps=InitialCaps(Names.Guns),
                    TotalGunsCarried(Play));
   g_free(caps);
   mvaddstr(Network ? 4 : 5,9,text->str);
   g_string_sprintf(text,_("Health             %3d"),Play->Health);
   mvaddstr(Network ? 5 : 7,9,text->str);
   g_string_sprintf(text,_("Bank %17s"),prstr=FormatPrice(Play->Bank));
   g_free(prstr);
   mvaddstr(Network ? 6 : 9,9,text->str);
   if (Play->Debt>0) attrset(DebtAttr);
   g_string_sprintf(text,_("Debt %17s"),prstr=FormatPrice(Play->Debt));
   g_free(prstr);
   mvaddstr(Network ? 7 : 11,9,text->str);
   attrset(TitleAttr);
   if (WantAntique) g_string_sprintf(text,_("Space %6d"),Play->CoatSize);
   else {
      g_string_sprintf(text,_("%s %3d  Space %6d"),
                       caps=InitialCaps(Names.Bitches),
                       Play->Bitches.Carried,Play->CoatSize);
      g_free(caps);
   }
   mvaddstr(0,Width-2-strlen(text->str),text->str);
   print_location(Location[(int)Play->IsAt].Name);
   attrset(StatsAttr);

   c=0;
   if (DispDrug) {
      if (WantAntique) mvaddstr(1,Width*3/4-5,_("Trenchcoat"));
      else {
         caps=InitialCaps(Names.Drugs);
         mvaddstr(1,Width*3/4-strlen(caps)/2,caps);
         g_free(caps);
      }
      for (i=0;i<NumDrug;i++) {
         if (Play->Drugs[i].Carried>0) {
            g_string_sprintf(text,"%-7s  %3d",Drug[i].Name,
                             Play->Drugs[i].Carried);
            mvaddstr(3+c/2,Width/2+3+(c%2)*17,text->str);
            c++;
         }
      }
   } else {
      caps=InitialCaps(Names.Guns);
      mvaddstr(1,Width*3/4-strlen(caps)/2,caps);
      g_free(caps);
      for (i=0;i<NumGun;i++) {
         if (Play->Guns[i].Carried>0) {
            g_string_sprintf(text,"%-22s %3d",Gun[i].Name,
                             Play->Guns[i].Carried);
            mvaddstr(3+c,Width/2+3,text->str);
            c++;
         }
      }
   }
   attrset(TextAttr);
   if (!Network) clear_line(15);
   refresh();
   g_string_free(text,TRUE);
}

void DisplaySpyReports(char *Data,Player *From,Player *To) {
/* Parses details about player "From" from string "Data" and then */
/* displays the lot, drugs and guns.                              */
   gchar *caps,*text;
   ReceivePlayerData(Data,From);

   clear_bottom();
   text=g_strdup_printf(_("Spy reports for %s"),GetPlayerName(From));
   mvaddstr(17,1,text); g_free(text);

   caps=InitialCaps(Names.Drugs);
   text=g_strdup_printf(_("%s..."),caps);
   mvaddstr(19,20,text); g_free(text); g_free(caps);
   print_status(From,1); nice_wait();
   clear_line(19);
   caps=InitialCaps(Names.Guns);
   text=g_strdup_printf(_("%s..."),caps);
   mvaddstr(19,20,text); g_free(text); g_free(caps);
   print_status(From,0); nice_wait();

   print_status(To,1); refresh();
}

Player *ListPlayers(Player *Play,char Select,char *Prompt) {
/* Displays the "Prompt" if non-NULL, and then lists all clients     */
/* currently playing dopewars, other than the current player "Play". */
/* If "Select" is TRUE, gives each player a letter and asks the user */
/* to select one, which is returned by the function.                 */
   Player *tmp=NULL;
   GSList *list;
   int i,c;
   gchar *text;

   attrset(TextAttr);
   clear_bottom();
   if (!FirstClient || (!g_slist_next(FirstClient) &&
       FirstClient->data == Play)) {
      text=_("No other players are currently logged on!");
      mvaddstr(18,(Width-strlen(text))/2,text);
      nice_wait();
      return 0;
   }
   mvaddstr(16,1,_("Players currently logged on:-"));

   i=0;
   for (list=FirstClient;list;list=g_slist_next(list)) {
      tmp=(Player *)list->data;
      if (strcmp(GetPlayerName(tmp),GetPlayerName(Play))==0) continue;
      if (Select) text=g_strdup_printf("%c. %s",'A'+i,GetPlayerName(tmp));
      else text=g_strdup(GetPlayerName(tmp));
      mvaddstr(17+i/2,(i%2)*40+1,text);
      g_free(text);
      i++;
   }

   if (Prompt) {
      attrset(PromptAttr); mvaddstr(22,10,Prompt); attrset(TextAttr);
   }
   if (Select) {
      curs_set(1);
      attrset(TextAttr);
      c=0;
      while (c<'A' || c>='A'+i) { c=bgetch(); c=toupper(c); }
      if (Prompt) addch(c);
      list=FirstClient;
      while (c>='A') {
         if (list!=FirstClient) list=g_slist_next(list);
         tmp=(Player *)list->data;
         while (strcmp(GetPlayerName(tmp),GetPlayerName(Play))==0) {
            list=g_slist_next(list);
            tmp=(Player *)list->data;
         }
         c--;
      }
      return tmp;
   } else {
      nice_wait();
   }
   return NULL;
}

char *nice_input(char *prompt,int sy,int sx,char digitsonly,char *displaystr) {
/* Displays the given "prompt" (if non-NULL) at coordinates sx,sy and   */
/* allows the user to input a string, which is returned. This is a      */
/* dynamically allocated string, and so must be freed by the calling    */
/* routine. If "digitsonly" is TRUE, the user will be permitted only to */
/* input numbers, although the suffixes m and k are allowed (the        */
/* strtoprice routine understands this notation for a 1000000 or 1000   */
/* multiplier) as well as a decimal point (. or ,)                      */
/* If "displaystr" is non-NULL, it is taken as a default response.      */
   int i,c,x;
   gboolean DecimalPoint,Suffix;
   GString *text;
   gchar *ReturnString;
   DecimalPoint=Suffix=FALSE;
   x=sx;
   move(sy,x);
   if (prompt) {
      attrset(PromptAttr);
      addstr(prompt);
      x+=strlen(prompt);
   }
   attrset(TextAttr);
   if (displaystr) {
      addstr(displaystr);
      i=strlen(displaystr);
      text=g_string_new(displaystr);
   } else {
      i=0;
      text=g_string_new("");
   }
   curs_set(1);
   while(1) {
      move(sy+(x+i)/Width,(x+i)%Width);
      c=bgetch();
      if (c==KEY_ENTER || c=='\n') {
         break;
      } else if ((c==8 || c==KEY_BACKSPACE || c==127) && i>0) {
         move(sy+(x+i-1)/Width,(x+i-1)%Width);
         addch(' ');
         i--;
         if (DecimalPoint && text->str[i]=='.') DecimalPoint=FALSE;
         if (Suffix) Suffix=FALSE;
         g_string_truncate(text,i);
      } else if (!Suffix) {
         if ((digitsonly && c>='0' && c<='9') ||
                 (!digitsonly && c>=32 && c!='^' && c<127)) {
            g_string_append_c(text,c);
            i++;
            addch(c);
         } else if (digitsonly && (c=='.' || c==',') && !DecimalPoint) {
            g_string_append_c(text,'.');
            addch(c);
            DecimalPoint=TRUE;
         } else if (digitsonly && (c=='M' || c=='m' || c=='k' || c=='K')
                    && !Suffix) {
            g_string_append_c(text,c);
            i++;
            addch(c);
            Suffix=TRUE;
         }
      }
   }
   curs_set(0);
   move(sy,x);
   ReturnString=text->str;
   g_string_free(text,FALSE); /* Leave the buffer to return */
   return ReturnString;
}

static void Curses_DoGame(Player *Play) {
/* Loop which handles the user playing an interactive game (i.e. "Play" */
/* is a client connected to a server, either locally or remotely)       */
/* dopewars is essentially server-driven, so this loop simply has to    */
/* make the screen look pretty, respond to user keypresses, and react   */
/* to messages from the server.                                         */
   gchar *buf,*OldName,*TalkMsg,*pt,*prstr;
   GString *text;
   int i,c;
   char IsCarrying;
#if NETWORKING || HAVE_SELECT
   fd_set readfs,writefs;
#endif
   int NumDrugsHere;
   int MaxSock;
   char HaveWorthless;
   Player *tmp;
   struct sigaction sact;

   DisplayMode=DM_NONE;
   QuitRequest=FALSE;

   ResizedFlag=0;
   sact.sa_handler=ResizeHandle;
   sact.sa_flags=0;
   sigemptyset(&sact.sa_mask);
   if (sigaction(SIGWINCH,&sact,NULL)==-1) {
      g_warning("Cannot install SIGWINCH handler!\n");
   }
   OldName=g_strdup(GetPlayerName(Play));
   attrset(TextAttr); clear_screen();
   display_message(NULL);
   DisplayFightMessage(NULL);
   print_status(Play,1);

   attrset(TextAttr);
   clear_bottom();
   buf=NULL;
   do {
      g_free(buf);
      buf=nice_input(_("Hey dude, what's your name? "),17,1,0,OldName);
   } while (buf[0]==0);
#if NETWORKING
   if (WantNetwork) {
      if (!ConnectToServer(Play)) { end_curses(); exit(1); }
      Play->fd=ClientSock;
   }
#endif /* NETWORKING */
   print_status(Play,TRUE);
   display_message("");

   SendAbilities(Play);
   SetPlayerName(Play,buf);
   SendNullClientMessage(Play,C_NONE,C_NAME,NULL,buf);
   g_free(buf); g_free(OldName);

   text=g_string_new("");

   while (1) {
      if (Play->Health==0) DisplayMode=DM_NONE;
      HaveWorthless=0;
      IsCarrying=0;
      for (i=0;i<NumDrug;i++) {
         if (Play->Drugs[i].Carried>0) {
            IsCarrying=1;
            if (Play->Drugs[i].Price==0) HaveWorthless=1;
         }
      }
      switch(DisplayMode) {
         case DM_STREET:
            attrset(TextAttr);
            NumDrugsHere=0;
            for (i=0;i<NumDrug;i++) if (Play->Drugs[i].Price>0) NumDrugsHere++;
            clear_bottom();
            g_string_sprintf(text,_("Hey dude, the prices of %s here are:"),
                             Names.Drugs);
            mvaddstr(16,1,text->str);
            i=-1;
            for (c=0;c<NumDrugsHere;c++) {
               if ((i=GetNextDrugIndex(i,Play))==-1) break;
               g_string_sprintf(text,"%c. %-10s %8s",'A'+c,Drug[i].Name,
                                prstr=FormatPrice(Play->Drugs[i].Price));
               g_free(prstr);
               mvaddstr(17+c/3,(c%3)*25+4,text->str);
            }
            attrset(PromptAttr);
            g_string_assign(text,_("Will you B>uy"));
            if (IsCarrying) g_string_append(text,_(", S>ell"));
            if (HaveWorthless && !WantAntique) g_string_append(text,_(", D>rop"));
            if (Network) g_string_append(text,_(", T>alk, P>age, L>ist"));
            if (!WantAntique && (Play->Bitches.Carried>0 ||
                Play->Flags&SPYINGON)) {
               g_string_append(text,_(", G>ive"));
            }
            if (Play->Flags & FIGHTING) {
               g_string_append(text,_(", F>ight"));
/*            } else if (Play->Flags&TRADING) {
               g_string_append(text,", T>rade");*/
            } else {
               g_string_append(text,_(", J>et"));
            }
            g_string_append(text,_(", or Q>uit? "));
            mvaddstr(22,40-strlen(text->str)/2,text->str);
            attrset(TextAttr);
            curs_set(1);
            break;
         case DM_FIGHT:
            DisplayFightMessage("");
            attrset(PromptAttr);
            g_string_assign(text,_("Do you "));
            if (Play->Flags&CANSHOOT) {
               if (TotalGunsCarried(Play)>0) g_string_append(text,_("F>ight, "));
               else g_string_append(text,_("S>tand, "));
            }
            if (Play->Flags&FIGHTING) g_string_append(text,_("R>un, "));
            g_string_append(text,_("D>eal ")); g_string_append(text,Names.Drugs);
            g_string_append(text,_(", or Q>uit? "));
            mvaddstr(22,40-strlen(text->str)/2,text->str);
            attrset(TextAttr);
            curs_set(1);
            break;
         case DM_DEAL:
            attrset(TextAttr);
            clear_bottom();
            mvaddstr(16,1,"Your trade:-");
            mvaddstr(19,1,"His trade:-");
            g_string_assign(text,"Do you A>dd, R>emove, O>K, D>eal ");
            g_string_append(text,Names.Drugs);
            g_string_append(text,", or Q>uit? ");
            attrset(PromptAttr);
            mvaddstr(22,40-strlen(text->str)/2,text->str);
            attrset(TextAttr);
            curs_set(1);
            break;
      }
      refresh();

      if (QuitRequest) return;
#if NETWORKING
      FD_ZERO(&readfs);
      FD_ZERO(&writefs);
      FD_SET(0,&readfs); MaxSock=1;
      if (Client) {
         FD_SET(Play->fd,&readfs);
         if (Play->WriteBuf.DataPresent) FD_SET(Play->fd,&writefs);
         MaxSock=ClientSock+2;
      }
      if (bselect(MaxSock,&readfs,&writefs,NULL,NULL)==-1) {
         if (errno==EINTR) {
            CheckForResize(Play);
            continue;
         }
         perror("bselect"); exit(1);
      }
      if (Client && FD_ISSET(Play->fd,&readfs)) {
         if (!ReadConnectionBufferFromWire(Play)) {
            attrset(TextAttr);
            clear_line(22);
            mvaddstr(22,0,_("Connection to server lost! "
                            "Reverting to single player mode"));
            nice_wait();
            SwitchToSinglePlayer(Play);
            print_status(Play,TRUE);
         } else {
            while ((pt=ReadFromConnectionBuffer(Play))!=NULL) {
               HandleClientMessage(pt,Play);
               g_free(pt);
            }
            if (QuitRequest) return;
         }
      }
      if (Client && FD_ISSET(Play->fd,&writefs)) {
         WriteConnectionBufferToWire(Play);
      }
      if (FD_ISSET(0,&readfs)) {
#elif HAVE_SELECT
      FD_ZERO(&readfs);
      FD_SET(0,&readfs); MaxSock=1;
      if (bselect(MaxSock,&readfs,NULL,NULL,NULL)==-1) {
         if (errno==EINTR) {
            CheckForResize(Play);
            continue;
         }
         perror("bselect"); exit(1);
      }
#endif /* NETWORKING */
         if (DisplayMode==DM_STREET) {
            c=GetKey(_("BSDTPLGFJQ"),"BSDTPLGFJQ",TRUE,FALSE);
         } else if (DisplayMode==DM_FIGHT) {
            c=GetKey(_("DRFSQ"),"DRFSQ",TRUE,FALSE);
         } else c=0;
#if ! (NETWORKING || HAVE_SELECT)
         CheckForResize(Play);
#endif
         if (DisplayMode==DM_STREET) {
            if (c=='J' && !(Play->Flags&FIGHTING)) {
               jet(Play,TRUE);
            } else if (c=='F' && Play->Flags&FIGHTING) {
               DisplayMode=DM_FIGHT;
            } else if (c=='T' && Play->Flags&TRADING) {
               DisplayMode=DM_DEAL;
            } else if (c=='B') {
               DealDrugs(Play,TRUE);
            } else if (c=='S' && IsCarrying) {
               DealDrugs(Play,FALSE);
            } else if (c=='D' && HaveWorthless && !WantAntique) {
               DropDrugs(Play);
            } else if (c=='G' && !WantAntique && Play->Bitches.Carried>0) {
               GiveErrand(Play);
            } else if (c=='Q') {
               if (want_to_quit()==1) {
                  DisplayMode=DM_NONE;
                  clear_bottom();
                  SendClientMessage(Play,C_NONE,C_WANTQUIT,NULL,NULL);
               }
            } else if (c=='L' && Network) {
               attrset(PromptAttr);
               mvaddstr(23,20,_("List what? P>layers or S>cores? "));
               i=GetKey(_("PS"),"PS",TRUE,FALSE);
               if (i=='P') {
                  ListPlayers(Play,FALSE,NULL);
               } else if (i=='S') {
                  DisplayMode=DM_NONE;
                  SendClientMessage(Play,C_NONE,C_REQUESTSCORE,NULL,NULL);
               }
            } else if (c=='P' && Network) {
               tmp=ListPlayers(Play,TRUE,
                   _("Whom do you want to page (talk privately to) ? "));
               if (tmp) {
                  attrset(TextAttr); clear_line(22);
                  TalkMsg=nice_input("Talk: ",22,0,0,NULL);
                  if (TalkMsg[0]) {
                     SendClientMessage(Play,C_NONE,C_MSGTO,tmp,TalkMsg);
                     buf=g_strdup_printf("%s->%s: %s",GetPlayerName(Play),
                             GetPlayerName(tmp),TalkMsg);
                     display_message(buf);
                     g_free(buf);
                  }
                  g_free(TalkMsg);
               }
            } else if (c=='T' && Client) {
               attrset(TextAttr); clear_line(22);
               TalkMsg=nice_input(_("Talk: "),22,0,0,NULL);
               if (TalkMsg[0]) {
                  SendClientMessage(Play,C_NONE,C_MSG,NULL,TalkMsg);
                  buf=g_strdup_printf("%s: %s",GetPlayerName(Play),TalkMsg);
                  display_message(buf);
                  g_free(buf);
               }
               g_free(TalkMsg);
            }
         } else if (DisplayMode==DM_FIGHT) {
            switch(c) {
               case 'D':
                  DisplayMode=DM_STREET;
                  break;
               case 'R':
                  jet(Play,TRUE);
                  break;
               case 'F':
                  if (TotalGunsCarried(Play)>0 && Play->Flags&CANSHOOT) {
                     buf=g_strdup_printf("%c",c);
                     Play->Flags &= ~CANSHOOT;
                     SendClientMessage(Play,C_NONE,C_FIGHTACT,NULL,buf);
                     g_free(buf);
                  }
                  break;
               case 'S':
                  if (TotalGunsCarried(Play)==0 && Play->Flags&CANSHOOT) {
                     buf=g_strdup_printf("%c",c);
                     Play->Flags &= ~CANSHOOT;
                     SendClientMessage(Play,C_NONE,C_FIGHTACT,NULL,buf);
                     g_free(buf);
                  }
                  break;
               case 'Q':
                  if (want_to_quit()==1) {
                     DisplayMode=DM_NONE; clear_bottom();
                     SendClientMessage(Play,C_NONE,C_WANTQUIT,NULL,NULL);
                  }
                  break;
            }
         } else if (DisplayMode==DM_DEAL) {
            switch(c) {
               case 'D':
                  DisplayMode=DM_STREET;
                  break;
               case 'Q':
                  if (want_to_quit()==1) {
                     DisplayMode=DM_NONE; clear_bottom();
                     SendClientMessage(Play,C_NONE,C_WANTQUIT,NULL,NULL);
                  }
                  break;
            }
         }
#if NETWORKING
      }
#endif
      curs_set(0);
   }
   g_string_free(text,TRUE);
}

void CursesLoop() {
   char c;
   gchar *Name=NULL;
   Player *Play;

   start_curses();
   Width=COLS; Depth=LINES;

/* Set up message handlers */
   ClientMessageHandlerPt = HandleClientMessage;
   SocketWriteTestPt = NULL;

/* Make the GLib log messages display nicely */
   g_log_set_handler(NULL,G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_WARNING,
                     LogMessage,NULL);

   display_intro();

   c='Y';
   while(c=='Y') {
      Play=g_new(Player,1);
      FirstClient=AddPlayer(0,Play,FirstClient);
      SetPlayerName(Play,Name);
      Curses_DoGame(Play);
      g_free(Name); Name=g_strdup(GetPlayerName(Play));
      ShutdownNetwork();
      CleanUpServer();
      attrset(TextAttr);
      mvaddstr(23,20,_("Play again? "));
      c=GetKey(_("YN"),"YN",TRUE,TRUE);
   }
   g_free(Name);
   end_curses();
}

#else

#include <glib.h>

void CursesLoop() {
   g_print(_("No curses client available - rebuild the binary passing the\n"
           "--enable-curses-client option to configure, or use a windowed\n"
           "client (if available) instead!\n"));
}

#endif /* CURSES_CLIENT */
