/* neotoppm.c - read a Neochrome NEO file and produce a portable pixmap
**
** Copyright (C) 2001 by Teemu Hukkanen <tjhukkan@iki.fi>
**  Based on pi1toppm by Steve Belczyk (seb3@gte.com) and Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "ppm.h"

#define ROWS 200
#define COLS 320
#define MAXVAL 7

static short screen[ROWS*COLS/4];        /* simulates the Atari's video RAM */

int
main( int argc, char * argv[] ) {
    FILE* ifp;
    pixel pal[16];                      /* Degas palette */
    pixel* pixelrow;
    int row;

    ppm_init( &argc, argv );

    /* Check args. */
    if ( argc > 2 )
        pm_usage( "[neofile]" );

    if ( argc == 2 )
        ifp = pm_openr( argv[1] );
    else
        ifp = stdin;

    /* Check header */
    {
        /* Flag */
        short j;
        pm_readbigshort (ifp, &j);
        if ( j != 0 )
            pm_error( "not a NEO file" );
    }
    {
        /* Check resolution word */
        short j;
        pm_readbigshort (ifp, &j);
        if ( j != 0 )
            pm_error( "not a NEO file" );
    }
    {
        int i;
        /* Read the palette. */
        for ( i = 0; i < 16; ++i ) {
            short j;
            
            pm_readbigshort (ifp, &j);
            PPM_ASSIGN( pal[i],
                        ( j & 0x700 ) >> 8,
                        ( j & 0x070 ) >> 4,
                        ( j & 0x007 ) );
        }
    }

    /* Skip rest of header */
    fseek(ifp, 128, SEEK_SET);

    {
        /* Read the screen data */
        int i;
        for ( i = 0; i < ROWS*COLS/4; ++i )
            pm_readbigshort( ifp, &screen[i] );
    }
    pm_close( ifp );

    /* Ok, get set for writing PPM. */
    ppm_writeppminit( stdout, COLS, ROWS, MAXVAL, 0 );
    pixelrow = ppm_allocrow( COLS );

    /* Now do the conversion. */
    for ( row = 0; row < ROWS; ++row ) {
        int col;
        for ( col = 0; col < COLS; ++col ) {
            int c, ind, b, plane;

            ind = 80 * row + ( ( col >> 4 ) << 2 );
            b = 0x8000 >> ( col & 0xf );
            c = 0;
            for ( plane = 0; plane < 4; ++plane )
                if ( b & screen[ind+plane] )
                    c |= (1 << plane);
            pixelrow[col] = pal[c];
        }
        ppm_writeppmrow( stdout, pixelrow, COLS, MAXVAL, 0 );
    }

    pm_close( stdout );

    exit( 0 );
}
