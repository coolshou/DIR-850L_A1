/* ppmto3d.c - convert a portable pixmap to a portable graymap
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

#include "ppm.h"
#include "lum.h"

static void
computeGrayscaleRow(const pixel * const inputRow,
                    gray *        const outputRow,
                    pixval        const maxval,
                    unsigned int  const cols) {

    if (maxval <= 255) {
        unsigned int col;
        /* Use fast approximation to 0.299 r + 0.587 g + 0.114 b. */
        for (col = 0; col < cols; ++col)
            outputRow[col] = ppm_fastlumin(inputRow[col]);
    } else {
        unsigned int col;
        /* Can't use fast approximation, so fall back on floats. */
        for (col = 0; col < cols; ++col)
            outputRow[col] = PPM_LUMIN(inputRow[col]) + 0.5;
    }
}



int
main (int argc, char *argv[]) {

    int offset; 
    int cols, rows, row;
    pixel* pixelrow;
    pixval maxval;

    FILE* Lifp;
    pixel* Lpixelrow;
    gray* Lgrayrow;
    int Lrows, Lcols, Lformat;
    pixval Lmaxval;
   
    FILE* Rifp;
    pixel* Rpixelrow;
    gray* Rgrayrow;
    int Rrows, Rcols, Rformat;
    pixval Rmaxval;
   
    ppm_init (&argc, argv);

    if (argc-1 > 3 || argc-1 < 2) 
        pm_error("Wrong number of arguments (%d).  Arguments are "
                 "leftppmfile rightppmfile [horizontal_offset]", argc-1);

    Lifp = pm_openr (argv[1]);
    Rifp = pm_openr (argv[2]);

    if (argc-1 >= 3) 
        offset = atoi (argv[3]);
    else
        offset = 30;

    ppm_readppminit (Lifp, &Lcols, &Lrows, &Lmaxval, &Lformat);
    ppm_readppminit (Rifp, &Rcols, &Rrows, &Rmaxval, &Rformat);
    
    if ((Lcols != Rcols) || (Lrows != Rrows) || 
        (Lmaxval != Rmaxval) || 
        (PPM_FORMAT_TYPE(Lformat) != PPM_FORMAT_TYPE(Rformat)))
        pm_error ("Pictures are not of same size and format");
    
    cols = Lcols;
    rows = Lrows;
    maxval = Lmaxval;
   
    ppm_writeppminit (stdout, cols, rows, maxval, 0);
    Lpixelrow = ppm_allocrow (cols);
    Lgrayrow = pgm_allocrow (cols);
    Rpixelrow = ppm_allocrow (cols);
    Rgrayrow = pgm_allocrow (cols);
    pixelrow = ppm_allocrow (cols);

    for (row = 0; row < rows; ++row) {
        ppm_readppmrow(Lifp, Lpixelrow, cols, maxval, Lformat);
        ppm_readppmrow(Rifp, Rpixelrow, cols, maxval, Rformat);

        computeGrayscaleRow(Lpixelrow, Lgrayrow, maxval, cols);
        computeGrayscaleRow(Rpixelrow, Rgrayrow, maxval, cols);
        {
            int col;
            gray* LgP;
            gray* RgP;
            pixel* pP;
            for (col = 0, pP = pixelrow, LgP = Lgrayrow, RgP = Rgrayrow;
                 col < cols + offset;
                 ++col) {
            
                if (col < offset/2)
                    ++LgP;
                else if (col >= offset/2 && col < offset) {
                    const pixval Blue = (pixval) (float) *LgP;
                    const pixval Red = (pixval) 0;
                    PPM_ASSIGN (*pP, Red, Blue, Blue);
                    ++LgP;
                    ++pP;
                } else if (col >= offset && col < cols) {
                    const pixval Red = (pixval) (float) *RgP;
                    const pixval Blue = (pixval) (float) *LgP;
                    PPM_ASSIGN (*pP, Red, Blue, Blue);
                    ++LgP;
                    ++RgP;
                    ++pP;
                } else if (col >= cols && col < cols + offset/2) {
                    const pixval Blue = (pixval) 0;
                    const pixval Red = (pixval) (float) *RgP;
                    PPM_ASSIGN (*pP, Red, Blue, Blue);
                    ++RgP;
                    ++pP;
                } else
                    ++RgP;
            }
        }    
        ppm_writeppmrow(stdout, pixelrow, cols, maxval, 0);
    }

    pm_close(Lifp);
    pm_close(Rifp);
    pm_close(stdout);

    return 0;
}
