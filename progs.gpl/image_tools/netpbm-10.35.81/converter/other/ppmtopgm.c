/* ppmtopgm.c - convert a portable pixmap to a portable graymap
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

#include "pm_c_util.h"
#include "ppm.h"
#include "pgm.h"
#include "lum.h"

static void
convertRaster(FILE *       const ifP,
              unsigned int const cols,
              unsigned int const rows,
              pixval       const maxval,
              int          const format, 
              pixel *      const inputRow,
              gray *       const outputRow, 
              FILE *       const ofP) {

    unsigned int row;

    for (row = 0; row < rows; ++row) {
        ppm_readppmrow( ifP, inputRow, cols, maxval, format );
        if (maxval <= 255) {
            /* Use fast approximation to 0.299 r + 0.587 g + 0.114 b */
            unsigned int col;
            for (col = 0; col < cols; ++col)
                outputRow[col] = (gray) ppm_fastlumin(inputRow[col]);
        } else {
            /* Can't use fast approximation, so fall back on floats. */
            int col;
            for (col = 0; col < cols; ++col) 
                outputRow[col] = (gray) (PPM_LUMIN(inputRow[col]) + 0.5);
        }
        pgm_writepgmrow(ofP, outputRow, cols, maxval, 0);
    }
}



int
main(int argc, char *argv[]) {

    FILE* ifP;
    const char * inputFilespec;
    int eof;
    
    ppm_init( &argc, argv );

    if (argc-1 > 1)
        pm_error("The only argument is the (optional) input filename");

    if (argc == 2)
        inputFilespec = argv[1];
    else
        inputFilespec = "-";
    
    ifP = pm_openr(inputFilespec);

    eof = FALSE;  /* initial assumption */

    while (!eof) {
        ppm_nextimage(ifP, &eof);
        if (!eof) {
            int rows, cols, format;
            pixval maxval;
            pixel* inputRow;
            gray* outputRow;

            ppm_readppminit(ifP, &cols, &rows, &maxval, &format);
            pgm_writepgminit(stdout, cols, rows, maxval, 0);

            inputRow = ppm_allocrow(cols);
            outputRow = pgm_allocrow(cols);

            convertRaster(ifP, cols, rows, maxval, format, 
                          inputRow, outputRow, stdout);

            ppm_freerow(inputRow);
            pgm_freerow(outputRow);
        }
    }
    pm_close(ifP);
    pm_close(stdout);

    return 0;
}
