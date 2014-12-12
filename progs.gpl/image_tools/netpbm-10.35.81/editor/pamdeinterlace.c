/******************************************************************************
                             pamdeinterlace
*******************************************************************************
  De-interlace an image, i.e. select every 2nd row.
   
  By Bryan Henderson, San Jose, CA 2001.11.11.

  Contributed to the public domain.
******************************************************************************/

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

enum evenodd {EVEN, ODD};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input files */
    enum evenodd rowsToTake;
};


static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optStruct3 opt;  /* set by OPTENT3 */
    optEntry *option_def;
    unsigned int option_def_index;

    unsigned int takeeven, takeodd;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "takeeven", OPT_FLAG, NULL, &takeeven, 0);
    OPTENT3(0,   "takeodd",  OPT_FLAG, NULL, &takeodd,  0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (takeeven && takeodd)
        pm_error("You cannot specify both -takeeven and -takeodd options.");

    if (takeodd)
        cmdlineP->rowsToTake = ODD;
    else
        cmdlineP->rowsToTake = EVEN;

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else
        pm_error("You specified too many arguments (%d).  The only "
                 "argument is the optional input file specification.",
                 argc-1);
}





int
main(int argc, char *argv[]) {

    FILE * ifP;
    tuple * tuplerow;   /* Row from input image */
    unsigned int row;
    struct cmdlineInfo cmdline;
    struct pam inpam;  
    struct pam outpam;

    pnm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);
    
    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    tuplerow = pnm_allocpamrow(&inpam);

    outpam = inpam;    /* Initial value -- most fields should be same */
    outpam.file = stdout;
    if (inpam.height % 2 == 0)
        outpam.height = inpam.height / 2;
    else {
        if (cmdline.rowsToTake == ODD)
            outpam.height = inpam.height / 2;
        else
            outpam.height = inpam.height / 2 + 1;
    }

    pnm_writepaminit(&outpam);

    {
        unsigned int modulusToTake;
            /* The row number mod 2 of the rows that are supposed to go into
               the output.
            */

        switch (cmdline.rowsToTake) {
        case EVEN: modulusToTake = 0; break;
        case ODD:  modulusToTake = 1; break;
        default: pm_error("INTERNAL ERROR: invalid rowsToTake");
        }

        /* Read input and write out rows extracted from it */
        for (row = 0; row < inpam.height; row++) {
            pnm_readpamrow(&inpam, tuplerow);
            if (row % 2 == modulusToTake)
                pnm_writepamrow(&outpam, tuplerow);
        }
    }
    pnm_freepamrow(tuplerow);
    pm_close(inpam.file);
    pm_close(outpam.file);
    
    return 0;
}

