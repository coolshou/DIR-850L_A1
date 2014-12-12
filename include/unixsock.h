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

#ifndef __USOCK_HEADER__
#define __USOCK_HEADER__

typedef void * usock_handle;

#ifdef __cplusplus
extern "C" {
#endif
/***********************************************************************/

int usock_fd(usock_handle usock);
usock_handle usock_open(int server, const char * name);
void usock_close(usock_handle usock);
int usock_send(usock_handle usock, const void * buf, unsigned int len, int flags);
int usock_recv(usock_handle usock, void * buf, unsigned int len, int flags);
int usock_recv_timed(usock_handle usock, void * buf, unsigned int len, int flags, int timeout);

/***********************************************************************/
#ifdef __cplusplus
}
#endif

#endif	// #ifndef __USOCK_HEADER__
