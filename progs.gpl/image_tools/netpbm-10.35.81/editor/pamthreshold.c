/* pamthreshold - convert a Netpbm image to black and white by thresholding  */

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Copyright (C) 2006 Erik Auerswald
 * auerswal@unix-ag.uni-kl.de */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "mallocvar.h"
#include "nstring.h"
#include "shhopt.h"
#include "pam.h"

#define MAX_ITERATIONS 100             /* stop after at most 100 iterations */

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFileName;
    unsigned int simple;
    float        threshold;
    bool         local;
    bool         dual;
    float        contrast;
    unsigned int width, height;
        /* geometry of local subimage.  Defined only if 'local' or 'dual'
           is true.
        */
};



struct range {
    /* A range of sample values, normalized to [0, 1] */
    samplen min;
    samplen max;
};



static void
initRange(struct range * const rangeP) {

    /* Initialize to "undefined" state */
    rangeP->min = 1.0;
    rangeP->max = 0.0;
}

          

static void
addToRange(struct range * const rangeP,
           samplen        const newSample) {

    rangeP->min = MIN(newSample, rangeP->min);
    rangeP->max = MAX(newSample, rangeP->max);
}



static float
spread(struct range const range) {

    assert(range.max >= range.min);
    
    return range.max - range.min;
}



static void
parseGeometry(const char *   const wxl,
              unsigned int * const widthP,
              unsigned int * const heightP,
              const char **  const errorP) {

    char * const xPos = strchr(wxl, 'x');
    if (!xPos)
        asprintfN(errorP, "There is no 'x'.  It should be WIDTHxHEIGHT");
    else {
        *widthP  = atoi(wxl);
        *heightP = atoi(xPos + 1);

        if (*widthP == 0)
            asprintfN(errorP, "Width is zero.");
        else if (*heightP == 0)
            asprintfN(errorP, "Height is zero.");
        else
            *errorP = NULL;
    }
}



static void
parseCommandLine(int                 argc, 
                 char **             argv,
                 struct cmdlineInfo *cmdlineP ) {
/*----------------------------------------------------------------------------
   Parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    /* vars for the option parser */
    optEntry * option_def;
    optStruct3 opt;
    unsigned int option_def_index = 0;   /* incremented by OPTENT3 */

    unsigned int thresholdSpec, localSpec, dualSpec, contrastSpec;
    const char * localOpt;
    const char * dualOpt;

    MALLOCARRAY_NOFAIL(option_def, 100);

    /* define the options */
    OPTENT3(0, "simple",    OPT_FLAG,   NULL,               
            &cmdlineP->simple,      0);
    OPTENT3(0, "local",     OPT_STRING, &localOpt,
            &localSpec,             0);
    OPTENT3(0, "dual",      OPT_STRING, &dualOpt,
            &dualSpec,              0);
    OPTENT3(0, "threshold", OPT_FLOAT,  &cmdlineP->threshold,
            &thresholdSpec,         0);
    OPTENT3(0, "contrast",  OPT_FLOAT,  &cmdlineP->contrast,
            &contrastSpec,          0);

    /* set the defaults */
    cmdlineP->width = cmdlineP->height = 0U;

    /* set the option description for optParseOptions3 */
    opt.opt_table     = option_def;
    opt.short_allowed = FALSE;           /* long options only */
    opt.allowNegNum   = FALSE;           /* we have no numbers at all */

    /* parse commandline, change argc, argv, and *cmdlineP */
    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

    if (cmdlineP->simple + localSpec + dualSpec > 1)
        pm_error("You may specify only one of -simple, -local, and -dual");

    if (!thresholdSpec)
        cmdlineP->threshold = 0.5;

    /* 0 <= threshold <= 1 */
    if ((cmdlineP->threshold < 0.0) || (cmdlineP->threshold > 1.0))
        pm_error("threshold must be in [0,1]");

    if (!contrastSpec)
        cmdlineP->contrast = 0.05;

    /* 0 <= contrast <= 1 */
    if ((cmdlineP->contrast < 0.0) || (cmdlineP->contrast > 1.0))
        pm_error("contrast must be in [0,1]");

    if (localSpec) {
        const char * error;
        cmdlineP->local = TRUE;

        parseGeometry(localOpt, &cmdlineP->width, &cmdlineP->height, &error);

        if (error) {
            pm_error("Invalid -local value '%s'.  %s", localOpt, error);
            strfree(error);
        }
    } else
        cmdlineP->local = FALSE;

    if (dualSpec) {
        const char * error;
        cmdlineP->dual = TRUE;

        parseGeometry(dualOpt, &cmdlineP->width, &cmdlineP->height, &error);

        if (error) {
            pm_error("Invalid -dual value '%s'.  %s", dualOpt, error);
            strfree(error);
        }
    } else
        cmdlineP->dual = FALSE;

    if (argc-1 < 1)
        cmdlineP->inputFileName = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFileName = argv[1];
    else 
        pm_error("Progam takes at most 1 parameter: the file name.  "
                 "You specified %d", argc-1);
}



/* simple thresholding (the same as in pamditherbw) */

static void
thresholdSimple(struct pam * const inpamP,
                struct pam * const outpamP,
                float        const threshold) {

    tuplen * inrow;    /* normalized input row */
    tuple * outrow;    /* raw output row */
    unsigned int row;  /* number of the current row */

    inrow  = pnm_allocpamrown(inpamP);
    outrow = pnm_allocpamrow(outpamP);

    /* do the simple thresholding */
    for (row = 0; row < inpamP->height; ++row) {
        unsigned int col;
        pnm_readpamrown(inpamP, inrow);
        for (col = 0; col < inpamP->width; ++col)
            outrow[col][0] =
                inrow[col][0] >= threshold ? PAM_BW_WHITE : PAM_BLACK;
        pnm_writepamrow(outpamP, outrow);
    }

    pnm_freepamrow(inrow);
    pnm_freepamrow(outrow);
}



static void
analyzeDistribution(struct pam *          const inpamP,
                    const unsigned int ** const histogramP,
                    struct range *        const rangeP) {
/*----------------------------------------------------------------------------
   Find the distribution of the sample values -- minimum, maximum, and
   how many of each value -- in input image *inpamP, whose file is
   positioned to the raster.

   Return the minimum and maximum as *rangeP and the frequency
   distribution as *histogramP, an array such that histogram[i] is the
   number of pixels that have sample value i.

   Leave the file positioned to the raster.
-----------------------------------------------------------------------------*/
    unsigned int row;
    tuple * inrow;
    tuplen * inrown;
    unsigned int * histogram;  /* malloced array */
    unsigned int i;

    pm_filepos rasterPos;      /* Position in input file of the raster */

    pm_tell2(inpamP->file, &rasterPos, sizeof(rasterPos));

    inrow = pnm_allocpamrow(inpamP);
    inrown = pnm_allocpamrown(inpamP);
    MALLOCARRAY(histogram, inpamP->maxval+1);
    if (histogram == NULL)
        pm_error("Unable to allocate space for %lu-entry histogram",
                 inpamP->maxval+1);

    /* Initialize histogram -- zero occurences of everything */
    for (i = 0; i <= inpamP->maxval; ++i)
        histogram[i] = 0;

    initRange(rangeP);

    for (row = 0; row < inpamP->height; ++row) {
        unsigned int col;
        pnm_readpamrow(inpamP, inrow);
        pnm_normalizeRow(inpamP, inrow, NULL, inrown);
        for (col = 0; col < inpamP->width; ++col) {
            ++histogram[inrow[col][0]];
            addToRange(rangeP, inrown[col][0]);
        }
    }
    *histogramP = histogram;

    pnm_freepamrow(inrow);
    pnm_freepamrown(inrown);

    pm_seek2(inpamP->file, &rasterPos, sizeof(rasterPos));
}



static void
computeWhiteBlackMeans(const unsigned int * const histogram,
                       sample               const maxval,
                       float                const threshold,
                       float *              const meanBlackP,
                       float *              const meanWhiteP) {
/*----------------------------------------------------------------------------
  Assuming that histogram[] and 'maxval' describe the pixels of an image,
  find the mean value of the pixels that are below 'threshold' and
  that are above 'threshold'.
-----------------------------------------------------------------------------*/
    unsigned int nWhite, nBlack;
        /* Number of would-be-black, would-be-white pixels */
    uint64_t sumBlack, sumWhite;
        /* Sum of all the would-be-black, would-be-white pixels */
    sample gray;

    assert(threshold * maxval <= maxval);

    for (gray = 0, nBlack = 0, sumBlack = 0;
         gray < threshold * maxval;
         ++gray) {
        nBlack += histogram[gray];
        sumBlack += gray * histogram[gray];
    }
    for (nWhite = 0, sumWhite = 0; gray <= maxval; ++gray) {
        nWhite += histogram[gray];
        sumWhite += gray * histogram[gray];
    }

    *meanBlackP = (float)sumBlack / nBlack / maxval;
    *meanWhiteP = (float)sumWhite / nWhite / maxval;
}



static void
computeGlobalThreshold(struct pam *         const inpamP,
                       const unsigned int * const histogram,
                       struct range         const globalRange,
                       float *              const thresholdP) {
/*----------------------------------------------------------------------------
   Compute the proper threshold to use for the image described by
   *inpamP, whose file is positioned to the raster.

   For our convenience:

     'histogram' describes the frequency of occurence of the various sample
     values in the image.

     'globalRange' describes the range (minimum, maximum) of sample values
     in the image.

   Return the threshold (scaled to [0, 1]) as *thresholdP.
-----------------------------------------------------------------------------*/
    /* Found this algo in the wikipedia article "Thresholding (image
       processing)."  It is said to be a special one-dimensional case
       of the "k-means clustering algorithm."

       The article claims it's proven to converge, by the way.
       We have an interation limit just as a safety net.

       This code originally implemented a rather different algorithm,
       while nonetheless carrying the comment that it implemented the
       Wikipedia article.  I changed it to match Wikipedia in May 2007
       (at that time there was also a fatal bug in the code, so it
       didn't implement any intentional algorithm).

       In May 2007, the Wikipedia article described an enhancement of
       the algorithm that uses a weighted average.  But that enhancement
       actually reduces the entire thing to: set the threshold to the
       mean pixel value.  It must be some kind of mistake.  We use the
       unenhanced version of the algorithm.
    */

    float threshold;           /* threshold is iteratively determined */
    float oldthreshold;        /* stop if oldthreshold==threshold */
    unsigned int iter;         /* count of done iterations */

    assert(betweenZeroAndOne(globalRange.min));
    assert(betweenZeroAndOne(globalRange.max));

    /* Use middle value (halfway between min and max) as initial threshold */
    threshold = (globalRange.min + globalRange.max) / 2.0;

    oldthreshold = -1.0;  /* initial value */
    iter = 0; /* initial value */

    /* adjust threshold to image */
    while (fabs(oldthreshold - threshold) > 2.0/inpamP->maxval &&
           iter < MAX_ITERATIONS) {
        float meanBlack, meanWhite;

        ++iter;

        computeWhiteBlackMeans(histogram, inpamP->maxval, threshold,
                               &meanBlack, &meanWhite);

        assert(betweenZeroAndOne(meanBlack));
        assert(betweenZeroAndOne(meanWhite));

        oldthreshold = threshold;

        threshold = (meanBlack + meanWhite) / 2;
    }

    assert(betweenZeroAndOne(threshold));

    *thresholdP = threshold;
}



static void
getLocalThreshold(tuplen **    const inrows,
                  unsigned int const windowWidth,
                  unsigned int const x,
                  unsigned int const localWidth,
                  unsigned int const localHeight,
                  float        const darkness,
                  float        const minSpread,
                  samplen      const defaultThreshold,
                  samplen *    const thresholdP) {
/*----------------------------------------------------------------------------
  Find a suitable threshold in local area around one pixel.

  inrows[][] is a an array of 'windowWidth' pixels by 'localHeight'.

  'x' is a column number within the window.

  We look at the rectangle consisting of the 'localWidth' columns
  surrounding x, all rows.  If x is near the left or right edge, we truncate
  the window as needed.

  We base the threshold on the local spread (difference between minimum
  and maximum sample values in the local areas) and the 'darkness'
  factor.  A higher 'darkness' gets a higher threshold.

  If the spread is less than 'minSpread', we return 'defaultThreshold' and
  'darkness' is irrelevant.

  'localWidth' must be odd.
-----------------------------------------------------------------------------*/
    unsigned int const startCol = x >= localWidth/2 ? x - localWidth/2 : 0;

    unsigned int col;
    struct range localRange;

    assert(localWidth % 2 == 1);  /* entry condition */

    initRange(&localRange);

    for (col = startCol; col <= x + localWidth/2 && col < windowWidth; ++col) {
        unsigned int row;

        for (row = 0; row < localHeight; ++row)
            addToRange(&localRange, inrows[row][col][0]);
    }

    if (spread(localRange) < minSpread)
        *thresholdP = defaultThreshold;
    else
        *thresholdP = localRange.min + darkness * spread(localRange);
}



static void
thresholdLocalRow(struct pam *       const inpamP,
                  tuplen **          const inrows,
                  unsigned int       const localWidth,
                  unsigned int       const windowHeight,
                  unsigned int       const row,
                  struct cmdlineInfo const cmdline,
                  struct range       const globalRange,
                  samplen            const globalThreshold,
                  tuple *            const outrow) {

    tuplen * const inrow = inrows[row % windowHeight];

    float minSpread;
    unsigned int col;

    if (cmdline.dual)
        minSpread = cmdline.contrast * spread(globalRange);
    else
        minSpread = 0.0;

    for (col = 0; col < inpamP->width; ++col) {
        samplen threshold;

        getLocalThreshold(inrows, inpamP->width, col, localWidth, windowHeight,
                          cmdline.threshold, minSpread, globalThreshold,
                          &threshold);
        
        outrow[col][0] = inrow[col][0] >= threshold ? PAM_BW_WHITE : PAM_BLACK;
    }
}



static void
thresholdLocal(struct pam *       const inpamP,
               struct pam *       const outpamP,
               struct cmdlineInfo const cmdline) {
/*----------------------------------------------------------------------------
  Threshold the image described by *inpamP, whose file is positioned to the
  raster, and output the resulting raster to the image described by
  *outpamP.

  Use local adaptive thresholding aka dynamic thresholding or dual
  thresholding (global for low contrast areas, LAT otherwise)
-----------------------------------------------------------------------------*/
    struct range globalRange; /* Range of sample values in entire image */
    tuplen ** inrows;
        /* vertical window of image containing the local area.  This is
           a ring of 'windowHeight' rows.  Row R of the image, when it is
           in the window, is inrows[R % windowHeight].
        */
    unsigned int windowHeight;  /* size of 'inrows' window */
    unsigned int nextRowToRead;
        /* Number of the next row to be read from the file into the inrows[]
           buffer.
        */
    tuple * outrow;           /* raw output row */
    unsigned int row;
        /* Number of the current row.  The current row is normally the
           one in the center of the inrows[] buffer (which has an actual
           center row because it is of odd height), but when near the top
           and bottom edge of the image, it is not.
        */
    const unsigned int * histogram;
    samplen globalThreshold;
        /* This is a threshold based on the entire image, to use in areas
           where the contrast is too small to use a locally-derived threshold.
        */
    unsigned int oddLocalWidth;
    unsigned int oddLocalHeight;
    unsigned int i;
    
    /* use a subimage with odd width and height to have a middle pixel */

    if (cmdline.width % 2 == 0)
        oddLocalWidth = cmdline.width + 1;
    else 
        oddLocalWidth = cmdline.width;
    if (cmdline.height % 2 == 0)
        oddLocalHeight = cmdline.height + 1;
    else
        oddLocalHeight = cmdline.height;

    windowHeight = MIN(oddLocalHeight, inpamP->height);

    analyzeDistribution(inpamP, &histogram, &globalRange);

    computeGlobalThreshold(inpamP, histogram, globalRange, &globalThreshold);

    outrow = pnm_allocpamrow(outpamP);

    MALLOCARRAY(inrows, windowHeight);

    if (inrows == NULL)
        pm_error("Unable to allocate memory for a %u-row array", windowHeight);

    for (i = 0; i < windowHeight; ++i)
        inrows[i] = pnm_allocpamrown(inpamP);

    /* Fill the vertical window buffer */
    nextRowToRead = 0;

    while (nextRowToRead < windowHeight)
        pnm_readpamrown(inpamP, inrows[nextRowToRead++ % windowHeight]);

    for (row = 0; row < inpamP->height; ++row) {
        thresholdLocalRow(inpamP, inrows, oddLocalWidth, windowHeight, row,
                          cmdline, globalRange, globalThreshold, outrow);

        pnm_writepamrow(outpamP, outrow);
        
        /* read next image line if available and necessary */
        if (row + windowHeight / 2 >= nextRowToRead &&
            nextRowToRead < inpamP->height)
            pnm_readpamrown(inpamP, inrows[nextRowToRead++ % windowHeight]);
    }

    free((void*)histogram);
    for (i = 0; i < windowHeight; ++i)
        pnm_freepamrow(inrows[i]);
    free(inrows);
    pnm_freepamrow(outrow);
}



static void
thresholdIterative(struct pam * const inpamP,
                   struct pam * const outpamP) {

    const unsigned int * histogram;
    struct range globalRange;
    samplen threshold;

    analyzeDistribution(inpamP, &histogram, &globalRange);

    computeGlobalThreshold(inpamP, histogram, globalRange, &threshold);

    pm_message("using global threshold %4.2f", threshold);

    thresholdSimple(inpamP, outpamP, threshold);
}



int
main(int argc, char **argv) {

    FILE * ifP; 
    struct cmdlineInfo cmdline;
    struct pam inpam, outpam;
    bool eof;  /* No more images in input stream */

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    if (cmdline.simple)
        ifP = pm_openr(cmdline.inputFileName);
    else
        ifP = pm_openr_seekable(cmdline.inputFileName);

    /* threshold each image in the PAM file */
    eof = FALSE;
    while (!eof) {
        pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

        /* set output image parameters for a bilevel image */
        outpam.size        = sizeof(outpam);
        outpam.len         = PAM_STRUCT_SIZE(tuple_type);
        outpam.file        = stdout;
        outpam.format      = PAM_FORMAT;
        outpam.plainformat = 0;
        outpam.height      = inpam.height;
        outpam.width       = inpam.width;
        outpam.depth       = 1;
        outpam.maxval      = 1;
        outpam.bytes_per_sample = 1;
        strcpy(outpam.tuple_type, "BLACKANDWHITE");

        pnm_writepaminit(&outpam);

        /* do the thresholding */

        if (cmdline.simple)
            thresholdSimple(&inpam, &outpam, cmdline.threshold);
        else if (cmdline.local || cmdline.dual)
            thresholdLocal(&inpam, &outpam, cmdline);
        else
            thresholdIterative(&inpam, &outpam);

        pnm_nextimage(ifP, &eof);
    }

    pm_close(ifP);

    return 0;
}
