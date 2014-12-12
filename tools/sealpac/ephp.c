/* vi: set sw=4 ts=4: */
/*
 * ephp.c
 *
 *  An embeded php implementation for xmldb.
 *
 *  Created by David Hsieh (david_hsieh@alphanetworks.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <time.h>

#include <dtrace.h>
#include <libxmldbc.h>
#include <xmldb.h>
#include <xstream.h>

#include "globals.h"
#include "ephp.h"

/******************************************************************/
/* name space & variables */

/* variable structure */
typedef struct var_type var_t;
struct var_type
{
	dlist_t list;
	char * name;
	strobj_t * value;
};

/* the root name space */
DLIST_HEAD(g_nspace);

DLIST_HEAD(g_ns_server);
DLIST_HEAD(g_ns_get);
DLIST_HEAD(g_ns_post);
DLIST_HEAD(g_ns_env);
DLIST_HEAD(g_ns_files);
DLIST_HEAD(g_ns_filetypes);

/******************************************************************/
/* token type. */

typedef struct cond_type		cond_t;
typedef struct statement_block	sblock_t;
typedef struct assign_type		assign_t;
typedef struct if_type			if_t;
//typedef struct func_type		func_t;	/* move to ephp.h */
typedef struct while_type		while_t;
typedef struct foreach_type		foreach_t;
typedef struct operand_type		operand_t;
typedef struct arg_type			arg_t;

typedef enum _op_type			op_type;
typedef enum if_op_type			ifop_type;
typedef enum if_next_type		ifnext_type;
typedef enum assign_op_type		aop_type;
typedef enum statement_type		sb_type;

/******************************************************************/

struct var_name
{
	char *					name;
	struct dlist_head *		nspace;
};

/* variable assignment
 *
 * syntax:
 *      $var = "prefix string : ".$something." ending message"."\n";
 *      $var = $var + 1;
 *      $var += $var1 + 1;
 *      $var++;
 *      $var--;
 */
enum assign_op_type { AOP_ASSIGN, AOP_ADD, AOP_SUB, AOP_INCREASE, AOP_DECREASE };
struct assign_type
{
	aop_type				op;
	struct var_name			var;
	struct dlist_head		operand;	/* list of operand_t */
};

/* 'if' block
 *
 * syntax:
 *      if ($value+1 < 10)              { ... }
 *      else if ($value == "0")         { ... }
 *      else if (query(...) == "0")     { ... }
 *      else                            { ... }
 *
 *      if ($mode == 0)                 { ... }
 *      if ($mode != 0)                 { ... }
 *      if ($mode <= 0)                 { ... }
 *      if ($mode >= 0)                 { ... }
 *      if ($mode < 0)                  { ... }
 *      if ($mode > 0)                  { ... }
 *
 *      if ($mode == 0 || $value == 0)  { ... }
 *      if ($mode == 0 && $value == 0)  { ... }
 */
struct if_type
{
	struct dlist_head		condition;	/* list of cond_t */
	struct dlist_head		_true, _false; /* list of sblock_t */
};

/* 'function' block
 *
 * syntax:
 *		get("/wan/inf:1/mode");
 *		set("/wan/inf:1/mode", "1");
 *		user_defined_func($test);
 */
#if 0	/* move to ephp.h */
struct func_type
{
	char *					fname;		/* function name */
	func_handler			func;		/* function handler */
	struct dlist_head		args;		/* list of arg_t */
};
#endif

/* 'while' block
 *
 * The condition part is same as 'if' block.
 *
 * syntax:
 *      while ($value+1 < 10) { ... }
 */
struct while_type
{
	struct dlist_head		condition;	/* list of cond_t */
	struct dlist_head		statement;	/* list of sblock_t */
};
struct foreach_type
{
	struct dlist_head		operand;	/* list of operand_t */
	struct dlist_head		statement;	/* list of sblock_t */
};

/* statement list */
enum statement_type { SB_ASSIGN, SB_IF, SB_FUNC, SB_WHILE, SB_FOREACH, SB_ECHO, SB_RETURN, SB_BREAK, SB_EXIT, SB_CONTINUE };
struct statement_block
{
	struct dlist_head		list;
	sb_type					type;
	union
	{
		struct dlist_head	_operand;
		assign_t			_assign;
		if_t				_if;
		func_t				_func;
		while_t				_while;
		foreach_t			_foreach;
	} u;
};

/******************************************************************/
/* if() condition */
enum if_next_type { IFNEXT_END, IFNEXT_AND, IFNEXT_OR };
enum if_op_type
{
	IFOP_EQUAL, IFOP_NOTEQUAL, IFOP_GREATERTHAN,
	IFOP_LESSTHAN, IFOP_GREATEREQUAL, IFOP_LESSEQUAL
};

struct cond_type
{
	struct dlist_head		list;
	struct dlist_head		operand[2];	/* list of operand_t */
	ifop_type				op;
	ifnext_type				next;
};

/* operand list */
enum _op_type { OTYPE_STR, OTYPE_VAR, OTYPE_FUNC };
struct operand_type
{
	struct dlist_head		list;
	op_type					type;
	union
	{
		char *				string;
		struct var_name		var;
		struct func_type	func;
	} u;
	char op;
};

/* argument list (for function) */
struct arg_type
{
	struct dlist_head		list;
	struct dlist_head		operand;	/* list of operand_t */
};

/******************************************************************/
static void delete_cond_type(cond_t * obj);
static void delete_operand_type(operand_t * obj);
static void delete_arg_type(arg_t * obj);
static void delete_sblock(sblock_t * obj);

static void destroy_cond_list(struct dlist_head * head);
static void destroy_operand_list(struct dlist_head * head);
static void destroy_arg_list(struct dlist_head * head);
static void destroy_sblist(struct dlist_head * head);

static void dump_cond_list(struct dlist_head * head);
static void dump_arg_list(struct dlist_head * head);
static void dump_operand_list(struct dlist_head * head);
static void dump_sblist(struct dlist_head * head, int depth);

/* -------------------------- */
static void delete_cond_type(cond_t * obj)
{
	dassert(obj);
	destroy_operand_list(&obj->operand[0]);
	destroy_operand_list(&obj->operand[1]);
	FREE(obj);
}

static void delete_operand_type(operand_t * obj)
{
	switch (obj->type)
	{
	case OTYPE_STR:
		if (obj->u.string) FREE(obj->u.string);
		break;
	case OTYPE_VAR:
		if (obj->u.var.name) FREE(obj->u.var.name);
		break;
	case OTYPE_FUNC:
		if (obj->u.func.fname) FREE(obj->u.func.fname);
		destroy_arg_list(&obj->u.func.args);
		break;
	}
	FREE(obj);
}

static void delete_arg_type(arg_t * obj)
{
	destroy_operand_list(&obj->operand);
	FREE(obj);
}

static void delete_sblock(sblock_t * obj)
{
	switch (obj->type)
	{
	case SB_ASSIGN:
		if (obj->u._assign.var.name) FREE(obj->u._assign.var.name);
		destroy_operand_list(&obj->u._assign.operand);
		break;

	case SB_IF:
		destroy_cond_list(&obj->u._if.condition);
		destroy_sblist(&obj->u._if._true);
		destroy_sblist(&obj->u._if._false);
		break;

	case SB_FUNC:
		if (obj->u._func.fname) FREE(obj->u._func.fname);
		destroy_arg_list(&obj->u._func.args);
		break;

	case SB_WHILE:
		destroy_cond_list(&obj->u._while.condition);
		destroy_sblist(&obj->u._while.statement);
		break;

	case SB_FOREACH:
		destroy_operand_list(&obj->u._foreach.operand);
		destroy_sblist(&obj->u._foreach.statement);
		break;

	case SB_ECHO:
	case SB_RETURN:
		destroy_operand_list(&obj->u._operand);
		break;

	case SB_BREAK:
	case SB_EXIT:
	case SB_CONTINUE:
		break;
	}
	FREE(obj);
}

/* -------------------------- */
static void destroy_cond_list(struct dlist_head * head)
{
	struct dlist_head * entry;
	while (!dlist_empty(head))
	{
		entry = head->next; dlist_del(entry);
		delete_cond_type(dlist_entry(entry, cond_t, list));
	}
}
static void destroy_operand_list(struct dlist_head * head)
{
	struct dlist_head * entry;
	while (!dlist_empty(head))
	{
		entry = head->next; dlist_del(entry);
		delete_operand_type(dlist_entry(entry, operand_t, list));
	}
}
static void destroy_arg_list(struct dlist_head * head)
{
	struct dlist_head * entry;
	while (!dlist_empty(head))
	{
		entry = head->next; dlist_del(entry);
		delete_arg_type(dlist_entry(entry, arg_t, list));
	}
}
static void destroy_sblist(struct dlist_head * head)
{
	struct dlist_head * entry;
	while (!dlist_empty(head))
	{
		entry = head->next; dlist_del(entry);
		delete_sblock(dlist_entry(entry, sblock_t, list));
	}
}
/* -------------------------- */

static FILE * dfd = NULL;

static int duprint(const char * format, ...)
{
	int ret=0;
	va_list marker;

	if (dfd)
	{
		va_start(marker, format);
		ret = vfprintf(dfd, format, marker);
		va_end(marker);
	}
	return ret;
}

static void dump_prefix(int depth)
{
	int i;
	if (dfd) for (i=0; i<depth; i++) duprint("    ");
}
static void dump_cond_list(struct dlist_head * head)
{
	struct dlist_head * entry;
	cond_t * cond;

	duprint("(");
	for (entry = head->next; entry != head; entry = entry->next)
	{
		cond = dlist_entry(entry, cond_t, list);

		dump_operand_list(&cond->operand[0]);
		switch (cond->op)
		{
		case IFOP_EQUAL:		duprint(" == "); break;
		case IFOP_NOTEQUAL:		duprint(" != "); break;
		case IFOP_GREATERTHAN:	duprint(" > "); break;
		case IFOP_LESSTHAN:		duprint(" < "); break;
		case IFOP_GREATEREQUAL:	duprint(" >= "); break;
		case IFOP_LESSEQUAL:	duprint(" <= "); break;
		}
		dump_operand_list(&cond->operand[1]);
		switch (cond->next)
		{
		case IFNEXT_AND:	duprint(" && "); break;
		case IFNEXT_OR:		duprint(" || "); break;
		case IFNEXT_END:	break;
		}
	}
	duprint(")");
}
static void dump_arg_list(struct dlist_head * head)
{
	struct dlist_head * entry;
	arg_t * arg;

	duprint("(");
	for (entry = head->next; entry != head; entry = entry->next)
	{
		arg = dlist_entry(entry, arg_t, list);
		dump_operand_list(&arg->operand);
		if (entry->next != head) duprint(", ");
	}
	duprint(")");
}
static void dump_operand_list(struct dlist_head * head)
{
	struct dlist_head * entry;
	operand_t * op;
	const char * ptr;

	for (entry = head->next; entry != head; entry = entry->next)
	{
		op = dlist_entry(entry, operand_t, list);
		switch (op->type)
		{
		case OTYPE_STR:
			duprint("\"");
			if (op->u.string)
			{
				for (ptr = op->u.string; *ptr; ptr++)
				{
					switch (*ptr)
					{
					case '\n':	duprint("\\n"); break;
					case '\r':	duprint("\\r"); break;
					case '\t':	duprint("\\t"); break;
					default:	duprint("%c", *ptr); break;
					}
				}
			}
			duprint("\"");
			break;
		case OTYPE_VAR:
			if		(op->u.var.nspace == &g_nspace)			duprint("$_GLOBALS[\"%s\"]", op->u.var.name);
			else if	(op->u.var.nspace == &g_ns_server)		duprint("$_SERVER[\"%s\"]", op->u.var.name);
			else if	(op->u.var.nspace == &g_ns_get)			duprint("$_GET[\"%s\"]", op->u.var.name);
			else if	(op->u.var.nspace == &g_ns_post)		duprint("$_POST[\"%s\"]", op->u.var.name);
			else if	(op->u.var.nspace == &g_ns_env)			duprint("$_ENV[\"%s\"]", op->u.var.name);
			else if	(op->u.var.nspace == &g_ns_files)		duprint("$_FILES[\"%s\"]", op->u.var.name);
			else if	(op->u.var.nspace == &g_ns_filetypes)	duprint("$_FILETYPES[\"%s\"]", op->u.var.name);
			else duprint("$%s", op->u.var.name);
			break;
		case OTYPE_FUNC:
			duprint("%s", op->u.func.fname);
			dump_arg_list(&op->u.func.args);
			break;
		}
		if (op->op) duprint("%c", op->op);
	}
}
static void dump_sblist(struct dlist_head * head, int depth)
{
	struct dlist_head * entry;
	sblock_t * sbptr;
	struct dlist_head * ns;

	for (entry = head->next; entry != head; entry = entry->next)
	{
		sbptr = dlist_entry(entry, sblock_t, list);
		dump_prefix(depth);
		switch (sbptr->type)
		{
		case SB_ASSIGN:
			ns = sbptr->u._assign.var.nspace;
			if		(ns == &g_nspace)		duprint("$_GLOBALS[\"%s\"] ", sbptr->u._assign.var.name);
			else if	(ns == &g_ns_server)	duprint("$_SERVER[\"%s\"] ", sbptr->u._assign.var.name);
			else if	(ns == &g_ns_get)		duprint("$_GET[\"%s\"] ", sbptr->u._assign.var.name);
			else if	(ns == &g_ns_post)		duprint("$_POST[\"%s\"] ", sbptr->u._assign.var.name);
			else if	(ns == &g_ns_env)		duprint("$_ENV[\"%s\"] ", sbptr->u._assign.var.name);
			else if	(ns == &g_ns_files)		duprint("$_FILES[\"%s\"]", sbptr->u._assign.var.name);
			else if	(ns == &g_ns_filetypes)	duprint("$_FILETYPES[\"%s\"]", sbptr->u._assign.var.name);
			else							duprint("$%s ", sbptr->u._assign.var.name);
			switch (sbptr->u._assign.op)
			{
			case AOP_ASSIGN:	duprint("= "); break;
			case AOP_ADD:		duprint("+= "); break;
			case AOP_SUB:		duprint("-= "); break;
			case AOP_INCREASE:	duprint("++"); break;
			case AOP_DECREASE:	duprint("--"); break;
			}
			dump_operand_list(&sbptr->u._assign.operand);
			duprint(";\n");
			break;
		case SB_IF:
			duprint("if ");
			dump_cond_list(&sbptr->u._if.condition);
			duprint("\n");
			dump_prefix(depth); duprint("{\n");
			dump_sblist(&sbptr->u._if._true, depth+1);
			dump_prefix(depth); duprint("}\n");
			if (!dlist_empty(&sbptr->u._if._false))
			{
				dump_prefix(depth); duprint("else\n");
				dump_prefix(depth); duprint("{\n");
				dump_sblist(&sbptr->u._if._false, depth+1);
				dump_prefix(depth); duprint("}\n");
			}
			break;
		case SB_FUNC:
			duprint("%s", sbptr->u._func.fname);
			dump_arg_list(&sbptr->u._func.args);
			duprint(";\n");
			break;
		case SB_WHILE:
			duprint("while ");
			dump_cond_list(&sbptr->u._while.condition);
			duprint("\n");
			dump_prefix(depth); duprint("{\n");
			dump_sblist(&sbptr->u._while.statement, depth+1);
			dump_prefix(depth); duprint("}\n");
			break;
		case SB_FOREACH:
			duprint("foreach (");
			dump_operand_list(&sbptr->u._foreach.operand);
			duprint(")\n");
			dump_prefix(depth); duprint("{\n");
			dump_sblist(&sbptr->u._foreach.statement, depth+1);
			dump_prefix(depth); duprint("}\n");
			break;
		case SB_ECHO:
			duprint("echo ");
			dump_operand_list(&sbptr->u._operand);
			duprint(";\n");
			break;
		case SB_RETURN:
			duprint("return ");
			dump_operand_list(&sbptr->u._operand);
			duprint(";\n");
			break;

		case SB_BREAK:		duprint("break;\n");	break;
		case SB_EXIT:		duprint("exit;\n");		break;
		case SB_CONTINUE:	duprint("continue;\n");	break;
		}
	}
}
/******************************************************************/

typedef struct usrfunc_type	usrfunc_t;

/* user define function */
struct usrfunc_type
{
	struct dlist_head	list;
	char *				name;
	struct dlist_head	args;		/* list of operand_t */
	struct dlist_head	statement;	/* list of sblock_t */
};

/* the list of user defined functions. */
static DLIST_HEAD(g_usrfunc);

bifunc_t bi_functions[] =
{
	/*  name            argc    handler */
	{   "get",          2,      NULL },
	{	"escape",		2,		NULL },
	{   "query",        1,      NULL },
	{   "set",          2,      NULL },
	{   "add",          2,      NULL },
	{   "anchor",       1,      NULL },
	{   "setattr",      3,      NULL },
	{   "del",          1,      NULL },
	{   "fread",        2,      NULL },
	{   "fwrite",       3,      NULL },
	{   "unlink",       1,      NULL },
	{   "map",          -1,     NULL },
	{   "i18n",         -1,     NULL },
	{	"I18N",			-1,		NULL },
	{   NULL, 0, NULL },
};

bifunc_t bi_ipv4func[] =
{
	/*  name                argc    handler */
	{   "ipv4networkid",    2,      NULL },
	{   "ipv4hostid",       2,      NULL },
	{   "ipv4int2mask",     1,      NULL },
	{   "ipv4mask2int",     1,      NULL },
	{   "ipv4ip",           3,      NULL },
	{   "ipv4maxhost",      1,      NULL },
	{   NULL,               0,      NULL },
};

bifunc_t bi_ipv6func[] =
{
	/*  name    argc    handler */
	{   NULL,   0,      NULL },
};

bifunc_t bi_strfunc[] =
{
	/*  name                argc    handler */
	{   "charcodeat",       1,      NULL },
	{   "cut",              3,      NULL },
	{   "cut_count",        2,      NULL },
	{   "isalpha",          1,      NULL },
	{   "isdigit",          1,      NULL },
	{   "isempty",          1,      NULL },
	{   "isxdigit",         1,      NULL },
	{   "scut",             3,      NULL },
	{   "scut_count",       2,      NULL },
	{   "strchr",           2,      NULL },
	{   "strlen",           1,      NULL },
	{   "strtoul",          2,      NULL },
	{   "tolower",          1,      NULL },
	{   "toupper",          1,      NULL },
	{   NULL,               0,      NULL },
};

static bifunc_t * find_built_in_func(const char * name)
{
	int i;
	/* basic set */
	for (i=0; bi_functions[i].name; i++)
		if (strcmp(name, bi_functions[i].name)==0)
			return &bi_functions[i];
	/* IPv4 set */
	for (i=0; bi_ipv4func[i].name; i++)
		if (strcmp(name, bi_ipv4func[i].name)==0)
			return &bi_ipv4func[i];
	/* IPv6 set */
	for (i=0; bi_ipv6func[i].name; i++)
		if (strcmp(name, bi_ipv6func[i].name)==0)
			return &bi_ipv6func[i];
	/* string func set */
	for (i=0; bi_strfunc[i].name; i++)
		if (strcmp(name, bi_strfunc[i].name)==0)
			return &bi_strfunc[i];
	EPHPDBG(d_dbg("%s: function [%s] not found !\n",__func__,name));
	return NULL;
}

static usrfunc_t * find_user_func(const char * name)
{
	struct dlist_head * entry;
	usrfunc_t * func;

	for (entry = g_usrfunc.next; entry != &g_usrfunc; entry = entry->next)
	{
		func = dlist_entry(entry, usrfunc_t, list);
		if (strcmp(name, func->name)==0) return func;
	}
	EPHPDBG(d_dbg("%s: function [%s] not found !\n",__func__, name));
	return NULL;
}

static void delete_usrfunc_type(struct usrfunc_type * ufunc)
{
	if (ufunc)
	{
		if (ufunc->name) FREE(ufunc->name);
		destroy_operand_list(&ufunc->args);
		destroy_sblist(&ufunc->statement);
		FREE(ufunc);
	}
}

static void destroy_usrfunc_list(struct dlist_head * head)
{
	struct dlist_head * entry;
	while (!dlist_empty(head))
	{
		entry = head->next; dlist_del(entry);
		delete_usrfunc_type(dlist_entry(entry, struct usrfunc_type, list));
	}
}

static void dump_usrfunc_args(struct dlist_head * head)
{
	struct dlist_head * entry;
	operand_t * op;

	duprint("(");
	for (entry = head->next; entry != head; entry = entry->next)
	{
		op = dlist_entry(entry, operand_t, list);
		dassert(op->type == OTYPE_STR);
		duprint("$%s", op->u.string);
		if (entry->next != head) duprint(", ");
	}
	duprint(")");
}

static void dump_usrfunc_list(struct dlist_head * head)
{
	struct dlist_head * entry;
	usrfunc_t * obj;

	for (entry = head->next; entry != head; entry = entry->next)
	{
		obj = dlist_entry(entry, usrfunc_t, list);
		duprint("function %s", obj->name);
		dump_usrfunc_args(&obj->args);
		duprint("\n");
		duprint("{\n");
		dump_sblist(&obj->statement, 1);
		duprint("}\n\n");
	}
}

/******************************************************************/

/* return -1 when EOF. */
static int read_php_token(xstream_t phpfd, strobj_t sobj, int * quot)
{
	int asterisk, c, d, ret=0;

	sobj_xstream_eatwhite(phpfd);
	c = xs_getc(phpfd);
	switch (c)
	{
	case EOF: ret = -1; break;
	case '/':
		/* Check if this is a comment block or a single '/'. */
		c = xs_getc(phpfd);
		switch (c)
		{
		case EOF: ret = -1; break;
		case '/':
			while ((c=xs_getc(phpfd))!=EOF) if (c=='\n' || c=='\r') break;
			ret = (c==EOF) ? -1 : read_php_token(phpfd, sobj, quot);
			break;
		case '*':
			asterisk = 0;
			while ((c=xs_getc(phpfd))!=EOF)
			{
				if (asterisk && c=='/') break;
				asterisk = (c=='*')?1:0;
			}
			ret = (c==EOF) ? -1 : read_php_token(phpfd, sobj, quot);
			break;
		default:
			xs_ungetc(c, phpfd);
			sobj_add_char(sobj, '/');
			*quot = 0;
			break;
		}
		break;

	case '=': case '&': case '|':
		sobj_add_char(sobj, c);
		d = xs_getc(phpfd);					/* peek the next character */
		if (c==d) sobj_add_char(sobj, d);	/* we are looking for '==', '&&' or '||'. */
		else xs_ungetc(d, phpfd);			/* not what we want, put it back. */
		*quot = 0;
		break;

	case '?':
		sobj_add_char(sobj, c);
		c = xs_getc(phpfd);					/* peek the next character */
		if (c=='>') sobj_add_char(sobj, c);	/* we are looking for '?>' */
		else xs_ungetc(c, phpfd);			/* not what we want, put it back. */
		*quot = 0;
		break;

	case '+': case '-':
		sobj_add_char(sobj, c);
		d = xs_getc(phpfd);					/* peek the next character */
		if (d=='=' || d==c) sobj_add_char(sobj,d); /* we are looking for +=, -=, ++ or -- */
		else xs_ungetc(d, phpfd);			/* not what we want, put it back. */
		*quot=0;
		break;

	case '!': case '<': case '>':
		sobj_add_char(sobj, c);
		c = xs_getc(phpfd);					/* peek the next character */
		if (c=='=') sobj_add_char(sobj,c);	/* we are looking for != <= or >= */
		else xs_ungetc(c, phpfd);			/* not what we want, put it back. */
		*quot=0;
		break;

	case ':': case ';':
	case '*': case '%': case '^': case '.': case ',': case '@': case '#':
	case '(': case ')': case '{': case '}': case '[': case ']':
		sobj_add_char(sobj, c);
		*quot = 0;
		break;

	default:
		xs_ungetc(c, phpfd);
		c = sobj_xstream_read(phpfd, sobj, "/+-=&|?*%^<>.,(){}[]:;!@#", quot);
		if (*quot && *quot == c) break;
		if (c == 0) ret = -1;	/* error occur */
		else if (c > 0) xs_ungetc(c, phpfd);
		break;
	}
	return ret;
}

/***********************************************************************/

#define ERR_BREAK(prefix, fmt, args...)                                 \
{                                                                       \
	client_printf(php->outfd, "\n!! %s >>>>>>>>>>>>>>>>>>>>>>>>>\n", (prefix)); \
	client_printf(php->outfd,   "!! %s: File:[%s]\n", (prefix), php->filename); \
	client_printf(php->outfd, fmt, ##args);                                 \
	client_printf(php->outfd, "\n!! %s <<<<<<<<<<<<<<<<<<<<<<<<<\n", (prefix)); \
	ret = -1;                                                           \
	break;                                                              \
}

static strobj_t	read_operand_list(ephp_t * php, struct dlist_head * head);
static void		find_1st_arg(struct dlist_head * head);
static void		find_2nd_arg(struct dlist_head * head);
static int		read_args_list   (ephp_t * php, struct dlist_head * head);
static int		read_cond_list   (ephp_t * php, struct dlist_head * head);

static int read_php(ephp_t * php, struct dlist_head * head, char * end);
static int parse_php(ephp_t * php, struct dlist_head * head);

/* return the strobj which end this list.
 * the caller is responsible to free the strobj_t.
 * error occur when NULL is returned. */
static strobj_t read_operand_list(ephp_t * php, struct dlist_head * head)
{
	int quot[2], ret = RET_ERROR;
	operand_t * op = NULL;
	strobj_t end = NULL;
	strobj_t sobj[2] = {NULL,NULL};
	bifunc_t * bif;
	struct dlist_head * ns;

	sobj[0] = sobj_new();
	sobj[1] = sobj_new();
	while (sobj[0] && sobj[1])
	{
		/* Read the current and next tokens. */
		sobj_free(sobj[0]);
		read_php_token(php->fd, sobj[0], &quot[0]);

		//d_dbg("%s: token1 [%s]\n",__func__, sobj_get_string(sobj[0]));

		if (!quot[0] && sobj_empty(sobj[0])) ERR_BREAK(SYNERR, NOTOKEN);
		if ((quot[0] == 0)
			&& (sobj_strcmp(sobj[0],  ";")==0 || sobj_strcmp(sobj[0],  ",")==0
			||  sobj_strcmp(sobj[0],  ")")==0 || sobj_strcmp(sobj[0], "&&")==0
			||	sobj_strcmp(sobj[0], "||")==0 || sobj_strcmp(sobj[0],  ">")==0
			||	sobj_strcmp(sobj[0],  "<")==0 || sobj_strcmp(sobj[0], "==")==0
			||	sobj_strcmp(sobj[0], "!=")==0 || sobj_strcmp(sobj[0], "<=")==0
			||	sobj_strcmp(sobj[0], ">=")==0))
		{
			/* reach the end, leaving now. */
			end = sobj[0];
			sobj[0] = NULL;
			ret = RET_SUCCESS;
			break;
		}

		/* Read the next token */
		sobj_free(sobj[1]);
		read_php_token(php->fd, sobj[1], &quot[1]);

		//d_dbg("%s: token2 [%s]\n",__func__, sobj_get_string(sobj[1]));

		/* Allocate space for this operand. */
		op = (operand_t *)MALLOC(sizeof(operand_t));
		if (!op) ERR_BREAK(INTERR, MEMERR);
		memset(op, 0, sizeof(operand_t));

		if (quot[0])
		{
			/* The string is in the quotation marks, it is raw message. */
			op->type = OTYPE_STR;
			op->u.string = sobj_strdup(sobj[0]);
		}
		else if (sobj_get_char(sobj[0], 0)=='$')
		{
			/* This string start with '$', it is variable. */
			sobj_remove_char(sobj[0], 0);	/* remove the 1st '$' */
			if (sobj_empty(sobj[0])) ERR_BREAK(SYNERR, "no variable name available !");
			op->type = OTYPE_VAR;

			/* Looking for the name space. */
			if		(sobj_strcmp(sobj[0], "_GLOBALS")==0)	ns = &g_nspace;
			else if	(sobj_strcmp(sobj[0], "_SERVER")==0)	ns = &g_ns_server;
			else if	(sobj_strcmp(sobj[0], "_GET")==0)		ns = &g_ns_get;
			else if	(sobj_strcmp(sobj[0], "_POST")==0)		ns = &g_ns_post;
			else if	(sobj_strcmp(sobj[0], "_ENV")==0)		ns = &g_ns_env;
			else if	(sobj_strcmp(sobj[0], "_FILES")==0)		ns = &g_ns_files;
			else if	(sobj_strcmp(sobj[0], "_FILETYPES")==0)	ns = &g_ns_filetypes;
			else											ns = NULL;

			if (ns)
			{
				if (sobj_strcmp(sobj[1], "[")!=0) ERR_BREAK(SYNERR, "expecting '['");
				sobj_free(sobj[0]); sobj_free(sobj[1]);
				read_php_token(php->fd, sobj[0], &quot[0]);
				read_php_token(php->fd, sobj[1], &quot[1]);
				if (sobj_strcmp(sobj[1], "]")!=0) ERR_BREAK(SYNERR, "expecting ']'");
				sobj_free(sobj[1]);
				read_php_token(php->fd, sobj[1], &quot[1]);
			}
			
			op->u.var.name = sobj_strdup(sobj[0]);
			op->u.var.nspace = ns;
		}
		else if (sobj_strcmp(sobj[1], "(")==0)
		{
			/* The next token is '(', it is function. */
			op->type = OTYPE_FUNC;
			op->u.func.fname = sobj_strdup(sobj[0]);
			INIT_DLIST_HEAD(&op->u.func.args);

			//d_dbg("%s: function name = [%s]\n",__func__, op->u.func.fname);

			/* Is this a built-in or user defined function ? */
			if ((bif = find_built_in_func(op->u.func.fname)) != NULL)
				op->u.func.func = bif->func;
			else
				op->u.func.func = NULL;
			/* read the arguments list. */
			ret = read_args_list(php, &op->u.func.args);
			if (ret < 0) break;

			/* Pass the 1st arg to sealpac_puts(). */
			if (strcmp(op->u.func.fname, "i18n")==0)
				find_1st_arg(&op->u.func.args);
			else if (strcmp(op->u.func.fname, "I18N")==0)
				find_2nd_arg(&op->u.func.args);

			/* refresh the next token. */
			sobj_free(sobj[1]);
			read_php_token(php->fd, sobj[1], &quot[1]);
		}
		else
		{
			/* This is a string without quotation marks. */
			op->type = OTYPE_STR;
			op->u.string = sobj_strdup(sobj[0]);
		}

		//d_dbg("%s: next token [%s]\n",__func__, sobj_get_string(sobj[1]));

		/* Check the next token */
		if (quot[1]) ERR_BREAK(SYNERR, QUOTERR);

		if		(sobj_strcmp(sobj[1], ".")==0) op->op = '.';
		else if	(sobj_strcmp(sobj[1], "+")==0) op->op = '+';
		else if	(sobj_strcmp(sobj[1], "-")==0) op->op = '-';
		else if	(sobj_strcmp(sobj[1], "*")==0) op->op = '*';
		else if	(sobj_strcmp(sobj[1], "/")==0) op->op = '/';
		else if	(sobj_strcmp(sobj[1], "%")==0) op->op = '%';
		else if	(	(sobj_strcmp(sobj[1], ";")==0)
				||	(sobj_strcmp(sobj[1], ",")==0)
				||	(sobj_strcmp(sobj[1], ")")==0)
				||	(sobj_strcmp(sobj[1],"&&")==0)
				||	(sobj_strcmp(sobj[1],"||")==0)
				||	(sobj_strcmp(sobj[1], ">")==0)
				||	(sobj_strcmp(sobj[1], "<")==0)
				||	(sobj_strcmp(sobj[1],"==")==0)
				||	(sobj_strcmp(sobj[1],"!=")==0)
				||	(sobj_strcmp(sobj[1],"<=")==0)
				||	(sobj_strcmp(sobj[1],">=")==0))
		{
			/* reach the end, leaving now */
			op->op = '\0';
			end = sobj[1];
			sobj[1] = NULL;
			ret = RET_SUCCESS;
		}
		else ERR_BREAK(SYNERR, "unknown token - [%s]", sobj_get_string(sobj[0]));

		/* add to the list */
		dlist_add_tail(&op->list, head);
		op = NULL;
		if (end) break;
	}
	if (op) delete_operand_type(op);
	if (sobj[0]) sobj_del(sobj[0]);
	if (sobj[1]) sobj_del(sobj[1]);
	return end;
}

static void find_1st_arg(struct dlist_head * head)
{
	struct dlist_head * entry;
	arg_t * arg;
	operand_t * op;

	if (dlist_empty(head)) return;
	arg = dlist_entry(head->next, arg_t, list);
	if (dlist_empty(&arg->operand)) return;

	entry = arg->operand.next;
	op = dlist_entry(entry, operand_t, list);
	if (op->type != OTYPE_STR) return;
	sealpac_puts(op->u.string);
}

static void find_2nd_arg(struct dlist_head * head)
{
	struct dlist_head * entry;
	arg_t * arg;
	operand_t * op;

	/* Get the 1st. */
	entry = dlist_get_next(NULL, head);
	if (entry==NULL) return;
	/* Get the 2nd. */
	entry = dlist_get_next(entry, head);
	if (entry==NULL) return;

	/* Get arg_t */
	arg = dlist_entry(entry, arg_t, list);
	/* Get 1st operand with OTYPE_STR - a constant string. */
	entry = dlist_get_next(NULL, &arg->operand);
	if (entry==NULL) return;
	op = dlist_entry(entry, operand_t, list);
	if (op->type != OTYPE_STR) return;
	sealpac_puts(op->u.string);
}

/* return -1 when error. */
static int read_args_list(ephp_t * php, struct dlist_head * head)
{
	int ret = RET_ERROR;
	arg_t * arg = NULL;
	strobj_t end = NULL;

	while (1)
	{
		/* allocate arg_t. */
		arg = (arg_t *)MALLOC(sizeof(arg_t));
		if (!arg) ERR_BREAK(INTERR, MEMERR);
		memset(arg, 0, sizeof(arg_t));
		INIT_DLIST_HEAD(&arg->operand);

		/* read the operand list */
		end = read_operand_list(php, &arg->operand);
		if (!end) break;

		/* add arg to the list */
		dlist_add_tail(&arg->list, head);
		arg = NULL;

		if (sobj_strcmp(end, ")")==0) {ret=RET_SUCCESS; break;}
		if (sobj_strcmp(end, ",")!=0) ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(end));

		/* The end token is ',', read the next operand. */
		sobj_del(end);
		end = NULL;
	}
	if (arg) delete_arg_type(arg);
	if (end) sobj_del(end);
	//d_dbg("%s: ret = %d\n",__func__,ret);
	return ret;
}

static int read_cond_list(ephp_t * php, struct dlist_head * head)
{
	int ret = RET_ERROR;
	strobj_t end = NULL;
	cond_t * cond = NULL;

	while (1)
	{
		/* allocate cond_t */
		cond = (cond_t *)MALLOC(sizeof(cond_t));
		if (!cond) ERR_BREAK(INTERR, MEMERR);
		memset(cond, 0, sizeof(cond_t));
		INIT_DLIST_HEAD(&cond->operand[0]);
		INIT_DLIST_HEAD(&cond->operand[1]);

		/* read the 1st operand list */
		end = read_operand_list(php, &cond->operand[0]);
		if (!end) break;

		/* check the operator */
		if		(sobj_strcmp(end,"==")==0)	cond->op = IFOP_EQUAL;
		else if	(sobj_strcmp(end,"!=")==0)	cond->op = IFOP_NOTEQUAL;
		else if	(sobj_strcmp(end, ">")==0)	cond->op = IFOP_GREATERTHAN;
		else if	(sobj_strcmp(end, "<")==0)	cond->op = IFOP_LESSTHAN;
		else if	(sobj_strcmp(end,">=")==0)	cond->op = IFOP_GREATEREQUAL;
		else if	(sobj_strcmp(end,"<=")==0)	cond->op = IFOP_LESSEQUAL;
		else ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(end));

		/* free the 'end' */
		sobj_del(end);
		end = NULL;

		/* read the 2nd operand list */
		end = read_operand_list(php, &cond->operand[1]);
		if (!end) break;

		/* check the end */
		if		(sobj_strcmp(end, ")")==0)	cond->next = IFNEXT_END;
		else if	(sobj_strcmp(end,"&&")==0)	cond->next = IFNEXT_AND;
		else if	(sobj_strcmp(end,"||")==0)	cond->next = IFNEXT_OR;
		else ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(end));

		/* add into the list */
		dlist_add_tail(&cond->list, head);
		cond = NULL;
		if (sobj_strcmp(end, ")")==0) { ret=RET_SUCCESS; break; }
		sobj_del(end);
		end = NULL;
	}
	if (cond) delete_cond_type(cond);
	if (end) sobj_del(end);
	return ret;
}

/*************************************************************************************/

static int read_php_assign(ephp_t * php, struct dlist_head * head, const char * name)
{
	sblock_t * sbptr = NULL;
	strobj_t sobj = NULL;
	int quot, ret = RET_ERROR;
	struct dlist_head * ns;

	EPHPDBG(d_dbg("%s: %s\n",__func__, name));

	do
	{
		/* allocate a string object */
		sobj = sobj_new();
		if (!sobj) ERR_BREAK(INTERR, MEMERR);

		/* allocate statement block */
		sbptr = (sblock_t *)MALLOC(sizeof(sblock_t));
		if (!sbptr) ERR_BREAK(INTERR, MEMERR);
		memset(sbptr, 0, sizeof(sblock_t));
		sbptr->type = SB_ASSIGN;
		INIT_DLIST_HEAD(&sbptr->list);
		INIT_DLIST_HEAD(&sbptr->u._assign.operand);

		/* looking for the name space */
		if		(strcmp(name, "$_GLOBALS")==0)		ns = &g_nspace;
		else if	(strcmp(name, "$_SERVER")==0)		ns = &g_ns_server;
		else if	(strcmp(name, "$_GET")==0)			ns = &g_ns_get;
		else if	(strcmp(name, "$_POST")==0)			ns = &g_ns_post;
		else if	(strcmp(name, "$_ENV")==0)			ns = &g_ns_env;
		else if	(strcmp(name, "$_FILES")==0)		ns = &g_ns_files;
		else if	(strcmp(name, "$_FILETYPES")==0)	ns = &g_ns_filetypes;
		else										ns = NULL;

		if (ns)
		{
			/* The next token should be '[' */
			sobj_free(sobj);
			read_php_token(php->fd, sobj, &quot);
			if (quot) ERR_BREAK(SYNERR, QUOTERR);
			if (sobj_strcmp(sobj, "[")!=0) ERR_BREAK(SYNERR, "expecting '['");
			/* Read the variable name */
			sobj_free(sobj);
			read_php_token(php->fd, sobj, &quot);
			sbptr->u._assign.var.name = sobj_strdup(sobj);
			/* The next token should be ']' */
			sobj_free(sobj);
			read_php_token(php->fd, sobj, &quot);
			if (quot) ERR_BREAK(SYNERR, QUOTERR);
			if (sobj_strcmp(sobj, "]")!=0) ERR_BREAK(SYNERR, "expecting ']'");
			/* The name space of this variable is GLOBAL */
			sbptr->u._assign.var.nspace = ns;
		}
		else
		{
			sbptr->u._assign.var.name = STRDUP(name+1);
			sbptr->u._assign.var.nspace = NULL;
		}

		/* get the operator */
		sobj_free(sobj);
		read_php_token(php->fd, sobj, &quot);
		if (sobj_empty(sobj)) ERR_BREAK(SYNERR, NOTOKEN);
		if (quot) ERR_BREAK(SYNERR, QUOTERR);

		EPHPDBG(d_dbg("%s: op = [%s]\n", __func__,sobj_get_string(sobj)));

		if		(sobj_strcmp(sobj,"+=")==0)	sbptr->u._assign.op = AOP_ADD;
		else if	(sobj_strcmp(sobj,"-=")==0)	sbptr->u._assign.op = AOP_SUB;
		else if	(sobj_strcmp(sobj,"++")==0)	sbptr->u._assign.op = AOP_INCREASE;
		else if	(sobj_strcmp(sobj,"--")==0)	sbptr->u._assign.op = AOP_DECREASE;
		else if	(sobj_strcmp(sobj, "=")==0)	sbptr->u._assign.op = AOP_ASSIGN;
		else ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj));

		/* We are gonna reuse 'sobj', so release it before we use it. */
		sobj_del(sobj); sobj = NULL;

		/* read the operands */
		sobj = read_operand_list(php, &sbptr->u._assign.operand);
		if (!sobj) break;
		if (sobj_strcmp(sobj, ";")!=0) ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj));

		/* add to the list */
		dlist_add_tail(&sbptr->list, head);
		sbptr = NULL;
		ret = 0;
	} while (0);
	if (sbptr) delete_sblock(sbptr);
	if (sobj) sobj_del(sobj);
	return ret;
}

static int read_php_fdecl(ephp_t * php, struct dlist_head * head)
{
	int quot[2], ret = RET_ERROR;
	strobj_t sobj[2] = {NULL,NULL};
	usrfunc_t * ufunc = NULL;
	operand_t * op = NULL;
	int exist = 0;

	do
	{
		/* allocate strobj */
		sobj[0] = sobj_new(); sobj[1] = sobj_new();
		if (!sobj[0] || !sobj[1]) ERR_BREAK(INTERR, MEMERR);

		/* read function name */
		read_php_token(php->fd, sobj[0], &quot[0]);
		if (sobj_empty(sobj[0])) ERR_BREAK(SYNERR, NOTOKEN);
		read_php_token(php->fd, sobj[1], &quot[1]);
		if (sobj_strcmp(sobj[1], "(")!=0)
			ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj[1]));

		/* Check if this function is already exist */
		if (find_built_in_func(sobj_get_string(sobj[0])) ||
			find_user_func(sobj_get_string(sobj[0])))
		{
			exist++;
			d_dbg("%s: The function - [%s] is already exist !\n",
				__func__,sobj_get_string(sobj[0]));
		}


		/* allocate sblock_t */
		ufunc = (usrfunc_t *)MALLOC(sizeof(usrfunc_t));
		if (!ufunc) ERR_BREAK(INTERR, MEMERR);
		memset(ufunc, 0, sizeof(usrfunc_t));
		ufunc->name = sobj_strdup(sobj[0]);
		INIT_DLIST_HEAD(&ufunc->args);
		INIT_DLIST_HEAD(&ufunc->statement);

		/* read argument list */
		while (1)
		{
			/* read argument */
			sobj_free(sobj[0]);
			read_php_token(php->fd, sobj[0], &quot[0]);
			if (sobj_empty(sobj[0])) ERR_BREAK(SYNERR, NOTOKEN);
			if (quot[0]) ERR_BREAK(SYNERR, QUOTERR);
			if (sobj_strcmp(sobj[0], ")")==0) { ret = RET_SUCCESS; break; }
			if (sobj_get_char(sobj[0], 0)!='$')
				ERR_BREAK(SYNERR, "expecting variables in function declaration !");
			sobj_remove_char(sobj[0], 0);

			/* save argument */
			op = (operand_t *)MALLOC(sizeof(operand_t));
			if (!op) ERR_BREAK(INTERR, MEMERR);
			memset(op, 0, sizeof(operand_t));
			op->type = OTYPE_STR;
			op->u.string = sobj_strdup(sobj[0]);
			op->op = '\0';
			dlist_add_tail(&op->list, &ufunc->args);

			/* read next token */
			sobj_free(sobj[1]);
			read_php_token(php->fd, sobj[1], &quot[1]);
			if (sobj_empty(sobj[1])) ERR_BREAK(SYNERR, NOTOKEN);
			if (quot[1]) ERR_BREAK(SYNERR, QUOTERR);
			if (sobj_strcmp(sobj[1], ")")==0) { ret = RET_SUCCESS; break; }
			if (sobj_strcmp(sobj[1], ",")!=0)
				ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj[0]));
		}
		if (ret < 0) break;

		/* next token should be '{' */
		sobj_free(sobj[0]);
		read_php_token(php->fd, sobj[0], &quot[0]);
		if (sobj_empty(sobj[0])) ERR_BREAK(SYNERR, NOTOKEN);
		if (quot[0]) ERR_BREAK(SYNERR, QUOTERR);
		if (sobj_strcmp(sobj[0], "{")!=0)
			ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj[0]));

		/* read the statement */
		ret = read_php(php, &ufunc->statement, "}");
		if (ret==RET_SUCCESS && !exist)
		{
			dlist_add_tail(&ufunc->list, &g_usrfunc);
			ufunc = NULL;
		}

	} while (0);
	if (ufunc) delete_usrfunc_type(ufunc);
	if (sobj[0]) sobj_del(sobj[0]);
	if (sobj[1]) sobj_del(sobj[1]);
	return ret;
}

static int read_php_echo(ephp_t * php, struct dlist_head * head)
{
	int ret = RET_ERROR;
	sblock_t * sbptr = NULL;
	strobj_t sobj = NULL;

	do
	{
		sbptr = (sblock_t *)MALLOC(sizeof(sblock_t));
		if (!sbptr) ERR_BREAK(INTERR, MEMERR);
		memset(sbptr, 0, sizeof(sblock_t));
		sbptr->type = SB_ECHO;
		INIT_DLIST_HEAD(&sbptr->u._operand);

		sobj = read_operand_list(php, &sbptr->u._operand);
		if (!sobj) break;
		if (sobj_strcmp(sobj, ";")!=0)
			ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj));

		dlist_add_tail(&sbptr->list, head);
		sbptr = NULL;
		ret = RET_SUCCESS;
	} while (0);
	if (sbptr) delete_sblock(sbptr);
	if (sobj) sobj_del(sobj);
	return ret;
}

static int read_php_return(ephp_t * php, struct dlist_head * head)
{
	int ret = RET_ERROR;
	sblock_t * sbptr = NULL;
	strobj_t sobj = NULL;

	do
	{
		sbptr = (sblock_t *)MALLOC(sizeof(sblock_t));
		if (!sbptr) ERR_BREAK(INTERR, MEMERR);
		memset(sbptr, 0, sizeof(sblock_t));
		sbptr->type = SB_RETURN;
		INIT_DLIST_HEAD(&sbptr->u._operand);

		sobj = read_operand_list(php, &sbptr->u._operand);
		if (!sobj) break;
		if (sobj_strcmp(sobj, ";")!=0)
			ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj));

		dlist_add_tail(&sbptr->list, head);
		sbptr = NULL;
		ret = RET_SUCCESS;
	} while (0);
	if (sbptr) delete_sblock(sbptr);
	if (sobj) sobj_del(sobj);
	return ret;
}

static int _rd_no_operand(ephp_t * php, struct dlist_head * head, sb_type type)
{
	int quot, ret = RET_ERROR;
	strobj_t sobj = NULL;
	sblock_t * sbptr = NULL;

	do
	{
		sobj = sobj_new();
		if (!sobj) ERR_BREAK(INTERR, MEMERR);

		/* the next token should be ';' */
		read_php_token(php->fd, sobj, &quot);
		if (sobj_empty(sobj)) ERR_BREAK(SYNERR, NOTOKEN);
		if (quot) ERR_BREAK(SYNERR, QUOTERR);
		if (sobj_strcmp(sobj, ";")!=0)
			ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj));

		/* add statement block */
		sbptr = (sblock_t *)MALLOC(sizeof(sblock_t));
		if (!sbptr) ERR_BREAK(INTERR, MEMERR);
		memset(sbptr, 0, sizeof(sblock_t));
		sbptr->type = type;
		dlist_add_tail(&sbptr->list, head);
		ret = RET_SUCCESS;
	} while (0);
	if (sobj) sobj_del(sobj);
	return ret;
}

static int read_php_break(ephp_t * php, struct dlist_head * head)
{
	return _rd_no_operand(php, head, SB_BREAK);
}

static int read_php_exit(ephp_t * php, struct dlist_head * head)
{
	return _rd_no_operand(php, head, SB_EXIT);
}

static int read_php_continue(ephp_t * php, struct dlist_head * head)
{
	return _rd_no_operand(php, head, SB_CONTINUE);
}

static int _rd_php_file(ephp_t * php, struct dlist_head * head, int err)
{
	int quot, ret = RET_ERROR;
	strobj_t sobj = NULL;
	char * fname = NULL;

	do
	{
		sobj = sobj_new();
		if (!sobj) ERR_BREAK(INTERR, MEMERR);

		/* the next token should be a quoted string. */
		read_php_token(php->fd, sobj, &quot);
		if (sobj_empty(sobj)) ERR_BREAK(SYNERR, NOTOKEN);
		if (!quot) ERR_BREAK(SYNERR, NOQUOTERR);
		fname = sobj_strdup(sobj);
		sobj_free(sobj);

		/* the next token should be ';' */
		read_php_token(php->fd, sobj, &quot);
		if (sobj_empty(sobj)) ERR_BREAK(SYNERR, NOTOKEN);
		if (quot) ERR_BREAK(SYNERR, QUOTERR);
		if (sobj_strcmp(sobj, ";")!=0)
			ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj));

		/* read and parse the include file */
		d_dbg("%s: file name = [%s]\n",__func__, fname);
		ret = RET_SUCCESS;
	} while (0);
	if (sobj) sobj_del(sobj);
	if (fname) FREE(fname);
	return ret;
}

static int read_php_require(ephp_t * php, struct dlist_head * head)
{
	return _rd_php_file(php, head, 1);
}

static int read_php_include(ephp_t * php, struct dlist_head * head)
{
	return _rd_php_file(php, head, 0);
}

static int read_php_if(ephp_t * php, struct dlist_head * head)
{
	int quot, ret = RET_ERROR;
	sblock_t * sbptr = NULL;
	strobj_t * sobj = NULL;

	do
	{
		/* allocate strobj */
		sobj = sobj_new();
		if (!sobj) ERR_BREAK(INTERR, MEMERR);

		/* allocate sblock_t */
		sbptr = (sblock_t *)MALLOC(sizeof(sblock_t));
		if (!sbptr) ERR_BREAK(INTERR, MEMERR);
		memset(sbptr, 0, sizeof(sblock_t));
		sbptr->type = SB_IF;
		INIT_DLIST_HEAD(&sbptr->u._if.condition);
		INIT_DLIST_HEAD(&sbptr->u._if._true);
		INIT_DLIST_HEAD(&sbptr->u._if._false);

		/* read the condition list */
		if (read_cond_list(php, &sbptr->u._if.condition) < 0) break;

		/* read the statement block */
		sobj_free(sobj);
		read_php_token(php->fd, sobj, &quot);
		EPHPDBG(d_dbg("%s: next token = [%s]\n",__func__, sobj_get_string(sobj)));
		if (!quot && sobj_strcmp(sobj, "{")==0)
		{
			if (read_php(php, &sbptr->u._if._true, "}") < 0) break;
		}
		else
		{
			xs_ungetc('\n', php->fd);
			if (quot) xs_ungetc('\"', php->fd);
			xs_ungets(sobj_get_string(sobj), php->fd);
			if (quot) xs_ungetc('\"', php->fd);
			if (read_php(php, &sbptr->u._if._true, NULL) < 0) break;
		}

		/* read else */
		sobj_free(sobj);
		read_php_token(php->fd, sobj, &quot);
		if (!quot && sobj_strcmp(sobj, "else")==0)
		{
			sobj_free(sobj);
			read_php_token(php->fd, sobj, &quot);
			if (!quot && sobj_strcmp(sobj, "if")==0)
			{
				sobj_free(sobj);
				read_php_token(php->fd, sobj, &quot);
				if (sobj_strcmp(sobj, "(")!=0)
					ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj));
				if (read_php_if(php, &sbptr->u._if._false) < 0) break;
			}
			else if (!quot && sobj_strcmp(sobj, "{")==0)
			{
				if (read_php(php, &sbptr->u._if._false, "}") < 0) break;
			}
			else
			{
				xs_ungetc('\n', php->fd);
				if (quot) xs_ungetc('\"', php->fd);
				xs_ungets(sobj_get_string(sobj), php->fd);
				if (quot) xs_ungetc('\"', php->fd);
				if (read_php(php, &sbptr->u._if._false, NULL) < 0) break;
			}
		}
		else
		{
			/* not 'else', put it back. */
			xs_ungetc('\n', php->fd);
			if (quot) xs_ungetc('\"', php->fd);
			xs_ungets(sobj_get_string(sobj), php->fd);
			if (quot) xs_ungetc('\"', php->fd);
		}

		dlist_add_tail(&sbptr->list, head);
		sbptr = NULL;
		ret = 0;

	} while (0);
	if (sobj) sobj_del(sobj);
	if (sbptr) delete_sblock(sbptr);
	return ret;
}

static int read_php_while(ephp_t * php, struct dlist_head * head)
{
	sblock_t * sbptr = NULL;
	strobj_t * sobj = NULL;
	int quot, ret = RET_ERROR;

	do
	{
		/* allocate strobj */
		sobj = sobj_new();
		if (!sobj) ERR_BREAK(INTERR, MEMERR);

		/* allocate sblock_t */
		sbptr = (sblock_t *)MALLOC(sizeof(sblock_t));
		if (!sbptr) ERR_BREAK(INTERR, MEMERR);
		memset(sbptr, 0, sizeof(sblock_t));
		sbptr->type = SB_WHILE;
		INIT_DLIST_HEAD(&sbptr->u._while.condition);
		INIT_DLIST_HEAD(&sbptr->u._while.statement);

		/* read condition */
		if (read_cond_list(php, &sbptr->u._while.condition) < 0) break;

		/* read the statement block */
		sobj_free(sobj);
		read_php_token(php->fd, sobj, &quot);
		if (!quot && sobj_strcmp(sobj, "{")==0)
		{
			if (read_php(php, &sbptr->u._while.statement, "}") < 0) break;
		}
		else
		{
			xs_ungetc('\n', php->fd);
			if (quot) xs_ungetc('\"', php->fd);
			xs_ungets(sobj_get_string(sobj), php->fd);
			if (quot) xs_ungetc('\"', php->fd);
			if (read_php(php, &sbptr->u._while.statement, NULL) < 0) break;
		}

		/* Done */
		dlist_add_tail(&sbptr->list, head);
		sbptr = NULL;
		ret = 0;

	} while (0);
	if (sbptr) delete_sblock(sbptr);
	if (sobj) sobj_del(sobj);
	return ret;
}

static int read_php_foreach(ephp_t * php, struct dlist_head * head)
{
	sblock_t * sbptr = NULL;
	strobj_t * sobj = NULL;
	int quot, ret = RET_ERROR;

	do
	{
		/* allocate sblock_t */
		sbptr = (sblock_t *)MALLOC(sizeof(sblock_t));
		if (!sbptr) ERR_BREAK(INTERR, MEMERR);
		memset(sbptr, 0, sizeof (sblock_t));
		sbptr->type = SB_FOREACH;
		INIT_DLIST_HEAD(&sbptr->u._foreach.operand);
		INIT_DLIST_HEAD(&sbptr->u._foreach.statement);

		/* read node */
		sobj = read_operand_list(php, &sbptr->u._foreach.operand);
		if (!sobj) break;
		if (sobj_strcmp(sobj, ")") != 0)
			ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj));

		/* read the statement block */
		sobj_free(sobj);
		read_php_token(php->fd, sobj, &quot);
		if (!quot && sobj_strcmp(sobj, "{") == 0)
		{
			if (read_php(php, &sbptr->u._foreach.statement, "}") < 0) break;
		}
		else
		{
			//xs_ungetc('\n', php->fd);
			if (quot) xs_ungetc('\"', php->fd);
			xs_ungets(sobj_get_string(sobj), php->fd);
			if (quot) xs_ungetc('\"', php->fd);
			if (read_php(php, &sbptr->u._foreach.statement, NULL) < 0) break;
		}

		/* Done */
		dlist_add_tail(&sbptr->list, head);
		sbptr = NULL;
		ret = 0;
	} while(0);
	if (sbptr) delete_sblock(sbptr);
	if (sobj) sobj_del(sobj);
	return ret;
}

static int read_php_func(ephp_t * php, struct dlist_head * head, const char * fname, func_handler func)
{
	sblock_t * sbptr = NULL;
	strobj_t * sobj = NULL;
	int quot, ret = RET_ERROR;

	do
	{
		/* allocate strobj */
		sobj = sobj_new();
		if (!sobj) ERR_BREAK(INTERR, MEMERR);

		/* allocate sblock_t */
		sbptr = (sblock_t *)MALLOC(sizeof(sblock_t));
		if (!sbptr) ERR_BREAK(INTERR, MEMERR);
		memset(sbptr, 0, sizeof(sblock_t));

		/* setup sblock_t */
		sbptr->type = SB_FUNC;
		sbptr->u._func.fname = STRDUP(fname);
		sbptr->u._func.func = func;
		INIT_DLIST_HEAD(&sbptr->u._func.args);

		/* read arguments */
		ret = read_args_list(php, &sbptr->u._func.args);
		if (ret < 0) break;

		/* Read the terminating ';' */
		read_php_token(php->fd, sobj, &quot);
		if (quot) ERR_BREAK(SYNERR, QUOTERR);
		if (sobj_strcmp(sobj, ";")!=0)
			ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj));

		/* Pass the 1st arg to sealpac_puts(). */
		if (strcmp(sbptr->u._func.fname, "i18n")==0)
			find_1st_arg(&sbptr->u._func.args);
		else if (strcmp(sbptr->u._func.fname, "I18N")==0)
			find_2nd_arg(&sbptr->u._func.args);

		dlist_add_tail(&sbptr->list, head);
		sbptr = NULL;
		ret = 0;

	} while (0);
	if (sbptr) delete_sblock(sbptr);
	if (sobj) sobj_del(sobj);
	d_dbg("%s: ret = %d\n",__func__,ret);
	return ret;
}

/* ---------------------------------------------------------------------- */
/* reutrn -1 when error. */
static int read_php(ephp_t * php, struct dlist_head * head, char * end)
{
	int quot[2], ret = RET_SUCCESS;
	strobj_t sobj[2];
	bifunc_t * bif;

	/* new sobj. */
	sobj[0] = sobj_new(); sobj[1] = sobj_new();

	/* read a token */
	if (sobj[0]) read_php_token(php->fd, sobj[0], &quot[0]);

	while (sobj[0] && sobj[1])
	{
		//d_dbg("%s: token=[%s], quot=%d, end=[%s]\n",__func__, sobj_get_string(sobj[0]), quot[0], end);

		if (sobj_empty(sobj[0])) ERR_BREAK(SYNERR, NOTOKEN);
		if (end && sobj_strcmp(sobj[0], end)==0) break; /* reach the end tag. */

		/* this one should not be in the quotation marks. */
		if (quot[0]) ERR_BREAK(SYNERR, QUOTERR);
		if (sobj_get_char(sobj[0],0)=='$') ret = read_php_assign(php, head, sobj_get_string(sobj[0]));
		else if	(sobj_strcmp(sobj[0],"function")==0) ret = read_php_fdecl  (php, head);
		else if	(sobj_strcmp(sobj[0],    "echo")==0) ret = read_php_echo   (php, head);
		else if	(sobj_strcmp(sobj[0],  "return")==0) ret = read_php_return (php, head);
		else if	(sobj_strcmp(sobj[0],   "break")==0) ret = read_php_break  (php, head);
		else if	(sobj_strcmp(sobj[0],    "exit")==0) ret = read_php_exit   (php, head);
		else if (sobj_strcmp(sobj[0],"continue")==0) ret = read_php_continue(php,head);
		else if	(sobj_strcmp(sobj[0], "require")==0) ret = read_php_require(php, head);
		else if	(sobj_strcmp(sobj[0], "include")==0) ret = read_php_include(php, head);
		else
		{
			/* read next token */
			sobj_free(sobj[1]);
			read_php_token(php->fd, sobj[1], &quot[1]);

			if (sobj_strcmp(sobj[1], "(")==0)
			{
				if		(sobj_strcmp(sobj[0],     "if")==0) ret = read_php_if     (php, head);
				else if	(sobj_strcmp(sobj[0],  "while")==0) ret = read_php_while  (php, head);
				else if (sobj_strcmp(sobj[0],"foreach")==0) ret = read_php_foreach(php, head);
				else
				{
					if ((bif = find_built_in_func(sobj_get_string(sobj[0]))))
					{
						ret = read_php_func(php, head, sobj_get_string(sobj[0]), bif->func);
					}
					else
					{
						ret = read_php_func(php, head, sobj_get_string(sobj[0]), NULL);
					}
				}
			}
			else
				ERR_BREAK(SYNERR, INVALID_TOKEN, sobj_get_string(sobj[0]));
		}

		/* If there is no 'end', we only read one statement block. */
		if (!end || (ret < 0)) break;
		sobj_free(sobj[0]);
		read_php_token(php->fd, sobj[0], &quot[0]);
	}

	if (sobj[0]) sobj_del(sobj[0]);
	if (sobj[1]) sobj_del(sobj[1]);
	return ret;
}

/*********************************************************************************/

/* variable: is the message raw message or variable name. */
static int add_echo(struct dlist_head * head, strobj_t message, int variable)
{
	sblock_t * sbptr = NULL;
	operand_t * op = NULL;
	int ret = -1;

	sbptr = (sblock_t *)MALLOC(sizeof(sblock_t));
	if (sbptr)
	{
		memset(sbptr, 0, sizeof(sblock_t));
		sbptr->type = SB_ECHO;
		INIT_DLIST_HEAD(&sbptr->u._operand);
		op = (operand_t *)MALLOC(sizeof(operand_t));
		if (op)
		{
			memset(op, 0, sizeof(operand_t));
			if (variable)
			{
				op->type = OTYPE_VAR;
				op->u.var.name = sobj_strdup(message);
				op->u.var.nspace = &g_nspace;
			}
			else
			{
				op->type = OTYPE_STR;
				op->u.string = sobj_strdup(message);
			}
			dlist_add_tail(&op->list, &sbptr->u._operand);
			dlist_add_tail(&sbptr->list, head);
			sbptr = NULL;
			op = NULL;
			ret = 0;
		}
		else
		{
			d_error("%s: memory allocation failed !\n");
		}
	}
	else
	{
		d_error("%s: memory allocation failed !!\n");
	}
	if (op) FREE(op);
	if (sbptr) FREE(sbptr);
	return ret;
}

static int handle_echo_shortcut(ephp_t * php, struct dlist_head * head)
{
	strobj_t sobj;
	int quot = 0, ret = RET_ERROR;

	do
	{
		/* we need a sobj. */
		sobj = sobj_new();
		if (!sobj) ERR_BREAK(INTERR, MEMERR);
		/* read one token */
		read_php_token(php->fd, sobj, &quot);
		if (quot) ERR_BREAK(INTERR, QUOTERR);
		d_dbg("%s: var = %s\n", __func__, sobj_get_string(sobj));
		/* add echo $variable */
		add_echo(head, sobj, 1);
		/* read the end tag */
		sobj_free(sobj);
		read_php_token(php->fd, sobj, &quot);
		if (sobj_strcmp(sobj, "?>") != 0)
			ERR_BREAK(SYNERR, "Expecting '?>' immediately !");
		/* Done */
		ret = RET_SUCCESS;
	} while (0);
	if (sobj) sobj_del(sobj);
	return ret;
}

static int check_format(ephp_t * php, struct dlist_head * head)
{
	int c;

	c = xs_getc(php->fd);
	if (c == EOF) return RET_ERROR;
	else if (c == '=')
	{
		c = xs_getc(php->fd);
		if (c == '$') return handle_echo_shortcut(php, head);
		client_puts("SYNTAX ERROR: expecting a variable after '=' !!\n", php->outfd);
		return RET_ERROR;
	}

	xs_ungetc(c, php->fd);
	return read_php(php, head, "?>");
}

/******************************************************************/

static int parse_php(ephp_t * php, struct dlist_head * head)
{
	int utf8[3];
	int i, c, ret = 0;
	strobj_t sobj;

	/* allocate string obj */
	sobj = sobj_new();
	if (!sobj)
	{
		d_error("%s: memory allocation failed !\n",__func__);
		return RET_ERROR;
	}

	/* Check if the file has UTF8 prefix, 0xef, 0xbb, 0xbf  */
	utf8[0] = xs_getc(php->fd);
	utf8[1] = xs_getc(php->fd);
	utf8[2] = xs_getc(php->fd);
	if (utf8[0] != 0xef || utf8[1] != 0xbb || utf8[2] != 0xbf)
	{
		/* Not a UTF8 prefix, restore the characters. */
		if (utf8[2] != EOF) xs_ungetc(utf8[2], php->fd);
		if (utf8[1] != EOF) xs_ungetc(utf8[1], php->fd);
		if (utf8[0] != EOF) xs_ungetc(utf8[0], php->fd);
	}

	/* the main loop to read php file. */
	while (ret == 0 && (c = xs_getc(php->fd)) != EOF)
	{
		/* looking for '<?' */
		if (c != '<')
		{
			/* this is not what we looking for, store it in sobj. */
			sobj_add_char(sobj, c);
		}
		else
		{
			c = xs_getc(php->fd);
			if (c == EOF)
			{
				sobj_add_char(sobj, '<');
				break;
			}
			if (c != '?')
			{
				sobj_add_char(sobj, '<');
				xs_ungetc(c, php->fd);
			}
			else
			{
				/* make the message outside php block a echo command. */
				if (!sobj_empty(sobj))
				{
					ret = add_echo(head, sobj, 0);
					sobj_free(sobj);
					if (ret) break;
				}
				ret = check_format(php, head);
				if (ret != 0) break;
			}
		}
	}

	if (ret == 0)
	{
		/* If the rest (sobj) are all newline character, ignore them. */
		i = sobj_get_length(sobj);
		while (i > 0)
		{
			i--;
			c = sobj_get_char(sobj, i);
			if (c != '\n' && c != '\r')
			{
				add_echo(head, sobj, 0);
				break;
			}
		}
	}
	if (sobj) sobj_del(sobj);
	d_dbg("%s: returning %d\n", __func__, ret);
	return ret;
}

/******************************************************************/

int xmldb_ephp(int fd, const char * file, unsigned long flags)
{
	int ret = RET_ERROR;
	ephp_t php;
	DLIST_HEAD(shead);

	/* set debug fd */
	//dfd = stdout;

	/* initialize the php instance */
	php.outfd = fd;
	php.filename = file;
	php.fd = xs_fopen(file, "r");
	if (php.fd)
	{
		/* parse php */
		ret = parse_php(&php, &shead);
		if (ret == RET_SUCCESS)
		{
			/* execute php */
			dump_usrfunc_list(&g_usrfunc);
			dump_sblist(&shead, 0);
		}

		/* done, clean it up */
		destroy_sblist(&shead);
		destroy_usrfunc_list(&g_usrfunc);
		xs_close(php.fd);
	}

	d_dbg("%s: ret=%d\n",__func__, ret);
	return ret;
}
