/* dopewars.h  Common structures and stuff for Dopewars                 */
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


#ifndef __DOPEWARS_H__
#define __DOPEWARS_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

/* Be careful not to include both sys/time.h and time.h on those systems */
/* which don't like it */
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#include <glib.h>
#include "dopeos.h"

/* Internationalization stuff */

#ifdef ENABLE_NLS
#include <locale.h>
#include <libintl.h>
#define _(String) gettext (String)
#ifdef gettext_noop
#define N_(String) gettext_noop (String)
#else
#define N_(String) (String)
#endif
#else
#define gettext(String) (String)
#define dgettext(Domain,Message) (Message)
#define dcgettext(Domain,Message,Type) (Message)
#define _(String) (String)
#define N_(String) (String)
#endif

/* Make price_t be a long long if the type is supported by the compiler */
#if SIZEOF_LONG_LONG == 0
typedef long price_t;
#else
typedef long long price_t;
#endif

#define A_PLAYERID      0
#define A_DRUGVALUE     1
#define A_NEWFIGHT      2
#define A_NUM           3
typedef struct ABILITIES {
   gboolean Local[A_NUM];
   gboolean Remote[A_NUM];
   gboolean Shared[A_NUM];
} Abilities;

struct COPS {
   int EscapeProb,DeputyEscape,HitProb,DeputyHit,Damage,
       Toughness,DropProb;
};

struct NAMES {
   gchar *Bitch,*Bitches,*Gun,*Guns,*Drug,*Drugs,*Month,*Year,
         *Officer,*ReserveOfficer,*LoanSharkName,*BankName,
         *GunShopName,*RoughPubName;
};

struct METASERVER {
   int Active;
   int HttpPort,UdpPort;
   gchar *Name,*Path,*LocalName,*Password,*Comment;
};

struct PRICES {
   price_t Spy,Tipoff;
};

struct BITCH {
   price_t MinPrice,MaxPrice;
};

#define CLIENT_AUTO   0
#define CLIENT_WINDOW 1
#define CLIENT_CURSES 2

extern int ClientSock,ListenSock;
extern char Network,Client,Server,NotifyMetaServer,AIPlayer;
extern int Port,Sanitized,DrugValue;
extern int NumLocation,NumGun,NumCop,NumDrug,NumSubway,NumPlaying,NumStoppedTo;
extern gchar *HiScoreFile,*ServerName,*Pager;
extern char WantHelp,WantVersion,WantAntique,WantColour,WantNetwork;
extern char WantedClient;
extern int LoanSharkLoc,BankLoc,GunShopLoc,RoughPubLoc;
extern int DrugSortMethod,FightTimeout,IdleTimeout,ConnectTimeout;
extern int MaxClients,AITurnPause;
extern struct PRICES Prices;
extern struct BITCH Bitch;
extern price_t StartCash,StartDebt;
extern struct COPS Cops;
extern struct NAMES Names;
extern struct METASERVER MetaServer;
extern int NumTurns;

#define DM_NONE       0
#define DM_STREET     1
#define DM_FIGHT      2
#define DM_DEAL       3

#define DS_ATOZ       1
#define DS_ZTOA       2
#define DS_CHEAPFIRST 3
#define DS_CHEAPLAST  4
#define DS_MAX        5

#define NUMSUBWAY    31
#define NUMHISCORE   18
#define NUMSTOPPEDTO 5
#define NUMPLAYING   18
#define NUMDISCOVER  3

#define NUMDRUG      12
#define NUMGUN       4
#define NUMCOP       2
#define NUMLOCATION  8

#define ESCAPE  0
#define DEFECT  1
#define SHOT    2

#define MINTRENCHPRICE 200
#define MAXTRENCHPRICE 300

#define ACID    0
#define COCAINE 1
#define HASHISH 2
#define HEROIN  3
#define LUDES   4
#define MDA     5
#define OPIUM   6
#define PCP     7
#define PEYOTE  8
#define SHROOMS 9
#define SPEED   10 
#define WEED    11

#define DEFLOANSHARK 1
#define DEFBANK      1
#define DEFGUNSHOP   2
#define DEFROUGHPUB  2

#define METAVERSION 2

#define FIRSTTURN   1
#define DEADHARDASS 2
#define TIPPEDOFF   4
#define SPIEDON     8
#define SPYINGON    16
#define FIGHTING    32
#define CANSHOOT    64
#define TRADING     128

#define LISTONLY    0
#define PAGETO      1
#define SELECTTIP   2
#define SELECTSPY   3

#define E_NONE       0
#define E_SUBWAY     1
#define E_OFFOBJECT  2
#define E_WEED       3
#define E_SAYING     4
#define E_LOANSHARK  5
#define E_BANK       6
#define E_GUNSHOP    7
#define E_ROUGHPUB   8
#define E_HIREBITCH  9
#define E_ARRIVE     10
#define E_MAX        11

#define E_FINISH     100

#define E_OUTOFSYNC  120
#define E_FIGHT      121
#define E_FIGHTASK   122
#define E_DOCTOR     123
#define E_MAXOOS     124

struct COP {
   gchar *Name,*DeputyName,*DeputiesName;
};
extern struct COP DefaultCop[NUMCOP],*Cop;

struct GUN {
   gchar *Name;
   price_t Price;
   int Space;
   int Damage;
};
extern struct GUN DefaultGun[NUMGUN],*Gun;

struct HISCORE {
   gchar *Time;
   price_t Money;
   char Dead;
   gchar *Name;
};

struct LOCATION {
   gchar *Name;
   int PolicePresence;
   int MinDrug,MaxDrug;
};
extern struct LOCATION DefaultLocation[NUMLOCATION],*Location;

struct DRUG {
   gchar *Name;
   price_t MinPrice,MaxPrice;
   int Cheap,Expensive;
   gchar *CheapStr;
};
extern struct DRUG DefaultDrug[NUMDRUG],*Drug;

struct DRUGS {
   gchar *ExpensiveStr1,*ExpensiveStr2;
   int CheapDivide,ExpensiveMultiply;
};
extern struct DRUGS Drugs;

struct INVENTORY {
   price_t Price,TotalValue;
   int Carried;
};
typedef struct INVENTORY Inventory;

struct PLAYER_T;
typedef struct PLAYER_T Player;

struct TDopeEntry {
   Player *Play;
   int Turns;
};
typedef struct TDopeEntry DopeEntry;

struct TDopeList {
   DopeEntry *Data;
   int Number;
};
typedef struct TDopeList DopeList;

typedef struct tagConnBuf {
   gchar *Data;     /* bytes waiting to be read/written      */
   int Length;      /* allocated length of the "Data" buffer */
   int DataPresent; /* number of bytes currently in "Data"   */
} ConnBuf;            

struct PLAYER_T {
   guint ID;
   int Turn;
   price_t Cash,Debt,Bank;
   int Health;
   int CoatSize;
   char IsAt;
   char Flags;
   gchar *Name;
   Inventory *Guns,*Drugs,Bitches;
   int fd;
   int EventNum,ResyncNum;
   time_t FightTimeout,IdleTimeout,ConnectTimeout;
   price_t DocPrice;
   DopeList SpyList,TipList;
   Player *OnBehalfOf;
   ConnBuf ReadBuf,WriteBuf;
   Abilities Abil;
   GPtrArray *FightArray;
   gint IsCop;
};

#define CM_SERVER 0
#define CM_PROMPT 1
#define CM_META   2
#define CM_SINGLE 3
typedef struct tag_serverdata {
   char *Name;
   int Port;
   int MaxPlayers,CurPlayers;
   char *Comment,*Version,*Update,*UpSince;
} ServerData;

#define NUMGLOB 80
struct GLOBALS {
   int *IntVal;
   price_t *PriceVal;
   gchar **StringVal;
   gchar ***StringList;
   char *Name,*Help;

   void **StructListPt,*StructStaticPt;
   int LenStruct;
   char *NameStruct;
   int *MaxIndex;
   void (*ResizeFunc)(int NewNum);
};

extern struct GLOBALS Globals[NUMGLOB];
extern Player Noone;
extern char *Discover[NUMDISCOVER];
extern char **Playing;
extern char **SubwaySaying;
extern char **StoppedTo;
extern GSList *ServerList;
extern GScannerConfig ScannerConfig;

GSList *RemovePlayer(Player *Play,GSList *First);
Player *GetPlayerByID(guint ID,GSList *First);
Player *GetPlayerByName(gchar *Name,GSList *First);
int CountPlayers(GSList *First);
GSList *AddPlayer(int fd,Player *NewPlayer,GSList *First);
void UpdatePlayer(Player *Play);
void CopyPlayer(Player *Dest,Player *Src);
int MaxHealth(Player *Play,int NumBitches);
void ClearInventory(Inventory *Guns,Inventory *Drugs);
int IsCarryingRandom(Player *Play,int amount);
void ChangeSpaceForInventory(Inventory *Guns,Inventory *Drugs,
                             Player *Play);
void InitList(DopeList *List);
void AddListEntry(DopeList *List,DopeEntry *NewEntry);
void RemoveListEntry(DopeList *List,int Entry);
int GetListEntry(DopeList *List,Player *Play);
void RemoveListPlayer(DopeList *List,Player *Play);
void RemoveAllEntries(DopeList *List,Player *Play);
void ClearList(DopeList *List);
int TotalGunsCarried(Player *Play);
int read_string(FILE *fp,char **buf);
int brandom(int bot,int top);
void AddInventory(Inventory *Cumul,Inventory *Add,int Length);
void TruncateInventoryFor(Inventory *Guns,Inventory *Drugs,
                          Player *Play);
void PrintInventory(Inventory *Guns,Inventory *Drugs);
price_t strtoprice(char *buf);
gchar *pricetostr(price_t price);
gchar *FormatPrice(price_t price);
char IsInventoryClear(Inventory *Guns,Inventory *Drugs);
void ResizeLocations(int NewNum);
void ResizeCops(int NewNum);
void ResizeGuns(int NewNum);
void ResizeDrugs(int NewNum);
void ResizeSubway(int NewNum);
void ResizePlaying(int NewNum);
void ResizeStoppedTo(int NewNum);
void AssignName(gchar **dest,gchar *src);
void CopyNames(struct NAMES *dest,struct NAMES *src);
void CopyMetaServer(struct METASERVER *dest,struct METASERVER *src);
void CopyLocation(struct LOCATION *dest,struct LOCATION *src);
void CopyCop(struct COP *dest,struct COP *src);
void CopyGun(struct GUN *dest,struct GUN *src);
void CopyDrug(struct DRUG *dest,struct DRUG *src);
void CopyDrugs(struct DRUGS *dest,struct DRUGS *src);
int GetNextDrugIndex(int OldIndex,Player *Play);
gchar *InitialCaps(gchar *string);
char StartsWithVowel(char *string);
char *GetPlayerName(Player *Play);
void SetPlayerName(Player *Play,char *Name);
void HandleCmdLine(int argc,char *argv[]);
void SetupParameters();
void HandleHelpTexts();
void ReadConfigFile(char *FileName);
gboolean ParseNextConfig(GScanner *scanner);
int GetGlobalIndex(gchar *ID1,gchar *ID2);
void *GetGlobalPointer(int GlobalIndex,int StructIndex);
void PrintConfigValue(int GlobalIndex,int StructIndex,gboolean IndexGiven,
                      GScanner *scanner);
void SetConfigValue(int GlobalIndex,int StructIndex,gboolean IndexGiven,
                    GScanner *scanner);
#endif
