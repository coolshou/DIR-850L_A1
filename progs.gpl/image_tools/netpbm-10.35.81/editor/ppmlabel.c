/*

           Add text labels to a PPM image

           by John Walker  --  kelvin@fourmilab.ch
           WWW home page: http://www.fourmilab.ch/
                  June 1995
*/

#define _XOPEN_SOURCE   /* get M_PI in math.h */

#include <math.h>
#include <string.h>

#include "pm_c_util.h"
#include "ppm.h"
#include "ppmdraw.h"

#define dtr(x)  (((x) * M_PI) / 180.0)

static int argn, rows, cols, x, y, size, angle, transparent;
static pixel **pixels;
static pixval maxval;
static pixel rgbcolor, backcolor;

/*  DRAWTEXT  --  Draw text at current location and advance to
          start of next line.  */

static void 
drawtext(const char * const text) {

    if (!transparent && strlen(text) > 0) {
        struct fillobj * handle;

        int left, top, right, bottom;
        int lx, ly;
        int p1x, p1y, p2x, p2y, p3x, p3y, p4x, p4y;
        double sina, cosa;

        handle = ppmd_fill_create();
        
        ppmd_text_box(size, 0, text, &left, &top, &right, &bottom);

        /* Displacement vector */ 

        lx = right;
        ly = -(top - bottom);

        /* Sine and cosine */

        sina = sin(dtr(angle));
        cosa = cos(dtr(angle));

        /* Rotated extent box corners */

        p1x = (int) ((x + left * cosa + bottom * sina) + 0.5);
        p1y = (int) ((y + bottom * cosa + -left * sina) + 0.5);

#define WERF    ppmd_fill_drawproc, handle

        p2x = (int) (p1x - sina * ly + 0.5);
        p2y = (int) ((p1y - cosa * ly) + 0.5);

        p3x = (int) (p1x + cosa * lx + -sina * ly + 0.5);
        p3y = (int) ((p1y - (cosa * ly + sina * lx)) + 0.5);

        p4x = (int) (p1x + cosa * lx + 0.5);
        p4y = (int) ((p1y - sina * lx) + 0.5);

        ppmd_line(pixels, cols, rows, maxval,
                  p1x, p1y, p2x, p2y,
                  WERF);
        ppmd_line(pixels, cols, rows, maxval,
                  p2x, p2y, p3x, p3y,
                  WERF);
        ppmd_line(pixels, cols, rows, maxval,
                  p3x, p3y, p4x, p4y,
                  WERF);
        ppmd_line(pixels, cols, rows, maxval,
                  p4x, p4y, p1x, p1y,
                  WERF);


        ppmd_fill(pixels, cols, rows, maxval,
                  handle, PPMD_NULLDRAWPROC, (char *) &backcolor);

        ppmd_fill_destroy(handle);
    }
    ppmd_text(pixels, cols, rows, maxval,
              x, y, size, angle, text,
              PPMD_NULLDRAWPROC, (char *) &rgbcolor);

    /* For convenience, simulate a carriage return to the next line.
       This allows multiple "-text" specifications or multiple lines
       in a -file input to write consecutive lines of text in a
       generally reasonable fashion.
    */

    x += (int) ((cos(dtr(angle + 270)) * size * 1.75) + 0.5);
    y -= (int) ((sin(dtr(angle + 270)) * size * 1.75) + 0.5);
}



int
main(int argc, char *argv[]) {

    FILE *ifP;

    /* Process standard command line arguments */

    ppm_init(&argc, argv);

    argn = 1;

    /* Check for explicit input file specification,  Note that
       we count on the fact that every command line switch
       takes a single argument.  If this becomes untrue due
       to a change in the future, you'll have to make this
       test smarter.
    */

    if ((argn != argc) && (argc == 2 || argv[argc - 2][0] != '-')) {
        ifP = pm_openr(argv[argc - 1]);
        argc--;
    } else
        ifP = stdin;

    /* Load input image */

    pixels = ppm_readppm(ifP, &cols, &rows, &maxval);
    pm_close(ifP);

    /* Set initial defaults */

    x = 0;
    y = rows / 2;
    size = 12;
    angle = 0;
    PPM_ASSIGN(rgbcolor, maxval, maxval, maxval);
    PPM_ASSIGN(backcolor, 0, 0, 0);
    transparent = TRUE;

    while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0') {

        if (pm_keymatch(argv[argn], "-angle", 1)) {
            argn++;
            if ((argn == argc) || (sscanf(argv[argn], "%d", &angle) != 1))
                pm_error("-angle doesn't have a value");

        } else if (pm_keymatch(argv[argn], "-background", 1)) {
            argn++;
            if (strcmp(argv[argn], "transparent") == 0) {
                transparent = TRUE;
            } else {
                transparent = FALSE;
                backcolor = ppm_parsecolor(argv[argn], maxval);
            }

        } else if (pm_keymatch(argv[argn], "-color", 1)
                   || pm_keymatch(argv[argn], "-colour", 1)) {
            argn++;
            rgbcolor = ppm_parsecolor(argv[argn], maxval);

        } else if (pm_keymatch(argv[argn], "-file", 1)) {
            char s[512];

            argn++;
            ifP = pm_openr(argv[argn]);
            while (fgets(s, sizeof s, ifP) != NULL) {
                while (s[0] != 0 && s[strlen(s) - 1] < ' ') {
                    s[strlen(s) - 1] = 0;
                }
                drawtext(s);
            }
            pm_close(ifP);

        } else if (pm_keymatch(argv[argn], "-size", 1)) {
            argn++;
            if ((argn == argc) || (sscanf(argv[argn], "%d", &size) != 1))
                pm_error("-size doesn't have a value");
        } else if (pm_keymatch(argv[argn], "-text", 1)) {
            argn++;
            drawtext(argv[argn]);

        } else if (pm_keymatch(argv[argn], "-u", 1)) {
            pm_error("-u doesn't have a value");

        } else if (pm_keymatch(argv[argn], "-x", 1)) {
            argn++;
            if ((argn == argc) || (sscanf(argv[argn], "%d", &x) != 1))
                pm_error("-x doesn't have a value");

        } else if (pm_keymatch(argv[argn], "-y", 1)) {
            argn++;
            if ((argn == argc) || (sscanf(argv[argn], "%d", &y) != 1))
                pm_error("-y doesn't have a value");

        } else
            pm_error("Unrecognized option: '%s'", argv[argn]);
        argn++;
    }

    if (argn != argc)
        pm_error("Extraneous arguments");

    ppm_writeppm(stdout, pixels, cols, rows, maxval, 0);

    ppm_freearray(pixels, rows);

    return 0;
}
