/******************************************************************************
                               pamsplit
*******************************************************************************
  Split a Netpbm format input file into multiple Netpbm format output files
  with one image per output file.

  By Bryan Henderson, Olympia WA; June 2000

  Contributed to the public domain by its author.
******************************************************************************/

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <string.h>
#include <stdio.h>
#include "pam.h"
#include "shhopt.h"
#include "nstring.h"
#include "mallocvar.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFileName;
    const char * outputFilePattern;
    unsigned int debug;
    unsigned int padname;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the pointers we place into *cmdlineP are sometimes to storage
   in the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int padnameSpec;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "debug",   OPT_FLAG, NULL, &cmdlineP->debug, 0);
    OPTENT3(0,   "padname", OPT_UINT, &cmdlineP->padname, &padnameSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!padnameSpec)
        cmdlineP->padname = 0;

    if (argc - 1 < 1) 
        cmdlineP->inputFileName = "-";
    else
        cmdlineP->inputFileName = argv[1];
    if (argc -1 < 2)
        cmdlineP->outputFilePattern = "image%d";
    else
        cmdlineP->outputFilePattern = argv[2];

    if (!strstr(cmdlineP->outputFilePattern, "%d"))
        pm_error("output file spec pattern parameter must include the "
                 "string '%%d',\n"
                 "to stand for the image sequence number.\n"
                 "You specified '%s'.", cmdlineP->outputFilePattern);
}



static void
extractOneImage(FILE * const infileP,
                FILE * const outfileP) {

    struct pam inpam;
    struct pam outpam;
    enum pm_check_code checkRetval;
    
    unsigned int row;
    tuple * tuplerow;

    pnm_readpaminit(infileP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    pnm_checkpam(&inpam, PM_CHECK_BASIC, &checkRetval);

    outpam = inpam;
    outpam.file = outfileP;

    pnm_writepaminit(&outpam);

    tuplerow = pnm_allocpamrow(&inpam);
    for (row = 0; row < inpam.height; ++row) {
        pnm_readpamrow(&inpam, tuplerow);
        pnm_writepamrow(&outpam, tuplerow);
    }
    pnm_freepamrow(tuplerow);
}



static void
computeOutputName(char          const outputFilePattern[], 
                  unsigned int  const padCount,
                  unsigned int  const imageSeq,
                  const char ** const outputNameP) {
/*----------------------------------------------------------------------------
   Compute the name of an output file given the pattern
   outputFilePattern[] and the image sequence number 'imageSeq'.
   outputFilePattern[] contains at least one instance of the string
   "%d" and we substitute the ASCII decimal representation of
   imageSeq for the firstone of them to generate the output file
   name.  We add leading zeroes as necessary to bring the number up to
   at least 'padCount' characters.
-----------------------------------------------------------------------------*/
    char * beforeSub;
    const char * afterSub;
    const char * filenameFormat;
        /* A format string for asprintfN for the file name */

    beforeSub = strdup(outputFilePattern);
    *(strstr(beforeSub, "%d")) = '\0';

    afterSub = strstr(outputFilePattern, "%d") + 2;

    /* Make filenameFormat something like "%s%04u%s" */
    asprintfN(&filenameFormat, "%%s%%0%ud%%s", padCount);

    asprintfN(outputNameP, filenameFormat, beforeSub, imageSeq, afterSub);

    strfree(filenameFormat);

    free(beforeSub);
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;

    FILE * ifP;
    bool eof;  /* No more images in input */
    unsigned int imageSeq;  
        /* Sequence of current image in input file.  First = 0 */

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
    
    ifP = pm_openr(cmdline.inputFileName);

    eof = FALSE;
    for (imageSeq = 0; !eof; ++imageSeq) {
        FILE * ofP;
        const char * outputFileName;  /* malloc'ed */

        computeOutputName(cmdline.outputFilePattern, cmdline.padname, 
                          imageSeq,
                          &outputFileName);
        pm_message("WRITING %s", outputFileName);

        ofP = pm_openw(outputFileName);
        extractOneImage(ifP, ofP);

        pm_close(ofP);
        strfree(outputFileName);

        pnm_nextimage(ifP, &eof);
    }
    pm_close(ifP);
    
    return 0;
}
