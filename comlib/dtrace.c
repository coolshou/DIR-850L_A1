/* vi: set sw=4 ts=4: */
/*
 * dtrace.c
 *
 *	A debug / trace helper module.
 *
 *	Created by David Hsieh <david_hsieh@alphanetworks.com>
 *	Copyright (C) 2004-2008 by Alpha Networks, Inc.
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

#ifdef DDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "dtrace.h"

/***********************************************************************/
static int ddbg_level = DBG_DEFAULT;
static FILE * out_fd = NULL;

/***********************************************************************/
void __dtrace(int level, const char * format, ...)
{
	va_list marker;

	if (ddbg_level <= level)
	{
		va_start(marker, format);
		vfprintf(out_fd ? out_fd : stdout, format, marker);
		va_end(marker);
	}
}

void __dassert(char * exp, char * file, int line)
{
	__dtrace(DBG_ALL, "DASSERT: file: %s, line: %d\n", file, line);
	__dtrace(DBG_ALL, "\t%s\n", exp);
	abort();
}

FILE * __set_output_file(const char * fname)
{
	if (out_fd) fclose(out_fd);
	out_fd = NULL;
	if (fname) out_fd = fopen(fname, "w");
	return out_fd;
}

FILE * __set_output_file_arg(const char * fname, const char * arg)
{
	if (out_fd) fclose(out_fd);
	out_fd = NULL;
	if (fname) out_fd = fopen(fname, arg);
	return out_fd;
}

int __set_dbg_level(int level)
{
	ddbg_level = level;
	return ddbg_level;
}

#ifdef TEST_DTRACE

int main(int argc, char * argv[])
{
	dtrace("Hello world !\n");
	dtrace("Hello: %d %s 0x%x\n", 12, "test", 12);
	return 0;
}

#endif

#endif	/* end of #ifdef DDEBUG */
