/* pbmtopgm.c - convert PBM to PGM by totalling pixels over sample area
 * AJCD 12/12/90
 */

#include <stdio.h>

#include "pm_c_util.h"
#include "pgm.h"

int
main(int argc, char *argv[]) {

    gray *outrow, maxval;
    int right, left, down, up;
    bit **inbits;
    int rows, cols;
    FILE *ifd;
    int row;
    int width, height;
    const char * const usage = "<w> <h> [pbmfile]";
   

    pgm_init( &argc, argv );

    if (argc > 4 || argc < 3)
        pm_usage(usage);

    width = atoi(argv[1]);
    height = atoi(argv[2]);
    if (width < 1 || height < 1)
        pm_error("width and height must be > 0");
    left=width/2; right=width-left;
    up=width/2; down=height-up;

    if (argc == 4)
        ifd = pm_openr(argv[3]);
    else
        ifd = stdin ;

    inbits = pbm_readpbm(ifd, &cols, &rows) ;
    
    if (width > cols)
        pm_error("You specified a sample width (%u columns) which is greater "
                 "than the image width (%u columns)", height, rows);
    if (height > rows)
        pm_error("You specified a sample height (%u rows) which is greater "
                 "than the image height (%u rows)", height, rows);

    outrow = pgm_allocrow(cols) ;
    maxval = MIN(PGM_OVERALLMAXVAL, width*height);
    pgm_writepgminit(stdout, cols, rows, maxval, 0) ;

    for (row = 0; row < rows; row++) {
        int const t = (row > up) ? (row-up) : 0;
        int const b = (row+down < rows) ? (row+down) : rows;
        int const onv = height - (t-row+up) - (row+down-b);
        unsigned int col;
        for (col = 0; col < cols; col++) {
            int const l = (col > left) ? (col-left) : 0;
            int const r = (col+right < cols) ? (col+right) : cols;
            int const onh = width - (l-col+left) - (col+right-r);
            int value;
            int x;

            value = 0;  /* initial value */

            for (x = l; x < r; ++x) {
                int y;
                for (y = t; y < b; ++y)
                    if (inbits[y][x] == PBM_WHITE) 
                        ++value;
            }
            outrow[col] = maxval*value/(onh*onv);
        }
        pgm_writepgmrow(stdout, outrow, cols, maxval, 0) ;
    }
    pm_close(ifd);

    return 0;
}
