/* pnmscale.c - read a portable anymap and scale it
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
*/

/* 

      DON'T ADD NEW FUNCTION TO THIS PROGRAM.  ADD IT TO pamscale.c
      INSTEAD.

*/

 
#include <math.h>
#include <string.h>

#include "pnm.h"
#include "shhopt.h"

/* The pnm library allows us to code this program without branching cases
   for PGM and PPM, but we do the branch anyway to speed up processing of 
   PGM images.
*/


struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespecs of input files */
    unsigned int xsize;
    unsigned int ysize;
    float xscale;
    float yscale;
    unsigned int xbox;
    unsigned int ybox;
    unsigned int pixels;
    unsigned int verbose;
    unsigned int nomix;
};


static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdline_p) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc( 100*sizeof( optEntry ) );
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    unsigned int xysize;
    int xsize, ysize, pixels;
    int reduce;
    float xscale, yscale, scale_parm;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "xsize",     OPT_UINT,    &xsize,               NULL, 0);
    OPTENT3(0, "width",     OPT_UINT,    &xsize,               NULL, 0);
    OPTENT3(0, "ysize",     OPT_UINT,    &ysize,               NULL, 0);
    OPTENT3(0, "height",    OPT_UINT,    &ysize,               NULL, 0);
    OPTENT3(0, "xscale",    OPT_FLOAT,   &xscale,              NULL, 0);
    OPTENT3(0, "yscale",    OPT_FLOAT,   &yscale,              NULL, 0);
    OPTENT3(0, "pixels",    OPT_UINT,    &pixels,              NULL, 0);
    OPTENT3(0, "reduce",    OPT_UINT,    &reduce,              NULL, 0);
    OPTENT3(0, "xysize",    OPT_FLAG,    NULL, &xysize,              0);
    OPTENT3(0, "verbose",   OPT_FLAG,    NULL, &cmdline_p->verbose,  0);
    OPTENT3(0, "nomix",     OPT_FLAG,    NULL, &cmdline_p->nomix,    0);

    /* Set the defaults. -1 = unspecified */
    /* (Now that we're using ParseOptions3, we don't have to do this -1
       nonsense, but we don't want to risk screwing these complex 
       option compatibilities up, so we'll convert that later.
    */
    xsize = -1;
    ysize = -1;
    xscale = -1.0;
    yscale = -1.0;
    pixels = -1;
    reduce = -1;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (xsize == 0)
        pm_error("-xsize/width must be greater than zero.");
    if (ysize == 0)
        pm_error("-ysize/height must be greater than zero.");
    if (xscale != -1.0 && xscale <= 0.0)
        pm_error("-xscale must be greater than zero.");
    if (yscale != -1.0 && yscale <= 0.0)
        pm_error("-yscale must be greater than zero.");
    if (reduce <= 0 && reduce != -1)
        pm_error("-reduce must be greater than zero.");

    if (xsize != -1 && xscale != -1)
        pm_error("Cannot specify both -xsize/width and -xscale.");
    if (ysize != -1 && yscale != -1)
        pm_error("Cannot specify both -ysize/height and -yscale.");
    
    if (xysize && 
        (xsize != -1 || xscale != -1 || ysize != -1 || yscale != -1 || 
         reduce != -1 || pixels != -1) )
        pm_error("Cannot specify -xysize with other dimension options.");
    if (pixels != -1 && 
        (xsize != -1 || xscale != -1 || ysize != -1 || yscale != -1 ||
         reduce != -1) )
        pm_error("Cannot specify -pixels with other dimension options.");
    if (reduce != -1 && 
        (xsize != -1 || xscale != -1 || ysize != -1 || yscale != -1) )
        pm_error("Cannot specify -reduce with other dimension options.");

    if (pixels == 0)
        pm_error("-pixels must be greater than zero");

    /* Get the program parameters */

    if (xysize) {
        /* parameters are xbox, ybox, and optional filespec */
        scale_parm = 0.0;
        if (argc-1 < 2)
            pm_error("You must supply at least two parameters with -xysize:\n "
                     "x and y dimensions of the bounding box.");
        else if (argc-1 > 3)
            pm_error("Too many arguments.  With -xysize, you need 2 or 3 "
                     "arguments.");
        else {
            char * endptr;
            cmdline_p->xbox = strtol(argv[1], &endptr, 10);
            if (strlen(argv[1]) > 0 && *endptr != '\0')
                pm_error("horizontal xysize not an integer: '%s'", argv[1]);
            if (cmdline_p->xbox <= 0)
                pm_error("horizontal size is not positive: %d", 
                         cmdline_p->xbox);

            cmdline_p->ybox = strtol(argv[2], &endptr, 10);
            if (strlen(argv[2]) > 0 && *endptr != '\0')
                pm_error("vertical xysize not an integer: '%s'", argv[2]);
            if (cmdline_p->ybox <= 0)
                pm_error("vertical size is not positive: %d", 
                         cmdline_p->ybox);
            
            if (argc-1 < 3)
                cmdline_p->input_filespec = "-";
            else
                cmdline_p->input_filespec = argv[3];
        }
    } else {
        cmdline_p->xbox = 0;
        cmdline_p->ybox = 0;
        
        if (xsize == -1 && xscale == -1 && ysize == -1 && yscale == -1
            && pixels == -1 && reduce == -1) {
            /* parameters are scale factor and optional filespec */
            if (argc-1 < 1)
                pm_error("With no dimension options, you must supply at least "
                         "one parameter: \nthe scale factor.");
            else {
                scale_parm = atof(argv[1]);

                if (scale_parm == 0.0)
                    pm_error("The scale parameter %s is not "
                             "a positive number.",
                             argv[1]);
                else {
                    if (argc-1 < 2)
                        cmdline_p->input_filespec = "-";
                    else
                        cmdline_p->input_filespec = argv[2];
                }
            }
        } else {
            /* Only parameter allowed is optional filespec */
            if (argc-1 < 1)
                cmdline_p->input_filespec = "-";
            else
                cmdline_p->input_filespec = argv[1];

            if (reduce != -1) {
                scale_parm = ((double) 1.0) / ((double) reduce);
                pm_message("reducing by %d gives scale factor of %f.", 
                           reduce, scale_parm);
            } else
                scale_parm = 0.0;
        }
    }

    cmdline_p->xsize = xsize == -1 ? 0 : xsize;
    cmdline_p->ysize = ysize == -1 ? 0 : ysize;
    cmdline_p->pixels = pixels == -1 ? 0 : pixels;

    if (scale_parm) {
        cmdline_p->xscale = scale_parm;
        cmdline_p->yscale = scale_parm;
    } else {
        cmdline_p->xscale = xscale == -1.0 ? 0.0 : xscale;
        cmdline_p->yscale = yscale == -1.0 ? 0.0 : yscale;
    }
}



static void
compute_output_dimensions(const struct cmdline_info cmdline, 
                          const int rows, const int cols,
                          int * newrowsP, int * newcolsP) {

    if (cmdline.pixels) {
        if (rows * cols <= cmdline.pixels) {
            *newrowsP = rows;
            *newcolsP = cols;
        } else {
            const double scale =
                sqrt( (float) cmdline.pixels / ((float) cols * (float) rows));
            *newrowsP = rows * scale;
            *newcolsP = cols * scale;
        }
    } else if (cmdline.xbox) {
        const double aspect_ratio = (float) cols / (float) rows;
        const double box_aspect_ratio = 
            (float) cmdline.xbox / (float) cmdline.ybox;
        
        if (box_aspect_ratio > aspect_ratio) {
            *newrowsP = cmdline.ybox;
            *newcolsP = *newrowsP * aspect_ratio + 0.5;
        } else {
            *newcolsP = cmdline.xbox;
            *newrowsP = *newcolsP / aspect_ratio + 0.5;
        }
    } else {
        if (cmdline.xsize)
            *newcolsP = cmdline.xsize;
        else if (cmdline.xscale)
            *newcolsP = cmdline.xscale * cols + .5;
        else if (cmdline.ysize)
            *newcolsP = cols * ((float) cmdline.ysize/rows) +.5;
        else
            *newcolsP = cols;

        if (cmdline.ysize)
            *newrowsP = cmdline.ysize;
        else if (cmdline.yscale)
            *newrowsP = cmdline.yscale * rows +.5;
        else if (cmdline.xsize)
            *newrowsP = rows * ((float) cmdline.xsize/cols) +.5;
        else
            *newrowsP = rows;
    }    

    /* If the calculations above yielded (due to rounding) a zero 
       dimension, we fudge it up to 1.  We do this rather than considering
       it a specification error (and dying) because it's friendlier to 
       automated processes that work on arbitrary input.  It saves them
       having to check their numbers to avoid catastrophe.
    */

    if (*newcolsP < 1) *newcolsP = 1;
    if (*newrowsP < 1) *newrowsP = 1;
}        



static void
horizontal_scale(const xel inputxelrow[], xel newxelrow[], 
                 const int cols, const int newcols, const float xscale, 
                 const int format, const xelval maxval,
                 float * const stretchP) {
/*----------------------------------------------------------------------------
   Take the input row inputxelrow[], which is 'cols' columns wide, and
   scale it by a factor of 'xscale', to create
   the output row newxelrow[], which is 'newcols' columns wide.

   'format' and 'maxval' describe the Netpbm format of the both input and
   output rows.
-----------------------------------------------------------------------------*/
    float r, g, b;
    float fraccoltofill, fraccolleft;
    unsigned int col;
    unsigned int newcol;
    
    newcol = 0;
    fraccoltofill = 1.0;  /* Output column is "empty" now */
    r = g = b = 0;          /* initial value */
    for (col = 0; col < cols; ++col) {
        /* Process one pixel from input ('inputxelrow') */
        fraccolleft = xscale;
        /* Output all columns, if any, that can be filled using information
           from this input column, in addition to what's already in the output
           column.
        */
        while (fraccolleft >= fraccoltofill) {
            /* Generate one output pixel in 'newxelrow'.  It will consist
               of anything accumulated from prior input pixels in 'r','g', 
               and 'b', plus a fraction of the current input pixel.
            */
            switch (PNM_FORMAT_TYPE(format)) {
            case PPM_TYPE:
                r += fraccoltofill * PPM_GETR(inputxelrow[col]);
                g += fraccoltofill * PPM_GETG(inputxelrow[col]);
                b += fraccoltofill * PPM_GETB(inputxelrow[col]);
                PPM_ASSIGN( newxelrow[newcol], 
                            MIN(maxval, (int) (r + 0.5)), 
                            MIN(maxval, (int) (g + 0.5)), 
                            MIN(maxval, (int) (b + 0.5))
                    );
                break;

            default:
                g += fraccoltofill * PNM_GET1(inputxelrow[col]);
                PNM_ASSIGN1( newxelrow[newcol], MIN(maxval, (int) (g + 0.5)));
                break;
            }
            fraccolleft -= fraccoltofill;
            /* Set up to start filling next output column */
            newcol++;
            fraccoltofill = 1.0;
            r = g = b = 0.0;
        }
        /* There's not enough left in the current input pixel to fill up 
           a whole output column, so just accumulate the remainder of the
           pixel into the current output column.
        */
        if (fraccolleft > 0.0) {
            switch (PNM_FORMAT_TYPE(format)) {
            case PPM_TYPE:
                r += fraccolleft * PPM_GETR(inputxelrow[col]);
                g += fraccolleft * PPM_GETG(inputxelrow[col]);
                b += fraccolleft * PPM_GETB(inputxelrow[col]);
                break;
                    
            default:
                g += fraccolleft * PNM_GET1(inputxelrow[col]);
                break;
            }
            fraccoltofill -= fraccolleft;
        }
    }

    if (newcol < newcols-1 || newcol > newcols)
        pm_error("Internal error: last column filled is %d, but %d "
                 "is the rightmost output column.",
                 newcol, newcols-1);

    if (newcol < newcols ) {
        /* We were still working on the last output column when we 
           ran out of input columns.  This would be because of rounding
           down, and we should be missing only a tiny fraction of that
           last output column.
        */

        *stretchP = fraccoltofill;

        switch (PNM_FORMAT_TYPE(format)) {
        case PPM_TYPE:
            r += fraccoltofill * PPM_GETR(inputxelrow[cols-1]);
            g += fraccoltofill * PPM_GETG(inputxelrow[cols-1]);
            b += fraccoltofill * PPM_GETB(inputxelrow[cols-1]);

            PPM_ASSIGN(newxelrow[newcol], 
                       MIN(maxval, (int) (r + 0.5)), 
                       MIN(maxval, (int) (g + 0.5)), 
                       MIN(maxval, (int) (b + 0.5))
                );
            break;
                
        default:
            g += fraccoltofill * PNM_GET1(inputxelrow[cols-1]);
            PNM_ASSIGN1( newxelrow[newcol], MIN(maxval, (int) (g + 0.5)));
            break;
        }
    } else 
        *stretchP = 0;
}



static void
zeroAccum(int const cols, int const format, 
          float rs[], float gs[], float bs[]) {

    int col;

    for ( col = 0; col < cols; ++col )
        rs[col] = gs[col] = bs[col] = 0.0;
}



static void
accumOutputRow(xel * const xelrow, float const fraction, 
               float rs[], float gs[], float bs[], 
               int const cols, int const format) {
/*----------------------------------------------------------------------------
   Take 'fraction' times the color in row xelrow and add it to 
   rs/gs/bs.  'fraction' is less than 1.0.
-----------------------------------------------------------------------------*/
    int col;

    switch ( PNM_FORMAT_TYPE(format) ) {
    case PPM_TYPE:
        for ( col = 0; col < cols; ++col ) {
            rs[col] += fraction * PPM_GETR(xelrow[col]);
            gs[col] += fraction * PPM_GETG(xelrow[col]);
            bs[col] += fraction * PPM_GETB(xelrow[col]);
        }
        break;

    default:
        for ( col = 0; col < cols; ++col)
            gs[col] += fraction * PNM_GET1(xelrow[col]);
        break;
    }
}



static void
makeRow(xel * const xelrow, float rs[], float gs[], float bs[],
        int const cols, xelval const maxval, int const format) {
/*----------------------------------------------------------------------------
   Make an xel row at 'xelrow' with format 'format' and
   maxval 'maxval' out of the color values in 
   rs[], gs[], and bs[].
-----------------------------------------------------------------------------*/
    int col;

    switch ( PNM_FORMAT_TYPE(format) ) {
    case PPM_TYPE:
        for ( col = 0; col < cols; ++col) {
            PPM_ASSIGN(xelrow[col], 
                       MIN(maxval, (int) (rs[col] + 0.5)), 
                       MIN(maxval, (int) (gs[col] + 0.5)), 
                       MIN(maxval, (int) (bs[col] + 0.5))
                );
        }
        break;

    default:
        for ( col = 0; col < cols; ++col ) {
            PNM_ASSIGN1(xelrow[col], 
                        MIN(maxval, (int) (gs[col] + 0.5)));
        }
        break;
    }
}



static void
scaleWithMixing(FILE * const ifP,
                int const cols, int const rows,
                xelval const maxval, int const format,
                int const newcols, int const newrows,
                xelval const newmaxval, int const newformat,
                float const xscale, float const yscale,
                bool const verbose) {
/*----------------------------------------------------------------------------
   Scale the image on input file 'ifP' (which is described by 
   'cols', 'rows', 'format', and 'maxval') by xscale horizontally and
   yscale vertically and write the result to standard output as format
   'newformat' and with maxval 'newmaxval'.

   The input file is positioned past the header, to the beginning of the
   raster.  The output file is too.

   Mix colors from input rows together in the output rows.
-----------------------------------------------------------------------------*/
    /* Here's how we think of the color mixing scaling operation:  
       
       First, I'll describe scaling in one dimension.  Assume we have
       a one row image.  A raster row is ordinarily a sequence of
       discrete pixels which have no width and no distance between
       them -- only a sequence.  Instead, think of the raster row as a
       bunch of pixels 1 unit wide adjacent to each other.  For
       example, we are going to scale a 100 pixel row to a 150 pixel
       row.  Imagine placing the input row right above the output row
       and stretching it so it is the same size as the output row.  It
       still contains 100 pixels, but they are 1.5 units wide each.
       Our goal is to make the output row look as much as possible
       like the input row, while observing that a pixel can be only
       one color.

       Output Pixel 0 is completely covered by Input Pixel 0, so we
       make Output Pixel 0 the same color as Input Pixel 0.  Output
       Pixel 1 is covered half by Input Pixel 0 and half by Input
       Pixel 1.  So we make Output Pixel 1 a 50/50 mix of Input Pixels
       0 and 1.  If you stand back far enough, input and output will
       look the same.

       This works for all scale factors, both scaling up and scaling down.
       
       This program always stretches or squeezes the input row to be the
       same length as the output row; The output row's pixels are always
       1 unit wide.

       The same thing works in the vertical direction.  We think of
       rows as stacked strips of 1 unit height.  We conceptually
       stretch the image vertically first (same process as above, but
       in place of a single-color pixels, we have a vector of colors).
       Then we take each row this vertical stretching generates and
       stretch it horizontally.  
    */

    xel* xelrow;  /* An input row */
    xel* vertScaledRow;
        /* An output row after vertical scaling, but before horizontal
           scaling
        */
    xel* newxelrow;
    float rowsleft;
        /* The number of rows of output that need to be formed from the
           current input row (the one in xelrow[]), less the number that 
           have already been formed (either in the rs/gs/bs accumulators
           or output to the file).  This can be fractional because of the
           way we define rows as having height.
        */
    float fracrowtofill;
        /* The fraction of the current output row (the one in vertScaledRow[])
           that hasn't yet been filled in from an input row.
        */
    float *rs, *gs, *bs;
        /* The red, green, and blue color intensities so far accumulated
           from input rows for the current output row.
        */
    int rowsread;
        /* Number of rows of the input file that have been read */
    int row;
    
    xelrow = pnm_allocrow(cols); 
    vertScaledRow = pnm_allocrow(cols);
    rs = (float*) pm_allocrow( cols, sizeof(float) );
    gs = (float*) pm_allocrow( cols, sizeof(float) );
    bs = (float*) pm_allocrow( cols, sizeof(float) );
    rowsread = 0;
    rowsleft = 0.0;
    zeroAccum(cols, format, rs, gs, bs);
    fracrowtofill = 1.0;

    newxelrow = pnm_allocrow( newcols );

    for ( row = 0; row < newrows; ++row ) {
        /* First scale Y from xelrow[] into vertScaledRow[]. */

        if ( newrows == rows ) { /* shortcut Y scaling if possible */
            pnm_readpnmrow( ifP, vertScaledRow, cols, newmaxval, format );
	    } else {
            while (fracrowtofill > 0) {
                if (rowsleft <= 0.0) {
                    if (rowsread < rows) {
                        pnm_readpnmrow(ifP, xelrow, cols, newmaxval, format);
                        ++rowsread;
                    } else {
                        /* We need another input row to fill up this
                           output row, but there aren't any more.
                           That's because of rounding down on our
                           scaling arithmetic.  So we go ahead with
                           the data from the last row we read, which
                           amounts to stretching out the last output
                           row.  
                        */
                        if (verbose)
                            pm_message("%f of bottom row stretched due to "
                                       "arithmetic imprecision", 
                                       fracrowtofill);
                    }
                    rowsleft = yscale;
                }
                if (rowsleft < fracrowtofill) {
                    accumOutputRow(xelrow, rowsleft, rs, gs, bs, 
                                   cols, format);
                    fracrowtofill -= rowsleft;
                    rowsleft = 0.0;
                } else {
                    accumOutputRow(xelrow, fracrowtofill, rs, gs, bs,
                                   cols, format);
                    rowsleft = rowsleft - fracrowtofill;
                    fracrowtofill = 0.0;
                }
            }
            makeRow(vertScaledRow, rs, gs, bs, cols, newmaxval, format);
            zeroAccum(cols, format, rs, gs, bs);
            fracrowtofill = 1.0;
	    }

        /* Now scale vertScaledRow horizontally into newxelrow and write
           it out. 
        */

        if (newcols == cols)	/* shortcut X scaling if possible */
            pnm_writepnmrow(stdout, vertScaledRow, newcols, 
                            newmaxval, newformat, 0);
        else {
            float stretch;

            horizontal_scale(vertScaledRow, newxelrow, cols, newcols, xscale, 
                             format, newmaxval, &stretch);
            
            if (verbose && row == 0)
                pm_message("%f of right column stretched due to "
                           "arithmetic imprecision", 
                           stretch);
            
            pnm_writepnmrow(stdout, newxelrow, newcols, 
                            newmaxval, newformat, 0 );
        }
	}
    pnm_freerow(newxelrow);
    pnm_freerow(xelrow);
    pnm_freerow(vertScaledRow);
}



static void
scaleWithoutMixing(FILE * const ifP,
                   int const cols, int const rows,
                   xelval const maxval, int const format,
                   int const newcols, int const newrows,
                   xelval const newmaxval, int const newformat,
                   float const xscale, float const yscale) {
/*----------------------------------------------------------------------------
   Scale the image on input file 'ifP' (which is described by 
   'cols', 'rows', 'format', and 'maxval') by xscale horizontally and
   yscale vertically and write the result to standard output as format
   'newformat' and with maxval 'newmaxval'.

   The input file is positioned past the header, to the beginning of the
   raster.  The output file is too.

   Don't mix colors from different input pixels together in the output
   pixels.  Each output pixel is an exact copy of some corresponding 
   input pixel.
-----------------------------------------------------------------------------*/
    xel* xelrow;  /* An input row */
    xel* newxelrow;
    int row;
    int rowInXelrow;

    xelrow = pnm_allocrow(cols); 
    rowInXelrow = -1;

    newxelrow = pnm_allocrow(newcols);

    for (row = 0; row < newrows; ++row) {
        int col;
        
        int const inputRow = (int) (row / yscale);

        for (; rowInXelrow < inputRow; ++rowInXelrow) 
            pnm_readpnmrow(ifP, xelrow, cols, newmaxval, format);
        

        for (col = 0; col < newcols; ++col) {
            int const inputCol = (int) (col / xscale);
            
            newxelrow[col] = xelrow[inputCol];
        }

        pnm_writepnmrow(stdout, newxelrow, newcols, 
                        newmaxval, newformat, 0 );
	}
    pnm_freerow(xelrow);
    pnm_freerow(newxelrow);
}



int
main(int argc, char **argv ) {

    struct cmdline_info cmdline;
    FILE* ifP;
    int rows, cols, format, newformat, newrows, newcols;
    xelval maxval, newmaxval;
    float xscale, yscale;

    pnm_init( &argc, argv );

    parse_command_line(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.input_filespec);

    pnm_readpnminit( ifP, &cols, &rows, &maxval, &format );

    /* Promote PBM files to PGM. */
    if ( PNM_FORMAT_TYPE(format) == PBM_TYPE ) {
        newformat = PGM_TYPE;
        newmaxval = PGM_MAXMAXVAL;
        pm_message( "promoting from PBM to PGM" );
	} else {
        newformat = format;
        newmaxval = maxval;
    }
    compute_output_dimensions(cmdline, rows, cols, &newrows, &newcols);

    /* We round the scale factor down so that we never fill up the 
       output while (a fractional pixel of) input remains unused.  Instead,
       we will run out of input while (a fractional pixel of) output is 
       unfilled -- which is easier for our algorithm to handle.
       */
    xscale = (float) newcols / cols;
    yscale = (float) newrows / rows;

    if (cmdline.verbose) {
        pm_message("Scaling by %f horizontally to %d columns.", 
                   xscale, newcols );
        pm_message("Scaling by %f vertically to %d rows.", 
                   yscale, newrows);
    }

    if (xscale * cols < newcols - 1 ||
        yscale * rows < newrows - 1) 
        pm_error("Arithmetic precision of this program is inadequate to "
                 "do the specified scaling.  Use a smaller input image "
                 "or a slightly different scale factor.");

    pnm_writepnminit(stdout, newcols, newrows, newmaxval, newformat, 0);

    if (cmdline.nomix) 
        scaleWithoutMixing(ifP, cols, rows, maxval, format,
                           newcols, newrows, newmaxval, newformat, 
                           xscale, yscale);
    else
        scaleWithMixing(ifP, cols, rows, maxval, format,
                        newcols, newrows, newmaxval, newformat, 
                        xscale, yscale, cmdline.verbose);

    pm_close(ifP);
    pm_close(stdout);
    
    exit(0);
}
