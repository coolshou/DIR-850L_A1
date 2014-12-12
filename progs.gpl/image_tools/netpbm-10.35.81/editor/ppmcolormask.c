/*=========================================================================
                             ppmcolormask
===========================================================================

  This program produces a PBM mask of areas containing a certain color.

  By Bryan Henderson, Olympia WA; April 2000.

  Contributed to the public domain by its author.
=========================================================================*/

#define _BSD_SOURCE  /* Make sure strdup() is in <string.h> */
#include <assert.h>
#include <string.h>

#include "pm_c_util.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"
#include "ppm.h"
#include "pbm.h"

enum matchType {
    MATCH_EXACT,
    MATCH_BK
};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilename;
    unsigned int colorCount;
    struct {
        enum matchType matchType;
        union {
            pixel    color;   /* matchType == MATCH_EXACT */
            bk_color bkColor; /* matchType == MATCH_BK */
        } u;
    } maskColor[16];
    unsigned int verbose;
};



static void
parseColorOpt(const char *         const colorOpt,
              struct cmdlineInfo * const cmdlineP) {

    unsigned int colorCount;
    char * colorOptWork;
    char * cursor;
    bool eol;
    
    colorOptWork = strdup(colorOpt);
    cursor = &colorOptWork[0];
    
    eol = FALSE;    /* initial value */
    colorCount = 0; /* initial value */
    while (!eol && colorCount < ARRAY_SIZE(cmdlineP->maskColor)) {
        const char * token;
        token = strsepN(&cursor, ",");
        if (token) {
            if (STRNEQ(token, "bk:", 3)) {
                cmdlineP->maskColor[colorCount].matchType = MATCH_BK;
                cmdlineP->maskColor[colorCount].u.bkColor =
                    ppm_bk_color_from_name(&token[3]);
            } else {
                cmdlineP->maskColor[colorCount].matchType = MATCH_EXACT;
                cmdlineP->maskColor[colorCount].u.color =
                    ppm_parsecolor(token, PPM_MAXMAXVAL);
            }
            ++colorCount;
        } else
            eol = TRUE;
    }
    free(colorOptWork);

    cmdlineP->colorCount = colorCount;
}



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that many of the strings that this function returns in the
   *cmdlineP structure are actually in the supplied argv array.  And
   sometimes, one of these strings is actually just a suffix of an entry
   in argv!
-----------------------------------------------------------------------------*/
    optEntry * option_def;
        /* Instructions to OptParseOptions3 on how to parse our options. */
    optStruct3 opt;

    unsigned int option_def_index;
    const char * colorOpt;
    unsigned int colorSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "color",      OPT_STRING, &colorOpt, &colorSpec,           0);
    OPTENT3(0, "verbose",    OPT_FLAG,   NULL, &cmdlineP->verbose,        0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and all of *cmdlineP. */

    if (colorSpec)
        parseColorOpt(colorOpt, cmdlineP);

    if (colorSpec) {
        if (argc-1 < 1)
            cmdlineP->inputFilename = "-";  /* he wants stdin */
        else if (argc-1 == 1)
            cmdlineP->inputFilename = argv[1];
        else
            pm_error("Too many arguments.  When you specify -color, "
                     "the only argument accepted is the optional input "
                     "file name.");
    } else {
        if (argc-1 < 1)
            pm_error("You must specify the -color option.");
        else {
            cmdlineP->colorCount = 1;
            cmdlineP->maskColor[0].matchType = MATCH_EXACT;
            cmdlineP->maskColor[0].u.color =
                ppm_parsecolor(argv[1], PPM_MAXMAXVAL);

            if (argc - 1 < 2)
                cmdlineP->inputFilename = "-";  /* he wants stdin */
            else if (argc-1 == 2)
                cmdlineP->inputFilename = argv[2];
            else 
                pm_error("Too many arguments.  The only arguments accepted "
                         "are the mask color and optional input file name");
        }
    }
}



static bool
isBkColor(pixel    const comparator,
          pixval   const maxval,
          bk_color const comparand) {

    /* TODO: keep a cache of the bk color for each color in
       a colorhash_table.
    */
    
    bk_color const comparatorBk = ppm_bk_color_from_color(comparator, maxval);

    return comparatorBk == comparand;
}



static bool
colorIsInSet(pixel              const color,
             pixval             const maxval,
             struct cmdlineInfo const cmdline) {

    bool isInSet;
    unsigned int i;

    for (i = 0, isInSet = FALSE;
         i < cmdline.colorCount && !isInSet; ++i) {

        assert(i < ARRAY_SIZE(cmdline.maskColor));

        switch(cmdline.maskColor[i].matchType) {
        case MATCH_EXACT:
            if (PPM_EQUAL(color, cmdline.maskColor[i].u.color))
                isInSet = TRUE;
            break;
        case MATCH_BK:
            if (isBkColor(color, maxval, cmdline.maskColor[i].u.bkColor))
                isInSet = TRUE;
            break;
        }
    }
    return isInSet;
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;

    FILE * ifP;

    /* Parameters of input image: */
    int rows, cols;
    pixval maxval;
    int format;

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilename);

    ppm_readppminit(ifP, &cols, &rows, &maxval, &format);
    pbm_writepbminit(stdout, cols, rows, 0);
    {
        pixel * const inputRow = ppm_allocrow(cols);
        bit *   const maskRow  = pbm_allocrow(cols);

        unsigned int numPixelsMasked;

        unsigned int row;
        for (row = 0, numPixelsMasked = 0; row < rows; ++row) {
            int col;
            ppm_readppmrow(ifP, inputRow, cols, maxval, format);
            for (col = 0; col < cols; ++col) {
                if (colorIsInSet(inputRow[col], maxval, cmdline)) {
                    maskRow[col] = PBM_BLACK;
                    ++numPixelsMasked;
                } else 
                    maskRow[col] = PBM_WHITE;
            }
            pbm_writepbmrow(stdout, maskRow, cols, 0);
        }

        if (cmdline.verbose)
            pm_message("%u pixels found matching %u requested colors",
                       numPixelsMasked, cmdline.colorCount);

        pbm_freerow(maskRow);
        ppm_freerow(inputRow);
    }
    pm_close(ifP);

    return 0;
}



