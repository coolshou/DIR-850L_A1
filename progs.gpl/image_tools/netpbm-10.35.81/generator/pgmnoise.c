
/*********************************************************************/
/* pgmnoise -  create a portable graymap with white noise            */
/* Frank Neumann, October 1993                                       */
/* V1.1 16.11.1993                                                   */
/*                                                                   */
/* version history:                                                  */
/* V1.0 12.10.1993  first version                                    */
/* V1.1 16.11.1993  Rewritten to be NetPBM.programming conforming    */
/*********************************************************************/

#include <unistd.h>

#include "pgm.h"

/* global variables */
#ifdef AMIGA
static char *version = "$VER: pgmnoise 1.1 (16.11.93)"; /* Amiga version identification */
#endif

/**************************/
/* start of main function */
/**************************/
int main(argc, argv)
int argc;
char *argv[];
{
	int argn, rows, cols, i, j;
	gray *destrow;
	const char * const usage = "width height\n        width and height are picture dimensions in pixels\n";
	time_t timenow;

	/* parse in 'default' parameters */
	pgm_init(&argc, argv);

	argn = 1;

	/* parse in dim factor */
	if (argn == argc)
		pm_usage(usage);
	if (sscanf(argv[argn], "%d", &cols) != 1)
		pm_usage(usage);
	argn++;
	if (argn == argc)
		pm_usage(usage);
	if (sscanf(argv[argn], "%d", &rows) != 1)
		pm_usage(usage);

	if (cols <= 0 || rows <= 0)
		pm_error("picture dimensions should be positive numbers");
	++argn;

	if (argn != argc)
		pm_usage(usage);

	/* no error checking required here, ppmlib does it all for us */
	destrow = pgm_allocrow(cols);

	pgm_writepgminit(stdout, cols, rows, PGM_MAXMAXVAL, 0);

	/* get time of day to feed the random number generator */
	timenow = time(NULL);
	srand(timenow ^ getpid());

	/* create the (gray) noise */
	for (i = 0; i < rows; i++)
	{
		for (j = 0; j < cols; j++)
			destrow[j] = rand() % (PGM_MAXMAXVAL+1);

		/* write out one line of graphic data */
		pgm_writepgmrow(stdout, destrow, cols, PGM_MAXMAXVAL, 0);
	}

	pgm_freerow(destrow);

	exit(0);
}

