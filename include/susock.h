/* vi: set sw=4 ts=4: */
/*
 *	Stream Unix Socket
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
#ifndef __STREAM_UNIX_SOCKET_HEADER__
#define __STREAM_UNIX_SOCKET_HEADER__

#include <unistd.h>

typedef void * susock_handle;
typedef void (*susock_callback)(susock_handle sock, susock_handle client);

int				susock_server_sloop_handler(int sock, void * param, void * data);
susock_handle	susock_server_open(const char * name, size_t max_client, susock_callback callback);
susock_handle	susock_open(const char * name);
int				susock_close(susock_handle sock);
ssize_t			susock_send(susock_handle sock, const void * buf, size_t len, int flags);
ssize_t			susock_recv(susock_handle sock, void * buf, size_t len, int flags);
int				susock_fd(void * client);

#endif
