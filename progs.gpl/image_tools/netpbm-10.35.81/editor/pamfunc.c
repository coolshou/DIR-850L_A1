/******************************************************************************
                               pamfunc
*******************************************************************************
  Apply one of various functions to each sample in a PAM image

  By Bryan Henderson, San Jose CA 2002.06.16.

  Contributed to the public domain

  ENHANCEMENT IDEAS:

  1) speed up by doing integer arithmetic instead of floating point for
  multiply/divide where possible.  Especially when multiplying by an 
  integer.

  2) For multiply/divide, give option of simply changing the maxval and
  leaving the raster alone.

******************************************************************************/

#include "pam.h"
#include "shhopt.h"

enum function {FN_MULTIPLY, FN_DIVIDE, FN_ADD, FN_SUBTRACT, FN_MIN, FN_MAX};

/* Note that when the user specifies a minimum, that means he's requesting
   a "max" function.
*/

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    enum function function;
    union {
        float multiplier;
        float divisor;
        int adder;
        int subtractor;
        unsigned int max;
        unsigned int min;
    } u;
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

    unsigned int multiplierSpec, divisorSpec, adderSpec, subtractorSpec;
    unsigned int maxSpec, minSpec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0,   "multiplier", OPT_FLOAT,  &cmdlineP->u.multiplier, 
            &multiplierSpec, 0);
    OPTENT3(0,   "divisor",    OPT_FLOAT,  &cmdlineP->u.divisor,
            &divisorSpec, 0);
    OPTENT3(0,   "adder",      OPT_INT,    &cmdlineP->u.adder,
            &adderSpec, 0);
    OPTENT3(0,   "subtractor", OPT_INT,    &cmdlineP->u.subtractor,
            &subtractorSpec, 0);
    OPTENT3(0,   "min",        OPT_UINT,   &cmdlineP->u.min,
            &minSpec, 0);
    OPTENT3(0,   "max",        OPT_UINT,   &cmdlineP->u.max,
            &maxSpec, 0);
    OPTENT3(0,   "verbose",    OPT_FLAG,   NULL, &cmdlineP->verbose,       0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (multiplierSpec + divisorSpec + adderSpec + subtractorSpec +
        minSpec + maxSpec > 1)
        pm_error("You may specify at most one of -multiplier, -divisor,"
                 "-adder, -subtractor, -min, and -max");

    if (multiplierSpec) {
        cmdlineP->function = FN_MULTIPLY;
        if (cmdlineP->u.multiplier < 0)
            pm_error("Multiplier must be nonnegative.  You specified %f", 
                     cmdlineP->u.multiplier);
    } else if (divisorSpec) {
        cmdlineP->function = FN_DIVIDE;
        if (cmdlineP->u.divisor < 0)
            pm_error("Divisor must be nonnegative.  You specified %f", 
                     cmdlineP->u.divisor);
    } else if (adderSpec) {
        cmdlineP->function = FN_ADD;
    } else if (subtractorSpec) {
        cmdlineP->function = FN_SUBTRACT;
    } else if (minSpec) {
        cmdlineP->function = FN_MAX;
    } else if (maxSpec) {
        cmdlineP->function = FN_MIN;
    } else 
        pm_error("You must specify one of -multiplier, -divisor, "
                 "-adder, -subtractor, -min, or -max");
        
    if (argc-1 > 1)
        pm_error("Too many arguments (%d).  File spec is the only argument.",
                 argc-1);

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else 
        cmdlineP->inputFilespec = argv[1];
    
}



static void
applyFunction(struct cmdlineInfo const cmdline,
              struct pam         const inpam,
              struct pam         const outpam,
              tuple *            const inputRow,
              tuple *            const outputRow) {

    float const oneOverDivisor = 1/cmdline.u.divisor;
        /* In my experiments, the compiler couldn't figure out that
           1/cmdline.u.divisor is a constant and instead recomputed it
           for each and every pixel.  division is slower than
           multiplication, so we want to multiply by
           1/cmdline.u.divisor instead of divide by cmdline.u.divisor,
           so we compute that here.  Note that if the function isn't
           divide, both cmdline.u.divisor and oneOverDivisor are
           meaningless.  
        */
    int col;

    for (col = 0; col < inpam.width; ++col) {
        int plane;
        for (plane = 0; plane < inpam.depth; ++plane) {
            sample const inSample = inputRow[col][plane];
            sample outSample;  /* Could be > maxval  */

            switch (cmdline.function) {
            case FN_MULTIPLY:
                outSample = ROUNDU(inSample * cmdline.u.multiplier);
                break;
            case FN_DIVIDE:
                outSample = ROUNDU(inSample * oneOverDivisor);
                break;
            case FN_ADD:
                outSample = MAX(0, (long)inSample + cmdline.u.adder);
                break;
            case FN_SUBTRACT:
                outSample = MAX(0, (long)inSample - cmdline.u.subtractor);
                break;
            case FN_MAX:
                outSample = MAX(inSample, cmdline.u.min);
                break;
            case FN_MIN:
                outSample = MIN(inSample, cmdline.u.max);
                break;
            }
            outputRow[col][plane] = MIN(outpam.maxval, outSample);
        }
    }
}                



int
main(int argc, char *argv[]) {

    FILE* ifP;
    tuple* inputRow;   /* Row from input image */
    tuple* outputRow;  /* Row of output image */
    int row;
    struct cmdlineInfo cmdline;
    struct pam inpam;   /* Input PAM image */
    struct pam outpam;  /* Output PAM image */

    pnm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    inputRow = pnm_allocpamrow(&inpam);

    outpam = inpam;    /* Initial value -- most fields should be same */
    outpam.file = stdout;

    pnm_writepaminit(&outpam);

    outputRow = pnm_allocpamrow(&outpam);

    for (row = 0; row < inpam.height; row++) {
        pnm_readpamrow(&inpam, inputRow);

        applyFunction(cmdline, inpam, outpam, inputRow, outputRow);

        pnm_writepamrow(&outpam, outputRow);
    }
    pnm_freepamrow(outputRow);
    pnm_freepamrow(inputRow);
    pm_close(inpam.file);
    pm_close(outpam.file);
    
    exit(0);
}

