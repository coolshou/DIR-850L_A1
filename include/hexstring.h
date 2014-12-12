/* vi: set sw=4 ts=4: */
/*
 *	hexstring.c
 *
 *	This file contains some useful function to manipulate HEX-string.
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

#ifndef __HEXSTRING_HEADER__
#define __HEXSTRING_HEADER__

size_t			read_hexstring(unsigned char * buf, size_t size, const char * string);
const char *	print_macaddr(const unsigned char * macaddr);
const char *	print_uuid(const unsigned char * uuid);

void dump_hex(FILE * out, const char * start, const char * end, char delimiter,
			 const unsigned char * bin, size_t size);

#endif	/* endif ifdef __HEXSTRING_HEADER__ */
