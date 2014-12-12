/* ppmdither.c - Ordered dithering of a color ppm file to a specified number
**               of primary shades.
**
** Copyright (C) 1991 by Christos Zoulas.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "ppm.h"
#include "mallocvar.h"

/* Besides having to have enough memory available, the limiting factor
   in the dithering matrix power is the size of the dithering value.
   We need 2*dith_power bits in an unsigned int.  We also reserve
   one bit to give headroom to do calculations with these numbers.
*/
#define MAX_DITH_POWER ((sizeof(unsigned int)*8 - 1) / 2)

typedef unsigned char ubyte;

static unsigned int dith_power;     /* base 2 log of dither matrix dimension */
static unsigned int dith_dim;      	/* dimension of the dither matrix	*/
static unsigned int dith_dm2;      	/* dith_dim squared				*/
static unsigned int **dith_mat; 	/* the dithering matrix			*/
static int debug;

/* COLOR():
 *	returns the index in the colormap for the
 *      r, g, b values specified.
 */
#define COLOR(r,g,b) (((r) * dith_ng + (g)) * dith_nb + (b))



static unsigned int
dither(pixval const p,
       pixval const maxval,
       unsigned int const d,
       unsigned int const ditheredMaxval) {
/*----------------------------------------------------------------------------
  Return the dithered intensity for a component of a pixel whose real 
  intensity for that component is 'p' based on a maxval of 'maxval'.
  The returned intensity is based on a maxval of ditheredMaxval.

  'd' is the entry in the dithering matrix for the position of this pixel
  within the dithered square.
-----------------------------------------------------------------------------*/
    unsigned int const ditherSquareMaxval = ditheredMaxval * dith_dm2;
        /* This is the maxval for an intensity that an entire dithered
           square can represent.
        */
    pixval const pScaled = ditherSquareMaxval * p / maxval;
        /* This is the input intensity P expressed with a maxval of
           'ditherSquareMaxval'
        */
    
    /* Now we scale the intensity back down to the 'ditheredMaxval', and
       as that will involve rounding, we round up or down based on the position
       in the dithered square, as determined by 'd'
    */

    return (pScaled + d) / dith_dm2;
}


/* 
 *	Return the value of a dither matrix which is 2**dith_power elements
 *  square at Row x, Column y.
 *	[graphics gems, p. 714]
 */
static unsigned int
dith_value(unsigned int y, unsigned int x, const unsigned int dith_power) { 

    unsigned int d;

    /*
     * Think of d as the density. At every iteration, d is shifted
     * left one and a new bit is put in the low bit based on x and y.
     * If x is odd and y is even, or visa versa, then a bit is shifted in.
     * This generates the checkerboard pattern seen in dithering.
     * This quantity is shifted again and the low bit of y is added in.
     * This whole thing interleaves a checkerboard pattern and y's bits
     * which is what you want.
     */
    int i;
    for (i = 0, d = 0; i < dith_power; i++, x >>= 1, y >>= 1)
        d = (d << 2) | (((x & 1) ^ (y & 1)) << 1) | (y & 1);
    return(d);
} /* end dith_value */



static unsigned int **
dith_matrix(unsigned int const dith_dim) {
/*----------------------------------------------------------------------------
   Create the dithering matrix for dimension 'dith_dim'.

   Return it in newly malloc'ed storage.

   Note that we assume 'dith_dim' is small enough that the dith_mat_sz
   computed within fits in an int.  Otherwise, results are undefined.
-----------------------------------------------------------------------------*/
    unsigned int ** dith_mat;
    {
        unsigned int const dith_mat_sz = 
            (dith_dim * sizeof(int *)) + /* pointers */
            (dith_dim * dith_dim * sizeof(int)); /* data */

        dith_mat = (unsigned int **) malloc(dith_mat_sz);

        if (dith_mat == NULL) 
            pm_error("Out of memory.  "
                     "Cannot allocate %d bytes for dithering matrix.",
                     dith_mat_sz);
    }
    {
        unsigned int * const dat = (unsigned int *) &dith_mat[dith_dim];
        unsigned int y;
        for (y = 0; y < dith_dim; y++)
            dith_mat[y] = &dat[y * dith_dim];
    }
    {
        unsigned int y;
        for (y = 0; y < dith_dim; y++) {
            unsigned int x;
            for (x = 0; x < dith_dim; x++) {
                dith_mat[y][x] = dith_value(y, x, dith_power);
                if (debug)
                    (void) fprintf(stderr, "%4d ", dith_mat[y][x]);
            }
            if (debug)
                (void) fprintf(stderr, "\n");
        }
    }
    return dith_mat;
}

    

static void
dith_setup(const unsigned int dith_power, 
           const unsigned int dith_nr, 
           const unsigned int dith_ng, 
           const unsigned int dith_nb, 
           const pixval output_maxval,
           pixel ** const colormapP) {
/*----------------------------------------------------------------------------
   Set up the dithering parameters, color map (lookup table) and
   dithering matrix.

   Return the colormap in newly malloc'ed storage and return its address
   as *colormapP.
-----------------------------------------------------------------------------*/
    unsigned int r, g, b;

    if (dith_nr < 2) 
        pm_error("too few shades for red, minimum of 2");
    if (dith_ng < 2) 
        pm_error("too few shades for green, minimum of 2");
    if (dith_nb < 2) 
        pm_error("too few shades for blue, minimum of 2");

    MALLOCARRAY(*colormapP, dith_nr * dith_ng * dith_nb);
    if (*colormapP == NULL) 
        pm_error("Unable to allocate space for the color lookup table "
                 "(%d by %d by %d pixels).", dith_nr, dith_ng, dith_nb);
    
    for (r = 0; r < dith_nr; r++) 
        for (g = 0; g < dith_ng; g++) 
            for (b = 0; b < dith_nb; b++) {
                PPM_ASSIGN((*colormapP)[COLOR(r,g,b)], 
                           (r * output_maxval / (dith_nr - 1)),
                           (g * output_maxval / (dith_ng - 1)),
                           (b * output_maxval / (dith_nb - 1)));
            }
    
    if (dith_power > MAX_DITH_POWER) {
        pm_error("Dithering matrix power %d is too large.  Must be <= %d",
                 dith_power, MAX_DITH_POWER);
    } else {
        dith_dim = (1 << dith_power);
        dith_dm2 = dith_dim * dith_dim;
    }

    dith_mat = dith_matrix(dith_dim);
} /* end dith_setup */


/* 
 *  Dither whole image
 */
static void
dith_dither(const unsigned int width, const unsigned int height, 
            const pixval maxval,
            const pixel * const colormap, 
            pixel ** const input, pixel ** const output,
            const unsigned int dith_nr,
            const unsigned int dith_ng,
            const unsigned int dith_nb, 
            const pixval output_maxval
            ) {

    const unsigned int dm = (dith_dim - 1);  /* A mask */
    unsigned int row, col; 

    for (row = 0; row < height; row++)
        for (col = 0; col < width; col++) {
            unsigned int const d = dith_mat[row & dm][(width-col-1) & dm];
            pixel const input_pixel = input[row][col];
            unsigned int const dithered_r = 
                dither(PPM_GETR(input_pixel), maxval, d, dith_nr-1);
            unsigned int const dithered_g = 
                dither(PPM_GETG(input_pixel), maxval, d, dith_ng-1);
            unsigned int const dithered_b = 
                dither(PPM_GETB(input_pixel), maxval, d, dith_nb-1);
            output[row][col] = 
                colormap[COLOR(dithered_r, dithered_g, dithered_b)];
        }
}


int
main( argc, argv )
    int argc;
    char* argv[];
    {
    FILE* ifp;
    pixel *colormap;    /* malloc'd */
    pixel **ipixels;        /* Input image */
    pixel **opixels;        /* Output image */
    int cols, rows;
    pixval maxval;  /* Maxval of the input image */
    pixval output_maxval;  /* Maxval in the dithered output image */
    unsigned int argn;
    const char* const usage = 
	"[-dim <num>] [-red <num>] [-green <num>] [-blue <num>] [ppmfile]";
    unsigned int dith_nr; /* number of red shades in output */
    unsigned int dith_ng; /* number of green shades	in output */
    unsigned int dith_nb; /* number of blue shades in output */


    ppm_init( &argc, argv );

    dith_nr = 5;  /* default */
    dith_ng = 9;  /* default */
    dith_nb = 5;  /* default */

    dith_power = 4;  /* default */
    debug = 0; /* default */
    argn = 1;

    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
	{
	if ( pm_keymatch( argv[argn], "-dim", 1) &&  argn + 1 < argc ) {
	    argn++;
	    if (sscanf(argv[argn], "%u", &dith_power) != 1)
		pm_usage( usage );
	}
	else if ( pm_keymatch( argv[argn], "-red", 1 ) && argn + 1 < argc ) {
	    argn++;
	    if (sscanf(argv[argn], "%u", &dith_nr) != 1)
		pm_usage( usage );
	}
	else if ( pm_keymatch( argv[argn], "-green", 1 ) && argn + 1 < argc ) {
	    argn++;
	    if (sscanf(argv[argn], "%u", &dith_ng) != 1)
		pm_usage( usage );
	}
	else if ( pm_keymatch( argv[argn], "-blue", 1 ) && argn + 1 < argc ) {
	    argn++;
	    if (sscanf(argv[argn], "%u", &dith_nb) != 1)
		pm_usage( usage );
	}
	else if ( pm_keymatch( argv[argn], "-debug", 6 )) {
        debug = 1;
	}
	else
	    pm_usage( usage );
	++argn;
	}

    if ( argn != argc )
	{
	ifp = pm_openr( argv[argn] );
	++argn;
	}
    else
	ifp = stdin;

    if ( argn != argc )
	pm_usage( usage );

    ipixels = ppm_readppm( ifp, &cols, &rows, &maxval );
    pm_close( ifp );
    opixels = ppm_allocarray(cols, rows);
    output_maxval = pm_lcm(dith_nr-1, dith_ng-1, dith_nb-1, PPM_MAXMAXVAL);
    dith_setup(dith_power, dith_nr, dith_ng, dith_nb, output_maxval, 
               &colormap);
    dith_dither(cols, rows, maxval, colormap, ipixels, opixels,
                dith_nr, dith_ng, dith_nb, output_maxval);
    ppm_writeppm(stdout, opixels, cols, rows, output_maxval, 0);
    pm_close(stdout);
    exit(0);
}
