/*----------------------------------------------------------------------------
                               pamtopnm
------------------------------------------------------------------------------
  Part of the Netpbm package.

  Convert PAM images to PBM, PGM, or PPM (i.e. PNM)

  By Bryan Henderson, San Jose CA 2000.08.05

  Contributed to the public domain by its author 2000.08.05.
-----------------------------------------------------------------------------*/

#include <string.h>

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input files */
    unsigned int assume;    /* -assume option */
};


static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0,   "assume",     OPT_FLAG,   NULL, &cmdlineP->assume,         0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 == 0) 
        cmdlineP->inputFilespec = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->inputFilespec = argv[1];
}



static void
validateTupleType(struct pam const inpam, 
                  int        const assumeTupleType) {
/*----------------------------------------------------------------------------
   Make sure the image has a tuple type we know how to convert to PNM.

   We're quite liberal, trying to accomodate all sorts of future
   twists on the formats.  If the tuple type _starts with_
   BLACKANDWHITE, GRAYSCALE, or RGB, and has at least as many planes
   as we'd need to convert to PBM, PGM, or PPM, respectively, we
   accept it.  We thus accomodate variations on these formats that add
   planes and add to the right end of the tuple type to explain them.

   If Callers specified 'assumeTupleType', we're even more liberal.
-----------------------------------------------------------------------------*/
    if (assumeTupleType) {
        /* User says tuple type is appropriate regardless of tuple_type. */
    } else {
        if (inpam.depth >= 1 && 
            strncmp(inpam.tuple_type, "BLACKANDWHITE", 
                    sizeof("BLACKANDWHITE")-1) == 0) {
            /* It's a PBMable image */
        } else if (inpam.depth >= 1 && 
                   strncmp(inpam.tuple_type, "GRAYSCALE",
                           sizeof("GRAYSCALE")-1) == 0) {
            /* It's a PGMable image */
        } else if (inpam.depth >= 3 &&
                   strncmp(inpam.tuple_type, "RGB", sizeof("RGB")-1) == 0) {
            /* It's a PPMable image */
        } else 
            pm_error("PAM image does not have a depth and tuple_type "
                     "consistent with a PNM image."
                     "According to its "
                     "header, depth is %d and tuple_type is '%s'.  "
                     "Use the -assume option to convert anyway.",
                     inpam.depth, inpam.tuple_type);
    }
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE* ifP;
    struct pam inpam;   /* Input PAM image */
    struct pam outpam;  /* Output PNM image */

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    validateTupleType(inpam, cmdline.assume);

    outpam = inpam;
    outpam.file = stdout;
    
    if (inpam.depth < 3) {
        outpam.depth = 1;
        if (inpam.maxval == 1)
            outpam.format = PBM_FORMAT;
        else 
            outpam.format = PGM_FORMAT;
    } else {
        outpam.depth = 3;
        outpam.format = PPM_FORMAT;
    }

    pnm_writepaminit(&outpam);

    {
        tuple *tuplerow;
        
        tuplerow = pnm_allocpamrow(&inpam);      
        { 
            int row;
            
            for (row = 0; row < inpam.height; row++) {
                pnm_readpamrow(&inpam, tuplerow);
                pnm_writepamrow(&outpam, tuplerow);
            }
        }
        pnm_freepamrow(tuplerow);        
    }
    return 0;
}
