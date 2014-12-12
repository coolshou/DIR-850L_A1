/* ppmtoneo.c - read a portable pixmap and write a Neochrome NEO file
**
** Copyright (C) 2001 by Teemu Hukkanen <tjhukkan@iki.fi>
**  based on ppmtopi1 by Steve Belczyk and Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "ppm.h"

#define COLS 320
#define ROWS 200
#define MAXVAL 7
#define MAXCOLORS 16

static short screen[ROWS*COLS/4];  /* simulate the ST's video RAM */

int
main(int argc, char *argv[] ) {

    FILE* ifp;
    pixel** pixels;
    colorhist_vector chv;
    colorhash_table cht;
    int rows, cols;
    int colors;
    int i;
    int row;
    pixval maxval;

    ppm_init( &argc, argv );

    if ( argc > 2 )
        pm_usage( "[ppmfile]" );

    if ( argc == 2 )
        ifp = pm_openr( argv[1] );
    else
        ifp = stdin;

    pixels = ppm_readppm( ifp, &cols, &rows, &maxval );
    pm_close( ifp );
    if ( (cols > COLS) || (rows > ROWS) )
        pm_error( "image is larger than %dx%d - sorry", COLS, ROWS );

    pm_message( "computing colormap..." );
    chv = ppm_computecolorhist( pixels, cols, rows, MAXCOLORS, &colors );
    if ( chv == (colorhist_vector) 0 ) {
        pm_error(
            "too many colors - try doing a 'pnmquant %d'", MAXCOLORS );
    }
    pm_message( "%d colors found", colors );

    /* Write NEO header */
    /* Flag */
    pm_writebigshort( stdout, (short) 0);

    /* resolution */
    pm_writebigshort( stdout, (short) 0 );       /* low resolution */

    /* palette */
    for ( i = 0; i < 16; ++i ) {
        short w;

        if ( i < colors ) {
            pixel p;

            p = chv[i].color;
            if ( maxval != MAXVAL )
                PPM_DEPTH( p, p, maxval, MAXVAL );
            w  = ( (int) PPM_GETR( p ) ) << 8;
            w |= ( (int) PPM_GETG( p ) ) << 4;
            w |= ( (int) PPM_GETB( p ) );
        } else
            w = 0;
        pm_writebigshort( stdout, w );
    }
    if ( maxval > MAXVAL )
        pm_message(
            "maxval is not %d - automatically rescaling colors", MAXVAL );

    /* Convert color vector to color hash table, for fast lookup. */
    cht = ppm_colorhisttocolorhash( chv, colors );
    ppm_freecolorhist( chv );

    /* Skip rest of header */
    fseek(stdout, 128, SEEK_SET);

    /* Clear the screen buffer. */
    for ( i = 0; i < ROWS*COLS/4; ++i )
        screen[i] = 0;

    /* Convert. */
    for ( row = 0; row < rows; ++row ) {
        int col;
        for ( col = 0; col < cols; ++col) {
            int color, ind, b, plane;
            pixel const p = pixels[row][col];

            color = ppm_lookupcolor( cht, &p );
            if ( color == -1 )
                pm_error(
                    "color not found?!?  row=%d col=%d  r=%d g=%d b=%d",
                    row, col, PPM_GETR(p), PPM_GETG(p), PPM_GETB(p) );
            ind = 80 * row + ( ( col >> 4 ) << 2 );
            b = 0x8000 >> (col & 0xf);
            for ( plane = 0; plane < 4; ++plane )
                if ( color & (1 << plane) )
                    screen[ind+plane] |= b;
            }
        }

    /* And write out the screen buffer. */
    for ( i = 0; i < ROWS*COLS/4; ++i )
        (void) pm_writebigshort( stdout, screen[i] );

    exit( 0 );
}
