/* pbmclean.c - pixel cleaning. Remove pixel if less than n connected
 *              identical neighbours, n=1 default.
 * AJCD 20/9/90
 * stern, Fri Oct 19 00:10:38 MET DST 2001
 *     add '-white/-black' flags to restrict operation to given blobs
 */

#include <stdio.h>
#include "pbm.h"
#include "shhopt.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input files */
    bool flipWhite;
    bool flipBlack;
    unsigned int connect;
    unsigned int verbose;
};

#define PBM_INVERT(p) ((p) == PBM_WHITE ? PBM_BLACK : PBM_WHITE)

/* input bitmap size and storage */
static bit *inrow[3] ;

#define THISROW (1)

enum compass_heading {
    WEST=0,
    NORTHWEST=1,
    NORTH=2,
    NORTHEAST=3,
    EAST=4,
    SOUTHEAST=5,
    SOUTH=6,
    SOUTHWEST=7
};
/* compass directions from west clockwise.  Indexed by enum compass_heading */
int const xd[] = { -1, -1,  0,  1, 1, 1, 0, -1 } ;
int const yd[] = {  0, -1, -1, -1, 0, 1, 1,  1 } ;

static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optStruct3 opt;  /* set by OPTENT3 */
    optEntry *option_def = malloc(100*sizeof(optEntry));
    unsigned int option_def_index;

    unsigned int black, white;
    unsigned int minneighborsSpec;

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "verbose", OPT_FLAG, NULL, &cmdlineP->verbose, 0);
    OPTENT3(0,   "black", OPT_FLAG, NULL, &black, 0);
    OPTENT3(0,   "white", OPT_FLAG, NULL, &white, 0);
    OPTENT3(0,   "minneighbors", OPT_UINT, &cmdlineP->connect, 
            &minneighborsSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = TRUE;  /* We sort of allow negative numbers as parms */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!black && !white) {
        cmdlineP->flipBlack = TRUE;
        cmdlineP->flipWhite = TRUE;
    } else {
        cmdlineP->flipBlack = !!black;
        cmdlineP->flipWhite = !!white;
    }    


    if (!minneighborsSpec) {
        /* Now we do a sleazy tour through the parameters to see if
           one is -N where N is a positive integer.  That's for
           backward compatibility, since Pbmclean used to have
           unconventional syntax where a -N option was used instead of
           the current -minneighbors option.  The only reason -N didn't
           get processed by pm_optParseOptions3() is that it looked
           like a negative number parameter instead of an option.  
           If we find a -N, we make like it was a -minneighbors=N option.
        */
        int i;
        bool foundNegative;

        cmdlineP->connect = 1;  /* default */
        foundNegative = FALSE;

        for (i = 1; i < argc; ++i) {
            if (foundNegative)
                argv[i-1] = argv[i];
            else {
                if (atoi(argv[i]) < 0) {
                    cmdlineP->connect = - atoi(argv[i]);
                    foundNegative = TRUE;
                }
            }
        }
        if (foundNegative)
            --argc;
    }

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
nextrow(FILE * const ifd,
        int    const row,
        int    const cols,
        int    const rows,
        int    const format) {
/*----------------------------------------------------------------------------
   Advance one row in the input.

   'row' is the row number that will be the current row.
-----------------------------------------------------------------------------*/
    bit * shuffle;

    /* First, get the "next" row in inrow[2] if this is the very first
       call to nextrow().
    */
    if (inrow[2] == NULL && row < rows) {
        inrow[2] = pbm_allocrow(cols);
        pbm_readpbmrow(ifd, inrow[2], cols, format);
    }
    /* Now advance the inrow[] window, rotating the buffer that now holds
       the "previous" row to use it for the new "next" row.
    */
    shuffle = inrow[0];

    inrow[0] = inrow[1];
    inrow[1] = inrow[2];
    inrow[2] = shuffle ;
    if (row+1 < rows) {
        /* Read the "next" row in from the file.  Allocate buffer if needed */
        if (inrow[2] == NULL)
            inrow[2] = pbm_allocrow(cols);
        pbm_readpbmrow(ifd, inrow[2], cols, format);
    } else {
        /* There is no next row */
        if (inrow[2]) {
            pbm_freerow(inrow[2]);
            inrow[2] = NULL; 
        }
    }
}



static unsigned int
likeNeighbors(bit *        const inrow[3], 
              unsigned int const col, 
              unsigned int const cols) {
    
    int const point = inrow[THISROW][col];
    enum compass_heading heading;
    int joined;

    joined = 0;  /* initial value */
    for (heading = WEST; heading <= SOUTHWEST; ++heading) {
        int x = col + xd[heading] ;
        int y = THISROW + yd[heading] ;
        if (x < 0 || x >= cols || !inrow[y]) {
            if (point == PBM_WHITE) joined++;
        } else if (inrow[y][x] == point) joined++ ;
    }
    return joined;
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE *ifp;
    bit *outrow;
    int cols, rows, format;
    unsigned int row;
    unsigned int nFlipped;  /* Number of pixels we have flipped so far */

    pbm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    ifp = pm_openr(cmdline.inputFilespec);

    inrow[0] = inrow[1] = inrow[2] = NULL;
    pbm_readpbminit(ifp, &cols, &rows, &format);

    outrow = pbm_allocrow(cols);

    pbm_writepbminit(stdout, cols, rows, 0) ;

    nFlipped = 0;  /* No pixels flipped yet */
    for (row = 0; row < rows; ++row) {
        unsigned int col;
        nextrow(ifp, row, cols, rows, format);
        for (col = 0; col < cols; ++col) {
            bit const thispoint = inrow[THISROW][col];
            if ((cmdline.flipWhite && thispoint == PBM_WHITE) ||
                (cmdline.flipBlack && thispoint == PBM_BLACK)) {
                if (likeNeighbors(inrow, col, cols) < cmdline.connect) {
                    outrow[col] = PBM_INVERT(thispoint);
                    ++nFlipped;
                } else
                    outrow[col] = thispoint;
            } else 
                outrow[col] = thispoint;
        }
        pbm_writepbmrow(stdout, outrow, cols, 0) ;
    }
    pbm_freerow(outrow);
    pm_close(ifp);

    if (cmdline.verbose)
        pm_message("%d pixels flipped", nFlipped);

    return 0;
}
