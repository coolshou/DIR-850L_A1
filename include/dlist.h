/* vi: set sw=4 ts=4: */
/*
 *	An implementation of the double linking list.
 *
 *	Copyright (c) 2004-2009 by Alpha Networks, Inc.
 *	Maintained by david_hsieh@alphanetworks.com
 *
 *	The dlist.h is free software; you can redistribute it and/or
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

#ifndef _DLIST_H_
#define _DLIST_H_

typedef struct dlist_head dlist_t;
struct dlist_head
{
	struct dlist_head * next;
	struct dlist_head * prev;
};

#define DLIST_HEAD_INIT(e)	{&(e),&(e)}
#define DLIST_HEAD(name) 	dlist_t name = {&(name),&(name)}
#define INIT_DLIST_HEAD(e)	do { (e)->next = (e)->prev = (e); } while (0)

static inline void __dlist_add(dlist_t * entry, dlist_t * prev, dlist_t * next)
{
	next->prev = entry;
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
}
static inline void __dlist_del(dlist_t * prev, dlist_t * next)
{
	next->prev = prev;
	prev->next = next;
}

/***************************************************************************/

#define dlist_entry(e, t, m) ((t *)((char *)(e)-(unsigned long)(&((t *)0)->m)))

static inline int dlist_empty(struct dlist_head * head)				{ return head->next == head; }
static inline void dlist_add(dlist_t * entry, dlist_t * head)		{ __dlist_add(entry, head, head->next); }
static inline void dlist_add_tail(dlist_t * entry, dlist_t * head)	{ __dlist_add(entry, head->prev, head); }
static inline void dlist_del(dlist_t * entry)
{
	__dlist_del(entry->prev, entry->next);
	entry->next = entry->prev = (void *)0;
}
static inline void dlist_del_init(dlist_t * entry)
{
	__dlist_del(entry->prev, entry->next);
	entry->next = entry->prev = entry;
}
static inline dlist_t * dlist_get_next(dlist_t * entry, dlist_t * head)
{
	entry = entry ? entry->next : head->next;
	return (entry == head) ? NULL : entry;
}
static inline dlist_t * dlist_get_prev(dlist_t * entry, dlist_t * head)
{
	entry = entry ? entry->prev : head->prev;
	return (entry == head) ? NULL : entry;
}

#endif
