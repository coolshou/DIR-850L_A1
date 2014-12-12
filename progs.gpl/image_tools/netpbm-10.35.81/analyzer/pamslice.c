/* 
 *
 * This program (Pamslice) was derived by Bryan Henderson in July 2002
 * from Pgmslice by Jos Dingjan.  Pgmslice did the same thing, but
 * only on PGM images.  Pamslice is a total rewrite.
 *
 * Copyright (C) 2000 Jos Dingjan <jos@tuatha.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

enum orientation {ROW, COLUMN};

#include "pam.h"
#include "shhopt.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    enum orientation orientation;
    union {
        unsigned int row;
        unsigned int col;
    } u;
    unsigned int onePlane;
    unsigned int plane;
    unsigned int xmgr;
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

    unsigned int rowSpec, colSpec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "row",    OPT_UINT,   &cmdlineP->u.row, &rowSpec,            0);
    OPTENT3(0, "column", OPT_UINT,   &cmdlineP->u.col, &colSpec,            0);
    OPTENT3(0, "plane",  OPT_UINT,   &cmdlineP->plane, &cmdlineP->onePlane, 0);
    OPTENT3(0, "xmgr",   OPT_FLAG,   NULL,             &cmdlineP->xmgr, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (rowSpec && colSpec)
        pm_error("You cannot specify both -row and -col");

    if (argc-1 > 1)
        pm_error("Too many arguments (%d).  Only argument is filename.",
                 argc-1);
    else if (argc-1 == 1) 
        cmdlineP->inputFilespec = argv[1];
    else
        cmdlineP->inputFilespec = "-";

    if (rowSpec) 
        cmdlineP->orientation = ROW;
    else if (colSpec)
        cmdlineP->orientation = COLUMN;
    else
        pm_error("You must specify either -column or -row");
}



static void
printSlice(FILE *       const outfile,
           tuple **     const tuples,
           unsigned int const rowstart,
           unsigned int const rowend,
           unsigned int const colstart,
           unsigned int const colend,
           unsigned int const planestart,
           unsigned int const planeend,
           bool         const xmgr) {

    unsigned int count;
    unsigned int row;


    if (xmgr) {
        fprintf(outfile,"@    title \"Graylevel\"\n");
        fprintf(outfile,"@    subtitle \"from (%d,%d) to (%d,%d)\"\n",
                colstart, rowstart, colend, rowend);
        if (colend - colstart == 1)
            fprintf(outfile,"@    xaxis label \"row\"\n");
        else
            fprintf(outfile,"@    xaxis label \"column\"\n");
        fprintf(outfile,"@    yaxis label \"grayvalue\"\n");
        if (planeend - planestart == 1)
            fprintf(stdout,"@    type xy\n");
        else
            fprintf(stdout,"@    type  nxy\n");
    }
    
    count = 0;
    for (row = rowstart; row < rowend; ++row) {
        unsigned int col;
        for (col = colstart; col < colend; ++col) {
            unsigned int plane;
            fprintf(outfile, "%d ", count++);
            for (plane = planestart; plane < planeend; ++plane)
                fprintf(outfile, "%lu ", tuples[row][col][plane]);
            fprintf(outfile, "\n");
        }
    }
}



int 
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE *ifP;
    struct pam inpam;
    tuple **tuples;
    unsigned int colstart, colend, rowstart, rowend, planestart, planeend;

    pgm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    tuples = pnm_readpam(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    switch (cmdline.orientation) {
    case COLUMN:
        if (cmdline.u.col >= inpam.width)
            pm_error("You specified column %u, but there are only %u columns "
                     "in the image.", cmdline.u.col, inpam.width);

        colstart = cmdline.u.col;
        colend   = colstart + 1;
        rowstart = 0;
        rowend   = inpam.height;

        break;
    case ROW:
        if (cmdline.u.row >= inpam.height)
            pm_error("You specified row %u, but there are only %u rows "
                     "in the image.", cmdline.u.row, inpam.height);
        colstart = 0;
        colend   = inpam.width;
        rowstart = cmdline.u.row;
        rowend   = rowstart + 1;

        break;
    }
    
    if (cmdline.onePlane) {
        if (cmdline.plane >= inpam.depth)
            pm_error("You specified plane %u, but there are only %u planes "
                     "in the image.", cmdline.plane, inpam.depth);
        planestart = cmdline.plane;
        planeend = planestart + 1;
    } else {
        planestart = 0;
        planeend = inpam.depth;
    }

    printSlice(stdout, tuples, 
               rowstart, rowend, colstart, colend, planestart, planeend,
               cmdline.xmgr);

    pm_close(ifP);
    pm_close(stdout);
        
    exit(0);
}
