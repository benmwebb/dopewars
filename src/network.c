/************************************************************************
 * network.c      Low-level networking routines                         *
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

#ifdef NETWORKING

#ifdef CYGWIN
#include <windows.h>            /* For datatypes such as BOOL */
#include <winsock.h>            /* For network functions */
#else
#include <sys/types.h>          /* For size_t etc. */
#include <sys/socket.h>         /* For struct sockaddr etc. */
#include <netinet/in.h>         /* For struct sockaddr_in etc. */
#include <arpa/inet.h>          /* For socklen_t */
#include <pwd.h>                /* For getpwuid */
#include <string.h>             /* For memcpy, strlen etc. */
#ifdef HAVE_UNISTD_H
#include <unistd.h>             /* For close(), various types and
                                 * constants */
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>              /* For fcntl() */
#endif
#include <netdb.h>              /* For gethostbyname() */
#endif /* CYGWIN */

#include <glib.h>
#include <errno.h>              /* For errno and Unix error codes */
#include <stdlib.h>             /* For exit() and atoi() */
#include <stdio.h>              /* For perror() */

#include "error.h"
#include "network.h"
#include "nls.h"

/* Maximum sizes (in bytes) of read and write buffers - connections should
 * be dropped if either buffer is filled */
#define MAXREADBUF   (32768)
#define MAXWRITEBUF  (65536)

/* SOCKS5 authentication method codes */
typedef enum {
  SM_NOAUTH = 0,                /* No authentication required */
  SM_GSSAPI = 1,                /* GSSAPI */
  SM_USERPASSWD = 2             /* Username/password authentication */
} SocksMethods;

static gboolean StartSocksNegotiation(NetworkBuffer *NetBuf,
                                      gchar *RemoteHost,
                                      unsigned RemotePort);
static gboolean StartConnect(int *fd, gchar *RemoteHost,
                             unsigned RemotePort, gboolean *doneOK,
                             LastError **error);

/*
 * g_strsplit from GLIB1 behaves differently to GLIB2, so we use this
 * wrapper function to give the GLIB2 behaviour in all circumstances.
 */
static gchar **my_strsplit(const gchar *string, const gchar *delim,
                           gint max_tokens)
{
#ifdef HAVE_GLIB2
  return g_strsplit(string, delim, max_tokens);
#else
  return g_strsplit(string, delim, max_tokens - 1);
#endif
}

#ifdef CYGWIN

void StartNetworking()
{
  WSADATA wsaData;
  LastError *error;
  GString *errstr;

  if (WSAStartup(MAKEWORD(1, 0), &wsaData) != 0) {
    error = NewError(ET_WINSOCK, WSAGetLastError(), NULL);
    errstr = g_string_new("");
    g_string_assign_error(errstr, error);
    g_log(NULL, G_LOG_LEVEL_CRITICAL, _("Cannot initialise WinSock (%s)!"),
          errstr->str);
    g_string_free(errstr, TRUE);
    FreeError(error);
    exit(1);
  }
}

void StopNetworking()
{
  WSACleanup();
}

void SetReuse(SOCKET sock)
{
  BOOL i = TRUE;

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&i,
                 sizeof(i)) == -1) {
    perror("setsockopt");
    exit(1);
  }
}

void SetBlocking(SOCKET sock, gboolean blocking)
{
  unsigned long param;

  param = blocking ? 0 : 1;
  ioctlsocket(sock, FIONBIO, &param);
}

#else

void StartNetworking()
{
}

void StopNetworking()
{
}

void SetReuse(int sock)
{
  int i = 1;

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) == -1) {
    perror("setsockopt");
    exit(1);
  }
}

void SetBlocking(int sock, gboolean blocking)
{
  fcntl(sock, F_SETFL, blocking ? 0 : O_NONBLOCK);
}

#endif /* CYGWIN */

static gboolean FinishConnect(int fd, LastError **error);

static void NetBufCallBack(NetworkBuffer *NetBuf, gboolean CallNow)
{
  if (NetBuf && NetBuf->CallBack) {
    (*NetBuf->CallBack) (NetBuf, NetBuf->status != NBS_PRECONNECT,
                         (NetBuf->status == NBS_CONNECTED
                          && NetBuf->WriteBuf.DataPresent)
                         || (NetBuf->status == NBS_SOCKSCONNECT
                             && NetBuf->negbuf.DataPresent)
                         || NetBuf->WaitConnect, CallNow);
  }
}

static void NetBufCallBackStop(NetworkBuffer *NetBuf)
{
  if (NetBuf && NetBuf->CallBack) {
    (*NetBuf->CallBack) (NetBuf, FALSE, FALSE, FALSE);
  }
}

static void InitConnBuf(ConnBuf *buf)
{
  buf->Data = NULL;
  buf->Length = 0;
  buf->DataPresent = 0;
}

static void FreeConnBuf(ConnBuf *buf)
{
  g_free(buf->Data);
  InitConnBuf(buf);
}

/* 
 * Initialises the passed network buffer, ready for use. Messages sent
 * or received on the buffered connection will be terminated by the
 * given character, and if they end in "StripChar" it will be removed
 * before the messages are sent or received.
 */
void InitNetworkBuffer(NetworkBuffer *NetBuf, char Terminator,
                       char StripChar, SocksServer *socks)
{
  NetBuf->fd = -1;
  NetBuf->InputTag = 0;
  NetBuf->CallBack = NULL;
  NetBuf->CallBackData = NULL;
  NetBuf->Terminator = Terminator;
  NetBuf->StripChar = StripChar;
  InitConnBuf(&NetBuf->ReadBuf);
  InitConnBuf(&NetBuf->WriteBuf);
  InitConnBuf(&NetBuf->negbuf);
  NetBuf->WaitConnect = FALSE;
  NetBuf->status = NBS_PRECONNECT;
  NetBuf->socks = socks;
  NetBuf->host = NULL;
  NetBuf->userpasswd = NULL;
  NetBuf->error = NULL;
}

void SetNetworkBufferCallBack(NetworkBuffer *NetBuf, NBCallBack CallBack,
                              gpointer CallBackData)
{
  NetBufCallBackStop(NetBuf);
  NetBuf->CallBack = CallBack;
  NetBuf->CallBackData = CallBackData;
  NetBufCallBack(NetBuf, FALSE);
}

/* 
 * Sets the function used to obtain a username and password for SOCKS5
 * username/password authentication.
 */
void SetNetworkBufferUserPasswdFunc(NetworkBuffer *NetBuf,
                                    NBUserPasswd userpasswd, gpointer data)
{
  NetBuf->userpasswd = userpasswd;
  NetBuf->userpasswddata = data;
}

/* 
 * Sets up the given network buffer to handle data being sent/received
 * through the given socket.
 */
void BindNetworkBufferToSocket(NetworkBuffer *NetBuf, int fd)
{
  NetBuf->fd = fd;
  SetBlocking(fd, FALSE);       /* We only deal with non-blocking sockets */
  NetBuf->status = NBS_CONNECTED;       /* Assume the socket is connected */
}

/* 
 * Returns TRUE if the pointer is to a valid network buffer, and it's
 * connected to an active socket.
 */
gboolean IsNetworkBufferActive(NetworkBuffer *NetBuf)
{
  return (NetBuf && NetBuf->fd >= 0);
}

gboolean StartNetworkBufferConnect(NetworkBuffer *NetBuf,
                                   gchar *RemoteHost, unsigned RemotePort)
{
  gchar *realhost;
  unsigned realport;
  gboolean doneOK;

  ShutdownNetworkBuffer(NetBuf);

  if (NetBuf->socks) {
    realhost = NetBuf->socks->name;
    realport = NetBuf->socks->port;
  } else {
    realhost = RemoteHost;
    realport = RemotePort;
  }

  if (StartConnect(&NetBuf->fd, realhost, realport, &doneOK,
                   &NetBuf->error)) {
    /* If we connected immediately, then set status, otherwise signal that 
     * we're waiting for the connect to complete */
    if (doneOK) {
      NetBuf->status = NetBuf->socks ? NBS_SOCKSCONNECT : NBS_CONNECTED;
      NetBuf->sockstat = NBSS_METHODS;
    } else {
      NetBuf->WaitConnect = TRUE;
    }

    if (NetBuf->socks
        && !StartSocksNegotiation(NetBuf, RemoteHost, RemotePort)) {
      return FALSE;
    }

    /* Notify the owner if necessary to check for the connection
     * completing and/or for data to be writeable */
    NetBufCallBack(NetBuf, FALSE);

    return TRUE;
  } else {
    return FALSE;
  }
}

/* 
 * Frees the network buffer's data structures (leaving it in the
 * 'initialised' state) and closes the accompanying socket.
 */
void ShutdownNetworkBuffer(NetworkBuffer *NetBuf)
{
  NetBufCallBackStop(NetBuf);

  if (NetBuf->fd >= 0)
    CloseSocket(NetBuf->fd);

  FreeConnBuf(&NetBuf->ReadBuf);
  FreeConnBuf(&NetBuf->WriteBuf);
  FreeConnBuf(&NetBuf->negbuf);

  FreeError(NetBuf->error);
  NetBuf->error = NULL;

  g_free(NetBuf->host);

  InitNetworkBuffer(NetBuf, NetBuf->Terminator, NetBuf->StripChar,
                    NetBuf->socks);
}

/* 
 * Updates the sets of read and write file descriptors to monitor
 * input to/output from the given network buffer. MaxSock is updated
 * with the highest-numbered file descriptor (plus 1) for use in a
 * later select() call.
 */
void SetSelectForNetworkBuffer(NetworkBuffer *NetBuf, fd_set *readfds,
                               fd_set *writefds, fd_set *errorfds,
                               int *MaxSock)
{
  if (!NetBuf || NetBuf->fd <= 0)
    return;
  FD_SET(NetBuf->fd, readfds);
  if (errorfds)
    FD_SET(NetBuf->fd, errorfds);
  if (NetBuf->fd >= *MaxSock)
    *MaxSock = NetBuf->fd + 1;
  if ((NetBuf->status == NBS_CONNECTED && NetBuf->WriteBuf.DataPresent)
      || (NetBuf->status == NBS_SOCKSCONNECT && NetBuf->negbuf.DataPresent)
      || NetBuf->WaitConnect) {
    FD_SET(NetBuf->fd, writefds);
  }
}

typedef enum {
  SEC_5FAILURE = 1,
  SEC_5RULESET = 2,
  SEC_5NETDOWN = 3,
  SEC_5UNREACH = 4,
  SEC_5CONNREF = 5,
  SEC_5TTLEXPIRED = 6,
  SEC_5COMMNOSUPP = 7,
  SEC_5ADDRNOSUPP = 8,

  SEC_REJECT = 91,
  SEC_NOIDENTD = 92,
  SEC_IDMISMATCH = 93,

  SEC_UNKNOWN = 200,
  SEC_AUTHFAILED,
  SEC_USERCANCEL,
  SEC_ADDRTYPE,
  SEC_REPLYVERSION,
  SEC_VERSION,
  SEC_NOMETHODS
} SocksErrorCode;

static ErrTable SocksErrStr[] = {
  /* SOCKS version 5 error messages */
  {SEC_5FAILURE, N_("SOCKS server general failure")},
  {SEC_5RULESET, N_("Connection denied by SOCKS ruleset")},
  {SEC_5NETDOWN, N_("SOCKS: Network unreachable")},
  {SEC_5UNREACH, N_("SOCKS: Host unreachable")},
  {SEC_5CONNREF, N_("SOCKS: Connection refused")},
  {SEC_5TTLEXPIRED, N_("SOCKS: TTL expired")},
  {SEC_5COMMNOSUPP, N_("SOCKS: Command not supported")},
  {SEC_5ADDRNOSUPP, N_("SOCKS: Address type not supported")},
  {SEC_NOMETHODS, N_("SOCKS server rejected all offered methods")},
  {SEC_ADDRTYPE, N_("Unknown SOCKS address type returned")},
  {SEC_AUTHFAILED, N_("SOCKS authentication failed")},
  {SEC_USERCANCEL, N_("SOCKS authentication cancelled by user")},

  /* SOCKS version 4 error messages */
  {SEC_REJECT, N_("SOCKS: Request rejected or failed")},
  {SEC_NOIDENTD, N_("SOCKS: Rejected - unable to contact identd")},
  {SEC_IDMISMATCH,
   N_("SOCKS: Rejected - identd reports different user-id")},

  /* SOCKS errors due to protocol violations */
  {SEC_UNKNOWN, N_("Unknown SOCKS reply code")},
  {SEC_REPLYVERSION, N_("Unknown SOCKS reply version code")},
  {SEC_VERSION, N_("Unknown SOCKS server version")},
  {0, NULL}
};

static void SocksAppendError(GString *str, LastError *error)
{
  LookupErrorCode(str, error->code, SocksErrStr, _("SOCKS error code %d"));
}

static ErrorType ETSocks = { SocksAppendError, NULL };

typedef enum {
  HEC_TRIESEX = 1,
  HEC_BADAUTH,
  HEC_BADREDIR,
  HEC_BADSTATUS,
  HEC_OK = 200,
  HEC_REDIRECT = 300,
  HEC_MOVEPERM = 301,
  HEC_MOVETEMP = 302,
  HEC_CLIENTERR = 400,
  HEC_AUTHREQ = 401,
  HEC_PROXYAUTH = 407,
  HEC_FORBIDDEN = 403,
  HEC_NOTFOUND = 404,
  HEC_SERVERERR = 500
} HTTPErrorCode;

static void HTTPAppendError(GString *str, LastError *error)
{
  switch (error->code) {
  case HEC_TRIESEX:
    /* Various HTTP error messages */
    g_string_append(str, _("Number of tries exceeded"));
    break;
  case HEC_BADAUTH:
    g_string_sprintfa(str, _("Bad auth header: %s"), (gchar *)error->data);
    break;
  case HEC_BADREDIR:
    g_string_sprintfa(str, _("Bad redirect: %s"), (gchar *)error->data);
    break;
  case HEC_BADSTATUS:
    g_string_sprintfa(str, _("Invalid HTTP status line: %s"),
                      (gchar *)error->data);
    break;
  case HEC_FORBIDDEN:
    g_string_append(str, _("403: forbidden"));
    break;
  case HEC_NOTFOUND:
    g_string_append(str, _("404: page not found"));
    break;
  case HEC_AUTHREQ:
    g_string_append(str, _("401: HTTP authentication failed"));
    break;
  case HEC_PROXYAUTH:
    g_string_append(str, _("407: HTTP proxy authentication failed"));
    break;
  case HEC_MOVETEMP:
  case HEC_MOVEPERM:
    g_string_append(str, _("Bad redirect message from server"));
    break;
  default:
    if (error->code < HEC_REDIRECT || error->code >= 600) {
      g_string_sprintfa(str, _("Unknown HTTP error %d"), error->code);
    } else if (error->code < HEC_CLIENTERR) {
      g_string_sprintfa(str, _("%d: redirect error"), error->code);
    } else if (error->code < HEC_SERVERERR) {
      g_string_sprintfa(str, _("%d: HTTP client error"), error->code);
    } else {
      g_string_sprintfa(str, _("%d: HTTP server error"), error->code);
    }
    break;
  }
}

static ErrorType ETHTTP = { HTTPAppendError, NULL };

static gboolean Socks5UserPasswd(NetworkBuffer *NetBuf)
{
  if (!NetBuf->userpasswd) {
    SetError(&NetBuf->error, &ETSocks, SEC_NOMETHODS, NULL);
    return FALSE;
  } else {
    /* Request a username and password (the callback function should in
     * turn call SendSocks5UserPasswd when it's done) */
    NetBuf->sockstat = NBSS_USERPASSWD;
    (*NetBuf->userpasswd) (NetBuf, NetBuf->userpasswddata);
    return TRUE;
  }
}

void SendSocks5UserPasswd(NetworkBuffer *NetBuf, gchar *user,
                          gchar *password)
{
  gchar *addpt;
  guint addlen;
  ConnBuf *conn;

  if (!user || !password || !user[0] || !password[0]) {
    SetError(&NetBuf->error, &ETSocks, SEC_USERCANCEL, NULL);
    NetBufCallBack(NetBuf, TRUE);
    return;
  }
  conn = &NetBuf->negbuf;
  addlen = 3 + strlen(user) + strlen(password);
  addpt = ExpandWriteBuffer(conn, addlen, &NetBuf->error);
  if (!addpt || strlen(user) > 255 || strlen(password) > 255) {
    SetError(&NetBuf->error, ET_CUSTOM, E_FULLBUF, NULL);
    NetBufCallBack(NetBuf, TRUE);
    return;
  }
  addpt[0] = 1;                 /* Subnegotiation version code */
  addpt[1] = strlen(user);
  strcpy(&addpt[2], user);
  addpt[2 + strlen(user)] = strlen(password);
  strcpy(&addpt[3 + strlen(user)], password);

  CommitWriteBuffer(NetBuf, conn, addpt, addlen);
}

static gboolean Socks5Connect(NetworkBuffer *NetBuf)
{
  guchar *addpt;
  guint addlen, hostlen;
  ConnBuf *conn;
  unsigned short int netport;

  conn = &NetBuf->negbuf;
  g_assert(NetBuf->host);
  hostlen = strlen(NetBuf->host);
  if (hostlen > 255)
    return FALSE;

  netport = htons(NetBuf->port);
  g_assert(sizeof(netport) == 2);

  addlen = hostlen + 7;
  addpt = ExpandWriteBuffer(conn, addlen, &NetBuf->error);
  if (!addpt)
    return FALSE;
  addpt[0] = 5;                 /* SOCKS version 5 */
  addpt[1] = 1;                 /* CONNECT */
  addpt[2] = 0;                 /* reserved - must be zero */
  addpt[3] = 3;                 /* Address type - FQDN */
  addpt[4] = hostlen;           /* Length of address */
  strcpy(&addpt[5], NetBuf->host);
  memcpy(&addpt[5 + hostlen], &netport, sizeof(netport));

  NetBuf->sockstat = NBSS_CONNECT;

  CommitWriteBuffer(NetBuf, conn, addpt, addlen);

  return TRUE;
}

static gboolean HandleSocksReply(NetworkBuffer *NetBuf)
{
  guchar *data;
  guchar addrtype;
  guint replylen;
  gboolean retval = TRUE;

  if (NetBuf->socks->version == 5) {
    if (NetBuf->sockstat == NBSS_METHODS) {
      data = GetWaitingData(NetBuf, 2);
      if (data) {
        retval = FALSE;
        if (data[0] != 5) {
          SetError(&NetBuf->error, &ETSocks, SEC_VERSION, NULL);
        } else if (data[1] == SM_NOAUTH) {
          retval = Socks5Connect(NetBuf);
        } else if (data[1] == SM_USERPASSWD) {
          retval = Socks5UserPasswd(NetBuf);
        } else {
          SetError(&NetBuf->error, &ETSocks, SEC_NOMETHODS, NULL);
        }
        g_free(data);
      }
    } else if (NetBuf->sockstat == NBSS_USERPASSWD) {
      data = GetWaitingData(NetBuf, 2);
      if (data) {
        retval = FALSE;
        if (data[1] != 0) {
          SetError(&NetBuf->error, &ETSocks, SEC_AUTHFAILED, NULL);
        } else {
          retval = Socks5Connect(NetBuf);
        }
        g_free(data);
      }
    } else if (NetBuf->sockstat == NBSS_CONNECT) {
      data = PeekWaitingData(NetBuf, 5);
      if (data) {
        retval = FALSE;
        addrtype = data[3];
        if (data[0] != 5) {
          SetError(&NetBuf->error, &ETSocks, SEC_VERSION, NULL);
        } else if (data[1] > 8) {
          SetError(&NetBuf->error, &ETSocks, SEC_UNKNOWN, NULL);
        } else if (data[1] != 0) {
          SetError(&NetBuf->error, &ETSocks, data[1], NULL);
        } else if (addrtype != 1 && addrtype != 3 && addrtype != 4) {
          SetError(&NetBuf->error, &ETSocks, SEC_ADDRTYPE, NULL);
        } else {
          retval = TRUE;
          replylen = 6;
          if (addrtype == 1)
            replylen += 4;      /* IPv4 address */
          else if (addrtype == 4)
            replylen += 16;     /* IPv6 address */
          else
            replylen += data[4];        /* FQDN */
          data = GetWaitingData(NetBuf, replylen);
          if (data) {
            NetBuf->status = NBS_CONNECTED;
            g_free(data);
            NetBufCallBack(NetBuf, FALSE);      /* status has changed */
          }
        }
      }
    }
    return retval;
  } else {
    data = GetWaitingData(NetBuf, 8);
    if (data) {
      retval = FALSE;
      if (data[0] != 0) {
        SetError(&NetBuf->error, &ETSocks, SEC_REPLYVERSION, NULL);
      } else {
        if (data[1] == 90) {
          NetBuf->status = NBS_CONNECTED;
          NetBufCallBack(NetBuf, FALSE);        /* status has changed */
          retval = TRUE;
        } else if (data[1] >= SEC_REJECT && data[1] <= SEC_IDMISMATCH) {
          SetError(&NetBuf->error, &ETSocks, data[1], NULL);
        } else {
          SetError(&NetBuf->error, &ETSocks, SEC_UNKNOWN, NULL);
        }
      }
      g_free(data);
    }
    return retval;
  }
}

/* 
 * Reads and writes data if the network connection is ready. Sets the
 * various OK variables to TRUE if no errors occurred in the relevant
 * operations, and returns TRUE if data was read and is waiting for
 * processing.
 */
static gboolean DoNetworkBufferStuff(NetworkBuffer *NetBuf,
                                     gboolean ReadReady,
                                     gboolean WriteReady,
                                     gboolean ErrorReady, gboolean *ReadOK,
                                     gboolean *WriteOK, gboolean *ErrorOK)
{
  gboolean DataWaiting = FALSE, ConnectDone = FALSE;
  gboolean retval;

  *ReadOK = *WriteOK = *ErrorOK = TRUE;

  if (ErrorReady || NetBuf->error)
    *ErrorOK = FALSE;
  else if (NetBuf->WaitConnect) {
    if (WriteReady) {
      retval = FinishConnect(NetBuf->fd, &NetBuf->error);
      ConnectDone = TRUE;
      NetBuf->WaitConnect = FALSE;

      if (retval) {
        if (NetBuf->socks) {
          NetBuf->status = NBS_SOCKSCONNECT;
          NetBuf->sockstat = NBSS_METHODS;
        } else {
          NetBuf->status = NBS_CONNECTED;
        }
      } else {
        NetBuf->status = NBS_PRECONNECT;
        *WriteOK = FALSE;
      }
    }
  } else {
    if (WriteReady)
      *WriteOK = WriteDataToWire(NetBuf);

    if (ReadReady) {
      *ReadOK = ReadDataFromWire(NetBuf);
      if (NetBuf->ReadBuf.DataPresent > 0 &&
          NetBuf->status == NBS_SOCKSCONNECT) {
        if (!HandleSocksReply(NetBuf)
            || NetBuf->error) { /* From SendSocks5UserPasswd, possibly */
          *ErrorOK = FALSE;
        }
      }
      if (NetBuf->ReadBuf.DataPresent > 0 &&
          NetBuf->status != NBS_SOCKSCONNECT) {
        DataWaiting = TRUE;
      }
    }
  }

  if (!(*ErrorOK && *WriteOK && *ReadOK)) {
    /* We don't want to check the socket any more */
    NetBufCallBackStop(NetBuf);
    /* If there were errors, then the socket is now useless - so close it */
    CloseSocket(NetBuf->fd);
    NetBuf->fd = -1;
  } else if (ConnectDone) {
    /* If we just connected, then no need to listen for write-ready status
     * any more */
    NetBufCallBack(NetBuf, FALSE);
  } else if (WriteReady
             && ((NetBuf->status == NBS_CONNECTED
                  && NetBuf->WriteBuf.DataPresent == 0)
                 || (NetBuf->status == NBS_SOCKSCONNECT
                     && NetBuf->negbuf.DataPresent == 0))) {
    /* If we wrote out everything, then tell the owner so that the socket
     * no longer needs to be checked for write-ready status */
    NetBufCallBack(NetBuf, FALSE);
  }

  return DataWaiting;
}

/* 
 * Responds to a select() call by reading/writing data as necessary.
 * If any data were read, TRUE is returned. "DoneOK" is set TRUE
 * unless a fatal error (i.e. the connection was broken) occurred.
 */
gboolean RespondToSelect(NetworkBuffer *NetBuf, fd_set *readfds,
                         fd_set *writefds, fd_set *errorfds,
                         gboolean *DoneOK)
{
  gboolean ReadOK, WriteOK, ErrorOK;
  gboolean DataWaiting = FALSE;

  *DoneOK = TRUE;
  if (!NetBuf || NetBuf->fd <= 0)
    return DataWaiting;
  DataWaiting = DoNetworkBufferStuff(NetBuf, FD_ISSET(NetBuf->fd, readfds),
                                     FD_ISSET(NetBuf->fd, writefds),
                                     errorfds ? FD_ISSET(NetBuf->fd,
                                                         errorfds) : FALSE,
                                     &ReadOK, &WriteOK, &ErrorOK);
  *DoneOK = (WriteOK && ErrorOK && ReadOK);
  return DataWaiting;
}

gboolean NetBufHandleNetwork(NetworkBuffer *NetBuf, gboolean ReadReady,
                             gboolean WriteReady, gboolean *DoneOK)
{
  gboolean ReadOK, WriteOK, ErrorOK;
  gboolean DataWaiting = FALSE;

  *DoneOK = TRUE;
  if (!NetBuf || NetBuf->fd <= 0)
    return DataWaiting;

  DataWaiting = DoNetworkBufferStuff(NetBuf, ReadReady, WriteReady, FALSE,
                                     &ReadOK, &WriteOK, &ErrorOK);

  *DoneOK = (WriteOK && ErrorOK && ReadOK);
  return DataWaiting;
}

/* 
 * Returns the number of complete (terminated) messages waiting in the
 * given network buffer. This is the number of times that
 * GetWaitingMessage() can be safely called without it returning NULL.
 */
gint CountWaitingMessages(NetworkBuffer *NetBuf)
{
  ConnBuf *conn;
  gint i, msgs = 0;

  if (NetBuf->status != NBS_CONNECTED)
    return 0;

  conn = &NetBuf->ReadBuf;

  if (conn->Data)
    for (i = 0; i < conn->DataPresent; i++) {
      if (conn->Data[i] == NetBuf->Terminator)
        msgs++;
    }
  return msgs;
}

gchar *PeekWaitingData(NetworkBuffer *NetBuf, int numbytes)
{
  ConnBuf *conn;

  conn = &NetBuf->ReadBuf;
  if (!conn->Data || conn->DataPresent < numbytes)
    return NULL;
  else
    return conn->Data;
}

gchar *GetWaitingData(NetworkBuffer *NetBuf, int numbytes)
{
  ConnBuf *conn;
  gchar *data;

  conn = &NetBuf->ReadBuf;
  if (!conn->Data || conn->DataPresent < numbytes)
    return NULL;

  data = g_new(gchar, numbytes);
  memcpy(data, conn->Data, numbytes);

  memmove(&conn->Data[0], &conn->Data[numbytes],
          conn->DataPresent - numbytes);
  conn->DataPresent -= numbytes;

  return data;
}

/* 
 * Reads a complete (terminated) message from the network buffer. The
 * message is removed from the buffer, and returned as a null-terminated
 * string (the network terminator is removed). If no complete message is
 * waiting, NULL is returned. The string is dynamically allocated, and
 * so must be g_free'd by the caller.
 */
gchar *GetWaitingMessage(NetworkBuffer *NetBuf)
{
  ConnBuf *conn;
  int MessageLen;
  char *SepPt;
  gchar *NewMessage;

  conn = &NetBuf->ReadBuf;
  if (!conn->Data || !conn->DataPresent || NetBuf->status != NBS_CONNECTED) {
    return NULL;
  }
  SepPt = memchr(conn->Data, NetBuf->Terminator, conn->DataPresent);
  if (!SepPt)
    return NULL;
  *SepPt = '\0';
  MessageLen = SepPt - conn->Data + 1;
  SepPt--;
  if (NetBuf->StripChar && *SepPt == NetBuf->StripChar)
    *SepPt = '\0';
  NewMessage = g_new(gchar, MessageLen);

  memcpy(NewMessage, conn->Data, MessageLen);
  if (MessageLen < conn->DataPresent) {
    memmove(&conn->Data[0], &conn->Data[MessageLen],
            conn->DataPresent - MessageLen);
  }
  conn->DataPresent -= MessageLen;
  return NewMessage;
}

/* 
 * Reads any waiting data on the given network buffer's TCP/IP connection
 * into the read buffer. Returns FALSE if the connection was closed, or
 * if the read buffer's maximum size was reached.
 */
gboolean ReadDataFromWire(NetworkBuffer *NetBuf)
{
  ConnBuf *conn;
  int CurrentPosition, BytesRead;

  conn = &NetBuf->ReadBuf;
  CurrentPosition = conn->DataPresent;
  while (1) {
    if (CurrentPosition >= conn->Length) {
      if (conn->Length == MAXREADBUF) {
        SetError(&NetBuf->error, ET_CUSTOM, E_FULLBUF, NULL);
        return FALSE;           /* drop connection */
      }
      if (conn->Length == 0)
        conn->Length = 256;
      else
        conn->Length *= 2;
      if (conn->Length > MAXREADBUF)
        conn->Length = MAXREADBUF;
      conn->Data = g_realloc(conn->Data, conn->Length);
    }
    BytesRead = recv(NetBuf->fd, &conn->Data[CurrentPosition],
                     conn->Length - CurrentPosition, 0);
    if (BytesRead == SOCKET_ERROR) {
#ifdef CYGWIN
      int Error = WSAGetLastError();

      if (Error == WSAEWOULDBLOCK)
        break;
      else {
        SetError(&NetBuf->error, ET_WINSOCK, Error, NULL);
        return FALSE;
      }
#else
      if (errno == EAGAIN)
        break;
      else if (errno != EINTR) {
        SetError(&NetBuf->error, ET_ERRNO, errno, NULL);
        return FALSE;
      }
#endif
    } else if (BytesRead == 0) {
      return FALSE;
    } else {
      CurrentPosition += BytesRead;
      conn->DataPresent = CurrentPosition;
    }
  }
  return TRUE;
}

gchar *ExpandWriteBuffer(ConnBuf *conn, int numbytes, LastError **error)
{
  int newlen;

  newlen = conn->DataPresent + numbytes;
  if (newlen > conn->Length) {
    conn->Length *= 2;
    conn->Length = MAX(conn->Length, newlen);
    if (conn->Length > MAXWRITEBUF)
      conn->Length = MAXWRITEBUF;
    if (newlen > conn->Length) {
      if (error)
        SetError(error, ET_CUSTOM, E_FULLBUF, NULL);
      return NULL;
    }
    conn->Data = g_realloc(conn->Data, conn->Length);
  }

  return (&conn->Data[conn->DataPresent]);
}

void CommitWriteBuffer(NetworkBuffer *NetBuf, ConnBuf *conn,
                       gchar *addpt, guint addlen)
{
  conn->DataPresent += addlen;

  /* If the buffer was empty before, we may need to tell the owner to
   * check the socket for write-ready status */
  if (NetBuf && addpt == conn->Data)
    NetBufCallBack(NetBuf, FALSE);
}

/* 
 * Writes the null-terminated string "data" to the network buffer, ready
 * to be sent to the wire when the network connection becomes free. The
 * message is automatically terminated. Fails to write the message without
 * error if the buffer reaches its maximum size (although this error will
 * be detected when an attempt is made to write the buffer to the wire).
 */
void QueueMessageForSend(NetworkBuffer *NetBuf, gchar *data)
{
  gchar *addpt;
  guint addlen;
  ConnBuf *conn;

  conn = &NetBuf->WriteBuf;

  if (!data)
    return;
  addlen = strlen(data) + 1;
  addpt = ExpandWriteBuffer(conn, addlen, NULL);
  if (!addpt)
    return;

  memcpy(addpt, data, addlen);
  addpt[addlen - 1] = NetBuf->Terminator;

  CommitWriteBuffer(NetBuf, conn, addpt, addlen);
}

static struct hostent *LookupHostname(gchar *host, LastError **error)
{
  struct hostent *he;

  if ((he = gethostbyname(host)) == NULL) {
#ifdef CYGWIN
    if (error)
      SetError(error, ET_WINSOCK, WSAGetLastError(), NULL);
#else
    if (error)
      SetError(error, ET_HERRNO, h_errno, NULL);
#endif
  }
  return he;
}

gboolean StartSocksNegotiation(NetworkBuffer *NetBuf, gchar *RemoteHost,
                               unsigned RemotePort)
{
  guint num_methods;
  ConnBuf *conn;
  struct hostent *he;
  guchar *addpt;
  guint addlen, i;
  struct in_addr *haddr;
  unsigned short int netport;
  gchar *username = NULL;

#ifdef CYGWIN
  DWORD bufsize;
#else
  struct passwd *pwd;
#endif

  conn = &NetBuf->negbuf;

  if (NetBuf->socks->version == 5) {
    num_methods = 1;
    if (NetBuf->userpasswd)
      num_methods++;
    addlen = 2 + num_methods;
    addpt = ExpandWriteBuffer(conn, addlen, &NetBuf->error);
    if (!addpt)
      return FALSE;
    addpt[0] = 5;               /* SOCKS version 5 */
    addpt[1] = num_methods;
    i = 2;
    addpt[i++] = SM_NOAUTH;
    if (NetBuf->userpasswd)
      addpt[i++] = SM_USERPASSWD;

    g_free(NetBuf->host);
    NetBuf->host = g_strdup(RemoteHost);
    NetBuf->port = RemotePort;

    CommitWriteBuffer(NetBuf, conn, addpt, addlen);

    return TRUE;
  }

  he = LookupHostname(RemoteHost, &NetBuf->error);
  if (!he)
    return FALSE;

  if (NetBuf->socks->user && NetBuf->socks->user[0]) {
    username = g_strdup(NetBuf->socks->user);
  } else {
#ifdef CYGWIN
    bufsize = 0;
    WNetGetUser(NULL, username, &bufsize);
    if (GetLastError() != ERROR_MORE_DATA) {
      SetError(&NetBuf->error, ET_WIN32, GetLastError(), NULL);
      return FALSE;
    } else {
      username = g_malloc(bufsize);
      if (WNetGetUser(NULL, username, &bufsize) != NO_ERROR) {
        SetError(&NetBuf->error, ET_WIN32, GetLastError(), NULL);
        return FALSE;
      }
    }
#else
    if (NetBuf->socks->numuid) {
      username = g_strdup_printf("%d", getuid());
    } else {
      pwd = getpwuid(getuid());
      if (!pwd || !pwd->pw_name)
        return FALSE;
      username = g_strdup(pwd->pw_name);
    }
#endif
  }
  addlen = 9 + strlen(username);

  haddr = (struct in_addr *)he->h_addr;
  g_assert(sizeof(struct in_addr) == 4);

  netport = htons(RemotePort);
  g_assert(sizeof(netport) == 2);

  addpt = ExpandWriteBuffer(conn, addlen, &NetBuf->error);
  if (!addpt)
    return FALSE;

  addpt[0] = 4;                 /* SOCKS version */
  addpt[1] = 1;                 /* CONNECT */
  memcpy(&addpt[2], &netport, sizeof(netport));
  memcpy(&addpt[4], haddr, sizeof(struct in_addr));
  strcpy(&addpt[8], username);
  g_free(username);
  addpt[addlen - 1] = '\0';

  CommitWriteBuffer(NetBuf, conn, addpt, addlen);

  return TRUE;
}

static gboolean WriteBufToWire(NetworkBuffer *NetBuf, ConnBuf *conn)
{
  int CurrentPosition, BytesSent;

  if (!conn->Data || !conn->DataPresent)
    return TRUE;
  if (conn->Length == MAXWRITEBUF) {
    SetError(&NetBuf->error, ET_CUSTOM, E_FULLBUF, NULL);
    return FALSE;
  }
  CurrentPosition = 0;
  while (CurrentPosition < conn->DataPresent) {
    BytesSent = send(NetBuf->fd, &conn->Data[CurrentPosition],
                     conn->DataPresent - CurrentPosition, 0);
    if (BytesSent == SOCKET_ERROR) {
#ifdef CYGWIN
      int Error = WSAGetLastError();

      if (Error == WSAEWOULDBLOCK)
        break;
      else {
        SetError(&NetBuf->error, ET_WINSOCK, Error, NULL);
        return FALSE;
      }
#else
      if (errno == EAGAIN)
        break;
      else if (errno != EINTR) {
        SetError(&NetBuf->error, ET_ERRNO, errno, NULL);
        return FALSE;
      }
#endif
    } else {
      CurrentPosition += BytesSent;
    }
  }
  if (CurrentPosition > 0 && CurrentPosition < conn->DataPresent) {
    memmove(&conn->Data[0], &conn->Data[CurrentPosition],
            conn->DataPresent - CurrentPosition);
  }
  conn->DataPresent -= CurrentPosition;
  return TRUE;
}

/* 
 * Writes any waiting data in the network buffer to the wire. Returns
 * TRUE on success, or FALSE if the buffer's maximum length is
 * reached, or the remote end has closed the connection.
 */
gboolean WriteDataToWire(NetworkBuffer *NetBuf)
{
  if (NetBuf->status == NBS_SOCKSCONNECT) {
    return WriteBufToWire(NetBuf, &NetBuf->negbuf);
  } else {
    return WriteBufToWire(NetBuf, &NetBuf->WriteBuf);
  }
}

static void SendHttpRequest(HttpConnection *conn)
{
  GString *text;
  char *userpasswd;

  conn->Tries++;
  conn->StatusCode = 0;
  conn->Status = HS_CONNECTING;

  text = g_string_new("");

  g_string_sprintf(text, "%s http://%s:%u%s HTTP/1.0",
                   conn->Method, conn->HostName, conn->Port, conn->Query);
  QueueMessageForSend(&conn->NetBuf, text->str);

  if (conn->Headers)
    QueueMessageForSend(&conn->NetBuf, conn->Headers);

  if (conn->user && conn->password) {
    userpasswd = g_strdup_printf("%s:%s", conn->user, conn->password);
    g_string_assign(text, "Authorization: Basic ");
    AddB64Enc(text, userpasswd);
    g_free(userpasswd);
    QueueMessageForSend(&conn->NetBuf, text->str);
  }
  if (conn->proxyuser && conn->proxypassword) {
    userpasswd =
        g_strdup_printf("%s:%s", conn->proxyuser, conn->proxypassword);
    g_string_assign(text, "Proxy-Authenticate: Basic ");
    AddB64Enc(text, userpasswd);
    g_free(userpasswd);
    QueueMessageForSend(&conn->NetBuf, text->str);
  }

  g_string_sprintf(text, "User-Agent: dopewars/%s", VERSION);
  QueueMessageForSend(&conn->NetBuf, text->str);

  /* Insert a blank line between headers and body */
  QueueMessageForSend(&conn->NetBuf, "");

  if (conn->Body)
    QueueMessageForSend(&conn->NetBuf, conn->Body);

  g_string_free(text, TRUE);
}

static gboolean StartHttpConnect(HttpConnection *conn)
{
  gchar *ConnectHost;
  unsigned ConnectPort;

  if (conn->Proxy) {
    ConnectHost = conn->Proxy;
    ConnectPort = conn->ProxyPort;
  } else {
    ConnectHost = conn->HostName;
    ConnectPort = conn->Port;
  }

  if (!StartNetworkBufferConnect(&conn->NetBuf, ConnectHost, ConnectPort)) {
    return FALSE;
  }
  return TRUE;
}

gboolean OpenHttpConnection(HttpConnection **connpt, gchar *HostName,
                            unsigned Port, gchar *Proxy,
                            unsigned ProxyPort, SocksServer *socks,
                            gchar *Method, gchar *Query, gchar *Headers,
                            gchar *Body)
{
  HttpConnection *conn;

  g_assert(HostName && Method && Query && connpt);

  conn = g_new0(HttpConnection, 1);

  InitNetworkBuffer(&conn->NetBuf, '\n', '\r', socks);
  conn->HostName = g_strdup(HostName);
  if (Proxy && Proxy[0])
    conn->Proxy = g_strdup(Proxy);
  conn->Method = g_strdup(Method);
  conn->Query = g_strdup(Query);
  if (Headers && Headers[0])
    conn->Headers = g_strdup(Headers);
  if (Body && Body[0])
    conn->Body = g_strdup(Body);
  conn->Port = Port;
  conn->ProxyPort = ProxyPort;
  *connpt = conn;

  if (StartHttpConnect(conn)) {
    SendHttpRequest(conn);
    return TRUE;
  } else {
    return FALSE;
  }
}

void CloseHttpConnection(HttpConnection *conn)
{
  ShutdownNetworkBuffer(&conn->NetBuf);
  g_free(conn->HostName);
  g_free(conn->Proxy);
  g_free(conn->Method);
  g_free(conn->Query);
  g_free(conn->Headers);
  g_free(conn->Body);
  g_free(conn->RedirHost);
  g_free(conn->RedirQuery);
  g_free(conn->user);
  g_free(conn->password);
  g_free(conn->proxyuser);
  g_free(conn->proxypassword);
  g_free(conn);
}

void SetHttpAuthentication(HttpConnection *conn, gboolean proxy,
                           gchar *user, gchar *password)
{
  gchar **ptuser, **ptpassword;

  g_assert(conn);
  if (proxy) {
    ptuser = &conn->proxyuser;
    ptpassword = &conn->proxypassword;
  } else {
    ptuser = &conn->user;
    ptpassword = &conn->password;
  }
  g_free(*ptuser);
  g_free(*ptpassword);
  if (user && password && user[0] && password[0]) {
    *ptuser = g_strdup(user);
    *ptpassword = g_strdup(password);
  } else {
    *ptuser = *ptpassword = NULL;
  }
  conn->waitinput = FALSE;
  if (conn->Status == HS_WAITCOMPLETE) {
    NetBufCallBack(&conn->NetBuf, TRUE);
  }
}

void SetHttpAuthFunc(HttpConnection *conn, HCAuthFunc authfunc,
                     gpointer data)
{
  g_assert(conn && authfunc);
  conn->authfunc = authfunc;
  conn->authdata = data;
}

static gboolean ParseHtmlLocation(gchar *uri, gchar **host, unsigned *port,
                                  gchar **query)
{
  gchar *uris, *colon, *slash;

  uris = g_strstrip(uri);
  if (!uris || strlen(uris) < 7 || g_strncasecmp(uris, "http://", 7) != 0)
    return FALSE;

  uris += 7;                    /* skip to hostname */

  /* ':' denotes the port to connect to */
  colon = strchr(uris, ':');
  if (colon && colon == uris)
    return FALSE;               /* No hostname */

  /* '/' denotes the start of the path of the HTML file */
  slash = strchr(uris, '/');
  if (slash && slash == uris)
    return FALSE;               /* No hostname */

  if (colon && (!slash || slash > colon)) {
    if (slash)
      *slash = '\0';
    *port = atoi(colon + 1);
    if (slash)
      *slash = '\\';
    if (*port == 0)
      return FALSE;             /* Invalid port */
    *host = g_strndup(uris, colon - uris);
  } else {
    *port = 80;
    if (slash)
      *host = g_strndup(uris, slash - uris);
    else
      *host = g_strdup(uris);
  }

  if (slash) {
    *query = g_strdup(slash);
  } else {
    *query = g_strdup("/");
  }
  return TRUE;
}

static void StartHttpAuth(HttpConnection *conn, gboolean proxy,
                          gchar *header, gboolean *doneOK)
{
  gchar *realm, **split;

  if (!conn->authfunc)
    return;

  split = my_strsplit(header, " ", 2);

  if (split[0] && split[1] && g_strcasecmp(split[0], "Basic") == 0 &&
      g_strncasecmp(split[1], "realm=", 6) == 0 && strlen(split[1]) > 6) {
    realm = &split[1][6];
    conn->waitinput = TRUE;
    (*conn->authfunc) (conn, proxy, realm, conn->authdata);
  } else {
    *doneOK = FALSE;
    SetError(&conn->NetBuf.error, &ETHTTP, HEC_BADAUTH, g_strdup(header));
  }

  g_strfreev(split);
}

static void ParseHtmlHeader(gchar *line, HttpConnection *conn,
                            gboolean *doneOK)
{
  gchar **split, *host, *query;
  unsigned port;

  split = my_strsplit(line, " ", 2);
  if (split[0] && split[1]) {
    if (g_strcasecmp(split[0], "Location:") == 0 &&
        (conn->StatusCode == HEC_MOVETEMP
         || conn->StatusCode == HEC_MOVEPERM)) {
      if (ParseHtmlLocation(split[1], &host, &port, &query)) {
        g_free(conn->RedirHost);
        g_free(conn->RedirQuery);
        conn->RedirHost = host;
        conn->RedirQuery = query;
        conn->RedirPort = port;
      } else {
        *doneOK = FALSE;
        SetError(&conn->NetBuf.error, &ETHTTP, HEC_BADREDIR,
                 g_strdup(line));
      }
    } else if (g_strcasecmp(split[0], "WWW-Authenticate:") == 0 &&
               conn->StatusCode == HEC_AUTHREQ) {
      StartHttpAuth(conn, FALSE, split[1], doneOK);
    } else if (g_strcasecmp(split[0], "Proxy-Authenticate:") == 0 &&
               conn->StatusCode == HEC_PROXYAUTH) {
      /* Proxy-Authenticate is, strictly speaking, an HTTP/1.1 thing, but
       * some HTTP/1.0 proxies seem to support it anyway */
      StartHttpAuth(conn, TRUE, split[1], doneOK);
    }
  }
  g_strfreev(split);
}

gchar *ReadHttpResponse(HttpConnection *conn, gboolean *doneOK)
{
  gchar *msg, **split;

  msg = GetWaitingMessage(&conn->NetBuf);
  if (msg)
    switch (conn->Status) {
    case HS_CONNECTING:        /* OK, we should have the HTTP status line */
      conn->Status = HS_READHEADERS;
      split = my_strsplit(msg, " ", 3);
      if (split[0] && split[1]) {
        conn->StatusCode = atoi(split[1]);
      } else {
        *doneOK = FALSE;
        SetError(&conn->NetBuf.error, &ETHTTP, HEC_BADSTATUS,
                 g_strdup(msg));
      }
      g_strfreev(split);
      break;
    case HS_READHEADERS:
      if (msg[0] == 0)
        conn->Status = HS_READSEPARATOR;
      else
        ParseHtmlHeader(msg, conn, doneOK);
      break;
    case HS_READSEPARATOR:
      conn->Status = HS_READBODY;
      break;
    case HS_READBODY:          /* At present, we do nothing special
                                * with the body */
      break;
    case HS_WAITCOMPLETE:      /* Well, we shouldn't be here at all... */
      g_free(msg);
      msg = NULL;
      break;
    }
  return msg;
}

gboolean HandleHttpCompletion(HttpConnection *conn)
{
  NBCallBack CallBack;
  gpointer CallBackData, userpasswddata;
  NBUserPasswd userpasswd;
  gboolean retry = FALSE;
  LastError **error;

  error = &conn->NetBuf.error;

  /* If we're still waiting for authentication etc., then signal that the
   * connection shouldn't be closed yet, and go into the "WAITCOMPLETE"
   * state */
  if (conn->waitinput) {
    conn->Status = HS_WAITCOMPLETE;
    return FALSE;
  }

  if (conn->Tries >= 5) {
    SetError(error, &ETHTTP, HEC_TRIESEX, NULL);
    return TRUE;
  }

  if (conn->RedirHost) {
    g_free(conn->HostName);
    g_free(conn->Query);
    conn->HostName = conn->RedirHost;
    conn->Query = conn->RedirQuery;
    conn->Port = conn->RedirPort;
    conn->RedirHost = conn->RedirQuery = NULL;
    retry = TRUE;
  }
  if (conn->StatusCode == HEC_AUTHREQ && conn->user && conn->password) {
    retry = TRUE;
  }
  if (conn->StatusCode == HEC_PROXYAUTH && conn->proxyuser &&
      conn->proxypassword) {
    retry = TRUE;
  }

  if (retry) {
    CallBack = conn->NetBuf.CallBack;
    userpasswd = conn->NetBuf.userpasswd;
    userpasswddata = conn->NetBuf.userpasswddata;
    CallBackData = conn->NetBuf.CallBackData;
    ShutdownNetworkBuffer(&conn->NetBuf);
    if (StartHttpConnect(conn)) {
      SendHttpRequest(conn);
      SetNetworkBufferCallBack(&conn->NetBuf, CallBack, CallBackData);
      SetNetworkBufferUserPasswdFunc(&conn->NetBuf,
                                     userpasswd, userpasswddata);
      return FALSE;
    }
  } else if (conn->StatusCode >= 300) {
    SetError(error, &ETHTTP, conn->StatusCode, NULL);
  }
  return TRUE;
}

gboolean IsHttpError(HttpConnection *conn)
{
  return (conn->NetBuf.error != NULL);
}

int CreateTCPSocket(LastError **error)
{
  int fd;

  fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd == SOCKET_ERROR && error) {
#ifdef CYGWIN
    SetError(error, ET_WINSOCK, WSAGetLastError(), NULL);
#else
    SetError(error, ET_ERRNO, errno, NULL);
#endif
  }

  return fd;
}

gboolean BindTCPSocket(int sock, unsigned port, LastError **error)
{
  struct sockaddr_in bindaddr;
  int retval;

  bindaddr.sin_family = AF_INET;
  bindaddr.sin_port = htons(port);
  bindaddr.sin_addr.s_addr = INADDR_ANY;
  memset(bindaddr.sin_zero, 0, sizeof(bindaddr.sin_zero));

  retval =
      bind(sock, (struct sockaddr *)&bindaddr, sizeof(struct sockaddr));

  if (retval == SOCKET_ERROR && error) {
#ifdef CYGWIN
    SetError(error, ET_WINSOCK, WSAGetLastError(), NULL);
#else
    SetError(error, ET_ERRNO, errno, NULL);
#endif
  }

  return (retval != SOCKET_ERROR);
}

gboolean StartConnect(int *fd, gchar *RemoteHost, unsigned RemotePort,
                      gboolean *doneOK, LastError **error)
{
  struct sockaddr_in ClientAddr;
  struct hostent *he;

  if (doneOK)
    *doneOK = FALSE;
  he = LookupHostname(RemoteHost, error);
  if (!he)
    return FALSE;

  *fd = CreateTCPSocket(error);
  if (*fd == SOCKET_ERROR)
    return FALSE;

  ClientAddr.sin_family = AF_INET;
  ClientAddr.sin_port = htons(RemotePort);
  ClientAddr.sin_addr = *((struct in_addr *)he->h_addr);
  memset(ClientAddr.sin_zero, 0, sizeof(ClientAddr.sin_zero));

  SetBlocking(*fd, FALSE);

  if (connect(*fd, (struct sockaddr *)&ClientAddr,
              sizeof(struct sockaddr)) == SOCKET_ERROR) {
#ifdef CYGWIN
    int errcode = WSAGetLastError();

    if (errcode == WSAEWOULDBLOCK)
      return TRUE;
    else if (error)
      SetError(error, ET_WINSOCK, errcode, NULL);
#else
    if (errno == EINPROGRESS)
      return TRUE;
    else if (error)
      SetError(error, ET_ERRNO, errno, NULL);
#endif
    CloseSocket(*fd);
    *fd = -1;
    return FALSE;
  } else {
    if (doneOK)
      *doneOK = TRUE;
  }
  return TRUE;
}

gboolean FinishConnect(int fd, LastError **error)
{
  int errcode;

#ifdef CYGWIN
  errcode = WSAGetLastError();
  if (errcode == 0)
    return TRUE;
  else {
    if (error) {
      SetError(error, ET_WINSOCK, errcode, NULL);
    }
    return FALSE;
  }
#else
#ifdef HAVE_SOCKLEN_T
  socklen_t optlen;
#else
  int optlen;
#endif

  optlen = sizeof(errcode);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcode, &optlen) == -1) {
    errcode = errno;
  }
  if (errcode == 0)
    return TRUE;
  else {
    if (error) {
      SetError(error, ET_ERRNO, errcode, NULL);
    }
    return FALSE;
  }
#endif /* CYGWIN */
}

static void AddB64char(GString *str, int c)
{
  if (c < 0)
    return;
  else if (c < 26)
    g_string_append_c(str, c + 'A');
  else if (c < 52)
    g_string_append_c(str, c - 26 + 'a');
  else if (c < 62)
    g_string_append_c(str, c - 52 + '0');
  else if (c == 62)
    g_string_append_c(str, '+');
  else
    g_string_append_c(str, '/');
}

/* 
 * Adds the plain text string "unenc" to the end of the GString "str",
 * using the Base64 encoding scheme.
 */
void AddB64Enc(GString *str, gchar *unenc)
{
  guint i;
  long value = 0;

  if (!unenc || !str)
    return;
  for (i = 0; i < strlen(unenc); i++) {
    value <<= 8;
    value |= (unsigned char)unenc[i];
    if (i % 3 == 2) {
      AddB64char(str, (value >> 18) & 0x3F);
      AddB64char(str, (value >> 12) & 0x3F);
      AddB64char(str, (value >> 6) & 0x3F);
      AddB64char(str, value & 0x3F);
      value = 0;
    }
  }
  if (i % 3 == 1) {
    AddB64char(str, (value >> 2) & 0x3F);
    AddB64char(str, (value << 4) & 0x3F);
    g_string_append(str, "==");
  } else if (i % 3 == 2) {
    AddB64char(str, (value >> 10) & 0x3F);
    AddB64char(str, (value >> 4) & 0x3F);
    AddB64char(str, (value << 2) & 0x3F);
    g_string_append_c(str, '=');
  }
}

#endif /* NETWORKING */
