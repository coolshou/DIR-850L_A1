/* vi: set sw=4 ts=4: */
/*
 *	strobj.c
 *
 *	String object. A helper to manipulate string text.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>

#include <dtrace.h>
#include <dlist.h>
#include <mem_helper.h>
#include <strobj.h>

#define STROBJ_STEP_SIZE	32
#define STROBJ_T(x)			(struct strobj *)(x)
#define SOLIST_T(x)			(struct strobj_list *)(x)

struct strobj
{
	struct dlist_head list;
	unsigned int flags;
	size_t total;	/* allocated size, not including the terminated NULL. */
	size_t size;	/* used size, not including the terminated NULL. */
	char * buff;	/* pointer to the buffer */
};

struct strobj_list
{
	struct dlist_head head;
	struct strobj * curr;
};

/****************************************************************************/

solist_t solist_new(void)
{
	struct strobj_list * sol = xmalloc(sizeof(struct strobj_list));
	if (sol)
	{
		INIT_DLIST_HEAD(&sol->head);
		sol->curr = NULL;
	}
	return sol;
}

void solist_free(solist_t list)
{
	struct strobj_list * sol = SOLIST_T(list);
	struct dlist_head * entry;
	struct strobj * so = NULL;

	dassert(sol);
	while (!dlist_empty(&sol->head))
	{
		entry = sol->head.next;
		dlist_del(entry);
		so = dlist_entry(entry, struct strobj, list);
		sobj_del(so);
	}
	sol->curr = NULL;
}

void solist_del(solist_t list)
{
	dassert(list);
	solist_free(list);
	xfree(list);
}

strobj_t solist_get_next(solist_t list)
{
	struct strobj_list * sol = SOLIST_T(list);
	struct dlist_head * entry;
	struct strobj * so = NULL;

	dassert(sol);

	if (sol->curr) entry = sol->curr->list.next;
	else entry = sol->head.next;
	if (entry != &sol->head) so = dlist_entry(entry, struct strobj, list);
	sol->curr = so;
	return so;
}

strobj_t solist_get_prev(solist_t list)
{
	struct strobj_list * sol = SOLIST_T(list);
	struct dlist_head * entry;
	struct strobj * so = NULL;

	dassert(sol);

	if (sol->curr) entry = sol->curr->list.prev;
	else entry = sol->head.prev;
	if (entry != &sol->head) so = dlist_entry(entry, struct strobj, list);
	sol->curr = so;
	return so;
}

void solist_get_reset(solist_t list)
{
	struct strobj_list * sol = SOLIST_T(list);
	if (sol) sol->curr = NULL;
}

void solist_remove(solist_t list, strobj_t obj)
{
	struct strobj_list * sol = SOLIST_T(list);
	struct strobj * so = STROBJ_T(obj);

	dassert(sol && so);
	if (sol->curr == so) sol->curr = NULL;
	dlist_del(&so->list);
}

void solist_add(solist_t list, strobj_t obj)
{
	struct strobj_list * sol = SOLIST_T(list);
	struct strobj * so = STROBJ_T(obj);

	dassert(sol && so);
	dlist_add_tail(&so->list, &sol->head);
}

int solist_get_count(solist_t list)
{
	struct strobj_list * sol = SOLIST_T(list);
	struct dlist_head * entry;
	int count = 0;

	entry = sol->head.next;
	while (entry != &sol->head)
	{
		count++;
		entry = entry->next;
	}
	return count;
}

/****************************************************************************/

/* Create a string object. */
strobj_t sobj_new(void)
{
	struct strobj * sobj = xmalloc(sizeof(struct strobj));
	if (sobj)
	{
		memset(sobj, 0, sizeof(struct strobj));
		INIT_DLIST_HEAD(&sobj->list);
	}
	return sobj;
}

/* Destroy the string object. */
void sobj_del(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);
	if (sobj)
	{
		if (sobj->buff) xfree(sobj->buff);
		xfree(sobj);
	}
}

/* Increase the space of the object. */
int sobj_inc_space(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);

	dassert(sobj);
	if (!sobj) return -1;
	if (sobj->buff)
	{
		sobj->total += STROBJ_STEP_SIZE;
		sobj->buff = xrealloc(sobj->buff, sobj->total + 1);
	}
	else
	{
		dassert(sobj->size == 0 && sobj->total == 0);
		sobj->total = STROBJ_STEP_SIZE;
		sobj->buff = xmalloc(sobj->total + 1);
	}
	return sobj->buff ? 0 : -1;
}

/* add a character at the tail of the object */
int sobj_add_char(strobj_t obj, int c)
{
	struct strobj * sobj = STROBJ_T(obj);

	dassert(sobj);
	if (!sobj) return -1;
	if ((sobj->total - sobj->size) < 1)
		if (sobj_inc_space(sobj) < 0) return -1;
	sobj->buff[sobj->size++] = (char)c;
	sobj->buff[sobj->size] = '\0';
	return 0;
}

/* append a string at the tail of the object */
int sobj_add_string(strobj_t obj, const char * string)
{
	struct strobj * sobj = STROBJ_T(obj);
	size_t i;

	dassert(sobj);
	if (!sobj) return -1;
	if (!string) return 0;
	i = strlen(string);
	if (i == 0) return 0;

	while ((sobj->total - sobj->size) < i)
		if (sobj_inc_space(sobj) < 0) return -1;
	strcpy(&(sobj->buff[sobj->size]), string);
	sobj->size += i;
	return 0;
}

/* format string */
int sobj_format(strobj_t obj, const char * format, ...)
{
	va_list marker;
	char buff[1024];

	dassert(obj);
	if (!obj) return -1;
	sobj_free(obj);
	if (!format) return 0;
	va_start(marker, format);
	vsnprintf(buff, sizeof(buff), format, marker);
	va_end(marker);
	buff[1023]='\0';
	sobj_add_string(obj, buff);
	return 0;
}

/* append format string */
int sobj_add_format(strobj_t obj, const char * format, ...)
{
	va_list marker;
	char buff[1024];

	dassert(obj);
	if (!obj) return -1;
	if (!format) return 0;
	va_start(marker, format);
	vsnprintf(buff, sizeof(buff), format, marker);
	va_end(marker);
	buff[1023]='\0';
	sobj_add_string(obj, buff);
	return 0;
}

/* duplicate a string from the object. */
char * sobj_strdup(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);

	dassert(sobj);
	if (!sobj) return NULL;
	if (sobj->buff) return xstrdup(sobj->buff);
	return xstrdup("");
}

/* get the string of the object */
const char * sobj_get_string(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);

	dassert(sobj);
	if (!sobj) return NULL;
	return sobj->buff ? sobj->buff : "";
}

/* get the flags of string object */
unsigned int sobj_get_flags(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);
	dassert(sobj);
	if (!sobj) return 0;
	return sobj->flags;
}

/* remove the white character of the string. */
const char * sobj_eat_all_white(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);
	struct strobj * newobj = NULL;
	const char * ptr;

	do
	{
		if (!sobj || !sobj->buff) break;
		newobj = sobj_new(); if (!newobj) break;

		ptr = sobj_eatwhite(sobj_reatwhite(sobj->buff));
		while (*ptr)
		{
			if (sobj_iswhite(*ptr))
			{
				sobj_add_char(newobj, ' ');
				ptr = sobj_eatwhite(ptr);
			}
			else
			{
				sobj_add_char(newobj, *ptr);
				ptr++;
			}
		}
		/* Copy the newobj to sobj */
		sobj_free(sobj);
		sobj->buff = newobj->buff;
		sobj->size = newobj->size;
		sobj->total = newobj->total;
		/* Free new object */
		newobj->buff = NULL;
		newobj->size = newobj->total = 0;
		sobj_del(newobj);
		return sobj->buff;

	} while (0);

	if (newobj) sobj_del(newobj);
	return NULL;
}

/* remove the indent of the string. */
const char * sobj_eat_indent(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);
	struct strobj * newobj = NULL;

	do
	{
		if (!sobj || !sobj->buff) break;
		newobj = sobj_new(); if (!newobj) break;
		sobj_add_string(newobj, sobj_eatindent(sobj_reatindent(sobj->buff)));
		/* Copy the newobj to sobj */
		sobj_free(sobj);
		sobj->buff = newobj->buff;
		sobj->size = newobj->size;
		sobj->total = newobj->total;
		/* Free new object */
		newobj->buff = NULL;
		newobj->size = newobj->total = 0;
		sobj_del(newobj);
		return sobj->buff;

	} while (0);

	if (newobj) sobj_del(newobj);
	return NULL;
}

int sobj_free(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);

	dassert(sobj);
	if (!sobj) return -1;
	if (sobj->buff) xfree(sobj->buff);
	sobj->buff	= NULL;
	sobj->total	= 0;
	sobj->size	= 0;
	sobj->flags	= 0;
	return 0;
}

int sobj_strcpy(strobj_t obj, const char * string)
{
	struct strobj * sobj = STROBJ_T(obj);

	dassert(sobj);
	if (!sobj) return -1;
	sobj->size = 0;
	sobj->flags = 0;
	return sobj_add_string(obj, string);
}

int sobj_move(strobj_t to, strobj_t from)
{
	struct strobj * tobj = STROBJ_T(to);
	struct strobj * fobj = STROBJ_T(from);

	dassert(tobj && fobj);
	if (!tobj || !fobj) return -1;

	if (tobj->buff) xfree(tobj->buff);
	tobj->buff	= fobj->buff;	fobj->buff	= NULL;
	tobj->total	= fobj->total;	fobj->total	= 0;
	tobj->size	= fobj->size;	fobj->size	= 0;
	tobj->flags	= fobj->flags;	fobj->flags	= 0;
	return 0;
}

char sobj_get_char(strobj_t obj, size_t i)
{
	struct strobj * sobj = STROBJ_T(obj);

	dassert(sobj);
	if (!sobj) return '\0';
	if (!sobj->buff) return '\0';
	if (i > sobj->size) return '\0';
	return sobj->buff[i];
}

size_t sobj_get_length(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);

	dassert(sobj);
	if (!sobj) return 0;
	return sobj->size;
}

int sobj_empty(strobj_t obj)
{
	return (sobj_get_length(obj)==0) ? 1 : 0;
}

strobj_t sobj_split(strobj_t obj, size_t at)
{
	struct strobj * sobj = STROBJ_T(obj);
	struct strobj * newobj;

	dassert(sobj);
	if (!sobj) return NULL;

	newobj = sobj_new();
	if (!newobj) return NULL;
	if (at >= sobj->size) return newobj;
	if (sobj_add_string(newobj, &(sobj->buff[at])) < 0)
	{
		sobj_del(newobj);
		return NULL;
	}
	sobj->buff[at] = '\0';
	sobj->size = strlen(sobj->buff);
	return newobj;
}

int sobj_strchr(strobj_t obj, int c)
{
	struct strobj * sobj = STROBJ_T(obj);
	char * ptr;

	dassert(sobj);
	if (!sobj) return -1;
	if (!sobj->buff) return -1;
	ptr = strchr(sobj->buff, c);
	if (ptr) return (int)(ptr - sobj->buff);
	return -1;
}

int sobj_strrchr(strobj_t obj, int c)
{
	struct strobj * sobj = STROBJ_T(obj);
	char * ptr;

	dassert(sobj);
	if (!sobj) return -1;
	if (!sobj->buff) return -1;
	ptr = strrchr(sobj->buff, c);
	if (ptr) return (int)(ptr - sobj->buff);
	return -1;
}

int sobj_strstr(strobj_t obj, const char * str)
{
	struct strobj *sobj = STROBJ_T(obj);
	char * ptr;

	dassert(sobj);
	if (!sobj) return -1;
	if (!sobj->buff) return -1;
	ptr = strstr(sobj->buff, str);
	if (ptr) return (int)(ptr - sobj->buff);
	return -1;
}

int sobj_strcmp(strobj_t obj, const char *s)
{
	struct strobj * sobj = STROBJ_T(obj);
	char * ptr;

	dassert(sobj);
	if (!sobj) return -1;
	ptr = sobj->buff ? sobj->buff : "";
	return strcmp(ptr, s);
}

int sobj_strncmp(strobj_t obj, const char *s, size_t n)
{
	struct strobj * sobj = STROBJ_T(obj);
	char * ptr;

	dassert(sobj);
	if (!sobj) return -1;
	ptr = sobj->buff ? sobj->buff : "";
	return strncmp(ptr, s, n);
}

int sobj_strcasecmp(strobj_t obj, const char * s)
{
	struct strobj * sobj = STROBJ_T(obj);
	char * ptr;

	dassert(sobj);
	if (!sobj) return -1;
	ptr = sobj->buff ? sobj->buff : "";
	return strcasecmp(ptr, s);
}

int sobj_strncasecmp(strobj_t obj, const char * s, size_t n)
{
	struct strobj * sobj = STROBJ_T(obj);
	char * ptr;

	dassert(sobj);
	if (!sobj) return -1;
	ptr = sobj->buff ? sobj->buff : "";
	return strncasecmp(ptr, s, n);
}

int sobj_remove_char(strobj_t obj, size_t at)
{
	struct strobj * sobj = STROBJ_T(obj);

	dassert(sobj);
	if (!sobj) return -1;
	if (!sobj->buff) return -1;
	if (at >= sobj->size) return -1;
	while (at < sobj->size)
	{
		sobj->buff[at] = sobj->buff[at+1];
		at++;
	}
	sobj->size--;
	return 0;
}

int sobj_remove_tail(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);

	dassert(sobj);
	if (!sobj) return -1;
	if (!sobj->buff) return -1;
	if (sobj->size > 0)
	{
		sobj->size--;
		sobj->buff[sobj->size] = '\0';
	}
	return 0;
}

static int find_pattern(char c, const char * str)
{
	while (*str && (*str!=c)) str++;
	return (int)(*str);
}

int sobj_xstream_eatwhite(xstream_t fd)
{
	int c;
	while ((c=xs_getc(fd)) != EOF)
	{
		if (!sobj_iswhite(c))
		{
			xs_ungetc(c, fd);
			break;
		}
	}
	return c;
}

/* This function will read a token from xstream.
 * The token is ended by white space, 'end' character or quotation mark.
 * The return vaule is 0 when error occur.
 * Otherwise the return value is the end of this token. */
int sobj_xstream_read(xstream_t fd, strobj_t sobj, const char * end, int * quot)
{
	return sobj_xstream_read_esc(fd, sobj, end, quot, '\\');
}

int sobj_xstream_read_esc(xstream_t fd, strobj_t sobj, const char * end, int * quot, char esc)
{
	int err=0, escape=0, q=-1, c;
	struct strobj * so = STROBJ_T(sobj);

	while ((c=xs_getc(fd)) != EOF)
	{
		/* We don't want the NULL character. */
		if (c == 0) { err++; break; }

		/* Check the first character to determine
		 * if the string is in quotation marks. */
		if (q < 0)
		{
			if (c=='\"' || c=='\'')
			{
				/* Now we got the quotation mark,
				 * save it and read the next character. */
				q = c;
				continue;
			}
			/* This string is not beginning with quot,
			 * it is not inside the quot. marks.  */
			q = 0;
		}

		if (q)
		{
			/* Read string inside the quotation mark. */
			/* Handle escape character. */
			if (escape)
			{
				switch (c)
				{
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				}
				if (sobj_add_char(sobj, c) < 0) { err++; break; }
				escape = 0;
			}
			/* got escape character ? */
			else if (esc && c==esc) { escape = 1; }
			/* Reach the end ? */
			else if (c==q) { so->flags |= SO_FLAG_QUOT_STRING; break; }
			/* normal character, save it. */
			else if (sobj_add_char(sobj, c) < 0) { err++; break; }
		}
		else
		{
			/* Read string without quotation marks */
			if (c=='\"' || c=='\'')
			{
				/* Reach the end, restore the quotation mark. */
				xs_ungetc(c, fd);
				so->flags &= ~SO_FLAG_QUOT_STRING;
				break;
			}
			else if (sobj_iswhite(c) || (end && find_pattern(c, end)))
			{
				/* Reach the end */
				so->flags &= ~SO_FLAG_QUOT_STRING;
				break;
			}
			/* Normal character, save it. */
			else if (sobj_add_char(sobj, c) < 0) { err++; break; }
		}
	}
	if (quot) *quot = q;
	if (err) return 0;
	return (int)c;
}

int sobj_xstream_read_tokens(xstream_t fd, solist_t list, char end, const char * delimiter)
{
	struct strobj_list * sol = SOLIST_T(list);
	struct strobj * delimit = sobj_new();
	int quot, c = 0, err = 0;
	struct strobj * token = NULL;

	//d_dbg("%s: end [%c], delimiter [%s]\n",__func__, end, delimiter);

	do
	{
		/* Prepare the delimiter */
		if (!delimit) { err++; break; }
		if (sobj_add_string(delimit, delimiter) < 0) { err++; break; }
		if (end)
		{
			if (sobj_add_char(delimit, end) < 0) { err++; break; }
		}

		do
		{
			/* allocate string object for token */
			token = sobj_new();
			if (!token) { err++; break; }
			/* read token */
			sobj_xstream_eatwhite(fd);
			c = sobj_xstream_read(fd, token, sobj_get_string(delimit), &quot);
			if (c == 0) { err++; break; }

			/* add the token to the list. */
			if (sobj_get_length(token) == 0) sobj_del(token);
			else dlist_add_tail(&token->list, &sol->head);
			token = NULL;

			if (c == (int)end) break;
			else if (delimiter && find_pattern(c, delimiter))
			{
				token = sobj_new();
				if (!token) { err++; break; }
				if (sobj_add_char(token, c) < 0) { err++; break; }
				dlist_add_tail(&token->list, &sol->head);
				token = NULL;
			}
		} while (!err && c!=EOF);
	} while (0);

	if (token) sobj_del(token);
	if (delimit) sobj_del(delimit);
	if (err)
	{
		solist_free(list);
		return 0;
	}
	return (int)c;
}

/*************************************************************************/

static void escape_character(char escape, const char * sc, const char * from, strobj_t to)
{
	while (*from)
	{
		if (find_pattern(*from, sc)) sobj_add_char(to, escape);
		sobj_add_char(to, *from++);
	}
}

#if 0
static void unescape_character(char escape, const char * from, strobj_t to)
{
	int escape_mode = 0;
	while (*from)
	{
		if (escape_mode) { sobj_add_char(to, *from); escape_mode = 0; }
		else if (*from == escape) { escape_mode++; }
		else { sobj_add_char(to, *from); }
		from++;
	}
}
#endif

void sobj_escape_javascript(const char * from, strobj_t to)
{
	static char patt[] = {'\\', '"', '\'', '\0'};
	dassert(from && to);
	escape_character('\\', patt, from, to);
}

/* The converted string should be in "". */
void sobj_escape_shellscript(const char * from, strobj_t to)
{
	static char patt[] = {'\\', '$', '"', '`', '\0'};
	dassert(from && to);
	escape_character('\\', patt, from, to);
}

/* HTML special characters
 * '&' -> '&amp;'	- ampersand,
 * '<' -> '&lt;'	- less then,
 * '>' -> '&gt;'	- greater then,
 * ' ' -> '&nbsp;'	- non-breaking space,
 * ''' -> '&apos;'	- apostrophe,
 * '"' -> '&quot;'	- quotation mark. */
void sobj_escape_html_sc(const char * from, strobj_t to)
{
	dassert(from && to);
	while (*from)
	{
		switch (*from)
		{
		case '&':	sobj_add_string(to, "&amp;");	break;
		case '<':	sobj_add_string(to, "&lt;");	break;
		case '>':	sobj_add_string(to, "&gt;");	break;
		/* Don't convert the space, white space wrapping will failed
		 * if we convert all the white space to &nbsp;
		 * David Hsieh*/
		//case ' ':	sobj_add_string(to, "&nbsp;");	break;
		case '"':	sobj_add_string(to, "&quot;");	break;
		//case '\'':	sobj_add_string(to, "&apos;");	break;
		default:	sobj_add_char(to, *from);		break;
		}
		from++;
	}
}

void sobj_unescape_html_sc(const char * from, strobj_t to)
{
	dassert(from && to);
	while (*from)
	{
		if (*from != '&') sobj_add_char(to, *from++);
		else if	(strncmp(from, "&amp;", 5)==0)	{ sobj_add_char(to, '&'); from+=5; }
		else if	(strncmp(from, "&lt;", 4)==0)	{ sobj_add_char(to, '<'); from+=4; }
		else if (strncmp(from, "&gt;", 4)==0)	{ sobj_add_char(to, '>'); from+=4; }
		else if	(strncmp(from, "&nbsp;", 6)==0)	{ sobj_add_char(to, ' '); from+=6; }
		else if	(strncmp(from, "&quot;", 6)==0)	{ sobj_add_char(to, '"'); from+=6; }
		else if	(strncmp(from, "&apos;", 6)==0)	{ sobj_add_char(to, '\''); from+= 6; }
		else sobj_add_char(to, *from++);
	}
}

/* XML special characters
 * '&' -> '&amp;'	- ampersand,
 * '<' -> '&lt;'	- less then,
 * '>' -> '&gt;'	- greater then,
 * ''' -> '&apos;'	- apostrophe,
 * '"' -> '&quot;'	- quotation mark. */
void sobj_escape_xml_sc(const char * from, strobj_t to)
{
	dassert(from && to);
	while (*from)
	{
		switch (*from)
		{
		case '&':	sobj_add_string(to, "&amp;");	break;
		case '<':	sobj_add_string(to, "&lt;");	break;
		case '>':	sobj_add_string(to, "&gt;");	break;
		case '"':	sobj_add_string(to, "&quot;");	break;
		case '\'':	sobj_add_string(to, "&apos;");	break;
		default:	sobj_add_char(to, *from);		break;
		}
		from++;
	}
}

void sobj_unescape_xml_sc(const char * from, strobj_t to)
{
	dassert(from && to);
	while (*from)
	{
		if (*from != '&') sobj_add_char(to, *from++);
		else if	(strncmp(from, "&amp;", 5)==0)	{ sobj_add_char(to, '&'); from+=5; }
		else if	(strncmp(from, "&lt;", 4)==0)	{ sobj_add_char(to, '<'); from+=4; }
		else if (strncmp(from, "&gt;", 4)==0)	{ sobj_add_char(to, '>'); from+=4; }
		else if	(strncmp(from, "&quot;", 6)==0)	{ sobj_add_char(to, '"'); from+=6; }
		else if	(strncmp(from, "&apos;", 6)==0)	{ sobj_add_char(to, '\''); from+= 6; }
		else if (strncmp(from, "&#39;", 5)==0) { sobj_add_char(to, '\''); from+= 5; }
		else sobj_add_char(to, *from++);
	}
}

static int ctoi(char c)
{
	if (c>='0' && c<='9') return (int)(c-'0');
	if (c>='a' && c<='f') return (int)(c-'a'+10);
	if (c>='A' && c<='F') return (int)(c-'A'+10);
	return 0;
}

strobj_t sobj_unescape_uri(strobj_t obj)
{
	struct strobj * sobj = STROBJ_T(obj);
	struct strobj * nobj;
	size_t i;
	char c;

	dassert(sobj);
	if (!sobj) return NULL;
	nobj = sobj_new();
	if (!nobj) return NULL;

	for (i=0; i < sobj->size; i++)
	{
		if (sobj->buff[i] == '%' && (i+2 < sobj->size) &&
			isxdigit(sobj->buff[i+1]) && isxdigit(sobj->buff[i+2]))
		{
			c = ctoi(sobj->buff[i+1]) * 16 + ctoi(sobj->buff[i+2]);
			i+=2;
		}
		else c = sobj->buff[i];
		sobj_add_char(nobj, c);
	}

	/* Copy the nobj to sobj */
	sobj_free(sobj);
	sobj->buff = nobj->buff;
	sobj->size = nobj->size;
	sobj->total = nobj->total;
	/* Free nobj */
	nobj->buff = NULL;
	nobj->size = nobj->total = 0;
	sobj_del(nobj);
	return sobj;
}

void sobj_urlencode_sc(const char * from, strobj_t to)
{
	dassert(from && to);
	char hex[16] = "0123456789ABCDEF";
	while (*from)
	{
		if(isalnum(*from) || *from == '-' || *from == '_' || *from == '.' || *from == '!' ||
		   *from == '~' || *from == '*' || *from == '\'' || *from == '(' || *from == ')')
		{
			sobj_add_char(to, *from);
		}
		else
		{
			sobj_add_char(to,'%');
			sobj_add_char(to,hex[(*from >> 4)]);
			sobj_add_char(to,hex[(*from & 0x0f)]);
		}
		from++;
	}
}

void sobj_urldecode(const char * from, strobj_t to)
{
	dassert(from && to);
	char c;
	while (*from)
	{
		if (*from == '%' &&
			isxdigit(*(from+1)) && isxdigit(*(from+2)))
		{
			c = ctoi(*(from+1))*16 + ctoi(*(from+2));
			from += 3;
		}
		else if(*from == '+')
		{
			c = ' ';
			*from++;
		}
		else c = *from++;
		sobj_add_char(to, c);
	}
}

