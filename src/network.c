/* network.c      Low-level networking routines                         */
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef NETWORKING

#ifdef CYGWIN
#include <windows.h>    /* For datatypes such as BOOL */
#include <winsock.h>    /* For network functions */
#else
#include <sys/types.h>  /* For size_t etc. */
#include <sys/socket.h> /* For struct sockaddr etc. */
#include <netinet/in.h> /* For struct sockaddr_in etc. */
#include <arpa/inet.h>  /* For socklen_t */
#include <string.h>     /* For memcpy, strlen etc. */
#ifdef HAVE_UNISTD_H
#include <unistd.h>     /* For close(), various types and constants */
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>      /* For fcntl() */
#endif
#include <netdb.h>      /* For gethostbyname() */
#endif /* CYGWIN */

#include <glib.h>
#include <errno.h>      /* For errno and Unix error codes */
#include <stdlib.h>     /* For exit() and atoi() */
#include <stdio.h>      /* For perror() */

#include "error.h"
#include "network.h"
#include "nls.h"

/* Maximum sizes (in bytes) of read and write buffers - connections should
   be dropped if either buffer is filled */
#define MAXREADBUF   (32768)
#define MAXWRITEBUF  (65536)

#ifdef CYGWIN

void StartNetworking() {
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(1,0),&wsaData)!=0) {
    g_warning(_("Cannot initialise WinSock!"));
    exit(1);
  }
}

void StopNetworking() {
  WSACleanup();
}

void SetReuse(SOCKET sock) {
  BOOL i=TRUE;
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&i,sizeof(i))==-1) {
    perror("setsockopt"); exit(1);
  }
}

void SetBlocking(SOCKET sock,gboolean blocking) {
  unsigned long param;
  param = blocking ? 0 : 1;
  ioctlsocket(sock,FIONBIO,&param);
}

#else

void StartNetworking() {}
void StopNetworking() {}

void SetReuse(int sock) {
  int i=1;
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&i,sizeof(i))==-1) {
    perror("setsockopt"); exit(1);
  }
}

void SetBlocking(int sock,gboolean blocking) {
  fcntl(sock,F_SETFL,blocking ? 0 : O_NONBLOCK);
}

#endif /* CYGWIN */

static gboolean FinishConnect(int fd,LastError *error);

static void NetBufCallBack(NetworkBuffer *NetBuf) {
   if (NetBuf && NetBuf->CallBack) {
      (*NetBuf->CallBack)(NetBuf,!NetBuf->WaitConnect,
                          NetBuf->WriteBuf.DataPresent || NetBuf->WaitConnect);
   }
}

static void NetBufCallBackStop(NetworkBuffer *NetBuf) {
   if (NetBuf && NetBuf->CallBack) (*NetBuf->CallBack)(NetBuf,FALSE,FALSE);
}

void InitNetworkBuffer(NetworkBuffer *NetBuf,char Terminator,char StripChar) {
/* Initialises the passed network buffer, ready for use. Messages sent */
/* or received on the buffered connection will be terminated by the    */
/* given character, and if they end in "StripChar" it will be removed  */
/* before the messages are sent or received.                           */
   NetBuf->fd=-1;
   NetBuf->InputTag=0;
   NetBuf->CallBack=NULL;
   NetBuf->CallBackData=NULL;
   NetBuf->Terminator=Terminator;
   NetBuf->StripChar=StripChar;
   NetBuf->ReadBuf.Data=NetBuf->WriteBuf.Data=NULL;
   NetBuf->ReadBuf.Length=NetBuf->WriteBuf.Length=0;
   NetBuf->ReadBuf.DataPresent=NetBuf->WriteBuf.DataPresent=0;
   NetBuf->WaitConnect=FALSE;
   ClearError(&NetBuf->error);
}

void SetNetworkBufferCallBack(NetworkBuffer *NetBuf,NBCallBack CallBack,
                              gpointer CallBackData) {
   NetBufCallBackStop(NetBuf);
   NetBuf->CallBack=CallBack;
   NetBuf->CallBackData=CallBackData;
   NetBufCallBack(NetBuf);
}

void BindNetworkBufferToSocket(NetworkBuffer *NetBuf,int fd) {
/* Sets up the given network buffer to handle data being sent/received */
/* through the given socket                                            */
   NetBuf->fd=fd;
}

gboolean IsNetworkBufferActive(NetworkBuffer *NetBuf) {
/* Returns TRUE if the pointer is to a valid network buffer, and it's */
/* connected to an active socket.                                     */
   return (NetBuf && NetBuf->fd>=0);
}

gboolean StartNetworkBufferConnect(NetworkBuffer *NetBuf,gchar *RemoteHost,
                                   unsigned RemotePort) {
  ShutdownNetworkBuffer(NetBuf);
  if (StartConnect(&NetBuf->fd,RemoteHost,RemotePort,TRUE,&NetBuf->error)) {
    NetBuf->WaitConnect=TRUE;

/* Notify the owner if necessary to check for the connection completing */
    NetBufCallBack(NetBuf);

    return TRUE;
  } else {
    return FALSE;
  }
}

void ShutdownNetworkBuffer(NetworkBuffer *NetBuf) {
/* Frees the network buffer's data structures (leaving it in the  */
/* 'initialised' state) and closes the accompanying socket.       */

   NetBufCallBackStop(NetBuf);

   if (NetBuf->fd>=0) CloseSocket(NetBuf->fd);

   g_free(NetBuf->ReadBuf.Data);
   g_free(NetBuf->WriteBuf.Data);

   InitNetworkBuffer(NetBuf,NetBuf->Terminator,NetBuf->StripChar);
}

void SetSelectForNetworkBuffer(NetworkBuffer *NetBuf,fd_set *readfds,
                               fd_set *writefds,fd_set *errorfds,int *MaxSock) {
/* Updates the sets of read and write file descriptors to monitor    */
/* input to/output from the given network buffer. MaxSock is updated */
/* with the highest-numbered file descriptor (plus 1) for use in a   */
/* later select() call.                                              */
   if (!NetBuf || NetBuf->fd<=0) return;
   FD_SET(NetBuf->fd,readfds);
   if (errorfds) FD_SET(NetBuf->fd,errorfds);
   if (NetBuf->fd >= *MaxSock) *MaxSock=NetBuf->fd+1;
   if (NetBuf->WriteBuf.DataPresent || NetBuf->WaitConnect) {
      FD_SET(NetBuf->fd,writefds);
   }
}

static gboolean DoNetworkBufferStuff(NetworkBuffer *NetBuf,gboolean ReadReady,
                                     gboolean WriteReady,gboolean ErrorReady,
                                     gboolean *ReadOK,gboolean *WriteOK,
                                     gboolean *ErrorOK) {
/* Reads and writes data if the network connection is ready. Sets the  */
/* various OK variables to TRUE if no errors occurred in the relevant  */
/* operations, and returns TRUE if data was read and is waiting for    */
/* processing.                                                         */
   gboolean DataWaiting=FALSE,ConnectDone=FALSE;
   gboolean retval;
   *ReadOK=*WriteOK=*ErrorOK=TRUE;

   if (ErrorReady) *ErrorOK=FALSE;
   else if (NetBuf->WaitConnect) {
      if (WriteReady) {
         retval=FinishConnect(NetBuf->fd,&NetBuf->error);
         ConnectDone=TRUE;
         NetBuf->WaitConnect=FALSE;

         if (!retval) {
            *WriteOK=FALSE;
         }
      }
   } else {
      if (WriteReady) *WriteOK=WriteDataToWire(NetBuf);

      if (ReadReady) {
         *ReadOK=ReadDataFromWire(NetBuf);
         if (NetBuf->ReadBuf.DataPresent>0) DataWaiting=TRUE;
      }
   }

   if (!(*ErrorOK && *WriteOK && *ReadOK)) {
/* We don't want to check the socket any more */
      NetBufCallBackStop(NetBuf);
/* If there were errors, then the socket is now useless - so close it */
      CloseSocket(NetBuf->fd);
      NetBuf->fd=-1;
   } else if (ConnectDone) {
/* If we just connected, then no need to listen for write-ready status
   any more */
      NetBufCallBack(NetBuf);
   } else if (WriteReady && NetBuf->WriteBuf.DataPresent==0) {
/* If we wrote out everything, then tell the owner so that the socket no
   longer needs to be checked for write-ready status */
      NetBufCallBack(NetBuf);
   }

   return DataWaiting;
}

gboolean RespondToSelect(NetworkBuffer *NetBuf,fd_set *readfds,
                         fd_set *writefds,fd_set *errorfds,
                         gboolean *DoneOK) {
/* Responds to a select() call by reading/writing data as necessary.   */
/* If any data were read, TRUE is returned. "DoneOK" is set TRUE       */
/* unless a fatal error (i.e. the connection was broken) occurred.     */
   gboolean ReadOK,WriteOK,ErrorOK;
   gboolean DataWaiting=FALSE;

   *DoneOK=TRUE;
   if (!NetBuf || NetBuf->fd<=0) return DataWaiting;
   DataWaiting=DoNetworkBufferStuff(NetBuf,FD_ISSET(NetBuf->fd,readfds),
                        FD_ISSET(NetBuf->fd,writefds),
                        errorfds ? FD_ISSET(NetBuf->fd,errorfds) : FALSE,
                        &ReadOK,&WriteOK,&ErrorOK);
   *DoneOK=(WriteOK && ErrorOK && ReadOK);
   return DataWaiting;
}

gboolean NetBufHandleNetwork(NetworkBuffer *NetBuf,gboolean ReadReady,
                             gboolean WriteReady,gboolean *DoneOK) {
   gboolean ReadOK,WriteOK,ErrorOK;
   gboolean DataWaiting=FALSE;

   *DoneOK=TRUE;
   if (!NetBuf || NetBuf->fd<=0) return DataWaiting;

   DataWaiting=DoNetworkBufferStuff(NetBuf,ReadReady,WriteReady,FALSE,
                                    &ReadOK,&WriteOK,&ErrorOK);

   *DoneOK=(WriteOK && ErrorOK && ReadOK);
   return DataWaiting;
}

gint CountWaitingMessages(NetworkBuffer *NetBuf) {
/* Returns the number of complete (terminated) messages waiting in the   */
/* given network buffer. This is the number of times that                */
/* GetWaitingMessage() can be safely called without it returning NULL.   */
   ConnBuf *conn;
   gint i,msgs=0;

   conn=&NetBuf->ReadBuf;

   if (conn->Data) for (i=0;i<conn->DataPresent;i++) {
      if (conn->Data[i]==NetBuf->Terminator) msgs++;
   }
   return msgs;
}

gchar *GetWaitingMessage(NetworkBuffer *NetBuf) {
/* Reads a complete (terminated) message from the network buffer. The    */
/* message is removed from the buffer, and returned as a null-terminated */
/* string (the network terminator is removed). If no complete message is */
/* waiting, NULL is returned. The string is dynamically allocated, and   */
/* so must be g_free'd by the caller.                                    */
   ConnBuf *conn;
   int MessageLen;
   char *SepPt;
   gchar *NewMessage;
   conn=&NetBuf->ReadBuf;
   if (!conn->Data || !conn->DataPresent) return NULL;
   SepPt=memchr(conn->Data,NetBuf->Terminator,conn->DataPresent);
   if (!SepPt) return NULL;
   *SepPt='\0';
   MessageLen=SepPt-conn->Data+1;
   SepPt--;
   if (NetBuf->StripChar && *SepPt==NetBuf->StripChar) *SepPt='\0';
   NewMessage=g_new(gchar,MessageLen);
   memcpy(NewMessage,conn->Data,MessageLen);
   if (MessageLen<conn->DataPresent) {
      memmove(&conn->Data[0],&conn->Data[MessageLen],
              conn->DataPresent-MessageLen);
   }
   conn->DataPresent-=MessageLen;
   return NewMessage;
}

gboolean ReadDataFromWire(NetworkBuffer *NetBuf) {
/* Reads any waiting data on the given network buffer's TCP/IP connection */
/* into the read buffer. Returns FALSE if the connection was closed, or   */
/* if the read buffer's maximum size was reached.                         */
   ConnBuf *conn;
   int CurrentPosition,BytesRead;
   conn=&NetBuf->ReadBuf;
   CurrentPosition=conn->DataPresent;
   while(1) {
      if (CurrentPosition>=conn->Length) {
         if (conn->Length==MAXREADBUF) {
            SetError(&NetBuf->error,ET_CUSTOM,E_FULLBUF);
            return FALSE; /* drop connection */
         }
         if (conn->Length==0) conn->Length=256; else conn->Length*=2;
         if (conn->Length>MAXREADBUF) conn->Length=MAXREADBUF;
         conn->Data=g_realloc(conn->Data,conn->Length);
      }
      BytesRead=recv(NetBuf->fd,&conn->Data[CurrentPosition],
                     conn->Length-CurrentPosition,0);
      if (BytesRead==SOCKET_ERROR) {
#ifdef CYGWIN
         int Error = WSAGetLastError();
         if (Error==WSAEWOULDBLOCK) break;
         else { SetError(&NetBuf->error,ET_WINSOCK,Error); return FALSE; }
#else
         if (errno==EAGAIN) break;
         else if (errno!=EINTR) {
            SetError(&NetBuf->error,ET_ERRNO,errno);
            return FALSE;
         }
#endif
      } else if (BytesRead==0) {
         return FALSE;
      } else {
         CurrentPosition+=BytesRead;
         conn->DataPresent=CurrentPosition;
      }
   }
   return TRUE;
}

void QueueMessageForSend(NetworkBuffer *NetBuf,gchar *data) {
/* Writes the null-terminated string "data" to the network buffer, ready   */
/* to be sent to the wire when the network connection becomes free. The    */
/* message is automatically terminated. Fails to write the message without */
/* error if the buffer reaches its maximum size (although this error will  */
/* be detected when an attempt is made to write the buffer to the wire).   */
   int AddLength,NewLength;
   ConnBuf *conn;
   conn=&NetBuf->WriteBuf;
   AddLength=strlen(data)+1;
   NewLength=conn->DataPresent+AddLength;
   if (NewLength > conn->Length) {
      conn->Length*=2;
      conn->Length=MAX(conn->Length,NewLength);
      if (conn->Length > MAXWRITEBUF) conn->Length=MAXWRITEBUF;
      if (NewLength > conn->Length) return;
      conn->Data=g_realloc(conn->Data,conn->Length);
   }
   memcpy(&conn->Data[conn->DataPresent],data,AddLength);
   conn->DataPresent=NewLength;
   conn->Data[NewLength-1]=NetBuf->Terminator;

/* If the buffer was empty before, we may need to tell the owner to check
   the socket for write-ready status */
   if (NewLength==AddLength) NetBufCallBack(NetBuf);
}

gboolean WriteDataToWire(NetworkBuffer *NetBuf) {
/* Writes any waiting data in the network buffer to the wire. Returns */
/* TRUE on success, or FALSE if the buffer's maximum length is        */
/* reached, or the remote end has closed the connection.              */
   ConnBuf *conn;
   int CurrentPosition,BytesSent;
   conn=&NetBuf->WriteBuf;
   if (!conn->Data || !conn->DataPresent) return TRUE;
   if (conn->Length==MAXWRITEBUF) {
      SetError(&NetBuf->error,ET_CUSTOM,E_FULLBUF);
      return FALSE;
   }
   CurrentPosition=0;
   while (CurrentPosition<conn->DataPresent) {
      BytesSent=send(NetBuf->fd,&conn->Data[CurrentPosition],
                     conn->DataPresent-CurrentPosition,0);
      if (BytesSent==SOCKET_ERROR) {
#ifdef CYGWIN
         int Error=WSAGetLastError();
         if (Error==WSAEWOULDBLOCK) break;
         else { SetError(&NetBuf->error,ET_WINSOCK,Error); return FALSE; }
#else
         if (errno==EAGAIN) break;
         else if (errno!=EINTR) {
            SetError(&NetBuf->error,ET_ERRNO,errno);
            return FALSE;
         }
#endif
      } else {
         CurrentPosition+=BytesSent;
      }
   }
   if (CurrentPosition>0 && CurrentPosition<conn->DataPresent) {
      memmove(&conn->Data[0],&conn->Data[CurrentPosition],
              conn->DataPresent-CurrentPosition);
   }
   conn->DataPresent-=CurrentPosition;
   return TRUE;
}

static void SendHttpRequest(HttpConnection *conn) {
   GString *text;
   char *userpasswd;

   conn->Tries++;
   conn->StatusCode=0;
   conn->Status=HS_CONNECTING;
   conn->authsupplied=FALSE;

   text=g_string_new("");

   g_string_sprintf(text,"%s http://%s:%u%s HTTP/1.0",
                    conn->Method,conn->HostName,conn->Port,conn->Query);
   QueueMessageForSend(&conn->NetBuf,text->str);

   if (conn->Headers) QueueMessageForSend(&conn->NetBuf,conn->Headers);

   if (conn->user && conn->password) {
      userpasswd = g_strdup_printf("%s:%s",conn->user,conn->password);
      g_string_assign(text,conn->proxyauth ? "Proxy-Authenticate" :
                                             "Authorization");
      g_string_append(text,": Basic ");
      AddB64Enc(text,userpasswd);
      g_free(userpasswd);
      QueueMessageForSend(&conn->NetBuf,text->str);
   }

   g_string_sprintf(text,"User-Agent: dopewars/%s",VERSION);
   QueueMessageForSend(&conn->NetBuf,text->str);

/* Insert a blank line between headers and body */
   QueueMessageForSend(&conn->NetBuf,"");

   if (conn->Body) QueueMessageForSend(&conn->NetBuf,conn->Body);

   g_string_free(text,TRUE);
}

static gboolean StartHttpConnect(HttpConnection *conn) {
   gchar *ConnectHost;
   unsigned ConnectPort;

   if (conn->Proxy) {
      ConnectHost=conn->Proxy; ConnectPort=conn->ProxyPort;
   } else {
      ConnectHost=conn->HostName; ConnectPort=conn->Port;
   }
      
   if (!StartNetworkBufferConnect(&conn->NetBuf,ConnectHost,ConnectPort)) {
      return FALSE;
   }
   return TRUE;
}

gboolean OpenHttpConnection(HttpConnection **connpt,gchar *HostName,
                            unsigned Port,gchar *Proxy,unsigned ProxyPort,
                            gchar *Method,gchar *Query,
                            gchar *Headers,gchar *Body) {
   HttpConnection *conn;
   g_assert(HostName && Method && Query && connpt);

   conn=g_new0(HttpConnection,1);
   InitNetworkBuffer(&conn->NetBuf,'\n','\r');
   conn->HostName=g_strdup(HostName);
   if (Proxy && Proxy[0]) conn->Proxy=g_strdup(Proxy);
   conn->Method=g_strdup(Method);
   conn->Query=g_strdup(Query);
   if (Headers && Headers[0]) conn->Headers=g_strdup(Headers);
   if (Body && Body[0]) conn->Body=g_strdup(Body);
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

void CloseHttpConnection(HttpConnection *conn) {
   ShutdownNetworkBuffer(&conn->NetBuf);
   g_free(conn->HostName);
   g_free(conn->Proxy);
   g_free(conn->Method);
   g_free(conn->Query);
   g_free(conn->Headers);
   g_free(conn->Body);
   g_free(conn->RedirHost);
   g_free(conn->RedirQuery);
   g_free(conn->realm);
   g_free(conn->user);
   g_free(conn->password);
   g_free(conn);
}

gboolean IsHttpError(HttpConnection *conn) {
   return IsError(&conn->NetBuf.error);
}

void SetHttpAuthentication(HttpConnection *conn,gchar *user,gchar *password) {
   g_assert(conn && user && password);
   g_free(conn->user);
   g_free(conn->password);
   conn->user = g_strdup(user);
   conn->password = g_strdup(password);
}

void SetHttpAuthFunc(HttpConnection *conn,HCAuthFunc authfunc) {
   g_assert(conn && authfunc);
   conn->authfunc = authfunc;
}

static gboolean ParseHtmlLocation(gchar *uri,gchar **host,unsigned *port,
                                  gchar **query) {
  gchar *uris,*colon,*slash;

  uris = g_strstrip(uri);
  if (!uris || strlen(uris)<7 ||
      g_strncasecmp(uris,"http://",7)!=0) return FALSE;

  uris+=7; /* skip to hostname */

/* ':' denotes the port to connect to */
  colon = strchr(uris,':');
  if (colon && colon==uris) return FALSE; /* No hostname */

/* '/' denotes the start of the path of the HTML file */
  slash = strchr(uris,'/');
  if (slash && slash==uris) return FALSE; /* No hostname */

  if (colon && (!slash || slash>colon)) {
    if (slash) *slash='\0';
    *port = atoi(colon+1);
    if (slash) *slash='\\';
    if (*port==0) return FALSE; /* Invalid port */
    *host = g_strndup(uris,colon-uris);
  } else {
    *port=80;
    if (slash) *host=g_strndup(uris,slash-uris);
    else *host=g_strdup(uris);
  }

  if (slash) {
    *query = g_strdup(slash);
  } else {
    *query = g_strdup("/");
  }
  return TRUE;
}

static void ParseHtmlHeader(gchar *line,HttpConnection *conn) {
  gchar **split,*host,*query;
  unsigned port;

  split=g_strsplit(line," ",1);
  if (split[0] && split[1]) {
    if (g_strcasecmp(split[0],"Location:")==0 &&
        (conn->StatusCode==302 || conn->StatusCode==301)) {
      if (ParseHtmlLocation(split[1],&host,&port,&query)) {
        g_print("Redirect to %s:%u%s\n",host,port,query);
        g_free(conn->RedirHost); g_free(conn->RedirQuery);
        conn->RedirHost=host; conn->RedirQuery=query;
        conn->RedirPort=port;
      } else {
        g_print("FIXME: Bad redirect\n");
      }
    } else if (g_strcasecmp(split[0],"WWW-Authenticate:")==0 &&
               conn->StatusCode==401) {
      g_print("FIXME: Authentication %s required\n",split[1]);
      conn->proxyauth=FALSE;
      if (conn->authfunc) conn->authsupplied=(*conn->authfunc)(conn,split[1]);
/* Proxy-Authenticate is, strictly speaking, an HTTP/1.1 thing, but some
   HTTP/1.0 proxies seem to support it anyway */
    } else if (g_strcasecmp(split[0],"Proxy-Authenticate:")==0 &&
               conn->StatusCode==407) {
      g_print("FIXME: Proxy authentication %s required\n",split[1]);
      conn->proxyauth=TRUE;
      if (conn->authfunc) conn->authsupplied=(*conn->authfunc)(conn,split[1]);
    }
  }
  g_strfreev(split);
}

gchar *ReadHttpResponse(HttpConnection *conn) {
   gchar *msg,**split;

   msg=GetWaitingMessage(&conn->NetBuf);
   if (msg) switch(conn->Status) {
      case HS_CONNECTING:    /* OK, we should have the HTTP status line */
         conn->Status=HS_READHEADERS;
         split=g_strsplit(msg," ",2);
         if (split[0] && split[1]) {
            conn->StatusCode=atoi(split[1]);
            g_print("HTTP status code %d returned\n",conn->StatusCode);
         } else g_warning("Invalid HTTP status line %s",msg);
         g_strfreev(split);
         break;
      case HS_READHEADERS:
         if (msg[0]==0) conn->Status=HS_READSEPARATOR;
         else ParseHtmlHeader(msg,conn);
         break;
      case HS_READSEPARATOR:
         conn->Status=HS_READBODY;
         break;
      case HS_READBODY:   /* At present, we do nothing special with the body */
         break;
   }
   return msg;
}

gboolean HandleHttpCompletion(HttpConnection *conn) {
   NBCallBack CallBack;
   gpointer CallBackData;
   gboolean retry=FALSE;

   if (conn->Tries>=5) {
      g_print("FIXME: Number of tries exceeded\n");
      return TRUE;
   }

   if (conn->RedirHost) {
      g_print("Following redirect to %s\n",conn->RedirHost);
      g_free(conn->HostName); g_free(conn->Query);
      conn->HostName = conn->RedirHost;
      conn->Query = conn->RedirQuery;
      conn->Port = conn->RedirPort;
      conn->RedirHost = conn->RedirQuery = NULL;
      retry = TRUE;
   }
   if (conn->authsupplied && conn->user && conn->password) {
      g_print("Trying again with authentication\n");
      retry = TRUE;
   }

   if (retry) {
      CallBack=conn->NetBuf.CallBack;
      CallBackData=conn->NetBuf.CallBackData;
      ShutdownNetworkBuffer(&conn->NetBuf);
      if (StartHttpConnect(conn)) {
         SendHttpRequest(conn);
         SetNetworkBufferCallBack(&conn->NetBuf,CallBack,CallBackData);
         return FALSE;
      }
   }
   return TRUE;
}

gboolean StartConnect(int *fd,gchar *RemoteHost,unsigned RemotePort,
                      gboolean NonBlocking,LastError *error) {
   struct sockaddr_in ClientAddr;
   struct hostent *he;

   if ((he=gethostbyname(RemoteHost))==NULL) {
#ifdef CYGWIN
      if (error) SetError(error,ET_WINSOCK,WSAGetLastError());
#else
      if (error) SetError(error,ET_HERRNO,h_errno);
#endif
      return FALSE;
   }
   *fd=socket(AF_INET,SOCK_STREAM,0);
   if (*fd==SOCKET_ERROR) {
#ifdef CYGWIN
      if (error) SetError(error,ET_WINSOCK,WSAGetLastError());
#else
      if (error) SetError(error,ET_ERRNO,errno);
#endif
      return FALSE;
   }

   ClientAddr.sin_family=AF_INET;
   ClientAddr.sin_port=htons(RemotePort);
   ClientAddr.sin_addr=*((struct in_addr *)he->h_addr);
   memset(ClientAddr.sin_zero,0,sizeof(ClientAddr.sin_zero));

   SetBlocking(*fd,!NonBlocking);

   if (connect(*fd,(struct sockaddr *)&ClientAddr,
       sizeof(struct sockaddr))==SOCKET_ERROR) {
#ifdef CYGWIN
      int errcode=WSAGetLastError();
      if (errcode==WSAEWOULDBLOCK) return TRUE;
      else if (error) SetError(error,ET_WINSOCK,errcode);
#else
      if (errno==EINPROGRESS) return TRUE;
      else if (error) SetError(error,ET_ERRNO,errno);
#endif
      CloseSocket(*fd); *fd=-1;
      return FALSE;
   } else {
      SetBlocking(*fd,FALSE); /* All connected sockets should be nonblocking */
   }
   return TRUE;
}

gboolean FinishConnect(int fd,LastError *error) {
   int errcode;
#ifdef CYGWIN
   errcode = WSAGetLastError();
   if (errcode==0) return TRUE;
   else {
     if (error) { SetError(error,ET_WINSOCK,errcode); }
     return FALSE;
   }
#else
#ifdef HAVE_SOCKLEN_T
   socklen_t optlen;
#else
   int optlen;
#endif

   optlen=sizeof(errcode);
   if (getsockopt(fd,SOL_SOCKET,SO_ERROR,&errcode,&optlen)==-1) {
      errcode = errno;
   }
   if (errcode==0) return TRUE;
   else {
     if (error) { SetError(error,ET_ERRNO,errcode); }
     return FALSE;
   }
#endif /* CYGWIN */
}

static void AddB64char(GString *str,int c) {
   if (c<0) return;
   else if (c<26) g_string_append_c(str,c+'A');
   else if (c<52) g_string_append_c(str,c-26+'a');
   else if (c<62) g_string_append_c(str,c-52+'0');
   else if (c==62) g_string_append_c(str,'+');
   else g_string_append_c(str,'/');
}

void AddB64Enc(GString *str,gchar *unenc) {
/* Adds the plain text string "unenc" to the end of the GString "str", */
/* using the Base64 encoding scheme.                                   */
   guint i;
   long value=0;
   if (!unenc || !str) return;
   for (i=0;i<strlen(unenc);i++) {
      value <<= 8;
      value |= (unsigned char)unenc[i];
      if (i % 3 == 2) {
        AddB64char(str,(value>>18)&0x3F);
        AddB64char(str,(value>>12)&0x3F);
        AddB64char(str,(value>>6)&0x3F);
        AddB64char(str,value&0x3F);
        value=0;
      }
   }
   if (i % 3 == 1) {
      AddB64char(str,(value>>2)&0x3F);
      AddB64char(str,(value<<4)&0x3F);
      g_string_append(str,"==");
   } else if (i % 3 == 2) {
      AddB64char(str,(value>>10)&0x3F);
      AddB64char(str,(value>>4)&0x3F);
      AddB64char(str,(value<<2)&0x3F);
      g_string_append_c(str,'=');
   }
}

#endif /* NETWORKING */
