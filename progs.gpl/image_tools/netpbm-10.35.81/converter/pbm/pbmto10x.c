/* pbmto10x.c - read a portable bitmap and produce a Gemini 10X printer file
**
** Copyright (C) 1990, 1994 by Ken Yap
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Modified to shorten stripes and eliminate blank stripes. Dec 1994.
*/

#include "pbm.h"
#include "mallocvar.h"

#define LOW_RES_ROWS    8       /* printed per pass */
#define HIGH_RES_ROWS   16      /* printed per pass */

static int  highres = 0;
static FILE *ifp;
static int  rows, cols, format;



static void
outstripe(char * const stripe, 
          char * const sP, 
          int    const reschar) {

    int ncols;

    char * p;

    p = sP;  /* initial value */

    /* scan backwards, removing empty columns */
    while (p != stripe) 
        if (*--p != 0) {
            ++p;
            break;
        }
    ncols = p - stripe;
    if (ncols > 0) {
        printf("\033%c%c%c", reschar, ncols % 256, ncols / 256);
        fwrite(stripe, sizeof(char), ncols, stdout);
    }
    putchar('\n');          /* flush buffer */
}



static void
res_60x72(void) {
    int i, item, npins, row, col;
    bit *bitrows[LOW_RES_ROWS], *bP[LOW_RES_ROWS];
    char *stripe, *sP;

    MALLOCARRAY(stripe, cols);
    if (stripe == NULL)
        pm_error("Unable to allocate %u bytes for a stripe buffer.",
                 cols * sizeof(stripe[0]));
    for (i = 0; i < LOW_RES_ROWS; ++i)
        bitrows[i] = pbm_allocrow(cols);
    printf("\033A\010");        /* '\n' = 8/72 */
    for (row = 0, sP = stripe; row < rows; row += LOW_RES_ROWS, sP = stripe) {
        if (row + LOW_RES_ROWS <= rows)
            npins = LOW_RES_ROWS;
        else
            npins = rows - row;
        for (i = 0; i < npins; ++i)
            pbm_readpbmrow(ifp, bP[i] = bitrows[i], cols, format);
        for (col = 0; col < cols; ++col) {
            item = 0;
            for (i = 0; i < npins; ++i)
                if (*(bP[i]++) == PBM_BLACK)
                    item |= 1 << (7 - i);
            *sP++ = item;
        }
        outstripe(stripe, sP, 'K');
    }
    printf("\033@");
    free(stripe);
}



static void
res_120x144(void) {
    int i, pin, item, npins, row, col;
    bit *bitrows[HIGH_RES_ROWS], *bP[HIGH_RES_ROWS];
    char *stripe, *sP;

    MALLOCARRAY(stripe, cols);
    if (stripe == NULL)
        pm_error("Unable to allocate %u bytes for a stripe buffer.",
                 cols * sizeof(stripe[0]));
    for (i = 0; i < HIGH_RES_ROWS; ++i)
        bitrows[i] = pbm_allocrow(cols);
    printf("\0333\001");            /* \n = 1/144" */
    for (row = 0, sP = stripe; row < rows; row += HIGH_RES_ROWS, sP = stripe) {
        if (row + HIGH_RES_ROWS <= rows)
            npins = HIGH_RES_ROWS;
        else
            npins = rows - row;
        for (i = 0; i < npins; ++i)
            pbm_readpbmrow(ifp, bP[i] = bitrows[i], cols, format);
        for (col = 0; col < cols; ++col) {
            item = 0;
            /* even rows */
            for (pin = i = 0; i < npins; i += 2, ++pin)
                if (*(bP[i]++) == PBM_BLACK)
                    item |= 1 << (7 - pin);
            *sP++ = item;
        }
        outstripe(stripe, sP, 'L');
        sP = stripe;
        for (col = 0; col < cols; ++col) {
            item = 0;
            /* odd rows */
            for (i = 1, pin = 0; i < npins; i += 2, ++pin)
                if (*(bP[i]++) == PBM_BLACK)
                    item |= 1 << (7 - pin);
            *sP++ = item;
        }
        outstripe(stripe, sP, 'L');
        printf("\033J\016");        /* 14/144 down, \n did 1/144 */
    }
    printf("\033@");
    free(stripe);
}



int
main(int argc, char * argv[]) {

    const char * fname;

    pbm_init( &argc, argv );

    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'h') {
        highres = 1;
        --argc;
        ++argv;
    }
    if (argc-1 > 1)
        pm_error("Too many arguments.  Only argument is file name");
    else if (argc-1 == 1)
        fname = argv[1];
    else
        fname = "-";
    
    ifp = pm_openr(fname);

    pbm_readpbminit(ifp, &cols, &rows, &format);

    if (highres)
        res_120x144();
    else
        res_60x72();

    pm_close(ifp);
    exit(0);
}



