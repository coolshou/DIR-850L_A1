/* vi: set sw=4 ts=4: */
/*
 *	APIs to use unix domain socket with DGRAM
 *
 *	Created by David Hsieh <david_hsieh@alphanetworks.com>
 *	Copyright (C) 2007-2009 by Alpha Networks, Inc.
 *
 *	This file is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either'
 *	version 2.1 of the License, or (at your option) any later version.
 *
 *	The GNU C Library is distributed in the hope that it will be useful,'
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	Lesser General Public License for more details.
 *
 *	You should have received a copy of the GNU Lesser General Public
 *	License along with the GNU C Library; if not, write to the Free
 *	Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *	02111-1307 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "unixsock.h"

struct usock_entry
{
	int sock;
	int is_server;
	struct sockaddr_un server;
	struct sockaddr_un client;
};

/***********************************************************************/

int usock_fd(usock_handle usock)
{
	struct usock_entry * us = (struct usock_entry *)usock;
	return us->sock;
}

const char * usock_server_path(usock_handle usock)
{
	struct usock_entry * us = (struct usock_entry *)usock;
	return us->server.sun_path;
}

const char * usock_client_path(usock_handle usock)
{
	struct usock_entry * us = (struct usock_entry *)usock;
	return us->client.sun_path;
}

/***********************************************************************/

usock_handle usock_open(int server, const char * name)
{
	struct usock_entry * entry = NULL;

	do
	{
		/* check socket name */
		if (!name) break;

		/* allocate entry space. */
		entry = (struct usock_entry *)malloc(sizeof(struct usock_entry));
		if (!entry) break;
		memset(entry, 0, sizeof(struct usock_entry));

		/* get socket fd */
		entry->sock = socket(AF_UNIX, SOCK_DGRAM, 0);
		if (entry->sock < 0) break;

		/* prepare server sockaddr */
		entry->server.sun_family = AF_UNIX;
		snprintf(entry->server.sun_path, sizeof(entry->server.sun_path), "%s", name);

		if (server)
		{
			/* if we are server, bind to server socket for incoming traffic. */
			unlink(entry->server.sun_path);
			if (bind(entry->sock, (struct sockaddr *)&entry->server, sizeof(entry->server)) < 0) break;
			entry->is_server = 1;
		}
		else
		{
			/* if we are client, bind to client socket for incomming traffic. */
			entry->client.sun_family = AF_UNIX;
			snprintf(entry->client.sun_path, sizeof(entry->client.sun_path), "%s_reply", name);
			unlink(entry->client.sun_path);
			if (bind(entry->sock, (struct sockaddr *)&(entry->client), sizeof(entry->client)) < 0) break;
			if (connect(entry->sock, (struct sockaddr *)&(entry->server), sizeof(entry->server)) < 0) break;
			entry->is_server = 0;
		}

		/* we are done ! */
		return entry;
	} while (0);

	if (entry->sock > 0)
	{
		close(entry->sock);
		unlink(server ? entry->server.sun_path : entry->client.sun_path);
	}
	free(entry);
	return NULL;
}

void usock_close(usock_handle usock)
{
	struct usock_entry * entry = (struct usock_entry *)usock;

	if (entry->sock > 0) close(entry->sock);
	unlink(entry->is_server ? entry->server.sun_path : entry->client.sun_path);
	free(entry);
}

int usock_send(usock_handle usock, const void * buf, unsigned int len, int flags)
{
	struct usock_entry * entry = (struct usock_entry *)usock;

	if (entry->is_server)
	{
		if (strlen(entry->client.sun_path) == 0) return -1;
		return sendto(entry->sock, buf, len, flags, (struct sockaddr *)&(entry->client), sizeof(entry->client));
	}
	return send(entry->sock, buf, len, flags);
}

int usock_recv(usock_handle usock, void * buf, unsigned int len, int flags)
{
	struct usock_entry * entry = (struct usock_entry *)usock;
	socklen_t fromlen;

	if (entry->is_server)
	{
		fromlen = sizeof(struct sockaddr_un);
		return recvfrom(entry->sock, buf, len, flags, (struct sockaddr *)&(entry->client), &fromlen);
	}
	return recv(entry->sock, buf, len, flags);
}

int usock_recv_timed(usock_handle usock, void * buf, unsigned int len, int flags, int timeout)
{
	int ret;
	struct timeval tv;
	fd_set fds;

	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(usock_fd(usock), &fds);
	ret = select(usock_fd(usock)+1, &fds, NULL, NULL, &tv);
	if (ret > 0) return usock_recv(usock, buf, len, flags);
	return ret;
}

/***********************************************************************/
