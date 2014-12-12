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
** Modified:
**
** June 6, 2001: Christopher W. Boyd <cboyd@pobox.com>
**               - added -reduce N to allow scaling by integer value
**                 in this case, scale_comp becomes 1/N and x/yscale
**                 get set as they should
**    
**
*/
 
#include <math.h>
#include "pnm.h"
#include "shhopt.h"

/* The pnm library allows us to code this program without branching cases
   for PGM and PPM, but we do the branch anyway to speed up processing of 
   PGM images.
*/

/* We do all our arithmetic in integers.  In order not to get killed by the
   rounding, we scale every number up by the factor SCALE, do the 
   arithmetic, then scale it back down.
   */
#define SCALE 4096
#define HALFSCALE 2048


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
    int xysize, xsize, ysize, pixels;
    int reduce;
    float xscale, yscale, scale_parm;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENTRY(0,   "xsize",     OPT_UINT,    &xsize,         0);
    OPTENTRY(0,   "width",     OPT_UINT,    &xsize,         0);
    OPTENTRY(0,   "ysize",     OPT_UINT,    &ysize,         0);
    OPTENTRY(0,   "height",    OPT_UINT,    &ysize,         0);
    OPTENTRY(0,   "xscale",    OPT_FLOAT,   &xscale,        0);
    OPTENTRY(0,   "yscale",    OPT_FLOAT,   &yscale,        0);
    OPTENTRY(0,   "pixels",    OPT_UINT,    &pixels,        0);
    OPTENTRY(0,   "xysize",    OPT_FLAG,    &xysize,        0);
    OPTENTRY(0,   "verbose",   OPT_FLAG,    &cmdline_p->verbose,        0);
    OPTENTRY(0,   "reduce",    OPT_UINT,    &reduce,        0);

    /* Set the defaults. -1 = unspecified */
    xsize = -1;
    ysize = -1;
    xscale = -1.0;
    yscale = -1.0;
    pixels = -1;
    xysize = 0;
    reduce = -1;
    cmdline_p->verbose = FALSE;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions2(&argc, argv, opt, 0);
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
            cmdline_p->xbox = atoi(argv[1]);
            cmdline_p->ybox = atoi(argv[2]);
            
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
                 const int cols, const int newcols, const long sxscale, 
                 const int format, const xelval maxval,
                 int * stretchP) {
/*----------------------------------------------------------------------------
   Take the input row inputxelrow[], which is 'cols' columns wide, and
   scale it by a factor of 'sxcale', which is in SCALEths to create
   the output row newxelrow[], which is 'newcols' columns wide.

   'format' and 'maxval' describe the Netpbm format of the both input and
   output rows.

   *stretchP is the number of columns (could be fractional) on the right 
   that we had to fill by stretching due to rounding problems.
-----------------------------------------------------------------------------*/
    long r, g, b;
    long fraccoltofill, fraccolleft;
    unsigned int col;
    unsigned int newcol;
    
    newcol = 0;
    fraccoltofill = SCALE;  /* Output column is "empty" now */
    r = g = b = 0;          /* initial value */
    for (col = 0; col < cols; ++col) {
        /* Process one pixel from input ('inputxelrow') */
        fraccolleft = sxscale;
        /* Output all columns, if any, that can be filled using information
           from this input column, in addition what's already in the output
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
                r /= SCALE;
                if ( r > maxval ) r = maxval;
                g /= SCALE;
                if ( g > maxval ) g = maxval;
                b /= SCALE;
                if ( b > maxval ) b = maxval;
                PPM_ASSIGN( newxelrow[newcol], r, g, b );
                break;

            default:
                g += fraccoltofill * PNM_GET1(inputxelrow[col]);
                g /= SCALE;
                if ( g > maxval ) g = maxval;
                PNM_ASSIGN1( newxelrow[newcol], g );
                break;
            }
            fraccolleft -= fraccoltofill;
            /* Set up to start filling next output column */
            newcol++;
            fraccoltofill = SCALE;
            r = g = b = 0;
        }
        /* There's not enough left in the current input pixel to fill up 
           a whole output column, so just accumulate the remainder of the
           pixel into the current output column.
        */
        if (fraccolleft > 0) {
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

    *stretchP = 0;   /* initial value */
    while (newcol < newcols) {
        /* We ran out of input columns before we filled up the output
           columns.  This would be because of rounding down.  For small
           images, we're probably missing only a tiny fraction of a column, 
           but for large images, it could be multiple columns.

           So we fake the remaining output columns by copying the rightmost
           legitimate pixel.  We call this stretching.
           */

        *stretchP += fraccoltofill;

        switch (PNM_FORMAT_TYPE(format)) {
        case PPM_TYPE:
            r += fraccoltofill * PPM_GETR(inputxelrow[cols-1]);
            g += fraccoltofill * PPM_GETG(inputxelrow[cols-1]);
            b += fraccoltofill * PPM_GETB(inputxelrow[cols-1]);

            r += HALFSCALE;  /* for rounding */
            r /= SCALE;
            if ( r > maxval ) r = maxval;
            g += HALFSCALE;  /* for rounding */
            g /= SCALE;
            if ( g > maxval ) g = maxval;
            b += HALFSCALE;  /* for rounding */
            b /= SCALE;
            if ( b > maxval ) b = maxval;
            PPM_ASSIGN(newxelrow[newcol], r, g, b );
            break;
                
        default:
            g += fraccoltofill * PNM_GET1(inputxelrow[cols-1]);
            g += HALFSCALE;  /* for rounding */
            g /= SCALE;
            if ( g > maxval ) g = maxval;
            PNM_ASSIGN1(newxelrow[newcol], g );
            break;
        }
        newcol++;
        fraccoltofill = SCALE;
    }
}


int
main(int argc, char **argv ) {

    struct cmdline_info cmdline;
    FILE* ifp;
    xel* xelrow;
    xel* tempxelrow;
    xel* newxelrow;
    xel* xP;
    xel* nxP;
    int rows, cols, format, newformat, rowsread, newrows, newcols;
    int row, col, needtoreadrow;
    xelval maxval, newmaxval;
    long sxscale, syscale;
    long fracrowtofill, fracrowleft;
    long* rs;
    long* gs;
    long* bs;
    int vertical_stretch;
        /* The number of rows we had to fill by stretching because of 
           rounding error, which made us run out of input rows before we
           had filled up the output rows.
           */

    pnm_init( &argc, argv );

    parse_command_line(argc, argv, &cmdline);

    ifp = pm_openr(cmdline.input_filespec);

    pnm_readpnminit( ifp, &cols, &rows, &maxval, &format );

    /* Promote PBM files to PGM. */
    if ( PNM_FORMAT_TYPE(format) == PBM_TYPE ) {
        newformat = PGM_TYPE;
        newmaxval = PGM_MAXMAXVAL;
        pm_message( "promoting from PBM to PGM" );
	}  else {
        newformat = format;
        newmaxval = maxval;
    }
    compute_output_dimensions(cmdline, rows, cols, &newrows, &newcols);

    /* We round the scale factor down so that we never fill up the
       output while (a fractional pixel of) input remains unused.
       Instead, we will run out of input while some of the output is
       unfilled.  We can address that by stretching, whereas the other
       case would require throwing away some of the input.
    */
    sxscale = SCALE * newcols / cols;
    syscale = SCALE * newrows / rows;

    if (cmdline.verbose) {
        pm_message("Scaling by %ld/%d = %f horizontally to %d columns.", 
                   sxscale, SCALE, (float) sxscale/SCALE, newcols );
        pm_message("Scaling by %ld/%d = %f vertically to %d rows.", 
                   syscale, SCALE, (float) syscale/SCALE, newrows);
    }

    xelrow = pnm_allocrow(cols);
    if (newrows == rows)	/* shortcut Y scaling if possible */
        tempxelrow = xelrow;
    else
        tempxelrow = pnm_allocrow( cols );
    rs = (long*) pm_allocrow( cols, sizeof(long) );
    gs = (long*) pm_allocrow( cols, sizeof(long) );
    bs = (long*) pm_allocrow( cols, sizeof(long) );
    rowsread = 0;
    fracrowleft = syscale;
    needtoreadrow = 1;
    for ( col = 0; col < cols; ++col )
	rs[col] = gs[col] = bs[col] = HALFSCALE;
    fracrowtofill = SCALE;
    vertical_stretch = 0;
    
    pnm_writepnminit( stdout, newcols, newrows, newmaxval, newformat, 0 );
    newxelrow = pnm_allocrow( newcols );
    
    for ( row = 0; row < newrows; ++row ) {
        /* First scale vertically from xelrow into tempxelrow. */
        if ( newrows == rows ) { /* shortcut vertical scaling if possible */
            pnm_readpnmrow( ifp, xelrow, cols, newmaxval, format );
	    } else {
            while ( fracrowleft < fracrowtofill ) {
                if ( needtoreadrow )
                    if ( rowsread < rows ) {
                        pnm_readpnmrow( ifp, xelrow, cols, newmaxval, format );
                        ++rowsread;
                    }
                switch ( PNM_FORMAT_TYPE(format) ) {
                case PPM_TYPE:
                    for ( col = 0, xP = xelrow; col < cols; ++col, ++xP ) {
                        rs[col] += fracrowleft * PPM_GETR( *xP );
                        gs[col] += fracrowleft * PPM_GETG( *xP );
                        bs[col] += fracrowleft * PPM_GETB( *xP );
                    }
                    break;

                default:
                    for ( col = 0, xP = xelrow; col < cols; ++col, ++xP )
                        gs[col] += fracrowleft * PNM_GET1( *xP );
                    break;
                }
                fracrowtofill -= fracrowleft;
                fracrowleft = syscale;
                needtoreadrow = 1;
            }
            /* Now fracrowleft is >= fracrowtofill, so we can produce a row. */
            if ( needtoreadrow ) {
                if ( rowsread < rows ) {
                    pnm_readpnmrow( ifp, xelrow, cols, newmaxval, format );
                    ++rowsread;
                    needtoreadrow = 0;
                } else {
                    /* We need another input row to fill up this output row,
                       but there aren't any more.  That's because of rounding
                       down on our scaling arithmetic.  So we go ahead with 
                       the data from the last row we read, which amounts to 
                       stretching out the last output row.
                    */
                    vertical_stretch += fracrowtofill;
                }
            }
            switch ( PNM_FORMAT_TYPE(format) ) {
            case PPM_TYPE:
                for ( col = 0, xP = xelrow, nxP = tempxelrow;
                      col < cols; ++col, ++xP, ++nxP ) {
                    register long r, g, b;

                    r = rs[col] + fracrowtofill * PPM_GETR( *xP );
                    g = gs[col] + fracrowtofill * PPM_GETG( *xP );
                    b = bs[col] + fracrowtofill * PPM_GETB( *xP );
                    r /= SCALE;
                    if ( r > newmaxval ) r = newmaxval;
                    g /= SCALE;
                    if ( g > newmaxval ) g = newmaxval;
                    b /= SCALE;
                    if ( b > newmaxval ) b = newmaxval;
                    PPM_ASSIGN( *nxP, r, g, b );
                    rs[col] = gs[col] = bs[col] = HALFSCALE;
                }
                break;

            default:
                for ( col = 0, xP = xelrow, nxP = tempxelrow;
                      col < cols; ++col, ++xP, ++nxP ) {
                    register long g;
                    
                    g = gs[col] + fracrowtofill * PNM_GET1( *xP );
                    g /= SCALE;
                    if ( g > newmaxval ) g = newmaxval;
                    PNM_ASSIGN1( *nxP, g );
                    gs[col] = HALFSCALE;
                }
                break;
            }
            fracrowleft -= fracrowtofill;
            if ( fracrowleft == 0 ) {
                fracrowleft = syscale;
                needtoreadrow = 1;
            }
            fracrowtofill = SCALE;
	    }

        /* Now scale tempxelrow horizontally into newxelrow & write it out. */

        if (newcols == cols)	/* shortcut X scaling if possible */
            pnm_writepnmrow(stdout, tempxelrow, newcols, 
                            newmaxval, newformat, 0);
        else {
            int stretch;

            horizontal_scale(tempxelrow, newxelrow, cols, newcols, sxscale, 
                             format, newmaxval, &stretch);
            
            if (cmdline.verbose && row == 0 && stretch != 0)
                pm_message("%d/%d = %f right columns filled by stretching "
                           "due to arithmetic imprecision", 
                           stretch, SCALE, (float) stretch/SCALE);
            
            pnm_writepnmrow(stdout, newxelrow, newcols, 
                            newmaxval, newformat, 0 );
        }
	}

    if (cmdline.verbose && vertical_stretch != 0)
        pm_message("%d/%d = %f bottom rows filled by stretching due to "
                   "arithmetic imprecision", 
                   vertical_stretch, SCALE, 
                   (float) vertical_stretch/SCALE);
    
    pm_close( ifp );
    pm_close( stdout );
    
    exit( 0 );
}
