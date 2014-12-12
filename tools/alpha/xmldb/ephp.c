/* vi: set sw=4 ts=4: */
/*
 *	epnp.c
 *
 *	An embeded php implementation for xmldb.
 *
 *	Created by David Hsieh (david_hsieh@alphanetworks.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dtrace.h"
#include "xmldb.h"
#include "ephp.h"

/******************************************************************/
//#define DUMP_SBLIST	1

#define SAVE_FREE(x)	{ if ((x)!=NULL) free(x); }

/******************************************************************/

/* the root name space list of the variables */
static var_t * g_nspace = NULL;

/* variable function prototypes */
static var_t * get_var(const char * name);
static void set_var(const char * name, const char * value);
static const char * get_var_value(const char * name);
static void clear_var(void);

/* Allocate a var_t and assign the name and value of it.
 * The newly alloated entry will insert into the list head
 * of the name space.  */
static void _new_var(const char * name, const char * value)
{
	var_t * var;
	if ((var = (var_t *)malloc(sizeof(var_t))) != NULL)
	{
		memset(var, 0, sizeof(var_t));
		var->name  = strdup(name);
		var->value = strdup(value);
		var->next  = g_nspace;
		g_nspace   = var;
	}
}

/* Search for the var_t by name.
 * The pointer to the var_t structure will be returned.
 * If the variable is not exist, a new will will be
 * alloacted. */
static var_t * get_var(const char * name)
{
	var_t * curr;

	for (curr = g_nspace; curr; curr = curr->next)
		if (strcmp(curr->name, name)==0) return curr;

	_new_var(name, "");
	return g_nspace;
}

/* Create or modify the value of the variable of 'name'.
 * If the variable is existing, the value will be modified,
 * or a new var_t will be created. */
static void set_var(const char * name, const char * value)
{
	var_t * var = get_var(name);

	if (var)
	{
		SAVE_FREE(var->value);
		var->value = strdup(value);
	}
	else
	{
		_new_var(name, value);
	}
}

/* Different from get_var(), this function return the pointer
 * to the value string instead of var_t structure. */
static const char * get_var_value(const char * name)
{
	return get_var(name)->value;
}

/* destroy all the name space variables. */
static void clear_var(void)
{
	var_t * curr, * next;

	curr = g_nspace;
	while (curr)
	{
		next = curr->next;
		SAVE_FREE(curr->name);
		SAVE_FREE(curr->value);
		free(curr);
		curr = next;
	}
	g_nspace = NULL;
}

/******************************************************************/
/* buffered write fd */
#define BUFFER_SIZE	1024
static char buffer[BUFFER_SIZE];
static int count = 0;
static void b_flush(int fd)
{
	write(fd, buffer, count);
	count = 0;
}
static void b_putc(char c, int fd)
{
	if (count < BUFFER_SIZE) buffer[count++] = c;
	if (count == BUFFER_SIZE) b_flush(fd);
}
static void b_puts(const char * str, int fd)
{
	while (*str) b_putc(*str++, fd);
}

/******************************************************************/
#define MAX_STRING_LEN	1024

static int find_pattern(char c, const char * str)
{
	int i;
	for (i=0; str[i] && c!=str[i]; i++);
	return (int)str[i];
}

static char * read_string_ex(const char ** message, const char * end, int * quote);
static char * read_string(const char ** message, char end, int * quote)
{
	char p[2];
	p[0] = end;
	p[1] = '\0';
	return read_string_ex(message, p, quote);
}

static char * read_string_ex(const char ** message, const char * end, int * quote)
{
	static char string[MAX_STRING_LEN];
	int i = 0;
	const char * msg = *message;
	char c;

	/* clear our string buffer */
	memset(string, 0, sizeof(string));

	/* check to see if string contains in '"' */
	if (*msg == '"')
	{
		*quote = 1;
		/* read string inside. "". */
		msg++;
		for (i=0; *msg && *msg != '"'; msg++)
		{
			if (i < MAX_STRING_LEN-1)
			{
				c = *msg;
				if (c=='\\')
				{
					msg++;
					c = *msg;
					if (c == 'n') c = '\n';
				}
				string[i++] = c;
			}
		}
		if (*msg == '"') *message = msg+1;
		else string[0] = '\0';
	}
	else
	{
		*quote = 0;
		/* read string ended with white space, or 'end' */
		for (i=0; *msg && !IS_WHITE(*msg); msg++)
		{
			if (end && find_pattern(*msg, end)) break;
			if (i < MAX_STRING_LEN-1) string[i++] = *msg;
		}
		*message = msg;
	}

	return string;
}

/******************************************************************/
static void destroy_strlst(struct dlist_head * head);

static const char * read_strlst(struct dlist_head * head, const char ** message, char * op, char * end)
{
	char ended[80];
	const char * curr;
	int quote;
	const char * token;
	strlist_t * strlst = NULL;

	strcpy(ended, op);
	strcat(ended, end);

	curr = xmldb_eatwhite(*message);
	while (*curr)
	{
		/* allocate a strlist_t entry */
		strlst = (strlist_t *)malloc(sizeof(strlist_t));
		if (!strlst) goto read_strlst_error;
		INIT_DLIST_HEAD(&strlst->link);

		/* check if this token is query() or queryjs() */
		if (strncmp(curr, "query", 5)==0)
		{
			if (strncmp(curr, "queryjs", 7)==0)
			{
				strlst->type = ST_QUERYJS;
				curr = xmldb_eatwhite(curr+7);
			}
			else
			{
				strlst->type = ST_QUERY;
				curr = xmldb_eatwhite(curr+5);
			}
			INIT_DLIST_HEAD(&strlst->u.slist);

			if (*curr != '(')
			{
				*message = curr;
				goto read_strlst_error;
			}
			curr = xmldb_eatwhite(curr+1);
			if (read_strlst(&strlst->u.slist, &curr, ".", ")")==NULL)
			{
				*message = curr;
				goto read_strlst_error;
			}
			if (*curr != ')')
			{
				*message = curr;
				goto read_strlst_error;
			}
			curr++;
		}
		else
		{
			token = read_string_ex(&curr, ended, &quote);

			if (!quote && token[0] == '$')
			{
				strlst->type = ST_VAR;
				strlst->u.str = strdup(token+1);
			}
			else
			{
				strlst->type = ST_STR;
				strlst->u.str = strdup(token);
			}
		}

		/* token done, check next */
		curr = xmldb_eatwhite(curr);
		if (find_pattern(*curr, end)) strlst->op = '\0';
		else strlst->op = *curr++;

		dlist_add_tail(&strlst->link, head);
		if (strlst->op == '\0') break;
		strlst = NULL;
		curr = xmldb_eatwhite(curr);
	}

	*message = curr;
	return curr;

read_strlst_error:
	if (strlst)
	{
		if (strlst->type == ST_QUERY || strlst->type == ST_QUERYJS)
			destroy_strlst(&strlst->u.slist);
		else if (strlst->u.str)
			free(strlst->u.str);
		free(strlst);
	}
	return NULL;
}


/******************************************************************/

static void destroy_strlst(struct dlist_head * head)
{
	struct dlist_head * entry;
	strlist_t * slist;

	while (!dlist_empty(head))
	{
		/* extract an entry from the list head. */
		entry = head->next;
		dlist_del(entry);
		slist = dlist_entry(entry, strlist_t, link);

		/* free string */
		switch (slist->type)
		{
		case ST_STR:
		case ST_VAR:
			SAVE_FREE(slist->u.str);
			break;
		case ST_QUERY:
		case ST_QUERYJS:
			destroy_strlst(&slist->u.slist);
			break;
		}
		free(slist);
	}
}

#ifdef DUMP_SBLIST
static void dump_strlst(struct dlist_head * head)
{
	struct dlist_head * entry;
	strlist_t * slist;

	entry = head->next;
	while (entry != head)
	{
		slist = dlist_entry(entry, strlist_t, link);
		switch (slist->type)
		{
		case ST_STR:
			printf("\"%s\"", slist->u.str);
			break;
		case ST_VAR:
			printf("$%s",slist->u.str);
			break;
		case ST_QUERY:
			printf("query("); dump_strlst(&slist->u.slist); printf(")");
			break;
		case ST_QUERYJS:
			printf("queryjs("); dump_strlst(&slist->u.slist); printf(")");
			break;
		}
		if (slist->op) printf("%c", slist->op);
		entry = entry->next;
	}
}
#endif

/******************************************************************/

static void free_match_t(match_t * ptr)
{
	if (ptr)
	{
		destroy_strlst(&ptr->pattern);
		destroy_strlst(&ptr->output);
		free(ptr);
	}
}

static void destroy_match_list(match_t * mptr)
{
	match_t * next;

	while (mptr)
	{
		next = mptr->next;
		free_match_t(mptr);
		mptr = next;
	}
}

/******************************************************************/

static void free_cond_t(cond_t * cptr)
{
	if (cptr)
	{
		destroy_strlst(&cptr->operand1);
		destroy_strlst(&cptr->operand2);
		free(cptr);
	}
}

static void destroy_cond_list(struct dlist_head * head)
{
	cond_t *cptr;
	struct dlist_head * entry;

	while (!dlist_empty(head))
	{
		entry = head->next;
		dlist_del(entry);
		cptr = dlist_entry(entry, cond_t, link);
		free_cond_t(cptr);
	}
}

/******************************************************************/

static void destroy_sblist(struct dlist_head * head)
{
	struct dlist_head * entry;
	sblock_t * sbptr;

	while (!dlist_empty(head))
	{
		entry = head->next;
		dlist_del(entry);
		sbptr = dlist_entry(entry, sblock_t, link);
		switch (sbptr->type)
		{
		case SB_REQUIRE:
			destroy_strlst(&sbptr->u._require.file);
			break;
		case SB_ANCHOR:
			destroy_strlst(&sbptr->u._anchor.node);
			break;
		case SB_QUERY:
		case SB_QUERYJS:
			destroy_strlst(&sbptr->u._query.node);
			break;
		case SB_DEL:
			destroy_strlst(&sbptr->u._del.node);
			break;
		case SB_SET:
			destroy_strlst(&sbptr->u._set.node);
			destroy_strlst(&sbptr->u._set.value);
			break;
		case SB_ECHO:
			destroy_strlst(&sbptr->u._echo.string);
			break;
		case SB_ASSIGN:
			SAVE_FREE(sbptr->u._assign.variable);
			destroy_strlst(&sbptr->u._assign.value);
			break;
		case SB_MAP:
			free_match_t(sbptr->u._map.default_output);
			destroy_match_list(sbptr->u._map.match);
			break;
		case SB_INCLOG:
			SAVE_FREE(sbptr->u._inclog.format);
			SAVE_FREE(sbptr->u._inclog.file);
			break;
		case SB_IF:
			destroy_cond_list(&sbptr->u._if.condition);
			destroy_sblist(&sbptr->u._if._true);
			destroy_sblist(&sbptr->u._if._false);
			break;
		case SB_FOR:
			destroy_strlst(&sbptr->u._for.node);
			destroy_sblist(&sbptr->u._for.statement);
			break;
		case SB_EXIT:
			break;
		}
		free(sbptr);
	}
}

#ifdef DUMP_SBLIST
static void dump_sblist(struct dlist_head * head, int depth)
{
	struct dlist_head * entry, *centry;
	sblock_t * sbptr;
	int i;
	match_t * mptr;
	cond_t * cptr;

#define PREFIX(c)	{ for (i=0; i<c; i++) printf("\t"); }

	entry = head->next;
	while (entry != head)
	{
		sbptr = dlist_entry(entry, sblock_t, link);
		PREFIX(depth);
		switch (sbptr->type)
		{
		case SB_REQUIRE:
			printf("require("); dump_strlst(&sbptr->u._require.file); printf(");\n");
			break;
		case SB_ANCHOR:
			printf("anchor("); dump_strlst(&sbptr->u._anchor.node); printf(");\n");
			break;
		case SB_QUERY:
			printf("query("); dump_strlst(&sbptr->u._query.node); printf(");\n");
			break;
		case SB_QUERYJS:
			printf("queryjs("); dump_strlst(&sbptr->u._query.node); printf(");\n");
			break;
		case SB_DEL:
			printf("del("); dump_strlst(&sbptr->u._del.node); printf(");\n");
			break;
		case SB_SET:
			printf("set("); dump_strlst(&sbptr->u._set.node);
			printf(","); dump_strlst(&sbptr->u._set.value); printf(");\n");
			break;
		case SB_ECHO:
			printf("echo "); dump_strlst(&sbptr->u._echo.string); printf(";\n");
			break;
		case SB_ASSIGN:
			printf("$%s ", sbptr->u._assign.variable);
			switch (sbptr->u._assign.op)
			{
			case AOP_ASSIGN:	printf("= "); dump_strlst(&sbptr->u._assign.value); break;
			case AOP_ADD:		printf("+= "); dump_strlst(&sbptr->u._assign.value); break;
			case AOP_SUB:		printf("-= "); dump_strlst(&sbptr->u._assign.value); break;
			case AOP_INCREASE:	printf("++ "); break;
			case AOP_DECREASE:	printf("-- "); break;
			}
			printf(";\n");
			break;
		case SB_MAP:
			printf("map(");
			dump_strlst(&sbptr->u._map.node);
			mptr = sbptr->u._map.default_output;
			if (mptr)
			{
				printf(", ");
				dump_strlst(&mptr->pattern);
				printf(",");
				dump_strlst(&mptr->output);
			}
			for (mptr=sbptr->u._map.match; mptr; mptr=mptr->next)
			{
				printf(", ");
				dump_strlst(&mptr->pattern);
				printf(",");
				dump_strlst(&mptr->output);
			}
			printf(");\n");
			break;
		case SB_INCLOG:
			PREFIX(depth);
			printf("inclog(%s, %s);\n", sbptr->u._inclog.format, sbptr->u._inclog.file);
			break;

		case SB_IF:
			printf("if (");
			centry = sbptr->u._if.condition.next;
			while (centry != &(sbptr->u._if.condition))
			{
				cptr = dlist_entry(centry, cond_t, link);
				dump_strlst(&cptr->operand1);
				switch (cptr->op)
				{
				case IFOP_EQUAL:		printf(" == "); break;
				case IFOP_NOTEQUAL:		printf(" != "); break;
				case IFOP_GREATEREQUAL:	printf(" >= "); break;
				case IFOP_LESSEQUAL:	printf(" <= "); break;
				case IFOP_GREATERTHAN:	printf(" > "); break;
				case IFOP_LESSTHAN:		printf(" < "); break;
				}
				dump_strlst(&cptr->operand2);
				if		(cptr->next == IFNEXT_AND) printf(" && ");
				else if (cptr->next == IFNEXT_OR) printf(" || ");
				else break;
				centry = centry->next;
			}
			printf(") {\n");
			dump_sblist(&sbptr->u._if._true, depth+1);

			PREFIX(depth);
			printf("} else {\n");
			dump_sblist(&sbptr->u._if._false, depth+1);

			PREFIX(depth);
			printf("}\n");
			break;

		case SB_FOR:
			printf("for (");
			dump_strlst(&sbptr->u._for.node);
			printf(") {\n");
			dump_sblist(&sbptr->u._for.statement, depth+1);

			PREFIX(depth);
			printf("}\n");
			break;
		case SB_EXIT:
			printf("exit;\n");
			break;
		}
		entry = entry->next;
	}
}
#endif

/******************************************************************/

static char * _get_line_from_file(FILE * file, int strip_new_line)
{
	static const int GROWBY = 80;	/* how large we will grow strings by */
	int ch;
	int idx = 0;
	char * linebuf = NULL;
	int linebufsz = 0;

	while (1)
	{
		ch = fgetc(file);
		if (ch == EOF) break;

		/* gow the line buffer as necessary */
		while (idx > linebufsz - 2)
			linebuf = realloc(linebuf, linebufsz += GROWBY);

		/* strip the tailing newline character if necessary */
		if (strip_new_line && ch == '\n') ch = 0;
		linebuf[idx++] = (char)ch;
		if (ch == '\n' || ch == '\0') break;
	}

	/* if the length is zero, return NULL buffer. */
	if (idx == 0) return NULL;

	linebuf[idx] = '\0';
	return linebuf;
}

/******************************************************************/

static int parse_assign(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_require(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_anchor(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_queryjs(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_query(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_del(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_set(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_echo(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_map(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_inclog(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_if(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_for(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_exit(int outfd, struct dlist_head * shead, const char ** ptr);
static int parse_ephp(int outfd, struct dlist_head * shead, const char ** ptr);

#define SHOW_GO(msg, label)	{ b_puts(msg, outfd); goto label; }

static int parse_one_strlst(int outfd, struct dlist_head * head, const char ** ptr)
{
	const char * curr = xmldb_eatwhite(*ptr);

	/* check for starting '(' */
	if (*curr != '(') SHOW_GO("SYNTAX ERROR: missing '(' !!", parse_one_strlst_error);
	curr++;
	/* read strlst */
	if (read_strlst(head, &curr, ".", ")")==NULL)
		SHOW_GO("SYNTAX ERROR !!", parse_one_strlst_error);
	/* check for ending ')' */
	curr = xmldb_eatwhite(curr);
	if (*curr != ')') SHOW_GO("SYNTAX ERROR: missing ')' !!", parse_one_strlst_error);
	/* check for ending ';' */
	curr = xmldb_eatwhite(curr+1);
	if (*curr != ';') SHOW_GO("SYNTAX ERROR: missing ';' !!", parse_one_strlst_error);
	*ptr = curr+1;
	return 0;

parse_one_strlst_error:
	*ptr = curr;
	return -1;
}

/* parse variable assignment */
static int parse_assign(int outfd, struct dlist_head * shead, const char ** ptr)
{
	char * str = NULL;
	const char * curr;
	sblock_t * sbptr;
	int quote;

	d_dbg("parse_assign ...\n");

	/* allocate a new sblock_t first */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_assign_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_ASSIGN;
	INIT_DLIST_HEAD(&sbptr->link);
	INIT_DLIST_HEAD(&sbptr->u._assign.value);

	/* read varaible name */
	curr = *ptr;
	str = read_string_ex(&curr, "=+-", &quote);
	if (strlen(str)==0) SHOW_GO("SYNTAX ERROR: no variable name !!", parse_assign_error);
	/* save variable name */
	sbptr->u._assign.variable = strdup(xmldb_reatwhite(str));
	/* check op */
	curr = xmldb_eatwhite(curr);
	if		(strncmp(curr, "+=", 2)==0)	{ sbptr->u._assign.op = AOP_ADD; curr+=2; }
	else if	(strncmp(curr, "-=", 2)==0)	{ sbptr->u._assign.op = AOP_SUB; curr+=2; }
	else if (strncmp(curr, "++", 2)==0)	{ sbptr->u._assign.op = AOP_INCREASE; curr+=2; }
	else if (strncmp(curr, "--", 2)==0)	{ sbptr->u._assign.op = AOP_DECREASE; curr+=2; }
	else if (*curr == '=')				{ sbptr->u._assign.op = AOP_ASSIGN; curr++; }
	else SHOW_GO("SYNTAX ERROR: unknown operator !!", parse_assign_error);
	/* read value string */
	curr = xmldb_eatwhite(curr);
	if (read_strlst(&sbptr->u._assign.value, &curr, ".+-*/%", ";")==NULL)
		SHOW_GO("SYNTAX ERROR !!", parse_assign_error);
	/* check for ';' */
	if (*curr != ';') SHOW_GO("SYNTAX ERROR: missing ';' !!", parse_assign_error);

	dlist_add_tail(&sbptr->link, shead);
	d_dbg("new assign : sbptr=0x%x\n", sbptr);
	*ptr = curr+1;
	return 0;

parse_assign_error:
	if (sbptr)
	{
		SAVE_FREE(sbptr->u._assign.variable);
		destroy_strlst(&sbptr->u._assign.value);
		free(sbptr);
	}
	*ptr = curr;
	return -1;
}
static int parse_require(int outfd, struct dlist_head * shead, const char ** ptr)
{
	sblock_t * sbptr;

	d_dbg("parse_require ...\n");

	/* allocate a new sblock_t */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_require_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_REQUIRE;
	INIT_DLIST_HEAD(&sbptr->u._require.file);

	if (parse_one_strlst(outfd, &sbptr->u._require.file, ptr) < 0)
		goto parse_require_error;

	dlist_add_tail(&sbptr->link, shead);
	d_dbg("new require : sbptr=0x%x\n",sbptr);
	return 0;

parse_require_error:
	if (sbptr)
	{
		destroy_strlst(&sbptr->u._require.file);
		free(sbptr);
	}
	return -1;
}
static int parse_anchor(int outfd, struct dlist_head * shead, const char ** ptr)
{
	sblock_t * sbptr;

	d_dbg("parse_anchor ...\n");

	/* allocate a new sblock_t */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_anchor_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_ANCHOR;
	INIT_DLIST_HEAD(&sbptr->u._anchor.node);

	if (parse_one_strlst(outfd, &sbptr->u._anchor.node, ptr) < 0)
		goto parse_anchor_error;

	dlist_add_tail(&sbptr->link, shead);
	d_dbg("new anchor : sbptr=0x%x\n", sbptr);
	return 0;

parse_anchor_error:
	if (sbptr)
	{
		destroy_strlst(&sbptr->u._anchor.node);
		free(sbptr);
	}
	return -1;
}
static int _parse_query(int outfd, struct dlist_head * shead, const char ** ptr, int js)
{
	sblock_t * sbptr;

	d_dbg("parse_query ...\n");

	/* allocate a new sblock_t */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_query_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = js ? SB_QUERYJS : SB_QUERY;
	INIT_DLIST_HEAD(&sbptr->u._query.node);

	if (parse_one_strlst(outfd, &sbptr->u._query.node, ptr) < 0)
		goto parse_query_error;

	dlist_add_tail(&sbptr->link, shead);
	d_dbg("new query : sbptr=0x%x\n", sbptr);
	return 0;

parse_query_error:
	if (sbptr)
	{
		destroy_strlst(&sbptr->u._query.node);
		free(sbptr);
	}
	return -1;
}
static int parse_query(int outfd, struct dlist_head * shead, const char ** ptr)
{
	return _parse_query(outfd, shead, ptr, 0);
}
static int parse_queryjs(int outfd, struct dlist_head * shead, const char ** ptr)
{
	return _parse_query(outfd, shead, ptr, 1);
}
static int parse_del(int outfd, struct dlist_head * shead, const char ** ptr)
{
	sblock_t * sbptr;

	d_dbg("parse_del ...\n");

	/* allocate a new sblock_t */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_del_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_DEL;
	INIT_DLIST_HEAD(&sbptr->u._del.node);

	if (parse_one_strlst(outfd, &sbptr->u._del.node, ptr) < 0)
		goto parse_del_error;

	dlist_add_tail(&sbptr->link, shead);
	d_dbg("new del: sbptr=0x%x\n", sbptr);
	return 0;

parse_del_error:
	if (sbptr)
	{
		destroy_strlst(&sbptr->u._del.node);
		free(sbptr);
	}
	return -1;
}
static int parse_set(int outfd, struct dlist_head * shead, const char ** ptr)
{
	const char * curr;
	sblock_t * sbptr;

	d_dbg("parse_set ...\n");

	/* allocate a new sblock_t first */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_set_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_SET;
	INIT_DLIST_HEAD(&sbptr->u._set.node);
	INIT_DLIST_HEAD(&sbptr->u._set.value);

	/* check for starting '(' */
	curr = xmldb_eatwhite(*ptr);
	if (*curr != '(') SHOW_GO("SYNTAX ERROR: missing '(' !!", parse_set_error);
	curr++;
	/* read node string */
	if (read_strlst(&sbptr->u._set.node, &curr, ".", ",")==NULL)
		SHOW_GO("SYNTAX ERROR !!", parse_set_error);
	/* check for ',' */
	curr = xmldb_eatwhite(curr);
	if (*curr != ',') SHOW_GO("SYNTAX ERROR: missing ',' !!", parse_set_error);
	/* read value string */
	curr = xmldb_eatwhite(curr+1);
	if (read_strlst(&sbptr->u._set.value, &curr, ".", ")")==NULL)
		SHOW_GO("SYNTAX ERROR !!", parse_set_error);
	/* check for ending ')' */
	curr = xmldb_eatwhite(curr);
	if (*curr != ')') SHOW_GO("SYNTAX ERROR: missing ')' !!", parse_set_error);
	/* check for ';' */
	curr = xmldb_eatwhite(curr+1);
	if (*curr != ';') SHOW_GO("SYNTAX ERROR: missing ';' !!", parse_set_error);

	dlist_add_tail(&sbptr->link, shead);
	d_dbg("new set : sbptr = 0x%x\n", sbptr);
	*ptr = curr+1;
	return 0;

parse_set_error:
	if (sbptr)
	{
		destroy_strlst(&sbptr->u._set.node);
		destroy_strlst(&sbptr->u._set.value);
		free(sbptr);
	}
	*ptr = curr;
	return -1;
}
static int parse_echo(int outfd, struct dlist_head * shead, const char ** ptr)
{
	const char * curr;
	sblock_t * sbptr;

	d_dbg("parse_echo ...\n");

	curr = xmldb_eatwhite(*ptr);

	/* allocate a new sblock_t */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_echo_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_ECHO;
	INIT_DLIST_HEAD(&sbptr->u._echo.string);

	/* read string */
	if (read_strlst(&sbptr->u._echo.string, &curr, ".", ";")==NULL)
		SHOW_GO("SYNTAX ERROR !!", parse_echo_error);
	/* check for ending ';' */
	if (*curr != ';') SHOW_GO("SYNTAX ERROR: missing ';' !!", parse_echo_error);

	dlist_add_tail(&sbptr->link, shead);
	d_dbg("new echo : sbptr=0x%x\n", sbptr);
	*ptr = curr+1;
	return 0;

parse_echo_error:
	if (sbptr)
	{
		destroy_strlst(&sbptr->u._echo.string);
		free(sbptr);
	}
	*ptr = curr;
	return -1;
}
static int parse_map(int outfd, struct dlist_head * shead, const char ** ptr)
{
	const char * curr;
	sblock_t * sbptr;
	match_t * mptr = NULL;
	strlist_t * slist;

	d_dbg("parse_map ...\n");

	/* allocate a new sblock_t first */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_map_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_MAP;
	INIT_DLIST_HEAD(&sbptr->u._map.node);

	/* check for starting '(' */
	curr = xmldb_eatwhite(*ptr);
	if (*curr != '(') SHOW_GO("SYNTAX ERROR: missing '(' !!", parse_map_error);
	curr++;
	/* read node string */
	if (read_strlst(&sbptr->u._map.node, &curr, ".", ",")==NULL)
		SHOW_GO("SYNTAX ERROR !!", parse_map_error);
	/* start to read map messages */
	curr = xmldb_eatwhite(curr);
	while (*curr)
	{
		/* check ending ')' */
		if (*curr == ')') { curr++; break; }
		/* check for ',' */
		if (*curr != ',') SHOW_GO("SYNTAX ERROR: missing ',' !!", parse_map_error);
		curr++;
		/* alloc a match_t for this match */
		mptr = (match_t *)malloc(sizeof(match_t));
		if (!mptr) SHOW_GO("MALLOC ERROR!!", parse_map_error);
		memset(mptr, 0, sizeof(match_t));
		INIT_DLIST_HEAD(&mptr->pattern);
		INIT_DLIST_HEAD(&mptr->output);
		/* read pattern */
		if (read_strlst(&mptr->pattern, &curr, ".", ",")==NULL)
			SHOW_GO("SYNTAX ERROR !!", parse_map_error);
		/* check for ',' */
		curr = xmldb_eatwhite(curr);
		if (*curr != ',') SHOW_GO("SYNTAX ERROR: missing ',' !!", parse_map_error);
		curr++;
		/* read output */
		if (read_strlst(&mptr->output, &curr, ".", ",)")==NULL)
			SHOW_GO("SYNTAX ERROR !!", parse_map_error);
		curr = xmldb_eatwhite(curr);
		/* if the pattern is empty, ignore this match */
		if (dlist_empty(&mptr->pattern)) { free_match_t(mptr); }
		else
		{
			slist = dlist_entry(mptr->pattern.next, strlist_t, link);
			/* check if this is the default output */
			if (slist->type == ST_STR && strcmp(slist->u.str, "*")==0)
			{
				free_match_t(sbptr->u._map.default_output);
				sbptr->u._map.default_output = mptr;
			}
			else
			{
				mptr->next = sbptr->u._map.match;
				sbptr->u._map.match = mptr;
			}
		}
		mptr = NULL;
		curr = xmldb_eatwhite(curr);
	}
	/* check for ';' */
	curr = xmldb_eatwhite(curr);
	if (*curr != ';') SHOW_GO("SYNTAX ERROR: missing ';' !!", parse_map_error);
	curr++;

	dlist_add_tail(&sbptr->link, shead);
	d_dbg("new map : sbptr=0x%x\n", sbptr);
	*ptr = curr;
	return 0;

parse_map_error:
	free_match_t(mptr);
	if (sbptr)
	{
		destroy_strlst(&sbptr->u._map.node);
		free_match_t(sbptr->u._map.default_output);
		destroy_match_list(sbptr->u._map.match);
		free(sbptr);
	}
	*ptr = curr;
	return -1;
}
static int parse_inclog(int outfd, struct dlist_head * shead, const char ** ptr)
{
	char * string;
	const char * curr;
	sblock_t * sbptr;
	int quote;

	d_dbg("parse_inclog ..\n");

	/* allocate a new sblock_t first */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_inclog_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_INCLOG;

	/* check for starting '(' */
	curr = xmldb_eatwhite(*ptr);
	if (*curr != '(') SHOW_GO("SYNTAX ERROR : missing '(' !!", parse_inclog_error);
	/* read format string */
	curr = xmldb_eatwhite(curr+1);
	string = read_string(&curr, ',', &quote);
	d_dbg("format string = [%s]\n", string);
	/* save format string */
	sbptr->u._inclog.format = strdup(string);
	/* check for ',' */
	curr = xmldb_eatwhite(curr);
	if (*curr != ',') SHOW_GO("SYNTAX ERROR : missing ',' !!", parse_inclog_error);
	/* read file name */
	curr = xmldb_eatwhite(curr+1);
	string = read_string(&curr, ')', &quote);
	d_dbg("file name = [%s]\n", string);
	/* save file name */
	sbptr->u._inclog.file = strdup(string);
	/* check for ending ')' */
	curr = xmldb_eatwhite(curr);
	if (*curr != ')') SHOW_GO("SYNTAX ERROR : missing ')' !!", parse_inclog_error);
	/* check for ';' */
	curr = xmldb_eatwhite(curr+1);
	if (*curr != ';') SHOW_GO("SYNTAX ERROR : missing ';' !!", parse_inclog_error);
	curr++;

	dlist_add_tail(&sbptr->link, shead);
	d_dbg("new inclog : sbptr = 0x%x, format=0x%x(%s), file=0x%x(%s)\n", sbptr,
			sbptr->u._inclog.format, sbptr->u._inclog.format, sbptr->u._inclog.file, sbptr->u._inclog.file);
	*ptr = curr;
	return 0;

parse_inclog_error:
	if (sbptr)
	{
		SAVE_FREE(sbptr->u._inclog.format);
		SAVE_FREE(sbptr->u._inclog.file);
		free(sbptr);
	}
	*ptr = curr;
	return -1;
}
static int parse_if(int outfd, struct dlist_head * shead, const char ** ptr)
{
	int ret;
	const char * curr;
	sblock_t * sbptr;
	cond_t * cptr = NULL;

	d_dbg("parse_if ...\n");

	/* allocate a new sblock_t */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_if_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_IF;
	INIT_DLIST_HEAD(&sbptr->u._if.condition);
	INIT_DLIST_HEAD(&sbptr->u._if._true);
	INIT_DLIST_HEAD(&sbptr->u._if._false);

	/* check for starting '(' */
	curr = xmldb_eatwhite(*ptr);
	if (*curr != '(') SHOW_GO("SYNTAX ERROR: missing '(' !!", parse_if_error);
	curr++;

	/* enter read condition loop */
	do
	{
		/* allocate structure cond_t */
		cptr = (cond_t *)malloc(sizeof(cond_t));
		if (!cptr) SHOW_GO("MALLOC ERROR !!", parse_if_error);
		memset(cptr, 0, sizeof(cond_t));
		INIT_DLIST_HEAD(&cptr->operand1);
		INIT_DLIST_HEAD(&cptr->operand2);
		/* read condition operand1 */
		curr = xmldb_eatwhite(curr);
		if (read_strlst(&cptr->operand1, &curr, ".+-*/%", "!=<>")==NULL)
			SHOW_GO("SYNTAX ERROR !!", parse_if_error);
		/* read operator */
		curr = xmldb_eatwhite(curr);
		if		(strncmp(curr, "==", 2)==0)	{ cptr->op = IFOP_EQUAL; curr+=2; }
		else if	(strncmp(curr, "!=", 2)==0) { cptr->op = IFOP_NOTEQUAL; curr+=2; }
		else if (strncmp(curr, "<=", 2)==0) { cptr->op = IFOP_LESSEQUAL; curr+=2; }
		else if (strncmp(curr, ">=", 2)==0) { cptr->op = IFOP_GREATEREQUAL; curr+=2; }
		else if (curr[0] == '>') { cptr->op = IFOP_GREATERTHAN; curr++; }
		else if (curr[0] == '<') { cptr->op = IFOP_LESSTHAN; curr++; }
		else SHOW_GO("SYNTAX ERROR: unknown operator !!", parse_if_error);
		/* read condition operand2 */
		curr = xmldb_eatwhite(curr);
		if (read_strlst(&cptr->operand2, &curr, ".+-*/%", "|&)")==NULL)
			SHOW_GO("SYNTAX ERROR !!", parse_if_error);
		/* is there any ..... */
		curr = xmldb_eatwhite(curr);
		if		(strncmp(curr, "||", 2)==0)	{ cptr->next = IFNEXT_OR; curr+=2; }
		else if	(strncmp(curr, "&&", 2)==0) { cptr->next = IFNEXT_AND; curr+=2; }
		else { cptr->next = IFNEXT_END; }

		dlist_add_tail(&cptr->link, &sbptr->u._if.condition);
		if (cptr->next == IFNEXT_END) { cptr = NULL; break; }
		cptr = NULL;

	} while (*curr);

	/* check for ')' */
	curr = xmldb_eatwhite(curr);
	if (*curr != ')') SHOW_GO("SYNATX ERROR: missing ')' !!", parse_if_error); 
	/* check for '{' */
	curr = xmldb_eatwhite(curr+1);
	if (*curr != '{') SHOW_GO("SYNTAX ERROR: missing '{' !!", parse_if_error);
	curr++;
	ret = parse_ephp(outfd, &sbptr->u._if._true, &curr);
	if (ret < 0) goto parse_if_error;
	/* check for '}' */
	if (*curr != '}') SHOW_GO("SYNTAX ERROR: missing '}' !!", parse_if_error);
	/* check for 'else' */
	curr = xmldb_eatwhite(curr+1);
	if (strncmp(curr, "else", 4))
	{
		dlist_add_tail(&sbptr->link, shead);
		goto parse_if_success;
	}
	curr+=4;

	/* check for '{' or 'if' */
	curr = xmldb_eatwhite(curr);
	if (*curr == '{')
	{
		curr++;
		ret = parse_ephp(outfd, &sbptr->u._if._false, &curr);
		if (ret < 0) goto parse_if_error;
		/* check for '}' */
		if (*curr != '}') SHOW_GO("SYNTAX ERROR: missing '}' !!", parse_if_error);
		curr++;
		dlist_add_tail(&sbptr->link, shead);
		goto parse_if_success;
	}
	if (curr[0]=='i' && curr[1]=='f')
	{
		curr+=2;
		ret = parse_if(outfd, &sbptr->u._if._false, &curr);
		if (ret < 0) goto parse_if_error;
		dlist_add_tail(&sbptr->link, shead);
	}

parse_if_success:
	*ptr = curr;
	return 0;

parse_if_error:
	free_cond_t(cptr);
	if (sbptr)
	{
		destroy_cond_list(&sbptr->u._if.condition);
		destroy_sblist(&sbptr->u._if._true);
		destroy_sblist(&sbptr->u._if._false);
		free(sbptr);
	}
	*ptr = curr;
	return -1;
}
static int parse_for(int outfd, struct dlist_head * shead, const char ** ptr)
{
	int ret;
	const char * curr;
	sblock_t * sbptr;

	d_dbg("parse_for ...\n");

	/* allocate a new sblock_t */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_for_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_FOR;
	INIT_DLIST_HEAD(&sbptr->u._for.node);
	INIT_DLIST_HEAD(&sbptr->u._for.statement);

	/* check for starting '(' */
	curr = xmldb_eatwhite(*ptr);
	if (*curr != '(') SHOW_GO("SYNTAX ERROR: missing '(' !!", parse_for_error);
	curr++;
	/* read node string */
	if (read_strlst(&sbptr->u._for.node, &curr, ".", ")")==NULL)
		SHOW_GO("SYNTAX ERROR !!", parse_for_error);
	/* check for ')' */
	curr = xmldb_eatwhite(curr);
	if (*curr != ')') SHOW_GO("SYNTAX ERROR: missing ')' !!", parse_for_error);
	/* check for '{' */
	curr = xmldb_eatwhite(curr+1);
	if (*curr != '{') SHOW_GO("SYNTAX ERROR: missing '{' !!", parse_for_error);
	curr++;
	ret = parse_ephp(outfd, &sbptr->u._for.statement, &curr);
	if (ret < 0) goto parse_for_error;
	/* check for '}' */
	if (*curr != '}') SHOW_GO("SYNTAX ERROR: missing '}' !!", parse_for_error);
	curr++;

	dlist_add_tail(&sbptr->link, shead);
	*ptr = curr;
	return 0;

parse_for_error:
	if (sbptr)
	{
		destroy_strlst(&sbptr->u._for.node);
		destroy_sblist(&sbptr->u._for.statement);
		free(sbptr);
	}
	*ptr = curr;
	return -1;
}
static int parse_exit(int outfd, struct dlist_head * shead, const char ** ptr)
{
	const char * curr;
	sblock_t * sbptr;

	d_dbg("parse_exit ...\n");

	/* allocate a new sblock_t */
	sbptr = (sblock_t *)malloc(sizeof(sblock_t));
	if (!sbptr) SHOW_GO("MALLOC ERROR !!", parse_exit_error);
	memset(sbptr, 0, sizeof(sblock_t));
	sbptr->type = SB_EXIT;

	/* check for ';' */
	curr = xmldb_eatwhite(*ptr);
	if (*curr != ';') SHOW_GO("SYNTAX ERROR : missing ';' !!", parse_exit_error);
	curr++;

	dlist_add_tail(&sbptr->link, shead);
	d_dbg("new exit : sbptr=0x%x\n", sbptr);
	*ptr = curr;
	return 0;

parse_exit_error:
	SAVE_FREE(sbptr);
	return -1;
}
static int parse_ephp(int outfd, struct dlist_head * shead, const char ** ptr)
{
	const char * curr;
	int ret = 0;

	curr = xmldb_eatwhite(*ptr);

	while (*curr)
	{
		if (*curr == '}')
		{
			ret = 0;
			break;
		}
		else if (*curr == '$')
		{
			curr++;
			ret = parse_assign(outfd, shead, &curr);
		}
		else if (strncmp(curr, "require", 7)==0)
		{
			curr += 7;
			ret = parse_require(outfd, shead, &curr);
		}
		else if (strncmp(curr, "anchor", 6)==0)
		{
			curr += 6;
			ret = parse_anchor(outfd, shead, &curr);
		}
		else if (strncmp(curr, "queryjs", 7)==0)
		{
			curr += 7;
			ret = parse_queryjs(outfd, shead, &curr);
		}
		else if (strncmp(curr, "query", 5)==0)
		{
			curr += 5;
			ret = parse_query(outfd, shead, &curr);
		}
		else if (strncmp(curr, "del", 3)==0)
		{
			curr += 3;
			ret = parse_del(outfd, shead, &curr);
		}
		else if (strncmp(curr, "set", 3)==0)
		{
			curr += 3;
			ret = parse_set(outfd, shead, &curr);
		}
		else if (strncmp(curr, "echo", 4)==0)
		{
			curr += 4;
			ret = parse_echo(outfd, shead, &curr);
		}
		else if (strncmp(curr, "map", 3)==0)
		{
			curr += 3;
			ret = parse_map(outfd, shead, &curr);
		}
		else if (strncmp(curr, "inclog", 6)==0)
		{
			curr += 6;
			ret = parse_inclog(outfd, shead, &curr);
		}
		else if (strncmp(curr, "if", 2)==0)
		{
			curr += 2;
			ret = parse_if(outfd, shead, &curr);
		}
		else if (strncmp(curr, "for", 3)==0)
		{
			curr += 3;
			ret = parse_for(outfd, shead, &curr);
		}
		else if (strncmp(curr, "exit", 4)==0)
		{
			curr += 4;
			ret = parse_exit(outfd, shead, &curr);
		}
		else
		{
			b_puts("EPHP: syntax error!!!\n", outfd);
			b_puts(curr, outfd);
			ret = -1;
			break;
		}

		if (ret)
		{
			b_puts("EPHP: syntax error!!!!\n", outfd);
			b_puts(curr, outfd);
			break;
		}
		curr = xmldb_eatwhite(curr);
	}

	*ptr = curr;
	return ret;
}

/**************************************************************************/

static struct xml_node * g_anchor = NULL; 

static int do_embeded_php(int outfd, FILE * ephpfd);
static int execute_require(int outfd, sblock_t * sbptr);
static int execute_anchor(int outfd, sblock_t * sbptr);
static int execute_queryjs(int outfd, sblock_t * sbptr);
static int execute_query(int outfd, sblock_t * sbptr);
static int execute_del(int outfd, sblock_t * sbptr);
static int execute_set(int outfd, sblock_t * sbptr);
static int execute_echo(int outfd, sblock_t * sbptr);
static int execute_assign(int outfd, sblock_t * sbptr);
static int execute_map(int outfd, sblock_t * sbptr);
static int execute_inclog(int outfd, sblock_t * sbptr);
static int execute_if(int outfd, sblock_t * sbptr);
static int execute_for(int outfd, sblock_t * sbptr);
static int execute_sblist(int outfd, struct dlist_head * shead);

/* */
static char * query_node(const char * node, int js)
{
	const char * str;
	char value[512];
	int i=0;

	str = xmldb_get_value(g_anchor, node);
	if (js)
	{
		while (*str && i < sizeof(value)-1)
		{
			if (*str == '"' || *str == '$' || *str == '\\') value[i++] = '\\';
			value[i++] = *str++;
		}
		value[i] = '\0';
		str = value;
	}
	return strdup(str);
}

/* */
static char * convert_strlst(struct dlist_head * head)
{
	struct dlist_head * entry;
	strlist_t * slist;
	char result[512];
	char final[MAX_STRING_LEN];
	char op = 0;
	char * node, * value;
	int js;

	memset(final, 0, sizeof(final));
	
	entry = head->next;
	while (entry != head)
	{
		js = 0;
		slist = dlist_entry(entry, strlist_t, link);
		switch (slist->type)
		{
		case ST_STR:
			strncpy(result, slist->u.str, sizeof(result)-1);
			break;
		case ST_VAR:
			strncpy(result, get_var_value(slist->u.str), sizeof(result)-1);
			break;
		case ST_QUERYJS:
			js = 1;
		case ST_QUERY:
			node = convert_strlst(&slist->u.slist);
			if (node) value = query_node(node, js);
			else value = NULL;
			if (value) strncpy(result, value, sizeof(result)-1);
			else memset(result, 0, sizeof(result));
			SAVE_FREE(node);
			SAVE_FREE(value);
			break;
		}
		switch (op)
		{
		case '.':	strcat(final, result); break;
		case '+':	sprintf(final, "%d", atoi(final)+atoi(result)); break;
		case '-':	sprintf(final, "%d", atoi(final)-atoi(result)); break;
		case '*':	sprintf(final, "%d", atoi(final)*atoi(result)); break;
		case '/':	if (atoi(result)) sprintf(final, "%d", atoi(final)/atoi(result)); break;
		case '%':	sprintf(final, "%d", atoi(final)%atoi(result)); break;
		default:	strcpy(final, result); break;
		}
		op = find_pattern(slist->op, ".+-*/%");
		if (!op) break;
		entry = entry->next;
	}
	return strdup(final);
}

static int check_condition(struct dlist_head * head)
{
	cond_t * cptr;
	struct dlist_head * entry;
	char * op1, * op2;
	int ret = -1;

	entry = head->next;
	while (entry != head)
	{
		cptr = dlist_entry(entry, cond_t, link);
		op1 = convert_strlst(&cptr->operand1);
		op2 = convert_strlst(&cptr->operand2);
		if (!op1 || !op2)
		{
			SAVE_FREE(op1);
			SAVE_FREE(op2);
			break;
		}
		switch (cptr->op)
		{
		case IFOP_EQUAL:		ret = (strcmp(op1, op2)==0)    ? 1:0; break;
		case IFOP_NOTEQUAL:		ret = (strcmp(op1, op2)!=0)    ? 1:0; break;
		case IFOP_GREATERTHAN:	ret = (atoi(op1) > atoi(op2))  ? 1:0; break;
		case IFOP_LESSTHAN:		ret = (atoi(op1) < atoi(op2))  ? 1:0; break;
		case IFOP_GREATEREQUAL:	ret = (atoi(op1) >= atoi(op2)) ? 1:0; break;
		case IFOP_LESSEQUAL:	ret = (atoi(op1) <= atoi(op2)) ? 1:0; break;
		}
		free(op1); free(op2);
		if (cptr->next == IFNEXT_AND && !ret) break;
		if (cptr->next == IFNEXT_OR && ret) break;
		if (cptr->next == IFNEXT_END) break;
		entry = entry->next;
	}
	return ret;
}

static int execute_require(int outfd, sblock_t * sbptr)
{
	int ret = -1;
	FILE * fd;
	char * fname;

	fname = convert_strlst(&sbptr->u._require.file);
	if (fname)
	{
		d_dbg(">>> require(%s)\n", fname);
		fd = fopen(fname, "r");
		if (fd)
		{
			ret = do_embeded_php(outfd, fd);
			fclose(fd);
		}
		free(fname);
	}
	return ret;
}
static int execute_anchor(int outfd, sblock_t * sbptr)
{
	int ret = -1;
	struct xml_node * root;
	char * node;

	node = convert_strlst(&sbptr->u._anchor.node);
	if (node)
	{
		d_dbg(">>> anchor(%s)\n", node);
		root = (node[0] == '/') ? NULL : g_anchor;
		g_anchor = xmldb_find_node(root, node, 0);
		ret = 0;
		free(node);
	}
	return ret;
}
static int _exec_query(int outfd, sblock_t * sbptr, int js)
{
	int ret = -1;
	char * node, * value;

	node = convert_strlst(&sbptr->u._query.node);
	if (node)
	{
		d_dbg(">>> query%s(%s)\n", js ? "js" : "", node);
		value = query_node(node, js);
		if (value)
		{
			b_puts(value, outfd);
			free(value);
			ret = 0;
		}
		free(node);
	}
	return ret;
}
static int execute_query(int outfd, sblock_t * sbptr)
{
	return _exec_query(outfd, sbptr, 0);
}
static int execute_queryjs(int outfd, sblock_t * sbptr)
{
	return _exec_query(outfd, sbptr, 1);
}
static int execute_del(int outfd, sblock_t * sbptr)
{
	int ret = -1;
	char * node;

	node = convert_strlst(&sbptr->u._del.node);
	if (node)
	{
		d_dbg(">>> del(%s)\n", node);
		xmldb_del_node(g_anchor, node);
		ret = 0;
		free(node);
	}
	return ret;
}
static int execute_set(int outfd, sblock_t * sbptr)
{
	int ret = -1;
	char * node, * value;

	node = convert_strlst(&sbptr->u._set.node);
	value = convert_strlst(&sbptr->u._set.value);
	if (node && value)
	{
		d_dbg(">>> set(%s, %s)\n", node, value);
		xmldb_set_value(g_anchor, node, value);
		ret = 0;
	}
	SAVE_FREE(node);
	SAVE_FREE(value);
	return ret;
}
static int execute_echo(int outfd, sblock_t * sbptr)
{
	int ret = -1;
	char * str;

	str = convert_strlst(&sbptr->u._echo.string);
	if (str)
	{
		d_dbg(">>> echo(%s)\n", str);
		b_puts(str, outfd);
		free(str);
		ret = 0;
	}
	return 0;
}
static int execute_assign(int outfd, sblock_t * sbptr)
{
	int ret = -1;
	char * str;
	char buf[MAX_STRING_LEN];

	
	str = convert_strlst(&sbptr->u._assign.value);
	if (str)
	{
		switch (sbptr->u._assign.op)
		{
		default:
		case AOP_ASSIGN:	strcpy(buf, str); break;
		case AOP_ADD:		sprintf(buf, "%d", atoi(get_var_value(sbptr->u._assign.variable)) + atoi(str)); break;
		case AOP_SUB:		sprintf(buf, "%d", atoi(get_var_value(sbptr->u._assign.variable)) - atoi(str)); break;
		case AOP_INCREASE:	sprintf(buf, "%d", atoi(get_var_value(sbptr->u._assign.variable)) + 1); break;
		case AOP_DECREASE:	sprintf(buf, "%d", atoi(get_var_value(sbptr->u._assign.variable)) - 1); break;
		}
		free(str);
		ret = 0;

		d_dbg(">>> $%s = %s\n", sbptr->u._assign.variable, buf);
		set_var(sbptr->u._assign.variable, buf);
	}
	return ret;
}
static int execute_map(int outfd, sblock_t * sbptr)
{
	int ret = -1;
	char * node, * value, * patt;
	char * output = NULL;
	match_t * mptr;

	node = convert_strlst(&sbptr->u._map.node);
	if (node)
	{
		value = query_node(node, 0);
		if (value)
		{
			for (mptr=sbptr->u._map.match; mptr && !output; mptr=mptr->next)
			{
				patt = convert_strlst(&mptr->pattern);
				if (patt)
				{
					if (strcmp(patt, value)==0)
					{
						output = convert_strlst(&mptr->output);
						d_dbg(">>> map [%s] to [%s]\n", value, output);
						break;
					}
					free(patt);
				}
			}
			if (!output && sbptr->u._map.default_output)
			{
				output = convert_strlst(&sbptr->u._map.default_output->output);
				d_dbg(">>> use default [%s]\n", output);
			}

			b_puts(output ? output : value, outfd);
			free(value);
			ret = 0;
		}
		free(node);
	}
	SAVE_FREE(output);
	return ret;
}

extern int o_tlogs;
static int execute_inclog(int outfd, sblock_t * sbptr)
{
	FILE * fh;
	char * line;
	char * field[16];
	char * curr;
	int i, idx;
	char cmd[256];

	d_dbg("execute_inclog(%s)\n", sbptr->u._inclog.file);

	if (o_tlogs)
	{
		d_dbg("use tlogs to translate log message !!\n");
		sprintf(cmd, "tlogs -l %s > /var/log/tlogsmsg", sbptr->u._inclog.file);
		system(cmd);
		fh = fopen("/var/log/tlogsmsg", "r");
	}
	else
	{
		d_dbg("include raw log file!!\n");
		fh = fopen(sbptr->u._inclog.file, "r");
	}
	
	if (fh)
	{
		while ((line = _get_line_from_file(fh, 1)) != NULL)
		{
			memset(field, 0, sizeof(field));
			i = 0;
			curr = (char *)xmldb_eatwhite(line);
			do
			{
				field[i] = curr;
				curr = strchr(curr, '|');
				if (curr)
				{
					*curr = '\0';
					curr = (char *)xmldb_eatwhite(curr+1);
				}
				xmldb_reatwhite(field[i]);
				i++;
			} while (curr);

#if 0
			{
				for (idx=0; idx < i; idx++)
				{
					d_dbg("field %d [%s]\n", idx, field[idx]);
				}
			}
#endif

			/* now output the log message. */
			curr = sbptr->u._inclog.format;
			d_dbg("format : [%s]\n", curr);
			while (*curr)
			{
				if (*curr == '%')
				{
					curr++;
					d_dbg("escape %c\n", *curr);
					if (*curr >= '0' && *curr <= '9')
					{
						idx = *curr - '0';
						d_dbg("field %d: [%s]\n", idx, field[idx]);
						b_puts(field[idx], outfd);
						curr++;
					}
					else if (*curr)
					{
						b_putc(*curr++, outfd);
					}
				}
				else
				{
					b_putc(*curr++, outfd);
				}
			}
			b_putc('\n', outfd);

			free(line);
		}
		fclose(fh);
	}

	return 0;
}
static int execute_if(int outfd, sblock_t * sbptr)
{
	int ret;

	ret = check_condition(&sbptr->u._if.condition);
	if (ret < 0)
	{
		return -1;
	}

	if (ret)	ret = execute_sblist(outfd, &sbptr->u._if._true);
	else		ret = execute_sblist(outfd, &sbptr->u._if._false);
	return ret;
}
static int execute_for(int outfd, sblock_t * sbptr)
{
	char *path, *entry;
	struct xml_node * pnode;
	struct xml_node * old_anchor;
	int id;
	char num[8];
	char oldnum[8];
	var_t * var;
	int ret = 0;

	d_dbg("execute_for()...\n");
	path = convert_strlst(&sbptr->u._for.node);
	if (!path) { b_puts("MALLOC ERROR !!", outfd); return -1; }

	entry = strrchr(path, '/');
	if (entry) *entry++ = '\0';
	else entry = path;

	d_dbg("execute_for(): path:%s, entry:%s\n", path, entry);

	/* find parent node */
	if (path == entry) pnode = g_anchor;
	else pnode = xmldb_find_node(g_anchor, path, 0);
	if (!pnode)
	{
		d_dbg("execute_for(): entry does not exist!\n");
		free(path);
		return 0;
	}

	/* save old anchor, old $# */
	old_anchor = g_anchor;
	var = get_var("#");
	if (var) strcpy(oldnum, var->value);
	else strcpy(oldnum, "0");

	/* id start at 1. */
	for (id=1; (g_anchor = xmldb_find_sibling(pnode->child, entry, id, 0))!=NULL; id++)
	{
		sprintf(num, "%d", id);
		set_var("#",num);
		ret = execute_sblist(outfd, &sbptr->u._for.statement);
		if (ret != 0) break;
	}

	/* restore old anchor, old $# */
	set_var("#", oldnum);
	g_anchor = old_anchor;

	free(path);
	return ret;
}
static int execute_sblist(int outfd, struct dlist_head * shead)
{
	struct dlist_head * entry;
	sblock_t * sbptr;
	int ret = 0;

	entry = shead->next;
	while (entry != shead)
	{
		sbptr = dlist_entry(entry, sblock_t, link);
		switch (sbptr->type)
		{
		case SB_REQUIRE:	ret = execute_require(outfd, sbptr);	break;
		case SB_ANCHOR:		ret = execute_anchor(outfd, sbptr);		break;
		case SB_QUERY:		ret = execute_query(outfd, sbptr);		break;
		case SB_QUERYJS:	ret = execute_queryjs(outfd, sbptr);	break;
		case SB_DEL:		ret = execute_del(outfd, sbptr);		break;
		case SB_SET:		ret = execute_set(outfd, sbptr);		break;
		case SB_ECHO:		ret = execute_echo(outfd, sbptr);		break;
		case SB_ASSIGN:		ret = execute_assign(outfd, sbptr);		break;
		case SB_MAP:		ret = execute_map(outfd, sbptr);		break;
		case SB_INCLOG:		ret = execute_inclog(outfd, sbptr);		break;
		case SB_IF:			ret = execute_if(outfd, sbptr);			break;
		case SB_FOR:		ret = execute_for(outfd, sbptr);		break;
		case SB_EXIT:		ret = 1; break;
		default:
			b_puts("INTERNAL ERROR: execute_sblist() !!", outfd);
			ret = -1;
			break;
		}
		if (ret != 0) break;
		entry = entry->next;
	}
	return ret;
}

/**************************************************************************/
/* php buffer */

#define PB_INCREASE_STEP	1024

static char * pb_buff = NULL;
static size_t pb_size = 0;
static size_t pb_allocsize = 0;

static int pb_putc(char c)
{
	if (pb_size >= pb_allocsize)
	{
		pb_allocsize += PB_INCREASE_STEP;
		pb_buff = (char *)realloc(pb_buff, pb_allocsize);
		dassert(pb_buff);
	}
	if (pb_buff)
	{
		pb_buff[pb_size++] = c;
		return 0;
	}
	return -1;
}

static int check_format(int outfd, struct dlist_head * shead)
{
	const char * ephp;
	if (pb_buff[0]=='=')
	{
		if (pb_buff[1] != '$')
		{
			b_puts("!! invalid format !!", outfd);
			return -1;
		}
		xmldb_reatwhite(&pb_buff[2]);
		b_puts(get_var_value(&pb_buff[2]), outfd);
		return 1;
	}
	if (strncmp(pb_buff, "ephp", 4)==0) ephp = &pb_buff[4];
	else ephp = pb_buff;

	return parse_ephp(outfd, shead, &ephp);
}

#if 0
static int pb_puts(const char * string)
{
	while (*string)
	{
		if (pb_putc(*string) < 0) return -1;
		string++;
	}
	return 0;
}
#endif

static int do_embeded_php(int outfd, FILE * ephpfd)
{
	int ret = 0;
	int c;
	int state = 0;
	struct dlist_head shead;

	INIT_DLIST_HEAD(&shead);

	do
	{
		if ((c=fgetc(ephpfd))==EOF) break;
		switch (state)
		{
			case 0:	/* looking for starting tag: "<?" */
				if (c == '<')
				{
					if ((c=fgetc(ephpfd))==EOF) break;
					if (c == '?')
					{
						state = 1;
						pb_size = 0;
					}
					else
					{
						b_putc('<', outfd);
						b_putc(c, outfd);
					}
				}
				else
				{
					b_putc(c, outfd);
				}
				break;
				
			case 1:	/* looking for ending tag "?>", and check for comment starting tag. */
				if (c == '/')
				{
					if ((c=fgetc(ephpfd))==EOF) break;
					if (c == '*') state = 2;
					else if (c=='/') state = 3;
					else { pb_putc('/'); pb_putc(c); }
				}
				else if (c == '?')
				{
					if ((c=fgetc(ephpfd))==EOF) break;
					if (c == '>')
					{
						pb_putc(0);
						ret = check_format(outfd, &shead);
						if (ret == 1) ret = 0;
						else if (ret == 0)
						{
#ifdef DUMP_SBLIST
							printf("Dump SB List:\n");
							dump_sblist(&shead, 1);
#endif
							ret = execute_sblist(outfd, &shead);
						}
						destroy_sblist(&shead);
						state = 0;
						if (ret != 0) c = EOF;
					}
					else
					{
						pb_putc('?');
						pb_putc(c);
					}
				}
				else
				{
					pb_putc(c);
				}
				break;

			case 2:	/* into comment, looking for ending tag */
				if (c == '*')
				{
					if ((c=fgetc(ephpfd))==EOF) break;
					if (c == '/') state = 1;
				}
				break;

			case 3:	/* into comment, looking for new line. */
				if (c == '\n' || c == '\r') state = 1;
				break;
		}
	} while (c != EOF);

	return ret;
}

int xmldb_ephp(int fd, char * message, unsigned long flags)
{
	int ret = -1;
	FILE * ephpfd = NULL;
	char * file;
	char * var, * next;
	char * value;

#if 0
	char test[]= "echo_test.php\nvar1=12345\nvar2=67890";
	file = test;
#else
	file = message;
#endif
	var = strchr(file, '\n');
	if (var) *var++ = '\0';

	d_dbg("xmldb_ephp(%s) flags:0x%x\n", file, flags);

	ephpfd = fopen(file, "r");
	if (ephpfd)
	{
		/* set variables */
		set_var("#", "0");

		while (var)
		{
			value = strchr(var, '=');
			if (!value) break;

			*value++ = '\0';
			next = strchr(value, '\n');
			if (next) *next++='\0';
			d_dbg("var(%s)=[%s]\n", var, value);
			set_var(var, value);
			var = next;
		}

		ret = do_embeded_php(fd, ephpfd);
		fclose(ephpfd);

		/* clear all varialbes */
		clear_var();
	}

	b_putc(0, fd);
	b_flush(fd);
	return ret;
}

