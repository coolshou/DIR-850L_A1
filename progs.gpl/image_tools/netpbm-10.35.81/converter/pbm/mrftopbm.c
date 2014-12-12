/* mrftopbm - convert mrf to pbm
 * public domain by RJM
 *
 * Adapted to Netpbm by Bryan Henderson 2003.08.09.  Bryan got his copy from
 * ftp://ibiblio.org/pub/linux/apps/convert, dated 1997.08.19.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "pm_c_util.h"
#include "nstring.h"
#include "pbm.h"


static int bitbox;
static int bitsleft;


static void 
bit_init(void) {
    bitbox=0; 
    bitsleft=0;
}



static int 
bit_input(FILE * const in) {
    if (bitsleft == 0)   {
        bitbox = fgetc(in);
        bitsleft = 8;
    }
    --bitsleft;
    return((bitbox&(1<<bitsleft))?1:0);
}



static void 
doSquare(FILE *          const in,
          unsigned char * const image,
          int             const ox,
          int             const oy,
          int             const w,
          int             const size) {

    if (size == 1 || bit_input(in)) { 
        /* It's all black or all white.  Next bit says which. */

        unsigned int const c = bit_input(in);

        unsigned int y;

        for (y = 0; y < size; ++y) {
            unsigned int x;
            for (x = 0; x < size; ++x)
                image[(oy+y)*w+ox+x] = c;
        }
    } else {
        /* not all one color, so recurse. */

        int halfsize = size >> 1;

        doSquare(in, image, ox,          oy,          w, halfsize);
        doSquare(in, image, ox+halfsize, oy,          w, halfsize);
        doSquare(in, image, ox,          oy+halfsize, w, halfsize);
        doSquare(in, image, ox+halfsize, oy+halfsize, w, halfsize);
    }
}



static void
writeOutput(FILE *                const ofP,
            int                   const cols,
            int                   const rows,
            const unsigned char * const image) {
            
    /* w64 is units-of-64-bits width, h64 same for height */
    unsigned int const w64 = (cols+63)/64;

    bit * bitrow;
    unsigned int row;

    pbm_writepbminit(ofP, cols, rows, FALSE);

    bitrow = pbm_allocrow(cols);

    for (row = 0; row < rows; ++row) {
        unsigned int col;
     
        for (col = 0; col < cols; ++col)
            bitrow[col] = 
                (image[row * (w64*64) + col] == 1) ? PBM_WHITE : PBM_BLACK;

        pbm_writepbmrow(ofP, bitrow, cols, FALSE);
    }
    pbm_freerow(bitrow);
}



static void
readMrfImage(FILE *           const ifP,
             bool             const expandAll,
             unsigned char ** const imageP,
             unsigned int *   const colsP,
             unsigned int *   const rowsP) {

    static unsigned char buf[128];
    unsigned int rows;
    unsigned int cols;
    unsigned int w64, h64;

    unsigned char * image;

    fread(buf, 1, 13, ifP);

    if (memcmp(buf, "MRF1", 4) != 0)
        pm_error("Input is not an mrf image.  "
                 "We know this because it does not start with 'MRF1'.");

    if (buf[12] != 0)
        pm_error("can't handle file subtype %u", buf[12]);

    cols = (buf[4] << 24) | (buf[5] << 16) | (buf[06] << 8) | buf[07] << 0;
    rows = (buf[8] << 24) | (buf[9] << 16) | (buf[10] << 8) | buf[11] << 0;

    /* w64 is units-of-64-bits width, h64 same for height */
    w64 = (cols+63)/64;
    h64 = (rows+63)/64;
    if (expandAll) {
        *colsP = w64*64;
        *rowsP = h64*64;
    } else {
        *colsP = cols;
        *rowsP = rows;
    }

    if (UINT_MAX/w64/64/h64/64 == 0)
        pm_error("Ridiculously large, unprocessable image: %u cols x %u rows",
                 cols, rows);

    image = calloc(w64*h64*64*64,1);
    if (image == NULL)
        pm_error("Unable to get memory for raster");
                 
    /* now recursively input squares. */

    bit_init();

    {
        unsigned int row;
        for (row = 0; row < h64; ++row) {
            unsigned int col;
            for (col = 0; col < w64; ++col)
                doSquare(ifP, image, col*64, row*64, w64*64, 64);
        }
    }
    *imageP = image;
}



int 
main(int argc, char *argv[]) {

    FILE *ifP;
    FILE *ofP;
    unsigned char *image;
    bool expandAll;
    unsigned int cols, rows;

    pbm_init(&argc, argv);

    expandAll = FALSE;  /* initial assumption */

    if (argc-1 >= 1 && STREQ(argv[1], "-a")) {
        expandAll = TRUE;
        argc--,argv++;
    }

    if (argc-1 > 1)
        pm_error("Too many arguments: %d.  Only argument is input file",
                 argc-1);

    if (argc-1 == 1) 
        ifP = pm_openr(argv[1]);
    else
        ifP = stdin;

    ofP = stdout;

    readMrfImage(ifP, expandAll, &image, &cols, &rows);
    
    pm_close(ifP);
    
    writeOutput(ofP, cols, rows, image);

    free(image);

    return 0;
}






