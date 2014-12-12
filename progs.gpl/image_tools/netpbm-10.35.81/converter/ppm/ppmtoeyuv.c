/* Bryan got this from mm.ftp-cs.berkeley.edu from the package
   mpeg-encode-1.5b-src under the name ppmtoeyuv.c on March 30, 2000.  
   The file was dated January 19, 1995.  

   Bryan changed the program to take an argument as the input filename
   and fixed a crash when the input image has an odd number of rows or 
   columns.

   Then Bryan updated the program on March 15, 2001 to use the Netpbm
   libraries to read the PPM input and handle multi-image PPM files
   and arbitrary maxvals.

   There was no attached documentation except for this:  Encoder/Berkeley
   YUV format is merely the concatenation of Y, U, and V data in order.
   Compare with Abekas YUV, which interlaces Y, U, and V data. 

   Future enhancement: It may be useful to have an option to do the
   calculations without multiplication tables to save memory at the
   expense of execution speed for large maxvals.  Actually, a large
   maxval without a lot of colors might actually make the tables
   slower.

*/

/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*  
 *  $Header: /n/picasso/users/keving/encode/src/RCS/readframe.c,v 1.1 1993/07/22 22:23:43 keving Exp keving $
 *  $Log: readframe.c,v $
 * Revision 1.1  1993/07/22  22:23:43  keving
 * nothing
 *
 */


/*==============*
 * HEADER FILES *
 *==============*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "pm_c_util.h"
#include "ppm.h"
#include "mallocvar.h"

typedef	unsigned char uint8;

/* Multiplication tables */

#define YUVMAXVAL 255
#define HALFYUVMAXVAL 128
/* multXXX are multiplication tables used in RGB-YCC calculations for 
   speed.  mult299[x] is x * .299, scaled to a maxval of 255.  These
   are malloc'ed and essentially constant.

   We use these tables because it is much faster to do a
   multiplication once for each possible sample value than once for
   each pixel in the image.  
*/
static float *mult299, *mult587, *mult114;
static float *mult16874, *mult33126, *mult5;
static float *mult41869, *mult08131;

static __inline__ float
luminance(const pixel p) {
    return mult299[PPM_GETR(p)]
        + mult587[PPM_GETG(p)] 
        + mult114[PPM_GETB(p)]
        ;
}

static __inline__ float
chrominance_red(const pixel p) {
    return mult5[PPM_GETR(p)] 
        + mult41869[PPM_GETG(p)]
        + mult08131[PPM_GETB(p)]
        ;
}

static __inline__ float
chrominance_blue(const pixel p) {
    return mult16874[PPM_GETR(p)] 
        + mult33126[PPM_GETG(p)]
        + mult5[PPM_GETB(p)]
        ;
}



static void
create_multiplication_tables(const pixval maxval) {

    int index;

    MALLOCARRAY_NOFAIL(mult299   , maxval+1);
    MALLOCARRAY_NOFAIL(mult587   , maxval+1);
    MALLOCARRAY_NOFAIL(mult114   , maxval+1);
    MALLOCARRAY_NOFAIL(mult16874 , maxval+1);
    MALLOCARRAY_NOFAIL(mult33126 , maxval+1);
    MALLOCARRAY_NOFAIL(mult5     , maxval+1);
    MALLOCARRAY_NOFAIL(mult41869 , maxval+1);
    MALLOCARRAY_NOFAIL(mult08131 , maxval+1);

    if (maxval == YUVMAXVAL) {
        /* fast path */
        for ( index = 0; index <= maxval; index++ ) {
            mult299[index]   =  0.29900*index;
            mult587[index]   =  0.58700*index;
            mult114[index]   =  0.11400*index;
            mult5[index]     =  0.50000*index;
            mult41869[index] = -0.41869*index;
            mult08131[index] = -0.08131*index;
            mult16874[index] = -0.16874*index;
            mult33126[index] = -0.33126*index;
        }
    } else {
        for ( index = 0; index <= maxval; index++ ) {
            mult299[index]   =  0.29900*index*(maxval/YUVMAXVAL);
            mult587[index]   =  0.58700*index*(maxval/YUVMAXVAL);
            mult114[index]   =  0.11400*index*(maxval/YUVMAXVAL);
            mult5[index]     =  0.50000*index*(maxval/YUVMAXVAL);
            mult41869[index] = -0.41869*index*(maxval/YUVMAXVAL);
            mult08131[index] = -0.08131*index*(maxval/YUVMAXVAL);
            mult16874[index] = -0.16874*index*(maxval/YUVMAXVAL);
            mult33126[index] = -0.33126*index*(maxval/YUVMAXVAL);
        }
        
    }
}



static void
free_multiplication_tables(void) {
    free(mult299   );
    free(mult587   );
    free(mult114   );
    free(mult16874 );
    free(mult33126 );
    free(mult5     );
    free(mult41869 );
    free(mult08131 );
}



/*===========================================================================*
 *
 * PPMtoYUV
 *
 *	convert PPM data into YUV data
 *	assumes that ydivisor = 1
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    none
 *
 * This function processes the input file in 4 pixel squares.  If the
 * Image does not have an even number of rows and columns, the rightmost
 * column or the bottom row gets ignored and output has one fewer row
 * or column than the input. 
 *
 *===========================================================================*/
static void 
PPMtoYUV(pixel ** const ppm_image, const int width, const int height,
         uint8 *** const orig_yP, 
         uint8 *** const orig_crP, 
         uint8 *** const orig_cbP) {

    int y;
    uint8 ** orig_y;
    uint8 ** orig_cr;
    uint8 ** orig_cb;

    orig_y = *orig_yP;
    orig_cr = *orig_crP;
    orig_cb = *orig_cbP;

    for (y = 0; y + 1 < height; y += 2) {
        uint8 *dy0, *dy1;
        uint8 *dcr, *dcb;
        const pixel *src0, *src1;
          /* Pair of contiguous rows of the ppm input image we are
             converting */
        int x;

        src0 = ppm_image[y];
        src1 = ppm_image[y + 1];

        dy0 = orig_y[y];
        dy1 = orig_y[y + 1];
        dcr = orig_cr[y / 2];
        dcb = orig_cb[y / 2];

        for ( x = 0; x + 1 < width; x += 2) {
            dy0[x] = luminance(src0[x]);
            dy1[x] = luminance(src1[x]);

            dy0[x+1] = luminance(src0[x+1]);
            dy1[x+1] = luminance(src1[x+1]);

            dcr[x/2] = ((
                chrominance_red(src0[x]) +
                chrominance_red(src1[x]) +
                chrominance_red(src0[x+1]) +
                chrominance_red(src1[x+1]) 
                ) / 4) + HALFYUVMAXVAL;

            dcb[x/2] = ((
                chrominance_blue(src0[x]) +
                chrominance_blue(src1[x]) +
                chrominance_blue(src0[x+1]) +
                chrominance_blue(src1[x+1]) 
                ) / 4) + HALFYUVMAXVAL;
        }
    }
}



static void 
WriteYUV(FILE *fpointer, const int width, const int height,
         uint8 ** const orig_y, uint8 ** const orig_cr, uint8 ** const orig_cb)
{
    register int y;

    for (y = 0; y < height; y++)                        /* Y */
        fwrite(orig_y[y], 1, width, fpointer);

    for (y = 0; y < height / 2; y++)                    /* U */
        fwrite(orig_cb[y], 1, width / 2, fpointer);

    for (y = 0; y < height / 2; y++)                    /* V */
        fwrite(orig_cr[y], 1, width / 2, fpointer);
}



static void
AllocYUV(int       const width, 
         int       const height,
         uint8 *** const orig_yP, 
         uint8 *** const orig_crP,
         uint8 *** const orig_cbP) {

    int y;
    uint8 ** orig_y;
    uint8 ** orig_cr;
    uint8 ** orig_cb;

    MALLOCARRAY_NOFAIL(*orig_yP, height);
    orig_y = *orig_yP;
    for (y = 0; y < height; y++) 
        MALLOCARRAY_NOFAIL(orig_y[y], width);

    MALLOCARRAY_NOFAIL(*orig_crP, height / 2);
    orig_cr = *orig_crP;
    for (y = 0; y < height / 2; y++) 
        MALLOCARRAY_NOFAIL(orig_cr[y], width / 2);

    MALLOCARRAY_NOFAIL(*orig_cbP, height / 2);
    orig_cb = *orig_cbP;
    for (y = 0; y < height / 2; y++) 
        MALLOCARRAY_NOFAIL(orig_cb[y], width / 2);
}



static void
FreeYUV(const int width, const int height,
        uint8 ** const orig_y, uint8 ** const orig_cr, uint8 ** const orig_cb){

    int y;

    if (orig_y) {
       for (y = 0; y < height; y++)
           free(orig_y[y]);
       free(orig_y);
    }

    if (orig_cr) {
       for (y = 0; y < height / 2; y++)
           free(orig_cr[y]);
       free(orig_cr);
    }

    if (orig_cb) {
       for (y = 0; y < height / 2; y++)
           free(orig_cb[y]);
       free(orig_cb);
    }
}



int
main(int argc, char **argv) {
    const char *input_filename;  /* NULL for stdin */
    FILE * ifp;
    int width, height;
    pixval maxval;
    pixel **ppm_image;   /* malloc'ed */
    uint8 **orig_y, **orig_cr, **orig_cb;
        /* orig_y is the height x width array of individual pixel luminances 
           orig_cr and orig_cb are the height/2 x width/2 arrays of average
           red and blue chrominance values over each 4 pixel square.
        */
    int eof;

    /* The following are width, height, and maxval of the image we 
       processed before this one.  Zero if there was no image before 
       this one.
    */
    int last_width, last_height;
    pixval last_maxval;

    ppm_init(&argc, argv);

    if (argc > 2) {
        pm_error("Program takes either one argument -- "
                "the input filename -- or no arguments (input is stdin)");
        exit(1);
    } else if (argc == 2)
        input_filename = argv[1];
    else input_filename = NULL;

    if (input_filename == NULL) ifp = stdin;
    else ifp = pm_openr(input_filename);

    eof = FALSE;
    last_maxval = 0;  /* No previous maxval */
    last_width = 0;	/* No previous width */
    last_height = 0;	/* No previous height */
    orig_y = orig_cr = orig_cb = 0;

    while (!eof) {
        ppm_image = ppm_readppm(ifp, &width, &height, &maxval);

        if (width % 2 != 0) 
            pm_message("Input image has odd number of columns.  The rightmost "
                       "column will be omitted from the output.");
        if (height % 2 != 0) 
            pm_message("Input image has odd number of rows.  The bottom "
                       "row will be omitted from the output.");

        if (maxval != last_maxval) {
            /* We're going to need all new multiplication tables. */
            free_multiplication_tables();
            create_multiplication_tables(maxval);
        }
        last_maxval = maxval;
        
        if (height != last_height || width != last_width) {
            FreeYUV(width, height, orig_y, orig_cr, orig_cb);
            /* Need new YUV buffers for different size */
            AllocYUV(width, height, &orig_y, &orig_cr, &orig_cb);
        }
        last_height = height;
        last_width = width;

        PPMtoYUV(ppm_image, width, height, &orig_y, &orig_cr, &orig_cb);

        WriteYUV(stdout, (width/2)*2, (height/2)*2, orig_y, orig_cr, orig_cb);

        ppm_freearray(ppm_image, height);
        ppm_nextimage(ifp, &eof);
    }
    FreeYUV(width, height, orig_y, orig_cr, orig_cb);
    free_multiplication_tables();
    pm_close(ifp);
        
    return 0;
}

