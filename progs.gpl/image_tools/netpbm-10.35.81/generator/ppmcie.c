/*

      Generate a PPM file representing a CIE color gamut chart

               by John Walker  --  kelvin@@fourmilab.ch
               WWW home page: http://www.fourmilab.ch/

    Permission  to  use, copy, modify, and distribute this software and
    its documentation  for  any  purpose  and  without  fee  is  hereby
    granted,  without any conditions or restrictions.  This software is
    provided "as is" without express or implied warranty.

    This program was called cietoppm in Walker's original work.  
    Because "cie" is not a graphics format, Bryan changed the name
    when he integrated it into the Netpbm package in March 2000.
*/

/*
  Modified by
  Andrew J. S. Hamilton 21 May 1999
  Andrew.Hamilton@Colorado.EDU
  http://casa.colorado.edu/~ajsh/

  Corrected XYZ -> RGB transform.
  Introduced gamma correction.
  Introduced option to plot 1976 u' v' chromaticities.
*/

#include <math.h>

#include "pm_c_util.h"
#include "ppm.h"
#include "ppmdraw.h"
#include "nstring.h"

#define CLAMP(v, l, h)  ((v) < (l) ? (l) : (v) > (h) ? (h) : (v))
#define TRUE    1
#define FALSE   0

#define Maxval  255                   /* Maxval to use in generated pixmaps */

/* A  color  system is defined by the CIE x and y  coordinates of its
   three primary illuminants and the x and y coordinates of the  white
   point. */

struct colorSystem {
    const char *name;                       /* Color system name */
    double xRed, yRed,                /* Red primary illuminant */
           xGreen, yGreen,            /* Green primary illuminant */
           xBlue, yBlue,              /* Blue primary illuminant */
           xWhite, yWhite,            /* White point */
           gamma;             /* gamma of nonlinear correction */
};

/* The  following  table  gives  the  CIE  color  matching  functions
   \bar{x}(\lambda),  \bar{y}(\lambda),  and   \bar{z}(\lambda),   for
   wavelengths  \lambda  at 5 nanometre increments from 380 nm through
   780 nm.  This table is used in conjunction with  Planck's  law  for
   the  energy spectrum of a black body at a given temperature to plot
   the black body curve on the CIE chart. */

static double cie_color_match[][3] = {
    { 0.0014, 0.0000, 0.0065 },       /* 380 nm */
    { 0.0022, 0.0001, 0.0105 },
    { 0.0042, 0.0001, 0.0201 },
    { 0.0076, 0.0002, 0.0362 },
    { 0.0143, 0.0004, 0.0679 },
    { 0.0232, 0.0006, 0.1102 },
    { 0.0435, 0.0012, 0.2074 },
    { 0.0776, 0.0022, 0.3713 },
    { 0.1344, 0.0040, 0.6456 },
    { 0.2148, 0.0073, 1.0391 },
    { 0.2839, 0.0116, 1.3856 },
    { 0.3285, 0.0168, 1.6230 },
    { 0.3483, 0.0230, 1.7471 },
    { 0.3481, 0.0298, 1.7826 },
    { 0.3362, 0.0380, 1.7721 },
    { 0.3187, 0.0480, 1.7441 },
    { 0.2908, 0.0600, 1.6692 },
    { 0.2511, 0.0739, 1.5281 },
    { 0.1954, 0.0910, 1.2876 },
    { 0.1421, 0.1126, 1.0419 },
    { 0.0956, 0.1390, 0.8130 },
    { 0.0580, 0.1693, 0.6162 },
    { 0.0320, 0.2080, 0.4652 },
    { 0.0147, 0.2586, 0.3533 },
    { 0.0049, 0.3230, 0.2720 },
    { 0.0024, 0.4073, 0.2123 },
    { 0.0093, 0.5030, 0.1582 },
    { 0.0291, 0.6082, 0.1117 },
    { 0.0633, 0.7100, 0.0782 },
    { 0.1096, 0.7932, 0.0573 },
    { 0.1655, 0.8620, 0.0422 },
    { 0.2257, 0.9149, 0.0298 },
    { 0.2904, 0.9540, 0.0203 },
    { 0.3597, 0.9803, 0.0134 },
    { 0.4334, 0.9950, 0.0087 },
    { 0.5121, 1.0000, 0.0057 },
    { 0.5945, 0.9950, 0.0039 },
    { 0.6784, 0.9786, 0.0027 },
    { 0.7621, 0.9520, 0.0021 },
    { 0.8425, 0.9154, 0.0018 },
    { 0.9163, 0.8700, 0.0017 },
    { 0.9786, 0.8163, 0.0014 },
    { 1.0263, 0.7570, 0.0011 },
    { 1.0567, 0.6949, 0.0010 },
    { 1.0622, 0.6310, 0.0008 },
    { 1.0456, 0.5668, 0.0006 },
    { 1.0026, 0.5030, 0.0003 },
    { 0.9384, 0.4412, 0.0002 },
    { 0.8544, 0.3810, 0.0002 },
    { 0.7514, 0.3210, 0.0001 },
    { 0.6424, 0.2650, 0.0000 },
    { 0.5419, 0.2170, 0.0000 },
    { 0.4479, 0.1750, 0.0000 },
    { 0.3608, 0.1382, 0.0000 },
    { 0.2835, 0.1070, 0.0000 },
    { 0.2187, 0.0816, 0.0000 },
    { 0.1649, 0.0610, 0.0000 },
    { 0.1212, 0.0446, 0.0000 },
    { 0.0874, 0.0320, 0.0000 },
    { 0.0636, 0.0232, 0.0000 },
    { 0.0468, 0.0170, 0.0000 },
    { 0.0329, 0.0119, 0.0000 },
    { 0.0227, 0.0082, 0.0000 },
    { 0.0158, 0.0057, 0.0000 },
    { 0.0114, 0.0041, 0.0000 },
    { 0.0081, 0.0029, 0.0000 },
    { 0.0058, 0.0021, 0.0000 },
    { 0.0041, 0.0015, 0.0000 },
    { 0.0029, 0.0010, 0.0000 },
    { 0.0020, 0.0007, 0.0000 },
    { 0.0014, 0.0005, 0.0000 },
    { 0.0010, 0.0004, 0.0000 },
    { 0.0007, 0.0002, 0.0000 },
    { 0.0005, 0.0002, 0.0000 },
    { 0.0003, 0.0001, 0.0000 },
    { 0.0002, 0.0001, 0.0000 },
    { 0.0002, 0.0001, 0.0000 },
    { 0.0001, 0.0000, 0.0000 },
    { 0.0001, 0.0000, 0.0000 },
    { 0.0001, 0.0000, 0.0000 },
    { 0.0000, 0.0000, 0.0000 }        /* 780 nm */
};

/* The following table gives the  spectral  chromaticity  co-ordinates
   x(\lambda) and y(\lambda) for wavelengths in 5 nanometre increments
   from 380 nm through  780  nm.   These  co-ordinates  represent  the
   position in the CIE x-y space of pure spectral colors of the given
   wavelength, and  thus  define  the  outline  of  the  CIE  "tongue"
   diagram. */

static double spectral_chromaticity[81][3] = {
    { 0.1741, 0.0050 },               /* 380 nm */
    { 0.1740, 0.0050 },
    { 0.1738, 0.0049 },
    { 0.1736, 0.0049 },
    { 0.1733, 0.0048 },
    { 0.1730, 0.0048 },
    { 0.1726, 0.0048 },
    { 0.1721, 0.0048 },
    { 0.1714, 0.0051 },
    { 0.1703, 0.0058 },
    { 0.1689, 0.0069 },
    { 0.1669, 0.0086 },
    { 0.1644, 0.0109 },
    { 0.1611, 0.0138 },
    { 0.1566, 0.0177 },
    { 0.1510, 0.0227 },
    { 0.1440, 0.0297 },
    { 0.1355, 0.0399 },
    { 0.1241, 0.0578 },
    { 0.1096, 0.0868 },
    { 0.0913, 0.1327 },
    { 0.0687, 0.2007 },
    { 0.0454, 0.2950 },
    { 0.0235, 0.4127 },
    { 0.0082, 0.5384 },
    { 0.0039, 0.6548 },
    { 0.0139, 0.7502 },
    { 0.0389, 0.8120 },
    { 0.0743, 0.8338 },
    { 0.1142, 0.8262 },
    { 0.1547, 0.8059 },
    { 0.1929, 0.7816 },
    { 0.2296, 0.7543 },
    { 0.2658, 0.7243 },
    { 0.3016, 0.6923 },
    { 0.3373, 0.6589 },
    { 0.3731, 0.6245 },
    { 0.4087, 0.5896 },
    { 0.4441, 0.5547 },
    { 0.4788, 0.5202 },
    { 0.5125, 0.4866 },
    { 0.5448, 0.4544 },
    { 0.5752, 0.4242 },
    { 0.6029, 0.3965 },
    { 0.6270, 0.3725 },
    { 0.6482, 0.3514 },
    { 0.6658, 0.3340 },
    { 0.6801, 0.3197 },
    { 0.6915, 0.3083 },
    { 0.7006, 0.2993 },
    { 0.7079, 0.2920 },
    { 0.7140, 0.2859 },
    { 0.7190, 0.2809 },
    { 0.7230, 0.2770 },
    { 0.7260, 0.2740 },
    { 0.7283, 0.2717 },
    { 0.7300, 0.2700 },
    { 0.7311, 0.2689 },
    { 0.7320, 0.2680 },
    { 0.7327, 0.2673 },
    { 0.7334, 0.2666 },
    { 0.7340, 0.2660 },
    { 0.7344, 0.2656 },
    { 0.7346, 0.2654 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 },
    { 0.7347, 0.2653 }                /* 780 nm */
};

static pixel **pixels;                /* Pixel map */
static int pixcols, pixrows;          /* Pixel map size */
static int sxsize = 512, sysize = 512; /* X, Y size */

/* Standard white point chromaticities. */

#define IlluminantC     0.3101, 0.3162  /* For NTSC television */
#define IlluminantD65   0.3127, 0.3291  /* For EBU and SMPTE */

/* Gamma of nonlinear correction.
   See Charles Poynton's ColorFAQ Item 45 and GammaFAQ Item 6 at
   http://www.inforamp.net/~poynton/ColorFAQ.html
   http://www.inforamp.net/~poynton/GammaFAQ.html
*/

#define GAMMA_REC709    0.      /* Rec. 709 */

static struct colorSystem const
    NTSCsystem = {
        "NTSC",
        0.67,  0.33,  0.21,  0.71,  0.14,  0.08,
        IlluminantC,   GAMMA_REC709
    },
    EBUsystem = {
        "EBU (PAL/SECAM)",
        0.64,  0.33,  0.29,  0.60,  0.15,  0.06,
        IlluminantD65, GAMMA_REC709
    },
    SMPTEsystem = {
        "SMPTE",
        0.630, 0.340, 0.310, 0.595, 0.155, 0.070,
        IlluminantD65, GAMMA_REC709
    },
    HDTVsystem = {
        "HDTV",
        0.670, 0.330, 0.210, 0.710, 0.150, 0.060,
        IlluminantD65,  GAMMA_REC709
    },
    /* Huh? -ajsh
    CIEsystem = {
        "CIE",
        0.7355, 0.2645, 0.2658, 0.7243, 0.1669, 0.0085,
        0.3894, 0.3324, GAMMA_REC709
        },
    */
    CIEsystem = {
        "CIE",
        0.7355,0.2645,0.2658,0.7243,0.1669,0.0085, 0.33333333, 0.33333333,
        GAMMA_REC709
    },
    Rec709system = {
        "CIE REC 709",
        0.64,  0.33,  0.30,  0.60,  0.15,  0.06,
        IlluminantD65, GAMMA_REC709
    };

/* Customsystem  is a variable that is initialized to CIE Rec 709, but
   we modify it with information specified by the user's options.
*/
static struct colorSystem Customsystem = {
    "Custom",
    0.64,  0.33,  0.30,  0.60,  0.15,  0.06,  
    IlluminantD65, GAMMA_REC709
};
    


static void
upvp_to_xy(double   const up,
           double   const vp,
           double * const xc,
           double * const yc) {
/*----------------------------------------------------------------------------
    Given 1976 coordinates u', v', determine 1931 chromaticities x, y
-----------------------------------------------------------------------------*/
    *xc = 9*up / (6*up - 16*vp + 12);
    *yc = 4*vp / (6*up - 16*vp + 12);
}



static void
xy_to_upvp(double   const xc,
           double   const yc,
           double * const up,
           double * const vp) {
/*----------------------------------------------------------------------------
    Given 1931 chromaticities x, y, determine 1976 coordinates u', v'
-----------------------------------------------------------------------------*/
    *up = 4*xc / (- 2*xc + 12*yc + 3);
    *vp = 9*yc / (- 2*xc + 12*yc + 3);
}



static void
xyz_to_rgb(const struct colorSystem * const cs,
           double                      const xc,
           double                      const yc,
           double                      const zc,
           double *                    const r,
           double *                    const g,
           double *                    const b) {
/*----------------------------------------------------------------------------
    Given  an additive tricolor system CS, defined by the CIE x and y
    chromaticities of its three primaries (z is derived  trivially  as
    1-(x+y)),  and  a  desired chromaticity (XC, YC, ZC) in CIE space,
    determine the contribution of each primary in a linear combination
    which   sums  to  the  desired  chromaticity.   If  the  requested
    chromaticity falls outside the  Maxwell  triangle  (color  gamut)
    formed  by the three primaries, one of the r, g, or b weights will
    be negative.  

    Caller can use constrain_rgb() to desaturate an outside-gamut
    color to the closest representation within the available
    gamut. 
-----------------------------------------------------------------------------*/
    double xr, yr, zr, xg, yg, zg, xb, yb, zb;
    double xw, yw, zw;
    double rx, ry, rz, gx, gy, gz, bx, by, bz;
    double rw, gw, bw;

    xr = cs->xRed;    yr = cs->yRed;    zr = 1 - (xr + yr);
    xg = cs->xGreen;  yg = cs->yGreen;  zg = 1 - (xg + yg);
    xb = cs->xBlue;   yb = cs->yBlue;   zb = 1 - (xb + yb);

    xw = cs->xWhite;  yw = cs->yWhite;  zw = 1 - (xw + yw);

    /* xyz -> rgb matrix, before scaling to white. */
    rx = yg*zb - yb*zg;  ry = xb*zg - xg*zb;  rz = xg*yb - xb*yg;
    gx = yb*zr - yr*zb;  gy = xr*zb - xb*zr;  gz = xb*yr - xr*yb;
    bx = yr*zg - yg*zr;  by = xg*zr - xr*zg;  bz = xr*yg - xg*yr;

    /* White scaling factors.
       Dividing by yw scales the white luminance to unity, as conventional. */
    rw = (rx*xw + ry*yw + rz*zw) / yw;
    gw = (gx*xw + gy*yw + gz*zw) / yw;
    bw = (bx*xw + by*yw + bz*zw) / yw;

    /* xyz -> rgb matrix, correctly scaled to white. */
    rx = rx / rw;  ry = ry / rw;  rz = rz / rw;
    gx = gx / gw;  gy = gy / gw;  gz = gz / gw;
    bx = bx / bw;  by = by / bw;  bz = bz / bw;

    /* rgb of the desired point */
    *r = rx*xc + ry*yc + rz*zc;
    *g = gx*xc + gy*yc + gz*zc;
    *b = bx*xc + by*yc + bz*zc;
}



static int
constrain_rgb(double * const r,
              double * const g,
              double * const b) {
/*----------------------------------------------------------------------------
    If  the  requested RGB shade contains a negative weight for one of
    the primaries, it lies outside the color  gamut  accessible  from
    the  given  triple  of  primaries.  Desaturate it by adding white,
    equal quantities of R, G, and B, enough to make RGB all positive.
-----------------------------------------------------------------------------*/
    double w;

    /* Amount of white needed is w = - min(0, *r, *g, *b) */
    w = (0 < *r) ? 0 : *r;
    w = (w < *g) ? w : *g;
    w = (w < *b) ? w : *b;
    w = - w;

    /* Add just enough white to make r, g, b all positive. */
    if (w > 0) {
        *r += w;  *g += w; *b += w;

        return 1;                     /* Color modified to fit RGB gamut */
    }

    return 0;                         /* Color within RGB gamut */
}



static void
gamma_correct(const struct colorSystem * const cs,
              double *                   const c) {
/*----------------------------------------------------------------------------
    Transform linear RGB values to nonlinear RGB values.

    Rec. 709 is ITU-R Recommendation BT. 709 (1990)
    ``Basic Parameter Values for the HDTV Standard for the Studio and for
    International Programme Exchange'', formerly CCIR Rec. 709.

    For details see
       http://www.inforamp.net/~poynton/ColorFAQ.html
       http://www.inforamp.net/~poynton/GammaFAQ.html
-----------------------------------------------------------------------------*/
    double gamma;

    gamma = cs->gamma;

    if (gamma == 0.) {
    /* Rec. 709 gamma correction. */
    double cc = 0.018;
    if (*c < cc) {
        *c *= (1.099 * pow(cc, 0.45) - 0.099) / cc;
    } else {
        *c = 1.099 * pow(*c, 0.45) - 0.099;
    }
    } else {
    /* Nonlinear color = (Linear color)^(1/gamma) */
    *c = pow(*c, 1./gamma);
    }
}



static void
gamma_correct_rgb(const struct colorSystem * const cs,
                  double * const r,
                  double * const g,
                  double * const b) {
    gamma_correct(cs, r);
    gamma_correct(cs, g);
    gamma_correct(cs, b);
}



#define Sz(x) (((x) * MIN(pixcols, pixrows)) / 512)
#define B(x, y) ((x) + xBias), (y)
#define Bixels(y, x) pixels[y][x + xBias]



static void
computeMonochromeColorLocation(
    double                     const waveLength,
    int                        const pxcols,
    int                        const pxrows,
    bool                       const upvp,
    int *                      const xP,
    int *                      const yP) {

    int const ix = (waveLength - 380) / 5;
    double const px = spectral_chromaticity[ix][0];
    double const py = spectral_chromaticity[ix][1];

    if (upvp) {
        double up, vp;
        xy_to_upvp(px, py, &up, &vp);
        *xP = up * (pxcols - 1);
        *yP = (pxrows - 1) - vp * (pxrows - 1);
    } else {
        *xP = px * (pxcols - 1);
        *yP = (pxrows - 1) - py * (pxrows - 1);
    }
}



static void
makeAllBlack(pixel **     const pixels,
             unsigned int const cols,
             unsigned int const rows) {

    unsigned int row;
    for (row = 0; row < rows; ++row) {
        unsigned int col;
        for (col = 0; col < cols; ++col)
            PPM_ASSIGN(pixels[row][col], 0, 0, 0);
    }
}



static void
drawTongueOutline(pixel ** const pixels,
                  int    const pixcols,
                  int    const pixrows,
                  pixval const maxval,
                  bool   const upvp,
                  int    const xBias,
                  int    const yBias) {

    int const pxcols = pixcols - xBias;
    int const pxrows = pixrows - yBias;

    pixel rgbcolor;
    int wavelength;
    int lx, ly;
    int fx, fy;

    PPM_ASSIGN(rgbcolor, maxval, maxval, maxval);

    for (wavelength = 380; wavelength <= 700; wavelength += 5) {
        int icx, icy;

        computeMonochromeColorLocation(wavelength, pxcols, pxrows, upvp,
                                       &icx, &icy);
        
        if (wavelength > 380)
            ppmd_line(pixels, pixcols, pixrows, Maxval,
                      B(lx, ly), B(icx, icy),
                      PPMD_NULLDRAWPROC, (char *) &rgbcolor);
        else {
            fx = icx;
            fy = icy;
        }
        lx = icx;
        ly = icy;
    }
    ppmd_line(pixels, pixcols, pixrows, maxval,
              B(lx, ly), B(fx, fy),
              PPMD_NULLDRAWPROC, (char *) &rgbcolor);
}



static void
findTongue(pixel ** const pixels,
           int      const pxcols,
           int      const xBias,
           int      const row,
           bool *   const presentP,
           int *    const leftEdgeP,
           int *    const rightEdgeP) {
/*----------------------------------------------------------------------------
  Find out if there is any tongue on row 'row' of image 'pixels', and if
  so, where.

  We assume the image consists of all black except a white outline of the
  tongue.
-----------------------------------------------------------------------------*/
    int i;

    for (i = 0;
         i < pxcols && PPM_GETR(Bixels(row, i)) == 0;
         ++i);

    if (i >= pxcols)
        *presentP = FALSE;
    else {
        int j;
        int const leftEdge = i;

        *presentP = TRUE;
        
        for (j = pxcols - 1;
             j >= leftEdge && PPM_GETR(Bixels(row, j)) == 0;
             --j);

        *rightEdgeP = j;
        *leftEdgeP = leftEdge;
    }
}



static void
fillInTongue(pixel **                   const pixels,
             int                        const pixcols,
             int                        const pixrows,
             pixval                     const maxval,
             const struct colorSystem * const cs,
             bool                       const upvp,
             int                        const xBias,
             int                        const yBias,
             bool                       const highlightGamut) {

    int const pxcols = pixcols - xBias;
    int const pxrows = pixrows - yBias;

    int y;

    /* Scan the image line by line and  fill  the  tongue  outline
       with the RGB values determined by the color system for the x-y
       co-ordinates within the tongue.
    */

    for (y = 0; y < pxrows; ++y) {
        bool present;  /* There is some tongue on this line */
        int leftEdge; /* x position of leftmost pixel in tongue on this line */
        int rightEdge; /* same, but rightmost */

        findTongue(pixels, pxcols, xBias, y, &present, &leftEdge, &rightEdge);

        if (present) {
            int x;

            for (x = leftEdge; x <= rightEdge; ++x) {
                double cx, cy, cz, jr, jg, jb, jmax;
                int r, g, b, mx;

                if (upvp) {
                    double up, vp;
                    up = ((double) x) / (pxcols - 1);
                    vp = 1.0 - ((double) y) / (pxrows - 1);
                    upvp_to_xy(up, vp, &cx, &cy);
                    cz = 1.0 - (cx + cy);
                } else {
                    cx = ((double) x) / (pxcols - 1);
                    cy = 1.0 - ((double) y) / (pxrows - 1);
                    cz = 1.0 - (cx + cy);
                }

                xyz_to_rgb(cs, cx, cy, cz, &jr, &jg, &jb);

                mx = Maxval;
        
                /* Check whether the requested color  is  within  the
                   gamut  achievable with the given color system.  If
                   not, draw it in a reduced  intensity,  interpolated
                   by desaturation to the closest within-gamut color. */
        
                if (constrain_rgb(&jr, &jg, &jb)) {
                    mx = highlightGamut ? Maxval : ((Maxval + 1) * 3) / 4;
                }
                /* Scale to max(rgb) = 1. */
                jmax = MAX(jr, MAX(jg, jb));
                if (jmax > 0) {
                    jr = jr / jmax;
                    jg = jg / jmax;
                    jb = jb / jmax;
                }
                /* gamma correct from linear rgb to nonlinear rgb. */
                gamma_correct_rgb(cs, &jr, &jg, &jb);
                r = mx * jr;
                g = mx * jg;
                b = mx * jb;
                PPM_ASSIGN(Bixels(y, x), (pixval) r, (pixval) g, (pixval) b);
            }
        }
    }
}



static void
drawAxes(pixel ** const pixels,
         int    const pixcols,
         int    const pixrows,
         pixval const maxval,
         bool   const upvp,
         int    const xBias,
         int    const yBias) {

    int const pxcols = pixcols - xBias;
    int const pxrows = pixrows - yBias;

    pixel rgbcolor;  /* Color of axes */
    int i;

    PPM_ASSIGN(rgbcolor, maxval, maxval, maxval);
    ppmd_line(pixels, pixcols, pixrows, maxval,
              B(0, 0), B(0, pxrows - 1), PPMD_NULLDRAWPROC,
              (char *) &rgbcolor);
    ppmd_line(pixels, pixcols, pixrows, maxval,
              B(0, pxrows - 1), B(pxcols - 1, pxrows - 1),
              PPMD_NULLDRAWPROC, (char *) &rgbcolor);
    
    /* Draw tick marks on X and Y axes every 0.1 units.  Also
       label axes.
    */
    
    for (i = 1; i <= 9; i += 1) {
        char s[20];

        /* X axis tick */

        sprintf(s, "0.%d", i);
        ppmd_line(pixels, pixcols, pixrows, maxval,
                  B((i * (pxcols - 1)) / 10, pxrows - Sz(1)),
                  B((i * (pxcols - 1)) / 10, pxrows - Sz(4)),
                  PPMD_NULLDRAWPROC, (char *) &rgbcolor);
        ppmd_text(pixels, pixcols, pixrows, maxval,
                  B((i * (pxcols - 1)) / 10 - Sz(11), pxrows + Sz(12)),
                  Sz(10), 0, s, PPMD_NULLDRAWPROC, (char *) &rgbcolor);

        /* Y axis tick */

        sprintf(s, "0.%d", 10 - i);
        ppmd_line(pixels, pixcols, pixrows, maxval,
                  B(0, (i * (pxrows - 1)) / 10),
                  B(Sz(3), (i * (pxrows - 1)) / 10),
                  PPMD_NULLDRAWPROC, (char *) &rgbcolor);

        ppmd_text(pixels, pixcols, pixrows, maxval,
                  B(Sz(-30), (i * (pxrows - 1)) / 10 + Sz(5)),
                  Sz(10), 0, s,
                  PPMD_NULLDRAWPROC, (char *) &rgbcolor);
    }
    ppmd_text(pixels, pixcols, pixrows, maxval,
              B((98 * (pxcols - 1)) / 100 - Sz(11), pxrows + Sz(12)),
              Sz(10), 0, (upvp ? "u'" : "x"),
              PPMD_NULLDRAWPROC, (char *) &rgbcolor);
    ppmd_text(pixels,  pixcols, pixrows, maxval,
              B(Sz(-22), (2 * (pxrows - 1)) / 100 + Sz(5)),
              Sz(10), 0, (upvp ? "v'" : "y"),
              PPMD_NULLDRAWPROC, (char *) &rgbcolor);
}



static void
plotWhitePoint(pixel **                   const pixels,
               int                        const pixcols,
               int                        const pixrows,
               pixval                     const maxval,
               const struct colorSystem * const cs,
               bool                       const upvp,
               int                        const xBias,
               int                        const yBias) {

    int const pxcols = pixcols - xBias;
    int const pxrows = pixrows - yBias;

    int wx, wy;

    pixel rgbcolor;  /* Color of the white point mark */

    PPM_ASSIGN(rgbcolor, 0, 0, 0);

    if (upvp) {
        double wup, wvp;
        xy_to_upvp(cs->xWhite, cs->yWhite, &wup, &wvp);
        wx = wup;
        wy = wvp;
        wx = (pxcols - 1) * wup;
        wy = (pxrows - 1) - ((int) ((pxrows - 1) * wvp));
    } else {
        wx = (pxcols - 1) * cs->xWhite;
        wy = (pxrows - 1) - ((int) ((pxrows - 1) * cs->yWhite));
    }

    PPM_ASSIGN(rgbcolor, 0, 0, 0);
    /* We draw the four arms of the cross separately so as to
       leave the pixel representing the precise white point
       undisturbed.
    */
    ppmd_line(pixels, pixcols, pixrows, maxval,
              B(wx + Sz(3), wy), B(wx + Sz(10), wy),
              PPMD_NULLDRAWPROC, (char *) &rgbcolor);
    ppmd_line(pixels, pixcols, pixrows, maxval,
              B(wx - Sz(3), wy), B(wx - Sz(10), wy),
              PPMD_NULLDRAWPROC, (char *) &rgbcolor);
    ppmd_line(pixels, pixcols, pixrows, maxval,
              B(wx, wy + Sz(3)), B(wx, wy + Sz(10)),
              PPMD_NULLDRAWPROC, (char *) &rgbcolor);
    ppmd_line(pixels, pixcols, pixrows, maxval,
              B(wx, wy - Sz(3)), B(wx, wy - Sz(10)),
              PPMD_NULLDRAWPROC, (char *) &rgbcolor);
}



static void
plotBlackBodyCurve(pixel **                   const pixels,
                   int                        const pixcols,
                   int                        const pixrows,
                   pixval                     const maxval,
                   bool                       const upvp,
                   int                        const xBias,
                   int                        const yBias) {

    int const pxcols = pixcols - xBias;
    int const pxrows = pixrows - yBias;

    double t;  /* temperature of black body */
    pixel rgbcolor;  /* color of the curve */
    int lx, ly;  /* Last point we've drawn on curve so far */

    PPM_ASSIGN(rgbcolor, 0, 0, 0);

    /* Plot black body curve from 1000 to 30000 kelvins. */

    for (t = 1000.0; t < 30000.0; t += 50.0) {
        double lambda, X = 0, Y = 0, Z = 0;
        int xb, yb;
        int ix;

        /* Determine X, Y, and Z for blackbody by summing color
           match functions over the visual range. */

        for (ix = 0, lambda = 380; lambda <= 780.0; ix++, lambda += 5) {
            double Me;

            /* Evaluate Planck's black body equation for the
               power at this wavelength. */

            Me = 3.74183e-16 * pow(lambda * 1e-9, -5.0) /
                (exp(1.4388e-2/(lambda * 1e-9 * t)) - 1.0);
            X += Me * cie_color_match[ix][0];
            Y += Me * cie_color_match[ix][1];
            Z += Me * cie_color_match[ix][2];
        }
        if (upvp) {
            double up, vp;
            xy_to_upvp(X / (X + Y + Z), Y / (X + Y + Z), &up, &vp);
            xb = (pxcols - 1) * up;
            yb = (pxrows - 1) - ((pxrows - 1) * vp);
        } else {
            xb = (pxcols - 1) * X / (X + Y + Z);
            yb = (pxrows - 1) - ((pxrows - 1) * Y / (X + Y + Z));
        }

        if (t > 1000) {
            ppmd_line(pixels, pixcols, pixrows, Maxval,
                      B(lx, ly), B(xb, yb), PPMD_NULLDRAWPROC,
                      (char *) &rgbcolor);

            /* Draw tick mark every 1000 kelvins */

            if ((((int) t) % 1000) == 0) {
                ppmd_line(pixels, pixcols, pixrows, Maxval,
                          B(lx, ly - Sz(2)), B(lx, ly + Sz(2)),
                          PPMD_NULLDRAWPROC, (char *) &rgbcolor);

                /* Label selected tick marks with decreasing density. */

                if (t <= 5000.1 || (t > 5000.0 && 
                                    ((((int) t) % 5000) == 0) &&
                                    t != 20000.0)) {
                    char bb[20];

                    sprintf(bb, "%g", t);
                    ppmd_text(pixels, pixcols, pixrows, Maxval,
                              B(lx - Sz(12), ly - Sz(4)), Sz(6), 0, bb,
                              PPMD_NULLDRAWPROC, (char *) &rgbcolor);
                }
  
            }
        }
        lx = xb;
        ly = yb;
    }
}



static bool
overlappingLegend(bool const upvp,
                  int  const waveLength) {

    bool retval;

    if (upvp)
        retval = (waveLength == 430 || waveLength == 640);
    else 
        retval = (waveLength == 460 || waveLength == 630 || waveLength == 640);
    return retval;
}



static void
plotMonochromeWavelengths(
    pixel **                   const pixels,
    int                        const pixcols,
    int                        const pixrows,
    pixval                     const maxval,
    const struct colorSystem * const cs,
    bool                       const upvp,
    int                        const xBias,
    int                        const yBias) {

    int const pxcols = pixcols - xBias;
    int const pxrows = pixrows - yBias;

    int x;  /* The wavelength we're plotting */

    for (x = (upvp? 420 : 450);
         x <= 650;
         x += (upvp? 10 : (x > 470 && x < 600) ? 5 : 10)) {

        pixel rgbcolor;

        /* Ick.  Drop legends that overlap and twiddle position
           so they appear at reasonable positions with respect to
           the tongue.
        */

        if (!overlappingLegend(upvp, x)) {
            double cx, cy, cz, jr, jg, jb, jmax;
            char wl[20];
            int bx = 0, by = 0, tx, ty, r, g, b;
            int icx, icy;  /* Location of the color on the tongue */

            if (x < 520) {
                bx = Sz(-22);
                by = Sz(2);
            } else if (x < 535) {
                bx = Sz(-8);
                by = Sz(-6);
            } else {
                bx = Sz(4);
            }

            computeMonochromeColorLocation(x, pxcols, pxrows, upvp,
                                           &icx, &icy);

            /* Draw the tick mark */
            PPM_ASSIGN(rgbcolor, maxval, maxval, maxval);
            tx = icx + ((x < 520) ? Sz(-2) : ((x >= 535) ? Sz(2) : 0));
            ty = icy + ((x < 520) ? 0 : ((x >= 535) ? Sz(-1) : Sz(-2))); 
            ppmd_line(pixels, pixcols, pixrows, Maxval,
                      B(icx, icy), B(tx, ty),
                      PPMD_NULLDRAWPROC, (char *) &rgbcolor);

            /* The following flailing about sets the drawing color to
               the hue corresponding to the pure wavelength (constrained
               to the display gamut). */

            if (upvp) {
                double up, vp;
                up = ((double) icx) / (pxcols - 1);
                vp = 1.0 - ((double) icy) / (pxrows - 1);
                upvp_to_xy(up, vp, &cx, &cy);
                cz = 1.0 - (cx + cy);
            } else {
                cx = ((double) icx) / (pxcols - 1);
                cy = 1.0 - ((double) icy) / (pxrows - 1);
                cz = 1.0 - (cx + cy);
            }

            xyz_to_rgb(cs, cx, cy, cz, &jr, &jg, &jb);
            (void) constrain_rgb(&jr, &jg, &jb);

            /* Scale to max(rgb) = 1 */
            jmax = MAX(jr, MAX(jg, jb));
            if (jmax > 0) {
                jr = jr / jmax;
                jg = jg / jmax;
                jb = jb / jmax;
            }
            /* gamma correct from linear rgb to nonlinear rgb. */
            gamma_correct_rgb(cs, &jr, &jg, &jb);
            r = Maxval * jr;
            g = Maxval * jg;
            b = Maxval * jb;
            PPM_ASSIGN(rgbcolor, (pixval) r, (pixval) g, (pixval) b);

            sprintf(wl, "%d", x);
            ppmd_text(pixels, pixcols, pixrows, Maxval,
                      B(icx + bx, icy + by), Sz(6), 0, wl,
                      PPMD_NULLDRAWPROC, (char *) &rgbcolor);
        }
    }
}



static void
writeLabel(pixel **                   const pixels,
           int                        const pixcols,
           int                        const pixrows,
           pixval                     const maxval,
           const struct colorSystem * const cs) {

    pixel rgbcolor;  /* color of text */
    char sysdesc[256];

    PPM_ASSIGN(rgbcolor, maxval, maxval, maxval);
    
    snprintfN(sysdesc, sizeof(sysdesc),
              "System: %s\n"
              "Primary illuminants (X, Y)\n"
              "     Red:  %0.4f, %0.4f\n"
              "     Green: %0.4f, %0.4f\n"
              "     Blue:  %0.4f, %0.4f\n"
              "White point (X, Y): %0.4f, %0.4f",
              cs->name, cs->xRed, cs->yRed, cs->xGreen, cs->yGreen,
              cs->xBlue, cs->yBlue, cs->xWhite, cs->yWhite);
    sysdesc[sizeof(sysdesc)-1] = '\0';  /* for robustness */

    ppmd_text(pixels, pixcols, pixrows, Maxval,
              pixcols / 3, Sz(24), Sz(12), 0, sysdesc,
              PPMD_NULLDRAWPROC, (char *) &rgbcolor);
}



int
main(int argc,
     char * argv[]) {

    int argn;
    const char * const usage = "[-[no]black] [-[no]wpoint] [-[no]label] [-no[axes]] [-full]\n\
[-xy|-upvp] [-rec709|-ntsc|-ebu|-smpte|-hdtv|-cie]\n\
[-red <x> <y>] [-green <x> <y>] [-blue <x> <y>]\n\
[-white <x> <y>] [-gamma <g>]\n\
[-size <s>] [-xsize|-width <x>] [-ysize|-height <y>]";
    const struct colorSystem *cs;

    int widspec = FALSE, hgtspec = FALSE;
    int xBias, yBias;
    int upvp = FALSE;             /* xy or u'v' color coordinates? */
    int showWhite = TRUE;             /* Show white point ? */
    int showBlack = TRUE;             /* Show black body curve ? */
    int fullChart = FALSE;            /* Fill entire tongue ? */
    int showLabel = TRUE;             /* Show labels ? */
    int showAxes = TRUE;              /* Plot axes ? */

    ppm_init(&argc, argv);
    argn = 1;

    cs = &Rec709system;  /* default */
    while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0') {
        if (pm_keymatch(argv[argn], "-xy", 2)) {
            upvp = FALSE;
        } else if (pm_keymatch(argv[argn], "-upvp", 1)) {
            upvp = TRUE;
        } else if (pm_keymatch(argv[argn], "-xsize", 1) ||
                   pm_keymatch(argv[argn], "-width", 2)) {
            if (widspec) {
                pm_error("already specified a size/width/xsize");
            }
            argn++;
            if ((argn == argc) || (sscanf(argv[argn], "%d", &sxsize) != 1))
                pm_usage(usage);
            widspec = TRUE;
        } else if (pm_keymatch(argv[argn], "-ysize", 1) ||
                   pm_keymatch(argv[argn], "-height", 2)) {
            if (hgtspec) {
                pm_error("already specified a size/height/ysize");
            }
            argn++;
            if ((argn == argc) || (sscanf(argv[argn], "%d", &sysize) != 1))
                pm_usage(usage);
            hgtspec = TRUE;
        } else if (pm_keymatch(argv[argn], "-size", 2)) {
            if (hgtspec || widspec) {
                pm_error("already specified a size/height/ysize");
            }
            argn++;
            if ((argn == argc) || (sscanf(argv[argn], "%d", &sysize) != 1))
                pm_usage(usage);
            sxsize = sysize;
            hgtspec = widspec = TRUE;
        } else if (pm_keymatch(argv[argn], "-rec709", 1)) {
            cs = &Rec709system;
        } else if (pm_keymatch(argv[argn], "-ntsc", 1)) {
            cs = &NTSCsystem;
        } else if (pm_keymatch(argv[argn], "-ebu", 1)) {
            cs = &EBUsystem;
        } else if (pm_keymatch(argv[argn], "-smpte", 2)) {
            cs = &SMPTEsystem;
        } else if (pm_keymatch(argv[argn], "-hdtv", 2)) {
            cs = &HDTVsystem;                 
        } else if (pm_keymatch(argv[argn], "-cie", 1)) {
            cs = &CIEsystem;                 
        } else if (pm_keymatch(argv[argn], "-black", 3)) {
            showBlack = TRUE;         /* Show black body curve */
        } else if (pm_keymatch(argv[argn], "-wpoint", 2)) {
            showWhite = TRUE;         /* Show white point of color system */
        } else if (pm_keymatch(argv[argn], "-noblack", 3)) {
            showBlack = FALSE;        /* Don't show black body curve */
        } else if (pm_keymatch(argv[argn], "-nowpoint", 3)) {
            showWhite = FALSE;        /* Don't show white point of system */
        } else if (pm_keymatch(argv[argn], "-label", 1)) {
            showLabel = TRUE;         /* Show labels. */
        } else if (pm_keymatch(argv[argn], "-nolabel", 3)) {
            showLabel = FALSE;        /* Don't show labels */
        } else if (pm_keymatch(argv[argn], "-axes", 1)) {
            showAxes = TRUE;          /* Show axes. */
        } else if (pm_keymatch(argv[argn], "-noaxes", 3)) {
            showAxes = FALSE;         /* Don't show axes */
        } else if (pm_keymatch(argv[argn], "-full", 1)) {
            fullChart = TRUE;         /* Fill whole tongue full-intensity */
        } else if (pm_keymatch(argv[argn], "-gamma", 2)) {
            cs = &Customsystem;
            argn++;
            if ((argn == argc) ||
                (sscanf(argv[argn], "%lf", &Customsystem.gamma) != 1))
                pm_usage(usage);
        } else if (pm_keymatch(argv[argn], "-red", 1)) {
            cs = &Customsystem;
            argn++;
            if ((argn == argc) ||
                (sscanf(argv[argn], "%lf", &Customsystem.xRed) != 1))
                pm_usage(usage);
            argn++;
            if ((argn == argc) ||
                (sscanf(argv[argn], "%lf", &Customsystem.yRed) != 1))
                pm_usage(usage);
        } else if (pm_keymatch(argv[argn], "-green", 1)) {
            cs = &Customsystem;
            argn++;
            if ((argn == argc) ||
                (sscanf(argv[argn], "%lf", &Customsystem.xGreen) != 1))
                pm_usage(usage);
            argn++;
            if ((argn == argc) ||
                (sscanf(argv[argn], "%lf", &Customsystem.yGreen) != 1))
                pm_usage(usage);
        } else if (pm_keymatch(argv[argn], "-blue", 1)) {
            cs = &Customsystem;
            argn++;
            if ((argn == argc) ||
                (sscanf(argv[argn], "%lf", &Customsystem.xBlue) != 1))
                pm_usage(usage);
            argn++;
            if ((argn == argc) ||
                (sscanf(argv[argn], "%lf", &Customsystem.yBlue) != 1))
                pm_usage(usage);
        } else if (pm_keymatch(argv[argn], "-white", 1)) {
            cs = &Customsystem;
            argn++;
            if ((argn == argc) ||
                (sscanf(argv[argn], "%lf", &Customsystem.xWhite) != 1))
                pm_usage(usage);
            argn++;
            if ((argn == argc) ||
                (sscanf(argv[argn], "%lf", &Customsystem.yWhite) != 1))
                pm_usage(usage);
        } else {
            pm_usage(usage);
        }
        argn++;
    }

    if (argn != argc) {               /* Extra bogus arguments ? */
        pm_usage(usage);
    }

    pixcols = sxsize;
    pixrows = sysize;

    pixels = ppm_allocarray(pixcols, pixrows);

    /* Partition into plot area and axes and establish subwindow. */

    xBias = Sz(32);
    yBias = Sz(20);

    makeAllBlack(pixels, pixcols, pixrows);

    drawTongueOutline(pixels, pixcols, pixrows, Maxval, upvp, xBias, yBias);

    fillInTongue(pixels, pixcols, pixrows, Maxval, cs, upvp, xBias, yBias,
                 fullChart);

    if (showAxes)
        drawAxes(pixels, pixcols, pixrows, Maxval, upvp, xBias, yBias);

    if (showWhite)
        plotWhitePoint(pixels, pixcols, pixrows, Maxval,
                       cs, upvp, xBias, yBias);

    if (showBlack)
        plotBlackBodyCurve(pixels, pixcols, pixrows, Maxval,
                           upvp, xBias, yBias);

    /* Plot wavelengths around periphery of the tongue. */

    if (showAxes)
        plotMonochromeWavelengths(pixels, pixcols, pixrows, Maxval,
                                  cs, upvp, xBias, yBias);

    if (showLabel)
        writeLabel(pixels, pixcols, pixrows, Maxval, cs);

    ppm_writeppm(stdout, pixels, pixcols, pixrows, Maxval, FALSE);

    return 0;
}
