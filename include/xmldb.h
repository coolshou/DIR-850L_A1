/* vi: set sw=4 ts=4:*/
/*
 *	xmldb.h
 *
 *	Global definition for xmldb_v2.
 *	Created by David Hsieh <david_hsieh@alphanetworks.com>
 *	Copyright (C) 2004-2008 by Alpha Networks, Inc.
 *
 */

#ifndef __XMLDB_HEADER_FILE__
#define __XMLDB_HEADER_FILE__

/***********************************/
/* the default unixsocket of xmldb */
/***********************************/
#ifndef CONFIG_XMLDB_DEFAULT_UNIXSOCK
#define XMLDB_DEFAULT_UNIXSOCK	"/var/run/xmldb_sock"
#else
#define XMLDB_DEFAULT_UNIXSOCK	CONFIG_XMLDB_DEFAULT_UNIXSOCK
#endif

/*
 * for backward compatible, we also define RGDB_DEFAULT_UNIXSOCK.
 * For new develop program, use XMLDB_DEFAULT_UNIXSOCK instead.
 */
#define RGDB_DEFAULT_UNIXSOCK	XMLDB_DEFAULT_UNIXSOCK

/* action type */
typedef unsigned short action_t;
#define XMLDB_NONE		0	/* no action */
#define XMLDB_GET		1	/* get value from node */
#define XMLDB_SET		2	/* set value to node */
#define XMLDB_DEL		3	/* delete node */
#define XMLDB_RELOAD	4	/* reload database from file */
#define XMLDB_DUMP		5	/* dump database to file */
#define XMLDB_READ		6	/* read database from file */
#define XMLDB_PATCH	7	/* read database from file */
#define XMLDB_EPHP		10	/* embeded php */
#define XMLDB_SETEXT	11	/* set extended get/set command */
#define XMLDB_TIMER		12	/* schedule a timer */
#define XMLDB_KILLTIMER	13	/* kill a timer */
#define XMLDB_WRITE		14	/* write the subset of database to file */

/*
 * for backward compatible, we also define RGDB_XXX actions.
 * For new develop program, use XMLDB_XXX actions instead.
 */
#define RGDB_NONE		XMLDB_NONE
#define RGDB_GET		XMLDB_GET
#define RGDB_SET		XMLDB_SET
#define RGDB_DEL		XMLDB_DEL
#define RGDB_RELOAD		XMLDB_RELOAD
#define RGDB_DUMP		XMLDB_DUMP
#define RGDB_WRITE		XMLDB_WRITE
#define RGDB_PATCH		6
#define RGDB_PATCH_BUFF	7
#define RGDB_SET_NOTIFY	8
#define RGDB_EPHP		XMLDB_EPHP

/* command struct */
typedef struct _rgdb_ipc_t rgdb_ipc_t;
struct _rgdb_ipc_t
{
	action_t		action;
	unsigned short	length;
	unsigned long	flags;
	int				retcode;
} __attribute__ ((packed));

/* The flag is for internal used only since XMLDBv3. */

/* RGDB flags */
#define RGDB_ESCAPE_JS  0x00000001

/* skip node 'proc' when dump, and keep 'proc' when load xml. */
#define RGDB_SKIP_PROC  0x00010000

/* flags for string convertor */
#define SCONV_FLAGS_MASK		0xffffff00	/* flags part */
#define SCONV_FLAGS_VALUE		0x000000ff	/* value part */

#define SCONV_FLAGS_ESCAPE		0x00000100	/* Add escape character */
#define SCONV_FLAGS_DATE		0x00000200	/* Convert to date time format */

/* Value for SCONV_FLAGS_ESCAPE */
#define SCONV_ESCAPE_JS			0x1			/* Add escape character for Javascript. */
#define SCONV_ESCAPE_SH			0x2			/* Add escape character for shell script. */
#define SCONV_ESCAPE_SC			0x3			/* Transfer special characters for HTML. */
#define SCONV_ESCAPE_XML		0x4			/* Transfer special characters for XML. */

/* Value for EPHP_FLAGS_DATE */
#define SCONV_DATE_ISO8859		0x1			/* ASCII/ISO 8859-1 ex. 2005-05-31T17:23:18 */

#endif
