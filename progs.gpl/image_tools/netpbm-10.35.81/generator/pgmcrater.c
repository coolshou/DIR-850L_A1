/*

			  Fractal cratering

	   Designed and implemented in November of 1989 by:

	    John Walker
	    Autodesk SA
	    Avenue des Champs-Montants 14b
	    CH-2074 MARIN
	    Switzerland
	    Usenet: kelvin@Autodesk.com
	    Fax:    038/33 88 15
	    Voice:  038/33 76 33

    The  algorithm  used  to  determine crater size is as described on
    pages 31 and 32 of:

	Peitgen, H.-O., and Saupe, D. eds., The Science Of Fractal
	    Images, New York: Springer Verlag, 1988.

    The  mathematical  technique  used	to calculate crater radii that
    obey the proper area law distribution from a uniformly distributed
    pseudorandom sequence was developed by Rudy Rucker.

    Permission	to  use, copy, modify, and distribute this software and
    its documentation  for  any  purpose  and  without	fee  is  hereby
    granted,  without any conditions or restrictions.  This software is
    provided "as is" without express or implied warranty.

				PLUGWARE!

    If you like this kind of stuff, you may also enjoy "James  Gleick's
    Chaos--The  Software"  for  MS-DOS,  available for $59.95 from your
    local software store or directly from Autodesk, Inc., Attn: Science
    Series,  2320  Marinship Way, Sausalito, CA 94965, USA.  Telephone:
    (800) 688-2344 toll-free or, outside the  U.S. (415)  332-2344  Ext
    4886.   Fax: (415) 289-4718.  "Chaos--The Software" includes a more
    comprehensive   fractal    forgery	  generator    which	creates
    three-dimensional  landscapes  as  well as clouds and planets, plus
    five more modules which explore other aspects of Chaos.   The  user
    guide  of  more  than  200	pages includes an introduction by James
    Gleick and detailed explanations by Rudy Rucker of the  mathematics
    and algorithms used by each program.

*/

/* Modifications by Arjen Bax, 2001-06-21: Remove black vertical line at right
 * edge. Make craters wrap around the image (enables tiling of image).
 */

#define _XOPEN_SOURCE   /* get M_PI in math.h */

#include <assert.h>
#include <math.h>
#include <unistd.h>

#include "pm_c_util.h"
#include "pgm.h"
#include "mallocvar.h"

static void gencraters ARGS((void));
static void initseed ARGS((void));

/* Definitions for obtaining random numbers. */

#define Cast(low, high) ((low)+((high)-(low)) * ((rand() & 0x7FFF) / arand))

/*  Data types	*/

typedef int Boolean;
#define FALSE 0
#define TRUE 1

#define V   (void)

/*  Display parameters	*/

#define SCRX	screenxsize	      /* Screen width */
#define SCRY	screenysize	      /* Screen height */
#define SCRGAMMA 1.0		      /* Display gamma */

/*  Local variables  */

#define ImageGamma  0.5 	      /* Inherent gamma of mapped image */

static int screenxsize = 256;	      /* Screen X size */
static int screenysize = 256;	      /* Screen Y size */
static double dgamma = SCRGAMMA;      /* Display gamma */
static double arand = 32767.0;	      /* Random number parameters */
static long ncraters = 50000L;	      /* Number of craters to generate */
static double CdepthPower = 1.5;      /* Crater depth power factor */
static double DepthBias = 0.707107;   /* Depth bias */

static int modulo(int t, int n)
{
    int m;
    assert(n>0);
    m = t % n;
    while (m<0) {
	m+=n;
    }
    return m;
}

/*  INITSEED  --  Generate initial random seed, if needed.  */

static void initseed()
{
    int i;

    i = time(NULL) ^ getpid();
    srand(i);
    for (i = 0; i < 7; i++) 
        V rand();
}

/*  GENCRATERS	--  Generate cratered terrain.	*/

static void gencraters()
{
    int i, j, x, y;
    long l;
    unsigned short *aux;
    int slopemin = -52, slopemax = 52;
#define RGBQuant    255
    unsigned char *slopemap;   /* Slope to pixel map */
    gray *pixels;	       /* Pixel vector */

#define Auxadr(x, y)  &aux[modulo(y, SCRY)*SCRX+modulo(x, SCRX)]

    /* Acquire the elevation array and initialize it to mean
       surface elevation. */

    MALLOCARRAY(aux, SCRX * SCRY);
    if (aux == NULL) 
        pm_error("out of memory allocating elevation array");

    /* Acquire the elevation buffer and initialize to mean
       initial elevation. */

    for (i = 0; i < SCRY; i++) {
	unsigned short *zax = aux + (((long) SCRX) * i);

	for (j = 0; j < SCRX; j++) {
	    *zax++ = 32767;
	}
    }

    /* Every time we go around this loop we plop another crater
       on the surface.	*/

    for (l = 0; l < ncraters; l++) {
	double g;
	int cx = Cast(0.0, ((double) SCRX - 1)),
	    cy = Cast(0.0, ((double) SCRY - 1)),
	    gx, gy, x, y;
	unsigned long amptot = 0, axelev;
	unsigned int npatch = 0;


	/* Phase 1.  Compute the mean elevation of the impact
		     area.  We assume the impact area is a
		     fraction of the total crater size. */

	/* Thanks, Rudy, for this equation  that maps the uniformly
	   distributed	numbers  from	Cast   into   an   area-law
	   distribution as observed on cratered bodies. */

	g = sqrt(1 / (M_PI * (1 - Cast(0, 0.9999))));

	/* If the crater is tiny, handle it specially. */

#if 0
	fprintf(stderr, "crater=%lu ", (unsigned long)l);
	fprintf(stderr, "cx=%d ", cx);
	fprintf(stderr, "cy=%d ", cy);
	fprintf(stderr, "size=%g\n", g);
#endif

	if (g < 3) {

	   /* Set pixel to the average of its Moore neighbourhood. */

	    for (y = cy - 1; y <= cy + 1; y++) {
		for (x = cx - 1; x <= cx + 1; x++) {
		    amptot += *Auxadr(x, y);
		    npatch++;
		}
	    }
	    axelev = amptot / npatch;

	    /* Perturb the mean elevation by a small random factor. */

	    x = (g >= 1) ? ((rand() >> 8) & 3) - 1 : 0;
	    *Auxadr(cx, cy) = axelev + x;

	    /* Jam repaint sizes to correct patch. */

	    gx = 1;
	    gy = 0;

	} else {

	    /* Regular crater.	Generate an impact feature of the
	       correct size and shape. */

	    /* Determine mean elevation around the impact area. */

	    gx = MAX(2, (g / 3));
	    gy = MAX(2, g / 3);

	    for (y = cy - gy; y <= cy + gy; y++) {
		for (x = cx-gx; x <= cx + gx; x++) {
		    amptot += *Auxadr(x,y);
		    npatch++;
		}
	    }
	    axelev = amptot / npatch;

	    gy = MAX(2, g);
	    g = gy;
	    gx = MAX(2, g);

	    for (y = cy - gy; y <= cy + gy; y++) {
		double dy = (cy - y) / (double) gy,
		       dysq = dy * dy;

		for (x = cx - gx; x <= cx + gx; x++) {
		    double dx = ((cx - x) / (double) gx),
			   cd = (dx * dx) + dysq,
			   cd2 = cd * 2.25,
			   tcz = DepthBias - sqrt(fabs(1 - cd2)),
			   cz = MAX((cd2 > 1) ? 0.0 : -10, tcz),
			   roll, iroll;
		    unsigned short av;

		    cz *= pow(g, CdepthPower);
		    if (dy == 0 && dx == 0 && ((int) cz) == 0) {
		       cz = cz < 0 ? -1 : 1;
		    }

#define 	    rollmin 0.9
		    roll = (((1 / (1 - MIN(rollmin, cd))) /
			     (1 / (1 - rollmin))) - (1 - rollmin)) / rollmin;
		    iroll = 1 - roll;

		    av = (axelev + cz) * iroll + (*Auxadr(x,y) + cz) * roll;
		    av = MAX(1000, MIN(64000, av));
		    *Auxadr(x,y) = av;
		}
	    }
	 }
	if ((l % 5000) == 4999) {
	    pm_message( "%ld craters generated of %ld (%ld%% done)",
		l + 1, ncraters, ((l + 1) * 100) / ncraters);
	}
    }

    i = MAX((slopemax - slopemin) + 1, 1);
    MALLOCARRAY(slopemap, i);
    if (slopemap == NULL)
        pm_error("out of memory allocating slope map");

    for (i = slopemin; i <= slopemax; i++) {

        /* Confused?   OK,  we're using the  left-to-right slope to
	   calculate a shade based on the sine of  the	angle  with
	   respect  to the vertical (light incident from the left).
	   Then, with one exponentiation, we account for  both	the
	   inherent   gamma   of   the	 image	(ad-hoc),  and	the
	   user-specified display gamma, using the identity:

		 (x^y)^z = (x^(y*z))		     */

	slopemap[i - slopemin] = i > 0 ?
	    (128 + 127.0 *
		pow(sin((M_PI / 2) * i / slopemax),
		       dgamma * ImageGamma)) :
	    (128 - 127.0 *
		pow(sin((M_PI / 2) * i / slopemin),
		       dgamma * ImageGamma));
    }

    /* Generate the screen image. */

    pgm_writepgminit(stdout, SCRX, SCRY, RGBQuant, FALSE);
    pixels = pgm_allocrow(SCRX);

    for (y = 0; y < SCRY; y++) {
	gray *pix = pixels;

	for (x = 0; x < SCRX; x++) {
	    int j = *Auxadr(x+1, y) - *Auxadr(x, y);
	    j = MIN(MAX(slopemin, j), slopemax);
	    *pix++ = slopemap[j - slopemin];
	}
	pgm_writepgmrow(stdout, pixels, SCRX, RGBQuant, FALSE);
    }
    pm_close(stdout);
    pgm_freerow(pixels);

#undef Auxadr
#undef Scradr
    free((char *) slopemap);
    free((char *) aux);
}

/*  MAIN  --  Main program.  */

int main(argc, argv)
  int argc;
  char *argv[];
{
    int i;
    Boolean gammaspec = FALSE, numspec = FALSE,
	    widspec = FALSE, hgtspec = FALSE;
    const char * const usage = "[-number <n>] [-width|-xsize <w>]\n\
                  [-height|-ysize <h>] [-gamma <f>]";

    DepthBias = sqrt(0.5);	      /* Get exact value for depth bias */


    pgm_init(&argc, argv);

    i = 1;
    while ((i < argc) && (argv[i][0] == '-') && (argv[i][1] != '\0')) {
        if (pm_keymatch(argv[i], "-gamma", 2)) {
	    if (gammaspec) {
                pm_error("already specified gamma correction");
	    }
	    i++;
            if ((i == argc) || (sscanf(argv[i], "%lf", &dgamma)  != 1))
		pm_usage(usage);
	    if (dgamma <= 0.0) {
                pm_error("gamma correction must be greater than 0");
	    }
	    gammaspec = TRUE;
        } else if (pm_keymatch(argv[i], "-number", 2)) {
	    if (numspec) {
                pm_error("already specified number of craters");
	    }
	    i++;
            if ((i == argc) || (sscanf(argv[i], "%ld", &ncraters) != 1))
		pm_usage(usage);
	    if (ncraters <= 0) {
                pm_error("number of craters must be greater than 0!");
	    }
	    numspec = TRUE;
        } else if (pm_keymatch(argv[i], "-xsize", 2) ||
                   pm_keymatch(argv[i], "-width", 2)) {
	    if (widspec) {
                pm_error("already specified a width/xsize");
	    }
	    i++;
            if ((i == argc) || (sscanf(argv[i], "%d", &screenxsize) != 1))
		pm_usage(usage);
	    if (screenxsize <= 0) {
                pm_error("screen width must be greater than 0");
	    }
	    widspec = TRUE;
        } else if (pm_keymatch(argv[i], "-ysize", 2) ||
                   pm_keymatch(argv[i], "-height", 2)) {
	    if (hgtspec) {
                pm_error("already specified a height/ysize");
	    }
	    i++;
            if ((i == argc) || (sscanf(argv[i], "%d", &screenysize) != 1))
		pm_usage(usage);
	    if (screenxsize <= 0) {
                pm_error("screen height must be greater than 0");
	    }
	    hgtspec = TRUE;
	} else {
	    pm_usage(usage);
	}
	i++;
    }

    initseed();
    gencraters();

    exit(0);
}
