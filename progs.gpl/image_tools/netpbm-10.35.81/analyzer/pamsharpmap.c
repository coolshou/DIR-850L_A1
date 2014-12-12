/*----------------------------------------------------------------------------
                                Pnmsharpness
------------------------------------------------------------------------------

  Bryan Henderson derived this (January 2004) from the program Pnmsharp
  by B.W. van Schooten and distributed to Bryan under the Perl
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
#include "mallocvar.h"

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
makeSharpnessPixel(struct pam * const pamP,
                   float *      const sharpness,
                   tuple        const sharpnessTuple) {

    unsigned int plane;
    for (plane = 0; plane < pamP->depth; ++plane)
        sharpnessTuple[plane] = 
            (sample)(sharpness[plane] * pamP->maxval + 0.5);
}



static void
makeBlackTuplen(struct pam * const pamP,
                tuplen       const tuplen) {

    unsigned int plane;
    for (plane = 0; plane < pamP->depth; ++plane)
        tuplen[plane] = 0.0;
}



static void
makeBlackRown(struct pam * const pamP,
              tuplen *     const tuplenrow) {

    unsigned int col;
    for (col = 0; col < pamP->width; ++col)
        makeBlackTuplen(pamP, tuplenrow[col]);
}



int
main(int argc, char **argv) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    tuplen ** tuplenarray;
    struct pam inpam;
    struct pam mappam;
    tuple ** map;
    int row;
    float * sharpness;

	pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

	tuplenarray = pnm_readpamn(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    mappam = inpam;
    mappam.file = stdout;
    mappam.maxval = 255;

    MALLOCARRAY_NOFAIL(sharpness, inpam.depth);
            
    map = pnm_allocpamarray(&mappam);
    makeBlackRown(&inpam, tuplenarray[0]);
    for (row = 1; row < inpam.height-1; ++row) {
        int col;
        makeBlackTuplen(&inpam, tuplenarray[row][0]);
        for (col = 1; col < inpam.width-1; ++col) {
            int dy;
            unsigned int plane;
            
            for (plane = 0; plane < inpam.depth; ++plane)
                sharpness[plane] = 0.0;

            for (dy = -1; dy <= 1; ++dy) {
                int dx;
                for (dx = -1; dx <= 1; ++dx) {
                    if (dx != 0 || dy != 0) {
                        unsigned int plane;
                        for (plane = 0; plane < inpam.depth; ++plane) {
                            samplen const sampleval = 
                                tuplenarray[row][col][plane];
                            samplen const sampleval2 = 
                                tuplenarray[row+dy][col+dx][plane];
                            sharpness[plane] += fabs(sampleval - sampleval2);
                        }
                    }
                }
            }
            makeSharpnessPixel(&mappam, sharpness, map[row][col]);
        }
        makeBlackTuplen(&inpam, tuplenarray[row][inpam.width-1]);
    }
    makeBlackRown(&inpam, tuplenarray[inpam.height-1]);
    free(sharpness);
    
    pnm_writepam(&mappam, map);

    pnm_freepamarray(map, &mappam);
	pnm_freepamarrayn(tuplenarray, &inpam);

	return 0;
}
