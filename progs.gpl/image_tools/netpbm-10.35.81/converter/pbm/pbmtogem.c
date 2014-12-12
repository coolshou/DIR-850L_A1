/* pbmtogem.c - read a portable bitmap and produce a GEM .img file
**
** Author: David Beckemeyer (bdt!david)
**
** Much of the code for this program was taken from other
** pbmto* programs.  I just modified the code to produce
** a .img header and generate .img "Bit Strings".
**
** Thanks to Diomidis D. Spinellis for the .img header format.
**
** Copyright (C) 1988 by David Beckemeyer (bdt!david) and Jef Poskanzer.
**
** Modified by Johann Haider to produce Atari ST compatible files
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/
/*
*  92/07/11 jh
*
*  Changes made to this program:
*  changed header length to count words to conform with Atari ST documentation
*  removed rounding of the imagewidth to the next word boundary
*  removed arbitrary limit to imagewidth
*  changed pattern length to 1 to simplify locating of compressable parts
*	in real world images
*  add solid run and pattern run compression
*
*  Deficiencies:
*  Compression of repeated scanlines not added
*  
*	Johann Haider (jh@fortec.tuwien.ac.at)
*
* 94/01/31 Andreas Schwab (schwab@ls5.informatik.uni-dortmund.de)
* Changed to remove architecture dependencies
* Added compression of repeated scanlines
*/

#include <stdio.h>
#include <string.h>
#include "pbm.h"

#define SOLID_0 0
#define SOLID_1 0xff
#define MINRUN 4
#define putsolid(v,c) putc((v&0x80)|c, stdout)
#define putpattern(v,c) putc(0, stdout);putc(c, stdout);putc(v, stdout)

static void putinit ARGS ((int rows, int cols));
static void putbit ARGS(( bit b ));
static void putitem ARGS(( void ));
static void putrow ARGS(( void ));
static void flushrow ARGS ((void));
static void putstring ARGS((register unsigned char *p, register int n));

int
main( argc, argv )
    int argc;
    char* argv[];
    {
    FILE* ifp;
    bit* bitrow;
    register bit* bP;
    int rows, cols, format, row, col;

    pbm_init( &argc, argv );

    if ( argc > 2 )
	pm_usage( "[pbmfile]" );

    if ( argc == 2 )
	ifp = pm_openr( argv[1] );
    else
	ifp = stdin;

    pbm_readpbminit( ifp, &cols, &rows, &format );

    bitrow = pbm_allocrow( cols );

    putinit (rows, cols);
    for ( row = 0; row < rows; ++row )
	{
#ifdef DEBUG
	fprintf (stderr, "row %d\n", row);
#endif
	pbm_readpbmrow( ifp, bitrow, cols, format );
        for ( col = 0, bP = bitrow; col < cols; ++col, ++bP )
	    putbit( *bP );
        putrow( );
        }
    flushrow ();

    pm_close( ifp );


    exit( 0 );
    }

static short item;
static int outcol, outmax;
static short bitsperitem, bitshift;
static short linerepeat;
static unsigned char *outrow, *lastrow;

static void
putinit (rows, cols)
     int rows, cols;
{
  if (pm_writebigshort (stdout, (short) 1) == -1 /* Image file version */
      || pm_writebigshort (stdout, (short) 8) == -1 /* Header length */
      || pm_writebigshort (stdout, (short) 1) == -1 /* Number of planes */
      || pm_writebigshort (stdout, (short) 1) == -1 /* Pattern length */
      || pm_writebigshort (stdout, (short) 372) == -1 /* Pixel width */
      || pm_writebigshort (stdout, (short) 372) == -1 /* Pixel height */
      || pm_writebigshort (stdout, (short) cols) == -1
      || pm_writebigshort (stdout, (short) rows) == -1)
    pm_error ("write error");
  item = 0;
  bitsperitem = 0;
  bitshift = 7;
  outcol = 0;
  outmax = (cols + 7) / 8;
  outrow = (unsigned char *) pm_allocrow (outmax, sizeof (unsigned char));
  lastrow = (unsigned char *) pm_allocrow (outmax, sizeof (unsigned char));
  linerepeat = -1;
}

#if __STDC__
static void
putbit( bit b )
#else /*__STDC__*/
static void
putbit( b )
    bit b;
#endif /*__STDC__*/
    {
    if ( bitsperitem == 8 )
	putitem( );
    ++bitsperitem;
    if ( b == PBM_BLACK )
	item += 1 << bitshift;
    --bitshift;
    }

static void
putitem( )
    {
    outrow[outcol++] = item;
    item = 0;
    bitsperitem = 0;
    bitshift = 7;
    }

static void
putstring (p, n)
register unsigned char *p;
register int n;
{
#ifdef DEBUG
    fprintf (stderr, "Bitstring, length: %d, pos %d\n", n, outcol);
#endif
    (void) putc((char) 0x80, stdout);     /* a Bit string */
    (void) putc(n, stdout);	/* count */
    fwrite( p, n, 1, stdout );
}

static void
putrow( )
{
  if (bitsperitem > 0)
    putitem ();
  outcol = 0;
  if (linerepeat == -1 || linerepeat == 255
      || memcmp (outrow, lastrow, outmax) != 0)
    {
      unsigned char *temp;
      if (linerepeat != -1) /* Unless first line */
	flushrow ();
      /* Swap the pointers */
      temp = outrow; outrow = lastrow; lastrow = temp;
      linerepeat = 1;
    }
  else
    /* Repeated line */
    linerepeat++;
}

static void
flushrow( )
    {
    register unsigned char *outp, *p, *q;
    register int count;
    int col = outmax;

    if (linerepeat > 1)
      {
	/* Put out line repeat count */
	fwrite ("\0\0\377", 3, 1, stdout);
	putchar (linerepeat);
      }
    for (outp = p = lastrow; col > 0;)
    {
	    for (q = p, count=0; (count < col) && (*q == *p); q++,count++);
	    if (count > MINRUN)
	    {
		if (p > outp)
		{
		    putstring (outp, p-outp);
		    outp = p;
		}
		col -= count;
		switch (*p)
		{
		case SOLID_0:
#ifdef DEBUG
/*			if (outcol > 0) */
			fprintf (stderr, "Solid run 0, length: %d\n", count);
#endif
			putsolid (SOLID_0, count);
			break;

		case SOLID_1:
#ifdef DEBUG
			fprintf (stderr, "Solid run 1, length: %d, pos %d\n", count, outcol);
#endif
			putsolid (SOLID_1, count);
			break;
		default:
#ifdef DEBUG
			fprintf (stderr, "Pattern run, length: %d\n", count);
#endif
			putpattern (*p, count);
			break;
		}
		outp = p = q;
	    }
	    else
	    {
		p++;
		col--;
	    }
    }		
    if (p > outp)
         putstring (outp, p-outp);
    if (ferror (stdout))
      pm_error ("write error");
}

