/* pnmedge.c - edge-detection
**
** Copyright (C) 1989 by Jef Poskanzer.
**   modified for pnm by Peter Kirchgessner, 1995.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <math.h>

#include "pm_c_util.h"
#include "pam.h"



static void
writeBlackRow(struct pam * const pamP) {

    tuple * const tuplerow = pnm_allocpamrow(pamP);

    unsigned int col;
    for (col = 0; col < pamP->width; ++col) {
        unsigned int plane;
        for (plane = 0; plane < pamP->depth; ++plane)
            tuplerow[col][plane] = 0;
    }
    pnm_writepamrow(pamP, tuplerow);
} 



static void
rotateRows(tuple ** const row0P,
           tuple ** const row1P,
           tuple ** const row2P) {
    /* Rotate rows. */
    tuple * const formerRow0 = *row0P;
    *row0P = *row1P;
    *row1P = *row2P;
    *row2P = formerRow0;
}



static long
horizGradient(tuple *      const tuplerow,
              unsigned int const col,
              unsigned int const plane) {

    return (long)tuplerow[col+1][plane] - (long)tuplerow[col-1][plane];
}



static long
horizAvg(tuple *      const tuplerow,
         unsigned int const col,
         unsigned int const plane) {

    return
        1 * (long)tuplerow[col-1][plane] +
        2 * (long)tuplerow[col  ][plane] +
        1 * (long)tuplerow[col+1][plane];

}



static void
computeOneRow(struct pam * const inpamP,
              struct pam * const outpamP,
              tuple *      const row0,
              tuple *      const row1,
              tuple *      const row2,
              tuple *      const orow) {
/*----------------------------------------------------------------------------
   Compute an output row from 3 input rows.

   The input rows must have the same maxval as the output row.
-----------------------------------------------------------------------------*/
    unsigned int plane;

    for (plane = 0; plane < inpamP->depth; ++plane) {
        unsigned int col;

        /* Left column is black */
        orow[0][plane] = 0;

        for (col = 1; col < inpamP->width - 1; ++col) {
            double const grad1 = 
                1 * horizGradient(row0, col, plane) +
                2 * horizGradient(row1, col, plane) +
                1 * horizGradient(row2, col, plane);

            double const grad2 = 
                horizAvg(row2, col, plane) - horizAvg(row0, col, plane);

            double const gradient = sqrt(SQR(grad1) + SQR(grad2));

            /* apply arbitrary scaling factor and maxval clipping */
            orow[col][plane] = MIN(outpamP->maxval, (long)(gradient / 1.8));

            /* Right column is black */
            orow[inpamP->width - 1][plane] = 0;
        }
    }
}



static void
writeMiddleRows(struct pam * const inpamP,
                struct pam * const outpamP) {

    tuple *row0, *row1, *row2;
    tuple *orow, *irow;
    unsigned int row;

    irow = pnm_allocpamrow(inpamP);
    orow = pnm_allocpamrow(outpamP);
    row0 = pnm_allocpamrow(outpamP);
    row1 = pnm_allocpamrow(outpamP);
    row2 = pnm_allocpamrow(outpamP);

    /* Read in the first two rows. */
    pnm_readpamrow(inpamP, irow);
    pnm_scaletuplerow(inpamP, row0, irow, outpamP->maxval);
    pnm_readpamrow(inpamP, irow);
    pnm_scaletuplerow(inpamP, row1, irow, outpamP->maxval);

    pm_message("row1[0][0]=%lu", row1[0][0]);

    for (row = 1; row < inpamP->height - 1; ++row) {
        /* Read in the next row and write out the current row.  */

        pnm_readpamrow(inpamP, irow);
        pnm_scaletuplerow(inpamP, row2, irow, outpamP->maxval);

        computeOneRow(inpamP, outpamP, row0, row1, row2, orow);

        pnm_writepamrow(outpamP, orow);

        rotateRows(&row0, &row1, &row2);
    }
    pnm_freepamrow(orow);
    pnm_freepamrow(row2);
    pnm_freepamrow(row1);
    pnm_freepamrow(row0);
}



int
main(int argc, char *argv[]) {
    FILE *ifP;
    struct pam inpam, outpam;

    pnm_init( &argc, argv );

    if (argc-1 == 1) 
        ifP = pm_openr(argv[1]);
    else if (argc-1 == 0)
        ifP = stdin;
    else
        pm_error("Too many arguments.  Program takes at most 1 argument: "
                 "input file name");

    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));
    if (inpam.width < 3)
        pm_error("Image is %u columns wide.  It must be at least 3.",
                 inpam.width);
    if (inpam.height < 3)
        pm_error("Image is %u rows high.  It must be at least 3.",
                 inpam.height);

    outpam = inpam;
    outpam.file = stdout;
    if (PAM_FORMAT_TYPE(inpam.format) == PBM_TYPE) {
        outpam.format = PGM_FORMAT;
        outpam.maxval = 255;
    }

    pnm_writepaminit(&outpam);

    /* First row is black: */
    writeBlackRow(&outpam      );

    writeMiddleRows(&inpam, &outpam);

    pm_close(ifP);

    /* Last row is black: */
    writeBlackRow(&outpam);

    pm_close(stdout);

    return 0;
}
