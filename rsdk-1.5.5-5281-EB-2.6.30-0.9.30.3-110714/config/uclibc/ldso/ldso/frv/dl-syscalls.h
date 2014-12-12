/* Copyright (C) 2003, 2004 Red Hat, Inc.
 * Contributed by Alexandre Oliva <aoliva@redhat.com>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

/* We can't use the real errno in ldso, since it has not yet
 * been dynamicly linked in yet. */
#include "sys/syscall.h"
extern int _dl_errno;
#undef __set_errno
#define __set_errno(X) {(_dl_errno) = (X);}
#include <sys/mman.h>

/* The code below is extracted from libc/sysdeps/linux/frv/_mmap.c */

#if DYNAMIC_LOADER_IN_SIMULATOR
#define __NR___syscall_mmap2	    __NR_mmap2
static __inline__ _syscall6(__ptr_t, __syscall_mmap2, __ptr_t, addr,
	size_t, len, int, prot, int, flags, int, fd, off_t, offset);

/* Make sure we don't get another definition of _dl_mmap from the
   machine-independent code.  */
#undef __NR_mmap
#undef __NR_mmap2

/* This is always 12, even on architectures where PAGE_SHIFT != 12.  */
# ifndef MMAP2_PAGE_SHIFT
#  define MMAP2_PAGE_SHIFT 12
# endif

#include <bits/uClibc_page.h> /* for PAGE_SIZE */
static __always_inline void *_dl_memset(void*,int,size_t);
static __always_inline ssize_t _dl_pread(int fd, void *buf, size_t count, off_t offset);

static __ptr_t
_dl_mmap(__ptr_t addr, size_t len, int prot, int flags, int fd, __off_t offset)
{
  size_t plen = (len + PAGE_SIZE - 1) & -PAGE_SIZE;

/* This is a hack to enable the dynamic loader to run within a
   simulator that doesn't support mmap, with a number of very ugly
   tricks.  Also, it's not as useful as it sounds, since only dynamic
   executables without DT_NEEDED dependencies can be run.  AFAIK, they
   can only be created with -pie.  This trick suffices to enable the
   dynamic loader to obtain a blank page that it maps early in the
   bootstrap. */
  if ((flags & MAP_FIXED) == 0)
    {
      void *_dl_mmap_base = 0;
      __ptr_t *ret = 0;

      if (! _dl_mmap_base)
	{
	  void *stack;
	  __asm__ ("mov sp, %0" : "=r" (stack));
	  _dl_mmap_base = (void *)(((long)stack + 2 * PAGE_SIZE) & -PAGE_SIZE);
	retry:
	  if (((void **)_dl_mmap_base)[0] == _dl_mmap_base
	      && ((void **)_dl_mmap_base)[1023] == _dl_mmap_base
	      && (((void **)_dl_mmap_base)[177]
		  == ((void **)_dl_mmap_base)[771]))
	    {
	      while (((void**)_dl_mmap_base)[177])
		{
		  _dl_mmap_base = ((void**)_dl_mmap_base)[177];
		  if (!(((void **)_dl_mmap_base)[0] == _dl_mmap_base
			&& ((void **)_dl_mmap_base)[1023] == _dl_mmap_base
			&& (((void **)_dl_mmap_base)[177]
			    == ((void**)_dl_mmap_base)[771])))
		    ((void(*)())0)();
		}
	    }
	  else
	    {
	      int i;
	      for (i = 0; i < (int)PAGE_SIZE; i++)
		if (*(char*)(_dl_mmap_base + i))
		  break;
	      if (i != PAGE_SIZE)
		{
		  _dl_mmap_base = (void*)((long)_dl_mmap_base + PAGE_SIZE);
		  goto retry;
		}
	      ((void**)_dl_mmap_base)[-1] =
		((void**)_dl_mmap_base)[0] =
		((void**)_dl_mmap_base)[1023] =
		_dl_mmap_base;
	    }
	}

      if (_dl_mmap_base)
	{
	  if (!(((void **)_dl_mmap_base)[0] == _dl_mmap_base
		&& ((void **)_dl_mmap_base)[1023] == _dl_mmap_base
		&& (((void **)_dl_mmap_base)[177]
		    == ((void**)_dl_mmap_base)[771])))
	    ((void(*)())0)();
	  ret = (__ptr_t)((char*)_dl_mmap_base + PAGE_SIZE);
	  _dl_mmap_base =
	    ((void**)_dl_mmap_base)[177] =
	    ((void**)_dl_mmap_base)[771] =
	    (char*)_dl_mmap_base + plen + PAGE_SIZE;
	  ((void**)_dl_mmap_base)[0] =
	    ((void**)_dl_mmap_base)[1023] =
	    _dl_mmap_base;
	}

      if ((flags & MAP_ANONYMOUS) != 0)
	{
	  _dl_memset (ret, 0, plen);
	  return ret;
	}

      flags |= MAP_FIXED;
      addr = ret;
    }
    if (offset & ((1 << MMAP2_PAGE_SHIFT) - 1)) {
#if 0
	__set_errno (EINVAL);
#endif
	return MAP_FAILED;
    }
    if ((flags & MAP_FIXED) != 0)
      {
	if (_dl_pread(fd, addr, len, offset) != (ssize_t)len)
	  return (void*)MAP_FAILED;
	if (plen != len)
	  _dl_memset (addr + len, 0, plen - len);
	return addr;
      }
    return(__syscall_mmap2(addr, len, prot, flags, fd, (off_t) (offset >> MMAP2_PAGE_SHIFT)));
}
#endif

#ifdef __NR_pread
#ifdef DYNAMIC_LOADER_IN_SIMULATOR
#include <unistd.h>

#define __NR___syscall_lseek __NR_lseek
static __always_inline unsigned long _dl_read(int fd, const void *buf, unsigned long count);

static __always_inline _syscall3(__off_t, __syscall_lseek, int, fd, __off_t, offset,
			int, whence);
static __always_inline ssize_t
_dl_pread(int fd, void *buf, size_t count, off_t offset)
{
  __off_t orig = __syscall_lseek (fd, 0, SEEK_CUR);
  ssize_t ret;

  if (orig == -1)
    return -1;

  if (__syscall_lseek (fd, offset, SEEK_SET) != offset)
    return -1;

  ret = _dl_read (fd, buf, count);

  if (__syscall_lseek (fd, orig, SEEK_SET) != orig)
    ((void(*)())0)();

  return ret;
}
#else
#define __NR___syscall_pread __NR_pread
static __always_inline _syscall5(ssize_t, __syscall_pread, int, fd, void *, buf,
			size_t, count, off_t, offset_hi, off_t, offset_lo);

static __always_inline ssize_t
_dl_pread(int fd, void *buf, size_t count, off_t offset)
{
  return(__syscall_pread(fd,buf,count,__LONG_LONG_PAIR (offset >> 31, offset)));
}
#endif
#endif
