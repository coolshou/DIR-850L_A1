/* vi: set sw=4 ts=4: */
/*
 *	Select loop
 *
 *	Created by David Hsieh <david_hsieh@alphanetworks.com>
 *	Copyright (C) 2004-2009 by Alpha Networks, Inc.
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

#ifndef __SLOOP_HEADER_H__
#define __SLOOP_HEADER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* default define */
#ifndef DEBUG_SLOOP
#define DEBUG_SLOOP			0
#endif
#ifndef DEBUG_SLOOP_DUMP
#define DEBUG_SLOOP_DUMP	1
#endif

#ifndef MAX_SLOOP_SOCKET
#define MAX_SLOOP_SOCKET	128
#endif
#ifndef MAX_SLOOP_SIGNAL
#define MAX_SLOOP_SIGNAL	16
#endif
#ifndef MAX_SLOOP_TIMEOUT
#define MAX_SLOOP_TIMEOUT	128
#endif

typedef void * sloop_handle;

typedef int (*sloop_socket_handler)(int sock, void * param, void * sloop_data);
typedef int (*sloop_signal_handler)(int sig, void * param, void * sloop_data);
typedef void (*sloop_timeout_handler)(void * param, void * sloop_data);

/* export functoin prototype */
long sloop_uptime(void);
void sloop_init(void * sloop_data);
sloop_handle sloop_register_read_sock(int sock, sloop_socket_handler handler, void * param);
sloop_handle sloop_register_write_sock(int sock, sloop_socket_handler handler, void * param);
sloop_handle sloop_register_signal(int sig, sloop_signal_handler handler, void * param);
sloop_handle sloop_register_timeout(unsigned int secs, unsigned int usecs, sloop_timeout_handler handler, void * param);
void sloop_cancel_read_sock(sloop_handle handle);
void sloop_cancel_write_sock(sloop_handle handle);
void sloop_cancel_signal(sloop_handle handle);
void sloop_cancel_timeout(sloop_handle handle);
void sloop_run(void);
void sloop_terminate(void);

#if DEBUG_SLOOP_DUMP
void sloop_dump_readers(void);
void sloop_dump_writers(void);
void sloop_dump_timeout(void);
void sloop_dump_signals(void);
void sloop_dump(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
