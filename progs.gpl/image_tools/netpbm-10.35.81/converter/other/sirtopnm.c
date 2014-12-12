/* sirtopnm.c - read a Solitaire Image Recorder file and write a portable anymap
**
** Copyright (C) 1991 by Marvin Landis.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "pnm.h"

int main( argc, argv )
int argc;
char* argv[];
{
    FILE *ifp;
    xel *xelrow, *xP;
    unsigned char *sirarray;
    int rows, cols, row, format, picsize, planesize;
    register int col, i;
    short info;

    pnm_init( &argc, argv );

    if ( argc > 2 )
	pm_usage( "[sirfile]" );

    if ( argc == 2 )
	ifp = pm_openr( argv[1] );
    else
	ifp = stdin;

    pm_readlittleshort( ifp, &info );
    if ( info != 0x3a4f)
	pm_error( "Input file is not a Solitaire file" );
    pm_readlittleshort( ifp, &info );
    pm_readlittleshort( ifp, &info );
    if ( info == 17 )
    {
	format = PGM_TYPE;
    }
    else if ( info == 11 )
    {
	format = PPM_TYPE;
    }
    else
	pm_error( "Input is not MGI TYPE 11 or MGI TYPE 17" );
    pm_readlittleshort( ifp, &info );
    cols = (int) ( info );
    pm_readlittleshort( ifp, &info );
    rows = (int) ( info );
    for ( i = 1; i < 1531; i++ )
	pm_readlittleshort( ifp, &info );

    pnm_writepnminit( stdout, cols, rows, 255, format, 0 );
    xelrow = pnm_allocrow( cols );
    switch ( PNM_FORMAT_TYPE(format) )
    {
	case PGM_TYPE:
            pm_message( "Writing a PGM file" );
	    for ( row = 0; row < rows; ++row )
	    {
	        for ( col = 0, xP = xelrow; col < cols; col++, xP++ )
	        	PNM_ASSIGN1( *xP, fgetc( ifp ) );
	        pnm_writepnmrow( stdout, xelrow, cols, 255, format, 0 );
	    }
	    break;
	case PPM_TYPE:
	    picsize = cols * rows * 3;
	    planesize = cols * rows;
            if ( !( sirarray = (unsigned char*) malloc( picsize ) ) ) 
	        pm_error( "Not enough memory to load SIR file" );
	    if ( fread( sirarray, 1, picsize, ifp ) != picsize )
	        pm_error( "Error reading SIR file" );
            pm_message( "Writing a PPM file" );
            for ( row = 0; row < rows; row++ )
	    {
	        for ( col = 0, xP = xelrow; col < cols; col++, xP++ )
        	    PPM_ASSIGN( *xP, sirarray[row*cols+col],
				 sirarray[planesize + (row*cols+col)],
				 sirarray[2*planesize + (row*cols+col)] );
                pnm_writepnmrow( stdout, xelrow, cols, 255, format, 0 );
	    }
	    break;
	default:
	    pm_error( "Shouldn't happen" );
    }

    pm_close( ifp );

    exit( 0 );
}
