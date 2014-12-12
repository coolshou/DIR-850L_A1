/* pbmtoeps.c - read a PBM image and produce Epson graphics
**
** Copyright (C) 1990 by John Tiller (tiller@galois.msfc.nasa.gov)
**			 and Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#define _BSD_SOURCE    /* Make sure strcasecmp() is in string.h */

#include <stdio.h>
#include <string.h>

#include "shhopt.h"
#include "mallocvar.h"

#include "pbm.h"


static char const esc = 033;

enum epsonProtocol {ESCP9, ESCP};

enum adjacence {ADJACENT_ANY, ADJACENT_YES, ADJACENT_NO};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* '-' if stdin */
    unsigned int dpi;  /* zero means "any" */
    enum adjacence adjacence;
    enum epsonProtocol protocol;
};



static void
parseCommandLine(int                 argc, 
                 char **             argv,
                 struct cmdlineInfo *cmdlineP ) {
/*----------------------------------------------------------------------------
   Parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    char * protocol;
    unsigned int adjacentSpec, nonadjacentSpec;
    unsigned int dpiSpec, protocolSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "protocol",   OPT_UINT,   &protocol,
            &protocolSpec,                    0);
    OPTENT3(0, "dpi",        OPT_UINT,   &cmdlineP->dpi,
            &dpiSpec,                    0);
    OPTENT3(0, "adjacent",   OPT_FLAG,   NULL,
            &adjacentSpec,                    0);
    OPTENT3(0, "nonadjacent",   OPT_FLAG,   NULL,
            &nonadjacentSpec,                    0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (!dpiSpec)
        cmdlineP->dpi = 0;
    else {
        if (cmdlineP->dpi == 0)
            pm_error("-dpi must be positive");
    }

    if (!protocolSpec)
        cmdlineP->protocol = ESCP9;
    else {
        if (strcasecmp(protocol, "escp9") == 0)
            cmdlineP->protocol = ESCP9;
        else if (strcasecmp(protocol, "escp") == 0)
            cmdlineP->protocol = ESCP;
        else if (strcasecmp(protocol, "escp2") == 0)
            pm_error("This program cannot do ESC/P2.  Try Pbmtoescp2.");
        else
            pm_error("Unrecognized value '%s' for -protocol.  "
                     "Only recognized values are 'escp9' and 'escp'",
                     protocol);
    }
    
    if (adjacentSpec && nonadjacentSpec)
        pm_error("You can't specify both -adjacent and -nonadjacent");
    else if (adjacentSpec)
        cmdlineP->adjacence = ADJACENT_YES;
    else if (nonadjacentSpec)
        cmdlineP->adjacence = ADJACENT_NO;
    else
        cmdlineP->adjacence = ADJACENT_ANY;

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else {
        cmdlineP->inputFilespec = argv[1];
        if (argc-1 > 1)
            pm_error("Too many arguments (%d).  The only non-option argument "
                     "is the file name", argc-1);
    }
}



static unsigned int
lineWidth(const bit ** const stripeBits,
          unsigned int const cols,
          unsigned int const stripeRows) {
/*----------------------------------------------------------------------------
   Return the column number just past the rightmost column of the stripe
   stripeBits[] that contains at least some black.

   The stripe is 'cols' wide by 'stripeRows' high.
-----------------------------------------------------------------------------*/
    unsigned int col;
    unsigned int endSoFar;
    
    endSoFar = 0;

    for (col = 0; col < cols; ++ col) {
        unsigned int stripeRow;  /* row number within stripe */

        for (stripeRow = 0; stripeRow < stripeRows; ++stripeRow) {
            if (stripeBits[stripeRow][col] == PBM_BLACK)
                endSoFar = col+1;
        }
    }
    return endSoFar;
}



static void
printStripe(const bit ** const stripeBits,
            unsigned int const cols,
            unsigned int const stripeRows,
            char         const m) {
/*----------------------------------------------------------------------------
   Print one stripe (a group of rows printed with one pass of the print
   head.  The stripe is cols columns wide by stripeRows high.
   stripeBits[row][col] is the pixel value for Row row, Column col within
   the stripe.

   'm' is the "m" parameter for the Select Bit Image command.  It controls
   such things as the horizontal density.
-----------------------------------------------------------------------------*/
    unsigned int col;

    /* Print header of Select Bit Image command */
    printf("%c%c%c%c%c", esc, '*', m, cols % 256, cols / 256);
    
    /* Print the data part of the Select Bit Image command */
    for (col = 0; col < cols; ++col) {
        unsigned int stripeRow;
        int val;
        
        val = 0;
        for (stripeRow = 0; stripeRow < stripeRows; ++stripeRow) 
            if (stripeBits[stripeRow][col] == PBM_BLACK)
                val |= (1 << (8-1-stripeRow));
        putchar(val);
    }
}



static void
computeM(enum epsonProtocol const protocol,
         unsigned int       const dpi,
         enum adjacence     const adjacence,
         char *             const mP) {
/*----------------------------------------------------------------------------
   Compute the "m" parameter for the Select Bit Image command.
-----------------------------------------------------------------------------*/
    switch (dpi) {
    case 0:
        /* Special value meaning "any dpi you feel is appropriate" */
        if (adjacence == ADJACENT_NO)
            *mP = 2;
        else {
            switch (protocol) {
            case ESCP9: *mP = 5; break;
            case ESCP:  *mP = 6; break;
            }
        }
        break;
    case 60: 
        if (adjacence == ADJACENT_NO)
            pm_error("You can't print at %u dpi "
                     "with adjacent dot printing", dpi);
        *mP = 0;
        break;
    case 120:
        *mP = adjacence == ADJACENT_NO ? 2 : 1;
        break;
    case 240:
        if (adjacence == ADJACENT_YES)
            pm_error("You can't print at %u dpi "
                     "without adjacent dot printing", dpi);
        *mP = 3;
        break;
    case 80:
        if (adjacence == ADJACENT_NO)
            pm_error("You can't print at %u dpi "
                     "with adjacent dot printing", dpi);
        *mP = 4;
        break;
    case 72:
        if (protocol != ESCP9)
            pm_error("%u dpi is possible only with the ESC/P 9-pin protocol", 
                     dpi);
        if (adjacence == ADJACENT_NO)
            pm_error("You can't print at %u dpi "
                     "with adjacent dot printing", dpi);
        *mP = 5;
        break;
    case 90:
        if (adjacence == ADJACENT_NO)
            pm_error("You can't print at %u dpi "
                     "with adjacent dot printing", dpi);
        *mP = 6;
        break;
    case 144:
        if (protocol != ESCP9)
            pm_error("%u dpi is possible only with the ESC/P 9-pin protocol", 
                     dpi);
        if (adjacence == ADJACENT_NO)
            pm_error("You can't print at %u dpi "
                     "with adjacent dot printing", dpi);
        *mP = 7;
        break;
    default:
        pm_error("Invalid DPI value: %u.  This program knows only "
                 "60, 72, 80, 90, 120, 144, and 240.", dpi);
    }
}



static void
convertToEpson(const bit **       const bits,
               int                const cols,
               int                const rows,
               enum epsonProtocol const protocol,
               unsigned int       const dpi,
               enum adjacence     const adjacence) {
    
    unsigned int const rowsPerStripe = 8;
    unsigned int const stripes = (rows + rowsPerStripe-1) / rowsPerStripe;

    unsigned int stripe;
    char m;
    
    computeM(protocol, dpi, adjacence, &m);

    /* Change line spacing to 8/72 inches. */
    printf("%c%c%c", esc, 'A', 8);

    /* Write out the rows, one stripe at a time.  A stripe is 8 rows --
       the amount written in one pass of the print head.  The bottommost
       stripe can be fewer than 8 rows.
    */

    for (stripe = 0; stripe < stripes; ++stripe) {
        const bit ** const stripeBits = &bits[stripe*rowsPerStripe];
        unsigned int const stripeRows = 
            MIN(rowsPerStripe, rows - stripe * rowsPerStripe);
            /* Number of rows in this stripe (8 for all but bottom stripe) */
        
        unsigned int const endcol = lineWidth(stripeBits, cols, stripeRows);
            /* Column where right margin (contiguous white area at right
               end of stripe) begins.  Zero if entire stripe is white.
            */

        if (endcol > 0)
            printStripe(stripeBits, endcol, stripeRows, m);

        putchar('\n');
    }
    putchar('\f');

    /* Restore normal line spacing. */
    printf("%c%c", esc, '@');
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE* ifP;
    const bit** bits;
    int rows, cols;

    pbm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    bits = (const bit **)pbm_readpbm(ifP, &cols, &rows);

    pm_close(ifP);

    convertToEpson(bits, cols, rows, 
                   cmdline.protocol, cmdline.dpi, cmdline.adjacence);

    pbm_freearray(bits, rows);

    return 0;
}
