/************************************************************************
 * AIPlayer.c     Code for dopewars computer players                    *
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

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <glib.h>
#include "dopewars.h"
#include "message.h"
#include "nls.h"
#include "tstring.h"
#include "util.h"
#include "AIPlayer.h"

#if NETWORKING
static int HandleAIMessage(char *Message, Player *AIPlay);
static void PrintAIMessage(char *Text);
static void AIDealDrugs(Player *AIPlay);
static void AIJet(Player *AIPlay);
static void AIGunShop(Player *AIPlay);
static void AIPayLoan(Player *AIPlay);
static void AISendRandomMessage(Player *AIPlay);
static void AISetName(Player *AIPlay);
static void AIHandleQuestion(char *Data, AICode AI, Player *AIPlay,
                             Player *From);

#define MINSAFECASH   300
#define MINSAFEHEALTH 140

/* Reserve some space for picking up new guns */
#define SPACERESERVE  10

/* 
 * Locations of the loan shark, bank, gun shop and pub
 * Note: these are not the same as the global variables
 *       LoanSharkLoc, BankLoc, GunShopLoc and RoughPubLoc,
 *       which are set locally. The remote server could
 *       have different locations set, and the AI must work
 *       out where these locations are for itself.
 */
int RealLoanShark, RealBank, RealGunShop, RealPub;

static void AIConnectFailed(NetworkBuffer *netbuf)
{
  GString *errstr;

  errstr = g_string_new(_("Connection closed by remote host"));
  if (netbuf->error)
    g_string_assign_error(errstr, netbuf->error);
  g_log(NULL, G_LOG_LEVEL_CRITICAL,
        _("Could not connect to dopewars server\n(%s)\n"
          "AI Player terminating abnormally."), errstr->str);
  g_string_free(errstr, TRUE);
}

static void AIStartGame(Player *AIPlay)
{
  Client = Network = TRUE;
  InitAbilities(AIPlay);
  SetAbility(AIPlay, A_DONEFIGHT, FALSE);
  SendAbilities(AIPlay);

  AISetName(AIPlay);
  g_message(_("Connection established\n"));
}

static void DisplayConnectStatus(NetworkBuffer *netbuf, NBStatus oldstatus,
                                 NBSocksStatus oldsocks)
{
  NBStatus status;
  NBSocksStatus sockstat;

  status = netbuf->status;
  sockstat = netbuf->sockstat;
  if (oldstatus == status && oldsocks == sockstat)
    return;

  switch (status) {
  case NBS_PRECONNECT:
    break;
  case NBS_SOCKSCONNECT:
    switch (sockstat) {
    case NBSS_METHODS:
      g_print(_("Connected to SOCKS server %s...\n"), Socks.name);
      break;
    case NBSS_USERPASSWD:
      g_print(_("Authenticating with SOCKS server\n"));
      break;
    case NBSS_CONNECT:
      g_print(_("Asking SOCKS for connect to %s...\n"), ServerName);
      break;
    }
    break;
  case NBS_CONNECTED:
    break;
  }
}

static void NetBufAuth(NetworkBuffer *netbuf, gpointer data)
{
  g_print(_("Using Socks.Auth.User and Socks.Auth.Password "
            "for SOCKS5 authentication\n"));
  SendSocks5UserPasswd(netbuf, Socks.authuser, Socks.authpassword);
}

/* 
 * Main loop for AI players. Connects to server, plays game,
 * and then disconnects.
 */
void AIPlayerLoop()
{
  GString *errstr;
  gchar *msg;
  Player *AIPlay;
  fd_set readfs, writefs;
  gboolean DoneOK, QuitRequest, datawaiting;
  int MaxSock;
  NBStatus oldstatus;
  NBSocksStatus oldsocks;
  NetworkBuffer *netbuf;

  errstr = g_string_new("");
  AIPlay = g_new(Player, 1);

  FirstClient = AddPlayer(0, AIPlay, FirstClient);
  g_message(_("AI Player started; attempting to contact "
              "server at %s:%d..."), ServerName, Port);

  /* Forget where the "special" locations are */
  RealLoanShark = RealBank = RealGunShop = RealPub = -1;

  netbuf = &AIPlay->NetBuf;
  oldstatus = netbuf->status;
  oldsocks = netbuf->sockstat;

  if (!StartNetworkBufferConnect(netbuf, ServerName, Port)) {
    AIConnectFailed(netbuf);
    return;
  } else {
    SetNetworkBufferUserPasswdFunc(netbuf, NetBufAuth, NULL);
    if (netbuf->status == NBS_CONNECTED) {
      AIStartGame(AIPlay);
    } else {
      DisplayConnectStatus(netbuf, oldstatus, oldsocks);
    }
  }

  while (1) {
    FD_ZERO(&readfs);
    FD_ZERO(&writefs);
    MaxSock = 0;

    SetSelectForNetworkBuffer(netbuf, &readfs, &writefs, NULL, &MaxSock);

    oldstatus = netbuf->status;
    oldsocks = netbuf->sockstat;
    if (bselect(MaxSock, &readfs, &writefs, NULL, NULL) == -1) {
      if (errno == EINTR)
        continue;
      printf("Error in select\n");
      exit(1);
    }

    datawaiting =
        RespondToSelect(netbuf, &readfs, &writefs, NULL, &DoneOK);

    if (oldstatus != NBS_CONNECTED &&
        (netbuf->status == NBS_CONNECTED || !DoneOK)) {
      if (DoneOK)
        AIStartGame(AIPlay);
      else {
        AIConnectFailed(netbuf);
        break;
      }
    } else if (netbuf->status != NBS_CONNECTED) {
      DisplayConnectStatus(netbuf, oldstatus, oldsocks);
    }
    if (datawaiting && netbuf->status == NBS_CONNECTED) {
      QuitRequest = FALSE;
      while ((msg = GetWaitingPlayerMessage(AIPlay)) != NULL) {
        if (HandleAIMessage(msg, AIPlay)) {
          QuitRequest = TRUE;
          break;
        }
      }
      if (QuitRequest) {
        g_print(_("AI Player terminated OK.\n"));
        break;
      }
    }
    if (!DoneOK) {
      g_print(_("Connection to server lost!\n"));
      break;
    }
  }
  ShutdownNetwork(AIPlay);
  g_string_free(errstr, TRUE);
  FirstClient = RemovePlayer(AIPlay, FirstClient);
}

/* 
 * Chooses a random name for the AI player, and informs the server
 */
void AISetName(Player *AIPlay)
{
  char *AINames[] = {
    "Chip", "Dopey", "Al", "Dan", "Bob", "Fred", "Bert", "Jim"
  };
  const gint NumNames = sizeof(AINames) / sizeof(AINames[0]);
  gchar *text;

  text = g_strdup_printf("AI) %s", AINames[brandom(0, NumNames)]);
  SetPlayerName(AIPlay, text);
  g_free(text);
  SendNullClientMessage(AIPlay, C_NONE, C_NAME, NULL,
                        GetPlayerName(AIPlay));
  g_print(_("Using name %s\n"), GetPlayerName(AIPlay));
}

/* 
 * Returns TRUE if it would be prudent to run away...
 */
gboolean ShouldRun(Player *AIPlay)
{
  gint TotalHealth;

  if (TotalGunsCarried(AIPlay) == 0)
    return TRUE;

  TotalHealth = AIPlay->Health + AIPlay->Bitches.Carried * 100;
  return (TotalHealth < MINSAFEHEALTH);
}

/* 
 * Decodes the fighting-related message "Msg", and then decides whether
 * to stand or run...
 */
static void HandleCombat(Player *AIPlay, gchar *Msg)
{
  gchar *text;
  gchar *AttackName, *DefendName, *BitchName;
  FightPoint fp;
  int DefendHealth, DefendBitches, BitchesKilled, ArmPercent;
  gboolean CanRunHere, Loot, CanFire;

  if (HaveAbility(AIPlay, A_NEWFIGHT)) {
    ReceiveFightMessage(Msg, &AttackName, &DefendName, &DefendHealth,
                        &DefendBitches, &BitchName, &BitchesKilled,
                        &ArmPercent, &fp, &CanRunHere, &Loot,
                        &CanFire, &text);
  } else {
    text = Msg;
    if (AIPlay->Flags & FIGHTING)
      fp = F_MSG;
    else
      fp = F_LASTLEAVE;
    CanFire = (AIPlay->Flags & CANSHOOT);
    CanRunHere = FALSE;
  }
  PrintAIMessage(text);

  if (ShouldRun(AIPlay)) {
    if (CanRunHere) {
      SendClientMessage(AIPlay, C_NONE, C_FIGHTACT, NULL, "R");
    } else {
      AIDealDrugs(AIPlay);
      AIJet(AIPlay);
    }
  } else if (fp == F_LASTLEAVE) {
    AIJet(AIPlay);
  } else {
    SendClientMessage(AIPlay, C_NONE, C_FIGHTACT, NULL, "F");
  }
}

/* 
 * Performs appropriate processing on an incoming network message
 * "Message" for AI player "AIPlay". Returns 1 if the game should
 * be ended as a result, 0 otherwise.
 */
int HandleAIMessage(char *Message, Player *AIPlay)
{
  char *Data, WasFighting;
  AICode AI;
  MsgCode Code;
  Player *From, *tmp;
  GSList *list;
  struct timeval tv;
  gboolean Handled;

  if (ProcessMessage(Message, AIPlay, &From, &AI, &Code,
                     &Data, FirstClient) == -1) {
    g_warning("Bad network message. Oops.");
    return 0;
  }
  Handled = HandleGenericClientMessage(From, AI, Code, AIPlay, Data, NULL);
  switch (Code) {
  case C_ENDLIST:
    g_print(_("Players in this game:-\n"));
    for (list = FirstClient; list; list = g_slist_next(list)) {
      tmp = (Player *)list->data;
      g_print("    %s\n", GetPlayerName(tmp));
    }
    break;
  case C_NEWNAME:
    AISetName(AIPlay);
    break;
  case C_FIGHTPRINT:
    HandleCombat(AIPlay, Data);
    break;
  case C_PRINTMESSAGE:
    PrintAIMessage(Data);
    break;
  case C_MSG:
    g_print("%s: %s\n", GetPlayerName(From), Data);
    break;
  case C_MSGTO:
    g_print("%s->%s: %s\n", GetPlayerName(From), GetPlayerName(AIPlay),
            Data);
    break;
  case C_JOIN:
    g_print(_("%s joins the game.\n"), Data);
    break;
  case C_LEAVE:
    if (From != &Noone) {
      g_print(_("%s has left the game.\n"), Data);
    }
    break;
  case C_SUBWAYFLASH:
    dpg_print(_("Jetting to %tde with %P cash and %P debt\n"),
              Location[AIPlay->IsAt].Name, AIPlay->Cash,
              AIPlay->Debt);
    /* Use bselect rather than sleep, as this is portable to Win32 */
    tv.tv_sec = AITurnPause;
    tv.tv_usec = 0;
    bselect(0, NULL, NULL, NULL, &tv);
    if (brandom(0, 100) < 10)
      AISendRandomMessage(AIPlay);
    break;
  case C_UPDATE:
    WasFighting = FALSE;
    if (From == &Noone) {
      if (AIPlay->Flags & FIGHTING)
        WasFighting = TRUE;
      ReceivePlayerData(AIPlay, Data, AIPlay);
    } else {
      ReceivePlayerData(AIPlay, Data, From);    /* spy reports */
    }
    if (!(AIPlay->Flags & FIGHTING) && WasFighting) {
      AIDealDrugs(AIPlay);
      AIJet(AIPlay);
    }
    if (AIPlay->Health == 0) {
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
    AIHandleQuestion(Data, AI, AIPlay, From);
    break;
  case C_HISCORE:
  case C_STARTHISCORE:
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
    if (!Handled)
      g_message("%s^%c^%s%s\n", GetPlayerName(From), Code,
                GetPlayerName(AIPlay), Data);
    break;
  }
  return 0;
}

/* 
 * Prints a message received via a printmessage or question
 * network message, stored in "Text".
 */
void PrintAIMessage(char *Text)
{
  unsigned i;
  gboolean SomeText = FALSE;

  for (i = 0; i < strlen(Text); i++) {
    if (Text[i] == '^') {
      if (SomeText)
        putchar('\n');
    } else {
      putchar(Text[i]);
      SomeText = TRUE;
    }
  }
  putchar('\n');
}

/* 
 * Buys and sell drugs for AI player "AIPlay".
 */
void AIDealDrugs(Player *AIPlay)
{
  price_t *Profit, MaxProfit;
  gchar *text;
  int i, LastHighest, Highest, Num, MinProfit;
  Profit = g_new(price_t, NumDrug);

  for (i = 0; i < NumDrug; i++) {
    Profit[i] =
        AIPlay->Drugs[i].Price - (Drug[i].MaxPrice + Drug[i].MinPrice) / 2;
  }
  MinProfit = 0;
  for (i = 0; i < NumDrug; i++)
    if (Profit[i] < MinProfit)
      MinProfit = Profit[i];
  MinProfit--;
  for (i = 0; i < NumDrug; i++)
    if (Profit[i] < 0)
      Profit[i] = MinProfit - Profit[i];
  LastHighest = -1;
  do {
    MaxProfit = MinProfit;
    Highest = -1;
    for (i = 0; i < NumDrug; i++) {
      if (Profit[i] > MaxProfit && i != LastHighest &&
          (LastHighest == -1 || Profit[LastHighest] > Profit[i])) {
        Highest = i;
        MaxProfit = Profit[i];
      }
    }
    LastHighest = Highest;
    if (Highest >= 0) {
      Num = AIPlay->Drugs[Highest].Carried;
      if (MaxProfit > 0 && Num > 0) {
        dpg_print(_("Selling %d %tde at %P\n"), Num, Drug[Highest].Name,
                  AIPlay->Drugs[Highest].Price);
        AIPlay->CoatSize += Num;
        AIPlay->Cash += Num * AIPlay->Drugs[Highest].Price;
        text = g_strdup_printf("drug^%d^%d", Highest, -Num);
        SendClientMessage(AIPlay, C_NONE, C_BUYOBJECT, NULL, text);
        g_free(text);
      }
      if (AIPlay->Drugs[Highest].Price != 0 &&
          AIPlay->CoatSize > SPACERESERVE) {
        Num = AIPlay->Cash / AIPlay->Drugs[Highest].Price;
        if (Num > AIPlay->CoatSize - SPACERESERVE) {
          Num = AIPlay->CoatSize - SPACERESERVE;
        }
        if (MaxProfit < 0 && Num > 0) {
          dpg_print(_("Buying %d %tde at %P\n"), Num, Drug[Highest].Name,
                    AIPlay->Drugs[Highest].Price);
          text = g_strdup_printf("drug^%d^%d", Highest, Num);
          AIPlay->CoatSize -= Num;
          AIPlay->Cash -= Num * AIPlay->Drugs[Highest].Price;
          SendClientMessage(AIPlay, C_NONE, C_BUYOBJECT, NULL, text);
          g_free(text);
        }
      }
    }
  } while (Highest >= 0);
  g_free(Profit);
}

/* 
 * Handles a visit to the gun shop by AI player "AIPlay".
 */
void AIGunShop(Player *AIPlay)
{
  int i;
  int Bought;
  gchar *text;

  do {
    Bought = 0;
    for (i = 0; i < NumGun; i++) {
      if (TotalGunsCarried(AIPlay) < AIPlay->Bitches.Carried + 2 &&
          Gun[i].Space <= AIPlay->CoatSize &&
          Gun[i].Price <= AIPlay->Cash - MINSAFECASH) {
        AIPlay->Cash -= Gun[i].Price;
        AIPlay->CoatSize -= Gun[i].Space;
        AIPlay->Guns[i].Carried++;
        Bought++;
        dpg_print(_("Buying a %tde for %P at the gun shop\n"),
                  Gun[i].Name, Gun[i].Price);
        text = g_strdup_printf("gun^%d^1", i);
        SendClientMessage(AIPlay, C_NONE, C_BUYOBJECT, NULL, text);
        g_free(text);
      }
    }
  } while (Bought);
  SendClientMessage(AIPlay, C_NONE, C_DONE, NULL, NULL);
}

/* 
 * Decides on a new game location for AI player "AIPlay" and jets there.
 */
void AIJet(Player *AIPlay)
{
  int NewLocation;
  char text[40];

  if (!AIPlay)
    return;
  NewLocation = AIPlay->IsAt;
  if (RealLoanShark >= 0
      && AIPlay->Cash > (price_t)((float)AIPlay->Debt * 1.2)) {
    NewLocation = RealLoanShark;
  } else if (RealPub >= 0 && brandom(0, 100) < 30
             && AIPlay->Cash > MINSAFECASH * 10) {
    NewLocation = RealPub;
  } else if (RealGunShop >= 0 && brandom(0, 100) < 70 &&
             TotalGunsCarried(AIPlay) < AIPlay->Bitches.Carried + 2 &&
             AIPlay->Cash > MINSAFECASH * 5) {
    NewLocation = RealGunShop;
  }
  while (NewLocation == AIPlay->IsAt)
    NewLocation = brandom(0, NumLocation);
  sprintf(text, "%d", NewLocation);
  SendClientMessage(AIPlay, C_NONE, C_REQUESTJET, NULL, text);
}

/* 
 * Pays off the loan of AI player "AIPlay" if this doesn't leave
 * the player with insufficient funds for further dealing.
 */
void AIPayLoan(Player *AIPlay)
{
  gchar *prstr;

  if (AIPlay->Cash - AIPlay->Debt >= MINSAFECASH) {
    prstr = pricetostr(AIPlay->Debt);
    SendClientMessage(AIPlay, C_NONE, C_PAYLOAN, NULL, prstr);
    g_free(prstr);
    dpg_print(_("Debt of %P paid off to loan shark\n"), AIPlay->Debt);
  }
  SendClientMessage(AIPlay, C_NONE, C_DONE, NULL, NULL);
}

/* 
 * Sends the answer "answer" from AI player "From" to the server,
 * claiming to be for player "To". Also prints the answer on the screen.
 */
void AISendAnswer(Player *From, Player *To, char *answer)
{
  SendClientMessage(From, C_NONE, C_ANSWER, To, answer);
  puts(answer);
}

/* 
 * Works out a sensible response to the question coded in "Data" and with
 * computer-readable code "AI", claiming to be from "From" and for AI
 * player "AIPlay", and sends it.
 */
void AIHandleQuestion(char *Data, AICode AI, Player *AIPlay, Player *From)
{
  char *Prompt, *allowed;

  if (From == &Noone)
    From = NULL;
  Prompt = Data;
  allowed = GetNextWord(&Prompt, "");
  PrintAIMessage(Prompt);
  switch (AI) {
  case C_ASKLOAN:
    if (RealLoanShark == -1) {
      g_print(_("Loan shark located at %s\n"),
              Location[AIPlay->IsAt].Name);
    }
    RealLoanShark = AIPlay->IsAt;
    AISendAnswer(AIPlay, From, "Y");
    break;
  case C_ASKGUNSHOP:
    if (RealGunShop == -1) {
      g_print(_("Gun shop located at %s\n"),
              Location[AIPlay->IsAt].Name);
    }
    RealGunShop = AIPlay->IsAt;
    AISendAnswer(AIPlay, From, "Y");
    break;
  case C_ASKPUB:
    if (RealPub == -1) {
      g_print(_("Pub located at %s\n"), Location[AIPlay->IsAt].Name);
    }
    RealPub = AIPlay->IsAt;
    AISendAnswer(AIPlay, From, "Y");
    break;
  case C_ASKBITCH:
  case C_ASKRUN:
  case C_ASKGUN:
    AISendAnswer(AIPlay, From, "Y");
    break;
  case C_ASKRUNFIGHT:
    AISendAnswer(AIPlay, From, ShouldRun(AIPlay) ? "R" : "F");
    break;
  case C_ASKBANK:
    if (RealBank == -1) {
      g_print(_("Bank located at %s\n"), Location[AIPlay->IsAt].Name);
    }
    RealBank = AIPlay->IsAt;
    AISendAnswer(AIPlay, From, "N");
    break;
  case C_MEETPLAYER:
    if (TotalGunsCarried(AIPlay) > 0)
      AISendAnswer(AIPlay, From, "A");
    else {
      AISendAnswer(AIPlay, From, "E");
      AIJet(AIPlay);
    }
    break;
  case C_ASKSEW:
    AISendAnswer(AIPlay, From, AIPlay->Health < MINSAFEHEALTH ? "Y" : "N");
    break;
  default:
    AISendAnswer(AIPlay, From, "N");
    break;
  }
}

/* 
 * Sends a random message to all other dopewars players.
 */
void AISendRandomMessage(Player *AIPlay)
{
  char *RandomInsult[5] = {
    /* Random messages to send from the AI player to other players */
    N_("Call yourselves drug dealers?"),
    N_("A trained monkey could do better..."),
    N_("Think you\'re hard enough to deal with the likes of me?"),
    N_("Zzzzz... are you dealing in candy or what?"),
    N_("Reckon I'll just have to shoot you for your own good.")
  };

  SendClientMessage(AIPlay, C_NONE, C_MSG, NULL,
                    _(RandomInsult[brandom(0, 5)]));
}

#else /* NETWORKING */

/* 
 * Whoops - the user asked that we run an AI player, but the binary was
 * built without that compiled in.
 */
void AIPlayerLoop()
{
  g_print(_("This binary has been compiled without networking support, "
            "and thus cannot act as an AI player.\nRecompile passing "
            "--enable-networking to the configure script."));
}

#endif /* NETWORKING */
