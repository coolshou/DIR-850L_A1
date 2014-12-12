/******************************************************************************
                             pammixinterlace
*******************************************************************************
  De-interlace an image by merging adjacent rows.
   
  Copyright (C) 2005 Bruce Guenter, FutureQuest, Inc.

  Permission to use, copy, modify, and distribute this software and its
  documentation for any purpose and without fee is hereby granted,
  provided that the above copyright notice appear in all copies and that
  both that copyright notice and this permission notice appear in
  supporting documentation.  This software is provided "as is" without
  express or implied warranty.

******************************************************************************/

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input files */
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

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
    /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else
        pm_error("You specified too many arguments (%d).  The only "
                 "argument is the optional input file specification.",
                 argc-1);
}



static void
allocateRowWindowBuffer(struct pam * const pamP,
                        tuple **     const tuplerow) {

    unsigned int row;

    for (row = 0; row < 3; ++row)
        tuplerow[row] = pnm_allocpamrow(pamP);
}



static void
freeRowWindowBuffer(tuple ** const tuplerow) {

    unsigned int row;

    for (row = 0; row < 3; ++row)
        pnm_freepamrow(tuplerow[row]);

}



static void
slideWindowDown(tuple ** const tuplerow) {
/*----------------------------------------------------------------------------
  Slide the 3-line tuple row window tuplerow[] down one row by moving
  pointers.

  tuplerow[2] ends up an uninitialized buffer.
-----------------------------------------------------------------------------*/
    tuple * const oldrow0 = tuplerow[0];
    tuplerow[0] = tuplerow[1];
    tuplerow[1] = tuplerow[2];
    tuplerow[2] = oldrow0;
}



int
main(int argc, char *argv[]) {

    FILE * ifP;
    struct cmdlineInfo cmdline;
    struct pam inpam;  
    struct pam outpam;
    tuple * tuplerow[3];
    tuple * outputrow;
    
    pnm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);
    
    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    outpam = inpam;    /* Initial value -- most fields should be same */
    outpam.file = stdout;

    pnm_writepaminit(&outpam);

    allocateRowWindowBuffer(&inpam, tuplerow);
    outputrow = pnm_allocpamrow(&outpam);

    if (inpam.height < 3) {
        unsigned int row;
        pm_message("WARNING: Image height less than 3.  No mixing done.");
        for (row = 0; row < inpam.height; ++row) {
            pnm_readpamrow(&inpam, tuplerow[0]);
            pnm_writepamrow(&outpam, tuplerow[0]);
        }
    } else {
        unsigned int row;

        pnm_readpamrow(&inpam, tuplerow[0]);
        pnm_readpamrow(&inpam, tuplerow[1]);

        /* Pass through first row */
        pnm_writepamrow(&outpam, tuplerow[0]);

        for (row = 2; row < inpam.height; ++row) {
            unsigned int col;
            pnm_readpamrow(&inpam, tuplerow[2]);
            for (col = 0; col < inpam.width; ++col) {
                unsigned int plane;

                for (plane = 0; plane < inpam.depth; ++plane) {
                    outputrow[col][plane] =
                        (tuplerow[0][col][plane]
                         + tuplerow[1][col][plane] * 2
                         + tuplerow[2][col][plane]) / 4;
                }
            }
            pnm_writepamrow(&outpam, outputrow);
            
            slideWindowDown(tuplerow);
        }

        /* Pass through last row */
        pnm_writepamrow(&outpam, tuplerow[1]);
    }

    freeRowWindowBuffer(tuplerow);
    pnm_freepamrow(outputrow);
    pm_close(inpam.file);
    pm_close(outpam.file);
    
    return 0;
}
