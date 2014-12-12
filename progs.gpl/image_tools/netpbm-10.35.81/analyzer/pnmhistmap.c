/* pnmhistmap.c -
 *  Draw a histogram for a PGM or PPM file
 *
 * Options: -verbose: the usual
 *      -max N: force scaling value to N
 *      -black: ignore all-black count
 *      -white: ignore all-white count
 *
 * - PGM histogram is a PBM file, PPM histogram is a PPM file
 * - No conditional code - assumes all three: PBM, PGM, PPM
 *
 * Copyright (C) 1993 by Wilson H. Bent, Jr (whb@usc.edu)
 *
 * 2004-12-11 john h. dubois iii (john@armory.com)
 * - Added options:
 *   -dots, -nmax, -red, -green, -blue, -width, -height, -lval, -rval
 * - Deal properly with maxvals other than 256
 */

#include <string.h>

#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"

static double const epsilon = .00001;

#define SCALE_H(value) (hscale_unity ? (value) : (int)((value) * hscale))

enum wantedColor {WANT_RED=0, WANT_GRN=1, WANT_BLU=2};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input files */
    unsigned int black;
    unsigned int white;
    unsigned int dots;
    bool         colorWanted[3];
        /* subscript is enum wantedColor */
    unsigned int verbose;
    unsigned int nmaxSpec;
    float        nmax;
    unsigned int lval;
    unsigned int rval;
    unsigned int widthSpec;
    unsigned int width;
    unsigned int height;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    unsigned int lvalSpec, rvalSpec, heightSpec;
    unsigned int redSpec, greenSpec, blueSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "black",     OPT_FLAG, NULL, &cmdlineP->black,   0);
    OPTENT3(0, "white",     OPT_FLAG, NULL, &cmdlineP->white,   0);
    OPTENT3(0, "dots",      OPT_FLAG, NULL, &cmdlineP->dots,    0);
    OPTENT3(0, "red",       OPT_FLAG, NULL, &redSpec,           0);
    OPTENT3(0, "green",     OPT_FLAG, NULL, &greenSpec,         0);
    OPTENT3(0, "blue",      OPT_FLAG, NULL, &blueSpec,          0);
    OPTENT3(0, "verbose",   OPT_FLAG, NULL, &cmdlineP->verbose, 0);
    OPTENT3(0, "nmax",      OPT_FLOAT, &cmdlineP->nmax,
            &cmdlineP->nmaxSpec,   0);
    OPTENT3(0, "lval",      OPT_UINT, &cmdlineP->lval,
            &lvalSpec,             0);
    OPTENT3(0, "rval",      OPT_UINT, &cmdlineP->rval,
            &rvalSpec,             0);
    OPTENT3(0, "width",     OPT_UINT, &cmdlineP->width,
            &cmdlineP->widthSpec,  0);
    OPTENT3(0, "height",    OPT_UINT, &cmdlineP->height,
            &heightSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!lvalSpec)
        cmdlineP->lval = 0;
    if (!rvalSpec)
        cmdlineP->rval = PNM_OVERALLMAXVAL;

    if (!redSpec && !greenSpec && !blueSpec) {
        cmdlineP->colorWanted[WANT_RED] = TRUE;
        cmdlineP->colorWanted[WANT_GRN] = TRUE;
        cmdlineP->colorWanted[WANT_BLU] = TRUE;
    } else {
        cmdlineP->colorWanted[WANT_RED] = redSpec;
        cmdlineP->colorWanted[WANT_GRN] = greenSpec;
        cmdlineP->colorWanted[WANT_BLU] = blueSpec;
    }

    if (!heightSpec)
        cmdlineP->height = 200;

    if (argc-1 == 0) 
        cmdlineP->inputFilespec = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->inputFilespec = argv[1];
}



static unsigned int
maxSlotCount(const unsigned int * const hist,
             unsigned int         const hist_width,
             bool                 const no_white,
             bool                 const no_black) {
/*----------------------------------------------------------------------------
   Return the maximum count among all the slots in hist[], not counting
   the first and last as suggested by 'no_white' and 'no_black'.
-----------------------------------------------------------------------------*/
    unsigned int hmax;
    unsigned int i;

    unsigned int const start = (no_black ? 1 : 0);
    unsigned int const finish = (no_white ? hist_width - 1 : hist_width);
    for (hmax = 0, i = start; i < finish; ++i)
        if (hmax < hist[i])
            hmax = hist[i];

    return hmax;
}



static void
clipHistogram(unsigned int * const hist,
              unsigned int   const hist_width,
              unsigned int   const hmax) {

            unsigned int i;

            for (i = 0; i < hist_width; ++i)
                hist[i] = MIN(hmax, hist[i]);
}



static void
pgm_hist(FILE *       const fp,
         int          const cols,
         int          const rows,
         xelval       const maxval,
         int          const format,
         bool         const dots,
         bool         const no_white,
         bool         const no_black,
         bool         const verbose,
         xelval       const startval,
         xelval       const endval,
         unsigned int const hist_width,
         unsigned int const hist_height,
         bool         const clipSpec,
         unsigned int const clipCount,
         double       const hscale) {

    bool const hscale_unity = hscale - 1 < epsilon;

    gray * grayrow;
    bit ** bits;
    int i, j;
    unsigned int * ghist;
    double vscale;
    unsigned int hmax;

    if ((ghist = calloc(hist_width, sizeof(int))) == NULL)
        pm_error ("Not enough memory for histogram array (%d bytes)",
                  hist_width * sizeof(int));
    if ((bits = pbm_allocarray (hist_width, hist_height)) == NULL)
        pm_error ("no space for output array (%d bits)",
                  hist_width * hist_height);
    memset (ghist, 0, sizeof (ghist));

    /* read the pixel values into the histogram arrays */
    grayrow = pgm_allocrow (cols);
    /*XX error-check! */
    if (verbose) pm_message ("making histogram...");
    for (i = rows; i > 0; --i) {
        pgm_readpgmrow (fp, grayrow, cols, maxval, format);
        for (j = cols-1; j >= 0; --j) {
            int value;

            if ((value = grayrow[j]) >= startval && value <= endval)
                ghist[SCALE_H(value-startval)]++;
        }
    }
    pgm_freerow (grayrow);
    fclose (fp);

    /* find the highest-valued slot and set the vertical scale value */
    if (verbose)
        pm_message ("finding max. slot height...");
    if (clipSpec)
        hmax = clipCount;
    else 
        hmax = maxSlotCount(ghist, hist_width, no_white, no_black);

    if (verbose)
        pm_message ("Done: height = %u", hmax);

    clipHistogram(ghist, hist_width, hmax);

    vscale = (double) hist_height / hmax;

    for (i = 0; i < hist_width; ++i) {
        int mark = hist_height - (int)(vscale * ghist[i]);
        for (j = 0; j < mark; ++j)
            bits[j][i] = PBM_BLACK;
        if (j < hist_height)
            bits[j++][i] = PBM_WHITE;
        for ( ; j < hist_height; ++j)
            bits[j][i] = dots ? PBM_BLACK : PBM_WHITE;
    }

    pbm_writepbm (stdout, bits, hist_width, hist_height, 0);
}



static unsigned int
maxSlotCountAll(unsigned int *       const hist[3],
                unsigned int         const hist_width,
                bool                 const no_white,
                bool                 const no_black) {
/*----------------------------------------------------------------------------
   Return the maximum count among all the slots in hist[x] not
   counting the first and last as suggested by 'no_white' and
   'no_black'.  hist[x] may be NULL to indicate none.
-----------------------------------------------------------------------------*/
    unsigned int hmax;
    unsigned int color;

    hmax = 0;

    for (color = 0; color < 3; ++color)
        if (hist[color])
            hmax = MAX(hmax, 
                       maxSlotCount(hist[color], 
                                    hist_width, no_white, no_black));
    
    return hmax;
}



static void
createHist(bool             const colorWanted[3],
           unsigned int     const hist_width,
           unsigned int * (* const histP)[3]) {
/*----------------------------------------------------------------------------
   Allocate the histogram arrays and set each slot count to zero.
-----------------------------------------------------------------------------*/
    unsigned int color;

    for (color = 0; color < 3; ++color)
        if (colorWanted[color]) {
            unsigned int * hist;
            unsigned int i;
            MALLOCARRAY(hist, hist_width);
            if (hist == NULL)
                pm_error ("Not enough memory for histogram arrays (%u bytes)",
                          hist_width * sizeof(int) * 3);

            for (i = 0; i < hist_width; ++i)
                hist[i] = 0;
            (*histP)[color] = hist;
        } else
            (*histP)[color] = NULL;
}



static void
clipHistogramAll(unsigned int * const hist[3],
                 unsigned int   const hist_width,
                 unsigned int   const hmax) {

    unsigned int color;

    for (color = 0; color < 3; ++color)
        if (hist[color])
            clipHistogram(hist[color], hist_width, hmax);
}



static void
ppm_hist(FILE *       const fp,
         int          const cols,
         int          const rows,
         xelval       const maxval,
         int          const format,
         bool         const dots,
         bool         const no_white,
         bool         const no_black,
         bool         const colorWanted[3],
         bool         const verbose,
         xelval       const startval,
         xelval       const endval,
         unsigned int const hist_width,
         unsigned int const hist_height,
         bool         const clipSpec,
         unsigned int const clipCount,
         double       const hscale) {

    bool const hscale_unity = hscale - 1 < epsilon;

    pixel *pixrow;
    pixel **pixels;
    int i, j;
    unsigned int * hist[3];  /* Subscript is enum wantedColor */
    double vscale;
    unsigned int hmax;

    createHist(colorWanted, hist_width, &hist);

    if ((pixels = ppm_allocarray (hist_width, hist_height)) == NULL)
        pm_error ("no space for output array (%d pixels)",
                  hist_width * hist_height);
    for (i = 0; i < hist_height; ++i)
        memset (pixels[i], 0, hist_width * sizeof (pixel));

    /* read the pixel values into the histogram arrays */
    pixrow = ppm_allocrow (cols);
    /*XX error-check! */
    if (verbose) pm_message ("making histogram...");
    for (i = rows; i > 0; --i) {
        ppm_readppmrow (fp, pixrow, cols, maxval, format);
        for (j = cols-1; j >= 0; --j) {
            int value;

            if (colorWanted[WANT_RED] && 
                (value = PPM_GETR(pixrow[j])) >= startval && 
                value <= endval)
                hist[WANT_RED][SCALE_H(value-startval)]++;
            if (colorWanted[WANT_GRN] && 
                (value = PPM_GETG(pixrow[j])) >= startval && 
                value <= endval)
                hist[WANT_GRN][SCALE_H(value-startval)]++;
            if (colorWanted[WANT_BLU] && 
                (value = PPM_GETB(pixrow[j])) >= startval && 
                value <= endval)
                hist[WANT_BLU][SCALE_H(value-startval)]++;
        }
    }
    ppm_freerow (pixrow);
    fclose (fp);

    /* find the highest-valued slot and set the vertical scale value */
    if (verbose)
        pm_message ("finding max. slot height...");
    if (clipSpec)
        hmax = clipCount;
    else 
        hmax = maxSlotCountAll(hist, hist_width, no_white, no_black);

    clipHistogramAll(hist, hist_width, hmax);

    vscale = (double) hist_height / hmax;
    if (verbose)
        pm_message("Done: height = %d, vertical scale factor = %g", 
                   hmax, vscale);

    for (i = 0; i < hist_width; ++i) {
        if (hist[WANT_RED]) {
            unsigned int j;
            bool plotted;
            plotted = FALSE;
            for (j = hist_height - (int)(vscale * hist[WANT_RED][i]); 
                 j < hist_height && !plotted; 
                 ++j) {
                PPM_PUTR(pixels[j][i], maxval);
                plotted = dots;
            }
        }
        if (hist[WANT_GRN]) {
            unsigned int j;
            bool plotted;
            plotted = FALSE;
            for (j = hist_height - (int)(vscale * hist[WANT_GRN][i]); 
                 j < hist_height && !plotted; 
                 ++j) {
                PPM_PUTG(pixels[j][i], maxval);
                plotted = dots;
            }
        }
        if (hist[WANT_BLU]) {
            unsigned int j;
            bool plotted;
            plotted = FALSE;
            for (j = hist_height - (int)(vscale * hist[WANT_BLU][i]); 
                 j < hist_height && !plotted; 
                 ++j) {
                PPM_PUTB(pixels[j][i], maxval);
                plotted = dots;
            }
        }
    }
    ppm_writeppm (stdout, pixels, hist_width, hist_height, maxval, 0);
}



int
main (int argc, char ** argv) {

    struct cmdlineInfo cmdline;
    FILE* ifP;
    int cols, rows;
    xelval maxval;
    int format;
    unsigned int hist_width;
    unsigned int range;
    double hscale;
    int hmax;

    pnm_init (&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    pnm_readpnminit(ifP, &cols, &rows, &maxval, &format);

    range = MIN(maxval, cmdline.rval) - cmdline.lval + 1;

    if (cmdline.widthSpec)
        hist_width = cmdline.width;
    else
        hist_width = range;

    hscale = (float)(hist_width-1) / (range-1);
    if (hscale - 1.0 < epsilon && cmdline.verbose)
        pm_message("Horizontal scale factor: %g (maxval = %u)", 
                   hscale, maxval);

    if (cmdline.nmaxSpec)
        hmax = cols * rows / hist_width * cmdline.nmax;

    switch (PNM_FORMAT_TYPE(format)) {
    case PPM_TYPE:
        ppm_hist(ifP, cols, rows, maxval, format,
                 cmdline.dots, cmdline.white, cmdline.black,
                 cmdline.colorWanted,
                 cmdline.verbose, cmdline.lval, cmdline.rval, 
                 hist_width, cmdline.height, cmdline.nmaxSpec, hmax, hscale);
        break;
    case PGM_TYPE:
        pgm_hist(ifP, cols, rows, maxval, format,
                 cmdline.dots, cmdline.white, cmdline.black,
                 cmdline.verbose, cmdline.lval, cmdline.rval,
                 hist_width, cmdline.height, cmdline.nmaxSpec, hmax, hscale);
        break;
    case PBM_TYPE:
        pm_error("Cannot do a histogram of a a PBM file");
        break;
    }
    return 0;
}
