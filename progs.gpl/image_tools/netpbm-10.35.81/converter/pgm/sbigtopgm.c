/*

    sbigtopgm.c - read a Santa Barbara Instruments Group CCDOPS file

    Note: All SBIG CCD astronomical cameras produce 14 bits or
	  (the ST-4 and ST-5) or 16 bits (ST-6 and later) per pixel.

		  Copyright (C) 1998 by John Walker
		       http://www.fourmilab.ch/

    If you find yourself having to add functionality included subsequent
    to the implementation of this program, you can probably find
    documentation of any changes to the SBIG file format on their
    Web site: http://www.sbig.com/

    Permission to use, copy, modify, and distribute this software and
    its documentation for any purpose and without fee is hereby
    granted, provided that the above copyright notice appear in all
    copies and that both that copyright notice and this permission
    notice appear in supporting documentation.	This software is
    provided "as is" without express or implied warranty.

*/

#include <string.h>

#include "pgm.h"
#include "nstring.h"

#define SBIG_HEADER_LENGTH  2048      /* File header length */

/*  looseCanon	--  Canonicalize a line from the file header so
		    items more sloppily formatted than those
		    written by CCDOPS are still accepted.  */

static void looseCanon(cp)
  char *cp;
{
    char *op = cp;
    char c;
    
    while ((c = *cp++) != 0) {
	if (!ISSPACE(c)) {
	    if (ISUPPER(c)) {
		c = tolower(c);
	    }
	    *op++ = c;
	}
    }
    *op++ = 0;
}

int main(argc, argv)
  int argc;
  char* argv[];
{
    FILE *ifp;
    gray *grayrow;
    register gray *gP;
    int argn, row;
    register int col;
    int maxval;
    int comp, rows, cols;
    char header[SBIG_HEADER_LENGTH];
    char *hdr;
    static char camera[80] = "ST-?";

    pgm_init(&argc, argv);

    argn = 1;

    if (argn < argc) {
	ifp = pm_openr(argv[argn]);
	argn++;
    } else {
	ifp = stdin;
    }

    if (argn != argc)
        pm_usage( "[sbigfile]" );

    if (fread(header, SBIG_HEADER_LENGTH, 1, ifp) < 1) {
        pm_error("error reading SBIG file header");
    }

    /*	Walk through the header and parse relevant parameters.	*/

    comp = -1;
    cols = -1;
    rows = -1;

    /*	The SBIG header specification equivalent to maxval is
        "Sat_level", the saturation level of the image.  This
	specification is optional, and was not included in files
	written by early versions of CCDOPS. It was introduced when it
	became necessary to distinguish 14-bit images with a Sat_level
	of 16383 from 16-bit images which saturate at 65535.  In
	addition, co-adding images or capturing with Track and
	Accumulate can increase the saturation level.  Since files
        which don't have a Sat_level line in the header were most
	probably written by early drivers for the ST-4 or ST-5, it
	might seem reasonable to make the default for maxval 16383,
	the correct value for those cameras.  I chose instead to use
	65535 as the default because the overwhelming majority of
        cameras in use today are 16 bit, and it's possible some
        non-SBIG software may omit the "optional" Sat_level
	specification.	Also, no harm is done if a larger maxval is
	specified than appears in the image--a simple contrast stretch
	will adjust pixels to use the full 0 to maxval range.  The
	converse, pixels having values greater than maxval, results in
	an invalid file which may cause problems in programs which
	attempt to process it.	*/

    maxval = 65535;

    hdr = header;

    for (;;) {
        char *cp = strchr(hdr, '\n');

	if (cp == NULL) {
            pm_error("malformed SBIG file header at character %d", hdr - header);
	}
	*cp = 0;
        if (strncmp(hdr, "ST-", 3) == 0) {
            char *ep = strchr(hdr + 3, ' ');

	    if (ep != NULL) {
		*ep = 0;
		strcpy(camera, hdr);
                *ep = ' ';
	    }
	}
	looseCanon(hdr);
        if (strncmp(hdr, "st-", 3) == 0) {
            comp = strstr(hdr, "compressed") != NULL;
        } else if (strncmp(hdr, "height=", 7) == 0) {
	    rows = atoi(hdr + 7);
        } else if (strncmp(hdr, "width=", 6) == 0) {
	    cols = atoi(hdr + 6);
        } else if (strncmp(hdr, "sat_level=", 10) == 0) {
	    maxval = atoi(hdr + 10);
        } else if (STREQ(hdr, "end")) {
	    break;
	}
	hdr = cp + 1;
    }

    if ((comp == -1) || (rows == -1) || (cols == -1)) {
        pm_error("required specification missing from SBIG file header");
    }

    fprintf(stderr, "SBIG %s %dx%d %s image, saturation level = %d.\n",
        camera, cols, rows, comp ? "compressed" : "uncompressed", maxval);

    if (maxval > PGM_OVERALLMAXVAL) {
        pm_error("Saturation level (%d levels) is too large.\n"
                 "This program's limit is %d.", maxval, PGM_OVERALLMAXVAL);
    }

    pgm_writepgminit(stdout, cols, rows, (gray) maxval, 0);
    grayrow = pgm_allocrow(cols);

#define DOSINT(fp) ((getc(fp) & 0xFF) | (getc(fp) << 8))

    for (row = 0; row < rows; row++) {
	int compthis = comp;

	if (comp) {
	    int rowlen = DOSINT(ifp); /* Compressed row length */

	    /*	If compression results in a row length >= the uncompressed
		row length, that row is output uncompressed.  We detect this
		by observing that the compressed row length is equal to
		that of an uncompressed row.  */

	    if (rowlen == cols * 2) {
		compthis = 0;
	    }
	}
	for (col = 0, gP = grayrow; col < cols; col++, gP++) {
	    gray g;

	    if (compthis) {

		if (col == 0) {
		    g = DOSINT(ifp);
		} else {
		    int delta = getc(ifp);

		    if (delta == 0x80) {
			g = DOSINT(ifp);
		    } else {
			g += ((signed char) delta);
		    }
		}
	    } else {
		g = DOSINT(ifp);
	    }
	    *gP = g;
	}
	pgm_writepgmrow(stdout, grayrow, cols, (gray) maxval, 0);
    }
    pm_close(ifp);
    pm_close(stdout);

    return 0;
}
