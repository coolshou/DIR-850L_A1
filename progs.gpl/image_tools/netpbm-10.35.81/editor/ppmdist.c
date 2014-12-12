#include "ppm.h"
#include "mallocvar.h"


/*
 * Yep, it's a very simple algorithm, but it was something I wanted to have
 * available.
 */

struct colorToGrayEntry {
    pixel           color;
    gray            gray;
    int             frequency;
};

/*
 * BUG: This number was chosen pretty arbitrarily.  The program is * probably
 * only useful for a very small numbers of colors - and that's * not only
 * because of the O(n) search that's used.  The idea lends * itself primarily
 * to low color (read: simple, machine generated) images.
 */
#define MAXCOLORS 255


static gray
newGrayValue(pixel *pix, struct colorToGrayEntry *colorToGrayMap, int colors) {

    int color;
    /*
     * Allowing this to be O(n), since the program is intended for small
     * n.  Later, perhaps sort by color (r, then g, then b) and bsearch.
     */
    for (color = 0; color < colors; color++) {
        if (PPM_EQUAL(*pix, colorToGrayMap[color].color))
            return colorToGrayMap[color].gray;
    }
    pm_error("This should never happen - contact the maintainer");
    return (-1);
}



static int
cmpColorToGrayEntryByIntensity(const void *entry1, const void *entry2) {

    return ((struct colorToGrayEntry *) entry1)->gray -
        ((struct colorToGrayEntry *) entry2)->gray;
}



static int
cmpColorToGrayEntryByFrequency(const void * entry1, const void * entry2) {

    return ((struct colorToGrayEntry *) entry1)->frequency -
        ((struct colorToGrayEntry *) entry2)->frequency;
}



int
main(int argc, char *argv[]) {

    FILE           *ifp;
    int             col, cols, row, rows, color, colors, argn;
    int             frequency;
    pixval          maxval;
    pixel         **pixels;
    pixel          *pP;
    colorhist_vector hist;
    gray           *grayrow;
    gray           *gP;
    struct colorToGrayEntry *colorToGrayMap;


    ppm_init(&argc, argv);

    argn = 1;
    /* Default is to sort colors by intensity */
    frequency = 0;

    while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0') {
        if (pm_keymatch(argv[argn], "-frequency", 2))
            frequency = 1;
        else if (pm_keymatch(argv[argn], "-intensity", 2))
            frequency = 0;
        else
            pm_usage( "[-frequency|-intensity] [ppmfile]" );
        ++argn;
    }

    if (argn < argc) {
        ifp = pm_openr(argv[argn]);
        ++argn;
    } else
        ifp = stdin;

    pixels = ppm_readppm(ifp, &cols, &rows, &maxval);
    pm_close(ifp);
    /* all done with the input file - it's entirely in memory */

    /*
     * Compute a histogram of the colors in the input.  This is good for
     * both frequency, and indirectly the intensity, of a color.
     */
    hist = ppm_computecolorhist(pixels, cols, rows, MAXCOLORS, &colors);

    if (hist == (colorhist_vector) 0)
        /*
         * BUG: This perhaps should use an exponential backoff, in
         * the number of colors, until success - cf pnmcolormap's
         * approach.  The results are then more what's expected, but
         * not necessarily very useful.
         */
        pm_error("Too many colors - Try reducing with pnmquant");

    /* copy the colors into another structure for sorting */
    MALLOCARRAY(colorToGrayMap, colors);
    for (color = 0; color < colors; color++) {
        colorToGrayMap[color].color = hist[color].color;
        colorToGrayMap[color].frequency = hist[color].value;
        /*
         * This next is derivable, of course, but it's far faster to
         * store it precomputed.  This can be skipped, when sorting
         * by frequency - but again, for a small number of colors
         * it's a small matter.
         */
        colorToGrayMap[color].gray = PPM_LUMIN(hist[color].color);
    }

    /*
     * sort by intensity - sorting by frequency (in the histogram) is
     * worth considering as a future addition.
     */
    if (frequency)
        qsort(colorToGrayMap, colors, sizeof(struct colorToGrayEntry),
              &cmpColorToGrayEntryByFrequency);
    else
        qsort(colorToGrayMap, colors, sizeof(struct colorToGrayEntry),
              &cmpColorToGrayEntryByIntensity);

    /*
     * create mapping between the n colors in input, to n evenly spaced
     * grayscale intensities.  This is done by overwriting the neatly
     * formed gray values corresponding to the input-colors, with a new
     * set of evenly spaced gray values.  Since maxval can be changed on
     * a lark, we just use gray levels 0..colors-1, and adjust maxval
     * accordingly
     */
    maxval = colors - 1;
    for (color = 0; color < colors; color++)
        colorToGrayMap[color].gray = color;

    /* write pgm file, mapping colors to intensities */
    pgm_writepgminit(stdout, cols, rows, maxval, 0);

    grayrow = pgm_allocrow(cols);

    for (row = 0; row < rows; row++) {
        for (col = 0, pP = pixels[row], gP = grayrow; col < cols;
             col++, pP++, gP++)
            *gP = newGrayValue(pP, colorToGrayMap, colors);
        pgm_writepgmrow(stdout, grayrow, cols, maxval, 0);
    }

    pm_close(stdout);

    exit(0);
}

