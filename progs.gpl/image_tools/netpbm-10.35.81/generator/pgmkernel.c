/* pgmkernel.c - generate a portable graymap convolution kernel
**
** Creates a Portable Graymap file containing a convolution filter
** with max value = 255 and minimum value > 127 that can be used as a 
** smoothing kernel for pnmconvol.
**
** Copyright (C) 1992 by Alberto Accomazzi, Smithsonian Astrophysical
** Observatory.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <math.h>
#include "pgm.h"
#include "mallocvar.h"

int
main ( argc, argv )
    int argc;
    char *argv[];
{
    register    int i, j;
    int     argn = 1, ixsize, iysize, maxval = 255;
    double  fxsize = 0.0, fysize = 0.0, w = 6.0, kxcenter, kycenter, 
        tmax = 0, *fkernel;
    const char  *usage = "[-weight f] width [height]";

    pgm_init( &argc, argv );

    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
        if ( pm_keymatch( argv[argn], "-weight", 2 )) {
            if (++argn >= argc)
                pm_usage( usage );
            else if (sscanf(argv[argn], "%lf", &w) != 1)
                pm_usage( usage );
        }
        else
            pm_usage( usage );
        argn++;
    }

    if (argn == argc)
        pm_usage( usage );
    
    if (sscanf(argv[argn], "%lf", &fxsize) != 1) 
        pm_error( "error reading input kernel x size, (%s)\n", argv[argn]);

    ++argn;
    if (argn == argc - 1) {
        if (sscanf(argv[argn], "%lf", &fysize) != 1)
            pm_error( "error reading input kernel y size, (%s)\n", argv[argn]);
    }
    else if (argn == argc)
        fysize = fxsize;
    else
        pm_usage( usage );

    if (fxsize <= 1 || fysize <= 1)
        pm_usage( usage );

    kxcenter = (fxsize - 1) / 2.0;
    kycenter = (fysize - 1) / 2.0;
    ixsize = fxsize + 0.999;
    iysize = fysize + 0.999;
    MALLOCARRAY(fkernel, ixsize * iysize);
    for (i = 0; i < iysize; i++) 
        for (j = 0; j < ixsize; j++) {
            fkernel[i*ixsize+j] = 1.0 / (1.0 + w * sqrt((double)
                                                        (i-kycenter)*(i-kycenter)+
                                                        (j-kxcenter)*(j-kxcenter)));
            if (tmax < fkernel[i*ixsize+j])
                tmax = fkernel[i*ixsize+j];
        }

    /* output PGM header + data (ASCII format only) */
    printf("P2\n%d %d\n%d\n", ixsize, iysize, maxval);
    
    for (i = 0; i < iysize; i++, printf("\n"))
        for (j = 0; j < ixsize; j++)
            printf(" %3d", (int)(maxval * (fkernel[i*ixsize+j] / 
                                           (2*tmax) + 0.5)));
    
    exit(0);
}

