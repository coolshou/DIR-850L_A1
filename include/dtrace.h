/* vi: set sw=4 ts=4: */
/*
 *	Copyright (C) 2004-2009 by Alpha Networks, Inc.
 *	Created by David Hsieh <david_hsieh@alphanetworks.com>
 *
 *	A debug/trace helper module.
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

#ifndef __DTRACE_HEADER_FILE__
#define __DTRACE_HEADER_FILE__

#include <stdio.h>

/* debug level definitions */
#define DBG_ALL		0
#define DBG_DEBUG	10
#define DBG_INFO	20
#define DBG_WARN	30
#define DBG_ERROR	40
#define DBG_FATAL	50
#define DBG_NONE	100

#ifndef DBG_DEFAULT
#define DBG_DEFAULT	DBG_ERROR
#endif

/* MACROs use for debug/trace/ */
#ifndef DDEBUG

#define dtrace(x, args...)		do {} while(0)	/* output the debug message. */
#define dassert(x)				do {} while(0)	/* well, my assert functoin */
#define d_output_file(f)		do {} while(0)	/* where the output message goes, default is stderr. */
#define d_output_file_arg(f,arg) do {} while(0)	/* where the output message goes, default is stderr. */
#define d_dbg_level(l)			do {} while(0)	/* assign the debug level. */

#define d_dbg(x, args...)		do {} while(0)
#define d_info(x, args...)		do {} while(0)
#define d_warn(x, args...)		do {} while(0)
#define d_error(x, args...)		do {} while(0)
#define d_fatal(x, args...)		do {} while(0)
#define d_die(x, args...)		do { exit(9); } while(0)

#else

#define dtrace(x, args...)		__dtrace(DBG_ALL, x, ##args)
#define dassert(exp)			(void)((exp) || (__dassert(#exp,__FILE__,__LINE__),0))
#define d_output_file(f)		__set_output_file(f)
#define d_output_file_arg(f,arg) __set_output_file_arg(f, arg)
#define d_dbg_level(l)			__set_dbg_level(l)

#define d_dbg(x, args...)		__dtrace(DBG_DEBUG, x, ##args)
#define d_info(x, args...)		__dtrace(DBG_INFO, x, ##args)
#define d_warn(x, args...)		__dtrace(DBG_WARN, x, ##args)
#define d_error(x, args...)		__dtrace(DBG_ERROR, x, ##args)
#define d_fatal(x, args...)		__dtrace(DBG_FATAL, x, ##args)
#define d_die(x, args...)		do { \
	__dtrace(DBG_FATAL, "DIE, line: %d @ file: %s\n",__LINE__,__FILE__); \
	__dtrace(DBG_FATAL, x, ##args); \
	exit(9); \
	} while(0)

#endif /* end of #ifdef DDEBUG */


/***********************************************************************/

#ifdef DDEBUG
#ifdef __cplusplus
extern "C" {
#endif
/*
 * exported function from dtrac.c
 *
 * DO NOT call the following exported function directly.
 * Use the macros above instead.
 */
void	__dtrace(int level, const char * format, ...);
void	__dassert(char * exp, char * file, int line);
FILE *	__set_output_file(const char * fname);
FILE *	__set_output_file_arg(const char * fname, const char * arg);
int		__set_dbg_level(int level);

#ifdef __cplusplus
}
#endif

#endif

#endif /* end of #ifndef __DTRACE_HEADER_FILE__ */
