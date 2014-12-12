#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilespec;  
    const char * maskFilespec;  
    unsigned int verbose;
    float        sharpness;
    float        threshold;
};



static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int sharpSpec, thresholdSpec;
    
    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "sharpness",       OPT_FLOAT,  &cmdlineP->sharpness,
            &sharpSpec,           0);
    OPTENT3(0, "threshold",       OPT_FLOAT,  &cmdlineP->threshold,
            &thresholdSpec,       0);
    OPTENT3(0, "verbose",         OPT_FLAG,   NULL,
            &cmdlineP->verbose,   0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (sharpSpec) {
        if (cmdlineP->sharpness < 0)
            pm_error("-sharpness less than zero doesn't make sense.  "
                     "You specified %f", cmdlineP->sharpness);
    } else
        cmdlineP->sharpness = 1.0;

    if (thresholdSpec) {
        if (cmdlineP->threshold < 0)
            pm_error("-threshold less than zero doesn't make sense.  "
                     "You specified %f", cmdlineP->threshold);
        if (cmdlineP->threshold > 1.0)
            pm_error("-threshold greater than unity doesn't make sense.  "
                     "You specified %f", cmdlineP->threshold);
        
    } else
        cmdlineP->threshold = 0.0;

    if (argc-1 < 1)
        pm_error("You must specify at least one argument:  The name "
                 "of the mask image file");
    else {
        cmdlineP->maskFilespec = argv[1];
        if (argc-1 < 2)
            cmdlineP->inputFilespec = "-";
        else {
            cmdlineP->inputFilespec = argv[2];
        
            if (argc-1 > 2)
                pm_error("There are at most two arguments:  mask file name "
                         "and input file name.  You specified %d", argc-1);
        }
    }
}        



static sample
sharpened(sample const inputSample,
          sample const maskSample,
          float  const sharpness,
          sample const threshold,
          sample const maxval) {

    int const edgeness = inputSample - maskSample;

    sample retval;

    if (abs(edgeness) > threshold) {
        float const rawResult = inputSample + edgeness * sharpness;
        
        retval = MIN(maxval, (unsigned)MAX(0, (int)(rawResult+0.5)));
    } else
        retval = inputSample;

    return retval;
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    struct pam inpam;
    struct pam maskpam;
    struct pam outpam;
    FILE * ifP;
    FILE * maskfP;
    tuple * inputTuplerow;
    tuple * maskTuplerow;
    tuple * outputTuplerow;
    unsigned int row;
    sample threshold;
        /* Magnitude of difference between image and unsharp mask below
           which they will be considered identical.
        */
    
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);
    maskfP = pm_openr(cmdline.maskFilespec);

    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));
    pnm_readpaminit(maskfP, &maskpam, PAM_STRUCT_SIZE(tuple_type));

    if (inpam.width  != maskpam.width || 
        inpam.height != maskpam.height ||
        inpam.depth  != maskpam.depth)
        pm_error("The mask image must be the same dimensions as the "
                 "input image.  The mask is %dx%dx%d, but the input is "
                 "%dx%dx%d.",
                 maskpam.width, maskpam.height, maskpam.depth,
                 inpam.width,   inpam.height,   inpam.depth);
    if (inpam.maxval != maskpam.maxval)
        pm_error("The mask image must have the same maxval as the "
                 "input image.  The input image has maxval %u, "
                 "but the mask image has maxval %u",
                 (unsigned)inpam.maxval, (unsigned)maskpam.maxval);

    threshold = (float)cmdline.threshold / inpam.maxval;

    outpam = inpam;
    outpam.file = stdout;

    inputTuplerow  = pnm_allocpamrow(&inpam);
    maskTuplerow   = pnm_allocpamrow(&maskpam);
    outputTuplerow = pnm_allocpamrow(&outpam);

    pnm_writepaminit(&outpam);

    for (row = 0; row < outpam.height; ++row) {
        unsigned int col;
        pnm_readpamrow(&inpam,   inputTuplerow);
        pnm_readpamrow(&maskpam, maskTuplerow);
        
        for (col = 0; col < outpam.width; ++col) {
            unsigned int plane;
            
            for (plane = 0; plane < outpam.depth; ++plane) {
                outputTuplerow[col][plane] =
                    sharpened(inputTuplerow[col][plane],
                              maskTuplerow[col][plane],
                              cmdline.sharpness,
                              threshold,
                              outpam.maxval);
            }
        }
        pnm_writepamrow(&outpam, outputTuplerow);
    }

    pm_close(ifP);
    pm_close(maskfP);

    pnm_freepamrow(inputTuplerow);
    pnm_freepamrow(maskTuplerow);
    pnm_freepamrow(outputTuplerow);
    
    return 0;
}
