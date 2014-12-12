/* pnmconvol.c - general MxN convolution on a PNM image
**
** Version 2.0.1 January 30, 1995
**
** Major rewriting by Mike Burns
** Copyright (C) 1994, 1995 by Mike Burns (burns@chem.psu.edu)
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/* A change history is at the bottom */

#include <assert.h>

#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* '-' if stdin */
    const char *kernelFilespec;
    unsigned int nooffset;
};

static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "nooffset",     OPT_FLAG,   NULL,                  
            &cmdlineP->nooffset,       0 );

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 < 1)
        pm_error("Need at least one argument: file specification of the "
                 "convolution kernel image.");

    cmdlineP->kernelFilespec = argv[1];

    if (argc-1 >= 2)
        cmdlineP->inputFilespec = argv[2];
    else
        cmdlineP->inputFilespec = "-";

    if (argc-1 > 2)
        pm_error("Too many arguments.  Only acceptable arguments are: "
                 "convolution file name and input file name");
}


/* Macros to verify that r,g,b values are within proper range */

#define CHECK_GRAY \
    if ( tempgsum < 0L ) g = 0; \
    else if ( tempgsum > maxval ) g = maxval; \
    else g = tempgsum;

#define CHECK_RED \
    if ( temprsum < 0L ) r = 0; \
    else if ( temprsum > maxval ) r = maxval; \
    else r = temprsum;

#define CHECK_GREEN \
    if ( tempgsum < 0L ) g = 0; \
    else if ( tempgsum > maxval ) g = maxval; \
    else g = tempgsum;

#define CHECK_BLUE \
    if ( tempbsum < 0L ) b = 0; \
    else if ( tempbsum > maxval ) b = maxval; \
    else b = tempbsum;

struct convolveType {
    void (*ppmConvolver)(const float ** const rweights,
                         const float ** const gweights,
                         const float ** const bweights);
    void (*pgmConvolver)(const float ** const weights);
};

static FILE* ifp;
static int crows, ccols, ccolso2, crowso2;
static int cols, rows;
static xelval maxval;
static int format, newformat;



static void
computeWeights(xel * const *   const cxels, 
               int             const ccols, 
               int             const crows,
               int             const cformat, 
               xelval          const cmaxval,
               bool            const offsetPgm,
               float ***       const rweightsP,
               float ***       const gweightsP,
               float ***       const bweightsP) {
/*----------------------------------------------------------------------------
   Compute the convolution matrix in normalized form from the PGM
   form.  Each element of the output matrix is the actual weight we give an
   input pixel -- i.e. the thing by which we multiple a value from the
   input image.

   'offsetPgm' means the PGM convolution matrix is defined in offset form so
   that it can represent negative values.  E.g. with maxval 100, 50 means
   0, 100 means 50, and 0 means -50.  If 'offsetPgm' is false, 0 means 0
   and there are no negative weights.
-----------------------------------------------------------------------------*/
    double const scale = (offsetPgm ? 2.0 : 1.0) / cmaxval;
    double const offset = offsetPgm ? - 1.0 : 0.0;

    float** rweights;
    float** gweights;
    float** bweights;

    float rsum, gsum, bsum;

    unsigned int crow;

    /* Set up the normalized weights. */
    rweights = (float**) pm_allocarray(ccols, crows, sizeof(float));
    gweights = (float**) pm_allocarray(ccols, crows, sizeof(float));
    bweights = (float**) pm_allocarray(ccols, crows, sizeof(float));

    rsum = gsum = bsum = 0.0;  /* initial value */
    
    for (crow = 0; crow < crows; ++crow) {
        unsigned int ccol;
        for (ccol = 0; ccol < ccols; ++ccol) {
            switch (PNM_FORMAT_TYPE(cformat)) {
            case PPM_TYPE:
                rsum += rweights[crow][ccol] =
                    (PPM_GETR(cxels[crow][ccol]) * scale + offset);
                gsum += gweights[crow][ccol] =
                    (PPM_GETG(cxels[crow][ccol]) * scale + offset);
                bsum += bweights[crow][ccol] =
                    (PPM_GETB(cxels[crow][ccol]) * scale + offset);
                break;
                
            default:
                gsum += gweights[crow][ccol] =
                    (PNM_GET1(cxels[crow][ccol]) * scale + offset);
                break;
            }
        }
    }
    *rweightsP = rweights;
    *gweightsP = gweights;
    *bweightsP = bweights;

    switch (PNM_FORMAT_TYPE(format)) {
    case PPM_TYPE:
        if (rsum < 0.9 || rsum > 1.1 || gsum < 0.9 || gsum > 1.1 ||
            bsum < 0.9 || bsum > 1.1) {
            pm_message("WARNING - this convolution matrix is biased.  " 
                       "red, green, and blue average weights: %f, %f, %f "
                       "(unbiased would be 1).",
                       rsum, gsum, bsum);

            if (rsum < 0 && gsum < 0 && bsum < 0)
                pm_message("Maybe you want the -nooffset option?");
        }
        break;

    default:
        if (gsum < 0.9 || gsum > 1.1)
            pm_message("WARNING - this convolution matrix is biased.  "
                       "average weight = %f (unbiased would be 1)",
                       gsum);
        break;
    }
}



/* General PGM Convolution
**
** No useful redundancy in convolution matrix.
*/

static void
pgm_general_convolve(const float ** const weights) {
    xel** xelbuf;
    xel* outputrow;
    xelval g;
    int row;
    xel **rowptr, *temprptr;
    int toprow, temprow;
    int i, irow;
    long tempgsum;

    /* Allocate space for one convolution-matrix's worth of rows, plus
       a row output buffer.
    */
    xelbuf = pnm_allocarray(cols, crows);
    outputrow = pnm_allocrow(cols);

    /* Allocate array of pointers to xelbuf */
    rowptr = (xel **) pnm_allocarray(1, crows);

    pnm_writepnminit(stdout, cols, rows, maxval, newformat, 0);

    /* Read in one convolution-matrix's worth of image, less one row. */
    for (row = 0; row < crows - 1; ++row) {
        pnm_readpnmrow(ifp, xelbuf[row], cols, maxval, format);
        if (PNM_FORMAT_TYPE(format) != newformat)
            pnm_promoteformatrow(xelbuf[row], cols, maxval, format, 
                                 maxval, newformat);
        /* Write out just the part we're not going to convolve. */
        if (row < crowso2)
            pnm_writepnmrow(stdout, xelbuf[row], cols, maxval, newformat, 0);
    }

    /* Now the rest of the image - read in the row at the end of
       xelbuf, and convolve and write out the row in the middle.
    */
    for (; row < rows; ++row) {
        int col;
        toprow = row + 1;
        temprow = row % crows;
        pnm_readpnmrow(ifp, xelbuf[temprow], cols, maxval, format);
        if (PNM_FORMAT_TYPE(format) != newformat)
            pnm_promoteformatrow(xelbuf[temprow], cols, maxval, format, 
                                 maxval, newformat);

        /* Arrange rowptr to eliminate the use of mod function to determine
           which row of xelbuf is 0...crows.  Mod function can be very costly.
        */
        temprow = toprow % crows;
        i = 0;
        for (irow = temprow; irow < crows; ++i, ++irow)
            rowptr[i] = xelbuf[irow];
        for (irow = 0; irow < temprow; ++irow, ++i)
            rowptr[i] = xelbuf[irow];

        for (col = 0; col < cols; ++col) {
            if (col < ccolso2 || col >= cols - ccolso2)
                outputrow[col] = rowptr[crowso2][col];
            else {
                int const leftcol = col - ccolso2;
                int crow;
                float gsum;
                gsum = 0.0;
                for (crow = 0; crow < crows; ++crow) {
                    int ccol;
                    temprptr = rowptr[crow] + leftcol;
                    for (ccol = 0; ccol < ccols; ++ccol)
                        gsum += PNM_GET1(*(temprptr + ccol))
                            * weights[crow][ccol];
                }
                tempgsum = gsum + 0.5;
            CHECK_GRAY;
            PNM_ASSIGN1( outputrow[col], g );
            }
        }
        pnm_writepnmrow(stdout, outputrow, cols, maxval, newformat, 0);
    }

    /* Now write out the remaining unconvolved rows in xelbuf. */
    for (irow = crowso2 + 1; irow < crows; ++irow)
        pnm_writepnmrow(stdout, rowptr[irow], cols, maxval, newformat, 0 );
}



/* PGM Mean Convolution
**
** This is the common case where you just want the target pixel replaced with
** the average value of its neighbors.  This can work much faster than the
** general case because you can reduce the number of floating point operations
** that are required since all the weights are the same.  You will only need
** to multiply by the weight once, not for every pixel in the convolution
** matrix.
**
** This algorithm works by creating sums for each column of crows height for
** the whole width of the image.  Then add ccols column sums together to obtain
** the total sum of the neighbors and multiply that sum by the weight.  As you
** move right to left to calculate the next pixel, take the total sum you just
** generated, add in the value of the next column and subtract the value of the
** leftmost column.  Multiply that by the weight and that's it.  As you move
** down a row, calculate new column sums by using previous sum for that column
** and adding in pixel on current row and subtracting pixel in top row.
**
*/


static void
pgm_mean_convolve(const float ** const weights) {
    float const gmeanweight = weights[0][0];

    int ccol, col;
    xel** xelbuf;
    xel* outputrow;
    xelval g;
    int row, crow;
    xel **rowptr, *temprptr;
    int leftcol;
    int i, irow;
    int toprow, temprow;
    int subrow, addrow;
    int subcol, addcol;
    long gisum;
    int tempcol, crowsp1;
    long tempgsum;
    long *gcolumnsum;

    /* Allocate space for one convolution-matrix's worth of rows, plus
    ** a row output buffer.  MEAN uses an extra row. */
    xelbuf = pnm_allocarray( cols, crows + 1 );
    outputrow = pnm_allocrow( cols );

    /* Allocate array of pointers to xelbuf. MEAN uses an extra row. */
    rowptr = (xel **) pnm_allocarray( 1, crows + 1);

    /* Allocate space for intermediate column sums */
    gcolumnsum = (long *) pm_allocrow( cols, sizeof(long) );
    for ( col = 0; col < cols; ++col )
    gcolumnsum[col] = 0L;

    pnm_writepnminit( stdout, cols, rows, maxval, newformat, 0 );

    /* Read in one convolution-matrix's worth of image, less one row. */
    for ( row = 0; row < crows - 1; ++row )
    {
    pnm_readpnmrow( ifp, xelbuf[row], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[row], cols, maxval, format, maxval, newformat );
    /* Write out just the part we're not going to convolve. */
    if ( row < crowso2 )
        pnm_writepnmrow( stdout, xelbuf[row], cols, maxval, newformat, 0 );
    }

    /* Do first real row only */
    subrow = crows;
    addrow = crows - 1;
    toprow = row + 1;
    temprow = row % crows;
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
    pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    temprow = toprow % crows;
    i = 0;
    for (irow = temprow; irow < crows; ++i, ++irow)
    rowptr[i] = xelbuf[irow];
    for (irow = 0; irow < temprow; ++irow, ++i)
    rowptr[i] = xelbuf[irow];

    gisum = 0L;
    for ( col = 0; col < cols; ++col )
    {
    if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
    else if ( col == ccolso2 )
        {
        leftcol = col - ccolso2;
        for ( crow = 0; crow < crows; ++crow )
        {
        temprptr = rowptr[crow] + leftcol;
        for ( ccol = 0; ccol < ccols; ++ccol )
            gcolumnsum[leftcol + ccol] += 
            PNM_GET1( *(temprptr + ccol) );
        }
        for ( ccol = 0; ccol < ccols; ++ccol)
        gisum += gcolumnsum[leftcol + ccol];
        tempgsum = (float) gisum * gmeanweight + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
    else
        {
        /* Column numbers to subtract or add to isum */
        subcol = col - ccolso2 - 1;
        addcol = col + ccolso2;  
        for ( crow = 0; crow < crows; ++crow )
        gcolumnsum[addcol] += PNM_GET1( rowptr[crow][addcol] );
        gisum = gisum - gcolumnsum[subcol] + gcolumnsum[addcol];
        tempgsum = (float) gisum * gmeanweight + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
    }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );

    ++row;
    /* For all subsequent rows do it this way as the columnsums have been
    ** generated.  Now we can use them to reduce further calculations.
    */
    crowsp1 = crows + 1;
    for ( ; row < rows; ++row )
    {
    toprow = row + 1;
    temprow = row % (crows + 1);
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    /* This rearrangement using crows+1 rowptrs and xelbufs will cause
    ** rowptr[0..crows-1] to always hold active xelbufs and for 
    ** rowptr[crows] to always hold the oldest (top most) xelbuf.
    */
    temprow = (toprow + 1) % crowsp1;
    i = 0;
    for (irow = temprow; irow < crowsp1; ++i, ++irow)
        rowptr[i] = xelbuf[irow];
    for (irow = 0; irow < temprow; ++irow, ++i)
        rowptr[i] = xelbuf[irow];

    gisum = 0L;
    for ( col = 0; col < cols; ++col )
        {
        if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
        else if ( col == ccolso2 )
        {
        leftcol = col - ccolso2;
        for ( ccol = 0; ccol < ccols; ++ccol )
            {
            tempcol = leftcol + ccol;
            gcolumnsum[tempcol] = gcolumnsum[tempcol]
            - PNM_GET1( rowptr[subrow][ccol] )
            + PNM_GET1( rowptr[addrow][ccol] );
            gisum += gcolumnsum[tempcol];
            }
        tempgsum = (float) gisum * gmeanweight + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
        else
        {
        /* Column numbers to subtract or add to isum */
        subcol = col - ccolso2 - 1;
        addcol = col + ccolso2;  
        gcolumnsum[addcol] = gcolumnsum[addcol]
            - PNM_GET1( rowptr[subrow][addcol] )
            + PNM_GET1( rowptr[addrow][addcol] );
        gisum = gisum - gcolumnsum[subcol] + gcolumnsum[addcol];
        tempgsum = (float) gisum * gmeanweight + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
        }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );
    }

    /* Now write out the remaining unconvolved rows in xelbuf. */
    for ( irow = crowso2 + 1; irow < crows; ++irow )
    pnm_writepnmrow(
            stdout, rowptr[irow], cols, maxval, newformat, 0 );

    }


/* PGM Horizontal Convolution
**
** Similar idea to using columnsums of the Mean and Vertical convolution,
** but uses temporary sums of row values.  Need to multiply by weights crows
** number of times.  Each time a new line is started, must recalculate the
** initials rowsums for the newest row only.  Uses queue to still access
** previous row sums.
**
*/

static void
pgm_horizontal_convolve(const float ** const weights) {
    int ccol, col;
    xel** xelbuf;
    xel* outputrow;
    xelval g;
    int row, crow;
    xel **rowptr, *temprptr;
    int leftcol;
    int i, irow;
    int temprow;
    int subcol, addcol;
    float gsum;
    int addrow, subrow;
    long **growsum, **growsumptr;
    int crowsp1;
    long tempgsum;

    /* Allocate space for one convolution-matrix's worth of rows, plus
    ** a row output buffer. */
    xelbuf = pnm_allocarray( cols, crows + 1 );
    outputrow = pnm_allocrow( cols );

    /* Allocate array of pointers to xelbuf */
    rowptr = (xel **) pnm_allocarray( 1, crows + 1);

    /* Allocate intermediate row sums.  HORIZONTAL uses an extra row. */
    /* crows current rows and 1 extra for newest added row.           */
    growsum = (long **) pm_allocarray( cols, crows + 1, sizeof(long) );
    growsumptr = (long **) pnm_allocarray( 1, crows + 1);

    pnm_writepnminit( stdout, cols, rows, maxval, newformat, 0 );

    /* Read in one convolution-matrix's worth of image, less one row. */
    for ( row = 0; row < crows - 1; ++row )
    {
    pnm_readpnmrow( ifp, xelbuf[row], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[row], cols, maxval, format, maxval, newformat );
    /* Write out just the part we're not going to convolve. */
    if ( row < crowso2 )
        pnm_writepnmrow( stdout, xelbuf[row], cols, maxval, newformat, 0 );
    }

    /* First row only */
    temprow = row % crows;
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
    pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    temprow = (row + 1) % crows;
    i = 0;
    for (irow = temprow; irow < crows; ++i, ++irow)
    rowptr[i] = xelbuf[irow];
    for (irow = 0; irow < temprow; ++irow, ++i)
    rowptr[i] = xelbuf[irow];

    for ( crow = 0; crow < crows; ++crow )
    growsumptr[crow] = growsum[crow];
 
    for ( col = 0; col < cols; ++col )
    {
    if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
    else if ( col == ccolso2 )
        {
        leftcol = col - ccolso2;
        gsum = 0.0;
        for ( crow = 0; crow < crows; ++crow )
        {
        temprptr = rowptr[crow] + leftcol;
        growsumptr[crow][leftcol] = 0L;
        for ( ccol = 0; ccol < ccols; ++ccol )
            growsumptr[crow][leftcol] += 
                PNM_GET1( *(temprptr + ccol) );
        gsum += growsumptr[crow][leftcol] * weights[crow][0];
        }
        tempgsum = gsum + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
    else
        {
        gsum = 0.0;
        leftcol = col - ccolso2;
        subcol = col - ccolso2 - 1;
        addcol = col + ccolso2;
        for ( crow = 0; crow < crows; ++crow )
        {
        growsumptr[crow][leftcol] = growsumptr[crow][subcol]
            - PNM_GET1( rowptr[crow][subcol] )
            + PNM_GET1( rowptr[crow][addcol] );
        gsum += growsumptr[crow][leftcol] * weights[crow][0];
        }
        tempgsum = gsum + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
        }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );


    /* For all subsequent rows */

    subrow = crows;
    addrow = crows - 1;
    crowsp1 = crows + 1;
    ++row;
    for ( ; row < rows; ++row )
    {
    temprow = row % crowsp1;
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    temprow = (row + 2) % crowsp1;
    i = 0;
    for (irow = temprow; irow < crowsp1; ++i, ++irow)
        {
        rowptr[i] = xelbuf[irow];
        growsumptr[i] = growsum[irow];
        }
    for (irow = 0; irow < temprow; ++irow, ++i)
        {
        rowptr[i] = xelbuf[irow];
        growsumptr[i] = growsum[irow];
        }

    for ( col = 0; col < cols; ++col )
        {
        if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
        else if ( col == ccolso2 )
        {
        gsum = 0.0;
        leftcol = col - ccolso2;
        growsumptr[addrow][leftcol] = 0L;
        for ( ccol = 0; ccol < ccols; ++ccol )
            growsumptr[addrow][leftcol] += 
            PNM_GET1( rowptr[addrow][leftcol + ccol] );
        for ( crow = 0; crow < crows; ++crow )
            gsum += growsumptr[crow][leftcol] * weights[crow][0];
        tempgsum = gsum + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
        else
        {
        gsum = 0.0;
        leftcol = col - ccolso2;
        subcol = col - ccolso2 - 1;
        addcol = col + ccolso2;  
        growsumptr[addrow][leftcol] = growsumptr[addrow][subcol]
            - PNM_GET1( rowptr[addrow][subcol] )
            + PNM_GET1( rowptr[addrow][addcol] );
        for ( crow = 0; crow < crows; ++crow )
            gsum += growsumptr[crow][leftcol] * weights[crow][0];
        tempgsum = gsum + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
        }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );
    }

    /* Now write out the remaining unconvolved rows in xelbuf. */
    for ( irow = crowso2 + 1; irow < crows; ++irow )
    pnm_writepnmrow(
            stdout, rowptr[irow], cols, maxval, newformat, 0 );

    }


/* PGM Vertical Convolution
**
** Uses column sums as in Mean Convolution.
**
*/


static void
pgm_vertical_convolve(const float ** const weights) {
    int ccol, col;
    xel** xelbuf;
    xel* outputrow;
    xelval g;
    int row, crow;
    xel **rowptr, *temprptr;
    int leftcol;
    int i, irow;
    int toprow, temprow;
    int subrow, addrow;
    int tempcol;
    float gsum;
    long *gcolumnsum;
    int crowsp1;
    int addcol;
    long tempgsum;

    /* Allocate space for one convolution-matrix's worth of rows, plus
    ** a row output buffer. VERTICAL uses an extra row. */
    xelbuf = pnm_allocarray( cols, crows + 1 );
    outputrow = pnm_allocrow( cols );

    /* Allocate array of pointers to xelbuf */
    rowptr = (xel **) pnm_allocarray( 1, crows + 1 );

    /* Allocate space for intermediate column sums */
    gcolumnsum = (long *) pm_allocrow( cols, sizeof(long) );
    for ( col = 0; col < cols; ++col )
    gcolumnsum[col] = 0L;

    pnm_writepnminit( stdout, cols, rows, maxval, newformat, 0 );

    /* Read in one convolution-matrix's worth of image, less one row. */
    for ( row = 0; row < crows - 1; ++row )
    {
    pnm_readpnmrow( ifp, xelbuf[row], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[row], cols, maxval, format, maxval, newformat );
    /* Write out just the part we're not going to convolve. */
    if ( row < crowso2 )
        pnm_writepnmrow( stdout, xelbuf[row], cols, maxval, newformat, 0 );
    }

    /* Now the rest of the image - read in the row at the end of
    ** xelbuf, and convolve and write out the row in the middle.
    */
    /* For first row only */

    toprow = row + 1;
    temprow = row % crows;
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
    pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    /* Arrange rowptr to eliminate the use of mod function to determine
    ** which row of xelbuf is 0...crows.  Mod function can be very costly.
    */
    temprow = toprow % crows;
    i = 0;
    for (irow = temprow; irow < crows; ++i, ++irow)
    rowptr[i] = xelbuf[irow];
    for (irow = 0; irow < temprow; ++irow, ++i)
    rowptr[i] = xelbuf[irow];

    for ( col = 0; col < cols; ++col )
    {
    if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
    else if ( col == ccolso2 )
        {
        gsum = 0.0;
        leftcol = col - ccolso2;
        for ( crow = 0; crow < crows; ++crow )
        {
        temprptr = rowptr[crow] + leftcol;
        for ( ccol = 0; ccol < ccols; ++ccol )
            gcolumnsum[leftcol + ccol] += 
            PNM_GET1( *(temprptr + ccol) );
        }
        for ( ccol = 0; ccol < ccols; ++ccol)
        gsum += gcolumnsum[leftcol + ccol] * weights[0][ccol];
        tempgsum = gsum + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
    else
        {
        gsum = 0.0;
        leftcol = col - ccolso2;
        addcol = col + ccolso2;  
        for ( crow = 0; crow < crows; ++crow )
        gcolumnsum[addcol] += PNM_GET1( rowptr[crow][addcol] );
        for ( ccol = 0; ccol < ccols; ++ccol )
        gsum += gcolumnsum[leftcol + ccol] * weights[0][ccol];
        tempgsum = gsum + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
    }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );

    /* For all subsequent rows */
    subrow = crows;
    addrow = crows - 1;
    crowsp1 = crows + 1;
    ++row;
    for ( ; row < rows; ++row )
    {
    toprow = row + 1;
    temprow = row % (crows +1);
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    /* Arrange rowptr to eliminate the use of mod function to determine
    ** which row of xelbuf is 0...crows.  Mod function can be very costly.
    */
    temprow = (toprow + 1) % crowsp1;
    i = 0;
    for (irow = temprow; irow < crowsp1; ++i, ++irow)
        rowptr[i] = xelbuf[irow];
    for (irow = 0; irow < temprow; ++irow, ++i)
        rowptr[i] = xelbuf[irow];

    for ( col = 0; col < cols; ++col )
        {
        if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
        else if ( col == ccolso2 )
        {
        gsum = 0.0;
        leftcol = col - ccolso2;
        for ( ccol = 0; ccol < ccols; ++ccol )
            {
            tempcol = leftcol + ccol;
            gcolumnsum[tempcol] = gcolumnsum[tempcol] 
            - PNM_GET1( rowptr[subrow][ccol] )
            + PNM_GET1( rowptr[addrow][ccol] );
            gsum = gsum + gcolumnsum[tempcol] * weights[0][ccol];
            }
        tempgsum = gsum + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
        else
        {
        gsum = 0.0;
        leftcol = col - ccolso2;
        addcol = col + ccolso2;
        gcolumnsum[addcol] = gcolumnsum[addcol]
            - PNM_GET1( rowptr[subrow][addcol] )
            + PNM_GET1( rowptr[addrow][addcol] );
        for ( ccol = 0; ccol < ccols; ++ccol )
            gsum += gcolumnsum[leftcol + ccol] * weights[0][ccol];
        tempgsum = gsum + 0.5;
        CHECK_GRAY;
        PNM_ASSIGN1( outputrow[col], g );
        }
        }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );
    }

    /* Now write out the remaining unconvolved rows in xelbuf. */
    for ( irow = crowso2 + 1; irow < crows; ++irow )
    pnm_writepnmrow(
            stdout, rowptr[irow], cols, maxval, newformat, 0 );

    }




/* PPM General Convolution Algorithm
**
** No redundancy in convolution matrix.  Just use brute force.
** See pgm_general_convolve() for more details.
*/

static void
ppm_general_convolve(const float ** const rweights,
                     const float ** const gweights,
                     const float ** const bweights) {
    int ccol, col;
    xel** xelbuf;
    xel* outputrow;
    xelval r, g, b;
    int row, crow;
    float rsum, gsum, bsum;
    xel **rowptr, *temprptr;
    int toprow, temprow;
    int i, irow;
    int leftcol;
    long temprsum, tempgsum, tempbsum;

    /* Allocate space for one convolution-matrix's worth of rows, plus
    ** a row output buffer. */
    xelbuf = pnm_allocarray( cols, crows );
    outputrow = pnm_allocrow( cols );

    /* Allocate array of pointers to xelbuf */
    rowptr = (xel **) pnm_allocarray( 1, crows );

    pnm_writepnminit( stdout, cols, rows, maxval, newformat, 0 );

    /* Read in one convolution-matrix's worth of image, less one row. */
    for ( row = 0; row < crows - 1; ++row )
    {
    pnm_readpnmrow( ifp, xelbuf[row], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[row], cols, maxval, format, maxval, newformat );
    /* Write out just the part we're not going to convolve. */
    if ( row < crowso2 )
        pnm_writepnmrow( stdout, xelbuf[row], cols, maxval, newformat, 0 );
    }

    /* Now the rest of the image - read in the row at the end of
    ** xelbuf, and convolve and write out the row in the middle.
    */
    for ( ; row < rows; ++row )
    {
    toprow = row + 1;
    temprow = row % crows;
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    /* Arrange rowptr to eliminate the use of mod function to determine
    ** which row of xelbuf is 0...crows.  Mod function can be very costly.
    */
    temprow = toprow % crows;
    i = 0;
    for (irow = temprow; irow < crows; ++i, ++irow)
        rowptr[i] = xelbuf[irow];
    for (irow = 0; irow < temprow; ++irow, ++i)
        rowptr[i] = xelbuf[irow];

    for ( col = 0; col < cols; ++col )
        {
        if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
        else
        {
        leftcol = col - ccolso2;
        rsum = gsum = bsum = 0.0;
        for ( crow = 0; crow < crows; ++crow )
            {
            temprptr = rowptr[crow] + leftcol;
            for ( ccol = 0; ccol < ccols; ++ccol )
            {
            rsum += PPM_GETR( *(temprptr + ccol) )
                * rweights[crow][ccol];
            gsum += PPM_GETG( *(temprptr + ccol) )
                * gweights[crow][ccol];
            bsum += PPM_GETB( *(temprptr + ccol) )
                * bweights[crow][ccol];
            }
            }
            temprsum = rsum + 0.5;
            tempgsum = gsum + 0.5;
            tempbsum = bsum + 0.5;
            CHECK_RED;
            CHECK_GREEN;
            CHECK_BLUE;
            PPM_ASSIGN( outputrow[col], r, g, b );
        }
        }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );
    }

    /* Now write out the remaining unconvolved rows in xelbuf. */
    for ( irow = crowso2 + 1; irow < crows; ++irow )
    pnm_writepnmrow(
            stdout, rowptr[irow], cols, maxval, newformat, 0 );

    }


/* PPM Mean Convolution
**
** Same as pgm_mean_convolve() but for PPM.
**
*/

static void
ppm_mean_convolve(const float ** const rweights,
                  const float ** const gweights,
                  const float ** const bweights) {
    /* All weights of a single color are the same so just grab any one
       of them.  
    */
    float const rmeanweight = rweights[0][0];
    float const gmeanweight = gweights[0][0];
    float const bmeanweight = bweights[0][0];

    int ccol, col;
    xel** xelbuf;
    xel* outputrow;
    xelval r, g, b;
    int row, crow;
    xel **rowptr, *temprptr;
    int leftcol;
    int i, irow;
    int toprow, temprow;
    int subrow, addrow;
    int subcol, addcol;
    long risum, gisum, bisum;
    long temprsum, tempgsum, tempbsum;
    int tempcol, crowsp1;
    long *rcolumnsum, *gcolumnsum, *bcolumnsum;



    /* Allocate space for one convolution-matrix's worth of rows, plus
    ** a row output buffer.  MEAN uses an extra row. */
    xelbuf = pnm_allocarray( cols, crows + 1 );
    outputrow = pnm_allocrow( cols );

    /* Allocate array of pointers to xelbuf. MEAN uses an extra row. */
    rowptr = (xel **) pnm_allocarray( 1, crows + 1);

    /* Allocate space for intermediate column sums */
    rcolumnsum = (long *) pm_allocrow( cols, sizeof(long) );
    gcolumnsum = (long *) pm_allocrow( cols, sizeof(long) );
    bcolumnsum = (long *) pm_allocrow( cols, sizeof(long) );
    for ( col = 0; col < cols; ++col )
    {
    rcolumnsum[col] = 0L;
    gcolumnsum[col] = 0L;
    bcolumnsum[col] = 0L;
    }

    pnm_writepnminit( stdout, cols, rows, maxval, newformat, 0 );

    /* Read in one convolution-matrix's worth of image, less one row. */
    for ( row = 0; row < crows - 1; ++row )
    {
    pnm_readpnmrow( ifp, xelbuf[row], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[row], cols, maxval, format, maxval, newformat );
    /* Write out just the part we're not going to convolve. */
    if ( row < crowso2 )
        pnm_writepnmrow( stdout, xelbuf[row], cols, maxval, newformat, 0 );
    }

    /* Do first real row only */
    subrow = crows;
    addrow = crows - 1;
    toprow = row + 1;
    temprow = row % crows;
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
    pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    temprow = toprow % crows;
    i = 0;
    for (irow = temprow; irow < crows; ++i, ++irow)
    rowptr[i] = xelbuf[irow];
    for (irow = 0; irow < temprow; ++irow, ++i)
    rowptr[i] = xelbuf[irow];

    risum = 0L;
    gisum = 0L;
    bisum = 0L;
    for ( col = 0; col < cols; ++col )
    {
    if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
    else if ( col == ccolso2 )
        {
        leftcol = col - ccolso2;
        for ( crow = 0; crow < crows; ++crow )
        {
        temprptr = rowptr[crow] + leftcol;
        for ( ccol = 0; ccol < ccols; ++ccol )
            {
            rcolumnsum[leftcol + ccol] += 
            PPM_GETR( *(temprptr + ccol) );
            gcolumnsum[leftcol + ccol] += 
            PPM_GETG( *(temprptr + ccol) );
            bcolumnsum[leftcol + ccol] += 
            PPM_GETB( *(temprptr + ccol) );
            }
        }
        for ( ccol = 0; ccol < ccols; ++ccol)
        {
        risum += rcolumnsum[leftcol + ccol];
        gisum += gcolumnsum[leftcol + ccol];
        bisum += bcolumnsum[leftcol + ccol];
        }
        temprsum = (float) risum * rmeanweight + 0.5;
        tempgsum = (float) gisum * gmeanweight + 0.5;
        tempbsum = (float) bisum * bmeanweight + 0.5;
        CHECK_RED;
        CHECK_GREEN;
        CHECK_BLUE;
        PPM_ASSIGN( outputrow[col], r, g, b );
        }
    else
        {
        /* Column numbers to subtract or add to isum */
        subcol = col - ccolso2 - 1;
        addcol = col + ccolso2;  
        for ( crow = 0; crow < crows; ++crow )
        {
        rcolumnsum[addcol] += PPM_GETR( rowptr[crow][addcol] );
        gcolumnsum[addcol] += PPM_GETG( rowptr[crow][addcol] );
        bcolumnsum[addcol] += PPM_GETB( rowptr[crow][addcol] );
        }
        risum = risum - rcolumnsum[subcol] + rcolumnsum[addcol];
        gisum = gisum - gcolumnsum[subcol] + gcolumnsum[addcol];
        bisum = bisum - bcolumnsum[subcol] + bcolumnsum[addcol];
        temprsum = (float) risum * rmeanweight + 0.5;
        tempgsum = (float) gisum * gmeanweight + 0.5;
        tempbsum = (float) bisum * bmeanweight + 0.5;
        CHECK_RED;
        CHECK_GREEN;
        CHECK_BLUE;
        PPM_ASSIGN( outputrow[col], r, g, b );
        }
    }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );

    ++row;
    /* For all subsequent rows do it this way as the columnsums have been
    ** generated.  Now we can use them to reduce further calculations.
    */
    crowsp1 = crows + 1;
    for ( ; row < rows; ++row )
    {
    toprow = row + 1;
    temprow = row % (crows + 1);
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    /* This rearrangement using crows+1 rowptrs and xelbufs will cause
    ** rowptr[0..crows-1] to always hold active xelbufs and for 
    ** rowptr[crows] to always hold the oldest (top most) xelbuf.
    */
    temprow = (toprow + 1) % crowsp1;
    i = 0;
    for (irow = temprow; irow < crowsp1; ++i, ++irow)
        rowptr[i] = xelbuf[irow];
    for (irow = 0; irow < temprow; ++irow, ++i)
        rowptr[i] = xelbuf[irow];

    risum = 0L;
    gisum = 0L;
    bisum = 0L;
    for ( col = 0; col < cols; ++col )
        {
        if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
        else if ( col == ccolso2 )
        {
        leftcol = col - ccolso2;
        for ( ccol = 0; ccol < ccols; ++ccol )
            {
            tempcol = leftcol + ccol;
            rcolumnsum[tempcol] = rcolumnsum[tempcol]
            - PPM_GETR( rowptr[subrow][ccol] )
            + PPM_GETR( rowptr[addrow][ccol] );
            risum += rcolumnsum[tempcol];
            gcolumnsum[tempcol] = gcolumnsum[tempcol]
            - PPM_GETG( rowptr[subrow][ccol] )
            + PPM_GETG( rowptr[addrow][ccol] );
            gisum += gcolumnsum[tempcol];
            bcolumnsum[tempcol] = bcolumnsum[tempcol]
            - PPM_GETB( rowptr[subrow][ccol] )
            + PPM_GETB( rowptr[addrow][ccol] );
            bisum += bcolumnsum[tempcol];
            }
        temprsum = (float) risum * rmeanweight + 0.5;
        tempgsum = (float) gisum * gmeanweight + 0.5;
        tempbsum = (float) bisum * bmeanweight + 0.5;
        CHECK_RED;
        CHECK_GREEN;
        CHECK_BLUE;
        PPM_ASSIGN( outputrow[col], r, g, b );
        }
        else
        {
        /* Column numbers to subtract or add to isum */
        subcol = col - ccolso2 - 1;
        addcol = col + ccolso2;  
        rcolumnsum[addcol] = rcolumnsum[addcol]
            - PPM_GETR( rowptr[subrow][addcol] )
            + PPM_GETR( rowptr[addrow][addcol] );
        risum = risum - rcolumnsum[subcol] + rcolumnsum[addcol];
        gcolumnsum[addcol] = gcolumnsum[addcol]
            - PPM_GETG( rowptr[subrow][addcol] )
            + PPM_GETG( rowptr[addrow][addcol] );
        gisum = gisum - gcolumnsum[subcol] + gcolumnsum[addcol];
        bcolumnsum[addcol] = bcolumnsum[addcol]
            - PPM_GETB( rowptr[subrow][addcol] )
            + PPM_GETB( rowptr[addrow][addcol] );
        bisum = bisum - bcolumnsum[subcol] + bcolumnsum[addcol];
        temprsum = (float) risum * rmeanweight + 0.5;
        tempgsum = (float) gisum * gmeanweight + 0.5;
        tempbsum = (float) bisum * bmeanweight + 0.5;
        CHECK_RED;
        CHECK_GREEN;
        CHECK_BLUE;
        PPM_ASSIGN( outputrow[col], r, g, b );
        }
        }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );
    }

    /* Now write out the remaining unconvolved rows in xelbuf. */
    for ( irow = crowso2 + 1; irow < crows; ++irow )
    pnm_writepnmrow(
            stdout, rowptr[irow], cols, maxval, newformat, 0 );

    }


/* PPM Horizontal Convolution
**
** Same as pgm_horizontal_convolve()
**
**/

static void
ppm_horizontal_convolve(const float ** const rweights,
                        const float ** const gweights,
                        const float ** const bweights) {
    int ccol, col;
    xel** xelbuf;
    xel* outputrow;
    xelval r, g, b;
    int row, crow;
    xel **rowptr, *temprptr;
    int leftcol;
    int i, irow;
    int temprow;
    int subcol, addcol;
    float rsum, gsum, bsum;
    int addrow, subrow;
    long **rrowsum, **rrowsumptr;
    long **growsum, **growsumptr;
    long **browsum, **browsumptr;
    int crowsp1;
    long temprsum, tempgsum, tempbsum;

    /* Allocate space for one convolution-matrix's worth of rows, plus
    ** a row output buffer. */
    xelbuf = pnm_allocarray( cols, crows + 1 );
    outputrow = pnm_allocrow( cols );

    /* Allocate array of pointers to xelbuf */
    rowptr = (xel **) pnm_allocarray( 1, crows + 1);

    /* Allocate intermediate row sums.  HORIZONTAL uses an extra row */
    rrowsum = (long **) pm_allocarray( cols, crows + 1, sizeof(long) );
    rrowsumptr = (long **) pnm_allocarray( 1, crows + 1);
    growsum = (long **) pm_allocarray( cols, crows + 1, sizeof(long) );
    growsumptr = (long **) pnm_allocarray( 1, crows + 1);
    browsum = (long **) pm_allocarray( cols, crows + 1, sizeof(long) );
    browsumptr = (long **) pnm_allocarray( 1, crows + 1);

    pnm_writepnminit( stdout, cols, rows, maxval, newformat, 0 );

    /* Read in one convolution-matrix's worth of image, less one row. */
    for ( row = 0; row < crows - 1; ++row )
    {
    pnm_readpnmrow( ifp, xelbuf[row], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[row], cols, maxval, format, maxval, newformat );
    /* Write out just the part we're not going to convolve. */
    if ( row < crowso2 )
        pnm_writepnmrow( stdout, xelbuf[row], cols, maxval, newformat, 0 );
    }

    /* First row only */
    temprow = row % crows;
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
    pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    temprow = (row + 1) % crows;
    i = 0;
    for (irow = temprow; irow < crows; ++i, ++irow)
    rowptr[i] = xelbuf[irow];
    for (irow = 0; irow < temprow; ++irow, ++i)
    rowptr[i] = xelbuf[irow];

    for ( crow = 0; crow < crows; ++crow )
    {
    rrowsumptr[crow] = rrowsum[crow];
    growsumptr[crow] = growsum[crow];
    browsumptr[crow] = browsum[crow];
    }
 
    for ( col = 0; col < cols; ++col )
    {
    if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
    else if ( col == ccolso2 )
        {
        leftcol = col - ccolso2;
        rsum = 0.0;
        gsum = 0.0;
        bsum = 0.0;
        for ( crow = 0; crow < crows; ++crow )
        {
        temprptr = rowptr[crow] + leftcol;
        rrowsumptr[crow][leftcol] = 0L;
        growsumptr[crow][leftcol] = 0L;
        browsumptr[crow][leftcol] = 0L;
        for ( ccol = 0; ccol < ccols; ++ccol )
            {
            rrowsumptr[crow][leftcol] += 
                PPM_GETR( *(temprptr + ccol) );
            growsumptr[crow][leftcol] += 
                PPM_GETG( *(temprptr + ccol) );
            browsumptr[crow][leftcol] += 
                PPM_GETB( *(temprptr + ccol) );
            }
        rsum += rrowsumptr[crow][leftcol] * rweights[crow][0];
        gsum += growsumptr[crow][leftcol] * gweights[crow][0];
        bsum += browsumptr[crow][leftcol] * bweights[crow][0];
        }
        temprsum = rsum + 0.5;
        tempgsum = gsum + 0.5;
        tempbsum = bsum + 0.5;
        CHECK_RED;
        CHECK_GREEN;
        CHECK_BLUE;
        PPM_ASSIGN( outputrow[col], r, g, b );
        }
    else
        {
        rsum = 0.0;
        gsum = 0.0;
        bsum = 0.0;
        leftcol = col - ccolso2;
        subcol = col - ccolso2 - 1;
        addcol = col + ccolso2;
        for ( crow = 0; crow < crows; ++crow )
        {
        rrowsumptr[crow][leftcol] = rrowsumptr[crow][subcol]
            - PPM_GETR( rowptr[crow][subcol] )
            + PPM_GETR( rowptr[crow][addcol] );
        rsum += rrowsumptr[crow][leftcol] * rweights[crow][0];
        growsumptr[crow][leftcol] = growsumptr[crow][subcol]
            - PPM_GETG( rowptr[crow][subcol] )
            + PPM_GETG( rowptr[crow][addcol] );
        gsum += growsumptr[crow][leftcol] * gweights[crow][0];
        browsumptr[crow][leftcol] = browsumptr[crow][subcol]
            - PPM_GETB( rowptr[crow][subcol] )
            + PPM_GETB( rowptr[crow][addcol] );
        bsum += browsumptr[crow][leftcol] * bweights[crow][0];
        }
        temprsum = rsum + 0.5;
        tempgsum = gsum + 0.5;
        tempbsum = bsum + 0.5;
        CHECK_RED;
        CHECK_GREEN;
        CHECK_BLUE;
        PPM_ASSIGN( outputrow[col], r, g, b );
        }
        }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );


    /* For all subsequent rows */

    subrow = crows;
    addrow = crows - 1;
    crowsp1 = crows + 1;
    ++row;
    for ( ; row < rows; ++row )
    {
    temprow = row % crowsp1;
    pnm_readpnmrow( ifp, xelbuf[temprow], cols, maxval, format );
    if ( PNM_FORMAT_TYPE(format) != newformat )
        pnm_promoteformatrow(
        xelbuf[temprow], cols, maxval, format, maxval, newformat );

    temprow = (row + 2) % crowsp1;
    i = 0;
    for (irow = temprow; irow < crowsp1; ++i, ++irow)
        {
        rowptr[i] = xelbuf[irow];
        rrowsumptr[i] = rrowsum[irow];
        growsumptr[i] = growsum[irow];
        browsumptr[i] = browsum[irow];
        }
    for (irow = 0; irow < temprow; ++irow, ++i)
        {
        rowptr[i] = xelbuf[irow];
        rrowsumptr[i] = rrowsum[irow];
        growsumptr[i] = growsum[irow];
        browsumptr[i] = browsum[irow];
        }

    for ( col = 0; col < cols; ++col )
        {
        if ( col < ccolso2 || col >= cols - ccolso2 )
        outputrow[col] = rowptr[crowso2][col];
        else if ( col == ccolso2 )
        {
        rsum = 0.0;
        gsum = 0.0;
        bsum = 0.0;
        leftcol = col - ccolso2;
        rrowsumptr[addrow][leftcol] = 0L;
        growsumptr[addrow][leftcol] = 0L;
        browsumptr[addrow][leftcol] = 0L;
        for ( ccol = 0; ccol < ccols; ++ccol )
            {
            rrowsumptr[addrow][leftcol] += 
            PPM_GETR( rowptr[addrow][leftcol + ccol] );
            growsumptr[addrow][leftcol] += 
            PPM_GETG( rowptr[addrow][leftcol + ccol] );
            browsumptr[addrow][leftcol] += 
            PPM_GETB( rowptr[addrow][leftcol + ccol] );
            }
        for ( crow = 0; crow < crows; ++crow )
            {
            rsum += rrowsumptr[crow][leftcol] * rweights[crow][0];
            gsum += growsumptr[crow][leftcol] * gweights[crow][0];
            bsum += browsumptr[crow][leftcol] * bweights[crow][0];
            }
        temprsum = rsum + 0.5;
        tempgsum = gsum + 0.5;
        tempbsum = bsum + 0.5;
        CHECK_RED;
        CHECK_GREEN;
        CHECK_BLUE;
        PPM_ASSIGN( outputrow[col], r, g, b );
        }
        else
        {
        rsum = 0.0;
        gsum = 0.0;
        bsum = 0.0;
        leftcol = col - ccolso2;
        subcol = col - ccolso2 - 1;
        addcol = col + ccolso2;  
        rrowsumptr[addrow][leftcol] = rrowsumptr[addrow][subcol]
            - PPM_GETR( rowptr[addrow][subcol] )
            + PPM_GETR( rowptr[addrow][addcol] );
        growsumptr[addrow][leftcol] = growsumptr[addrow][subcol]
            - PPM_GETG( rowptr[addrow][subcol] )
            + PPM_GETG( rowptr[addrow][addcol] );
        browsumptr[addrow][leftcol] = browsumptr[addrow][subcol]
            - PPM_GETB( rowptr[addrow][subcol] )
            + PPM_GETB( rowptr[addrow][addcol] );
        for ( crow = 0; crow < crows; ++crow )
            {
            rsum += rrowsumptr[crow][leftcol] * rweights[crow][0];
            gsum += growsumptr[crow][leftcol] * gweights[crow][0];
            bsum += browsumptr[crow][leftcol] * bweights[crow][0];
            }
        temprsum = rsum + 0.5;
        tempgsum = gsum + 0.5;
        tempbsum = bsum + 0.5;
        CHECK_RED;
        CHECK_GREEN;
        CHECK_BLUE;
        PPM_ASSIGN( outputrow[col], r, g, b );
        }
        }
    pnm_writepnmrow( stdout, outputrow, cols, maxval, newformat, 0 );
    }

    /* Now write out the remaining unconvolved rows in xelbuf. */
    for ( irow = crowso2 + 1; irow < crows; ++irow )
    pnm_writepnmrow(
            stdout, rowptr[irow], cols, maxval, newformat, 0 );

    }


/* PPM Vertical Convolution
**
** Same as pgm_vertical_convolve()
**
*/

static void
ppm_vertical_convolve(const float ** const rweights,
                      const float ** const gweights,
                      const float ** const bweights) {
    int ccol, col;
    xel** xelbuf;
    xel* outputrow;
    xelval r, g, b;
    int row, crow;
    xel **rowptr, *temprptr;
    int i, irow;
    int toprow, temprow;
    int subrow, addrow;
    int tempcol;
    long *rcolumnsum, *gcolumnsum, *bcolumnsum;
    int crowsp1;
    int addcol;
    long temprsum, tempgsum, tempbsum;

    /* Allocate space for one convolution-matrix's worth of rows, plus
    ** a row output buffer. VERTICAL uses an extra row. */
    xelbuf = pnm_allocarray(cols, crows + 1);
    outputrow = pnm_allocrow(cols);

    /* Allocate array of pointers to xelbuf */
    rowptr = (xel **) pnm_allocarray(1, crows + 1);

    /* Allocate space for intermediate column sums */
    MALLOCARRAY_NOFAIL(rcolumnsum, cols);
    MALLOCARRAY_NOFAIL(gcolumnsum, cols);
    MALLOCARRAY_NOFAIL(bcolumnsum, cols);

    for (col = 0; col < cols; ++col) {
        rcolumnsum[col] = 0L;
        gcolumnsum[col] = 0L;
        bcolumnsum[col] = 0L;
    }

    pnm_writepnminit(stdout, cols, rows, maxval, newformat, 0);

    /* Read in one convolution-matrix's worth of image, less one row. */
    for (row = 0; row < crows - 1; ++row) {
        pnm_readpnmrow(ifp, xelbuf[row], cols, maxval, format);
        if (PNM_FORMAT_TYPE(format) != newformat)
            pnm_promoteformatrow(xelbuf[row], cols, maxval, format, 
                                 maxval, newformat);
        /* Write out just the part we're not going to convolve. */
        if (row < crowso2)
            pnm_writepnmrow(stdout, xelbuf[row], cols, maxval, newformat, 0);
    }

    /* Now the rest of the image - read in the row at the end of
    ** xelbuf, and convolve and write out the row in the middle.
    */
    /* For first row only */

    toprow = row + 1;
    temprow = row % crows;
    pnm_readpnmrow(ifp, xelbuf[temprow], cols, maxval, format);
    if (PNM_FORMAT_TYPE(format) != newformat)
        pnm_promoteformatrow(xelbuf[temprow], cols, maxval, format, maxval, 
                             newformat);

    /* Arrange rowptr to eliminate the use of mod function to determine
    ** which row of xelbuf is 0...crows.  Mod function can be very costly.
    */
    temprow = toprow % crows;
    i = 0;
    for (irow = temprow; irow < crows; ++i, ++irow)
        rowptr[i] = xelbuf[irow];
    for (irow = 0; irow < temprow; ++irow, ++i)
        rowptr[i] = xelbuf[irow];

    for (col = 0; col < cols; ++col) {
        if (col < ccolso2 || col >= cols - ccolso2)
            outputrow[col] = rowptr[crowso2][col];
        else if (col == ccolso2) {
            int const leftcol = col - ccolso2;
            float rsum, gsum, bsum;
            rsum = 0.0;
            gsum = 0.0;
            bsum = 0.0;
            for (crow = 0; crow < crows; ++crow) {
                temprptr = rowptr[crow] + leftcol;
                for (ccol = 0; ccol < ccols; ++ccol) {
                    rcolumnsum[leftcol + ccol] += 
                        PPM_GETR(*(temprptr + ccol));
                    gcolumnsum[leftcol + ccol] += 
                        PPM_GETG(*(temprptr + ccol));
                    bcolumnsum[leftcol + ccol] += 
                        PPM_GETB(*(temprptr + ccol));
                }
            }
            for (ccol = 0; ccol < ccols; ++ccol) {
                rsum += rcolumnsum[leftcol + ccol] * rweights[0][ccol];
                gsum += gcolumnsum[leftcol + ccol] * gweights[0][ccol];
                bsum += bcolumnsum[leftcol + ccol] * bweights[0][ccol];
            }
            temprsum = rsum + 0.5;
            tempgsum = gsum + 0.5;
            tempbsum = bsum + 0.5;
            CHECK_RED;
            CHECK_GREEN;
            CHECK_BLUE;
            PPM_ASSIGN(outputrow[col], r, g, b);
        } else {
            int const leftcol = col - ccolso2;
            float rsum, gsum, bsum;
            rsum = 0.0;
            gsum = 0.0;
            bsum = 0.0;
            addcol = col + ccolso2;  
            for (crow = 0; crow < crows; ++crow) {
                rcolumnsum[addcol] += PPM_GETR( rowptr[crow][addcol]);
                gcolumnsum[addcol] += PPM_GETG( rowptr[crow][addcol]);
                bcolumnsum[addcol] += PPM_GETB( rowptr[crow][addcol]);
            }
            for (ccol = 0; ccol < ccols; ++ccol) {
                rsum += rcolumnsum[leftcol + ccol] * rweights[0][ccol];
                gsum += gcolumnsum[leftcol + ccol] * gweights[0][ccol];
                bsum += bcolumnsum[leftcol + ccol] * bweights[0][ccol];
            }
            temprsum = rsum + 0.5;
            tempgsum = gsum + 0.5;
            tempbsum = bsum + 0.5;
            CHECK_RED;
            CHECK_GREEN;
            CHECK_BLUE;
            PPM_ASSIGN(outputrow[col], r, g, b);
        }
    }
    pnm_writepnmrow(stdout, outputrow, cols, maxval, newformat, 0);
    
    /* For all subsequent rows */
    subrow = crows;
    addrow = crows - 1;
    crowsp1 = crows + 1;
    ++row;
    for (; row < rows; ++row) {
        toprow = row + 1;
        temprow = row % (crows +1);
        pnm_readpnmrow(ifp, xelbuf[temprow], cols, maxval, format);
        if (PNM_FORMAT_TYPE(format) != newformat)
            pnm_promoteformatrow(xelbuf[temprow], cols, maxval, format, 
                                 maxval, newformat);

        /* Arrange rowptr to eliminate the use of mod function to determine
        ** which row of xelbuf is 0...crows.  Mod function can be very costly.
        */
        temprow = (toprow + 1) % crowsp1;
        i = 0;
        for (irow = temprow; irow < crowsp1; ++i, ++irow)
            rowptr[i] = xelbuf[irow];
        for (irow = 0; irow < temprow; ++irow, ++i)
            rowptr[i] = xelbuf[irow];

        for (col = 0; col < cols; ++col) {
            if (col < ccolso2 || col >= cols - ccolso2)
                outputrow[col] = rowptr[crowso2][col];
            else if (col == ccolso2) {
                int const leftcol = col - ccolso2;
                float rsum, gsum, bsum;
                rsum = 0.0;
                gsum = 0.0;
                bsum = 0.0;

                for (ccol = 0; ccol < ccols; ++ccol) {
                    tempcol = leftcol + ccol;
                    rcolumnsum[tempcol] = rcolumnsum[tempcol] 
                        - PPM_GETR(rowptr[subrow][ccol])
                        + PPM_GETR(rowptr[addrow][ccol]);
                    rsum = rsum + rcolumnsum[tempcol] * rweights[0][ccol];
                    gcolumnsum[tempcol] = gcolumnsum[tempcol] 
                        - PPM_GETG(rowptr[subrow][ccol])
                        + PPM_GETG(rowptr[addrow][ccol]);
                    gsum = gsum + gcolumnsum[tempcol] * gweights[0][ccol];
                    bcolumnsum[tempcol] = bcolumnsum[tempcol] 
                        - PPM_GETB(rowptr[subrow][ccol])
                        + PPM_GETB(rowptr[addrow][ccol]);
                    bsum = bsum + bcolumnsum[tempcol] * bweights[0][ccol];
                }
                temprsum = rsum + 0.5;
                tempgsum = gsum + 0.5;
                tempbsum = bsum + 0.5;
                CHECK_RED;
                CHECK_GREEN;
                CHECK_BLUE;
                PPM_ASSIGN(outputrow[col], r, g, b);
            } else {
                int const leftcol = col - ccolso2;
                float rsum, gsum, bsum;
                rsum = 0.0;
                gsum = 0.0;
                bsum = 0.0;
                addcol = col + ccolso2;
                rcolumnsum[addcol] = rcolumnsum[addcol]
                    - PPM_GETR(rowptr[subrow][addcol])
                    + PPM_GETR(rowptr[addrow][addcol]);
                gcolumnsum[addcol] = gcolumnsum[addcol]
                    - PPM_GETG(rowptr[subrow][addcol])
                    + PPM_GETG(rowptr[addrow][addcol]);
                bcolumnsum[addcol] = bcolumnsum[addcol]
                    - PPM_GETB(rowptr[subrow][addcol])
                    + PPM_GETB(rowptr[addrow][addcol]);
                for (ccol = 0; ccol < ccols; ++ccol) {
                    rsum += rcolumnsum[leftcol + ccol] * rweights[0][ccol];
                    gsum += gcolumnsum[leftcol + ccol] * gweights[0][ccol];
                    bsum += bcolumnsum[leftcol + ccol] * bweights[0][ccol];
                }
                temprsum = rsum + 0.5;
                tempgsum = gsum + 0.5;
                tempbsum = bsum + 0.5;
                CHECK_RED;
                CHECK_GREEN;
                CHECK_BLUE;
                PPM_ASSIGN(outputrow[col], r, g, b);
            }
        }
        pnm_writepnmrow(stdout, outputrow, cols, maxval, newformat, 0);
    }

    /* Now write out the remaining unconvolved rows in xelbuf. */
    for (irow = crowso2 + 1; irow < crows; ++irow)
        pnm_writepnmrow(stdout, rowptr[irow], cols, maxval, newformat, 0);

}



static void
determineConvolveType(xel * const *         const cxels,
                      struct convolveType * const typeP) {
/*----------------------------------------------------------------------------
   Determine which form of convolution is best.  The general form always
   works, but with some special case convolution matrices, faster forms
   of convolution are possible.

   We don't check for the case that one of the PPM colors can have 
   differing types.  We handle only cases where all PPMs are of the same
   special case.
-----------------------------------------------------------------------------*/
    int horizontal, vertical;
    int tempcxel, rtempcxel, gtempcxel, btempcxel;
    int crow, ccol;

    switch (PNM_FORMAT_TYPE(format)) {
    case PPM_TYPE:
        horizontal = TRUE;  /* initial assumption */
        crow = 0;
        while (horizontal && (crow < crows)) {
            ccol = 1;
            rtempcxel = PPM_GETR(cxels[crow][0]);
            gtempcxel = PPM_GETG(cxels[crow][0]);
            btempcxel = PPM_GETB(cxels[crow][0]);
            while (horizontal && (ccol < ccols)) {
                if ((PPM_GETR(cxels[crow][ccol]) != rtempcxel) ||
                    (PPM_GETG(cxels[crow][ccol]) != gtempcxel) ||
                    (PPM_GETB(cxels[crow][ccol]) != btempcxel)) 
                    horizontal = FALSE;
                ++ccol;
            }
            ++crow;
        }

        vertical = TRUE;   /* initial assumption */
        ccol = 0;
        while (vertical && (ccol < ccols)) {
            crow = 1;
            rtempcxel = PPM_GETR(cxels[0][ccol]);
            gtempcxel = PPM_GETG(cxels[0][ccol]);
            btempcxel = PPM_GETB(cxels[0][ccol]);
            while (vertical && (crow < crows)) {
                if ((PPM_GETR(cxels[crow][ccol]) != rtempcxel) |
                    (PPM_GETG(cxels[crow][ccol]) != gtempcxel) |
                    (PPM_GETB(cxels[crow][ccol]) != btempcxel))
                    vertical = FALSE;
                ++crow;
            }
            ++ccol;
        }
        break;
        
    default:
        horizontal = TRUE; /* initial assumption */
        crow = 0;
        while (horizontal && (crow < crows)) {
            ccol = 1;
            tempcxel = PNM_GET1(cxels[crow][0]);
            while (horizontal && (ccol < ccols)) {
                if (PNM_GET1(cxels[crow][ccol]) != tempcxel)
                    horizontal = FALSE;
                ++ccol;
            }
            ++crow;
        }
        
        vertical = TRUE;  /* initial assumption */
        ccol = 0;
        while (vertical && (ccol < ccols)) {
            crow = 1;
            tempcxel = PNM_GET1(cxels[0][ccol]);
            while (vertical && (crow < crows)) {
                if (PNM_GET1(cxels[crow][ccol]) != tempcxel)
                    vertical = FALSE;
                ++crow;
            }
            ++ccol;
        }
        break;
    }
    
    /* Which type do we have? */
    if (horizontal && vertical) {
        typeP->ppmConvolver = ppm_mean_convolve;
        typeP->pgmConvolver = pgm_mean_convolve;
    } else if (horizontal) {
        typeP->ppmConvolver = ppm_horizontal_convolve;
        typeP->pgmConvolver = pgm_horizontal_convolve;
    } else if (vertical) {
        typeP->ppmConvolver = ppm_vertical_convolve;
        typeP->pgmConvolver = pgm_vertical_convolve;
    } else {
        typeP->ppmConvolver = ppm_general_convolve;
        typeP->pgmConvolver = pgm_general_convolve;
    }
}



static void
convolveIt(int                 const format,
           struct convolveType const convolveType,
           const float**       const rweights,
           const float**       const gweights,
           const float**       const bweights) {

    switch (PNM_FORMAT_TYPE(format)) {
    case PPM_TYPE:
        convolveType.ppmConvolver(rweights, gweights, bweights);
        break;

    default:
        convolveType.pgmConvolver(gweights);
    }
}



static void
readKernel(const char * const fileName,
           int *        const colsP,
           int *        const rowsP,
           xelval *     const maxvalP,
           int *        const formatP,
           xel ***      const xelsP) {
/*----------------------------------------------------------------------------
   Read in the pseudo-PNM that is the convolution matrix.

   This is essentially pnm_readpnm(), except that it can take sample values
   that exceed the maxval, which is not legal in PNM.  That's why it's
   psuedo-PNM and not true PNM.
-----------------------------------------------------------------------------*/

    /* pm_getuint() is supposed to be internal to libnetpbm, but since we're
       doing this backward compatibility hack here, we use it anyway.
    */

    unsigned int
    pm_getuint(FILE * const file);

    FILE * fileP;
    xel ** xels;
    int cols, rows;
    xelval maxval;
    int format;
    unsigned int row;

    fileP = pm_openr(fileName);

    pnm_readpnminit(fileP, &cols, &rows, &maxval, &format);

    xels = pnm_allocarray(cols, rows);

    for (row = 0; row < rows; ++row) {
        if (format == PGM_FORMAT || format == PPM_FORMAT) {
            /* Plain format -- can't use pnm_readpnmrow() because it will
               reject a sample > maxval
            */
            unsigned int col;
            for (col = 0; col < cols; ++col) {
                switch (format) {
                case PGM_FORMAT: {
                    gray const g = pm_getuint(fileP);
                    PNM_ASSIGN1(xels[row][col], g);
                    } break;
                case PPM_FORMAT: {
                    pixval const r = pm_getuint(fileP);
                    pixval const g = pm_getuint(fileP);
                    pixval const b = pm_getuint(fileP);

                    PNM_ASSIGN(xels[row][col], r, g, b);
                } break;
                default:
                    assert(false);
                }
            }
        } else {
            /* Raw or PBM format -- pnm_readpnmrow() won't do any maxval
               checking
            */
            pnm_readpnmrow(fileP, xels[row], cols, maxval, format);
        }
    }
    *colsP   = cols;
    *rowsP   = rows;
    *maxvalP = maxval;
    *formatP = format;
    *xelsP   = xels;

    pm_close(fileP);
}



int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    xel** cxels;
    int cformat;
    xelval cmaxval;
    struct convolveType convolveType;
    float ** rweights;
    float ** gweights;
    float ** bweights;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    readKernel(cmdline.kernelFilespec,
               &ccols, &crows, &cmaxval, &cformat, &cxels);

    if (ccols % 2 != 1 || crows % 2 != 1)
        pm_error("the convolution matrix must have an odd number of "
                 "rows and columns" );

    ccolso2 = ccols / 2;
    crowso2 = crows / 2;

    ifp = pm_openr(cmdline.inputFilespec);

    pnm_readpnminit(ifp, &cols, &rows, &maxval, &format);
    if (cols < ccols || rows < crows)
        pm_error("the image is smaller than the convolution matrix" );

    newformat = MAX(PNM_FORMAT_TYPE(cformat), PNM_FORMAT_TYPE(format));
    if (PNM_FORMAT_TYPE(cformat) != newformat)
        pnm_promoteformat(cxels, ccols, crows, cmaxval, cformat, 
                          cmaxval, newformat);
    if (PNM_FORMAT_TYPE(format) != newformat) {
        switch (PNM_FORMAT_TYPE(newformat)) {
        case PPM_TYPE:
            if (PNM_FORMAT_TYPE(format) != newformat)
                pm_message("promoting to PPM");
            break;
        case PGM_TYPE:
            if (PNM_FORMAT_TYPE(format) != newformat)
                pm_message("promoting to PGM");
            break;
        }
    }

    computeWeights(cxels, ccols, crows, newformat, cmaxval, !cmdline.nooffset,
                   &rweights, &gweights, &bweights);

    /* Handle certain special cases when runtime can be improved. */

    determineConvolveType(cxels, &convolveType);

    convolveIt(format, convolveType, 
               (const float **)rweights, 
               (const float **)gweights, 
               (const float **)bweights);

    pm_close(stdout);
    pm_close(ifp);
    return 0;
}



/******************************************************************************
                            SOME CHANGE HISTORY
*******************************************************************************

 Version 2.0.1 Changes
 ---------------------
 Fixed four lines that were improperly allocated as sizeof( float ) when they
 should have been sizeof( long ).

 Version 2.0 Changes
 -------------------

 Version 2.0 was written by Mike Burns (derived from Jef Poskanzer's
 original) in January 1995.

 Reduce run time by general optimizations and handling special cases of
 convolution matrices.  Program automatically determines if convolution 
 matrix is one of the types it can make use of so no extra command line
 arguments are necessary.

 Examples of convolution matrices for the special cases are

    Mean       Horizontal    Vertical
    x x x        x x x        x y z
    x x x        y y y        x y z
    x x x        z z z        x y z

 I don't know if the horizontal and vertical ones are of much use, but
 after working on the mean convolution, it gave me ideas for the other two.

 Some other compiler dependent optimizations
 -------------------------------------------
 Created separate functions as code was getting too large to put keep both
 PGM and PPM cases in same function and also because SWITCH statement in
 inner loop can take progressively more time the larger the size of the 
 convolution matrix.  GCC is affected this way.

 Removed use of MOD (%) operator from innermost loop by modifying manner in
 which the current xelbuf[] is chosen.

 This is from the file pnmconvol.README, dated August 1995, extracted in
 April 2000, which was in the March 1994 Netpbm release:

 ----------------------------------------------------------------------------- 
 This is a faster version of the pnmconvol.c program that comes with netpbm.
 There are no changes to the command line arguments, so this program can be
 dropped in without affecting the way you currently run it.  An updated man
 page is also included.
 
 My original intention was to improve the running time of applying a
 neighborhood averaging convolution matrix to an image by using a different
 algorithm, but I also improved the run time of performing the general
 convolution by optimizing that code.  The general convolution runs in 1/4 to
 1/2 of the original time and neighborhood averaging runs in near constant
 time for the convolution masks I tested (3x3, 5x5, 7x7, 9x9).
 
 Sample times for two computers are below.  Times are in seconds as reported
 by /bin/time for a 512x512 pgm image.
 
 Matrix                  IBM RS6000      SUN IPC
 Size & Type                220
 
 3x3
 original pnmconvol         6.3            18.4
 new general case           3.1             6.0
 new average case           1.8             2.6
 
 5x5
 original pnmconvol        11.9            44.4
 new general case           5.6            11.9
 new average case           1.8             2.6
 
 7x7
 original pnmconvol        20.3            82.9
 new general case           9.4            20.7
 new average case           1.8             2.6
 
 9x9
 original pnmconvol        30.9           132.4
 new general case          14.4            31.8
 new average case           1.8             2.6
 
 
 Send all questions/comments/bugs to me at burns@chem.psu.edu.
 
 - Mike
 
 ----------------------------------------------------------------------------
 Mike Burns                                              System Administrator
 burns@chem.psu.edu                                   Department of Chemistry
 (814) 863-2123                             The Pennsylvania State University
 ----------------------------------------------------------------------------

*/
