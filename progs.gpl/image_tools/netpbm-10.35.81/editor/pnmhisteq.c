/*
                 pnmhisteq.c

           Equalize histogram for a PNM image

  By Bryan Henderson 2005.09.10, based on ideas from the program of
  the same name by John Walker (kelvin@fourmilab.ch) -- March MVM.
  WWW home page: http://www.fourmilab.ch/ in 1995.

  This program is contributed to the public domain by its author.
*/

#include <string.h>

#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFileName;
    unsigned int gray;
    const char * wmap;
    const char * rmap;
    unsigned int verbose;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    unsigned int rmapSpec, wmapSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "rmap",     OPT_STRING, &cmdlineP->rmap, 
            &rmapSpec,          0);
    OPTENT3(0, "wmap",     OPT_STRING, &cmdlineP->wmap, 
            &wmapSpec,          0);
    OPTENT3(0, "gray",     OPT_FLAG,   NULL,
            &cmdlineP->gray,    0);
    OPTENT3(0, "verbose",  OPT_FLAG,   NULL,
            &cmdlineP->verbose, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (!wmapSpec)
        cmdlineP->wmap = NULL;
    if (!rmapSpec)
        cmdlineP->rmap = NULL;

    if (argc-1 < 1)
        cmdlineP->inputFileName = "-";
    else {
        cmdlineP->inputFileName = argv[1];
        if (argc-1 > 1)
            pm_error("Too many arguments (%d).  The only argument is the "
                     "input file name.", argc-1);
    }
}



static void
computeLuminosityHistogram(xel * const *   const xels,
                           unsigned int    const rows,
                           unsigned int    const cols,
                           xelval          const maxval,
                           int             const format,
                           bool            const monoOnly,
                           unsigned int ** const lumahistP,
                           xelval *        const lminP,
                           xelval *        const lmaxP,
                           unsigned int *  const pixelCountP) {
/*----------------------------------------------------------------------------
  Scan the image and build the luminosity histogram.  If the input is
  a PPM, we calculate the luminosity of each pixel from its RGB
  components.
-----------------------------------------------------------------------------*/
    xelval lmin, lmax;
    unsigned int pixelCount;
    unsigned int * lumahist;

    MALLOCARRAY(lumahist, maxval + 1);
    if (lumahist == NULL)
        pm_error("Out of storage allocating array for %u histogram elements",
                 maxval + 1);

    {
        unsigned int i;
        /* Initialize histogram to zeroes everywhere */
        for (i = 0; i <= maxval; ++i)
            lumahist[i] = 0;
    }
    lmin = maxval;  /* initial value */
    lmax = 0;       /* initial value */

    switch (PNM_FORMAT_TYPE(format)) {
    case PGM_TYPE:
    case PBM_TYPE: {
        /* Compute intensity histogram */

        unsigned int row;

        pixelCount = rows * cols;
        for (row = 0; row < rows; ++row) {
            unsigned int col;
            for (col = 0; col < cols; ++col) {
                xelval const l = PNM_GET1(xels[row][col]);
                lmin = MIN(lmin, l);
                lmax = MAX(lmax, l);
                ++lumahist[l];
            }
        }
    }
    break;
    case PPM_TYPE: {
        unsigned int row;

        for (row = 0, pixelCount = 0; row < rows; ++row) {
            unsigned int col;
            for (col = 0; col < cols; ++col) {
                xel const thisXel = xels[row][col];
                if (!monoOnly || PPM_ISGRAY(thisXel)) {
                    xelval const l = PPM_LUMIN(thisXel);

                    lmin = MIN(lmin, l);
                    lmax = MAX(lmax, l);

                    ++lumahist[l];
                    ++pixelCount;
                }
            }
        }
    }
    break;
    default:
        pm_error("invalid input format format");
    }

    *lumahistP = lumahist;
    *pixelCountP = pixelCount;
    *lminP = lmin;
    *lmaxP = lmax;
}



static void
findMaxLuma(const xelval * const lumahist,
            xelval         const maxval,
            xelval *       const maxLumaP) {

    xelval maxluma;
    unsigned int i;

    for (i = 0, maxluma = 0; i <= maxval; ++i)
        if (lumahist[i] > 0)
            maxluma = i;

    *maxLumaP = maxluma;
}



static void
readMapFile(const char * const rmapFileName,
            xelval       const maxval,
            gray *       const lumamap) {

    int rmcols, rmrows; 
    gray rmmaxv;
    int rmformat;
    FILE * rmapfP;
        
    rmapfP = pm_openr(rmapFileName);
    pgm_readpgminit(rmapfP, &rmcols, &rmrows, &rmmaxv, &rmformat);
    
    if (rmmaxv != maxval)
        pm_error("maxval in map file (%u) different from input (%u)",
                 rmmaxv, maxval);
    
    if (rmrows != 1)
        pm_error("Map must have 1 row.  Yours has %u", rmrows);
    
    if (rmcols != maxval + 1)
        pm_error("Map must have maxval + 1 (%u) columns.  Yours has %u",
                 maxval + 1, rmcols);
    
    pgm_readpgmrow(rmapfP, lumamap, maxval+1, rmmaxv, rmformat);
    
    pm_close(rmapfP);
}



static void
computeMap(const unsigned int * const lumahist,
           xelval               const maxval,
           unsigned int         const pixelCount,
           gray *               const lumamap) {

    /* Calculate initial histogram equalization curve. */
    
    unsigned int i;
    unsigned int pixsum;
    xelval maxluma;

    for (i = 0, pixsum = 0; i <= maxval; ++i) {
            
        /* With 16 bit grays, the following calculation can
           overflow a 32 bit long.  So, we do it in floating
           point.
        */

        lumamap[i] = ROUNDU((((double) pixsum * maxval)) / pixelCount);

        pixsum += lumahist[i];
    }

    findMaxLuma(lumahist, maxval, &maxluma);

    {
        double const lscale = (double)maxval /
            ((lumahist[maxluma] > 0) ?
             (double) lumamap[maxluma] : (double) maxval);

        unsigned int i;

        /* Normalize so that the brightest pixels are set to maxval. */

        for (i = 0; i <= maxval; ++i)
            lumamap[i] = MIN(maxval, ROUNDU(lumamap[i] * lscale));
    }
}



static void
getMapping(const char *         const rmapFileName,
           const unsigned int * const lumahist,
           xelval               const maxval,
           unsigned int         const pixelCount,
           gray **              const lumamapP) {
/*----------------------------------------------------------------------------
  Calculate the luminosity mapping table which gives the
  histogram-equalized luminosity for each original luminosity.
-----------------------------------------------------------------------------*/
    gray * lumamap;

    lumamap = pgm_allocrow(maxval+1);

    if (rmapFileName)
        readMapFile(rmapFileName, maxval, lumamap);
    else
        computeMap(lumahist, maxval, pixelCount, lumamap);

    *lumamapP = lumamap;
}



static void
reportMap(const unsigned int * const lumahist,
          xelval               const maxval,
          const gray *         const lumamap) {

    unsigned int i;

    fprintf(stderr, "  Luminosity map    Number of\n");
    fprintf(stderr, " Original    New     Pixels  \n");

    for (i = 0; i <= maxval; ++i) {
        if (lumahist[i] > 0) {
            fprintf(stderr,"%6d -> %6d  %8u\n", i,
                    lumamap[i], lumahist[i]);
        }
    }
}



static void
remap(xel **       const xels,
      unsigned int const cols,
      unsigned int const rows,
      xelval       const maxval,
      int          const format,
      bool         const monoOnly,
      const gray * const lumamap) {
/*----------------------------------------------------------------------------
   Update the array 'xels' to have the new intensities.
-----------------------------------------------------------------------------*/
    switch (PNM_FORMAT_TYPE(format)) {
    case PPM_TYPE: {
        unsigned int row;
        for (row = 0; row < rows; ++row) {
            unsigned int col;
            for (col = 0; col < cols; ++col) {
                xel const thisXel = xels[row][col];
                if (monoOnly && PPM_ISGRAY(thisXel)) {
                    /* Leave this pixel alone */
                } else {
                    struct hsv hsv;
                    xelval iv;

                    hsv = ppm_hsv_from_color(thisXel, maxval);
                    iv = MIN(maxval, ROUNDU(hsv.v * maxval));
                    
                    hsv.v = MIN(1.0, 
                                ((double) lumamap[iv]) / ((double) maxval));

                    xels[row][col] = ppm_color_from_hsv(hsv, maxval);
                }
            }
        }
    }
    break;

    case PBM_TYPE:
    case PGM_TYPE: {
        unsigned int row;
        for (row = 0; row < rows; ++row) {
            unsigned int col;
            for (col = 0; col < cols; ++col)
                PNM_ASSIGN1(xels[row][col],
                            lumamap[PNM_GET1(xels[row][col])]);
        }
    }
    break;
    }
}



static void
writeMap(const char * const wmapFileName,
         const gray * const lumamap,
         xelval       const maxval) {

    FILE * const wmapfP = pm_openw(wmapFileName);

    pgm_writepgminit(wmapfP, maxval+1, 1, maxval, 0);

    pgm_writepgmrow(wmapfP, lumamap, maxval+1, maxval, 0);

    pm_close(wmapfP);
}



int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    xelval lmin, lmax;
    gray * lumamap;           /* Luminosity map */
    unsigned int * lumahist;  /* Histogram of luminosity values */
    int rows, cols;           /* Rows, columns of input image */
    xelval maxval;            /* Maxval of input image */
    int format;               /* Format indicator (PBM/PGM/PPM) */
    xel ** xels;              /* Pixel array */
    unsigned int pixelCount;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFileName);

    xels = pnm_readpnm(ifP, &cols, &rows, &maxval, &format);

    pm_close(ifP);

    computeLuminosityHistogram(xels, rows, cols, maxval, format,
                               cmdline.gray, &lumahist, &lmin, &lmax,
                               &pixelCount);

    getMapping(cmdline.rmap, lumahist, maxval, pixelCount, &lumamap);

    if (cmdline.verbose)
        reportMap(lumahist, maxval, lumamap);

    remap(xels, cols, rows, maxval, format, !!cmdline.gray, lumamap);

    pnm_writepnm(stdout, xels, cols, rows, maxval, format, 0);

    if (cmdline.wmap)
        writeMap(cmdline.wmap, lumamap, maxval);

    pgm_freerow(lumamap);

    return 0;
}
