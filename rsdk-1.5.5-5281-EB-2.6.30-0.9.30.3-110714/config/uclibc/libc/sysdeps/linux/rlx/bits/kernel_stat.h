#ifndef _BITS_STAT_STRUCT_H
#define _BITS_STAT_STRUCT_H

#ifndef _LIBC
#error bits/kernel_stat.h is for internal uClibc use only!
#endif

/* This file provides whatever this particular arch's kernel thinks
 * struct kernel_stat should look like...  It turns out each arch has a
 * different opinion on the subject... */

#include <sgidefs.h>

struct kernel_stat {
	__kernel_dev_t	st_dev;
	long		st_pad1[3];
	__kernel_ino_t	st_ino;
	__kernel_mode_t	st_mode;
	__kernel_nlink_t st_nlink;
	__kernel_uid_t	st_uid;
	__kernel_gid_t	st_gid;
	__kernel_dev_t	st_rdev;
	long		st_pad2[2];
	__kernel_off_t	st_size;
	long		st_pad3;
	struct timespec	st_atim;
	struct timespec	st_mtim;
	struct timespec	st_ctim;
	long		st_blksize;
	long		st_blocks;
	long		st_pad4[14];
};

struct kernel_stat64 {
	unsigned long	st_dev;
	unsigned long	st_pad0[3];	/* Reserved for st_dev expansion  */
	unsigned long long	st_ino;
	__kernel_mode_t	st_mode;
	__kernel_nlink_t st_nlink;
	__kernel_uid_t	st_uid;
	__kernel_gid_t	st_gid;
	unsigned long	st_rdev;
	unsigned long	st_pad1[3];	/* Reserved for st_rdev expansion  */
	long long	st_size;
	struct timespec	st_atim;
	struct timespec	st_mtim;
	struct timespec	st_ctim;
	unsigned long	st_blksize;
	unsigned long	st_pad2;
	long long	st_blocks;
};

#endif	/*  _BITS_STAT_STRUCT_H */

