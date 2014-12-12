/* psidtopgm.c - convert PostScript "image" data into a portable graymap
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

#include "pgm.h"


static int
gethexit(FILE * const ifp) {

    for ( ; ; ) {
        int const i = getc(ifp);
        if (i == EOF)
            pm_error("EOF / read error");
        else {
            char const c = (char) i;
            if (c >= '0' && c <= '9')
                return c - '0';
            else if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            else if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            /* Else ignore - whitespace. */
        }
    }
}



int
main(int     argc,
     char ** argv) {

    FILE * ifP;
    gray * grayrow;
    int argn, row;
    gray maxval;
    int rows, cols, bitspersample;

    pgm_init(&argc, argv);

    argn = 1;
    
    if (argn + 3 > argc)
        pm_error("Too few arguments");

    cols = atoi(argv[argn++]);
    rows = atoi(argv[argn++]);
    bitspersample = atoi(argv[argn++]);
    if (cols <= 0)
        pm_error("Columns must be positive.  You specified %d",
                 cols);
    if (rows <= 0)
        pm_error("Rows must be positive.  You specified %d",
                 rows);
    if (bitspersample <= 0)
        pm_error("Bits per sample must be positive.  You specified %d",
                 bitspersample);

    if (argn < argc)
        ifP = pm_openr(argv[argn++]);
    else
        ifP = stdin;

    if (argn != argc)
        pm_error("Too many arguments");

    maxval = pm_bitstomaxval(bitspersample);
    if (maxval > PGM_OVERALLMAXVAL)
        pm_error("bits/sample (%d) is too large.", bitspersample);

    pgm_writepgminit(stdout, cols, rows, maxval, 0);
    grayrow = pgm_allocrow((cols + 7) / 8 * 8);
    for (row = 0; row < rows; ++row) {
        unsigned int col;
        for (col = 0; col < cols; ) {
            int val;
            val = gethexit(ifP) << 4;
            val += gethexit(ifP);
            switch (bitspersample) {
            case 1:
                grayrow[col++] = val >> 7;
                grayrow[col++] = ( val >> 6 ) & 0x1;
                grayrow[col++] = ( val >> 5 ) & 0x1;
                grayrow[col++] = ( val >> 4 ) & 0x1;
                grayrow[col++] = ( val >> 3 ) & 0x1;
                grayrow[col++] = ( val >> 2 ) & 0x1;
                grayrow[col++] = ( val >> 1 ) & 0x1;
                grayrow[col++] = val & 0x1;
                break;

            case 2:
                grayrow[col++] = val >> 6;
                grayrow[col++] = ( val >> 4 ) & 0x3;
                grayrow[col++] = ( val >> 2 ) & 0x3;
                grayrow[col++] = val & 0x3;
                break;

            case 4:
                grayrow[col++] = val >> 4;
                grayrow[col++] = val & 0xf;
                break;

            case 8:
                grayrow[col++] = val;
                break;

            default:
                pm_error("This program does not know how to interpret a"
                         "%d bitspersample image", bitspersample );
            }
        }
        pgm_writepgmrow(stdout, grayrow, cols, (gray) maxval, 0);
    }
    pgm_freerow(grayrow);
    pm_close(ifP);
    pm_close(stdout);

    return 0;
}
