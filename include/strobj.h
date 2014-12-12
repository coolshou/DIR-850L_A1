/* vi: set sw=4 ts=4: */
/*
 * strobj.h
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
#ifndef __STROBJ_HEADER_FILE__
#define __STROBJ_HEADER_FILE__

#include <xstream.h>

#define SO_FLAG_QUOT_STRING	0x0001

typedef void * strobj_t;
typedef void * solist_t;

/* functions to manipulate the string object. */
static inline int sobj_iswhite(char c)
{
    return ((c)==' ' || (c)=='\t' || (c)=='\n' || (c)=='\r');
}

static inline const char * sobj_eatwhite(const char * string)
{
    if (string == NULL) return NULL;
    while (sobj_iswhite(*string)) string++;
    return string;
}

static inline char * sobj_reatwhite(char * string)
{
    int i;
    if (string == NULL) return NULL;
    i = strlen(string) - 1;
    while (i>=0 && sobj_iswhite(string[i])) string[i--] = '\0';
    return string;
}

static inline int sobj_isindent(char c)
{
    return ((c)=='\t' || (c)=='\n' || (c)=='\r');
}

static inline const char * sobj_eatindent(const char * string)
{
    if (string == NULL) return NULL;
    while (sobj_isindent(*string)) string++;
    return string;
}

static inline char * sobj_reatindent(char * string)
{
    int i;
    if (string == NULL) return NULL;
    i = strlen(string) - 1;
    while (i>=0 && sobj_isindent(string[i])) string[i--] = '\0';
    return string;
}

strobj_t		sobj_new(void);
void			sobj_del(strobj_t obj);
int				sobj_inc_space(strobj_t obj);
int				sobj_add_char(strobj_t obj, int c);
int				sobj_add_string(strobj_t obj, const char * string);
int				sobj_format(strobj_t obj, const char * format, ...);
int				sobj_add_format(strobj_t obj, const char * format, ...);
char *			sobj_strdup(strobj_t obj);
const char *	sobj_get_string(strobj_t obj);
unsigned int	sobj_get_flags(strobj_t obj);
const char *	sobj_eat_all_white(strobj_t obj);
const char *	sobj_eat_indent(strobj_t obj);
int				sobj_free(strobj_t obj);
int				sobj_strcpy(strobj_t obj, const char * string);
int				sobj_move(strobj_t to, strobj_t from);
char			sobj_get_char(strobj_t obj, size_t i);
size_t			sobj_get_length(strobj_t obj);
int				sobj_empty(strobj_t obj);
strobj_t		sobj_split(strobj_t obj, size_t at);
int				sobj_strchr(strobj_t obj, int c);
int				sobj_strrchr(strobj_t obj, int c);
int				sobj_strstr(strobj_t obj, const char * str);
int				sobj_strcmp(strobj_t obj, const char *s);
int				sobj_strncmp(strobj_t obj, const char *s, size_t n);
int				sobj_strcasecmp(strobj_t obj, const char * s);
int				sobj_strncasecmp(strobj_t obj, const char * s, size_t n);
int				sobj_remove_char(strobj_t obj, size_t at);
int				sobj_remove_tail(strobj_t obj);
int				sobj_xstream_eatwhite(xstream_t fd);
int				sobj_xstream_read(xstream_t fd, strobj_t sobj, const char * end, int * quot);
int				sobj_xstream_read_esc(xstream_t fd, strobj_t sobj, const char * end, int * quot, char esc);
int				sobj_xstream_read_tokens(xstream_t fd, solist_t list, char end, const char * delimiter);
void			sobj_escape_javascript(const char * from, strobj_t to);
void			sobj_escape_shellscript(const char * from, strobj_t to);
void			sobj_escape_html_sc(const char * from, strobj_t to);
void			sobj_unescape_html_sc(const char * from, strobj_t to);
void			sobj_escape_xml_sc(const char * from, strobj_t to);
void			sobj_unescape_xml_sc(const char * from, strobj_t to);
strobj_t		sobj_unescape_uri(strobj_t obj);
void			sobj_urlencode_sc(const char * from, strobj_t to);
void			sobj_urldecode(const char * from, strobj_t to);


/* functions to manipulate the string object list. */
solist_t		solist_new(void);
void			solist_free(solist_t list);
void			solist_del(solist_t list);
strobj_t		solist_get_next(solist_t list);
strobj_t		solist_get_prev(solist_t list);
void			solist_get_reset(solist_t list);
void			solist_remove(solist_t list, strobj_t obj);
void			solist_add(solist_t list, strobj_t obj);
int				solist_get_count(solist_t list);

#endif
