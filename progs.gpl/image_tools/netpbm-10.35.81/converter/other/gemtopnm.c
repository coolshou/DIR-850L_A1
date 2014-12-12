/*
 * Convert a GEM .img file to PBM
 *
 * Author: Diomidis D. Spinellis
 * (C) Copyright 1988 Diomidis D. Spinellis.
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
 *
 * Comments and additions should be sent to the author:
 *
 *                     Diomidis D. Spinellis
 *                     1 Myrsinis Str.
 *                     GR-145 62 Kifissia
 *                     GREECE
 *
 * 92/07/11 Johann Haider
 * Changed to read from stdin if file is omitted
 * Changed to handle line length not a multipe of 8
 *
 * 94/01/31 Andreas Schwab (schwab@ls5.informatik.uni-dortmund.de)
 * Changed to remove architecture dependency and conform to
 * PBM coding standard.
 * Added more tests for garbage.
 *
 * 2000/04/30 John Elliott <jce@seasip.demon.co.uk> Added ability to
 * read 4-plane color IMG files.  Therefore changed from PBM to PPM.
 * Bryan changed it further to use the PNM facilities so it outputs
 * both PBM and PPM in the Netpbm tradition.  Name changed from
 * gemtopbm to gemtopnm.  
 */

#include <assert.h>
#include "pnm.h"

#define MAXVAL 3
#define LIGHT  2
#define DARK   1
#define BLACK  0

char pattern[8];

static void getinit ARGS ((FILE *file, int *colsP, int *rowsP, int *padrightP,
			   int *patlenP, int *planesP));

int
main(argc, argv)
	int             argc;
	char           *argv[];
{
	int     debug = 0;
	FILE    *f;
    int     row;
	int     rows, cols, padright, patlen, planes;
      /* attributes of input image */
    int type;  /* The format type (PBM/PPM) of the output image */
	bit	*bitrow[4];
      /* One row of input, one or four planes.  (If one, only [0] is defined)*/
    xel * xelrow;  /* One row of output */
	const char * const usage = "[-debug] [gem IMG file]";
	int argn;

/* Process multiple planes by maintaining a separate row of bits for each
 * plane. In a single-plane image, all we have to do is write out the 
 * first plane; in a multiple-plane image, we combine them just before writing
 * out the row.
 */
	pnm_init( &argc, argv );
    
    argn = 1;

	while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0')
	  {
	    if (pm_keymatch(argv[1], "-debug", 2))
	      debug = 1;
	    else
	      pm_usage (usage);
	    ++argn;
	  }

	if (argc == argn)
		f = stdin;
	else {
		f = pm_openr (argv[argn]);
		++argn;
	}

	if (argn != argc)
	  pm_usage (usage);

	getinit (f, &cols, &rows, &padright, &patlen, &planes);

    if (planes == 1) 
        type = PBM_TYPE;
    else 
        type = PPM_TYPE;

	pnm_writepnminit( stdout, cols, rows, MAXVAL, type, 0 );

    { 
        /* allocate input row data structure */
        int plane;
        for (plane = 0; plane < planes; plane++) 
            bitrow[plane] = malloc (cols + padright);
    }
    xelrow = pnm_allocrow(cols+padright);   /* Output row */

	for (row = 0; row < rows; ) {
      int linerep;
      int plane;

	  linerep = 1;
	  for (plane = 0; plane < planes; plane++) {
        int col;
		col = 0;
		while (col < cols) {
            int c;
			switch (c = getc(f)) {
			case 0x80:	/* Bit String */
            {
                int j;
				c = getc(f);	/* Byte count */
				if (debug)
                  pm_message("bit string of %d bytes", c);
				
				if (col + c * 8 > cols + padright)
				  pm_error ("bad byte count");
				for (j = 0; j < c; ++j) {
                    int cc, k;
					cc = getc(f);
					for (k = 0x80; k; k >>= 1) {
						bitrow[plane][col] = (k & cc) ? 0 : 1;
						++col;
					}
				}
            }
            break;
			case 0:		/* Pattern run */
            {
                int j, l;
				c = getc(f);	/* Repeat count */
				if (debug)
					pm_message("pattern run of %d repetitions",	c);
                /* line repeat */
                if (c == 0) {
                    c = getc(f);
                    if (c != 0x00ff)
                        pm_error( "badly formed line repeat" );
                    linerep = getc(f);
                    break;
                }
				fread (pattern, 1, patlen, f);
				if (col + c * patlen * 8 > cols + padright)
				  pm_error ("bad pattern repeat count");
				for (j = 0; j < c; ++j)
					for (l = 0; l < patlen; ++l) {
                        int k;
						for (k = 0x80; k; k >>= 1) {
							bitrow[plane][col] = (k & pattern[l]) ? 0 : 1;
							++col;
						}
                    }
            }
            break;

			default:	/* Solid run */
            {
                int l, j;
				if (debug)
					pm_message("solid run of %d bytes %s", c & 0x7f,
                               c & 0x80 ? "on" : "off" );
                /* each byte had eight bits DSB */
                l = (c & 0x80) ? 0: 1;
                c = (c & 0x7f) * 8;
                if (col + c > cols + padright)
                    pm_error ("bad solid run repeat count");
                for (j = 0; j < c; ++j) {
                    bitrow[plane][col] = l;
					++col;
                }
            }
				break;

			case EOF:	/* End of file */
				pm_error( "end of file reached" );

			}
		}
                if ( debug )
                        pm_message( "EOL plane %d row %d", plane, row );
                if (col != cols + padright)
                        pm_error( "EOL beyond edge" );
	  }

	  if (planes == 4) {
          /* Construct a pixel from the 4 planes of bits for this row */
          int col;
          for (col = 0; col < cols; col++) {
            int r, g, b, i;

            const int r_bit = !bitrow[0][col];
            const int g_bit = !bitrow[1][col];
            const int b_bit = !bitrow[2][col];
            i = bitrow[3][col];

			/* Deal with weird GEM palette - white/black/gray are
               encoded oddly 
            */
			if (r_bit == g_bit && g_bit == b_bit) {
                /* It's black, white, or gray */
				if (r_bit && i) r = LIGHT;
				else if (r_bit) r = BLACK;
				else if (i) r = MAXVAL;
				else r = DARK;
				g = b = r;	
			} else {
                /* It's one of the twelve colored colors */
                if (!i) {
                    /* Low intensity */
                    r = r_bit * LIGHT; 
                    g = g_bit * LIGHT;
                    b = b_bit * LIGHT;
                } else {
                    /* Normal intensity */
                    r = r_bit * MAXVAL;
                    g = g_bit * MAXVAL;
                    b = b_bit * MAXVAL;
                }
            }
            PPM_ASSIGN(xelrow[col], r, g, b);
		}
	  } else {
          int col;
          for (col = 0; col < cols; col++) 
              PNM_ASSIGN1(xelrow[col], bitrow[0][col]);
      }
	  while (linerep--) {
		pnm_writepnmrow( stdout, xelrow, cols, MAXVAL, type, 0 );
		++row;
	  }
	}
    pnm_freerow(xelrow);
	pm_close( f );
	pm_close( stdout );
	exit(0);
}


static void
getinit (file, colsP, rowsP, padrightP, patlenP, planesP)
     FILE *file;
     int *colsP;
     int *rowsP;
     int *padrightP;
     int *patlenP;
     int *planesP;
{
  short s;
  short headlen;

  if (pm_readbigshort (file, &s) == -1) /* Image file version */
    pm_error ("EOF / read error");
  if (s != 1)
    pm_error ("unknown version number (%d)", (int) s);
  if (pm_readbigshort (file, &headlen) == -1) /* Header length in words */
    pm_error ("EOF / read error");
  if (headlen < 8)
    pm_error ("short header (%d)", (int) headlen);
  if (pm_readbigshort (file, &s) == -1) /* Number of planes */
    pm_error ("EOF / read error");
  if (s != 4 && s != 1)
    pm_error ("This program can interpret IMGs with only 1 or 4 planes");
  *planesP = s;
  if (pm_readbigshort (file, &s) == -1) /* Pattern definition length (bytes) */
    pm_error ("EOF / read error");
  if (s < 1 || s > 8)
    pm_error ("illegal pattern length (%d)", (int) s);
  *patlenP = (int) s;
  if (pm_readbigshort (file, &s) == -1 /* Pixel height (microns) */
      || pm_readbigshort (file, &s) == -1 /* Pixel height (microns) */
      || pm_readbigshort (file, &s) == -1) /* Scan line width */
    pm_error ("EOF / read error");
  *colsP = (int) s;
  if (pm_readbigshort (file, &s) == -1) /* Number of scan line items */
    pm_error ("EOF / read error");
  *rowsP = (int) s;
  *padrightP = 7 - ((*colsP + 7) & 7);

  headlen -= 8;
  while (headlen-- > 0)
    {
      (void) getc (file);
      (void) getc (file);
    }
}

