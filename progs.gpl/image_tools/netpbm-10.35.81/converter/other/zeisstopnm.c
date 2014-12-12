/* zeisstopnm.c - convert a Zeiss confocal image into a portable anymap
**
** Copyright (C) 1993 by Oliver Trepte, oliver@fysik4.kth.se
**
** Derived from the pbmplus package,
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
**
** This conversion utility is based on a mail from Keith Bartels to
** the confocal mail group (confocal@ubvm.cc.buffalo.edu) in June 1993.
**
** I'm including here a description of the Zeiss confocal image format. I
** obtained this over the phone on 4-10-1991 form a Zeiss Engineer, and from
** what I hear, it is probably not correct for the new scopes.
** Zeiss puts its header information and the end of the file, so I call it
** a tailer.  This is nice because most conversions programs that work with raw
** image files will read the image simply ignore the tailer.  The file contains:
**
** The image data: NxN  1 byte pixels (where N is the number of pixels in a
** scan line) in a standard raw rastering order.
**
** The tailer contains:
** 256 bytes: the blue Look-Up-Table (LUT)
** 256 bytes: the red LUT
** 256 bytes: the green LUT
** 8 bytes: empty
** 2 bytes: no. of columns in the image. hi-byte, low-byte
** 2 bytes: no. of rows in the image.
** 2 bytes: x-position of upper left pixel  (0 for 512x512 images)
** 2 bytes: y-position of upper left pixel  (0 for 512x512 images)
** 16 bytes: empty
** 32 bytes: test from upper right corner of image, ASCII.
** 128 bytes: text from the bottom two rows of the screen, ASCII.
** 64 bytes: reserved
** -----------
** 1024 bytes TOTAL
**
** So, image files contain NxN + 1024 bytes.
**
** Keith Bartels
** keith@VISION.EE.UTEXAS.EDU
**
*/

#include "pnm.h"

int
main( argc, argv )
    int argc;
    char* argv[];
    {
    FILE* ifp;
    int argn, row, i;
    register int col;
    int rows=0, cols=0;
    int format = 0;
    xel* xelrow;
    register xel* xP;
    char* buf = NULL;
    unsigned char *lutr, *lutg, *lutb;
    long nread = 0;
    unsigned char* byteP;
    const char* const usage = "[-pgm|-ppm] [Zeissfile]";


    pnm_init( &argc, argv );

    argn = 1;

    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
	{
	if ( pm_keymatch( argv[argn], "-pgm", 3 ) )
	    {
	    if ( argn >= argc )
		pm_usage( usage );
	    format = PGM_TYPE;
	    }
	else if ( pm_keymatch( argv[argn], "-ppm", 3 ) )
	    {
	    if ( argn >= argc )
		pm_usage( usage );
	    format = PPM_TYPE;
	    }
	else
	    pm_usage( usage );
	++argn;
	}

    if ( argn < argc )
	{
	ifp = pm_openr( argv[argn] );
	++argn;
	}
    else
	ifp = stdin;

    if ( argn != argc )
	pm_usage( usage );

    /* Read the image to a buffer */

    buf = pm_read_unknown_size( ifp, &nread );

    /* Check the format of the file */

    if (nread <=1024)
	pm_error( "Input file not in Zeiss format (too small)" );

    lutg = (unsigned char *)buf+(nread-1024+512);
    lutr = (unsigned char *)buf+(nread-1024+256);
    lutb = (unsigned char *)buf+(nread-1024);

    cols = ((unsigned char) buf[nread-1024+768+8]) +
	(((unsigned char) buf[nread-1024+768+9]) << 8);
    rows = ((unsigned char) buf[nread-1024+768+10]) +
	(((unsigned char) buf[nread-1024+768+11]) << 8);

    if ( cols <= 0 )
	pm_error( "invalid cols: %d", cols );
    if ( rows <= 0 )
	pm_error( "invalid rows: %d", rows );

    if (cols*rows != nread-1024)
	pm_error( "Hmm, %d rows, %d cols, %ld total image size",
		 rows, cols, nread-1024);

    /* Choose pgm or ppm */
    /* If the LUTs all contain 0,1,2,3,4..255, it is a pgm file */

    for (i=0; i<256 && format==0; i++)
	if (lutr[i] != i || lutg[i] != i || lutb[i] != i)
	    format = PPM_TYPE;

    if (format == 0)
	format = PGM_TYPE;

    pnm_writepnminit( stdout, cols, rows, 255, format, 0 );
    xelrow = pnm_allocrow( cols );
    byteP = (unsigned char *) buf;

    switch ( PNM_FORMAT_TYPE(format) )
        {
        case PGM_TYPE:
        pm_message( "writing PGM file, %d rows %d columns", rows, cols );
        break;

        case PPM_TYPE:
        pm_message( "writing PPM file, %d rows %d columns", rows, cols );
        break;

        default:
        pm_error( "shouldn't happen" );
        }

    for ( row = 0; row < rows; ++row )
    {
	switch ( PNM_FORMAT_TYPE(format) )
	{
	case PGM_TYPE:
	    for ( col = 0, xP = xelrow; col < cols; ++col, ++xP, ++byteP )
		PNM_ASSIGN1( *xP, *byteP );
	    break;

	case PPM_TYPE:
	    for ( col = 0, xP = xelrow; col < cols; ++col, ++xP, ++byteP )
		PPM_ASSIGN( *xP, lutr[*byteP], lutg[*byteP], lutb[*byteP] );
	    break;

        default:
	    pm_error( "shouldn't happen" );
        }

	pnm_writepnmrow( stdout, xelrow, cols, 255, format, 0 );
    }

    free( buf );
    pm_close( stdout );

    exit( 0 );
}
