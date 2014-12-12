/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "dtrace.h"
#include "xmldb.h"
#include "rgdb.h"

struct map_sect
{
	int num;
	char * default_output;
	char * patt[MAX_MAP_SECTION];
	char * output[MAX_MAP_SECTION];
};

static char g_tag[MAX_PATCH_MSG_LEN];
static int g_var[10];

extern void verbose(const char * format, ...);

/****************************************************************************/

long pi_get_offset(PINFO * pi)
{
	if (pi->fh) return ftell(pi->fh);
	return pi->offset;
}

int pi_set_offset(PINFO * pi, long offset)
{
	if (pi->fh) return fseek(pi->fh, offset, SEEK_SET);
	if (offset < pi->size)
	{
		pi->offset = offset;
		return 0;
	}
	return -1;
}

int pi_getc(PINFO * pi)
{
	if (pi->fh) return fgetc(pi->fh);
	if (pi->offset < pi->size) return (int)(pi->buff[pi->offset++]);
	return EOF;
}

/****************************************************************************/

#define PI_INCREASE_STEP	1024

int pi_putc(PINFO * pi, char c)
{
	//d_dbg("pi_putc(%c), allocsize=%d, size=%d\n", c, pi->allocsize, pi->size);
	if (pi->size >= pi->allocsize)
	{
		pi->allocsize += PI_INCREASE_STEP;
		pi->buff = (char *)realloc(pi->buff, pi->allocsize);
		//d_dbg("alloc buff=0x%x, size = %d\n", pi->buff, pi->allocsize);
	}
	if (pi->buff)
	{
		//d_dbg("pi_putc(%c)\n%s\n", c, pi->buff);
		pi->buff[pi->size++] = c;
		return 0;
	}
	return -1;
}

int pi_puts(PINFO * pi, const char * string)
{
	while (*string)
	{
		if (pi_putc(pi, *string)<0) return -1;
		string++;
	}
	return 0;
}

static int pi_puttag(PINFO * pi, const char * tag)
{
	if (pi_putc(pi, '{')<0) return -1;
	if (pi_putc(pi, '{')<0) return -1;
	while (*tag)
	{
		if (pi_putc(pi, *tag)<0) return -1;
		tag++;
	}
	if (pi_putc(pi, '}')<0) return -1;
	if (pi_putc(pi, '}')<0) return -1;
	return 0;
}

/****************************************************************************/
/* buffered write fd */

static char buffer[1024];
static int count = 0;

static void buffer_flush(int fd)
{
	write(fd, buffer, count);
	count = 0;
}

static void buffer_putc(char c, int fd)
{
	if (count < 1024) buffer[count++] = c;
	if (count == 1024) buffer_flush(fd);
}

/****************************************************************************/

/* read string from *message, if string is contain in ", \ will be used
 * as escape character, else the string is ended with white space, or 'end',
 * if 'end' is non-zero. when exit, message will advance to the end of message
 */
static char * read_string(const char ** message, char end)
{
	static char string[256];
	int i = 0;
	const char * msg = *message;

	//d_dbg("read_string(%s), end with (%c)\n", msg, end);

	/* clear our string buffer */
	memset(string, 0, sizeof(string));

	/* check to see if string contains in " */
	if (*msg == '"')
	{
		/* read string inside "". */
		msg++;
		for (i=0; *msg && *msg!='"'; msg++)
		{
			if (i < 255)
			{
				if (msg[0]=='\\' && msg[1]) msg++;
				string[i++] = *msg;
			}
		}
		if (*msg == '"') *message = msg+1;
		else string[0] = '\0';
	}
	else
	{
		/* read string ended with white space, or 'end' */
		for (i=0; *msg && !IS_WHITE(*msg); msg++)
		{
			if (end && *msg == end) break;
			if (i<255) string[i++] = *msg;
		}
		*message = msg;
	}

	//d_dbg("read_string return(%s)\n", string);
	return string;
}

static int extract_maps(struct map_sect * maps, const char * msg)
{
	int state=0;
	char * pattern=NULL, * output=NULL;

	while (*msg)
	{
		switch (state)
		{
		case 0:	/* section start state, do init. */
			pattern = output = NULL;
			state = 1;
			break;

		case 1:	/* pattern state, extract pattern string. */
			msg = xmldb_eatwhite(msg);
			pattern = strdup(read_string(&msg, ':'));
			msg = xmldb_eatwhite(msg);
			if (*msg==':')
			{
				state = 2;
				msg++;
				break;
			}
			/* invalid map message */
			d_error("extract_maps(): invalid message: [%s]\n", msg);
			goto invalid;
			break;

		case 2:	/* output state, extract output message. */
			msg = xmldb_eatwhite(msg);
			output = strdup(read_string(&msg, ','));
			msg = xmldb_eatwhite(msg);
			if (*msg == ',' || *msg == '\0')
			{
				if (maps->num < MAX_MAP_SECTION)
				{
					if (strcmp(pattern, "*")==0)
					{
						//d_dbg("extract_maps(): add DEFAULT: [%s]\n", output);
						if (maps->default_output) free(maps->default_output);
						maps->default_output = strdup(output);
					}
					else
					{
						//d_dbg("extract_maps(): add map: [%s]->[%s]\n", pattern, output);
						maps->patt[maps->num] = strdup(pattern);
						maps->output[maps->num] = strdup(output);
						maps->num++;
					}
				}
				free(pattern);
				free(output);
				pattern = output = NULL;
				state = 0;
				if (*msg) msg++;
				break;
			}
			/* invalid map message */
			d_error("extract_maps(): invalid message: [%s]\n", msg);
			goto invalid;
			break;
		}
	}
	if (pattern) free(pattern);
	if (output) free(output);
	return maps->num;

invalid:
	if (pattern) free(pattern);
	if (output) free(output);
	return -1;
}

static void clean_map(struct map_sect * maps)
{
	int i;
	for (i=0; i<maps->num; i++)
	{
		if (maps->patt[i]) free(maps->patt[i]);
		if (maps->output[i]) free(maps->output[i]);
		maps->patt[i] = maps->output[i] = NULL;
	}
	if (maps->default_output) free(maps->default_output);
	maps->default_output = NULL;
	maps->num = 0;
}

/****************************************************************************/

/* add escape character for javascript. */
static int _output_js(int fd, const char * value)
{
	while (*value)
	{
		if (*value == '"' || *value == '$' || *value == '\\') buffer_putc('\\', fd);
		buffer_putc(*value++, fd);
	}
	return 0;
}

/* generic output value message */
static int _output(int fd, const char * value, unsigned long flags)
{
	if (flags & RGDB_ESCAPE_JS) return _output_js(fd, value);

	while (*value) buffer_putc(*value++, fd);
	return 0;
}

/* map output */
static int map_output(int fd, const char * value, char * map, unsigned long flags)
{
	struct map_sect maps;
	int i;
	int ret = -1;;

	memset(&maps, 0, sizeof(maps));
	if (extract_maps(&maps, map) < 0) d_error("map_output: invalid map message: [%s]\n", map);
	for (i=0; i<maps.num; i++)
	{
		if (strcmp(value, maps.patt[i])==0)
		{
			ret = _output(fd, maps.output[i], flags);
			clean_map(&maps);
			return ret;
		}
	}
	if (maps.default_output) ret = _output(fd, maps.default_output, flags);
	else ret = _output(fd, value, flags);
	clean_map(&maps);
	return ret;
}

/****************************************************************************/

static int read_tag_msg(PINFO * pi, char * buff, size_t size)
{
	int c=0;
	size_t i=0;
	while (i < size-2)
	{
		c = pi_getc(pi);
		if (c==EOF) break;
		if (c=='}')
		{
			c = pi_getc(pi);
			if (c==EOF) break;
			if (c=='}') break;

			buff[i++] = '}';
			buff[i++] = c;
		}
		else
		{
			buff[i++] = c;
		}
	}
	buff[i] = '\0';
	return c;
}

static int readnum(const char ** ptr, int id, int max)
{
	int i=0;
	const char *num = *ptr;

	if		(num[0]=='#') { i = id; num++; }
	else if	(num[0]=='@') { i = max; num++; }
	else if (strncmp(num, "var", 3)==0)
	{
		i = g_var[(num[3]-'0')%10];
		num += 4;
	}
	else
	{
		while (*num)
		{
			if (*num < '0' || *num > '9') break;
			i = i*10 + (*num - '0');
			num++;
		}
	}
	*ptr = num;
	return i;
}

static int find_pattern(char c, const char * str)
{
	int i;
	for (i=0; str[i] && c!=str[i]; i++);
	return (int)str[i];
}

static int get_node_from_cstring(char * buff, size_t size, char ** cstr)
{
	int i=0;
	char * ptr = *cstr;

	if (buff && size > 0)
	{
		for (i=0; *ptr; ptr++)
		{
			if (find_pattern(*ptr, "+-*/%><=!")) break;
			if (i < size) buff[i++] = *ptr;
		}
		buff[i] = '\0';
	}
	*cstr = ptr;
	return i;
}

static int do_calculate(int v, const char * cstr, int id, int max)
{
	int i;
	while (*cstr)
	{
		cstr = xmldb_eatwhite(cstr);
		if		(strncmp(cstr, "==", 2)==0)
		{
			cstr = xmldb_eatwhite(cstr+2);
			v = (v == readnum(&cstr, id, max)) ? 1 : 0;
		}
		else if	(strncmp(cstr, "!=", 2)==0)
		{
			cstr = xmldb_eatwhite(cstr+2);
			v = (v != readnum(&cstr, id, max)) ? 1 : 0;
		}
		else if (strncmp(cstr, ">=", 2)==0 || strncmp(cstr, "!<", 2)==0)
		{
			cstr = xmldb_eatwhite(cstr+2);
			v = (v >= readnum(&cstr, id, max)) ? 1 : 0;
		}
		else if (strncmp(cstr, "<=", 2)==0 || strncmp(cstr, "!>", 2)==0)
		{
			cstr = xmldb_eatwhite(cstr+2);
			v = (v <= readnum(&cstr, id, max)) ? 1 : 0;
		}
		else
		{
			switch (cstr[0])
			{
			case '+':
				cstr = xmldb_eatwhite(cstr+1);
				v += readnum(&cstr, id, max);
				break;
			case '-':
				cstr = xmldb_eatwhite(cstr+1);
				v -= readnum(&cstr, id, max);
				break;
			case '*':
				cstr = xmldb_eatwhite(cstr+1);
				v *= readnum(&cstr, id, max);
				break;
			case '/':
				cstr = xmldb_eatwhite(cstr+1);
				i = readnum(&cstr, id, max);
				if (i>0) v /= i;
				else d_error("divide by zero!\n");
				break;
			case '%':
				cstr = xmldb_eatwhite(cstr+1);
				v %= readnum(&cstr, id, max);
				break;
			case '>':
				cstr = xmldb_eatwhite(cstr+1);
				v = (v > readnum(&cstr, id, max)) ? 1 : 0;
				break;
			case '<':
				cstr = xmldb_eatwhite(cstr+1);
				v = (v < readnum(&cstr, id, max)) ? 1 : 0;
				break;
			}
		}
	}
	return v;
}

static int do_array_count(PINFO * pi, const char * path)
{
	char * parent, * entry;
	int id;
	struct xml_node * pnode;

	/* prepare parent and entry name */
	parent = strdup(path);
	if (!parent) { verbose("do_array_count: memory alloc fail!\n"); return 0; }
	entry = strrchr(parent, '/');
	if (entry) *entry++ = '\0';
	else entry = parent;
	d_dbg("do_array_count: path=[%s], parent=[%s] entry=[%s]\n", path, parent, entry);

	/* find parent node */
	if (entry == parent) pnode = pi->current;
	else pnode = xmldb_find_node(pi->current, parent, 0);
	if (!pnode) { free(parent); return 0; }

	/* id start at 1. */
	id = 1;
	for (id = 1; xmldb_find_sibling(pnode->child, entry, id, 0); id++);

	free(parent);
	return id-1;
}

/****************************************************************************/

static int do_patch(PINFO * pi, int id, int depth);

static int do_mapping(PINFO * pi, const char * msg, int id, int depth)
{
	char * buff, * path, * map;
	const char * value;
	int i;
	char node[80];
	char str[16];

	//d_dbg("do_mapping(%d): id=%d [%s]\n", depth, id, msg);

	/* duplicate the message string */
	buff = strdup(msg);
	if (!buff) { verbose("do_mapping: memory allocation fail!\n"); return -1; }

	/* extract node path, and get value */
	path = (char *)xmldb_eatwhite(buff);
	map = strchr(path, ',');
	if (map) *map++ = '\0';
	xmldb_reatwhite(path);
	i = strlen(path);
	d_dbg("do_mapping(%d): path=[%s], map=[%s]\n", depth, path, map);
	if (strcmp(path, "#")==0)
	{
		snprintf(str, sizeof(str)-1, "%d", id);
		value = str;
	}
	else if (path[i-1] == '#')
	{
		path[i-1] = '\0';
		snprintf(str, sizeof(str)-1, "%d", do_array_count(pi, path));
		value = str;
	}
	else if (path[0] == '%')
	{
		path++;
		get_node_from_cstring(node, sizeof(node), &path);
		//d_dbg("do_mapping(%d): node=[%s], path=[%s]\n", depth, node, path);
		if (node[0])
		{
			if (strcmp(node, "#")==0) i = id;
			else if (strncmp(node, "var", 3)==0) i = g_var[(node[3]-'0')%10];
			else i = atoi(xmldb_get_value(pi->current, node));
			snprintf(str, sizeof(str)-1, "%d", do_calculate(i, path, id, pi->max));
			value = str;
		}
		else
		{
			value = "";
		}
	}
	else
	{
		if (strncmp(path, "var", 3)==0)
		{
			snprintf(str, sizeof(str)-1, "%d", g_var[(path[3]-'0')%10]);
			value = str;
		}
		else
		{
			value = xmldb_get_value(pi->current, path);
		}
	}

	/* is there map ? */
	d_dbg("do_mapping(%d) : value[%s]\n", depth, value);
	if (map && map[0]) map_output(pi->outfd, value, map, pi->flags);
	else _output(pi->outfd, value, pi->flags);

	free(buff);
	d_dbg("do_mapping(%d) done!\n", depth);
	return 0;
}

static int do_if_condition(struct xml_node * root, const char * tagmsg, int id, int depth)
{
	char *buff, *entry, *op, *oprand;
	const char *ptr;
	int res = 0;
	const char *op1, *op2;
	int val1, val2;
	char buff1[16], buff2[16];

	d_dbg("do_if_condition(%d): id=%d [%s]\n", depth, id, tagmsg);

	buff = strdup(tagmsg);
	if (!buff) { verbose("do_if_condition(%d): mem alloc fail!\n", depth); return -1; }

	ptr = xmldb_eatwhite(xmldb_reatwhite(buff));
	entry = op = oprand = NULL;
	while (*ptr)
	{	/* Get entry */
		ptr = xmldb_eatwhite(ptr);
		entry = strdup(read_string(&ptr, 0));
		if (!entry || !*entry) break;
		/* Get op */
		ptr = xmldb_eatwhite(ptr);
		op = strdup(read_string(&ptr, 0));
		if (!op || !*op) break;
		/* oprand */
		ptr = xmldb_eatwhite(ptr);
		oprand = strdup(read_string(&ptr, 0));
		ptr = xmldb_eatwhite(ptr);

		d_dbg("do_if_condition(%d): entry[%s], op[%s], oprand[%s]\n", depth, entry, op, oprand);

		if (entry[0] == '%')
		{
			entry++;

			if (strncmp(entry, "var", 3)==0) val1 = g_var[(entry[3]-'0')%10];
			else val1 = atoi(xmldb_get_value(root, entry));

			if (strncmp(oprand, "var", 3)==0) val2 = g_var[(entry[3]-'0')%10];
			else val2 = atoi(oprand);
			
			if 		(strcmp(op, "==")==0)	res = (val1 == val2) ? 1 : 0;
			else if	(strcmp(op, "!=")==0)	res = (val1 != val2) ? 1 : 0;
			else if (strcmp(op, "<=")==0 ||
					 strcmp(op, "!>")==0)	res = (val1 <= val2) ? 1 : 0;
			else if (strcmp(op, ">=")==0 ||
					 strcmp(op, "!<")==0)	res = (val1 >= val2) ? 1 : 0;
			else if	(strcmp(op, "<")==0)	res = (val1 < val2) ? 1 : 0;
			else if	(strcmp(op, ">")==0)	res = (val1 > val2) ? 1 : 0;
			else { verbose("do_if_condition(%d): unknown op [%s]!\n", depth, op); break; }
		}
		else
		{
			if (strncmp(entry, "var", 3)==0) { snprintf(buff1, sizeof(buff1)-1, "%d", g_var[(entry[3]-'0')%10]); op1 = buff1; }
			else op1 = xmldb_get_value(root, entry);

			if (strncmp(oprand, "var", 3)==0) { snprintf(buff2, sizeof(buff2)-1, "%d", g_var[(entry[3]-'0')%10]); op2 = buff2; }
			else op2 = oprand;

			if		(strcmp(op, "==")==0)	res = (strcmp(op1, op2)==0) ? 1 : 0;
			else if	(strcmp(op, "!=")==0)	res = (strcmp(op1, op2)!=0) ? 1 : 0;
			else { verbose("do_if_condition(%d): unknown op [%s]!\n", depth, op); break; }
		}

		if		(strncmp(ptr, "&&", 2)==0)	{ if (!res) break; }
		else if	(strncmp(ptr, "||", 2)==0)	{ if (res) break; }
		else								{ break; }
		ptr = ptr+2;

		if (entry) free(entry); if (op) free(op); if (oprand) free(oprand);
		entry = op = oprand = NULL;
	}
	if (entry) free(entry); if (op) free(op); if (oprand) free(oprand);
	entry = op = oprand = NULL;
	free(buff);
	d_dbg("do_if_condition(%d): res = %d\n", depth, res);
	return res;
}

static int do_if(PINFO * pi, const char * tagmsg, int id, int depth)
{
	char * msg;
	int res, c, nif;
	PINFO info;
	int break_from_else = 0;
	int break_from_fi = 0;

	d_dbg("do_if(%d): id:%d [%s]\n", depth, id, tagmsg);
	d_dbg("  pi->current = 0x%x\n", pi->current);

	res = do_if_condition(pi->current, tagmsg, id, depth);
	d_dbg("do_if(%d): res = %d\n", depth, res);

	/* read out all the message before "$else" of "$fi" */
	memset(&info, 0, sizeof(info));
	info.flags = pi->flags;
	info.outfd = pi->outfd;
	info.current = pi->current;
	info.fh = NULL;
	nif = 1;
	do
	{	/* check first '{' */
		c = pi_getc(pi);
		if (c==EOF) break;
		if (c!='{') { if (res) pi_putc(&info, c); continue; }

		/* check second '{' */
		c = pi_getc(pi);
		if (c==EOF) { if (res) pi_putc(&info, '{'); break; }
		if (c!='{') { if (res) { pi_putc(&info, '{'); pi_putc(&info, c); } continue; }

		/* reach our tag, start '{{', read tag message before '}}' */
		c = read_tag_msg(pi, g_tag, sizeof(g_tag));
		msg = (char *)xmldb_eatwhite(xmldb_reatwhite(g_tag));

		if (strncmp(msg, "$if", 3)==0)
		{
			nif++;
			d_dbg("reach $if (nif=%d)\n", nif);
			if (res) pi_puttag(&info, msg);
		}
		else if (strncmp(msg, "$else", 5)==0)
		{
			d_dbg("reach $else (nif=%d)\n", nif);
			if (nif == 1)
			{
				d_dbg("break from else\n");
				break_from_else = 1;
				break;
			}
			if (res) pi_puttag(&info, msg);
		}
		else if (strncmp(msg, "$fi", 3)==0)
		{
			d_dbg("reach $fi (nif=%d)\n", nif);
			if (--nif)
			{
				if (res) pi_puttag(&info, msg);
			}
			else
			{
				d_dbg("break from fi\n");
				break_from_fi = 1;
				break;
			}
		}
		else
		{
			if (res) pi_puttag(&info, msg);
		}
	} while (c!=EOF);

	if (res)
	{
		d_dbg("do_patch:\n%s\n", info.buff);
		do_patch(&info, id, depth);
	}

	if (break_from_else)
	{	/* read out all the message before "$fi" */
		d_dbg("break_from_else, search $fi\n");
		nif = 1;
		do
		{	/* check first '{' */
			c = pi_getc(pi);
			if (c==EOF) break;
			if (c!='{') { if (!res) pi_putc(&info, c); continue; }

			/* check second '{' */
			c = pi_getc(pi);
			if (c==EOF) { if (!res) pi_putc(&info, '{'); break; }
			if (c!='{') { if (!res) { pi_putc(&info, '{'); pi_putc(&info, c); } continue; }

			/* reach our tag, start '{{', read tag message before '}}' */
			c = read_tag_msg(pi, g_tag, sizeof(g_tag));
			msg = (char *)xmldb_eatwhite(xmldb_reatwhite(g_tag));

			if (strncmp(msg, "$if", 3)==0)
			{
				nif++;
				if (!res) pi_puttag(&info, msg);
			}
			else if (strncmp(msg, "$fi", 3)==0)
			{
				if (--nif)
				{
					if (!res) pi_puttag(&info, msg);
				}
				else
				{
					d_dbg("break_from_fi\n");
					break_from_fi = 1;
					break;
				}
			}
			else
			{
				if (!res) pi_puttag(&info, msg);
			}
		} while (c!=EOF);
		if (!res) do_patch(&info, id, depth);
	}

	if (info.buff) free(info.buff);
	
	return 0;
}

static int do_array(PINFO * pi, const char * tag, int depth)
{
	char * path, *entry, *tagmsg;
	int array, c, id;
	PINFO info;
	struct xml_node *pnode;

	d_dbg("do_array(%d): [%s]\n", depth, tag);
	
	/* get path from tag */
	path = strdup(tag);
	if (!path) { verbose("do_array: memory alloc fail!\n"); return -1; }

	entry = strrchr(path, '/');
	if (entry) *entry++ = '\0';
	else entry = path;

	d_dbg("do_array(%s), path;%s, entry:%s\n", tag, ((path==entry) ? NULL : path), entry);
	
	memset(&info, 0, sizeof(info));
	info.flags = pi->flags;
	info.outfd = pi->outfd;
	info.fh = NULL;

	/* read out all the array message */
	array = 1;
	do
	{
		c = pi_getc(pi);
		if (c==EOF) break;
		if (c!='{') { pi_putc(&info, c); continue; }
		c = pi_getc(pi);
		if (c==EOF) { pi_putc(&info, '{'); break; }
		if (c!='{') { pi_putc(&info, '{'); pi_putc(&info, c); continue; }

		c = read_tag_msg(pi, g_tag, sizeof(g_tag));
		tagmsg = (char *)xmldb_eatwhite(xmldb_reatwhite(g_tag));

		if		(strncmp(tagmsg, "$arraystart:", 12)==0)
		{
			array++;
			pi_puttag(&info, tagmsg);
		}
		else if	(strncmp(tagmsg, "$arrayend", 9)==0)
		{
			if (--array) pi_puttag(&info, tagmsg);
			else break;
		}
		else
		{
			pi_puttag(&info, tagmsg);
		}
	} while (c!=EOF);

	/* find parent node */
	if (path == entry) pnode = pi->current;
	else pnode = xmldb_find_node(pi->current, path, 0);
	if (!pnode)
	{
		verbose("do_array: can not find node [%s]\n", path);
		free(path);
		return 0;
	}

	/* find max id */
	for (info.max=1; xmldb_find_sibling(pnode->child, entry, info.max, 0); info.max++);
	info.max--;

	/* id start at 1. */
	for (id=1; (info.current = xmldb_find_sibling(pnode->child, entry, id, 0))!=NULL; id++)
	{
		pi_set_offset(&info, 0);
		do_patch(&info, id, depth);
	}

	if (info.buff) free(info.buff);
	free(path);
	return 0;
}

static int do_variables(PINFO * pi, const char * tag, int id, int depth)
{
	int r, i, value=0;
	const char * tagmsg;

	d_dbg("do_variables: tag[%s]\n", tag);

	r = (tag[0]-'0')%10;
	tagmsg = xmldb_eatwhite(tag+1);
	if (tagmsg[0] != '=') { verbose("do_variable: invalid format: [%s]\n", tagmsg); return -1; }

	d_dbg("do_variables: result to var [%d]\n", r);

	tagmsg = xmldb_eatwhite(tagmsg+1);
	d_dbg("do_variables: tagmsg[%s]\n", tagmsg);
	value = readnum(&tagmsg, id, pi->max);
	d_dbg("do_variables: value = %d\n", value);
	tagmsg = xmldb_eatwhite(tagmsg);
	d_dbg("do_variables: tagmsg[%s]\n", tagmsg);
	
	while (*tagmsg)
	{
		switch (tagmsg[0])
		{
		case '+':
			tagmsg = xmldb_eatwhite(tagmsg+1);
			value += readnum(&tagmsg, id, pi->max);
			break;
		case '-':
			tagmsg = xmldb_eatwhite(tagmsg+1);
			value -= readnum(&tagmsg, id, pi->max);
			break;
		case '*':
			tagmsg = xmldb_eatwhite(tagmsg+1);
			value *= readnum(&tagmsg, id, pi->max);
			break;
		case '/':
			tagmsg = xmldb_eatwhite(tagmsg+1);
			i = readnum(&tagmsg, id, pi->max);
			if (i>0) value /= i;
			else verbose("do_variable: divide by zero!\n");
			break;
		case '%':
			tagmsg = xmldb_eatwhite(tagmsg+1);
			value %= readnum(&tagmsg, id, pi->max);
			break;
		}
	}
	g_var[r] = value;

	return 0;
}

static int do_patch(PINFO * pi, int id, int depth)
{
	int c;
	char * tagmsg;
	int ret=0;

	d_dbg("do_patch(%d): id=%d\n", depth, id);
	d_dbg("  pi->current = 0x%x\n", pi->current);

	do
	{
		/* check first '{' */
		c = pi_getc(pi);
		if (c==EOF) break;
		if (c!='{') { buffer_putc(c, pi->outfd); continue; }

		/* check second '{' */
		c = pi_getc(pi);
		if (c==EOF) { buffer_putc('{', pi->outfd); break; }
		if (c!='{') { buffer_putc('{', pi->outfd); buffer_putc(c, pi->outfd); continue; }

		/* reach our tag start '{{', read tag message before '}}' */
		c = read_tag_msg(pi, g_tag, sizeof(g_tag));
		tagmsg = (char *)xmldb_eatwhite(xmldb_reatwhite(g_tag));

		switch (tagmsg[0])
		{
		case '$':
			if		(strncmp(tagmsg, "$arraystart:", 12) == 0)
			{
				ret = do_array(pi, tagmsg+12, depth+1);
			}
			else if (strncmp(tagmsg, "$if", 3) == 0)
			{
				ret = do_if(pi, tagmsg+3, id, depth+1);
			}
			else if (strncmp(tagmsg, "$var", 4) == 0)
			{
				ret = do_variables(pi, tagmsg+4, id, depth+1);
			}
			else
			{
				verbose("do_patch: unknown tag msg: %s\n", tagmsg);
			}
			break;
		case '@':	/* Setting path. */
			if (id==0) pi->current = xmldb_find_node(pi->current, tagmsg+1, 0);
			d_dbg("  change current to 0x%08x\n", pi->current);
			break;
		default:	/* Value mapping */
			ret = do_mapping(pi, tagmsg, id, depth);
			break;
		}
		if (ret < 0) break;
	} while (c!=EOF);
	return ret;
}

/****************************************************************************/

int xmldb_patch_file(int fd, const char * file, unsigned long flags)
{
	PINFO info;
	int ret;

	d_dbg("xmldb_patch_file(%s)\n", file);

	memset(&g_var, 0, sizeof(g_var));
	memset(&info, 0, sizeof(info));
	info.current = &g_xml_root;
	info.outfd = fd;
	info.flags = flags;
	info.fh = fopen(file, "r");
	if (info.fh == NULL)
	{
		verbose("Can't open template file! %d, (%s)\n", errno, strerror(errno));
		return -1;
	}

	ret = do_patch(&info, 0, 0);
	buffer_putc(0, fd);

	if (info.fh) fclose(info.fh);
	buffer_flush(fd);
	return ret;
}

int xmldb_patch_message(int fd, struct xml_node * root, const char * msg, unsigned long flags)
{
	PINFO info;
	int ret;

	d_dbg("xmldb_patch_message(%s)\n", msg);

	memset(&g_var, 0, sizeof(g_var));
	memset(&info, 0, sizeof(info));
	info.current = root;
	info.outfd = fd;
	info.flags = flags;
	ret = do_mapping(&info, msg, 0, 0);
	buffer_putc(0, fd);
	buffer_flush(fd);
	d_dbg("xmldb_patch_message(%s) done!\n", msg);
	return ret;
}

int xmldb_patch_buffer(int fd, unsigned short length, unsigned long flags)
{
	PINFO info;
	int size;

	d_dbg("xmldb_patch_buffer(len=%d)\n", length);

	memset(&g_var, 0, sizeof(g_var));
	memset(&info, 0, sizeof(info));
	info.current = &g_xml_root;
	info.outfd = fd;
	info.flags = flags;
	info.allocsize = (size_t)length;
	info.buff = (char *)malloc(info.allocsize);
	if (info.buff)
	{
		size = read(fd, info.buff, info.allocsize);
		d_dbg("read: size=%d, %s\n", size, info.buff);
		info.size = size;
		do_patch(&info, 0, 0);
		buffer_putc(0, fd);
		buffer_flush(fd);
	}
	else
	{
		size = 0;
	}
	return size;
}

/****************************************************************************/
