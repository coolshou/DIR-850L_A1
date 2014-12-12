/* bioradtopgm.c - convert a Biorad confocal image into a portable graymap
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
*/

#include <ctype.h>

#include "pgm.h"
#include "nstring.h"

#define BYTE unsigned char
#define BIORAD_HEADER_LENGTH 76
#define BYTE_TO_WORD(lsb,msb) (((BYTE) lsb) + (((BYTE) msb) << 8))

int
main( argc, argv )
    int argc;
    char* argv[];
    {
    FILE* ifp;
    gray* grayrow;
    register gray* gP;
    int argn, row, i;
    register int col, val, val2;
    int rows=0, cols=0, image_num= -1, image_count, byte_word, check_word;
    int maxval;
    BYTE buf[BIORAD_HEADER_LENGTH];
    const char* const usage = "[-image#] [Bioradfile]";


    pgm_init( &argc, argv );

    argn = 1;

    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
	if ( ISDIGIT( argv[argn][1] ))
	{
	    image_num = atoi( (argv[argn]+1) );
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

    for ( i = 0; i < BIORAD_HEADER_LENGTH; ++i )
    {
	val = getc( ifp );
	if ( val == EOF )
	    pm_error( "EOF / read error" );
	buf[ i ] = val;
    }

    cols = BYTE_TO_WORD(buf[0], buf[1]);
    rows = BYTE_TO_WORD(buf[2], buf[3]);
    image_count = BYTE_TO_WORD(buf[4], buf[5]);
    byte_word = BYTE_TO_WORD(buf[14], buf[15]);
    check_word = BYTE_TO_WORD(buf[54], buf[55]);

    if ( check_word != 12345 )
	pm_error( "Not a Biorad file" );

    if ( cols <= 0 )
	pm_error( "Strange image size, cols = %d", cols);

    if ( rows <= 0 )
	pm_error( "Strange image size, rows = %d", rows);

    if ( image_count <= 0 )
	pm_error( "Number of images in file is %d", image_count);

    if ( byte_word )
	maxval = 255;
    else
    {
	maxval = 65535;   /* Perhaps this should be something else */

    }

    pm_message( "Image size: %d cols, %d rows", cols, rows);
    pm_message( "%s",
	       (byte_word) ? "Byte image (8 bits)" : "Word image (16 bits)");

    if ( image_num < 0 )
	pm_message( "Input contains %d image%c",
		   image_count, (image_count > 1) ? 's' : '\0');
    else
    {
	if ( image_num >= image_count )
	    pm_error( "Cannot extract image %d, input contains only %d image%s",
		     image_num, image_count, (image_count > 1) ? "s" : "" );
	for ( i = (byte_word) ? image_num : image_num*2 ; i > 0 ; --i ) {
	    for ( row = 0; row < rows; ++row)
		for ( col = 0; col < cols; ++col )
		{
		    val = getc( ifp );
		    if ( val == EOF ) {
			pm_error( "EOF / read error" );
		    }
		}
	}

	pgm_writepgminit( stdout, cols, rows, (gray) maxval, 0 );
	grayrow = pgm_allocrow( cols );

	for ( row = 0; row < rows; ++row)
	{
	    for ( col = 0, gP = grayrow; col < cols; ++col )
	    {
		val = getc( ifp );
		if ( val == EOF )
		    pm_error( "EOF / read error" );
		if (byte_word)
		    *gP++ = val;
		else
		{
		    val2 = getc( ifp );
		    if ( val2 == EOF )
			pm_error( "EOF / read error" );
		    *gP++ = BYTE_TO_WORD(val, val2);
		}
	    }
	    pgm_writepgmrow( stdout, grayrow, cols, (gray) maxval, 0 );
	}

	pm_close( ifp );
	pm_close( stdout );

    }
    exit( 0 );
}
