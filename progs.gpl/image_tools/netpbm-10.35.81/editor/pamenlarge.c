/*=============================================================================
                             pamenlarge
===============================================================================
  By Bryan Henderson 2004.09.26.  Contributed to the public domain by its
  author.
=============================================================================*/

#include "pam.h"
#include "mallocvar.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  
    unsigned int scaleFactor;
};



static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    if (argc-1 < 1)
        pm_error("You must specify at least one argument:  The scale factor");
    else {
        cmdlineP->scaleFactor = atoi(argv[1]);
        
        if (cmdlineP->scaleFactor < 1)
            pm_error("Scale factor must be an integer at least 1.  "
                     "You specified '%s'", argv[1]);

        if (argc-1 >= 2)
            cmdlineP->inputFilespec = argv[2];
        else
            cmdlineP->inputFilespec = "-";
    }
}        



static void
makeOutputRowMap(tuple **     const outTupleRowP,
                 struct pam * const outpamP,
                 struct pam * const inpamP,
                 tuple *      const inTuplerow) {
/*----------------------------------------------------------------------------
   Create a tuple *outTupleRowP which is actually a row of pointers into
   inTupleRow[], so as to map input pixels to output pixels by stretching.
-----------------------------------------------------------------------------*/
    tuple * newtuplerow;
    int col;

    MALLOCARRAY_NOFAIL(newtuplerow, outpamP->width);

    for (col = 0 ; col < inpamP->width; ++col) {
        unsigned int const scaleFactor = outpamP->width / inpamP->width;
        unsigned int subcol;

        for (subcol = 0; subcol < scaleFactor; ++subcol)
            newtuplerow[col * scaleFactor + subcol] = inTuplerow[col];
    }
    *outTupleRowP = newtuplerow;
}



int
main(int    argc, 
     char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    struct pam inpam;
    struct pam outpam; 
    tuple * tuplerow;
    tuple * newtuplerow;
    int row;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);
 
    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    outpam = inpam; 
    outpam.file   = stdout;
    outpam.width  = inpam.width * cmdline.scaleFactor;
    outpam.height = inpam.height * cmdline.scaleFactor; 

    pnm_writepaminit(&outpam);

    tuplerow = pnm_allocpamrow(&inpam);

    makeOutputRowMap(&newtuplerow, &outpam, &inpam, tuplerow);

    for (row = 0; row < inpam.height; ++row) {
        pnm_readpamrow(&inpam, tuplerow);
        pnm_writepamrowmult(&outpam, newtuplerow, cmdline.scaleFactor);
    }

    free(newtuplerow);

    pnm_freepamrow(tuplerow);

    pm_close(ifP);
    pm_close(stdout);

    return 0;
}

