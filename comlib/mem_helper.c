/* vi: set sw=4 ts=4: */
/*
 * mem_helper.c
 *
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

#include "dtrace.h"
#include "dlist.h"
#include "mem_helper.h"

#ifndef CONFIG_MEM_HELPER_DISABLE

/* tacking the allocator file name */
#ifdef CONFIG_MEM_HELPER_TRACKING

#ifdef CONFIG_MEM_HELPER_TRACKING_FILES
#define MAX_TRACKING_FILES					CONFIG_MEM_HELPER_TRACKING_FILES
#else
#define MAX_TRACKING_FILES					64
#endif

static char * _mh_files[MAX_TRACKING_FILES] = {NULL};
#endif

/* memory block header */

#define SIGN_MASK				0xffff0000
#define SIZE_MASK				0x0000ffff
#define MEMBLK_SIGN				0x55aa0000
#define SUPER_BLOCK_SIGN		0x5a5a0000
#define MEMHDR(p)				(&(((struct _memblk_hdr_t *)p)[-1]))
#define MEMPTR(h)				(&(((struct _memblk_hdr_t *)h)[1]))
#define MEMBLK_CHECK(h)			((((struct _memblk_hdr_t *)h)->flags & SIGN_MASK) == MEMBLK_SIGN)
#define MEMBLK_SUPER(h)			((((struct _memblk_hdr_t *)h)->flags & SIGN_MASK) == SUPER_BLOCK_SIGN)
#define MEMBLK_SIZE(h)			(((struct _memblk_hdr_t *)h)->flags & SIZE_MASK)

/* memory block header type */
typedef struct _memblk_hdr_t	memblk_hdr_t;
struct _memblk_hdr_t
{
	struct dlist_head			link;
	unsigned int				flags;
#ifdef CONFIG_MEM_HELPER_TRACKING
	int							file, line;
#endif
};

/*
 * Memory block pool.
 *
 * mpool_free_list & mpool_used_list are the pool
 * of the allocated memory block.
 * Super blocks are stored in mpool_free_list[0].
 * Different size of block is store in different mpool_free_list.
 *
 */
#define MEM_POOLS_COUNT		22
static struct dlist_head	mpool_free_list[MEM_POOLS_COUNT];	/* free mem block lists. */
static struct dlist_head	mpool_used_list[MEM_POOLS_COUNT];	/* used mem block lists. */

static size_t	mpool_used[MEM_POOLS_COUNT];	/* used block count */
static size_t	mpool_free[MEM_POOLS_COUNT];	/* free block count */
static size_t	mpool_max_used[MEM_POOLS_COUNT];/* record the max used count */

/*
 * the total number of memblock of size 2^s should be
 * mpool_count[s] * mpool_increase[s].
 */
static size_t	mpool_count[MEM_POOLS_COUNT];
#ifdef MPOOL_HAVE_EXTERNAL_INCREASE_TABLE
extern size_t mpool_increase[32];
#else
static size_t mpool_increase[32] =
{
	0,		/* 2^0	=     0 byte block */
	0,		/* 2^1	=     2 bytes block */
	8,		/* 2^2	=     4 bytes block */
	8,		/* 2^3	=     8 bytes block */
	8,		/* 2^4	=    16 bytes block */
	8,		/* 2^5	=    32 bytes block */
	8,		/* 2^6	=    64 bytes block */
	8,		/* 2^7	=   128 bytes block */
	8,		/* 2^8	=   256 bytes block */
	8,		/* 2^9	=   512 bytes block */
	8,		/* 2^10	=    1K bytes block */
	8,		/* 2^11	=    2K bytes block */
	4,		/* 2^12	=    4K bytes block */
	2,		/* 2^13 =    8K bytes block */
	1,		/* 2^14	=   16K bytes block */
	1,		/* 2^15	=   32K bytes block */
	1,		/* 2^16	=   64K bytes block */
	1,		/* 2^17	=  128K bytes block */
	1,		/* 2^18	=  512K bytes block */
	1,		/* 2^19	=    1M bytes block */
	1,		/* 2^20	=    2M bytes block */
	1,		/* 2^21	=    4M bytes block */
	0,		/* 2^22	=    8M bytes block */
	0,		/* 2^23	=   16M bytes block */
	0,		/* 2^24	=   32M bytes block */
	0,		/* 2^25	=   64M bytes block */
	0,		/* 2^26	=  128M bytes block */
	0,		/* 2^27	=  256M bytes block */
	0,		/* 2^28	=  512M bytes block */
	0,		/* 2^29	= 1024M bytes block */
	0,		/* 2^30	= 2048M bytes block */
	0		/* 2^31	= 4096M bytes block */
};
#endif

/* initialize the whole memory helper module */
void mh_init_all(void)
{
	int i;

	for (i=0; i<MEM_POOLS_COUNT; i++)
	{
		INIT_DLIST_HEAD(&mpool_free_list[i]);
		INIT_DLIST_HEAD(&mpool_used_list[i]);
		mpool_count[i] = mpool_used[i] = mpool_free[i] = mpool_max_used[i] = 0;
	}
#ifdef CONFIG_MEM_HELPER_TRACKING
	for (i=0; i<MAX_TRACKING_FILES; i++) _mh_files[i] = NULL;
#endif
}

/* free all memory helper module */
void mh_free_all(void)
{
	memblk_hdr_t * blk;
	struct dlist_head * entry;

#ifdef CONFIG_MEM_HELPER_TRACKING
	{
		int i;
		for (i=0; i<MAX_TRACKING_FILES; i++) if (_mh_files[i]) free(_mh_files[i]);
	}
#endif

	/* All the super blocks are listed in mpool_free_list[0] */
	while (!dlist_empty(&mpool_free_list[0]))
	{
		entry = mpool_free_list[0].next;
		dlist_del(entry);
		blk = dlist_entry(entry, memblk_hdr_t, link);
		free(blk);
	}

	mh_init_all();
}

static size_t count_list(struct dlist_head * head)
{
	struct dlist_head * entry;
	size_t count = 0;

	for (entry = head->next; entry != head; entry = entry->next) count++;
	return count;
}

/* dump memory helper status */
void mh_dump(FILE * fd)
{
	int i;
	size_t total_size, free_size, used_size, block_size;

	total_size = free_size = used_size = block_size = 0;

	fprintf(fd, "Block size   total    free    used     max\n");
	fprintf(fd, "----------   -----   -----   -----   -----\n");
#define FM		"2^%03d         %4d    %4d    %4d    %4d\n"

	for (i=1; i<MEM_POOLS_COUNT; i++)
	{
		block_size = 1 << i;
		free_size  += (block_size * mpool_free[i]);
		used_size  += (block_size * mpool_used[i]);
		total_size += (block_size * (mpool_free[i] + mpool_used[i]));
		fprintf(fd, FM, i, mpool_free[i]+mpool_used[i], mpool_free[i],mpool_used[i],mpool_max_used[i]);
	}
#undef FM
	fprintf(fd, "------------------------------------------\n");
	fprintf(fd, "total size : %d bytes\n", total_size);
	fprintf(fd, "free size  : %d bytes\n", free_size);
	fprintf(fd, "used size  : %d bytes\n", used_size);

}

/* dump used list */
void mh_dump_used(FILE * fd)
{
	int i;
	struct dlist_head * head;
	struct dlist_head * entry;
	memblk_hdr_t * hdr = NULL;

	for (i=0; i<MEM_POOLS_COUNT; i++)
	{
		head = &mpool_used_list[i];
		for (entry = head->next; entry != head; entry = entry->next)
		{
			hdr = dlist_entry(entry, memblk_hdr_t, link);
#ifdef CONFIG_MEM_HELPER_TRACKING
			fprintf(fd, "entry[%03d]: 0x%.8x, file:%s, line:%d\n", i, (unsigned int)entry, _mh_files[hdr->file], hdr->line);
#else
			fprintf(fd, "entry[%03d]: 0x%.8x, file:%d, line:%d\n", i, entry, hdr->file, hdr->line);
#endif
		}
	}
}

void mh_diagnostic(FILE * fd)
{
	size_t count;
	int i;
	/* check free count */
	for (i=1; i<MEM_POOLS_COUNT; i++)
	{
		count = count_list(&mpool_free_list[i]);
		if (mpool_free[i] != count)
		{
			fprintf(fd, "mpool_free[%d] = %d, but mpool_free_list[%d] have %d entries!\n",
					i, mpool_free[i], i, count);
		}
		count = count_list(&mpool_used_list[i]);
		if (mpool_used[i] != count)
		{
			fprintf(fd, "mpool_used[%d] = %d, but mpool_used_list[%d] have %d entries!\n",
					i, mpool_used[i], i, count);
		}
		count = mpool_count[i] * mpool_increase[i];
		if (count != mpool_free[i] + mpool_used[i])
		{
			fprintf(fd, "mpool_free[%d]=%d, mpool_used[%d]=%d, but total is %d * %d = %d\n",
					i,mpool_free[i],i,mpool_used[i],mpool_count[i],mpool_increase[i],count);
		}
	}
}

/**************************************************************************/

/* allocate memory blocks with size 2^s. */
static void alloc_memblk(int s)
{
	size_t block_size;
	size_t required_size;
	unsigned char * ptr;
	memblk_hdr_t * blk;
	int i;

	dassert(s > 1);
	
	if (mpool_increase[s] > 0)
	{
		/* single block size */
		block_size = sizeof(memblk_hdr_t) + (1 << s);
		/* the super block size we need */
		required_size  = sizeof(memblk_hdr_t) + block_size * mpool_increase[s];

		//d_dbg("alloc_memblk: alloc 2^%d bytes block, add %d blocks, total %d bytes!!\n",
		//		s, mpool_increase[s], required_size);

		/* Allocate super block */
		ptr = (unsigned char *)malloc(required_size);
		if (ptr)
		{
			/* init the super block header and link to mpool_free_list[0] */
			blk = (memblk_hdr_t *)ptr;
			ptr += sizeof(memblk_hdr_t);

			INIT_DLIST_HEAD(&blk->link);
			blk->flags = SUPER_BLOCK_SIGN;
			dlist_add_tail(&blk->link, &mpool_free_list[0]);
			mpool_count[s]++;

			/* put the memory blocks into mpool_free_list[s]. */
			for (i=0; i<mpool_increase[s]; i++)
			{
				blk = (memblk_hdr_t *)ptr;
				ptr += block_size;

				INIT_DLIST_HEAD(&blk->link);
				blk->flags = MEMBLK_SIGN | (s & SIZE_MASK);
				dlist_add_tail(&blk->link, &mpool_free_list[s]);
				mpool_free[s]++;
			}
		}
		else
		{
			d_error("alloc_memblk(%d): unable to alloc memory, required %d bytes !!\n",
					s, required_size);
		}
	}
}

/* alloc a memblk from free pool. */
static memblk_hdr_t * memalloc(int s)
{
	struct dlist_head * entry;
	memblk_hdr_t * hdr = NULL;

	dassert(s > 1);

	/* if free pool is empty, allocate blocks  */
	if (dlist_empty(&mpool_free_list[s])) alloc_memblk(s);

	if (!dlist_empty(&mpool_free_list[s]))
	{
		/* remove from free list */
		entry = mpool_free_list[s].next;
		dlist_del(entry);
		mpool_free[s]--;
		/* add to used list */
		dlist_add(entry, &mpool_used_list[s]);
		mpool_used[s]++;
		/* record the max used. */
		if (mpool_used[s] > mpool_max_used[s]) mpool_max_used[s] = mpool_used[s];
		/* get the pointer to the header */
		hdr = dlist_entry(entry, memblk_hdr_t, link);
	}
	else
	{
		d_error("%s(): fail to allocate size 2^%d !\n",__FUNCTION__,s);
	}
	return hdr;
}

static int _is_entry_in_used(memblk_hdr_t * hdr, int s)
{
	struct dlist_head * curr = mpool_used_list[s].next;

	while (curr != &mpool_used_list[s])
	{
		if (curr == &(hdr->link)) return 1;
		curr = curr->next;
	}
	return 0;
}

static void memfree(memblk_hdr_t * hdr)
{
	int s;

	dassert(hdr);
	s = MEMBLK_SIZE(hdr);

	/* check if this is in the used list. */
	if (_is_entry_in_used(hdr, s))
	{
		/* remove from used list */
		dlist_del(&hdr->link);
		mpool_used[s]--;
		/* add into free list */
		dlist_add(&hdr->link, &mpool_free_list[s]);
		mpool_free[s]++;
	}
	else
	{
		d_error("%s: hdr: 0x%08x is already freed!!\n",__FUNCTION__, hdr);
	}
}

#ifdef CONFIG_MEM_HELPER_TRACKING
static void memblk_tracking(memblk_hdr_t * hdr, const char * file, int line)
{
	int i;

	dassert(hdr);
	dassert(file);
	for (i=0; i<MAX_TRACKING_FILES && _mh_files[i]; i++)
	{
		if (strcmp(_mh_files[i], file)==0) break;
	}
	if (i<MAX_TRACKING_FILES)
	{
		if (_mh_files[i] == NULL) _mh_files[i] = strdup(file);
	}
	else
	{
		dtrace("Woops, %d file name pools is not enough !!!\n", MAX_TRACKING_FILES);
	}
	hdr->file = i;
	hdr->line = line;
}
#endif


/***************************************************************************/

void * mh_malloc(
		size_t size
#ifdef CONFIG_MEM_HELPER_TRACKING
		, const char * __file, int __line
#endif
		)
{
	int i;
	size_t s;
	memblk_hdr_t * hdr;

	for (i=3; i<MEM_POOLS_COUNT && size>0; i++)
	{
		s = (size_t)(1 << i);
		if (size <= s)
		{
			hdr = memalloc(i);
			if (hdr)
			{
#ifdef CONFIG_MEM_HELPER_TRACKING
				memblk_tracking(hdr, __file, __line);
#endif
				return MEMPTR(hdr);
			}
		}
	}
	return NULL;
}

char * mh_strdup(
		const char * string
#ifdef CONFIG_MEM_HELPER_TRACKING
		, const char * __file, int __line
#endif
		)
{
	void * ptr;
	size_t s;
	s = strlen(string);
	ptr = mh_malloc(s+1, __file, __line);
	if (ptr) memcpy(ptr, string, s+1);
	return ptr;
	
}

void * mh_calloc(
		size_t nmemb, size_t size
#ifdef CONFIG_MEM_HELPER_TRACKING
		, const char * __file, int __line
#endif
		)
{
	void * ptr;
	ptr = mh_malloc(nmemb * size, __file, __line);
	if (ptr) memset(ptr, 0, nmemb * size);
	return ptr;
}

void mh_free(void *ptr)
{
	memblk_hdr_t * mhdr;

	if (!ptr) return;

	mhdr = MEMHDR(ptr);
	if (MEMBLK_CHECK(mhdr)) memfree(mhdr);
	else d_error("0x%08x is not our memblock, or it is corrupted !!\n", ptr);
}

void * mh_realloc(
		void *ptr, size_t size
#ifdef CONFIG_MEM_HELPER_TRACKING
		, const char * __file, int __line
#endif
		)
{
	memblk_hdr_t * mhdr;
	void * new_ptr;
	size_t my_size;

	if (ptr)
	{
		mhdr = MEMHDR(ptr);
		if (!MEMBLK_CHECK(mhdr))
		{
			d_error("0x%08x is not our memblock, or it is corrupted !!\n", ptr);
			return NULL;
		}
		my_size = 1 << MEMBLK_SIZE(mhdr);
		//d_dbg("%s: this block ptr=0x%08x, hdr=0x%08x, size=%d, request size = %d\n",__FUNCTION__,ptr,mhdr,my_size,size);
		if (my_size < size)
		{
			new_ptr = mh_malloc(size, __file, __line);
			if (new_ptr) memcpy(new_ptr, ptr, my_size);
			mh_free(ptr);
			return new_ptr;
		}
		return ptr;
	}

	if (size) return mh_malloc(size, __file, __line);
	return NULL;
}

#endif	/* end of #ifndef CONFIG_MEM_HELPER_DISABLE */
