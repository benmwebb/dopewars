/* message.h    Header file for Dopewars message-handling routines      */
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


#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include "dopewars.h"

#define C_PRINTMESSAGE 'A'
#define C_LIST         'B'
#define C_ENDLIST      'C'
#define C_NEWNAME      'D' 
#define C_MSG          'E'
#define C_MSGTO        'F'
#define C_JOIN         'G'
#define C_LEAVE        'H'
#define C_SUBWAYFLASH  'I'
#define C_UPDATE       'J'
#define C_DRUGHERE     'K'
#define C_GUNSHOP      'L'
#define C_LOANSHARK    'M'
#define C_BANK         'N'
#define C_QUESTION     'O'
#define C_HISCORE      'Q'
#define C_STARTHISCORE 'R' 
#define C_ENDHISCORE   'S'
#define C_BUYOBJECT    'T'
#define C_DONE         'U'
#define C_REQUESTJET   'V'
#define C_PAYLOAN      'W'
#define C_ANSWER       'X'
#define C_DEPOSIT      'Y'
#define C_PUSH         'Z'
#define C_QUIT         'a'
#define C_RENAME       'b'
#define C_NAME         'c'
#define C_SACKBITCH    'd'
#define C_TIPOFF       'e'
#define C_SPYON        'f'
#define C_WANTQUIT     'g'
#define C_CONTACTSPY   'h'
#define C_KILL         'i'
#define C_REQUESTSCORE 'j'
#define C_INIT         'k'
#define C_DATA         'l'
#define C_FIGHTPRINT   'm'
#define C_FIGHTACT     'n'
#define C_TRADE        'o'
#define C_CHANGEDISP   'p'
#define C_NETMESSAGE   'q'
#define C_ABILITIES    'r'

#define C_NONE        'A'
#define C_ASKLOAN     'B'
#define C_COPSMESG    'C'
#define C_ASKBITCH    'D'
#define C_ASKGUN      'E'
#define C_ASKGUNSHOP  'F'
#define C_ASKPUB      'G'
#define C_ASKBANK     'H'
#define C_ASKRUN      'I'
#define C_ASKRUNFIGHT 'J'
#define C_ASKSEW      'K'
#define C_MEETPLAYER  'L'
#define C_FIGHT       'M'
#define C_FIGHTDONE   'N'

#define DT_LOCATION    'A'
#define DT_DRUG        'B'
#define DT_GUN         'C'
#define DT_PRICES      'D'

#define F_ARRIVED      'A'
#define F_STAND        'S'
#define F_HIT          'H'
#define F_MISS         'M'
#define F_RELOAD       'R'
#define F_LEAVE        'L'
#define F_LASTLEAVE    'D'
#define F_MSG          'G'

void SendClientMessage(Player *From,char AICode,char Code,
                       Player *To,char *Data);
void SendNullClientMessage(Player *From,char AICode,char Code,
                           Player *To,char *Data);
void DoSendClientMessage(Player *From,char AICode,char Code,
                         Player *To,char *Data,Player *BufOwn);
void SendServerMessage(Player *From,char AICode,char Code,
                       Player *To,char *Data);
void SendPrintMessage(Player *From,char AICode,Player *To,char *Data);
void SendQuestion(Player *From,char AICode,Player *To,char *Data);

#if NETWORKING
gchar *ReadFromConnectionBuffer(Player *Play);
gboolean ReadConnectionBufferFromWire(Player *Play);
void WriteToConnectionBuffer(Player *Play,gchar *data);
gboolean WriteConnectionBufferToWire(Player *Play);
gchar *bgets(int fd);
#endif /* NETWORKING */

extern GSList *FirstClient;

extern void (*ClientMessageHandlerPt) (char *,Player *);
extern void (*SocketWriteTestPt) (Player *,gboolean);

void chomp(char *str);
void BroadcastToClients(char AICode,char Code,char *Data,Player *From,
                        Player *Except);
void SendInventory(Player *From,char AICode,char Code,Player *To,
                   Inventory *Guns,Inventory *Drugs);
void ReceiveInventory(char *Data,Inventory *Guns,Inventory *Drugs);
void SendPlayerData(Player *To);
void SendSpyReport(Player *To,Player *SpiedOn);
void ReceivePlayerData(Player *Play,char *text,Player *From);
void SendInitialData(Player *To);
void ReceiveInitialData(Player *Play,char *data);
void SendMiscData(Player *To);
void ReceiveMiscData(char *Data);
gchar *GetNextWord(gchar **Data,gchar *Default);
void AssignNextWord(gchar **Data,gchar **Dest);
int GetNextInt(gchar **Data,int Default);
price_t GetNextPrice(gchar **Data,price_t Default);
char *SetupNetwork(gboolean NonBlocking);
char *FinishSetupNetwork();
void ShutdownNetwork();
void SwitchToSinglePlayer(Player *Play);
int ProcessMessage(char *Msg,Player *Play,Player **Other,char *AICode,
                   char *Code,char **Data,GSList *First);
void ReceiveDrugsHere(char *text,Player *To);
gboolean HandleGenericClientMessage(Player *From,char AICode,char Code,
                               Player *To,char *Data,char *DisplayMode);
char *OpenMetaServerConnection(int *HttpSock);
void CloseMetaServerConnection(int HttpSock);
void ClearServerList();
void ReadMetaServerData(int HttpSock);
void InitAbilities(Player *Play);
void SendAbilities(Player *Play);
void ReceiveAbilities(Player *Play,gchar *Data);
void CombineAbilities(Player *Play);
gboolean HaveAbility(Player *Play,gint Type);
void SendFightReload(Player *To);
void SendOldCanFireMessage(Player *To,GString *text);
void SendOldFightPrint(Player *To,GString *text,gboolean FightOver);
void SendFightLeave(Player *Play,gboolean FightOver);
void ReceiveFightMessage(gchar *Data,gchar **AttackName,gchar **DefendName,
                         int *BitchesKilled,gchar *FightPoint,gboolean *Loot);
void SendFightMessage(Player *Attacker,Player *Defender,
                      int BitchesKilled,gchar FightPoint,
                      gboolean Loot,gboolean Broadcast,gchar *Msg);
void FormatFightMessage(Player *To,GString *text,
                        gchar *AttackName,gchar *DefendName,int Health,
                        int Bitches,int BitchesKilled,int ArmPercent,
                        gchar FightPoint,gboolean Loot);
#endif
