 /* pnmcut.c - cut a rectangle out of a portable anymap
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

#include <limits.h>
#include "pnm.h"
#include "shhopt.h"

#define UNSPEC INT_MAX
    /* UNSPEC is the value we use for an argument that is not specified
       by the user.  Theoretically, the user could specify this value,
       but we hope not.
       */

struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespecs of input files */

    /* The following describe the rectangle the user wants to cut out. 
       the value UNSPEC for any of them indicates that value was not
       specified.  A negative value means relative to the far edge.
       'width' and 'height' are not negative.  These specifications 
       do not necessarily describe a valid rectangle; they are just
       what the user said.
       */
    int left;
    int right;
    int top;
    int bottom;
    int width;
    int height;
    int pad;

    int verbose;
};



static xel black_xel;  /* A black xel */


static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdline_p) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optStruct *option_def = malloc(100*sizeof(optStruct));
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct2 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENTRY(0,   "left",       OPT_INT,    &cmdline_p->left,           0);
    OPTENTRY(0,   "right",      OPT_INT,    &cmdline_p->right,          0);
    OPTENTRY(0,   "top",        OPT_INT,    &cmdline_p->top,            0);
    OPTENTRY(0,   "bottom",     OPT_INT,    &cmdline_p->bottom,         0);
    OPTENTRY(0,   "width",      OPT_INT,    &cmdline_p->width,          0);
    OPTENTRY(0,   "height",     OPT_INT,    &cmdline_p->height,         0);
    OPTENTRY(0,   "pad",        OPT_FLAG,   &cmdline_p->pad,            0);
    OPTENTRY(0,   "verbose",    OPT_FLAG,   &cmdline_p->verbose,        0);

    /* Set the defaults */
    cmdline_p->left = UNSPEC;
    cmdline_p->right = UNSPEC;
    cmdline_p->top = UNSPEC;
    cmdline_p->bottom = UNSPEC;
    cmdline_p->width = UNSPEC;
    cmdline_p->height = UNSPEC;
    cmdline_p->pad = FALSE;
    cmdline_p->verbose = FALSE;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = TRUE;  /* We may have parms that are negative numbers */

    optParseOptions2(&argc, argv, opt, 0);
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (cmdline_p->width < 0)
        pm_error("-width may not be negative.");
    if (cmdline_p->height < 0)
        pm_error("-height may not be negative.");

    if ((argc-1) != 0 && (argc-1) != 1 && (argc-1) != 4 && (argc-1) != 5)
        pm_error("Wrong number of arguments.  "
                 "Must be 0, 1, 4, or 5 arguments.");

    switch (argc-1) {
    case 0:
        cmdline_p->input_filespec = "-";
        break;
    case 1:
        cmdline_p->input_filespec = argv[1];
        break;
    case 4:
    case 5: {
        int warg, harg;  /* The "width" and "height" command line arguments */

        if (sscanf(argv[1], "%d", &cmdline_p->left) != 1)
            pm_error("Invalid number for left column argument");
        if (sscanf(argv[2], "%d", &cmdline_p->top) != 1)
            pm_error("Invalid number for top row argument");
        if (sscanf(argv[3], "%d", &warg) != 1)
            pm_error("Invalid number for width argument");
        if (sscanf(argv[4], "%d", &harg) != 1)
            pm_error("Invalid number for height argument");

        if (warg > 0) {
            cmdline_p->width = warg;
            cmdline_p->right = UNSPEC;
        } else {
            cmdline_p->width = UNSPEC;
            cmdline_p->right = warg -1;
        }
        if (harg > 0) {
            cmdline_p->height = harg;
            cmdline_p->bottom = UNSPEC;
        } else {
            cmdline_p->height = UNSPEC;
            cmdline_p->bottom = harg - 1;
        }

        if (argc-1 == 4)
            cmdline_p->input_filespec = "-";
        else
            cmdline_p->input_filespec = argv[5];
        break;
    }
    }
}



static void
compute_cut_bounds(const int cols, const int rows,
                   const int leftarg, const int rightarg, 
                   const int toparg, const int bottomarg,
                   const int widtharg, const int heightarg,
                   int * const leftcol_p, int * const rightcol_p,
                   int * const toprow_p, int * const bottomrow_p) {
/*----------------------------------------------------------------------------
   From the values given on the command line 'leftarg', 'rightarg',
   'toparg', 'bottomarg', 'widtharg', and 'heightarg', determine what
   rectangle the user wants cut out.

   Any of these arguments may be UNSPEC to indicate "not specified".
   Any except 'widtharg' and 'heightarg' may be negative to indicate
   relative to the far edge.  'widtharg' and 'heightarg' are positive.

   Return the location of the rectangle as *leftcol_p, *rightcol_p,
   *toprow_p, and *bottomrow_p.  
-----------------------------------------------------------------------------*/

    int leftcol, rightcol, toprow, bottomrow;
        /* The left and right column numbers and top and bottom row numbers
           specified by the user, except with negative values translated
           into the actual values.

           Note that these may very well be negative themselves, such
           as when the user says "column -10" and there are only 5 columns
           in the image.
           */

    /* Translate negative column and row into real column and row */
    /* Exploit the fact that UNSPEC is a positive number */

    if (leftarg >= 0)
        leftcol = leftarg;
    else
        leftcol = cols + leftarg;
    if (rightarg >= 0)
        rightcol = rightarg;
    else
        rightcol = cols + rightarg;
    if (toparg >= 0)
        toprow = toparg;
    else
        toprow = rows + toparg;
    if (bottomarg >= 0)
        bottomrow = bottomarg;
    else
        bottomrow = rows + bottomarg;

    /* Sort out left, right, and width specifications */

    if (leftcol == UNSPEC && rightcol == UNSPEC && widtharg == UNSPEC) {
        *leftcol_p = 0;
        *rightcol_p = cols - 1;
    }
    if (leftcol == UNSPEC && rightcol == UNSPEC && widtharg != UNSPEC) {
        *leftcol_p = 0;
        *rightcol_p = 0 + widtharg - 1;
    }
    if (leftcol == UNSPEC && rightcol != UNSPEC && widtharg == UNSPEC) {
        *leftcol_p = 0;
        *rightcol_p = rightcol;
    }
    if (leftcol == UNSPEC && rightcol != UNSPEC && widtharg != UNSPEC) {
        *leftcol_p = rightcol - widtharg + 1;
        *rightcol_p = rightcol;
    }
    if (leftcol != UNSPEC && rightcol == UNSPEC && widtharg == UNSPEC) {
        *leftcol_p = leftcol;
        *rightcol_p = cols - 1;
    }
    if (leftcol != UNSPEC && rightcol == UNSPEC && widtharg != UNSPEC) {
        *leftcol_p = leftcol;
        *rightcol_p = leftcol + widtharg - 1;
    }
    if (leftcol != UNSPEC && rightcol != UNSPEC && widtharg == UNSPEC) {
        *leftcol_p = leftcol;
        *rightcol_p = rightcol;
    }
    if (leftcol != UNSPEC && rightcol != UNSPEC && widtharg != UNSPEC) {
        pm_error("You may not specify left, right, and width.\n"
                 "Choose at most two of these.");
    }


    /* Sort out top, bottom, and height specifications */

    if (toprow == UNSPEC && bottomrow == UNSPEC && heightarg == UNSPEC) {
        *toprow_p = 0;
        *bottomrow_p = rows - 1;
    }
    if (toprow == UNSPEC && bottomrow == UNSPEC && heightarg != UNSPEC) {
        *toprow_p = 0;
        *bottomrow_p = 0 + heightarg - 1;
    }
    if (toprow == UNSPEC && bottomrow != UNSPEC && heightarg == UNSPEC) {
        *toprow_p = 0;
        *bottomrow_p = bottomrow;
    }
    if (toprow == UNSPEC && bottomrow != UNSPEC && heightarg != UNSPEC) {
        *toprow_p = bottomrow - heightarg + 1;
        *bottomrow_p = bottomrow;
    }
    if (toprow != UNSPEC && bottomrow == UNSPEC && heightarg == UNSPEC) {
        *toprow_p = toprow;
        *bottomrow_p = rows - 1;
    }
    if (toprow != UNSPEC && bottomrow == UNSPEC && heightarg != UNSPEC) {
        *toprow_p = toprow;
        *bottomrow_p = toprow + heightarg - 1;
    }
    if (toprow != UNSPEC && bottomrow != UNSPEC && heightarg == UNSPEC) {
        *toprow_p = toprow;
        *bottomrow_p = bottomrow;
    }
    if (toprow != UNSPEC && bottomrow != UNSPEC && heightarg != UNSPEC) {
        pm_error("You may not specify top, bottom, and height.\n"
                 "Choose at most two of these.");
    }

}



static void
reject_out_of_bounds(const int cols, const int rows, 
                     const int leftcol, const int rightcol, 
                     const int toprow, const int bottomrow) {

    /* Reject coordinates off the edge */

    if (leftcol < 0)
        pm_error("You have specified a left edge (%d) that is beyond\n"
                 "the left edge of the image (0)", leftcol);
    if (rightcol > cols-1)
        pm_error("You have specified a right edge (%d) that is beyond\n"
                 "the right edge of the image (%d)", rightcol, cols-1);
    if (rightcol < 0)
        pm_error("You have specified a right edge (%d) that is beyond\n"
                 "the left edge of the image (0)", rightcol);
    if (rightcol > cols-1)
        pm_error("You have specified a right edge (%d) that is beyond\n"
                 "the right edge of the image (%d)", rightcol, cols-1);
    if (leftcol > rightcol) 
        pm_error("You have specified a left edge (%d) that is to the right\n"
                 "of the right edge you specified (%d)", 
                 leftcol, rightcol);
    
    if (toprow < 0)
        pm_error("You have specified a top edge (%d) that is above the top "
                 "edge of the image (0)", toprow);
    if (bottomrow > rows-1)
        pm_error("You have specified a bottom edge (%d) that is below the\n"
                 "bottom edge of the image (%d)", bottomrow, rows-1);
    if (bottomrow < 0)
        pm_error("You have specified a bottom edge (%d) that is above the\n"
                 "top edge of the image (0)", bottomrow);
    if (bottomrow > rows-1)
        pm_error("You have specified a bottom edge (%d) that is below the\n"
                 "bottom edge of the image (%d)", bottomrow, rows-1);
    if (toprow > bottomrow) 
        pm_error("You have specified a top edge (%d) that is below\n"
                 "the bottom edge you specified (%d)", 
                 toprow, bottomrow);
}



static void
write_black_rows(FILE *outfile, const int rows, const int cols, 
                 xel * const output_row,
                 const pixval maxval, const int format) {
/*----------------------------------------------------------------------------
   Write out to file 'outfile' 'rows' rows of 'cols' black xels each,
   part of an image of format 'format' with maxval 'maxval'.  

   Use *output_row as a buffer.  It is at least 'cols' xels wide.
-----------------------------------------------------------------------------*/
    int row;
    for (row = 0; row < rows; row++) {
        int col;
        for (col = 0; col < cols; col++) output_row[col] = black_xel;
        pnm_writepnmrow(outfile, output_row, cols, maxval, format, 0);
    }
}



int
main(int argc, char *argv[]) {

    FILE* ifp;
    xel* xelrow;  /* Row from input image */
    xel* output_row;  /* Row of output image */
    xelval maxval;
    int rows, cols, format, row;
    int leftcol, rightcol, toprow, bottomrow;
    int output_cols;  /* Width of output image */
    struct cmdline_info cmdline;

    pnm_init( &argc, argv );

    parse_command_line(argc, argv, &cmdline);

    ifp = pm_openr(cmdline.input_filespec);

    pnm_readpnminit(ifp, &cols, &rows, &maxval, &format);
    xelrow = pnm_allocrow(cols);

    black_xel = pnm_blackxel(maxval, format);

    compute_cut_bounds(cols, rows, 
                       cmdline.left, cmdline.right, 
                       cmdline.top, cmdline.bottom, 
                       cmdline.width, cmdline.height, 
                       &leftcol, &rightcol, &toprow, &bottomrow);

    if (!cmdline.pad)
        reject_out_of_bounds(cols, rows, leftcol, rightcol, toprow, bottomrow);

    if (cmdline.verbose) {
        pm_message("Image goes from Row 0, Column 0 through Row %d, Column %d",
                   rows-1, cols-1);
        pm_message("Cutting from Row %d, Column %d through Row %d Column %d",
                   toprow, leftcol, bottomrow, rightcol);
    }

    output_cols = rightcol-leftcol+1;
    output_row = pnm_allocrow(output_cols);
    
    pnm_writepnminit(stdout, output_cols, bottomrow-toprow+1, 
                     maxval, format, 0 );

    /* Implementation note:  If speed is ever an issue, we can probably
       speed up significantly the non-padding case by writing a special
       case loop here for the case cmdline.pad == FALSE.
       */

    /* Write out top padding */
    write_black_rows(stdout, 0 - toprow, output_cols, output_row, 
                     maxval, format);
    
    /* Read input and write out rows extracted from it */
    for (row = 0; row < rows; row++) {
        pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
        if (row >= toprow && row <= bottomrow) {
            int col;
            /* Put in left padding */
            for (col = leftcol; col < 0; col++) { 
                output_row[col-leftcol] = black_xel;
            }
            /* Put in extracted columns */
            for (col = MAX(leftcol, 0); col <= MIN(rightcol, cols-1); col++) {
                output_row[col-leftcol] = xelrow[col];
            }
            /* Put in right padding */
            for (col = MAX(cols, leftcol); col <= rightcol; col++) {
                output_row[col-leftcol] = black_xel;
            }
            pnm_writepnmrow(stdout, output_row, output_cols, 
                            maxval, format, 0);
        }
    }
    /* Note that we may be tempted just to quit after reaching the bottom
       of the extracted image, but that would cause a broken pipe problem
       for the process that's feeding us the image.
       */
    /* Write out bottom padding */
    write_black_rows(stdout, bottomrow - (rows-1), output_cols, output_row, 
                     maxval, format);

    pnm_freerow(output_row);
    pnm_freerow(xelrow);
    pm_close(ifp);
    pm_close(stdout);
    
    exit( 0 );
}

