/* pnmrotate.c - read a portable anymap and rotate it by some angle
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

#define _XOPEN_SOURCE   /* get M_PI in math.h */

#include <math.h>
#include <assert.h>

#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"

#define SCALE 4096
#define HALFSCALE 2048

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilespec;  /* Filespecs of input file */
    float angle;                /* Angle to rotate, in radians */
    unsigned int noantialias;
    const char * background;  /* NULL if none */
    unsigned int keeptemp;  /* For debugging */
    unsigned int verbose;
};


enum rotationDirection {CLOCKWISE, COUNTERCLOCKWISE};

struct shearParm {
    /* These numbers tell how to shear a pixel, but I haven't figured out 
       yet exactly what each means.
    */
    long fracnew0;
    long omfracnew0;
    unsigned int shiftWhole;
    unsigned int shiftUnits;
};



static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int backgroundSpec;
    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "background",  OPT_STRING, &cmdlineP->background, 
            &backgroundSpec,        0);
    OPTENT3(0, "noantialias", OPT_FLAG,   NULL, 
            &cmdlineP->noantialias, 0);
    OPTENT3(0, "keeptemp",    OPT_FLAG,   NULL, 
            &cmdlineP->keeptemp,    0);
    OPTENT3(0, "verbose",     OPT_FLAG,   NULL, 
            &cmdlineP->verbose,     0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = TRUE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!backgroundSpec)
        cmdlineP->background = NULL;

    if (argc-1 < 1)
        pm_error("You must specify at least one argument:  the angle "
                 "to rotate.");
    else {
        int rc;
        float angleArg;

        rc = sscanf(argv[1], "%f", &angleArg);

        if (rc != 1)
            pm_error("Invalid angle argument: '%s'.  Must be a floating point "
                     "number of degrees.", argv[1]);
        else if (angleArg < -90.0 || angleArg > 90.0)
            pm_error("angle must be between -90 and 90, inclusive.  "
                     "You specified %f.  "
                     "Use 'pamflip' for other rotations.", angleArg);
        else {
            /* Convert to radians */
            cmdlineP->angle = angleArg * M_PI / 180.0;

            if (argc-1 < 2)
                cmdlineP->inputFilespec = "-";
            else {
                cmdlineP->inputFilespec = argv[2];
                
                if (argc-1 > 2)
                    pm_error("Program takes at most two arguments "
                             "(angle and filename).  You specified %d",
                             argc-1);
            }
        }
    }
}



static void
storeImage(const char * const fileName,
           xel **       const xels,
           unsigned int const cols,
           unsigned int const rows,
           xelval       const maxval,
           int          const format) {

    FILE * ofP;

    ofP = pm_openw(fileName);

    pnm_writepnm(ofP, xels, cols, rows, maxval, format, 0);

    pm_close(ofP);
}

  

static void
computeNewFormat(bool     const antialias, 
                 int      const format,
                 xelval   const maxval,
                 int *    const newformatP,
                 xelval * const newmaxvalP) {

    if (antialias && PNM_FORMAT_TYPE(format) == PBM_TYPE) {
        *newformatP = PGM_TYPE;
        *newmaxvalP = PGM_MAXMAXVAL;
        pm_message("promoting from PBM to PGM - "
                   "use -noantialias to avoid this");
    } else {
        *newformatP = format;
        *newmaxvalP = maxval;
    }
}



static bool
isWhite(xel    const color,
        xelval const maxval) {

    return (PPM_GETR(color) == maxval &&
            PPM_GETG(color) == maxval &&
            PPM_GETB(color) == maxval);
}



static bool
isBlack(xel const color) {

    return (PPM_GETR(color) == 0 &&
            PPM_GETG(color) == 0 &&
            PPM_GETB(color) == 0);
}



static xel
backgroundColor(const char * const backgroundColorName,
                xel *        const topRow,
                int          const cols,
                xelval       const maxval,
                int          const format) {

    xel retval;

    if (backgroundColorName) {
        retval = ppm_parsecolor(backgroundColorName, maxval);

        switch(PNM_FORMAT_TYPE(format)) {
        case PGM_TYPE:
            if (!PPM_ISGRAY(retval))
                pm_error("Image is PGM (grayscale), "
                         "but you specified a non-gray "
                         "background color '%s'", backgroundColorName);

            break;
        case PBM_TYPE:
            if (!isWhite(retval, maxval) && !isBlack(retval))
                pm_error("Image is PBM (black and white), "
                         "but you specified '%s', which is neither black "
                         "nor white, as background color", 
                         backgroundColorName);
            break;
        }
    } else 
        retval = pnm_backgroundxelrow(topRow, cols, maxval, format);

    return retval;
}



static void
reportBackground(xel const bgColor) {

    pm_message("Background color %u/%u/%u",
               PPM_GETR(bgColor), PPM_GETG(bgColor), PPM_GETB(bgColor));
}



static void
shearX(xel * const inRow, 
       xel * const outRow, 
       int   const cols, 
       int   const format,
       xel   const bgxel,
       bool  const antialias,
       float const shiftAmount,
       int   const newcols) {
/*----------------------------------------------------------------------------
   Shift a the row inRow[] right by 'shiftAmount' pixels and return the
   result as outRow[].

   The input row is 'cols' columns wide, whereas the output row is
   'newcols'.

   The format of the input row is 'format'.

   We shift the row on a background of color 'bgxel'.

   The output row has the same format and maxval as the input.
   
   'shiftAmount' may not be negative.
   
   'shiftAmount' can be fractional, so we either just go by the
   nearest integer value or mix pixels to achieve the shift, depending
   on 'antialias'.
-----------------------------------------------------------------------------*/
    assert(shiftAmount >= 0.0);

    if (antialias) {
        unsigned int const shiftWhole = (unsigned int) shiftAmount;
        long const fracShift = (shiftAmount - shiftWhole) * SCALE;
        long const omfracShift = SCALE - fracShift;

        unsigned int col;
        xel * nxP;
        xel prevxel;

        for (col = 0; col < newcols; ++col)
            outRow[col] = bgxel;
            
        prevxel = bgxel;
        for (col = 0, nxP = &(outRow[shiftWhole]);
             col < cols; ++col, ++nxP) {

            xel const p = inRow[col];

            switch (PNM_FORMAT_TYPE(format)) {
            case PPM_TYPE:
                PPM_ASSIGN(*nxP,
                           (fracShift * PPM_GETR(prevxel) 
                            + omfracShift * PPM_GETR(p) 
                            + HALFSCALE) / SCALE,
                           (fracShift * PPM_GETG(prevxel) 
                            + omfracShift * PPM_GETG(p) 
                            + HALFSCALE) / SCALE,
                           (fracShift * PPM_GETB(prevxel) 
                            + omfracShift * PPM_GETB(p) 
                            + HALFSCALE) / SCALE );
                break;
                
            default:
                PNM_ASSIGN1(*nxP,
                            (fracShift * PNM_GET1(prevxel) 
                             + omfracShift * PNM_GET1(p) 
                             + HALFSCALE) / SCALE );
                break;
            }
            prevxel = p;
        }
        if (fracShift> 0 && shiftWhole + cols < newcols) {
            switch (PNM_FORMAT_TYPE(format)) {
            case PPM_TYPE:
                PPM_ASSIGN(*nxP,
                           (fracShift * PPM_GETR(prevxel) 
                            + omfracShift * PPM_GETR(bgxel) 
                            + HALFSCALE) / SCALE,
                           (fracShift * PPM_GETG(prevxel) 
                            + omfracShift * PPM_GETG(bgxel) 
                            + HALFSCALE) / SCALE,
                           (fracShift * PPM_GETB(prevxel) 
                            + omfracShift * PPM_GETB(bgxel) 
                            + HALFSCALE) / SCALE );
                break;
                    
            default:
                PNM_ASSIGN1(*nxP,
                            (fracShift * PNM_GET1(prevxel) 
                             + omfracShift * PNM_GET1(bgxel) 
                             + HALFSCALE) / SCALE );
                break;
            }
        }
    } else {
        unsigned int const shiftCols = (unsigned int) (shiftAmount + 0.5);
        unsigned int col;
        unsigned int outcol;

        outcol = 0;  /* initial value */
        
        for (col = 0; col < shiftCols; ++col)
            outRow[outcol++] = bgxel;
        for (col = 0; col < cols; ++col)
            outRow[outcol++] = inRow[col];
        for (col = shiftCols + cols; col < newcols; ++col)
            outRow[outcol++] = bgxel;
        
        assert(outcol == newcols);
    }
}



static void
shearXFromInputFile(FILE *                 const ifP,
                    unsigned int           const cols,
                    unsigned int           const rows,
                    xelval                 const maxval,
                    int                    const format,
                    enum rotationDirection const direction,
                    float                  const xshearfac,
                    xelval                 const newmaxval,
                    int                    const newformat,
                    bool                   const antialias,
                    const char *           const background,
                    xel ***                const shearedXelsP,
                    unsigned int *         const newcolsP,
                    xel *                  const bgColorP) {
/*----------------------------------------------------------------------------
   Shear X from input file into newly malloced xel array.  Return that
   array as *shearedColsP, and its width as *tempColsP.  Everything else
   about the sheared image is the same as for the input image.

   The input image on file 'ifP' is described by 'cols', 'rows',
   'maxval', and 'format'.

   Along the way, figure out what the background color of the output should
   be based on the contents of the file and the user's directive
   'background' and return that as *bgColorP.
-----------------------------------------------------------------------------*/
    unsigned int const maxShear = (rows - 0.5) * xshearfac + 0.5;
    unsigned int const newcols = cols + maxShear;
    
    xel ** shearedXels;
    xel * xelrow;
    xel bgColor;
    unsigned int row;

    shearedXels = pnm_allocarray(newcols, rows);

    xelrow = pnm_allocrow(cols);

    for (row = 0; row < rows; ++row) {
        /* The shear factor is designed to shear over the entire width
           from the left edge of of the left pixel to the right edge of
           the right pixel.  We use the distance of the center of this
           pixel from the relevant edge to compute shift amount:
        */
        float const xDistance = 
            (direction == COUNTERCLOCKWISE ? row + 0.5 : (rows-0.5 - row));
        float const shiftAmount = xshearfac * xDistance;

        pnm_readpnmrow(ifP, xelrow, cols, maxval, format);

        pnm_promoteformatrow(xelrow, cols, maxval, format, 
                             newmaxval, newformat);

        if (row == 0)
            bgColor =
                backgroundColor(background, xelrow, cols, newmaxval, format);

        shearX(xelrow, shearedXels[row], cols, newformat, bgColor,
               antialias, shiftAmount, newcols);
    }
    pnm_freerow(xelrow);

    *shearedXelsP = shearedXels;
    *newcolsP = newcols;

    assert(rows >= 1);  /* Ergo, bgColor is defined */
    *bgColorP = bgColor;
}



static void 
shearYNoAntialias(xel **           const inxels,
                  xel **           const outxels,
                  int              const cols,
                  int              const inrows,
                  int              const outrows,
                  int              const format,
                  xel              const bgColor,
                  struct shearParm const shearParm[]) {
/*----------------------------------------------------------------------------
   Shear the image in 'inxels' ('cols' x 'inrows') vertically into
   'outxels' ('cols' x 'outrows'), both format 'format'.  shearParm[X]
   tells how much to shear pixels in Column X (clipped to Rows 0
   through 'outrow' -1) and 'bgColor' is what to use for background
   where there is none of the input in the output.

   We do not do any antialiasing.  We simply move whole pixels.

   We go row by row instead of column by column to save real memory.  Going
   row by row, the working set is only a few pages, whereas going column by
   column, it would be one page per output row plus one page per input row.
-----------------------------------------------------------------------------*/
    unsigned int inrow;
    unsigned int outrow;

    /* Fill the output with background */
    for (outrow = 0; outrow < outrows; ++outrow) {
        unsigned int col;
        for (col = 0; col < cols; ++col)
            outxels[outrow][col] = bgColor;
    }

    /* Overlay that background with sheared image */
    for (inrow = 0; inrow < inrows; ++inrow) {
        unsigned int col;
        for (col = 0; col < cols; ++col) {
            int const outrow = inrow + shearParm[col].shiftUnits;
            if (outrow >= 0 && outrow < outrows)
                outxels[outrow][col] = inxels[inrow][col];
        }
    }
}



static void
shearYColAntialias(xel ** const inxels, 
                   xel ** const outxels,
                   int    const col,
                   int    const inrows,
                   int    const outrows,
                   int    const format,
                   xel    const bgxel,
                   struct shearParm shearParm[]) {
/*-----------------------------------------------------------------------------
  Shear a column vertically.
-----------------------------------------------------------------------------*/
    long const fracnew0   = shearParm[col].fracnew0;
    long const omfracnew0 = shearParm[col].omfracnew0;
    int  const shiftWhole = shearParm[col].shiftWhole;
        
    int outrow;

    xel prevxel;
    int inrow;
        
    /* Initialize everything to background color */
    for (outrow = 0; outrow < outrows; ++outrow)
        outxels[outrow][col] = bgxel;

    prevxel = bgxel;
    for (inrow = 0; inrow < inrows; ++inrow) {
        int const outrow = inrow + shiftWhole;

        if (outrow >= 0 && outrow < outrows) {
            xel * const nxP = &(outxels[outrow][col]);
            xel const x = inxels[inrow][col];
            switch ( PNM_FORMAT_TYPE(format) ) {
            case PPM_TYPE:
                PPM_ASSIGN(*nxP,
                           (fracnew0 * PPM_GETR(prevxel) 
                            + omfracnew0 * PPM_GETR(x) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETG(prevxel) 
                            + omfracnew0 * PPM_GETG(x) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETB(prevxel) 
                            + omfracnew0 * PPM_GETB(x) 
                            + HALFSCALE) / SCALE );
                break;
                        
            default:
                PNM_ASSIGN1(*nxP,
                            (fracnew0 * PNM_GET1(prevxel) 
                             + omfracnew0 * PNM_GET1(x) 
                             + HALFSCALE) / SCALE );
                break;
            }
            prevxel = x;
        }
    }
    if (fracnew0 > 0 && shiftWhole + inrows < outrows) {
        xel * const nxP = &(outxels[shiftWhole + inrows][col]);
        switch (PNM_FORMAT_TYPE(format)) {
        case PPM_TYPE:
            PPM_ASSIGN(*nxP,
                       (fracnew0 * PPM_GETR(prevxel) 
                        + omfracnew0 * PPM_GETR(bgxel) 
                        + HALFSCALE) / SCALE,
                       (fracnew0 * PPM_GETG(prevxel) 
                        + omfracnew0 * PPM_GETG(bgxel) 
                        + HALFSCALE) / SCALE,
                       (fracnew0 * PPM_GETB(prevxel) 
                        + omfracnew0 * PPM_GETB(bgxel) 
                        + HALFSCALE) / SCALE);
            break;
                
        default:
            PNM_ASSIGN1(*nxP,
                        (fracnew0 * PNM_GET1(prevxel) 
                         + omfracnew0 * PNM_GET1(bgxel) 
                         + HALFSCALE) / SCALE);
            break;
        }
    }
} 



static void
shearImageY(xel **                 const inxels,
            int                    const cols,
            int                    const inrows,
            int                    const format,
            xel                    const bgxel,
            bool                   const antialias,
            enum rotationDirection const direction,
            float                  const yshearfac,
            int                    const yshearjunk,
            xel ***                const outxelsP,
            unsigned int *         const outrowsP) {
    
    unsigned int const maxShear = (cols - 0.5) * yshearfac + 0.5;
    unsigned int const outrows = inrows + maxShear - 2 * yshearjunk;

    struct shearParm * shearParm;  /* malloc'ed */
    int col;
    xel ** outxels;
    
    outxels = pnm_allocarray(cols, outrows);

    MALLOCARRAY(shearParm, cols);
    if (shearParm == NULL)
        pm_error("Unable to allocate memory for shearParm");

    for (col = 0; col < cols; ++col) {
        /* The shear factor is designed to shear over the entire height
           from the top edge of of the top pixel to the bottom edge of
           the bottom pixel.  We use the distance of the center of this
           pixel from the relevant edge to compute shift amount:
        */
        float const yDistance = 
            (direction == CLOCKWISE ? col + 0.5 : (cols-0.5 - col));
        float const shiftAmount = yshearfac * yDistance;

        shearParm[col].fracnew0   = (shiftAmount - (int)shiftAmount) * SCALE;
        shearParm[col].omfracnew0 = SCALE - shearParm[col].fracnew0;
        shearParm[col].shiftWhole = (int)shiftAmount - yshearjunk;
        shearParm[col].shiftUnits = (int)(shiftAmount + 0.5) - yshearjunk;
    }
    if (!antialias)
        shearYNoAntialias(inxels, outxels, cols, inrows, outrows, format,
                          bgxel, shearParm);
    else {
        /* TODO: do this row-by-row, same as for noantialias, to save
           real memory.
        */
        for (col = 0; col < cols; ++col) 
            shearYColAntialias(inxels, outxels, col, inrows, outrows, format, 
                               bgxel, shearParm);
    }
    free(shearParm);
    
    *outxelsP = outxels;
    *outrowsP = outrows;
}



static void
shearFinal(xel * const inRow, 
           xel * const outRow, 
           int   const incols, 
           int   const outcols,
           int   const format,
           xel   const bgxel,
           bool  const antialias,
           float const shiftAmount,
           int   const x2shearjunk) {


    assert(shiftAmount >= 0.0);

    {
        unsigned int col;
        for (col = 0; col < outcols; ++col)
            outRow[col] = bgxel;
    }

    if (antialias) {
        long const fracnew0   = (shiftAmount - (int) shiftAmount) * SCALE; 
        long const omfracnew0 = SCALE - fracnew0; 
        unsigned int const shiftWhole = (int)shiftAmount - x2shearjunk;

        xel prevxel;
        unsigned int col;

        prevxel = bgxel;
        for (col = 0; col < incols; ++col) {
            int const new = shiftWhole + col;
            if (new >= 0 && new < outcols) {
                xel * const nxP = &(outRow[new]);
                xel const x = inRow[col];
                switch (PNM_FORMAT_TYPE(format)) {
                case PPM_TYPE:
                    PPM_ASSIGN(*nxP,
                               (fracnew0 * PPM_GETR(prevxel) 
                                + omfracnew0 * PPM_GETR(x) 
                                + HALFSCALE) / SCALE,
                               (fracnew0 * PPM_GETG(prevxel) 
                                + omfracnew0 * PPM_GETG(x) 
                                + HALFSCALE) / SCALE,
                               (fracnew0 * PPM_GETB(prevxel) 
                                + omfracnew0 * PPM_GETB(x) 
                                + HALFSCALE) / SCALE);
                    break;
                    
                default:
                    PNM_ASSIGN1(*nxP,
                                (fracnew0 * PNM_GET1(prevxel) 
                                 + omfracnew0 * PNM_GET1(x) 
                                 + HALFSCALE) / SCALE );
                    break;
                }
                prevxel = x;
            }
        }
        if (fracnew0 > 0 && shiftWhole + incols < outcols) {
            xel * const nxP = &(outRow[shiftWhole + incols]);
            switch (PNM_FORMAT_TYPE(format)) {
            case PPM_TYPE:
                PPM_ASSIGN(*nxP,
                           (fracnew0 * PPM_GETR(prevxel) 
                            + omfracnew0 * PPM_GETR(bgxel) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETG(prevxel) 
                            + omfracnew0 * PPM_GETG(bgxel) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETB(prevxel) 
                            + omfracnew0 * PPM_GETB(bgxel) 
                            + HALFSCALE) / SCALE);
                break;
                
            default:
                PNM_ASSIGN1(*nxP,
                            (fracnew0 * PNM_GET1(prevxel) 
                             + omfracnew0 * PNM_GET1(bgxel) 
                             + HALFSCALE) / SCALE );
                break;
            }
        }
    } else {
        unsigned int const shiftCols =
            (unsigned int)(shiftAmount + 0.5) - x2shearjunk;

        unsigned int col;
        for (col = 0; col < incols; ++col) {
            unsigned int const outcol = shiftCols + col;
            if (outcol >= 0 && outcol < outcols)
                outRow[outcol] = inRow[col];
        }
    }
}



static void
shearXToOutputFile(FILE *                 const ofP,
                   xel **                 const xels,
                   unsigned int           const cols, 
                   unsigned int           const rows,
                   xelval                 const maxval,
                   int                    const format,
                   enum rotationDirection const direction,
                   float                  const xshearfac,
                   int                    const x2shearjunk,
                   xel                    const bgColor,
                   bool                   const antialias) {
/*----------------------------------------------------------------------------
   Shear horizontally the image in 'xels' and write the result to file
   'ofP'.  'cols', 'rows', 'maxval', and 'format' describe the image in
   'xels'.  They also describe the output image, except that it will be
   wider as dictated by the shearing parameters.

   Shear over background color 'bgColor'.

   Do a smooth pixel-mixing shear iff 'antialias' is true.
-----------------------------------------------------------------------------*/
    unsigned int const maxShear = (rows - 0.5) * xshearfac + 0.5;
    unsigned int const newcols = cols + maxShear - 2 * x2shearjunk;

    unsigned int row;
    xel * xelrow;
    
    pnm_writepnminit(stdout, newcols, rows, maxval, format, 0);

    xelrow = pnm_allocrow(newcols);

    for (row = 0; row < rows; ++row) {
        /* The shear factor is designed to shear over the entire width
           from the left edge of of the left pixel to the right edge of
           the right pixel.  We use the distance of the center of this
           pixel from the relevant edge to compute shift amount:
        */
        float const xDistance = 
            (direction == COUNTERCLOCKWISE ? row + 0.5 : (rows-0.5 - row));
        float const shiftAmount = xshearfac * xDistance;

        shearFinal(xels[row], xelrow, cols, newcols, format, 
                   bgColor, antialias, shiftAmount, x2shearjunk);

        pnm_writepnmrow(stdout, xelrow, newcols, maxval, format, 0);
    }
    pnm_freerow(xelrow);
}



int
main(int argc, char *argv[]) { 

    struct cmdlineInfo cmdline;
    FILE * ifP;
    xel ** shear1xels;
    xel ** shear2xels;
    xel bgColor;
    int rows, cols, format;
    int newformat;
    unsigned int newrows;
    int newRowsWithJunk;
    unsigned int shear1Cols;
    int yshearjunk, x2shearjunk;
    xelval maxval, newmaxval;
    float xshearfac, yshearfac;
    enum rotationDirection direction;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    pnm_readpnminit(ifP, &cols, &rows, &maxval, &format);
    
    computeNewFormat(!cmdline.noantialias, format, maxval, 
                     &newformat, &newmaxval);

    xshearfac = fabs(tan(cmdline.angle / 2.0));
    yshearfac = fabs(sin(cmdline.angle));
    direction = cmdline.angle > 0 ? COUNTERCLOCKWISE : CLOCKWISE;

    /* The algorithm we use, for maximum speed, is 3 simple shears:
       A horizontal, a vertical, and another horizontal.
    */

    shearXFromInputFile(ifP, cols, rows, maxval, format,
                        direction, xshearfac,
                        newmaxval, newformat,
                        !cmdline.noantialias, cmdline.background,
                        &shear1xels, &shear1Cols, &bgColor);
    
    pm_close(ifP);

    if (cmdline.verbose)
        reportBackground(bgColor);

    if (cmdline.keeptemp)
        storeImage("pnmrotate_stage1.pnm", shear1xels, shear1Cols, rows,
                   newmaxval, newformat);

    yshearjunk = (shear1Cols - cols) * yshearfac;
    newRowsWithJunk = (shear1Cols - 1) * yshearfac + rows + 0.999999;
    x2shearjunk = (newRowsWithJunk - rows - yshearjunk - 1) * xshearfac;

    shearImageY(shear1xels, shear1Cols, rows, newformat,
                bgColor, !cmdline.noantialias, direction,
                yshearfac, yshearjunk,
                &shear2xels, &newrows);

    pnm_freearray(shear1xels, rows);

    if (cmdline.keeptemp)
        storeImage("pnmrotate_stage2.pnm", shear2xels, shear1Cols, newrows, 
                   newmaxval, newformat);

    shearXToOutputFile(stdout, shear2xels, shear1Cols, newrows,
                       newmaxval, newformat,
                       direction, xshearfac, x2shearjunk, 
                       bgColor, !cmdline.noantialias);

    pnm_freearray(shear2xels, newrows);
    pm_close(stdout);
    
    return 0;
}
