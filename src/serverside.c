/************************************************************************
 * serverside.c   Handles the server side of dopewars                   *
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>          /* For size_t etc. */
#include <sys/stat.h>

#ifdef CYGWIN
#include <windows.h>            /* For datatypes such as BOOL */
#include <winsock.h>            /* For network functions */
#include <process.h>            /* For getpid */
#else
#include <sys/socket.h>         /* For struct sockaddr etc. */
#include <netinet/in.h>         /* For struct sockaddr_in etc. */
#include <sys/un.h>             /* For struct sockaddr_un */
#include <arpa/inet.h>          /* For socklen_t */
#endif /* CYGWIN */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <glib.h>
#include "dopewars.h"
#include "message.h"
#include "network.h"
#include "nls.h"
#include "serverside.h"
#include "tstring.h"
#include "util.h"

#ifdef GUI_SERVER
#include "gtkport/gtkport.h"
#endif

static const price_t MINTRENCHPRICE = 200, MAXTRENCHPRICE = 300;

#define ESCAPE      0
#define DEFECT      1
#define SHOT        2
#define NUMDISCOVER 3
char *Discover[NUMDISCOVER] = {
  /* Things that can "happen" to your spies - look for strings containing
   * "The spy %s!" to see how these strings are used. */
  N_("escaped"), N_("defected"), N_("was shot")
};

/* The two keys that are valid answers to the Attack/Evade question. If
 * you wish to translate them, do so in the same order as they given here.
 * You will also need to translate the answers given by the clients. */
static char *attackquestiontr = N_("AE");

/* If we haven't talked to the metaserver for 3 hours, then remind it that
 * we still exist, so we don't get wiped from the list of active servers */
#define METAUPDATETIME  (10800)

/* Don't report players logging in/out to the metaserver more frequently
 * than once every minute (so as not to overload the metaserver, or slow
 * down our own server). */
#define METAMINTIME (60)

int TerminateRequest, ReregisterRequest, RelogRequest;

int MetaUpdateTimeout;
int MetaMinTimeout;
gboolean WantQuit = FALSE;

#ifdef CYGWIN
static SERVICE_STATUS_HANDLE scHandle;
#endif

/* Do we want to update the player details on the metaserver when the
 * timeout expires? */
gboolean MetaPlayerPending = FALSE;

GSList *FirstServer = NULL;

#ifdef NETWORKING
static GScanner *Scanner;

/* Data waiting to be sent to/read from the metaserver */
HttpConnection *MetaConn = NULL;
#endif

/* Handle to the high score file */
static FILE *ScoreFP = NULL;

/* Pointer to the filename of a pid file (if non-NULL) */
char *PidFile;

static char HelpText[] = {
  /* Help on various general server commands */
  N_("dopewars server version %s commands and settings\n\n"
     "help                       Displays this help screen\n"
     "list                       Lists all players logged on\n"
     "push <player>              Politely asks the named player to leave\n"
     "kill <player>              Abruptly breaks the connection with the "
     "named player\n"
     "msg:<mesg>                 Send message to all players\n"
     "quit                       Gracefully quit, after notifying all players\n"
     "<variable>=<value>         Sets the named variable to the given value\n"
     "<variable>                 Displays the value of the named variable\n"
     "<list>[x].<var>=<value>    Sets the named variable in the given list,\n"
     "                           index x, to the given value\n"
     "<list>[x].<var>            Displays the value of the named list variable\n"
     "\nValid variables are listed below:-\n\n")
};

typedef enum _OfferForce {
  NOFORCE, FORCECOPS, FORCEBITCH
} OfferForce;

int SendSingleHighScore(Player *Play, struct HISCORE *Score,
                        int ind, gboolean Bold);
static int SendCopOffer(Player *To, OfferForce Force);
static int OfferObject(Player *To, gboolean ForceBitch);
static gboolean HighScoreWrite(FILE *fp, struct HISCORE *MultiScore,
                               struct HISCORE *AntiqueScore);

#ifdef GUI_SERVER
static void GuiHandleMeta(gpointer data, gint socket,
                          GdkInputCondition condition);
static void MetaSocketStatus(NetworkBuffer *NetBuf,
                             gboolean Read, gboolean Write,
                             gboolean CallNow);
#endif

#ifdef NETWORKING
static gboolean MetaConnectError(HttpConnection *conn)
{
  GString *errstr;

  if (!IsHttpError(conn))
    return FALSE;
  errstr = g_string_new("");
  g_string_assign_error(errstr, MetaConn->NetBuf.error);
  dopelog(1, _("Failed to connect to metaserver at %s:%u (%s)"),
          MetaServer.Name, MetaServer.Port, errstr->str);
  g_string_free(errstr, TRUE);
  return TRUE;
}

static void ServerHttpAuth(HttpConnection *conn, gboolean proxyauth,
                           gchar *realm, gpointer data)
{
  gchar *user = NULL, *password = NULL;

  if (proxyauth) {
    if (MetaServer.proxyuser[0] && MetaServer.proxypassword[0]) {
      user = MetaServer.proxyuser;
      password = MetaServer.proxypassword;
      dopelog(3,
              _("Using MetaServer.Proxy.User and "
                "MetaServer.Proxy.Password for HTTP proxy authentication"));
    } else {
      dopelog(0, _("Unable to authenticate with HTTP proxy; please "
                   "set MetaServer.Proxy.User and "
                   "MetaServer.Proxy.Password variables"));
    }
  } else {
    if (MetaServer.authuser[0] && MetaServer.authpassword[0]) {
      user = MetaServer.authuser;
      password = MetaServer.authpassword;
      dopelog(3,
              _("Using MetaServer.Auth.User and MetaServer.Auth.Password "
                "for HTTP authentication"));
    } else {
      dopelog(0, _("Unable to authenticate with HTTP server; please set "
                   "MetaServer.Auth.User and "
                   "MetaServer.Auth.Password variables"));
    }
  }
  SetHttpAuthentication(conn, proxyauth, user, password);
}

static void ServerNetBufAuth(NetworkBuffer *netbuf, gpointer data)
{
  dopelog(3, _("Using Socks.Auth.User and Socks.Auth.Password "
               "for SOCKS5 authentication"));
  SendSocks5UserPasswd(netbuf, Socks.authuser, Socks.authpassword);
}
#endif

/* 
 * Sends server details to the metaserver, if specified. If "Up" is
 * TRUE, informs the metaserver that the server is now accepting
 * connections - otherwise tells the metaserver that this server is
 * about to go down. If "SendData" is TRUE, then also sends game
 * data (e.g. scores) to the metaserver. If "RespectTimeout" is TRUE
 * then the update is delayed if a previous update happened too
 * recently. If networking is disabled, this function does nothing.
 */
void RegisterWithMetaServer(gboolean Up, gboolean SendData,
                            gboolean RespectTimeout)
{
#if NETWORKING
  struct HISCORE MultiScore[NUMHISCORE], AntiqueScore[NUMHISCORE];
  GString *headers, *body;
  gchar *prstr;
  gboolean retval;
  int i;

  if (!MetaServer.Active || !NotifyMetaServer || WantQuit)
    return;

  if (MetaMinTimeout > time(NULL) && RespectTimeout) {
    dopelog(3, _("Attempt to connect to metaserver too frequently "
                 "- waiting for next timeout"));
    MetaPlayerPending = TRUE;
    return;
  }

  /* If the previous connect hung for so long that it's still active, then
   * break the connection before we start a new one */
  if (MetaConn)
    CloseHttpConnection(MetaConn);

  headers = g_string_new("");
  body = g_string_new("");

  g_string_assign(body, "output=text&");

  g_string_sprintfa(body, "up=%d&port=%d&version=", Up ? 1 : 0, Port);
  AddURLEnc(body, VERSION);
  g_string_sprintfa(body, "&players=%d&maxplay=%d&comment=",
                    CountPlayers(FirstServer), MaxClients);
  AddURLEnc(body, MetaServer.Comment);

  if (MetaServer.LocalName[0]) {
    g_string_append(body, "&hostname=");
    AddURLEnc(body, MetaServer.LocalName);
  }
  if (MetaServer.Password[0]) {
    g_string_append(body, "&password=");
    AddURLEnc(body, MetaServer.Password);
  }

  if (SendData && HighScoreRead(ScoreFP, MultiScore, AntiqueScore, TRUE)) {
    for (i = 0; i < NUMHISCORE; i++) {
      if (MultiScore[i].Name && MultiScore[i].Name[0]) {
        g_string_sprintfa(body, "&nm[%d]=", i);
        AddURLEnc(body, MultiScore[i].Name);
        g_string_sprintfa(body, "&dt[%d]=", i);
        AddURLEnc(body, MultiScore[i].Time);
        g_string_sprintfa(body, "&st[%d]=%s&sc[%d]=", i,
                          MultiScore[i].Dead ? "dead" : "alive", i);
        AddURLEnc(body, prstr = FormatPrice(MultiScore[i].Money));
        g_free(prstr);
      }
    }
  }

  g_string_sprintf(headers,
                   "Content-Type: application/x-www-form-urlencoded\n"
                   "Content-Length: %d", (int)strlen(body->str));

  retval = OpenHttpConnection(&MetaConn, MetaServer.Name, MetaServer.Port,
                              MetaServer.ProxyName, MetaServer.ProxyPort,
                              UseSocks && MetaServer.UseSocks ? &Socks : NULL,
                              "POST", MetaServer.Path, headers->str,
                              body->str);
  g_string_free(headers, TRUE);
  g_string_free(body, TRUE);

  if (retval) {
    dopelog(2, _("Waiting for metaserver connect to %s:%u..."),
            MetaServer.Name, MetaServer.Port);
  } else {
    MetaConnectError(MetaConn);
    CloseHttpConnection(MetaConn);
    MetaConn = NULL;
    return;
  }
  SetHttpAuthFunc(MetaConn, ServerHttpAuth, NULL);

  if (Socks.authuser && Socks.authuser[0] &&
      Socks.authpassword && Socks.authpassword[0]) {
    SetNetworkBufferUserPasswdFunc(&MetaConn->NetBuf, ServerNetBufAuth,
                                   NULL);
  }
#ifdef GUI_SERVER
  SetNetworkBufferCallBack(&MetaConn->NetBuf, MetaSocketStatus, NULL);
#endif
  MetaPlayerPending = FALSE;

  MetaUpdateTimeout = time(NULL) + METAUPDATETIME;
  MetaMinTimeout = time(NULL) + METAMINTIME;
#endif /* NETWORKING */
}

#ifdef NETWORKING
void HandleServerPlayer(Player *Play)
{
  gchar *buf;
  gboolean MessageRead = FALSE;

  while ((buf = GetWaitingPlayerMessage(Play)) != NULL) {
    MessageRead = TRUE;
    HandleServerMessage(buf, Play);
    g_free(buf);
  }
  /* Reset the idle timeout (if necessary) */
  if (MessageRead && IdleTimeout) {
    Play->IdleTimeout = time(NULL) + (time_t) IdleTimeout;
  }
}
#endif /* NETWORKING */

/* 
 * Sends details (name, ID) about player "Play" to player "To", using
 * message code "Code".
 */
void SendPlayerDetails(Player *Play, Player *To, MsgCode Code)
{
  GString *text;

  text = g_string_new(GetPlayerName(Play));
  if (HaveAbility(To, A_PLAYERID)) {
    g_string_sprintfa(text, "^%d", Play->ID);
  }
  SendServerMessage(NULL, C_NONE, Code, To, text->str);
  g_string_free(text, TRUE);
}

/* 
 * Given a message "buf", from player "Play", performs processing and
 * sends suitable replies.
 */
void HandleServerMessage(gchar *buf, Player *Play)
{
  Player *To, *tmp, *pt;
  GSList *list;
  char *Data;
  AICode AI;
  MsgCode Code;
  gchar *text;
  DopeEntry NewEntry;
  int i;
  price_t money;

  if (ProcessMessage(buf, Play, &To, &AI, &Code, &Data, FirstServer) == -1) {
    g_warning("Bad message");
    return;
  }
  switch (Code) {
  case C_MSGTO:
    if (Network) {
      dopelog(3, "%s->%s: %s", GetPlayerName(Play), GetPlayerName(To),
              Data);
    }
    SendServerMessage(Play, AI, Code, To, Data);
    break;
  case C_ABILITIES:
    ReceiveAbilities(Play, Data);
    break;
  case C_NAME:
    pt = GetPlayerByName(Data, FirstServer);
    if (pt && pt != Play) {
      if (ConnectTimeout) {
        Play->ConnectTimeout = time(NULL) + (time_t) ConnectTimeout;
      }
      SendServerMessage(NULL, C_NONE, C_NEWNAME, Play, NULL);
    } else if (strlen(GetPlayerName(Play)) == 0 && Data[0]) {
      if (CountPlayers(FirstServer) < MaxClients || !Network) {
        SendAbilities(Play);
        CombineAbilities(Play);
        SendInitialData(Play);
        SendMiscData(Play);
        SetPlayerName(Play, Data);
        for (list = FirstServer; list; list = g_slist_next(list)) {
          pt = (Player *)list->data;
          if (pt != Play && IsConnectedPlayer(pt) && !IsCop(pt)) {
            SendPlayerDetails(pt, Play, C_LIST);
          }
        }
        if (ServerMOTD && ServerMOTD[0]) {
          SendPrintMessage(NULL, C_MOTD, Play, ServerMOTD);
        }
        SendServerMessage(NULL, C_NONE, C_ENDLIST, Play, NULL);
        RegisterWithMetaServer(TRUE, FALSE, TRUE);
        Play->ConnectTimeout = 0;

        if (Network) {
          dopelog(2, _("%s joins the game!"), GetPlayerName(Play));
        }
        for (list = FirstServer; list; list = g_slist_next(list)) {
          pt = (Player *)list->data;
          if (IsConnectedPlayer(pt) && pt != Play) {
            SendPlayerDetails(Play, pt, C_JOIN);
          }
        }
        Play->EventNum = E_ARRIVE;
        SendPlayerData(Play);
        SendEvent(Play);
      } else {
        /* Message displayed in the server when too many players try to
         * connect */
        dopelog(2, _("MaxClients (%d) exceeded - dropping connection"),
                MaxClients);
        if (MaxClients == 1) {
          text = g_strdup_printf(
                                  /* Message sent to a player if the
                                   * server is full */
                                  _("Sorry, but this server has a limit of "
                                   "1 player, which has been reached.^"
                                   "Please try connecting again later."));
        } else {
          text = g_strdup_printf(
                                  /* Message sent to a player if the
                                   * server is full */
                                  _("Sorry, but this server has a limit of "
                                   "%d players, which has been reached.^"
                                   "Please try connecting again later."),
                                  MaxClients);
        }
        SendServerMessage(NULL, C_NONE, C_PRINTMESSAGE, Play, text);
        g_free(text);
        /* Make sure they do actually disconnect, eventually! */
        if (ConnectTimeout) {
          Play->ConnectTimeout = time(NULL) + (time_t) ConnectTimeout;
        }
      }
    } else {
      /* A player changed their name during the game (unusual, and not
       * really properly supported anyway) - notify all players of the
       * change */
      dopelog(2, _("%s will now be known as %s"), GetPlayerName(Play),
              Data);
      BroadcastToClients(C_NONE, C_RENAME, Data, Play, Play);
      SetPlayerName(Play, Data);
    }
    break;
  case C_WANTQUIT:
    if (Play->EventNum != E_FINISH) {
      FinishGame(Play, NULL);
    }
    break;
  case C_REQUESTJET:
    i = atoi(Data);
    if (Play->EventNum == E_FIGHT || Play->EventNum == E_FIGHTASK) {
      if (CanRunHere(Play)) {
        break;
      } else {
        RunFromCombat(Play, i);
      }
      if (Play->EventNum == E_WAITDONE) {
        Play->EventNum = Play->ResyncNum;
        SendEvent(Play);
      }
    }
    if (NumTurns > 0 && Play->Turn >= NumTurns
        && Play->EventNum != E_FINISH) {
      /* Message displayed when a player reaches their maximum number of
       * turns */
      FinishGame(Play, _("Your dealing time is up..."));
    } else if (i != Play->IsAt && (NumTurns == 0 || Play->Turn < NumTurns)
               && Play->EventNum == E_NONE && Play->Health > 0) {
      dopelog(4, "%s jets to %s", GetPlayerName(Play), Location[i].Name);
      Play->IsAt = i;
      Play->Turn++;
      Play->Debt = (price_t)((float)Play->Debt * 1.1);
      Play->Bank = (price_t)((float)Play->Bank * 1.05);
      SendPlayerData(Play);
      Play->EventNum = E_SUBWAY;
      SendEvent(Play);
    } else {
      /* A player has tried to jet to a new location, but we don't allow
       * them to. (e.g. they're still fighting someone, or they're
       * supposed to be dead) */
      dopelog(3, _("%s: DENIED jet to %s"), GetPlayerName(Play),
              Location[i].Name);
    }
    break;
  case C_REQUESTSCORE:
    SendHighScores(Play, FALSE, NULL);
    break;
  case C_CONTACTSPY:
    for (list = FirstServer; list; list = g_slist_next(list)) {
      tmp = (Player *)list->data;
      i = GetListEntry(&(tmp->SpyList), Play);
      if (tmp != Play && i >= 0 && tmp->SpyList.Data[i].Turns >= 0) {
        SendSpyReport(Play, tmp);
      }
    }
    break;
  case C_DEPOSIT:
    money = strtoprice(Data);
    if (Play->Bank + money >= 0 && Play->Cash - money >= 0) {
      Play->Bank += money;
      Play->Cash -= money;
      SendPlayerData(Play);
    }
    break;
  case C_PAYLOAN:
    money = strtoprice(Data);
    if (Play->Debt - money >= 0 && Play->Cash - money >= 0) {
      Play->Debt -= money;
      Play->Cash -= money;
      SendPlayerData(Play);
    }
    break;
  case C_BUYOBJECT:
    BuyObject(Play, Data);
    break;
  case C_FIGHTACT:
    if (Data[0] == 'R')
      RunFromCombat(Play, -1);
    else
      Fire(Play);
    break;
  case C_ANSWER:
    HandleAnswer(Play, To, Data);
    break;
  case C_DONE:
    if (Play->EventNum == E_WAITDONE) {
      Play->EventNum = Play->ResyncNum;
      SendEvent(Play);
    } else if (Play->EventNum != E_NONE && Play->EventNum < E_OUTOFSYNC) {
      Play->EventNum++;
      SendEvent(Play);
    }
    break;
  case C_SPYON:
    if (Play->Cash >= Prices.Spy) {
      dopelog(3, _("%s now spying on %s"), GetPlayerName(Play),
              GetPlayerName(To));
      Play->Cash -= Prices.Spy;
      LoseBitch(Play, NULL, NULL);
      NewEntry.Play = Play;
      NewEntry.Turns = -1;
      AddListEntry(&(To->SpyList), &NewEntry);
      SendPlayerData(Play);
    } else {
      g_warning(_("%s spy on %s: DENIED"), GetPlayerName(Play),
                GetPlayerName(To));
    }
    break;
  case C_TIPOFF:
    if (Play->Cash >= Prices.Tipoff) {
      dopelog(3, _("%s tipped off the cops to %s"), GetPlayerName(Play),
              GetPlayerName(To));
      Play->Cash -= Prices.Tipoff;
      LoseBitch(Play, NULL, NULL);
      NewEntry.Play = Play;
      NewEntry.Turns = 0;
      AddListEntry(&(To->TipList), &NewEntry);
      SendPlayerData(Play);
    } else {
      g_warning(_("%s tipoff about %s: DENIED"), GetPlayerName(Play),
                GetPlayerName(To));
    }
    break;
  case C_SACKBITCH:
    if (Play->Bitches.Carried > 0) {
      LoseBitch(Play, NULL, NULL);
      SendPlayerData(Play);
    }
    break;
  case C_MSG:
    if (Network)
      dopelog(3, "%s: %s", GetPlayerName(Play), Data);
    BroadcastToClients(C_NONE, C_MSG, Data, Play, Play);
    break;
  default:
    dopelog(0, _("Unknown message: %s:%c:%s:%s"), GetPlayerName(Play),
            Code, GetPlayerName(To), Data);
    break;
  }
}

/* 
 * Notifies all clients that player "Play" has left the game and
 * cleans up after them if necessary.
 */
void ClientLeftServer(Player *Play)
{
  Player *tmp;
  GSList *list;

  if (!IsConnectedPlayer(Play))
    return;

  if (Play->EventNum == E_FIGHT || Play->EventNum == E_FIGHTASK) {
    WithdrawFromCombat(Play);
  }
  for (list = FirstServer; list; list = g_slist_next(list)) {
    tmp = (Player *)list->data;
    if (tmp != Play) {
      RemoveAllEntries(&(tmp->TipList), Play);
      RemoveAllEntries(&(tmp->SpyList), Play);
    }
  }
  BroadcastToClients(C_NONE, C_LEAVE, GetPlayerName(Play), Play, Play);
}

/* 
 * Closes down the server and frees up associated handles and memory.
 */
void CleanUpServer()
{
  while (FirstServer) {
    FirstServer = RemovePlayer((Player *)FirstServer->data, FirstServer);
  }
#if NETWORKING
  if (Server)
    CloseSocket(ListenSock);
#endif
}

/* 
 * Responds to a SIGUSR1 signal, and requests the main event loop to
 * reregister the server with the dopewars metaserver.
 */
void ReregisterHandle(int sig)
{
  ReregisterRequest = 1;
}

/* 
 * Responds to a SIGHUP signal, and requests the main event loop to
 * close and then reopen the log file (if any).
 */
void RelogHandle(int sig)
{
  RelogRequest = 1;
}

/* 
 * Traps an attempt by the user to send dopewars a SIGTERM or SIGINT
 * (e.g. pressing Ctrl-C) and signals for a "nice" shutdown. Restores
 * the default signal action (to terminate without cleanup) so that
 * the user can still close the program easily if this cleanup code
 * then causes problems or long delays.
 */
void BreakHandle(int sig)
{
  struct sigaction sact;

  TerminateRequest = 1;
  sact.sa_handler = SIG_DFL;
  sact.sa_flags = 0;
  sigaction(SIGTERM, &sact, NULL);
  sigaction(SIGINT, &sact, NULL);
}

/* 
 * Prints the server help screen to the given file pointer.
 */
void PrintHelpTo(FILE *fp)
{
  int i;
  GString *VarName;

  VarName = g_string_new("");
  fprintf(fp, _(HelpText), VERSION);
  for (i = 0; i < NUMGLOB; i++) {
    if (Globals[i].NameStruct[0]) {
      g_string_sprintf(VarName, "%s%s.%s", Globals[i].NameStruct,
                       Globals[i].StructListPt ? "[x]" : "",
                       Globals[i].Name);
    } else {
      g_string_assign(VarName, Globals[i].Name);
    }
    fprintf(fp, "%-26s %s\n", VarName->str, _(Globals[i].Help));
  }
  fprintf(fp, "\n\n");
  g_string_free(VarName, TRUE);
}

static NetworkBuffer *reply_netbuf;
static void ServerReply(const gchar *msg)
{
  int msglen;
  gchar *msgcp;

  if (reply_netbuf) {
    msglen = strlen(msg);
    while (msglen > 0 && msg[msglen - 1] == '\n')
      msglen--;
    if (msglen > 0) {
      msgcp = g_strndup(msg, msglen);
      QueueMessageForSend(reply_netbuf, msgcp);
      g_free(msgcp);
    }
  } else
    g_print(msg);
}

/* 
 * Displays a simple help screen listing the server commands and options.
 */
void ServerHelp(void)
{
  int i;
  GString *VarName;

  VarName = g_string_new("");
  g_print(_(HelpText), VERSION);
  for (i = 0; i < NUMGLOB; i++) {
    if (Globals[i].NameStruct[0]) {
      g_string_sprintf(VarName, "%s%s.%s", Globals[i].NameStruct,
                       Globals[i].StructListPt ? "[x]" : "",
                       Globals[i].Name);
    } else {
      g_string_assign(VarName, Globals[i].Name);
    }
    g_print("%-26s\t%s\n", VarName->str, _(Globals[i].Help));
  }
  g_string_free(VarName, TRUE);
}

#if NETWORKING
/* 
 * Creates a pid file (if "PidFile" is non-NULL) and writes the process
 * ID into it.
 */
void CreatePidFile(void)
{
  FILE *fp;
  char *OpenError;

  if (!PidFile)
    return;
  fp = fopen(PidFile, "w");
  if (fp) {
    dopelog(1, _("Maintaining pid file %s"), PidFile);
    fprintf(fp, "%ld\n", (long)getpid());
    fclose(fp);
    chmod(PidFile, S_IREAD | S_IWRITE);
  } else {
    OpenError = strerror(errno);
    g_warning(_("Cannot create pid file %s: %s"), PidFile, OpenError);
  }
}

/* 
 * Removes the previously-created pid file "PidFile".
 */
void RemovePidFile(void)
{
  if (PidFile)
    unlink(PidFile);
}

static gboolean StartServer(void)
{
  LastError *sockerr = NULL;
  GString *errstr;

#ifndef CYGWIN
  struct sigaction sact;
#endif

  if (!CheckHighScoreFileConfig())
    return FALSE;
  Scanner = g_scanner_new(&ScannerConfig);
  Scanner->msg_handler = ScannerErrorHandler;
  Scanner->input_name = "(stdin)";
  CreatePidFile();

  /* Make the output line-buffered, so that the log file (if used) is
   * updated regularly */
  fflush(stdout);

#ifdef SETVBUF_REVERSED         /* 2nd and 3rd arguments are reversed on
                                 * some systems */
  setvbuf(stdout, _IOLBF, (char *)NULL, 0);
#else
  setvbuf(stdout, (char *)NULL, _IOLBF, 0);
#endif

  Network = TRUE;
  FirstServer = NULL;
  ClientMessageHandlerPt = NULL;
  ListenSock = CreateTCPSocket(&sockerr);
  if (ListenSock == SOCKET_ERROR) {
    errstr = g_string_new("");
    g_string_assign_error(errstr, sockerr);
    g_log(NULL, G_LOG_LEVEL_CRITICAL,
          _("Cannot create server (listening) socket (%s) Aborting."),
          errstr->str);
    g_string_free(errstr, TRUE);
    FreeError(sockerr);
    exit(1);
  }

  /* This doesn't seem to work properly under Win32 */
#ifndef CYGWIN
  SetReuse(ListenSock);
#endif

  SetBlocking(ListenSock, FALSE);

  if (!BindTCPSocket(ListenSock, Port, &sockerr)) {
    errstr = g_string_new("");
    g_string_assign_error(errstr, sockerr);
    g_log(NULL, G_LOG_LEVEL_CRITICAL,
          _("Cannot bind to port %u (%s) Aborting."), Port, errstr->str);
    g_string_free(errstr, TRUE);
    FreeError(sockerr);
    exit(1);
  }

  if (listen(ListenSock, 10) == SOCKET_ERROR) {
    g_log(NULL, G_LOG_LEVEL_CRITICAL,
          _("Cannot listen to network socket. Aborting."));
    exit(1);
  }

  /* Initial startup message for the server */
  dopelog(0, _("dopewars server version %s ready and waiting for "
               "connections on port %d."), VERSION, Port);

  MetaUpdateTimeout = MetaMinTimeout = 0;

  TerminateRequest = ReregisterRequest = RelogRequest = 0;

#if !CYGWIN
  sact.sa_handler = ReregisterHandle;
  sact.sa_flags = 0;
  sigemptyset(&sact.sa_mask);
  if (sigaction(SIGUSR1, &sact, NULL) == -1) {
    /* Warning messages displayed if we fail to trap various signals */
    g_warning(_("Cannot install SIGUSR1 interrupt handler!"));
  }
  sact.sa_handler = RelogHandle;
  sact.sa_flags = 0;
  sigemptyset(&sact.sa_mask);
  if (sigaction(SIGHUP, &sact, NULL) == -1) {
    g_warning(_("Cannot install SIGHUP interrupt handler!"));
  }
  sact.sa_handler = BreakHandle;
  sact.sa_flags = 0;
  sigemptyset(&sact.sa_mask);
  if (sigaction(SIGINT, &sact, NULL) == -1) {
    g_warning(_("Cannot install SIGINT interrupt handler!"));
  }
  if (sigaction(SIGTERM, &sact, NULL) == -1) {
    g_warning(_("Cannot install SIGTERM interrupt handler!"));
  }
  sact.sa_handler = SIG_IGN;
  sact.sa_flags = 0;
  if (sigaction(SIGPIPE, &sact, NULL) == -1) {
    g_warning(_("Cannot install pipe handler!"));
  }
#endif

  RegisterWithMetaServer(TRUE, TRUE, FALSE);
  return TRUE;
}

/* 
 * Begin the process of shutting down the server. In order to do this,
 * we need to log out all of the currently connected players, and tell
 * the metaserver that we're shutting down. We only shut down properly
 * once all of these messages have been completely sent and
 * acknowledged. (Of course, this can be overridden by a SIGINT or
 * similar in the case of unresponsive players.)
 */
void RequestServerShutdown(void)
{
  RegisterWithMetaServer(FALSE, FALSE, FALSE);
  BroadcastToClients(C_NONE, C_QUIT, NULL, NULL, NULL);
  WantQuit = TRUE;
}

/* 
 * Returns TRUE if the actions initiated by RequestServerShutdown()
 * have been successfully completed, such that we can shut down the
 * server properly now.
 */
gboolean IsServerShutdown(void)
{
  return (WantQuit && !FirstServer && !MetaConn);
}

static GPrintFunc StartServerReply(NetworkBuffer *netbuf)
{
  reply_netbuf = netbuf;
  if (netbuf)
    return g_set_print_handler(ServerReply);
  else
    return NULL;
}

static void FinishServerReply(GPrintFunc oldprint)
{
  if (oldprint)
    g_set_print_handler(oldprint);
}

static void HandleServerCommand(char *string, NetworkBuffer *netbuf)
{
  GSList *list;
  Player *tmp;
  GPrintFunc oldprint;

  oldprint = StartServerReply(netbuf);

  g_scanner_input_text(Scanner, string, strlen(string));
  if (!ParseNextConfig(Scanner, TRUE)) {
    if (g_strcasecmp(string, "help") == 0 || g_strcasecmp(string, "h") == 0
        || strcmp(string, "?") == 0) {
      ServerHelp();
    } else if (g_strcasecmp(string, "quit") == 0) {
      RequestServerShutdown();
    } else if (g_strncasecmp(string, "msg:", 4) == 0) {
      BroadcastToClients(C_NONE, C_MSG, string + 4, NULL, NULL);
    } else if (g_strcasecmp(string, "list") == 0) {
      if (FirstServer) {
        g_print(_("Users currently logged on:-\n"));
        for (list = FirstServer; list; list = g_slist_next(list)) {
          tmp = (Player *)list->data;
          if (!IsCop(tmp)) {
            g_print("%s\n", GetPlayerName(tmp));
          }
        }
      } else
        g_print(_("No users currently logged on!\n"));
    } else if (g_strncasecmp(string, "push ", 5) == 0) {
      tmp = GetPlayerByName(string + 5, FirstServer);
      if (tmp) {
        g_print(_("Pushing %s\n"), GetPlayerName(tmp));
        SendServerMessage(NULL, C_NONE, C_PUSH, tmp, NULL);
      } else
        g_print(_("No such user!\n"));
    } else if (g_strncasecmp(string, "kill ", 5) == 0) {
      tmp = GetPlayerByName(string + 5, FirstServer);
      if (tmp) {
        g_print(_("%s killed\n"), GetPlayerName(tmp));
        BroadcastToClients(C_NONE, C_KILL, GetPlayerName(tmp), tmp,
                           (Player *)FirstServer->data);
        FirstServer = RemovePlayer(tmp, FirstServer);
      } else
        g_print(_("No such user!\n"));
    } else {
      g_print(_("Unknown command - try \"help\" for help...\n"));
    }
  }
  FinishServerReply(oldprint);
}

Player *HandleNewConnection(void)
{
  int cadsize;
  int ClientSock;
  struct sockaddr_in ClientAddr;
  Player *tmp;
  cadsize = sizeof(struct sockaddr);
  if ((ClientSock = accept(ListenSock, (struct sockaddr *)&ClientAddr,
                           &cadsize)) == -1) {
    perror("accept socket");
    exit(1);
  }
  dopelog(2, _("got connection from %s"), inet_ntoa(ClientAddr.sin_addr));
  tmp = g_new(Player, 1);

  FirstServer = AddPlayer(ClientSock, tmp, FirstServer);
  if (ConnectTimeout) {
    tmp->ConnectTimeout = time(NULL) + (time_t) ConnectTimeout;
  }
  return tmp;
}

void StopServer()
{
  dopelog(0, _("dopewars server terminating."));
  g_scanner_destroy(Scanner);
  CleanUpServer();
  RemovePidFile();
}

void RemovePlayerFromServer(Player *Play)
{
  if (!WantQuit && strlen(GetPlayerName(Play)) > 0) {
    dopelog(2, _("%s leaves the server!"), GetPlayerName(Play));
    ClientLeftServer(Play);
    /* Blank the name, so that CountPlayers ignores this player */
    SetPlayerName(Play, NULL);
    /* Report the new high scores (if any) and the new number of players
     * to the metaserver */
    RegisterWithMetaServer(TRUE, TRUE, TRUE);
  }
  FirstServer = RemovePlayer(Play, FirstServer);
}

#ifndef CYGWIN
static gchar sockpref[] = "/tmp/.dopewars";

static gchar *GetLocalSockDir(void)
{
  return g_strdup_printf("%s-%u", sockpref, Port);
}

gchar *GetLocalSocket(void)
{
  return g_strdup_printf("%s-%u/socket", sockpref, Port);
}

static void CloseLocalSocket(int localsock)
{
  gchar *sockname, *sockdir;

  if (localsock >= 0)
    close(localsock);

  sockname = GetLocalSocket();
  sockdir = GetLocalSockDir();
  unlink(sockname);
  rmdir(sockdir);
  g_free(sockname);
  g_free(sockdir);
}

static int SetupLocalSocket(void)
{
  int sock;
  struct sockaddr_un addr;
  gchar *sockname, *sockdir;

  CloseLocalSocket(-1);

  sock = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sock == -1)
    return -1;

  SetBlocking(sock, FALSE);

  sockname = GetLocalSocket();
  sockdir = GetLocalSockDir();
  if (mkdir(sockdir, S_IRUSR | S_IWUSR | S_IXUSR) == -1)
    return -1;

  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sockname, sizeof(addr.sun_path));
  addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

  if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    return -1;

  chmod(sockname, S_IRUSR | S_IWUSR);
  g_free(sockname);
  g_free(sockdir);

  listen(sock, 10);

  return sock;
}
#endif

/* 
 * Initialises server, processes network and interactive messages, and
 * finally cleans up the server on exit.
 */
void ServerLoop()
{
  Player *tmp;
  GSList *list, *nextlist;
  fd_set readfs, writefs, errorfs;
  int topsock;
  struct timeval timeout;
  int MinTimeout;
  GString *LineBuf;
  gboolean DoneOK;
  gchar *buf;

#ifndef CYGWIN
  int localsock;
  GPrintFunc oldprint;
  GSList *localconn = NULL;
#endif

  if (!StartServer())
    return;

#ifdef HAVE_FORK
  /* Daemonize; continue if the fork was successful and we are the child,
   * or if the fork failed */
  if (Daemonize && fork() > 0)
    return;
#endif

#ifndef CYGWIN
  localsock = SetupLocalSocket();
  if (localsock == -1) {
    dopelog(0, _("Could not set up Unix domain socket for admin "
                 "connections - check permissions on /tmp!"));
  }
#endif

  LineBuf = g_string_new("");
  while (1) {
    FD_ZERO(&readfs);
    FD_ZERO(&writefs);
    FD_ZERO(&errorfs);
    FD_SET(ListenSock, &readfs);
    FD_SET(ListenSock, &errorfs);
    topsock = ListenSock + 1;
#ifndef CYGWIN
    if (localsock >= 0) {
      FD_SET(localsock, &readfs);
      topsock = MAX(topsock, localsock + 1);
    }
    for (list = localconn; list; list = g_slist_next(list)) {
      NetworkBuffer *netbuf;

      netbuf = (NetworkBuffer *)list->data;
      SetSelectForNetworkBuffer(netbuf, &readfs, &writefs, &errorfs,
                                &topsock);
    }
#endif
    if (MetaConn) {
      SetSelectForNetworkBuffer(&MetaConn->NetBuf, &readfs, &writefs,
                                &errorfs, &topsock);
    }
    for (list = FirstServer; list; list = g_slist_next(list)) {
      tmp = (Player *)list->data;
      if (!IsCop(tmp)) {
        SetSelectForNetworkBuffer(&tmp->NetBuf, &readfs, &writefs,
                                  &errorfs, &topsock);
      }
    }
    MinTimeout = GetMinimumTimeout(FirstServer);
    if (MinTimeout != -1) {
      timeout.tv_sec = MinTimeout;
      timeout.tv_usec = 0;
    }
    if (select(topsock, &readfs, &writefs, &errorfs,
               MinTimeout == -1 ? NULL : &timeout) == -1) {
      if (errno == EINTR) {
        if (ReregisterRequest) {
          ReregisterRequest = 0;
          RegisterWithMetaServer(TRUE, TRUE, FALSE);
          continue;
        } else if (TerminateRequest) {
          TerminateRequest = 0;
          RequestServerShutdown();
          if (IsServerShutdown())
            break;
          else
            continue;
        } else if (RelogRequest) {      /* Re-open log file */
          RelogRequest = 0;
          CloseLog();
          OpenLog();
          continue;
        } else
          continue;
      }
      perror("select");
      break;
    }
    FirstServer = HandleTimeouts(FirstServer);
    if (FD_ISSET(ListenSock, &readfs)) {
      HandleNewConnection();
    }
#ifndef CYGWIN
    if (localsock >= 0 && FD_ISSET(localsock, &readfs)) {
      int newlocal;
      NetworkBuffer *netbuf;

      newlocal = accept(localsock, NULL, NULL);
      netbuf = g_new(NetworkBuffer, 1);

      InitNetworkBuffer(netbuf, '\n', '\r', NULL);
      BindNetworkBufferToSocket(netbuf, newlocal);
      localconn = g_slist_append(localconn, netbuf);
      oldprint = StartServerReply(netbuf);
      g_print(_("dopewars server version %s ready for admin commands; "
                "try \"help\" for help"), VERSION);
      FinishServerReply(oldprint);
      dopelog(1, _("New admin connection"));
    }
    list = localconn;
    while (list) {
      NetworkBuffer *netbuf;

      nextlist = g_slist_next(list);
      netbuf = (NetworkBuffer *)list->data;
      if (netbuf) {
        if (RespondToSelect(netbuf, &readfs, &writefs, &errorfs, &DoneOK)) {
          while ((buf = GetWaitingMessage(netbuf)) != NULL) {
            dopelog(2, _("Admin command: %s"), buf);
            HandleServerCommand(buf, netbuf);
            g_free(buf);
          }
        }
        if (!DoneOK) {
          dopelog(1, _("Admin connection closed"));
          localconn = g_slist_remove(localconn, netbuf);
          ShutdownNetworkBuffer(netbuf);
          g_free(netbuf);
        }
        list = nextlist;
      }
    }
    if (IsServerShutdown())
      break;
#endif
    if (MetaConn) {
      if (RespondToSelect(&MetaConn->NetBuf, &readfs, &writefs,
                          &errorfs, &DoneOK)) {
        while ((buf = ReadHttpResponse(MetaConn, &DoneOK))) {
          gboolean ReadingBody = (MetaConn->Status == HS_READBODY);

          if (buf[0] || !ReadingBody) {
            dopelog(ReadingBody ? 2 : 4, "MetaServer: %s", buf);
          }
          g_free(buf);
        }
      }
      if (!DoneOK && HandleHttpCompletion(MetaConn)) {
        if (!MetaConnectError(MetaConn)) {
          dopelog(4, "MetaServer: (closed)\n");
        }
        CloseHttpConnection(MetaConn);
        MetaConn = NULL;
        if (IsServerShutdown())
          break;
      }
    }
    list = FirstServer;
    while (list) {
      nextlist = g_slist_next(list);
      tmp = (Player *)list->data;
      if (tmp) {
        if (RespondToSelect(&tmp->NetBuf, &readfs, &writefs,
                            &errorfs, &DoneOK)) {
          /* If any complete messages were read, process them */
          HandleServerPlayer(tmp);
        }
        if (!DoneOK) {
          /* The socket has been shut down, or the buffer was filled -
           * remove player */
          RemovePlayerFromServer(tmp);
          if (IsServerShutdown())
            break;
          tmp = NULL;
        }
      }
      list = nextlist;
    }
    if (list && IsServerShutdown())
      break;
  }
#ifndef CYGWIN
  CloseLocalSocket(localsock);
#endif
  StopServer();
  g_string_free(LineBuf, TRUE);
}

#ifdef GUI_SERVER
static GtkWidget *TextOutput;
static gint ListenTag = 0;
static void SocketStatus(NetworkBuffer *NetBuf, gboolean Read,
                         gboolean Write, gboolean CallNow);
static void GuiSetTimeouts(void);
static time_t NextTimeout = 0;
static guint TimeoutTag = 0;

static gint GuiDoTimeouts(gpointer data)
{
  /* Forget the TimeoutTag so that GuiSetTimeouts doesn't delete it -
   * it'll be deleted automatically anyway when we return FALSE */
  TimeoutTag = 0;
  NextTimeout = 0;

  FirstServer = HandleTimeouts(FirstServer);
  GuiSetTimeouts();
  return FALSE;
}

void GuiSetTimeouts(void)
{
  int MinTimeout;
  time_t TimeNow;

  TimeNow = time(NULL);
  MinTimeout = GetMinimumTimeout(FirstServer);
  if (TimeNow + MinTimeout < NextTimeout || NextTimeout < TimeNow) {
    if (TimeoutTag > 0)
      gtk_timeout_remove(TimeoutTag);
    TimeoutTag = 0;
    if (MinTimeout > 0) {
      TimeoutTag = gtk_timeout_add(MinTimeout * 1000, GuiDoTimeouts, NULL);
      NextTimeout = TimeNow + MinTimeout;
    }
  }
}

static void GuiServerPrintFunc(const gchar *string)
{
  TextViewAppend(GTK_TEXT_VIEW(TextOutput), string, NULL, TRUE);
}

static void GuiServerLogMessage(const gchar *log_domain,
                                GLogLevelFlags log_level,
                                const gchar *message, gpointer user_data)
{
  GString *text;

  text = GetLogString(log_level, message);
  if (text) {
    g_string_append(text, "\n");
    GuiServerPrintFunc(text->str);
    g_string_free(text, TRUE);
  }
}

#ifdef CYGWIN
static void ServiceFailure(const gchar *log_domain,
                           GLogLevelFlags log_level, const gchar *message,
                           gpointer user_data)
{
  SERVICE_STATUS status;

  g_print("%s\n", message);
  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  status.dwCurrentState = SERVICE_STOPPED;
  status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
  status.dwWin32ExitCode = ERROR_NETWORK_UNREACHABLE;
  status.dwCheckPoint = 0;
  status.dwWaitHint = 0;
  SetServiceStatus(scHandle, &status);
}
#endif

static void GuiQuitServer()
{
  gtk_main_quit();
  StopServer();
}

static void GuiDoCommand(GtkWidget *widget, gpointer data)
{
  gchar *text;

  text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  gtk_editable_delete_text(GTK_EDITABLE(widget), 0, -1);
  HandleServerCommand(text, NULL);
  g_free(text);
  if (IsServerShutdown())
    GuiQuitServer();
}

void GuiHandleMeta(gpointer data, gint socket, GdkInputCondition condition)
{
  gboolean DoneOK;
  gchar *buf;

  if (!MetaConn)
    return;
  if (NetBufHandleNetwork(&MetaConn->NetBuf, condition & GDK_INPUT_READ,
                          condition & GDK_INPUT_WRITE, &DoneOK)) {
    while ((buf = ReadHttpResponse(MetaConn, &DoneOK))) {
      gboolean ReadingBody = (MetaConn->Status == HS_READBODY);

      if (buf[0] || !ReadingBody) {
        dopelog(ReadingBody ? 2 : 4, "MetaServer: %s", buf);
      }
      g_free(buf);
    }
  }
  if (!DoneOK && HandleHttpCompletion(MetaConn)) {
    if (!MetaConnectError(MetaConn)) {
      dopelog(4, "MetaServer: (closed)\n");
    }
    CloseHttpConnection(MetaConn);
    MetaConn = NULL;
    if (IsServerShutdown())
      GuiQuitServer();
  }
}

static void GuiHandleSocket(gpointer data, gint socket,
                            GdkInputCondition condition)
{
  Player *Play;
  gboolean DoneOK;

  Play = (Player *)data;

  /* Sanity check - is the player still around? */
  if (!g_slist_find(FirstServer, (gpointer)Play))
    return;

  if (PlayerHandleNetwork(Play, condition & GDK_INPUT_READ,
                          condition & GDK_INPUT_WRITE, &DoneOK)) {
    HandleServerPlayer(Play);
    GuiSetTimeouts();           /* We may have set some new timeouts */
  }
  if (!DoneOK) {
    RemovePlayerFromServer(Play);
    if (IsServerShutdown())
      GuiQuitServer();
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
                                     GuiHandleSocket,
                                     NetBuf->CallBackData);
  }
  if (CallNow)
    GuiHandleSocket(NetBuf->CallBackData, NetBuf->fd, 0);
}

void MetaSocketStatus(NetworkBuffer *NetBuf, gboolean Read, gboolean Write,
                      gboolean CallNow)
{
  if (NetBuf->InputTag)
    gdk_input_remove(NetBuf->InputTag);
  NetBuf->InputTag = 0;
  if (Read || Write) {
    NetBuf->InputTag = gdk_input_add(NetBuf->fd,
                                     (Read ? GDK_INPUT_READ : 0) |
                                     (Write ? GDK_INPUT_WRITE : 0),
                                     GuiHandleMeta, NetBuf->CallBackData);
  }
  if (CallNow)
    GuiHandleMeta(NetBuf->CallBackData, NetBuf->fd, 0);
}

static void GuiNewConnect(gpointer data, gint socket,
                          GdkInputCondition condition)
{
  Player *Play;

  if (condition & GDK_INPUT_READ) {
    Play = HandleNewConnection();
    SetNetworkBufferCallBack(&Play->NetBuf, SocketStatus, (gpointer)Play);
  }
}

static gboolean TriedPoliteShutdown = FALSE;

static gint GuiRequestDelete(GtkWidget *widget, GdkEvent * event,
                             gpointer data)
{
  if (TriedPoliteShutdown) {
    GuiQuitServer();
  } else {
    TriedPoliteShutdown = TRUE;
    HandleServerCommand("quit", NULL);
    if (IsServerShutdown())
      GuiQuitServer();
  }
  return TRUE;                  /* Never allow automatic deletion - we
                                 * handle it manually */
}

#ifdef CYGWIN
static HWND mainhwnd = NULL;
static BOOL systray = FALSE;

static BOOL RegisterStatus(DWORD state)
{
  SERVICE_STATUS status;

  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  status.dwCurrentState = state;
  status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
  status.dwWin32ExitCode = NO_ERROR;
  status.dwCheckPoint = 0;
  status.dwWaitHint = 5000;
  return SetServiceStatus(scHandle, &status);
}

static VOID WINAPI ServiceHandler(DWORD control)
{
  DWORD state = SERVICE_RUNNING;

  switch (control) {
  case SERVICE_CONTROL_STOP:
    state = SERVICE_STOP_PENDING;
    break;
  }
  if (!RegisterStatus(state)) {
    dopelog(0, _("Failed to set NT Service status"));
    return;
  }

  if (mainhwnd
      && !PostMessage(mainhwnd, MYWM_SERVICE, 0, (LPARAM) control)) {
    dopelog(0, _("Failed to post service notification message"));
    return;
  }
}

static VOID WINAPI ServiceInit(DWORD argc, LPTSTR * argv)
{
  scHandle = RegisterServiceCtrlHandler("dopewars-server", ServiceHandler);
  if (!scHandle) {
    dopelog(0, _("Failed to register service handler"));
    return;
  }
  if (!RegisterStatus(SERVICE_START_PENDING)) {
    dopelog(0, _("Failed to set NT Service status"));
    return;
  }

  GuiServerLoop(TRUE);

  if (!RegisterStatus(SERVICE_STOPPED)) {
    dopelog(0, _("Failed to set NT Service status"));
    return;
  }
}

void ServiceMain(void)
{
  SERVICE_TABLE_ENTRY services[] = {
    {"dopewars-server", ServiceInit},
    {NULL, NULL}
  };

  if (!StartServiceCtrlDispatcher(services)) {
    dopelog(0, _("Failed to start NT Service"));
  }
}

static LRESULT CALLBACK GuiServerWndProc(HWND hwnd, UINT msg,
                                         WPARAM wparam, LPARAM lparam)
{
  if (hwnd == mainhwnd)
    switch (msg) {
    case MYWM_SERVICE:
      if (lparam == SERVICE_CONTROL_STOP) {
        GuiQuitServer();
      }
      break;
    case MYWM_TASKBAR:
      if ((UINT) lparam == WM_LBUTTONDOWN)
        ShowWindow(mainhwnd, SW_SHOW);
      break;
    case WM_SYSCOMMAND:
      if (wparam == SC_MINIMIZE && systray) {
        ShowWindow(mainhwnd, SW_HIDE);
        return TRUE;
      }
      break;
    }
  return FALSE;
}

static void SetupTaskBarIcon(GtkWidget *widget)
{
  NOTIFYICONDATA nid;

  nid.cbSize = sizeof(NOTIFYICONDATA);
  nid.uID = 1000;
  if (widget && !widget->hWnd)
    return;
  if (!widget && !mainhwnd)
    return;

  nid.hWnd = mainhwnd;
  if (widget && MinToSysTray) {
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = MYWM_TASKBAR;
    nid.hIcon = mainIcon;
    strcpy(nid.szTip, "dopewars server - running");
    systray = Shell_NotifyIcon(NIM_ADD, &nid);
  } else {
    systray = FALSE;
    Shell_NotifyIcon(NIM_DELETE, &nid);
  }
}
#endif /* CYGWIN */

void GuiServerLoop(gboolean is_service)
{
  GtkWidget *window, *text, *hbox, *vbox, *entry, *label;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
                     GTK_SIGNAL_FUNC(GuiRequestDelete), NULL);
  gtk_window_set_default_size(GTK_WINDOW(window), 500, 250);

  /* Title of dopewars server window (if used) */
  gtk_window_set_title(GTK_WINDOW(window), _("dopewars server"));

  gtk_container_set_border_width(GTK_CONTAINER(window), 7);

  vbox = gtk_vbox_new(FALSE, 7);
  TextOutput = text = gtk_scrolled_text_view_new(&hbox);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 4);
  label = gtk_label_new(_("Command:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  entry = gtk_entry_new();
  gtk_signal_connect(GTK_OBJECT(entry), "activate",
                     GTK_SIGNAL_FUNC(GuiDoCommand), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show_all(window);

  if (is_service) {
#ifdef CYGWIN
    g_log_set_handler(NULL, G_LOG_LEVEL_CRITICAL, ServiceFailure, NULL);
#endif
  } else {
    g_set_print_handler(GuiServerPrintFunc);
    g_log_set_handler(NULL,
                      LogMask() | G_LOG_LEVEL_MESSAGE |
                      G_LOG_LEVEL_WARNING, GuiServerLogMessage, NULL);
  }
  if (!StartServer())
    return;

  ListenTag =
      gdk_input_add(ListenSock, GDK_INPUT_READ, GuiNewConnect, NULL);
#ifdef CYGWIN
  mainhwnd = window->hWnd;
  SetupTaskBarIcon(window);
  SetCustomWndProc(GuiServerWndProc);
  if (is_service && !RegisterStatus(SERVICE_RUNNING)) {
    dopelog(0, _("Failed to set NT Service status"));
    return;
  }
#endif
  gtk_main();
#ifdef CYGWIN
  SetupTaskBarIcon(NULL);
#endif
}
#endif /* GUI_SERVER */

#endif /* NETWORKING */

/* 
 * Tells player "Play" that the game is over; display "Message".
 */
void FinishGame(Player *Play, char *Message)
{
  Play->EventNum = E_FINISH;
  ClientLeftServer(Play);
  SendHighScores(Play, TRUE, Message);

  /* Blank the name, so that CountPlayers ignores this player */
  SetPlayerName(Play, NULL);

  /* Make sure they do actually disconnect, eventually! */
  if (ConnectTimeout) {
    Play->ConnectTimeout = time(NULL) + (time_t) ConnectTimeout;
  }
}

/* 
 * Reads a batch of NUMHISCORE high scores into "HiScore" from "fp".
 */
void HighScoreTypeRead(struct HISCORE *HiScore, FILE *fp)
{
  int i;
  char *buf;

  for (i = 0; i < NUMHISCORE; i++) {
    if (read_string(fp, &HiScore[i].Name) == EOF)
      break;
    read_string(fp, &HiScore[i].Time);
    read_string(fp, &buf);
    HiScore[i].Money = strtoprice(buf);
    g_free(buf);
    HiScore[i].Dead = (fgetc(fp) > 0);
  }
}

/* 
 * Writes out a batch of NUMHISCORE high scores from "HiScore" to "fp".
 */
void HighScoreTypeWrite(struct HISCORE *HiScore, FILE *fp)
{
  int i;
  gchar *text;

  for (i = 0; i < NUMHISCORE; i++) {
    if (HiScore[i].Name) {
      fwrite(HiScore[i].Name, strlen(HiScore[i].Name) + 1, 1, fp);
    } else
      fputc(0, fp);
    if (HiScore[i].Time) {
      fwrite(HiScore[i].Time, strlen(HiScore[i].Time) + 1, 1, fp);
    } else
      fputc(0, fp);
    text = pricetostr(HiScore[i].Money);
    fwrite(text, strlen(text) + 1, 1, fp);
    g_free(text);
    fputc(HiScore[i].Dead ? 1 : 0, fp);
  }
}

/* 
 * Closes the high score file opened by OpenHighScoreFile, below.
 */
void CloseHighScoreFile()
{
  if (ScoreFP)
    fclose(ScoreFP);
}

/* 
 * If we're running setuid/setgid, drop down to the privilege level of the
 * user that started the dopewars process.
 */
void DropPrivileges()
{
#ifndef CYGWIN
  /* Ignore the return from setregid; we'll check it ourselves to be sure
   * (this avoids problems when running under fakeroot) */
  setregid(getgid(), getgid());
  if (getgid() != getegid()) {
    perror("setregid");
    exit(1);
  }

  setreuid(getuid(), getuid());
  if (getuid() != geteuid()) {
    perror("setreuid");
    exit(1);
  }
#endif
}

static const gchar SCOREHEADER[] = "DOPEWARS SCORES V.";
static const guint SCOREHDRLEN = sizeof(SCOREHEADER) - 1; /* Don't include \0 */
static const guint SCOREVERSION = 1;

static gboolean HighScoreReadHeader(FILE *fp, gint *ScoreVersion)
{
  gchar *header;

  if (read_string(fp, &header) != EOF) {
    if (header && strlen(header) > SCOREHDRLEN &&
        strncmp(header, SCOREHEADER, SCOREHDRLEN) == 0) {
      if (ScoreVersion)
        *ScoreVersion = atoi(header + SCOREHDRLEN);
      g_free(header);
      return TRUE;
    }
  }
  g_free(header);
  return FALSE;
}

static void HighScoreWriteHeader(FILE *fp)
{
  gchar *header;

  header = g_strdup_printf("%s%d", SCOREHEADER, SCOREVERSION);
  fwrite(header, strlen(header) + 1, 1, fp);
}

/* 
 * Converts an old format high score file to the new format.
 */
void ConvertHighScoreFile(void)
{
  FILE *old, *backup;
  gchar *BackupFile;
  int ch;
  char *OldError = NULL, *BackupError = NULL;
  struct HISCORE MultiScore[NUMHISCORE], AntiqueScore[NUMHISCORE];

  /* The user running dopewars must be allowed to mess with the score file */
  DropPrivileges();

  BackupFile = g_strdup_printf("%s.bak", ConvertFile);

  old = fopen(ConvertFile, "r+");
  if (!old)
    OldError = strerror(errno);

  backup = fopen(BackupFile, "w");
  if (!backup)
    BackupError = strerror(errno);

  if (old && backup) {

    /* First, make sure that the "old" file doesn't have a valid "new"
     * header */
    rewind(old);
    if (HighScoreReadHeader(old, NULL)) {
      g_log(NULL, G_LOG_LEVEL_CRITICAL,
            _("The high score file %s\n"
              "is already in the new format! Aborting."), ConvertFile);
      fclose(old);
      fclose(backup);
    } else {
      /* Make a backup of the old file */
      ftruncate(fileno(backup), 0);
      rewind(backup);
      rewind(old);
      while (1) {
        ch = fgetc(old);
        if (ch == EOF)
          break;
        else
          fputc(ch, backup);
      }
      fclose(backup);

      /* Read in the scores without the header, and then write out with
       * the header */
      if (!HighScoreRead(old, MultiScore, AntiqueScore, FALSE)) {
        g_log(NULL, G_LOG_LEVEL_CRITICAL,
              _("Error reading scores from %s."), ConvertFile);
      } else {
        ftruncate(fileno(old), 0);
        rewind(old);
        if (HighScoreWrite(old, MultiScore, AntiqueScore)) {
          g_message(_("The high score file %s has been converted to the "
                      "new format.\nA backup of the old file has been "
                      "created as %s.\n"), ConvertFile, BackupFile);
        }
      }
      fclose(old);
    }
  } else {
    if (!old) {
      g_log(NULL, G_LOG_LEVEL_CRITICAL,
            _("Cannot open high score file %s: %s."),
            ConvertFile, OldError);
    } else if (!backup) {
      g_log(NULL, G_LOG_LEVEL_CRITICAL,
            _("Cannot create backup (%s) of the\nhigh score file: %s."),
            BackupFile, BackupError);
    }
  }

  g_free(BackupFile);
}

/* State, set by OpenHighScoreFile, and later used by
 * CheckHighScoreFileConfig */
static gboolean NewFile;
static int OpenError;

/* 
 * Opens the high score file for later use, and then drops privileges.
 */
void OpenHighScoreFile(void)
{
  NewFile = FALSE;
  OpenError = 0;

  if (ScoreFP)
    return;                     /* If already opened, then we're done */

  /* Win32 gets upset if we use "a+" so we use this nasty hack instead */
  ScoreFP = fopen(HiScoreFile, "r+");
  if (!ScoreFP) {
    ScoreFP = fopen(HiScoreFile, "w+");
    if (!ScoreFP)
      OpenError = errno;
    NewFile = TRUE;
  }

  DropPrivileges();
}

/* 
 * Checks the high score file opened by OpenHighScoreFile, above. Also warns
 * the user about other problems encountered during startup. Returns
 * TRUE if it's valid; otherwise, returns FALSE.
 */
gboolean CheckHighScoreFileConfig(void)
{

  if (!ScoreFP) {
    g_log(NULL, G_LOG_LEVEL_CRITICAL,
          _("Cannot open high score file %s.\n"
            "(%s.) Either ensure you have permissions to access\n"
            "this file and directory, or specify an alternate high score "
            "file with the\n-f command line option."),
          HiScoreFile, strerror(OpenError));
    return FALSE;
  }

  if (NewFile) {
    HighScoreWriteHeader(ScoreFP);
    fflush(ScoreFP);
  } else if (!HighScoreReadHeader(ScoreFP, NULL)) {
    g_log(NULL, G_LOG_LEVEL_CRITICAL,
          _("%s does not appear to be a valid\n"
            "high score file - please check it. If it is a high score file\n"
            "from an older version of dopewars, then first convert it to the\n"
            "new format by running \"dopewars -C %s\"\n"
            "from the command line."), HiScoreFile, HiScoreFile);
    return FALSE;
  }

  if (ConfigErrors) {
#ifdef CYGWIN
    g_warning(_("Errors were encountered during the reading of the "
                "configuration file.\nAs as result, some settings may not "
                "work as expected. Please consult the\n"
                "file \"dopewars-log.txt\" for further details."));
#else
    g_warning(_("Errors were encountered during the reading of the "
                "configuration\nfile. As a result, some settings may not "
                "work as expected. Please see the\nmessages on standard "
                "output for further details."));
#endif
  }

  return TRUE;
}

/* 
 * Reads all the high scores into MultiScore and AntiqueScore (antique
 * mode scores). If ReadHeader is TRUE, read the high score file header
 * first. Returns TRUE on success, FALSE on failure.
 */
gboolean HighScoreRead(FILE *fp, struct HISCORE *MultiScore,
                       struct HISCORE *AntiqueScore, gboolean ReadHeader)
{
  gint ScoreVersion = 0;
  memset(MultiScore, 0, sizeof(struct HISCORE) * NUMHISCORE);
  memset(AntiqueScore, 0, sizeof(struct HISCORE) * NUMHISCORE);
  if (fp && ReadLock(fp) == 0) {
    rewind(fp);
    if (ReadHeader && !HighScoreReadHeader(fp, &ScoreVersion)) {
      ReleaseLock(fp);
      return FALSE;
    }
    HighScoreTypeRead(AntiqueScore, fp);
    HighScoreTypeRead(MultiScore, fp);
    ReleaseLock(fp);
  } else
    return FALSE;
  return TRUE;
}

/* 
 * Writes out all the high scores from MultiScore and AntiqueScore; returns
 * TRUE on success, FALSE on failure.
 */
gboolean HighScoreWrite(FILE *fp, struct HISCORE *MultiScore,
                        struct HISCORE *AntiqueScore)
{
  if (fp && WriteLock(fp) == 0) {
    ftruncate(fileno(fp), 0);
    rewind(fp);
    HighScoreWriteHeader(fp);
    HighScoreTypeWrite(AntiqueScore, fp);
    HighScoreTypeWrite(MultiScore, fp);
    ReleaseLock(fp);
    fflush(fp);
  } else
    return 0;
  return 1;
}

/* 
 * Adds "Play" to the high score list if necessary, and then sends the
 * scores over the network to "Play".
 * If "EndGame" is TRUE, add the current score if it's high enough and
 * display an explanatory message. "Message" is tacked onto the start
 * if it's non-NULL. The client is then informed that the game's over.
 */
void SendHighScores(Player *Play, gboolean EndGame, char *Message)
{
  struct HISCORE MultiScore[NUMHISCORE], AntiqueScore[NUMHISCORE], Score;
  struct HISCORE *HiScore;
  struct tm *timep;
  time_t tim;
  GString *text;
  int i, j, InList = -1;

  text = g_string_new("");
  if (!HighScoreRead(ScoreFP, MultiScore, AntiqueScore, TRUE)) {
    g_warning(_("Unable to read high score file %s"), HiScoreFile);
  }
  if (Message) {
    g_string_assign(text, Message);
    if (strlen(text->str) > 0)
      g_string_append_c(text, '^');
  }
  if (WantAntique)
    HiScore = AntiqueScore;
  else
    HiScore = MultiScore;
  if (EndGame) {
    Score.Money = Play->Cash + Play->Bank - Play->Debt;
    Score.Name = g_strdup(GetPlayerName(Play));
    Score.Dead = (Play->Health == 0);
    tim = time(NULL);
    timep = gmtime(&tim);
    Score.Time = g_new(char, 80);       /* Yuck! */

    strftime(Score.Time, 80, "%d-%m-%Y", timep);
    Score.Time[79] = '\0';
    for (i = 0; i < NUMHISCORE; i++) {
      if (InList == -1 && (Score.Money > HiScore[i].Money ||
                           !HiScore[i].Time || HiScore[i].Time[0] == 0)) {
        InList = i;
        g_string_append(text,
                        _("Congratulations! You made the high scores!"));
        SendPrintMessage(NULL, C_NONE, Play, text->str);
        g_free(HiScore[NUMHISCORE - 1].Name);
        g_free(HiScore[NUMHISCORE - 1].Time);
        for (j = NUMHISCORE - 1; j > i; j--) {
          memcpy(&HiScore[j], &HiScore[j - 1], sizeof(struct HISCORE));
        }
        memcpy(&HiScore[i], &Score, sizeof(struct HISCORE));
        break;
      }
    }
    if (InList == -1) {
      g_string_append(text,
                      _("You didn't even make the high score table..."));
      SendPrintMessage(NULL, C_NONE, Play, text->str);
    }
  }
  SendServerMessage(NULL, C_NONE, C_STARTHISCORE, Play, NULL);

  j = 0;
  for (i = 0; i < NUMHISCORE; i++) {
    if (SendSingleHighScore(Play, &HiScore[i], j, InList == i))
      j++;
  }
  if (InList == -1 && EndGame)
    SendSingleHighScore(Play, &Score, j, TRUE);
  SendServerMessage(NULL, C_NONE, C_ENDHISCORE, Play,
                    EndGame ? "end" : NULL);
  if (!EndGame)
    SendDrugsHere(Play, FALSE);
  if (EndGame && !HighScoreWrite(ScoreFP, MultiScore, AntiqueScore)) {
    g_warning(_("Unable to write high score file %s"), HiScoreFile);
  }
  for (i = 0; i < NUMHISCORE; i++) {
    g_free(MultiScore[i].Name);
    g_free(MultiScore[i].Time);
    g_free(AntiqueScore[i].Name);
    g_free(AntiqueScore[i].Time);
  }
  g_string_free(text, TRUE);
}

/* 
 * Sends a single high score in "Score" with position "ind" to player
 * "Play". If Bold is TRUE, instructs the client to display the score in
 * bold text.
 */
int SendSingleHighScore(Player *Play, struct HISCORE *Score,
                        int ind, gboolean Bold)
{
  gchar *Data, *prstr;

  if (!Score->Time || Score->Time[0] == 0)
    return 0;
  Data = g_strdup_printf("%d^%c%c%18s  %-14s %-34s %8s%c", ind,
                         Bold ? 'B' : 'N', Bold ? '>' : ' ',
                         prstr = FormatPrice(Score->Money),
                         Score->Time, Score->Name,
                         Score->Dead ? _("(R.I.P.)") : "",
                         Bold ? '<' : ' ');
  SendServerMessage(NULL, C_NONE, C_HISCORE, Play, Data);
  g_free(prstr);
  g_free(Data);
  return 1;
}

/* 
 * In order for the server to keep track of the state of each client, each
 * client's state is identified by its EventNum data member. So, for example,
 * there is a state for fighting the cops, a state for going to the bank, and
 * so on. This function instructs client player "To" to carry out the actions
 * expected of it in its current state. It is the client's responsibility to
 * ensure that it carries out the correct actions to advance itself to the
 * "next" state; if it fails in this duty it will hang!
 */
void SendEvent(Player *To)
{
  price_t Money;
  int i, j;
  gchar *text;
  Player *Play;
  GSList *list;

  if (!To)
    return;
  if (To->EventNum == E_MAX)
    To->EventNum = E_NONE;
  if (To->EventNum == E_NONE || To->EventNum >= E_OUTOFSYNC)
    return;
  Money = To->Cash + To->Bank - To->Debt;

  ClearPrices(To);

  while (To->EventNum < E_MAX) {
    switch (To->EventNum) {
    case E_SUBWAY:
      SendServerMessage(NULL, C_NONE, C_SUBWAYFLASH, To, NULL);
      break;
    case E_OFFOBJECT:
      To->OnBehalfOf = NULL;
      for (i = 0; i < To->TipList.Number; i++) {
        dopelog(3, _("%s: Tipoff from %s"), GetPlayerName(To),
                GetPlayerName(To->TipList.Data[i].Play));
        To->OnBehalfOf = To->TipList.Data[i].Play;
        SendCopOffer(To, FORCECOPS);
        return;
      }
      for (i = 0; i < To->SpyList.Number; i++) {
        if (To->SpyList.Data[i].Turns < 0) {
          dopelog(3, _("%s: Spy offered by %s"), GetPlayerName(To),
                  GetPlayerName(To->SpyList.Data[i].Play));
          To->OnBehalfOf = To->SpyList.Data[i].Play;
          SendCopOffer(To, FORCEBITCH);
          return;
        }
        To->SpyList.Data[i].Turns++;
        if (To->SpyList.Data[i].Turns > 3 &&
            brandom(0, 100) < 10 + To->SpyList.Data[i].Turns) {
          if (TotalGunsCarried(To) > 0)
            j = brandom(0, NUMDISCOVER);
          else
            j = brandom(0, NUMDISCOVER - 1);
          text =
              dpg_strdup_printf(_("One of your %tde was spying for %s."
                                  "^The spy %s!"), Names.Bitches,
                                GetPlayerName(To->SpyList.Data[i].Play),
                                _(Discover[j]));
          if (j != DEFECT)
            LoseBitch(To, NULL, NULL);
          SendPlayerData(To);
          SendPrintMessage(NULL, C_NONE, To, text);
          g_free(text);
          text = g_strdup_printf(_("Your spy working with %s has "
                                   "been discovered!^The spy %s!"),
                                 GetPlayerName(To), _(Discover[j]));
          if (j == ESCAPE)
            GainBitch(To->SpyList.Data[i].Play);
          To->SpyList.Data[i].Play->Flags &= ~SPYINGON;
          SendPlayerData(To->SpyList.Data[i].Play);
          SendPrintMessage(NULL, C_NONE, To->SpyList.Data[i].Play, text);
          g_free(text);
          RemoveListEntry(&(To->SpyList), i);
          i--;
        }
      }
      if (Money > 3000000)
        i = 130;
      else if (Money > 1000000)
        i = 115;
      else
        i = 100;
      if (brandom(0, i) > 75) {
        if (SendCopOffer(To, NOFORCE))
          return;
      }
      break;
    case E_SAYING:
      if (!Sanitized && brandom(0, 100) < 15
          && (NumSubway > 0 || NumPlaying > 0)) {
        int subwaychance = 50;

        if (NumSubway == 0)
          subwaychance = 0;
        if (NumPlaying == 0)
          subwaychance = 100;
        if (brandom(0, 100) < subwaychance) {
          text = g_strdup_printf(_("The lady next to you on the subway "
                                   "said,^ \"%s\"%s"),
                                 SubwaySaying[brandom(0, NumSubway)],
                                 brandom(0, 100) < 30 ?
                                 _("^    (at least, you -think- that's "
                                   "what she said)") : "");
        } else {
          text = g_strdup_printf(_("You hear someone playing %s"),
                                 Playing[brandom(0, NumPlaying)]);
        }
        SendPrintMessage(NULL, C_NONE, To, text);
        g_free(text);
      }
      break;
    case E_LOANSHARK:
      if (To->IsAt + 1 == LoanSharkLoc && To->Debt > 0) {
        text = dpg_strdup_printf(_("YN^Would you like to visit %tde?"),
                                 Names.LoanSharkName);
        SendQuestion(NULL, C_ASKLOAN, To, text);
        g_free(text);
        return;
      }
      break;
    case E_BANK:
      if (To->IsAt + 1 == BankLoc) {
        text = dpg_strdup_printf(_("YN^Would you like to visit %tde?"),
                                 Names.BankName);
        SendQuestion(NULL, C_ASKBANK, To, text);
        g_free(text);
        return;
      }
      break;
    case E_GUNSHOP:
      if (To->IsAt + 1 == GunShopLoc && !Sanitized && NumGun > 0
          && !WantAntique) {
        text = dpg_strdup_printf(_("YN^Would you like to visit %tde?"),
                                 Names.GunShopName);
        SendQuestion(NULL, C_ASKGUNSHOP, To, text);
        g_free(text);
        return;
      }
      break;
    case E_ROUGHPUB:
      if (To->IsAt + 1 == RoughPubLoc && !WantAntique) {
        text = dpg_strdup_printf(_("YN^Would you like to visit %tde?"),
                                 Names.RoughPubName);
        SendQuestion(NULL, C_ASKPUB, To, text);
        g_free(text);
        return;
      }
      break;
    case E_HIREBITCH:
      if (To->IsAt + 1 == RoughPubLoc && !WantAntique) {
        To->Bitches.Price = prandom(Bitch.MinPrice, Bitch.MaxPrice);
        text =
            dpg_strdup_printf(_
                              ("YN^^Would you like to hire a %tde for %P?"),
                              Names.Bitch, To->Bitches.Price);
        SendQuestion(NULL, C_ASKBITCH, To, text);
        g_free(text);
        return;
      }
      break;
    case E_ARRIVE:
      for (list = FirstServer; list; list = g_slist_next(list)) {
        Play = (Player *)list->data;
        if (IsConnectedPlayer(Play) && Play != To
            && NumGun > 0 && Play->IsAt == To->IsAt
            && Play->EventNum == E_NONE && TotalGunsCarried(To) > 0) {
          text = g_strdup_printf(_("%s^%s is already here!^"
                                   "Do you Attack, or Evade?"),
                                 attackquestiontr,
                                 GetPlayerName(Play));
          /* Steal this to keep track of the potential defender */
          To->OnBehalfOf = Play;

          SendDrugsHere(To, TRUE);
          SendQuestion(NULL, C_MEETPLAYER, To, text);
          g_free(text);
          return;
        }
      }
      SendDrugsHere(To, TRUE);
      break;
    default:
      break;
    }
    To->EventNum++;
  }
  if (To->EventNum >= E_MAX)
    To->EventNum = E_NONE;
}

/* 
 * In response to client player "To" being in state E_OFFOBJECT,
 * randomly engages the client in combat with the cops or offers
 * other random events. Returns 0 if the client should then be
 * advanced to the next state, 1 otherwise (i.e. if there are
 * questions pending which the client must answer first)
 * If Force==FORCECOPS, engage in combat with the cops for certain
 * If Force==FORCEBITCH, offer the client a bitch for certain
 */
int SendCopOffer(Player *To, OfferForce Force)
{
  int i;

  /* The cops are more likely to attack in locations with higher police
   * presence ratings */
  i = brandom(0, 80 + Location[To->IsAt].PolicePresence);

  if (Force == FORCECOPS)
    i = 100;
  else if (Force == FORCEBITCH)
    i = 0;
  else
    To->OnBehalfOf = NULL;
  if (i < 33) {
    return (OfferObject(To, Force == FORCEBITCH));
  } else if (i < 50) {
    return (RandomOffer(To));
  } else if (Sanitized || NumCop == 0 || NumGun == 0) {
    return 0;
  } else {
    CopsAttackPlayer(To);
    return 1;
  }
  return 1;
}

/* 
 * Has the cops attack player "Play".
 */
void CopsAttackPlayer(Player *Play)
{
  Player *Cops;
  gint CopIndex, NumDeputy, GunIndex;

  if (NumCop == 0 || NumGun == 0) {
    g_warning(_("No cops or guns!"));
    return;
  }

  CopIndex = 1 - Play->CopIndex;
  if (CopIndex < 0) {
    g_warning(_("Cops cannot attack other cops!"));
    return;
  }
  if (CopIndex > NumCop)
    CopIndex = NumCop;
  Cops = g_new(Player, 1);

  FirstServer = AddPlayer(0, Cops, FirstServer);
  SetPlayerName(Cops, Cop[CopIndex - 1].Name);
  Cops->CopIndex = CopIndex;
  Cops->Cash = brandom(100, 2000);
  Cops->Debt = Cops->Bank = 0;

  NumDeputy = brandom(Cop[CopIndex - 1].MinDeputies,
                      Cop[CopIndex - 1].MaxDeputies);
  Cops->Bitches.Carried = NumDeputy;
  GunIndex = Cop[CopIndex - 1].GunIndex;
  if (GunIndex >= NumGun)
    GunIndex = NumGun - 1;
  Cops->Guns[GunIndex].Carried =
      (NumDeputy * Cop[CopIndex - 1].DeputyGun) + Cop[CopIndex - 1].CopGun;
  Cops->Health = 100;

  Play->EventNum++;
  AttackPlayer(Cops, Play);
}

/* 
 * Starts combat between player "Play" and player "Attacked"; if
 * either player is currently engaged in combat, add the other
 * player to the existing combat. If neither player is currently
 * fighting, start a new combat between them. Either player can be
 * the cops.
 */
void AttackPlayer(Player *Play, Player *Attacked)
{
  GPtrArray *FightArray;

  g_assert(Play && Attacked);

  if (Play->FightArray && Attacked->FightArray) {
    if (Play->FightArray == Attacked->FightArray) {
      g_error(_("Players are already in a fight!"));
    } else {
      g_error(_("Players are already in separate fights!"));
    }
    return;
  }
  if (NumGun == 0) {
    g_error(_("Cannot start fight - no guns to use!"));
    return;
  }

  if (!Play->FightArray && !Attacked->FightArray) {
    FightArray = g_ptr_array_new();
  } else {
    FightArray =
        Play->FightArray ? Play->FightArray : Attacked->FightArray;
  }

  if (!Play->FightArray) {
    Play->ResyncNum = Play->EventNum;
    g_ptr_array_add(FightArray, Play);
  }
  if (!Attacked->FightArray) {
    Attacked->ResyncNum = Attacked->EventNum;
    g_ptr_array_add(FightArray, Attacked);
  }
  Play->FightArray = Attacked->FightArray = FightArray;
  Play->EventNum = Attacked->EventNum = E_FIGHT;

  Play->Attacking = Attacked;

  SendFightMessage(Attacked, Play, 0, F_ARRIVED, (price_t)0, TRUE, NULL);

  Fire(Play);
}

/* 
 * Returns TRUE if player "Other" is not allied with player "Play".
 */
gboolean IsOpponent(Player *Play, Player *Other)
{
  return TRUE;
}

void HandleDamage(Player *Defend, Player *Attack, int Damage,
                  int *BitchesKilled, price_t *Loot)
{
  Inventory *Guns, *Drugs;
  price_t Bounty;

  Guns = (Inventory *)g_malloc0(sizeof(Inventory) * NumGun);
  Drugs = (Inventory *)g_malloc0(sizeof(Inventory) * NumDrug);
  ClearInventory(Guns, Drugs);

  Bounty = 0;
  if (Defend->Health <= Damage && Defend->Bitches.Carried == 0) {
    Bounty = Defend->Cash + Defend->Bank - Defend->Debt;
    AddInventory(Guns, Defend->Guns, NumGun);
    AddInventory(Drugs, Defend->Drugs, NumDrug);
    Defend->Health = 0;
  } else if (Defend->Bitches.Carried > 0 && Defend->Health <= Damage) {
    if (IsCop(Defend))
      LoseBitch(Defend, NULL, NULL);
    else
      LoseBitch(Defend, Guns, Drugs);
    Defend->Health = 100;
    *BitchesKilled = 1;
  } else {
    Defend->Health -= Damage;
  }
  if (IsCop(Attack)) {          /* Don't let cops loot players */
    ClearInventory(Guns, Drugs);
  } else {
    TruncateInventoryFor(Guns, Drugs, Attack);
  }
  SendPlayerData(Defend);
  if (Bounty < 0)
    Bounty = 0;
  if (!IsInventoryClear(Guns, Drugs)) {
    AddInventory(Attack->Guns, Guns, NumGun);
    AddInventory(Attack->Drugs, Drugs, NumDrug);
    ChangeSpaceForInventory(Guns, Drugs, Attack);
  }
  Attack->Cash += Bounty;
  if (Bounty > 0 || !IsInventoryClear(Guns, Drugs)) {
    if (Bounty > 0)
      *Loot = Bounty;
    else
      *Loot = -1;
    SendPlayerData(Attack);
  }
  g_free(Guns);
  g_free(Drugs);
}

void GetFightRatings(Player *Attack, Player *Defend,
                     int *AttackRating, int *DefendRating)
{
  int i;

  /* Base values */
  *AttackRating = 80;
  *DefendRating = 100;

  for (i = 0; i < NumGun; i++) {
    *AttackRating += Gun[i].Damage * Attack->Guns[i].Carried;
  }
  if (IsCop(Attack))
    *AttackRating -= Cop[Attack->CopIndex - 1].AttackPenalty;

  *DefendRating -= 5 * Defend->Bitches.Carried;
  if (IsCop(Defend))
    *DefendRating -= Cop[Defend->CopIndex - 1].DefendPenalty;

  *DefendRating = MAX(*DefendRating, 10);
  *AttackRating = MAX(*AttackRating, 10);
}

void AllowNextShooter(Player *Play)
{
  Player *NextShooter;

  if (FightTimeout) {
    NextShooter = GetNextShooter(Play);
    if (NextShooter && !CanPlayerFire(NextShooter)) {
      NextShooter->FightTimeout = 0;
    }
  }
}

void DoReturnFire(Player *Play)
{
  guint ArrayInd;
  Player *Defend;

  if (!Play || !Play->FightArray)
    return;

  if (FightTimeout != 0 || !IsCop(Play)) {
    for (ArrayInd = 0;
         Play->FightArray && ArrayInd < Play->FightArray->len;
         ArrayInd++) {
      Defend = (Player *)g_ptr_array_index(Play->FightArray, ArrayInd);
      if (IsCop(Defend) && CanPlayerFire(Defend))
        Fire(Defend);
    }
  }
}

/*
 * Puts the given player into the "fight ended" state.
 */
static void WaitForFightDone(Player *Play)
{
  if (HaveAbility(Play, A_DONEFIGHT)) {
    Play->EventNum = E_WAITDONE;
  } else {
    Play->EventNum = Play->ResyncNum;
    SendEvent(Play);
  }
}

/* 
 * Withdraws player "Play" from combat, and levies any penalties on
 * the player for this cowardly act, if applicable. If "ToLocation"
 * is >=0, then it identifies the location that the player is
 * trying to run to.
 */
void RunFromCombat(Player *Play, int ToLocation)
{
  int EscapeProb, RandNum;
  guint ArrayInd;
  gboolean FightingCop = FALSE;
  Player *Defend;
  char BackupAt;

  if (!Play || !Play->FightArray)
    return;

  EscapeProb = 60;

  /* Penalise players that are attacking others */
  if (Play->Attacking)
    EscapeProb /= 2;

  RandNum = brandom(0, 100);

  if (RandNum < EscapeProb) {
    if (!IsCop(Play) && brandom(0, 100) < 30) {
      for (ArrayInd = 0; ArrayInd < Play->FightArray->len; ArrayInd++) {
        Defend = (Player *)g_ptr_array_index(Play->FightArray, ArrayInd);
        if (IsCop(Defend)) {
          FightingCop = TRUE;
          break;
        }
      }
      if (FightingCop)
        Play->CopIndex--;
    }
    BackupAt = Play->IsAt;
    Play->IsAt = ToLocation;
    WithdrawFromCombat(Play);
    Play->IsAt = BackupAt;
    WaitForFightDone(Play);
  } else {
    SendFightMessage(Play, NULL, 0, F_FAILFLEE, (price_t)0, TRUE, NULL);
    AllowNextShooter(Play);
    if (FightTimeout)
      SetFightTimeout(Play);
    DoReturnFire(Play);
  }
}

void CheckForKilledPlayers(Player *Play)
{
  Player *Defend;
  guint ArrayInd;
  GPtrArray *KilledPlayers;

  KilledPlayers = g_ptr_array_new();
  for (ArrayInd = 0; ArrayInd < Play->FightArray->len; ArrayInd++) {
    Defend = (Player *)g_ptr_array_index(Play->FightArray, ArrayInd);

    if (Defend && Defend != Play && IsOpponent(Play, Defend) &&
        Defend->Health == 0) {
      g_ptr_array_add(KilledPlayers, (gpointer)Defend);
    }
  }
  for (ArrayInd = 0; ArrayInd < KilledPlayers->len; ArrayInd++) {
    Defend = (Player *)g_ptr_array_index(KilledPlayers, ArrayInd);
    WithdrawFromCombat(Defend);
    if (IsCop(Defend)) {
      if (!IsCop(Play))
        Play->CopIndex = -Defend->CopIndex;
      FirstServer = RemovePlayer(Defend, FirstServer);
    } else {
      FinishGame(Defend, _("You're dead! Game over."));
    }
  }

  g_ptr_array_free(KilledPlayers, FALSE);
}

/* 
 * If "Play" is attacking someone, and no cops are currently present,
 * then have the cops intervene (with a probability dependent on the
 * current location's PolicePresence)
 */
static void CheckCopsIntervene(Player *Play)
{
  guint ArrayInd;
  Player *Defend;

  if (!Play || !Play->FightArray || NumCop == 0 || NumGun == 0)
    return;                     /* Sanity check */

  if (!Play->Attacking)
    return;                     /* Cops don't attack "innocent victims" ;) */

  if (brandom(0, 100) > Location[Play->IsAt].PolicePresence) {
    return;                     /* The cops shouldn't _always_ attack
                                 * (unless P.P. == 100) */
  }

  for (ArrayInd = 0; Play->FightArray && ArrayInd < Play->FightArray->len;
       ArrayInd++) {
    Defend = (Player *)g_ptr_array_index(Play->FightArray, ArrayInd);
    if (IsCop(Defend))
      return;                   /* We don't want _more_ cops! */
  }

  /* OK - let 'em have it... */
  CopsAttackPlayer(Play);
}

/* 
 * Returns a suitable player (or cop) for "Play" to fire at. If "Play"
 * is attacking a designated target already, return that, otherwise
 * return the first valid opponent in the player's FightArray.
 */
static Player *GetFireTarget(Player *Play)
{
  Player *Defend;
  guint ArrayInd;

  if (Play->Attacking
      && g_slist_find(FirstServer, (gpointer)Play->Attacking)) {
    return Play->Attacking;
  } else {
    Play->Attacking = NULL;
    for (ArrayInd = 0; ArrayInd < Play->FightArray->len; ArrayInd++) {
      Defend = (Player *)g_ptr_array_index(Play->FightArray, ArrayInd);
      if (Defend && Defend != Play && IsOpponent(Play, Defend)) {
        return Defend;
      }
    }
  }
  return NULL;
}

static int GetArmour(Player *Play)
{
  int Armour;

  if (IsCop(Play)) {
    if (Play->Bitches.Carried == 0)
      Armour = Cop[Play->CopIndex - 1].Armour;
    else
      Armour = Cop[Play->CopIndex - 1].DeputyArmour;
  } else {
    if (Play->Bitches.Carried == 0)
      Armour = PlayerArmour;
    else
      Armour = BitchArmour;
  }
  if (Armour == 0)
    Armour = 1;
  return Armour;
}

/* 
 * Fires all weapons of player "Play" at an opponent, and resets
 * the fight timeout (the reload time).
 */
void Fire(Player *Play)
{
  int Damage, i, j;
  int AttackRating, DefendRating;
  int BitchesKilled;
  price_t Loot;
  FightPoint fp;
  Player *Defend;

  if (!Play->FightArray)
    return;
  if (!CanPlayerFire(Play))
    return;

  AllowNextShooter(Play);
  if (FightTimeout)
    SetFightTimeout(Play);

  Defend = GetFireTarget(Play);
  if (Defend) {
    Damage = 0;
    BitchesKilled = 0;
    Loot = 0;
    if (TotalGunsCarried(Play) > 0) {
      GetFightRatings(Play, Defend, &AttackRating, &DefendRating);
      if (brandom(0, AttackRating) > brandom(0, DefendRating)) {
        fp = F_HIT;
        for (i = 0; i < NumGun; i++)
          for (j = 0; j < Play->Guns[i].Carried; j++) {
            Damage += brandom(0, Gun[i].Damage);
          }
        Damage = Damage * 100 / GetArmour(Defend);
        if (Damage == 0)
          Damage = 1;
        HandleDamage(Defend, Play, Damage, &BitchesKilled, &Loot);
      } else
        fp = F_MISS;
    } else
      fp = F_STAND;
    SendFightMessage(Play, Defend, BitchesKilled, fp, Loot, TRUE, NULL);
  }
  CheckForKilledPlayers(Play);

  /* Careful, as we might have killed Player "Play" */
  if (g_slist_find(FirstServer, (gpointer)Play))
    DoReturnFire(Play);

  if (g_slist_find(FirstServer, (gpointer)Play))
    CheckCopsIntervene(Play);
}

gboolean CanPlayerFire(Player *Play)
{
  return (FightTimeout == 0 || Play->FightTimeout == 0 ||
          Play->FightTimeout <= time(NULL));
}

gboolean CanRunHere(Player *Play)
{
  return (Play->ResyncNum < E_ARRIVE && Play->ResyncNum != E_NONE);
}

/* 
 * To avoid boring waits, return the player who is next in line to be
 * able to shoot (i.e. with the smallest FightTimeout) so that this
 * player can be allowed to shoot immediately. If a player is already
 * eligible to shoot, or there is a tie for "first place" then do
 * nothing (i.e. return NULL).
 */
Player *GetNextShooter(Player *Play)
{
  Player *MinPlay, *Defend;
  time_t MinTimeout;
  guint ArrayInd;
  gboolean Tie = FALSE;

  if (!FightTimeout)
    return NULL;

  MinPlay = NULL;
  MinTimeout = 0;
  for (ArrayInd = 0; ArrayInd < Play->FightArray->len; ArrayInd++) {
    Defend = (Player *)g_ptr_array_index(Play->FightArray, ArrayInd);
    if (Defend == Play)
      continue;
    if (Defend->FightTimeout == 0)
      return NULL;
    if (MinTimeout == 0 || Defend->FightTimeout <= MinTimeout) {
      Tie = (Defend->FightTimeout == MinTimeout);
      MinPlay = Defend;
      MinTimeout = Defend->FightTimeout;
    }
  }

  return (Tie ? NULL : MinPlay);
}

void ResolveTipoff(Player *Play)
{
  GString *text;

  if (IsCop(Play) || !CanRunHere(Play))
    return;

  if (g_slist_find(FirstServer, (gpointer)Play->OnBehalfOf)) {
    dopelog(4, _("%s: tipoff by %s finished OK."), GetPlayerName(Play),
            GetPlayerName(Play->OnBehalfOf));
    RemoveListPlayer(&(Play->TipList), Play->OnBehalfOf);
    text = g_string_new("");
    if (Play->Health == 0) {
      g_string_sprintf(text,
                       _("Following your tipoff, the cops ambushed %s, "
                         "who was shot dead!"), GetPlayerName(Play));
    } else {
      dpg_string_sprintf(text,
                         _("Following your tipoff, the cops ambushed %s, "
                           "who escaped with %d %tde. "), GetPlayerName(Play),
                         Play->Bitches.Carried, Names.Bitches);
    }
    GainBitch(Play->OnBehalfOf);
    SendPlayerData(Play->OnBehalfOf);
    SendPrintMessage(NULL, C_NONE, Play->OnBehalfOf, text->str);
    g_string_free(text, TRUE);
  }
  Play->OnBehalfOf = NULL;
}

/* 
 * Cleans up combat after player "Play" has left.
 */
void WithdrawFromCombat(Player *Play)
{
  guint AttackInd, DefendInd;
  gboolean FightDone;
  Player *Attack, *Defend;
  GSList *list;
  gchar *text;

  for (list = FirstServer; list; list = g_slist_next(list)) {
    Attack = (Player *)list->data;
    if (Attack->Attacking == Play)
      Attack->Attacking = NULL;
  }

  if (!Play->FightArray)
    return;

  ResolveTipoff(Play);
  FightDone = TRUE;
  for (AttackInd = 0; AttackInd < Play->FightArray->len; AttackInd++) {
    Attack = (Player *)g_ptr_array_index(Play->FightArray, AttackInd);
    for (DefendInd = 0; DefendInd < AttackInd; DefendInd++) {
      Defend = (Player *)g_ptr_array_index(Play->FightArray, DefendInd);
      if (Attack != Play && Defend != Play && IsOpponent(Attack, Defend)) {
        FightDone = FALSE;
        break;
      }
    }
    if (!FightDone)
      break;
  }

  SendFightLeave(Play, FightDone);
  g_ptr_array_remove(Play->FightArray, (gpointer)Play);

  if (FightDone) {
    for (DefendInd = 0; DefendInd < Play->FightArray->len; DefendInd++) {
      Defend = (Player *)g_ptr_array_index(Play->FightArray, DefendInd);
      Defend->FightArray = NULL;
      ResolveTipoff(Defend);
      if (IsCop(Defend)) {
        FirstServer = RemovePlayer(Defend, FirstServer);
      } else if (Defend->Health == 0) {
        FinishGame(Defend, _("You're dead! Game over."));
      } else if (CanRunHere(Defend)
                 && brandom(0, 100) > Location[Defend->IsAt].PolicePresence) {
        Defend->EventNum = E_DOCTOR;
        Defend->DocPrice = prandom(Bitch.MinPrice, Bitch.MaxPrice) *
            Defend->Health / 500;
        text =
            dpg_strdup_printf(_
                              ("YN^Do you pay a doctor %P to sew you up?"),
                              Defend->DocPrice);
        SendQuestion(NULL, C_ASKSEW, Defend, text);
        g_free(text);
      } else {
        WaitForFightDone(Defend);
      }
    }
    g_ptr_array_free(Play->FightArray, TRUE);
  }
  Play->FightArray = NULL;
  Play->Attacking = NULL;
}

/* 
 * Inform player "To" of random offers or happenings. Returns 0 if
 * the client can immediately be advanced to the next state, or 1
 * there are first questions to be answered.
 */
int RandomOffer(Player *To)
{
  int r, amount, ind;
  GString *text;

  r = brandom(0, 100);

  text = g_string_new(NULL);

  if (!Sanitized && (r < 10)) {
    g_string_assign(text, _("You were mugged in the subway!"));
    To->Cash = To->Cash * (price_t)brandom(80, 95) / 100l;
  } else if (r < 30) {
    amount = brandom(3, 7);
    ind = IsCarryingRandom(To, amount);
    if (ind == -1 && amount > To->CoatSize) {
      g_string_free(text, TRUE);
      return 0;
    }
    if (ind == -1) {
      ind = brandom(0, NumDrug);
      dpg_string_sprintf(text,
                         _("You meet a friend! He gives you %d %tde."),
                         amount, Drug[ind].Name);
      To->Drugs[ind].Carried += amount;
      To->CoatSize -= amount;
    } else {
      dpg_string_sprintf(text,
                         _("You meet a friend! You give him %d %tde."),
                         amount, Drug[ind].Name);
      To->Drugs[ind].TotalValue =
          To->Drugs[ind].TotalValue * (To->Drugs[ind].Carried -
                                       amount) / To->Drugs[ind].Carried;
      To->Drugs[ind].Carried -= amount;
      To->CoatSize += amount;
    }
    SendPlayerData(To);
    SendPrintMessage(NULL, C_NONE, To, text->str);
  } else if (Sanitized) {
    dopelog(3, _("Sanitized away a RandomOffer"));
  } else if (r < 50) {
    amount = brandom(3, 7);
    ind = IsCarryingRandom(To, amount);
    if (ind != -1) {
      dpg_string_sprintf(text, _("Police dogs chase you for %d blocks! "
                                 "You dropped some %tde! That's a drag, man!"),
                         brandom(3, 7), Names.Drugs);
      To->Drugs[ind].TotalValue = To->Drugs[ind].TotalValue *
          (To->Drugs[ind].Carried - amount) / To->Drugs[ind].Carried;
      To->Drugs[ind].Carried -= amount;
      To->CoatSize += amount;
      SendPlayerData(To);
      SendPrintMessage(NULL, C_NONE, To, text->str);
    } else {
      ind = brandom(0, NumDrug);
      amount = brandom(3, 7);
      if (amount > To->CoatSize) {
        g_string_free(text, TRUE);
        return 0;
      }
      dpg_string_sprintf(text,
                         _("You find %d %tde on a dead dude in the subway!"),
                         amount, Drug[ind].Name);
      To->Drugs[ind].Carried += amount;
      To->CoatSize -= amount;
      SendPlayerData(To);
      SendPrintMessage(NULL, C_NONE, To, text->str);
    }
  } else if (r < 60
             && To->Drugs[WEED].Carried + To->Drugs[HASHISH].Carried > 0) {
    ind = (To->Drugs[WEED].Carried >
           To->Drugs[HASHISH].Carried) ? WEED : HASHISH;
    amount = brandom(2, 6);
    if (amount > To->Drugs[ind].Carried)
      amount = To->Drugs[ind].Carried;
    dpg_string_sprintf(text,
                       _("Your mama made brownies with some of your %tde! "
                         "They were great!"), Drug[ind].Name);
    To->Drugs[ind].TotalValue = To->Drugs[ind].TotalValue *
        (To->Drugs[ind].Carried - amount) / To->Drugs[ind].Carried;
    To->Drugs[ind].Carried -= amount;
    To->CoatSize += amount;
    SendPlayerData(To);
    SendPrintMessage(NULL, C_NONE, To, text->str);
  } else if (r < 65) {
    g_string_assign(text,
                    _("YN^There is some weed that smells like paraquat "
                     "here!^It looks good! Will you smoke it? "));
    To->EventNum = E_WEED;
    SendQuestion(NULL, C_NONE, To, text->str);
    g_string_free(text, TRUE);
    return 1;
  } else if (NumStoppedTo > 0) {
    g_string_sprintf(text, _("You stopped to %s."),
                     StoppedTo[brandom(0, NumStoppedTo)]);
    amount = brandom(1, 10);
    if (To->Cash >= amount)
      To->Cash -= amount;
    SendPlayerData(To);
    SendPrintMessage(NULL, C_NONE, To, text->str);
  }
  g_string_free(text, TRUE);
  return 0;
}

/* 
 * Offers player "To" bitches/trenchcoats or guns. If ForceBitch is
 * TRUE, then a bitch is definitely offered. Returns 0 if the client
 * can advance immediately to the next state, 1 otherwise.
 */
int OfferObject(Player *To, gboolean ForceBitch)
{
  int ObjNum;
  gchar *text = NULL;

  if (brandom(0, 100) < 50 || ForceBitch) {
    if (WantAntique) {
      To->Bitches.Price = prandom(MINTRENCHPRICE, MAXTRENCHPRICE);
      text = dpg_strdup_printf(_("YN^Would you like to buy a bigger "
                                 "trenchcoat for %P?"), To->Bitches.Price);
    } else {
      To->Bitches.Price =
          prandom(Bitch.MinPrice, Bitch.MaxPrice) / (price_t)10;
      text =
          dpg_strdup_printf(_
                            ("YN^Hey dude! I'll help carry your %tde for a "
                             "mere %P. Yes or no?"), Names.Drugs,
                            To->Bitches.Price);
    }
    SendQuestion(NULL, C_ASKBITCH, To, text);
    g_free(text);
    return 1;
  } else if (!Sanitized && NumGun > 0
             && (TotalGunsCarried(To) < To->Bitches.Carried + 2)) {
    ObjNum = brandom(0, NumGun);
    To->Guns[ObjNum].Price = Gun[ObjNum].Price / 10;
    if (Gun[ObjNum].Space > To->CoatSize)
      return 0;
    text = dpg_strdup_printf(_("YN^Would you like to buy a %tde for %P?"),
                             Gun[ObjNum].Name, To->Guns[ObjNum].Price);
    SendQuestion(NULL, C_ASKGUN, To, text);
    g_free(text);
    return 1;
  }
  return 0;
}

/* Whether a particular drug is especially cheap or expensive */
enum DealType {
  DT_NORMAL, DT_CHEAP, DT_EXPENSIVE
};

/* 
 * Generates drug prices and drug busts etc. for player "To"
 * "Deal" is an array of size NumDrug.
 */
static void GenerateDrugsHere(Player *To, enum DealType *Deal)
{
  int NumEvents, NumDrugs, NumRandom, i;

  for (i = 0; i < NumDrug; i++) {
    To->Drugs[i].Price = 0;
    Deal[i] = DT_NORMAL;
  }
  NumEvents = 0;
  if (brandom(0, 100) < 70)
    NumEvents = 1;
  if (brandom(0, 100) < 40 && NumEvents == 1)
    NumEvents = 2;
  if (brandom(0, 100) < 5 && NumEvents == 2)
    NumEvents = 3;
  NumDrugs = 0;
  while (NumEvents > 0) {
    i = brandom(0, NumDrug);
    if (Deal[i] != DT_NORMAL)
      continue;
    if (Drug[i].Expensive && (!Drug[i].Cheap || brandom(0, 100) < 50)) {
      Deal[i] = DT_EXPENSIVE;
      To->Drugs[i].Price = prandom(Drug[i].MinPrice, Drug[i].MaxPrice)
          * Drugs.ExpensiveMultiply;
      NumDrugs++;
      NumEvents--;
    } else if (Drug[i].Cheap) {
      Deal[i] = DT_CHEAP;
      To->Drugs[i].Price = prandom(Drug[i].MinPrice, Drug[i].MaxPrice)
          / Drugs.CheapDivide;
      NumDrugs++;
      NumEvents--;
    }
  }
  NumRandom = brandom(Location[To->IsAt].MinDrug,
                      Location[To->IsAt].MaxDrug);
  if (NumRandom > NumDrug)
    NumRandom = NumDrug;

  NumDrugs = NumRandom - NumDrugs;
  while (NumDrugs > 0) {
    i = brandom(0, NumDrug);
    if (To->Drugs[i].Price == 0) {
      To->Drugs[i].Price = prandom(Drug[i].MinPrice, Drug[i].MaxPrice);
      NumDrugs--;
    }
  }
}

/* 
 * Sends details of drug prices to player "To". If "DisplayBusts"
 * is TRUE, also regenerates drug prices and sends details of
 * special events such as drug busts.
 */
void SendDrugsHere(Player *To, gboolean DisplayBusts)
{
  int i;
  enum DealType *Deal = NULL;
  gchar *prstr;
  GString *text;
  gboolean First;

  Deal = g_new0(enum DealType, NumDrug);
  if (DisplayBusts)
    GenerateDrugsHere(To, Deal);

  text = g_string_new(NULL);
  First = TRUE;
  if (DisplayBusts) {
    for (i = 0; i < NumDrug; i++) {
      if (Deal[i] != DT_NORMAL) {
        if (!First)
          g_string_append_c(text, '^');
        if (Deal[i] == DT_CHEAP) {
          g_string_append(text, Drug[i].CheapStr);
        } else {
          dpg_string_sprintfa(text, brandom(0, 100) < 50
                              ? Drugs.ExpensiveStr1 : Drugs.ExpensiveStr2,
                              Drug[i].Name);
        }
        First = FALSE;
      }
    }
  }
  g_free(Deal);

  if (!First)
    SendPrintMessage(NULL, C_NONE, To, text->str);
  g_string_truncate(text, 0);
  for (i = 0; i < NumDrug; i++) {
    g_string_sprintfa(text, "%s^",
                      (prstr = pricetostr(To->Drugs[i].Price)));
    g_free(prstr);
  }
  SendServerMessage(NULL, C_NONE, C_DRUGHERE, To, text->str);
  g_string_free(text, TRUE);
}

/* 
 * Handles the incoming message in "answer" from player "From" and
 * intended for player "To".
 */
void HandleAnswer(Player *From, Player *To, char *answer)
{
  int i;
  gchar *text;
  Player *Defender;

  if (!From || From->EventNum == E_NONE)
    return;
  if (answer[0] == 'Y' && From->EventNum == E_OFFOBJECT
      && From->Bitches.Price && From->Bitches.Price > From->Cash)
    answer[0] = 'N';
  if ((From->EventNum == E_FIGHT || From->EventNum == E_FIGHTASK)
      && CanRunHere(From)) {
    From->EventNum = E_FIGHT;
    if (answer[0] == 'R' || answer[0] == 'Y') {
      RunFromCombat(From, -1);
    } else {
      Fire(From);
    }
  } else if (answer[0] == 'Y')
    switch (From->EventNum) {
    case E_OFFOBJECT:
      if (g_slist_find(FirstServer, (gpointer)From->OnBehalfOf)) {
        dopelog(3, _("%s: offer was on behalf of %s"), GetPlayerName(From),
                GetPlayerName(From->OnBehalfOf));
        if (From->Bitches.Price) {
          text = dpg_strdup_printf(_("%s has accepted your %tde!"
                                     "^Use the G key to contact your spy."),
                                   GetPlayerName(From), Names.Bitch);
          From->OnBehalfOf->Flags |= SPYINGON;
          SendPlayerData(From->OnBehalfOf);
          SendPrintMessage(NULL, C_NONE, From->OnBehalfOf, text);
          g_free(text);
          i = GetListEntry(&(From->SpyList), From->OnBehalfOf);
          if (i >= 0)
            From->SpyList.Data[i].Turns = 0;
        }
      }
      if (From->Bitches.Price) {
        text = g_strdup_printf("bitch^0^1");
        BuyObject(From, text);
        g_free(text);
      } else {
        for (i = 0; i < NumGun; i++)
          if (From->Guns[i].Price) {
            text = g_strdup_printf("gun^%d^1", i);
            BuyObject(From, text);
            g_free(text);
            break;
          }
      }
      From->OnBehalfOf = NULL;
      From->EventNum++;
      SendEvent(From);
      break;
    case E_LOANSHARK:
      SendServerMessage(NULL, C_NONE, C_LOANSHARK, From, NULL);
      break;
    case E_BANK:
      SendServerMessage(NULL, C_NONE, C_BANK, From, NULL);
      break;
    case E_GUNSHOP:
      for (i = 0; i < NumGun; i++)
        From->Guns[i].Price = Gun[i].Price;
      SendServerMessage(NULL, C_NONE, C_GUNSHOP, From, NULL);
      break;
    case E_HIREBITCH:
      text = g_strdup_printf("bitch^0^1");
      BuyObject(From, text);
      g_free(text);
      From->EventNum++;
      SendEvent(From);
      break;
    case E_ROUGHPUB:
      From->EventNum++;
      SendEvent(From);
      break;
    case E_WEED:
      FinishGame(From, _("You hallucinated for three days on the wildest "
                         "trip you ever imagined!^Then you died because "
                         "your brain disintegrated!"));
      break;
    case E_DOCTOR:
      if (From->Cash >= From->DocPrice) {
        From->Cash -= From->DocPrice;
        From->Health = 100;
        SendPlayerData(From);
      }
      WaitForFightDone(From);
      break;
    default:
      break;
  } else if (From->EventNum == E_ARRIVE) {
    if ((answer[0] == 'A' || answer[0] == 'T') &&
        IsConnectedPlayer(From->OnBehalfOf) &&
        g_slist_find(FirstServer, (gpointer)From->OnBehalfOf)) {
      Defender = From->OnBehalfOf;
      From->OnBehalfOf = NULL;  /* So we don't think it was a tipoff */
      if (Defender->IsAt == From->IsAt) {
        if (answer[0] == 'A') {
          From->EventNum = Defender->EventNum = E_NONE;
          AttackPlayer(From, Defender);
        }
      } else {
        text = g_strdup_printf(_("Too late - %s has just left!"),
                               GetPlayerName(Defender));
        SendPrintMessage(NULL, C_NONE, From, text);
        g_free(text);
        From->EventNum++;
        SendEvent(From);
      }
    } else {
      From->EventNum++;
      SendEvent(From);
    }
    From->OnBehalfOf = NULL;
  } else
    switch (From->EventNum) {
    case E_ROUGHPUB:
      From->EventNum++;
      From->EventNum++;
      SendEvent(From);
      break;
    case E_DOCTOR:
      WaitForFightDone(From);
      break;
    case E_HIREBITCH:
    case E_GUNSHOP:
    case E_BANK:
    case E_LOANSHARK:
    case E_OFFOBJECT:
    case E_WEED:
      if (g_slist_find(FirstServer, (gpointer)From->OnBehalfOf)) {
        dopelog(3, _("%s: offer was on behalf of %s"), GetPlayerName(From),
                GetPlayerName(From->OnBehalfOf));
        if (From->Bitches.Price && From->EventNum == E_OFFOBJECT) {
          text = dpg_strdup_printf(_("%s has rejected your %tde!"),
                                   GetPlayerName(From), Names.Bitch);
          GainBitch(From->OnBehalfOf);
          SendPlayerData(From->OnBehalfOf);
          SendPrintMessage(NULL, C_NONE, From->OnBehalfOf, text);
          g_free(text);
          RemoveListPlayer(&(From->SpyList), From->OnBehalfOf);
        }
      }
      From->EventNum++;
      SendEvent(From);
      break;
    default:
      break;
    }
}

/* 
 * Processes a request stored in "data" from player "From" to buy an
 * object (bitch, gun, or drug).
 * Objects can be sold if the amount given in "data" is negative, and
 * given away if their current price is zero.
 */
void BuyObject(Player *From, char *data)
{
  char *cp, *type;
  int index, i, amount;

  cp = data;
  type = GetNextWord(&cp, "");
  index = GetNextInt(&cp, 0);
  amount = GetNextInt(&cp, 0);
  if (strcmp(type, "drug") == 0) {
    if (index >= 0 && index < NumDrug
        && From->Drugs[index].Carried + amount >= 0
        && From->CoatSize - amount >= 0 && (From->Drugs[index].Price != 0
                                            || amount < 0)
        && From->Cash >= amount * From->Drugs[index].Price) {
      if (amount > 0) {
        From->Drugs[index].TotalValue += amount * From->Drugs[index].Price;
      } else if (From->Drugs[index].Carried != 0) {
        From->Drugs[index].TotalValue = From->Drugs[index].TotalValue *
            (From->Drugs[index].Carried +
             amount) / From->Drugs[index].Carried;
      }
      From->Drugs[index].Carried += amount;
      From->CoatSize -= amount;
      From->Cash -= amount * From->Drugs[index].Price;
      SendPlayerData(From);

      if (!Sanitized && NumCop > 0 && NumGun > 0
          && (From->Drugs[index].Price == 0 &&
              brandom(0, 100) < Location[From->IsAt].PolicePresence)) {
        gchar *text;

        text = dpg_strdup_printf(_("The cops spot you dropping %tde!"),
                                 Names.Drugs);
        SendPrintMessage(NULL, C_NONE, From, text);
        g_free(text);
        CopsAttackPlayer(From);
      }
    }
  } else if (strcmp(type, "gun") == 0) {
    if (index >= 0 && index < NumGun
        && TotalGunsCarried(From) + amount >= 0
        && TotalGunsCarried(From) + amount <= From->Bitches.Carried + 2
        && From->Guns[index].Price != 0
        && From->CoatSize - amount * Gun[index].Space >= 0
        && From->Cash >= amount * From->Guns[index].Price) {
      From->Guns[index].Carried += amount;
      From->CoatSize -= amount * Gun[index].Space;
      From->Cash -= amount * From->Guns[index].Price;
      SendPlayerData(From);
    }
  } else if (strcmp(type, "bitch") == 0) {
    if (From->Bitches.Carried + amount >= 0
        && From->Bitches.Price != 0
        && amount * From->Bitches.Price <= From->Cash) {
      for (i = 0; i < amount; i++)
        GainBitch(From);
      if (amount > 0)
        From->Cash -= amount * From->Bitches.Price;
      SendPlayerData(From);
    }
  }
}

/* 
 * Clears the bitch and gun prices stored for player "Play".
 */
void ClearPrices(Player *Play)
{
  int i;

  Play->Bitches.Price = 0;
  for (i = 0; i < NumGun; i++)
    Play->Guns[i].Price = 0;
}

/* 
 * Gives player "Play" a new bitch (or larger trenchcoat).
 */
void GainBitch(Player *Play)
{
  Play->CoatSize += 10;
  Play->Bitches.Carried++;
}

/* 
 * Loses one bitch belonging to player "Play". If drugs or guns are
 * lost with the bitch, 1 is returned (0 otherwise) and the lost
 * items are added to "Guns" and "Drugs" if non-NULL.
 */
int LoseBitch(Player *Play, Inventory *Guns, Inventory *Drugs)
{
  int losedrug = 0, i, num, drugslost;
  int *GunIndex, tmp;
  GunIndex = g_new(int, NumGun);

  ClearInventory(Guns, Drugs);
  Play->CoatSize -= 10;
  if (TotalGunsCarried(Play) > 0) {
    if (brandom(0, 100) <
        TotalGunsCarried(Play) * 100 / (Play->Bitches.Carried + 2)) {
      for (i = 0; i < NumGun; i++)
        GunIndex[i] = i;
      for (i = 0; i < NumGun * 5; i++) {
        num = brandom(0, NumGun - 1);
        tmp = GunIndex[num + 1];
        GunIndex[num + 1] = GunIndex[num];
        GunIndex[num] = tmp;
      }
      for (i = 0; i < NumGun; i++) {
        if (Play->Guns[GunIndex[i]].Carried > 0) {
          Play->Guns[GunIndex[i]].Carried--;
          losedrug = 1;
          Play->CoatSize += Gun[GunIndex[i]].Space;
          if (Guns)
            Guns[GunIndex[i]].Carried++;
          break;
        }
      }
    }
  }
  for (i = 0; i < NumDrug; i++)
    if (Play->Drugs[i].Carried > 0) {
      num =
          (int)((float)Play->Drugs[i].Carried /
                (Play->Bitches.Carried + 2.0) + 0.5);
      if (num > 0) {
        Play->Drugs[i].TotalValue = Play->Drugs[i].TotalValue *
            (Play->Drugs[i].Carried - num) / Play->Drugs[i].Carried;
        Play->Drugs[i].Carried -= num;
        if (Drugs)
          Drugs[i].Carried += num;
        Play->CoatSize += num;
        losedrug = 1;
      }
    }
  while (Play->CoatSize < 0) {
    drugslost = 0;
    for (i = 0; i < NumDrug; i++) {
      if (Play->Drugs[i].Carried > 0) {
        losedrug = 1;
        drugslost = 1;
        Play->Drugs[i].TotalValue = Play->Drugs[i].TotalValue *
            (Play->Drugs[i].Carried - 1) / Play->Drugs[i].Carried;
        Play->Drugs[i].Carried--;
        Play->CoatSize++;
        if (Play->CoatSize >= 0)
          break;
      }
    }
    if (!drugslost)
      for (i = 0; i < NumGun; i++) {
        if (Play->Guns[i].Carried > 0) {
          losedrug = 1;
          Play->Guns[i].Carried--;
          Play->CoatSize += Gun[i].Space;
          if (Play->CoatSize >= 0)
            break;
        }
      }
  }
  Play->Bitches.Carried--;
  g_free(GunIndex);
  return losedrug;
}

/* 
 * If fight timeouts are in force, sets the timeout for the given player.
 */
void SetFightTimeout(Player *Play)
{
  if (FightTimeout)
    Play->FightTimeout = time(NULL) + (time_t) FightTimeout;
  else
    Play->FightTimeout = 0;
}

/* 
 * Removes any fight timeout for the given player.
 */
void ClearFightTimeout(Player *Play)
{
  Play->FightTimeout = 0;
}

/* 
 * Given the time of a pending event in "timeout" and the current time in
 * "timenow", updates "mintime" with the number of seconds to that event,
 * unless "mintime" is already smaller (as long as it's not -1, which
 * means "uninitialized"). Returns 1 if the timeout has already expired.
 */
int AddTimeout(time_t timeout, time_t timenow, int *mintime)
{
  if (timeout == 0)
    return 0;
  else if (timeout <= timenow)
    return 1;
  else {
    if (*mintime < 0 || timeout - timenow < *mintime)
      *mintime = timeout - timenow;
    return 0;
  }
}

/* 
 * Returns the number of seconds until the next scheduled event. If such
 * an event has already expired, returns 0. If no events are pending,
 * returns -1. "First" should point to a list of valid players.
 */
int GetMinimumTimeout(GSList *First)
{
  Player *Play;
  GSList *list;
  int mintime = -1;
  time_t timenow;

  timenow = time(NULL);
  if (AddTimeout(MetaMinTimeout, timenow, &mintime))
    return 0;
  if (AddTimeout(MetaUpdateTimeout, timenow, &mintime))
    return 0;
  for (list = First; list; list = g_slist_next(list)) {
    Play = (Player *)list->data;
    if (AddTimeout(Play->FightTimeout, timenow, &mintime) ||
        AddTimeout(Play->IdleTimeout, timenow, &mintime) ||
        AddTimeout(Play->ConnectTimeout, timenow, &mintime))
      return 0;
  }
  return mintime;
}

/* 
 * Given a list of players in "First", checks to see if any events
 * have timed out, and if so, performs the necessary actions. The
 * new start of the list is returned, since a player may be removed
 * if their ConnectTimeout has expired.
 */
GSList *HandleTimeouts(GSList *First)
{
  GSList *list, *nextlist;
  Player *Play;
  time_t timenow;

  timenow = time(NULL);
  if (MetaMinTimeout <= timenow) {
    MetaMinTimeout = 0;
    if (MetaPlayerPending) {
      dopelog(3, _("Sending pending updates to the metaserver..."));
      RegisterWithMetaServer(TRUE, TRUE, FALSE);
    }
  }
  if (MetaUpdateTimeout != 0 && MetaUpdateTimeout <= timenow) {
    dopelog(3, _("Sending reminder message to the metaserver..."));
    RegisterWithMetaServer(TRUE, FALSE, FALSE);
  }
  list = First;
  while (list) {
    nextlist = g_slist_next(list);
    Play = (Player *)list->data;
    if (Play->IdleTimeout != 0 && Play->IdleTimeout <= timenow) {
      Play->IdleTimeout = 0;
      dopelog(1, _("Player removed due to idle timeout"));
      SendPrintMessage(NULL, C_NONE, Play,
                       "Disconnected due to idle timeout");
      ClientLeftServer(Play);
      /* Blank the name, so that CountPlayers ignores this player */
      SetPlayerName(Play, NULL);
      /* Make sure they do actually disconnect, eventually! */
      if (ConnectTimeout) {
        Play->ConnectTimeout = time(NULL) + (time_t) ConnectTimeout;
      }
    } else if (Play->ConnectTimeout != 0
               && Play->ConnectTimeout <= timenow) {
      Play->ConnectTimeout = 0;
      dopelog(1, _("Player removed due to connect timeout"));
      First = RemovePlayer(Play, First);
    } else if (IsConnectedPlayer(Play) &&
               Play->FightTimeout != 0 && Play->FightTimeout <= timenow) {
      ClearFightTimeout(Play);
      if (IsCop(Play))
        Fire(Play);
      else
        SendFightReload(Play);
    }
    list = nextlist;
  }
  return First;
}
