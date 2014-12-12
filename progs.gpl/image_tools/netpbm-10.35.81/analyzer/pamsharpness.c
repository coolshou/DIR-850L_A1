/*----------------------------------------------------------------------------
                                Pnmsharpness
------------------------------------------------------------------------------

  Bryan Henderson derived this (January 2004) from the program of the
  same name by B.W. van Schooten and distributed to Bryan under the Perl
  Artistic License, as part of the Photopnmtools package.  Bryan placed
  his modifications in the public domain.

  This is the copyright/license notice from the original:

   Copyright (c) 2002, 2003 by B.W. van Schooten. All rights reserved.
   This software is distributed under the Perl Artistic License.
   No warranty. See file 'artistic.license' for more details.

   boris@13thmonkey.org
   www.13thmonkey.org/~boris/photopnmtools/ 
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>

#include "pam.h"
#include "shhopt.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* '-' if stdin */
    unsigned int context;
};



static void
parseCommandLine ( int argc, char ** argv,
                   struct cmdlineInfo *cmdlineP ) {
/*----------------------------------------------------------------------------
   parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int contextSpec;

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "context",       OPT_UINT,   &cmdlineP->context,       
            &contextSpec,         0 );

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!contextSpec)
        cmdlineP->context = 1;

    if (cmdlineP->context < 1)
        pm_error("-context must be at least 1");


    if (argc-1 > 1)
        pm_error("The only argument is the input file name");
    else if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else
        cmdlineP->inputFilespec = argv[1];
}



static void
computeSharpness(struct pam * const inpamP,
                 tuplen **    const tuplenarray,
                 double *     const sharpnessP) {

    int row;
    double totsharp;
    
    totsharp = 0.0;

    for (row = 1; row < inpamP->height-1; ++row) {
        int col;
        for (col = 1; col < inpamP->width-1; ++col) {
            int dy;
            for (dy = -1; dy <= 1; ++dy) {
                int dx;
                for (dx = -1; dx <= 1; ++dx) {
                    if (dx != 0 || dy != 0) {
                        unsigned int plane;
                        for (plane = 0; plane < inpamP->depth; ++plane) {
                            samplen const sampleval = 
                                tuplenarray[row][col][plane];
                            samplen const sampleval2 = 
                                tuplenarray[row+dy][col+dx][plane];
                            totsharp += fabs(sampleval - sampleval2);
                        }
                    }
                }
            }
		}
	}
    *sharpnessP = 
        totsharp / (inpamP->width * inpamP->height * inpamP->depth * 8);
        /* The 8 above is for the 8 neighbors to which we compare each pixel */
}



int
main(int argc, char **argv) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    tuplen ** tuplenarray;
    struct pam inpam;
    double sharpness;

	pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

	tuplenarray = pnm_readpamn(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    if (inpam.height < 3 || inpam.width < 3)
        pm_error("sharpness is undefined for an image less than 3 pixels "
                 "in all directions.  This image is %d x %d",
                 inpam.width, inpam.height);

    computeSharpness(&inpam, tuplenarray, &sharpness);

    pm_message("Sharpness = %f\n", sharpness);

	pnm_freepamarrayn(tuplenarray, &inpam);
    pm_close(ifP);
	return 0;
}
