/* pnmcrop.c - crop a portable anymap
**
** Copyright (C) 1988 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/* IDEA FOR EFFICIENCY IMPROVEMENT:

   If we have to read the input into a regular file because it is not
   seekable (a pipe), find the borders as we do the copy, so that we
   do 2 passes through the file instead of 3.  Also find the background
   color in that pass to save yet another pass with -sides.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"

enum bg_choice {BG_BLACK, BG_WHITE, BG_DEFAULT, BG_SIDES};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilespec;
    enum bg_choice background;
    unsigned int left, right, top, bottom;
    unsigned int verbose;
    unsigned int margin;
    const char * borderfile;  /* NULL if none */
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry * option_def;
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int blackOpt, whiteOpt, sidesOpt;
    unsigned int marginSpec, borderfileSpec;
    
    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "black",      OPT_FLAG, NULL, &blackOpt,            0);
    OPTENT3(0, "white",      OPT_FLAG, NULL, &whiteOpt,            0);
    OPTENT3(0, "sides",      OPT_FLAG, NULL, &sidesOpt,            0);
    OPTENT3(0, "left",       OPT_FLAG, NULL, &cmdlineP->left,      0);
    OPTENT3(0, "right",      OPT_FLAG, NULL, &cmdlineP->right,     0);
    OPTENT3(0, "top",        OPT_FLAG, NULL, &cmdlineP->top,       0);
    OPTENT3(0, "bottom",     OPT_FLAG, NULL, &cmdlineP->bottom,    0);
    OPTENT3(0, "verbose",    OPT_FLAG, NULL, &cmdlineP->verbose,   0);
    OPTENT3(0, "margin",     OPT_UINT,   &cmdlineP->margin,    
            &marginSpec,     0);
    OPTENT3(0, "borderfile", OPT_STRING, &cmdlineP->borderfile,
            &borderfileSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 == 0)
        cmdlineP->inputFilespec = "-";  /* stdin */
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else 
        pm_error("Too many arguments (%d).  "
                 "Only need one: the input filespec", argc-1);

    if (blackOpt && whiteOpt)
        pm_error("You cannot specify both -black and -white");
    else if (sidesOpt &&( blackOpt || whiteOpt ))
        pm_error("You cannot specify both -sides and either -black or -white");
    else if (blackOpt)
        cmdlineP->background = BG_BLACK;
    else if (whiteOpt)
        cmdlineP->background = BG_WHITE;
    else if (sidesOpt)
        cmdlineP->background = BG_SIDES;
    else
        cmdlineP->background = BG_DEFAULT;

    if (!cmdlineP->left && !cmdlineP->right && !cmdlineP->top
        && !cmdlineP->bottom) {
        cmdlineP->left = cmdlineP->right = cmdlineP->top 
            = cmdlineP->bottom = TRUE;
    }

    if (!marginSpec)
        cmdlineP->margin = 0;

    if (!borderfileSpec)
        cmdlineP->borderfile = NULL;
}



static xel
background3Corners(FILE * const ifP,
                   int    const rows,
                   int    const cols,
                   pixval const maxval,
                   int    const format) {
/*----------------------------------------------------------------------------
  Read in the whole image, and check all the corners to determine the
  background color.  This is a quite reliable way to determine the
  background color.

  Expect the file to be positioned to the start of the raster, and leave
  it positioned arbitrarily.
----------------------------------------------------------------------------*/
    int row;
    xel ** xels;
    xel background;   /* our return value */

    xels = pnm_allocarray(cols, rows);

    for (row = 0; row < rows; ++row)
        pnm_readpnmrow( ifP, xels[row], cols, maxval, format );

    background = pnm_backgroundxel(xels, cols, rows, maxval, format);

    pnm_freearray(xels, rows);

    return background;
}



static xel
background2Corners(FILE * const ifP,
                   int    const cols,
                   pixval const maxval,
                   int    const format) {
/*----------------------------------------------------------------------------
  Look at just the top row of pixels and determine the background
  color from the top corners; often this is enough to accurately
  determine the background color.

  Expect the file to be positioned to the start of the raster, and leave
  it positioned arbitrarily.
----------------------------------------------------------------------------*/
    xel *xelrow;
    xel background;   /* our return value */
    
    xelrow = pnm_allocrow(cols);

    pnm_readpnmrow(ifP, xelrow, cols, maxval, format);

    background = pnm_backgroundxelrow(xelrow, cols, maxval, format);

    pnm_freerow(xelrow);

    return background;
}



static xel
computeBackground(FILE *         const ifP,
                  int            const cols,
                  int            const rows,
                  xelval         const maxval,
                  int            const format,
                  enum bg_choice const backgroundChoice,
                  int            const verbose) {
/*----------------------------------------------------------------------------
   Determine what color is the background color of the image in file
   *ifP, which is described by 'cols', 'rows', 'maxval', and 'format'.

   'backgroundChoice' is the method we are to use in determining the
   background color.
   
   Expect the file to be positioned to the start of the raster, and leave
   it positioned arbitrarily.
-----------------------------------------------------------------------------*/
    xel background;  /* Our return value */
    
    switch (backgroundChoice) {
    case BG_WHITE:
	    background = pnm_whitexel(maxval, format);
        break;
    case BG_BLACK:
	    background = pnm_blackxel(maxval, format);
        break;
    case BG_SIDES: 
        background = 
            background3Corners(ifP, rows, cols, maxval, format);
        break;
    case BG_DEFAULT: 
        background = 
            background2Corners(ifP, cols, maxval, format);
        break;
    }

    if (verbose) {
        pixel const backgroundPixel = pnm_xeltopixel(background, format);
        pm_message("Background color is %s", 
                   ppm_colorname(&backgroundPixel, maxval, TRUE /*hexok*/));
    }
    return(background);
}



static void
findBordersInImage(FILE *         const ifP,
                   unsigned int   const cols,
                   unsigned int   const rows,
                   xelval         const maxval,
                   int            const format,
                   xel            const backgroundColor,
                   bool           const verbose, 
                   bool *         const hasBordersP,
                   unsigned int * const leftP,
                   unsigned int * const rightP, 
                   unsigned int * const topP,
                   unsigned int * const bottomP) {
/*----------------------------------------------------------------------------
   Find the left, right, top, and bottom borders in the image 'ifP'.
   Return their sizes in pixels as *leftP, *rightP, *topP, and *bottomP.
   
   Iff the image is all background, *hasBordersP == FALSE.

   Expect the input file to be positioned to the beginning of the
   image raster and leave it positioned arbitrarily.
-----------------------------------------------------------------------------*/
    xel* xelrow;        /* A row of the input image */
    int row;
    bool gottop;
    int left, right, bottom, top;
        /* leftmost, etc. nonbackground pixel found so far; -1 for none */

    xelrow = pnm_allocrow(cols);
    
    left   = cols;  /* initial value */
    right  = -1;   /* initial value */
    top    = rows;   /* initial value */
    bottom = -1;  /* initial value */

    gottop = FALSE;
    for (row = 0; row < rows; ++row) {
        int col;
        int thisRowLeft;
        int thisRowRight;

        pnm_readpnmrow(ifP, xelrow, cols, maxval, format);
        
        col = 0;
        while (PNM_EQUAL(xelrow[col], backgroundColor) && col < cols)
            ++col;
        thisRowLeft = col;

        col = cols-1;
        while (col >= thisRowLeft && PNM_EQUAL(xelrow[col], backgroundColor))
            --col;
        thisRowRight = col + 1;

        if (thisRowLeft < cols) {
            /* This row is not entirely background */
            
            left  = MIN(thisRowLeft,  left);
            right = MAX(thisRowRight, right);

            if (!gottop) {
                gottop = TRUE;
                top = row;
            }
            bottom = row + 1;   /* New candidate */
        }
    }

    if (right == -1)
        *hasBordersP = FALSE;
    else {
        *hasBordersP = TRUE;
        assert(right <= cols); assert(bottom <= rows);
        *leftP       = left - 0;
        *rightP      = cols - right;
        *topP        = top - 0;
        *bottomP     = rows - bottom;
    }
}



static void
findBordersInFile(const char *   const borderFileName,
                  xel            const backgroundColor,
                  bool           const verbose, 
                  bool *         const hasBordersP,
                  unsigned int * const leftP,
                  unsigned int * const rightP, 
                  unsigned int * const topP,
                  unsigned int * const bottomP) {

    FILE * borderFileP;
    int cols;
    int rows;
    xelval maxval;
    int format;
    
    borderFileP = pm_openr(borderFileName);
    
    pnm_readpnminit(borderFileP, &cols, &rows, &maxval, &format);
    
    findBordersInImage(borderFileP, cols, rows, maxval, format, 
                       backgroundColor, verbose, hasBordersP,
                       leftP, rightP, topP, bottomP);

    pm_close(borderFileP);
} 



static void
reportOneEdge(unsigned int const oldBorderSize,
              unsigned int const newBorderSize,
              const char * const place) {

#define ending(n) (((n) > 1) ? "s" : "")

    if (newBorderSize > oldBorderSize)
        pm_message("Adding %u pixel%s to the %u-pixel %s border",
                   newBorderSize - oldBorderSize,
                   ending(newBorderSize - oldBorderSize),
                   oldBorderSize, place);
    else if (newBorderSize < oldBorderSize)
        pm_message("Cropping %u pixel%s from the %u-pixel %s border",
                   oldBorderSize - newBorderSize,
                   ending(oldBorderSize - newBorderSize),
                   oldBorderSize, place);
    else
        pm_message("Leaving %s border unchanged at %u pixel%s",
                   place, oldBorderSize, ending(oldBorderSize));
}        



static void
reportCroppingParameters(unsigned int const oldLeftBorderSize,
                         unsigned int const oldRightBorderSize,
                         unsigned int const oldTopBorderSize,
                         unsigned int const oldBottomBorderSize,
                         unsigned int const newLeftBorderSize,
                         unsigned int const newRightBorderSize,
                         unsigned int const newTopBorderSize,
                         unsigned int const newBottomBorderSize) {

    if (oldLeftBorderSize == 0 && oldRightBorderSize == 0 &&
        oldTopBorderSize == 0 && oldBottomBorderSize == 0)
        pm_message("No Border found.");

    reportOneEdge(oldLeftBorderSize,   newLeftBorderSize,   "left"   );
    reportOneEdge(oldRightBorderSize,  newRightBorderSize,  "right"  );
    reportOneEdge(oldTopBorderSize,    newTopBorderSize,    "top"    );
    reportOneEdge(oldBottomBorderSize, newBottomBorderSize, "bottom" );
}




static void
fillRow(xel *        const xelrow,
        unsigned int const cols,
        xel          const color) {

    unsigned int col;

    for (col = 0; col < cols; ++col)
        xelrow[col] = color;
}



static void
writeCropped(FILE *       const ifP,
             unsigned int const cols,
             unsigned int const rows,
             xelval       const maxval,
             int          const format,
             unsigned int const oldLeftBorder,
             unsigned int const oldRightBorder,
             unsigned int const oldTopBorder,
             unsigned int const oldBottomBorder,
             unsigned int const newLeftBorder,
             unsigned int const newRightBorder,
             unsigned int const newTopBorder,
             unsigned int const newBottomBorder,
             xel          const backgroundColor,
             FILE *       const ofP) {

    /* In order to do cropping, padding or both at the same time, we have
       a rather complicated row buffer:

       xelrow[] is both the input and the output buffer.  So it contains
       the foreground pixels, the original border pixels, and the new
       border pixels.

       The foreground pixels are in the center of the
       buffer, starting at Column 'foregroundLeft' and going to
       'foregroundRight'.

       There is space to the left of that for the larger of the input
       left border and the output left border.

       Similarly, there is space to the right of the foreground pixels
       for the larger of the input right border and the output right
       border.

       We have to read an entire row, including the pixels we'll be
       leaving out of the output, so we pick a starting location in
       the buffer that lines up the first foreground pixel at
       'foregroundLeft'.

       When we output the row, we pick a starting location in the 
       buffer that includes the proper number of left border pixels
       before 'foregroundLeft'.

       That's for the middle rows.  For the top and bottom, we just use
       the left portion of xelrow[], starting at 0.
    */

    unsigned int const foregroundCols =
        cols - oldLeftBorder - oldRightBorder;
    unsigned int const outputCols     = 
        foregroundCols + newLeftBorder + newRightBorder;
    unsigned int const foregroundRows =
        rows - oldTopBorder - oldBottomBorder;
    unsigned int const outputRows     =
        foregroundRows + newTopBorder + newBottomBorder;

    unsigned int const foregroundLeft  = MAX(oldLeftBorder, newLeftBorder);
        /* Index into xelrow[] of leftmost pixel of foreground */
    unsigned int const foregroundRight = foregroundLeft + foregroundCols;
        /* Index into xelrow[] just past rightmost pixel of foreground */

    unsigned int const allocCols =
        foregroundRight + MAX(oldRightBorder, newRightBorder);

    xel *xelrow;
    unsigned int i;

    assert(outputCols == newLeftBorder + foregroundCols + newRightBorder);
    assert(outputRows == newTopBorder + foregroundRows + newBottomBorder);
    
    pnm_writepnminit(ofP, outputCols, outputRows, maxval, format, 0);

    xelrow = pnm_allocrow(allocCols);

    /* Read off existing top border */
    for (i = 0; i < oldTopBorder; ++i)
        pnm_readpnmrow(ifP, xelrow, cols, maxval, format);


    /* Output new top border */
    fillRow(xelrow, outputCols, backgroundColor);
    for (i = 0; i < newTopBorder; ++i)
        pnm_writepnmrow(ofP, xelrow, outputCols, maxval, format, 0);


    /* Read and output foreground rows */
    for (i = 0; i < foregroundRows; ++i) {
        /* Set left border pixels */
        fillRow(&xelrow[foregroundLeft - newLeftBorder], newLeftBorder,
                backgroundColor);

        /* Read foreground pixels */
        pnm_readpnmrow(ifP, &(xelrow[foregroundLeft - oldLeftBorder]), cols,
                       maxval, format);

        /* Set right border pixels */
        fillRow(&xelrow[foregroundRight], newRightBorder, backgroundColor);
        
        pnm_writepnmrow(ofP,
                        &(xelrow[foregroundLeft - newLeftBorder]), outputCols,
                        maxval, format, 0);
    }

    /* Read off existing bottom border */
    for (i = 0; i < oldBottomBorder; ++i)
        pnm_readpnmrow(ifP, xelrow, cols, maxval, format);

    /* Output new bottom border */
    fillRow(xelrow, outputCols, backgroundColor);
    for (i = 0; i < newBottomBorder; ++i)
        pnm_writepnmrow(ofP, xelrow, outputCols, maxval, format, 0);

    pnm_freerow(xelrow);
}



static void
determineNewBorders(struct cmdlineInfo const cmdline,
                    unsigned int       const leftBorderSize,
                    unsigned int       const rightBorderSize,
                    unsigned int       const topBorderSize,
                    unsigned int       const bottomBorderSize,
                    unsigned int *     const newLeftSizeP,
                    unsigned int *     const newRightSizeP,
                    unsigned int *     const newTopSizeP,
                    unsigned int *     const newBottomSizeP) {

    *newLeftSizeP   = cmdline.left   ? cmdline.margin : leftBorderSize   ;
    *newRightSizeP  = cmdline.right  ? cmdline.margin : rightBorderSize  ;
    *newTopSizeP    = cmdline.top    ? cmdline.margin : topBorderSize    ;
    *newBottomSizeP = cmdline.bottom ? cmdline.margin : bottomBorderSize ;
}
        


int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;   
        /* The program's regular input file.  Could be a seekable copy of it
           in a temporary file.
        */

    xelval maxval;
    int format;
    int rows, cols;   /* dimensions of input image */
    bool hasBorders;
    unsigned int oldLeftBorder, oldRightBorder, oldTopBorder, oldBottomBorder;
        /* The sizes of the borders in the input image */
    unsigned int newLeftBorder, newRightBorder, newTopBorder, newBottomBorder;
        /* The sizes of the borders in the output image */
    xel background;
    pm_filepos rasterpos;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr_seekable(cmdline.inputFilespec);

    pnm_readpnminit(ifP, &cols, &rows, &maxval, &format);

    pm_tell2(ifP, &rasterpos, sizeof(rasterpos));

    background = computeBackground(ifP, cols, rows, maxval, format,
                                   cmdline.background, cmdline.verbose);

    if (cmdline.borderfile) {
        findBordersInFile(cmdline.borderfile,
                          background, cmdline.verbose, &hasBorders,
                          &oldLeftBorder, &oldRightBorder,
                          &oldTopBorder,  &oldBottomBorder);
    } else {
        pm_seek2(ifP, &rasterpos, sizeof(rasterpos));

        findBordersInImage(ifP, cols, rows, maxval, format, 
                           background, cmdline.verbose, &hasBorders,
                           &oldLeftBorder, &oldRightBorder,
                           &oldTopBorder,  &oldBottomBorder);
    }
    if (!hasBorders)
        pm_error("The image is entirely background; "
                 "there is nothing to crop.");

    determineNewBorders(cmdline, 
                        oldLeftBorder, oldRightBorder,
                        oldTopBorder,  oldBottomBorder,
                        &newLeftBorder, &newRightBorder,
                        &newTopBorder,  &newBottomBorder);

    if (cmdline.verbose) 
        reportCroppingParameters(oldLeftBorder, oldRightBorder,
                                 oldTopBorder,  oldBottomBorder,
                                 newLeftBorder, newRightBorder,
                                 newTopBorder,  newBottomBorder);

    pm_seek2(ifP, &rasterpos, sizeof(rasterpos));

    writeCropped(ifP, cols, rows, maxval, format,
                 oldLeftBorder, oldRightBorder,
                 oldTopBorder,  oldBottomBorder,
                 newLeftBorder, newRightBorder,
                 newTopBorder,  newBottomBorder,
                 background, stdout);

    pm_close(stdout);
    pm_close(ifP);
    
    return 0;
}
