/************************************************************************
 * network.h      Header file for low-level networking routines         *
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

#ifndef __NETWORK_H__
#define __NETWORK_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Various includes necessary for select() calls */
#ifdef CYGWIN
#include <windows.h>
#else
#include <sys/types.h>
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

#include <glib.h>

#include "error.h"

#ifdef NETWORKING

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

typedef struct _ConnBuf {
  gchar *Data;                  /* bytes waiting to be read/written */
  gint Length;                  /* allocated length of the "Data" buffer */
  gint DataPresent;             /* number of bytes currently in "Data" */
} ConnBuf;

typedef struct _NetworkBuffer NetworkBuffer;

typedef void (*NBCallBack) (NetworkBuffer *NetBuf, gboolean Read,
                            gboolean Write, gboolean CallNow);

typedef void (*NBUserPasswd) (NetworkBuffer *NetBuf, gpointer data);

/* Information about a SOCKS server */
typedef struct _SocksServer {
  gchar *name;                  /* hostname */
  unsigned port;                /* port number */
  int version;                  /* desired protocol version (usually
                                 * 4 or 5) */
  gboolean numuid;              /* if TRUE, send numeric user IDs rather
                                 * than names */
  char *user;                   /* if not blank, override the username
                                 * with this */
  gchar *authuser;              /* if set, the username for SOCKS5 auth */
  gchar *authpassword;          /* if set, the password for SOCKS5 auth */
} SocksServer;

/* The status of a network buffer */
typedef enum {
  NBS_PRECONNECT,               /* Socket is not yet connected */
  NBS_SOCKSCONNECT,             /* A CONNECT request is being sent to a
                                 * SOCKS server */
  NBS_CONNECTED                 /* Socket is connected */
} NBStatus;

/* Status of a SOCKS v5 negotiation */
typedef enum {
  NBSS_METHODS,                 /* Negotiation of available methods */
  NBSS_USERPASSWD,              /* Username-password request is being sent */
  NBSS_CONNECT                  /* CONNECT request is being sent */
} NBSocksStatus;

/* Handles reading and writing messages from/to a network connection */
struct _NetworkBuffer {
  int fd;                       /* File descriptor of the socket */
  gint InputTag;                /* Identifier for gdk_input routines */
  NBCallBack CallBack;          /* Function called when the socket
                                 * status changes */
  gpointer CallBackData;        /* Data accessible to the callback
                                 * function */
  char Terminator;              /* Character that separates messages */
  char StripChar;               /* Char that should be removed
                                 * from messages */
  ConnBuf ReadBuf;              /* New data, waiting for the application */
  ConnBuf WriteBuf;             /* Data waiting to be written to the wire */
  ConnBuf negbuf;               /* Output for protocol negotiation
                                 * (e.g. SOCKS) */
  gboolean WaitConnect;         /* TRUE if a non-blocking connect is in
                                 * progress */
  NBStatus status;              /* Status of the connection (if any) */
  NBSocksStatus sockstat;       /* Status of SOCKS negotiation (if any) */
  SocksServer *socks;           /* If non-NULL, a SOCKS server to use */
  NBUserPasswd userpasswd;      /* Function to supply username and
                                 * password for SOCKS5 authentication */
  gpointer userpasswddata;      /* data to pass to the above function */
  gchar *host;                  /* If non-NULL, the host to connect to */
  unsigned port;                /* If non-NULL, the port to connect to */
  LastError *error;             /* Any error from the last operation */
};

/* Keeps track of the progress of an HTTP connection */
typedef enum {
  HS_CONNECTING,                /* Waiting for connect() to complete */
  HS_READHEADERS,               /* Reading HTTP headers */
  HS_READSEPARATOR,             /* Reading the header/body separator line */
  HS_READBODY,                  /* Reading HTTP body */
  HS_WAITCOMPLETE               /* Done reading, now waiting for
                                 * authentication etc. before closing
                                 * and/or retrying the connection */
} HttpStatus;

typedef struct _HttpConnection HttpConnection;

typedef void (*HCAuthFunc) (HttpConnection *conn, gboolean proxyauth,
                            gchar *realm, gpointer data);

/* A structure used to keep track of an HTTP connection */
struct _HttpConnection {
  gchar *HostName;              /* The machine on which the desired page
                                 * resides */
  unsigned Port;                /* The port */
  gchar *Proxy;                 /* If non-NULL, a web proxy to use */
  unsigned ProxyPort;           /* The port to use for talking to
                                 * the proxy */
  gchar *Method;                /* e.g. GET, POST */
  gchar *Query;                 /* e.g. the path of the desired webpage */
  gchar *Headers;               /* if non-NULL, e.g. Content-Type */
  gchar *Body;                  /* if non-NULL, data to send */
  gchar *RedirHost;             /* if non-NULL, a hostname to redirect to */
  gchar *RedirQuery;            /* if non-NULL, the path to redirect to */
  unsigned RedirPort;           /* The port on the host to redirect to */
  HCAuthFunc authfunc;          /* Callback function for authentication */
  gpointer authdata;            /* Data to be passed to authfunc */
  gboolean waitinput;           /* TRUE if we're waiting for auth etc. to
                                 * be supplied */
  gchar *user;                  /* The supplied username for HTTP auth */
  gchar *password;              /* The supplied password for HTTP auth */
  gchar *proxyuser;             /* The supplied username for HTTP
                                 * proxy auth */
  gchar *proxypassword;         /* The supplied password for HTTP
                                 * proxy auth */
  NetworkBuffer NetBuf;         /* The actual network connection itself */
  gint Tries;                   /* Number of requests actually sent so far */
  gint StatusCode;              /* 0=no status yet, otherwise an HTTP
                                 * status code */
  HttpStatus Status;
};

void InitNetworkBuffer(NetworkBuffer *NetBuf, char Terminator,
                       char StripChar, SocksServer *socks);
void SetNetworkBufferCallBack(NetworkBuffer *NetBuf, NBCallBack CallBack,
                              gpointer CallBackData);
void SetNetworkBufferUserPasswdFunc(NetworkBuffer *NetBuf,
                                    NBUserPasswd userpasswd,
                                    gpointer data);
gboolean IsNetworkBufferActive(NetworkBuffer *NetBuf);
void BindNetworkBufferToSocket(NetworkBuffer *NetBuf, int fd);
gboolean StartNetworkBufferConnect(NetworkBuffer *NetBuf,
                                   gchar *RemoteHost, unsigned RemotePort);
void ShutdownNetworkBuffer(NetworkBuffer *NetBuf);
void SetSelectForNetworkBuffer(NetworkBuffer *NetBuf, fd_set *readfds,
                               fd_set *writefds, fd_set *errorfds,
                               int *MaxSock);
gboolean RespondToSelect(NetworkBuffer *NetBuf, fd_set *readfds,
                         fd_set *writefds, fd_set *errorfds,
                         gboolean *DoneOK);
gboolean NetBufHandleNetwork(NetworkBuffer *NetBuf, gboolean ReadReady,
                             gboolean WriteReady, gboolean *DoneOK);
gboolean ReadDataFromWire(NetworkBuffer *NetBuf);
gboolean WriteDataToWire(NetworkBuffer *NetBuf);
void QueueMessageForSend(NetworkBuffer *NetBuf, gchar *data);
gint CountWaitingMessages(NetworkBuffer *NetBuf);
gchar *GetWaitingMessage(NetworkBuffer *NetBuf);
void SendSocks5UserPasswd(NetworkBuffer *NetBuf, gchar *user,
                          gchar *password);
gchar *GetWaitingData(NetworkBuffer *NetBuf, int numbytes);
gchar *PeekWaitingData(NetworkBuffer *NetBuf, int numbytes);
gchar *ExpandWriteBuffer(ConnBuf *conn, int numbytes, LastError **error);
void CommitWriteBuffer(NetworkBuffer *NetBuf, ConnBuf *conn, gchar *addpt,
                       guint addlen);

gboolean OpenHttpConnection(HttpConnection **conn, gchar *HostName,
                            unsigned Port, gchar *Proxy,
                            unsigned ProxyPort, SocksServer *socks,
                            gchar *Method, gchar *Query, gchar *Headers,
                            gchar *Body);
void CloseHttpConnection(HttpConnection *conn);
gchar *ReadHttpResponse(HttpConnection *conn, gboolean *doneOK);
void SetHttpAuthentication(HttpConnection *conn, gboolean proxy,
                           gchar *user, gchar *password);
void SetHttpAuthFunc(HttpConnection *conn, HCAuthFunc authfunc,
                     gpointer data);
gboolean HandleHttpCompletion(HttpConnection *conn);
gboolean IsHttpError(HttpConnection *conn);

int CreateTCPSocket(LastError **error);
gboolean BindTCPSocket(int sock, unsigned port, LastError **error);
void StartNetworking(void);
void StopNetworking(void);

#ifdef CYGWIN
#define CloseSocket(sock) closesocket(sock)
void SetReuse(SOCKET sock);
void SetBlocking(SOCKET sock, gboolean blocking);
#else
#define CloseSocket(sock) close(sock)
void SetReuse(int sock);
void SetBlocking(int sock, gboolean blocking);
#endif

void AddB64Enc(GString *str, gchar *unenc);

#endif /* NETWORKING */

#endif /* __NETWORK_H__ */
