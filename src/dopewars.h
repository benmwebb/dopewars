/* dopewars.h  Common structures and stuff for dopewars                 */
/* Copyright (C)  1998-2001  Ben Webb                                   */
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

/* "Abilities" are protocol extensions, which are negotiated between the
   client and server at connect-time. */
typedef enum {
   A_PLAYERID = 0,  /* Use numeric IDs rather than player names
                       in network messages */
   A_DRUGVALUE,     /* Server keeps track of purchase price of drugs */
   A_NEWFIGHT,      /* Use new unified fighting code */
   A_TSTRING,       /* We understand the %Txx (tstring) notation */

   A_NUM            /* N.B. Must be last */
} AbilType;

typedef struct ABILITIES {
   gboolean Local[A_NUM];   /* Abilities that we have */
   gboolean Remote[A_NUM];  /* Those that the other end of the connection has */
   gboolean Shared[A_NUM];  /* Abilites shared by us and the remote host */
} Abilities;

struct NAMES {
   gchar *Bitch,*Bitches,*Gun,*Guns,*Drug,*Drugs,*Month,*Year,
         *LoanSharkName,*BankName,*GunShopName,*RoughPubName;
};

struct METASERVER {
   gboolean Active;
   gchar *Name;
   unsigned Port;
   gchar *ProxyName;
   unsigned ProxyPort;
   gchar *Path,*LocalName,*Password,*Comment;
};

struct PRICES {
   price_t Spy,Tipoff;
};

struct BITCH {
   price_t MinPrice,MaxPrice;
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
   E_FIGHT, E_FIGHTASK, E_DOCTOR,
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

extern int ClientSock,ListenSock;
extern gboolean Network,Client,Server,NotifyMetaServer,AIPlayer;
extern unsigned Port;
extern gboolean Sanitized,ConfigVerbose,DrugValue;
extern int NumLocation,NumGun,NumCop,NumDrug,NumSubway,NumPlaying,NumStoppedTo;
extern gchar *HiScoreFile,*ServerName,*Pager,*ConvertFile;
extern gboolean WantHelp,WantVersion,WantAntique,WantColour,
                WantNetwork,WantConvert;
extern ClientType WantedClient;
extern int LoanSharkLoc,BankLoc,GunShopLoc,RoughPubLoc;
extern int DrugSortMethod,FightTimeout,IdleTimeout,ConnectTimeout;
extern int MaxClients,AITurnPause;
extern struct PRICES Prices;
extern struct BITCH Bitch;
extern price_t StartCash,StartDebt;
extern struct NAMES Names;
extern struct METASERVER MetaServer;
extern int NumTurns;
extern int PlayerArmour,BitchArmour;
extern int LogLevel;
extern gchar *LogTimestamp;

#define MAXLOG        6

#define DS_ATOZ       1
#define DS_ZTOA       2
#define DS_CHEAPFIRST 3
#define DS_CHEAPLAST  4
#define DS_MAX        5

#define NUMHISCORE   18

#define DEFLOANSHARK 1
#define DEFBANK      1
#define DEFGUNSHOP   2
#define DEFROUGHPUB  2

#define METAVERSION 2

struct COP {
   gchar *Name,*DeputyName,*DeputiesName;
   gint Armour,DeputyArmour;
   gint AttackPenalty,DefendPenalty;
   gint MinDeputies,MaxDeputies;
   gint GunIndex;
   gint CopGun,DeputyGun;
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
   int MinDrug,MaxDrug;
};
extern struct LOCATION *Location;

struct DRUG {
   gchar *Name;
   price_t MinPrice,MaxPrice;
   gboolean Cheap,Expensive;
   gchar *CheapStr;
};
extern struct DRUG *Drug;

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

typedef struct _NetworkBuffer NetworkBuffer;

typedef void (*NBCallBack)(NetworkBuffer *NetBuf,gboolean Read,gboolean Write);

typedef enum {
  ET_NOERROR, ET_CUSTOM, ET_ERRNO,
#ifdef CYGWIN
  ET_WIN32, ET_WINSOCK
#else
  ET_HERRNO
#endif
} ErrorType;

typedef struct _LastError {
  gint code;
  ErrorType type;
} LastError;

/* Handles reading and writing messages from/to a network connection */
struct _NetworkBuffer {
   int fd;                /* File descriptor of the socket */
   gint InputTag;         /* Identifier for gdk_input routines */
   NBCallBack CallBack;   /* Function called when the socket read- or
                             write-able status changes */
   gpointer CallBackData; /* Data accessible to the callback function */
   char Terminator;       /* Character that separates messages */
   char StripChar;        /* Character that should be removed from messages */
   ConnBuf ReadBuf;       /* New data, waiting for the application */
   ConnBuf WriteBuf;      /* Data waiting to be written to the wire */
   gboolean WaitConnect;  /* TRUE if a non-blocking connect is in progress */
   LastError error;       /* Any error from the last operation */
};

struct PLAYER_T {
   guint ID;
   int Turn;
   price_t Cash,Debt,Bank;
   int Health;
   int CoatSize;
   char IsAt;
   PlayerFlags Flags;
   gchar *Name;
   Inventory *Guns,*Drugs,Bitches;
   EventCode EventNum,ResyncNum;
   time_t FightTimeout,IdleTimeout,ConnectTimeout;
   price_t DocPrice;
   DopeList SpyList,TipList;
   Player *OnBehalfOf;
   NetworkBuffer NetBuf;
   Abilities Abil;
   GPtrArray *FightArray; /* If non-NULL, a list of players in a fight */
   Player *Attacking;     /* The player that this player is attacking */
   gint CopIndex;  /* if >0,  then this player is a cop, described
                              by Cop[CopIndex-1]
                      if ==0, this is a normal player that has killed no cops
                      if <0,  then this is a normal player, who has killed
                              cops up to Cop[-1-CopIndex] */
};

#define SN_PROMPT "(Prompt)"
#define SN_META   "(MetaServer)"
#define SN_SINGLE "(Single)"

typedef struct tag_serverdata {
   char *Name;
   unsigned Port;
   int MaxPlayers,CurPlayers;
   char *Comment,*Version,*Update,*UpSince;
} ServerData;

struct GLOBALS {
   int *IntVal;
   gboolean *BoolVal;
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

extern const int NUMGLOB;
extern struct GLOBALS Globals[];

extern Player Noone;
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
price_t prandom(price_t bot,price_t top);
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
void SetupParameters(void);
void HandleHelpTexts(void);
int GeneralStartup(int argc,char *argv[]);
void ReadConfigFile(char *FileName);
gboolean ParseNextConfig(GScanner *scanner);
int GetGlobalIndex(gchar *ID1,gchar *ID2);
void *GetGlobalPointer(int GlobalIndex,int StructIndex);
void PrintConfigValue(int GlobalIndex,int StructIndex,gboolean IndexGiven,
                      GScanner *scanner);
void SetConfigValue(int GlobalIndex,int StructIndex,gboolean IndexGiven,
                    GScanner *scanner);
gboolean IsCop(Player *Play);
void dopelog(int loglevel,const gchar *format,...);
GLogLevelFlags LogMask(void);
GString *GetLogString(GLogLevelFlags log_level,const gchar *message);
#endif
