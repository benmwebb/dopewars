/************************************************************************
 * curses_client.c  dopewars client using the (n)curses console library *
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

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <glib.h>
#include "curses_client.h"
#include "cursesport/cursesport.h"
#include "dopewars.h"
#include "message.h"
#include "nls.h"
#include "serverside.h"
#include "tstring.h"

static void PrepareHighScoreScreen(void);
static void PrintHighScore(char *Data);

static int ResizedFlag;
static SCREEN *cur_screen;

#define PromptAttr   (COLOR_PAIR(1))
#define TextAttr     (COLOR_PAIR(2))
#define LocationAttr (COLOR_PAIR(3)|A_BOLD)
#define TitleAttr    (COLOR_PAIR(4))
#define StatsAttr    (COLOR_PAIR(5))
#define DebtAttr     (COLOR_PAIR(6))

/* Current size of the screen */
static int Width, Depth;

#ifdef NETWORKING
static enum {
  CM_SERVER, CM_PROMPT, CM_META, CM_SINGLE
} ConnectMethod = CM_SERVER;
#endif

static gboolean CanFire = FALSE, RunHere = FALSE;
static FightPoint fp;

/* Function definitions; make them static so as not to clash with
 * functions of the same name in different clients */
static void display_intro(void);
static void ResizeHandle(int sig);
static void CheckForResize(Player *Play);
static int GetKey(char *allowed, char *orig_allowed, gboolean AllowOther,
                  gboolean PrintAllowed, gboolean ExpandOut);
static void clear_bottom(void), clear_screen(void);
static void clear_line(int line), clear_exceptfor(int skip);
static void nice_wait(void);
static void DisplayFightMessage(Player *Play, char *text);
static void DisplaySpyReports(char *Data, Player *From, Player *To);
static void display_message(char *buf);
static void print_location(char *text);
static void print_status(Player *Play, gboolean DispDrug);
static char *nice_input(char *prompt, int sy, int sx, gboolean digitsonly,
                        char *displaystr, char passwdchar);
static Player *ListPlayers(Player *Play, gboolean Select, char *Prompt);
static void HandleClientMessage(char *buf, Player *Play);
static void PrintMessage(const gchar *text);
static void GunShop(Player *Play);
static void LoanShark(Player *Play);
static void Bank(Player *Play);

#ifdef NETWORKING
static void HttpAuthFunc(HttpConnection *conn, gboolean proxyauth,
                         gchar *realm, gpointer data);
static void SocksAuthFunc(NetworkBuffer *netbuf, gpointer data);
#endif

static DispMode DisplayMode;
static gboolean QuitRequest;

/* 
 * Initialises the curses library for accessing the screen.
 */
static void start_curses(void)
{
  cur_screen = newterm(NULL, stdout, stdin);
  if (WantColour) {
    start_color();
    init_pair(1, COLOR_MAGENTA, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
    init_pair(4, COLOR_BLUE, COLOR_WHITE);
    init_pair(5, COLOR_WHITE, COLOR_BLUE);
    init_pair(6, COLOR_RED, COLOR_WHITE);
  }
  cbreak();
  noecho();
  nodelay(stdscr, FALSE);
  keypad(stdscr, TRUE);
  curs_set(0);
}

/* 
 * Shuts down the curses screen library.
 */
static void end_curses(void)
{
  keypad(stdscr, FALSE);
  curs_set(1);
  erase();
  refresh();
  endwin();
}

/* 
 * Handles a SIGWINCH signal, which is sent to indicate that the
 * size of the curses screen has changed.
 */
void ResizeHandle(int sig)
{
  ResizedFlag = 1;
}

/* 
 * Checks to see if the curses window needs to be resized - i.e. if a
 * SIGWINCH signal has been received.
 */
void CheckForResize(Player *Play)
{
  sigset_t sigset;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGWINCH);
  sigprocmask(SIG_BLOCK, &sigset, NULL);
  if (ResizedFlag) {
    ResizedFlag = 0;
    end_curses();
    start_curses();
    Width = COLS;
    Depth = LINES;
    attrset(TextAttr);
    clear_screen();
    display_message("");
    DisplayFightMessage(Play, "");
    print_status(Play, TRUE);
  }
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}

static void LogMessage(const gchar *log_domain, GLogLevelFlags log_level,
                       const gchar *message, gpointer user_data)
{
  attrset(TextAttr);
  clear_bottom();
  PrintMessage(message);
  nice_wait();
  attrset(TextAttr);
  clear_bottom();
}

/* 
 * Displays a dopewars introduction screen.
 */
void display_intro(void)
{
  GString *text;

  attrset(TextAttr);
  clear_screen();
  attrset(TitleAttr);

  /* Curses client introduction screen */
  text = g_string_new(_("D O P E W A R S"));
  mvaddstr(1, (Width - text->len) / 2, text->str);

  attrset(TextAttr);

  mvaddstr(3, 1, _("Based on John E. Dell's old Drug Wars game, dopewars "
                   "is a simulation of an"));
  mvaddstr(4, 1, _("imaginary drug market.  dopewars is an All-American "
                   "game which features"));
  mvaddstr(5, 1, _("buying, selling, and trying to get past the cops!"));

  mvaddstr(7, 1, _("The first thing you need to do is pay off your "
                   "debt to the Loan Shark. After"));
  mvaddstr(8, 1, _("that, your goal is to make as much money as "
                   "possible (and stay alive)! You"));
  mvaddstr(9, 1, _("have one month of game time to make your fortune."));

  mvaddstr(11, 18, _("Copyright (C) 1998-2002  Ben Webb "
                     "ben@bellatrix.pcl.ox.ac.uk"));
  g_string_sprintf(text, _("Version %s"), VERSION);
  mvaddstr(11, 2, text->str);
  g_string_assign(text, _("dopewars is released under the GNU "
                          "General Public Licence"));
  mvaddstr(12, (Width - text->len) / 2, text->str);

  mvaddstr(14, 7, _("Icons and Graphics            Ocelot Mantis"));
  mvaddstr(15, 7, _("Drug Dealing and Research     Dan Wolf"));
  mvaddstr(16, 7, _("Play Testing                  Phil Davis           "
                    "Owen Walsh"));
  mvaddstr(17, 7, _("Extensive Play Testing        Katherine Holt       "
                    "Caroline Moore"));
  mvaddstr(18, 7, _("Constructive Criticism        Andrea Elliot-Smith  "
                    "Pete Winn"));
  mvaddstr(19, 7, _("Unconstructive Criticism      James Matthews"));

  mvaddstr(21, 3, _("For information on the command line options, type "
                    "dopewars -h at your"));
  mvaddstr(22, 1,
           _("Unix prompt. This will display a help screen, listing "
             "the available options."));

  g_string_free(text, TRUE);
  nice_wait();
  attrset(TextAttr);
  clear_screen();
  refresh();
}

#ifdef NETWORKING
/* 
 * Prompts the user to enter a server name and port to connect to.
 */
static void SelectServerManually(void)
{
  gchar *text, *PortText;

  if (ServerName[0] == '(')
    AssignName(&ServerName, "localhost");
  attrset(TextAttr);
  clear_bottom();
  mvaddstr(17, 1,
           /* Prompts for hostname and port when selecting a server
            * manually */
           _("Please enter the hostname and port of a dopewars server:-"));
  text = nice_input(_("Hostname: "), 18, 1, FALSE, ServerName, '\0');
  AssignName(&ServerName, text);
  g_free(text);
  PortText = g_strdup_printf("%d", Port);
  text = nice_input(_("Port: "), 19, 1, TRUE, PortText, '\0');
  Port = atoi(text);
  g_free(text);
  g_free(PortText);
}

/* 
 * Contacts the dopewars metaserver, and obtains a list of valid
 * server/port pairs, one of which the user should select.
 * Returns TRUE on success; on failure FALSE is returned, and
 * errstr is assigned an error message.
 */
static gboolean SelectServerFromMetaServer(Player *Play, GString *errstr)
{
  int c;
  GSList *ListPt;
  ServerData *ThisServer;
  GString *text;
  gint index;
  fd_set readfds, writefds;
  int maxsock;
  gboolean DoneOK;
  HttpConnection *MetaConn;

  attrset(TextAttr);
  clear_bottom();
  mvaddstr(17, 1, _("Please wait... attempting to contact metaserver..."));
  refresh();

  if (OpenMetaHttpConnection(&MetaConn)) {
    SetHttpAuthFunc(MetaConn, HttpAuthFunc, NULL);
    SetNetworkBufferUserPasswdFunc(&MetaConn->NetBuf, SocksAuthFunc, NULL);
  } else {
    g_string_assign_error(errstr, MetaConn->NetBuf.error);
    CloseHttpConnection(MetaConn);
    return FALSE;
  }

  ClearServerList(&ServerList);

  do {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(0, &readfds);
    maxsock = 1;
    SetSelectForNetworkBuffer(&MetaConn->NetBuf, &readfds, &writefds,
                              NULL, &maxsock);
    if (bselect(maxsock, &readfds, &writefds, NULL, NULL) == -1) {
      if (errno == EINTR) {
        CheckForResize(Play);
        continue;
      }
      perror("bselect");
      exit(1);
    }
    if (FD_ISSET(0, &readfds)) {
      /* So that Ctrl-L works */
      c = getch();
      if (c == '\f')
        wrefresh(curscr);
    }
    if (RespondToSelect
        (&MetaConn->NetBuf, &readfds, &writefds, NULL, &DoneOK)) {
      while (HandleWaitingMetaServerData(MetaConn, &ServerList, &DoneOK)) {
      }
    }
    if (!DoneOK && HandleHttpCompletion(MetaConn)) {
      if (IsHttpError(MetaConn)) {
        g_string_assign_error(errstr, MetaConn->NetBuf.error);
        CloseHttpConnection(MetaConn);
        return FALSE;
      }
    }
  } while (DoneOK);
  CloseHttpConnection(MetaConn);

  text = g_string_new("");

  ListPt = ServerList;
  while (ListPt) {
    ThisServer = (ServerData *)(ListPt->data);
    attrset(TextAttr);
    clear_bottom();
    /* Printout of metaserver information in curses client */
    g_string_sprintf(text, _("Server : %s"), ThisServer->Name);
    mvaddstr(17, 1, text->str);
    g_string_sprintf(text, _("Port   : %d"), ThisServer->Port);
    mvaddstr(18, 1, text->str);
    g_string_sprintf(text, _("Version    : %s"), ThisServer->Version);
    mvaddstr(18, 40, text->str);
    if (ThisServer->CurPlayers == -1) {
      g_string_sprintf(text, _("Players: -unknown- (maximum %d)"),
                       ThisServer->MaxPlayers);
    } else {
      g_string_sprintf(text, _("Players: %d (maximum %d)"),
                       ThisServer->CurPlayers, ThisServer->MaxPlayers);
    }
    mvaddstr(19, 1, text->str);
    g_string_sprintf(text, _("Up since   : %s"), ThisServer->UpSince);
    mvaddstr(19, 40, text->str);
    g_string_sprintf(text, _("Comment: %s"), ThisServer->Comment);
    mvaddstr(20, 1, text->str);
    attrset(PromptAttr);
    mvaddstr(23, 1,
             _("N>ext server; P>revious server; S>elect this server... "));

    /* The three keys that are valid responses to the previous question -
     * if you translate them, keep the keys in the same order (N>ext,
     * P>revious, S>elect) as they are here, otherwise they'll do the
     * wrong things. */
    c = GetKey(_("NPS"), "NPS", FALSE, FALSE, FALSE);
    switch (c) {
    case 'S':
      AssignName(&ServerName, ThisServer->Name);
      Port = ThisServer->Port;
      ListPt = NULL;
      break;
    case 'N':
      ListPt = g_slist_next(ListPt);
      if (!ListPt)
        ListPt = ServerList;
      break;
    case 'P':
      index = g_slist_position(ServerList, ListPt) - 1;
      if (index >= 0)
        ListPt = g_slist_nth(ServerList, (guint)index);
      else
        ListPt = g_slist_last(ListPt);
      break;
    }
  }
  if (!ServerList) {
    g_string_assign(errstr, "No servers listed on metaserver");
    return FALSE;
  }
  clear_line(17);
  refresh();
  g_string_free(text, TRUE);
  return TRUE;
}

static void DisplayConnectStatus(NetworkBuffer *netbuf,
                                 NBStatus oldstatus,
                                 NBSocksStatus oldsocks)
{
  NBStatus status;
  NBSocksStatus sockstat;
  GString *text;

  status = netbuf->status;
  sockstat = netbuf->sockstat;

  if (oldstatus == status && oldsocks == sockstat)
    return;

  text = g_string_new("");

  switch (status) {
  case NBS_PRECONNECT:
    break;
  case NBS_SOCKSCONNECT:
    switch (sockstat) {
    case NBSS_METHODS:
      g_string_sprintf(text, _("Connected to SOCKS server %s..."),
                       Socks.name);
      break;
    case NBSS_USERPASSWD:
      g_string_assign(text, _("Authenticating with SOCKS server"));
      break;
    case NBSS_CONNECT:
      g_string_sprintf(text, _("Asking SOCKS for connect to %s..."),
                       ServerName);
      break;
    }
    break;
  case NBS_CONNECTED:
    break;
  }
  if (text->str[0]) {
    mvaddstr(17, 1, text->str);
    refresh();
  }
  g_string_free(text, TRUE);
}

void HttpAuthFunc(HttpConnection *conn, gboolean proxyauth,
                  gchar *realm, gpointer data)
{
  gchar *text, *user, *password = NULL;

  attrset(TextAttr);
  clear_bottom();
  if (proxyauth) {
    text = g_strdup_printf(_("Proxy authentication required for realm %s"),
                           realm);
  } else {
    text =
        g_strdup_printf(_("Authentication required for realm %s"), realm);
  }
  mvaddstr(17, 1, text);
  mvaddstr(18, 1, _("(Enter a blank username to cancel)"));
  g_free(text);

  user = nice_input(_("User name: "), 19, 1, FALSE, NULL, '\0');
  if (user && user[0]) {
    password = nice_input(_("Password: "), 20, 1, FALSE, NULL, '*');
  }

  SetHttpAuthentication(conn, proxyauth, user, password);
  g_free(user);
  g_free(password);
}

void SocksAuthFunc(NetworkBuffer *netbuf, gpointer data)
{
  gchar *user, *password = NULL;

  attrset(TextAttr);
  clear_bottom();
  mvaddstr(17, 1, _("SOCKS authentication required (enter a blank "
                    "username to cancel)"));

  user = nice_input(_("User name: "), 18, 1, FALSE, NULL, '\0');
  if (user && user[0]) {
    password = nice_input(_("Password: "), 19, 1, FALSE, NULL, '*');
  }

  SendSocks5UserPasswd(netbuf, user, password);
  g_free(user);
  g_free(password);
}

static gboolean DoConnect(Player *Play, GString *errstr)
{
  NetworkBuffer *netbuf;
  fd_set readfds, writefds;
  int maxsock, c;
  gboolean doneOK = TRUE;
  NBStatus oldstatus;
  NBSocksStatus oldsocks;

  netbuf = &Play->NetBuf;
  oldstatus = netbuf->status;
  oldsocks = netbuf->sockstat;

  if (!StartNetworkBufferConnect(netbuf, ServerName, Port)) {
    doneOK = FALSE;
  } else {
    SetNetworkBufferUserPasswdFunc(netbuf, SocksAuthFunc, NULL);
    while (netbuf->status != NBS_CONNECTED) {
      DisplayConnectStatus(netbuf, oldstatus, oldsocks);
      oldstatus = netbuf->status;
      oldsocks = netbuf->sockstat;
      FD_ZERO(&readfds);
      FD_ZERO(&writefds);
      FD_SET(0, &readfds);
      maxsock = 1;
      SetSelectForNetworkBuffer(netbuf, &readfds, &writefds, NULL,
                                &maxsock);
      if (bselect(maxsock, &readfds, &writefds, NULL, NULL) == -1) {
        if (errno == EINTR) {
          CheckForResize(Play);
          continue;
        }
        perror("bselect");
        exit(1);
      }
      if (FD_ISSET(0, &readfds)) {
        /* So that Ctrl-L works */
        c = getch();
#ifndef CYGWIN
        if (c == '\f')
          wrefresh(curscr);
#endif
      }
      RespondToSelect(netbuf, &readfds, &writefds, NULL, &doneOK);
    }
  }

  if (!doneOK)
    g_string_assign_error(errstr, netbuf->error);
  return doneOK;
}

/* 
 * Connects to a dopewars server. Prompts the user to select a server
 * if necessary. Returns TRUE, unless the user elected to quit the
 * program rather than choose a valid server.
 */
static gboolean ConnectToServer(Player *Play)
{
  gboolean MetaOK = TRUE, NetOK = TRUE, firstrun = FALSE;
  GString *errstr;
  gchar *text;
  int c;

  errstr = g_string_new("");

  if (g_strcasecmp(ServerName, SN_META) == 0 || ConnectMethod == CM_META) {
    ConnectMethod = CM_META;
    MetaOK = SelectServerFromMetaServer(Play, errstr);
  } else if (g_strcasecmp(ServerName, SN_PROMPT) == 0 ||
             ConnectMethod == CM_PROMPT) {
    ConnectMethod = CM_PROMPT;
    SelectServerManually();
  } else if (g_strcasecmp(ServerName, SN_SINGLE) == 0 ||
             ConnectMethod == CM_SINGLE) {
    ConnectMethod = CM_SINGLE;
    g_string_free(errstr, TRUE);
    return TRUE;
  } else
    firstrun = TRUE;

  while (1) {
    attrset(TextAttr);
    clear_bottom();
    if (MetaOK && !firstrun) {
      mvaddstr(17, 1, _("Please wait... attempting to contact "
                        "dopewars server..."));
      refresh();
      NetOK = DoConnect(Play, errstr);
    }
    if (!NetOK || !MetaOK || firstrun) {
      firstrun = FALSE;
      clear_line(16);
      clear_line(17);
      if (!MetaOK) {
        /* Display of an error while contacting the metaserver */
        mvaddstr(16, 1, _("Cannot get metaserver details"));
        text = g_strdup_printf("   (%s)", errstr->str);
        mvaddstr(17, 1, text);
        g_free(text);
      } else if (!NetOK) {
        /* Display of an error message while trying to contact a dopewars
         * server (the error message itself is displayed on the next
         * screen line) */
        mvaddstr(16, 1, _("Could not start multiplayer dopewars"));
        text = g_strdup_printf("   (%s)", errstr->str);
        mvaddstr(17, 1, text);
        g_free(text);
      }
      MetaOK = NetOK = TRUE;
      attrset(PromptAttr);
      mvaddstr(18, 1,
               _("Will you... C>onnect to a named dopewars server"));
      mvaddstr(19, 1,
               _("            L>ist the servers on the metaserver, and "
                 "select one"));
      mvaddstr(20, 1,
               _("            Q>uit (where you can start a server "
                 "by typing \"dopewars -s\")"));
      mvaddstr(21, 1, _("         or P>lay single-player ? "));
      attrset(TextAttr);

      /* Translate these 4 keys in line with the above options, keeping
       * the order the same (C>onnect, L>ist, Q>uit, P>lay single-player) */
      c = GetKey(_("CLQP"), "CLQP", FALSE, FALSE, FALSE);
      switch (c) {
      case 'Q':
        g_string_free(errstr, TRUE);
        return FALSE;
      case 'P':
        g_string_free(errstr, TRUE);
        return TRUE;
      case 'L':
        MetaOK = SelectServerFromMetaServer(Play, errstr);
        break;
      case 'C':
        SelectServerManually();
        break;
      }
    } else
      break;
  }
  g_string_free(errstr, TRUE);
  Client = Network = TRUE;
  return TRUE;
}
#endif /* NETWORKING */

/* 
 * Displays the list of locations and prompts the user to select one.
 * If "AllowReturn" is TRUE, then if the current location is selected
 * simply drop back to the main game loop, otherwise send a request
 * to the server to move to the new location. If FALSE, the user MUST
 * choose a new location to move to. The active client player is
 * passed in "Play".
 * N.B. May set the global variable DisplayMode.
 * Returns: TRUE if the user chose to jet to a new location,
 *          FALSE if the action was cancelled instead.
 */
static gboolean jet(Player *Play, gboolean AllowReturn)
{
  int i, c;
  char text[80];

  attrset(TextAttr);
  clear_bottom();
  for (i = 0; i < NumLocation; i++) {
    sprintf(text, "%d. %s", i + 1, Location[i].Name);
    mvaddstr(17 + i / 3, (i % 3) * 20 + 12, text);
  }
  attrset(PromptAttr);

  /* Prompt when the player chooses to "jet" to a new location */
  mvaddstr(22, 22, _("Where to, dude ? "));
  attrset(TextAttr);
  curs_set(1);
  do {
    c = bgetch();
    if (c >= '1' && c < '1' + NumLocation) {
      addstr(Location[c - '1'].Name);
      if (Play->IsAt != c - '1') {
        sprintf(text, "%d", c - '1');
        DisplayMode = DM_NONE;
        SendClientMessage(Play, C_NONE, C_REQUESTJET, NULL, text);
      } else
        c = 0;
    } else
      c = 0;
  } while (c == 0 && !AllowReturn);

  curs_set(0);
  return (c != 0);
}

/* 
 * Prompts the user "Play" to drop some of the currently carried drugs.
 */
static void DropDrugs(Player *Play)
{
  int i, c, num, NumDrugs;
  GString *text;
  gchar *buf;

  attrset(TextAttr);
  clear_bottom();
  text = g_string_new("");
  dpg_string_sprintf(text,
                     /* List of drugs that you can drop (%tde = "drugs" by 
                      * default) */
                     _("You can\'t get any cash for the following "
                       "carried %tde :"), Names.Drugs);
  mvaddstr(16, 1, text->str);
  NumDrugs = 0;
  for (i = 0; i < NumDrug; i++) {
    if (Play->Drugs[i].Carried > 0 && Play->Drugs[i].Price == 0) {
      g_string_sprintf(text, "%c. %-10s %-8d", NumDrugs + 'A',
                       Drug[i].Name, Play->Drugs[i].Carried);
      mvaddstr(17 + NumDrugs / 3, (NumDrugs % 3) * 25 + 4, text->str);
      NumDrugs++;
    }
  }
  attrset(PromptAttr);
  mvaddstr(22, 20, _("What do you want to drop? "));
  curs_set(1);
  attrset(TextAttr);
  c = bgetch();
  c = toupper(c);
  for (i = 0; c >= 'A' && c < 'A' + NumDrugs && i < NumDrug; i++) {
    if (Play->Drugs[i].Carried > 0 && Play->Drugs[i].Price == 0) {
      c--;
      if (c < 'A') {
        addstr(Drug[i].Name);
        buf =
            nice_input(_("How many do you drop? "), 23, 8, TRUE, NULL,
                       '\0');
        num = atoi(buf);
        g_free(buf);
        if (num > 0) {
          g_string_sprintf(text, "drug^%d^%d", i, -num);
          SendClientMessage(Play, C_NONE, C_BUYOBJECT, NULL, text->str);
        }
      }
    }
  }
  g_string_free(text, TRUE);
}

/* 
 * Prompts the user (i.e. the owner of client "Play") to buy drugs if
 * "Buy" is TRUE, or to sell drugs otherwise. A list of available drugs
 * is displayed, and on receiving the selection, the user is prompted
 * for the number of drugs desired. Finally a message is sent to the
 * server to buy or sell the required quantity.
 */
static void DealDrugs(Player *Play, gboolean Buy)
{
  int i, c, NumDrugsHere;
  gchar *text, *input;
  int DrugNum, CanCarry, CanAfford;

  NumDrugsHere = 0;
  for (c = 0; c < NumDrug; c++)
    if (Play->Drugs[c].Price > 0)
      NumDrugsHere++;

  clear_line(22);
  attrset(PromptAttr);
  if (Buy) {
    /* Buy and sell prompts for dealing drugs or guns */
    mvaddstr(22, 20, _("What do you wish to buy? "));
  } else {
    mvaddstr(22, 20, _("What do you wish to sell? "));
  }
  curs_set(1);
  attrset(TextAttr);
  c = bgetch();
  c = toupper(c);
  if (c >= 'A' && c < 'A' + NumDrugsHere) {
    DrugNum = -1;
    c -= 'A';
    for (i = 0; i <= c; i++)
      DrugNum = GetNextDrugIndex(DrugNum, Play);
    addstr(Drug[DrugNum].Name);
    CanCarry = Play->CoatSize;
    CanAfford = Play->Cash / Play->Drugs[DrugNum].Price;

    if (Buy) {
      /* Display of number of drugs you could buy and/or carry, when
       * buying drugs */
      text = g_strdup_printf(_("You can afford %d, and can carry %d. "),
                             CanAfford, CanCarry);
      mvaddstr(23, 2, text);
      input = nice_input(_("How many do you buy? "), 23, 2 + strlen(text),
                         TRUE, NULL, '\0');
      c = atoi(input);
      g_free(input);
      g_free(text);
      if (c >= 0) {
        text = g_strdup_printf("drug^%d^%d", DrugNum, c);
        SendClientMessage(Play, C_NONE, C_BUYOBJECT, NULL, text);
        g_free(text);
      }
    } else {
      /* Display of number of drugs you have, when selling drugs */
      text =
          g_strdup_printf(_("You have %d. "),
                          Play->Drugs[DrugNum].Carried);
      mvaddstr(23, 2, text);
      input = nice_input(_("How many do you sell? "), 23, 2 + strlen(text),
                         TRUE, NULL, '\0');
      c = atoi(input);
      g_free(input);
      g_free(text);
      if (c >= 0) {
        text = g_strdup_printf("drug^%d^%d", DrugNum, -c);
        SendClientMessage(Play, C_NONE, C_BUYOBJECT, NULL, text);
        g_free(text);
      }
    }
  }
  curs_set(0);
}

/* 
 * Prompts the user (player "Play") to give an errand to one of his/her
 * bitches. The decision is relayed to the server for implementation.
 */
static void GiveErrand(Player *Play)
{
  int c, y;
  GString *text;
  Player *To;

  text = g_string_new("");
  attrset(TextAttr);
  clear_bottom();
  y = 17;

  /* Prompt for sending your bitches out to spy etc. (%tde = "bitches" by
   * default) */
  dpg_string_sprintf(text,
                     _("Choose an errand to give one of your %tde..."),
                     Names.Bitches);
  mvaddstr(y++, 1, text->str);
  attrset(PromptAttr);
  if (Play->Bitches.Carried > 0) {
    dpg_string_sprintf(text,
                       _("   S>py on another dealer                  "
                         "(cost: %P)"), Prices.Spy);
    mvaddstr(y++, 2, text->str);
    dpg_string_sprintf(text,
                       _("   T>ip off the cops to another dealer     "
                         "(cost: %P)"), Prices.Tipoff);
    mvaddstr(y++, 2, text->str);
    mvaddstr(y++, 2, _("   G>et stuffed"));
  }
  if (Play->Flags & SPYINGON) {
    mvaddstr(y++, 2, _("or C>ontact your spies and receive reports"));
  }
  mvaddstr(y++, 2, _("or N>o errand ? "));
  curs_set(1);
  attrset(TextAttr);

  /* Translate these 5 keys to match the above options, keeping the
   * original order the same (S>py, T>ip off, G>et stuffed, C>ontact spy,
   * N>o errand) */
  c = GetKey(_("STGCN"), "STGCN", TRUE, FALSE, FALSE);

  if (Play->Bitches.Carried > 0 || c == 'C')
    switch (c) {
    case 'S':
      To = ListPlayers(Play, TRUE, _("Whom do you want to spy on? "));
      if (To)
        SendClientMessage(Play, C_NONE, C_SPYON, To, NULL);
      break;
    case 'T':
      To = ListPlayers(Play, TRUE,
                       _("Whom do you want to tip the cops off to? "));
      if (To)
        SendClientMessage(Play, C_NONE, C_TIPOFF, To, NULL);
      break;
    case 'G':
      attrset(PromptAttr);
      /* Prompt for confirmation of sacking a bitch */
      addstr(_(" Are you sure? "));

      /* The two keys that are valid for answering Yes/No - if you
       * translate them, keep them in the same order - i.e. "Yes" before
       * "No" */
      c = GetKey(_("YN"), "YN", FALSE, TRUE, FALSE);

      if (c == 'Y')
        SendClientMessage(Play, C_NONE, C_SACKBITCH, NULL, NULL);
      break;
    case 'C':
      if (Play->Flags & SPYINGON) {
        SendClientMessage(Play, C_NONE, C_CONTACTSPY, NULL, NULL);
      }
      break;
    }
}

/* 
 * Asks the user if he/she _really_ wants to quit dopewars.
 */
static int want_to_quit(void)
{
  attrset(TextAttr);
  clear_line(22);
  attrset(PromptAttr);
  mvaddstr(22, 1, _("Are you sure you want to quit? "));
  attrset(TextAttr);
  return (GetKey(_("YN"), "YN", FALSE, TRUE, FALSE) != 'N');
}

/* 
 * Prompts the user to change his or her name, and notifies the server.
 */
static void change_name(Player *Play, gboolean nullname)
{
  gchar *NewName;

  /* Prompt for player to change his/her name */
  NewName = nice_input(_("New name: "), 23, 0, FALSE, NULL, '\0');

  if (NewName[0]) {
    if (nullname) {
      SendNullClientMessage(Play, C_NONE, C_NAME, NULL, NewName);
    } else {
      SendClientMessage(Play, C_NONE, C_NAME, NULL, NewName);
    }
    SetPlayerName(Play, NewName);
  }
  g_free(NewName);
}

/* 
 * Given a message "Message" coming in for player "Play", performs
 * processing and reacts properly; if a message indicates the end of the
 * game, the global variable QuitRequest is set. The global variable
 * DisplayMode may also be changed by this routine as a result of network
 * traffic.
 */
void HandleClientMessage(char *Message, Player *Play)
{
  char *pt, *Data, *wrd;
  AICode AI;
  MsgCode Code;
  Player *From, *tmp;
  GSList *list;
  gchar *text;
  int i;
  gboolean Handled;

  /* Ignore To: field - all messages will be for Player "Play" */
  if (ProcessMessage(Message, Play, &From, &AI, &Code, &Data, FirstClient)
      == -1) {
    return;
  }

  Handled =
      HandleGenericClientMessage(From, AI, Code, Play, Data, &DisplayMode);
  switch (Code) {
  case C_ENDLIST:
    if (FirstClient && g_slist_next(FirstClient)) {
      ListPlayers(Play, FALSE, NULL);
    }
    break;
  case C_STARTHISCORE:
    PrepareHighScoreScreen();
    break;
  case C_HISCORE:
    PrintHighScore(Data);
    break;
  case C_ENDHISCORE:
    if (strcmp(Data, "end") == 0) {
      QuitRequest = TRUE;
    } else {
      nice_wait();
      clear_screen();
      display_message("");
      print_status(Play, TRUE);
      refresh();
    }
    break;
  case C_PUSH:
    attrset(TextAttr);
    clear_line(22);
    mvaddstr(22, 0, _("You have been pushed from the server. "
                      "Reverting to single player mode."));
    nice_wait();
    SwitchToSinglePlayer(Play);
    print_status(Play, TRUE);
    break;
  case C_QUIT:
    attrset(TextAttr);
    clear_line(22);
    mvaddstr(22, 0,
             _("The server has terminated. Reverting to "
               "single player mode."));
    nice_wait();
    SwitchToSinglePlayer(Play);
    print_status(Play, TRUE);
    break;
  case C_MSG:
    text = g_strdup_printf("%s: %s", GetPlayerName(From), Data);
    display_message(text);
    g_free(text);
    break;
  case C_MSGTO:
    text = g_strdup_printf("%s->%s: %s", GetPlayerName(From),
                           GetPlayerName(Play), Data);
    display_message(text);
    g_free(text);
    break;
  case C_JOIN:
    text = g_strdup_printf(_("%s joins the game!"), Data);
    display_message(text);
    g_free(text);
    break;
  case C_LEAVE:
    if (From != &Noone) {
      text = g_strdup_printf(_("%s has left the game."), Data);
      display_message(text);
      g_free(text);
    }
    break;
  case C_RENAME:
    /* Displayed when a player changes his/her name */
    text = g_strdup_printf(_("%s will now be known as %s."),
                           GetPlayerName(From), Data);
    SetPlayerName(From, Data);
    mvaddstr(22, 0, text);
    g_free(text);
    nice_wait();
    break;
  case C_PRINTMESSAGE:
    PrintMessage(Data);
    nice_wait();
    break;
  case C_FIGHTPRINT:
    DisplayFightMessage(Play, Data);
    break;
  case C_SUBWAYFLASH:
    DisplayFightMessage(Play, NULL);
    for (list = FirstClient; list; list = g_slist_next(list)) {
      tmp = (Player *)list->data;
      tmp->Flags &= ~FIGHTING;
    }
    for (i = 0; i < 4; i++) {
      print_location(_("S U B W A Y"));
      refresh();
      MicroSleep(100000);
      print_location("");
      refresh();
      MicroSleep(100000);
    }
    print_location(Location[(int)Play->IsAt].Name);
    break;
  case C_QUESTION:
    pt = Data;
    wrd = GetNextWord(&pt, "");
    PrintMessage(pt);
    addch(' ');
    i = GetKey(_(wrd), wrd, FALSE, TRUE, TRUE);
    wrd = g_strdup_printf("%c", i);
    SendClientMessage(Play, C_NONE, C_ANSWER,
                      From == &Noone ? NULL : From, wrd);
    g_free(wrd);
    break;
  case C_LOANSHARK:
    LoanShark(Play);
    SendClientMessage(Play, C_NONE, C_DONE, NULL, NULL);
    break;
  case C_BANK:
    Bank(Play);
    SendClientMessage(Play, C_NONE, C_DONE, NULL, NULL);
    break;
  case C_GUNSHOP:
    GunShop(Play);
    SendClientMessage(Play, C_NONE, C_DONE, NULL, NULL);
    break;
  case C_UPDATE:
    if (From == &Noone) {
      ReceivePlayerData(Play, Data, Play);
      print_status(Play, TRUE);
      refresh();
    } else {
      DisplaySpyReports(Data, From, Play);
    }
    break;
  case C_NEWNAME:
    clear_line(22);
    clear_line(23);
    attrset(TextAttr);
    mvaddstr(22, 0, _("Unfortunately, somebody else is already "
                      "using \"your\" name. Please change it."));
    change_name(Play, TRUE);
    break;
  default:
    if (!Handled) {
      text = g_strdup_printf("%s^%c^%s^%s", GetPlayerName(From), Code,
                             GetPlayerName(Play), Data);
      mvaddstr(22, 0, text);
      g_free(text);
      nice_wait();
    }
    break;
  }
}

/* 
 * Responds to a "starthiscore" message by clearing the screen and
 * displaying the title for the high scores screen.
 */
void PrepareHighScoreScreen(void)
{
  char *text;

  attrset(TextAttr);
  clear_screen();
  attrset(TitleAttr);
  text = _("H I G H   S C O R E S");
  mvaddstr(0, (Width - strlen(text)) / 2, text);
  attrset(TextAttr);
}

/* 
 * Prints a high score coded in "Data"; first word is the index of the
 * score (i.e. y screen coordinate), second word is the text, the first
 * letter of which identifies whether it's to be printed bold or not.
 */
void PrintHighScore(char *Data)
{
  char *cp;
  int index;

  cp = Data;
  index = GetNextInt(&cp, 0);
  if (!cp || strlen(cp) < 2)
    return;
  move(index + 2, 0);
  attrset(TextAttr);
  if (cp[0] == 'B')
    standout();
  addstr(&cp[1]);
  if (cp[0] == 'B')
    standend();
}

/* 
 * Prints a message "text" received via. a "printmessage" message in the
 * bottom part of the screen.
 */
void PrintMessage(const gchar *text)
{
  guint i, line;

  attrset(TextAttr);
  clear_line(16);

  line = 1;
  for (i = 0; i < strlen(text) && (text[i] == '^' || text[i] == '\n'); i++)
    line++;
  clear_exceptfor(line);

  line = 17;
  move(line, 1);
  for (i = 0; i < strlen(text); i++) {
    if (text[i] == '^' || text[i] == '\n') {
      line++;
      move(line, 1);
    } else if (text[i] != '\r')
      addch((guchar)text[i]);
  }
}

static void SellGun(Player *Play)
{
  gchar *text;
  gint gunind;

  clear_line(22);
  if (TotalGunsCarried(Play) == 0) {
    /* Error - player tried to sell guns that he/she doesn't have
     * (%tde="guns" by default) */
    text = dpg_strdup_printf(_("You don't have any %tde to sell!"),
                             Names.Guns);
    mvaddstr(22, (Width - strlen(text)) / 2, text);
    g_free(text);
    nice_wait();
    clear_line(23);
  } else {
    attrset(PromptAttr);
    mvaddstr(22, 20, _("What do you wish to sell? "));
    curs_set(1);
    attrset(TextAttr);
    gunind = bgetch();
    gunind = toupper(gunind);
    if (gunind >= 'A' && gunind < 'A' + NumGun) {
      gunind -= 'A';
      addstr(Gun[gunind].Name);
      if (Play->Guns[gunind].Carried == 0) {
        clear_line(22);
        /* Error - player tried to sell some guns that he/she doesn't have */
        mvaddstr(22, 10, _("You don't have any to sell!"));
        nice_wait();
        clear_line(23);
      } else {
        Play->Cash += Gun[gunind].Price;
        Play->CoatSize += Gun[gunind].Space;
        Play->Guns[gunind].Carried--;
        text = g_strdup_printf("gun^%d^-1", gunind);
        SendClientMessage(Play, C_NONE, C_BUYOBJECT, NULL, text);
        g_free(text);
        print_status(Play, FALSE);
      }
    }
  }
}

static void BuyGun(Player *Play)
{
  gchar *text;
  gint gunind;

  clear_line(22);
  if (TotalGunsCarried(Play) >= Play->Bitches.Carried + 2) {
    text = dpg_strdup_printf(
                              /* Error - player tried to buy more guns
                               * than his/her bitches can carry (1st
                               * %tde="bitches", 2nd %tde="guns" by
                               * default) */
                              _("You'll need more %tde to carry "
                                "any more %tde!"),
                              Names.Bitches, Names.Guns);
    mvaddstr(22, (Width - strlen(text)) / 2, text);
    g_free(text);
    nice_wait();
    clear_line(23);
  } else {
    attrset(PromptAttr);
    mvaddstr(22, 20, _("What do you wish to buy? "));
    curs_set(1);
    attrset(TextAttr);
    gunind = bgetch();
    gunind = toupper(gunind);
    if (gunind >= 'A' && gunind < 'A' + NumGun) {
      gunind -= 'A';
      addstr(Gun[gunind].Name);
      if (Gun[gunind].Space > Play->CoatSize) {
        clear_line(22);
        /* Error - player tried to buy a gun that he/she doesn't have
         * space for (%tde="gun" by default) */
        text = dpg_strdup_printf(_("You don't have enough space to "
                                   "carry that %tde!"), Names.Gun);
        mvaddstr(22, (Width - strlen(text)) / 2, text);
        g_free(text);
        nice_wait();
        clear_line(23);
      } else if (Gun[gunind].Price > Play->Cash) {
        clear_line(22);
        /* Error - player tried to buy a gun that he/she can't afford
         * (%tde="gun" by default) */
        text = dpg_strdup_printf(_("You don't have enough cash to buy "
                                   "that %tde!"), Names.Gun);
        mvaddstr(22, (Width - strlen(text)) / 2, text);
        g_free(text);
        nice_wait();
        clear_line(23);
      } else {
        Play->Cash -= Gun[gunind].Price;
        Play->CoatSize -= Gun[gunind].Space;
        Play->Guns[gunind].Carried++;
        text = g_strdup_printf("gun^%d^1", gunind);
        SendClientMessage(Play, C_NONE, C_BUYOBJECT, NULL, text);
        g_free(text);
        print_status(Play, FALSE);
      }
    }
  }
}

/* 
 * Allows player "Play" to buy and sell guns interactively. Passes the
 * decisions on to the server for sanity checking and implementation.
 */
void GunShop(Player *Play)
{
  int i, action;
  gchar *text;

  print_status(Play, FALSE);
  attrset(TextAttr);
  clear_bottom();
  for (i = 0; i < NumGun; i++) {
    text =
        dpg_strdup_printf("%c. %-22tde %12P", 'A' + i, Gun[i].Name,
                          Gun[i].Price);
    mvaddstr(17 + i / 2, (i % 2) * 40 + 1, text);
    g_free(text);
  }
  do {
    /* Prompt for actions in the gun shop */
    text = _("Will you B>uy, S>ell, or L>eave? ");
    attrset(PromptAttr);
    clear_line(22);
    mvaddstr(22, 40 - strlen(text) / 2, text);
    attrset(TextAttr);

    /* Translate these three keys in line with the above options, keeping
     * the order (B>uy, S>ell, L>eave) the same - you can change the
     * wording of the prompt, but if you change the order in this key
     * list, the keys will do the wrong things! */
    action = GetKey(_("BSL"), "BSL", FALSE, FALSE, FALSE);
    if (action == 'S')
      SellGun(Play);
    else if (action == 'B')
      BuyGun(Play);
  } while (action != 'L');
  print_status(Play, TRUE);
}

/* 
 * Allows player "Play" to pay off loans interactively.
 */
void LoanShark(Player *Play)
{
  gchar *text, *prstr;
  price_t money;

  do {
    clear_bottom();
    attrset(PromptAttr);

    /* Prompt for paying back loans from the loan shark */
    text =
        nice_input(_("How much money do you pay back? "), 19, 1, TRUE,
                   NULL, '\0');
    attrset(TextAttr);
    money = strtoprice(text);
    g_free(text);
    if (money < 0)
      money = 0;
    if (money > Play->Debt)
      money = Play->Debt;
    if (money > Play->Cash) {
      /* Error - player doesn't have enough money to pay back the loan */
      mvaddstr(20, 1, _("You don't have that much money!"));
      nice_wait();
    } else {
      SendClientMessage(Play, C_NONE, C_PAYLOAN, NULL,
                        (prstr = pricetostr(money)));
      g_free(prstr);
      money = 0;
    }
  } while (money != 0);
}

/* 
 * Allows player "Play" to pay in or withdraw money from the bank
 * interactively.
 */
void Bank(Player *Play)
{
  gchar *text, *prstr;
  price_t money = 0;
  int action;

  do {
    clear_bottom();
    attrset(PromptAttr);
    /* Prompt for dealing with the bank in the curses client */
    mvaddstr(18, 1, _("Do you want to D>eposit money, W>ithdraw money, "
                      "or L>eave ? "));
    attrset(TextAttr);

    /* Make sure you keep the order the same if you translate these keys!
     * (D>eposit, W>ithdraw, L>eave) */
    action = GetKey(_("DWL"), "DWL", FALSE, FALSE, FALSE);

    if (action == 'D' || action == 'W') {
      /* Prompt for putting money in or taking money out of the bank */
      text = nice_input(_("How much money? "), 19, 1, TRUE, NULL, '\0');

      money = strtoprice(text);
      g_free(text);
      if (money < 0)
        money = 0;
      if (action == 'W')
        money = -money;
      if (money > Play->Cash) {
        /* Error - player has tried to put more money into the bank than
         * he/she has */
        mvaddstr(20, 1, _("You don't have that much money!"));
        nice_wait();
      } else if (-money > Play->Bank) {
        /* Error - player has tried to withdraw more money from the bank
         * than there is in the account */
        mvaddstr(20, 1, _("There isn't that much money in the bank..."));
        nice_wait();
      } else if (money != 0) {
        SendClientMessage(Play, C_NONE, C_DEPOSIT, NULL,
                          (prstr = pricetostr(money)));
        g_free(prstr);
        money = 0;
      }
    }
  } while (action != 'L' && money != 0);
}

/* 
 * Waits for keyboard input; will only accept a key listed in the
 * "allowed" string. This string may have been translated; thus
 * the "orig_allowed" string contains the untranslated keys.
 * Returns the untranslated key corresponding to the key pressed
 * (e.g. if allowed[2] is pressed, orig_allowed[2] is returned)
 * Case insensitive. If "AllowOther" is TRUE, keys other than the
 * given selection are allowed, and cause a zero return value.
 * If "PrintAllowed" is TRUE, the allowed keys are printed after
 * the prompt. If "ExpandOut" is also TRUE, the full words for
 * the commands, rather than just their first letters, are displayed.
 */
int GetKey(char *allowed, char *orig_allowed, gboolean AllowOther,
           gboolean PrintAllowed, gboolean ExpandOut)
{
  int ch;
  guint AllowInd, WordInd, i;

  /* Expansions of the single-letter keypresses for the benefit of the
   * user. i.e. "Yes" is printed for the key "Y" etc. You should indicate
   * to the user which letter in the word corresponds to the keypress, by
   * capitalising it or similar. */
  gchar *Words[] = { N_("Y:Yes"), N_("N:No"), N_("R:Run"),
    N_("F:Fight"), N_("A:Attack"), N_("E:Evade")
  };
  guint numWords = sizeof(Words) / sizeof(Words[0]);
  gchar *trWord;

  curs_set(1);
  ch = '\0';

  if (!allowed || strlen(allowed) == 0)
    return 0;

  if (PrintAllowed) {
    addch('[' | TextAttr);
    for (AllowInd = 0; AllowInd < strlen(allowed); AllowInd++) {
      if (AllowInd > 0)
        addch('/' | TextAttr);
      WordInd = 0;
      while (WordInd < numWords &&
             orig_allowed[AllowInd] != Words[WordInd][0])
        WordInd++;

      if (ExpandOut && WordInd < numWords) {
        trWord = _(Words[WordInd]);
        for (i = 2; i < strlen(trWord); i++)
          addch((guchar)trWord[i] | TextAttr);
      } else
        addch((guchar)allowed[AllowInd] | TextAttr);
    }
    addch(']' | TextAttr);
    addch(' ' | TextAttr);
  }

  do {
    ch = bgetch();
    ch = toupper(ch);
    for (AllowInd = 0; AllowInd < strlen(allowed); AllowInd++) {
      if (allowed[AllowInd] == ch) {
        addch((guint)ch | TextAttr);
        curs_set(0);
        return orig_allowed[AllowInd];
      }
    }
  } while (!AllowOther);

  curs_set(0);
  return 0;
}

/* 
 * Clears one whole line on the curses screen.
 */
void clear_line(int line)
{
  int i;

  move(line, 0);
  for (i = 0; i < Width; i++)
    addch(' ');
}

/* 
 * Clears the bottom of the screen (i.e. from line 16 to line 23)
 * except for the top "skip" lines.
 */
void clear_exceptfor(int skip)
{
  int i;

  for (i = 16 + skip; i <= 23; i++)
    clear_line(i);
}


/* 
 * Clears screen lines 16 to 23.
 */
void clear_bottom(void)
{
  int i;

  for (i = 16; i <= 23; i++)
    clear_line(i);
}

/* 
 * Clears the entire screen; 24 lines of 80 characters each.
 */
void clear_screen(void)
{
  int i;

  for (i = 0; i < Depth; i++)
    clear_line(i);
}

/* 
 * Displays a prompt on the bottom screen line and waits for the user
 * to press a key.
 */
void nice_wait()
{
  gchar *text;

  attrset(PromptAttr);
  text = _("Press any key...");
  mvaddstr(23, (Width - strlen(text)) / 2, text);
  bgetch();
  attrset(TextAttr);
}

/* 
 * Handles the display of messages pertaining to player-player fights
 * in the lower part of screen (fighting sub-screen). Adds the new line
 * of text in "text" and scrolls up previous messages if necessary
 * If "text" is NULL, initialises the area
 * If "text" is a blank string, redisplays the message area
 * Messages are displayed from lines 16 to 20; line 22 is used for
 * the prompt for the user.
 */
void DisplayFightMessage(Player *Play, char *text)
{
  static char Messages[5][79];
  static int x, y;
  gchar *textpt;
  gchar *AttackName, *DefendName, *BitchName;
  gint i, DefendHealth, DefendBitches, BitchesKilled, ArmPercent;
  gboolean Loot;

  if (text == NULL) {
    x = 0;
    y = 15;
    for (i = 0; i < 5; i++)
      Messages[i][0] = '\0';
  } else if (!text[0]) {
    attrset(TextAttr);
    clear_bottom();
    for (i = 16; i <= 20; i++)
      mvaddstr(i, 1, Messages[i - 16]);
  } else {
    if (HaveAbility(Play, A_NEWFIGHT)) {
      ReceiveFightMessage(text, &AttackName, &DefendName, &DefendHealth,
                          &DefendBitches, &BitchName, &BitchesKilled,
                          &ArmPercent, &fp, &RunHere, &Loot, &CanFire,
                          &textpt);
    } else {
      textpt = text;
      if (Play->Flags & FIGHTING)
        fp = F_MSG;
      else
        fp = F_LASTLEAVE;
      CanFire = (Play->Flags & CANSHOOT);
      RunHere = FALSE;
    }
    while (textpt[0]) {
      if (y < 20)
        y++;
      else
        for (i = 0; i < 4; i++)
          strcpy(Messages[i], Messages[i + 1]);

      strncpy(Messages[y - 16], textpt, 78);
      Messages[y - 16][78] = '\0';
      textpt += MIN(strlen(textpt), 78);
    }
  }
}

/* 
 * Displays a network message "buf" in the message area (lines
 * 10 to 14) scrolling previous messages up.
 * If "buf" is NULL, clears the message area
 * If "buf" is a blank string, redisplays the message area
 */
void display_message(char *buf)
{
  guint x, y;
  guint wid;
  static gchar Messages[5][200];
  gchar *bufpt;

  if (Width <= 4)
    return;

  wid = MIN(Width - 4, 200);

  if (!buf) {
    for (y = 0; y < 5; y++) {
      memset(Messages[y], ' ', 200);
      if (Network) {
        mvaddch(y + 10, 0, ' ' | TextAttr);
        addch(ACS_VLINE | StatsAttr);
        for (x = 0; x < wid; x++)
          addch(' ' | StatsAttr);
        addch(ACS_VLINE | StatsAttr);
        addch(' ' | TextAttr);
      }
    }
  } else if (Network) {
    bufpt = buf;
    while (bufpt[0] != 0) {
      memmove(Messages[0], Messages[1], 200 * 4);
      memset(Messages[4], ' ', 200);
      memcpy(Messages[4], bufpt,
             strlen(bufpt) > wid ? wid : strlen(bufpt));
      bufpt += MIN(strlen(bufpt), wid);
    }
    for (y = 0; y < 5; y++)
      for (x = 0; x < wid; x++) {
        mvaddch(y + 10, x + 2, (guchar)Messages[y][x] | StatsAttr);
      }
    refresh();
  }
}

/* 
 * Displays the string "text" at the top of the screen. Usually used for
 * displaying the current location or the "Subway" flash.
 */
void print_location(char *text)
{
  int i;

  if (!text)
    return;
  attrset(LocationAttr);
  move(0, Width / 2 - 9);
  for (i = 0; i < 18; i++)
    addch(' ');
  mvaddstr(0, (Width - strlen(text)) / 2, text);
  attrset(TextAttr);
}

/* 
 * Displays the status of player "Play" - i.e. the current turn, the
 * location, bitches, available space, cash, guns, health and bank
 * details. If "DispDrugs" is TRUE, displays the carried drugs on the
 * right hand side of the screen; if FALSE, displays the carried guns.
 */
void print_status(Player *Play, gboolean DispDrug)
{
  int i, c;
  GString *text;

  text = g_string_new(NULL);
  attrset(TitleAttr);
  clear_line(0);
  g_string_sprintf(text, "%s%02d%s", Names.Month, Play->Turn, Names.Year);
  mvaddstr(0, 3, text->str);

  attrset(StatsAttr);
  for (i = 2; i <= 14; i++) {
    mvaddch(i, 1, ACS_VLINE);
    mvaddch(i, Width - 2, ACS_VLINE);
  }
  mvaddch(1, 1, ACS_ULCORNER);
  for (i = 0; i < Width - 4; i++)
    addch(ACS_HLINE);
  addch(ACS_URCORNER);

  mvaddch(1, Width / 2, ACS_TTEE);
  for (i = 2; i <= (Network ? 8 : 13); i++) {
    move(i, 2);
    for (c = 2; c < Width / 2; c++)
      addch(' ');
    addch(ACS_VLINE);
    for (c = Width / 2 + 1; c < Width - 2; c++)
      addch(' ');
  }
  if (!Network) {
    mvaddch(14, 1, ACS_LLCORNER);
    for (i = 0; i < Width - 4; i++)
      addch(ACS_HLINE);
    addch(ACS_LRCORNER);
    mvaddch(14, Width / 2, ACS_BTEE);
  } else {
    mvaddch(9, 1, ACS_LTEE);
    for (i = 0; i < Width - 4; i++)
      addch(ACS_HLINE);
    addch(ACS_RTEE);

    /* Title of the "Messages" window in the curses client */
    mvaddstr(9, 15, _("Messages"));

    mvaddch(9, Width / 2, ACS_BTEE);
    mvaddch(15, 1, ACS_LLCORNER);
    for (i = 0; i < Width - 4; i++)
      addch(ACS_HLINE);
    addch(ACS_LRCORNER);
  }

  /* Title of the "Stats" window in the curses client */
  mvaddstr(1, Width / 4 - 2, _("Stats"));

  attrset(StatsAttr);

  /* Display of the player's cash in the stats window (careful to keep the
   * formatting if you change the length of the "Cash" word) */
  dpg_string_sprintf(text, _("Cash %17P"), Play->Cash);
  mvaddstr(3, 9, text->str);

  /* Display of the total number of guns carried (%Tde="Guns" by default) */
  dpg_string_sprintf(text, _("%-19Tde%3d"), Names.Guns,
                     TotalGunsCarried(Play));
  mvaddstr(Network ? 4 : 5, 9, text->str);

  /* Display of the player's health */
  g_string_sprintf(text, _("Health             %3d"), Play->Health);
  mvaddstr(Network ? 5 : 7, 9, text->str);

  /* Display of the player's bank balance */
  dpg_string_sprintf(text, _("Bank %17P"), Play->Bank);
  mvaddstr(Network ? 6 : 9, 9, text->str);

  if (Play->Debt > 0)
    attrset(DebtAttr);
  /* Display of the player's debt */
  dpg_string_sprintf(text, _("Debt %17P"), Play->Debt);
  mvaddstr(Network ? 7 : 11, 9, text->str);
  attrset(TitleAttr);

  /* Display of the player's trenchcoat size (antique mode only) */
  if (WantAntique)
    g_string_sprintf(text, _("Space %6d"), Play->CoatSize);
  else {
    /* Display of the player's number of bitches, and available space
     * (%Tde="Bitches" by default) */
    dpg_string_sprintf(text, _("%Tde %3d  Space %6d"), Names.Bitches,
                       Play->Bitches.Carried, Play->CoatSize);
  }
  mvaddstr(0, Width - 2 - strlen(text->str), text->str);
  print_location(Location[(int)Play->IsAt].Name);
  attrset(StatsAttr);

  c = 0;
  if (DispDrug) {
    /* Title of the "trenchcoat" window (antique mode only) */
    if (WantAntique)
      mvaddstr(1, Width * 3 / 4 - 5, _("Trenchcoat"));
    else {
      /* Title of the "drugs" window (the only important bit in this
       * string is the "%Tde" which is "Drugs" by default; the %/.../ part 
       * is ignored, so you don't need to translate it; see doc/i18n.html) 
       */
      dpg_string_sprintf(text, _("%/Stats: Drugs/%Tde"), Names.Drugs);
      mvaddstr(1, Width * 3 / 4 - strlen(text->str) / 2, text->str);
    }
    for (i = 0; i < NumDrug; i++) {
      if (Play->Drugs[i].Carried > 0) {
        /* Display of carried drugs with price (%tde="Opium", etc. by
         * default) */
        if (HaveAbility(Play, A_DRUGVALUE)) {
          dpg_string_sprintf(text, _("%-7tde  %3d @ %P"), Drug[i].Name,
                             Play->Drugs[i].Carried,
                             Play->Drugs[i].TotalValue /
                             Play->Drugs[i].Carried);
          mvaddstr(3 + c, Width / 2 + 3, text->str);
        } else {
          /* Display of carried drugs (%tde="Opium", etc. by default) */
          dpg_string_sprintf(text, _("%-7tde  %3d"), Drug[i].Name,
                             Play->Drugs[i].Carried);
          mvaddstr(3 + c / 2, Width / 2 + 3 + (c % 2) * 17, text->str);
        }
        c++;
      }
    }
  } else {
    /* Title of the "guns" window (the only important bit in this string
     * is the "%Tde" which is "Guns" by default) */
    dpg_string_sprintf(text, _("%/Stats: Guns/%Tde"), Names.Guns);
    mvaddstr(1, Width * 3 / 4 - strlen(text->str) / 2, text->str);
    for (i = 0; i < NumGun; i++) {
      if (Play->Guns[i].Carried > 0) {
        /* Display of carried guns (%tde="Baretta", etc. by default) */
        dpg_string_sprintf(text, _("%-22tde %3d"), Gun[i].Name,
                           Play->Guns[i].Carried);
        mvaddstr(3 + c, Width / 2 + 3, text->str);
        c++;
      }
    }
  }
  attrset(TextAttr);
  if (!Network)
    clear_line(15);
  refresh();
  g_string_free(text, TRUE);
}

/* 
 * Parses details about player "From" from string "Data" and then
 * displays the lot, drugs and guns.
 */
void DisplaySpyReports(char *Data, Player *From, Player *To)
{
  gchar *text;

  ReceivePlayerData(To, Data, From);

  clear_bottom();
  text = g_strdup_printf(_("Spy reports for %s"), GetPlayerName(From));
  mvaddstr(17, 1, text);
  g_free(text);

  /* Message displayed with a spy's list of drugs (%Tde="Drugs" by
   * default) */
  text = dpg_strdup_printf(_("%/Spy: Drugs/%Tde..."), Names.Drugs);
  mvaddstr(19, 20, text);
  g_free(text);
  print_status(From, TRUE);
  nice_wait();
  clear_line(19);

  /* Message displayed with a spy's list of guns (%Tde="Guns" by default) */
  text = dpg_strdup_printf(_("%/Spy: Guns/%Tde..."), Names.Guns);
  mvaddstr(19, 20, text);
  g_free(text);
  print_status(From, FALSE);
  nice_wait();

  print_status(To, TRUE);
  refresh();
}

/* 
 * Displays the "Prompt" if non-NULL, and then lists all clients
 * currently playing dopewars, other than the current player "Play".
 * If "Select" is TRUE, gives each player a letter and asks the user
 * to select one, which is returned by the function.
 */
Player *ListPlayers(Player *Play, gboolean Select, char *Prompt)
{
  Player *tmp = NULL;
  GSList *list;
  int i, c;
  gchar *text;

  attrset(TextAttr);
  clear_bottom();
  if (!FirstClient || (!g_slist_next(FirstClient) &&
                       FirstClient->data == Play)) {
    text = _("No other players are currently logged on!");
    mvaddstr(18, (Width - strlen(text)) / 2, text);
    nice_wait();
    return 0;
  }
  mvaddstr(16, 1, _("Players currently logged on:-"));

  i = 0;
  for (list = FirstClient; list; list = g_slist_next(list)) {
    tmp = (Player *)list->data;
    if (strcmp(GetPlayerName(tmp), GetPlayerName(Play)) == 0)
      continue;
    if (Select)
      text = g_strdup_printf("%c. %s", 'A' + i, GetPlayerName(tmp));
    else
      text = g_strdup(GetPlayerName(tmp));
    mvaddstr(17 + i / 2, (i % 2) * 40 + 1, text);
    g_free(text);
    i++;
  }

  if (Prompt) {
    attrset(PromptAttr);
    mvaddstr(22, 10, Prompt);
    attrset(TextAttr);
  }
  if (Select) {
    curs_set(1);
    attrset(TextAttr);
    c = 0;
    while (c < 'A' || c >= 'A' + i) {
      c = bgetch();
      c = toupper(c);
    }
    if (Prompt)
      addch((guint)c);
    list = FirstClient;
    while (c >= 'A') {
      if (list != FirstClient)
        list = g_slist_next(list);
      tmp = (Player *)list->data;
      while (strcmp(GetPlayerName(tmp), GetPlayerName(Play)) == 0) {
        list = g_slist_next(list);
        tmp = (Player *)list->data;
      }
      c--;
    }
    return tmp;
  } else {
    nice_wait();
  }
  return NULL;
}

/* 
 * Displays the given "prompt" (if non-NULL) at coordinates sx,sy and
 * allows the user to input a string, which is returned. This is a
 * dynamically allocated string, and so must be freed by the calling
 * routine. If "digitsonly" is TRUE, the user will be permitted only to
 * input numbers, although the suffixes m and k are allowed (the
 * strtoprice routine understands this notation for a 1000000 or 1000
 * multiplier) as well as a decimal point (. or ,)
 * If "displaystr" is non-NULL, it is taken as a default response.
 * If "passwdchar" is non-zero, it is displayed instead of the user's
 * keypresses (e.g. for entering passwords)
 */
char *nice_input(char *prompt, int sy, int sx, gboolean digitsonly,
                 char *displaystr, char passwdchar)
{
  int i, c, x;
  gboolean DecimalPoint, Suffix;
  GString *text;
  gchar *ReturnString;

  DecimalPoint = Suffix = FALSE;

  x = sx;
  move(sy, x);
  if (prompt) {
    attrset(PromptAttr);
    addstr(prompt);
    x += strlen(prompt);
  }
  attrset(TextAttr);
  if (displaystr) {
    if (passwdchar) {
      for (i = strlen(displaystr); i; i--)
        addch((guint)passwdchar);
    } else {
      addstr(displaystr);
    }
    i = strlen(displaystr);
    text = g_string_new(displaystr);
  } else {
    i = 0;
    text = g_string_new("");
  }

  curs_set(1);
  do {
    move(sy + (x + i) / Width, (x + i) % Width);
    c = bgetch();
    if ((c == 8 || c == KEY_BACKSPACE || c == 127) && i > 0) {
      move(sy + (x + i - 1) / Width, (x + i - 1) % Width);
      addch(' ');
      i--;
      if (DecimalPoint && text->str[i] == '.')
        DecimalPoint = FALSE;
      if (Suffix)
        Suffix = FALSE;
      g_string_truncate(text, i);
    } else if (!Suffix) {
      if ((digitsonly && c >= '0' && c <= '9') ||
          (!digitsonly && c >= 32 && c != '^' && c < 127)) {
        g_string_append_c(text, c);
        i++;
        addch((guint)passwdchar ? passwdchar : c);
      } else if (digitsonly && (c == '.' || c == ',') && !DecimalPoint) {
        g_string_append_c(text, '.');
        i++;
        addch((guint)passwdchar ? passwdchar : c);
        DecimalPoint = TRUE;
      } else if (digitsonly
                 && (c == 'M' || c == 'm' || c == 'k' || c == 'K')
                 && !Suffix) {
        g_string_append_c(text, c);
        i++;
        addch((guint)passwdchar ? passwdchar : c);
        Suffix = TRUE;
      }
    }
  } while (c != '\n' && c != KEY_ENTER);
  curs_set(0);
  move(sy, x);
  ReturnString = text->str;
  g_string_free(text, FALSE);   /* Leave the buffer to return */
  return ReturnString;
}

/* 
 * Loop which handles the user playing an interactive game (i.e. "Play"
 * is a client connected to a server, either locally or remotely)
 * dopewars is essentially server-driven, so this loop simply has to
 * make the screen look pretty, respond to user keypresses, and react
 * to messages from the server.
 */
static void Curses_DoGame(Player *Play)
{
  gchar *buf, *OldName, *TalkMsg;
  GString *text;
  int i, c;
  char IsCarrying;

#if NETWORKING || HAVE_SELECT
  fd_set readfs;
#endif
#ifdef NETWORKING
  fd_set writefs;
  gboolean DoneOK;
  gchar *pt;
  gboolean justconnected = FALSE;
#endif
  int NumDrugsHere;
  int MaxSock;
  char HaveWorthless;
  Player *tmp;
  struct sigaction sact;

  DisplayMode = DM_NONE;
  QuitRequest = FALSE;

  ResizedFlag = 0;
  sact.sa_handler = ResizeHandle;
  sact.sa_flags = 0;
  sigemptyset(&sact.sa_mask);
  if (sigaction(SIGWINCH, &sact, NULL) == -1) {
    g_warning(_("Cannot install SIGWINCH interrupt handler!"));
  }
  OldName = g_strdup(GetPlayerName(Play));
  attrset(TextAttr);
  clear_screen();
  display_message(NULL);
  DisplayFightMessage(Play, NULL);
  print_status(Play, TRUE);

  attrset(TextAttr);
  clear_bottom();
  buf = NULL;
  do {
    g_free(buf);
    buf =
        nice_input(_("Hey dude, what's your name? "), 17, 1, FALSE,
                   OldName, '\0');
  } while (buf[0] == 0);
#if NETWORKING
  if (WantNetwork) {
    if (!ConnectToServer(Play)) {
      end_curses();
      exit(1);
    }
    justconnected = TRUE;
  }
#endif /* NETWORKING */
  print_status(Play, TRUE);
  display_message("");

  InitAbilities(Play);
  SendAbilities(Play);
  SetPlayerName(Play, buf);
  SendNullClientMessage(Play, C_NONE, C_NAME, NULL, buf);
  g_free(buf);
  g_free(OldName);

  text = g_string_new("");

  while (1) {
    if (Play->Health == 0)
      DisplayMode = DM_NONE;
    HaveWorthless = 0;
    IsCarrying = 0;
    for (i = 0; i < NumDrug; i++) {
      if (Play->Drugs[i].Carried > 0) {
        IsCarrying = 1;
        if (Play->Drugs[i].Price == 0)
          HaveWorthless = 1;
      }
    }
    switch (DisplayMode) {
    case DM_STREET:
      attrset(TextAttr);
      NumDrugsHere = 0;
      for (i = 0; i < NumDrug; i++)
        if (Play->Drugs[i].Price > 0)
          NumDrugsHere++;
      clear_bottom();
      /* Display of drug prices (%tde="drugs" by default) */
      dpg_string_sprintf(text, _("Hey dude, the prices of %tde here are:"),
                         Names.Drugs);
      mvaddstr(16, 1, text->str);
      for (c = 0, i = GetNextDrugIndex(-1, Play);
           c < NumDrugsHere && i != -1;
           c++, i = GetNextDrugIndex(i, Play)) {
        /* List of individual drug names for selection (%tde="Opium" etc.
         * by default) */
        dpg_string_sprintf(text, _("%c. %-10tde %8P"), 'A' + c,
                           Drug[i].Name, Play->Drugs[i].Price);
        mvaddstr(17 + c / 3, (c % 3) * 25 + 4, text->str);
      }
      attrset(PromptAttr);
      /* Prompts for "normal" actions in curses client */
      g_string_assign(text, _("Will you B>uy"));
      if (IsCarrying)
        g_string_append(text, _(", S>ell"));
      if (HaveWorthless && !WantAntique)
        g_string_append(text, _(", D>rop"));
      if (Network)
        g_string_append(text, _(", T>alk, P>age, L>ist"));
      if (!WantAntique && (Play->Bitches.Carried > 0 ||
                           Play->Flags & SPYINGON)) {
        g_string_append(text, _(", G>ive"));
      }
      if (Play->Flags & FIGHTING) {
        g_string_append(text, _(", F>ight"));
      } else {
        g_string_append(text, _(", J>et"));
      }
      g_string_append(text, _(", or Q>uit? "));
      mvaddstr(22, 40 - strlen(text->str) / 2, text->str);
      attrset(TextAttr);
      curs_set(1);
      break;
    case DM_FIGHT:
      DisplayFightMessage(Play, "");
      attrset(PromptAttr);
      /* Prompts for actions during fights in curses client */
      g_string_assign(text, _("Do you "));
      if (CanFire) {
        if (TotalGunsCarried(Play) > 0) {
          g_string_append(text, _("F>ight, "));
        } else {
          g_string_append(text, _("S>tand, "));
        }
      }
      if (fp != F_LASTLEAVE)
        g_string_append(text, _("R>un, "));
      if (!RunHere || fp == F_LASTLEAVE)
        /* (%tde = "drugs" by default here) */
        dpg_string_sprintfa(text, _("D>eal %tde, "), Names.Drugs);
      g_string_append(text, _("or Q>uit? "));
      mvaddstr(22, 40 - strlen(text->str) / 2, text->str);
      attrset(TextAttr);
      curs_set(1);
      break;
    case DM_DEAL:
      attrset(TextAttr);
      clear_bottom();
      mvaddstr(16, 1, "Your trade:-");
      mvaddstr(19, 1, "His trade:-");
      g_string_assign(text, "Do you A>dd, R>emove, O>K, D>eal ");
      g_string_append(text, Names.Drugs);
      g_string_append(text, ", or Q>uit? ");
      attrset(PromptAttr);
      mvaddstr(22, 40 - strlen(text->str) / 2, text->str);
      attrset(TextAttr);
      curs_set(1);
      break;
    case DM_NONE:
      break;
    }
    refresh();

    if (QuitRequest)
      return;
#if NETWORKING
    FD_ZERO(&readfs);
    FD_ZERO(&writefs);
    FD_SET(0, &readfs);
    MaxSock = 1;
    if (Client) {
      if (justconnected) {
        /* Deal with any messages that came in while we were connect()ing */
        justconnected = FALSE;
        while ((pt = GetWaitingPlayerMessage(Play)) != NULL) {
          HandleClientMessage(pt, Play);
          g_free(pt);
        }
        if (QuitRequest)
          return;
      }
      SetSelectForNetworkBuffer(&Play->NetBuf, &readfs, &writefs,
                                NULL, &MaxSock);
    }
    if (bselect(MaxSock, &readfs, &writefs, NULL, NULL) == -1) {
      if (errno == EINTR) {
        CheckForResize(Play);
        continue;
      }
      perror("bselect");
      exit(1);
    }
    if (Client) {
      if (RespondToSelect(&Play->NetBuf, &readfs, &writefs, NULL, &DoneOK)) {
        while ((pt = GetWaitingPlayerMessage(Play)) != NULL) {
          HandleClientMessage(pt, Play);
          g_free(pt);
        }
        if (QuitRequest)
          return;
      }
      if (!DoneOK) {
        attrset(TextAttr);
        clear_line(22);
        mvaddstr(22, 0, _("Connection to server lost! "
                          "Reverting to single player mode"));
        nice_wait();
        SwitchToSinglePlayer(Play);
        print_status(Play, TRUE);
      }
    }
    if (FD_ISSET(0, &readfs)) {
#elif HAVE_SELECT
    FD_ZERO(&readfs);
    FD_SET(0, &readfs);
    MaxSock = 1;
    if (bselect(MaxSock, &readfs, NULL, NULL, NULL) == -1) {
      if (errno == EINTR) {
        CheckForResize(Play);
        continue;
      }
      perror("bselect");
      exit(1);
    }
#endif /* NETWORKING */
    if (DisplayMode == DM_STREET) {
      /* N.B. You must keep the order of these keys the same as the
       * original when you translate (B>uy, S>ell, D>rop, T>alk, P>age,
       * L>ist, G>ive errand, F>ight, J>et, Q>uit) */
      c = GetKey(_("BSDTPLGFJQ"), "BSDTPLGFJQ", TRUE, FALSE, FALSE);

    } else if (DisplayMode == DM_FIGHT) {
      /* N.B. You must keep the order of these keys the same as the
       * original when you translate (D>eal drugs, R>un, F>ight, S>tand,
       * Q>uit) */
      c = GetKey(_("DRFSQ"), "DRFSQ", TRUE, FALSE, FALSE);

    } else
      c = 0;
#if ! (NETWORKING || HAVE_SELECT)
    CheckForResize(Play);
#endif
    if (DisplayMode == DM_STREET) {
      if (c == 'J' && !(Play->Flags & FIGHTING)) {
        jet(Play, TRUE);
      } else if (c == 'F' && Play->Flags & FIGHTING) {
        DisplayMode = DM_FIGHT;
      } else if (c == 'T' && Play->Flags & TRADING) {
        DisplayMode = DM_DEAL;
      } else if (c == 'B') {
        DealDrugs(Play, TRUE);
      } else if (c == 'S' && IsCarrying) {
        DealDrugs(Play, FALSE);
      } else if (c == 'D' && HaveWorthless && !WantAntique) {
        DropDrugs(Play);
      } else if (c == 'G' && !WantAntique && Play->Bitches.Carried > 0) {
        GiveErrand(Play);
      } else if (c == 'Q') {
        if (want_to_quit() == 1) {
          DisplayMode = DM_NONE;
          clear_bottom();
          SendClientMessage(Play, C_NONE, C_WANTQUIT, NULL, NULL);
        }
      } else if (c == 'L' && Network) {
        attrset(PromptAttr);
        mvaddstr(23, 20, _("List what? P>layers or S>cores? "));
        /* P>layers, S>cores */
        i = GetKey(_("PS"), "PS", TRUE, FALSE, FALSE);
        if (i == 'P') {
          ListPlayers(Play, FALSE, NULL);
        } else if (i == 'S') {
          DisplayMode = DM_NONE;
          SendClientMessage(Play, C_NONE, C_REQUESTSCORE, NULL, NULL);
        }
      } else if (c == 'P' && Network) {
        tmp = ListPlayers(Play, TRUE,
                          _("Whom do you want to page "
                            "(talk privately to) ? "));
        if (tmp) {
          attrset(TextAttr);
          clear_line(22);
          /* Prompt for sending player-player messages */
          TalkMsg = nice_input(_("Talk: "), 22, 0, FALSE, NULL, '\0');
          if (TalkMsg[0]) {
            SendClientMessage(Play, C_NONE, C_MSGTO, tmp, TalkMsg);
            buf = g_strdup_printf("%s->%s: %s", GetPlayerName(Play),
                                  GetPlayerName(tmp), TalkMsg);
            display_message(buf);
            g_free(buf);
          }
          g_free(TalkMsg);
        }
      } else if (c == 'T' && Client) {
        attrset(TextAttr);
        clear_line(22);
        TalkMsg = nice_input(_("Talk: "), 22, 0, FALSE, NULL, '\0');
        if (TalkMsg[0]) {
          SendClientMessage(Play, C_NONE, C_MSG, NULL, TalkMsg);
          buf = g_strdup_printf("%s: %s", GetPlayerName(Play), TalkMsg);
          display_message(buf);
          g_free(buf);
        }
        g_free(TalkMsg);
      }
    } else if (DisplayMode == DM_FIGHT) {
      switch (c) {
      case 'D':
        DisplayMode = DM_STREET;
        if (!(Play->Flags & FIGHTING) && HaveAbility(Play, A_DONEFIGHT)) {
          SendClientMessage(Play, C_NONE, C_DONE, NULL, NULL);
        }
        break;
      case 'R':
        if (RunHere) {
          SendClientMessage(Play, C_NONE, C_FIGHTACT, NULL, "R");
        } else {
          jet(Play, TRUE);
        }
        break;
      case 'F':
        if (TotalGunsCarried(Play) > 0 && CanFire) {
          buf = g_strdup_printf("%c", c);
          Play->Flags &= ~CANSHOOT;
          SendClientMessage(Play, C_NONE, C_FIGHTACT, NULL, buf);
          g_free(buf);
        }
        break;
      case 'S':
        if (TotalGunsCarried(Play) == 0 && CanFire) {
          buf = g_strdup_printf("%c", c);
          Play->Flags &= ~CANSHOOT;
          SendClientMessage(Play, C_NONE, C_FIGHTACT, NULL, buf);
          g_free(buf);
        }
        break;
      case 'Q':
        if (want_to_quit() == 1) {
          DisplayMode = DM_NONE;
          clear_bottom();
          SendClientMessage(Play, C_NONE, C_WANTQUIT, NULL, NULL);
        }
        break;
      }
    } else if (DisplayMode == DM_DEAL) {
      switch (c) {
      case 'D':
        DisplayMode = DM_STREET;
        break;
      case 'Q':
        if (want_to_quit() == 1) {
          DisplayMode = DM_NONE;
          clear_bottom();
          SendClientMessage(Play, C_NONE, C_WANTQUIT, NULL, NULL);
        }
        break;
      }
    }
#if NETWORKING
    }
#endif
    curs_set(0);
  }
  g_string_free(text, TRUE);
}

void CursesLoop(void)
{
  char c;
  Player *Play;

  if (!CheckHighScoreFileConfig())
    return;

  /* Save the configuration, so we can restore those elements that get
   * overwritten when we connect to a dopewars server */
  BackupConfig();

  start_curses();
  Width = COLS;
  Depth = LINES;

  /* Set up message handlers */
  ClientMessageHandlerPt = HandleClientMessage;

  /* Make the GLib log messages display nicely */
  g_log_set_handler(NULL,
                    LogMask() | G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_WARNING,
                    LogMessage, NULL);

  display_intro();

  Play = g_new(Player, 1);
  FirstClient = AddPlayer(0, Play, FirstClient);
  do {
    Curses_DoGame(Play);
    ShutdownNetwork(Play);
    CleanUpServer();
    RestoreConfig();
    attrset(TextAttr);
    mvaddstr(23, 20, _("Play again? "));
    c = GetKey(_("YN"), "YN", TRUE, TRUE, FALSE);
  } while (c == 'Y');
  FirstClient = RemovePlayer(Play, FirstClient);
  end_curses();
}
