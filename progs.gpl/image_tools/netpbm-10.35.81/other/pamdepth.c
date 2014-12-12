/*=============================================================================
                            pamdepth
===============================================================================
  Change the maxval in a Netpbm image.

  This replaces Pnmdepth.

  By Bryan Henderson January 2006.

  Contributed to the public domain by its author.
=============================================================================*/
#include <assert.h>

#include "shhopt.h"
#include "mallocvar.h"
#include "pam.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFileName;
    unsigned int newMaxval;
    unsigned int verbose;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec strings we return are stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry * option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "verbose",  OPT_STRING, NULL, 
            &cmdlineP->verbose, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 < 1)
        pm_error("You must specify at least one argument -- the new maxval");
    else {
        int const intval = atoi(argv[1]);

        if (intval < 1)
            pm_error("New maxval must be at least 1.  You specified %d",
                     intval);
        else if (intval > PNM_OVERALLMAXVAL)
            pm_error("newmaxval (%d) is too large.\n"
                     "The maximum allowed by the PNM formats is %d.",
                     intval, PNM_OVERALLMAXVAL);
        else
            cmdlineP->newMaxval = intval;

        if (argc-1 < 2)
            cmdlineP->inputFileName = "-";
        else
            cmdlineP->inputFileName = argv[2];
    }
}



static void
createSampleMap(sample   const oldMaxval,
                sample   const newMaxval,
                sample** const sampleMapP) {

    unsigned int i;

    sample * sampleMap;

    MALLOCARRAY_NOFAIL(sampleMap, oldMaxval+1);

    for (i = 0; i <= oldMaxval; ++i)
        sampleMap[i] = (i * newMaxval + oldMaxval / 2) / oldMaxval;

    *sampleMapP = sampleMap;
}



static void
transformRaster(struct pam * const inpamP,
                struct pam * const outpamP) {
                
    tuple * tuplerow;
    unsigned int row;
    sample * sampleMap;  /* malloc'ed */

    createSampleMap(inpamP->maxval, outpamP->maxval, &sampleMap);

    assert(inpamP->height == outpamP->height);
    assert(inpamP->width  == outpamP->width);
    assert(inpamP->depth  == outpamP->depth);

    tuplerow = pnm_allocpamrow(inpamP);

    for (row = 0; row < inpamP->height; ++row) {
        unsigned int col;
        pnm_readpamrow(inpamP, tuplerow);

        for (col = 0; col < inpamP->width; ++col) {
            unsigned int plane;
            for (plane = 0; plane < inpamP->depth; ++plane)
                tuplerow[col][plane] = sampleMap[tuplerow[col][plane]];
        }
        pnm_writepamrow(outpamP, tuplerow);
	}

    pnm_freepamrow(tuplerow);

    free(sampleMap);
}



int
main(int    argc,
     char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    struct pam inpam;
    struct pam outpam;
    bool eof;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFileName);

    eof = FALSE;
    while (!eof) {
        pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

        outpam = inpam;  /* initial value */
        
        outpam.file = stdout;
        outpam.maxval = cmdline.newMaxval;
        
        if (PNM_FORMAT_TYPE(inpam.format) == PBM_TYPE) {
            pm_message( "promoting from PBM to PGM" );
            outpam.format = PGM_TYPE;
        } else
            outpam.format = inpam.format;
        
        pnm_writepaminit(&outpam);

        transformRaster(&inpam, &outpam);

        pnm_nextimage(ifP, &eof);
    }
    pm_close(ifP);
    pm_close(stdout);

    return 0;
}
