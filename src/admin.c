/************************************************************************
 * admin.c        dopewars server administration                        *
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

#ifndef CYGWIN
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <glib.h>

#include "network.h"
#include "nls.h"
#include "serverside.h"

static int OpenSocket(void)
{
  struct sockaddr_un addr;
  int sock;
  gchar *sockname;

  sockname = GetLocalSocket();

  g_print(_("Attempting to connect to local dopewars server via. "
            "Unix domain\n socket %s...\n"), sockname);
  sock = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket");
    exit(1);
  }

  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sockname, sizeof(addr.sun_path));
  addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

  if (connect(sock, (struct sockaddr *)&addr,
              sizeof(struct sockaddr_un)) == -1) {
    perror("connect");
    exit(1);
  }

  g_print(_("Connection established; use Ctrl-D to "
            "close your session.\n\n"));
  g_free(sockname);

  return sock;
}

void AdminServer(void)
{
  int sock, topsock;
  NetworkBuffer *netbuf;
  fd_set readfds, writefds, errorfds;
  gchar *msg, inbuf[200];
  gboolean doneOK;

  sock = OpenSocket();
  netbuf = g_new(NetworkBuffer, 1);

  InitNetworkBuffer(netbuf, '\n', '\r', NULL);
  BindNetworkBufferToSocket(netbuf, sock);

  while (1) {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&errorfds);

    FD_SET(0, &readfds);
    topsock = 1;
    SetSelectForNetworkBuffer(netbuf, &readfds, &writefds, &errorfds,
                              &topsock);

    if (select(topsock, &readfds, &writefds, &errorfds, NULL) == -1) {
      if (errno == EINTR)
        continue;
      else
        perror("select");
      break;
    }

    if (FD_ISSET(0, &readfds)) {
      if (fgets(inbuf, sizeof(inbuf), stdin)) {
        inbuf[sizeof(inbuf) - 1] = '\0';
        if (strlen(inbuf) > 0) {
          if (inbuf[strlen(inbuf) - 1] == '\n')
            inbuf[strlen(inbuf) - 1] = '\0';
          QueueMessageForSend(netbuf, inbuf);
        }
      } else
        break;
    }

    if (RespondToSelect(netbuf, &readfds, &writefds, &errorfds, &doneOK)) {
      while ((msg = GetWaitingMessage(netbuf)) != NULL) {
        g_print("%s\n", msg);
        g_free(msg);
      }
    }
    if (!doneOK)
      break;
  }
  ShutdownNetworkBuffer(netbuf);
  g_free(netbuf);
  g_print("Connection closed\n");
}
#endif
