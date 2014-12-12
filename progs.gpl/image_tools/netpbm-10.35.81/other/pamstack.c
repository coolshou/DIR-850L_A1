/*----------------------------------------------------------------------------
                               pamstack
------------------------------------------------------------------------------
  Part of the Netpbm package.

  Combine the channels (stack the planes) of multiple PAM images to create
  a single PAM image.


  By Bryan Henderson, San Jose CA 2000.08.05

  Contributed to the public domain by its author 2002.05.05.
-----------------------------------------------------------------------------*/

#include <string.h>

#include "mallocvar.h"
#include "nstring.h"
#include "shhopt.h"
#include "pam.h"

#define MAX_INPUTS 16
    /* The most input PAMs we allow user to specify */

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *tupletype;       /* Tuple type for output PAM */
    unsigned int nInput;
        /* The number of input PAMs.  At least 1, at most 16. */
    const char * inputFileName[MAX_INPUTS];
        /* The PAM files to combine, in order. */
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec strings we return are stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry * option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;
    extern struct pam pam;  /* Just so we can look at field sizes */

    unsigned int option_def_index;
    unsigned int tupletypeSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);
    
    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "tupletype",  OPT_STRING, &cmdlineP->tupletype, 
            &tupletypeSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!tupletypeSpec)
        cmdlineP->tupletype = "";
    else
        if (strlen(cmdlineP->tupletype)+1 > sizeof(pam.tuple_type))
            pm_error("Tuple type name specified is too long.  Maximum of "
                     "%u characters allowed.", sizeof(pam.tuple_type));

    cmdlineP->nInput = 0;  /* initial value */
    { 
        unsigned int argn;
        bool stdinUsed;
        for (argn = 1, stdinUsed = false; argn < argc; ++argn) {
            if (cmdlineP->nInput >= MAX_INPUTS) 
                pm_error("You may not specify more than %u input images.",
                         MAX_INPUTS);
            cmdlineP->inputFileName[cmdlineP->nInput++] = argv[argn];
            if (streq(argv[argn], "-")) {
                if (stdinUsed)
                    pm_error("You cannot specify Standard Input ('-') "
                             "for more than one input file");
                stdinUsed = true;
            }
        }
    }
    if (cmdlineP->nInput < 1)
        cmdlineP->inputFileName[cmdlineP->nInput++] = "-";
}



static void
openAllStreams(unsigned int  const nInput,
               const char ** const inputFileName,
               FILE **       const ifP) {

    unsigned int inputSeq;

    for (inputSeq = 0; inputSeq < nInput; ++inputSeq)
        ifP[inputSeq] = pm_openr(inputFileName[inputSeq]);
}



static void
outputRaster(const struct pam       inpam[], 
             unsigned int     const nInput,
             struct pam             outpam) {

    tuple *inrow;
    tuple *outrow;
        
    outrow = pnm_allocpamrow(&outpam);
    inrow = pnm_allocpamrow(&outpam);      

    { 
        int row;
        
        for (row = 0; row < outpam.height; row++) {
            unsigned int inputSeq;
            int outplane;
            outplane = 0;  /* initial value */
            for (inputSeq = 0; inputSeq < nInput; ++inputSeq) {
                struct pam thisInpam = inpam[inputSeq];
                int col;

                pnm_readpamrow(&thisInpam, inrow);

                for (col = 0; col < outpam.width; col ++) {
                    int inplane;
                    for (inplane = 0; inplane < thisInpam.depth; ++inplane) 
                        outrow[col][outplane+inplane] = inrow[col][inplane];
                }
                outplane += thisInpam.depth;
            }
            pnm_writepamrow(&outpam, outrow);
        }
    }
    pnm_freepamrow(outrow);
    pnm_freepamrow(inrow);        
}



static void
processOneImageInAllStreams(unsigned int const nInput,
                            FILE *       const ifP[],
                            FILE *       const ofP,
                            const char * const tupletype) {

    struct pam inpam[MAX_INPUTS];   /* Input PAM images */
    struct pam outpam;  /* Output PAM image */

    unsigned int inputSeq;
        /* The horizontal sequence -- i.e. the sequence of the
           input stream, not the sequence of an image within a
           stream.
        */

    unsigned int outputDepth;
    outputDepth = 0;  /* initial value */
    
    for (inputSeq = 0; inputSeq < nInput; ++inputSeq) {

        pnm_readpaminit(ifP[inputSeq], &inpam[inputSeq], 
                        PAM_STRUCT_SIZE(tuple_type));

        if (inputSeq > 0) {
            /* All images, including this one, must be compatible with the 
               first image.
            */
            if (inpam[inputSeq].width != inpam[0].width)
                pm_error("Image no. %u does not have the same width as "
                         "Image 0.", inputSeq);
            if (inpam[inputSeq].height != inpam[0].height)
                pm_error("Image no. %u does not have the same height as "
                         "Image 0.", inputSeq);
            if (inpam[inputSeq].maxval != inpam[0].maxval)
                pm_error("Image no. %u does not have the same maxval as "
                         "Image 0.", inputSeq);
        }
        outputDepth += inpam[inputSeq].depth;
    }

    outpam        = inpam[0];     /* Initial value */
    outpam.depth  = outputDepth;
    outpam.file   = ofP;
    outpam.format = PAM_FORMAT;
    strcpy(outpam.tuple_type, tupletype);

    pm_message("Writing %u channel PAM image", outpam.depth);

    pnm_writepaminit(&outpam);

    outputRaster(inpam, nInput, outpam);
}



static void
nextImageAllStreams(unsigned int const nInput,
                    FILE *       const ifP[],
                    bool *       const eofP) {
/*----------------------------------------------------------------------------
   Advance all the streams ifP[] to the next image.

   Return *eofP == TRUE iff at least one stream has no next image.
-----------------------------------------------------------------------------*/
    unsigned int inputSeq;

    for (inputSeq = 0; inputSeq < nInput; ++inputSeq) {
        bool eof;
        pnm_nextimage(ifP[inputSeq], &eof);
        if (eof)
            *eofP = eof;
    }
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP[MAX_INPUTS];
    bool eof;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    openAllStreams(cmdline.nInput, cmdline.inputFileName, ifP);

    eof = FALSE;
    while (!eof) {
        processOneImageInAllStreams(cmdline.nInput, ifP, stdout,
                                    cmdline.tupletype);

        nextImageAllStreams(cmdline.nInput, ifP, &eof);
    }

    return 0;
}
