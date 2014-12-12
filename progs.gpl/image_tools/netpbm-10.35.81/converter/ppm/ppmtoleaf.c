/* ppmtoleaf.c - read a portable pixmap and produce a ileaf img file
 *
 * Copyright (C) 1994 by Bill O'Donnell.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 *
 * Known problems: pgms are not converted to leaf grayscales; they are
 * converted to 8-bit color images with all gray for colors.
 * 
 */

#include <stdio.h>
#include "ppm.h"

#define MAXCOLORS 256

pixel **pixels;
colorhash_table cht;

int Red[MAXCOLORS], Green[MAXCOLORS], Blue[MAXCOLORS];



static int
colorstobpp( colors )
int colors;
{
    int bpp;
    
    if ( colors <= 2 )
	bpp = 1;
    else if ( colors <= 256 )
	bpp = 8;
    else
	bpp = 24;
    return bpp;
}



static int
GetPixel( x, y )
int x, y;
{
    int color;
    
    color = ppm_lookupcolor( cht, &pixels[y][x] );
    return color;
}



/* OK, this routine is not wicked efficient, but it is simple to follow
   and it works. */
static void
leaf_writeimg(width, height, depth, ncolors, maxval)
int width;
int height;
int depth;
int ncolors;
{
    int i,row,col;
    
    /* NOTE: byte order in ileaf img file fmt is big-endian, always! */
    
    /* magic */
    fputc(0x89, stdout);
    fputc(0x4f, stdout);
    fputc(0x50, stdout);
    fputc(0x53, stdout);
    
    /* version 4 */
    fputc(0x00, stdout);
    fputc(0x04, stdout);
    
    /* h resolution: pixels/inch: say 75=screen resolution */
    fputc(0x00, stdout);
    fputc(75, stdout);
    
    /* v resolution: pixels/inch: say 75=screen resolution */
    fputc(0x00, stdout);
    fputc(75, stdout);
    
    /* unique id, could be anything */
    fputc(0x01, stdout);
    fputc(0x02, stdout);
    fputc(0x03, stdout);
    fputc(0x04, stdout);
    
    /* x offset, always zero */
    fputc(0x00, stdout);
    fputc(0x00, stdout);
    
    /* y offset, always zero */
    fputc(0x00, stdout);
    fputc(0x00, stdout);
    
    /* dimensions 64k x 64k max */
    fputc((unsigned char)((width >> 8) & 0x00ff), stdout);
    fputc((unsigned char)(width  & 0x00ff), stdout);
    fputc((unsigned char)((height >> 8) & 0x00ff), stdout);
    fputc((unsigned char)(height  & 0x00ff), stdout);
    
    /* depth */
    fputc(0x00, stdout);
    fputc((unsigned char)depth, stdout);
    
    /* compressed, 0=uncompressed, 1=compressed */
    fputc(0x00, stdout);
    
    /* format, mono/gray = 0x20000000, RGB=0x29000000 */
    if (depth == 1)
	fputc(0x20, stdout);
    else
	fputc(0x29, stdout);
    fputc(0x00, stdout);
    fputc(0x00, stdout);
    fputc(0x00, stdout);
    
    /* colormap size */
    if (depth == 8)
    {
	fputc((unsigned char)((ncolors >> 8) & 0x00ff), stdout);
	fputc((unsigned char)(ncolors  & 0x00ff), stdout);
	for (i=0; i<256; i++)
	    fputc((unsigned char) Red[i]*255/maxval, stdout);
	for (i=0; i<256; i++)
	    fputc((unsigned char) Green[i]*255/maxval, stdout);
	for (i=0; i<256; i++)
	    fputc((unsigned char) Blue[i]*255/maxval, stdout);
	
	for (row=0; row<height; row++) 
	{
	    for (col=0; col<width; col++) 
		fputc(GetPixel(col, row), stdout);
	    if (width % 2)
		fputc(0x00, stdout); /* pad to 2-bytes */
	}
    } else if (depth == 1) {
	/* mono image */
	/* no colormap */
	fputc(0x00, stdout);
	fputc(0x00, stdout);

	for (row=0; row<height; row++) 
	{
	    unsigned char bits = 0;
	    for (col=0; col<width; col++) {
		if (GetPixel(col,row))
		    bits |= (unsigned char) (0x0080 >> (col % 8));
		if (((col + 1) % 8) == 0)  {
		    fputc(bits, stdout);
		    bits = 0;
		}
	    }
	    if ((width % 8) != 0)
		fputc(bits, stdout);
	    if ((width % 16) && (width % 16) <= 8)
		fputc(0x00, stdout);  /* 16 bit pad */
	}
    } else {
	/* no colormap, direct or true color (24 bit) image */
	fputc(0x00, stdout);
	fputc(0x00, stdout);
	
	for (row=0; row<height; row++) 
	{
	    for (col=0; col<width; col++) 
		fputc(pixels[row][col].r * 255 / maxval, stdout);
	    if (width % 2)
		fputc(0x00, stdout); /* pad to 2-bytes */
	    for (col=0; col<width; col++) 
		fputc(pixels[row][col].g * 255 / maxval, stdout);
	    if (width % 2)
		fputc(0x00, stdout); /* pad to 2-bytes */
	    for (col=0; col<width; col++) 
		fputc(pixels[row][col].b * 255 / maxval, stdout);
	    if (width % 2)
		fputc(0x00, stdout); /* pad to 2-bytes */
	}
    }
}



int
main( argc, argv )
int argc;
char *argv[];
{
    FILE *ifd;
    int argn, rows, cols, colors, i, BitsPerPixel;
    pixval maxval;
    colorhist_vector chv;
    const char * const usage = "[ppmfile]";
    
    ppm_init(&argc, argv);
    
    argn = 1;
    
    if ( argn < argc )
    {
	ifd = pm_openr( argv[argn] );
	argn++;
    } else
	ifd = stdin;
    
    if ( argn != argc )
	pm_usage( usage );
    
    pixels = ppm_readppm( ifd, &cols, &rows, &maxval );
    
    pm_close( ifd );
    
    /* Figure out the colormap. */
    fprintf( stderr, "(Computing colormap..." );
    fflush( stderr );
    chv = ppm_computecolorhist( pixels, cols, rows, MAXCOLORS, &colors );
    if ( chv != (colorhist_vector) 0 )
    {
	fprintf( stderr, "  Done.  %d colors found.)\n", colors );
	
	for ( i = 0; i < colors; i++ )
	{
	    Red[i] = (int) PPM_GETR( chv[i].color );
	    Green[i] = (int) PPM_GETG( chv[i].color );
	    Blue[i] = (int) PPM_GETB( chv[i].color );
	}
	BitsPerPixel = colorstobpp( colors );
	
	/* And make a hash table for fast lookup. */
	cht = ppm_colorhisttocolorhash( chv, colors );
	ppm_freecolorhist( chv );
    } else {
	BitsPerPixel = 24;
	fprintf( stderr, "  Done.  24-bit true color %d color image.)\n", colors );
    }
    
    leaf_writeimg(cols, rows, BitsPerPixel, colors, maxval);
    
    return( 0 );
}



