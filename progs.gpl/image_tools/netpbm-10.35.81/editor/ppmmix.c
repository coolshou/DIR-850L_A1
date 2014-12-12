
/*********************************************************************/
/* ppmmix -  mix together two pictures like with a fader             */
/* Frank Neumann, October 1993                                       */
/* V1.2 16.11.1993                                                   */
/*                                                                   */
/* version history:                                                  */
/* V1.0 Aug   1993    first version                                  */
/* V1.1 12.10.1993    uses ppm libs&headers, integer math, cleanups  */
/* V1.2 16.11.1993    Rewritten to be NetPBM.programming conforming  */
/*********************************************************************/

#include "ppm.h"

/* global variables */
#ifdef AMIGA
static char *version = "$VER: ppmmix 1.2 (16.11.93)"; /* Amiga version identification */
#endif

/**************************/
/* start of main function */
/**************************/
int main(argc, argv)
int argc;
char *argv[];
{
	FILE *ifp1, *ifp2;
	int argn, rows, cols, format, i = 0, j = 0;
	int rows2, cols2, format2;
	pixel *srcrow1, *srcrow2, *destrow;
	pixel *pP1, *pP2, *pP3;
	pixval maxval, maxval2;
	pixval r1, r2, r3, g1, g2, g3, b1, b2, b3;
	double fadefactor;
	long longfactor;
	const char * const usage = "fadefactor ppmfile1 ppmfile2\n        fadefactor: 0.0 = only ppmfile1, 1.0 = only ppmfile2\n";

	/* parse in 'default' parameters */
	ppm_init(&argc, argv);

	argn = 1;

	/* parse in dim factor */
	if (argn == argc)
		pm_usage(usage);
	if (sscanf(argv[argn], "%lf", &fadefactor) != 1)
		pm_usage(usage);
	if (fadefactor < 0.0 || fadefactor > 1.0)
		pm_error("fade factor must be in the range from 0.0 to 1.0 ");
	++argn;

	/* parse in filenames and open files (cannot be stdin-filters, sorry..) */
	if (argn == argc-2)
	{
		ifp1 = pm_openr(argv[argn]);
		++argn;
		ifp2 = pm_openr(argv[argn]);
	}
	else
		pm_usage(usage);

	/* read first data from both files and compare sizes etc. */
	ppm_readppminit(ifp1, &cols, &rows, &maxval, &format);
	ppm_readppminit(ifp2, &cols2, &rows2, &maxval2, &format2);

    if ( (cols != cols2) || (rows != rows2) )
        pm_error("image sizes are different!");

    if ( maxval != maxval2)
		pm_error("images have different maxvalues");

	if (format != format2)
	{
		pm_error("images have different PxM types");
	}

	/* no error checking required here, ppmlib does it all for us */
	srcrow1 = ppm_allocrow(cols);
	srcrow2 = ppm_allocrow(cols);

	longfactor = (long)(fadefactor * 65536);

	/* allocate a row of pixel data for the new pixels */
	destrow = ppm_allocrow(cols);

	ppm_writeppminit(stdout, cols, rows, maxval, 0);

	for (i = 0; i < rows; i++)
	{
		ppm_readppmrow(ifp1, srcrow1, cols, maxval, format);
		ppm_readppmrow(ifp2, srcrow2, cols, maxval, format);

		pP1 = srcrow1;
		pP2 = srcrow2;
        pP3 = destrow;

		for (j = 0; j < cols; j++)
		{
			r1 = PPM_GETR(*pP1);
			g1 = PPM_GETG(*pP1);
			b1 = PPM_GETB(*pP1);

			r2 = PPM_GETR(*pP2);
			g2 = PPM_GETG(*pP2);
			b2 = PPM_GETB(*pP2);

			r3 = r1 + (((r2 - r1) * longfactor) >> 16);
			g3 = g1 + (((g2 - g1) * longfactor) >> 16);
			b3 = b1 + (((b2 - b1) * longfactor) >> 16);


			PPM_ASSIGN(*pP3, r3, g3, b3);

			pP1++;
			pP2++;
			pP3++;
		}

		/* write out one line of graphic data */
		ppm_writeppmrow(stdout, destrow, cols, maxval, 0);
	}

	pm_close(ifp1);
	pm_close(ifp2);
	ppm_freerow(srcrow1);
	ppm_freerow(srcrow2);
	ppm_freerow(destrow);

	exit(0);
}

