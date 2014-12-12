/******************************************************************************
                                  Pnmnorm
*******************************************************************************

  This program normalizes the contrast in a Netpbm image.

  by Bryan Henderson bryanh@giraffe-data.com San Jose CA March 2002.
  Adapted from Ppmnorm.

  Ppmnorm is by Wilson H. Bent, Jr. (whb@usc.edu)
  Extensively hacked from pgmnorm.c, which carries the following note:
  
  Copyright (C) 1989, 1991 by Jef Poskanzer.
  
  Permission to use, copy, modify, and distribute this software and its
  documentation for any purpose and without fee is hereby granted, provided
  that the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.  This software is provided "as is" without express or
  implied warranty.
  
  (End of note from pgmnorm.c)

  Pgmnorm's man page also said:
  
  Partially based on the fbnorm filter in Michael Mauldin's "Fuzzy Pixmap"
  package.
*****************************************************************************/

#include <assert.h>

#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"

enum brightMethod {BRIGHT_LUMINOSITY, BRIGHT_COLORVALUE, BRIGHT_SATURATION};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    unsigned int bvalueSpec;
    xelval bvalue;
    unsigned int bpercentSpec;
    float bpercent;
    unsigned int wvalueSpec;
    xelval wvalue;
    unsigned int wpercentSpec;
    float wpercent;
    enum brightMethod brightMethod;
    unsigned int keephues;
    float maxExpansion;
        /* The maximum allowed expansion factor for expansion specified
           by per centile.  This is a factor, not a per cent increase.
           E.g. 50% increase means a factor of 1.50.
        */
};



static void
parseCommandLine (int argc, char ** argv,
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

    unsigned int luminosity, colorvalue, saturation;
    unsigned int maxexpandSpec;
    float maxexpand;
    
    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "bpercent",      OPT_FLOAT,   
            &cmdlineP->bpercent,   &cmdlineP->bpercentSpec, 0);
    OPTENT3(0,   "wpercent",      OPT_FLOAT,   
            &cmdlineP->wpercent,   &cmdlineP->wpercentSpec, 0);
    OPTENT3(0,   "bvalue",        OPT_UINT,   
            &cmdlineP->bvalue,     &cmdlineP->bvalueSpec, 0);
    OPTENT3(0,   "wvalue",        OPT_UINT,   
            &cmdlineP->wvalue,     &cmdlineP->wvalueSpec, 0);
    OPTENT3(0,   "maxexpand",     OPT_FLOAT,   
            &maxexpand,            &maxexpandSpec, 0);
    OPTENT3(0,   "keephues",      OPT_FLAG,   
            NULL,                  &cmdlineP->keephues, 0);
    OPTENT3(0,   "luminosity",    OPT_FLAG,   
            NULL,                  &luminosity, 0);
    OPTENT3(0,   "colorvalue",    OPT_FLAG,   
            NULL,                  &colorvalue, 0);
    OPTENT3(0,   "saturation",    OPT_FLAG,   
            NULL,                  &saturation, 0);
    OPTENT3(0,   "brightmax",     OPT_FLAG,   
            NULL,                  &colorvalue, 0);

    /* Note: -brightmax was documented and accepted long before it was
       actually implemented.  By the time we implemented it, we
       decided -colorvalue was a better name for it.
    */

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (!cmdlineP->wpercentSpec)
        cmdlineP->wpercent = 1.0;
    if (!cmdlineP->bpercentSpec)
        cmdlineP->bpercent = 2.0;

    if (cmdlineP->wpercent < 0.0)
        pm_error("You specified a negative value for wpercent: %f",
                 cmdlineP->wpercent);
    if (cmdlineP->bpercent < 0.0)
        pm_error("You specified a negative value for bpercent: %f",
                 cmdlineP->bpercent);
    if (cmdlineP->wpercent > 100.0)
        pm_error("You specified a per centage > 100 for wpercent: %f",
                 cmdlineP->wpercent);
    if (cmdlineP->bpercent > 100.0)
        pm_error("You specified a per centage > 100 for bpercent: %f",
                 cmdlineP->bpercent);

    if (luminosity + colorvalue + saturation > 1)
        pm_error("You can specify only one of "
                 "-luminosity, -colorvalue, and -saturation");
    else {
        if (colorvalue)
            cmdlineP->brightMethod = BRIGHT_COLORVALUE;
        else if (saturation)
            cmdlineP->brightMethod = BRIGHT_SATURATION;
        else
            cmdlineP->brightMethod = BRIGHT_LUMINOSITY;
    }
    if (maxexpandSpec) {
        if (maxexpand < 0)
            pm_error("-maxexpand must be positive.  You specified %f",
                     maxexpand);
        else
            cmdlineP->maxExpansion = 1 + (float)maxexpand/100;
    } else
        cmdlineP->maxExpansion = 1e6;  /* essentially infinite */

    if (argc-1 > 1)
        pm_error("Program takes at most one argument: the input file "
                 "specification.  "
                 "You specified %d arguments.", argc-1);
    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else
        cmdlineP->inputFilespec = argv[1];
}



static void
buildHistogram(FILE *   const ifp, 
               int      const cols,
               int      const rows,
               xelval   const maxval,
               int      const format,
               unsigned int   hist[],
               enum brightMethod const brightMethod) {
/*----------------------------------------------------------------------------
   Build the histogram of brightness values for the image that is in file
   'ifp', which is positioned just after the header (at the raster).

   The histogram is the array hist[] such that hist[x] is the number
   of xels in the image that have brightness x.  That brightness is
   either the color value (intensity of most intense component) of the
   xel or it is the luminosity of the xel, depending on
   'brightMethod'.  In either case, it is based on the same maxval as
   the image, which is 'maxval'.  The image is 'cols' columns wide by
   'rows' rows high.

   Leave the file positioned arbitrarily.
-----------------------------------------------------------------------------*/
    int row;
    xel * xelrow;
    
    xelrow = pnm_allocrow(cols);

    {
        unsigned int i;
        for (i = 0; i <= maxval; ++i)
            hist[i] = 0;
    }
    for (row = 0; row < rows; ++row) {
        int col;
        pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
        for (col = 0; col < cols; ++col) {
            xelval brightness;
            xel const p = xelrow[col];
            if (PNM_FORMAT_TYPE(format) == PPM_TYPE) {
                switch(brightMethod) {
                case BRIGHT_LUMINOSITY:
                    brightness = PPM_LUMIN(p);
                    break;
                case BRIGHT_COLORVALUE:
                    brightness = ppm_colorvalue(p);
                    break;
                case BRIGHT_SATURATION:
                    brightness = ppm_saturation(p, maxval);
                    break;
                }
            } else
                brightness = PNM_GET1(p);
            ++hist[brightness];
        }
    }
    pnm_freerow(xelrow);
}



static void
computeBottomPercentile(unsigned int         hist[], 
                        unsigned int   const highest,
                        unsigned int   const total,
                        float          const percent, 
                        unsigned int * const percentileP) {
/*----------------------------------------------------------------------------
   Compute the lowest index of hist[] such that the sum of the hist[]
   values with that index and lower represent at least 'percent' per cent of
   'n' (which is assumed to be the sum of all the values in hist[],
   given to us to save us the time of computing it).
-----------------------------------------------------------------------------*/
    unsigned int cutoff = total * percent / 100.0;
    unsigned int count;
    unsigned int percentile;

    percentile = 0; /* initial value */
    count = hist[0];  /* initial value */

    while (count < cutoff) {
        if (percentile == highest)
            pm_error("Internal error: computeBottomPercentile() received"
                     "a 'total' value greater than the sum of the hist[]"
                     "values");
        ++percentile;
        count += hist[percentile];
    }        
    *percentileP = percentile;
}



static void
computeTopPercentile(unsigned int         hist[], 
                     unsigned int   const highest, 
                     unsigned int   const total,
                     float          const percent, 
                     unsigned int * const percentileP) {
/*----------------------------------------------------------------------------
   Compute the highest index of hist[] such that the sum of the hist[]
   values with that index and higher represent 'percent' per cent of
   'n' (which is assumed to be the sum of all the values in hist[],
   given to us to save us the time of computing it).
-----------------------------------------------------------------------------*/
    unsigned int cutoff = total * percent / 100.0;
    unsigned int count;
    unsigned int percentile;

    percentile = highest; /* initial value */
    count = hist[highest];

    while (count < cutoff) {
        --percentile;
        count += hist[percentile];
    }
    *percentileP = percentile;
}



static void
computeAdjustmentForExpansionLimit(xelval   const maxval,
                                   xelval   const unlBvalue,
                                   xelval   const unlWvalue,
                                   float    const maxExpansion,
                                   xelval * const bLowerP,
                                   xelval * const wRaiseP) {
/*----------------------------------------------------------------------------
   Assuming 'unlBvalue' and 'unlWvalue' are the appropriate bvalue and
   wvalue to normalize the image to 0 .. maxval, compute the amount
   by which the bvalue must be raised and the wvalue lowered from that
   in order to cap the expansion factor at 'maxExpansion'.

   E.g. if 'maxval' is 100, 'unlBvalue' is 20 and 'unlWvalue' is 70, that
   implies an expansion factor of 100/50 (because the range goes from
   70-20, which is 50, to 100 - 0, which is 100).  If 'maxEpansion' is
   1.333, these values are unacceptable.  To get down to the desired 1.333
   factor, we need the span of bvalue to wvalue to be 75, not 50.  So
   we need to raise the bvalue and lower the wvalue by a total of 25.
   We apportion that adjustment to bvalue and wvalue in proportion to
   how close each is already to it's end (which we call the margin).
   'unlBvalue' is 20 from its end, while 'unlWvalue' is 30 from its end,
   so we want to lower the bvalue by 10 and raise the wvalue by 15.
   Ergo we return *bLowerP = 10 and *wRaise = 15.
-----------------------------------------------------------------------------*/
    unsigned int const newRange = maxval - 0;
        /* The range of sample values after normalization, if we used
           the unlimited bvalue and wvalue
        */
    unsigned int const oldRange = unlWvalue - unlBvalue;
        /* The range of sample values in the original image that normalize
           to 0 .. maxval, if we used the unlimited bvalue and wvalue
        */
    float const unlExpansion = (float)newRange/oldRange;
    
    if (unlExpansion <= maxExpansion) {
        /* No capping is necessary.  Unlimited values are already within
           range.
           */
        *bLowerP = 0;
        *wRaiseP = 0;
    } else {
        unsigned int const totalWidening = newRange/maxExpansion - oldRange;
            /* Amount by which the (bvalue, wvalue) range must be widened
               to limit expansion to 'maxExpansion'
            */
        unsigned int const bMargin = unlBvalue - 0;
        unsigned int const wMargin = maxval - unlWvalue;

        /* Apportion 'totalWidening' between the black and and the
           white end
        */
        *bLowerP =
            ROUNDU((float)bMargin / (bMargin + wMargin) * totalWidening);
        *wRaiseP =
            ROUNDU((float)wMargin / (bMargin + wMargin) * totalWidening);

        pm_message("limiting expansion of %.1f%% to %.1f%%",
                   (unlExpansion - 1) * 100, (maxExpansion -1) * 100);
    }
}



static void
computeEndValues(FILE *             const ifp,
                 int                const cols,
                 int                const rows,
                 xelval             const maxval,
                 int                const format,
                 struct cmdlineInfo const cmdline,
                 xelval *           const bvalueP,
                 xelval *           const wvalueP) {
/*----------------------------------------------------------------------------
   Figure out what original values will be translated to full white and
   full black -- thus defining to what all the other values get translated.

   This may involve looking at the image.  The image is in the file
   'ifp', which is positioned just past the header (at the raster).
   Leave it positioned arbitrarily.
-----------------------------------------------------------------------------*/
    unsigned int * hist;  /* malloc'ed */

    MALLOCARRAY(hist, PNM_OVERALLMAXVAL+1);

    if (hist == NULL)
        pm_error("Unable to allocate storage for intensity histogram.");
    else {
        xelval unlimitedBvalue, unlimitedWvalue;
        unsigned int bLower, wRaise;

        buildHistogram(ifp, cols, rows, maxval, format, hist,
                       cmdline.brightMethod);

        if (cmdline.bvalueSpec && !cmdline.bpercentSpec) {
            unlimitedBvalue = cmdline.bvalue;
        } else {
            xelval percentBvalue;
            computeBottomPercentile(hist, maxval, cols*rows, cmdline.bpercent, 
                                    &percentBvalue);
            if (cmdline.bvalueSpec)
                unlimitedBvalue = MIN(percentBvalue, cmdline.bvalue);
            else
                unlimitedBvalue = percentBvalue;
        }

        if (cmdline.wvalueSpec && !cmdline.wpercentSpec) {
            unlimitedWvalue = cmdline.wvalue;
        } else {
            xelval percentWvalue;
            computeTopPercentile(hist, maxval, cols*rows, cmdline.wpercent, 
                                 &percentWvalue);
            if (cmdline.wvalueSpec)
                unlimitedWvalue = MAX(percentWvalue, cmdline.wvalue);
            else
                unlimitedWvalue = percentWvalue;
        }

        computeAdjustmentForExpansionLimit(
            maxval, unlimitedBvalue, unlimitedWvalue, cmdline.maxExpansion,
            &bLower, &wRaise);

        *bvalueP = unlimitedBvalue - bLower;
        *wvalueP = unlimitedWvalue + wRaise;

        free(hist);
    }
}



static void
computeTransferFunction(xelval            const bvalue, 
                        xelval            const wvalue,
                        xelval            const maxval,
                        xelval **         const newBrightnessP) {
/*----------------------------------------------------------------------------
   Compute the transfer function, i.e. the array *newBrightnessP such that
   (*newBrightnessP)[x] is the brightness of the xel that should replace a
   xel with brightness x.  Brightness in this case means either luminosity
   or color value (and it doesn't matter to us which).

   'bvalue' is the highest brightness that should map to zero brightness;
   'wvalue' is the lowest brightness that should map to full brightness.
   brightnesses in between should be stretched linearly.  (That stretching
   could conceivably result in more brightnesses mapping to zero and full
   brightness, due to rounding).

   Define function only for values 0..maxval.
-----------------------------------------------------------------------------*/
    xelval * newBrightness;
    xelval i;

    MALLOCARRAY(newBrightness, maxval+1);

    if (newBrightness == NULL)
        pm_error("Unable to allocate memory for transfer function.");

    /* Clip the lowest brightnesses to zero */
    if (bvalue > 0) 
        for (i = 0; i < bvalue; ++i)
            newBrightness[i] = 0;

    /* Map the middle brightnesses linearly onto 0..maxval */
    {
        unsigned int const range = wvalue - bvalue;
        unsigned int val;
        /* The following for loop is a hand optimization of this one:
           for (i = bvalue; i <= wvalue; ++i)
             newBrightness[i] = (i-bvalue)*maxval/range);
           (with proper rounding)
        */
        for (i = bvalue, val = range/2; 
             i <= wvalue; 
             ++i, val += maxval)
            newBrightness[i] = MIN(val / range, maxval);

        assert(newBrightness[bvalue] == 0);
        assert(newBrightness[wvalue] == maxval);
    }

    /* Clip the highest brightnesses to maxval */
    for (i = wvalue+1; i <= maxval; ++i)
        newBrightness[i] = maxval;

    *newBrightnessP = newBrightness;
}
            


static float
brightScaler(xel               const p,
             pixval            const maxval,
             xelval            const newBrightness[],
             enum brightMethod const brightMethod) {
/*----------------------------------------------------------------------------
  Return the multiple by which the brightness pixel of color 'p' (based
  on maxval 'maxval') should be changed according to the transfer
  function newBrightness[], using the 'brightMethod' measure of
  brightness.

  For example, if 'brightMethod' is BRIGHT_LUMINOSITY, p is has
  luminosity 50, and newBrightness[50] is 75, we would return 1.5.
-----------------------------------------------------------------------------*/
    xelval oldBrightness;
    float scaler;
             
    switch (brightMethod) {
    case BRIGHT_LUMINOSITY:
        oldBrightness = PPM_LUMIN(p);
        break;
    case BRIGHT_COLORVALUE:
        oldBrightness = ppm_colorvalue(p);
        break;
    case BRIGHT_SATURATION:
        oldBrightness = ppm_saturation(p, maxval);
        break;
    }
    if (oldBrightness == 0) {
        assert(newBrightness[oldBrightness] == 0);
        /* Doesn't matter what we scale by.  zero times anything is zero. */
        scaler = 1.0;
    } else
        scaler = (float)newBrightness[oldBrightness]/oldBrightness;

    return scaler;
}
            


static void
writeRowNormalized(xel *             const xelrow,
                   int               const cols,
                   xelval            const maxval,
                   int               const format,
                   enum brightMethod const brightMethod,
                   bool              const keephues,
                   xelval            const newBrightness[],
                   xel *             const rowbuf) {
/*----------------------------------------------------------------------------
   Write to Standard Output a normalized version of the xel row 
   'xelrow'.  Normalize it via the transfer function newBrightness[].

   Use 'rowbuf' as a work buffer.  It is at least 'cols' columns wide.
-----------------------------------------------------------------------------*/
    xel * const outrow = rowbuf;
                
    unsigned int col;
    for (col = 0; col < cols; ++col) {
        xel const p = xelrow[col];

        if (PPM_FORMAT_TYPE(format) == PPM_TYPE) {
            if (keephues) {
                float const scaler =
                    brightScaler(p, maxval, newBrightness, brightMethod);

                xelval const r = MIN((int)(PPM_GETR(p)*scaler+0.5), maxval);
                xelval const g = MIN((int)(PPM_GETG(p)*scaler+0.5), maxval);
                xelval const b = MIN((int)(PPM_GETB(p)*scaler+0.5), maxval);
                PNM_ASSIGN(outrow[col], r, g, b);
            } else 
                PNM_ASSIGN(outrow[col], 
                           newBrightness[PPM_GETR(p)], 
                           newBrightness[PPM_GETG(p)], 
                           newBrightness[PPM_GETB(p)]);
        } else 
            PNM_ASSIGN1(outrow[col], newBrightness[PNM_GET1(p)]);
    }
    pnm_writepnmrow(stdout, outrow, cols, maxval, format, 0);
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE *ifP;
    pm_filepos imagePos;
    xelval maxval;
    int rows, cols, format;
    xelval bvalue, wvalue;
    
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr_seekable(cmdline.inputFilespec);

    /* Rescale so that bvalue maps to 0, wvalue maps to maxval. */
    pnm_readpnminit(ifP, &cols, &rows, &maxval, &format);
    pm_tell2(ifP, &imagePos, sizeof(imagePos));

    computeEndValues(ifP, cols, rows, maxval, format, cmdline, 
                     &bvalue, &wvalue);
        
    if (wvalue <= bvalue)
        pm_error("The colors which become black would overlap the "
                 "colors which become white.");
    else {
        xelval * newBrightness;
        int row;
        xel * xelrow;
        xel * rowbuf;
        
        xelrow = pnm_allocrow(cols);

        pm_message("remapping %d..%d to %d..%d", bvalue, wvalue, 0, maxval);

        computeTransferFunction(bvalue, wvalue, maxval, &newBrightness);

        pm_seek2(ifP, &imagePos, sizeof(imagePos));
        pnm_writepnminit(stdout, cols, rows, maxval, format, 0);

        rowbuf = pnm_allocrow(cols);

        for (row = 0; row < rows; ++row) {
            pnm_readpnmrow(ifP, xelrow, cols, maxval, format);
            writeRowNormalized(xelrow, cols, maxval, format,
                               cmdline.brightMethod, cmdline.keephues,
                               newBrightness, rowbuf);
        }
        free(newBrightness);
        pnm_freerow(rowbuf);
        pnm_freerow(xelrow);
    }
    pm_close(ifP);
    return 0;
}
