
/*********************************************************************/
/* ppmshift -  shift lines of a picture left or right by x pixels    */
/* Frank Neumann, October 1993                                       */
/* V1.1 16.11.1993                                                   */
/*                                                                   */
/* version history:                                                  */
/* V1.0    11.10.1993  first version                                 */
/* V1.1    16.11.1993  Rewritten to be NetPBM.programming conforming */
/*********************************************************************/

#include "ppm.h"

/* global variables */
#ifdef AMIGA
static char *version = "$VER: ppmshift 1.1 (16.11.93)"; /* Amiga version identification */
#endif

/**************************/
/* start of main function */
/**************************/
int main(argc, argv)
int argc;
char *argv[];
{
	FILE* ifp;
	time_t timenow;
	int argn, rows, cols, format, i = 0, j = 0;
	pixel *srcrow, *destrow;
	pixel *pP = NULL, *pP2 = NULL;
	pixval maxval;
	int shift, nowshift;
	const char * const usage = "shift [ppmfile]\n        shift: maximum number of pixels to shift a line by\n";

	/* parse in 'default' parameters */
	ppm_init(&argc, argv);

	argn = 1;

	/* parse in shift number */
	if (argn == argc)
		pm_usage(usage);
	if (sscanf(argv[argn], "%d", &shift) != 1)
		pm_usage(usage);
	if (shift < 0)
		pm_error("shift factor must be 0 or more");
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

	if (shift > cols)
    {
		shift = cols;
        pm_message("shift amount is larger than picture width - reset to %d", shift);
    }

	/* no error checking required here, ppmlib does it all for us */
	srcrow = ppm_allocrow(cols);

	/* allocate a row of pixel data for the new pixels */
	destrow = ppm_allocrow(cols);

	ppm_writeppminit(stdout, cols, rows, maxval, 0);

	/* get time of day to feed the random number generator */
	timenow = time(NULL);
	srand(timenow);

	/** now do the shifting **/
	/* the range by which a line is shifted lays in the range from */
	/* -shift/2 .. +shift/2 pixels; however, within this range it is */
    /* randomly chosen */
	for (i = 0; i < rows; i++)
	{
		if (shift != 0)
			nowshift = (rand() % (shift+1)) - ((shift+1) / 2);
		else
			nowshift = 0;

		ppm_readppmrow(ifp, srcrow, cols, maxval, format);

		pP = srcrow;
		pP2 = destrow;

		/* if the shift value is less than zero, we take the original pixel line and */
		/* copy it into the destination line translated to the left by x pixels. The */
        /* empty pixels on the right end of the destination line are filled up with  */
		/* the pixel that is the right-most in the original pixel line.              */
		if (nowshift < 0)
		{
			pP+= abs(nowshift);
			for (j = 0; j < cols; j++)
			{
				PPM_ASSIGN(*pP2, PPM_GETR(*pP), PPM_GETG(*pP), PPM_GETB(*pP));
				pP2++;
                if (j < (cols+nowshift)-1)
					pP++;
			}
		}
		/* if the shift value is 0 or positive, the first <nowshift> pixels of the */
		/* destination line are filled with the first pixel from the source line,  */
		/* and the rest of the source line is copied to the dest line              */
		else
		{
			for (j = 0; j < cols; j++)
			{
				PPM_ASSIGN(*pP2, PPM_GETR(*pP), PPM_GETG(*pP), PPM_GETB(*pP));
				pP2++;
                if (j >= nowshift)
					pP++;
			}
		}

		/* write out one line of graphic data */
		ppm_writeppmrow(stdout, destrow, cols, maxval, 0);
	}

	pm_close(ifp);
	ppm_freerow(srcrow);
	ppm_freerow(destrow);

	exit(0);
}

