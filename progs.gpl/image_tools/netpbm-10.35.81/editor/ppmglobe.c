/*
 * This code written 2003
 * by Max Gensthaler <Max@Gensthaler.de>
 * Distributed under the Gnu Public License (GPL)
 *
 * Gensthaler called it 'ppmglobemap'.
 *
 * Translations of comments and C dialect by Bryan Henderson May 2003.
 */


#define _XOPEN_SOURCE  /* get M_PI in math.h */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "ppm.h"
#include "colorname.h"
#include "shhopt.h"
#include "mallocvar.h"


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFileName;  /* Filename of input files */
    unsigned int stripcount;
    const char * background;
    unsigned int closeok;
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

    unsigned int backgroundSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "background",     OPT_STRING, &cmdlineP->background, 
            &backgroundSpec, 0);
    OPTENT3(0, "closeok",        OPT_FLAG, NULL,
            &cmdlineP->closeok, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!backgroundSpec)
        cmdlineP->background = NULL;

    if (argc - 1 < 1) 
        pm_error("You must specify at least one argument:  the strip count");
    else {
        int const stripcount = atoi(argv[1]);
        if (stripcount <= 0)
            pm_error("The strip count must be positive.  You specified %d",
                     stripcount);
            
        cmdlineP->stripcount = stripcount;

        if (argc-1 < 2)
            cmdlineP->inputFileName = "-";
        else
            cmdlineP->inputFileName = argv[2];
    
        if (argc - 1 > 2)
            pm_error("There are at most two arguments: strip count "
                     "and input file name.  "
                     "You specified %u", argc-1);
    }
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    pixel ** srcPixels;
    pixel ** dstPixels;
    int srcCols, srcRows;
    unsigned int dstCols, dstRows;
    pixval srcMaxval, dstMaxval;
    unsigned int stripWidth;
        /* Width in pixels of each strip.  In the output image, this means
           the rectangular strip in which the lens-shaped foreground strip
           is placed..
        */
    unsigned int row;
    pixel backgroundColor;

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
    
    ifP = pm_openr(cmdline.inputFileName);
    
    srcPixels = ppm_readppm(ifP, &srcCols, &srcRows, &srcMaxval);

    pm_close(ifP);

    stripWidth = srcCols / cmdline.stripcount;

    if (stripWidth < 1)
        pm_error("You asked for %u strips, but the image is only "
                 "%u pixels wide, so that is impossible.",
                 cmdline.stripcount, srcCols);

    dstCols   = stripWidth * cmdline.stripcount;
    dstRows   = srcRows;
    dstMaxval = srcMaxval;

    if (cmdline.background == NULL)
        PPM_ASSIGN(backgroundColor, 0, 0, 0);
    else
        pm_parse_dictionary_name(cmdline.background,
                                 dstMaxval, cmdline.closeok,
                                 &backgroundColor);

    dstPixels = ppm_allocarray(dstCols, dstRows);
    
    for (row = 0; row < dstRows; ++row) {
        double const factor = sin(M_PI * row / dstRows);
            /* Amount by which we squeeze the foreground image of each
               strip in this row.
            */
        int const stripBorder = (int)((stripWidth*(1.0-factor)/2.0) + 0.5);
            /* Distance from the edge (either one) of a strip to the
               foreground image within that strip -- i.e. number of pixels
               of background color, which User will cut out with scissors
               after he prints the image.
            */
        unsigned int dstCol;

        for (dstCol = 0; dstCol < dstCols; ++dstCol) {
            if (dstCol % stripWidth < stripBorder
                || dstCol % stripWidth >= stripWidth - stripBorder)
                dstPixels[row][dstCol] = backgroundColor;
            else {
                unsigned int const leftEdge =
                    (dstCol / stripWidth) * stripWidth;
                unsigned int const srcCol = leftEdge +
                    (int)((dstCol % stripWidth - stripBorder) / factor + 0.5);
                dstPixels[row][dstCol] = srcPixels[row][srcCol];
            }
        }
    }

    ppm_writeppm(stdout, dstPixels, dstCols, dstRows, dstMaxval, 0);

    return 0;
}
