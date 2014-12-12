/******************************************************************************
                               pampick
*******************************************************************************
  Select specified images from a Netpbm image stream and create a new
  Netpbm image stream containing them.

  By Bryan Henderson, San Jose CA; October 2005

  Contributed to the public domain by its author.
******************************************************************************/

#include <string.h>
#include <stdio.h>

#include "pam.h"
#include "shhopt.h"
#include "nstring.h"
#include "mallocvar.h"


struct uintSet {
    unsigned int allocSize;
    unsigned int count;
    unsigned int * list;
};



static void
initUintSet(struct uintSet * const uintSetP,
            unsigned int     const allocSize) {

    uintSetP->allocSize = allocSize;
    uintSetP->count     = 0;

    MALLOCARRAY(uintSetP->list, allocSize);
    if (uintSetP->list == NULL)
        pm_error("Could not allocate memory for an array of %u numbers.",
                 allocSize);
}



static void
termUintSet(struct uintSet * const uintSetP) {

    free(uintSetP->list);
}



static bool
isMemberOfUintSet(const struct uintSet * const uintSetP,
                  unsigned int           const searchValue) {

    bool retval;
    unsigned int i;

    retval = FALSE;  /* initial assumption */
    
    for (i = 0; i < uintSetP->count; ++i) {
        if (uintSetP->list[i] == searchValue)
            retval = TRUE;
    }
    return retval;
}



static void
addToUintSet(struct uintSet * const uintSetP,
             unsigned int     const newValue) {

    if (uintSetP->count >= uintSetP->allocSize)
        pm_error("Overflow of number list");
    else {
        if (!isMemberOfUintSet(uintSetP, newValue))
            uintSetP->list[uintSetP->count++] = newValue;
    }
}



struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    struct uintSet imageSeqList;
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

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */

    /* We have no options.  We use the parser just to gripe if user
       specifies an option.  But when we add an option in the future,
       it will go right here with an OPTENT3() macro invocation.
    */

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    initUintSet(&cmdlineP->imageSeqList, argc-1);

    {
        unsigned int i;

        for (i = 0; i < argc-1; ++i) {
            if (strlen(argv[i+1]) == 0)
                pm_error("An image sequence argument is a null string!");
            else {
                char * endPtr;
                int const strtolResult = strtol(argv[i+1], &endPtr, 10);

                if (*endPtr != '\0')
                    pm_error("Garbage in sequence number argument '%s': '%s'",
                             argv[i+1], endPtr);
                
                if (strtolResult < 0)
                    pm_error("Image sequence number cannot be negative.  "
                             "You specified %d", strtolResult);

                addToUintSet(&cmdlineP->imageSeqList, strtolResult);
            }
        }
    }
    free(option_def);
}



static void
destroyCmdline(struct cmdlineInfo * const cmdlineP) {

    termUintSet(&cmdlineP->imageSeqList);
}



static void
extractOneImage(FILE * const infileP,
                FILE * const outfileP) {
/*----------------------------------------------------------------------------
  Copy a complete image from input stream *infileP to output stream
  *outfileP.

  But if outfileP == NULL, just read the image and discard it.
-----------------------------------------------------------------------------*/
    struct pam inpam;
    struct pam outpam;
    enum pm_check_code checkRetval;
    
    unsigned int row;
    tuple * tuplerow;

    pnm_readpaminit(infileP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    pnm_checkpam(&inpam, PM_CHECK_BASIC, &checkRetval);

    outpam = inpam;
    outpam.file = outfileP;

    if (outfileP)
        pnm_writepaminit(&outpam);

    tuplerow = pnm_allocpamrow(&inpam);
    for (row = 0; row < inpam.height; ++row) {
        pnm_readpamrow(&inpam, tuplerow);
        if (outfileP)
            pnm_writepamrow(&outpam, tuplerow);
    }
    pnm_freepamrow(tuplerow);
}



static void
failIfUnpickedImages(const struct uintSet * const uintSetP,
                     unsigned int           const imageCount) {
/*----------------------------------------------------------------------------
   Abort the program (pm_error()) if there are any image sequence numbers
   in the set *uintSetP that are greater than or equal to 'imageCount'.
-----------------------------------------------------------------------------*/
    unsigned int i;

    for (i = 0; i < uintSetP->count; ++i) {
        if (uintSetP->list[i] >= imageCount)
            pm_error("You asked for image sequence number %u "
                     "(relative to 0).  "
                     "The number of images in the input stream is only %u.",
                     uintSetP->list[i], imageCount);
    }
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;

    bool eof;  /* No more images in input */
    unsigned int imageSeq;  
        /* Sequence of current image in input file.  First = 0 */

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
    
    eof = FALSE;
    for (imageSeq = 0; !eof; ++imageSeq) {
        if (isMemberOfUintSet(&cmdline.imageSeqList, imageSeq)) {
            pm_message("Extracting Image #%u", imageSeq);

            extractOneImage(stdin, stdout);
        } else
            extractOneImage(stdin, NULL);

        pnm_nextimage(stdin, &eof);
    }

    failIfUnpickedImages(&cmdline.imageSeqList, imageSeq);

    destroyCmdline(&cmdline);

    pm_close(stdin);
    pm_close(stdout);
    
    return 0;
}
