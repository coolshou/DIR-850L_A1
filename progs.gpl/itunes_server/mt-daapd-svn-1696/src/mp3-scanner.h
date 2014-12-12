/*
 * $Id: mp3-scanner.h 1423 2006-11-06 03:42:38Z rpedde $
 * Header file for mp3 scanner and monitor
 *
 * Copyright (C) 2003 Ron Pedde (ron@pedde.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MP3_SCANNER_H_
#define _MP3_SCANNER_H_

//#include <mslib.h>
#include <sys/types.h>
#include "ff-dbstruct.h"  /* for MP3FILE */


#define SCAN_NOT_COMPDIR  0
#define SCAN_IS_COMPDIR   1
#define SCAN_TEST_COMPDIR 2

#define WINAMP_GENRE_UNKNOWN 148

/* save char set and analyze it curtis@alpha 04_01_2009 */
#define CHAR_SET_LIST_LEN 30
#define CHAR_SET_LEN 30

/* debug for scan-mp3.c and mp3-scanner.c curtis@alpha 04_07_2009 */
//#define FILE_SCAN
#ifdef FILE_SCAN
#define	DBG_FILE_SCAN(x)	x
	#else
#define	DBG_FILE_SCAN(x)
#endif

int total_mp3;
int unicode16_text;

extern void scan_filename(char *path, int compdir, char *extensions, char *code_page);

extern char *scan_winamp_genre[];
extern int scan_init(char **patharray,char *code_page);
extern void make_composite_tags(MP3FILE *song);

#ifndef TRUE
# define TRUE  1
# define FALSE 0
#endif


#endif /* _MP3_SCANNER_H_ */
