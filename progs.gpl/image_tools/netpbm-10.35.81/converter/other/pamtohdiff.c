/******************************************************************************
                                 pamtohdiff
*******************************************************************************
  This program creates a PAM output which a horizontal difference image of
  the input PAM.  The samples in each row are a number to be added to to
  the previous output row to create the next output row (and the first
  output row is simply the same as the input row).

  Because these samples must be positive and negative and PAM samples are
  unsigned, we bias each PAM sample by the input maxval and make the output
  maxval twice the input maxval.

  By Bryan Henderson, San Jose, CA 2002.04.15.

******************************************************************************/
#include <string.h>
#include <stdio.h>

#include "pam.h"
#include "shhopt.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input files */
    unsigned int verbose;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc( 100*sizeof( optEntry ) );
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "verbose",   OPT_FLAG,    NULL, &cmdlineP->verbose,  0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else
        pm_error("Too many arguments.");
}



int 
main(int argc, char *argv[]) {
    FILE *ifP;
    struct cmdlineInfo cmdline;
    struct pam inpam, outpam;
    unsigned int row;
    tuple * inrow;
    tuple * outrow;
    tuple * prevrow;

    pnm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    outpam = inpam;
    outpam.file = stdout;
    outpam.format = PAM_FORMAT;
    strcpy(outpam.tuple_type, "hdiff");

    pnm_writepaminit(&outpam);

    inrow = pnm_allocpamrow(&inpam);
    outrow = pnm_allocpamrow(&outpam);
    prevrow = pnm_allocpamrow(&inpam);

    pnm_setpamrow(&inpam, prevrow, 0);

    /* All arithmetic in this operation and in the reverse operation
       (to recover the image) is done modulus the maxval+1 (the hdiff
       PAM and the image have the same maxval) in order to produce
       legal PAM samples (which must be in the range 0..maxval).  This
       might seem to throw away information, but it doesn't.  Example:
       maxval is 99.  Intensity goes from 90 in Row 0 to 10 in Row 1.
       The difference is -80.  -80 mod 100 is 20, so 20 goes in the
       hdiff output.  On reconstructing the image, the interpreter
       knows the "20" can't be +20, because that would create the
       sample 90 + 20 = 110, and violate maxval.  So it must be -80.
       Modulus arithmetic by the interpreter effectively makes that
       decision.  
    */


    /* The bias is just to make it easier to look at the output visually.
       If you display the values as intensities, and your differences are
       all +/- half of maxval, you can see positive transitions as bright
       spots and negative transitions as dark spots.
    */
    
    {
        unsigned int const bias = outpam.maxval/2;
        for (row = 0; row < inpam.height; ++row) {
            unsigned int col;
            pnm_readpamrow(&inpam, inrow);
            for (col = 0; col < inpam.width; ++col) {
            unsigned int plane;
            for (plane = 0; plane < inpam.depth; ++plane) {
                
                sample const sampleValue = inrow[col][plane];
                int const difference = sampleValue - prevrow[col][plane];
                outrow[col][plane] = (difference + bias) % (outpam.maxval+1);
                prevrow[col][plane] = sampleValue;
            }
        }
            pnm_writepamrow(&outpam, outrow);
        }
    }
    pnm_freepamrow(prevrow);
    pnm_freepamrow(outrow);
    pnm_freepamrow(inrow);

    exit(0);
}

