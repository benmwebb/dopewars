/************************************************************************
 * message.h      Header file for dopewars message-handling routines    *
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

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include "error.h"
#include "dopewars.h"
#include "network.h"

typedef enum {
  C_PRINTMESSAGE = 'A',
  C_LIST, C_ENDLIST, C_NEWNAME, C_MSG, C_MSGTO, C_JOIN, C_LEAVE,
  C_SUBWAYFLASH, C_UPDATE, C_DRUGHERE, C_GUNSHOP, C_LOANSHARK,
  C_BANK, C_QUESTION, C_UNUSED, C_HISCORE, C_STARTHISCORE, C_ENDHISCORE,
  C_BUYOBJECT, C_DONE, C_REQUESTJET, C_PAYLOAN, C_ANSWER, C_DEPOSIT, C_PUSH,
  C_QUIT = 'a',
  C_RENAME, C_NAME, C_SACKBITCH, C_TIPOFF, C_SPYON, C_WANTQUIT,
  C_CONTACTSPY, C_KILL, C_REQUESTSCORE, C_INIT, C_DATA,
  C_FIGHTPRINT, C_FIGHTACT, C_TRADE, C_CHANGEDISP,
  C_NETMESSAGE, C_ABILITIES
} MsgCode;

typedef enum {
  C_NONE = 'A',
  C_ASKLOAN, C_COPSMESG, C_ASKBITCH, C_ASKGUN, C_ASKGUNSHOP,
  C_ASKPUB, C_ASKBANK, C_ASKRUN, C_ASKRUNFIGHT, C_ASKSEW,
  C_MEETPLAYER, C_FIGHT, C_FIGHTDONE, C_MOTD
} AICode;

#define DT_LOCATION    'A'
#define DT_DRUG        'B'
#define DT_GUN         'C'
#define DT_PRICES      'D'

typedef enum {
  F_ARRIVED = 'A', F_STAND = 'S', F_HIT = 'H',
  F_MISS = 'M', F_RELOAD = 'R', F_LEAVE = 'L',
  F_LASTLEAVE = 'D', F_FAILFLEE = 'F', F_MSG = 'G'
} FightPoint;

void SendClientMessage(Player *From, AICode AI, MsgCode Code,
                       Player *To, char *Data);
void SendNullClientMessage(Player *From, AICode AI, MsgCode Code,
                           Player *To, char *Data);
void DoSendClientMessage(Player *From, AICode AI, MsgCode Code,
                         Player *To, char *Data, Player *BufOwn);
void SendServerMessage(Player *From, AICode AI, MsgCode Code,
                       Player *To, char *Data);
void SendPrintMessage(Player *From, AICode AI, Player *To, char *Data);
void SendQuestion(Player *From, AICode AI, Player *To, char *Data);

#if NETWORKING
gboolean PlayerHandleNetwork(Player *Play, gboolean ReadReady,
                             gboolean WriteReady, gboolean *DoneOK);
gboolean ReadPlayerDataFromWire(Player *Play);
void QueuePlayerMessageForSend(Player *Play, gchar *data);
gboolean WritePlayerDataToWire(Player *Play);
gchar *GetWaitingPlayerMessage(Player *Play);

gboolean OpenMetaHttpConnection(HttpConnection **conn);
gboolean HandleWaitingMetaServerData(HttpConnection *conn, GSList **listpt,
                                     gboolean *doneOK);
void ClearServerList(GSList **listpt);
#endif /* NETWORKING */

extern GSList *FirstClient;

extern void (*ClientMessageHandlerPt) (char *, Player *);

void AddURLEnc(GString *str, gchar *unenc);
void chomp(char *str);
void BroadcastToClients(AICode AI, MsgCode Code, char *Data, Player *From,
                        Player *Except);
void SendInventory(Player *From, AICode AI, MsgCode Code, Player *To,
                   Inventory *Guns, Inventory *Drugs);
void ReceiveInventory(char *Data, Inventory *Guns, Inventory *Drugs);
void SendPlayerData(Player *To);
void SendSpyReport(Player *To, Player *SpiedOn);
void ReceivePlayerData(Player *Play, char *text, Player *From);
void SendInitialData(Player *To);
void ReceiveInitialData(Player *Play, char *data);
void SendMiscData(Player *To);
void ReceiveMiscData(char *Data);
gchar *GetNextWord(gchar **Data, gchar *Default);
void AssignNextWord(gchar **Data, gchar **Dest);
int GetNextInt(gchar **Data, int Default);
price_t GetNextPrice(gchar **Data, price_t Default);
void ShutdownNetwork(Player *Play);
void SwitchToSinglePlayer(Player *Play);
int ProcessMessage(char *Msg, Player *Play, Player **Other, AICode *AI,
                   MsgCode *Code, char **Data, GSList *First);
void ReceiveDrugsHere(char *text, Player *To);
gboolean HandleGenericClientMessage(Player *From, AICode AI, MsgCode Code,
                                    Player *To, char *Data,
                                    DispMode *DisplayMode);
void InitAbilities(Player *Play);
void SendAbilities(Player *Play);
void ReceiveAbilities(Player *Play, gchar *Data);
void CombineAbilities(Player *Play);
void SetAbility(Player *Play, gint Type, gboolean Set);
gboolean HaveAbility(Player *Play, gint Type);
void SendFightReload(Player *To);
void SendOldCanFireMessage(Player *To, GString *text);
void SendOldFightPrint(Player *To, GString *text, gboolean FightOver);
void SendFightLeave(Player *Play, gboolean FightOver);
void ReceiveFightMessage(gchar *Data, gchar **AttackName,
                         gchar **DefendName, int *DefendHealth,
                         int *DefendBitches, gchar **BitchName,
                         int *BitchesKilled, int *ArmPercent,
                         FightPoint *fp, gboolean *CanRunHere,
                         gboolean *Loot, gboolean *CanFire,
                         gchar **Message);
void SendFightMessage(Player *Attacker, Player *Defender,
                      int BitchesKilled, FightPoint fp, price_t Loot,
                      gboolean Broadcast, gchar *Msg);
void FormatFightMessage(Player *To, GString *text, Player *Attacker,
                        Player *Defender, int BitchesKilled,
                        int ArmPercent, FightPoint fp, price_t Loot);

#endif /* __MESSAGE_H__ */
