/*=============================================================================
                                    xvminitoppm
===============================================================================
   Convert XV mini thumbnail image to PPM.

   This replaces the program of the same name by Ingo Wilken
   (Ingo.Wilken@informatik.uni-oldenburg.de), 1993.

   Written by Bryan Henderson in April 2006 and contributed to the public
   domain.
=============================================================================*/

#include <assert.h>
#include <string.h>

#include "pm_c_util.h"
#include "nstring.h"
#include "ppm.h"

#define BUFSIZE 256


typedef struct xvPalette {
    unsigned int red[256];
    unsigned int grn[256];
    unsigned int blu[256];
} xvPalette;


struct cmdlineInfo {
    const char * inputFileName;
};



static void
parseCommandLine(int const argc,
                 char *    argv[],
                 struct cmdlineInfo * const cmdlineP) {

    if (argc-1 < 1)
        cmdlineP->inputFileName = "-";
    else {
        cmdlineP->inputFileName = argv[1];

        if (argc-1 > 1)
            pm_error("Too many arguments: %u.  Only argument is optional "
                     "input file name.", argc-1);
    }
}



static void
getLine(FILE * const ifP,
        char * const buf,
        size_t const size) {

    char * rc;

    rc = fgets(buf, size, ifP);
    if (rc == NULL) {
        if (ferror(ifP))
            pm_perror("read error");
        else
            pm_error("unexpected EOF");
    }
}



static void
makeXvPalette(xvPalette * const xvPaletteP) {

    unsigned int paletteIndex;
    unsigned int r;

    paletteIndex = 0;

    for (r = 0; r < 8; ++r) {
        unsigned int g;
        for (g = 0; g < 8; ++g) {
            unsigned int b;
            for (b = 0; b < 4; ++b) {
                xvPaletteP->red[paletteIndex] = (r*255)/7;
                xvPaletteP->grn[paletteIndex] = (g*255)/7;
                xvPaletteP->blu[paletteIndex] = (b*255)/3;
                ++paletteIndex;
            }
        }
    }

}



static void
readXvHeader(FILE *         const ifP,
             unsigned int * const colsP,
             unsigned int * const rowsP,
             unsigned int * const maxvalP) {
           
    char buf[256];
    unsigned int cols, rows, maxval;
    int rc;
    bool endOfComments;
    
    getLine(ifP, buf, sizeof(buf));

    if (!STRNEQ(buf, "P7 332", 6))
        pm_error("Input is not a XV thumbnail picture.  It does not "
                 "begin with the characters 'P7 332'.");

    endOfComments = FALSE;
    while (!endOfComments) {
        getLine(ifP, buf, sizeof(buf));
        if (STRNEQ(buf, "#END_OF_COMMENTS", 16))
            endOfComments = TRUE;
        else if (STRNEQ(buf, "#BUILTIN", 8))
            pm_error("This program does not know how to "
                     "convert builtin XV thumbnail pictures");
    }
    getLine(ifP, buf, sizeof(buf));
    rc = sscanf(buf, "%u %u %u", &cols, &rows, &maxval);
    if (rc != 3)
        pm_error("error parsing dimension info '%s'.  "
                 "It does not consist of 3 decimal numbers.", buf);
    if (maxval != 255)
        pm_error("bogus XV thumbnail maxval %u.  Should be 255", maxval);
    *colsP = cols;
    *rowsP = rows;
    *maxvalP = maxval;
}



static void
writePpm(FILE *             const ifP,
         const xvPalette *  const xvPaletteP,
         unsigned int       const cols,
         unsigned int       const rows,
         pixval             const maxval,
         FILE *             const ofP) {
/*----------------------------------------------------------------------------
   Write out the PPM image, from the XV-mini input file ifP, which is
   positioned to the raster.

   The raster contains indices into the palette *xvPaletteP.
-----------------------------------------------------------------------------*/
    pixel * pixrow;
    unsigned int row;

    pixrow = ppm_allocrow(cols);

    ppm_writeppminit(ofP, cols, rows, maxval, 0);

    for (row = 0; row < rows; ++row) {
        unsigned int col;
        for (col = 0; col < cols; ++col) {
            int byte;
            byte = fgetc(ifP);
            if (byte == EOF)
                pm_error("unexpected EOF");
            else {
                unsigned int const paletteIndex = byte;
                assert(byte >= 0);
                
                PPM_ASSIGN(pixrow[col],
                           xvPaletteP->red[paletteIndex],
                           xvPaletteP->grn[paletteIndex],
                           xvPaletteP->blu[paletteIndex]);
            }
        }
        ppm_writeppmrow(ofP, pixrow, cols, maxval, 0);
    }

    ppm_freerow(pixrow);
}



int 
main(int    argc,
     char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    unsigned int cols, rows;
    pixval maxval;
    xvPalette xvPalette;
 
    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFileName);

    makeXvPalette(&xvPalette);

    readXvHeader(ifP, &cols, &rows, &maxval);

    writePpm(ifP, &xvPalette, cols, rows, maxval, stdout);

    pm_close(ifP);

    return 0;
}



