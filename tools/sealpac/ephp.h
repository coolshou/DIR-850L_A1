/* vi: set sw=4 ts=4: */
/*
 * epnp.h
 *
 *  An embeded php implementation for xmldb.
 *
 *  Created by David Hsieh (david_hsieh@alphanetworks.com)
 */

#ifndef __EPHP_HEADER_FILE__
#define __EPHP_HEADER_FILE__

#include <stdio.h>
#include <dlist.h>
#include <strobj.h>

typedef unsigned int fl_t;
typedef void * anchor_t;

/* The macro for debugging */
#ifdef DEBUG_EPHP
#define EPHPDBG(x)	x
#else
#define EPHPDBG(x)
#endif

/******************************************************************/

#define SYNERR			"SYNTAX ERROR"
#define INTERR			"INTERNAL ERROR"
#define MEMERR			"memory allocation failed !"
#define NOTOKEN			"unable to read token !"
#define QUOTERR			"should not see quotation mark here !"
#define NOQUOTERR		"should see quotation mark here !"
#define INVALID_TOKEN	"got an invalid token [%s]"

/*****************************************************************/

#define RET_ERROR		(-1)
#define RET_SUCCESS		0
#define RET_EXIT		1
#define RET_BREAK		2
#define RET_RETURN		3
#define RET_CONTINUE	4

#define FL_NORMAL		0x0000
#define FL_FUNC			0x0001
#define FL_WHILE		0x0002
#define FL_IF			0x0003
#define FL_FOREACH		0x0004

/*****************************************************************/

typedef struct func_type	func_t;
typedef struct bifunc_type	bifunc_t;
typedef struct ephp_instance ephp_t;
typedef int (*func_handler)(ephp_t * php, fl_t flags, dlist_t * ns, strobj_t out, func_t * param);

struct func_type
{
	char *					fname;	/* function name */
	func_handler			func;	/* function handler */
	struct dlist_head		args;	/* list of arg_t */
};

struct bifunc_type
{
	char *			name;	/* name of the function */
	size_t			argc;	/* numbers of arguments */
	func_handler	func;	/* handler */
};

struct ephp_instance
{
	int			outfd;
	xstream_t	fd;
	const char *filename;
	anchor_t	anchor;
	dlist_t		usrfunc;
	dlist_t		ns_global;
	dlist_t		ns_server;
	dlist_t		ns_get;
	dlist_t		ns_post;
	dlist_t		ns_env;
	dlist_t		ns_files;
	dlist_t		ns_filetypes;
};

/*****************************************************************/
/* built-in function table */
extern bifunc_t bi_functions[];
extern bifunc_t bi_ipv4func[];
extern bifunc_t bi_ipv6func[];
extern bifunc_t bi_strfunc[];

#endif
