/* 
 *
 * (c) Robert Tinsley, 2003 (http://www.thepoacher.net/contact)
 *
 * Released under the GPL (http://www.fsf.org/licenses/gpl.txt)
 *
 *
 * CHANGES
 *
 * v1.00 2003-02-28 Original version
 *
 * v1.10 2003-03-02
 *  + changed to use pam_* routines rather than ppm_*
 *  + changed to use pm_* rather than fopen()/fclose()
 *  + renamed from ppmgrid to pampup9
 *  + wrote a man-page (actually, html)
 *
 * Changes by Bryan Henderson for inclusion in the Netpbm package (fully
 * exploiting Netpbm library).  Renamed to Pampop9.  March 2003.
 *
 */

#include <stdio.h>
#include <stdlib.h> /* atoi() */

#include "pam.h"

static const char * const copyright = 
  "(c) Robert Tinsley 2003 (http://www.thepoacher.net/contact)";

static const char *usagestr = "pnmfile|- xtiles ytiles xdelta ydelta";

int main(int argc, char *argv[])
{
    const char *filename = "-";
    int xtiles, ytiles, xdelta, ydelta;
    FILE *fp;

    struct pam spam, dpam;
    tuple **spix, *dpix;
    int xtilesize, ytilesize, sx, sy, dx, dy, p;



    pnm_init(&argc, argv);

    if (argc-1 != 5) {
        pm_error("Wrong number of arguments.  Program requires 5 arguments; "
                 "you supplied %d.  Usage: %s", argc-1, usagestr);
    }

    filename = argv[1];
    xtiles = atoi(argv[2]);
    ytiles = atoi(argv[3]);
    xdelta = atoi(argv[4]);
    ydelta = atoi(argv[5]);

    if (filename == NULL || *filename == '\0'
        || xtiles <= 0 || ytiles <= 0 || xdelta < 0 || ydelta < 0) 
        pm_error("invalid argument");

    /* read src pam */

    fp = pm_openr(filename);

    spix = pnm_readpam(fp, &spam, PAM_STRUCT_SIZE(tuple_type));

    pm_close(fp);

    /* init dst pam */

    xtilesize = spam.width  - (xtiles - 1) * xdelta;
    ytilesize = spam.height - (ytiles - 1) * ydelta;

    if (xtilesize <= 0)
        pm_error("xtilesize must be positive.  You specified %d", xtilesize);
    if (ytilesize <= 0)
        pm_error("ytilesize must be positive.  You specified %d", ytilesize);

    dpam = spam;
    dpam.file = stdout;
    dpam.width  = xtiles * xtilesize;
    dpam.height = ytiles * ytilesize;

    dpix = pnm_allocpamrow(&dpam);

    pnm_writepaminit(&dpam);

    /* generate dst pam */

    for (dy = 0; dy < dpam.height; dy++) {
        sy = ((int) (dy / ytilesize)) * ydelta + (dy % ytilesize);
        for (dx = 0; dx < dpam.width; dx++) {
            sx = ((int) (dx / xtilesize)) * xdelta + (dx % xtilesize);
                for (p = 0; p < spam.depth; ++p) {
                    dpix[dx][p] = spix[sy][sx][p];
            }
        }
        pnm_writepamrow(&dpam, dpix);
    }

    /* all done */

    pnm_freepamarray(spix, &spam);
    pnm_freepamrow(dpix);

    exit(EXIT_SUCCESS);
}
