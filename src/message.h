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
#define F_FAILFLEE     'F'
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
/* Keeps track of the progress of an HTTP connection */
typedef enum _HttpStatus {
   HS_CONNECTING,HS_READHEADERS,HS_READSEPARATOR,HS_READBODY
} HttpStatus;

/* A structure used to keep track of an HTTP connection */
typedef struct _HttpConnection {
   gchar *HostName;       /* The machine on which the desired page resides */
   unsigned Port;         /* The port */
   gchar *Proxy;          /* If non-NULL, a web proxy to use */
   unsigned ProxyPort;    /* The port to use for talking to the proxy */
   gchar *Method;         /* e.g. GET, POST */
   gchar *Query;          /* e.g. the path of the desired webpage */
   gchar *Headers;        /* if non-NULL, e.g. Content-Type */
   gchar *Body;           /* if non-NULL, data to send */
   NetworkBuffer NetBuf;  /* The actual network connection itself */
   gint Tries;            /* Number of requests actually sent so far */
   gint StatusCode;       /* 0=no status yet, otherwise an HTTP status code */
   HttpStatus Status;
} HttpConnection;

char *StartConnect(int *fd,gchar *RemoteHost,unsigned RemotePort,
                   gboolean NonBlocking);
char *FinishConnect(int fd);

void InitNetworkBuffer(NetworkBuffer *NetBuf,char Terminator,char StripChar);
gboolean IsNetworkBufferActive(NetworkBuffer *NetBuf);
void BindNetworkBufferToSocket(NetworkBuffer *NetBuf,int fd);
gboolean StartNetworkBufferConnect(NetworkBuffer *NetBuf,gchar *RemoteHost,
                                   unsigned RemotePort);
void ShutdownNetworkBuffer(NetworkBuffer *NetBuf);
void SetSelectForNetworkBuffer(NetworkBuffer *NetBuf,fd_set *readfds,
                               fd_set *writefds,fd_set *errorfds,int *MaxSock);
gboolean RespondToSelect(NetworkBuffer *NetBuf,fd_set *readfds,
                         fd_set *writefds,fd_set *errorfds,
                         gboolean *DoneOK);
gboolean NetBufHandleNetwork(NetworkBuffer *NetBuf,gboolean ReadReady,
                             gboolean WriteReady,gboolean *DoneOK);
gboolean PlayerHandleNetwork(Player *Play,gboolean ReadReady,
                             gboolean WriteReady,gboolean *DoneOK);
gboolean ReadPlayerDataFromWire(Player *Play);
void QueuePlayerMessageForSend(Player *Play,gchar *data);
gboolean WritePlayerDataToWire(Player *Play);
gchar *GetWaitingPlayerMessage(Player *Play);

gboolean ReadDataFromWire(NetworkBuffer *NetBuf);
gboolean WriteDataToWire(NetworkBuffer *NetBuf);
void QueueMessageForSend(NetworkBuffer *NetBuf,gchar *data);
gint CountWaitingMessages(NetworkBuffer *NetBuf);
gchar *GetWaitingMessage(NetworkBuffer *NetBuf);

HttpConnection *OpenHttpConnection(gchar *HostName,unsigned Port,
                                   gchar *Proxy,unsigned ProxyPort,
                                   gchar *Method,gchar *Query,
                                   gchar *Headers,gchar *Body);
HttpConnection *OpenMetaHttpConnection(void);
void CloseHttpConnection(HttpConnection *conn);
gchar *ReadHttpResponse(HttpConnection *conn);
gboolean HandleWaitingMetaServerData(HttpConnection *conn);

gchar *bgets(int fd);
#endif /* NETWORKING */

extern GSList *FirstClient;

extern void (*ClientMessageHandlerPt) (char *,Player *);
extern void (*SocketWriteTestPt) (Player *,gboolean);

void AddURLEnc(GString *str,gchar *unenc);
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
char *FinishSetupNetwork(void);
void ShutdownNetwork(void);
void SwitchToSinglePlayer(Player *Play);
int ProcessMessage(char *Msg,Player *Play,Player **Other,char *AICode,
                   char *Code,char **Data,GSList *First);
void ReceiveDrugsHere(char *text,Player *To);
gboolean HandleGenericClientMessage(Player *From,char AICode,char Code,
                               Player *To,char *Data,char *DisplayMode);
#ifdef NETWORKING
char *OpenMetaServerConnection(int *HttpSock);
void CloseMetaServerConnection(int HttpSock);
void ClearServerList(void);
void ReadMetaServerData(int HttpSock);
#endif
void InitAbilities(Player *Play);
void SendAbilities(Player *Play);
void ReceiveAbilities(Player *Play,gchar *Data);
void CombineAbilities(Player *Play);
void SetAbility(Player *Play,gint Type,gboolean Set);
gboolean HaveAbility(Player *Play,gint Type);
void SendFightReload(Player *To);
void SendOldCanFireMessage(Player *To,GString *text);
void SendOldFightPrint(Player *To,GString *text,gboolean FightOver);
void SendFightLeave(Player *Play,gboolean FightOver);
void ReceiveFightMessage(gchar *Data,gchar **AttackName,gchar **DefendName,
                         int *DefendHealth,int *DefendBitches,
                         gchar **BitchName,
                         int *BitchesKilled,int *ArmPercent,
                         gchar *FightPoint,gboolean *CanRunHere,
                         gboolean *Loot,gboolean *CanFire,gchar **Message);
void SendFightMessage(Player *Attacker,Player *Defender,
                      int BitchesKilled,gchar FightPoint,
                      price_t Loot,gboolean Broadcast,gchar *Msg);
void FormatFightMessage(Player *To,GString *text,Player *Attacker,
                        Player *Defender,int BitchesKilled,int ArmPercent,
                        gchar FightPoint,price_t Loot);
#endif
