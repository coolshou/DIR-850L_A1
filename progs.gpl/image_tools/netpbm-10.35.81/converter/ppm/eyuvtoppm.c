/* Bryan got this from mm.ftp-cs.berkeley.edu from the package
   mpeg-encode-1.5b-src under the name eyuvtoppm.c on March 30, 2000.  
   The file was dated April 14, 1995.  

   Bryan rewrote the program entirely to match Netpbm coding style,
   use the Netpbm libraries and also to output to stdout and ignore
   any specification of an output file on the command line and not
   segfault when called with no arguments.

   There was no attached documentation except for this:  Encoder/Berkeley
   YUV format is merely the concatenation of Y, U, and V data in order.
   Compare with Abekda YUV, which interlaces Y, U, and V data.  */

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
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ppm.h"
#include "shhopt.h"
#include "mallocvar.h"

typedef unsigned char uint8;

#define CHOP(x)     ((x < 0) ? 0 : ((x > 255) ? 255 : x))



struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespecs of input file */
    unsigned int width;
    unsigned int height;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdline_info *cmdlineP) {

    optStruct3 opt;   /* Set by OPTENT3 */
    unsigned int option_def_index;
    optEntry *option_def = malloc(100*sizeof(optEntry));

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3('w', "width",     OPT_UINT,  &cmdlineP->width,   NULL,         0);
    OPTENT3('h', "height",    OPT_UINT,  &cmdlineP->height,  NULL,         0);
    
    /* DEFAULTS */
    cmdlineP->width = 352;
    cmdlineP->height = 240;

    opt.opt_table = option_def;
    opt.short_allowed = TRUE;
    opt.allowNegNum = FALSE;

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

    if (cmdlineP->width == 0)
        pm_error("The width cannot be zero.");
    if (cmdlineP->width % 2 != 0)
        pm_error("The width of an eyuv image must be an even number.  "
                 "You specified %u.", cmdlineP->width);
    if (cmdlineP->height == 0)
        pm_error("The height cannot be zero.");
    if (cmdlineP->height % 2 != 0)
        pm_error("The height of an eyuv image must be an even number.  "
                 "You specified %u.", cmdlineP->height);


    if (argc-1 == 0) 
        cmdlineP->input_filespec = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->input_filespec = argv[1];

}



static uint8 ** 
AllocUint8Array(unsigned int const cols, unsigned int const rows) {

    uint8 **retval;
    unsigned int y;

    MALLOCARRAY(retval, rows);
    if (retval == NULL)
        pm_error("Unable to allocate storage for %d x %d byte array.",
                 cols, rows);

    for (y = 0; y < rows; y++) {
        MALLOCARRAY(retval[y], cols);
        if (retval[y] == NULL)
            pm_error("Unable to allocate storage for %d x %d byte array.",
                     cols, rows);
    }
    return retval;
}



static int ** 
AllocIntArray(unsigned int const cols, unsigned int const rows) {

    int **retval;
    unsigned int y;

    MALLOCARRAY(retval, rows);
    if (retval == NULL)
        pm_error("Unable to allocate storage for %d x %d byte array.",
                 cols, rows);

    for (y = 0; y < rows; y++) {
        MALLOCARRAY(retval[y], cols);
        if (retval[y] == NULL)
            pm_error("Unable to allocate storage for %d x %d byte array.",
                     cols, rows);
    }
    return retval;
}



static void
allocateStorage(int const cols, int const rows,
                int *** const YP, int *** const UP, int *** const VP,
                pixel *** const pixelsP, 
                uint8 *** const orig_yP, uint8 *** const orig_cbP,
                uint8 *** const orig_crP) {

    *YP = AllocIntArray(cols, rows);
    *UP = AllocIntArray(cols, rows);
    *VP = AllocIntArray(cols, rows);

    *pixelsP = ppm_allocarray(cols, rows);

    *orig_yP  = AllocUint8Array(cols, rows);
    *orig_cbP = AllocUint8Array(cols, rows);
    *orig_crP = AllocUint8Array(cols, rows);
}



static void 
FreeArray(void ** const array, unsigned int const rows) {

    unsigned int y;

    for (y = 0; y < rows; y++)
        free(array[y]);
    free(array);
}



static void
freeStorage(int const rows,
            int ** const Y, int ** const U, int ** const V,
            pixel ** const pixels, 
            uint8 ** const orig_y, uint8 ** const orig_cb,
            uint8 ** const orig_cr) {
    
    FreeArray((void**) orig_y, rows); 
    FreeArray((void**) orig_cb, rows); 
    FreeArray((void**) orig_cr, rows);

    ppm_freearray(pixels, rows);

    FreeArray((void**) Y, rows);
    FreeArray((void**) U, rows);
    FreeArray((void**) V, rows);
}



static void 
YUVtoPPM(unsigned int const cols, unsigned int const rows,
         uint8 ** const orig_y, uint8 ** const orig_cb, uint8 ** const orig_cr,
         pixel ** const pixels, 
         int ** const Y, int ** const U, int ** const V) {
/*----------------------------------------------------------------------------
   Convert the YUV image in arrays orig_y[][], orig_cb[][], and orig_cr[][]
   to a PPM image in the array (already allocated) pixels[][].

   Use the preallocated areas Y[][], U[][], and V[][] for working space.
-----------------------------------------------------------------------------*/
    
    int y;

    for ( y = 0; y < rows/2; y ++ ) {
        int x;
        for ( x = 0; x < cols/2; x ++ ) {
            U[y][x] = orig_cb[y][x] - 128;
            V[y][x] = orig_cr[y][x] - 128;
        }
    }

    for ( y = 0; y < rows; y ++ ) {
        int x;
        for ( x = 0; x < cols; x ++ ) 
            Y[y][x] = orig_y[y][x] - 16;
    }

    for ( y = 0; y < rows; y++ ) {
        int x;
        for ( x = 0; x < cols; x++ ) {
            long   tempR, tempG, tempB;
            int     r, g, b;
            /* look at yuvtoppm source for explanation */

            tempR = 104635*V[y/2][x/2];
            tempG = -25690*U[y/2][x/2] + -53294 * V[y/2][x/2];
            tempB = 132278*U[y/2][x/2];

            tempR += (Y[y][x]*76310);
            tempG += (Y[y][x]*76310);
            tempB += (Y[y][x]*76310);
            
            r = CHOP((int)(tempR >> 16));
            g = CHOP((int)(tempG >> 16));
            b = CHOP((int)(tempB >> 16));
            
            PPM_ASSIGN(pixels[y][x], r, g, b);
        }
    }
}



static void 
ReadYUV(FILE * const yuvfile,
        unsigned int const cols, unsigned int const rows,
        uint8 ** const orig_y, 
        uint8 ** const orig_cb, 
        uint8 ** const orig_cr,
        bool * const eofP) {

    unsigned int y;
    int c;

    c = fgetc(yuvfile);
    if (c < 0)
        *eofP = TRUE;
    else {
        *eofP = FALSE;
        ungetc(c, yuvfile);
    }
    if (!*eofP) {
        for (y = 0; y < rows; y++)            /* Y */
            fread(orig_y[y], 1, cols, yuvfile);
        
        for (y = 0; y < rows / 2; y++)            /* U */
            fread(orig_cb[y], 1, cols / 2, yuvfile);
        
        for (y = 0; y < rows / 2; y++)            /* V */
            fread(orig_cr[y], 1, cols / 2, yuvfile);
        if (feof(yuvfile))
            pm_error("Premature end of file reading EYUV input file");
    }
}



int
main(int argc, char **argv) {

    FILE *ifp;
    struct cmdline_info cmdline;
    unsigned int frameSeq;

    /* The following are addresses of malloc'ed storage areas for use by
       subroutines.
    */
    int ** Y;
    int ** U;
    int ** V;
    uint8 **orig_y, **orig_cb, **orig_cr;
    pixel ** pixels;

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    /* Allocate all the storage once, to save time. */
    allocateStorage(cmdline.width, cmdline.height,
                    &Y, &U, &V, &pixels, &orig_y, &orig_cb, &orig_cr);

    ifp = pm_openr(cmdline.input_filespec);

    for (frameSeq = 0; !feof(ifp); frameSeq++) {
        bool eof;

        ReadYUV(ifp, cmdline.width, cmdline.height, 
                orig_y, orig_cb, orig_cr, &eof);
        if (!eof) {
            pm_message("Converting Frame %u", frameSeq);

            YUVtoPPM(cmdline.width, cmdline.height, orig_y, orig_cb, orig_cr, 
                     pixels, Y, U, V);
            ppm_writeppm(stdout, pixels, cmdline.width, cmdline.height, 
                         255, FALSE);
        }
    }

    freeStorage(cmdline.height, Y, U, V, pixels, orig_y, orig_cb, orig_cr);

    pm_close(ifp);
    exit(0);
}





