/*
   I recently had a visit from my mom who owns a Sony Mavica camera.
   This camera produces standard MPEG and JPEG files, but it also
   creates 64x48 pixel thumbnails for preview/index on its own tiny
   LCD screen.  These files are named with an extension that is
   ".411".

   Sony appears not to want to document the ".411" file format, but it
   is clear from various web pages that it is a variant of the
   CCIR.601 standard YUV encoding used in MPEG.  The name indicates
   that the file content consists of chunks of 6 bytes: 4 bytes of
   image Y values, followed by 1 bytes of U and one byte of V values
   that apply to the previous 4 Y pixel values.

   There appear to be some commercial 411 file readers on the net, and
   there is the Java-based Javica program, but I prefer Open Source
   command-line utilities.  So, I grabbed a copy of netpbm-9.11 from
   SourceForge and hacked the eyuvtoppm.c file so that it looks like
   this.  While this may not be exactly the right thing to do, it
   produces results which are close enough for me.

   There are all sorts of user-interface gotchas possible in here that
   I'm not going to bother changing -- especially not without actual
   documentation from Sony about what they intend to do with ".411"
   files in the future.  I place my modifications into the public
   domain, but I ask that my name & e-mail be mentioned in the
   commentary of any derived version.

   Steve Allen <sla@alumni.caltech.edu>, 2001-03-01

   Bryan Henderson reworked the program to use the Netpbm libraries to
   create the PPM output and follow some other Netpbm conventions.
   2001-03-03.  Bryan's contribution is public domain.  
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
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.  */

/*==============*
 * HEADER FILES *
 *==============*/
#include <stdio.h>
#include <stdlib.h>

#include "ppm.h"
#include "shhopt.h"
#include "mallocvar.h"

typedef unsigned char uint8;

#define CHOP(x)     ((x < 0) ? 0 : ((x > 255) ? 255 : x))

struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespec of input file */
    int width;
    int height;
};



static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdline_p) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/

    optStruct *option_def = malloc(100*sizeof(optStruct));
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct2 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENTRY(0,   "width",      OPT_INT,    &cmdline_p->width,          0);
    OPTENTRY(0,   "height",     OPT_INT,    &cmdline_p->height,         0);

    /* Set the defaults */
    cmdline_p->width = 64;
    cmdline_p->height = 48;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */
    
    optParseOptions2(&argc, argv, opt, 0);
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (cmdline_p->width <= 0)
        pm_error("-width must be positive.");
    if (cmdline_p->height <= 0)
        pm_error("-height must be positive.");

    if (argc > 2)
        pm_error("There is at most 1 argument: the input file spec.  "
                 "You supplied %d", argc-1);
    else {
        if (argc > 1)
            cmdline_p->input_filespec = argv[1];
        else
            cmdline_p->input_filespec = "-";
    }
}



static void 
ReadYUV(FILE *fpointer, const int width, const int height, 
        uint8 ** const orig_y, uint8 ** const orig_cb, 
        uint8 ** const orig_cr) {

    int y, x, i;

    for (y = 0; y < height; y++) {
        for (x = 0, i = 0; x < width; x+=4, i++) {
            fread(orig_y[y]+x , 1, 4, fpointer);
            fread(orig_cb[y]+i, 1, 1, fpointer);
            fread(orig_cr[y]+i, 1, 1, fpointer);
        }
    }
}



static void
AllocYCC(const int width, const int height,
         uint8 *** const orig_yP, uint8 *** const orig_crP, 
         uint8 *** const orig_cbP) {
    int y;

    MALLOCARRAY_NOFAIL(*orig_yP, height);
    for (y = 0; y < height; y++) 
        MALLOCARRAY_NOFAIL((*orig_yP)[y], width);

    MALLOCARRAY_NOFAIL(*orig_crP, height);
    for (y = 0; y < height; y++) 
        MALLOCARRAY_NOFAIL((*orig_crP)[y], width/4);

    MALLOCARRAY_NOFAIL(*orig_cbP, height);
    for (y = 0; y < height; y++) 
        MALLOCARRAY_NOFAIL((*orig_cbP)[y], width/4);
}



static void 
YUVtoPPM(const int width, const int height, 
         uint8 ** const orig_y, 
         uint8 ** const orig_cb,
         uint8 ** const orig_cr,
         pixel ** const ppm_image
         ) {
    int     **Y, **U, **V;
    int    y;

    /* first, allocate tons of memory */

    MALLOCARRAY_NOFAIL(Y, height);
    for (y = 0; y < height; y++) 
        MALLOCARRAY_NOFAIL(Y[y], width);
    
    MALLOCARRAY_NOFAIL(U, height);
    for (y = 0; y < height; y++) 
        MALLOCARRAY_NOFAIL(U[y], width / 4);

    MALLOCARRAY_NOFAIL(V, height);
    for (y = 0; y < height; y++) 
        MALLOCARRAY_NOFAIL(V[y], width/4);

	for ( y = 0; y < height; y ++ ) {
        int x;
	    for ( x = 0; x < width/4; x ++ ) {
            U[y][x] = orig_cb[y][x] - 128;
            V[y][x] = orig_cr[y][x] - 128;
	    }
    }
	for ( y = 0; y < height; y ++ ) {
        int x;
	    for ( x = 0; x < width; x ++ )
	    {
            Y[y][x] = orig_y[y][x] - 16;
	    }
    }
    for ( y = 0; y < height; y++ ) {
        int x;
        for ( x = 0; x < width; x++ ) {
            pixval r, g, b;
            long   tempR, tempG, tempB;
            /* look at yuvtoppm source for explanation */
            
            tempR = 104635*V[y][x/4];
            tempG = -25690*U[y][x/4] + -53294 * V[y][x/4];
            tempB = 132278*U[y][x/4];
            
            tempR += (Y[y][x]*76310);
            tempG += (Y[y][x]*76310);
            tempB += (Y[y][x]*76310);
            
            r = CHOP((int)(tempR >> 16));
            g = CHOP((int)(tempG >> 16));
            b = CHOP((int)(tempB >> 16));

            PPM_ASSIGN(ppm_image[y][x], r, g, b);
        }
    }
    /* We really should free the Y, U, and V arrays now */
}



int
main(int argc, char **argv) {
    FILE *infile;
    struct cmdline_info cmdline;
    uint8 **orig_y, **orig_cb, **orig_cr;
    pixel **ppm_image;

    ppm_init(&argc, argv);

    parse_command_line(argc, argv, &cmdline);

    AllocYCC(cmdline.width, cmdline.height, &orig_y, &orig_cr, &orig_cb);
    ppm_image = ppm_allocarray(cmdline.width, cmdline.height);

    pm_message("Reading (%dx%d):  %s\n", cmdline.width, cmdline.height, 
               cmdline.input_filespec);
    infile = pm_openr(cmdline.input_filespec);
    ReadYUV(infile, cmdline.width, cmdline.height, orig_y, orig_cb, orig_cr);
    pm_close(infile);

    YUVtoPPM(cmdline.width, cmdline.height, orig_y, orig_cb, orig_cr, 
             ppm_image);

    ppm_writeppm(stdout, ppm_image, cmdline.width, cmdline.height, 255, 0);

    ppm_freearray(ppm_image, cmdline.height);

    return 0;
}

