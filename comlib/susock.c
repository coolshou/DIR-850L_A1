/* vi: set sw=4 ts=4: */
/*
 *	Stream Unix Socket
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/un.h>


#include <dtrace.h>
#include <dirutil.h>
#include <susock.h>

#define SUSOCK_MAGIC	0x5C50C3E7
#define SUSCLI_MAGIC	0x5C5C71E7

//typedef void (*susock_callback)(susock_handle * sock, void * param);

struct susock_client
{
	uint32_t magic;
	int fd;
	struct stream_socket * susock;
};

struct stream_socket
{
	uint32_t magic;
	int fd;
	size_t max_client;
	struct susock_client * clients;
	susock_callback callback;
};

/*************************************************************************/

int susock_fd(susock_handle sock)
{
	struct susock_client * c = (struct susock_client *)sock;
	struct stream_socket * s = (struct stream_socket *)sock;
	if (sock == NULL) return -1;
	if (c->magic == SUSCLI_MAGIC) return c->fd;
	if (s->magic == SUSOCK_MAGIC) return s->fd;
	return -1;
}

ssize_t susock_send(susock_handle sock, const void * buf, size_t len, int flags)
{
	struct susock_client * c = (struct susock_client *)sock;
	struct stream_socket * s = (struct stream_socket *)sock;
	if (sock == NULL) return -1;
	if (c->magic == SUSCLI_MAGIC) return send(c->fd, buf, len, flags);
	if (s->magic == SUSOCK_MAGIC) return send(s->fd, buf, len, flags);
	return -1;
}

ssize_t susock_recv(susock_handle sock, void * buf, size_t len, int flags)
{
	struct susock_client * c = (struct susock_client *)sock;
	struct stream_socket * s = (struct stream_socket *)sock;
	if (sock == NULL) return -1;
	if (c->magic == SUSCLI_MAGIC) return recv(c->fd, buf, len, flags);
	if (s->magic == SUSOCK_MAGIC) return recv(s->fd, buf, len, flags);
	return -1;
}

int susock_close(susock_handle sock)
{
	size_t i;
	struct susock_client * c = (struct susock_client *)sock;
	struct stream_socket * s = (struct stream_socket *)sock;

	if (sock == NULL) return -1;
	if (c->magic == SUSCLI_MAGIC)
	{
		if (c->fd >= 0) close(c->fd);
		c->fd = -1;
		return 0;
	}
	else if (s->magic == SUSOCK_MAGIC)
	{
		if (s->clients)
		{
			for (i=0; i<s->max_client; i++)
			{
				if (s->clients[i].fd >= 0)
				{
					if (s->clients[i].fd >= 0)
						close(s->clients[i].fd);
					s->clients[i].fd = -1;
				}
			}
			free(s->clients);
		}
		if (s->fd >= 0) close(s->fd);
		free(sock);
		return 0;
	}
	return -1;
}

susock_handle susock_open(const char * name)
{
	struct sockaddr_un where;
	struct stream_socket * sock = (struct stream_socket *)malloc(sizeof(struct stream_socket));
	susock_handle sh = NULL;

	if (sock)
	{
		memset(sock, 0, sizeof(struct stream_socket));
		sock->magic = SUSOCK_MAGIC;
		if (name)
		{
			sock->fd = socket(AF_UNIX, SOCK_STREAM, 0);
			if (sock->fd >= 0)
			{
				fcntl(sock->fd, F_SETFD, FD_CLOEXEC);
				where.sun_family = AF_UNIX;
				snprintf(where.sun_path, sizeof(where.sun_path), "%s", name);
				if (connect(sock->fd, (struct sockaddr *)&where, sizeof(where)) >=0)
				{
					sh = sock;
					sock = NULL;
				}
			}
		}
	}
	if (sock)
	{
		if (sock->fd >= 0) close(sock->fd);
		free(sock);
	}
	return sh;
}

/*************************************************************************/

int susock_server_sloop_handler(int s, void * param, void * data)
{
	struct stream_socket * sock = (struct stream_socket *)param;
	struct sockaddr_un from;
	socklen_t len;
	int fd = -1;
	size_t i;

	/* accept this connection. */
	len = sizeof(from);
	fd = accept(s, (struct sockaddr *)&from, &len);
	if (fd < 0)
	{
		d_error("%s: socket not accepted: %s.\n",__func__,strerror(errno));
		return fd;
	}
	/* look for a free client space. */
	dassert(sock!=NULL);
	dassert(sock->magic == SUSOCK_MAGIC);
	if (sock && sock->magic == SUSOCK_MAGIC && sock->clients)
	{
		for (i=0; i<sock->max_client; i++)
		{
			if (sock->clients[i].fd < 0)
			{
				/* call the callback function. */
				sock->clients[i].magic = SUSCLI_MAGIC;
				sock->clients[i].fd = fd;
				sock->clients[i].susock = sock;
				d_dbg("%s: found space @ %d\n",__func__,i);
				if (sock->callback) sock->callback(sock, &sock->clients[i]);
				return 0;
			}
		}
	}
	/* no space if we reach here. */
	d_warn("%s: no space for client.\n",__func__);
	close(fd);
	return 0;
}

susock_handle susock_server_open(const char * name, size_t max_client, susock_callback callback)
{
	struct sockaddr_un where;
	struct stat st;
	char * dir = NULL;
	int s = -1;;
	susock_handle sh = NULL;
	struct stream_socket * sock = NULL;
	size_t i;

	do
	{
		/* We need the socket name. */
		if (!name) return NULL;
		/* check if unixsocket is already created */
		if (stat(name, &st) >= 0)
		{
			fprintf(stderr, "%s: '%s' is already exist!\n",__func__,name);
			break;
		}
#if 0
		/* make sure the path is valid. */
		dir = dirname(name);
		if (!make_valid_path(dir, 0770))
		{
			fprintf(stderr, "%s: cound not make path to %s: %s.\n",__func__,
				name,strerror(errno));
			break;
		}
#endif
		/* get the socket */
		s = socket(AF_UNIX, SOCK_STREAM, 0);
		if (s < 0)
		{
			fprintf(stderr, "%s: socket: %s.\n",__func__,strerror(errno));
			break;
		}
		/* bind the socket */
		where.sun_family = AF_UNIX;
		snprintf(where.sun_path, sizeof(where.sun_path), "%s", name);
		if (bind(s, (struct sockaddr *)&where, sizeof(where)) < 0)
		{
			fprintf(stderr, "%s: bind: %s.\n",__func__,strerror(errno));
			break;
		}
		/* allocate stream_socket for this 'socket' */
		sock = (struct stream_socket *)malloc(sizeof(struct stream_socket));
		if (!sock)
		{
			fprintf(stderr, "%s: malloc error.\n",__func__);
			break;
		}
		memset(sock, 0, sizeof(struct stream_socket));
		sock->magic = SUSOCK_MAGIC;
		sock->max_client = max_client;
		sock->callback = callback;
		/* alocate client spaces. */
		sock->clients = (struct susock_client *)malloc(sizeof(struct susock_client) * max_client);
		if (!sock->clients)
		{
			fprintf(stderr, "%s: malloc error!\n",__func__);
			break;
		}
		/* ready to listem to this socket. */
		chmod(where.sun_path, 0777);
		listen(s, max_client);
		/* almost there, setup clients space. */
		sock->fd = s;
		for (i=0; i<max_client; i++)
		{
			sock->clients[i].magic = SUSCLI_MAGIC;
			sock->clients[i].fd = -1;
		}

		sh = (susock_handle)sock;
		s = (-1);	// clear it, so we will not release it.
		sock = NULL;// clear it, so we will not release it.
	} while (0);

	if (sock) susock_close(sock);
	if (s >= 0) close(s);
	if (dir) free(dir);
	return sh;
}
