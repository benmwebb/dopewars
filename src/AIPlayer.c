/* AIPlayer.c   Code for dopewars computer players                      */
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


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <glib.h>
#include "dopeos.h"
#include "dopewars.h"
#include "message.h"
#include "tstring.h"
#include "AIPlayer.h"

#if NETWORKING
#define NUMNAMES      8
#define MINSAFECASH   300
#define MINSAFEHEALTH 140

/* Reserve some space for picking up new guns */
#define SPACERESERVE  10

/* Locations of the loan shark, bank, gun shop and pub      */
/* Note: these are not the same as the global variables     */
/*       LoanSharkLoc, BankLoc, GunShopLoc and RoughPubLoc, */
/*       which are set locally. The remote server could     */
/*       have different locations set, and the AI must work */
/*       out where these locations are for itself.          */
int RealLoanShark,RealBank,RealGunShop,RealPub;

void AIPlayerLoop() {
/* Main loop for AI players. Connects to server, plays game, */
/* and then disconnects.                                     */
   gchar *pt;
   Player *AIPlay;
   fd_set readfs,writefs;
   gboolean ReadOK,QuitRequest;

   AIPlay=g_new(Player,1);
   FirstClient=AddPlayer(0,AIPlay,FirstClient);
   g_message(_("AI Player started; attempting to contact server at %s:%d..."),
             ServerName,Port);
   pt=SetupNetwork(FALSE);
   if (pt) {
      g_log(NULL,G_LOG_LEVEL_CRITICAL,
            _("Could not connect to dopewars server\n(%s)\n"
              "AI Player terminating abnormally."),_(pt));
      return;
   }
   AIPlay->fd=ClientSock;

   InitAbilities(AIPlay);
   SendAbilities(AIPlay);

   AISetName(AIPlay);
   g_message(_("Connection established\n"));

   /* Forget where the "special" locations are */
   RealLoanShark=RealBank=RealGunShop=RealPub=-1;

   while (1) {
      FD_ZERO(&readfs);
      FD_ZERO(&writefs);
      FD_SET(ClientSock,&readfs);
      if (AIPlay->WriteBuf.DataPresent) FD_SET(ClientSock,&writefs);
      if (bselect(ClientSock+1,&readfs,&writefs,NULL,NULL)==-1) {
         if (errno==EINTR) continue;
         printf("Error in select\n"); exit(1);
      }
      if (FD_ISSET(ClientSock,&writefs)) {
         WriteConnectionBufferToWire(AIPlay);
      }
      if (FD_ISSET(ClientSock,&readfs)) {
         QuitRequest=FALSE;
         ReadOK=ReadConnectionBufferFromWire(AIPlay);

         while ((pt=ReadFromConnectionBuffer(AIPlay))!=NULL) {
            if (HandleAIMessage(pt,AIPlay)) {
               QuitRequest=TRUE;
               break;
            }
         }
         if (QuitRequest) break;

         if (!ReadOK) {
            g_print(_("Connection to server lost!\n"));
            break;
         }
      }
   }
   ShutdownNetwork();
   g_print(_("AI Player terminated OK.\n"));
}

void AISetName(Player *AIPlay) {
/* Chooses a random name for the AI player, and informs the server */
   char *AINames[NUMNAMES] = {
      "Chip", "Dopey", "Al", "Dan", "Bob", "Fred", "Bert", "Jim"
   };
   gchar *text;
   text=g_strdup_printf("AI) %s",AINames[brandom(0,NUMNAMES)]);
   SetPlayerName(AIPlay,text);
   g_free(text);
   SendNullClientMessage(AIPlay,C_NONE,C_NAME,NULL,GetPlayerName(AIPlay));
   g_print(_("Using name %s\n"),GetPlayerName(AIPlay));
}

gboolean ShouldRun(Player *AIPlay) {
/* Returns TRUE if it would be prudent to run away...           */
   gint TotalHealth;

   if (TotalGunsCarried(AIPlay)==0) return TRUE;

   TotalHealth=AIPlay->Health + AIPlay->Bitches.Carried*100;
   return (TotalHealth < MINSAFEHEALTH);
}

static void HandleCombat(Player *AIPlay,gchar *Msg) {
/* Decodes the fighting-related message "Msg", and then decides whether */
/* to stand or run...                                                   */
   gchar *text;
   gchar *AttackName,*DefendName,*BitchName,FightPoint;
   int DefendHealth,DefendBitches,BitchesKilled,ArmPercent;
   gboolean CanRunHere,Loot,CanFire;

   if (HaveAbility(AIPlay,A_NEWFIGHT)) {
      ReceiveFightMessage(Msg,&AttackName,&DefendName,&DefendHealth,
                          &DefendBitches,&BitchName,&BitchesKilled,
                          &ArmPercent,&FightPoint,&CanRunHere,&Loot,
                          &CanFire,&text);
   } else {
      text=Msg;
      if (AIPlay->Flags&FIGHTING) FightPoint=F_MSG;
      else FightPoint=F_LASTLEAVE;
      CanFire = (AIPlay->Flags&CANSHOOT);
      CanRunHere=FALSE;
   }
   PrintAIMessage(text);

   if (ShouldRun(AIPlay)) {
      if (CanRunHere) {
         SendClientMessage(AIPlay,C_NONE,C_FIGHTACT,NULL,"R");
      } else {
         AIDealDrugs(AIPlay);
         AIJet(AIPlay);
      }
   } else if (FightPoint==F_LASTLEAVE) {
      AIJet(AIPlay);
   } else {
      SendClientMessage(AIPlay,C_NONE,C_FIGHTACT,NULL,"F");
   }
}

int HandleAIMessage(char *Message,Player *AIPlay) {
/* Performs appropriate processing on an incoming network message */
/* "Message" for AI player "AIPlay". Returns 1 if the game should */
/* be ended as a result, 0 otherwise.                             */
   char *Data,Code,AICode,WasFighting;
   Player *From,*tmp;
   GSList *list;
   struct timeval tv;
   gboolean Handled;
   if (ProcessMessage(Message,AIPlay,&From,&AICode,&Code,
                      &Data,FirstClient)==-1) {
      g_warning("Bad network message. Oops."); return 0;
   }
   Handled=HandleGenericClientMessage(From,AICode,Code,AIPlay,Data,NULL);
   switch(Code) {
      case C_ENDLIST:
         g_print(_("Players in this game:-\n"));
         for (list=FirstClient;list;list=g_slist_next(list)) {
            tmp=(Player *)list->data;
            g_print("    %s\n",GetPlayerName(tmp));
         }
         break;
      case C_NEWNAME:
         AISetName(AIPlay);
         break;
      case C_FIGHTPRINT:
         HandleCombat(AIPlay,Data);
         break;
      case C_PRINTMESSAGE:
         PrintAIMessage(Data);
         break;
      case C_MSG:
         g_print("%s: %s\n",GetPlayerName(From),Data);
         break;
      case C_MSGTO:
         g_print("%s->%s: %s\n",GetPlayerName(From),GetPlayerName(AIPlay),Data);
         break;
      case C_JOIN:
         g_print(_("%s joins the game.\n"),Data); break;
      case C_LEAVE:
         if (From!=&Noone) {
            g_print(_("%s has left the game.\n"),Data);
         }
         break;
      case C_SUBWAYFLASH:
         /* Use bselect rather than sleep, as this is portable to Win32 */
         tv.tv_sec=AITurnPause;
         tv.tv_usec=0;
         bselect(0,NULL,NULL,NULL,&tv);
         dpg_print(_("Jetting to %tde with %P cash and %P debt\n"),
                Location[(int)AIPlay->IsAt].Name,AIPlay->Cash,AIPlay->Debt);
         if (brandom(0,100)<10) AISendRandomMessage(AIPlay);
         break;
      case C_UPDATE:
         WasFighting=FALSE;
         if (From==&Noone) {
            if (AIPlay->Flags & FIGHTING) WasFighting=TRUE;
            ReceivePlayerData(AIPlay,Data,AIPlay);
         } else {
            ReceivePlayerData(AIPlay,Data,From); /* spy reports */
         }
         if (!(AIPlay->Flags & FIGHTING) && WasFighting) {
            AIDealDrugs(AIPlay);
            AIJet(AIPlay);
         }
         if (AIPlay->Health==0) {
            g_print(_("AI Player killed. Terminating normally.\n"));
            return 1;
         }
         break;
      case C_DRUGHERE:
         AIDealDrugs(AIPlay);
         AIJet(AIPlay);
         break;
      case C_GUNSHOP:
         AIGunShop(AIPlay);
         break;
      case C_LOANSHARK:
         AIPayLoan(AIPlay);
         break;
      case C_QUESTION:
         AIHandleQuestion(Data,AICode,AIPlay,From);
         break;
      case C_HISCORE: case C_STARTHISCORE:
         break;
      case C_ENDHISCORE:
         g_print(_("Game time is up. Leaving game.\n"));
         return 1;
      case C_PUSH:
         g_print(_("AI Player pushed from the server.\n"));
         return 1;
      case C_QUIT:
         g_print(_("The server has terminated.\n"));
         return 1;
      default:
         if (!Handled) g_message("%s^%c^%s%s\n",GetPlayerName(From),Code,
                                                GetPlayerName(AIPlay),Data);
         break;
   }
   return 0;
}

void PrintAIMessage(char *Text) {
/* Prints a message received via a printmessage or question */
/* network message, stored in "Text"                        */
   int i;
   char SomeText=0;
   for (i=0;i<strlen(Text);i++) {
      if (Text[i]=='^') {
         if (SomeText) putchar('\n'); 
      } else {
         putchar(Text[i]);
         SomeText=1;
      }
   }
   putchar('\n');
}

void AIDealDrugs(Player *AIPlay) {
/* Buy and sell drugs for AI player "AIPlay" */
   price_t *Profit,MaxProfit;
   gchar *text;
   int i,LastHighest,Highest,Num,MinProfit;
   Profit = g_new(price_t,NumDrug);
   for (i=0;i<NumDrug;i++) {
      Profit[i]=AIPlay->Drugs[i].Price-(Drug[i].MaxPrice+Drug[i].MinPrice)/2;
   }
   MinProfit=0;
   for (i=0;i<NumDrug;i++) if (Profit[i]<MinProfit) MinProfit=Profit[i];
   MinProfit--;
   for (i=0;i<NumDrug;i++) if (Profit[i]<0) Profit[i]=MinProfit-Profit[i];
   LastHighest=-1;
   while (1) {
      MaxProfit=MinProfit;
      Highest=-1;
      for (i=0;i<NumDrug;i++) {
         if (Profit[i]>MaxProfit && i!=LastHighest && 
             (LastHighest==-1 || Profit[LastHighest]>Profit[i])) {
            Highest=i;
            MaxProfit=Profit[i];
         }
      }
      LastHighest=Highest;
      if (Highest==-1) break;
      Num=AIPlay->Drugs[Highest].Carried;
      if (MaxProfit>0 && Num>0) {
         dpg_print(_("Selling %d %tde at %P\n"),Num,Drug[Highest].Name,
                   AIPlay->Drugs[Highest].Price);
         AIPlay->CoatSize+=Num;
         AIPlay->Cash+=Num*AIPlay->Drugs[Highest].Price;
         text=g_strdup_printf("drug^%d^%d",Highest,-Num);
         SendClientMessage(AIPlay,C_NONE,C_BUYOBJECT,NULL,text);
         g_free(text);
      }
      if (AIPlay->Drugs[Highest].Price != 0 &&
          AIPlay->CoatSize>SPACERESERVE) {
         Num=AIPlay->Cash/AIPlay->Drugs[Highest].Price;
         if (Num>AIPlay->CoatSize-SPACERESERVE) {
            Num=AIPlay->CoatSize-SPACERESERVE;
         }
         if (MaxProfit<0 && Num>0) {
            dpg_print(_("Buying %d %tde at %P\n"),Num,Drug[Highest].Name,
                      AIPlay->Drugs[Highest].Price);
            text=g_strdup_printf("drug^%d^%d",Highest,Num);
            AIPlay->CoatSize-=Num;
            AIPlay->Cash-=Num*AIPlay->Drugs[Highest].Price;
            SendClientMessage(AIPlay,C_NONE,C_BUYOBJECT,NULL,text);
            g_free(text);
         }
      }
   }
   g_free(Profit);
}

void AIGunShop(Player *AIPlay) {
/* Handles a visit to the gun shop by AI player "AIPlay" */
   int i;
   int Bought;
   gchar *text;
   do {
      Bought=0;
      for (i=0;i<NumGun;i++) {
         if (TotalGunsCarried(AIPlay)<AIPlay->Bitches.Carried+2 &&
             Gun[i].Space<=AIPlay->CoatSize &&
             Gun[i].Price<=AIPlay->Cash-MINSAFECASH) {
            AIPlay->Cash-=Gun[i].Price;
            AIPlay->CoatSize-=Gun[i].Space;
            AIPlay->Guns[i].Carried++;
            Bought++;
            dpg_print(_("Buying a %tde for %P at the gun shop\n"),
                      Gun[i].Name,Gun[i].Price);
            text=g_strdup_printf("gun^%d^1",i);
            SendClientMessage(AIPlay,C_NONE,C_BUYOBJECT,NULL,text);
            g_free(text);
         }
      }
   } while (Bought);
   SendClientMessage(AIPlay,C_NONE,C_DONE,NULL,NULL);
}

void AIJet(Player *AIPlay) {
/* Decides on a new game location for AI player "AIPlay" and jets there */
   int NewLocation;
   char text[40];
   if (!AIPlay) return;
   NewLocation=AIPlay->IsAt;
   if (RealLoanShark>=0 && AIPlay->Cash > (price_t)((float)AIPlay->Debt*1.2)) {
      NewLocation=RealLoanShark;
   } else if (RealPub>=0 && brandom(0,100)<30 && AIPlay->Cash>MINSAFECASH*10) {
      NewLocation=RealPub;
   } else if (RealGunShop>=0 && brandom(0,100)<70 &&
              TotalGunsCarried(AIPlay)<AIPlay->Bitches.Carried+2 &&
              AIPlay->Cash>MINSAFECASH*5) {
      NewLocation=RealGunShop;
   }
   while (NewLocation==AIPlay->IsAt) NewLocation=brandom(0,NumLocation);
   sprintf(text,"%d",NewLocation);
   SendClientMessage(AIPlay,C_NONE,C_REQUESTJET,NULL,text);
}

void AIPayLoan(Player *AIPlay) {
/* Pays off the loan of AI player "AIPlay" if this doesn't leave */
/* the player with insufficient funds for further dealing        */
   gchar *prstr;
   if (AIPlay->Cash-AIPlay->Debt >= MINSAFECASH) {
      prstr=pricetostr(AIPlay->Debt);
      SendClientMessage(AIPlay,C_NONE,C_PAYLOAN,NULL,prstr);
      g_free(prstr);
      dpg_print(_("Debt of %P paid off to loan shark\n"),AIPlay->Debt);
   }
   SendClientMessage(AIPlay,C_NONE,C_DONE,NULL,NULL);
}

void AISendAnswer(Player *From,Player *To,char *answer) {
/* Sends the answer "answer" from AI player "From" to the server,        */
/* claiming to be for player "To". Also prints the answer on the screen. */
   SendClientMessage(From,C_NONE,C_ANSWER,To,answer); puts(answer);
}

void AIHandleQuestion(char *Data,char AICode,Player *AIPlay,Player *From) {
/* Works out a sensible response to the question coded in "Data" and with */
/* computer-readable code "AICode", claiming to be from "From" and for AI */
/* player "AIPlay", and sends it                                          */
   char *Prompt,*allowed;
   if (From==&Noone) From=NULL;
   Prompt=Data;
   allowed=GetNextWord(&Prompt,"");
   PrintAIMessage(Prompt);
   switch (AICode) {
      case C_ASKLOAN:
         if (RealLoanShark==-1) {
            g_print(_("Loan shark located at %s\n"),
                   Location[(int)AIPlay->IsAt].Name);
         }
         RealLoanShark=AIPlay->IsAt;
         AISendAnswer(AIPlay,From,"Y");
         break;
      case C_ASKGUNSHOP:
         if (RealGunShop==-1) {
            g_print(_("Gun shop located at %s\n"),
                   Location[(int)AIPlay->IsAt].Name);
         }
         RealGunShop=AIPlay->IsAt;
         AISendAnswer(AIPlay,From,"Y");
         break;
      case C_ASKPUB:
         if (RealPub==-1) {
            g_print(_("Pub located at %s\n"),Location[(int)AIPlay->IsAt].Name);
         }
         RealPub=AIPlay->IsAt;
         AISendAnswer(AIPlay,From,"Y");
         break;
      case C_ASKBITCH: case C_ASKRUN: case C_ASKGUN:
         AISendAnswer(AIPlay,From,"Y");
         break;
      case C_ASKRUNFIGHT:
         AISendAnswer(AIPlay,From,ShouldRun(AIPlay) ? "R" : "F");
         break;
      case C_ASKBANK:
         if (RealBank==-1) {
            g_print(_("Bank located at %s\n"),Location[(int)AIPlay->IsAt].Name);
         }
         RealBank=AIPlay->IsAt;
         AISendAnswer(AIPlay,From,"N");
         break;
      case C_MEETPLAYER:
         if (TotalGunsCarried(AIPlay)>0) AISendAnswer(AIPlay,From,"A");
         else {
            AISendAnswer(AIPlay,From,"E");
            AIJet(AIPlay);
         }
         break;
      case C_ASKSEW:
         AISendAnswer(AIPlay,From,AIPlay->Health<MINSAFEHEALTH ? "Y" : "N");
         break;
      default:
         AISendAnswer(AIPlay,From,"N");
         break;
   }
}

void AISendRandomMessage(Player *AIPlay) {
/* Sends a random message to all other dopewars players */
   char *RandomInsult[5]= {
      N_("Call yourselves drug dealers?"),
      N_("A trained monkey could do better..."),
      N_("Think you\'re hard enough to deal with the likes of me?"),
      N_("Zzzzz... are you dealing in candy or what?"),
      N_("Reckon I'll just have to shoot you for your own good.")
   };
   SendClientMessage(AIPlay,C_NONE,C_MSG,NULL,_(RandomInsult[brandom(0,5)]));
}

#else /* NETWORKING */

void AIPlayerLoop() {
   g_print(_("This binary has been compiled without networking support, and "
             "thus cannot act as an AI player.\nRecompile passing "
             "--enable-networking to the configure script."));
}

#endif /* NETWORKING */
