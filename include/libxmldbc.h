/* vi: set sw=4 ts=4: */
/*
 *	libxmldbc.h
 *
 *	Copyright (C) 2004-2009 by Alpha Networks, Inc.
 *	Created by David Hsieh <david_hsieh@alphanetworks.com>
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

#ifndef __LIBXMLDBC_HEADER__
#define __LIBXMLDBC_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CMD_LEN	1024

typedef unsigned long flag_t;
typedef const char * sock_t;

int		lxmldbc_run_shell(char * buf, int size, const char * format, ...);
int		lxmldbc_system(const char * format, ...);
char *	lxmldbc_eatwhite(char * string);
char * 	lxmldbc_reatwhite(char * ptr);

int		xmldbc_get(		sock_t sn, flag_t f, const char * node, FILE * out);
ssize_t	xmldbc_get_wb(	sock_t sn, flag_t f, const char * node, char * buff, size_t size);
int		xmldbc_ephp(	sock_t sn, flag_t f, const char * file, FILE * out);
ssize_t	xmldbc_ephp_wb(	sock_t sn, flag_t f, const char * file, char * buff, size_t size);

int		xmldbc_set(		sock_t sn, flag_t f, const char * node, const char * value);
int		xmldbc_setext(	sock_t sn, flag_t f, const char * node, const char * cmd);
int		xmldbc_del(		sock_t sk, flag_t f, const char * node);
int		xmldbc_reload(	sock_t sn, flag_t f, const char * file);
int		xmldbc_patch(	sock_t sn, flag_t f, const char * file);
int		xmldbc_read(	sock_t sn, flag_t f, const char * file);
int		xmldbc_dump(	sock_t sn, flag_t f, const char * file);
int		xmldbc_write(	sock_t sn, flag_t f, const char * node, FILE * out);
int		xmldbc_timer(	sock_t sn, flag_t f, const char * cmd);
int		xmldbc_killtimer(sock_t sn,flag_t f, const char * tag);

#ifdef __cplusplus
}
#endif
#endif
