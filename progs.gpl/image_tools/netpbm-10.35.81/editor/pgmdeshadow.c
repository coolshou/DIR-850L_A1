/*============================================================================
                        pgmdeshadow
==============================================================================
   Read PGM containing scanned black/white text, deshadow, write PGM.
============================================================================*/
/*
    This code is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this code; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * Algorithm reference: Luc Vincent, "Morphological Grayscale Reruction
 * in Image Analysis: Applications and Efficient Algorithms," IEEE
 * Transactions on Image Processing, vol. 2, no. 2, April 1993, pp. 176-201.
 *
 * The algorithm used here is "fast hybrid grayscale reruction,"
 * described as follows on pp. 198-199:
 *
 * I: mask image (binary or grayscale)
 * J: marker image, defined on domain D_I, J <= I.
 *    Reruction is determined directly in J.
 *
 * Scan D_I in raster order:
 *   Let p be the current pixel;
 *   J(p) <- (max{J(q),q member_of N_G_plus(p) union {p}}) ^ I(p)
 *       [Note that ^ here refers to "pointwise minimum.]
 *
 * Scan D_I in antiraster order:
 *   Let p be the current pixel;
 *   J(p) <- (max{J(q),q member_of N_G_minus(p) union {p}}) ^ I(p)
 *       [Note that ^ here refers to "pointwise minimum.]
 *   If there exists q member_of N_G_minus(p) such that J(q) < J(p) and
 *       J(q) < I(q), then fifo_add(p)
 *
 * Propagation step:
 *   While fifo_empty() is false
 *   p <- fifo_first()
 *   For every pixel q member_of N_G(p):
 *     If J(q) < J(p) and I(q) ~= J(q), then
 *       J(q) <- min{J(p),I(q)}
 *       fifo_add(q)
 */

#include <stdio.h>

#include "pm_c_util.h"
#include "mallocvar.h"
#include "shhopt.h"
#include "pgm.h"


struct cmdlineInfo {
    const char * inputFileName;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry * option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */

    option_def[0].type = OPT_END;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 < 1)
        cmdlineP->inputFileName = "-";
    else {
        cmdlineP->inputFileName = argv[1];
        if (argc-1 > 1)
            pm_error ("Too many arguments.  The only argument is "
                      "the optional input file name");
    }
}



static void
initializeDeshadowMarker(gray **      const inputPixels,
                         gray **      const markerPixels,
                         unsigned int const cols,
                         unsigned int const rows,
                         gray         const maxval) {
/*----------------------------------------------------------------------------
  Fill the image with maxval and then copy 1-pixel-wide borders
-----------------------------------------------------------------------------*/
    { /* Make middle white */
        unsigned int row;
        
        for (row = 1; row < rows-1; ++row) {
            unsigned int col;
            for (col = 1; col < cols-1; ++col)
                markerPixels[row][col] = maxval;
        }
    }
    { /* Copy top edge */
        unsigned int col;
        for (col = 0; col < cols; ++col)
            markerPixels[0][col] = inputPixels[0][col];
    }
    { /* Copy bottom edge */
        unsigned int col;
        for (col = 0; col < cols; ++col)
            markerPixels[rows-1][col] = inputPixels[rows-1][col];
    }
    { /* Copy left edge */
        unsigned int row;
        for (row = 0; row < rows; ++row)
            markerPixels[row][0] = inputPixels[row][0];
    }
    { /* Copy right edge */
        unsigned int row;
        for (row = 0; row < rows; ++row)
            markerPixels[row][cols-1] = inputPixels[row][cols-1];
    }
}



static gray
min5(gray const a,
     gray const b,
     gray const c,
     gray const d,
     gray const e) {
    
    return MIN(a,MIN(b,MIN(c,MIN(d,e))));
}



static gray
minNortheastPixel(gray **      const pixels,
                  unsigned int const col,
                  unsigned int const row) {
/*----------------------------------------------------------------------------
  Return the minimum pixel value from among the immediate north-east
  neighbors of (col, row) in pixels[][].
-----------------------------------------------------------------------------*/
    return min5(pixels[row][col],
                pixels[row][col-1],
                pixels[row-1][col-1],
                pixels[row-1][col],
                pixels[row-1][col+1]);
}



static gray
minSouthwestPixel(gray **      const pixels,
                  unsigned int const col,
                  unsigned int const row) {
/*----------------------------------------------------------------------------
  Return the minimum pixel value from among the immediate south-west
  neighbors of (col, row) in pixels[][].
-----------------------------------------------------------------------------*/
    return min5(pixels[row][col],
                pixels[row][col+1],
                pixels[row+1][col-1],
                pixels[row+1][col],
                pixels[row+1][col+1]);
}



static void
estimateBackground(gray **      const inputPixels,
                   gray **      const markerPixels,
                   unsigned int const cols,
                   unsigned int const rows,
                   gray         const maxval) {
/*----------------------------------------------------------------------------
   Update markerPixels[].
-----------------------------------------------------------------------------*/
    unsigned int const passes = 2;
        /* make only two passes since the image is not really complicated
           (otherwise could go up to 10)
        */

    unsigned int pass;
    bool stable;

    for (pass = 0, stable = FALSE; pass < passes && !stable; ++pass) {
        int row;

        stable = TRUE;  /* initial assumption */

        /* scan in raster order */

        for (row = 1; row < rows; ++row) {
            unsigned int col;
            for (col = 1; col < cols-1; ++col) {
                gray const minpixel =
                    minNortheastPixel(markerPixels, col, row);

                if (minpixel > inputPixels[row][col]) {
                    markerPixels[row][col] = minpixel;
                    stable = FALSE;
                } else
                    markerPixels[row][col] = inputPixels[row][col];
            }       
        }
        /* scan in anti-raster order */
        
        for (row = rows-2; row >= 0; --row) {
            int col;
            for (col = cols-2; col > 0; --col) {
                gray const minpixel =
                    minSouthwestPixel(markerPixels, col, row);
                
                if (minpixel > inputPixels[row][col]) {
                    markerPixels[row][col] = minpixel;
                    stable = FALSE;
                } else
                    markerPixels[row][col] = inputPixels[row][col];
            }
        }
    }
}



static void
divide(gray **      const dividendPixels,
       gray **      const divisorPixels,
       unsigned int const cols,
       unsigned int const rows,
       gray         const maxval) {
/*----------------------------------------------------------------------------
   Divide each pixel of dividendPixels[][] by the corresponding pixel
   in divisorPixels[], replacing the dividendPixels[][] pixel with the
   quotient.

   But leave a one-pixel border around dividendPixels[][] unmodified.

   Make sure the results are reasonable and not larger than maxval.
-----------------------------------------------------------------------------*/
    unsigned int row;

    for (row = 1; row < rows-1; ++row) {
        unsigned int col;
        for (col = 1; col < cols-1; ++col) {
            gray const divisor  = divisorPixels[row][col];
            gray const dividend = dividendPixels[row][col];

            gray quotient;

            if (divisor == 0)
                quotient = maxval;
            else {
                if (25 * divisor < 3 * maxval && 25 * dividend < 3 * maxval)
                    quotient = maxval;
                else
                    quotient =
                        MIN(maxval,
                            maxval * (dividend + dividend/2) / divisor);
            }        
            dividendPixels[row][col] = quotient;
        }
    }
}



static void
deshadow(gray **      const inputPixels,
         unsigned int const cols,
         unsigned int const rows,
         gray         const maxval) {
/*----------------------------------------------------------------------------
   Deshadow the image described by inputPixels[], 'cols', 'rows', and
   'maxval'.  (Modify inputPixels[][]).
-----------------------------------------------------------------------------*/
    gray ** markerPixels;

    markerPixels = pgm_allocarray(cols, rows);

    initializeDeshadowMarker(inputPixels, markerPixels, cols, rows, maxval);
    
    estimateBackground(inputPixels, markerPixels, cols, rows, maxval);
    
    divide(inputPixels, markerPixels, cols, rows, maxval);

    pgm_freearray(markerPixels, rows);
}



int
main(int argc, char* argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    gray maxval;
    gray ** pixels;
    int cols, rows;

    pgm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFileName);
    
    pixels = pgm_readpgm(ifP, &cols, &rows, &maxval);
    pm_close(ifP);
    
    deshadow(pixels, cols, rows, maxval);
    
    pgm_writepgm(stdout, pixels, cols, rows, maxval, 0);

    pgm_freearray(pixels, rows);

    return 0;
}
