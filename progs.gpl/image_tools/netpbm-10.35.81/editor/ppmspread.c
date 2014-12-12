/*********************************************************************/
/* ppmspread -  randomly displace a PPM's pixels by a certain amount */
/* Frank Neumann, October 1993                                       */
/* V1.1 16.11.1993                                                   */
/*                                                                   */
/* version history:                                                  */
/* V1.0 12.10.1993    first version                                  */
/* V1.1 16.11.1993    Rewritten to be NetPBM.programming conforming  */
/*********************************************************************/

#include <string.h>
#include "ppm.h"

/* global variables */
#ifdef AMIGA
static char *version = "$VER: ppmspread 1.1 (16.11.93)"; /* Amiga version identification */
#endif

/**************************/
/* start of main function */
/**************************/
int main(argc, argv)
int argc;
char *argv[];
{
	FILE* ifp;
	int argn, rows, cols, i, j;
	int xdis, ydis, xnew, ynew;
	pixel **destarray, **srcarray;
	pixel *pP, *pP2;
	pixval maxval;
	pixval r1, g1, b1;
	int amount;
	time_t timenow;
	const char * const usage = "amount [ppmfile]\n        amount: # of pixels to displace a pixel by at most\n";

	/* parse in 'default' parameters */
	ppm_init(&argc, argv);

	argn = 1;

	/* parse in amount & seed */
	if (argn == argc)
		pm_usage(usage);
	if (sscanf(argv[argn], "%d", &amount) != 1)
		pm_usage(usage);
	if (amount < 0)
		pm_error("amount should be a positive number");
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

	/* read entire picture into buffer */
	srcarray = ppm_readppm(ifp, &cols, &rows, &maxval);

	/* allocate an entire picture buffer for dest picture */
	destarray = ppm_allocarray(cols, rows);

	/* clear out the buffer */
	for (i=0; i < rows; i++)
		memset(destarray[i], 0, cols * sizeof(pixel));

	/* set seed for random number generator */
	/* get time of day to feed the random number generator */
	timenow = time(NULL);
	srand(timenow);

	/* start displacing pixels */
	for (i = 0; i < rows; i++)
	{
		pP = srcarray[i];

		for (j = 0; j < cols; j++)
		{
			xdis = (rand() % (amount+1)) - ((amount+1) / 2);
			ydis = (rand() % (amount+1)) - ((amount+1) / 2);

			xnew = j + xdis;
			ynew = i + ydis;

			/* only set the displaced pixel if it's within the bounds of the image */
			if (xnew >= 0 && xnew < cols && ynew >= 0 && ynew < rows)
			{
				/* displacing a pixel is accomplished by swapping it with another */
				/* pixel in its vicinity - so, first store other pixel's RGB      */
                pP2 = srcarray[ynew] + xnew;
				r1 = PPM_GETR(*pP2);
				g1 = PPM_GETG(*pP2);
				b1 = PPM_GETB(*pP2);
				/* set second pixel to new value */
				pP2 = destarray[ynew] + xnew;
				PPM_ASSIGN(*pP2, PPM_GETR(*pP), PPM_GETG(*pP), PPM_GETB(*pP));

				/* now, set first pixel to (old) value of second */
				pP2 = destarray[i] + j;
				PPM_ASSIGN(*pP2, r1, g1, b1);
			}
			else
			{
                /* displaced pixel is out of bounds; leave the old pixel there */
                pP2 = destarray[i] + j;
				PPM_ASSIGN(*pP2, PPM_GETR(*pP), PPM_GETG(*pP), PPM_GETB(*pP));
			}
			pP++;
		}
	}

	/* write out entire dest picture in one go */
	ppm_writeppm(stdout, destarray, cols, rows, maxval, 0);

	pm_close(ifp);
	ppm_freearray(srcarray, rows);
	ppm_freearray(destarray, rows);

	exit(0);
}

