/************************************************************************
 * message.c      Message-handling routines for dopewars                *
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef CYGWIN
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include "dopewars.h"
#include "message.h"
#include "network.h"
#include "nls.h"
#include "serverside.h"
#include "tstring.h"
#include "util.h"

/* Maximum sizes (in bytes) of read and write buffers - connections should
 * be dropped if either buffer is filled */
#define MAXREADBUF   (32768)
#define MAXWRITEBUF  (65536)

/* *INDENT-OFF* */
/* dopewars is built around a client-server model. Each client handles the
   user interface, but all the important calculation and processing is
   handled by the server. All communication is conducted via. TCP by means
   of plain text newline-delimited messages.

   New message structure:-
       Other^ACData

   Other:   The ID of the player at the "other end" of the connection;
            for messages received by the client, this is the player from
            which the message originates, while for messages received by
            the server, it's the player to which to deliver the message.
   A:       One-letter code; used by AI players to identify the message subtype
                                       (check AIPlayer.h)
   C:       One-letter code to identify the message type (check message.h)
   Data:    Message-dependent information


   For compatibility with old clients and servers, the old message structure
   is still supported:-
       From^To^ACData

   From,To:  Player names identifying the sender and intended recipient of
             the message. Either field may be blank, although the server will
             usually reject incoming messages if they are not properly
             identified with a correct "From" field.
   A,C,Data: As above, for the new message structure

   For example, a common message is the "printmessage" message (message code 
   C is C_PRINTMESSAGE), which simply instructs the client to display "Data". 
   Any ^ characters within Data are replaced by newlines on output. So in order 
   for the server to instruct player "Fred" (ID 1) to display "Hello world" it
   would send the message:-
       ^AAHello world                   (new format)
       ^Fred^AAHello world              (old format)
   Note that the server has left the Other (or From) field blank, and has
   specified the AI code 'A' - defined in AIPlayer.h as C_NONE (i.e. an
   "unimportant" message) as well as the main code 'A', defined as
   C_PRINTMESSAGE in message.h. Note also that the destination player (Fred)
   is not specified in the new format; the player is identified by the socket
   through which the message is transmitted.

   When the network is down, a server is simulated locally. Messages from
   the client are passed directly to the server message handling routine,
   and vice versa.  */
/* *INDENT-ON* */

GSList *FirstClient;

void (*ClientMessageHandlerPt)(char *, Player *) = NULL;

/* 
 * Sends a message from player "From" to player "To" via. the server.
 * AI, Code and Data define the message.
 */
void SendClientMessage(Player *From, AICode AI, MsgCode Code,
                       Player *To, char *Data)
{
  DoSendClientMessage(From, AI, Code, To, Data, From);
}

/* 
 * Sends a message from player "From" to player "To" via. the server,
 * sending a blank name for "From" (this is required with the old message
 * format, up to and including the first successful C_NAME message, but has
 * no effect with the new format. AI, Code and Data define the message.
 */
void SendNullClientMessage(Player *From, AICode AI, MsgCode Code,
                           Player *To, char *Data)
{
  DoSendClientMessage(NULL, AI, Code, To, Data, From);
}

/* 
 * Send a message from client player "From" with computer code "AI",
 * human-readable code "Code" and data "Data". The message is sent to the
 * server, identifying itself as for "To". "BufOwn" identifies the player
 * which owns the buffers used for the actual wire connection. With the
 * new message format, "From" is ignored. From, To, or Data may be NULL.
 */
void DoSendClientMessage(Player *From, AICode AI, MsgCode Code,
                         Player *To, char *Data, Player *BufOwn)
{
  GString *text;
  Player *ServerFrom;

  g_assert(BufOwn != NULL);
  text = g_string_new(NULL);
  if (HaveAbility(BufOwn, A_PLAYERID)) {
    if (To)
      g_string_sprintfa(text, "%d", To->ID);
    g_string_sprintfa(text, "^%c%c%s", AI, Code, Data ? Data : "");
  } else {
    g_string_sprintf(text, "%s^%s^%c%c%s", From ? GetPlayerName(From) : "",
                     To ? GetPlayerName(To) : "", AI, Code,
                     Data ? Data : "");
  }
#if NETWORKING
  if (!Network) {
#endif
    if (From)
      ServerFrom = GetPlayerByName(GetPlayerName(From), FirstServer);
    else if (FirstServer)
      ServerFrom = (Player *)(FirstServer->data);
    else {
      ServerFrom = g_new(Player, 1);
      FirstServer = AddPlayer(0, ServerFrom, FirstServer);
    }
    HandleServerMessage(text->str, ServerFrom);
#if NETWORKING
  } else {
    QueuePlayerMessageForSend(BufOwn, text->str);
  }
#endif /* NETWORKING */
  g_string_free(text, TRUE);
}

/* 
 * Shorthand for the server sending a "printmessage"; instructs the
 * client "To" to display "Data".
 */
void SendPrintMessage(Player *From, AICode AI, Player *To, char *Data)
{
  SendServerMessage(From, AI, C_PRINTMESSAGE, To, Data);
}

/* 
 * Shorthand for the server sending a "question"; instructs the client
 * "To" to display the second word of Data and accept any letter within
 * the first word of Data as suitable reply.
 */
void SendQuestion(Player *From, AICode AI, Player *To, char *Data)
{
  SendServerMessage(From, AI, C_QUESTION, To, Data);
}

/* 
 * Sends a message from the server to client player "To" with computer
 * code "AI", human-readable code "Code" and data "Data", claiming
 * to be from player "From".
 */
void SendServerMessage(Player *From, AICode AI, MsgCode Code,
                       Player *To, char *Data)
{
  GString *text;

  if (IsCop(To))
    return;
  text = g_string_new(NULL);
  if (HaveAbility(To, A_PLAYERID)) {
    if (From)
      g_string_sprintfa(text, "%d", From->ID);
    g_string_sprintfa(text, "^%c%c%s", AI, Code, Data ? Data : "");
  } else {
    g_string_sprintf(text, "%s^%s^%c%c%s", From ? GetPlayerName(From) : "",
                     To ? GetPlayerName(To) : "", AI, Code,
                     Data ? Data : "");
  }
#if NETWORKING
  if (!Network) {
#endif
    if (ClientMessageHandlerPt)
      (*ClientMessageHandlerPt)(text->str, (Player *)(FirstClient->data));
#if NETWORKING
  } else {
    QueuePlayerMessageForSend(To, text->str);
  }
#endif
  g_string_free(text, TRUE);
}

/* 
 * Sets up the "abilities" of player "Play". Abilities are used to extend
 * the protocol; if both the client and server share an ability, then the
 * protocol extension can be used.
 */
void InitAbilities(Player *Play)
{
  int i;

  /* Clear all abilities */
  for (i = 0; i < A_NUM; i++) {
    Play->Abil.Local[i] = FALSE;
    Play->Abil.Remote[i] = FALSE;
    Play->Abil.Shared[i] = FALSE;
  }

  /* Set local abilities; abilities that are client-dependent (e.g.
   * A_NEWFIGHT) can be overridden by individual clients if required, by
   * calling SetAbility, prior to calling SendAbilities */
  Play->Abil.Local[A_PLAYERID] = TRUE;
  Play->Abil.Local[A_NEWFIGHT] = TRUE;
  Play->Abil.Local[A_DRUGVALUE] = (DrugValue ? TRUE : FALSE);
  Play->Abil.Local[A_TSTRING] = TRUE;
  Play->Abil.Local[A_DONEFIGHT] = TRUE;

  if (!Network)
    for (i = 0; i < A_NUM; i++) {
      Play->Abil.Remote[i] = Play->Abil.Shared[i] = Play->Abil.Local[i];
    }
}

/* 
 * Sends abilities of player "Play" to the other end of the client-server
 * connection.
 */
void SendAbilities(Player *Play)
{
  int i;
  gchar Data[A_NUM + 1];

  if (!Network)
    return;
  for (i = 0; i < A_NUM; i++)
    Data[i] = (Play->Abil.Local[i] ? '1' : '0');
  Data[A_NUM] = '\0';
  if (Server)
    SendServerMessage(NULL, C_NONE, C_ABILITIES, Play, Data);
  else
    SendClientMessage(Play, C_NONE, C_ABILITIES, NULL, Data);
}

/* 
 * Fills in the "remote" abilities of player "Play" using the message data
 * in "Data". These are the abilities of the server/client at the other
 * end of the connection.
 */
void ReceiveAbilities(Player *Play, gchar *Data)
{
  int i, Length;

  InitAbilities(Play);
  if (!Network)
    return;
  Length = MIN(strlen(Data), A_NUM);
  for (i = 0; i < Length; i++) {
    Play->Abil.Remote[i] = (Data[i] == '1' ? TRUE : FALSE);
  }
}

/* 
 * Combines the local and remote abilities of player "Play". The resulting
 * shared abilities are used to determine when protocol extensions can be
 * used.
 */
void CombineAbilities(Player *Play)
{
  int i;

  for (i = 0; i < A_NUM; i++) {
    Play->Abil.Shared[i] = (Play->Abil.Remote[i] && Play->Abil.Local[i]);
  }
}

/* 
 * Sets ability "Type" of player "Play", and also sets shared abilities if
 * networking is not active (the local server should support all abilities
 * that the client uses). Call this function prior to calling SendAbilities
 * so that the ability is recognised properly when networking _is_ active
 */
void SetAbility(Player *Play, gint Type, gboolean Set)
{
  if (Type < 0 || Type >= A_NUM)
    return;
  Play->Abil.Local[Type] = Set;
  if (!Network) {
    Play->Abil.Remote[Type] = Play->Abil.Shared[Type] =
        Play->Abil.Local[Type];
  }
}

/* 
 * Returns TRUE if ability "Type" is one of player "Play"'s shared abilities
 */
gboolean HaveAbility(Player *Play, gint Type)
{
  if (Type < 0 || Type >= A_NUM)
    return FALSE;
  else
    return (Play->Abil.Shared[Type]);
}

#if NETWORKING
/* 
 * Reads and writes player data from/to the network if it is ready.
 * If any data were read, TRUE is returned. "DoneOK" is set TRUE
 * unless a fatal error (i.e. the connection was broken) occurred.
 */
gboolean PlayerHandleNetwork(Player *Play, gboolean ReadReady,
                             gboolean WriteReady, gboolean *DoneOK)
{
  gboolean DataWaiting = FALSE;

  *DoneOK = TRUE;
  if (!Play)
    return DataWaiting;
  DataWaiting =
      NetBufHandleNetwork(&Play->NetBuf, ReadReady, WriteReady, DoneOK);

  return DataWaiting;
}

gchar *GetWaitingPlayerMessage(Player *Play)
{
  return GetWaitingMessage(&Play->NetBuf);
}

gboolean ReadPlayerDataFromWire(Player *Play)
{
  return ReadDataFromWire(&Play->NetBuf);
}

void QueuePlayerMessageForSend(Player *Play, gchar *data)
{
  QueueMessageForSend(&Play->NetBuf, data);
}

gboolean WritePlayerDataToWire(Player *Play)
{
  return WriteDataToWire(&Play->NetBuf);
}

typedef enum {
  MEC_INTERNAL,
  MEC_BADREPLY
} MetaErrorCode;

static void MetaAppendError(GString *str, LastError *error)
{
  switch (error->code) {
  case MEC_INTERNAL:
    g_string_sprintfa(str, _("Internal metaserver error \"%s\""),
                      (gchar *)error->data);
    break;
  case MEC_BADREPLY:
    g_string_sprintfa(str, _("Bad metaserver reply \"%s\""),
                      (gchar *)error->data);
    break;
  default:
    g_string_sprintfa(str, _("Unknown metaserver error code %d"),
                      error->code);
    break;
  }
}

static ErrorType ETMeta = { MetaAppendError, NULL };

gboolean OpenMetaHttpConnection(HttpConnection **conn)
{
  gchar *query;
  gboolean retval;

  query = g_strdup_printf("%s?output=text&getlist=%d",
                          MetaServer.Path, METAVERSION);
  retval = OpenHttpConnection(conn, MetaServer.Name, MetaServer.Port,
                              MetaServer.ProxyName, MetaServer.ProxyPort,
                              UseSocks
                              && MetaServer.UseSocks ? &Socks : NULL,
                              "GET", query, NULL, NULL);
  g_free(query);
  return retval;
}

gboolean HandleWaitingMetaServerData(HttpConnection *conn,
                                     GSList **listpt, gboolean *doneOK)
{
  gchar *msg;
  ServerData *NewServer;

  g_assert(conn && listpt && doneOK);

  /* If we're done reading the headers, only read if the data for a whole
   * server is available (8 lines) N.B. "Status" is from the _last_ read */
  if (conn->Status == HS_READBODY && conn->StatusCode == 200) {
    if (CountWaitingMessages(&conn->NetBuf) < 8)
      return FALSE;

    NewServer = g_new0(ServerData, 1);
    NewServer->Name = ReadHttpResponse(conn, doneOK);
    msg = ReadHttpResponse(conn, doneOK);
    NewServer->Port = atoi(msg);
    g_free(msg);
    NewServer->Version = ReadHttpResponse(conn, doneOK);
    msg = ReadHttpResponse(conn, doneOK);
    if (msg[0])
      NewServer->CurPlayers = atoi(msg);
    else
      NewServer->CurPlayers = -1;
    g_free(msg);
    msg = ReadHttpResponse(conn, doneOK);
    NewServer->MaxPlayers = atoi(msg);
    g_free(msg);
    NewServer->Update = ReadHttpResponse(conn, doneOK);
    NewServer->Comment = ReadHttpResponse(conn, doneOK);
    NewServer->UpSince = ReadHttpResponse(conn, doneOK);
    *listpt = g_slist_append(*listpt, NewServer);
  } else if (conn->Status == HS_READSEPARATOR && conn->StatusCode == 200) {
    /* This should be the first line of the body, the "MetaServer:" line */
    msg = ReadHttpResponse(conn, doneOK);
    if (!msg)
      return FALSE;
    if (strlen(msg) >= 14 && strncmp(msg, "FATAL ERROR:", 12) == 0) {
      SetError(&conn->NetBuf.error, &ETMeta, MEC_INTERNAL,
               g_strdup(&msg[13]));
      *doneOK = FALSE;
      return FALSE;
    } else if (strncmp(msg, "MetaServer:", 11) != 0) {
      SetError(&conn->NetBuf.error, &ETMeta, MEC_BADREPLY, g_strdup(msg));
      g_free(msg);
      *doneOK = FALSE;
      return FALSE;
    }
    g_free(msg);
  } else {
    msg = ReadHttpResponse(conn, doneOK);
    if (!msg)
      return FALSE;
    g_free(msg);
  }
  return TRUE;
}

void ClearServerList(GSList **listpt)
{
  ServerData *ThisServer;

  while (*listpt) {
    ThisServer = (ServerData *)((*listpt)->data);
    g_free(ThisServer->Name);
    g_free(ThisServer->Comment);
    g_free(ThisServer->Version);
    g_free(ThisServer->Update);
    g_free(ThisServer->UpSince);
    g_free(ThisServer);
    *listpt = g_slist_remove(*listpt, ThisServer);
  }
}
#endif /* NETWORKING */

/* 
 * Removes a terminating newline from "str", if one is present.
 */
void chomp(char *str)
{
  int len = strlen(str);

  if (str[len - 1] == '\n')
    str[len - 1] = 0;
}

/* 
 * Adds the plain text string "unenc" to the end of the GString "str",
 * replacing "special" characters in the same way as the
 * application/x-www-form-urlencoded media type, suitable for sending
 * to CGI scripts etc.
 */
void AddURLEnc(GString *str, gchar *unenc)
{
  guint i;

  if (!unenc || !str)
    return;
  for (i = 0; i < strlen(unenc); i++) {
    if ((unenc[i] >= 'a' && unenc[i] <= 'z') ||
        (unenc[i] >= 'A' && unenc[i] <= 'Z') ||
        (unenc[i] >= '0' && unenc[i] <= '9') ||
        unenc[i] == '-' || unenc[i] == '_' || unenc[i] == '.') {
      g_string_append_c(str, unenc[i]);
    } else if (unenc[i] == ' ') {
      g_string_append_c(str, '+');
    } else {
      g_string_sprintfa(str, "%%%02X", unenc[i]);
    }
  }
}

/* 
 * Sends the message made up of AI,Code and Data to all players except
 * "Except" (if non-NULL). It will be sent by the server, and on behalf of
 * player "From".
 */
void BroadcastToClients(AICode AI, MsgCode Code, char *Data,
                        Player *From, Player *Except)
{
  Player *tmp;
  GSList *list;

  for (list = FirstServer; list; list = g_slist_next(list)) {
    tmp = (Player *)list->data;
    if (IsConnectedPlayer(tmp) && tmp != Except) {
      SendServerMessage(From, AI, Code, tmp, Data);
    }
  }
}

/* 
 * Encodes an Inventory structure into a string, and sends it as the data
 * with a server message constructed from the other arguments.
 */
void SendInventory(Player *From, AICode AI, MsgCode Code,
                   Player *To, Inventory *Guns, Inventory *Drugs)
{
  int i;
  GString *text;

  text = g_string_new(NULL);
  for (i = 0; i < NumGun; i++) {
    g_string_sprintfa(text, "%d:", Guns ? Guns[i].Carried : 0);
  }
  for (i = 0; i < NumDrug; i++) {
    g_string_sprintfa(text, "%d:", Drugs ? Drugs[i].Carried : 0);
  }
  SendServerMessage(From, AI, Code, To, text->str);
  g_string_free(text, TRUE);
}

/* 
 * Decodes a string representation (in "Data") to its original Inventory
 * contents, and stores it in "Guns" and "Drugs" if non-NULL.
 */
void ReceiveInventory(char *Data, Inventory *Guns, Inventory *Drugs)
{
  int i, val;
  char *pt;

  pt = Data;
  for (i = 0; i < NumGun; i++) {
    val = GetNextInt(&pt, 0);
    if (Guns)
      Guns[i].Carried = val;
  }
  for (i = 0; i < NumDrug; i++) {
    val = GetNextInt(&pt, 0);
    if (Drugs)
      Drugs[i].Carried = val;
  }
}

/* 
 * Sends all pertinent data about player "To" from the server
 * to player "To".
 */
void SendPlayerData(Player *To)
{
  SendSpyReport(To, To);
}

/* 
 * Sends pertinent data about player "SpiedOn" from the server
 * to player "To".
 */
void SendSpyReport(Player *To, Player *SpiedOn)
{
  gchar *cashstr, *debtstr, *bankstr;
  GString *text;
  int i;

  text = g_string_new(NULL);
  g_string_sprintf(text, "%s^%s^%s^%d^%d^%d^%d^%d^",
                   (cashstr = pricetostr(SpiedOn->Cash)),
                   (debtstr = pricetostr(SpiedOn->Debt)),
                   (bankstr = pricetostr(SpiedOn->Bank)),
                   SpiedOn->Health, SpiedOn->CoatSize,
                   SpiedOn->IsAt, SpiedOn->Turn, SpiedOn->Flags);
  g_free(cashstr);
  g_free(debtstr);
  g_free(bankstr);
  for (i = 0; i < NumGun; i++) {
    g_string_sprintfa(text, "%d^", SpiedOn->Guns[i].Carried);
  }
  for (i = 0; i < NumDrug; i++) {
    g_string_sprintfa(text, "%d^", SpiedOn->Drugs[i].Carried);
  }
  if (HaveAbility(To, A_DRUGVALUE))
    for (i = 0; i < NumDrug; i++) {
      g_string_sprintfa(text, "%s^",
                        (cashstr =
                         pricetostr(SpiedOn->Drugs[i].TotalValue)));
      g_free(cashstr);
    }
  g_string_sprintfa(text, "%d", SpiedOn->Bitches.Carried);
  if (To != SpiedOn)
    SendServerMessage(SpiedOn, C_NONE, C_UPDATE, To, text->str);
  else
    SendServerMessage(NULL, C_NONE, C_UPDATE, To, text->str);
  g_string_free(text, TRUE);
}

#define NUMNAMES 12

void SendInitialData(Player *To)
{
  gchar *LocalNames[NUMNAMES] = { Names.Bitch, Names.Bitches, Names.Gun,
    Names.Guns, Names.Drug, Names.Drugs,
    Names.Month, Names.Year, Names.LoanSharkName,
    Names.BankName, Names.GunShopName,
    Names.RoughPubName
  };
  gint i;
  GString *text;

  if (!Network)
    return;
  if (!HaveAbility(To, A_TSTRING))
    for (i = 0; i < NUMNAMES; i++) {
      LocalNames[i] = GetDefaultTString(LocalNames[i]);
    }
  text = g_string_new("");
  g_string_sprintf(text, "%s^%d^%d^%d^", VERSION, NumLocation, NumGun,
                   NumDrug);
  for (i = 0; i < 8; i++) {
    g_string_append(text, LocalNames[i]);
    g_string_append_c(text, '^');
  }
  if (HaveAbility(To, A_PLAYERID))
    g_string_sprintfa(text, "%d^", To->ID);

  /* Player ID is expected after the first 8 names, so send the rest now */
  for (i = 8; i < NUMNAMES; i++) {
    g_string_append(text, LocalNames[i]);
    g_string_append_c(text, '^');
  }

  if (!HaveAbility(To, A_TSTRING))
    for (i = 0; i < NUMNAMES; i++) {
      g_free(LocalNames[i]);
    }

  g_string_sprintfa(text, "%c%s^", Currency.Prefix ? '1' : '0',
                    Currency.Symbol);
  SendServerMessage(NULL, C_NONE, C_INIT, To, text->str);
  g_string_free(text, TRUE);
}

void ReceiveInitialData(Player *Play, char *Data)
{
  char *pt, *ServerVersion, *curr;
  GSList *list;

  pt = Data;
  ServerVersion = GetNextWord(&pt, "(unknown)");
  ResizeLocations(GetNextInt(&pt, NumLocation));
  ResizeGuns(GetNextInt(&pt, NumGun));
  ResizeDrugs(GetNextInt(&pt, NumDrug));
  for (list = FirstClient; list; list = g_slist_next(list)) {
    UpdatePlayer((Player *)list->data);
  }
  AssignName(&Names.Bitch, GetNextWord(&pt, ""));
  AssignName(&Names.Bitches, GetNextWord(&pt, ""));
  AssignName(&Names.Gun, GetNextWord(&pt, ""));
  AssignName(&Names.Guns, GetNextWord(&pt, ""));
  AssignName(&Names.Drug, GetNextWord(&pt, ""));
  AssignName(&Names.Drugs, GetNextWord(&pt, ""));
  AssignName(&Names.Month, GetNextWord(&pt, ""));
  AssignName(&Names.Year, GetNextWord(&pt, ""));
  if (HaveAbility(Play, A_PLAYERID))
    Play->ID = GetNextInt(&pt, 0);

  /* Servers up to version 1.4.8 don't send the following names, so
   * default to the existing values if they haven't been sent */
  AssignName(&Names.LoanSharkName, GetNextWord(&pt, Names.LoanSharkName));
  AssignName(&Names.BankName, GetNextWord(&pt, Names.BankName));
  AssignName(&Names.GunShopName, GetNextWord(&pt, Names.GunShopName));
  AssignName(&Names.RoughPubName, GetNextWord(&pt, Names.RoughPubName));

  /* Currency data are only sent by versions >= 1.5.3 */
  curr = GetNextWord(&pt, NULL);
  if (curr && strlen(curr) >= 1) {
    Currency.Prefix = (curr[0] == '1');
    AssignName(&Currency.Symbol, &curr[1]);
  }

  if (strcmp(VERSION, ServerVersion) != 0) {
    g_message(_("This server is version %s, while your client is "
                "version %s.\nBe warned that different versions may not "
                "be fully compatible!\nRefer to the website at "
                "http://dopewars.sourceforge.net/\nfor the latest version."),
              ServerVersion, VERSION);
  }
}

void SendMiscData(Player *To)
{
  gchar *text, *prstr[2], *LocalName;
  int i;
  gboolean HaveTString;

  if (!Network)
    return;
  HaveTString = HaveAbility(To, A_TSTRING);
  text = g_strdup_printf("0^%c%s^%s^", DT_PRICES,
                         (prstr[0] = pricetostr(Prices.Spy)),
                         (prstr[1] = pricetostr(Prices.Tipoff)));
  SendServerMessage(NULL, C_NONE, C_DATA, To, text);
  g_free(prstr[0]);
  g_free(prstr[1]);
  g_free(text);
  for (i = 0; i < NumGun; i++) {
    if (HaveTString)
      LocalName = Gun[i].Name;
    else
      LocalName = GetDefaultTString(Gun[i].Name);
    text = g_strdup_printf("%d^%c%s^%s^%d^%d^", i, DT_GUN, LocalName,
                           (prstr[0] = pricetostr(Gun[i].Price)),
                           Gun[i].Space, Gun[i].Damage);
    if (!HaveTString)
      g_free(LocalName);
    SendServerMessage(NULL, C_NONE, C_DATA, To, text);
    g_free(prstr[0]);
    g_free(text);
  }
  for (i = 0; i < NumDrug; i++) {
    if (HaveTString)
      LocalName = Drug[i].Name;
    else
      LocalName = GetDefaultTString(Drug[i].Name);
    text = g_strdup_printf("%d^%c%s^%s^%s^", i, DT_DRUG, LocalName,
                           (prstr[0] = pricetostr(Drug[i].MinPrice)),
                           (prstr[1] = pricetostr(Drug[i].MaxPrice)));
    if (!HaveTString)
      g_free(LocalName);
    SendServerMessage(NULL, C_NONE, C_DATA, To, text);
    g_free(prstr[0]);
    g_free(prstr[1]);
    g_free(text);
  }
  for (i = 0; i < NumLocation; i++) {
    if (HaveTString)
      LocalName = Location[i].Name;
    else
      LocalName = GetDefaultTString(Location[i].Name);
    text = g_strdup_printf("%d^%c%s^", i, DT_LOCATION, LocalName);
    if (!HaveTString)
      g_free(LocalName);
    SendServerMessage(NULL, C_NONE, C_DATA, To, text);
    g_free(text);
  }
}

/* 
 * Decodes information about locations, drugs, prices, etc. in "Data"
 */
void ReceiveMiscData(char *Data)
{
  char *pt, *Name, Type;
  int i;

  pt = Data;
  i = GetNextInt(&pt, 0);
  Name = GetNextWord(&pt, "");
  Type = Name[0];
  if (strlen(Name) > 1) {
    switch (Type) {
    case DT_LOCATION:
      if (i >= 0 && i < NumLocation) {
        AssignName(&Location[i].Name, &Name[1]);
        Location[i].PolicePresence = 10;
        Location[i].MinDrug = NumDrug / 2 + 1;
        Location[i].MaxDrug = NumDrug;
      }
      break;
    case DT_GUN:
      if (i >= 0 && i < NumGun) {
        AssignName(&Gun[i].Name, &Name[1]);
        Gun[i].Price = GetNextPrice(&pt, (price_t)0);
        Gun[i].Space = GetNextInt(&pt, 0);
        Gun[i].Damage = GetNextInt(&pt, 0);
      }
      break;
    case DT_DRUG:
      if (i >= 0 && i < NumDrug) {
        AssignName(&Drug[i].Name, &Name[1]);
        Drug[i].MinPrice = GetNextPrice(&pt, (price_t)0);
        Drug[i].MaxPrice = GetNextPrice(&pt, (price_t)0);
      }
      break;
    case DT_PRICES:
      Prices.Spy = strtoprice(&Name[1]);
      Prices.Tipoff = GetNextPrice(&pt, (price_t)0);
      break;
    }
  }
}

/* 
 * Decode player data from the string "text" into player "From"; "Play"
 * specifies the player that owns the network connection.
 */
void ReceivePlayerData(Player *Play, char *text, Player *From)
{
  char *cp;
  int i;

  cp = text;
  From->Cash = GetNextPrice(&cp, (price_t)0);
  From->Debt = GetNextPrice(&cp, (price_t)0);
  From->Bank = GetNextPrice(&cp, (price_t)0);
  From->Health = GetNextInt(&cp, 100);
  From->CoatSize = GetNextInt(&cp, 0);
  From->IsAt = GetNextInt(&cp, 0);
  From->Turn = GetNextInt(&cp, 0);
  From->Flags = GetNextInt(&cp, 0);
  for (i = 0; i < NumGun; i++) {
    From->Guns[i].Carried = GetNextInt(&cp, 0);
  }
  for (i = 0; i < NumDrug; i++) {
    From->Drugs[i].Carried = GetNextInt(&cp, 0);
  }
  if (HaveAbility(Play, A_DRUGVALUE)) {
    for (i = 0; i < NumDrug; i++) {
      From->Drugs[i].TotalValue = GetNextPrice(&cp, (price_t)0);
    }
  }
  From->Bitches.Carried = GetNextInt(&cp, 0);
}

gchar *GetNextWord(gchar **Data, gchar *Default)
{
  gchar *Word;

  if (*Data == NULL || **Data == '\0')
    return Default;
  Word = *Data;
  while (**Data != '\0' && **Data != '^')
    (*Data)++;
  if (**Data != '\0') {
    **Data = '\0';
    (*Data)++;
  }
  return Word;
}

void AssignNextWord(gchar **Data, gchar **Dest)
{
  if (!Dest)
    return;
  g_free(*Dest);
  *Dest = g_strdup(GetNextWord(Data, ""));
}

int GetNextInt(gchar **Data, int Default)
{
  gchar *Word = GetNextWord(Data, NULL);

  if (Word)
    return atoi(Word);
  else
    return Default;
}

price_t GetNextPrice(gchar **Data, price_t Default)
{
  gchar *Word = GetNextWord(Data, NULL);

  if (Word)
    return strtoprice(Word);
  else
    return Default;
}

/* 
 * Called when the client is pushed off the server, or the server
 * terminates. Using the client information, starts a local server
 * to reproduce the current game situation as best as possible so
 * that the game can be continued in single player mode.
 */
void SwitchToSinglePlayer(Player *Play)
{
  if (Network && Client && FirstClient) {
    Player *NewPlayer;

    ShutdownNetwork(Play);
    CleanUpServer();
    Network = Server = Client = FALSE;
    InitAbilities(Play);
    NewPlayer = g_new(Player, 1);
    FirstServer = AddPlayer(0, NewPlayer, FirstServer);
    CopyPlayer(NewPlayer, Play);
    NewPlayer->Flags = 0;
    NewPlayer->EventNum = E_ARRIVE;
    SendEvent(NewPlayer);
  }
}

/* 
 * Closes down the client side of the network connection. Clears the list
 * of client players (with the exception of "you", the player "Play"),
 * and closes the network socket.
 */
void ShutdownNetwork(Player *Play)
{
  if (Play != FirstClient->data) {
    g_error("Oops! FirstClient should be player!");
  }
  while (g_slist_next(FirstClient)) {
    FirstClient = RemovePlayer((Player *)g_slist_next(FirstClient)->data,
                               FirstClient);
  }
#ifdef NETWORKING
  ShutdownNetworkBuffer(&Play->NetBuf);
#endif
  Client = Network = Server = FALSE;
}

/* 
 * Given a "raw" message in "Msg" and a pointer to the start of the linked
 * list of known players in "First", sets the other arguments to the message
 * fields. Data is returned as a pointer into the message "Msg", and should
 * therefore NOT be g_free'd. "Play" is a pointer to the player which is
 * receiving the message. "Other" is the player that is identified by the
 * message; for messages to clients, this will be the player "From" which
 * the message claims to be, while for messages to servers, this will be
 * the player "To" which to send messages. Returns 0 on success, -1 on failure.
 */
int ProcessMessage(char *Msg, Player *Play, Player **Other, AICode *AI,
                   MsgCode *Code, char **Data, GSList *First)
{
  gchar *pt, *buf;
  guint ID;

  if (!First || !Play)
    return -1;

  *AI = C_NONE;
  *Code = C_PRINTMESSAGE;
  *Other = &Noone;
  pt = Msg;
  if (HaveAbility(Play, A_PLAYERID)) {
    buf = GetNextWord(&pt, NULL);
    if (buf[0]) {
      ID = atoi(buf);
      *Other = GetPlayerByID(ID, First);
    }
  } else {
    buf = GetNextWord(&pt, NULL);
    if (Client)
      *Other = GetPlayerByName(buf, First);
    buf = GetNextWord(&pt, NULL);
    if (Server)
      *Other = GetPlayerByName(buf, First);
  }
  if (!(*Other))
    return -1;

  if (strlen(pt) >= 2) {
    *AI = pt[0];
    *Code = pt[1];
    *Data = &pt[2];
    return 0;
  }
  return -1;
}

/* 
 * Decodes the message data "text" into a list of drug prices for
 * player "To"
 */
void ReceiveDrugsHere(char *text, Player *To)
{
  char *cp;
  int i;

  To->EventNum = E_ARRIVE;
  cp = text;
  for (i = 0; i < NumDrug; i++) {
    To->Drugs[i].Price = GetNextPrice(&cp, (price_t)0);
  }
}

/* 
 * Handles messages that both human clients and AI players deal with
 * in the same way.
 */
gboolean HandleGenericClientMessage(Player *From, AICode AI, MsgCode Code,
                                    Player *To, char *Data,
                                    DispMode *DisplayMode)
{
  Player *tmp;
  gchar *pt;

  switch (Code) {
  case C_LIST:
  case C_JOIN:
    tmp = g_new(Player, 1);

    FirstClient = AddPlayer(0, tmp, FirstClient);
    pt = Data;
    SetPlayerName(tmp, GetNextWord(&pt, NULL));
    if (HaveAbility(To, A_PLAYERID))
      tmp->ID = GetNextInt(&pt, 0);
    break;
  case C_DATA:
    ReceiveMiscData(Data);
    break;
  case C_INIT:
    ReceiveInitialData(To, Data);
    break;
  case C_ABILITIES:
    ReceiveAbilities(To, Data);
    CombineAbilities(To);
    break;
  case C_LEAVE:
    if (From != &Noone)
      FirstClient = RemovePlayer(From, FirstClient);
    break;
  case C_TRADE:
    if (DisplayMode)
      *DisplayMode = DM_DEAL;
    break;
  case C_DRUGHERE:
    ReceiveDrugsHere(Data, To);
    if (DisplayMode)
      *DisplayMode = DM_STREET;
    break;
  case C_FIGHTPRINT:
    if (From != &Noone) {
      From->Flags |= FIGHTING;
      To->Flags |= CANSHOOT;
    }
    if (DisplayMode)
      *DisplayMode = DM_FIGHT;
    break;
  case C_CHANGEDISP:
    if (DisplayMode) {
      if (Data[0] == 'N' && *DisplayMode == DM_STREET)
        *DisplayMode = DM_NONE;
      if (Data[0] == 'Y' && *DisplayMode == DM_NONE)
        *DisplayMode = DM_STREET;
    }
    break;
  default:
    return FALSE;
    break;
  }
  return TRUE;
}

void SendFightReload(Player *To)
{
  SendFightMessage(To, NULL, 0, F_RELOAD, (price_t)0, FALSE, NULL);
}

void SendOldCanFireMessage(Player *To, GString *text)
{
  if (To->EventNum == E_FIGHT) {
    To->EventNum = E_FIGHTASK;
    if (CanRunHere(To) && To->Health > 0 && !HaveAbility(To, A_NEWFIGHT)) {
      if (text->len > 0)
        g_string_append_c(text, '^');
      if (TotalGunsCarried(To) == 0) {
        g_string_prepend(text, "YN^");
        g_string_append(text, _("Do you run?"));
      } else {
        g_string_prepend(text, "RF^");
        g_string_append(text, _("Do you run, or fight?"));
      }
      SendQuestion(NULL, C_NONE, To, text->str);
    } else {
      SendOldFightPrint(To, text, FALSE);
    }
  }
}

void SendOldFightPrint(Player *To, GString *text, gboolean FightOver)
{
  gboolean Fighting, CanShoot;

  Fighting = !FightOver;
  CanShoot = CanPlayerFire(To);

  To->Flags &= ~(CANSHOOT + FIGHTING);
  if (Fighting)
    To->Flags |= FIGHTING;
  if (Fighting && CanShoot)
    To->Flags |= CANSHOOT;
  SendPlayerData(To);
  To->Flags &= ~(CANSHOOT + FIGHTING);

  SendServerMessage(NULL, C_NONE, C_FIGHTPRINT, To, text->str);
}

void SendFightLeave(Player *Play, gboolean FightOver)
{
  SendFightMessage(Play, NULL, 0, FightOver ? F_LASTLEAVE : F_LEAVE,
                   (price_t)0, TRUE, NULL);
}

void ReceiveFightMessage(gchar *Data, gchar **AttackName,
                         gchar **DefendName, int *DefendHealth,
                         int *DefendBitches, gchar **BitchName,
                         int *BitchesKilled, int *ArmPercent,
                         FightPoint *fp, gboolean *CanRunHere,
                         gboolean *Loot, gboolean *CanFire,
                         gchar **Message)
{
  gchar *pt, *Flags;

  pt = Data;
  *AttackName = GetNextWord(&pt, "");
  *DefendName = GetNextWord(&pt, "");
  *DefendHealth = GetNextInt(&pt, 0);
  *DefendBitches = GetNextInt(&pt, 0);
  *BitchName = GetNextWord(&pt, "");
  *BitchesKilled = GetNextInt(&pt, 0);
  *ArmPercent = GetNextInt(&pt, 0);

  Flags = GetNextWord(&pt, NULL);
  if (Flags && strlen(Flags) >= 4) {
    *fp = Flags[0];
    *CanRunHere = (Flags[1] == '1');
    *Loot = (Flags[2] == '1');
    *CanFire = (Flags[3] == '1');
  } else {
    *fp = F_MSG;
    *CanRunHere = *Loot = *CanFire = FALSE;
  }
  *Message = pt;
}

void SendFightMessage(Player *Attacker, Player *Defender,
                      int BitchesKilled, FightPoint fp,
                      price_t Loot, gboolean Broadcast, gchar *Msg)
{
  guint ArrayInd;
  int ArmPercent, Damage, MaxDamage, i;
  Player *To;
  GString *text;
  gchar *BitchName;

  if (!Attacker->FightArray)
    return;

  MaxDamage = Damage = 0;
  for (i = 0; i < NumGun; i++) {
    if (Gun[i].Damage > MaxDamage)
      MaxDamage = Gun[i].Damage;
    Damage += Gun[i].Damage * Attacker->Guns[i].Carried;
  }
  MaxDamage *= (Attacker->Bitches.Carried + 2);
  ArmPercent = Damage * 100 / MaxDamage;

  text = g_string_new("");

  for (ArrayInd = 0; ArrayInd < Attacker->FightArray->len; ArrayInd++) {
    To = (Player *)g_ptr_array_index(Attacker->FightArray, ArrayInd);
    if (!Broadcast && To != Attacker)
      continue;
    g_string_truncate(text, 0);
    if (HaveAbility(To, A_NEWFIGHT)) {
      if (Defender) {
        if (IsCop(Defender)) {
          if (Defender->Bitches.Carried == 1) {
            BitchName = Cop[Defender->CopIndex - 1].DeputyName;
          } else {
            BitchName = Cop[Defender->CopIndex - 1].DeputiesName;
          }
        } else {
          if (Defender->Bitches.Carried == 1) {
            BitchName = Names.Bitch;
          } else {
            BitchName = Names.Bitches;
          }
        }
      } else
        BitchName = "";
      g_string_sprintf(text, "%s^%s^%d^%d^%s^%d^%d^%c%c%c%c^",
                       Attacker == To ? "" : GetPlayerName(Attacker),
                       (Defender == To || Defender == NULL)
                       ? "" : GetPlayerName(Defender),
                       Defender ? Defender->Health : 0,
                       Defender ? Defender->Bitches.Carried : 0,
                       BitchName,
                       BitchesKilled, ArmPercent,
                       fp, CanRunHere(To) ? '1' : '0',
                       Loot ? '1' : '0',
                       fp != F_ARRIVED && fp != F_LASTLEAVE &&
                       CanPlayerFire(To) ? '1' : '0');
    }
    if (Msg) {
      g_string_append(text, Msg);
    } else {
      FormatFightMessage(To, text, Attacker, Defender, BitchesKilled,
                         ArmPercent, fp, Loot);
    }
    if (HaveAbility(To, A_NEWFIGHT)) {
      SendServerMessage(NULL, C_NONE, C_FIGHTPRINT, To, text->str);
    } else if (CanRunHere(To)) {
      if (fp != F_ARRIVED && fp != F_MSG &&
          fp != F_LASTLEAVE &&
          (fp != F_LEAVE || Attacker != To) &&
          CanPlayerFire(To) && To->EventNum == E_FIGHT) {
        SendOldCanFireMessage(To, text);
      } else if (text->len > 0)
        SendPrintMessage(NULL, C_NONE, To, text->str);
    } else {
      SendOldFightPrint(To, text, fp == F_LASTLEAVE);
    }
  }
  g_string_free(text, TRUE);
}

void FormatFightMessage(Player *To, GString *text, Player *Attacker,
                        Player *Defender, int BitchesKilled,
                        int ArmPercent, FightPoint fp, price_t Loot)
{
  gchar *Armament, *DefendName, *AttackName;
  int Health, Bitches;
  gchar *BitchName, *BitchesName;

  if (Defender && IsCop(Defender)) {
    BitchName = Cop[Defender->CopIndex - 1].DeputyName;
    BitchesName = Cop[Defender->CopIndex - 1].DeputiesName;
  } else {
    BitchName = Names.Bitch;
    BitchesName = Names.Bitches;
  }

  AttackName = (!Attacker
                || Attacker == To ? "" : GetPlayerName(Attacker));
  DefendName = (!Defender
                || Defender == To ? "" : GetPlayerName(Defender));
  Health = Defender ? Defender->Health : 0;
  Bitches = Defender ? Defender->Bitches.Carried : 0;

  switch (fp) {
  case F_ARRIVED:
    Armament = ArmPercent < 10 ? _("pitifully armed") :
        ArmPercent < 25 ? _("lightly armed") :
        ArmPercent < 60 ? _("moderately well armed") :
        ArmPercent < 80 ? _("heavily armed") : _("armed to the teeth");
    if (DefendName[0]) {
      if (IsCop(Defender) && !AttackName[0]) {
        if (Bitches == 0) {
          dpg_string_sprintfa(text, _("%s - %s - is chasing you, man!"),
                              DefendName, Armament);
        } else {
          dpg_string_sprintfa(text,
                              _("%s and %d %tde - %s - are chasing you, man!"),
                              DefendName, Bitches, BitchesName, Armament);
        }
      } else {
        dpg_string_sprintfa(text, _("%s arrives with %d %tde, %s!"),
                            DefendName, Bitches, BitchesName, Armament);
      }
    }
    break;
  case F_STAND:
    if (AttackName[0]) {
      g_string_sprintfa(text, _("%s stands and takes it"), AttackName);
    } else {
      g_string_append(text, _("You stand there like a dummy."));
    }
    break;
  case F_FAILFLEE:
    if (AttackName[0]) {
      g_string_sprintfa(text, _("%s tries to get away, but fails."),
                        AttackName);
    } else {
      g_string_append(text, _("Panic! You can't get away!"));
    }
    break;
  case F_LEAVE:
  case F_LASTLEAVE:
    if (Attacker->Health > 0) {
      if (AttackName[0]) {
        if (!IsCop(Attacker) && brandom(0, 100) < 70
            && Attacker->IsAt >= 0) {
          g_string_sprintfa(text, _("%s has got away to %s!"), AttackName,
                            Location[Attacker->IsAt].Name);
        } else {
          g_string_sprintfa(text, _("%s has got away!"), AttackName);
        }
      } else {
        g_string_sprintfa(text, _("You got away!"));
      }
    }
    break;
  case F_RELOAD:
    if (!AttackName[0]) {
      g_string_append(text, _("Guns reloaded..."));
    }
    break;
  case F_MISS:
    if (AttackName[0] && DefendName[0]) {
      g_string_sprintfa(text, _("%s shoots at %s... and misses!"),
                        AttackName, DefendName);
    } else if (AttackName[0]) {
      g_string_sprintfa(text, _("%s shoots at you... and misses!"),
                        AttackName);
    } else if (DefendName[0]) {
      g_string_sprintfa(text, _("You missed %s!"), DefendName);
    }
    break;
  case F_HIT:
    if (AttackName[0] && DefendName[0]) {
      if (Health == 0 && Bitches == 0) {
        g_string_sprintfa(text, _("%s shoots %s dead."),
                          AttackName, DefendName);
      } else if (BitchesKilled) {
        dpg_string_sprintfa(text, _("%s shoots at %s and kills a %tde!"),
                            AttackName, DefendName, BitchName);
      } else {
        g_string_sprintfa(text, _("%s shoots at %s."),
                          AttackName, DefendName);
      }
    } else if (AttackName[0]) {
      if (Health == 0 && Bitches == 0) {
        g_string_sprintfa(text, _("%s wasted you, man! What a drag!"),
                          AttackName);
      } else if (BitchesKilled) {
        dpg_string_sprintfa(text,
                            _("%s shoots at you... and kills a %tde!"),
                            AttackName, BitchName);
      } else {
        g_string_sprintfa(text, _("%s hits you, man!"), AttackName);
      }
    } else if (DefendName[0]) {
      if (Health == 0 && Bitches == 0) {
        g_string_sprintfa(text, _("You killed %s!"), DefendName);
      } else if (BitchesKilled) {
        dpg_string_sprintfa(text, _("You hit %s, and killed a %tde!"),
                            DefendName, BitchName);
      } else {
        g_string_sprintfa(text, _("You hit %s!"), DefendName);
      }
      if (Loot > 0) {
        dpg_string_sprintfa(text, _(" You find %P on the body!"), Loot);
      } else if (Loot < 0) {
        g_string_append(text, _(" You loot the body!"));
      }
    }
    break;
  case F_MSG:
    break;
  }
}
