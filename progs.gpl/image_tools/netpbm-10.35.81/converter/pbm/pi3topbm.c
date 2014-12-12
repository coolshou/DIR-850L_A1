/*
 * Convert a ATARI Degas .pi3 file to a portable bitmap file.
 *
 * Author: David Beckemeyer
 *
 * This code was derived from the original gemtopbm program written
 * by Diomidis D. Spinellis.
 *
 * (C) Copyright 1988 David Beckemeyer and Diomidis D. Spinellis.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 */

#include <stdio.h>
#include "pbm.h"

int
main(argc, argv)
	int             argc;
	char           *argv[];
{
	int             debug = 0;
	FILE           *f;
	int             x;
	int             i, k;
	int             c;
	int		rows, cols;
	bit		*bitrow;
	short res;
	int black, white;
	const char * const usage = "[-debug] [pi3file]";
	int argn = 1;

	pbm_init( &argc, argv );

	while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0')
	  {
	    if (pm_keymatch(argv[1], "-debug", 2))
	      debug = 1;
	    else
	      pm_usage (usage);
	    ++argn;
	  }

	if (argn == argc)
	    f = stdin;
	else
	  {
	    f = pm_openr (argv[argn]);
	    ++argn;
	  }

	if (argn != argc)
	  pm_usage (usage);

	if (pm_readbigshort (f, &res) == -1)
		pm_error ("EOF / read error");

	if (debug)
		pm_message ("resolution is %d", res);

	/* only handles hi-rez 640x400 */
	if (res != 2)
		pm_error( "bad resolution" );

	pm_readbigshort (f, &res);
	if (res == 0)
	  {
	    black = PBM_WHITE;
	    white = PBM_BLACK;
	  }
	else
	  {
	    black = PBM_BLACK;
	    white = PBM_WHITE;
	  }

	for (i = 1; i < 16; i++)
	  if (pm_readbigshort (f, &res) == -1)
	    pm_error ("EOF / read error");

	cols = 640;
	rows = 400;
	pbm_writepbminit( stdout, cols, rows, 0 );
	bitrow = pbm_allocrow( cols );

	for (i = 0; i < rows; ++i) {
		x = 0;
		while (x < cols) {
			if ((c = getc(f)) == EOF)
				pm_error( "end of file reached" );
			for (k = 0x80; k; k >>= 1) {
				bitrow[x] = (k & c) ? black : white;
				++x;
			}
		}
		pbm_writepbmrow( stdout, bitrow, cols, 0 );
	}
	pm_close( f );
	pm_close( stdout );
	exit(0);
}
