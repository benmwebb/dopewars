/* win32_client.c  dopewars client using the Win32 user interface       */
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

#ifdef CYGWIN

#include <windows.h>
#include <commctrl.h>
#include <glib.h>
#include <stdlib.h>
#include "dopeid.h"

#include "dopeos.h"
#include "dopewars.h"
#include "curses_client.h"
#include "win32_client.h"
#include "message.h"
#include "serverside.h"

#ifdef WIN32_CLIENT

#define WM_SOCKETDATA   (WM_USER+100)
#define WM_ADDSPYREPORT (WM_USER+101)

#define BT_BUY  ((LPARAM)1)
#define BT_SELL ((LPARAM)2)
#define BT_DROP ((LPARAM)3)

HINSTANCE hInst;

BOOL InGame=FALSE,MetaServerRead=FALSE;

struct STATS {
   HWND SellButton,BuyButton,DropButton,JetButton,StatGroup,
        HereList,CarriedList,MessageEdit,Location,Date,Space,Cash,Debt,Bank,
        Guns,Bitches,Health;
};

struct CLIENTDATA {
   HWND Window;
   struct STATS Stats;
   gchar *PlayerName;
   Player *Play;
};


static HWND PlayerListWnd=NULL,TalkWnd=NULL,InventoryWnd=NULL,ScoresWnd=NULL,
            FightWnd=NULL,GunShopWnd=NULL,SpyReportsWnd=NULL;

static struct CLIENTDATA ClientData;

static void PrintMessage(char *Data);
static void UpdateInventory(HWND HereList,HWND CarriedList,
                            HWND BuyButton,HWND SellButton,HWND DropButton,
                            Inventory *Objects,
                            int NumObjects,gboolean AreDrugs);
static void UpdatePlayerLists();
static void UpdatePlayerList(HWND listwin,gboolean IncludeSelf);
static void EndGame();
static void DisplayFightMessage(char *Data);
static BOOL CALLBACK ScoresWndProc(HWND hwnd,UINT msg,UINT wParam,
                                   LONG lParam);
static void AddScoreToDialog(char *Data);
static BOOL CALLBACK GunShopWndProc(HWND hwnd,UINT msg,UINT wParam,
                                    LONG lParam);
static BOOL CALLBACK QuestionWndProc(HWND hwnd,UINT msg,UINT wParam,
                                     LONG lParam);
static BOOL CALLBACK TransferWndProc(HWND hwnd,UINT msg,UINT wParam,
                                     LONG lParam);
static BOOL CALLBACK NewNameWndProc(HWND hwnd,UINT msg,UINT wParam,
                                    LONG lParam);
static void UpdateControls();
static void SetSocketWriteTest(Player *Play,gboolean WriteTest);
static void DisplaySpyReports(Player *Play);
static void CreateStats(HWND hwnd,struct STATS *Stats,
                        gboolean CreateEdit,gboolean CreateButtons);
static void SizeStats(HWND hwnd,struct STATS *Stats,RECT *rect);
static void ShowStats(struct STATS *Stats,int State);

static void LogMessage(const gchar *log_domain,GLogLevelFlags log_level,
                       const gchar *message,gpointer user_data) {
   MessageBox(ClientData.Window,message,
              log_level&G_LOG_LEVEL_WARNING ? "Warning" : "Message",MB_OK);
}

static void DisplayStats(Player *Play,struct STATS *Stats) {
   gchar *prstr,*caps;
   GString *text;

   text=g_string_new("");
   SetWindowText(Stats->Location,Location[(int)Play->IsAt].Name);

   g_string_sprintf(text,"%s%02d%s",Names.Month,Play->Turn,Names.Year);
   SetWindowText(Stats->Date,text->str);

   g_string_sprintf(text,"Space available: %d",Play->CoatSize);
   SetWindowText(Stats->Space,text->str);

   dpg_string_sprintf(text,"Cash: %P",Play->Cash);
   SetWindowText(Stats->Cash,text->str);

   dpg_string_sprintf(text,"Debt: %P",Play->Debt);
   SetWindowText(Stats->Debt,text->str);

   dpg_string_sprintf(text,"Bank: %P",Play->Bank);
   SetWindowText(Stats->Bank,text->str);

   dpg_string_sprintf(text,"%Tde: %d",Names.Guns,TotalGunsCarried(Play));
   SetWindowText(Stats->Guns,text->str);

   if (!WantAntique) {
      dpg_string_sprintf(text,"%Tde: %d",Names.Bitches,Play->Bitches.Carried);
      SetWindowText(Stats->Bitches,text->str);
   } else SetWindowText(Stats->Bitches,"");

   g_string_sprintf(text,"Health: %d",Play->Health);
   SetWindowText(Stats->Health,text->str);

   g_string_free(text,TRUE);
}

static void UpdateStatus(Player *Play,gboolean DisplayDrugs) {
   DisplayStats(Play,&ClientData.Stats);

   UpdateInventory(ClientData.Stats.HereList,ClientData.Stats.CarriedList,
                   ClientData.Stats.BuyButton,ClientData.Stats.SellButton,
                   ClientData.Stats.DropButton,Play->Drugs,NumDrug,TRUE);
   if (GunShopWnd) {
      UpdateInventory(GetDlgItem(GunShopWnd,LB_GUNSHERE),
                      GetDlgItem(GunShopWnd,LB_GUNSCARRIED),
                      GetDlgItem(GunShopWnd,BT_BUYGUN),
                      GetDlgItem(GunShopWnd,BT_SELLGUN),
                      NULL,ClientData.Play->Guns,NumGun,FALSE);
   }
   if (InventoryWnd) {
      UpdateInventory(NULL,GetDlgItem(InventoryWnd,LB_INVENDRUGS),
                      NULL,NULL,NULL,ClientData.Play->Drugs,NumDrug,TRUE);
      UpdateInventory(NULL,GetDlgItem(InventoryWnd,LB_INVENGUNS),
                      NULL,NULL,NULL,ClientData.Play->Guns,NumGun,FALSE);
   }
}

void UpdateInventory(HWND HereList,HWND CarriedList,
                     HWND BuyButton,HWND SellButton,HWND DropButton,
                     Inventory *Objects,int NumObjects,gboolean AreDrugs) {
   Player *Play;
   gint i;
   price_t price;
   gchar *name,*prstr,*text;
   LRESULT addresult;
   gboolean CanBuy=FALSE,CanSell=FALSE,CanDrop=FALSE;

   Play=ClientData.Play;

   SendMessage(HereList,LB_RESETCONTENT,0,0);
   SendMessage(CarriedList,LB_RESETCONTENT,0,0);
   for (i=0;i<NumObjects;i++) {
      if (AreDrugs) {
         name=Drug[i].Name; price=Objects[i].Price;
      } else {
         name=Gun[i].Name; price=Gun[i].Price;
      }
      if (HereList && price>0) {
         CanBuy=TRUE;
         text=dpg_strdup_printf("%s\t%P",name,price);
         addresult=SendMessage(HereList,LB_ADDSTRING,0,(LPARAM)text);
         if (addresult>=0 && addresult<NumObjects) {
            SendMessage(HereList,LB_SETITEMDATA,(WPARAM)addresult,(LPARAM)i);
         }
         g_free(text);
      }
      if (Objects[i].Carried>0) {
         if (price>0) CanSell=TRUE; else CanDrop=TRUE;
         text=g_strdup_printf("%s\t%d",name,Objects[i].Carried);
         addresult=SendMessage(CarriedList,LB_ADDSTRING,0,(LPARAM)text);
         if (addresult>=0 && addresult<NumObjects) {
            SendMessage(CarriedList,LB_SETITEMDATA,(WPARAM)addresult,(LPARAM)i);
         }
         g_free(text);
      }
   }
   if (BuyButton) EnableWindow(BuyButton,CanBuy);
   if (SellButton) EnableWindow(SellButton,CanSell);
   if (DropButton) EnableWindow(DropButton,CanDrop);
}

static void SetErrandMenus(HWND hwnd) {
   gchar *text,*prstr;
   HMENU menu;
   menu=GetMenu(hwnd);
   if (!menu) return;

   text=dpg_strdup_printf("&Spy\t(%P)",Prices.Spy);
   ModifyMenu(menu,ID_SPY,MF_BYCOMMAND | MF_STRING,ID_SPY,text);
   g_free(text);

   text=dpg_strdup_printf("&Tipoff\t(%P)",Prices.Tipoff);
   ModifyMenu(menu,ID_TIPOFF,MF_BYCOMMAND | MF_STRING,ID_TIPOFF,text);
   g_free(text);

   DrawMenuBar(hwnd);
}

static void HandleClientMessage(char *pt,Player *Play) {
   char *Data,Code,AICode,DisplayMode;
   Player *From,*tmp;
   gboolean Handled;
   gchar *text;
   LRESULT Answer;
   GSList *list;
   if (ProcessMessage(pt,Play,&From,&AICode,&Code,&Data,FirstClient)==-1) {
      return;
   }
   Handled=HandleGenericClientMessage(From,AICode,Code,Play,Data,&DisplayMode);
   switch(Code) {
      case C_STARTHISCORE:
         if (ScoresWnd) {
            SetWindowPos(ScoresWnd,HWND_TOP,0,0,0,0,
                         SWP_NOMOVE|SWP_NOSIZE);
         } else {
            ScoresWnd=CreateDialog(hInst,MAKEINTRESOURCE(HiScoreDia),
                                   ClientData.Window,ScoresWndProc);
            ShowWindow(ScoresWnd,SW_SHOWNORMAL);
         }
         SendDlgItemMessage(ScoresWnd,LB_HISCORES,LB_RESETCONTENT,0,0);
         break;
      case C_HISCORE:
         AddScoreToDialog(Data); break;
      case C_ENDHISCORE:
         if (strcmp(Data,"end")==0) EndGame();
         break;
      case C_PRINTMESSAGE:
         PrintMessage(Data); break;
      case C_FIGHTPRINT:
         DisplayFightMessage(Data); break;
      case C_PUSH:
         g_warning("You have been pushed from the server.");
         SwitchToSinglePlayer(Play);
         break;
      case C_QUIT:
         g_warning("The server has terminated.");
         SwitchToSinglePlayer(Play);
         break;
      case C_NEWNAME:
         DialogBox(hInst,MAKEINTRESOURCE(NewNameDia),
                   ClientData.Window,NewNameWndProc);
         break;
      case C_BANK:
         DialogBoxParam(hInst,MAKEINTRESOURCE(BankDialog),
                        ClientData.Window,TransferWndProc,(LPARAM)C_BANK);
         break;
      case C_LOANSHARK:
         DialogBoxParam(hInst,MAKEINTRESOURCE(DebtDialog),
                        ClientData.Window,TransferWndProc,(LPARAM)C_LOANSHARK);
         break;
      case C_GUNSHOP:
         EnableWindow(ClientData.Window,FALSE);
         GunShopWnd=CreateDialog(hInst,MAKEINTRESOURCE(GunShopDia),
                                 ClientData.Window,GunShopWndProc);
         EnableWindow(GunShopWnd,TRUE);
         ShowWindow(GunShopWnd,SW_SHOW);
         break;
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
         text=g_strdup_printf("%s joins the game!",Data);
         PrintMessage(text); g_free(text);
         UpdatePlayerLists();
         break;
      case C_LEAVE:
         if (From!=&Noone) {
            text=g_strdup_printf("%s has left the game.",Data);
            PrintMessage(text); g_free(text);
            UpdatePlayerLists();
         }
         break;
      case C_QUESTION:
         Answer=DialogBoxParam(hInst,MAKEINTRESOURCE(QuestionDia),
                               ClientData.Window,QuestionWndProc,(LPARAM)Data);
         if (Answer!=0) {
            text=g_strdup_printf("%c",(char)Answer);
            SendClientMessage(Play,C_NONE,C_ANSWER,
                              From==&Noone ? NULL : From,text);
            g_free(text);
         }
         break;
      case C_SUBWAYFLASH:
         DisplayFightMessage(NULL);
         for (list=FirstClient;list;list=g_slist_next(list)) {
            tmp=(Player *)list->data;
            tmp->Flags &= ~FIGHTING;
         }
         text=g_strdup_printf("Jetting to %s",Location[(int)Play->IsAt].Name);
         PrintMessage(text); g_free(text);
         break;
      case C_ENDLIST:
         SetErrandMenus(ClientData.Window);
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
         UpdateInventory(ClientData.Stats.HereList,ClientData.Stats.CarriedList,
                         ClientData.Stats.BuyButton,ClientData.Stats.SellButton,
                         ClientData.Stats.DropButton,Play->Drugs,NumDrug,TRUE);
         if (InventoryWnd) {
            UpdateInventory(NULL,GetDlgItem(InventoryWnd,LB_INVENDRUGS),
                            NULL,NULL,NULL,Play->Drugs,NumDrug,TRUE);
         }
         break;
   }
}

static void StartGame() {
   Player *Play;
   Play=ClientData.Play=g_new(Player,1);
   FirstClient=AddPlayer(0,Play,FirstClient);
   Play->fd=ClientSock;
   SetAbility(Play,A_NEWFIGHT,FALSE);
   SendAbilities(Play);
   SetPlayerName(Play,ClientData.PlayerName);
   SendNullClientMessage(Play,C_NONE,C_NAME,NULL,ClientData.PlayerName);
   InGame=TRUE;
   UpdateControls();
   if (Network) {
      SetSocketWriteTest(ClientData.Play,TRUE);
   }
   SetWindowText(ClientData.Stats.MessageEdit,"");
   UpdatePlayerLists();
}

void EndGame() {
   DisplayFightMessage(NULL);
   g_free(ClientData.PlayerName);
   ClientData.PlayerName=g_strdup(GetPlayerName(ClientData.Play));
   if (Network) {
      WSAAsyncSelect(ClientSock,ClientData.Window,0,0);
   }
   ShutdownNetwork();
   UpdatePlayerLists();
   CleanUpServer();
   InGame=FALSE;
   UpdateControls();
}

void PrintMessage(char *text) {
   int buflen;
   gchar *buffer,**split,*joined;
   if (!text) return;

   while (*text=='^') text++;
   split=g_strsplit(text,"^",0);
   joined=g_strjoinv("\r\n",split);
   g_strfreev(split);

   if (joined[strlen(joined)-1]!='\n') {
      buffer=g_strdup_printf("%s\r\n",joined);
   } else buffer=joined;

   buflen=GetWindowTextLength(ClientData.Stats.MessageEdit);
   SendMessage(ClientData.Stats.MessageEdit,EM_SETSEL,(WPARAM)buflen,
               (WPARAM)buflen);
   SendMessage(ClientData.Stats.MessageEdit,EM_REPLACESEL,FALSE,(LPARAM)buffer);
   g_free(buffer);
}

void UpdateControls() {
   HMENU Menu;
   UINT State;

   Menu=GetMenu(ClientData.Window);
   State = MF_BYPOSITION | (InGame ? MF_ENABLED : MF_GRAYED);

   EnableMenuItem(Menu,1,State);
   EnableMenuItem(Menu,2,State);
   EnableMenuItem(Menu,3,State);
   DrawMenuBar(ClientData.Window);

   EnableWindow(ClientData.Stats.JetButton,InGame);
   if (!InGame) {
      EnableWindow(ClientData.Stats.BuyButton,FALSE);
      EnableWindow(ClientData.Stats.DropButton,FALSE);
      EnableWindow(ClientData.Stats.SellButton,FALSE);
   }
}

void SetSocketWriteTest(Player *Play,gboolean WriteTest) {
   if (Network) {
      WSAAsyncSelect(Play->fd,ClientData.Window,WM_SOCKETDATA,
                     FD_READ|(WriteTest ? FD_WRITE : 0));
   }
}

static void UpdateDealDialog(HWND hwnd,LONG DealType,int DrugInd) {
   GString *text;
   gchar *prstr;
   Player *Play;
   int CanDrop,CanCarry,CanAfford,MaxDrug;

   Play=ClientData.Play;
   text=g_string_new("");

   CanDrop=Play->Drugs[DrugInd].Carried;
   CanCarry=Play->CoatSize;
   dpg_string_sprintf(text,"at %P",Play->Drugs[DrugInd].Price);
   SetDlgItemText(hwnd,ST_DEALPRICE,text->str);

   g_string_sprintf(text,"You are currently carrying %d %s",
                    CanDrop,Drug[DrugInd].Name);
   SetDlgItemText(hwnd,ST_DEALCARRY,text->str);
   if (DealType==BT_BUY) {
      CanAfford=Play->Cash/Play->Drugs[DrugInd].Price;
      g_string_sprintf(text,"You can afford %d, and can carry %d",
                       CanAfford,CanCarry);
      SetDlgItemText(hwnd,ST_DEALLIMIT,text->str);
      MaxDrug=MIN(CanCarry,CanAfford);
   } else {
      MaxDrug=CanDrop;
      SetDlgItemText(hwnd,ST_DEALLIMIT,"");
   }

   SendDlgItemMessage(hwnd,SB_DEALNUM,UDM_SETRANGE,
                      0,(LPARAM)MAKELONG(MaxDrug,0));
   SendDlgItemMessage(hwnd,SB_DEALNUM,UDM_SETPOS,0,(LPARAM)MaxDrug);
/* g_string_sprintf(text,"%d",MaxDrug);
   SetDlgItemText(hwnd,ED_DEALNUM,text->str);*/

   g_string_free(text,TRUE);
}

void UpdatePlayerLists() {
   if (TalkWnd) {
      UpdatePlayerList(GetDlgItem(TalkWnd,LB_TALKPLAYERS),FALSE);
   }
   if (PlayerListWnd) {
      UpdatePlayerList(GetDlgItem(PlayerListWnd,LB_PLAYERLIST),FALSE);
   }
}

void UpdatePlayerList(HWND listwin,gboolean IncludeSelf) {
   GSList *list;
   LRESULT row;
   Player *Play;
   SendMessage(listwin,LB_RESETCONTENT,0,0);
   for (list=FirstClient;list;list=g_slist_next(list)) {
      Play=(Player *)list->data;
      if (IncludeSelf || Play!=ClientData.Play) {
         row=SendMessage(listwin,LB_ADDSTRING,0,(LPARAM)GetPlayerName(Play));
         if (row!=LB_ERR && row!=LB_ERRSPACE) {
            SendMessage(listwin,LB_SETITEMDATA,(WPARAM)row,(LPARAM)Play);
         }
      }
   }
}

BOOL CALLBACK TalkWndProc(HWND hwnd,UINT msg,UINT wParam,
                          LONG lParam) {
   int textlength,listlength,i;
   LRESULT LBResult;
   gchar *text;
   GString *message;
   HWND MessageWnd,ListWnd;
   Player *Play;
   switch(msg) {
      case WM_INITDIALOG:
         CheckDlgButton(hwnd,CB_TALKALL,lParam ? BST_CHECKED : BST_UNCHECKED);
         UpdatePlayerList(GetDlgItem(hwnd,LB_TALKPLAYERS),FALSE);
         return TRUE;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_CANCEL) {
            DestroyWindow(hwnd); return TRUE;
         } else if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==BT_TALKSEND) {
            MessageWnd=GetDlgItem(hwnd,EB_TALKMESSAGE);
            ListWnd=GetDlgItem(hwnd,LB_TALKPLAYERS);
            textlength=GetWindowTextLength(MessageWnd);
            if (textlength>0) {
               textlength++;
               text=g_new(gchar,textlength);
               GetWindowText(MessageWnd,text,textlength);
               SetWindowText(MessageWnd,"");
               message=g_string_new("");
               if (IsDlgButtonChecked(hwnd,CB_TALKALL)==BST_CHECKED) {
                  SendClientMessage(ClientData.Play,C_NONE,C_MSG,NULL,text);
                  g_string_sprintf(message,"%s: %s",
                                   GetPlayerName(ClientData.Play),text);
                  PrintMessage(message->str);
               } else {
                  LBResult=SendMessage(ListWnd,LB_GETCOUNT,0,0);
                  if (LBResult!=LB_ERR) {
                     listlength=(int)LBResult;
                     for (i=0;i<listlength;i++) {
                        LBResult=SendMessage(ListWnd,LB_GETSEL,(WPARAM)i,0);
                        if (LBResult!=LB_ERR && LBResult>0) {
                           LBResult=SendMessage(ListWnd,LB_GETITEMDATA,
                                                (WPARAM)i,0);
                           if (LBResult!=LB_ERR && LBResult) {
                              Play=(Player *)LBResult;
                              SendClientMessage(ClientData.Play,C_NONE,C_MSGTO,
                                                Play,text);
                              g_string_sprintf(message,"%s->%s: %s",
                                               GetPlayerName(ClientData.Play), 
                                               GetPlayerName(Play),text);
                              PrintMessage(message->str);
                           }
                        }
                     }
                  }
               }
               g_free(text);
               g_string_free(message,TRUE);
               return TRUE;
            }
         }
         break;
      case WM_CLOSE:
         DestroyWindow(hwnd);
         return TRUE;
      case WM_DESTROY:
         TalkWnd=NULL;
         break;
   }
   return FALSE;
}

BOOL CALLBACK ScoresWndProc(HWND hwnd,UINT msg,UINT wParam,
                            LONG lParam) {
   switch(msg) {
      case WM_INITDIALOG:
         return TRUE;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_CANCEL) {
            DestroyWindow(hwnd); return TRUE;
         }
         break;
      case WM_CLOSE:
         DestroyWindow(hwnd);
         return TRUE;
      case WM_DESTROY:
         ScoresWnd=NULL;
         break;
   }
   return FALSE;
}

void AddScoreToDialog(char *Data) {
   char *cp;
   int index;
   if (!ScoresWnd) return;
   cp=Data;
   index=GetNextInt(&cp,0);
   if (!cp || strlen(cp)<1) return;
   SendDlgItemMessage(ScoresWnd,LB_HISCORES,LB_INSERTSTRING,index,
                      (LPARAM)&cp[1]);
}

BOOL CALLBACK GunShopWndProc(HWND hwnd,UINT msg,UINT wParam,
                             LONG lParam) {
   LRESULT LBRes,row;
   HWND ListWnd;
   int GunInd;
   gchar *Title;
   GString *text;
   int Tabs=45;
   switch(msg) {
      case WM_INITDIALOG:
         SendMessage(GetDlgItem(hwnd,LB_GUNSHERE),LB_SETTABSTOPS,
                     (WPARAM)1,(LPARAM)&Tabs);
         SendMessage(GetDlgItem(hwnd,LB_GUNSCARRIED),LB_SETTABSTOPS,
                     (WPARAM)1,(LPARAM)&Tabs);
         UpdateInventory(GetDlgItem(hwnd,LB_GUNSHERE),
                         GetDlgItem(hwnd,LB_GUNSCARRIED),
                         GetDlgItem(hwnd,BT_BUYGUN),
                         GetDlgItem(hwnd,BT_SELLGUN),
                         NULL,ClientData.Play->Guns,NumGun,FALSE);
         return TRUE;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED) switch(LOWORD(wParam)) {
            case ID_OK:
               DestroyWindow(hwnd); return TRUE;
            case BT_BUYGUN:
            case BT_SELLGUN:
               if (LOWORD(wParam)==BT_BUYGUN) {
                  ListWnd=GetDlgItem(hwnd,LB_GUNSHERE);
               } else {
                  ListWnd=GetDlgItem(hwnd,LB_GUNSCARRIED);
               }
               row=SendMessage(ListWnd,LB_GETCURSEL,0,0);
               GunInd=-1;
               if (row!=LB_ERR) {
                  LBRes=SendMessage(ListWnd,LB_GETITEMDATA,row,0);
                  if (LBRes!=LB_ERR) GunInd=(int)LBRes;
               }
               if (GunInd==-1) break;
               Title=g_strdup_printf("%s %s",
                        LOWORD(wParam)==BT_BUYGUN ? "Buy" : "Sell",
                        Names.Guns);
               text=g_string_new("");
               if (LOWORD(wParam)==BT_SELLGUN &&
                   TotalGunsCarried(ClientData.Play)==0) {
                  g_string_sprintf(text,"You don't have any %s!",Names.Guns);
                  MessageBox(hwnd,text->str,Title,MB_OK);
               } else if (LOWORD(wParam)==BT_BUYGUN &&
                          Gun[GunInd].Space > ClientData.Play->CoatSize) {
                  g_string_sprintf(text,
                        "You don't have enough space to carry that %s!",
                        Names.Gun);
                  MessageBox(hwnd,text->str,Title,MB_OK);
               } else if (LOWORD(wParam)==BT_BUYGUN &&
                          Gun[GunInd].Price > ClientData.Play->Cash) {
                  g_string_sprintf(text,
                        "You don't have enough cash to buy that %s!",
                        Names.Gun);
                  MessageBox(hwnd,text->str,Title,MB_OK);
               } else if (LOWORD(wParam)==BT_SELLGUN &&
                          ClientData.Play->Guns[GunInd].Carried==0) {
                  MessageBox(hwnd,"You don't have any to sell!",Title,MB_OK);
               } else {
                  g_string_sprintf(text,"gun^%d^%d",GunInd,
                                   LOWORD(wParam)==BT_BUYGUN ? 1 : -1);
                  SendClientMessage(ClientData.Play,C_NONE,C_BUYOBJECT,
                                    NULL,text->str);
               }
               g_free(Title);
               g_string_free(text,TRUE);
               break;
         }
         break;
      case WM_CLOSE:
         DestroyWindow(hwnd);
         return TRUE;
      case WM_DESTROY:
         GunShopWnd=NULL;
         EnableWindow(ClientData.Window,TRUE);
         SendClientMessage(ClientData.Play,C_NONE,C_DONE,NULL,NULL);
         return TRUE;
   }
   return FALSE;
}

static BOOL CALLBACK InventoryWndProc(HWND hwnd,UINT msg,UINT wParam,
                                      LONG lParam) {
   int Tabs=45;
   HWND List;
   switch(msg) {
      case WM_INITDIALOG:
         List=GetDlgItem(hwnd,LB_INVENDRUGS);
         SendMessage(List,LB_SETTABSTOPS,(WPARAM)1,(LPARAM)&Tabs);
         UpdateInventory(NULL,List,NULL,NULL,NULL,
                         ClientData.Play->Drugs,NumDrug,TRUE);
         List=GetDlgItem(hwnd,LB_INVENGUNS);
         SendMessage(List,LB_SETTABSTOPS,(WPARAM)1,(LPARAM)&Tabs);
         UpdateInventory(NULL,List,NULL,NULL,NULL,
                         ClientData.Play->Guns,NumGun,FALSE);
         return TRUE;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_CANCEL) {
            DestroyWindow(hwnd); return TRUE;
         }
         break;
      case WM_CLOSE:
         DestroyWindow(hwnd);
         return TRUE;
      case WM_DESTROY:
         InventoryWnd=NULL;
         break;
   }
   return FALSE;
}

BOOL CALLBACK PlayerListWndProc(HWND hwnd,UINT msg,UINT wParam,
                                LONG lParam) {
   switch(msg) {
      case WM_INITDIALOG:
         UpdatePlayerList(GetDlgItem(hwnd,LB_PLAYERLIST),FALSE);
         return TRUE;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_CANCEL) {
            DestroyWindow(hwnd); return TRUE;
         }
         break;
      case WM_CLOSE:
         DestroyWindow(hwnd);
         return TRUE;
      case WM_DESTROY:
         PlayerListWnd=NULL;
         break;
   }
   return FALSE;
}

BOOL CALLBACK NewNameWndProc(HWND hwnd,UINT msg,UINT wParam,
                             LONG lParam) {
   int buflen;
   gchar *text;
   HWND EditWnd;
   switch(msg) {
      case WM_INITDIALOG:
         SetDlgItemText(hwnd,EB_NEWNAME,GetPlayerName(ClientData.Play));
         return TRUE;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_OK) {
            EditWnd=GetDlgItem(hwnd,EB_NEWNAME);
            buflen=GetWindowTextLength(EditWnd);
            text=g_new(char,buflen+1);
            GetWindowText(EditWnd,text,buflen+1);
            SetPlayerName(ClientData.Play,text);
            SendNullClientMessage(ClientData.Play,C_NONE,C_NAME,NULL,text);
            g_free(text);
            DestroyWindow(hwnd);
            return TRUE;
         }
         break;
      case WM_DESTROY:
         EndDialog(hwnd,0); return TRUE;
   }
   return FALSE;
}

BOOL CALLBACK TransferWndProc(HWND hwnd,UINT msg,UINT wParam,
                              LONG lParam) {
   static char Type;
   HWND MoneyWnd;
   int buflen;
   price_t money;
   gchar *text,*prstr;
   switch(msg) {
      case WM_INITDIALOG:
         Type=(char)lParam;
         text=dpg_strdup_printf("Cash: %P",ClientData.Play->Cash);
         SetDlgItemText(hwnd,ST_MONEY,text);
         g_free(text); g_free(prstr);
         if (Type==C_BANK) {
            CheckDlgButton(hwnd,RB_WITHDRAW,BST_CHECKED);
            text=dpg_strdup_printf("Bank: %P",ClientData.Play->Bank);
         } else {
            text=dpg_strdup_printf("Debt: %P",ClientData.Play->Debt);
         }
         SetDlgItemText(hwnd,ST_BANK,text);
         g_free(text);
         return TRUE;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED) switch(LOWORD(wParam)) {
            case ID_CANCEL:
               SendMessage(hwnd,WM_CLOSE,0,0);
               return TRUE;
            case ID_OK:
               MoneyWnd=GetDlgItem(hwnd,EB_MONEY);
               buflen=GetWindowTextLength(MoneyWnd);
               prstr=g_new(char,buflen+1);
               GetWindowText(MoneyWnd,prstr,buflen+1);
               money=strtoprice(prstr); g_free(prstr);
               if (money<0) money=0;
               if (Type==C_LOANSHARK) {
                  if (money>ClientData.Play->Debt) money=ClientData.Play->Debt;
               } else {
                  if (IsDlgButtonChecked(hwnd,RB_WITHDRAW)==BST_CHECKED) {
                     money=-money;
                  }
                  if (-money>ClientData.Play->Bank) {
                     MessageBox(hwnd,
                                "There isn't that much money in the bank...",
                                "Bank",MB_OK);
                     return TRUE;
                  }
               }
               if (money>ClientData.Play->Cash) {
                  MessageBox(hwnd,"You don't have that much money!",
                             Type==C_LOANSHARK ? "Pay loan" : "Bank",MB_OK);
                  return TRUE;
               }
               text=pricetostr(money);
               SendClientMessage(ClientData.Play,C_NONE,
                                 Type==C_LOANSHARK ? C_PAYLOAN : C_DEPOSIT,
                                 NULL,text);
               g_free(text);
               SendMessage(hwnd,WM_CLOSE,0,0);
               return TRUE;
         }
         break;
      case WM_CLOSE:
         EndDialog(hwnd,0);
         SendClientMessage(ClientData.Play,C_NONE,C_DONE,NULL,NULL);
         return TRUE;
   }
   return FALSE;
}

BOOL CALLBACK QuestionWndProc(HWND hwnd,UINT msg,UINT wParam,
                              LONG lParam) {
   char *Data;
   HWND button;
   gchar *Words[] = { "&Yes", "&No", "&Run", "&Fight", "&Attack", "&Evade" };
   gint NumWords = sizeof(Words) / sizeof(Words[0]);
   gint i,x,xspacing,y,Width,Height,NumButtons;
   RECT rect;
   gchar **split,*Responses,*LabelText;
   gint Answer;
   switch(msg) {
      case WM_INITDIALOG:
         Data=(char *)lParam;
         split=g_strsplit(Data,"^",1);
         if (!split[0] || !split[1]) {
            g_warning("Bad QUESTION message %s",Data); return TRUE;
         }
         g_strdelimit(split[1],"^",'\n');
         Responses=split[0]; LabelText=split[1];
         while (*LabelText=='\n') LabelText++;
         SetDlgItemText(hwnd,ST_TEXT,LabelText);
         GetClientRect(hwnd,&rect);
         NumButtons=0;
         for (i=0;i<NumWords;i++) {
            Answer=Words[i][0];
            if (Answer=='&' && strlen(Words[i])>=2) Answer=Words[i][1];
            if (strchr(Responses,Answer)) NumButtons++;
         }
         y=rect.bottom-38;
         Width=60; Height=28;
         xspacing=(rect.right-20)/(NumButtons+1);
         NumButtons=0;
         for (i=0;i<NumWords;i++) {
            Answer=Words[i][0];
            if (Answer=='&' && strlen(Words[i])>=2) Answer=Words[i][1];
            if (strchr(Responses,Answer)) {
               NumButtons++;
               x=10+xspacing*NumButtons-Width/2;
               button=CreateWindow("BUTTON",Words[i],
                                   WS_CHILD|WS_TABSTOP|WS_VISIBLE|BS_PUSHBUTTON,
                                   x,y,Width,Height,hwnd,
                                   (HMENU)Answer,hInst,NULL);
            }
         }
         g_strfreev(split);
         break;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED) {
            EndDialog(hwnd,LOWORD(wParam));
            return TRUE;
         }
         break;
      case WM_CLOSE:
         EndDialog(hwnd,0); return TRUE;
   }
   return FALSE;
}

static BOOL CALLBACK DealWndProc(HWND hwnd,UINT msg,UINT wParam,
                                LONG lParam) {
   static LONG DealType=0;
   static char *DealString="";
   gchar *text;
   char Num[40];
   Player *Play;
   HWND ListWnd,UpDownWnd;
   LRESULT Selection,Index,Data;
   static int DrugInd;
   int i,FirstInd,amount;
   gboolean DrugIndOK;
   switch(msg) {
      case WM_INITDIALOG:
         UpDownWnd=CreateUpDownControl(WS_CHILD|WS_BORDER|WS_VISIBLE|
                                       UDS_ALIGNRIGHT|UDS_ARROWKEYS|
                                       UDS_SETBUDDYINT|UDS_NOTHOUSANDS,
                                       0,0,1,1,hwnd,SB_DEALNUM,hInst,
                                       GetDlgItem(hwnd,ED_DEALNUM),
                                       0,100,100);
         SendMessage(UpDownWnd,UDM_SETBASE,10,0);
         Play=ClientData.Play;
         DealType=lParam;
         switch(DealType) {
            case BT_BUY:  DealString="Buy";  break;
            case BT_SELL: DealString="Sell"; break;
            case BT_DROP: DealString="Drop"; break;
         }
         SetDlgItemText(hwnd,ST_DEALTYPE,DealString);
         text=g_strdup_printf("%s how many?",DealString);
         SetDlgItemText(hwnd,ST_DEALNUM,text); g_free(text);
         if (DealType==BT_BUY) ListWnd=ClientData.Stats.HereList;
         else ListWnd=ClientData.Stats.CarriedList;
         DrugInd=-1;
         Selection=SendMessage(ListWnd,LB_GETCURSEL,0,0);
         if (Selection!=LB_ERR) {
            Data=SendMessage(ListWnd,LB_GETITEMDATA,Selection,0);
            if (Data!=LB_ERR) DrugInd=(int)Data;
         }

         DrugIndOK=FALSE;
         FirstInd=-1;
         for (i=0;i<NumDrug;i++) {
            if ((DealType==BT_DROP && Play->Drugs[i].Carried>0 &&
                 Play->Drugs[i].Price==0) ||
                (DealType==BT_SELL && Play->Drugs[i].Carried>0 &&
                 Play->Drugs[i].Price!=0) ||
                (DealType==BT_BUY && Play->Drugs[i].Price!=0)) {
               if (FirstInd==-1) FirstInd=i;
               if (DrugInd==i) DrugIndOK=TRUE;
            }
         }
         if (!DrugIndOK) {
            if (FirstInd==-1) { EndDialog(hwnd,0); return TRUE; }
            else DrugInd=FirstInd;
         }
         for (i=0;i<NumDrug;i++) {
            if ((DealType==BT_DROP && Play->Drugs[i].Carried>0 &&
                 Play->Drugs[i].Price==0) ||
                (DealType==BT_SELL && Play->Drugs[i].Carried>0 &&
                 Play->Drugs[i].Price!=0) ||
                (DealType==BT_BUY && Play->Drugs[i].Price!=0)) {
               Index=SendDlgItemMessage(hwnd,CB_DEALDRUG,CB_ADDSTRING,
                                        0,(LPARAM)Drug[i].Name);
               if (Index!=CB_ERR) {
                  SendDlgItemMessage(hwnd,CB_DEALDRUG,CB_SETITEMDATA,Index,i);
                  if (i==DrugInd) {
                     SendDlgItemMessage(hwnd,CB_DEALDRUG,CB_SETCURSEL,Index,0);
                  }
               }
            }
         }
         UpdateDealDialog(hwnd,DealType,DrugInd);
         break;
      case WM_COMMAND:
         if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==CB_DEALDRUG) {
            Index=SendDlgItemMessage(hwnd,CB_DEALDRUG,CB_GETCURSEL,0,0);
            if (Index!=CB_ERR) {
               Data=SendDlgItemMessage(hwnd,CB_DEALDRUG,CB_GETITEMDATA,Index,0);
               if (Data!=CB_ERR) {
                  DrugInd=(int)Data;
                  UpdateDealDialog(hwnd,DealType,DrugInd);
               }
            }
         } else if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_OK) {
            Num[0]=0;
            GetDlgItemText(hwnd,ED_DEALNUM,Num,39);
            amount=atoi(Num);
            text=g_strdup_printf("drug^%d^%d",DrugInd,
                                 DealType==BT_BUY ? amount : -amount);
            SendClientMessage(ClientData.Play,C_NONE,C_BUYOBJECT,NULL,text);
            g_free(text);
            EndDialog(hwnd,0); return TRUE;
         } else if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_CANCEL) {
            EndDialog(hwnd,0); return TRUE;
         }
         break;
      case WM_CLOSE:
         EndDialog(hwnd,0); return TRUE;
   }
   return FALSE;
}

static BOOL CALLBACK JetWndProc(HWND hwnd,UINT msg,UINT wParam,
                                LONG lParam) {
   gint boxsize,i,row,col,ButtonWidth,NewLocation;
   HWND button;
   gchar *name,AccelChar;
   SIZE TextSize;
   HDC hDC;
   gchar *text;
   ButtonWidth=40;
   switch(msg) {
      case WM_INITDIALOG:
         boxsize=1;
         while (boxsize*boxsize < NumLocation) boxsize++;
         for (i=0;i<NumLocation;i++) {
            if (i<9) AccelChar='1'+i;
            else if (i<35) AccelChar='A'+i-9;
            else AccelChar='\0';
            row=i/boxsize; col=i%boxsize;
            if (AccelChar=='\0') name=Location[i].Name;
            else {
               name=g_strdup_printf("&%c. %s",AccelChar,Location[i].Name);
            }
            button=CreateWindow("BUTTON",name,
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|BS_PUSHBUTTON,
                                0,0,1,1,hwnd,(HMENU)i,hInst,NULL);
            hDC=GetDC(button);
            if (GetTextExtentPoint32(hDC,name,strlen(name),&TextSize) &&
                TextSize.cx+15 > ButtonWidth) {
               ButtonWidth=TextSize.cx+15;
            }
            ReleaseDC(button,hDC);
            if (i==ClientData.Play->IsAt) EnableWindow(button,FALSE);
            if (AccelChar!='\0') g_free(name);
         }
         for (i=0;i<NumLocation;i++) {
            row=i/boxsize; col=i%boxsize;
            MoveWindow(GetDlgItem(hwnd,i),
                       7+col*(ButtonWidth+5),38+row*33,ButtonWidth,28,FALSE);
         }
         SetWindowPos(hwnd,HWND_TOP,0,0,
                      14+boxsize*(ButtonWidth+5),65+boxsize*33,
                      SWP_NOMOVE|SWP_NOOWNERZORDER);
         break;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED) {
            EndDialog(hwnd,0);
            NewLocation=(gint)(LOWORD(wParam));
            text=g_strdup_printf("%d",NewLocation);
            SendClientMessage(ClientData.Play,C_NONE,C_REQUESTJET,NULL,text);
            g_free(text);
            return TRUE;
         }
         break;
      case WM_CLOSE:
         EndDialog(hwnd,0); return TRUE;
   }
   return FALSE;
}

#define MaxInd 3
#define NumIDs 5

static void ShowPage(HWND hwnd,int TabIndex,int ShowType) {
   static int IDs[MaxInd][NumIDs]={
      { ST_HOSTNAME,ST_PORT,ED_HOSTNAME,ED_PORT,BT_CONNECT },
      { CB_ANTIQUE,BT_STARTSINGLE,0,0,0 },
      { LB_SERVERLIST,BT_UPDATE,0,0,0 }
   };
   int i;
   if (!hwnd || TabIndex<0 || TabIndex>=MaxInd) return;

   for (i=0;i<NumIDs;i++) if (IDs[TabIndex][i]!=0) {
      ShowWindow(GetDlgItem(hwnd,IDs[TabIndex][i]),ShowType);
   }
}

static gboolean GetEnteredName(HWND hwnd) {
   int buflen;
   g_free(ClientData.PlayerName);
   buflen=GetWindowTextLength(GetDlgItem(hwnd,ED_NAME));
   ClientData.PlayerName=g_new(char,buflen+1);
   GetDlgItemText(hwnd,ED_NAME,ClientData.PlayerName,buflen+1);
   return (ClientData.PlayerName && ClientData.PlayerName[0]);
}

static void FillMetaServerList(HWND hwnd) {
   ServerData *ThisServer;
   GSList *ListPt;
   GString *text;

   text=g_string_new("");
   SendMessage(hwnd,LB_RESETCONTENT,0,0);
   for (ListPt=ServerList;ListPt;ListPt=g_slist_next(ListPt)) {
      ThisServer=(ServerData *)(ListPt->data);
      g_string_sprintf(text,"%s\t%d",ThisServer->Name,ThisServer->Port);
      SendMessage(hwnd,LB_ADDSTRING,0,(LPARAM)text->str);
   }
   g_string_free(text,TRUE);
}

static void UpdateMetaServerList(HWND hwnd) {
   char *MetaError;
   int HttpSock;
   MetaError=OpenMetaServerConnection(&HttpSock);

   if (MetaError) {
      return;
   }
   ReadMetaServerData(HttpSock);
   CloseMetaServerConnection(HttpSock);
   MetaServerRead=TRUE;
   FillMetaServerList(hwnd);
}

static BOOL CALLBACK ErrandWndProc(HWND hwnd,UINT msg,UINT wParam,
                                   LONG lParam) {
   LRESULT LBResult;
   HWND ListWnd;
   Player *Play;
   static LONG ErrandType=0;
   switch(msg) {
      case WM_INITDIALOG:
         ErrandType=lParam;
         UpdatePlayerList(GetDlgItem(hwnd,LB_ERRANDPLAY),FALSE);
         break;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_CANCEL) {
            EndDialog(hwnd,1); return TRUE;
         } else if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_OK) {
            ListWnd=GetDlgItem(hwnd,LB_ERRANDPLAY);
            LBResult=SendMessage(ListWnd,LB_GETCURSEL,0,0);
            if (LBResult!=LB_ERR) {
               LBResult=SendMessage(ListWnd,LB_GETITEMDATA,LBResult,0);
               if (LBResult!=LB_ERR) {
                  Play=(Player *)LBResult;
                  SendClientMessage(ClientData.Play,C_NONE,
                                    ErrandType==ID_SPY ? C_SPYON : C_TIPOFF,
                                    Play,NULL);
                  EndDialog(hwnd,0); return TRUE;
               }
            }
         }
         break;
      case WM_CLOSE:
         EndDialog(hwnd,0); return TRUE;
   }
   return FALSE;
}

static BOOL CALLBACK NewGameWndProc(HWND hwnd,UINT msg,UINT wParam,
                                    LONG lParam) {
   gchar *NetworkError;
   int buflen;
   static HWND tabwnd=NULL;
   TC_ITEM tie;
   RECT rect;
   NMHDR *nmhdr;
   switch(msg) {
      case WM_INITDIALOG:
         if (ClientData.PlayerName) {
            SetDlgItemText(hwnd,ED_NAME,ClientData.PlayerName);
         }
         SetDlgItemText(hwnd,ED_HOSTNAME,ServerName);
         SetDlgItemInt(hwnd,ED_PORT,Port,FALSE);
         CheckDlgButton(hwnd,CB_ANTIQUE,
                        WantAntique ? BST_CHECKED : BST_UNCHECKED);
         GetClientRect(hwnd,&rect);
         tabwnd=CreateWindow(WC_TABCONTROL,"",
                             WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
                             10,40,rect.right-20,rect.bottom-50,hwnd,
                             NULL,hInst,NULL);
         tie.mask = TCIF_TEXT | TCIF_IMAGE;
         tie.iImage = -1;
         tie.pszText = "Server";
         TabCtrl_InsertItem(tabwnd,0,&tie);
         tie.pszText = "Single player";
         TabCtrl_InsertItem(tabwnd,1,&tie);
         tie.pszText = "Metaserver";
         TabCtrl_InsertItem(tabwnd,2,&tie);
         ShowPage(hwnd,TabCtrl_GetCurSel(tabwnd),SW_SHOW);
         FillMetaServerList(GetDlgItem(hwnd,LB_SERVERLIST));
         return TRUE;
      case WM_NOTIFY:
         nmhdr = (NMHDR *)lParam;
         if (nmhdr && nmhdr->code==TCN_SELCHANGE) {
            ShowPage(hwnd,TabCtrl_GetCurSel(tabwnd),SW_SHOW);
         } else if (nmhdr && nmhdr->code==TCN_SELCHANGING) {
            ShowPage(hwnd,TabCtrl_GetCurSel(tabwnd),SW_HIDE);
         }
         break;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==BT_CONNECT) {
            if (!GetEnteredName(hwnd)) return TRUE;
            buflen=GetWindowTextLength(GetDlgItem(hwnd,ED_HOSTNAME));
            GetDlgItemText(hwnd,ED_HOSTNAME,ServerName,buflen+1);
            Port=GetDlgItemInt(hwnd,ED_PORT,NULL,FALSE);
            NetworkError=SetupNetwork(FALSE);
            if (!NetworkError) {
               EndDialog(hwnd,1);
               StartGame();
            }
            return TRUE;
         } else if (HIWORD(wParam)==BN_CLICKED &&
                    LOWORD(wParam)==BT_STARTSINGLE) {
            if (IsDlgButtonChecked(hwnd,CB_ANTIQUE)==BST_CHECKED)
               WantAntique=TRUE; else WantAntique=FALSE;
            if (!GetEnteredName(hwnd)) return TRUE;
            EndDialog(hwnd,1);
            StartGame();
            return TRUE;
         } else if (HIWORD(wParam)==BN_CLICKED &&
                    LOWORD(wParam)==BT_UPDATE) {
            UpdateMetaServerList(GetDlgItem(hwnd,LB_SERVERLIST));
            return TRUE;
         }
         break;
      case WM_CLOSE:
         EndDialog(hwnd,0); return TRUE;
   }
   return FALSE;
}

BOOL CALLBACK AboutWndProc(HWND hwnd,UINT msg,UINT wParam,LONG lParam) {
   switch(msg) {
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_OK) {
            EndDialog(hwnd,1); return TRUE;
         }
         break;
      case WM_CLOSE:
         EndDialog(hwnd,0); return TRUE;
   }
   return FALSE;
}

void CreateStats(HWND hwnd,struct STATS *Stats,
                 gboolean CreateEdit,gboolean CreateButtons) {
   int Tabs=45;
   if (!Stats) return;
   memset(Stats,0,sizeof(struct STATS));
   if (CreateButtons) {
      Stats->SellButton=CreateWindow("BUTTON","<- &Sell",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|BS_PUSHBUTTON,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
      Stats->BuyButton=CreateWindow("BUTTON","&Buy ->",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|BS_PUSHBUTTON,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
      Stats->DropButton=CreateWindow("BUTTON","&Drop <-",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|BS_PUSHBUTTON,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
      Stats->JetButton=CreateWindow("BUTTON","&Jet!",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|BS_PUSHBUTTON,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   }
   Stats->StatGroup=CreateWindow("BUTTON","Stats",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|BS_GROUPBOX,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   Stats->HereList=CreateWindow("LISTBOX","Here",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|WS_VSCROLL|
                                WS_BORDER|LBS_HASSTRINGS|LBS_USETABSTOPS,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   SendMessage(Stats->HereList,LB_SETTABSTOPS,
               (WPARAM)1,(LPARAM)&Tabs);
   Stats->CarriedList=CreateWindow("LISTBOX","Carried",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|WS_VSCROLL|
                                WS_BORDER|LBS_HASSTRINGS|LBS_USETABSTOPS,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   SendMessage(Stats->CarriedList,LB_SETTABSTOPS,
               (WPARAM)1,(LPARAM)&Tabs);
   if (CreateEdit) {
      Stats->MessageEdit=CreateWindow("EDIT","",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|WS_VSCROLL|
                                WS_BORDER|ES_LEFT|ES_AUTOVSCROLL|
                                ES_MULTILINE|ES_READONLY,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   }
   Stats->Location=CreateWindow("STATIC","",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|SS_CENTER,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   Stats->Date=CreateWindow("STATIC","",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|SS_CENTER,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   Stats->Space=CreateWindow("STATIC","",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|SS_CENTER,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   Stats->Cash=CreateWindow("STATIC","",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|SS_CENTER,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   Stats->Debt=CreateWindow("STATIC","",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|SS_CENTER,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   Stats->Bank=CreateWindow("STATIC","",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|SS_CENTER,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   Stats->Guns=CreateWindow("STATIC","",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|SS_CENTER,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   Stats->Bitches=CreateWindow("STATIC","",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|SS_CENTER,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
   Stats->Health=CreateWindow("STATIC","",
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE|SS_CENTER,
                                0,0,1,1,hwnd,NULL,hInst,NULL);
}

void SizeStats(HWND hwnd,struct STATS *Stats,RECT *rect) {
   LONG EditDepth,ListDepth,editspace,TextHeight;
   HDC hDC;
   int width,depth,ButtonX,ButtonY,ButtonWid;
   TEXTMETRIC TextMetric;
   if (!Stats || !rect) return;

   width=rect->right-rect->left;
   depth=rect->bottom-rect->top;

   if (Stats->MessageEdit) EditDepth=(depth-126)/2; else EditDepth=0;
   ListDepth=(depth-126)-EditDepth;
   editspace=(width-25)/3;
   hDC=GetDC(Stats->Location);
   GetTextMetrics(hDC,&TextMetric);
   ReleaseDC(Stats->Location,hDC);
   TextHeight=TextMetric.tmHeight;

   MoveWindow(Stats->StatGroup,rect->left+10,rect->top+10,
              width-20,86,TRUE);
   MoveWindow(Stats->Location,rect->left+15,rect->top+25,
              editspace-5,TextHeight,TRUE);
   MoveWindow(Stats->Date,rect->left+15+editspace,rect->top+25,
              editspace-5,TextHeight,TRUE);
   MoveWindow(Stats->Space,rect->left+15+2*editspace,rect->top+25,
              editspace-5,TextHeight,TRUE);
   MoveWindow(Stats->Cash,rect->left+15,rect->top+25+TextHeight+4,
              editspace-5,TextHeight,TRUE);
   MoveWindow(Stats->Debt,rect->left+15+editspace,rect->top+25+TextHeight+4,
              editspace-5,TextHeight,TRUE);
   MoveWindow(Stats->Bank,rect->left+15+2*editspace,rect->top+25+TextHeight+4,
              editspace-5,TextHeight,TRUE);
   MoveWindow(Stats->Guns,rect->left+15,rect->top+25+TextHeight*2+8,
              editspace-5,TextHeight,TRUE);
   MoveWindow(Stats->Bitches,rect->left+15+editspace,
              rect->top+25+TextHeight*2+8,editspace-5,TextHeight,TRUE);
   MoveWindow(Stats->Health,rect->left+15+2*editspace,
              rect->top+25+TextHeight*2+8,editspace-5,TextHeight,TRUE);
   if (Stats->MessageEdit) {
      MoveWindow(Stats->MessageEdit,rect->left+10,rect->top+106,
                 width-20,EditDepth,TRUE);
   }
   if (Stats->SellButton) {
      ButtonWid=60;
      ButtonX=rect->left+width/2-30;
      ButtonY=rect->top+116+EditDepth;
      MoveWindow(Stats->SellButton,ButtonX,ButtonY,60,28,TRUE);
      MoveWindow(Stats->BuyButton,ButtonX,ButtonY+35,60,28,TRUE);
      MoveWindow(Stats->DropButton,ButtonX,ButtonY+70,60,28,TRUE);
      MoveWindow(Stats->JetButton,ButtonX,ButtonY+105,60,28,TRUE);
   } else ButtonWid=0;
   MoveWindow(Stats->HereList,rect->left+10,rect->top+116+EditDepth,
              (width-ButtonWid)/2-15,ListDepth,TRUE);
   MoveWindow(Stats->CarriedList,
              rect->left+(width+ButtonWid)/2+5,rect->top+116+EditDepth,
              (width-ButtonWid)/2-15,ListDepth,TRUE);
}

void ShowStats(struct STATS *Stats,int State) {
   ShowWindow(Stats->StatGroup,State);
   ShowWindow(Stats->Location,State);
   ShowWindow(Stats->Date,State);
   ShowWindow(Stats->Space,State);
   ShowWindow(Stats->Space,State);
   ShowWindow(Stats->Cash,State);
   ShowWindow(Stats->Debt,State);
   ShowWindow(Stats->Bank,State);
   ShowWindow(Stats->Guns,State);
   ShowWindow(Stats->Bitches,State);
   ShowWindow(Stats->Health,State);
   if (Stats->MessageEdit) ShowWindow(Stats->MessageEdit,State);
   if (Stats->SellButton) {
      ShowWindow(Stats->SellButton,State);
      ShowWindow(Stats->BuyButton,State);
      ShowWindow(Stats->DropButton,State);
      ShowWindow(Stats->JetButton,State);
   }
   ShowWindow(Stats->HereList,State);
   ShowWindow(Stats->CarriedList,State);
}

LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg,UINT wParam,LONG lParam) {
   RECT rect;
   char *pt;
   switch(msg) {
      case WM_CREATE:
         CreateStats(hwnd,&ClientData.Stats,TRUE,TRUE);
         ClientData.PlayerName=NULL;
         ClientData.Play=NULL;
         ClientMessageHandlerPt = HandleClientMessage;
         SocketWriteTestPt = SetSocketWriteTest;
         g_log_set_handler(NULL,G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING,
                           LogMessage,NULL);
         UpdateControls();
         break;
      case WM_SIZE:
         GetClientRect(hwnd,&rect);
         SizeStats(hwnd,&ClientData.Stats,&rect);
         break;
      case WM_CLOSE:
         if (!InGame || MessageBox(hwnd,"Abandon current game?",
                                   "Quit Game",MB_YESNO)==IDYES) {
            DestroyWindow(hwnd);
         }
         break;
      case WM_DESTROY:
         PostQuitMessage(0);
         break;
      case WM_SOCKETDATA:
         if (WSAGETSELECTEVENT(lParam)==FD_WRITE) {
            WriteConnectionBufferToWire(ClientData.Play);
            if (ClientData.Play->WriteBuf.DataPresent==0) {
               SetSocketWriteTest(ClientData.Play,FALSE);
            }
         } else {
            if (ReadConnectionBufferFromWire(ClientData.Play)) {
               while ((pt=ReadFromConnectionBuffer(ClientData.Play))!=NULL) {
                  HandleClientMessage(pt,ClientData.Play); g_free(pt);
               }
            } else {
               if (Network) WSAAsyncSelect(ClientSock,ClientData.Window,0,0);
               g_warning("Connection to server lost - switching "
                         "to single player mode");
               SwitchToSinglePlayer(ClientData.Play);
            }
         }
         break;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED &&
             (HWND)lParam==ClientData.Stats.JetButton) {
            if (ClientData.Play->Flags & FIGHTING) {
               DisplayFightMessage("");
            } else {
               DialogBox(hInst,MAKEINTRESOURCE(JetDialog),hwnd,JetWndProc);
            }
         } else if (HIWORD(wParam)==BN_CLICKED &&
             (HWND)lParam==ClientData.Stats.BuyButton) {
            DialogBoxParam(hInst,MAKEINTRESOURCE(DealDialog),
                           hwnd,DealWndProc,BT_BUY);
         } else if (HIWORD(wParam)==BN_CLICKED &&
             (HWND)lParam==ClientData.Stats.SellButton) {
            DialogBoxParam(hInst,MAKEINTRESOURCE(DealDialog),
                           hwnd,DealWndProc,BT_SELL);
         } else if (HIWORD(wParam)==BN_CLICKED &&
             (HWND)lParam==ClientData.Stats.DropButton) {
            DialogBoxParam(hInst,MAKEINTRESOURCE(DealDialog),
                           hwnd,DealWndProc,BT_DROP);
         } else if (HIWORD(wParam)==0) switch(LOWORD(wParam)) {
            case ID_EXIT:
               SendMessage(hwnd,WM_CLOSE,0,0);
               break;
            case ID_NEWGAME:
               if (InGame && MessageBox(hwnd,"Abandon current game?",
                                        "Start new game",MB_YESNO)==IDYES) {
                  EndGame();
               }
               if (!InGame) DialogBox(hInst,MAKEINTRESOURCE(NewGameDialog),
                                      hwnd,NewGameWndProc);
               break;
            case ID_TALKTOALL:
            case ID_TALKTOPLAYERS:
               if (TalkWnd) {
                  SetWindowPos(TalkWnd,HWND_TOP,0,0,0,0,
                               SWP_NOMOVE|SWP_NOSIZE);
               } else {
                  TalkWnd=CreateDialogParam(hInst,
                                  MAKEINTRESOURCE(TalkDialog),hwnd,
                                  TalkWndProc,
                                  LOWORD(wParam)==ID_TALKTOALL ? TRUE : FALSE);
                  ShowWindow(TalkWnd,SW_SHOWNORMAL);
               }
               break;
            case ID_SPY:
               DialogBoxParam(hInst,MAKEINTRESOURCE(ErrandDialog),hwnd,
                              ErrandWndProc,(LPARAM)ID_SPY);
               break;
            case ID_TIPOFF:
               DialogBoxParam(hInst,MAKEINTRESOURCE(ErrandDialog),hwnd,
                              ErrandWndProc,(LPARAM)ID_TIPOFF);
               break;
            case ID_SACKBITCH:
               if (MessageBox(hwnd,"Are you sure? (Any drugs or guns carried\n"
                              "by this bitch may be lost!)",
                              "Sack Bitch",MB_YESNO)==IDYES) {
                  SendClientMessage(ClientData.Play,C_NONE,C_SACKBITCH,
                                    NULL,NULL);
               }
               break;
            case ID_GETSPY:
               if (SpyReportsWnd) DestroyWindow(SpyReportsWnd);
               SpyReportsWnd=NULL;
               SendClientMessage(ClientData.Play,C_NONE,C_CONTACTSPY,
                                 NULL,NULL);
               break;
            case ID_LISTPLAYERS:
               if (PlayerListWnd) {
                  SetWindowPos(PlayerListWnd,HWND_TOP,0,0,0,0,
                               SWP_NOMOVE|SWP_NOSIZE);
               } else {
                  PlayerListWnd=CreateDialog(hInst,
                                     MAKEINTRESOURCE(PlayerListDia),hwnd,
                                     PlayerListWndProc);
                  ShowWindow(PlayerListWnd,SW_SHOWNORMAL);
               }
               break;
            case ID_LISTINVENTORY:
               if (InventoryWnd) {
                  SetWindowPos(InventoryWnd,HWND_TOP,0,0,0,0,
                               SWP_NOMOVE|SWP_NOSIZE);
               } else {
                  InventoryWnd=CreateDialog(hInst,
                                     MAKEINTRESOURCE(InventoryDia),hwnd,
                                     InventoryWndProc);
                  ShowWindow(InventoryWnd,SW_SHOWNORMAL);
               }
               break;
            case ID_LISTSCORES:
               SendClientMessage(ClientData.Play,C_NONE,C_REQUESTSCORE,
                                 NULL,NULL);
               break;
            case ID_ABOUT:
               DialogBox(hInst,MAKEINTRESOURCE(AboutDialog),hwnd,AboutWndProc);
               break;
         }
      default:
         return DefWindowProc(hwnd,msg,wParam,lParam);
   }
   return FALSE;
}

static BOOL CALLBACK FightWndProc(HWND hwnd,UINT msg,WPARAM wParam,
                                  LPARAM lParam) {
   char text[10];
   switch(msg) {
      case WM_INITDIALOG:
         return TRUE;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED) switch(LOWORD(wParam)) {
            case BT_DEALDRUGS:
               EnableWindow(ClientData.Window,TRUE);
               EnableWindow(hwnd,FALSE);
               ShowWindow(hwnd,SW_HIDE);
               return TRUE;
            case BT_FIGHT:
               ClientData.Play->Flags &= ~CANSHOOT;
               if (TotalGunsCarried(ClientData.Play)==0) {
                  text[0]='S';
               } else {
                  text[0]='F';
               }
               text[1]='\0';
               SendClientMessage(ClientData.Play,C_NONE,C_FIGHTACT,NULL,text);
               return TRUE;
            case BT_RUN:
               EnableWindow(ClientData.Window,TRUE);
               EnableWindow(hwnd,FALSE);
               ShowWindow(hwnd,SW_HIDE);
               DialogBox(hInst,MAKEINTRESOURCE(JetDialog),
                         ClientData.Window,JetWndProc);
               return TRUE;
         }
         break;
      case WM_DESTROY:
         EnableWindow(ClientData.Window,TRUE);
         FightWnd=NULL;
         break;
   }
   return FALSE;
}

void DisplayFightMessage(char *Data) {
   int buflen;
   gchar *buffer,**split,*joined;
   HWND EditWnd,Fight,Run;
   if (!Data) {
      if (FightWnd) DestroyWindow(FightWnd);
      FightWnd=NULL;
      return;
   }
   EnableWindow(ClientData.Window,FALSE);
   if (!FightWnd) {
      FightWnd=CreateDialog(hInst,MAKEINTRESOURCE(FightDialog),
                            ClientData.Window,FightWndProc);
   }
   ShowWindow(FightWnd,SW_SHOW);
   EnableWindow(FightWnd,TRUE);

   while (*Data=='^') Data++;
   split=g_strsplit(Data,"^",0);
   joined=g_strjoinv("\r\n",split);
   g_strfreev(split);

   if (joined[strlen(joined)-1]!='\n') {
      buffer=g_strdup_printf("%s\r\n",joined);
   } else buffer=joined;

   EditWnd=GetDlgItem(FightWnd,EB_FIGHTSTATUS);

   buflen=GetWindowTextLength(EditWnd);
   SendMessage(EditWnd,EM_SETSEL,(WPARAM)buflen,(WPARAM)buflen);
   SendMessage(EditWnd,EM_REPLACESEL,FALSE,(LPARAM)buffer);
   g_free(buffer);

   Fight=GetDlgItem(FightWnd,BT_FIGHT);
   Run=GetDlgItem(FightWnd,BT_RUN);
   if (TotalGunsCarried(ClientData.Play)==0) {
      SetWindowText(Fight,"&Stand");
   } else {
      SetWindowText(Fight,"&Fight");
   }
   ShowWindow(Fight,ClientData.Play->Flags&CANSHOOT ? SW_SHOW : SW_HIDE);
   ShowWindow(Run,ClientData.Play->Flags&FIGHTING ? SW_SHOW : SW_HIDE);
}

static BOOL CALLBACK SpyReportsWndProc(HWND hwnd,UINT msg,WPARAM wParam,
                                       LPARAM lParam) {
   Player *Play;
   TC_ITEM tie;
   RECT rect,StatRect;
   static HWND tab=NULL;
   static int TabPos=0;
   static GSList *TabList=NULL;
   GSList *list;
   struct STATS *Stats;
   static struct STATS *SelStats=NULL;
   NMHDR *nmhdr;
   switch(msg) {
      case WM_INITDIALOG:
         tab=CreateWindow(WC_TABCONTROL,"",
                          WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                          10,10,100,100,hwnd,NULL,hInst,NULL);
         TabPos=0;
         TabList=NULL;
         SelStats=NULL;
         SetWindowPos(hwnd,HWND_TOP,0,0,500,320,SWP_NOZORDER|SWP_NOMOVE);
         return TRUE;
      case WM_COMMAND:
         if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam)==ID_CANCEL) {
            DestroyWindow(hwnd); return TRUE;
         }
         break;
      case WM_SIZE:
         GetClientRect(hwnd,&rect);
         MoveWindow(GetDlgItem(hwnd,ID_CANCEL),(rect.right-60)/2,rect.bottom-35,
                    60,30,TRUE);
         SetRect(&StatRect,5,5,rect.right-10,rect.bottom-45);
         MoveWindow(tab,StatRect.left,StatRect.top,StatRect.right,
                    StatRect.bottom,TRUE);
         TabCtrl_AdjustRect(tab,FALSE,&StatRect);
         OffsetRect(&StatRect,5,5);
         for (list=TabList;list;list=g_slist_next(list)) {
            Stats=(struct STATS *)list->data;
            SizeStats(hwnd,Stats,&StatRect);
         }
         break;
      case WM_NOTIFY:
         nmhdr = (NMHDR *)lParam;
         list=g_slist_nth(TabList,TabCtrl_GetCurSel(tab));
         if (nmhdr && list && nmhdr->code==TCN_SELCHANGE) {
            if (SelStats) ShowStats(SelStats,SW_HIDE);
            SelStats=(struct STATS *)list->data;
            ShowStats(SelStats,SW_SHOW);
         }
         break;
      case WM_ADDSPYREPORT:
         Play=(Player *)lParam;
         if (Play && tab) {
            tie.mask = TCIF_TEXT | TCIF_IMAGE;
            tie.iImage = -1;
            tie.pszText = GetPlayerName(Play);
            Stats=g_new(struct STATS,1);
            TabList=g_slist_append(TabList,(gpointer)Stats);
            CreateStats(hwnd,Stats,FALSE,FALSE);
            DisplayStats(Play,Stats);
            UpdateInventory(NULL,Stats->HereList,NULL,NULL,NULL,
                            Play->Drugs,NumDrug,TRUE);
            UpdateInventory(NULL,Stats->CarriedList,NULL,NULL,NULL,
                            Play->Guns,NumGun,FALSE);
            TabCtrl_InsertItem(tab,TabPos,&tie);
            GetClientRect(hwnd,&rect);
            SetRect(&StatRect,5,5,rect.right-10,rect.bottom-45);
            TabCtrl_AdjustRect(tab,FALSE,&StatRect);
            OffsetRect(&StatRect,5,5);
            if (TabPos!=TabCtrl_GetCurSel(tab)) {
               ShowStats(Stats,SW_HIDE);
            } else {
               if (SelStats) ShowStats(SelStats,SW_HIDE);
               SelStats=Stats; ShowStats(SelStats,SW_SHOW);
            }
            SizeStats(hwnd,Stats,&StatRect);
            TabPos++;
         }
         return TRUE;
      case WM_CLOSE:
         DestroyWindow(hwnd); return TRUE;
      case WM_DESTROY:
         while (TabList) {
            Stats=(struct STATS *)TabList->data;
            TabList=g_slist_remove(TabList,(gpointer)Stats);
            g_free(Stats);
         }
         SpyReportsWnd=NULL;
         break;
   }
   return FALSE;
}

void DisplaySpyReports(Player *Play) {
   if (!SpyReportsWnd) {
      SpyReportsWnd=CreateDialog(hInst,MAKEINTRESOURCE(SpyReportsDia),
                                 ClientData.Window,SpyReportsWndProc);
   }
   ShowWindow(SpyReportsWnd,SW_SHOWNORMAL);
   SendMessage(SpyReportsWnd,WM_ADDSPYREPORT,0,(LPARAM)Play);
}

#endif /* WIN32_CLIENT */

static void ServerLogMessage(const gchar *log_domain,GLogLevelFlags log_level,
                             const gchar *message,gpointer user_data) {
   DWORD NumChar;
   gchar *text;
   text=g_strdup_printf("%s\n",message);
   WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),text,strlen(text),&NumChar,NULL);
   g_free(text);
}

static void Win32PrintFunc(const gchar *string) {
   DWORD NumChar;
   WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),string,strlen(string),
                &NumChar,NULL);
}

int APIENTRY Win32Loop(HINSTANCE hInstance,HINSTANCE hPrevInstance,
                       LPSTR lpszCmdParam,int nCmdShow) {
   static char szAppName[] = "dopewars";
   HWND hwnd;
   MSG msg;
   gchar **split;
   WNDCLASS wc;
   int argc;
   SetupParameters();
   split=g_strsplit(lpszCmdParam," ",0);
   argc=0;
   while (split[argc]) argc++;
   g_set_print_handler(Win32PrintFunc);
   HandleCmdLine(argc,split);
   g_strfreev(split);
   if (WantVersion || WantHelp) {
      AllocConsole();
      HandleHelpTexts();
      newterm(NULL,NULL,NULL);
      bgetch();
   } else {
      StartNetworking();
      if (Server) {
         AllocConsole();
         SetConsoleTitle("dopewars server");
         g_log_set_handler(NULL,G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING,
                           ServerLogMessage,NULL);
         newterm(NULL,NULL,NULL);
         ServerLoop();
      } else if (WantedClient==CLIENT_CURSES) {
         AllocConsole();
         SetConsoleTitle("dopewars");
         CursesLoop();
      } else {
#ifdef WIN32_CLIENT
         hInst=hInstance;
         if (!hPrevInstance) {
           wc.style          = CS_HREDRAW|CS_VREDRAW;
           wc.lpfnWndProc    = MainWndProc;
           wc.cbClsExtra     = 0;
           wc.cbWndExtra     = 0;
           wc.hInstance      = hInstance;
           wc.hIcon          = LoadIcon(NULL,IDI_APPLICATION);
           wc.hCursor        = LoadCursor(NULL,IDC_ARROW);
           wc.hbrBackground  = GetStockObject(LTGRAY_BRUSH);
           wc.lpszMenuName   = MAKEINTRESOURCE(MainMenu);
           wc.lpszClassName  = szAppName;
           RegisterClass(&wc);
         }
         InitCommonControls();
         hwnd=ClientData.Window=CreateWindow(szAppName,"dopewars",
                        WS_OVERLAPPEDWINDOW|CS_HREDRAW|CS_VREDRAW|WS_SIZEBOX,
                        CW_USEDEFAULT,0,460,460,NULL,NULL,hInstance,NULL);
         ShowWindow(hwnd,nCmdShow);
         UpdateWindow(hwnd);
         while(GetMessage(&msg,NULL,0,0)) {
            if ((!PlayerListWnd || !IsDialogMessage(PlayerListWnd,&msg)) &&
                (!TalkWnd || !IsDialogMessage(TalkWnd,&msg)) &&
                (!InventoryWnd || !IsDialogMessage(InventoryWnd,&msg)) &&
                (!FightWnd || !IsDialogMessage(FightWnd,&msg)) &&
                (!GunShopWnd || !IsDialogMessage(GunShopWnd,&msg)) &&
                (!SpyReportsWnd || !IsDialogMessage(SpyReportsWnd,&msg)) &&
                (!ScoresWnd || !IsDialogMessage(ScoresWnd,&msg))) {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
            }
         }
         StopNetworking();
         return msg.wParam;
      }
#else
      g_print("No windowed client available - rebuild the binary passing the\n"
              "--enable-win32-client option to configure, or use the curses\n"
              "client (if available) instead!\n");
#endif
      StopNetworking();
   }
   if (PidFile) g_free(PidFile);
   return 0;
}

#endif /* CYGWIN */
