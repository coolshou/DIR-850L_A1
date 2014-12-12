
/*********************************************************************/
/* ppmflash -  brighten a picture up to total whiteout               */
/* Frank Neumann, August 1993                                        */
/* V1.4 16.11.1993                                                   */
/*                                                                   */
/* version history:                                                  */
/* V1.0 ~ 15.August 1993    first version                            */
/* V1.1 03.09.1993          uses ppm libs & header files             */
/* V1.2 03.09.1993          integer arithmetics instead of float     */
/*                          (gains about 50 % speed up)              */
/* V1.3 11.10.1993          reads only one line at a time - this     */
/*                          saves LOTS of memory on big picturs      */
/* V1.4 16.11.1993          Rewritten to be NetPBM.programming con-  */
/*                          forming                                  */
/*********************************************************************/

#include "ppm.h"

/* global variables */
#ifdef AMIGA
static char *version = "$VER: ppmflash 1.4 (16.11.93)";
#endif

/**************************/
/* start of main function */
/**************************/
int main(argc, argv)
    int argc;
    char *argv[];
{
    FILE* ifp;
    int argn, rows, cols, i, j, format;
    pixel *srcrow, *destrow;
    pixel *pP, *pP2;
    pixval maxval;
    double flashfactor;
    long longfactor;
    const char* const usage = "flashfactor [ppmfile]\n        flashfactor: 0.0 = original picture, 1.0 = total whiteout\n";

    /* parse in 'default' parameters */
    ppm_init( &argc, argv );

    argn = 1;

    /* parse in flash factor */
    if (argn == argc)
        pm_usage(usage);
    if (sscanf(argv[argn], "%lf", &flashfactor) != 1)
        pm_usage(usage);
    if (flashfactor < 0.0 || flashfactor > 1.0)
        pm_error("flash factor must be in the range from 0.0 to 1.0 ");
    ++argn;

    /* parse in filename (if present, stdin otherwise) */
    if (argn != argc)
    {
        ifp = pm_openr(argv[argn]);
        ++argn;
    }
    else
        ifp = stdin;

    if (argn != argc)
        pm_usage(usage);

    /* read first data from file */
    ppm_readppminit(ifp, &cols, &rows, &maxval, &format);

    /* no error checking required here, ppmlib does it all for us */
    srcrow = ppm_allocrow(cols);

    longfactor = (long)(flashfactor * 65536);

    /* allocate a row of pixel data for the new pixels */
    destrow = ppm_allocrow(cols);

    ppm_writeppminit(stdout, cols, rows, maxval, 0);

    /** now do the flashing **/
    /* the 'float' parameter for flashing is sort of faked - in fact, we */
    /* convert it to a range from 0 to 65536 for integer math. Shouldn't */
    /* be something you'll have to worry about, though. */

    for (i = 0; i < rows; i++) {
        ppm_readppmrow(ifp, srcrow, cols, maxval, format);

        pP = srcrow;
        pP2 = destrow;

        for (j = 0; j < cols; j++) {
            PPM_ASSIGN(*pP2, 
                       PPM_GETR(*pP) + 
                       (((maxval - PPM_GETR(*pP)) * longfactor) >> 16),
                       PPM_GETG(*pP) + 
                       (((maxval - PPM_GETG(*pP)) * longfactor) >> 16),
                       PPM_GETB(*pP) + 
                       (((maxval - PPM_GETB(*pP)) * longfactor) >> 16));

            pP++;
            pP2++;
        }

        /* write out one line of graphic data */
        ppm_writeppmrow(stdout, destrow, cols, maxval, 0);
    }

    pm_close(ifp);
    ppm_freerow(srcrow);
    ppm_freerow(destrow);

    exit(0);
}

