/* ppmtosix.c - read a portable pixmap and produce a color sixel file
**
** Copyright (C) 1991 by Rick Vinci.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "ppm.h"

static void WriteHeader ARGS((void));
static void WriteColorMap 
  ARGS((colorhist_vector chv, int colors, pixval maxval));
static void WriteRawImage ARGS((colorhash_table cht, int rows, int cols));
static void WritePackedImage ARGS((colorhash_table cht, int rows, int cols));
static void WriteEnd ARGS((void));
#define MAXVAL 100
#define MAXCOLORS 256

#define DCS '\220'   /* Device Control String */
#define ST  '\234'   /* String Terminator */
#define CSI '\233'   /* Control String Introducer */
#define ESC '\033'   /* Escape character */

static pixel** pixels;   /* stored ppm pixmap input */
static colorhash_table cht;
int margin;



int
main( argc, argv )
    int argc;
    char* argv[];
{
    FILE* ifp;
    int argn, rows, cols, colors;
    int raw;
    pixval maxval;
    colorhist_vector chv;
    const char* const usage = "[-raw] [-margin] [ppmfile]";


    ppm_init( &argc, argv );

    argn = 1;
    raw = 0;
    margin = 0;

    /* Parse args. */
    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
	{
	if ( pm_keymatch( argv[argn], "-raw", 2 ) )
	    raw = 1;
	else if ( pm_keymatch( argv[argn], "-margin", 2 ) )
	    margin = 1;
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

    /* Read in the whole ppmfile. */
    pixels = ppm_readppm( ifp, &cols, &rows, &maxval );
    pm_close( ifp );

    /* Print a warning if we could lose accuracy when rescaling colors. */
    if ( maxval > MAXVAL )
	pm_message(
	    "maxval is not %d - automatically rescaling colors", MAXVAL );

    /* Figure out the colormap. */
    pm_message( "computing colormap..." );
    chv = ppm_computecolorhist( pixels, cols, rows, MAXCOLORS, &colors );
    if ( chv == (colorhist_vector) 0 )
	pm_error( "too many colors - try doing a 'pnmquant %d'", MAXCOLORS );
    pm_message( "%d colors found", colors );

    /* Make a hash table for fast color lookup. */
    cht = ppm_colorhisttocolorhash( chv, colors );

    WriteHeader();
    WriteColorMap( chv, colors, maxval );
    if ( raw == 1 )
	WriteRawImage( cht, rows, cols );
    else
	WritePackedImage( cht, rows, cols );
    WriteEnd();

    /* If the program failed, it previously aborted with nonzero completion
       code, via various function calls.
    */
    return 0;
}



static void
WriteHeader()
    {
    if ( margin == 1 )
	printf( "%c%d;%ds", CSI, 14, 72 );
    printf( "%c", DCS );  /* start with Device Control String */
    printf( "0;0;8q" );   /* Horizontal Grid Size at 1/90" and graphics On */
    printf( "\"1;1\n" );  /* set aspect ratio 1:1 */
    }



static void
WriteColorMap( colorhist_vector chv, int colors, pixval maxval )
    {
    register int colornum;
    pixel p;

    for ( colornum = 0; colornum < colors ; ++colornum )
	{
	p = chv[colornum].color;
	if ( maxval != MAXVAL )
	    PPM_DEPTH( p, p, maxval, MAXVAL );
	printf( "#%d;2;%d;%d;%d", colornum,
	    (int) PPM_GETR( p ), (int) PPM_GETG( p ), (int) PPM_GETB( p ) );
	}
    printf( "\n" );
    }



static void
WriteRawImage( cht, rows, cols )
    colorhash_table cht;
    int rows, cols;
    {
    int rownum, colnum, b;
    const char* sixel = "@ACGO_";
    register pixel* pP;

    for ( rownum = 0; rownum < rows; ++rownum )
	{
	b = rownum % 6;
	for ( colnum = 0, pP = pixels[rownum]; colnum < cols; ++colnum, ++pP )
	    printf( "#%d%c", ppm_lookupcolor(cht, pP), sixel[b] );
	printf( "$\n" );   /* Carriage Return */
	if ( b == 5 )
	    printf( "-\n" );   /* Line Feed (one sixel height) */
	}
    }



static void
WritePackedImage( cht, rows, cols )
    colorhash_table cht;
    int rows, cols;
    {
    int rownum, colnum, b, repeat, thiscolor, nextcolor;
    const char* sixel = "@ACGO_";
    register pixel* pP;

    for ( rownum = 0; rownum < rows; ++rownum )
	{
	b = rownum % 6;
	repeat = 1;
	for ( colnum = 0, pP = pixels[rownum]; colnum < cols; ++colnum, ++pP )
	    {
	    thiscolor = ppm_lookupcolor(cht, pP);
	    if ( colnum == cols -1 )   /* last pixel in row */
		if ( repeat == 1 )
		    printf( "#%d%c", thiscolor, sixel[b] );
		else
		    printf( "#%d!%d%c", thiscolor, repeat, sixel[b] );
	    else   /* not last pixel in row */
		{
		nextcolor =  ppm_lookupcolor(cht, pP+1);
		if ( thiscolor == nextcolor )
		    ++repeat;
		else
		    if ( repeat == 1 )
			printf( "#%d%c", thiscolor, sixel[b] );
		    else
		    {
		    printf( "#%d!%d%c", thiscolor, repeat, sixel[b] );
		    repeat = 1;
		    }
		}
	    }   /* end column loop */
	printf( "$\n" );   /* Carriage Return */
	if ( b == 5 )
	    printf( "-\n" );   /* Line Feed (one sixel height) */
	}
    }



static void
WriteEnd()
    {
    if ( margin == 1 )
	printf ( "%c%d;%ds", CSI, 1, 80 );
    printf( "%c\n", ST );
    }
