
/*********************************************************************/
/* ppmtv -  make a 'look-alike ntsc' picture from a PPM file       */
/* Frank Neumann, October 1993                                       */
/* V1.1 16.11.1993                                                   */
/*                                                                   */
/* version history:                                                  */
/* V1.0 12.10.1993    first version                                  */
/* V1.1 16.11.1993    Rewritten to be NetPBM.programming conforming  */
/*********************************************************************/

#include "ppm.h"

/**************************/
/* start of main function */
/**************************/
int main(argc, argv)
int argc;
char *argv[];
{
	FILE* ifp;
	int argn, rows, cols, format, i = 0, j = 0;
	pixel *srcrow, *destrow;
	pixel *pP = NULL, *pP2 = NULL;
	pixval maxval;
	double dimfactor;
	long longfactor;
	const char * const usage = "dimfactor [ppmfile]\n        dimfactor: 0.0 = total blackness, 1.0 = original picture\n";

	/* parse in 'default' parameters */
	ppm_init(&argc, argv);

	argn = 1;

	/* parse in dim factor */
	if (argn == argc)
		pm_usage(usage);
	if (sscanf(argv[argn], "%lf", &dimfactor) != 1)
		pm_usage(usage);
	if (dimfactor < 0.0 || dimfactor > 1.0)
		pm_error("dim factor must be in the range from 0.0 to 1.0 ");
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

	longfactor = (long)(dimfactor * 65536);

	/* allocate a row of pixel data for the new pixels */
	destrow = ppm_allocrow(cols);

	ppm_writeppminit(stdout, cols, rows, maxval, 0);

	/** now do the ntsc'ing (actually very similar to ppmdim) **/
	for (i = 0; i < rows; i++)
	{
		ppm_readppmrow(ifp, srcrow, cols, maxval, format);

		pP = srcrow;
		pP2 = destrow;

		for (j = 0; j < cols; j++)
		{
			/* every alternating row is left in unchanged condition */
			if (i & 1)
			{
				PPM_ASSIGN(*pP2, PPM_GETR(*pP), PPM_GETG(*pP), PPM_GETB(*pP));
			}
			/* and the other lines are dimmed to the specified factor */
			else
			{
				PPM_ASSIGN(*pP2, (PPM_GETR(*pP) * longfactor) >> 16,
								 (PPM_GETG(*pP) * longfactor) >> 16,
								 (PPM_GETB(*pP) * longfactor) >> 16);
			}
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

