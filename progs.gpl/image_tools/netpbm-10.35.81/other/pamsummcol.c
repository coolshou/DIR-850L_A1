/******************************************************************************
                               pamsummcol
*******************************************************************************
  Summarize the columns of a PAM image with various functions.

  By Bryan Henderson, San Jose CA 2004.02.07.

  Contributed to the public domain


******************************************************************************/

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

enum function {FN_ADD, FN_MEAN, FN_MIN, FN_MAX};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    enum function function;
    unsigned int verbose;
};


static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int sumSpec, meanSpec, minSpec, maxSpec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0,   "sum",      OPT_FLAG,  NULL, &sumSpec,           0);
    OPTENT3(0,   "mean",     OPT_FLAG,  NULL, &meanSpec,          0);
    OPTENT3(0,   "min",      OPT_FLAG,  NULL, &minSpec,           0);
    OPTENT3(0,   "max",      OPT_FLAG,  NULL, &maxSpec,           0);
    OPTENT3(0,   "verbose",  OPT_FLAG,  NULL, &cmdlineP->verbose, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (sumSpec + minSpec + maxSpec > 1)
        pm_error("You may specify at most one of -sum, -min, and -max");

    if (sumSpec) {
        cmdlineP->function = FN_ADD;
    } else if (meanSpec) {
        cmdlineP->function = FN_MEAN;
    } else if (minSpec) {
        cmdlineP->function = FN_MIN;
    } else if (maxSpec) {
        cmdlineP->function = FN_MAX;
    } else 
        pm_error("You must specify one of -sum, -min, or -max");
        
    if (argc-1 > 1)
        pm_error("Too many arguments (%d).  File spec is the only argument.",
                 argc-1);

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else 
        cmdlineP->inputFilespec = argv[1];
    
}


struct accum {
    union {
        unsigned int sum;
        unsigned int min;
        unsigned int max;
    } u;
};



static void
createAccumulator(enum function    const function,
                  unsigned int     const cols,
                  unsigned int     const planes,
                  struct accum *** const accumulatorP) {
    
    struct accum ** accumulator;
    unsigned int col;

    MALLOCARRAY_NOFAIL(accumulator, cols);

    for (col = 0; col < cols; ++col) {
        unsigned int plane;

        MALLOCARRAY_NOFAIL(accumulator[col], planes);

        for (plane = 0; plane < planes; ++plane) {
            switch(function) {
            case FN_ADD:  accumulator[col][plane].u.sum = 0;        break;
            case FN_MEAN: accumulator[col][plane].u.sum = 0;        break;
            case FN_MIN:  accumulator[col][plane].u.min = UINT_MAX; break;
            case FN_MAX:  accumulator[col][plane].u.max = 0;        break;
            } 
        }
    }
    *accumulatorP = accumulator;
}



static void
destroyAccumulator(struct accum **    accumulator,
                   unsigned int const cols) {

    unsigned int col;
    for (col = 0; col < cols; ++col)
        free(accumulator[col]);

    free(accumulator);
}



static void
aggregate(struct pam *    const inpamP,
          tuple *         const tupleRow,
          enum function   const function,
          struct accum ** const accumulator) {

    unsigned int col;

    for (col = 0; col < inpamP->width; ++col) {
        unsigned int plane;
        for (plane = 0; plane < inpamP->depth; ++plane) {
            switch(function) {
            case FN_ADD:  
            case FN_MEAN: 
                if (accumulator[col][plane].u.sum > 
                    UINT_MAX - tupleRow[col][plane])
                    pm_error("Numerical overflow in Column %u", col);
                accumulator[col][plane].u.sum += tupleRow[col][plane];
            break;
            case FN_MIN:  
                if (tupleRow[col][plane] < accumulator[col][plane].u.min)
                    accumulator[col][plane].u.min = tupleRow[col][plane];
                break;
            case FN_MAX:
                if (tupleRow[col][plane] > accumulator[col][plane].u.min)
                    accumulator[col][plane].u.min = tupleRow[col][plane];
                break;
            } 
        }
    }
}



static void
makeSummaryRow(struct accum ** const accumulator,
               unsigned int  const   count,
               struct pam *  const   pamP,
               enum function const   function,
               tuple *       const   tupleRow) {
    
    unsigned int col;

    for (col = 0; col < pamP->width; ++col) {
        unsigned int plane;
        for (plane = 0; plane < pamP->depth; ++plane) {
            switch(function) {
            case FN_ADD:  
                tupleRow[col][plane] = 
                    MIN(accumulator[col][plane].u.sum, pamP->maxval);
                break;
            case FN_MEAN: 
                tupleRow[col][plane] = 
                    ROUNDU((double)accumulator[col][plane].u.sum / count);
                break;
            case FN_MIN:  
                tupleRow[col][plane] = 
                    accumulator[col][plane].u.min;
                break;
            case FN_MAX:
                tupleRow[col][plane] = 
                    accumulator[col][plane].u.max;
                break;
            }
        } 
    }
}



int
main(int argc, char *argv[]) {

    FILE* ifP;
    tuple* inputRow;   /* Row from input image */
    tuple* outputRow;  /* Output row */
    int row;
    struct cmdlineInfo cmdline;
    struct pam inpam;   /* Input PAM image */
    struct pam outpam;  /* Output PAM image */
    struct accum ** accumulator;  /* malloc'ed two-dimensional array */

    pnm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    createAccumulator(cmdline.function, inpam.width, inpam.depth, 
                      &accumulator);

    inputRow = pnm_allocpamrow(&inpam);

    outpam = inpam;    /* Initial value -- most fields should be same */
    outpam.file = stdout;
    outpam.height = 1;

    pnm_writepaminit(&outpam);

    outputRow = pnm_allocpamrow(&outpam);

    for (row = 0; row < inpam.height; row++) {
        pnm_readpamrow(&inpam, inputRow);

        aggregate(&inpam, inputRow, cmdline.function, accumulator);
    }
    makeSummaryRow(accumulator, inpam.height, &outpam, cmdline.function, 
                   outputRow);
    pnm_writepamrow(&outpam, outputRow);

    pnm_freepamrow(outputRow);
    pnm_freepamrow(inputRow);
    destroyAccumulator(accumulator, inpam.width);
    pm_close(inpam.file);
    pm_close(outpam.file);
    
    return 0;
}
