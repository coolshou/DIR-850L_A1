/* vi: set sw=4 ts=4: */
/*
 *	xstream.h
 *
 *	extensible stream module.
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

#ifndef __XSTREAM_HEADER_FILE__
#define __XSTREAM_HEADER_FILE__

//typedef enum { XSTYPE_UNKNOWN=0, XSTYPE_FILE, XSTYPE_BUFFER, XSTYPE_FILDES } xstype_t;
typedef enum { XSTYPE_UNKNOWN=0, XSTYPE_FILE, XSTYPE_BUFFER, XSTYPE_FDOPEN } xstype_t;
typedef void * xstream_t;

xstream_t xs_fopen(const char * file, const char * mode);
xstream_t xs_fdopen(int fd, const char * mode, int max);
xstream_t xs_bopen(void * buff, size_t size);

int xs_close(xstream_t fd);

int xs_getc(xstream_t fd);
int xs_ungetc(int c, xstream_t fd);
int xs_ungets(const char * s, xstream_t fd);

#endif
