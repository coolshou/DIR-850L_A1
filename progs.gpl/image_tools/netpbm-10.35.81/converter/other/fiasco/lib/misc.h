/*
 *  misc.h
 *
 *  Written by:		Stefan Frank
 *			Ullrich Hafner
 *		
 *  This file is part of FIASCO (�F�ractal �I�mage �A�nd �S�equence �CO�dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:49:37 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef _MISC_H
#define _MISC_H

#include "config.h"

#if TIME_WITH_SYS_TIME
#	include <sys/time.h>
#	include <time.h>
#else  /* not TIME_WITH_SYS_TIME */
#	if HAVE_SYS_TIME_H
#		include <sys/time.h>
#	else /* not HAVE_SYS_TIME_H */
#		include <time.h>
#	endif /* not HAVE_SYS_TIME_H */
#endif /* not TIME_WITH_SYS_TIME */

#include <stdio.h>
#include "types.h"
#include "bit-io.h"

enum action_e {START, STOP};

void *
Calloc (size_t n, size_t size);
void
Free (void *memory);
unsigned
prg_timer (clock_t *ptimer, enum action_e action);
int 
read_int(FILE *infile);
real_t 
read_real(FILE *infile);
unsigned
read_rice_code (unsigned rice_k, bitfile_t *input);
void
write_rice_code (unsigned value, unsigned rice_k, bitfile_t *output);
void
write_bin_code (unsigned value, unsigned maxval, bitfile_t *output);
unsigned
bits_bin_code (unsigned value, unsigned maxval);
unsigned
bits_rice_code (unsigned value, unsigned rice_k);
unsigned
read_bin_code (unsigned maxval, bitfile_t *input);
unsigned *
init_clipping (void);
real_t
variance (const word_t *pixels, unsigned x0, unsigned y0,
	  unsigned width, unsigned height, unsigned cols);

#ifndef HAVE_MEMMOVE
void *
memmove(void *dest, const void *src, size_t n);
#endif /* not HAVE_MEMMOVE */

double
Log2 (double x);
#ifndef HAVE_STRDUP
char *
strdup (const char *s);
#endif
#ifndef HAVE_STRCASECMP
bool_t
strcaseeq (const char *s1, const char *s2);
#else  /* HAVE_STRCASECMP */
int
strcasecmp (const char *s1, const char *s2);
#define strcaseeq(s1, s2) (strcasecmp ((s1), (s2)) == 0)
#endif /* HAVE_STRCASECMP */

int
sort_asc_word (const void *value1, const void *value2);
int
sort_desc_word (const void *value1, const void *value2);
int
sort_asc_pair (const void *value1, const void *value2);
int
sort_desc_pair (const void *value1, const void *value2);

#endif /* not _MISC_H */

