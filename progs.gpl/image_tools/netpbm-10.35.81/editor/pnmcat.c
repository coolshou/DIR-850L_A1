/* pnmcat.c - concatenate portable anymaps
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "pnm.h"
#include "mallocvar.h"
#include "shhopt.h"


enum backcolor {BACK_BLACK, BACK_WHITE, BACK_AUTO};

enum orientation {TOPBOTTOM, LEFTRIGHT};

enum justification {JUST_CENTER, JUST_MIN, JUST_MAX};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char **inputFilespec;  
    unsigned int nfiles;
    enum backcolor backcolor;
    enum orientation orientation;
    enum justification justification;
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
    
    unsigned int leftright, topbottom, black, white, jtop, jbottom,
        jleft, jright, jcenter;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "leftright",  OPT_FLAG,   NULL, &leftright,   0);
    OPTENT3(0, "lr",         OPT_FLAG,   NULL, &leftright,   0);
    OPTENT3(0, "topbottom",  OPT_FLAG,   NULL, &topbottom,   0);
    OPTENT3(0, "tb",         OPT_FLAG,   NULL, &topbottom,   0);
    OPTENT3(0, "black",      OPT_FLAG,   NULL, &black,       0);
    OPTENT3(0, "white",      OPT_FLAG,   NULL, &white,       0);
    OPTENT3(0, "jtop",       OPT_FLAG,   NULL, &jtop,        0);
    OPTENT3(0, "jbottom",    OPT_FLAG,   NULL, &jbottom,     0);
    OPTENT3(0, "jleft",      OPT_FLAG,   NULL, &jleft,       0);
    OPTENT3(0, "jright",     OPT_FLAG,   NULL, &jright,      0);
    OPTENT3(0, "jcenter",    OPT_FLAG,   NULL, &jcenter,     0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (leftright + topbottom > 1)
        pm_error("You may specify only one of -topbottom (-tb) and "
                 "-leftright (-lr)");
    else if (leftright)
        cmdlineP->orientation = LEFTRIGHT;
    else if (topbottom)
        cmdlineP->orientation = TOPBOTTOM;
    else
        pm_error("You must specify either -leftright or -topbottom");

    if (black + white > 1)
        pm_error("You may specify only one of -black and -white");
    else if (black)
        cmdlineP->backcolor = BACK_BLACK;
    else if (white)
        cmdlineP->backcolor = BACK_WHITE;
    else
        cmdlineP->backcolor = BACK_AUTO;

    if (jtop + jbottom + jleft + jright + jcenter > 1)
        pm_error("You may specify onlyone of -jtop, -jbottom, "
                 "-jleft, and -jright");
    else {
        switch (cmdlineP->orientation) {
        case LEFTRIGHT:
            if (jleft)
                pm_error("-jleft is invalid with -leftright");
            if (jright)
                pm_error("-jright is invalid with -leftright");
            if (jtop)
                cmdlineP->justification = JUST_MIN;
            else if (jbottom)
                cmdlineP->justification = JUST_MAX;
            else if (jcenter)
                cmdlineP->justification = JUST_CENTER;
            else
                cmdlineP->justification = JUST_CENTER;
            break;
        case TOPBOTTOM:
            if (jtop)
                pm_error("-jtop is invalid with -topbottom");
            if (jbottom)
                pm_error("-jbottom is invalid with -topbottom");
            if (jleft)
                cmdlineP->justification = JUST_MIN;
            else if (jright)
                cmdlineP->justification = JUST_MAX;
            else if (jcenter)
                cmdlineP->justification = JUST_CENTER;
            else
                cmdlineP->justification = JUST_CENTER;
            break;
        }
    }

    if (argc-1 < 1) {
        MALLOCARRAY_NOFAIL(cmdlineP->inputFilespec, 1);
        cmdlineP->inputFilespec[0] = "-";
        cmdlineP->nfiles = 1;
    } else {
        unsigned int i;

        MALLOCARRAY_NOFAIL(cmdlineP->inputFilespec, argc-1);
        
        for (i = 0; i < argc-1; ++i)
            cmdlineP->inputFilespec[i] = argv[1+i];
        cmdlineP->nfiles = argc-1;
    }
}        



static void
computeOutputParms(unsigned int     const nfiles,
                   enum orientation const orientation,
                   int                    cols[], 
                   int                    rows[],
                   xelval                 maxval[],
                   int                    format[],
                   int *            const newcolsP,
                   int *            const newrowsP,
                   xelval *         const newmaxvalP,
                   int *            const newformatP) {

    int newcols, newrows;
    int newformat;
    xelval newmaxval;

    unsigned int i;

    newcols = 0;
    newrows = 0;

    for (i = 0; i < nfiles; ++i)	{
        if (i == 0) {
            newmaxval = maxval[i];
            newformat = format[i];
	    } else {
            if (PNM_FORMAT_TYPE(format[i]) > PNM_FORMAT_TYPE(newformat))
                newformat = format[i];
            if (maxval[i] > newmaxval)
                newmaxval = maxval[i];
	    }
        switch (orientation) {
        case LEFTRIGHT:
            newcols += cols[i];
            if (rows[i] > newrows)
                newrows = rows[i];
            break;
        case TOPBOTTOM:
            newrows += rows[i];
            if (cols[i] > newcols)
                newcols = cols[i];
            break;
	    }
	}
    *newrowsP   = newrows;
    *newcolsP   = newcols;
    *newmaxvalP = newmaxval;
    *newformatP = newformat;
}



static void
concatenateLeftRight(FILE *             const ofp,
                     unsigned int       const nfiles,
                     int                const newcols,
                     int                const newrows,
                     xelval             const newmaxval,
                     int                const newformat,
                     enum justification const justification,
                     FILE *                   ifp[],
                     int                      cols[],
                     int                      rows[],
                     xelval                   maxval[],
                     int                      format[],
                     xel *                    xelrow[],
                     xel                      background[]) {

    unsigned int row;
    
    xel * const newxelrow = pnm_allocrow(newcols);

    for (row = 0; row < newrows; ++row) {
        unsigned int new;
        unsigned int i;

        new = 0;
        for (i = 0; i < nfiles; ++i) {
            int padtop;

            switch (justification) {
            case JUST_MIN:
                padtop = 0;
                break;
            case JUST_MAX:
                padtop = newrows - rows[i];
                break;
            case JUST_CENTER:
                padtop = ( newrows - rows[i] ) / 2;
                break;
            }
            if (row < padtop || row >= padtop + rows[i]) {
                unsigned int col;
                for (col = 0; col < cols[i]; ++col)
                    newxelrow[new+col] = background[i];
            } else {
                if (row != padtop) {
                    /* first row already read */
                    pnm_readpnmrow(
                        ifp[i], xelrow[i], cols[i], maxval[i], format[i] );
                    pnm_promoteformatrow(
                        xelrow[i], cols[i], maxval[i], format[i],
                        newmaxval, newformat );
                }
                {
                    unsigned int col;
                    for (col = 0; col < cols[i]; ++col)
                        newxelrow[new+col] = xelrow[i][col];
                }
            }
            new += cols[i];
        }
        pnm_writepnmrow(ofp, newxelrow, newcols, newmaxval, newformat, 0);
    }
}



static void
concatenateTopBottom(FILE *             const ofp,
                     unsigned int       const nfiles,
                     int                const newcols,
                     int                const newrows,
                     xelval             const newmaxval,
                     int                const newformat,
                     enum justification const justification,
                     FILE *                   ifp[],
                     int                      cols[],
                     int                      rows[],
                     xelval                   maxval[],
                     int                      format[],
                     xel *                    xelrow[],
                     xel                      background[]) {

    int new;
    xel * const newxelrow = pnm_allocrow(newcols);
    int padleft;
    unsigned int i;
    unsigned int row;
    
    i = 0;
    switch (justification) {
    case JUST_MIN:
        padleft = 0;
        break;
    case JUST_MAX:
        padleft = newcols - cols[i];
        break;
    case JUST_CENTER:
        padleft = (newcols - cols[i]) / 2;
        break;
    }

    new = 0;

    for (row = 0; row < newrows; ++row) {
        if (row - new >= rows[i]) {
            new += rows[i];
            ++i;
            if (i >= nfiles)
                pm_error("INTERNAL ERROR: i > nfiles");
            switch (justification) {
            case JUST_MIN:
                padleft = 0;
                break;
            case JUST_MAX:
                padleft = newcols - cols[i];
                break;
            case JUST_CENTER:
                padleft = (newcols - cols[i]) / 2;
                break;
            }
        }
        if (row - new > 0) {
            pnm_readpnmrow(
                ifp[i], xelrow[i], cols[i], maxval[i], format[i]);
            pnm_promoteformatrow(
                xelrow[i], cols[i], maxval[i], format[i],
                newmaxval, newformat);
        }
        {
            unsigned int col;

            for (col = 0; col < padleft; ++col)
                newxelrow[col] = background[i];
            for (col = 0; col < cols[i]; ++col)
                newxelrow[padleft+col] = xelrow[i][col];
            for (col = padleft + cols[i]; col < newcols; ++col)
                newxelrow[col] = background[i];
        }
        pnm_writepnmrow(ofp,
                        newxelrow, newcols, newmaxval, newformat, 0);
	}
}



int
main(int argc, char ** argv) {

    struct cmdlineInfo cmdline;
    FILE** ifp;
    xel** xelrow;
    xel* background;
    xelval* maxval;
    xelval newmaxval;
    int* rows;
    int* cols;
    int* format;
    int newformat;
    unsigned int i;
    int newrows, newcols;

    pnm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);
    
    MALLOCARRAY_NOFAIL(ifp,        cmdline.nfiles);
    MALLOCARRAY_NOFAIL(xelrow,     cmdline.nfiles);
    MALLOCARRAY_NOFAIL(background, cmdline.nfiles);
    MALLOCARRAY_NOFAIL(maxval,     cmdline.nfiles);
    MALLOCARRAY_NOFAIL(rows,       cmdline.nfiles);
    MALLOCARRAY_NOFAIL(cols,       cmdline.nfiles);
    MALLOCARRAY_NOFAIL(format,     cmdline.nfiles);

    for (i = 0; i < cmdline.nfiles; ++i) {
        ifp[i] = pm_openr(cmdline.inputFilespec[i]);
        pnm_readpnminit(ifp[i], &cols[i], &rows[i], &maxval[i], &format[i]);
        xelrow[i] = pnm_allocrow(cols[i]);
    }

    computeOutputParms(cmdline.nfiles, cmdline.orientation,
                       cols, rows, maxval, format,
                       &newcols, &newrows, &newmaxval, &newformat);

    for (i = 0; i < cmdline.nfiles; ++i) {
        /* Read first row just to get a good guess at the background. */
        pnm_readpnmrow(ifp[i], xelrow[i], cols[i], maxval[i], format[i]);
        pnm_promoteformatrow(
            xelrow[i], cols[i], maxval[i], format[i], newmaxval, newformat);
        switch (cmdline.backcolor) {
        case BACK_AUTO:
            background[i] =
                pnm_backgroundxelrow(
                    xelrow[i], cols[i], newmaxval, newformat);
            break;
        case BACK_BLACK:
            background[i] = pnm_blackxel(newmaxval, newformat);
            break;
        case BACK_WHITE:
            background[i] = pnm_whitexel(newmaxval, newformat);
            break;
        }
	}

    pnm_writepnminit(stdout, newcols, newrows, newmaxval, newformat, 0);

    switch (cmdline.orientation) {
    case LEFTRIGHT:
        concatenateLeftRight(stdout, cmdline.nfiles,
                             newcols, newrows, newmaxval, newformat,
                             cmdline.justification,
                             ifp, cols, rows, maxval, format, xelrow,
                             background);
        break;
    case TOPBOTTOM:
        concatenateTopBottom(stdout, cmdline.nfiles,
                             newcols, newrows, newmaxval, newformat,
                             cmdline.justification,
                             ifp, cols, rows, maxval, format, xelrow,
                             background);
        break;
    }
    free(cmdline.inputFilespec);

    for (i = 0; i < cmdline.nfiles; ++i)
        pm_close(ifp[i]);

    pm_close(stdout);

    return 0;
}
