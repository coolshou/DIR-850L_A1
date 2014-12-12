/* vi: set sw=4 ts=4: */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pqueue.h"
#include "dtrace.h"

#define MIN_CAPACITY 128	/* min allocated buffer for a packet */

static int pqueue_alloc(int seq, unsigned char *packet, int packlen, pqueue_t ** new);
int packet_timeout_usecs = DEFAULT_PACKET_TIMEOUT * 1000000;
static pqueue_t *pq_head = NULL, *pq_tail = NULL;

/* contains a list of free queue elements.*/
static pqueue_t *pq_freelist_head = NULL;

static int pqueue_alloc(int seq, unsigned char *packet, int packlen, pqueue_t ** new)
{
	pqueue_t *newent;

	//d_dbg("PQUEUE: seq=%d, packlen=%d\n", seq, packlen);

	/* search the freelist for one that has sufficient space  */
	if (pq_freelist_head)
	{
		for (newent = pq_freelist_head; newent; newent = newent->next)
		{
			if (newent->capacity >= packlen)
			{
				/* unlink from freelist */
				if (pq_freelist_head == newent) pq_freelist_head = newent->next;
				if (newent->prev) newent->prev->next = newent->next;
				if (newent->next) newent->next->prev = newent->prev;
				if (pq_freelist_head) pq_freelist_head->prev = NULL;
				break;
			} /* end if capacity >= packlen */
		} /* end for */

		/* nothing found? Take first and reallocate it */
		if (NULL == newent)
		{
			newent = pq_freelist_head;
			pq_freelist_head = pq_freelist_head->next;

			if (pq_freelist_head) pq_freelist_head->prev = NULL;

			d_dbg("PQUEUE: realloc capacity %d to %d\n", newent->capacity, packlen);

			newent->packet = (unsigned char *) realloc(newent->packet, packlen);
			if (!newent->packet)
			{
				d_warn("PQUEUE: error reallocating packet: %s\n", strerror(errno));
				return -1;
			}
			newent->capacity = packlen;
		}

		d_dbg("PQUEUE: Recycle entry from freelist. Capacity: %d\n", newent->capacity);
	}
	else
	{
		/* allocate a new one */
		newent = (pqueue_t *) malloc(sizeof(pqueue_t));
		if (!newent)
		{
			d_warn("PQUEUE: error allocating newent: %s\n", strerror(errno));
			return -1;
		}
		newent->capacity = 0;

		d_dbg("PQUEUE: Alloc new queue entry\n");
	}

	if (!newent->capacity)
	{
		/* a new queue entry was allocated. Allocate the packet buffer */
		int size = packlen < MIN_CAPACITY ? MIN_CAPACITY : packlen;
		/* Allocate at least MIN_CAPACITY */
		d_dbg("PQUEUE: allocating for packet size %d\n", size);
		newent->packet = (unsigned char *) malloc(size);
		if (!newent->packet)
		{
			d_warn("PQUEUE: error allocating packet: %s\n", strerror(errno));
			return -1;
		}
		newent->capacity = size;
	} /* endif ! capacity */

	assert(newent->capacity >= packlen);

	/* store the contents into the buffer */
	memcpy(newent->packet, packet, packlen);

	newent->next = newent->prev = NULL;
	newent->seq = seq;
	newent->packlen = packlen;

	gettimeofday(&newent->expires, NULL);
	newent->expires.tv_usec += packet_timeout_usecs;
	newent->expires.tv_sec += (newent->expires.tv_usec / 1000000);
	newent->expires.tv_usec %= 1000000;

	*new = newent;
	return 0;
}

int pqueue_add(int seq, unsigned char *packet, int packlen)
{
	pqueue_t *newent, *point;

	/* get a new entry */
	if (0 != pqueue_alloc(seq, packet, packlen, &newent)) return -1;

	for (point = pq_head; point != NULL; point = point->next)
	{
		if (point->seq == seq)
		{	/* queue already contains this packet */
			d_warn("PQUEUE: discarding duplicate packet %d\n", seq);
			return -1;
		}
		if (point->seq > seq)
		{	/* gone too far: point->seq > seq and point->prev->seq < seq */
			if (point->prev)
			{	/* insert between point->prev and point */
				d_dbg("PQUEUE: adding %d between %d and %d\n", seq, point->prev->seq, point->seq);
				point->prev->next = newent;
			}
			else
			{	/* insert at head of queue, before point */
				d_dbg("PQUEUE: adding %d before %d\n", seq, point->seq);
				pq_head = newent;
			}
			newent->prev = point->prev;	/* will be NULL, at head of queue */
			newent->next = point;
			point->prev = newent;
			return 0;
		}
	}

	/* We didn't find anywhere to insert the packet,
	 * so there are no packets in the queue with higher sequences than this one,
	 * so all the packets in the queue have lower sequences,
	 * so this packet belongs at the end of the queue (which might be empty)
	 */

	if (pq_head == NULL)
	{
		d_dbg("PQUEUE: adding %d to empty queue\n", seq);
		pq_head = newent;
	}
	else
	{
		d_dbg("PQUEUE: adding %d as tail, after %d\n", seq, pq_tail->seq);
		pq_tail->next = newent;
	}
	newent->prev = pq_tail;
	pq_tail = newent;

	return 0;
}

int pqueue_del(pqueue_t * point)
{
#ifdef DEBUG_PQUEUE
	int pq_count = 0;
	int pq_freelist_count = 0;
	pqueue_t * point;
#endif
	
	d_dbg("PQUEUE: Move seq %d to freelist\n", point->seq);

	/* unlink from pq */
	if (pq_head == point) pq_head = point->next;
	if (pq_tail == point) pq_tail = point->prev;
	if (point->prev) point->prev->next = point->next;
	if (point->next) point->next->prev = point->prev;

	/* add point to the freelist */
	point->next = pq_freelist_head;
	point->prev = NULL;

	if (point->next) point->next->prev = point;
	pq_freelist_head = point;

#ifdef DEBUG_PQUEUE
	for (point = pq_head; point; point = point->next) ++pq_count;
	for (point = pq_freelist_head; point; point = point->next) ++pq_freelist_count;
	d_dbg("PQUEUE: queue length is %d, freelist length is %d\n", pq_count, pq_freelist_count);
#endif

	return 0;
}

pqueue_t * pqueue_head()
{
	return pq_head;
}

int pqueue_expiry_time(pqueue_t * entry)
{
	struct timeval tv;
	int expiry_time;

	gettimeofday(&tv, NULL);
	expiry_time = (entry->expires.tv_sec - tv.tv_sec) * 1000000;
	expiry_time += (entry->expires.tv_usec - tv.tv_usec);
	return expiry_time;
}
