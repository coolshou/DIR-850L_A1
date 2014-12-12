/* pnmnlfilt.c - 4 in 1 (2 non-linear) filter
**             - smooth an anyimage
**             - do alpha trimmed mean filtering on an anyimage
**             - do optimal estimation smoothing on an anyimage
**             - do edge enhancement on an anyimage
**
** Version 1.0
**
** The implementation of an alpha-trimmed mean filter
** is based on the description in IEEE CG&A May 1990
** Page 23 by Mark E. Lee and Richard A. Redner.
**
** The paper recommends using a hexagon sampling region around each
** pixel being processed, allowing an effective sub pixel radius to be
** specified. The hexagon values are sythesized by area sampling the
** rectangular pixels with a hexagon grid. The seven hexagon values
** obtained from the 3x3 pixel grid are used to compute the alpha
** trimmed mean. Note that an alpha value of 0.0 gives a conventional
** mean filter (where the radius controls the contribution of
** surrounding pixels), while a value of 0.5 gives a median filter.
** Although there are only seven values to trim from before finding
** the mean, the algorithm has been extended from that described in
** CG&A by using interpolation, to allow a continuous selection of
** alpha value between and including 0.0 to 0.5  The useful values
** for radius are between 0.3333333 (where the filter will have no
** effect because only one pixel is sampled), to 1.0, where all
** pixels in the 3x3 grid are sampled.
**
** The optimal estimation filter is taken from an article "Converting Dithered
** Images Back to Gray Scale" by Allen Stenger, Dr Dobb's Journal, November
** 1992, and this article references "Digital Image Enhancement andNoise Filtering by
** Use of Local Statistics", Jong-Sen Lee, IEEE Transactions on Pattern Analysis and
** Machine Intelligence, March 1980.
**
** Also borrow the  technique used in pgmenhance(1) to allow edge
** enhancement if the alpha value is negative.
**
** Author:
**         Graeme W. Gill, 30th Jan 1993
**         graeme@labtam.oz.au
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <math.h>

#include "pm_c_util.h"
#include "pnm.h"

/* MXIVAL is the maximum input sample value we can handle.
   It is limited by our willingness to allocate storage in various arrays
   that are indexed by sample values.

   We use PPM_MAXMAXVAL because that used to be the maximum possible
   sample value in the format, and most images still limit themselves to
   this value.
*/

#define MXIVAL PPM_MAXMAXVAL   

xelval omaxval; 
    /* global so that pixel processing code can get at it quickly */
int noisevariance;      
    /* global so that pixel processing code can get at it quickly */

/*
 * Declared static here rather than passing a jillion options in the call to
 * do_one_frame().   Also it makes a huge amount of sense to only malloc the
 * row buffers once instead of for each frame (with the corresponding free'ing
 * of course).
*/
static  xel *irows[3];
static  xel *irow0, *irow1, *irow2, *orow;
static  double radius=0.0,alpha= -1.0;
static  int rows, cols, format, oformat, row, col;
static  int (*atfunc)(int *);
static  xelval maxval;

#define NOIVAL (MXIVAL + 1)             /* number of possible input values */

#define SCALEB 8                                /* scale bits */
#define SCALE (1 << SCALEB)     /* scale factor */
#define MXSVAL (MXIVAL * SCALE) /* maximum scaled values */

#define CSCALEB 2                               /* coarse scale bits */
#define CSCALE (1 << CSCALEB)   /* coarse scale factor */
#define MXCSVAL (MXIVAL * CSCALE)       /* maximum coarse scaled values */
#define NOCSVAL (MXCSVAL + 1)   /* number of coarse scaled values */
#define SCTOCSC(x) ((x) >> (SCALEB - CSCALEB))  /*  scaled to coarse scaled */
#define CSCTOSC(x) ((x) << (SCALEB - CSCALEB))  /*  course scaled to scaled */

#ifndef MAXINT
# define MAXINT 0x7fffffff      /* assume this is a 32 bit machine */
#endif

/* round and scale floating point to scaled integer */
#define ROUNDSCALE(x) ((int)(((x) * (double)SCALE) + 0.5))
/* round and un-scale scaled integer value */
#define RUNSCALE(x) (((x) + (1 << (SCALEB-1))) >> SCALEB) 
/* rounded un-scale */
#define UNSCALE(x) ((x) >> SCALEB)

static double
sqr(double const arg) {
    return arg * arg;
}



/* We restrict radius to the values: 0.333333 <= radius <= 1.0 */
/* so that no fewer and no more than a 3x3 grid of pixels around */
/* the pixel in question needs to be read. Given this, we only */
/* need 3 or 4 weightings per hexagon, as follows: */
/*                  _ _                         */
/* Vertical hex:   |_|_|  1 2                   */
/*                 |X|_|  0 3                   */
/*                                       _      */
/*              _                      _|_|   1 */
/* Middle hex: |_| 1  Horizontal hex: |X|_| 0 2 */
/*             |X| 0                    |_|   3 */
/*             |_| 2                            */

/* all filters */
int V0[NOIVAL],V1[NOIVAL],V2[NOIVAL],V3[NOIVAL];        /* vertical hex */
int M0[NOIVAL],M1[NOIVAL],M2[NOIVAL];                   /* middle hex */
int H0[NOIVAL],H1[NOIVAL],H2[NOIVAL],H3[NOIVAL];        /* horizontal hex */

/* alpha trimmed and edge enhancement only */
int ALFRAC[NOIVAL * 8];                 /* fractional alpha divider table */

/* optimal estimation only */
int AVEDIV[7 * NOCSVAL];                /* divide by 7 to give average value */
int SQUARE[2 * NOCSVAL];                /* scaled square lookup table */

/* ************************************************** *
   Hexagon intersecting square area functions 
   Compute the area of the intersection of a triangle 
   and a rectangle 
   ************************************************** */

/* Triangle orientation is per geometric axes (not graphical axies) */

#define NW 0    /* North west triangle /| */
#define NE 1    /* North east triangle |\ */
#define SW 2    /* South west triangle \| */
#define SE 3    /* South east triangle |/ */
#define STH 2
#define EST 1

#define SWAPI(a,b) (t = a, a = -b, b = -t)

static double 
triang_area(double rx0, double ry0, double rx1, double ry1,
            double tx0, double ty0, double tx1, double ty1,
            int tt) {
/* rx0,ry0,rx1,ry1:       rectangle boundaries */
/* tx0,ty0,tx1,ty1:       triangle boundaries */
/* tt:                    triangle type */

    double a,b,c,d;
    double lx0,ly0,lx1,ly1;

    /* Convert everything to a NW triangle */
    if (tt & STH) {
        double t;
        SWAPI(ry0,ry1);
        SWAPI(ty0,ty1);
    }
    if (tt & EST) {
        double t;
        SWAPI(rx0,rx1);
        SWAPI(tx0,tx1);
    }
    /* Compute overlapping box */
    if (tx0 > rx0)
        rx0 = tx0;
    if (ty0 > ry0)
        ry0 = ty0;
    if (tx1 < rx1)
        rx1 = tx1;
    if (ty1 < ry1)
        ry1 = ty1;
    if (rx1 <= rx0 || ry1 <= ry0)
        return 0.0;

    /* Need to compute diagonal line intersection with the box */
    /* First compute co-efficients to formulas x = a + by and y = c + dx */
    b = (tx1 - tx0)/(ty1 - ty0);
    a = tx0 - b * ty0;
    d = (ty1 - ty0)/(tx1 - tx0);
    c = ty0 - d * tx0;
    
    /* compute top or right intersection */
    tt = 0;
    ly1 = ry1;
    lx1 = a + b * ly1;
    if (lx1 <= rx0)
        return (rx1 - rx0) * (ry1 - ry0);
    else if (lx1 > rx1) {
        /* could be right hand side */
        lx1 = rx1;
        ly1 = c + d * lx1;
        if (ly1 <= ry0)
            return (rx1 - rx0) * (ry1 - ry0);
        tt = 1; /* right hand side intersection */
    }
    /* compute left or bottom intersection */
    lx0 = rx0;
    ly0 = c + d * lx0;
    if (ly0 >= ry1)
        return (rx1 - rx0) * (ry1 - ry0);
    else if (ly0 < ry0) {
        /* could be right hand side */
        ly0 = ry0;
        lx0 = a + b * ly0;
        if (lx0 >= rx1)
            return (rx1 - rx0) * (ry1 - ry0);
        tt |= 2;        /* bottom intersection */
    }
    
    if (tt == 0) {
        /* top and left intersection */
        /* rectangle minus triangle */
        return ((rx1 - rx0) * (ry1 - ry0))
            - (0.5 * (lx1 - rx0) * (ry1 - ly0));
    } else if (tt == 1) {
        /* right and left intersection */
        return ((rx1 - rx0) * (ly0 - ry0))
            + (0.5 * (rx1 - rx0) * (ly1 - ly0));
    } else if (tt == 2) {
        /* top and bottom intersection */
        return ((rx1 - lx1) * (ry1 - ry0))
            + (0.5 * (lx1 - lx0) * (ry1 - ry0));
    } else {
        /* tt == 3 */ 
        /* right and bottom intersection */
        /* triangle */
        return (0.5 * (rx1 - lx0) * (ly1 - ry0));
    }
}



static double
rectang_area(double rx0, double ry0, double rx1, double ry1, 
             double tx0, double ty0, double tx1, double ty1) {
/* Compute rectangle area */
/* rx0,ry0,rx1,ry1:  rectangle boundaries */
/* tx0,ty0,tx1,ty1:  rectangle boundaries */

    /* Compute overlapping box */
    if (tx0 > rx0)
        rx0 = tx0;
    if (ty0 > ry0)
        ry0 = ty0;
    if (tx1 < rx1)
        rx1 = tx1;
    if (ty1 < ry1)
        ry1 = ty1;
    if (rx1 <= rx0 || ry1 <= ry0)
        return 0.0;
    return (rx1 - rx0) * (ry1 - ry0);
}




static double 
hex_area(double sx, double sy, double hx, double hy, double d) {
/* compute the area of overlap of a hexagon diameter d, */
/* centered at hx,hy, with a unit square of center sx,sy. */
/* sx,sy:    square center */
/* hx,hy,d:  hexagon center and diameter */

    double hx0,hx1,hx2,hy0,hy1,hy2,hy3;
    double sx0,sx1,sy0,sy1;

    /* compute square co-ordinates */
    sx0 = sx - 0.5;
    sy0 = sy - 0.5;
    sx1 = sx + 0.5;
    sy1 = sy + 0.5;

    /* compute hexagon co-ordinates */
    hx0 = hx - d/2.0;
    hx1 = hx;
    hx2 = hx + d/2.0;
    hy0 = hy - 0.5773502692 * d;    /* d / sqrt(3) */
    hy1 = hy - 0.2886751346 * d;    /* d / sqrt(12) */
    hy2 = hy + 0.2886751346 * d;    /* d / sqrt(12) */
    hy3 = hy + 0.5773502692 * d;    /* d / sqrt(3) */

    return
        triang_area(sx0,sy0,sx1,sy1,hx0,hy2,hx1,hy3,NW) +
        triang_area(sx0,sy0,sx1,sy1,hx1,hy2,hx2,hy3,NE) +
        rectang_area(sx0,sy0,sx1,sy1,hx0,hy1,hx2,hy2) +
        triang_area(sx0,sy0,sx1,sy1,hx0,hy0,hx1,hy1,SW) +
        triang_area(sx0,sy0,sx1,sy1,hx1,hy0,hx2,hy1,SE);
}




static void
setupAvediv(void) {

    unsigned int i;

    for (i=0; i < (7 * NOCSVAL); ++i) {
        /* divide scaled value by 7 lookup */
        AVEDIV[i] = CSCTOSC(i)/7;       /* scaled divide by 7 */
    }

}




static void
setupSquare(void) {

    unsigned int i;

    for (i=0; i < (2 * NOCSVAL); ++i) {
        /* compute square and rescale by (val >> (2 * SCALEB + 2)) table */
        int const val = CSCTOSC(i - NOCSVAL); 
        /* NOCSVAL offset to cope with -ve input values */
        SQUARE[i] = (val * val) >> (2 * SCALEB + 2);
    }
}




static void
setup1(double   const alpha,
       double   const radius,
       double   const maxscale,
       int *    const alpharangeP,
       double * const meanscaleP,
       double * const mmeanscaleP,
       double * const alphafractionP,
       int *    const noisevarianceP) {


    setupAvediv();
    setupSquare();

    if (alpha >= 0.0 && alpha <= 0.5) {
        /* alpha trimmed mean */
        double const noinmean =  ((0.5 - alpha) * 12.0) + 1.0;
            /* number of elements (out of a possible 7) used in the mean */

        *mmeanscaleP = *meanscaleP = maxscale/noinmean;
        if (alpha == 0.0) {
            /* mean filter */ 
            *alpharangeP = 0;
            *alphafractionP = 0.0;            /* not used */
        } else if (alpha < (1.0/6.0)) {
            /* mean of 5 to 7 middle values */
            *alpharangeP = 1;
            *alphafractionP = (7.0 - noinmean)/2.0;
        } else if (alpha < (1.0/3.0)) {
            /* mean of 3 to 5 middle values */
            *alpharangeP = 2;
            *alphafractionP = (5.0 - noinmean)/2.0;
        } else {
            /* mean of 1 to 3 middle values */
            /* alpha == 0.5 == median filter */
            *alpharangeP = 3;
            *alphafractionP = (3.0 - noinmean)/2.0;
        }
    } else if (alpha >= 1.0 && alpha <= 2.0) {
        /* optimal estimation - alpha controls noise variance threshold. */
        double const alphaNormalized = alpha - 1.0;
            /* normalize it to 0.0 -> 1.0 */
        double const noinmean = 7.0;
        *alpharangeP = 5;                 /* edge enhancement function */
        *mmeanscaleP = *meanscaleP = maxscale;  /* compute scaled hex values */
        *alphafractionP = 1.0/noinmean;   
            /* Set up 1:1 division lookup - not used */
        *noisevarianceP = sqr(alphaNormalized * omaxval) / 8.0;    
            /* estimate of noise variance */
    } else if (alpha >= -0.9 && alpha <= -0.1) {
        /* edge enhancement function */
        double const posAlpha = -alpha;
            /* positive alpha value */
        *alpharangeP = 4;                 /* edge enhancement function */
        *meanscaleP = maxscale * (-posAlpha/((1.0 - posAlpha) * 7.0));
            /* mean of 7 and scaled by -posAlpha/(1-posAlpha) */
        *mmeanscaleP = maxscale * (1.0/(1.0 - posAlpha) + *meanscaleP);    
            /* middle pixel has 1/(1-posAlpha) as well */
        *alphafractionP = 0.0;    /* not used */
    } else {
        /* An entry condition on 'alpha' makes this impossible */
        pm_error("INTERNAL ERROR: impossible alpha value: %f", alpha);
    }
}




static void
setupAlfrac(double const alphafraction) {
    /* set up alpha fraction lookup table used on big/small */

    unsigned int i;

    for (i=0; i < (NOIVAL * 8); ++i) {
        ALFRAC[i] = ROUNDSCALE(i * alphafraction);
    }
}




static void
setupPixelWeightingTables(double const radius,
                          double const meanscale,
                          double const mmeanscale) {

    /* Setup pixel weighting tables - note we pre-compute mean
       division here too. 
    */
    double const hexhoff = radius/2;      
        /* horizontal offset of vertical hex centers */
    double const hexvoff = 3.0 * radius/sqrt(12.0); 
        /* vertical offset of vertical hex centers */

    double const tabscale  = meanscale  / (radius * hexvoff);
    double const mtabscale = mmeanscale / (radius * hexvoff);

    /* scale tables to normalize by hexagon area, and number of
       hexes used in mean 
    */
    double const v0 =
        hex_area(0.0,  0.0, hexhoff, hexvoff, radius) * tabscale;
    double const v1 =
        hex_area(0.0,  1.0, hexhoff, hexvoff, radius) * tabscale;
    double const v2 =
        hex_area(1.0,  1.0, hexhoff, hexvoff, radius) * tabscale;
    double const v3 =
        hex_area(1.0,  0.0, hexhoff, hexvoff, radius) * tabscale;
    double const m0 =
        hex_area(0.0,  0.0, 0.0,     0.0,     radius) * mtabscale;
    double const m1 =
        hex_area(0.0,  1.0, 0.0,     0.0,     radius) * mtabscale;
    double const m2 =
        hex_area(0.0, -1.0, 0.0,     0.0,     radius) * mtabscale;
    double const h0 =
        hex_area(0.0,  0.0, radius,  0.0,     radius) * tabscale;
    double const h1 =
        hex_area(1.0,  1.0, radius,  0.0,     radius) * tabscale;
    double const h2 =
        hex_area(1.0,  0.0, radius,  0.0,     radius) * tabscale;
    double const h3 =
        hex_area(1.0, -1.0, radius,  0.0,     radius) * tabscale;

    unsigned int i;

    for (i=0; i <= MXIVAL; ++i) {
        V0[i] = ROUNDSCALE(i * v0);
        V1[i] = ROUNDSCALE(i * v1);
        V2[i] = ROUNDSCALE(i * v2);
        V3[i] = ROUNDSCALE(i * v3);
        M0[i] = ROUNDSCALE(i * m0);
        M1[i] = ROUNDSCALE(i * m1);
        M2[i] = ROUNDSCALE(i * m2);
        H0[i] = ROUNDSCALE(i * h0);
        H1[i] = ROUNDSCALE(i * h1);
        H2[i] = ROUNDSCALE(i * h2);
        H3[i] = ROUNDSCALE(i * h3);
    }
}




/* Table initialization function - return alpha range */
static int 
atfilt_setup(double const alpha,
             double const radius,
             double const maxscale) {

    int alpharange;                 /* alpha range value 0 - 5 */
    double meanscale;               /* scale for finding mean */
    double mmeanscale;              /* scale for finding mean - midle hex */
    double alphafraction;   
        /* fraction of next largest/smallest to subtract from sum */

    setup1(alpha, radius, maxscale,
           &alpharange, &meanscale, &mmeanscale, &alphafraction,
           &noisevariance);

    setupAlfrac(alphafraction);

    setupPixelWeightingTables(radius, meanscale, mmeanscale);

    return alpharange;
}



static int 
atfilt0(int * p) {
/* Core pixel processing function - hand it 3x3 pixels and return result. */
/* Mean filter */
    /* 'p' is 9 pixel values from 3x3 neighbors */
    int retv;
    /* map to scaled hexagon values */
    retv = M0[p[0]] + M1[p[3]] + M2[p[7]];
    retv += H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
    retv += V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
    retv += V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
    retv += H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
    retv += V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
    retv += V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
    return UNSCALE(retv);
}

#define CHECK(xx) {\
        h0 += xx; \
        if (xx > big) \
            big = xx; \
        else if (xx < small) \
            small = xx; }

static int 
atfilt1(int * p) {
/* Mean of 5 - 7 middle values */
/* 'p' is 9 pixel values from 3x3 neighbors */

    int h0,h1,h2,h3,h4,h5,h6;       /* hexagon values    2 3   */
                                    /*                  1 0 4  */
                                    /*                   6 5   */
    int big,small;
    /* map to scaled hexagon values */
    h0 = M0[p[0]] + M1[p[3]] + M2[p[7]];
    h1 = H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
    h2 = V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
    h3 = V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
    h4 = H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
    h5 = V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
    h6 = V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
    /* sum values and also discover the largest and smallest */
    big = small = h0;
    CHECK(h1);
    CHECK(h2);
    CHECK(h3);
    CHECK(h4);
    CHECK(h5);
    CHECK(h6);
    /* Compute mean of middle 5-7 values */
    return UNSCALE(h0 -ALFRAC[(big + small)>>SCALEB]);
}
#undef CHECK

#define CHECK(xx) {\
        h0 += xx; \
        if (xx > big1) {\
            if (xx > big0) {\
                big1 = big0; \
                big0 = xx; \
            } else \
                big1 = xx; \
        } \
        if (xx < small1) {\
            if (xx < small0) {\
                small1 = small0; \
                small0 = xx; \
                } else \
                    small1 = xx; \
        }\
    }


static int 
atfilt2(int *p) {
/* Mean of 3 - 5 middle values */
/* 'p' is 9 pixel values from 3x3 neighbors */
    int h0,h1,h2,h3,h4,h5,h6;       /* hexagon values    2 3   */
                                    /*                  1 0 4  */
                                    /*                   6 5   */
    int big0,big1,small0,small1;
    /* map to scaled hexagon values */
    h0 = M0[p[0]] + M1[p[3]] + M2[p[7]];
    h1 = H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
    h2 = V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
    h3 = V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
    h4 = H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
    h5 = V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
    h6 = V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
    /* sum values and also discover the 2 largest and 2 smallest */
    big0 = small0 = h0;
    small1 = MAXINT;
    big1 = 0;
    CHECK(h1);
    CHECK(h2);
    CHECK(h3);
    CHECK(h4);
    CHECK(h5);
    CHECK(h6);
    /* Compute mean of middle 3-5 values */
    return UNSCALE(h0 -big0 -small0 -ALFRAC[(big1 + small1)>>SCALEB]);
}

#undef CHECK

#define CHECK(xx) {\
        h0 += xx; \
        if (xx > big2) \
                { \
                if (xx > big1) \
                        { \
                        if (xx > big0) \
                                { \
                                big2 = big1; \
                                big1 = big0; \
                                big0 = xx; \
                                } \
                        else \
                                { \
                                big2 = big1; \
                                big1 = xx; \
                                } \
                        } \
                else \
                        big2 = xx; \
                } \
        if (xx < small2) \
                { \
                if (xx < small1) \
                        { \
                        if (xx < small0) \
                                { \
                                small2 = small1; \
                                small1 = small0; \
                                small0 = xx; \
                                } \
                        else \
                                { \
                                small2 = small1; \
                                small1 = xx; \
                                } \
                        } \
                else \
                        small2 = xx; \
                                         }}

static int 
atfilt3(int *p) {
/* Mean of 1 - 3 middle values. If only 1 value, then this is a median
   filter. 
*/
/* 'p' is pixel values from 3x3 neighbors */
    int h0,h1,h2,h3,h4,h5,h6;       /* hexagon values    2 3   */
                                    /*                  1 0 4  */
                                    /*                   6 5   */
    int big0,big1,big2,small0,small1,small2;
    /* map to scaled hexagon values */
    h0 = M0[p[0]] + M1[p[3]] + M2[p[7]];
    h1 = H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
    h2 = V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
    h3 = V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
    h4 = H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
    h5 = V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
    h6 = V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
    /* sum values and also discover the 3 largest and 3 smallest */
    big0 = small0 = h0;
    small1 = small2 = MAXINT;
    big1 = big2 = 0;
    CHECK(h1);
    CHECK(h2);
    CHECK(h3);
    CHECK(h4);
    CHECK(h5);
    CHECK(h6);
    /* Compute mean of middle 1-3 values */
    return  UNSCALE(h0 -big0 -big1 -small0 -small1 
                    -ALFRAC[(big2 + small2)>>SCALEB]);
}
#undef CHECK

static int 
atfilt4(int *p) {
/* Edge enhancement */
/* notice we use the global omaxval */
/* 'p' is 9 pixel values from 3x3 neighbors */

    int hav;
    /* map to scaled hexagon values and compute enhance value */
    hav = M0[p[0]] + M1[p[3]] + M2[p[7]];
    hav += H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
    hav += V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
    hav += V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
    hav += H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
    hav += V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
    hav += V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
    if (hav < 0)
        hav = 0;
    hav = UNSCALE(hav);
    if (hav > omaxval)
        hav = omaxval;
    return hav;
}

static int 
atfilt5(int *p) {
/* Optimal estimation - do smoothing in inverse proportion */
/* to the local variance. */
/* notice we use the globals noisevariance and omaxval*/
/* 'p' is 9 pixel values from 3x3 neighbors */

    int mean,variance,temp;
    int h0,h1,h2,h3,h4,h5,h6;       /* hexagon values    2 3   */
                                    /*                  1 0 4  */
                                    /*                   6 5   */
    /* map to scaled hexagon values */
    h0 = M0[p[0]] + M1[p[3]] + M2[p[7]];
    h1 = H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
    h2 = V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
    h3 = V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
    h4 = H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
    h5 = V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
    h6 = V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
    mean = h0 + h1 + h2 + h3 + h4 + h5 + h6;
    mean = AVEDIV[SCTOCSC(mean)];   /* compute scaled mean by dividing by 7 */
    temp = (h1 - mean); variance = SQUARE[NOCSVAL + SCTOCSC(temp)];  
        /* compute scaled variance */
    temp = (h2 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)]; 
        /* and rescale to keep */
    temp = (h3 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)]; 
        /* within 32 bit limits */
    temp = (h4 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)];
    temp = (h5 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)];
    temp = (h6 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)];
    temp = (h0 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)];   
    /* (temp = h0 - mean) */
    if (variance != 0)      /* avoid possible divide by 0 */
        temp = mean + (variance * temp) / (variance + noisevariance);   
            /* optimal estimate */
    else temp = h0;
    if (temp < 0)
        temp = 0;
    temp = RUNSCALE(temp);
    if (temp > omaxval)
        temp = omaxval;
    return temp;
}



static void 
do_one_frame(FILE *ifp) {

    pnm_writepnminit( stdout, cols, rows, omaxval, oformat, 0 );
    
    if ( PNM_FORMAT_TYPE(oformat) == PPM_TYPE ) {
        int pr[9],pg[9],pb[9];          /* 3x3 neighbor pixel values */
        int r,g,b;

        for ( row = 0; row < rows; row++ ) {
            int po,no;           /* offsets for left and right colums in 3x3 */
            xel *ip0, *ip1, *ip2, *op;

            if (row == 0) {
                irow0 = irow1;
                pnm_readpnmrow( ifp, irow1, cols, maxval, format );
            }
            if (row == (rows-1))
                irow2 = irow1;
            else
                pnm_readpnmrow( ifp, irow2, cols, maxval, format );

            for (col = cols-1,po= col>0?1:0,no=0,
                     ip0=irow0,ip1=irow1,ip2=irow2,op=orow;
                 col >= 0;
                 col--,ip0++,ip1++,ip2++,op++, no |= 1,po = col!= 0 ? po : 0) {
                                /* grab 3x3 pixel values */
                pr[0] = PPM_GETR( *ip1 );
                pg[0] = PPM_GETG( *ip1 );
                pb[0] = PPM_GETB( *ip1 );
                pr[1] = PPM_GETR( *(ip1-no) );
                pg[1] = PPM_GETG( *(ip1-no) );
                pb[1] = PPM_GETB( *(ip1-no) );
                pr[5] = PPM_GETR( *(ip1+po) );
                pg[5] = PPM_GETG( *(ip1+po) );
                pb[5] = PPM_GETB( *(ip1+po) );
                pr[3] = PPM_GETR( *(ip2) );
                pg[3] = PPM_GETG( *(ip2) );
                pb[3] = PPM_GETB( *(ip2) );
                pr[2] = PPM_GETR( *(ip2-no) );
                pg[2] = PPM_GETG( *(ip2-no) );
                pb[2] = PPM_GETB( *(ip2-no) );
                pr[4] = PPM_GETR( *(ip2+po) );
                pg[4] = PPM_GETG( *(ip2+po) );
                pb[4] = PPM_GETB( *(ip2+po) );
                pr[6] = PPM_GETR( *(ip0+po) );
                pg[6] = PPM_GETG( *(ip0+po) );
                pb[6] = PPM_GETB( *(ip0+po) );
                pr[8] = PPM_GETR( *(ip0-no) );
                pg[8] = PPM_GETG( *(ip0-no) );
                pb[8] = PPM_GETB( *(ip0-no) );
                pr[7] = PPM_GETR( *(ip0) );
                pg[7] = PPM_GETG( *(ip0) );
                pb[7] = PPM_GETB( *(ip0) );
                r = (*atfunc)(pr);
                g = (*atfunc)(pg);
                b = (*atfunc)(pb);
                PPM_ASSIGN( *op, r, g, b );
            }
            pnm_writepnmrow( stdout, orow, cols, omaxval, oformat, 0 );
            if (irow1 == irows[2]) {
                irow1 = irows[0];
                irow2 = irows[1];
                irow0 = irows[2];
            } else if (irow1 == irows[1]) {
                irow2 = irows[0];
                irow0 = irows[1];
                irow1 = irows[2];
            }
            else    /* must be at irows[0] */
            {
                irow0 = irows[0];
                irow1 = irows[1];
                irow2 = irows[2];
            }
        }
    } else {
        /* Else must be PGM */
        int p[9];               /* 3x3 neighbor pixel values */
        int pv;
        int promote;

        /* we scale maxval to omaxval */
        promote = ( PNM_FORMAT_TYPE(format) != PNM_FORMAT_TYPE(oformat) );

        for ( row = 0; row < rows; row++ ) {
            int po,no;          /* offsets for left and right colums in 3x3 */
            xel *ip0, *ip1, *ip2, *op;

            if (row == 0) {
                irow0 = irow1;
                pnm_readpnmrow( ifp, irow1, cols, maxval, format );
                if ( promote )
                    pnm_promoteformatrow( irow1, cols, maxval, 
                                          format, maxval, oformat );
            }
            if (row == (rows-1))
                irow2 = irow1;
            else {
                pnm_readpnmrow( ifp, irow2, cols, maxval, format );
                if ( promote )
                    pnm_promoteformatrow( irow2, cols, maxval, 
                                          format, maxval, oformat );
            }

            for (col = cols-1,po= col>0?1:0,no=0,
                     ip0=irow0,ip1=irow1,ip2=irow2,op=orow;
                 col >= 0;
                 col--,ip0++,ip1++,ip2++,op++, no |= 1,po = col!= 0 ? po : 0) {
                /* grab 3x3 pixel values */
                p[0] = PNM_GET1( *ip1 );
                p[1] = PNM_GET1( *(ip1-no) );
                p[5] = PNM_GET1( *(ip1+po) );
                p[3] = PNM_GET1( *(ip2) );
                p[2] = PNM_GET1( *(ip2-no) );
                p[4] = PNM_GET1( *(ip2+po) );
                p[6] = PNM_GET1( *(ip0+po) );
                p[8] = PNM_GET1( *(ip0-no) );
                p[7] = PNM_GET1( *(ip0) );
                pv = (*atfunc)(p);
                PNM_ASSIGN1( *op, pv );
            }
            pnm_writepnmrow( stdout, orow, cols, omaxval, oformat, 0 );
            if (irow1 == irows[2]) {
                irow1 = irows[0];
                irow2 = irows[1];
                irow0 = irows[2];
            } else if (irow1 == irows[1]) {
                irow2 = irows[0];
                irow0 = irows[1];
                irow1 = irows[2];
            } else {
                /* must be at irows[0] */
                irow0 = irows[0];
                irow1 = irows[1];
                irow2 = irows[2];
            }
        }
    }
}



static void
verifySame(unsigned int const imageSeq, 
           int const imageCols, int const imageRows,
           xelval const imageMaxval, int const imageFormat,
           int const cols, int const rows,
           xelval const maxval, int const format) {
/*----------------------------------------------------------------------------
   Issue error message and exit the program if the imageXXX arguments don't
   match the XXX arguments.
-----------------------------------------------------------------------------*/
    if (imageCols != cols)
        pm_error("Width of Image %u (%d) is not the same as Image 0 (%d)",
                 imageSeq, imageCols, cols);
    if (imageRows != rows)
        pm_error("Height of Image %u (%d) is not the same as Image 0 (%d)",
                 imageSeq, imageRows, rows);
    if (imageMaxval != maxval)
        pm_error("Maxval of Image %u (%u) is not the same as Image 0 (%u)",
                 imageSeq, imageMaxval, maxval);
    if (imageFormat != format)
        pm_error("Format of Image %u is not the same as Image 0",
                 imageSeq);
}



int (*atfuncs[6]) (int *) = {atfilt0,atfilt1,atfilt2,atfilt3,atfilt4,atfilt5};




int
main(int argc, char *argv[]) {

    FILE * ifp;
	bool eof;  /* We've hit the end of the input stream */
    unsigned int imageSeq;  /* Sequence number of image, starting from 0 */

    const char* const usage = "alpha radius pnmfile\n"
        "0.0 <= alpha <= 0.5 for alpha trimmed mean -or- \n"
        "1.0 <= alpha <= 2.0 for optimal estimation -or- \n"
        "-0.1 >= alpha >= -0.9 for edge enhancement\n"
        "0.3333 <= radius <= 1.0 specify effective radius\n";

    pnm_init( &argc, argv );

    if ( argc < 3 || argc > 4 )
        pm_usage( usage );

    if ( sscanf( argv[1], "%lf", &alpha ) != 1 )
        pm_usage( usage );
    if ( sscanf( argv[2], "%lf", &radius ) != 1 )
        pm_usage( usage );
        
    if ((alpha > -0.1 && alpha < 0.0) || (alpha > 0.5 && alpha < 1.0))
        pm_error( "Alpha must be in range 0.0 <= alpha <= 0.5 "
                  "for alpha trimmed mean" );
    if (alpha > 2.0)
        pm_error( "Alpha must be in range 1.0 <= alpha <= 2.0 "
                  "for optimal estimation" );
    if (alpha < -0.9 || (alpha > -0.1 && alpha < 0.0))
        pm_error( "Alpha must be in range -0.9 <= alpha <= -0.1 "
                  "for edge enhancement" );
    if (radius < 0.333 || radius > 1.0)
        pm_error( "Radius must be in range 0.333333333 <= radius <= 1.0" );

    if ( argc == 4 )
        ifp = pm_openr( argv[3] );
    else
        ifp = stdin;
        
    pnm_readpnminit( ifp, &cols, &rows, &maxval, &format );
        
    if (maxval > MXIVAL) 
        pm_error("The maxval of the input image (%d) is too large.\n"
                 "This program's limit is %d.", 
                 maxval, MXIVAL);
        
    oformat = PNM_FORMAT_TYPE(format);
    /* force output to max precision without forcing new 2-byte format */
    omaxval = MIN(maxval, PPM_MAXMAXVAL);
        
    atfunc = atfuncs[atfilt_setup(alpha, radius,
                                  (double)omaxval/(double)maxval)];

    if ( oformat < PGM_TYPE ) {
        oformat = RPGM_FORMAT;
        pm_message( "promoting file to PGM" );
    }

    orow = pnm_allocrow(cols);
    irows[0] = pnm_allocrow(cols);
    irows[1] = pnm_allocrow(cols);
    irows[2] = pnm_allocrow(cols);
    irow0 = irows[0];
    irow1 = irows[1];
    irow2 = irows[2];

    eof = FALSE;  /* We're already in the middle of the first image */
    imageSeq = 0;
    while (!eof) {
        do_one_frame(ifp);
        pm_nextimage(ifp, &eof);
        if (!eof) {
            /* Read and validate header of next image */
            int imageCols, imageRows;
            xelval imageMaxval;
            int imageFormat;

            ++imageSeq;
            pnm_readpnminit(ifp, &imageCols, &imageRows, 
                            &imageMaxval, &imageFormat);
            verifySame(imageSeq,
                       imageCols, imageRows, imageMaxval, imageFormat,
                       cols, rows, maxval, format);
        }
    }

    pnm_freerow(irow0);
    pnm_freerow(irow1);
    pnm_freerow(irow2);
    pnm_freerow(orow);
    pm_close(ifp);

    return 0;
}


