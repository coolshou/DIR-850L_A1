/*----------------------------------------------------------------------------
                               pgmtopgm
------------------------------------------------------------------------------
  Part of the Netpbm package.

  Copy PGM image from Standard Input to Standard Output


  By Bryan Henderson, San Jose CA 2002.09.07

  Contributed to the public domain by its author 2002.09.07
-----------------------------------------------------------------------------*/

#include "pgm.h"

int
main(int argc, char *argv[]) {
    int format;
    int rows, cols;
    gray maxval;
    int row;
    gray* grayrow;
    
    pgm_init(&argc, argv);
    
    if (argc-1 != 0)
        pm_error("Program takes no arguments.  Input is from Standard Input");

    pgm_readpgminit(stdin, &cols, &rows, &maxval, &format);

    pgm_writepgminit(stdout, cols, rows, maxval, 0);

    grayrow = pgm_allocrow(cols);

    for (row = 0; row < rows; row++) {
        pgm_readpgmrow(stdin, grayrow, cols, maxval, format);
        pgm_writepgmrow(stdout, grayrow, cols, maxval, 0);
    }
    pgm_freerow(grayrow);

    pm_close(stdin);

    return 0;
}
