/*============================================================================
                               pamlookup
==============================================================================

  Look up integers or ordered pairs from an index image in a lookup table and
  produce a corresponding image containing the results of the lookups.
  
  The index image and lookup table are PAM images.  The output image is
  a PAM image with the width and height of the index image and tuples of
  the kind in the lookup table.

  By Bryan Henderson, San Jose CA 2002.11.10

============================================================================*/

#include "pam.h"
#include "shhopt.h"
#include "pm_system.h"
#include "nstring.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *indexFilespec;  
    char *lookupFilespec;
    char *missingcolor;  /* -missingcolor value.  null if not specified */
    unsigned int fit;  /* -fit option */
};



static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    
    unsigned int lookupfileSpec, missingcolorSpec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "lookupfile",     OPT_STRING, &cmdlineP->lookupFilespec,  
            &lookupfileSpec, 0);
    OPTENT3(0,   "missingcolor", OPT_STRING, 
            &cmdlineP->missingcolor,   &missingcolorSpec, 0);
    OPTENT3(0,   "fit", OPT_FLAG, 
            NULL,   &cmdlineP->fit, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!lookupfileSpec)
        pm_error("You must specify the -lookupfile option");

    if (!missingcolorSpec)
        cmdlineP->missingcolor = NULL;

    if (argc-1 < 1)
        cmdlineP->indexFilespec = "-";
    else
        cmdlineP->indexFilespec = argv[1];
}        



static void
fitLookup(tuple **     const inputLookup, 
          struct pam   const inputLookuppam,
          tuple ***    const fitLookupP,
          struct pam * const fitLookuppamP,
          unsigned int const cols,
          unsigned int const rows) {
/*----------------------------------------------------------------------------
  Scale the lookup table image so that it has dimensions 'cols' x 'rows'.
-----------------------------------------------------------------------------*/
    const char * pamscaleCommand;
    struct pamtuples inPamtuples;
    struct pamtuples outPamtuples;

    *fitLookuppamP = inputLookuppam;
    fitLookuppamP->width = cols;
    fitLookuppamP->height = rows;

    asprintfN(&pamscaleCommand, "pamscale -width=%u -height=%u", cols, rows);

    inPamtuples.pamP = (struct pam *) &inputLookuppam;
    inPamtuples.tuplesP = (tuple ***) &inputLookup;
    outPamtuples.pamP = fitLookuppamP;
    outPamtuples.tuplesP = fitLookupP;
    
    pm_system(&pm_feed_from_pamtuples, &inPamtuples,
              &pm_accept_to_pamtuples, &outPamtuples,
              pamscaleCommand);

    strfree(pamscaleCommand);
}



static void
getLookup(const char * const lookupFilespec, 
          unsigned int const indexDegree,
          unsigned int const indexMaxval,
          tuple ***    const lookupP,
          struct pam * const lookuppamP,
          bool         const fit) {

    FILE*  lookupfileP;

    struct pam inputLookuppam;
    tuple** inputLookup;

    lookupfileP = pm_openr(lookupFilespec);
    inputLookup = pnm_readpam(lookupfileP, 
                              &inputLookuppam, PAM_STRUCT_SIZE(tuple_type));

    pm_close(lookupfileP);
    
    if (fit) {
        fitLookup(inputLookup, inputLookuppam, lookupP, lookuppamP,
                  indexMaxval + 1, 
                  indexDegree > 1 ? indexMaxval + 1 : 1);
        pnm_freepamarray(inputLookup, &inputLookuppam);
    } else {
        *lookupP = inputLookup;
        *lookuppamP = inputLookuppam;
    }
        
    if (indexDegree == 1 && lookuppamP->height != 1)
        pm_error("Your index image has integer indices, "
                 "so the lookup table image must be one row.  "
                 "Yours is %d rows.", 
                 lookuppamP->height);

    if (lookuppamP->width - 1 > indexMaxval)
        pm_message("Warning:  your lookup table image is wider than "
                   "the maxval of "
                   "your index message, so the right end of the lookup "
                   "table image will have no effect on the output.");
    if (indexDegree == 2 && lookuppamP->height - 1 > indexMaxval)
        pm_message("Warning: your lookup table image is taller than "
                   "the maxval of "
                   "your index message, so the bottom end of the lookup "
                   "table image has no effect on the output.");
}



static void
computeDefaultTuple(struct cmdlineInfo const cmdline, 
                    tuple **           const lookup,
                    struct pam *       const lookuppamP, 
                    tuple *            const defaultTupleP) {

    tuple retval;

    retval = pnm_allocpamtuple(lookuppamP);

    /* Note: "missing color" really makes sense only for a color
       lookup, whereas this program allows an arbitrary PAM image as a
       lookup table.  We should probably check here for a lookup file
       that has a visual image tuple type, but we don't out of
       laziness.  The program probably ought to have a generic
       "missing tuple type" option too.  
    */
    if (cmdline.missingcolor) {
        pixel const color = 
            ppm_parsecolor(cmdline.missingcolor, lookuppamP->maxval);

        if (lookuppamP->depth >= 3) {
            retval[PAM_RED_PLANE] = PPM_GETR(color);
            retval[PAM_GRN_PLANE] = PPM_GETG(color);
            retval[PAM_BLU_PLANE] = PPM_GETB(color);
        } else {
            if (PPM_GETR(color) != PPM_GETG(color) ||
                PPM_GETR(color) != PPM_GETB(color))
                pm_error("You specified as a missing color something which "
                         "is not monochrome, but your lookup table file, "
                         "and thus your "
                         "output file, can contain only monochrome values");
            else
                retval[0] = PPM_GETR(color);
        }
    } else 
        pnm_assigntuple(lookuppamP, retval, lookup[0][0]);

    *defaultTupleP = retval;
}



static void
doLookup(struct pam const indexpam,
         struct pam const outpamarg,
         tuple      const defaultTuple,
         tuple **   const lookup,
         struct pam const lookuppam) {

    struct pam outpam;
    unsigned int row;

    tuple* tuplerowIndex;
    tuple* tuplerowOut;

    outpam = outpamarg;

    tuplerowIndex = pnm_allocpamrow(&indexpam);
    tuplerowOut = pnm_allocpamrow(&outpam);

    pnm_writepaminit(&outpam);

    for (row = 0; row < outpam.height; ++row) {
        unsigned int col;
        pnm_readpamrow(&indexpam, tuplerowIndex);
        
        for (col = 0; col < outpam.width; ++col) {
            unsigned int indexRow, indexCol;
            tuple v;
            
            if (indexpam.depth < 2) {
                indexRow = 0;
                indexCol = tuplerowIndex[col][0];
            } else {
                indexRow = tuplerowIndex[col][0];
                indexCol = tuplerowIndex[col][1];
            }

            if (indexRow >= lookuppam.height || indexCol >= lookuppam.width)
                v = defaultTuple;
            else
                v = lookup[indexRow][indexCol];

            pnm_assigntuple(&outpam, tuplerowOut[col], v);
        }
        pnm_writepamrow(&outpam, tuplerowOut);
    }
    pnm_freepamrow(tuplerowIndex);
    pnm_freepamrow(tuplerowOut);
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    struct pam indexpam;
    struct pam outpam;
    FILE*  ifP;
    struct pam lookuppam;
    tuple** lookup;

    tuple defaultTuple;
    
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.indexFilespec);

    pnm_readpaminit(ifP, &indexpam, PAM_STRUCT_SIZE(tuple_type));

    if (indexpam.depth != 1 && indexpam.depth != 2)
        pm_error("The input (index) file must have depth 1 or 2.  "
                 "Yours has depth %d",
                 indexpam.depth);

    getLookup(cmdline.lookupFilespec, indexpam.depth, indexpam.maxval, 
              &lookup, &lookuppam, cmdline.fit);

    computeDefaultTuple(cmdline, lookup, &lookuppam, &defaultTuple);

    outpam = lookuppam;
    outpam.height = indexpam.height;
    outpam.width = indexpam.width;
    outpam.file = stdout;
    
    doLookup(indexpam, outpam, defaultTuple, lookup, lookuppam);

    pm_close(ifP);

    pnm_freepamtuple(defaultTuple);
    pnm_freepamarray(lookup, &lookuppam);
    
    exit(0);
}

