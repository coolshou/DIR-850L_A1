/* vi: set sw=4 ts=4: */
/*
 *	ephp.h
 *
 *	An embeded php implementation for xmldb.
 *
 *	Created by David Hsieh (david_hsieh@alphanetworks.com)
 */

#ifndef __EPHP_HEADER__
#define __EPHP_HEADER__

#include "dlist.h"

/* variable structure */
typedef struct _var_t var_t;
struct _var_t
{
	struct _var_t *	next;
	char *			name;
	char *			value;
};

/****************************************************************************/
/* string list type. */

typedef enum _sltype sl_type;
enum _sltype { ST_STR, ST_VAR, ST_QUERY, ST_QUERYJS };

typedef struct strlist_type strlist_t;
struct strlist_type
{
	struct dlist_head		link;
	sl_type					type;	/* ST type */
	union
	{
		char *				str;	/* ST_STR, ST_VAR */
		struct dlist_head	slist;	/* ST_QUERY, ST_QUERYJS */
	}						u;
	char					op;
};

/* match in map() */
typedef struct _match_type match_t;
struct _match_type
{
	struct _match_type *	next;
	struct dlist_head		pattern;
	struct dlist_head		output;
};

/* if() condition */
typedef enum _if_op_type ifop_t;
enum _if_op_type
{
	IFOP_EQUAL,
	IFOP_NOTEQUAL,
	IFOP_GREATERTHAN,
	IFOP_LESSTHAN,
	IFOP_GREATEREQUAL,
	IFOP_LESSEQUAL
};
typedef enum _if_next_type ifnext_t;
enum _if_next_type
{
	IFNEXT_END,
	IFNEXT_AND,
	IFNEXT_OR
};
typedef struct _cond_type cond_t;
struct _cond_type
{
	struct dlist_head		link;
	struct dlist_head		operand1;
	struct dlist_head		operand2;
	ifop_t					op;
	ifnext_t				next;
};

/****************************************************************************/

typedef struct statement_block sblock_t;

/* require function
 *
 * syntax:
 *		require("/www/somefile-ppp.php");
 *		require(/www/somefile-ppp.php);
 *		require("/www/somefile-".$var.".php");
 */
typedef struct require_type require_t;
struct require_type
{
	struct dlist_head file;
};

/* anchor function
 *
 * syntax:
 *		anchor("/wan/rg/inf:1/static");
 *		anchor(/wan/rg/inf:1/static);
 *		anchor("/wan/rg/inf:".$num."/static");
 */
typedef struct anchor_type anchor_t;
struct anchor_type
{
	struct dlist_head node;
};

/* query function
 *
 * syntax:
 *		query("/wan/rg/inf:1/static/ipaddress");
 *		query(/wan/rg/inf:1/static/ipaddress);
 *		query("/wan/rg/inf:".$num."/static/ipaddress");
 */
typedef struct query_type query_t;
struct query_type
{
	struct dlist_head node;
};

/* del function
 *
 * syntax:
 *		del("/wan/rg/inf:1/static/ipaddress");
 *		del(/wan/rg/inf:1/static/ipaddress);
 *		del("/wan/rg/inf:".$num."/static/ipaddress");
 */
typedef struct del_type del_t;
struct del_type
{
	struct dlist_head node;
};

/* set function
 *
 * syntax:
 *		set("/wan/rg/inf:1/static/ipaddress", "2.1.1.253");
 *		set(/wan/rg/inf:1/static/ipaddress, 2.1.1.253);
 *		set("/wan/rg/inf:".$num."/static/ipaddress", "2.1.1.".$num);
 */
typedef struct set_type set_t;
struct set_type
{
	struct dlist_head node;
	struct dlist_head value;
};

/* echo function
 *
 * syntax:
 *		echo "prefix string".$variable."\n";
 *		echo "$not is not a variable\n";
 */
typedef struct echo_type echo_t;
struct echo_type
{
	struct dlist_head string;
};

/* variable assignment
 *
 * syntax:
 *		$var = "prefix string : ".$something." ending message"."\n";
 *		$var = $var + 1;
 *		$var += $var1 + 1;
 *		$var++;
 *		$var--;
 *		
 */
typedef enum _assign_op_type aop_t;
enum _assign_op_type { AOP_ASSIGN, AOP_ADD, AOP_SUB, AOP_INCREASE, AOP_DECREASE };
typedef struct assign_type assign_t;
struct assign_type
{
	aop_t				op;
	char * 				variable;
	struct dlist_head	value;
};

/* map function
 *
 * syntax:
 *		map(/test/node/entry:1/mode, 0,"Mode 1", 1,"Mode 2", *,"Unknown Mode");
 *		map("/test/node/entry:".$num."/mode", $mode, "Enable", *,"Disabled");
 */
typedef struct map_type map_t;
struct map_type
{
	struct dlist_head	node;
	match_t *			default_output;
	match_t *			match;
};

/* inclog function
 *
 * syntax:
 *		inclog("Field 0 [%0], Field 1 [%1], Field 2 [%2]", /var/log/message);
 */
typedef struct inclog_type inclog_t;
struct inclog_type
{
	char *				format;
	char *				file;
};

/* 'if' block
 *
 * syntax:
 *		if ($value+1 < 10)				{ ... }
 *		else if ($value == "0")			{ ... }
 *		else if (query(...) == "0")		{ ... }
 *		else							{ ... }
 *
 * 		if ($mode == 0)					{ ... }
 * 		if ($mode != 0)					{ ... }
 * 		if ($mode <= 0)					{ ... }
 * 		if ($mode >= 0)					{ ... }
 * 		if ($mode < 0)					{ ... }
 * 		if ($mode > 0)					{ ... }
 *
 *		if ($mode == 0 || $value == 0)	{ ... }
 *		if ($mode == 0 && $value == 0)	{ ... }
 */
typedef struct if_type if_t;
struct if_type
{
	struct dlist_head	condition;
	struct dlist_head	_true;
	struct dlist_head	_false;
};

/* 'for' block */
typedef struct for_type for_t;
struct for_type
{
	struct dlist_head	node;
	struct dlist_head	statement;
};

typedef enum _sbtype	sb_type;
enum _sbtype
{
	SB_REQUIRE,
	SB_ANCHOR,
	SB_QUERY,
	SB_QUERYJS,
	SB_DEL,
	SB_SET,
	SB_ECHO,
	SB_ASSIGN,
	SB_MAP,
	SB_INCLOG,
	SB_IF,
	SB_FOR,
	SB_EXIT
};

struct statement_block
{
	struct dlist_head	link;
	sb_type				type;
	union
	{
		require_t		_require;
		anchor_t		_anchor;
		query_t			_query;
		del_t			_del;
		set_t			_set;
		echo_t			_echo;
		assign_t		_assign;
		map_t			_map;
		inclog_t		_inclog;
		if_t			_if;
		for_t			_for;
	}					u;
};

/* function prototype */
int xmldb_ephp(int fd, char * file, unsigned long flags);

#endif
