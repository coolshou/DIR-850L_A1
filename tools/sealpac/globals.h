/* vi: set sw=4 ts=4: */
/*
 * globals.h
 *
 *	v0.10	- The 1st.
 *	v0.20	- Add I18N() support. <david_hsieh@alphanetworks.com>
 */

#ifndef __GLOBALS_HEADER__
#define __GLOBALS_HEADER__

#include <mem_helper.h>

#define PROGNAME	"sealpac"
#define VERSION		"0.20"

#if 1
/* memory helper functions */
#define MALLOC	xmalloc
#define FREE	xfree
#define REALLOC	xrealloc
#define STRDUP	xstrdup
#else
#define MALLOC	malloc
#define FREE	free
#define REALLOC	realloc
#define STRDUP	strdup
#endif

#define IS_WHITE(c)	(( (c)==' ' || (c)=='\t' || (c)=='\r' || (c)=='\n' ) ? 1 : 0 )

/* psedo xmlnode type */
typedef void * xmlnode_t;

/* functions exported from sealpac.c */
int client_puts(const char * str, int fd);
int client_printf(int fd, const char * format, ...);
void sealpac_puts(const char * string);

/* functions exported from ephp.c */
int xmldb_ephp(int fd, const char * file, unsigned long flags);

#endif
