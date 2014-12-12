/*----------------------------------------------------------------------------
                               ppmtoppm
------------------------------------------------------------------------------
  Part of the Netpbm package.

  Copy PPM image from Standard Input to Standard Output


  By Bryan Henderson, San Jose CA 2002.09.07

  Contributed to the public domain by its author 2002.09.07
-----------------------------------------------------------------------------*/

#include "ppm.h"

int
main(int argc, char *argv[]) {
    int format;
    int rows, cols;
    pixval maxval;
    int row;
    pixel* pixelrow;
    
    ppm_init(&argc, argv);

    if (argc-1 != 0)
        pm_error("Program takes no arguments.  Input is from Standard Input");

    ppm_readppminit(stdin, &cols, &rows, &maxval, &format);

    ppm_writeppminit(stdout, cols, rows, maxval, 0);

    pixelrow = ppm_allocrow(cols);

    for (row = 0; row < rows; row++) {
        ppm_readppmrow(stdin, pixelrow, cols, maxval, format);
        ppm_writeppmrow(stdout, pixelrow, cols, maxval, 0);
    }
    ppm_freerow(pixelrow);

    pm_close(stdin);

    exit(0);
}
