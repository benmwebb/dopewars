/************************************************************************
 * dopewars.c     dopewars - general purpose routines and init          *
 * Copyright (C)  1998-2022  Ben Webb                                   *
 *                Email: benwebb@users.sf.net                           *
 *                WWW: https://dopewars.sourceforge.io/                 *
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

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <stdarg.h>

#include "configfile.h"
#include "convert.h"
#include "dopewars.h"
#include "admin.h"
#include "log.h"
#include "message.h"
#include "nls.h"
#include "serverside.h"
#include "sound.h"
#include "tstring.h"
#include "AIPlayer.h"
#include "util.h"
#include "winmain.h"

#ifdef CURSES_CLIENT
#include "curses_client/curses_client.h"
#endif

#ifdef GUI_CLIENT
#include "gui_client/gtk_client.h"
#endif

#ifdef GUI_SERVER
#include "gtkport/gtkport.h"
#endif

int ClientSock, ListenSock;
gboolean Network, Client, Server, WantAntique = FALSE, UseSounds = TRUE;

/* 
 * dopewars acting as standalone TCP server:
 *           Network=Server=TRUE   Client=FALSE
 * dopewars acting as client, connecting to standalone server:
 *           Network=Client=TRUE   Server=FALSE
 * dopewars in single-player or antique mode:
 *           Network=Server=Client=FALSE
 */
int Port = 7902;
gboolean Sanitized, ConfigVerbose, DrugValue, Antique = FALSE;
gchar *HiScoreFile = NULL, *ServerName = NULL;
gchar *ServerMOTD = NULL, *BindAddress = NULL, *PlayerName = NULL;

struct DATE StartDate = {
  1, 12, 1984
};

#ifdef CYGWIN
gboolean MinToSysTray = TRUE;
#else
gboolean Daemonize = TRUE;
#endif

#ifdef CYGWIN
#define SNDPATH "sounds\\19.5degs\\"
#else
#define SNDPATH DPDATADIR"/dopewars/"
#endif

gchar *OurWebBrowser = NULL;
gint ConfigErrors = 0;
gboolean LocaleIsUTF8 = FALSE;

int NumLocation = 0, NumGun = 0, NumCop = 0, NumDrug = 0, NumSubway = 0;
int NumPlaying = 0, NumStoppedTo = 0;
int DebtInterest = 10, BankInterest = 5;
Player Noone;
int LoanSharkLoc, BankLoc, GunShopLoc, RoughPubLoc;
int DrugSortMethod = DS_ATOZ;
int FightTimeout = 5, IdleTimeout = 14400, ConnectTimeout = 300;
int MaxClients = 20, AITurnPause = 5;
price_t StartCash = 2000, StartDebt = 5500;
GSList *ServerList = NULL;

GScannerConfig ScannerConfig = {
  " \t\n",                      /* Ignore these characters */

  /* Valid characters for starting an identifier */
  G_CSET_a_2_z "_" G_CSET_A_2_Z,

  /* Valid characters for continuing an identifier */
  G_CSET_a_2_z "._-0123456789" G_CSET_A_2_Z,

  "#\n",                        /* Single line comments start with # and
                                 * end with \n */
  FALSE,                        /* Are symbols case sensitive? */
  TRUE,                         /* Ignore C-style comments? */
  TRUE,                         /* Ignore single-line comments? */
  TRUE,                         /* Treat C-style comments as single tokens 
                                 * - do not break into words? */
  TRUE,                         /* Read identifiers as tokens? */
  TRUE,                         /* Read single-character identifiers as
                                 * 1-character strings? */
  TRUE,                         /* Allow the parsing of NULL as the
                                 * G_TOKEN_IDENTIFIER_NULL ? */
  FALSE,                        /* Allow symbols (defined by
                                 * g_scanner_scope_add_symbol) ? */
  TRUE,                         /* Allow binary numbers in 0b1110 format ? */
  TRUE,                         /* Allow octal numbers in C-style e.g. 034 ? */
  FALSE,                        /* Allow floats? */
  TRUE,                         /* Allow hex numbers in C-style e.g. 0xFF ? */
  TRUE,                         /* Allow hex numbers in $FF format ? */
  TRUE,                         /* Allow '' strings (no escaping) ? */
  TRUE,                         /* Allow "" strings (\ escapes parsed) ? */
  TRUE,                         /* Convert octal, binary and hex to int? */
  FALSE,                        /* Convert ints to floats? */
  FALSE,                        /* Treat all identifiers as strings? */
  TRUE,                         /* Leave single characters (e.g. {,=)
                                 * unchanged, instead of returning
                                 * G_TOKEN_CHAR ? */
  FALSE,                        /* Replace read symbols with the token
                                 * given by their value, instead of
                                 * G_TOKEN_SYMBOL ? */
  FALSE                         /* scope_0_fallback... */
};

struct LOCATION StaticLocation, *Location = NULL;
struct DRUG StaticDrug, *Drug = NULL;
struct GUN StaticGun, *Gun = NULL;
struct COP StaticCop, *Cop = NULL;
struct NAMES Names = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
struct SOUNDS Sounds = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/* N.B. The slightly over-enthusiastic comments here are for the benefit
 * of translators ;) */
struct NAMES DefaultNames = {
  /* Name of a single bitch - if you need to use different words for
     "bitch" depending on where in the sentence it occurs (e.g. subject or
     object) then read doc/i18n.html about the %tde (etc.) notation. N.B.
     This notation can be used for most of the translatable strings in
     dopewars. */
  N_("bitch"),
  /* Word used for two or more bitches */
  N_("bitches"),
  /* Word used for a single gun */
  N_("gun"),
  /* Word used for two or more guns */
  N_("guns"),
  /* Word used for a single drug */
  N_("drug"),
  /* Word used for two or more drugs */
  N_("drugs"),
  /* String for displaying the game date or turn number. This is passed
     to the strftime() function, with the exception that %T is used to
     mean the turn number rather than the calendar date. */
  N_("%m-%d-%Y"),
  /* Names of the loan shark, the bank, the gun shop, and the pub,
     respectively */
  N_("the Loan Shark"), N_("the Bank"),
  N_("Dan\'s House of Guns"), N_("the pub")
};

struct CURRENCY Currency = {
  NULL, TRUE
};

struct PRICES Prices = {
  20000, 10000
};

struct BITCH Bitch = {
  50000, 150000
};

#ifdef NETWORKING
struct METASERVER MetaServer = {
  FALSE, NULL, NULL, NULL, NULL
};

struct METASERVER DefaultMetaServer = {
  TRUE, "https://dopewars.sourceforge.io/metaserver.php", "",
  "", "dopewars server"
};

SocksServer Socks = { NULL, 0, 0, FALSE, NULL, NULL, NULL };
gboolean UseSocks;
#endif

int NumTurns = 31;

int PlayerArmor = 100, BitchArmor = 50;

struct LOG Log;

struct GLOBALS Globals[] = {
  /* The following strings are the helptexts for all the options that can
     be set in a dopewars configuration file, or in the server. See
     doc/configfile.html for more detailed explanations. */
  {&Port, NULL, NULL, NULL, NULL, "Port", N_("Network port to connect to"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 65535},
  {NULL, NULL, NULL, &HiScoreFile, NULL, "HiScoreFile",
   N_("Name of the high score file"), NULL, NULL, 0, "", NULL, NULL, FALSE,
   0, 0},
  {NULL, NULL, NULL, &ServerName, NULL, "Server",
   N_("Name of the server to connect to"), NULL, NULL, 0, "", NULL,
   NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &ServerMOTD, NULL, "ServerMOTD",
   N_("Server's welcome message of the day"), NULL, NULL, 0, "", NULL,
   NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &BindAddress, NULL, "BindAddress",
   N_("Network address for the server to listen on"), NULL, NULL, 0, "",
   NULL, NULL, FALSE, 0, 0},
#ifdef NETWORKING
  {NULL, &UseSocks, NULL, NULL, NULL, "Socks.Active",
   N_("TRUE if a SOCKS server should be used for networking"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
#ifndef CYGWIN
  {NULL, &Socks.numuid, NULL, NULL, NULL, "Socks.NumUID",
   N_("TRUE if numeric user IDs should be used for SOCKS4"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
#endif
  {NULL, NULL, NULL, &Socks.user, NULL, "Socks.User",
   N_("If not blank, the username to use for SOCKS4"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Socks.name, NULL, "Socks.Name",
   N_("The hostname of a SOCKS server to use"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {&Socks.port, NULL, NULL, NULL, NULL, "Socks.Port",
   N_("The port number of a SOCKS server to use"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 65535},
  {&Socks.version, NULL, NULL, NULL, NULL, "Socks.Version",
   N_("The version of the SOCKS protocol to use (4 or 5)"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 4, 5},
  {NULL, NULL, NULL, &Socks.authuser, NULL, "Socks.Auth.User",
   N_("Username for SOCKS5 authentication"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Socks.authpassword, NULL, "Socks.Auth.Password",
   N_("Password for SOCKS5 authentication"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, &MetaServer.Active, NULL, NULL, NULL, "MetaServer.Active",
   N_("TRUE if server should report to a metaserver"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &MetaServer.URL, NULL, "MetaServer.URL",
   N_("Metaserver URL to report/get server details to/from"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &MetaServer.LocalName, NULL, "MetaServer.LocalName",
   N_("Preferred hostname of your server machine"), NULL, NULL, 0, "",
   NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &MetaServer.Password, NULL, "MetaServer.Password",
   N_("Authentication for LocalName with the metaserver"), NULL, NULL, 0,
   "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &MetaServer.Comment, NULL, "MetaServer.Comment",
   N_("Server description, reported to the metaserver"), NULL, NULL, 0, "",
   NULL, NULL, FALSE, 0, 0},
#endif /* NETWORKING */
#ifdef CYGWIN
  {NULL, &MinToSysTray, NULL, NULL, NULL, "MinToSysTray",
   N_("If TRUE, the server minimizes to the System Tray"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
#else
  {NULL, &Daemonize, NULL, NULL, NULL, "Daemonize",
   N_("If TRUE, the server runs in the background"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &OurWebBrowser, NULL, "WebBrowser",
   N_("The command used to start your web browser"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
#endif
  {&NumTurns, NULL, NULL, NULL, NULL, "NumTurns",
   N_("No. of game turns (if 0, game never ends)"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {&StartDate.day, NULL, NULL, NULL, NULL, "StartDate.Day",
   N_("Day of the month on which the game starts"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 1, 31},
  {&StartDate.month, NULL, NULL, NULL, NULL, "StartDate.Month",
   N_("Month in which the game starts"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 1, 12},
  {&StartDate.year, NULL, NULL, NULL, NULL, "StartDate.Year",
   N_("Year in which the game starts"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {NULL, NULL, NULL, &Currency.Symbol, NULL, "Currency.Symbol",
   N_("The currency symbol (e.g. $)"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, &Currency.Prefix, NULL, NULL, NULL, "Currency.Prefix",
   N_("If TRUE, the currency symbol precedes prices"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Log.File, NULL, "Log.File",
   N_("File to write log messages to"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {&Log.Level, NULL, NULL, NULL, NULL, "Log.Level",
   N_("Controls the number of log messages produced"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 5},
  {NULL, NULL, NULL, &Log.Timestamp, NULL, "Log.Timestamp",
   N_("strftime() format string for log timestamps"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, &Sanitized, NULL, NULL, NULL, "Sanitized",
   N_("Random events are sanitized"), NULL, NULL, 0, "", NULL, NULL, FALSE,
   0, 0},
  {NULL, &DrugValue, NULL, NULL, NULL, "DrugValue",
   N_("TRUE if the value of bought drugs should be saved"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, &ConfigVerbose, NULL, NULL, NULL, "ConfigVerbose",
   N_("Be verbose in processing config file"), NULL, NULL, 0, "", NULL,
   NULL, FALSE, 0, 0},
  {&NumLocation, NULL, NULL, NULL, NULL, "NumLocation",
   N_("Number of locations in the game"),
   (void **)(&Location), NULL, sizeof(struct LOCATION), "", NULL,
   ResizeLocations, FALSE, 1, -1},
  {&NumCop, NULL, NULL, NULL, NULL, "NumCop",
   N_("Number of types of cop in the game"),
   (void **)(&Cop), NULL, sizeof(struct COP), "", NULL, ResizeCops, FALSE,
   0, -1},
  {&NumGun, NULL, NULL, NULL, NULL, "NumGun",
   N_("Number of guns in the game"),
   (void **)(&Gun), NULL, sizeof(struct GUN), "", NULL, ResizeGuns, FALSE,
   0, -1},
  {&NumDrug, NULL, NULL, NULL, NULL, "NumDrug",
   N_("Number of drugs in the game"),
   (void **)(&Drug), NULL, sizeof(struct DRUG), "", NULL, ResizeDrugs,
   FALSE, 1, -1},
  {&LoanSharkLoc, NULL, NULL, NULL, NULL, "LoanShark",
   N_("Location of the Loan Shark"), NULL, NULL, 0, "", NULL, NULL, FALSE,
   0, 0},
  {&BankLoc, NULL, NULL, NULL, NULL, "Bank", N_("Location of the bank"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {&GunShopLoc, NULL, NULL, NULL, NULL, "GunShop",
   N_("Location of the gun shop"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {&RoughPubLoc, NULL, NULL, NULL, NULL, "RoughPub",
   N_("Location of the pub"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {&DebtInterest, NULL, NULL, NULL, NULL, "DebtInterest",
   N_("Daily interest rate on the loan shark debt"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, -100, -200},
  {&BankInterest, NULL, NULL, NULL, NULL, "BankInterest",
   N_("Daily interest rate on your bank balance"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, -100, -200},
  {NULL, NULL, NULL, &Names.LoanSharkName, NULL, "LoanSharkName",
   N_("Name of the loan shark"), NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Names.BankName, NULL, "BankName",
   N_("Name of the bank"), NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Names.GunShopName, NULL, "GunShopName",
   N_("Name of the gun shop"), NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Names.RoughPubName, NULL, "RoughPubName",
   N_("Name of the pub"), NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, &UseSounds, NULL, NULL, NULL, "UseSounds",
   N_("TRUE if sounds should be enabled"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.FightHit, NULL, "Sounds.FightHit",
   N_("Sound file played for a gun \"hit\""), NULL, NULL, 0, "",
   NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.FightMiss, NULL, "Sounds.FightMiss",
   N_("Sound file played for a gun \"miss\""), NULL, NULL, 0, "",
   NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.FightReload, NULL, "Sounds.FightReload",
   N_("Sound file played when guns are reloaded"), NULL, NULL, 0, "",
   NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.EnemyBitchKilled, NULL, "Sounds.EnemyBitchKilled",
   N_("Sound file played when an enemy bitch/deputy is killed"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.BitchKilled, NULL, "Sounds.BitchKilled",
   N_("Sound file played when one of your bitches is killed"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.EnemyKilled, NULL, "Sounds.EnemyKilled",
   N_("Sound file played when another player or cop is killed"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.Killed, NULL, "Sounds.Killed",
   N_("Sound file played when you are killed"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.EnemyFailFlee, NULL, "Sounds.EnemyFailFlee",
   N_("Sound file played when a player tries to escape, but fails"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.FailFlee, NULL, "Sounds.FailFlee",
   N_("Sound file played when you try to escape, but fail"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.EnemyFlee, NULL, "Sounds.EnemyFlee",
   N_("Sound file played when a player successfully escapes"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.Flee, NULL, "Sounds.Flee",
   N_("Sound file played when you successfully escape"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.Jet, NULL, "Sounds.Jet",
   N_("Sound file played on arriving at a new location"), NULL, NULL, 0, "",
   NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.TalkToAll, NULL, "Sounds.TalkToAll",
   N_("Sound file played when a player sends a public chat message"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.TalkPrivate, NULL, "Sounds.TalkPrivate",
   N_("Sound file played when a player sends a private chat message"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.JoinGame, NULL, "Sounds.JoinGame",
   N_("Sound file played when a player joins the game"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.LeaveGame, NULL, "Sounds.LeaveGame",
   N_("Sound file played when a player leaves the game"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.StartGame, NULL, "Sounds.StartGame",
   N_("Sound file played at the start of the game"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Sounds.EndGame, NULL, "Sounds.EndGame",
   N_("Sound file played at the end of the game"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {&DrugSortMethod, NULL, NULL, NULL, NULL, "DrugSortMethod",
   N_("Sort key for listing available drugs"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 1, 4},
  {&FightTimeout, NULL, NULL, NULL, NULL, "FightTimeout",
   N_("No. of seconds in which to return fire"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {&IdleTimeout, NULL, NULL, NULL, NULL, "IdleTimeout",
   N_("Players are disconnected after this many seconds"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {&ConnectTimeout, NULL, NULL, NULL, NULL, "ConnectTimeout",
   N_("Time in seconds for connections to be made or broken"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {&MaxClients, NULL, NULL, NULL, NULL, "MaxClients",
   N_("Maximum number of TCP/IP connections"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {&AITurnPause, NULL, NULL, NULL, NULL, "AITurnPause",
   N_("Seconds between turns of AI players"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {NULL, NULL, &StartCash, NULL, NULL, "StartCash",
   N_("Amount of cash that each player starts with"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {NULL, NULL, &StartDebt, NULL, NULL, "StartDebt",
   N_("Amount of debt that each player starts with"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {NULL, NULL, NULL, &StaticLocation.Name, NULL, "Name",
   N_("Name of each location"), (void **)(&Location), &StaticLocation,
   sizeof(struct LOCATION), "Location", &NumLocation, NULL, FALSE, 0, -1},
  {&(StaticLocation.PolicePresence), NULL, NULL, NULL, NULL,
   "PolicePresence",
   N_("Police presence at each location (%)"),
   (void **)(&Location), &StaticLocation,
   sizeof(struct LOCATION), "Location", &NumLocation, NULL, FALSE, 0, 100},
  {&(StaticLocation.MinDrug), NULL, NULL, NULL, NULL, "MinDrug",
   N_("Minimum number of drugs at each location"),
   (void **)(&Location), &StaticLocation,
   sizeof(struct LOCATION), "Location", &NumLocation, NULL, FALSE, 1, -1},
  {&(StaticLocation.MaxDrug), NULL, NULL, NULL, NULL, "MaxDrug",
   N_("Maximum number of drugs at each location"),
   (void **)(&Location), &StaticLocation,
   sizeof(struct LOCATION), "Location", &NumLocation, NULL, FALSE, 1, -1},
  {&PlayerArmor, NULL, NULL, NULL, NULL, "PlayerArmour",
   N_("% resistance to gunshots of each player"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 100},
  {&PlayerArmor, NULL, NULL, NULL, NULL, "PlayerArmor",
   N_("% resistance to gunshots of each player"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 100},
  {&BitchArmor, NULL, NULL, NULL, NULL, "BitchArmour",
   N_("% resistance to gunshots of each bitch"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 1, 100},
  {&BitchArmor, NULL, NULL, NULL, NULL, "BitchArmor",
   N_("% resistance to gunshots of each bitch"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 1, 100},
  {NULL, NULL, NULL, &StaticCop.Name, NULL, "Name",
   N_("Name of each cop"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &StaticCop.DeputyName, NULL, "DeputyName",
   N_("Name of each cop's deputy"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &StaticCop.DeputiesName, NULL, "DeputiesName",
   N_("Name of each cop's deputies"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 0, 0},
  {&StaticCop.Armor, NULL, NULL, NULL, NULL, "Armour",
   N_("% resistance to gunshots of each cop"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 1, 100},
  {&StaticCop.Armor, NULL, NULL, NULL, NULL, "Armor",
   N_("% resistance to gunshots of each cop"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 1, 100},
  {&StaticCop.DeputyArmor, NULL, NULL, NULL, NULL, "DeputyArmour",
   N_("% resistance to gunshots of each deputy"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 1, 100},
  {&StaticCop.DeputyArmor, NULL, NULL, NULL, NULL, "DeputyArmor",
   N_("% resistance to gunshots of each deputy"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 1, 100},
  {&StaticCop.AttackPenalty, NULL, NULL, NULL, NULL, "AttackPenalty",
   N_("Attack penalty relative to a player"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 0, 100},
  {&StaticCop.DefendPenalty, NULL, NULL, NULL, NULL, "DefendPenalty",
   N_("Defend penalty relative to a player"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 0, 100},
  {&StaticCop.MinDeputies, NULL, NULL, NULL, NULL, "MinDeputies",
   N_("Minimum number of accompanying deputies"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 0, -1},
  {&StaticCop.MaxDeputies, NULL, NULL, NULL, NULL, "MaxDeputies",
   N_("Maximum number of accompanying deputies"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 0, -1},
  {&StaticCop.GunIndex, NULL, NULL, NULL, NULL, "GunIndex",
   N_("Zero-based index of the gun that cops are armed with"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 0, -1},
  {&StaticCop.CopGun, NULL, NULL, NULL, NULL, "CopGun",
   N_("Number of guns that each cop carries"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 0, -1},
  {&StaticCop.DeputyGun, NULL, NULL, NULL, NULL, "DeputyGun",
   N_("Number of guns that each deputy carries"),
   (void **)(&Cop), &StaticCop, sizeof(struct COP), "Cop", &NumCop,
   NULL, FALSE, 0, -1},
  {NULL, NULL, NULL, &StaticDrug.Name, NULL, "Name",
   N_("Name of each drug"),
   (void **)(&Drug), &StaticDrug,
   sizeof(struct DRUG), "Drug", &NumDrug, NULL, FALSE, 0, 0},
  {NULL, NULL, &(StaticDrug.MinPrice), NULL, NULL, "MinPrice",
   N_("Minimum normal price of each drug"),
   (void **)(&Drug), &StaticDrug,
   sizeof(struct DRUG), "Drug", &NumDrug, NULL, FALSE, 1, -1},
  {NULL, NULL, &(StaticDrug.MaxPrice), NULL, NULL, "MaxPrice",
   N_("Maximum normal price of each drug"),
   (void **)(&Drug), &StaticDrug,
   sizeof(struct DRUG), "Drug", &NumDrug, NULL, FALSE, 1, -1},
  {NULL, &(StaticDrug.Cheap), NULL, NULL, NULL, "Cheap",
   N_("TRUE if this drug can be specially cheap"),
   (void **)(&Drug), &StaticDrug,
   sizeof(struct DRUG), "Drug", &NumDrug, NULL, FALSE, 0, 0},
  {NULL, &(StaticDrug.Expensive), NULL, NULL, NULL, "Expensive",
   N_("TRUE if this drug can be specially expensive"),
   (void **)(&Drug), &StaticDrug,
   sizeof(struct DRUG), "Drug", &NumDrug, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &StaticDrug.CheapStr, NULL, "CheapStr",
   N_("Message displayed when this drug is specially cheap"),
   (void **)(&Drug), &StaticDrug,
   sizeof(struct DRUG), "Drug", &NumDrug, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Drugs.ExpensiveStr1, NULL, "Drugs.ExpensiveStr1",
   N_("Format string used for expensive drugs 50% of time"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Drugs.ExpensiveStr2, NULL, "Drugs.ExpensiveStr2",
   /* xgettext:no-c-format */
   N_("Format string used for expensive drugs 50% of time"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {&(Drugs.CheapDivide), NULL, NULL, NULL, NULL, "Drugs.CheapDivide",
   N_("Divider for drug price when it's specially cheap"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 1, -1},
  {&(Drugs.ExpensiveMultiply), NULL, NULL, NULL, NULL,
   "Drugs.ExpensiveMultiply",
   N_("Multiplier for specially expensive drug prices"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 1, -1},
  {NULL, NULL, NULL, &StaticGun.Name, NULL, "Name",
   N_("Name of each gun"),
   (void **)(&Gun), &StaticGun,
   sizeof(struct GUN), "Gun", &NumGun, NULL, FALSE, 0, 0},
  {NULL, NULL, &(StaticGun.Price), NULL, NULL, "Price",
   N_("Price of each gun"),
   (void **)(&Gun), &StaticGun,
   sizeof(struct GUN), "Gun", &NumGun, NULL, FALSE, 0, 0},
  {&(StaticGun.Space), NULL, NULL, NULL, NULL, "Space",
   N_("Space taken by each gun"),
   (void **)(&Gun), &StaticGun,
   sizeof(struct GUN), "Gun", &NumGun, NULL, FALSE, 0, -1},
  {&(StaticGun.Damage), NULL, NULL, NULL, NULL, "Damage",
   N_("Damage done by each gun"),
   (void **)(&Gun), &StaticGun,
   sizeof(struct GUN), "Gun", &NumGun, NULL, FALSE, 0, -1},
  {NULL, NULL, NULL, &Names.Bitch, NULL, "Names.Bitch",
   N_("Word used to denote a single \"bitch\""), NULL, NULL, 0, "", NULL,
   NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Names.Bitches, NULL, "Names.Bitches",
   N_("Word used to denote two or more \"bitches\""),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Names.Gun, NULL, "Names.Gun",
   N_("Word used to denote a single gun or equivalent"), NULL, NULL, 0, "",
   NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Names.Guns, NULL, "Names.Guns",
   N_("Word used to denote two or more guns"), NULL, NULL, 0, "", NULL,
   NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Names.Drug, NULL, "Names.Drug",
   N_("Word used to denote a single drug or equivalent"), NULL, NULL, 0,
   "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Names.Drugs, NULL, "Names.Drugs",
   N_("Word used to denote two or more drugs"), NULL, NULL, 0, "", NULL,
   NULL, FALSE, 0, 0},
  {NULL, NULL, NULL, &Names.Date, NULL, "Names.Date",
   N_("strftime() format string for displaying the game turn"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, 0},
  {NULL, NULL, &Prices.Spy, NULL, NULL, "Prices.Spy",
   N_("Cost for a bitch to spy on the enemy"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {NULL, NULL, &Prices.Tipoff, NULL, NULL, "Prices.Tipoff",
   N_("Cost for a bitch to tipoff the cops to an enemy"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {NULL, NULL, &Bitch.MinPrice, NULL, NULL, "Bitch.MinPrice",
   N_("Minimum price to hire a bitch"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {NULL, NULL, &Bitch.MaxPrice, NULL, NULL, "Bitch.MaxPrice",
   N_("Maximum price to hire a bitch"),
   NULL, NULL, 0, "", NULL, NULL, FALSE, 0, -1},
  {NULL, NULL, NULL, NULL, &SubwaySaying, "SubwaySaying",
   N_("List of things which you overhear on the subway"),
   NULL, NULL, 0, "", &NumSubway, ResizeSubway, FALSE, 0, 0},
  {&NumSubway, NULL, NULL, NULL, NULL, "NumSubwaySaying",
   N_("Number of subway sayings"),
   NULL, NULL, 0, "", NULL, ResizeSubway, FALSE, 0, -1},
  {NULL, NULL, NULL, NULL, &Playing, "Playing",
   N_("List of songs which you can hear playing"),
   NULL, NULL, 0, "", &NumPlaying, ResizePlaying, FALSE, 0, 0},
  {&NumPlaying, NULL, NULL, NULL, NULL, "NumPlaying",
   N_("Number of playing songs"),
   NULL, NULL, 0, "", NULL, ResizePlaying, FALSE, 0, -1},
  {NULL, NULL, NULL, NULL, &StoppedTo, "StoppedTo",
   N_("List of things which you can stop to do"),
   NULL, NULL, 0, "", &NumStoppedTo, ResizeStoppedTo, FALSE, 0, 0},
  {&NumStoppedTo, NULL, NULL, NULL, NULL, "NumStoppedTo",
   N_("Number of things which you can stop to do"),
   NULL, NULL, 0, "", NULL, ResizeStoppedTo, FALSE, 0, -1}
};
const int NUMGLOB = sizeof(Globals) / sizeof(Globals[0]);

char **Playing = NULL;
char *DefaultPlaying[] = {
  /* Default list of songs that you can hear playing (N.B. this can be
     overridden in the configuration file with the "Playing" variable) -
     look for "You hear someone playing %s" to see how these are used. */
  N_("`Are you Experienced` by Jimi Hendrix"),
  N_("`Cheeba Cheeba` by Tone Loc"),
  N_("`Comin` in to Los Angeles` by Arlo Guthrie"),
  N_("`Commercial` by Spanky and Our Gang"),
  N_("`Late in the Evening` by Paul Simon"),
  N_("`Light Up` by Styx"),
  N_("`Mexico` by Jefferson Airplane"),
  N_("`One toke over the line` by Brewer & Shipley"),
  N_("`The Smokeout` by Shel Silverstein"),
  N_("`White Rabbit` by Jefferson Airplane"),
  N_("`Itchycoo Park` by Small Faces"),
  N_("`White Punks on Dope` by the Tubes"),
  N_("`Legend of a Mind` by the Moody Blues"),
  N_("`Eight Miles High` by the Byrds"),
  N_("`Acapulco Gold` by Riders of the Purple Sage"),
  N_("`Kicks` by Paul Revere & the Raiders"),
  N_("the Nixon tapes"),
  N_("`Legalize It` by Mojo Nixon & Skid Roper")
};

char **StoppedTo = NULL;
char *DefaultStoppedTo[] = {
  /* Default list of things which you can "stop to do" (random events that
     cost you a little money). These can be overridden with the "StoppedTo"
     variable in the configuration file. See the later string "You stopped
     to %s." to see how these strings are used. */
  N_("have a beer"),
  N_("smoke a joint"),
  N_("smoke a cigar"),
  N_("smoke a Djarum"),
  N_("smoke a cigarette")
};

struct COP DefaultCop[] = {
  /* Name of the first police officer to attack you */
  {N_("Officer Hardass"),
   /* Name of a single deputy of the first police officer */
   N_("deputy"),
   /* Word used for more than one deputy of the first police officer */
   N_("deputies"), 4, 3, 30, 30, 2, 8, 0, 1, 1},
  /* Ditto, for the other police officers */
  {N_("Officer Bob"), N_("deputy"), N_("deputies"), 15, 4, 30, 20, 4, 10,
   0, 2, 1},
  {N_("Agent Smith"), N_("cop"), N_("cops"), 50, 6, 20, 20, 6, 18, 1, 3, 2}
};

struct GUN DefaultGun[] = {
  /* The names of the default guns */
  {N_("Baretta"), 3000, 4, 5},
  {N_(".38 Special"), 3500, 4, 9},
  {N_("Ruger"), 2900, 4, 4},
  {N_("Saturday Night Special"), 3100, 4, 7}
};

struct DRUG DefaultDrug[] = {
  /* The names of the default drugs, and the messages displayed when they
     are specially cheap or expensive */
  {N_("Acid"), 1000, 4400, TRUE, FALSE,
   N_("The market is flooded with cheap home-made acid!")},
  {N_("Cocaine"), 15000, 29000, FALSE, TRUE, ""},
  {N_("Hashish"), 480, 1280, TRUE, FALSE,
   N_("The Marrakesh Express has arrived!")},
  {N_("Heroin"), 5500, 13000, FALSE, TRUE, ""},
  {N_("Ludes"), 11, 60, TRUE, FALSE,
   N_("Rival drug dealers raided a pharmacy and are selling cheap ludes!")},
  {N_("MDA"), 1500, 4400, FALSE, FALSE, ""},
  {N_("Opium"), 540, 1250, FALSE, TRUE, ""},
  {N_("PCP"), 1000, 2500, FALSE, FALSE, ""},
  {N_("Peyote"), 220, 700, FALSE, FALSE, ""},
  {N_("Shrooms"), 630, 1300, FALSE, FALSE, ""},
  {N_("Speed"), 90, 250, FALSE, TRUE, ""},
  {N_("Weed"), 315, 890, TRUE, FALSE,
   N_("Colombian freighter dusted the Coast Guard! "
      "Weed prices have bottomed out!")}
};

#define NUMDRUG (sizeof(DefaultDrug)/sizeof(DefaultDrug[0]))

struct LOCATION DefaultLocation[] = {
  /* The names of the default locations */
  {N_("Bronx"), 10, NUMDRUG / 2 + 1, NUMDRUG},
  {N_("Ghetto"), 5, NUMDRUG / 2 + 2, NUMDRUG},
  {N_("Central Park"), 15, NUMDRUG / 2, NUMDRUG},
  {N_("Manhattan"), 90, NUMDRUG / 2 - 2, NUMDRUG - 2},
  {N_("Coney Island"), 20, NUMDRUG / 2, NUMDRUG},
  {N_("Brooklyn"), 70, NUMDRUG / 2 - 2, NUMDRUG - 1},
  {N_("Queens"), 50, NUMDRUG / 2, NUMDRUG},
  {N_("Staten Island"), 20, NUMDRUG / 2, NUMDRUG}
};

struct DRUGS Drugs = { NULL, NULL, 0, 0 };
struct DRUGS DefaultDrugs = {
  /* Messages displayed for drug busts, etc. */
  N_("Cops made a big %tde bust! Prices are outrageous!"),
  N_("Addicts are buying %tde at ridiculous prices!"),
  4, 4
};

char **SubwaySaying = NULL;
char *DefaultSubwaySaying[] = {
  /* Default list of things which the "lady on the subway" can tell you
     (N.B. can be overridden with the "SubwaySaying" config. file
     variable). Look for "the lady next to you" to see how these strings
     are used. */
  N_("Wouldn\'t it be funny if everyone suddenly quacked at once?"),
  N_("The Pope was once Jewish, you know"),
  N_("I\'ll bet you have some really interesting dreams"),
  N_("So I think I\'m going to Amsterdam this year"),
  N_("Son, you need a yellow haircut"),
  N_("I think it\'s wonderful what they\'re doing with incense these days"),
  N_("I wasn\'t always a woman, you know"),
  N_("Does your mother know you\'re a dope dealer?"),
  N_("Are you high on something?"),
  N_("Oh, you must be from California"),
  N_("I used to be a hippie, myself"),
  N_("There\'s nothing like having lots of money"),
  N_("You look like an aardvark!"),
  N_("I don\'t believe in Ronald Reagan"),
  N_("Courage!  Bush is a noodle!"),
  N_("Haven\'t I seen you on TV?"),
  N_("I think hemorrhoid commercials are really neat!"),
  N_("We\'re winning the war for drugs!"),
  N_("A day without dope is like night"),
  /* xgettext:no-c-format */
  N_("We only use 20% of our brains, so why not burn out the other 80%"),
  N_("I\'m soliciting contributions for Zombies for Christ"),
  N_("I\'d like to sell you an edible poodle"),
  N_("Winners don\'t do drugs... unless they do"),
  N_("Kill a cop for Christ!"),
  N_("I am the walrus!"),
  N_("Jesus loves you more than you will know"),
  N_("I feel an unaccountable urge to dye my hair blue"),
  N_("Wasn\'t Jane Fonda wonderful in Barbarella"),
  N_("Just say No... well, maybe... ok, what the hell!"),
  N_("Would you like a jelly baby?"),
  N_("Drugs can be your friend!")
};

static gboolean SetConfigValue(int GlobalIndex, int StructIndex,
                               gboolean IndexGiven, Converter *conv,
                               GScanner *scanner);
/* 
 * Returns a random integer not less than bot and less than top.
 */
int brandom(int bot, int top)
{
  return (int)((float)(top - bot) * rand() / (RAND_MAX + 1.0)) + bot;
}

/* 
 * Returns a random price not less than bot and less than top.
 */
price_t prandom(price_t bot, price_t top)
{
  return (price_t)((float)(top - bot) * rand() / (RAND_MAX + 1.0)) + bot;
}

/* 
 * Returns the total numbers of players in the list starting at "First";
 * players still in the process of connecting or leaving, and those that
 * are actually cops (server-created internal AI players) are ignored.
 */
int CountPlayers(GSList *First)
{
  GSList *list;
  Player *Play;
  int count = 0;

  for (list = First; list; list = g_slist_next(list)) {
    Play = (Player *)list->data;
    if (strlen(GetPlayerName(Play)) > 0 && !IsCop(Play))
      count++;
  }
  return count;
}

/* 
 * Adds the new Player structure "NewPlayer" to the linked list
 * pointed to by "First", and initializes all fields. Returns the new
 * start of the list. If this function is called by the server, then
 * it should pass the file descriptor of the socket used to
 * communicate with the client player.
 */
GSList *AddPlayer(int fd, Player *NewPlayer, GSList *First)
{
  Player *tmp;
  GSList *list;

  list = First;
  NewPlayer->ID = 0;
  /* Generate a unique player ID, if we're the server (clients get their
   * IDs from the server, so don't need to generate IDs) */
  if (Server) {
    while (list) {
      tmp = (Player *)list->data;
      if (tmp->ID == NewPlayer->ID) {
        NewPlayer->ID++;
        list = First;
      } else {
        list = g_slist_next(list);
      }
    }
  }
  NewPlayer->Name = NULL;
  SetPlayerName(NewPlayer, NULL);
  NewPlayer->IsAt = 0;
  NewPlayer->EventNum = E_NONE;
  NewPlayer->FightTimeout = NewPlayer->ConnectTimeout =
      NewPlayer->IdleTimeout = 0;
  NewPlayer->Guns = (Inventory *)g_malloc0(NumGun * sizeof(Inventory));
  NewPlayer->Drugs = (Inventory *)g_malloc0(NumDrug * sizeof(Inventory));
  InitList(&(NewPlayer->SpyList));
  InitList(&(NewPlayer->TipList));
  NewPlayer->Turn = 1;
  NewPlayer->date = g_date_new_dmy(StartDate.day, StartDate.month,
                                   StartDate.year);
  NewPlayer->Cash = StartCash;
  NewPlayer->Debt = StartDebt;
  NewPlayer->Bank = 0;
  NewPlayer->Bitches.Carried = 8;
  NewPlayer->CopIndex = 0;
  NewPlayer->Health = 100;
  NewPlayer->CoatSize = 100;
  NewPlayer->Flags = 0;
#ifdef NETWORKING
  InitNetworkBuffer(&NewPlayer->NetBuf, '\n', '\r',
                    UseSocks ? &Socks : NULL);
  if (Server)
    BindNetworkBufferToSocket(&NewPlayer->NetBuf, fd);
#endif
  InitAbilities(NewPlayer);
  NewPlayer->FightArray = NULL;
  NewPlayer->Attacking = NULL;
  return g_slist_append(First, (gpointer)NewPlayer);
}

/* 
 * Returns TRUE only if the given player has properly connected (i.e. has
 * a valid name).
 */
gboolean IsConnectedPlayer(Player *play)
{
  return (play && play->Name && play->Name[0]);
}

/* 
 * Redimensions the Gun and Drug lists for "Play".
 */
void UpdatePlayer(Player *Play)
{
  Play->Guns =
      (Inventory *)g_realloc(Play->Guns, NumGun * sizeof(Inventory));
  Play->Drugs =
      (Inventory *)g_realloc(Play->Drugs, NumDrug * sizeof(Inventory));
}

/* 
 * Removes the Player structure pointed to by "Play" from the linked
 * list starting at "First". The client socket is freed if called
 * from the server. The new start of the list is returned.
 */
GSList *RemovePlayer(Player *Play, GSList *First)
{
  g_assert(Play);
  g_assert(First);

  First = g_slist_remove(First, (gpointer)Play);
#ifdef NETWORKING
  if (!IsCop(Play))
    ShutdownNetworkBuffer(&Play->NetBuf);
#endif
  ClearList(&(Play->SpyList));
  ClearList(&(Play->TipList));
  g_date_free(Play->date);
  g_free(Play->Name);
  g_free(Play->Guns);
  g_free(Play->Drugs);
  g_free(Play);
  return First;
}

/* 
 * Copies player "Src" to player "Dest".
 */
void CopyPlayer(Player *Dest, Player *Src)
{
  if (!Dest || !Src)
    return;
  Dest->Turn = Src->Turn;
  Dest->Cash = Src->Cash;
  Dest->Debt = Src->Debt;
  Dest->Bank = Src->Bank;
  Dest->Health = Src->Health;
  ClearInventory(Dest->Guns, Dest->Drugs);
  AddInventory(Dest->Guns, Src->Guns, NumGun);
  AddInventory(Dest->Drugs, Src->Drugs, NumDrug);
  Dest->CoatSize = Src->CoatSize;
  Dest->IsAt = Src->IsAt;
  g_free(Dest->Name);
  Dest->Name = g_strdup(Src->Name);
  Dest->Bitches.Carried = Src->Bitches.Carried;
  Dest->Flags = Src->Flags;
}

gboolean IsCop(Player *Play)
{
  return (Play->CopIndex > 0);
}

char *GetPlayerName(Player *Play)
{
  if (Play->Name)
    return Play->Name;
  else
    return "";
}

void SetPlayerName(Player *Play, char *Name)
{
  if (Play->Name)
    g_free(Play->Name);
  if (!Name)
    Play->Name = g_strdup("");
  else
    Play->Name = g_strdup(Name);
}

/* 
 * Searches the linked list starting at "First" for a Player structure
 * with the given ID. Returns a pointer to this structure, or NULL if
 * no match can be found.
 */
Player *GetPlayerByID(guint ID, GSList *First)
{
  GSList *list;
  Player *Play;

  for (list = First; list; list = g_slist_next(list)) {
    Play = (Player *)list->data;
    if (Play->ID == ID)
      return Play;
  }
  return NULL;
}

/* 
 * Searches the linked list starting at "First" for a Player structure
 * with the name "Name". Returns a pointer to this structure, or NULL
 * if no match can be found.
 */
Player *GetPlayerByName(char *Name, GSList *First)
{
  GSList *list;
  Player *Play;

  if (Name == NULL || Name[0] == 0)
    return &Noone;
  for (list = First; list; list = g_slist_next(list)) {
    Play = (Player *)list->data;
    if (!IsCop(Play) && strcmp(GetPlayerName(Play), Name) == 0)
      return Play;
  }
  return NULL;
}

/* 
 * Forms a price based on the string representation in "buf".
 */
price_t strtoprice(char *buf)
{
  guint i, buflen, FracNum;
  gchar digit, suffix;
  gboolean minus, InFrac;
  price_t val = 0;

  minus = FALSE;
  if (!buf || !buf[0])
    return 0;

  buflen = strlen(buf);
  suffix = buf[buflen - 1];
  suffix = toupper(suffix);
  if (suffix == 'M')
    FracNum = 6;
  else if (suffix == 'K')
    FracNum = 3;
  else
    FracNum = 0;

  for (i = 0, InFrac = FALSE; i < buflen && (!InFrac || FracNum > 0); i++) {
    digit = buf[i];
    if (digit == '.' || digit == ',') {
      InFrac = TRUE;
    } else if (digit >= '0' && digit <= '9') {
      if (InFrac)
        FracNum--;
      val *= 10;
      val += (digit - '0');
    } else if (digit == '-')
      minus = TRUE;
  }

  for (i = 0; i < FracNum; i++)
    val *= 10;
  if (minus)
    val = -val;
  return val;
}

/* 
 * Prints "price" directly into a dynamically-allocated string buffer
 * and returns a pointer to this buffer. It is the responsbility of
 * the user to g_free this buffer when it is finished with.
 */
gchar *pricetostr(price_t price)
{
  GString *PriceStr;
  gchar *NewBuffer;
  price_t absprice;

  if (price < 0)
    absprice = -price;
  else
    absprice = price;
  PriceStr = g_string_new(NULL);
  while (absprice != 0) {
    g_string_prepend_c(PriceStr, '0' + (absprice % 10));
    absprice /= 10;
    if (absprice == 0) {
      if (price < 0)
        g_string_prepend_c(PriceStr, '-');
    }
  }
  NewBuffer = PriceStr->str;
  /* Free the string structure, but not the actual char array */
  g_string_free(PriceStr, FALSE);
  return NewBuffer;
}

/* 
 * Takes the number in "price" and prints it into a dynamically-allocated
 * string, adding commas to split up thousands, and adding a currency
 * symbol to the start. Returns a pointer to the string, which must be
 * g_free'd by the user when it is finished with.
 */
gchar *FormatPrice(price_t price)
{
  GString *PriceStr;
  gchar *NewBuffer;
  char thou[10];
  gboolean First = TRUE;
  price_t absprice;

  PriceStr = g_string_new(NULL);
  if (price < 0)
    absprice = -price;
  else
    absprice = price;
  while (First || absprice > 0) {
    if (absprice >= 1000)
      sprintf(thou, "%03d", (int)(absprice % 1000l));
    else
      sprintf(thou, "%d", (int)(price % 1000l));
    price /= 1000l;
    absprice /= 1000l;
    if (!First)
      g_string_prepend_c(PriceStr, ',');
    g_string_prepend(PriceStr, thou);
    First = FALSE;
  }
  if (Currency.Prefix)
    g_string_prepend(PriceStr, Currency.Symbol);
  else
    g_string_append(PriceStr, Currency.Symbol);

  NewBuffer = PriceStr->str;
  /* Free the string structure only, not the char data */
  g_string_free(PriceStr, FALSE);
  return NewBuffer;
}

/* 
 * Returns the total number of guns being carried by "Play".
 */
int TotalGunsCarried(Player *Play)
{
  int i, c;

  c = 0;
  for (i = 0; i < NumGun; i++)
    c += Play->Guns[i].Carried;
  return c;
}

/* 
 * Capitalises the first character of "string" and writes the resultant
 * string into a dynamically-allocated copy; the user must g_free this
 * string (a pointer to which is returned) when it is no longer needed.
 */
gchar *InitialCaps(gchar *string)
{
  gchar *buf;

  if (!string)
    return NULL;
  buf = g_strdup(string);
  if (strlen(buf) >= 1)
    buf[0] = toupper(buf[0]);
  return buf;
}

/* 
 * Returns TRUE if "string" starts with a vowel.
 */
char StartsWithVowel(char *string)
{
  int c;

  if (!string || strlen(string) < 1)
    return FALSE;
  c = toupper(string[0]);
  return (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U');
}

/* 
 * Reads a NULL-terminated string into the buffer "buf" from file "fp".
 * buf is sized to hold the string; this is a dynamic string and must be
 * freed by the calling routine. Returns 0 on success, EOF on failure.
 */
int read_string(FILE *fp, char **buf)
{
  int c;
  GString *text;

  text = g_string_new("");
  do {
    c = fgetc(fp);
    if (c != EOF && c != 0)
      g_string_append_c(text, (char)c);
  } while (c != EOF && c != 0);

  *buf = text->str;

  /* Free the GString, but not the actual data text->str */
  g_string_free(text, FALSE);
  if (c == EOF)
    return EOF;
  else
    return 0;
}

/* 
 * This function simply clears the given inventories "Guns"
 * and "Drugs" if they are non-NULL.
 */
void ClearInventory(Inventory *Guns, Inventory *Drugs)
{
  int i;

  if (Guns)
    for (i = 0; i < NumGun; i++) {
      Guns[i].Carried = 0;
      Guns[i].TotalValue = 0;
    }
  if (Drugs)
    for (i = 0; i < NumDrug; i++) {
      Drugs[i].Carried = 0;
      Drugs[i].TotalValue = 0;
    }
}

/* 
 * Returns TRUE only if "Guns" and "Drugs" contain no objects.
 */
char IsInventoryClear(Inventory *Guns, Inventory *Drugs)
{
  int i;

  if (Guns)
    for (i = 0; i < NumGun; i++)
      if (Guns[i].Carried > 0)
        return FALSE;
  if (Drugs)
    for (i = 0; i < NumDrug; i++)
      if (Drugs[i].Carried > 0)
        return FALSE;
  return TRUE;
}

/* 
 * Adds inventory "Add" into the contents of inventory "Cumul"
 * Each inventory is of length "Length".
 * N.B. TotalValue is not modified, as it is assumed that the
 * new items are free (if this is not the case it must be
 * handled elsewhere).
 */
void AddInventory(Inventory *Cumul, Inventory *Add, int Length)
{
  int i;

  for (i = 0; i < Length; i++)
    Cumul[i].Carried += Add[i].Carried;
}

/* 
 * Given the lists of "Guns" and "Drugs" (which the given player "Play"
 * must have sufficient room to carry) updates the player's space to
 * reflect carrying them.
 */
void ChangeSpaceForInventory(Inventory *Guns, Inventory *Drugs,
                             Player *Play)
{
  int i;

  if (Guns)
    for (i = 0; i < NumGun; i++) {
      Play->CoatSize -= Guns[i].Carried * Gun[i].Space;
    }
  if (Drugs)
    for (i = 0; i < NumDrug; i++) {
      Play->CoatSize -= Drugs[i].Carried;
    }
}

/* 
 * Discards items from "Guns" and/or "Drugs" (if non-NULL) if necessary
 * such that player "Play" is able to carry them all. The cheapest
 * objects are discarded.
 */
void TruncateInventoryFor(Inventory *Guns, Inventory *Drugs, Player *Play)
{
  int i, Total, CheapIndex;
  int CheapestGun;

  Total = 0;
  if (Guns)
    for (i = 0; i < NumGun; i++)
      Total += Guns[i].Carried;
  Total += TotalGunsCarried(Play);
  while (Guns && Total > Play->Bitches.Carried + 2) {
    CheapIndex = -1;
    for (i = 0; i < NumGun; i++)
      if (Guns[i].Carried && (CheapIndex == -1
                              || Gun[i].Price <= Gun[CheapIndex].Price)) {
        CheapIndex = i;
      }
    i = Total - Play->Bitches.Carried - 2;
    if (Guns[CheapIndex].Carried > i) {
      Guns[CheapIndex].Carried -= i;
      Total -= i;
    } else {
      Total -= Guns[CheapIndex].Carried;
      Guns[CheapIndex].Carried = 0;
    }
  }

  Total = Play->CoatSize;
  if (Guns)
    for (i = 0; i < NumGun; i++)
      Total -= Guns[i].Carried * Gun[i].Space;
  if (Drugs)
    for (i = 0; i < NumDrug; i++)
      Total -= Drugs[i].Carried;
  while (Total < 0) {
    CheapestGun = -1;
    CheapIndex = -1;
    if (Guns)
      for (i = 0; i < NumGun; i++)
        if (Guns[i].Carried && (CheapIndex == -1
                                || Gun[i].Price <= Gun[CheapIndex].Price)) {
          CheapIndex = i;
          CheapestGun = Gun[i].Price / Gun[i].Space;
        }
    if (Drugs)
      for (i = 0; i < NumDrug; i++)
        if (Drugs[i].Carried &&
            (CheapIndex == -1 ||
             (CheapestGun == -1
              && Drug[i].MinPrice <= Drug[CheapIndex].MinPrice)
             || (CheapestGun >= 0 && Drug[i].MinPrice <= CheapestGun))) {
          CheapIndex = i;
          CheapestGun = -1;
        }
    if (Guns && CheapestGun >= 0) {
      Guns[CheapIndex].Carried--;
      Total += Gun[CheapIndex].Space;
    } else {
      if (Drugs && Drugs[CheapIndex].Carried >= -Total) {
        Drugs[CheapIndex].TotalValue =
            Drugs[CheapIndex].TotalValue *
            (Drugs[CheapIndex].Carried + Total) /
            Drugs[CheapIndex].Carried;
        Drugs[CheapIndex].Carried += Total;
        Total = 0;
      } else {
        Total += Drugs[CheapIndex].Carried;
        Drugs[CheapIndex].Carried = 0;
        Drugs[CheapIndex].TotalValue = 0;
      }
    }
  }
}

/* 
 * Returns an index into the drugs array of a random drug that "Play" is
 * carrying at least "amount" of. If no suitable drug is found after 5
 * attempts, returns -1.
 */
int IsCarryingRandom(Player *Play, int amount)
{
  int i, ind;

  for (i = 0; i < 5; i++) {
    ind = brandom(0, NumDrug);
    if (Play->Drugs[ind].Carried >= amount) {
      return ind;
    }
  }
  return -1;
}

/* 
 * Returns an index into the "Drugs" array maintained by player "Play"
 * of the next available drug after "OldIndex", following the current
 * sort method (defined globally as "DrugSortMethod").
 */
int GetNextDrugIndex(int OldIndex, Player *Play)
{
  int i, MaxIndex;

  MaxIndex = -1;
  for (i = 0; i < NumDrug; i++) {
    if (Play->Drugs[i].Price != 0 && i != OldIndex && i != MaxIndex &&
        (MaxIndex == -1
         || (DrugSortMethod == DS_ATOZ
          && g_ascii_strncasecmp(Drug[MaxIndex].Name, Drug[i].Name, strlen(Drug[i].Name)) > 0)
         || (DrugSortMethod == DS_ZTOA
          && g_ascii_strncasecmp(Drug[MaxIndex].Name, Drug[i].Name, strlen(Drug[i].Name)) < 0)
         || (DrugSortMethod == DS_CHEAPFIRST
          && Play->Drugs[MaxIndex].Price > Play->Drugs[i].Price)
         || (DrugSortMethod == DS_CHEAPLAST
          && Play->Drugs[MaxIndex].Price < Play->Drugs[i].Price)) &&
        (OldIndex == -1
         || (DrugSortMethod == DS_ATOZ
          && g_ascii_strncasecmp(Drug[OldIndex].Name, Drug[i].Name, strlen(Drug[i].Name)) <= 0)
         || (DrugSortMethod == DS_ZTOA
          && g_ascii_strncasecmp(Drug[OldIndex].Name, Drug[i].Name, strlen(Drug[i].Name)) >= 0)
         || (DrugSortMethod == DS_CHEAPFIRST
          && Play->Drugs[OldIndex].Price <= Play->Drugs[i].Price)
         || (DrugSortMethod == DS_CHEAPLAST
          && Play->Drugs[OldIndex].Price >= Play->Drugs[i].Price))) {
      MaxIndex = i;
    }
  }
  return MaxIndex;
}

/* 
 * A DopeList is akin to a Vector class; it is a list of DopeEntry
 * structures, which can be dynamically extended or compressed. This
 * function initializes the newly-created list pointed to by "List"
 * (A DopeEntry contains a Player pointer and a counter, and is used
 * by the server to keep track of tipoffs and spies.)
 */
void InitList(DopeList *List)
{
  List->Data = NULL;
  List->Number = 0;
}

/* 
 * Clears the list pointed to by "List".
 */
void ClearList(DopeList *List)
{
  g_free(List->Data);
  InitList(List);
}

/* 
 * Adds a new DopeEntry (pointed to by "NewEntry") to the list "List".
 * A copy of NewEntry is placed into the list, so the original
 * structure pointed to by NewEntry can be reused.
 */
void AddListEntry(DopeList *List, DopeEntry *NewEntry)
{
  if (!NewEntry || !List)
    return;
  List->Number++;
  List->Data = (DopeEntry *)g_realloc(List->Data, List->Number *
                                      sizeof(DopeEntry));
  memmove(&(List->Data[List->Number - 1]), NewEntry, sizeof(DopeEntry));
}

/* 
 * Removes the DopeEntry at index "Index" from list "List".
 */
void RemoveListEntry(DopeList *List, int Index)
{
  if (!List || Index < 0 || Index >= List->Number)
    return;

  if (Index < List->Number - 1) {
    memmove(&(List->Data[Index]), &(List->Data[Index + 1]),
              (List->Number - 1 - Index) * sizeof(DopeEntry));
  }
  List->Number--;
  List->Data = (DopeEntry *)g_realloc(List->Data, List->Number *
                                      sizeof(DopeEntry));
  if (List->Number == 0)
    List->Data = NULL;
}

/* 
 * Returns the index of the DopeEntry matching "Play" in list "List"
 * or -1 if this is not found.
 */
int GetListEntry(DopeList *List, Player *Play)
{
  int i;

  for (i = List->Number - 1; i >= 0; i--) {
    if (List->Data[i].Play == Play)
      return i;
  }
  return -1;
}

/* 
 * Removes (if it exists) the DopeEntry in list "List" matching "Play".
 */
void RemoveListPlayer(DopeList *List, Player *Play)
{
  RemoveListEntry(List, GetListEntry(List, Play));
}

/* 
 * Similar to RemoveListPlayer, except that if the list contains "Play" more
 * than once, all the matching entries are removed, not just the first.
 */
void RemoveAllEntries(DopeList *List, Player *Play)
{
  int i;

  do {
    i = GetListEntry(List, Play);
    if (i >= 0)
      RemoveListEntry(List, i);
  } while (i >= 0);
}

void ResizeLocations(int NewNum)
{
  int i;

  if (NewNum < NumLocation)
    for (i = NewNum; i < NumLocation; i++) {
      g_free(Location[i].Name);
    }
  Location = g_realloc(Location, sizeof(struct LOCATION) * NewNum);
  if (NewNum > NumLocation) {
    memset(&Location[NumLocation], 0,
           (NewNum - NumLocation) * sizeof(struct LOCATION));
    for (i = NumLocation; i < NewNum; i++) {
      Location[i].Name = g_strdup("");
    }
  }
  NumLocation = NewNum;
}

void ResizeCops(int NewNum)
{
  int i;

  if (NewNum < NumCop)
    for (i = NewNum; i < NumCop; i++) {
      g_free(Cop[i].Name);
      g_free(Cop[i].DeputyName);
      g_free(Cop[i].DeputiesName);
    }
  Cop = g_realloc(Cop, sizeof(struct COP) * NewNum);
  if (NewNum > NumCop) {
    memset(&Cop[NumCop], 0, (NewNum - NumCop) * sizeof(struct COP));
    for (i = NumCop; i < NewNum; i++) {
      Cop[i].Name = g_strdup("");
      Cop[i].DeputyName = g_strdup("");
      Cop[i].DeputiesName = g_strdup("");
    }
  }
  NumCop = NewNum;
}

void ResizeGuns(int NewNum)
{
  int i;

  if (NewNum < NumGun)
    for (i = NewNum; i < NumGun; i++) {
      g_free(Gun[i].Name);
    }
  Gun = g_realloc(Gun, sizeof(struct GUN) * NewNum);
  if (NewNum > NumGun) {
    memset(&Gun[NumGun], 0, (NewNum - NumGun) * sizeof(struct GUN));
    for (i = NumGun; i < NewNum; i++) {
      Gun[i].Name = g_strdup("");
    }
  }
  NumGun = NewNum;
}

void ResizeDrugs(int NewNum)
{
  int i;

  if (NewNum < NumDrug)
    for (i = NewNum; i < NumDrug; i++) {
      g_free(Drug[i].Name);
      g_free(Drug[i].CheapStr);
    }
  Drug = g_realloc(Drug, sizeof(struct DRUG) * NewNum);
  if (NewNum > NumDrug) {
    memset(&Drug[NumDrug], 0, (NewNum - NumDrug) * sizeof(struct DRUG));
    for (i = NumDrug; i < NewNum; i++) {
      Drug[i].Name = g_strdup("");
      Drug[i].CheapStr = g_strdup("");
    }
  }
  NumDrug = NewNum;
}

void ResizeSubway(int NewNum)
{
  int i;

  if (NewNum < NumSubway)
    for (i = NewNum; i < NumSubway; i++) {
      g_free(SubwaySaying[i]);
    }
  SubwaySaying = g_realloc(SubwaySaying, sizeof(char *) * NewNum);
  if (NewNum > NumSubway)
    for (i = NumSubway; i < NewNum; i++) {
      SubwaySaying[i] = g_strdup("");
    }
  NumSubway = NewNum;
}

void ResizePlaying(int NewNum)
{
  int i;

  if (NewNum < NumPlaying)
    for (i = NewNum; i < NumPlaying; i++) {
      g_free(Playing[i]);
    }
  Playing = g_realloc(Playing, sizeof(char *) * NewNum);
  if (NewNum > NumPlaying)
    for (i = NumPlaying; i < NewNum; i++) {
      Playing[i] = g_strdup("");
    }
  NumPlaying = NewNum;
}

void ResizeStoppedTo(int NewNum)
{
  int i;

  if (NewNum < NumStoppedTo)
    for (i = NewNum; i < NumStoppedTo; i++) {
      g_free(StoppedTo[i]);
    }
  StoppedTo = g_realloc(StoppedTo, sizeof(char *) * NewNum);
  if (NewNum > NumStoppedTo)
    for (i = NumStoppedTo; i < NewNum; i++) {
      StoppedTo[i] = g_strdup("");
    }
  NumStoppedTo = NewNum;
}

/* 
 * Sets the dynamically-sized string pointed to by *dest to a copy of
 * "src" - src can safely be freed or reused afterwards. Any existing
 * string in "dest" is freed. The function returns immediately if src
 * and *dest are already the same.
 */
void AssignName(gchar **dest, gchar *src)
{
  if (*dest == src)
    return;
  g_free(*dest);
  *dest = g_strdup(src);
}

void CopyNames(struct NAMES *dest, struct NAMES *src)
{
  AssignName(&dest->Bitch, _(src->Bitch));
  AssignName(&dest->Bitches, _(src->Bitches));
  AssignName(&dest->Gun, _(src->Gun));
  AssignName(&dest->Guns, _(src->Guns));
  AssignName(&dest->Drug, _(src->Drug));
  AssignName(&dest->Drugs, _(src->Drugs));
  AssignName(&dest->Date, _(src->Date));
  AssignName(&dest->LoanSharkName, _(src->LoanSharkName));
  AssignName(&dest->BankName, _(src->BankName));
  AssignName(&dest->GunShopName, _(src->GunShopName));
  AssignName(&dest->RoughPubName, _(src->RoughPubName));
}

#ifdef NETWORKING
void CopyMetaServer(struct METASERVER *dest, struct METASERVER *src)
{
  dest->Active = src->Active;
  AssignName(&dest->URL, src->URL);
  AssignName(&dest->LocalName, src->LocalName);
  AssignName(&dest->Password, src->Password);
  AssignName(&dest->Comment, src->Comment);
}
#endif

void CopyLocation(struct LOCATION *dest, struct LOCATION *src)
{
  AssignName(&dest->Name, _(src->Name));
  dest->PolicePresence = src->PolicePresence;
  dest->MinDrug = src->MinDrug;
  dest->MaxDrug = src->MaxDrug;
}

void CopyCop(struct COP *dest, struct COP *src)
{
  AssignName(&dest->Name, _(src->Name));
  AssignName(&dest->DeputyName, _(src->DeputyName));
  AssignName(&dest->DeputiesName, _(src->DeputiesName));
  dest->Armor = src->Armor;
  dest->DeputyArmor = src->DeputyArmor;
  dest->AttackPenalty = src->AttackPenalty;
  dest->DefendPenalty = src->DefendPenalty;
  dest->MinDeputies = src->MinDeputies;
  dest->MaxDeputies = src->MaxDeputies;
  dest->GunIndex = src->GunIndex;
  dest->CopGun = src->CopGun;
  dest->DeputyGun = src->DeputyGun;
}

void CopyGun(struct GUN *dest, struct GUN *src)
{
  AssignName(&dest->Name, _(src->Name));
  dest->Price = src->Price;
  dest->Space = src->Space;
  dest->Damage = src->Damage;
}

void CopyDrug(struct DRUG *dest, struct DRUG *src)
{
  AssignName(&dest->Name, src->Name[0] ? _(src->Name) : "");
  dest->MinPrice = src->MinPrice;
  dest->MaxPrice = src->MaxPrice;
  dest->Cheap = src->Cheap;
  dest->Expensive = src->Expensive;
  AssignName(&dest->CheapStr, src->CheapStr[0] ? _(src->CheapStr) : "");
}

void CopyDrugs(struct DRUGS *dest, struct DRUGS *src)
{
  AssignName(&dest->ExpensiveStr1, _(src->ExpensiveStr1));
  AssignName(&dest->ExpensiveStr2, _(src->ExpensiveStr2));
  dest->CheapDivide = src->CheapDivide;
  dest->ExpensiveMultiply = src->ExpensiveMultiply;
}

static struct PRICES BackupPrices;
static struct NAMES BackupNames;
static struct DRUG *BackupDrug = NULL;
static struct GUN *BackupGun = NULL;
static struct LOCATION *BackupLocation = NULL;
static struct CURRENCY BackupCurrency = { NULL, TRUE };
static gint NumBackupDrug = 0, NumBackupGun = 0, NumBackupLocation = 0;

void BackupConfig(void)
{
  gint i;

  BackupPrices.Spy = Prices.Spy;
  BackupPrices.Tipoff = Prices.Tipoff;
  AssignName(&BackupCurrency.Symbol, Currency.Symbol);
  BackupCurrency.Prefix = Currency.Prefix;
  CopyNames(&BackupNames, &Names);

  /* Free existing backups of guns, drugs, and locations */
  for (i = 0; i < NumBackupGun; i++)
    g_free(BackupGun[i].Name);
  g_free(BackupGun);
  for (i = 0; i < NumBackupDrug; i++) {
    g_free(BackupDrug[i].Name);
    g_free(BackupDrug[i].CheapStr);
  }
  g_free(BackupDrug);
  for (i = 0; i < NumBackupLocation; i++)
    g_free(BackupLocation[i].Name);
  g_free(BackupLocation);

  NumBackupGun = NumGun;
  BackupGun = g_new0(struct GUN, NumGun);

  for (i = 0; i < NumGun; i++)
    CopyGun(&BackupGun[i], &Gun[i]);

  NumBackupDrug = NumDrug;
  BackupDrug = g_new0(struct DRUG, NumDrug);

  for (i = 0; i < NumDrug; i++)
    CopyDrug(&BackupDrug[i], &Drug[i]);

  NumBackupLocation = NumLocation;
  BackupLocation = g_new0(struct LOCATION, NumLocation);

  for (i = 0; i < NumLocation; i++)
    CopyLocation(&BackupLocation[i], &Location[i]);
}

void RestoreConfig(void)
{
  gint i;

  Prices.Spy = BackupPrices.Spy;
  Prices.Tipoff = BackupPrices.Tipoff;
  CopyNames(&Names, &BackupNames);
  AssignName(&Currency.Symbol, BackupCurrency.Symbol);
  Currency.Prefix = BackupCurrency.Prefix;

  ResizeGuns(NumBackupGun);
  for (i = 0; i < NumGun; i++)
    CopyGun(&Gun[i], &BackupGun[i]);
  ResizeDrugs(NumBackupDrug);
  for (i = 0; i < NumDrug; i++)
    CopyDrug(&Drug[i], &BackupDrug[i]);
  ResizeLocations(NumBackupLocation);
  for (i = 0; i < NumLocation; i++)
    CopyLocation(&Location[i], &BackupLocation[i]);
}

void ScannerErrorHandler(GScanner *scanner, gchar *msg, gint error)
{
  g_print("%s\n", msg);
}

/*
 * On Windows systems, check the current config file referenced by "scanner"
 * for a UTF-8 header. If one is found, "conv" and "encoding" are set
 * for UTF-8 encoding.
 */
static void CheckConfigHeader(GScanner *scanner, Converter *conv,
                              gchar **encoding)
{
#ifdef CYGWIN
  GTokenType token;

  token = g_scanner_peek_next_token(scanner);
  if (token == (guchar)'\357') {
    /* OK; assume this is a Windows-style \357 \273 \277 UTF-8 header */
    if (g_scanner_get_next_token(scanner) != (guchar)'\357'
        || g_scanner_get_next_token(scanner) != (guchar)'\273'
        || g_scanner_get_next_token(scanner) != (guchar)'\277') {
      return;
    }
    Conv_SetCodeset(conv, "UTF-8");
    if (encoding) {
      g_free(*encoding);
      *encoding = g_strdup("UTF-8");
    }
  }
#endif
}

/* 
 * Read a configuration file given by "FileName"
 */
static gboolean ReadConfigFile(char *FileName, gchar **encoding)
{
  FILE *fp;
  Converter *conv;

  GScanner *scanner;

  fp = fopen(FileName, "r");
  if (fp) {
    conv = Conv_New();
    if (encoding) {
      *encoding = NULL;
    }
    scanner = g_scanner_new(&ScannerConfig);
    scanner->input_name = FileName;
    scanner->msg_handler = ScannerErrorHandler;
    g_scanner_input_file(scanner, fileno(fp));
    CheckConfigHeader(scanner, conv, encoding);
    while (!g_scanner_eof(scanner)) {
      if (!ParseNextConfig(scanner, conv, encoding, FALSE)) {
        ConfigErrors++;
        g_scanner_error(scanner,
                        _("Unable to process configuration file %s, line %d"),
                        FileName, g_scanner_cur_line(scanner));
      }
    }
    g_scanner_destroy(scanner);
    Conv_Free(conv);
    fclose(fp);
    return TRUE;
  } else {
    return FALSE;
  }
}

gboolean ParseNextConfig(GScanner *scanner, Converter *conv,
                         gchar **encoding, gboolean print)
{
  GTokenType token;
  gchar *ID1, *ID2;
  gulong ind = 0;
  int GlobalIndex;
  gboolean IndexGiven = FALSE;

  ID1 = ID2 = NULL;
  token = g_scanner_get_next_token(scanner);
  if (token == G_TOKEN_EOF)
    return TRUE;
  if (token != G_TOKEN_IDENTIFIER) {
    g_scanner_unexp_token(scanner, G_TOKEN_IDENTIFIER, NULL, NULL,
                          NULL, NULL, FALSE);
    return FALSE;
  }

  if (g_ascii_strncasecmp(scanner->value.v_identifier, "include", 7) == 0) {
    token = g_scanner_get_next_token(scanner);
    if (token == G_TOKEN_STRING) {
      if (!ReadConfigFile(scanner->value.v_string, NULL)) {
        g_scanner_error(scanner, _("Unable to open file %s"),
                        scanner->value.v_string);
      }
      return TRUE;
    } else {
      g_scanner_unexp_token(scanner, G_TOKEN_STRING, NULL, NULL,
                            NULL, NULL, FALSE);
      return FALSE;
    }
  } else if (g_ascii_strncasecmp(scanner->value.v_identifier, "encoding", 8) == 0) {
    token = g_scanner_get_next_token(scanner);
    if (token == G_TOKEN_STRING) {
      Conv_SetCodeset(conv, scanner->value.v_string);
      if (encoding) {
	g_free(*encoding);
	*encoding = g_strdup(scanner->value.v_string);
      }
      return TRUE;
    } else {
      g_scanner_unexp_token(scanner, G_TOKEN_STRING, NULL, NULL,
                            NULL, NULL, FALSE);
      return FALSE;
    }
  }

  ID1 = g_strdup(scanner->value.v_identifier);
  token = g_scanner_get_next_token(scanner);
  if (token == G_TOKEN_LEFT_BRACE) {
    token = g_scanner_get_next_token(scanner);
    if (token != G_TOKEN_INT) {
      g_scanner_unexp_token(scanner, G_TOKEN_INT, NULL, NULL,
                            NULL, NULL, FALSE);
      return FALSE;
    }
    ind = scanner->value.v_int;
    IndexGiven = TRUE;
    token = g_scanner_get_next_token(scanner);
    if (token != G_TOKEN_RIGHT_BRACE) {
      g_scanner_unexp_token(scanner, G_TOKEN_RIGHT_BRACE, NULL, NULL,
                            NULL, NULL, FALSE);
      return FALSE;
    }
    token = g_scanner_get_next_token(scanner);
    if (token == '.') {
      token = g_scanner_get_next_token(scanner);
      if (token != G_TOKEN_IDENTIFIER) {
        g_scanner_unexp_token(scanner, G_TOKEN_IDENTIFIER, NULL, NULL,
                              NULL, NULL, FALSE);
        return FALSE;
      }
      ID2 = g_strdup(scanner->value.v_identifier);
      token = g_scanner_get_next_token(scanner);
    }
  }
  GlobalIndex = GetGlobalIndex(ID1, ID2);
  g_free(ID1);
  g_free(ID2);
  if (GlobalIndex == -1)
    return FALSE;
  if (token == G_TOKEN_EOF) {
    PrintConfigValue(GlobalIndex, (int)ind, IndexGiven, scanner);
    return TRUE;
  } else if (token == G_TOKEN_EQUAL_SIGN) {
    if (CountPlayers(FirstServer) > 0) {
      g_warning(_("Configuration can only be changed interactively "
                  "when no\nplayers are logged on. Wait for all "
                  "players to log off, or remove\nthem with the "
                  "push or kill commands, and try again."));
    } else {
      if (SetConfigValue(GlobalIndex, (int)ind, IndexGiven, conv, scanner)
          && print) {
        PrintConfigValue(GlobalIndex, (int)ind, IndexGiven, scanner);
      }
    }
    return TRUE;
  } else {
    return FALSE;
  }
  return FALSE;
}

int GetGlobalIndex(gchar *ID1, gchar *ID2)
{
  int i;
  const int NumGlob = sizeof(Globals) / sizeof(Globals[0]);

  if (!ID1)
    return -1;
  for (i = 0; i < NumGlob; i++) {
    if (!ID2 && !Globals[i].NameStruct[0]
        && g_ascii_strcasecmp(ID1, Globals[i].Name) == 0) {
      /* Just a bog-standard ID1=value */
      return i;
    }
    if (g_ascii_strcasecmp(ID1, Globals[i].NameStruct) == 0 && ID2
        && g_ascii_strcasecmp(ID2, Globals[i].Name) == 0
        && Globals[i].StructStaticPt && Globals[i].StructListPt) {
      /* ID1[index].ID2=value */
      return i;
    }
  }
  return -1;
}

static void *GetGlobalPointer(int GlobalIndex, int StructIndex)
{
  void *ValPt = NULL;

  if (Globals[GlobalIndex].IntVal) {
    ValPt = (void *)Globals[GlobalIndex].IntVal;
  } else if (Globals[GlobalIndex].PriceVal) {
    ValPt = (void *)Globals[GlobalIndex].PriceVal;
  } else if (Globals[GlobalIndex].BoolVal) {
    ValPt = (void *)Globals[GlobalIndex].BoolVal;
  } else if (Globals[GlobalIndex].StringVal) {
    ValPt = (void *)Globals[GlobalIndex].StringVal;
  }
  if (!ValPt)
    return NULL;

  if (Globals[GlobalIndex].StructStaticPt &&
      Globals[GlobalIndex].StructListPt) {
    return (char *)ValPt - (char *)Globals[GlobalIndex].StructStaticPt +
        (char *)*(Globals[GlobalIndex].StructListPt) +
        (StructIndex - 1) * Globals[GlobalIndex].LenStruct;
  } else {
    return ValPt;
  }
}

gchar **GetGlobalString(int GlobalIndex, int StructIndex)
{
  g_assert(Globals[GlobalIndex].StringVal);

  return (gchar **)GetGlobalPointer(GlobalIndex, StructIndex);
}

gint *GetGlobalInt(int GlobalIndex, int StructIndex)
{
  g_assert(Globals[GlobalIndex].IntVal);

  return (gint *)GetGlobalPointer(GlobalIndex, StructIndex);
}

price_t *GetGlobalPrice(int GlobalIndex, int StructIndex)
{
  g_assert(Globals[GlobalIndex].PriceVal);

  return (price_t *)GetGlobalPointer(GlobalIndex, StructIndex);
}

gboolean *GetGlobalBoolean(int GlobalIndex, int StructIndex)
{
  g_assert(Globals[GlobalIndex].BoolVal);

  return (gboolean *)GetGlobalPointer(GlobalIndex, StructIndex);
}

gchar ***GetGlobalStringList(int GlobalIndex, int StructIndex)
{
  g_assert(Globals[GlobalIndex].StringList);

  return (gchar ***)GetGlobalPointer(GlobalIndex, StructIndex);
}

gboolean CheckMaxIndex(GScanner *scanner, int GlobalIndex, int StructIndex,
                       gboolean IndexGiven)
{
  if (!Globals[GlobalIndex].MaxIndex ||
      (Globals[GlobalIndex].StringList && !IndexGiven) ||
      (IndexGiven && StructIndex >= 1 &&
       StructIndex <= *(Globals[GlobalIndex].MaxIndex))) {
    return TRUE;
  }
  /* Error message displayed when you try to set, for example,
   * Drug[10].Name when NumDrug<10 (%s="Drug" and %d=10 in this example) */
  g_scanner_error(scanner,
                  _("Index into %s array should be between 1 and %d"),
                  (Globals[GlobalIndex].NameStruct
                   && Globals[GlobalIndex].
                   NameStruct[0]) ? Globals[GlobalIndex].
                  NameStruct : Globals[GlobalIndex].Name,
                  *(Globals[GlobalIndex].MaxIndex));
  return FALSE;
}

void PrintConfigValue(int GlobalIndex, int StructIndex,
                      gboolean IndexGiven, GScanner *scanner)
{
  gchar *GlobalName;
  int i;

  if (!CheckMaxIndex(scanner, GlobalIndex, StructIndex, IndexGiven))
    return;
  if (Globals[GlobalIndex].NameStruct[0]) {
    GlobalName =
        g_strdup_printf("%s[%d].%s", Globals[GlobalIndex].NameStruct,
                        StructIndex, Globals[GlobalIndex].Name);
  } else
    GlobalName = Globals[GlobalIndex].Name;
  if (Globals[GlobalIndex].IntVal) {
    /* Display of a numeric config. file variable - e.g. "NumDrug is 6" */
    g_print(_("%s is %d\n"), GlobalName,
            *GetGlobalInt(GlobalIndex, StructIndex));
  } else if (Globals[GlobalIndex].BoolVal) {
    /* Display of a boolean config. file variable - e.g. "DrugValue is
       TRUE" */
    g_print(_("%s is %s\n"), GlobalName,
            *GetGlobalBoolean(GlobalIndex, StructIndex) ?
            "TRUE" : "FALSE");
  } else if (Globals[GlobalIndex].PriceVal) {
    /* Display of a price config. file variable - e.g. "Bitch.MinPrice is
       $200" */
    dpg_print(_("%s is %P\n"), GlobalName,
              *GetGlobalPrice(GlobalIndex, StructIndex));
  } else if (Globals[GlobalIndex].StringVal) {
    /* Display of a string config. file variable - e.g. "LoanSharkName is
       \"the loan shark\"" */
    g_print(_("%s is \"%s\"\n"), GlobalName,
            *GetGlobalString(GlobalIndex, StructIndex));
  } else if (Globals[GlobalIndex].StringList) {
    if (IndexGiven) {
      /* Display of an indexed string list config. file variable - e.g.
         "StoppedTo[1] is have a beer" */
      g_print(_("%s[%d] is %s\n"), GlobalName, StructIndex,
              (*(Globals[GlobalIndex].StringList))[StructIndex - 1]);
    } else {
      GString *text;

      text = g_string_new("");
      /* Display of the first part of an entire string list config. file
         variable - e.g. "StoppedTo is { " (followed by "have a beer",
         "smoke a joint" etc.) */
      g_string_printf(text, _("%s is { "), GlobalName);
      if (Globals[GlobalIndex].MaxIndex) {
        for (i = 0; i < *(Globals[GlobalIndex].MaxIndex); i++) {
          if (i > 0)
            g_string_append(text, ", ");
          g_string_append_printf(text, "\"%s\"",
                            (*(Globals[GlobalIndex].StringList))[i]);
        }
      }
      g_string_append(text, " }\n");

      g_print("%s", text->str);
      g_string_free(text, TRUE);
    }
  }
  if (Globals[GlobalIndex].NameStruct[0])
    g_free(GlobalName);
}

static gboolean SetConfigValue(int GlobalIndex, int StructIndex,
                               gboolean IndexGiven, Converter *conv,
                               GScanner *scanner)
{
  gchar *GlobalName, *tmpstr;
  GTokenType token;
  int IntVal, NewNum;
  Player *tmp;
  GSList *list, *StartList;
  gboolean parsed;

  if (!CheckMaxIndex(scanner, GlobalIndex, StructIndex, IndexGiven))
    return FALSE;
  if (Globals[GlobalIndex].NameStruct[0]) {
    GlobalName =
        g_strdup_printf("%s[%d].%s", Globals[GlobalIndex].NameStruct,
                        StructIndex, Globals[GlobalIndex].Name);
  } else {
    GlobalName = Globals[GlobalIndex].Name;
  }
  if (Globals[GlobalIndex].IntVal) {
    gboolean minus = FALSE;

    /* GScanner doesn't understand negative numbers, so we need to
     * explicitly check for a prefixed minus sign */
    token = g_scanner_get_next_token(scanner);
    if (token == '-') {
      minus = TRUE;
      token = g_scanner_get_next_token(scanner);
    }
    if (token == G_TOKEN_INT) {
      IntVal = (int)scanner->value.v_int;
      if (minus) {
        IntVal = -IntVal;
      }
      if (IntVal < Globals[GlobalIndex].MinVal) {
        g_scanner_warn(scanner, _("%s can be no smaller than %d - ignoring!"),
                       GlobalName, Globals[GlobalIndex].MinVal);
        return FALSE;
      }
      if (Globals[GlobalIndex].MaxVal > Globals[GlobalIndex].MinVal
          && IntVal > Globals[GlobalIndex].MaxVal) {
        g_scanner_warn(scanner, _("%s can be no larger than %d - ignoring!"),
                       GlobalName, Globals[GlobalIndex].MaxVal);
        return FALSE;
      }
      if (Globals[GlobalIndex].ResizeFunc) {
        (*(Globals[GlobalIndex].ResizeFunc)) (IntVal);
        /* Displayed, for example, when you set NumDrug=10 to allow
           Drug[10].Name etc. to be set */
        if (ConfigVerbose)
          g_print(_("Resized structure list to %d elements\n"), IntVal);
        for (list = FirstClient; list; list = g_slist_next(list)) {
          tmp = (Player *)list->data;
          UpdatePlayer(tmp);
        }
        for (list = FirstServer; list; list = g_slist_next(list)) {
          tmp = (Player *)list->data;
          UpdatePlayer(tmp);
        }
      }
      *GetGlobalInt(GlobalIndex, StructIndex) = IntVal;
    } else {
      g_scanner_unexp_token(scanner, G_TOKEN_INT, NULL, NULL,
                            NULL, NULL, FALSE);
      return FALSE;
    }
  } else if (Globals[GlobalIndex].BoolVal) {
    scanner->config->cset_identifier_first =
        G_CSET_a_2_z "01" G_CSET_A_2_Z;
    scanner->config->cset_identifier_nth = G_CSET_a_2_z G_CSET_A_2_Z;
    token = g_scanner_get_next_token(scanner);
    scanner->config->cset_identifier_first = G_CSET_a_2_z "_" G_CSET_A_2_Z;
    scanner->config->cset_identifier_nth =
        G_CSET_a_2_z "._0123456789" G_CSET_A_2_Z;
    parsed = FALSE;
    if (token == G_TOKEN_IDENTIFIER) {
      if (g_ascii_strncasecmp(scanner->value.v_identifier, "TRUE", 4) == 0 ||
          strcmp(scanner->value.v_identifier, "1") == 0) {
        parsed = TRUE;
        *GetGlobalBoolean(GlobalIndex, StructIndex) = TRUE;
      } else if (g_ascii_strncasecmp(scanner->value.v_identifier, "FALSE", 5) == 0
                 || strcmp(scanner->value.v_identifier, "0") == 0) {
        parsed = TRUE;
        *GetGlobalBoolean(GlobalIndex, StructIndex) = FALSE;
      }
    }
    if (!parsed) {
      g_scanner_unexp_token(scanner, G_TOKEN_NONE, NULL, NULL, NULL,
                            _("expected a boolean value (one of 0, FALSE, "
                              "1, TRUE)"), FALSE);
      return FALSE;
    }
  } else if (Globals[GlobalIndex].PriceVal) {
    token = g_scanner_get_next_token(scanner);
    if (token == G_TOKEN_INT) {
      *GetGlobalPrice(GlobalIndex, StructIndex) =
          (price_t)scanner->value.v_int;
    } else {
      g_scanner_unexp_token(scanner, G_TOKEN_INT, NULL, NULL,
                            NULL, NULL, FALSE);
      return FALSE;
    }
  } else if (Globals[GlobalIndex].StringVal) {
    scanner->config->identifier_2_string = TRUE;
    scanner->config->cset_identifier_first =
        G_CSET_a_2_z "._0123456789" G_CSET_A_2_Z G_CSET_LATINS
        G_CSET_LATINC;
    scanner->config->cset_identifier_nth =
        G_CSET_a_2_z " ._0123456789" G_CSET_A_2_Z G_CSET_LATINS
        G_CSET_LATINC;
    token = g_scanner_get_next_token(scanner);
    if (token == G_TOKEN_STRING) {
      tmpstr = Conv_ToInternal(conv, scanner->value.v_string, -1);
      AssignName(GetGlobalString(GlobalIndex, StructIndex), tmpstr);
      g_free(tmpstr);
    } else if (token == G_TOKEN_IDENTIFIER) {
      tmpstr = Conv_ToInternal(conv, scanner->value.v_identifier, -1);
      AssignName(GetGlobalString(GlobalIndex, StructIndex), tmpstr);
      g_free(tmpstr);
    } else {
      g_scanner_unexp_token(scanner, G_TOKEN_STRING, NULL, NULL,
                            NULL, NULL, FALSE);
    }
    scanner->config->identifier_2_string = FALSE;
    scanner->config->cset_identifier_first = G_CSET_a_2_z "_" G_CSET_A_2_Z;
    scanner->config->cset_identifier_nth =
        G_CSET_a_2_z "._0123456789" G_CSET_A_2_Z;
  } else if (Globals[GlobalIndex].StringList) {
    token = g_scanner_get_next_token(scanner);
    if (IndexGiven) {
      if (token == G_TOKEN_STRING) {
        tmpstr = Conv_ToInternal(conv, scanner->value.v_string, -1);
        AssignName(&(*(Globals[GlobalIndex].StringList))[StructIndex - 1],
                   tmpstr);
        g_free(tmpstr);
      } else {
        g_scanner_unexp_token(scanner, G_TOKEN_STRING, NULL, NULL,
                              NULL, NULL, FALSE);
        return FALSE;
      }
    } else {
      StartList = NULL;
      if (token != G_TOKEN_LEFT_CURLY) {
        g_scanner_unexp_token(scanner, G_TOKEN_LEFT_CURLY, NULL, NULL,
                              NULL, NULL, FALSE);
        return FALSE;
      }
      NewNum = 0;
      do {
        token = g_scanner_get_next_token(scanner);
        if (token == G_TOKEN_STRING) {
          tmpstr = g_strdup(scanner->value.v_string);
          NewNum++;
          StartList = g_slist_append(StartList, tmpstr);
        } else if (token != G_TOKEN_RIGHT_CURLY && token != G_TOKEN_COMMA) {
          g_scanner_unexp_token(scanner, G_TOKEN_STRING, NULL, NULL,
                                NULL, NULL, FALSE);
          return FALSE;
        }
      } while (token != G_TOKEN_RIGHT_CURLY);
      (*Globals[GlobalIndex].ResizeFunc) (NewNum);
      NewNum = 0;
      for (list = StartList; list; NewNum++, list = g_slist_next(list)) {
        AssignName(&(*(Globals[GlobalIndex].StringList))[NewNum],
                   (char *)list->data);
        g_free(list->data);
      }
      g_slist_free(StartList);
    }
  }
  if (Globals[GlobalIndex].NameStruct[0])
    g_free(GlobalName);

  Globals[GlobalIndex].Modified = TRUE;
  return TRUE;
}

/*
 * Returns the URL of the directory containing local HTML documentation.
 */
gchar *GetDocRoot(void)
{
  gchar *path;
#ifdef CYGWIN
  gchar *bindir;

  bindir = GetBinaryDir();
  path = g_strdup_printf("file://%s/doc/", bindir);
  g_free(bindir);
#else
  path = g_strdup_printf("file://%s/", DPDOCDIR);
#endif
  return path;
}

/*
 * Returns the URL of the index file for the local HTML documentation.
 */
gchar *GetDocIndex(void)
{
  gchar *file, *root;

  root = GetDocRoot();
  file = g_strdup_printf("%sindex.html", root);
  g_free(root);
  return file;
}

#ifdef CYGWIN
extern gchar *appdata_path;
#endif

/*
 * Returns the pathname of the global (all users) configuration file,
 * as a dynamically-allocated string that must be later freed. On
 * error, NULL is returned.
 */
gchar *GetGlobalConfigFile(void)
{
#ifdef CYGWIN
  gchar *bindir, *conf = NULL;

  /* Global configuration is in the same directory as the dopewars binary */
  bindir = GetBinaryDir();
  if (bindir) {
    conf = g_strdup_printf("%s/dopewars-config.txt", bindir);
    g_free(bindir);
  }
  return conf;
#else
  return g_strdup("/etc/dopewars");
#endif
}

/*
 * Returns the pathname of the local (per-user) configuration file,
 * as a dynamically-allocated string that must be later freed. On
 * error, NULL is returned.
 */
gchar *GetLocalConfigFile(void)
{
#ifdef CYGWIN
  return g_strdup_printf("%s/dopewars-config.txt",
                         appdata_path ? appdata_path : ".");
#else
  gchar *home, *conf = NULL;

  /* Local config is in the user's home directory */
  home = getenv("HOME");
  if (home) {
    conf = g_strdup_printf("%s/.dopewars", home);
  }
  return conf;
#endif
}

/* 
 * Sets up data - such as the location of the high score file - to
 * hard-coded internal values, and then processes the global and
 * user-specific configuration files.
 */
static void SetupParameters(GSList *extraconfigs, gboolean antique)
{
  gchar *conf;
  GSList *list;
  int i, defloc;

  DrugValue = TRUE;
  Sanitized = ConfigVerbose = FALSE;

  g_free(Currency.Symbol);
  /* The currency symbol */
  Currency.Symbol = g_strdup(_("$"));
  Currency.Prefix = (strcmp("Currency.Prefix=TRUE",
  /* Translate this to "Currency.Prefix=FALSE" if you want your currency
     symbol to follow all prices. */
                            _("Currency.Prefix=TRUE")) == 0);

  /* Set hard-coded default values */
  AssignName(&ServerName, "localhost");
  AssignName(&ServerMOTD, "");
  AssignName(&BindAddress, "");
  AssignName(&OurWebBrowser, "/usr/bin/firefox");

  AssignName(&Sounds.FightHit, SNDPATH"colt.wav");
  AssignName(&Sounds.EnemyBitchKilled, SNDPATH"shotdown.wav");
  AssignName(&Sounds.BitchKilled, SNDPATH"losebitch.wav");
  AssignName(&Sounds.EnemyKilled, SNDPATH"shotdown.wav");
  AssignName(&Sounds.Killed, SNDPATH"die.wav");
  AssignName(&Sounds.EnemyFlee, SNDPATH"run.wav");
  AssignName(&Sounds.Flee, SNDPATH"run.wav");
  AssignName(&Sounds.Jet, SNDPATH"train.wav");
  AssignName(&Sounds.TalkPrivate, SNDPATH"murmur.wav");
  AssignName(&Sounds.TalkToAll, SNDPATH"message.wav");
  AssignName(&Sounds.EndGame, SNDPATH"bye.wav");

  LoanSharkLoc = DEFLOANSHARK;
  BankLoc = DEFBANK;
  if (antique) {
    GunShopLoc = RoughPubLoc = 0;
  } else {
    GunShopLoc = DEFGUNSHOP;
    RoughPubLoc = DEFROUGHPUB;
  }

  CopyNames(&Names, &DefaultNames);
  CopyDrugs(&Drugs, &DefaultDrugs);

#ifdef NETWORKING
  CopyMetaServer(&MetaServer, &DefaultMetaServer);
  AssignName(&Socks.name, "socks");
  Socks.port = 1080;
  Socks.version = 4;
  g_free(Socks.user);
  g_free(Socks.authuser);
  g_free(Socks.authpassword);
  Socks.user = g_strdup("");
  Socks.numuid = FALSE;
  Socks.authuser = g_strdup("");
  Socks.authpassword = g_strdup("");
  UseSocks = FALSE;
#endif

  defloc = sizeof(DefaultLocation) / sizeof(DefaultLocation[0]);
  g_assert(defloc >= 6);
  if (antique) {
    defloc = 6;
  }
  ResizeLocations(defloc);
  for (i = 0; i < NumLocation; i++)
    CopyLocation(&Location[i], &DefaultLocation[i]);
  ResizeCops(sizeof(DefaultCop) / sizeof(DefaultCop[0]));
  for (i = 0; i < NumCop; i++)
    CopyCop(&Cop[i], &DefaultCop[i]);
  ResizeGuns(sizeof(DefaultGun) / sizeof(DefaultGun[0]));
  for (i = 0; i < NumGun; i++)
    CopyGun(&Gun[i], &DefaultGun[i]);
  ResizeDrugs(sizeof(DefaultDrug) / sizeof(DefaultDrug[0]));
  for (i = 0; i < NumDrug; i++)
    CopyDrug(&Drug[i], &DefaultDrug[i]);
  ResizeSubway(sizeof(DefaultSubwaySaying) /
               sizeof(DefaultSubwaySaying[0]));
  for (i = 0; i < NumSubway; i++) {
    AssignName(&SubwaySaying[i], _(DefaultSubwaySaying[i]));
  }
  ResizePlaying(sizeof(DefaultPlaying) / sizeof(DefaultPlaying[0]));
  for (i = 0; i < NumPlaying; i++) {
    AssignName(&Playing[i], _(DefaultPlaying[i]));
  }
  ResizeStoppedTo(sizeof(DefaultStoppedTo) / sizeof(DefaultStoppedTo[0]));
  for (i = 0; i < NumStoppedTo; i++) {
    AssignName(&StoppedTo[i], _(DefaultStoppedTo[i]));
  }

  /* Replace nasty null pointers with null strings */
  for (i = 0; i < NUMGLOB; ++i) {
    if (Globals[i].StringVal && !*Globals[i].StringVal) {
      *Globals[i].StringVal = g_strdup("");
    }
  }

  /* Now read in the global configuration file */
  conf = GetGlobalConfigFile();
  if (conf) {
    ReadConfigFile(conf, NULL);
    g_free(conf);
  }

  /* Next, try the local configuration file */
  conf = GetLocalConfigFile();
  if (conf) {
    ReadConfigFile(conf, &LocalCfgEncoding);
    g_free(conf);
  }

  /* Finally, any configuration files named on the command line */
  for (list = extraconfigs; list; list = g_slist_next(list)) {
    ReadConfigFile(list->data, NULL);
  }
}

void GetDateString(GString *str, Player *Play)
{
  gchar buf[200], *turn, *pt;

  turn = g_strdup_printf("%d", Play->Turn);
  g_string_assign(str, Names.Date);
  while ((pt = strstr(str->str, "%T")) != NULL) {
    int ind = pt - str->str;

    g_string_erase(str, ind, 2);
    g_string_insert(str, ind, turn);
  }

  g_date_strftime(buf, sizeof(buf), str->str, Play->date);
  g_string_assign(str, buf);
  g_free(turn);
}

static void PluginHelp(void)
{
  gchar *plugins;
#ifdef HAVE_GETOPT_LONG
  g_print(_("  -u, --plugin=FILE       use sound plugin \"FILE\"\n"
            "                            "));
#else
  g_print(_("  -u file  use sound plugin \"file\"\n"
            "	          "));
#endif
  plugins = GetPluginList();
  g_print(_("(%s available)\n"), plugins);
  g_free(plugins);
}

void HandleHelpTexts(gboolean fullhelp)
{
  g_print(_("dopewars version %s\n"), VERSION);
  if (!fullhelp) {
    return;
  }

  g_print(
#ifdef HAVE_GETOPT_LONG
           /* Usage information, printed when the user runs "dopewars -h"
              (version with support for GNU long options) */
           _("Usage: dopewars [OPTION]...\n\
Drug dealing game based on \"Drug Wars\" by John E. Dell\n\
  -b, --no-color,         \"black and white\" - i.e. do not use pretty colors\n\
      --no-colour           (by default colors are used where available)\n\
  -n, --single-player     be boring and don't connect to any available dopewars\n\
                            servers (i.e. single player mode)\n\
  -a, --antique           \"antique\" dopewars - keep as closely to the original\n\
                            version as possible (no networking)\n\
  -f, --scorefile=FILE    specify a file to use as the high score table (by\n\
                            default %s/dopewars.sco is used)\n\
  -o, --hostname=ADDR     specify a hostname where the server for multiplayer\n\
                            dopewars can be found\n\
  -s, --public-server     run in server mode (note: see the -A option for\n\
                            configuring a server once it\'s running)\n\
  -S, --private-server    run a \"private\" server (do not notify the metaserver)\n\
  -p, --port=PORT         specify the network port to use (default: 7902)\n\
  -g, --config-file=FILE  specify the pathname of a dopewars configuration file;\n\
                            this file is read immediately when the -g option\n\
                            is encountered\n\
  -r, --pidfile=FILE      maintain pid file \"FILE\" while running the server\n\
  -l, --logfile=FILE      write log information to \"FILE\"\n\
  -A, --admin             connect to a locally-running server for administration\n\
  -c, --ai-player         create and run a computer player\n\
  -w, --windowed-client   force the use of a graphical (windowed)\n\
                            client (GTK+ or Win32)\n\
  -t, --text-client       force the use of a text-mode client (curses) (by\n\
                            default, a windowed client is used when possible)\n\
  -P, --player=NAME       set player name to \"NAME\"\n\
  -C, --convert=FILE      convert an \"old format\" score file to the new format\n"), DPSCOREDIR);
  PluginHelp();
  g_print(_("  -h, --help              display this help information\n\
  -v, --version           output version information and exit\n\n\
dopewars is Copyright (C) Ben Webb 1998-2022, and released under the GNU GPL\n\
Report bugs to the author at benwebb@users.sf.net\n"));
#else
           /* Usage information, printed when the user runs "dopewars -h"
              (short options only version) */
           _("Usage: dopewars [OPTION]...\n\
Drug dealing game based on \"Drug Wars\" by John E. Dell\n\
  -b       \"black and white\" - i.e. do not use pretty colors\n\
              (by default colors are used where the terminal supports them)\n\
  -n       be boring and don't connect to any available dopewars servers\n\
              (i.e. single player mode)\n\
  -a       \"antique\" dopewars - keep as closely to the original version as\n\
              possible (no networking)\n\
  -f file  specify a file to use as the high score table\n\
              (by default %s/dopewars.sco is used)\n\
  -o addr  specify a hostname where the server for multiplayer dopewars\n\
              can be found\n\
  -s       run in server mode (note: see the -A option for configuring a\n\
              server once it\'s running)\n\
  -S       run a \"private\" server (i.e. do not notify the metaserver)\n\
  -p port  specify the network port to use (default: 7902)\n\
  -g file  specify the pathname of a dopewars configuration file; this file\n\
              is read immediately when the -g option is encountered\n\
  -r file  maintain pid file \"file\" while running the server\n\
  -l file  write log information to \"file\"\n\
  -c       create and run a computer player\n\
  -w       force the use of a graphical (windowed) client (GTK+ or Win32)\n\
  -t       force the use of a text-mode client (curses)\n\
              (by default, a windowed client is used when possible)\n\
  -P name  set player name to \"name\"\n\
  -C file  convert an \"old format\" score file to the new format\n\
  -A       connect to a locally-running server for administration\n"),
           DPSCOREDIR);
  PluginHelp();
g_print(_("  -h       display this help information\n\
  -v       output version information and exit\n\n\
dopewars is Copyright (C) Ben Webb 1998-2022, and released under the GNU GPL\n\
Report bugs to the author at benwebb@users.sf.net\n"));
#endif
}

struct CMDLINE *ParseCmdLine(int argc, char *argv[])
{
  int c;
  struct CMDLINE *cmdline = g_new0(struct CMDLINE, 1);
  static const gchar *options = "anbchvf:o:sSp:g:r:wtC:l:NAu:P:";

#ifdef HAVE_GETOPT_LONG
  static const struct option long_options[] = {
    {"no-color", no_argument, NULL, 'b'},
    {"no-colour", no_argument, NULL, 'b'},
    {"single-player", no_argument, NULL, 'n'},
    {"antique", no_argument, NULL, 'a'},
    {"scorefile", required_argument, NULL, 'f'},
    {"hostname", required_argument, NULL, 'o'},
    {"public-server", no_argument, NULL, 's'},
    {"private-server", no_argument, NULL, 'S'},
    {"port", required_argument, NULL, 'p'},
    {"config-file", required_argument, NULL, 'g'},
    {"pidfile", required_argument, NULL, 'r'},
    {"ai-player", no_argument, NULL, 'c'},
    {"windowed-client", no_argument, NULL, 'w'},
    {"text-client", no_argument, NULL, 't'},
    {"player", required_argument, NULL, 'P'},
    {"convert", required_argument, NULL, 'C'},
    {"logfile", required_argument, NULL, 'l'},
    {"admin", no_argument, NULL, 'A'},
    {"plugin", required_argument, NULL, 'u'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {0, 0, 0, 0}
  };
#endif

  cmdline->scorefile = cmdline->servername = cmdline->pidfile
      = cmdline->logfile = cmdline->plugin = cmdline->convertfile
      = cmdline->playername = NULL;
  cmdline->configs = NULL;
  cmdline->color = cmdline->network = TRUE;
  cmdline->client = CLIENT_AUTO;

  do {
#ifdef HAVE_GETOPT_LONG
    c = getopt_long(argc, argv, options, long_options, NULL);
#else
    c = getopt(argc, argv, options);
#endif
    switch (c) {
    case 'n':
      cmdline->network = FALSE;
      break;
    case 'b':
      cmdline->color = FALSE;
      break;
    case 'c':
      cmdline->ai = TRUE;
      break;
    case 'a':
      cmdline->antique = TRUE;
      cmdline->network = FALSE;
      break;
    case 'v':
      cmdline->version = TRUE;
      break;
    case 'h':
    case 0:
    case '?':
      cmdline->help = TRUE;
      break;
    case 'f':
      AssignName(&cmdline->scorefile, optarg);
      break;
    case 'o':
      AssignName(&cmdline->servername, optarg);
      break;
    case 's':
      cmdline->server = TRUE;
      cmdline->notifymeta = TRUE;
      break;
    case 'S':
      cmdline->server = TRUE;
      cmdline->notifymeta = FALSE;
      break;
    case 'p':
      cmdline->setport = TRUE;
      cmdline->port = atoi(optarg);
      break;
    case 'g':
      cmdline->configs = g_slist_append(cmdline->configs, g_strdup(optarg));
      break;
    case 'r':
      AssignName(&cmdline->pidfile, optarg);
      break;
    case 'l':
      AssignName(&cmdline->logfile, optarg);
      break;
    case 'u':
      AssignName(&cmdline->plugin, optarg);
      break;
    case 'w':
      cmdline->client = CLIENT_WINDOW;
      break;
    case 't':
      cmdline->client = CLIENT_CURSES;
      break;
    case 'P':
      AssignName(&cmdline->playername, optarg);
      break;
    case 'C':
      AssignName(&cmdline->convertfile, optarg);
      cmdline->convert = TRUE;
      break;
    case 'A':
      cmdline->admin = TRUE;
      break;
    }
  } while (c != -1);

  return cmdline;
}

void FreeCmdLine(struct CMDLINE *cmdline)
{
  GSList *list;

  g_free(cmdline->scorefile);
  g_free(cmdline->servername);
  g_free(cmdline->pidfile);
  g_free(cmdline->logfile);
  g_free(cmdline->plugin);
  g_free(cmdline->convertfile);
  g_free(cmdline->playername);

  for (list = cmdline->configs; list; list = g_slist_next(list)) {
    g_free(list->data);
  }
  g_slist_free(list);
  g_free(cmdline);
}

static gchar *priv_hiscore = NULL;

/* 
 * Does general startup stuff (command line, dropping privileges,
 * and high score init; config files are handled later)
 */
struct CMDLINE *GeneralStartup(int argc, char *argv[])
{
  /* First, open the hard-coded high score file with possibly
   * elevated privileges */
#ifdef CYGWIN
  priv_hiscore = g_strdup_printf("%s/dopewars.sco",
                                 appdata_path ? appdata_path : DPSCOREDIR);
#else
  priv_hiscore = g_strdup_printf("%s/dopewars.sco", DPSCOREDIR);
#endif
  HiScoreFile = g_strdup(priv_hiscore);
  OpenHighScoreFile();
  DropPrivileges();

  /* Initialize variables */
  Log.File = g_strdup("");
  Log.Level = 2;
  Log.Timestamp = g_strdup("[%H:%M:%S] ");
  srand((unsigned)time(NULL));
  Noone.Name = g_strdup("Noone");
  Server = Client = Network = FALSE;

  return ParseCmdLine(argc, argv);
}

void InitConfiguration(struct CMDLINE *cmdline)
{
  ConfigErrors = 0;
  SetupParameters(cmdline->configs, cmdline->antique);

  if (cmdline->scorefile) {
    AssignName(&HiScoreFile, cmdline->scorefile);
  }
  if (cmdline->servername) {
    AssignName(&ServerName, cmdline->servername);
  }
  if (cmdline->playername) {
    AssignName(&PlayerName, cmdline->playername);
  }
  if (cmdline->pidfile) {
    AssignName(&PidFile, cmdline->pidfile);
  }
  if (cmdline->logfile) {
    AssignName(&Log.File, cmdline->logfile);
    OpenLog();
  }
  if (cmdline->setport) {
    Port = cmdline->port;
  }
#ifdef NETWORKING
  if (cmdline->server) {
    MetaServer.Active = cmdline->notifymeta;
  }
#endif
  WantAntique = cmdline->antique;

  if (!cmdline->version && !cmdline->help && !cmdline->ai
      && !cmdline->convert && !cmdline->admin) {
    /* Open a user-specified high score file with no privileges, if one
     * was given */
    if (strcmp(priv_hiscore, HiScoreFile) != 0) {
      CloseHighScoreFile();
      OpenHighScoreFile();
    }
  } else {
    CloseHighScoreFile();
  }
}

/*
 * Removes any ^ or \n characters from the given string, which is
 * modified in place.
 */
void StripTerminators(gchar *str)
{
  guint i;

  if (str) {
    for (i = 0; i < strlen(str); i++) {
      switch(str[i]) {
      case '^':
      case '\n':
        str[i] = '~';
        break;
      default:
        break;
      }
    }
  }
}

#ifndef CYGWIN

#if defined(NETWORKING) && !defined(GUI_SERVER)
static void ServerLogMessage(const gchar *log_domain,
                             GLogLevelFlags log_level,
                             const gchar *message, gpointer user_data)
{
  GString *text;

  text = GetLogString(log_level, message);
  if (text) {
    fprintf(Log.fp ? Log.fp : stdout, "%s\n", text->str);
    g_string_free(text, TRUE);
  }
}
#endif

#ifndef CURSES_CLIENT
/*
 * Stub function to report an error if the Curses client is requested and
 * it isn't compiled in.
 */
void CursesLoop(struct CMDLINE *cmdline)
{
  g_print(_("No curses client available - rebuild the binary passing the\n"
            "--enable-curses-client option to configure, or use a windowed\n"
            "client (if available) instead!\n"));
}
#endif

#ifndef GUI_CLIENT
/*
 * Stub function to report an error if the GTK+ client is requested and
 * it isn't compiled in.
 */
#ifdef CYGWIN
gboolean GtkLoop(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                 struct CMDLINE *cmdline, gboolean ReturnOnFail)
#else
gboolean GtkLoop(int *argc, char **argv[], struct CMDLINE *cmdline,
                 gboolean ReturnOnFail)
#endif
{
  if (!ReturnOnFail) {
    g_print(_("No graphical client available - rebuild the binary\n"
              "passing the --enable-gui-client option to configure, or\n"
              "use the curses client (if available) instead!\n"));
  }
  return FALSE;
}
#endif

static void DefaultLogMessage(const gchar *log_domain,
                              GLogLevelFlags log_level,
                              const gchar *message, gpointer user_data)
{
  GString *text;

  text = GetLogString(log_level, message);
  if (text) {
    g_print("%s\n", text->str);
    g_string_free(text, TRUE);
  }
}

/* 
 * Standard program entry - Win32 uses WinMain() instead, in winmain.c
 */
int main(int argc, char *argv[])
{
  struct CMDLINE *cmdline;
#ifdef ENABLE_NLS
  const char *charset;
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
  LocaleIsUTF8 = g_get_charset(&charset);
#endif
  WantUTF8Errors(FALSE);
  g_log_set_handler(NULL, LogMask(), DefaultLogMessage, NULL);
  cmdline = GeneralStartup(argc, argv);
  if (cmdline->logfile) {
    AssignName(&Log.File, cmdline->logfile);
  }
  OpenLog();
  SoundInit();
  if (cmdline->version || cmdline->help) {
    HandleHelpTexts(cmdline->help);
  } else if (cmdline->admin) {
#ifdef NETWORKING
    AdminServer(cmdline);
#else
    g_print(_("This binary has been compiled without networking "
              "support, and thus cannot run\nin admin mode. "
              "Recompile passing --enable-networking to the "
              "configure script.\n"));
#endif
  } else if (cmdline->convert) {
    ConvertHighScoreFile(cmdline->convertfile);
  } else {
    InitNetwork();
    if (cmdline->server) {
#ifdef NETWORKING
#ifdef GUI_SERVER
      Server = TRUE;
      gtk_set_locale();
      gtk_init(&argc, &argv);
      GuiServerLoop(cmdline, FALSE);
#else
      g_log_set_handler(NULL, LogMask(), ServerLogMessage, NULL);
      ServerLoop(cmdline);
#endif /* GUI_SERVER */
#else
      g_print(_("This binary has been compiled without networking "
                "support, and thus cannot run\nin server mode. "
                "Recompile passing --enable-networking to the "
                "configure script.\n"));
#endif /* NETWORKING */
    } else if (cmdline->ai) {
      AIPlayerLoop(cmdline);
    } else
      switch (cmdline->client) {
      case CLIENT_AUTO:
        if (!GtkLoop(&argc, &argv, cmdline, TRUE))
          CursesLoop(cmdline);
        break;
      case CLIENT_WINDOW:
        GtkLoop(&argc, &argv, cmdline, FALSE);
        break;
      case CLIENT_CURSES:
        CursesLoop(cmdline);
        break;
      }
#ifdef NETWORKING
    StopNetworking();
#endif
  }
  FreeCmdLine(cmdline);
  CloseLog();
  CloseHighScoreFile();
  g_free(PidFile);
  g_free(Log.File);
  SoundClose();
  return 0;
}

#endif /* CYGWIN */
