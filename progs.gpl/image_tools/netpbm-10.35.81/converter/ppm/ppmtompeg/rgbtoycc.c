/*===========================================================================*
 * rgbtoycc.c                                    *
 *                                       *
 *  Procedures to convert from RGB space to YUV space            *
 *                                       *
 * EXPORTED PROCEDURES:                              *
 *  PNMtoYUV                                 *
 *  PPMtoYUV                                 *
 *                                       *
 *===========================================================================*/

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

/*==============*
 * HEADER FILES *
 *==============*/

#include "pnm.h"
#include "all.h"
#include "frame.h"
#include "rgbtoycc.h"


static float *mult299, *mult587, *mult114, *mult16874, *mult33126,
    *mult5, *mult41869, *mult08131;  /* malloc'ed */
    /* These are tables we use for fast arithmetic */
static pixval table_maxval = 0;
    /* The maxval used to compute the above arrays.  Zero means
       the above arrays don't exist yet
    */

static void
compute_mult_tables(const pixval maxval) {

    /* For speed, we do the arithmetic with eight tables that reduce a
       bunch of multiplications and divisions to a simple table lookup.
       
       Because a large maxval could require a significant amount of
       table space, we allocate the space dynamically.

       If we had to compute the tables for every frame, it wouldn't be 
       fast at all, but since all the frames normally have the same
       maxval, we only need to compute them once.  But just in case,
       we check each frame to see if it has a different maxval and
       recompute the tables if so.
    */
    
    if (table_maxval != maxval) {
        /* We need to compute or re-compute the multiplication tables */
        if (table_maxval != 0) {
            free(mult299); free(mult587); free(mult114); free(mult16874);
            free(mult33126); free(mult5); free(mult41869); free(mult08131);  
        } 
        table_maxval = maxval;

        mult299   = malloc((table_maxval+1)*sizeof(float));
        mult587   = malloc((table_maxval+1)*sizeof(float));
        mult114   = malloc((table_maxval+1)*sizeof(float));
        mult16874 = malloc((table_maxval+1)*sizeof(float));
        mult33126 = malloc((table_maxval+1)*sizeof(float));
        mult5     = malloc((table_maxval+1)*sizeof(float));
        mult41869 = malloc((table_maxval+1)*sizeof(float));
        mult08131 = malloc((table_maxval+1)*sizeof(float));

        if (mult299 == NULL || mult587 == NULL || mult114 == NULL ||
            mult16874 == NULL || mult33126 == NULL || mult5 == NULL ||
            mult41869 == NULL || mult08131 == NULL) 
            pm_error("Unable to allocate storage for arithmetic tables.\n"
                     "We need %d bytes, which is the maxval of the input "
                     "image, plus 1,\n"
                     "times the storage size of a floating point value.", 
                     8 * (table_maxval+1)*sizeof(float));

        {
            int index;

            for (index = 0; index <= table_maxval; index++ ) {
                mult299[index]   = index*0.29900  * 255 / table_maxval; 
                mult587[index]   = index*0.58700  * 255 / table_maxval;
                mult114[index]   = index*0.11400  * 255 / table_maxval;
                mult16874[index] = -0.16874*index * 255 / table_maxval;
                mult33126[index] = -0.33126*index * 255 / table_maxval;
                mult5[index]     = index*0.50000  * 255 / table_maxval;
                mult41869[index] = -0.41869*index * 255 / table_maxval;
                mult08131[index] = -0.08131*index * 255 / table_maxval;
            }
        }
    }
}


/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/


void
PNMtoYUV(MpegFrame *  const frameP,
         xel **       const xels,
         unsigned int const cols,
         unsigned int const rows,
         xelval       const maxval) {
/*----------------------------------------------------------------------------
   Set the raster of the MPEG frame *frameP from the libnetpbm input
   'xels'.  Note that the raster information in a MpegFrame is in YUV
   form.
-----------------------------------------------------------------------------*/
    int x, y;
    uint8 *dy0, *dy1;
    uint8 *dcr, *dcb;
    pixel *src0, *src1;

    compute_mult_tables(maxval);  /* This sets up mult299[], etc. */

    Frame_AllocYCC(frameP);

    /*
     * okay.  Now, convert everything into YCbCr space. (the specific
     * numbers come from the JPEG source, jccolor.c) The conversion
     * equations to be implemented are therefore
     *
     * Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
     * Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B
     * Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B
     *
     * With Y, Cb, and Cr then normalized to the range 0 - 255.
     */

    for (y = 0; y < rows; y += 2) {
        for (x = 0, src0 = xels[y], src1 = xels[y + 1],
                 dy0 = frameP->orig_y[y], dy1 = frameP->orig_y[y + 1],
                 dcr = frameP->orig_cr[y >> 1], dcb = frameP->orig_cb[y >> 1];
             x < cols;
             x += 2, dy0 += 2, dy1 += 2, dcr++,
                 dcb++, src0 += 2, src1 += 2) {

            *dy0 = (mult299[PPM_GETR(*src0)] +
                    mult587[PPM_GETG(*src0)] +
                    mult114[PPM_GETB(*src0)]);

            *dy1 = (mult299[PPM_GETR(*src1)] +
                    mult587[PPM_GETG(*src1)] +
                    mult114[PPM_GETB(*src1)]);

            dy0[1] = (mult299[PPM_GETR(src0[1])] +
                      mult587[PPM_GETG(src0[1])] +
                      mult114[PPM_GETB(src0[1])]);

            dy1[1] = (mult299[PPM_GETR(src1[1])] +
                      mult587[PPM_GETG(src1[1])] +
                      mult114[PPM_GETB(src1[1])]);

            *dcb = ((mult16874[PPM_GETR(*src0)] +
                     mult33126[PPM_GETG(*src0)] +
                     mult5[PPM_GETB(*src0)] +
                     mult16874[PPM_GETR(*src1)] +
                     mult33126[PPM_GETG(*src1)] +
                     mult5[PPM_GETB(*src1)] +
                     mult16874[PPM_GETR(src0[1])] +
                     mult33126[PPM_GETG(src0[1])] +
                     mult5[PPM_GETB(src0[1])] +
                     mult16874[PPM_GETR(src1[1])] +
                     mult33126[PPM_GETG(src1[1])] +
                     mult5[PPM_GETB(src1[1])]) / 4) + 128;

            *dcr = ((mult5[PPM_GETR(*src0)] +
                     mult41869[PPM_GETG(*src0)] +
                     mult08131[PPM_GETB(*src0)] +
                     mult5[PPM_GETR(*src1)] +
                     mult41869[PPM_GETG(*src1)] +
                     mult08131[PPM_GETB(*src1)] +
                     mult5[PPM_GETR(src0[1])] +
                     mult41869[PPM_GETG(src0[1])] +
                     mult08131[PPM_GETB(src0[1])] +
                     mult5[PPM_GETR(src1[1])] +
                     mult41869[PPM_GETG(src1[1])] +
                     mult08131[PPM_GETB(src1[1])]) / 4) + 128;

            DBG_PRINT(("%3d,%3d: (%3d,%3d,%3d) --> (%3d,%3d,%3d)\n", 
                       x, y, 
                       PPM_GETR(*src0), PPM_GETG(*src0), PPM_GETB(*src0), 
                       *dy0, *dcb, *dcr));
        }
    }
}
