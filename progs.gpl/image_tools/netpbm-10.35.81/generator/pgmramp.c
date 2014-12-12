/* pgmramp.c - generate a grayscale ramp
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <math.h>
#include "pgm.h"
#include "shhopt.h"

enum ramptype {RT_LR, RT_TB, RT_RECT, RT_ELLIP};


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    enum ramptype ramptype;
    unsigned int cols;
    unsigned int rows;
    gray maxval;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
  Convert program invocation arguments (argc,argv) into a format the 
  program can use easily, struct cmdlineInfo.  Validate arguments along
  the way and exit program with message if invalid.

  Note that some string information we return as *cmdlineP is in the storage 
  argv[] points to.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int lrSpec, tbSpec, rectangleSpec, ellipseSpec;
    unsigned int maxvalSpec;
    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0,   "lr",        OPT_FLAG, NULL,              &lrSpec,        0);
    OPTENT3(0,   "tb",        OPT_FLAG, NULL,              &tbSpec,        0);
    OPTENT3(0,   "rectangle", OPT_FLAG, NULL,              &rectangleSpec, 0);
    OPTENT3(0,   "ellipse",   OPT_FLAG, NULL,              &ellipseSpec,   0);
    OPTENT3(0,   "maxval",    OPT_UINT, &cmdlineP->maxval, &maxvalSpec,    0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (lrSpec + tbSpec + rectangleSpec + ellipseSpec == 0)
        pm_error("You must specify one of -lr, -tb, -rectangle, or -ellipse");
    if (lrSpec + tbSpec + rectangleSpec + ellipseSpec > 1)
        pm_error("You may specify at most one of "
                 "-lr, -tb, -rectangle, or -ellipse");
    if (lrSpec)
        cmdlineP->ramptype = RT_LR;
    else if (tbSpec)
        cmdlineP->ramptype = RT_TB;
    else if (rectangleSpec)
        cmdlineP->ramptype = RT_RECT;
    else if (ellipseSpec)
        cmdlineP->ramptype = RT_ELLIP;
    else
        pm_error("INTERNAL ERROR - no ramp type option found");

    if (!maxvalSpec)
        cmdlineP->maxval = PGM_MAXMAXVAL;
    else {
        if (cmdlineP->maxval > PGM_OVERALLMAXVAL)
            pm_error("The value you specified for -maxval (%u) is too big.  "
                     "Max allowed is %u", cmdlineP->maxval, PGM_OVERALLMAXVAL);
        
        if (cmdlineP->maxval < 1)
            pm_error("You cannot specify 0 for -maxval");
    }    

    if (argc-1 < 2)
        pm_error("Need two arguments: width and height.");
    else if (argc-1 > 2)
        pm_error("Only two arguments allowed: width and height.  "
                 "You specified %d", argc-1);
    else {
        cmdlineP->cols = atoi(argv[1]);
        cmdlineP->rows = atoi(argv[2]);
        if (cmdlineP->cols <= 0)
            pm_error("width argument must be a positive number.  You "
                     "specified '%s'", argv[1]);
        if (cmdlineP->rows <= 0)
            pm_error("height argument must be a positive number.  You "
                     "specified '%s'", argv[2]);
    }
}



int 
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    gray *grayrow;
    int rowso2, colso2;
    unsigned int row;

    pgm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    colso2 = MAX(1, cmdline.cols / 2);
    rowso2 = MAX(1, cmdline.rows / 2);
    
    pgm_writepgminit(stdout, cmdline.cols, cmdline.rows, cmdline.maxval, 0);
    grayrow = pgm_allocrow(cmdline.cols);
    
    for (row = 0; row < cmdline.rows; ++row) {
        unsigned int col;
        for (col = 0; col < cmdline.cols; ++col) {
            switch (cmdline.ramptype) {
            case RT_LR:
                grayrow[col] = 
                    col * cmdline.maxval / MAX(cmdline.cols-1, 1);
                break;
            case RT_TB:
                grayrow[col] = 
                    row * cmdline.maxval / MAX(cmdline.rows-1, 1);
                break;

            case RT_RECT: {
                float const r = fabs((int)(rowso2 - row)) / rowso2;
                float const c = fabs((int)(colso2 - col)) / colso2;
                grayrow[col] = 
                    cmdline.maxval - (r + c) / 2.0 * cmdline.maxval;
            }
            break;
            
            case RT_ELLIP: {
                float const r = fabs((int)(rowso2 - row)) / rowso2;
                float const c = fabs((int)(colso2 - col)) / colso2;
                float v;

                v = r * r + c * c;
                if ( v < 0.0 ) v = 0.0;
                else if ( v > 1.0 ) v = 1.0;
                grayrow[col] = cmdline.maxval - v * cmdline.maxval;
            }
            break;
            }
	    }
        pgm_writepgmrow(stdout, grayrow, cmdline.cols, cmdline.maxval, 0);
	}

    pgm_freerow(grayrow);
    pm_close(stdout);
    return 0;
}
