#include <string.h>

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"



struct cmdlineInfo {
    tuple colorTopLeft;
    tuple colorTopRight;
    tuple colorBottomLeft;
    tuple colorBottomRight;
    unsigned depth;
    unsigned int cols;
    unsigned int rows;
    unsigned int maxval;
};

static void
parseCommandLine(int argc, char **argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
  Convert program invocation arguments (argc,argv) into a format the 
  program can use easily, struct cmdlineInfo.  Validate arguments along
  the way and exit program with message if invalid.

  Note that some string information we return as *cmdlineP is in the storage 
  argv[] points to.
-----------------------------------------------------------------------------*/
    optEntry * option_def;
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int maxvalSpec;
    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;
    OPTENT3(0, "maxval", OPT_UINT, &cmdlineP->maxval, &maxvalSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!maxvalSpec)
        cmdlineP->maxval = 255;
    else {
        if (cmdlineP->maxval > PAM_OVERALL_MAXVAL)
            pm_error("The value you specified for -maxval (%u) is too big.  "
                     "Max allowed is %u", cmdlineP->maxval,
                     PAM_OVERALL_MAXVAL);
        
        if (cmdlineP->maxval < 1)
            pm_error("You cannot specify 0 for -maxval");
    }    

    if (argc-1 != 6) {
        pm_error("Need 6 arguments: colorTopLeft, colorTopRight, "
                 "colorBottomLeft, colorBottomRight, width, height"); 
    } else {
        cmdlineP->colorTopLeft     = pnm_parsecolor(argv[1], cmdlineP->maxval);
        cmdlineP->colorTopRight    = pnm_parsecolor(argv[2], cmdlineP->maxval);
        cmdlineP->colorBottomLeft  = pnm_parsecolor(argv[3], cmdlineP->maxval);
        cmdlineP->colorBottomRight = pnm_parsecolor(argv[4], cmdlineP->maxval);
        cmdlineP->cols = atoi(argv[5]);
        cmdlineP->rows = atoi(argv[6]);
        if (cmdlineP->cols <= 0)
            pm_error("width argument must be a positive number.  You "
                     "specified '%s'", argv[5]);
        if (cmdlineP->rows <= 0)
            pm_error("height argument must be a positive number.  You "
                     "specified '%s'", argv[6]);
    }
}



static void
freeCmdline(struct cmdlineInfo const cmdline) {

    pnm_freepamtuple(cmdline.colorTopLeft);
    pnm_freepamtuple(cmdline.colorTopRight);
    pnm_freepamtuple(cmdline.colorBottomLeft);
    pnm_freepamtuple(cmdline.colorBottomRight);
}



static void
interpolate(struct pam * const pamP,
            tuple *      const tuplerow,
            tuple        const first,
            tuple        const last) {

    unsigned int plane;
    
    for (plane = 0; plane < pamP->depth; ++plane) {
        int const spread = last[plane] - first[plane];

        int col;

        if (INT_MAX / pamP->width < abs(spread))
            pm_error("Arithmetic overflow.  You must reduce the width of "
                     "the image (now %u) or the range of color values "
                     "(%u in plane %u) so that their "
                     "product is less than %d",
                     pamP->width, abs(spread), plane, INT_MAX);

        for (col = 0; col < pamP->width; ++col)
            tuplerow[col][plane] =
                first[plane] + (spread * col / (int)pamP->width);
    }
}



static int
isgray(struct pam * const pamP,
       tuple        const color) {

    return (color[PAM_RED_PLANE] == color[PAM_GRN_PLANE])
            && (color[PAM_RED_PLANE] == color[PAM_BLU_PLANE]);
}



static tuple *
createEdge(const struct pam * const pamP,
           tuple              const topColor,
           tuple              const bottomColor) {
/*----------------------------------------------------------------------------
   Create a left or right edge, interpolating from top to bottom.
-----------------------------------------------------------------------------*/
    struct pam interpPam;
    tuple * tupleRow;

    interpPam = *pamP;  /* initial value */
    interpPam.width = pamP->height;
    interpPam.height = 1;

    tupleRow = pnm_allocpamrow(&interpPam);

    interpolate(&interpPam, tupleRow, topColor, bottomColor);

    return tupleRow;
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    struct pam pam;
    tuple * tupleRow;
    tuple * leftEdge;
    tuple * rightEdge;
    unsigned int row;
    
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    pam.size             = sizeof pam;
    pam.len              = PAM_STRUCT_SIZE(tuple_type);
    pam.file             = stdout;
    pam.plainformat      = 0;
    pam.width            = cmdline.cols;
    pam.height           = cmdline.rows;
    pam.maxval           = cmdline.maxval;
    pam.bytes_per_sample = pnm_bytespersample(pam.maxval);
    pam.format           = PAM_FORMAT;
    if (isgray(&pam, cmdline.colorTopLeft)
            && isgray(&pam, cmdline.colorTopRight)
            && isgray(&pam, cmdline.colorBottomLeft)
            && isgray(&pam, cmdline.colorBottomRight)) {
        pam.depth = 1;
        strcpy(pam.tuple_type, PAM_PGM_TUPLETYPE);
    } else {
        pam.depth = 3;
        strcpy(pam.tuple_type, PAM_PPM_TUPLETYPE);
    }

    pnm_writepaminit(&pam);
    
    tupleRow = pnm_allocpamrow(&pam);

    leftEdge  = createEdge(&pam,
                           cmdline.colorTopLeft, cmdline.colorBottomLeft);
    rightEdge = createEdge(&pam,
                           cmdline.colorTopRight, cmdline.colorBottomRight);

    /* interpolate each row between the left edge and the right edge */
    for (row = 0; row < pam.height; ++row) {
        interpolate(&pam, tupleRow, leftEdge[row], rightEdge[row]);
        pnm_writepamrow(&pam, tupleRow); 
    }

    pm_close(stdout);
    pnm_freepamrow(rightEdge);
    pnm_freepamrow(leftEdge);
    pnm_freepamrow(tupleRow);

    freeCmdline(cmdline);

    return 0;
}
