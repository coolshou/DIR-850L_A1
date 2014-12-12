/* pbmtoepsi.c
**
**    by Doug Crabill, based heavily on pbmtoascii
**
**    Converts a pbm file to an encapsulated PostScript style bitmap.
**    Note that it does NOT covert the pbm file to PostScript, only to
**    a bitmap to be added to a piece of PostScript generated elsewhere.
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

#include "pbm.h"
#include "shhopt.h"

#if !defined(MAXINT)
#define MAXINT (0x7fffffff)
#endif

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input files */

    unsigned int dpiX;     /* horiz component of DPI option */
    unsigned int dpiY;     /* vert component of DPI option */
    unsigned int bbonly;

    unsigned int verbose;
};



static void
parse_dpi(char * const dpiOpt, 
          unsigned int * const dpiXP, unsigned int * const dpiYP) {

    char *dpistr2;
    unsigned int dpiX, dpiY;

    dpiX = strtol(dpiOpt, &dpistr2, 10);
    if (dpistr2 == dpiOpt) 
        pm_error("Invalid value for -dpi: '%s'.  Must be either number "
                 "or NxN ", dpiOpt);
    else {
        if (*dpistr2 == '\0') {
            *dpiXP = dpiX;
            *dpiYP = dpiX;
        } else if (*dpistr2 == 'x') {
            char * dpistr3;

            dpistr2++;  /* Move past 'x' */
            dpiY = strtol(dpistr2, &dpistr3, 10);        
            if (dpistr3 != dpistr2 && *dpistr3 == '\0') {
                *dpiXP = dpiX;
                *dpiYP = dpiY;
            } else {
                pm_error("Invalid value for -dpi: '%s'.  Must be either "
                         "number or NxN", dpiOpt);
            }
        }
    }
}


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

    char * dpiOpt;
    unsigned int dpiOptSpec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "bbonly",     OPT_FLAG,   NULL, &cmdlineP->bbonly,        0);
    OPTENT3(0, "verbose",    OPT_FLAG,   NULL, &cmdlineP->verbose,       0);
    OPTENT3(0, "dpi",        OPT_STRING, &dpiOpt,         &dpiOptSpec,   0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */
    

    if (dpiOptSpec)
        parse_dpi(dpiOpt, &cmdlineP->dpiX, &cmdlineP->dpiY);
    else
        cmdlineP->dpiX = cmdlineP->dpiY = 72;
    
    if ((argc-1) > 1)
        pm_error("Too many arguments (%d).  Only argument is input filespec",
                 argc-1);
    
    if (argc-1 == 0)
        cmdlineP->inputFilespec = "-";
    else
        cmdlineP->inputFilespec = argv[1];
}



static void
findPrincipalImage(bit ** const bits, 
                   int    const rows,
                   int    const cols,
                   int *  const topP,
                   int *  const bottomP,
                   int *  const leftP,
                   int *  const rightP) {

    int top, bottom, left, right;
    int row;

    /* Initial values */
    top = MAXINT;
    bottom = -MAXINT;
    left = MAXINT;
    right = -MAXINT;
 
    for (row = 0; row < rows; row++) {
        int col;
        for (col = 0; col < cols; col++) {
            if (bits[row][col] == PBM_BLACK) {
                if (row < top) 
                    top = row;
                if (row > bottom) 
                    bottom = row;
                if (col < left) 
                    left = col;
                if (col > right) 
                    right = col;
            }
        }
    }
    *topP = top;
    *bottomP = bottom;
    *leftP = left;
    *rightP = right;
}



static void
outputBoundingBox(int const top, int const bottom,
                  int const left, int const right,
                  int const rows,
                  unsigned int const dpiX, unsigned int const dpiY) {

    float const xScale = 72.0 / dpiX;
    float const yScale = 72.0 / dpiY;

    printf("%%%%BoundingBox: %d %d %d %d\n", 
           ROUND(left*xScale),  ROUND((rows - bottom)*yScale), 
           ROUND(right*xScale), ROUND((rows - top)*yScale));
}



static unsigned char
eightPixels(bit ** const bits,
            int    const row,
            int    const col,
            int    const cols) {
/*----------------------------------------------------------------------------
  Compute a byte that represents the 8 pixels starting at Column 'col' of
  row 'row' of the raster 'bits'.  The most significant bit of the result
  represents the leftmost pixel, with 1 meaning black.

  The row is 'cols' columns wide, so fill on the right with white if there
  are not eight pixels in the row starting with Column 'col'.
-----------------------------------------------------------------------------*/
    unsigned int bitPos;
    unsigned char octet;

    octet = 0;  /* initial value */

    for (bitPos = 0; bitPos < 8; ++bitPos) {
        octet <<= 1;
        if (col + bitPos < cols)
            if (bits[row][col + bitPos] == PBM_BLACK)
                octet += 1;
    }
    return(octet);
}



int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE *ifP;
    bit **bits;
    int rows, cols;
    int top, bottom, left, right;
        /* boundaries of principal part of image -- i.e. excluding white
           borders
        */

    pbm_init( &argc, argv );
    
    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);
    bits = pbm_readpbm( ifP, &cols, &rows );
    pm_close(ifP);

    findPrincipalImage(bits, rows, cols, &top, &bottom, &left, &right);

    printf("%%!PS-Adobe-2.0 EPSF-1.2\n");

    outputBoundingBox(top, bottom, left, right, rows, 
                      cmdline.dpiX, cmdline.dpiY);

    if (!cmdline.bbonly) {
        int row;
        printf("%%%%BeginPreview: %d %d 1 %d\n", 
               right - left + 1, bottom - top + 1, bottom - top + 1);

        for (row = top; row <= bottom; row++) {
            int col;

            printf("%% ");

            for (col = left; col <= right; col += 8) 
                printf("%02x", eightPixels(bits, row, col, cols));

            printf("\n");
        }
        printf("%%%%EndImage\n");
        printf("%%%%EndPreview\n");
    }
    exit(0);
}
