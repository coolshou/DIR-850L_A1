/* pnmtosir.c - read a portable anymap and produce a Solitaire Image Recorder
**		file (MGI TYPE 11 or MGI TYPE 17)
**
** Copyright (C) 1991 by Marvin Landis
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "pnm.h"

#define MAXCOLORS 256

int main(int argc, char * argv[]) {
    FILE* ifp;
    xel** xels;
    register xel* xP;
    const char* dumpname;
    int rows, cols, format, row, col;
    int m, n;
    int grayscale;
    xelval maxval;
    const char* const usage = "[pnmfile]";
    unsigned char ub;
    unsigned short Header[16];
    unsigned short LutHeader[16];
    unsigned short Lut[2048];

    pnm_init( &argc, argv );

    if ( argc > 2 )
        pm_usage( usage );

    if ( argc == 2 )
	{
        dumpname = argv[1];
        ifp = pm_openr( argv[1] );
	}
    else
	{
        dumpname = "Standard Input";
        ifp = stdin;
	}
    
    xels = pnm_readpnm( ifp, &cols, &rows, &maxval, &format );
    pm_close( ifp );
    
    /* Figure out the colormap. */
    switch ( PNM_FORMAT_TYPE(format) )
	{
	case PPM_TYPE:
        grayscale = 0;
        pm_message( "Writing a 24-bit SIR format (MGI TYPE 11)" );
        break;

    case PGM_TYPE:
        grayscale = 1;
        pm_message( "Writing a grayscale SIR format (MGI TYPE 17)" );
        break;

	default:
        grayscale = 1;
        pm_message( "Writing a monochrome SIR format (MGI TYPE 17)" );
        break;
	}

    /* Set up the header. */
    Header[0] = 0x3a4f;
    Header[1] = 0;
    if (grayscale)
        Header[2] = 17;
    else
        Header[2] = 11;
    Header[3] = cols;
    Header[4] = rows;
    Header[5] = 0;
    Header[6] = 1;
    Header[7] = 6;
    Header[8] = 0;
    Header[9] = 0;
    for (n = 0; n < 10; n++)
        pm_writelittleshort(stdout,Header[n]);
    for (n = 10; n < 256; n++)
        pm_writelittleshort(stdout,0);

    /* Create color map */
    LutHeader[0] = 0x1524;
    LutHeader[1] = 0;
    LutHeader[2] = 5;
    LutHeader[3] = 256;
    LutHeader[4] = 256;
    for (n = 0; n < 5; n++)
        pm_writelittleshort(stdout,LutHeader[n]);
    for (n = 5; n < 256; n++)
        pm_writelittleshort(stdout,0);
 
    for(n = 0; n < 3; n ++)
        for (m = 0; m < 256; m++)
            Lut[m * 4 + n] = m << 8;
    for (n = 0; n < 1024; n++)
        pm_writelittleshort(stdout,Lut[n]);
 
    /* Finally, write out the data. */
    switch ( PNM_FORMAT_TYPE(format) )
    {
	case PPM_TYPE:
	    for ( row = 0; row < rows; ++row )
            for ( col = 0, xP = xels[row]; col < cols; ++col, ++xP )
            {
                ub = (char) ( PPM_GETR( *xP ) * ( 255 / maxval ) ); 
                fputc( ub, stdout );
            }
        for ( row = 0; row < rows; ++row )
            for ( col = 0, xP = xels[row]; col < cols; ++col, ++xP )
            {  
                ub = (char) ( PPM_GETG( *xP ) * ( 255 / maxval ) );
                fputc( ub, stdout );
            }
        for ( row = 0; row < rows; ++row )
            for ( col = 0, xP = xels[row]; col < cols; ++col, ++xP )
            {  
                ub = (char) ( PPM_GETB( *xP ) * ( 255 / maxval ) );
                fputc( ub, stdout );
            }
	    break;

    default:
        for ( row = 0; row < rows; ++row )
            for ( col = 0, xP = xels[row]; col < cols; ++col, ++xP )
            {
                register unsigned long val;

                val = PNM_GET1( *xP );
                ub = (char) ( val * ( 255 / maxval ) );
                fputc( ub, stdout );
            }
        break;
    }

    exit( 0 );
}

