/* vi: set sw=4 ts=4: */
/*
 * Simple doubly linked list implementation.
 * Stolen from <linux/list.h>
 */

#ifndef _DLIST_H_
#define _DLIST_H_

struct dlist_head
{
	struct dlist_head *next, *prev;
};

#define DLIST_HEAD_INIT(name)	{ &(name), &(name) }
#define DLIST_HEAD(name) \
		struct dlist_head name = LIST_HEAD_INIT(name)

#define INIT_DLIST_HEAD(ptr) do { \
		(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/* Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already !
 */
static inline void __dlist_add(struct dlist_head * new, struct dlist_head * prev, struct dlist_head * next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/* Delete a list entry by marking the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __dlist_del(struct dlist_head * prev, struct dlist_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/***************************************************************************/


/* dlist_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void dlist_add(struct dlist_head * new, struct dlist_head * head)
{
	__dlist_add(new, head, head->next);
}

/* dlist_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void dlist_add_tail(struct dlist_head * new, struct dlist_head * head)
{
	__dlist_add(new, head->prev, head);
}

/* dlist_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is in an undefined state.
 */
static inline void dlist_del(struct dlist_head * entry)
{
	__dlist_del(entry->prev, entry->next);
	entry->next = (void *)0;
	entry->prev = (void *)0;
}

/* dlist_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void dlist_del_init(struct dlist_head * entry)
{
	__dlist_del(entry->prev, entry->next);
	INIT_DLIST_HEAD(entry);
}

/* dlist_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int dlist_empty(struct dlist_head * head)
{
	return head->next == head;
}

/* dlist_entry - get the struct for this entry.
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embeded in.
 * @member:	the name of the list_struct within the struct.
 */
#define dlist_entry(ptr, type, member) \
		((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))


#endif
