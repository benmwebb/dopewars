/************************************************************************
 * dopewars.h     Common structures and stuff for dopewars              *
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

#ifndef __DOPEWARS_H__
#define __DOPEWARS_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

/* Be careful not to include both sys/time.h and time.h on those systems
 * which don't like it */
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
#include "error.h"
#include "network.h"
#include "util.h"

/* Make price_t be a long long if the type is supported by the compiler */
#if SIZEOF_LONG_LONG == 0
typedef long price_t;
#else
typedef long long price_t;
#endif

/* "Abilities" are protocol extensions, which are negotiated between the
 * client and server at connect-time. */
typedef enum {
  A_PLAYERID = 0,               /* Use numeric IDs rather than player
                                 * names in network messages */
  A_DRUGVALUE,                  /* Server keeps track of purchase price
                                 * of drugs */
  A_NEWFIGHT,                   /* Use new unified fighting code */
  A_TSTRING,                    /* We understand the %Txx (tstring)
                                 * notation */
  A_DONEFIGHT,                  /* A fight is only considered over once the
                                 * client sends the server a C_DONE message */
  A_NUM                         /* N.B. Must be last */
} AbilType;

typedef struct ABILITIES {
  gboolean Local[A_NUM];        /* Abilities that we have */
  gboolean Remote[A_NUM];       /* Those that the other end of the
                                 * connection has */
  gboolean Shared[A_NUM];       /* Abilites shared by us and the
                                 * remote host */
} Abilities;

struct NAMES {
  gchar *Bitch, *Bitches, *Gun, *Guns, *Drug, *Drugs;
  gchar *Month, *Year, *LoanSharkName, *BankName;
  gchar *GunShopName, *RoughPubName;
};

#ifdef NETWORKING

struct METASERVER {
  gboolean Active;
  gchar *Name;
  unsigned Port;
  gchar *ProxyName;
  unsigned ProxyPort;
  gchar *Path, *LocalName, *Password, *Comment;
  gboolean UseSocks;
  gchar *authuser, *authpassword, *proxyuser, *proxypassword;
};
#endif

struct CURRENCY {
  gchar *Symbol;
  gboolean Prefix;
};

struct PRICES {
  price_t Spy, Tipoff;
};

struct BITCH {
  price_t MinPrice, MaxPrice;
};

typedef enum {
  CLIENT_AUTO, CLIENT_WINDOW, CLIENT_CURSES
} ClientType;

typedef enum {
  DM_NONE, DM_STREET, DM_FIGHT, DM_DEAL
} DispMode;

typedef enum {
  E_NONE = 0,
  E_SUBWAY, E_OFFOBJECT, E_WEED, E_SAYING, E_LOANSHARK,
  E_BANK, E_GUNSHOP, E_ROUGHPUB, E_HIREBITCH, E_ARRIVE,
  E_MAX,

  E_FINISH = 100,

  E_OUTOFSYNC = 120,
  E_FIGHT, E_FIGHTASK, E_DOCTOR, E_WAITDONE,
  E_MAXOOS
} EventCode;

typedef enum {
  FIRSTTURN   = 1 << 0,
  DEADHARDASS = 1 << 1,
  TIPPEDOFF   = 1 << 2,
  SPIEDON     = 1 << 3,
  SPYINGON    = 1 << 4,
  FIGHTING    = 1 << 5,
  CANSHOOT    = 1 << 6,
  TRADING     = 1 << 7
} PlayerFlags;

typedef enum {
  ACID = 0,
  COCAINE, HASHISH, HEROIN, LUDES, MDA, OPIUM, PCP,
  PEYOTE, SHROOMS, SPEED, WEED
} DrugIndex;

struct LOG {
  gchar *File;
  gint Level;
  gchar *Timestamp;
  FILE *fp;
};

extern int ClientSock, ListenSock;
extern gboolean Network, Client, Server, NotifyMetaServer, AIPlayer;
extern unsigned Port;
extern gboolean Sanitized, ConfigVerbose, DrugValue;
extern int NumLocation, NumGun, NumCop, NumDrug, NumSubway, NumPlaying,
           NumStoppedTo;
extern gchar *HiScoreFile, *ServerName, *ConvertFile, *ServerMOTD;
extern gboolean WantHelp, WantVersion, WantAntique, WantColour,
                WantNetwork, WantConvert, WantAdmin;
#ifdef CYGWIN
extern gboolean MinToSysTray;
#else
extern gboolean Daemonize;
#endif
extern gchar *WebBrowser;
extern ClientType WantedClient;
extern int LoanSharkLoc, BankLoc, GunShopLoc, RoughPubLoc;
extern int DrugSortMethod, FightTimeout, IdleTimeout, ConnectTimeout;
extern int MaxClients, AITurnPause;
extern struct CURRENCY Currency;
extern struct PRICES Prices;
extern struct BITCH Bitch;
extern price_t StartCash, StartDebt;
extern struct NAMES Names;

#ifdef NETWORKING
extern struct METASERVER MetaServer;
extern SocksServer Socks;
extern gboolean UseSocks;
#endif

extern int NumTurns;
extern int PlayerArmour, BitchArmour;

#define MAXLOG        6

#define DS_ATOZ       1
#define DS_ZTOA       2
#define DS_CHEAPFIRST 3
#define DS_CHEAPLAST  4
#define DS_MAX        5

#define NUMHISCORE    18

#define DEFLOANSHARK  1
#define DEFBANK       1
#define DEFGUNSHOP    2
#define DEFROUGHPUB   2

#define METAVERSION   2

struct COP {
  gchar *Name, *DeputyName, *DeputiesName;
  gint Armour, DeputyArmour;
  gint AttackPenalty, DefendPenalty;
  gint MinDeputies, MaxDeputies;
  gint GunIndex;
  gint CopGun, DeputyGun;
};
extern struct COP *Cop;

struct GUN {
  gchar *Name;
  price_t Price;
  int Space;
  int Damage;
};
extern struct GUN *Gun;

struct HISCORE {
  gchar *Time;
  price_t Money;
  gboolean Dead;
  gchar *Name;
};

struct LOCATION {
  gchar *Name;
  int PolicePresence;
  int MinDrug, MaxDrug;
};
extern struct LOCATION *Location;

struct DRUG {
  gchar *Name;
  price_t MinPrice, MaxPrice;
  gboolean Cheap, Expensive;
  gchar *CheapStr;
};
extern struct DRUG *Drug;

struct DRUGS {
  gchar *ExpensiveStr1, *ExpensiveStr2;
  int CheapDivide, ExpensiveMultiply;
};
extern struct DRUGS Drugs;

struct INVENTORY {
  price_t Price, TotalValue;
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

struct PLAYER_T {
  guint ID;
  int Turn;
  price_t Cash, Debt, Bank;
  int Health;
  int CoatSize;
  int IsAt;
  PlayerFlags Flags;
  gchar *Name;
  Inventory *Guns, *Drugs, Bitches;
  EventCode EventNum, ResyncNum;
  time_t FightTimeout, IdleTimeout, ConnectTimeout;
  price_t DocPrice;
  DopeList SpyList, TipList;
  Player *OnBehalfOf;
#ifdef NETWORKING
  NetworkBuffer NetBuf;
#endif
  Abilities Abil;
  GPtrArray *FightArray;        /* If non-NULL, a list of players
                                 * in a fight */
  Player *Attacking;            /* The player that this player
                                 * is attacking */
  gint CopIndex;                /* if >0, then this player is a cop,
                                 * described by Cop[CopIndex-1];
                                 * if ==0, this is a normal player that
                                 * has killed no cops;
                                 * if <0, then this is a normal player,
                                 * who has killed cops up to
                                 * Cop[-1-CopIndex] */
};

#define SN_PROMPT "(Prompt)"
#define SN_META   "(MetaServer)"
#define SN_SINGLE "(Single)"

typedef struct tag_serverdata {
  char *Name;
  unsigned Port;
  int MaxPlayers, CurPlayers;
  char *Comment, *Version, *Update, *UpSince;
} ServerData;

struct GLOBALS {
  int *IntVal;
  gboolean *BoolVal;
  price_t *PriceVal;
  gchar **StringVal;
  gchar ***StringList;
  char *Name, *Help;

  void **StructListPt, *StructStaticPt;
  int LenStruct;
  char *NameStruct;
  int *MaxIndex;
  void (*ResizeFunc) (int NewNum);
  gboolean Modified;
  int MinVal;
};

extern const int NUMGLOB;
extern struct GLOBALS Globals[];

extern Player Noone;
extern char **Playing;
extern char **SubwaySaying;
extern char **StoppedTo;
extern GSList *ServerList;
extern GScannerConfig ScannerConfig;
extern struct LOG Log;
extern gint ConfigErrors;

GSList *RemovePlayer(Player *Play, GSList *First);
Player *GetPlayerByID(guint ID, GSList *First);
Player *GetPlayerByName(gchar *Name, GSList *First);
int CountPlayers(GSList *First);
GSList *AddPlayer(int fd, Player *NewPlayer, GSList *First);
void UpdatePlayer(Player *Play);
void CopyPlayer(Player *Dest, Player *Src);
void ClearInventory(Inventory *Guns, Inventory *Drugs);
int IsCarryingRandom(Player *Play, int amount);
void ChangeSpaceForInventory(Inventory *Guns, Inventory *Drugs,
                             Player *Play);
void InitList(DopeList *List);
void AddListEntry(DopeList *List, DopeEntry *NewEntry);
void RemoveListEntry(DopeList *List, int Entry);
int GetListEntry(DopeList *List, Player *Play);
void RemoveListPlayer(DopeList *List, Player *Play);
void RemoveAllEntries(DopeList *List, Player *Play);
void ClearList(DopeList *List);
int TotalGunsCarried(Player *Play);
int read_string(FILE *fp, char **buf);
int brandom(int bot, int top);
price_t prandom(price_t bot, price_t top);
void AddInventory(Inventory *Cumul, Inventory *Add, int Length);
void TruncateInventoryFor(Inventory *Guns, Inventory *Drugs, Player *Play);
void PrintInventory(Inventory *Guns, Inventory *Drugs);
price_t strtoprice(char *buf);
gchar *pricetostr(price_t price);
gchar *FormatPrice(price_t price);
char IsInventoryClear(Inventory *Guns, Inventory *Drugs);
void ResizeLocations(int NewNum);
void ResizeCops(int NewNum);
void ResizeGuns(int NewNum);
void ResizeDrugs(int NewNum);
void ResizeSubway(int NewNum);
void ResizePlaying(int NewNum);
void ResizeStoppedTo(int NewNum);
void AssignName(gchar **dest, gchar *src);
void CopyNames(struct NAMES *dest, struct NAMES *src);

#ifdef NETWORKING
void CopyMetaServer(struct METASERVER *dest, struct METASERVER *src);
#endif
void CopyLocation(struct LOCATION *dest, struct LOCATION *src);
void CopyCop(struct COP *dest, struct COP *src);
void CopyGun(struct GUN *dest, struct GUN *src);
void CopyDrug(struct DRUG *dest, struct DRUG *src);
void CopyDrugs(struct DRUGS *dest, struct DRUGS *src);
int GetNextDrugIndex(int OldIndex, Player *Play);
gchar *InitialCaps(gchar *string);
char StartsWithVowel(char *string);
char *GetPlayerName(Player *Play);
void SetPlayerName(Player *Play, char *Name);
void HandleCmdLine(int argc, char *argv[]);
void SetupParameters(void);
void HandleHelpTexts(void);
void GeneralStartup(int argc, char *argv[]);
void ReadConfigFile(char *FileName);
gboolean ParseNextConfig(GScanner *scanner, gboolean print);
int GetGlobalIndex(gchar *ID1, gchar *ID2);
gchar **GetGlobalString(int GlobalIndex, int StructIndex);
gint *GetGlobalInt(int GlobalIndex, int StructIndex);
price_t *GetGlobalPrice(int GlobalIndex, int StructIndex);
gboolean *GetGlobalBoolean(int GlobalIndex, int StructIndex);
gchar ***GetGlobalStringList(int GlobalIndex, int StructIndex);
void PrintConfigValue(int GlobalIndex, int StructIndex,
                      gboolean IndexGiven, GScanner *scanner);
gboolean SetConfigValue(int GlobalIndex, int StructIndex,
                        gboolean IndexGiven, GScanner *scanner);
gboolean IsCop(Player *Play);
void dopelog(int loglevel, const gchar *format, ...);
GLogLevelFlags LogMask(void);
GString *GetLogString(GLogLevelFlags log_level, const gchar *message);
void RestoreConfig(void);
void ScannerErrorHandler(GScanner *scanner, gchar *msg, gint error);
void OpenLog(void);
void CloseLog(void);
gboolean IsConnectedPlayer(Player *play);
void BackupConfig(void);
void WriteConfigFile(FILE *fp);
gchar *GetDocIndex(void);
gchar *GetGlobalConfigFile(void);
gchar *GetLocalConfigFile(void);

#ifndef CURSES_CLIENT
void CursesLoop(void);
#endif

#ifndef GUI_CLIENT
#ifdef CYGWIN
gboolean GtkLoop(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                 gboolean ReturnOnFail);
#else
gboolean GtkLoop(int *argc, char **argv[], gboolean ReturnOnFail);
#endif
#endif

#endif
