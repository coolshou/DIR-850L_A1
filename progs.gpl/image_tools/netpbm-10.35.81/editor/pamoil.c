/* pgmoil.c - read a portable pixmap and turn into an oil painting
**
** Copyright (C) 1990 by Wilson Bent (whb@hoh-2.att.com)
** Shamelessly butchered into a color version by Chris Sheppard
** 2001
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pam.h"
#include "mallocvar.h"

static void 
convertRow(struct pam const inpam, tuple ** const tuples,
           tuple * const tuplerow, int const row, int const smearFactor,
           int * const hist) {

    int sample;
    for (sample = 0; sample < inpam.depth; sample++) {
        int col;
        for (col = 0; col < inpam.width; ++col)  {
            int i;
            int drow;
            int modalval;
                /* The sample value that occurs most often in the neighborhood
                   of the pixel being examined
                */

            /* Compute hist[] - frequencies, in the neighborhood, of each 
               sample value
            */
            for (i = 0; i <= inpam.maxval; ++i) hist[i] = 0;

            for (drow = row - smearFactor; drow <= row + smearFactor; ++drow) {
                if (drow >= 0 && drow < inpam.height) {
                    int dcol;
                    for (dcol = col - smearFactor; 
                         dcol <= col + smearFactor; 
                         ++dcol) {
                        if ( dcol >= 0 && dcol < inpam.width )
                            ++hist[tuples[drow][dcol][sample]];
                    }
                }
            }
            {
                /* Compute modalval */
                int sampleval;
                int maxfreq;

                maxfreq = 0;
                modalval = 0;

                for (sampleval = 0; sampleval <= inpam.maxval; ++sampleval) {
                    if (hist[sampleval] > maxfreq) {
                        maxfreq = hist[sampleval];
                        modalval = sampleval;
                    }
                }
            }
            tuplerow[col][sample] = modalval;
        }
    }
}



int
main(int argc, char *argv[] ) {
    struct pam inpam, outpam;
    FILE* ifp;
    tuple ** tuples;
    tuple * tuplerow;
    int * hist;
        /* A buffer for the convertRow subroutine to use */
    int argn;
    int row;
    int smearFactor;
    const char* const usage = "[-n <n>] [ppmfile]";

    ppm_init( &argc, argv );

    argn = 1;
    smearFactor = 3;       /* DEFAULT VALUE */

    /* Check for options. */
    if ( argn < argc && argv[argn][0] == '-' ) {
        if ( argv[argn][1] == 'n' ) {
            ++argn;
            if ( argn == argc || sscanf(argv[argn], "%d", &smearFactor) != 1 )
                pm_usage( usage );
        } else
            pm_usage( usage );
        ++argn;
    }
    if ( argn < argc ) {
        ifp = pm_openr( argv[argn] );
        ++argn;
    } else
        ifp = stdin;

    if ( argn != argc )
        pm_usage( usage );

    tuples = pnm_readpam(ifp, &inpam, PAM_STRUCT_SIZE(tuple_type));
    pm_close(ifp);

    MALLOCARRAY(hist, inpam.maxval + 1);
    if (hist == NULL)
        pm_error("Unable to allocate memory for histogram.");

    outpam = inpam; outpam.file = stdout;

    pnm_writepaminit(&outpam);

    tuplerow = pnm_allocpamrow(&inpam);

    for (row = 0; row < inpam.height; ++row) {
        convertRow(inpam, tuples, tuplerow, row, smearFactor, hist);
        pnm_writepamrow(&outpam, tuplerow);
    }

    pnm_freepamrow(tuplerow);
    free(hist);
    pnm_freepamarray(tuples, &inpam);

    pm_close(stdout);
    exit(0);
}

