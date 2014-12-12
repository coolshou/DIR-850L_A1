/* ----------------------------------------------------------------------
 *
 * Create a single image stereogram from a height map.
 * by Scott Pakin <scott+pbm@pakin.org>
 * Adapted to Netbpm conventions by Bryan Henderson.
 * Revised by Scott Pakin.
 *
 * The core of this program is a simple adaptation of the code in
 * "Displaying 3D Images: Algorithms for Single Image Random Dot
 * Stereograms" by Harold W. Thimbleby, Stuart Inglis, and Ian
 * H. Witten in IEEE Computer, 27(10):38-48, October 1994.  See that
 * paper for a thorough explanation of what's going on here.
 *
 * ----------------------------------------------------------------------
 *
 * Copyright (C) 2006 Scott Pakin <scott+pbm@pakin.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ----------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "pm_config.h"
#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

/* Define a few helper macros. */
#define round2int(X) ((int)((X)+0.5))      /* Nonnegative numbers only */

enum outputType {OUTPUT_BW, OUTPUT_GRAYSCALE, OUTPUT_COLOR};

/* ---------------------------------------------------------------------- */

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilespec;  /* '-' if stdin */
    unsigned int verbose;        /* -verbose option */
    unsigned int crosseyed;      /* -crosseyed option */
    unsigned int makemask;       /* -makemask option */
    unsigned int dpi;            /* -dpi option */
    float eyesep;                /* -eyesep option */
    float depth;                 /* -depth option */
    unsigned int maxvalSpec;     /* -maxval option count */
    unsigned int maxval;         /* -maxval option value x*/
    int guidesize;               /* -guidesize option */
    unsigned int magnifypat;     /* -magnifypat option */
    unsigned int xshift;         /* -xshift option */
    unsigned int yshift;         /* -yshift option */
    const char * patFilespec;    /* -patfile option.  Null if none */
    unsigned int randomseed;     /* -randomseed option */
    enum outputType outputType;  /* Type of output file */
};



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
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int patfileSpec, dpiSpec, eyesepSpec, depthSpec,
        guidesizeSpec, magnifypatSpec, xshiftSpec, yshiftSpec, randomseedSpec;

    unsigned int blackandwhite, grayscale, color;


    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "verbose",         OPT_FLAG,   NULL,
            (unsigned int *)&cmdlineP->verbose,       0);
    OPTENT3(0, "crosseyed",       OPT_FLAG,   NULL,
            &cmdlineP->crosseyed,     0);
    OPTENT3(0, "makemask",        OPT_FLAG,   NULL,
            &cmdlineP->makemask,      0);
    OPTENT3(0, "blackandwhite",   OPT_FLAG,   NULL,
            &blackandwhite,           0);
    OPTENT3(0, "grayscale",       OPT_FLAG,   NULL,
            &grayscale,               0);
    OPTENT3(0, "color",           OPT_FLAG,   NULL,
            &color,                   0);
    OPTENT3(0, "dpi",             OPT_UINT,   &cmdlineP->dpi,
            &dpiSpec,                 0);
    OPTENT3(0, "eyesep",          OPT_FLOAT,  &cmdlineP->eyesep,
            &eyesepSpec,              0);
    OPTENT3(0, "depth",           OPT_FLOAT,  &cmdlineP->depth,
            &depthSpec,               0);
    OPTENT3(0, "maxval",          OPT_UINT,   &cmdlineP->maxval,
            &cmdlineP->maxvalSpec,    0);
    OPTENT3(0, "guidesize",       OPT_INT,    &cmdlineP->guidesize,
            &guidesizeSpec,           0);
    OPTENT3(0, "magnifypat",      OPT_UINT,   &cmdlineP->magnifypat,
            &magnifypatSpec,          0);
    OPTENT3(0, "xshift",          OPT_UINT,   &cmdlineP->xshift,
            &xshiftSpec,              0);
    OPTENT3(0, "yshift",          OPT_UINT,   &cmdlineP->yshift,
            &yshiftSpec,              0);
    OPTENT3(0, "patfile",         OPT_STRING, &cmdlineP->patFilespec,
            &patfileSpec,             0);
    OPTENT3(0, "randomseed",      OPT_UINT,   &cmdlineP->randomseed,
            &randomseedSpec,          0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (blackandwhite + grayscale + color == 0)
        cmdlineP->outputType = OUTPUT_BW;
    else if (blackandwhite + grayscale + color > 1)
        pm_error("You may specify only one of -blackandwhite, -grayscale, "
                 "and -color");
    else {
        if (blackandwhite)
            cmdlineP->outputType = OUTPUT_BW;
        else if (grayscale)
            cmdlineP->outputType = OUTPUT_GRAYSCALE;
        else {
            assert(color);
            cmdlineP->outputType = OUTPUT_COLOR;
        }
    }
    if (!patfileSpec)
        cmdlineP->patFilespec = NULL;

    if (!dpiSpec)
        cmdlineP->dpi = 96;
    else if (cmdlineP->dpi < 1)
        pm_error("The argument to -dpi must be a positive integer");

    if (!eyesepSpec)
        cmdlineP->eyesep = 2.5;
    else if (cmdlineP->eyesep <= 0.0)
        pm_error("The argument to -eyesep must be a positive number");

    if (!depthSpec)
        cmdlineP->depth = (1.0/3.0);
    else if (cmdlineP->depth < 0.0 || cmdlineP->depth > 1.0)
        pm_error("The argument to -depth must be a number from 0.0 to 1.0");

    if (cmdlineP->maxvalSpec) {
        if (cmdlineP->maxval < 1)
            pm_error("-maxval must be at least 1");
        else if (cmdlineP->maxval > PNM_OVERALLMAXVAL)
            pm_error("-maxval must be at most %u.  You specified %u",
                     PNM_OVERALLMAXVAL, cmdlineP->maxval);
    }

    if (!guidesizeSpec)
        cmdlineP->guidesize = 0;

    if (!magnifypatSpec)
        cmdlineP->magnifypat = 1;
    else if (cmdlineP->magnifypat < 1)
        pm_error("The argument to -magnifypat must be a positive integer");

    if (!xshiftSpec)
        cmdlineP->xshift = 0;

    if (!yshiftSpec)
        cmdlineP->yshift = 0;

    if (!randomseedSpec)
        cmdlineP->randomseed = time(NULL);

    if (xshiftSpec && !cmdlineP->patFilespec)
        pm_error("-xshift is valid only with -patfile");
    if (yshiftSpec && !cmdlineP->patFilespec)
        pm_error("-yshift is valid only with -patfile");

    if (cmdlineP->makemask && cmdlineP->patFilespec)
        pm_error("You may not specify both -makemask and -patfile");

    if (cmdlineP->patFilespec && blackandwhite)
        pm_error("-blackandwhite is not valid with -patfile");
    if (cmdlineP->patFilespec && grayscale)
        pm_error("-grayscale is not valid with -patfile");
    if (cmdlineP->patFilespec && color)
        pm_error("-color is not valid with -patfile");
    if (cmdlineP->patFilespec && cmdlineP->maxvalSpec)
        pm_error("-maxval is not valid with -patfile");


    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else
        pm_error("Too many non-option arguments: %d.  Only argument is "
                 "input file name", argc-1);
}



static int
separation(double             const dist,
           double             const eyesep,
           unsigned int       const dpi,
           double             const dof /* depth of field */
    ) {
/*----------------------------------------------------------------------------
  Return a separation in pixels which corresponds to a 3-D distance
  between the viewer's eyes and a point on an object.
-----------------------------------------------------------------------------*/
    int const pixelEyesep = round2int(eyesep * dpi);

    return round2int((1.0 - dof * dist) * pixelEyesep / (2.0 - dof * dist));
}



static void
reportImageParameters(const char * const fileDesc,
                      const struct pam * const pamP) {

    pm_message("%s: tuple type '%s', %d wide x %d high x %d deep, maxval %lu",
               fileDesc, pamP->tuple_type, pamP->width, pamP->height,
               pamP->depth, pamP->maxval);
}



/*----------------------------------------------------------------------------
   Background generators
-----------------------------------------------------------------------------*/

struct outGenerator;

typedef tuple coord2Color(struct outGenerator *, int, int);
    /* A type to use for functions that map a 2-D coordinate to a color. */
typedef void outGenStateTerm(struct outGenerator *);


typedef struct outGenerator {
    struct pam pam;
    coord2Color * getTuple;
        /* Map from a height-map (x,y) coordinate to a tuple */
    outGenStateTerm * terminateState;
    void * stateP;
} outGenerator;



struct randomState {
    /* The state of a randomColor generator. */
    unsigned int magnifypat;
    tuple *      currentRow;
    unsigned int prevy;
};


#ifndef LITERAL_FN_DEF_MATCH
static coord2Color randomColor;
#endif

static tuple
randomColor(outGenerator * const outGenP,
            int            const x,
            int            const y) {
/*----------------------------------------------------------------------------
   Return a random RGB value.
-----------------------------------------------------------------------------*/
    struct randomState * const stateP = outGenP->stateP;

    /* Every time we start a new row, we select a new sequence of random
       colors.
    */
    if (y/stateP->magnifypat != stateP->prevy/stateP->magnifypat) {
        unsigned int const modulus = outGenP->pam.maxval + 1;
        int col;

        for (col = 0; col < outGenP->pam.width; ++col) {
            tuple const thisTuple = stateP->currentRow[col];

            unsigned int plane;

            for (plane = 0; plane < outGenP->pam.depth; ++plane) {
                unsigned int const randval = rand();
                thisTuple[plane] = randval % modulus;
            }
        }
    }

    /* Return the appropriate column from the pregenerated color row. */
    stateP->prevy = y;
    return stateP->currentRow[x/stateP->magnifypat];
}



#ifndef LITERAL_FN_DEF_MATCH
static outGenStateTerm termRandomColor;
#endif

static void
termRandomColor(outGenerator * const outGenP) {

    struct randomState * const stateP = outGenP->stateP;

    pnm_freepamrow(stateP->currentRow);
}



static void
initRandomColor(outGenerator *     const outGenP,
                const struct pam * const inPamP,
                struct cmdlineInfo const cmdline) {

    struct randomState * stateP;

    outGenP->pam.format      = PAM_FORMAT;
    outGenP->pam.plainformat = 0;

    switch (cmdline.outputType) {
    case OUTPUT_BW:
        strcpy(outGenP->pam.tuple_type, PAM_PBM_TUPLETYPE);
        outGenP->pam.maxval = 1;
        outGenP->pam.depth = 1;
        break;
    case OUTPUT_GRAYSCALE:
        strcpy(outGenP->pam.tuple_type, PAM_PGM_TUPLETYPE);
        outGenP->pam.maxval =
            cmdline.maxvalSpec ? cmdline.maxval : inPamP->maxval;
        outGenP->pam.depth = 1;
        break;
    case OUTPUT_COLOR:
        strcpy(outGenP->pam.tuple_type, PAM_PPM_TUPLETYPE);
        outGenP->pam.maxval =
            cmdline.maxvalSpec ? cmdline.maxval : inPamP->maxval;
        outGenP->pam.depth = 3;
        break;
    }

    MALLOCVAR_NOFAIL(stateP);

    stateP->currentRow = pnm_allocpamrow(&outGenP->pam);
    stateP->magnifypat = cmdline.magnifypat;
    stateP->prevy      = (unsigned int)(-cmdline.magnifypat);

    outGenP->stateP         = stateP;
    outGenP->getTuple       = &randomColor;
    outGenP->terminateState = &termRandomColor;
}



struct patternPixelState {
    /* This is the state of a patternPixel generator.*/
    struct pam   patPam;     /* Descriptor of pattern image */
    tuple **     patTuples;  /* Entire image read from the pattern file */
    unsigned int xshift;
    unsigned int yshift;
    unsigned int magnifypat;
};



#ifndef LITERAL_FN_DEF_MATCH
static coord2Color patternPixel;
#endif

static tuple
patternPixel(outGenerator * const outGenP,
             int            const x,
             int            const y) {
/*----------------------------------------------------------------------------
  Return a pixel from the pattern file.
-----------------------------------------------------------------------------*/
    struct patternPixelState * const stateP = outGenP->stateP;
    struct pam * const patPamP = &stateP->patPam;

    int patx, paty;

    paty = ((y - stateP->yshift) / stateP->magnifypat) % patPamP->height;

    if (paty < 0)
        paty += patPamP->height;

    patx = ((x - stateP->xshift) / stateP->magnifypat) % patPamP->width;

    if (patx < 0)
        patx += patPamP->width;

    return stateP->patTuples[paty][patx];
}



#ifndef LITERAL_FN_DEF_MATCH
static outGenStateTerm termPatternPixel;
#endif

static void
termPatternPixel(outGenerator * const outGenP) {

    struct patternPixelState * const stateP = outGenP->stateP;

    pnm_freepamarray(stateP->patTuples, &stateP->patPam);
}



static void
initPatternPixel(outGenerator *     const outGenP,
                 struct cmdlineInfo const cmdline) {

    struct patternPixelState * stateP;
    FILE * patternFileP;

    MALLOCVAR_NOFAIL(stateP);
        
    patternFileP = pm_openr(cmdline.patFilespec);

    stateP->patTuples =
        pnm_readpam(patternFileP,
                    &stateP->patPam, PAM_STRUCT_SIZE(tuple_type));

    pm_close(patternFileP);

    stateP->xshift     = cmdline.xshift;
    stateP->yshift     = cmdline.yshift;
    stateP->magnifypat = cmdline.magnifypat;

    outGenP->stateP          = stateP;
    outGenP->getTuple        = &patternPixel;
    outGenP->terminateState  = &termPatternPixel;
    outGenP->pam.format      = stateP->patPam.format;
    outGenP->pam.plainformat = stateP->patPam.plainformat;
    outGenP->pam.depth       = stateP->patPam.depth;
    outGenP->pam.maxval      = stateP->patPam.maxval;
    strcpy(outGenP->pam.tuple_type, stateP->patPam.tuple_type);

    if (cmdline.verbose)
        reportImageParameters("Pattern file", &stateP->patPam);
}



static void
createoutputGenerator(struct cmdlineInfo const cmdline,
                      const struct pam * const inPamP,
                      outGenerator **    const outputGeneratorPP) {

    outGenerator * outGenP;
    
    MALLOCVAR_NOFAIL(outGenP);

    outGenP->pam.size   = sizeof(struct pam);
    outGenP->pam.len    = PAM_STRUCT_SIZE(tuple_type);
    outGenP->pam.file   = stdout;
    outGenP->pam.height = inPamP->height + 3 * abs(cmdline.guidesize);
        /* Allow room for guides. */
    outGenP->pam.width  = inPamP->width;

    if (cmdline.patFilespec) {
        /* Background pixels should come from the pattern file. */

        initPatternPixel(outGenP, cmdline);
    } else {
        /* Background pixels should be generated randomly */

        initRandomColor(outGenP, inPamP, cmdline);
    }

    outGenP->pam.bytes_per_sample = pnm_bytespersample(outGenP->pam.maxval);

    *outputGeneratorPP = outGenP;
}



static void
destroyoutputGenerator(outGenerator * const outputGeneratorP) {

    outputGeneratorP->terminateState(outputGeneratorP);
    free(outputGeneratorP);
}


/* End of background generators */

/* ---------------------------------------------------------------------- */


static void
makeWhiteRow(const struct pam * const pamP,
             tuple *            const tuplerow) {

    int col;

    for (col = 0; col < pamP->width; ++col) {
        unsigned int plane;
        for (plane = 0; plane < pamP->depth; ++plane)
            tuplerow[col][plane] = pamP->maxval;
    }
}



static void
writeRowCopies(const struct pam *  const outPamP,
               const tuple *       const outrow,
               unsigned int        const copyCount) {

    unsigned int i;
    for (i = 0; i < copyCount; ++i)
        pnm_writepamrow(outPamP, outrow);
}



/* Draw a pair of guide boxes. */
static void
drawguides(int                const guidesize,
           const struct pam * const outPamP,
           double             const eyesep,
           unsigned int       const dpi,
           double             const depthOfField) {

    int const far = separation(0, eyesep, dpi, depthOfField);
        /* Space between the two guide boxes. */
    int const width = outPamP->width;    /* Width of the output image */

    tuple *outrow;             /* One row of output data */
    tuple blackTuple;
    int col;

    pnm_createBlackTuple(outPamP, &blackTuple);

    outrow = pnm_allocpamrow(outPamP);

    /* Leave some blank rows before the guides. */
    makeWhiteRow(outPamP, outrow);
    writeRowCopies(outPamP, outrow, guidesize);

    /* Draw the guides. */
    if ((width - far + guidesize)/2 < 0 ||
        (width + far - guidesize)/2 >= width)
        pm_message("warning: the guide boxes are completely out of bounds "
                   "at %d DPI", dpi);
    else if ((width - far - guidesize)/2 < 0 ||
             (width + far + guidesize)/2 >= width)
        pm_message("warning: the guide boxes are partially out of bounds "
                   "at %d DPI", dpi);

    for (col = (width - far - guidesize)/2;
         col < (width - far + guidesize)/2;
         ++col)
        if (col >= 0 && col < width)
            pnm_assigntuple(outPamP, outrow[col], blackTuple);

    for (col = (width + far - guidesize)/2;
         col < (width + far + guidesize)/2;
         ++col)
        if (col >= 0 && col < width)
            pnm_assigntuple(outPamP, outrow[col], blackTuple);

    writeRowCopies(outPamP,outrow, guidesize);

    /* Leave some blank rows after the guides. */
    makeWhiteRow(outPamP, outrow);
    writeRowCopies(outPamP, outrow, guidesize);

    pnm_freerow(outrow);
}



/* Do the bulk of the work.  See the paper cited above for code
 * comments.  All I (Scott) did was transcribe the code and make
 * minimal changes for Netpbm.  And some style changes by Bryan to
 * match Netpbm style.
 */
static void
makeStereoRow(const struct pam * const inPamP,
              tuple *            const inRow,
              int *              const same,
              double             const depthOfField,
              double             const eyesep,
              unsigned int       const dpi) {

#define Z(X) (1.0-inRow[X][0]/(double)inPamP->maxval)

    int const width       = inPamP->width;
    int const pixelEyesep = round2int(eyesep * dpi);
        /* Separation in pixels between the viewer's eyes */

    int col;

    for (col = 0; col < width; ++col)
        same[col] = col;

    for (col = 0; col < width; ++col) {
        int const s = separation(Z(col), eyesep, dpi, depthOfField);
        int left, right;

        left  = col - s/2;  /* initial value */
        right = left + s;   /* initial value */

        if (0 <= left && right < width) {
            int visible;
            int t;
            double zt;

            t = 1;  /* initial value */

            do {
                double const dof = depthOfField;
                zt = Z(col) + 2.0*(2.0 - dof*Z(col))*t/(dof*pixelEyesep);
                visible = Z(col-t) < zt && Z(col+t) < zt;
                ++t;
            } while (visible && zt < 1);
            if (visible) {
                int l;

                l = same[left];
                while (l != left && l != right) {
                    if (l < right) {
                        left = l;
                        l = same[left];
                    } else {
                        same[left] = right;
                        left = right;
                        l = same[left];
                        right = l;
                    }
                }
                same[left] = right;
            }
        }
    }
}



static void
makeMaskRow(const struct pam * const outPamP,
            const int *        const same,
            const tuple *      const outRow) {
    int col;

    for (col = outPamP->width-1; col >= 0; --col) {
        bool const duplicate = (same[col] != col);
        unsigned int plane;

        for (plane = 0; plane < outPamP->depth; ++plane)
            outRow[col][plane] = duplicate ? outPamP->maxval : 0;
    }
}



static void
makeImageRow(outGenerator * const outGenP,
             int            const row,
             const int *    const same,
             const tuple *  const outRow) {
/*----------------------------------------------------------------------------
  same[N] is one of two things:

  same[N] == N means to generate a value for Column N independent of
  other columns in the row.

  same[N] > N means Column N should be identical to Column same[N].
  
  same[N] < N is not allowed.
-----------------------------------------------------------------------------*/
    int col;
    for (col = outGenP->pam.width-1; col >= 0; --col) {
        bool const duplicate = (same[col] != col);
        
        tuple newtuple;
        unsigned int plane;

        if (duplicate) {
            assert(same[col] > col);
            assert(same[col] < outGenP->pam.width);

            newtuple = outRow[same[col]];
        } else 
            newtuple = outGenP->getTuple(outGenP, col, row);

        for (plane = 0; plane < outGenP->pam.depth; ++plane)
            outRow[col][plane] = newtuple[plane];
    }
}



static void
invertHeightRow(const struct pam * const heightPamP,
                tuple *            const tupleRow) {

    int col;

    for (col = 0; col < heightPamP->width; ++col)
        tupleRow[col][0] = heightPamP->maxval - tupleRow[col][0];
}



static void
makeImageRows(const struct pam * const inPamP,
              outGenerator *     const outputGeneratorP,
              double             const depthOfField,
              double             const eyesep,
              unsigned int       const dpi,
              bool               const crossEyed,
              bool               const makeMask) {

    tuple * inRow;     /* One row of pixels read from the height-map file */
    tuple * outRow;    /* One row of pixels to write to the height-map file */
    int * same;
        /* Array: same[N] is the column number of a pixel to the right forced
           to have the same color as the one in column N
        */
    int row;           /* Current row in the input and output files */

    inRow = pnm_allocpamrow(inPamP);
    outRow = pnm_allocpamrow(&outputGeneratorP->pam);
    MALLOCARRAY(same, inPamP->width);
    if (same == NULL)
        pm_error("Unable to allocate space for \"same\" array.");

    /* See the paper cited above for code comments.  All I (Scott) did was
     * transcribe the code and make minimal changes for Netpbm.  And some
     * style changes by Bryan to match Netpbm style.
     */
    for (row = 0; row < inPamP->height; ++row) {
        pnm_readpamrow(inPamP, inRow);
        if (crossEyed)
            /* Invert heights for cross-eyed (as opposed to wall-eyed)
               people.
            */
            invertHeightRow(inPamP, inRow);

        /* Determine color constraints. */
        makeStereoRow(inPamP, inRow, same, depthOfField, eyesep, dpi);

        if (makeMask)
            makeMaskRow(&outputGeneratorP->pam, same, outRow);
        else
            makeImageRow(outputGeneratorP, row, same, outRow);

        /* Write the resulting row. */
        pnm_writepamrow(&outputGeneratorP->pam, outRow);
    }
    free(same);
    pnm_freepamrow(outRow);
    pnm_freepamrow(inRow);
}



static void
produceStereogram(FILE *             const ifP,
                  struct cmdlineInfo const cmdline) {

    struct pam inPam;    /* PAM information for the height-map file */
    outGenerator * outputGeneratorP;
        /* Handle of an object that generates background pixels */
    
    pnm_readpaminit(ifP, &inPam, PAM_STRUCT_SIZE(tuple_type));

    createoutputGenerator(cmdline, &inPam, &outputGeneratorP);

    if (cmdline.verbose) {
        reportImageParameters("Input (height map) file", &inPam);
        if (inPam.depth > 1)
            pm_message("Ignoring all but the first plane of input.");
        reportImageParameters("Output (stereogram) file",
                              &outputGeneratorP->pam);
    }

    pnm_writepaminit(&outputGeneratorP->pam);

    /* Draw guide boxes at the top, if desired. */
    if (cmdline.guidesize < 0)
        drawguides(-cmdline.guidesize, &outputGeneratorP->pam,
                   cmdline.eyesep,
                   cmdline.dpi, cmdline.depth);

    makeImageRows(&inPam, outputGeneratorP,
                  cmdline.depth, cmdline.eyesep, cmdline.dpi,
                  cmdline.crosseyed, cmdline.makemask);

    /* Draw guide boxes at the bottom, if desired. */
    if (cmdline.guidesize > 0)
        drawguides(cmdline.guidesize, &outputGeneratorP->pam,
                   cmdline.eyesep, cmdline.dpi, cmdline.depth);

    destroyoutputGenerator(outputGeneratorP);
}



static void
reportParameters(struct cmdlineInfo const cmdline) {

    unsigned int const pixelEyesep = round2int(cmdline.eyesep * cmdline.dpi);

    pm_message("Eye separation: %.4g inch * %d DPI = %u pixels",
               cmdline.eyesep, cmdline.dpi, pixelEyesep);

    if (cmdline.magnifypat > 1)
        pm_message("Background magnification: %uX * %uX",
                   cmdline.magnifypat, cmdline.magnifypat);
    pm_message("\"Optimal\" pattern width: %u / (%u * 2) = %u pixels",
               pixelEyesep, cmdline.magnifypat,
               pixelEyesep/(cmdline.magnifypat * 2));
    pm_message("Unique 3-D depth levels possible: %u",
               separation(0, cmdline.eyesep, cmdline.dpi, cmdline.depth) -
               separation(1, cmdline.eyesep, cmdline.dpi, cmdline.depth) + 1);
    if (cmdline.patFilespec && (cmdline.xshift || cmdline.yshift))
        pm_message("Pattern shift: (%u, %u)", cmdline.xshift, cmdline.yshift);
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;      /* Parsed command line */
    FILE * ifP;
    
    /* Parse the command line. */
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
    
    if (cmdline.verbose)
        reportParameters(cmdline);
    
    srand(cmdline.randomseed);

    ifP = pm_openr(cmdline.inputFilespec);
        
    /* Produce a stereogram. */
    produceStereogram(ifP, cmdline);

    pm_close(ifP);
    
    return 0;
}
