/* pamscale.c - rescale (resample) a PNM image

   This program evolved out of Jef Poskanzer's program Pnmscale from
   his Pbmplus package (which was derived from Poskanzer's 1989
   Ppmscale).  The resampling logic was taken from Michael Reinelt's
   program Pnmresample, somewhat recoded to follow Netpbm conventions.
   Michael submitted that for inclusion in Netpbm in December 2003.
   The frame of the program is by Bryan Henderson, and the old scaling
   algorithm is based on that in Jef Poskanzer's Pnmscale, but
   completely rewritten by Bryan Henderson ca. 2000.  Plenty of other
   people contributed code changes over the years.

   Copyright (C) 2003 by Michael Reinelt <reinelt@eunet.at>
  
   Copyright (C) 1989, 1991 by Jef Poskanzer.
  
   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose and without fee is hereby granted, provided
   that the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation.  This software is provided "as is" without express or
   implied warranty.
*/

#define _XOPEN_SOURCE   /* get M_PI in math.h */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"


/****************************/
/****************************/
/********* filters **********/
/****************************/
/****************************/

/* Most of the filters are FIR (finite impulse respone), but some
** (sinc, bessel) are IIR (infinite impulse respone).
** They should be windowed with hanning, hamming, blackman or
** kaiser window.
** For sinc and bessel the blackman window will be used per default.
*/

#define EPSILON 1e-7


/* x^2 and x^3 helper functions */
static __inline__ double 
pow2 (double x)
{
  return x*x;
}

static __inline__ double 
pow3 (double x) 
{
  return x*x*x;
}


/* box, pulse, Fourier window, */
/* box function also know as rectangle function */
/* 1st order (constant) b-spline */

#define radius_point (0.0)
#define radius_box (0.5)

static double 
filter_box (double x)
{
    if (x <  0.0) x = -x;
    if (x <= 0.5) return 1.0;
    return 0.0;
}


/* triangle, Bartlett window, */
/* triangle function also known as lambda function */
/* 2nd order (linear) b-spline */

#define radius_triangle (1.0)

static double 
filter_triangle (double x)
{
    if (x <  0.0) x = -x;
    if (x < 1.0) return 1.0-x;
    return 0.0;
}


/* 3rd order (quadratic) b-spline */

#define radius_quadratic (1.5)

static double 
filter_quadratic(double x)
{
    if (x <  0.0) x = -x;
    if (x < 0.5) return 0.75-pow2(x);
    if (x < 1.5) return 0.50*pow2(x-1.5);
    return 0.0;
}


/* 4th order (cubic) b-spline */

#define radius_cubic (2.0)

static double 
filter_cubic(double x)
{
    if (x <  0.0) x = -x;
    if (x < 1.0) return 0.5*pow3(x) - pow2(x) + 2.0/3.0;
    if (x < 2.0) return pow3(2.0-x)/6.0;
    return 0.0;
}


/* Catmull-Rom spline, Overhauser spline */

#define radius_catrom (2.0)

static double 
filter_catrom(double x)
{
    if (x <  0.0) x = -x;
    if (x < 1.0) return  1.5*pow3(x) - 2.5*pow2(x)         + 1.0;
    if (x < 2.0) return -0.5*pow3(x) + 2.5*pow2(x) - 4.0*x + 2.0;
    return 0.0;
}


/* Mitchell & Netravali's two-param cubic */
/* see Mitchell&Netravali,  */
/* "Reconstruction Filters in Computer Graphics", SIGGRAPH 88 */

#define radius_mitchell (2.0)

static double 
filter_mitchell(double x)
{

    double b = 1.0/3.0;
    double c = 1.0/3.0;

    double p0 = (  6.0 -  2.0*b         ) / 6.0;
    double p2 = (-18.0 + 12.0*b +  6.0*c) / 6.0;
    double p3 = ( 12.0 -  9.0*b -  6.0*c) / 6.0;
    double q0 = (         8.0*b + 24.0*c) / 6.0;
    double q1 = (      - 12.0*b - 48.0*c) / 6.0;
    double q2 = (         6.0*b + 30.0*c) / 6.0;
    double q3 = (      -      b -  6.0*c) / 6.0;

    if (x <  0.0) x = -x;
    if (x <  1.0) return p3*pow3(x) + p2*pow2(x)        + p0;
    if (x < 2.0) return q3*pow3(x) + q2*pow2(x) + q1*x + q0;
    return 0.0;
}


/* Gaussian filter (infinite) */

#define radius_gauss (1.25)

static double 
filter_gauss(double x)
{
    return exp(-2.0*pow2(x)) * sqrt(2.0/M_PI);
}


/* sinc, perfect lowpass filter (infinite) */

#define radius_sinc (4.0)

static double 
filter_sinc(double x)
{
    /* Note: Some people say sinc(x) is sin(x)/x.  Others say it's
       sin(PI*x)/(PI*x), a horizontal compression of the former which is
       zero at integer values.  We use the latter, whose Fourier transform
       is a canonical rectangle function (edges at -1/2, +1/2, height 1).
    */
    if (x == 0.0) return 1.0;
    return sin(M_PI*x)/(M_PI*x);
}


/* Bessel (for circularly symm. 2-d filt, infinite) */
/* See Pratt "Digital Image Processing" p. 97 for Bessel functions */

#define radius_bessel (3.2383)

static double 
filter_bessel(double x)
{
    if (x == 0.0) return M_PI/4.0;
    return j1(M_PI*x)/(2.0*x);
}


/* Hanning window (infinite) */

#define radius_hanning (1.0)

static double 
filter_hanning(double x)
{
    return 0.5*cos(M_PI*x) + 0.5;
}


/* Hamming window (infinite) */

#define radius_hamming (1.0)

static double 
filter_hamming(double x)
{
  return 0.46*cos(M_PI*x) + 0.54;
}


/* Blackman window (infinite) */

#define radius_blackman (1.0)

static double 
filter_blackman(double x)
{
    return 0.5*cos(M_PI*x) + 0.08*cos(2.0*M_PI*x) + 0.42;
}


/* parameterized Kaiser window (infinite) */
/* from Oppenheim & Schafer, Hamming */

#define radius_kaiser (1.0)

/* modified zeroth order Bessel function of the first kind. */
static double 
bessel_i0(double x)
{
  
    int i;
    double sum, y, t;
  
    sum = 1.0;
    y = pow2(x)/4.0;
    t = y;
    for (i=2; t>EPSILON; i++) {
        sum += t;
        t   *= (double)y/pow2(i);
    }
    return sum;
}

static double 
filter_kaiser(double x)
{
    /* typically 4<a<9 */
    /* param a trades off main lobe width (sharpness) */
    /* for side lobe amplitude (ringing) */
  
    double a   = 6.5;
    double i0a = 1.0/bessel_i0(a);
  
    return i0a*bessel_i0(a*sqrt(1.0-pow2(x)));
}


/* normal distribution (infinite) */
/* Normal(x) = Gaussian(x/2)/2 */

#define radius_normal (1.0)

static double 
filter_normal(double x)
{
    return exp(-pow2(x)/2.0) / sqrt(2.0*M_PI);
    return 0.0;
}


/* Hermite filter */

#define radius_hermite  (1.0)

static double 
filter_hermite(double x)
{
    /* f(x) = 2|x|^3 - 3|x|^2 + 1, -1 <= x <= 1 */
    if (x <  0.0) x = -x;
    if (x <  1.0) return 2.0*pow3(x) - 3.0*pow2(x) + 1.0;
    return 0.0;
}


/* Lanczos filter */

#define radius_lanczos (3.0)

static double 
filter_lanczos(double x)
{
    if (x <  0.0) x = -x;
    if (x <  3.0) return filter_sinc(x) * filter_sinc(x/3.0);
    return(0.0);
}



typedef struct {
    const char *name;
    double (*function)(double);
    double radius;
        /* This is how far from the Y axis (on either side) the
           function has significant value.  (You can use this to limit
           how much of your domain you bother to compute the function
           over).  
        */
    bool windowed;
} filter;


static filter Filters[] = {
    { "point",     filter_box,       radius_point,     FALSE },
    { "box",       filter_box,       radius_box,       FALSE },
    { "triangle",  filter_triangle,  radius_triangle,  FALSE },
    { "quadratic", filter_quadratic, radius_quadratic, FALSE },
    { "cubic",     filter_cubic,     radius_cubic,     FALSE },
    { "catrom",    filter_catrom,    radius_catrom,    FALSE },
    { "mitchell",  filter_mitchell,  radius_mitchell,  FALSE },
    { "gauss",     filter_gauss,     radius_gauss,     FALSE },
    { "sinc",      filter_sinc,      radius_sinc,      TRUE  },
    { "bessel",    filter_bessel,    radius_bessel,    TRUE  },
    { "hanning",   filter_hanning,   radius_hanning,   FALSE },
    { "hamming",   filter_hamming,   radius_hamming,   FALSE },
    { "blackman",  filter_blackman,  radius_blackman,  FALSE },
    { "kaiser",    filter_kaiser,    radius_kaiser,    FALSE },
    { "normal",    filter_normal,    radius_normal,    FALSE },
    { "hermite",   filter_hermite,   radius_hermite,   FALSE },
    { "lanczos",   filter_lanczos,   radius_lanczos,   FALSE },
   { NULL },
};


typedef double (*basicFunction_t)(double);


/****************************/
/****************************/
/****** end of filters ******/
/****************************/
/****************************/



enum scaleType {SCALE_SEPARATE, SCALE_BOXFIT, SCALE_BOXFILL, SCALE_PIXELMAX};
    /* This is a way of specifying the output dimensions.

       SCALE_SEPARATE means specify the horizontal and vertical scaling
       separately.  One or both may be unspecified.

       SCALE_BOXFIT means specify height and width of a box, and the
       image must be scaled, preserving aspect ratio, to the largest
       size that will fit in the box.  Some of the box may be empty.

       SCALE_BOXFILL means specify height and width of a box, and the
       image must be scaled, preserving aspect ratio, to the smallest
       size that completely fills the box.  Some of the image may be
       outside the box.

       SCALE_PIXELMAX means specify the maximum number of pixels the result
       should have and scale preserving aspect ratio and maximizing image
       size.
    */

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
     * in a form easy for the program to use.
     */
    const char * inputFileName;  /* Filespec of input file */
    unsigned int nomix;
    basicFunction_t filterFunction; /* NULL if not using resample method */
    basicFunction_t windowFunction;
        /* Meaningful only when filterFunction != NULL */
    double filterRadius;           
        /* Meaningful only when filterFunction != NULL */
    enum scaleType scaleType;
    /* 'xsize' and 'ysize' are numbers of pixels.  Their meaning depends upon
       'scaleType'.  for SCALE_BOXFIT and SCALE_BOXFILL, they are the box 
       dimensions.  For SCALE_SEPARATE, they are the separate dimensions, or
       zero to indicate unspecified.  For SCALE_PIXELMAX, they are
       meaningless.
    */
    unsigned int xsize;
    unsigned int ysize;
    /* 'xscale' and 'yscale' are meaningful only for scaleType == 
       SCALE_SEPARATE and only where the corresponding xsize/ysize is
       unspecified.  0.0 means unspecified.
    */
    float xscale;
    float yscale;
    /* 'pixels' is meaningful only for scaleType == SCALE_PIXELMAX */
    unsigned int pixels; 
    unsigned int linear;
    unsigned int verbose;
};



static void
lookupFilterByName(const char * const filtername,
                   filter *     const filterP) {

    unsigned int i;
    bool found;

    found = FALSE;  /* initial assumption */

    for (i=0; Filters[i].name; ++i) {
        if (strcmp(filtername, Filters[i].name) == 0) {
            *filterP = Filters[i];
            found = TRUE;
        }
    }
    if (!found) {
        unsigned int i;
        char known_filters[1024];
        strcpy(known_filters, "");
        for (i = 0; Filters[i].name; ++i) {
            const char * const name = Filters[i].name;
            if (strlen(known_filters) + strlen(name) + 1 + 1 < 
                sizeof(known_filters)) {
                strcat(known_filters, name);
                strcat(known_filters, " ");
            }
        }
        pm_error("No such filter as '%s'.  Known filter names are: %s",
                 filtername, known_filters);
    }
}



static void
processFilterOptions(unsigned int const         filterSpec,
                     const char                 filterOpt[],
                     unsigned int const         windowSpec,
                     const char                 windowOpt[],
                     struct cmdlineInfo * const cmdlineP) {

    if (filterSpec) {
        filter baseFilter;
        lookupFilterByName(filterOpt, &baseFilter);
        cmdlineP->filterFunction = baseFilter.function; 
        cmdlineP->filterRadius   = baseFilter.radius;

        if (windowSpec) {
            filter windowFilter;
            lookupFilterByName(windowOpt, &windowFilter);
            
            if (cmdlineP->windowFunction == filter_box)
                cmdlineP->windowFunction = NULL;
            else
                cmdlineP->windowFunction = windowFilter.function;
        } else {
            /* Default for most filters is no window.  Those that _require_
               a window function get Blackman.
               */
            if (baseFilter.windowed)
                cmdlineP->windowFunction = filter_blackman;
            else
                cmdlineP->windowFunction = NULL;
        }
    } else
        cmdlineP->filterFunction = NULL;
}



static void
parseXyParms(int                  const argc, 
             char **              const argv,
             struct cmdlineInfo * const cmdlineP) {

    /* parameters are box width (columns), box height (rows), and
       optional filespec 
    */
    if (argc-1 < 2)
        pm_error("You must supply at least two parameters with "
                 "-xyfit/xyfill/xysize: "
                 "x and y dimensions of the bounding box.");
    else if (argc-1 > 3)
        pm_error("Too many arguments.  With -xyfit/xyfill/xysize, "
                 "you need 2 or 3 arguments.");
    else {
        char * endptr;
        cmdlineP->xsize = strtol(argv[1], &endptr, 10);
        if (strlen(argv[1]) > 0 && *endptr != '\0')
            pm_error("horizontal size argument not an integer: '%s'", 
                     argv[1]);
        if (cmdlineP->xsize <= 0)
            pm_error("horizontal size argument is not positive: %d", 
                     cmdlineP->xsize);
        
        cmdlineP->ysize = strtol(argv[2], &endptr, 10);
        if (strlen(argv[2]) > 0 && *endptr != '\0')
            pm_error("vertical size argument not an integer: '%s'", 
                     argv[2]);
        if (cmdlineP->ysize <= 0)
            pm_error("vertical size argument is not positive: %d", 
                     cmdlineP->ysize);
        
        if (argc-1 < 3)
            cmdlineP->inputFileName = "-";
        else
            cmdlineP->inputFileName = argv[3];
    }
}



static void
parseScaleParms(int                   const argc, 
                char **               const argv,
                struct cmdlineInfo  * const cmdlineP) {

    /* parameters are scale factor and optional filespec */
    if (argc-1 < 1)
        pm_error("With no dimension options, you must supply at least "
                 "one parameter: the scale factor.");
    else {
        cmdlineP->xscale = cmdlineP->yscale = atof(argv[1]);
        
        if (cmdlineP->xscale == 0.0)
            pm_error("The scale parameter %s is not a positive number.",
                     argv[1]);
        else {
            if (argc-1 < 2)
                cmdlineP->inputFileName = "-";
            else
                cmdlineP->inputFileName = argv[2];
        }
    }
}



static void
parseFilespecOnlyParms(int                   const argc, 
                       char **               const argv,
                       struct cmdlineInfo  * const cmdlineP) {

    /* Only parameter allowed is optional filespec */
    if (argc-1 < 1)
        cmdlineP->inputFileName = "-";
    else
        cmdlineP->inputFileName = argv[1];
}


static void 
parseCommandLine(int argc, 
                 char ** argv, 
                 struct cmdlineInfo  * const cmdlineP) {
/* --------------------------------------------------------------------------
   Parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
--------------------------------------------------------------------------*/
    optEntry *option_def;
    /* Instructions to optParseOptions3 on how to parse our options. */
    optStruct3 opt;
  
    unsigned int option_def_index;
    unsigned int xyfit, xyfill;
    int xsize, ysize, pixels;
    int reduce;
    float xscale, yscale;
    const char *filterOpt, *window;
    unsigned int filterSpec, windowSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "xsize",     OPT_UINT,    &xsize,     NULL,                 0);
    OPTENT3(0, "width",     OPT_UINT,    &xsize,     NULL,                 0);
    OPTENT3(0, "ysize",     OPT_UINT,    &ysize,     NULL,                 0);
    OPTENT3(0, "height",    OPT_UINT,    &ysize,     NULL,                 0);
    OPTENT3(0, "xscale",    OPT_FLOAT,   &xscale,    NULL,                 0);
    OPTENT3(0, "yscale",    OPT_FLOAT,   &yscale,    NULL,                 0);
    OPTENT3(0, "pixels",    OPT_UINT,    &pixels,    NULL,                 0);
    OPTENT3(0, "reduce",    OPT_UINT,    &reduce,    NULL,                 0);
    OPTENT3(0, "xysize",    OPT_FLAG,    NULL,       &xyfit,               0);
    OPTENT3(0, "xyfit",     OPT_FLAG,    NULL,       &xyfit,               0);
    OPTENT3(0, "xyfill",    OPT_FLAG,    NULL,       &xyfill,              0);
    OPTENT3(0, "verbose",   OPT_FLAG,    NULL,       &cmdlineP->verbose,  0);
    OPTENT3(0, "filter",    OPT_STRING,  &filterOpt, &filterSpec,          0);
    OPTENT3(0, "window",    OPT_STRING,  &window,    &windowSpec,          0);
    OPTENT3(0, "nomix",     OPT_FLAG,    NULL,       &cmdlineP->nomix,    0);
    OPTENT3(0, "linear",    OPT_FLAG,    NULL,       &cmdlineP->linear,   0);
  
    /* Set the defaults. -1 = unspecified */

    /* (Now that we're using ParseOptions3, we don't have to do this -1
     * nonsense, but we don't want to risk screwing these complex 
     * option compatibilities up, so we'll convert that later.
     */
    xsize = -1;
    ysize = -1;
    xscale = -1.0;
    yscale = -1.0;
    pixels = -1;
    reduce = -1;
    
    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;   /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
    /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (cmdlineP->nomix && filterSpec) 
        pm_error("You cannot specify both -nomix and -filter.");

    processFilterOptions(filterSpec, filterOpt, windowSpec, window,
                         cmdlineP);

    if (xsize == 0)
        pm_error("-xsize/width must be greater than zero.");
    if (ysize == 0)
        pm_error("-ysize/height must be greater than zero.");
    if (xscale != -1.0 && xscale <= 0.0)
        pm_error("-xscale must be greater than zero.");
    if (yscale != -1.0 && yscale <= 0.0)
        pm_error("-yscale must be greater than zero.");
    if (reduce <= 0 && reduce != -1)
        pm_error("-reduce must be greater than zero.");

    if (xsize != -1 && xscale != -1)
        pm_error("Cannot specify both -xsize/width and -xscale.");
    if (ysize != -1 && yscale != -1)
        pm_error("Cannot specify both -ysize/height and -yscale.");
    
    if ((xyfit || xyfill) &&
        (xsize != -1 || xscale != -1 || ysize != -1 || yscale != -1 || 
         reduce != -1 || pixels != -1) )
        pm_error("Cannot specify -xyfit/xyfill/xysize with other "
                 "dimension options.");
    if (xyfit && xyfill)
        pm_error("Cannot specify both -xyfit and -xyfill");
    if (pixels != -1 && 
        (xsize != -1 || xscale != -1 || ysize != -1 || yscale != -1 ||
         reduce != -1) )
        pm_error("Cannot specify -pixels with other dimension options.");
    if (reduce != -1 && 
        (xsize != -1 || xscale != -1 || ysize != -1 || yscale != -1) )
        pm_error("Cannot specify -reduce with other dimension options.");

    if (pixels == 0)
        pm_error("-pixels must be greater than zero");

    /* Get the program parameters */

    if (xyfit || xyfill) {
        cmdlineP->scaleType = xyfit ? SCALE_BOXFIT : SCALE_BOXFILL;
        parseXyParms(argc, argv, cmdlineP);
    } else if (reduce != -1) {
        cmdlineP->scaleType = SCALE_SEPARATE;
        parseFilespecOnlyParms(argc, argv, cmdlineP);
        cmdlineP->xsize = cmdlineP->ysize = 0;
        cmdlineP->xscale = cmdlineP->yscale = 
            ((double) 1.0) / ((double) reduce);
        pm_message("reducing by %d gives scale factor of %f.", 
                   reduce, cmdlineP->xscale);
    } else if (pixels != -1) {
        cmdlineP->scaleType = SCALE_PIXELMAX;
        parseFilespecOnlyParms(argc, argv, cmdlineP);
        cmdlineP->pixels = pixels;
    } else if (xsize == -1 && xscale == -1 && ysize == -1 && yscale == -1
               && pixels == -1 && reduce == -1) {
        cmdlineP->scaleType = SCALE_SEPARATE;
        parseScaleParms(argc, argv, cmdlineP);
        cmdlineP->xsize = cmdlineP->ysize = 0;
    } else {
        cmdlineP->scaleType = SCALE_SEPARATE;
        parseFilespecOnlyParms(argc, argv, cmdlineP);
        cmdlineP->xsize = xsize == -1 ? 0 : xsize;
        cmdlineP->ysize = ysize == -1 ? 0 : ysize;
        cmdlineP->xscale = xscale == -1.0 ? 0.0 : xscale;
        cmdlineP->yscale = yscale == -1.0 ? 0.0 : yscale;
    }
}



static void 
computeOutputDimensions(struct cmdlineInfo  const cmdline, 
                        int                 const rows, 
                        int                 const cols, 
                        int *               const newrowsP, 
                        int *               const newcolsP) {

    switch(cmdline.scaleType) {
    case SCALE_PIXELMAX: {
        if (rows * cols <= cmdline.pixels) {
            *newrowsP = rows;
            *newcolsP = cols;
        } else {
            const double scale =
                sqrt( (float) cmdline.pixels / ((float) cols * (float) rows));
            *newrowsP = rows * scale;
            *newcolsP = cols * scale;
        }
    } break;
    case SCALE_BOXFIT:
    case SCALE_BOXFILL: {
        double const aspect_ratio = (float) cols / (float) rows;
        double const box_aspect_ratio = 
            (float) cmdline.xsize / (float) cmdline.ysize;
        
        if ((box_aspect_ratio > aspect_ratio && 
             cmdline.scaleType == SCALE_BOXFIT) ||
            (box_aspect_ratio < aspect_ratio &&
             cmdline.scaleType == SCALE_BOXFILL)) {
            *newrowsP = cmdline.ysize;
            *newcolsP = *newrowsP * aspect_ratio + 0.5;
        } else {
            *newcolsP = cmdline.xsize;
            *newrowsP = *newcolsP / aspect_ratio + 0.5;
        }
    } break;
    case SCALE_SEPARATE: {
        if (cmdline.xsize)
            *newcolsP = cmdline.xsize;
        else if (cmdline.xscale)
            *newcolsP = cmdline.xscale * cols + .5;
        else if (cmdline.ysize)
            *newcolsP = cols * ((float) cmdline.ysize/rows) +.5;
        else
            *newcolsP = cols;

        if (cmdline.ysize)
            *newrowsP = cmdline.ysize;
        else if (cmdline.yscale)
            *newrowsP = cmdline.yscale * rows +.5;
        else if (cmdline.xsize)
            *newrowsP = rows * ((float) cmdline.xsize/cols) +.5;
        else
            *newrowsP = rows;
    }
    }

    /* If the calculations above yielded (due to rounding) a zero 
     * dimension, we fudge it up to 1.  We do this rather than considering
     * it a specification error (and dying) because it's friendlier to 
     * automated processes that work on arbitrary input.  It saves them
     * having to check their numbers to avoid catastrophe.
     */
  
    if (*newcolsP < 1) *newcolsP = 1;
    if (*newrowsP < 1) *newrowsP = 1;
}




/****************************/
/****************************/
/******* resampling *********/
/****************************/
/****************************/

/* The resample code was inspired by Paul Heckbert's zoom program.
** http://www.cs.cmu.edu/~ph/zoom
*/

struct filterFunction {
/*----------------------------------------------------------------------------
   A function to convolve with the samples.
-----------------------------------------------------------------------------*/
    basicFunction_t basicFunction;
        /* The basic shape of the function.  Its horizontal scale is
           designed to filter out frequencies greater than 1.
        */
    basicFunction_t windowFunction;
        /* A function to multiply by basicFunction().  NULL if none. */
    double windowScaler;
        /* Factor by which to compress windowFunction() horizontally */
    double horizontalScaler;
        /* Factor by which to compress basicFunction() *
           windowFunction horizontally.  Note that compressing
           horizontally in the sample domain is equivalent to
           expanding horizontally (and shrinking vertically) in the
           frequency domain.  I.e. values less than unity have the
           effect of chopping out high frequencies.
        */
    double radius;
        /* A final filter.  filterFunction(x) is zero for |x| > radius
           regardless of what the rest of the members say.

           Implementation note:  This is important because windowFunction(),
           out of laziness, doesn't do the whole job of windowing.  It is
           not zero beyond the cutoff points as it should be.  If not for
           that, radius would only be a hint to describe what the other
           members already do, so the convolver knows where to stop.
        */
};

typedef struct {
    /* A term of the linear combination of input rows that makes up an
       output row.  I.e. an input row and its weight.

       Alternatively, the analogous thing for a column.
    */
    int    position;    /* Row/column number in the input image */
    double weight;      /* Weight to be given to that row/col.  In [0, 1]. */
} WEIGHT;

typedef struct {
    /* A description of the linear combination of input rows that
       generates a particular output row.  An output row is a weighted
       average of some input rows.  E.g. Row 2 of the output might be
       composed of 50% of Row 2 of the input and 50% of Row 3 of the
       input.

       Alternatively, the analogous thing for columns.
    */
    unsigned int nWeight;
        /* Number of elements in 'Weight'.  They're consecutive, starting
           at index 0.
        */
    unsigned int allocWeight;
        /* Number of allocated frames in 'Weight' */ 
    WEIGHT *Weight;
        /* The terms of the linear combination.  Has 'nWeight' elements. 
           The coefficients (weights) of each add up to unity.
        */
} WLIST;

typedef struct {
    /* This identifies a row of the input image. */
    int rowNumber;
        /* The row number in the input image of the row.
           -1 means no row.
        */
    tuple *tuplerow;  
        /* The tuples of the row.
           If rowNumber = -1, these are arbitrary, but allocated, tuples.
        */
} SCANLINE;

typedef struct {
    /* A vertical window of a raster */
    int width;    /* Width of the window, in columns */
    int height;   /* Height of the window, in rows */
    SCANLINE *line;
        /* An array of 'height' elements, malloced.
           This identifies the lines of the input image that compose the
           window.  The index order is NOT the order of the rows in the
           image.  E.g. line[0] isn't always the topmost row of the window.
           Rather, the rows are arranged in a cycle and you have to know
           indpendently where the topmost one is.  E.g. the rows of a 5
           line window with topmost row at index 3 might be:

              line[0] = Row 24
              line[1] = Row 25
              line[2] = Row 26
              line[3] = Row 22
              line[4] = Row 23
        */
} SCAN;



static int
appendWeight(WLIST * const WList,
             int     const index,
             double  const weight) {
/*----------------------------------------------------------------------------
   Add a weighting of 'weight' for index 'index' to the weight list
   'WList'.
-----------------------------------------------------------------------------*/
    if (weight == 0.0) {
        /* A weight of 0 in the list is redundant, so we don't add it.
           A weight entry says "Add W fraction of the pixel at index I,"
           so where W is 0, it's the same as not having the entry at all.
        */
    } else {
        unsigned int const n = WList->nWeight;

        assert(WList->allocWeight >= n+1);
        
        WList->Weight[n].position = index;
        WList->Weight[n].weight   = weight;
        ++WList->nWeight;
    }
    return 0;
}



static sample
floatToSample(double const value,
              sample const maxval) {

    /* Take care here, the conversion of any floating point value <=
       -1.0 to an unsigned type is _undefined_.  See ISO 9899:1999
       section 6.3.1.4.  Not only is it undefined it also does the
       wrong thing in actual practice, EG on Darwin PowerPC (my iBook
       running OS X) negative values clamp to maxval.  We get negative
       values because some of the filters (EG catrom) have negative
       weights.  
    */

    return MIN(maxval, (sample)(MAX(0.0, (value + 0.5))));
}



static void
initWeightList(WLIST *      const weightListP,
               unsigned int const maxWeights) {

    weightListP->nWeight = 0;
    weightListP->allocWeight = maxWeights;
    MALLOCARRAY(weightListP->Weight, maxWeights);
    if (weightListP->Weight == NULL)
        pm_error("Out of memory allocating a %u-element weight list.",
                 maxWeights);
}



static void
createWeightList(unsigned int          const targetPos,
                 unsigned int          const sourceSize,
                 double                const scale,
                 struct filterFunction       filter,
                 WLIST *               const weightListP) {
/*----------------------------------------------------------------------------
   Create a weight list for computing target pixel number 'targetPos' from
   a set of source pixels.  These pixels are a line of pixels either 
   horizontally or vertically.  The weight list is a list of weights to give
   each source pixel in the set.
   
   The source pixel set is a window of source pixels centered on some
   point.  The weights are defined by the function 'filter' of
   the position within the window, and normalized to add up to 1.0.
   Technically, the window is infinite, but we know that the filter
   function is zero beyond a certain distance from the center of the
   window.

   For example, assume 'targetPos' is 5.  That means we're computing weights
   for either Column 5 or Row 5 of the target image.  Assume it's Column 5.
   Assume 'radius' is 1.  That means a window of two pixels' worth of a
   source row determines the color of the Column 5 pixel of a target
   row.  Assume 'filter' is a triangle function -- 1 at 0, sloping
   down to 0 at -1 and 1.

   Now assume that the scale factor is 2 -- the target image will be
   twice the size of the source image.  That means the two-pixel-wide
   window of the source row that affects Column 5 of the target row
   (centered at target position 5.5) goes from position 1.75 to
   3.75, centered at 2.75.  That means the window covers 1/4 of
   Column 1, all of Column 2, and 3/4 of Column 3 of the source row.

   We want to calculate 3 weights, one to be applied to each source pixel
   in computing the target pixel.  Ideally, we would compute the average
   height of the filter function over each source pixel region.  But 
   that's too hard.  So we approximate by assuming that the filter function
   is constant within each region, at the value the function has at the
   _center_ of the region.

   So for the Column 1 region, which goes from 1.75 to 2.00, centered
   -.875 from the center of the window, we assume a constant function
   value of triangle(-.875), which equals .125.  For the 2.00-3.00
   region, we get triangle(-.25) = .75.  For the 3.00-3.75 region, we
   get triangle(.125) = .875.  So the weights for the 3 regions, which
   we get by multiplying this constant function value by the width of
   the region and normalizing so they add up to 1 are:

      Source Column 1:  .125*.25 / 1.4375 = .022
      Source Column 2:  .75*1.00 / 1.4375 = .521
      Source Column 3:  .875*.75 / 1.4375 = .457

   These are the weights we return.  Thus, if we assume that the source
   pixel 1 has value 10, source pixel 2 has value 20, and source pixel 3
   has value 30, Caller would compute target pixel 5's value as

      10*.022 + 20*.521 + 30*.457 = 24

-----------------------------------------------------------------------------*/
    /* 'windowCenter', is the continous position within the source of
       the center of the window that will influence target pixel
       'targetPos'.  'left' and 'right' are the edges of the window.
       'leftPixel' and 'rightPixel' are the pixel positions of the
       pixels at the edges of that window.  Note that if we're
       doing vertical weights, "left" and "right" mean top and
       bottom.  
    */
    double const windowCenter = ((double)targetPos + 0.5) / scale;
    double left = MAX(0.0, windowCenter - filter.radius - EPSILON);
    unsigned int const leftPixel = floor(left);
    double right = MIN((double)sourceSize - EPSILON, 
                       windowCenter + filter.radius + EPSILON);
    unsigned int const rightPixel = floor(right);

    double norm;
    unsigned int j;

    initWeightList(weightListP, rightPixel - leftPixel + 1);

    /* calculate weights */
    norm = 0.0;  /* initial value */

    for (j = leftPixel; j <= rightPixel; ++j) {
        /* Calculate the weight that source pixel 'j' will have in the 
           value of target pixel 'targetPos'.
        */
        double const regionLeft   = MAX(left, (double)j);
        double const regionRight  = MIN(right, (double)(j + 1));
        double const regionWidth  = regionRight - regionLeft;
        double const regionCenter = (regionRight + regionLeft) / 2;
        double const dist         = regionCenter - windowCenter;
        double weight;

        weight = filter.basicFunction(filter.horizontalScaler * dist);
        if (filter.windowFunction)
            weight *= filter.windowFunction(
                filter.horizontalScaler * filter.windowScaler * dist);

        assert(regionWidth <= 1.0);
        weight *= regionWidth;
        norm += weight;
        appendWeight(weightListP, j, weight);
    }

    if (norm == 0.0)
        pm_error("INTERNAL ERROR: No source pixels contribute to target "
                 "pixel %u", targetPos);

    /* normalize the weights so they add up to 1.0 */
    if (norm != 1.0) {
        unsigned int n;
        for (n = 0; n < weightListP->nWeight; ++n) {
            weightListP->Weight[n].weight /= norm;
        }
    }
}



static void
createWeightListSet(unsigned int          const sourceSize,
                    unsigned int          const targetSize,
                    struct filterFunction const filterFunction,
                    WLIST **              const weightListSetP) {
/*----------------------------------------------------------------------------
   Create the set of weight lists that will effect the resample.

   This is where the actual work of resampling gets done.

   The weight list set is a bunch of factors one can multiply by the
   pixels in a region to effect a resampling.  Multiplying by these
   factors effects all of the following transformations on the
   original pixels:
   
   1) Filter out any frequencies that are artifacts of the
      original sampling.  We assume a perfect sampling was done,
      which means the original continuous dataset had a maximum
      frequency of 1/2 of the original sample rate and anything
      above that is an artifact of the sampling.  So we filter out
      anything above 1/2 of the original sample rate (sample rate
      == pixel resolution).
      
   2) Filter out any frequencies that are too high to be captured
      by the new sampling -- i.e. frequencies above 1/2 the new
      sample rate.  This is the information we must lose due to low
      sample rate.
      
   3) Sample the result at the new sample rate.

   We do all three of these steps in a single convolution of the
   original pixels.  Steps (1) and (2) can be combined into a
   single frequency domain rectangle function.  A frequency domain
   rectangle function is a pixel domain sinc function, which is
   what we assume 'filterFunction' is.  We get Step 3 by computing
   the convolution only at the new sample points.
   
   I don't know what any of this means when 'filterFunction' is
   not sinc.  Maybe it just means some approximation or additional
   filtering steps are happening.
-----------------------------------------------------------------------------*/
    double const scale = (double)targetSize / sourceSize;
    WLIST *weightListSet;  /* malloc'ed */
    unsigned int targetPos;

    MALLOCARRAY_NOFAIL(weightListSet, targetSize);
    
    for (targetPos = 0; targetPos < targetSize; ++targetPos)
        createWeightList(targetPos, sourceSize, scale, filterFunction,
                         &weightListSet[targetPos]);

    *weightListSetP = weightListSet;
}



static struct filterFunction
makeFilterFunction(double          const scale,
                   basicFunction_t       basicFunction,
                   double          const basicRadius,
                   basicFunction_t       windowFunction) {
/*----------------------------------------------------------------------------
   Create a function to convolve with the samples (so it isn't actually
   a filter function, but the Fourier transform of a filter function.
   A filter function is something you multiply by in the frequency domain)
   to create a function from which one can resample.

   Convolving with this function will achieve two goals:

   1) filter out high frequencies that are artifacts of the original
      sampling (i.e. the turning of a continuous function into a staircase
      function);
   2) filter out frequencies higher than half the resample rate, so that
      the resample will be a perfect sampling of it, and not have aliasing.


   To make the calculation even more efficient, we take advantage
   of the fact that the weight list doesn't depend on the
   particular old and new sample rates at all except -- all that's
   important is their ratio (which is 'scale').  So we assume the
   original sample rate is 1 and the new sample rate is 'scale'.

-----------------------------------------------------------------------------*/
    double const freqLimit = MIN(1.0, scale);
        /* We're going to cut out any frequencies above this, to accomplish
           Steps (1) and (2) above.
        */

    struct filterFunction retval;

    retval.basicFunction = basicFunction;
    retval.windowFunction = windowFunction;
    
    retval.horizontalScaler = freqLimit;

    /* Our 'windowFunction' argument is a function normalized to the
       domain (-1, 1).  We need to scale it horizontally to fit the
       basic filter function.  We assume the radius of the filter
       function is the area to which the window should fit (i.e. zero
       beyond the radius, nonzero inside the radius).  But that's
       really a misuse of radius, because radius is supposed to be
       just the distance beyond which we can assume for convenience
       that the filter function is zero, possibly giving up some
       precision.

       But note that 'windowFunction' isn't zero outside (-1, 1), even
       though the actual window function is supposed to be.  Hence,
       scaling the window function exactly to the radius stops our
       calculations from noticing the wrong values outside (-1, 1) --
       we'll never use them.
    */
    retval.windowScaler = 1/basicRadius;

    retval.radius = basicRadius / retval.horizontalScaler;

    return retval;
}
                   

                   
static void
destroyWeightListSet(WLIST *      const weightListSet, 
                     unsigned int const size) {

    unsigned int i;

    for (i = 0; i < size; ++i)
        free(weightListSet[i].Weight);

    free(weightListSet);
}



static void
createScanBuf(struct pam * const pamP,
              double       const maxRowWeights,
              bool         const verbose,
              SCAN *       const scanbufP) {

    SCAN scanbuf;
    unsigned int lineNumber;

    scanbuf.width = pamP->width;
    scanbuf.height = maxRowWeights;
    MALLOCARRAY_NOFAIL(scanbuf.line, scanbuf.height);
  
    for (lineNumber = 0; lineNumber < scanbuf.height; ++lineNumber) {
        scanbuf.line[lineNumber].rowNumber = -1;
        scanbuf.line[lineNumber].tuplerow = pnm_allocpamrow(pamP);
    }
  
    if (verbose)
        pm_message("scanline buffer: %d lines of %d pixels", 
                   scanbuf.height, scanbuf.width);

    *scanbufP = scanbuf;
}



static void
destroyScanbuf(SCAN const scanbuf) {

    unsigned int lineNumber;

    for (lineNumber = 0; lineNumber < scanbuf.height; ++lineNumber)
        pnm_freepamrow(scanbuf.line[lineNumber].tuplerow);

    free(scanbuf.line);
}



static void
resampleDimensionMessage(struct pam * const inpamP,
                         struct pam * const outpamP) {

    pm_message ("resampling from %d*%d to %d*%d (%f, %f)", 
                inpamP->width, inpamP->height, 
                outpamP->width, outpamP->height,
                (double)outpamP->width/inpamP->width, 
                (double)outpamP->height/inpamP->height);
}



static void
addInPixel(const struct pam * const pamP,
           tuple              const tuple,
           float              const weight,
           bool               const haveOpacity,
           unsigned int       const opacityPlane,
           double *           const accum) {
/*----------------------------------------------------------------------------
  Add into *accum the values from the tuple 'tuple', weighted by
  'weight'.

  Iff 'haveOpacity', Plane 'opacityPlane' of the tuple is an opacity
  (alpha, transparency) plane.
-----------------------------------------------------------------------------*/
    unsigned int plane;

    for (plane = 0; plane < pamP->depth; ++plane) {
        sample adjustedForOpacity;
        
        if (haveOpacity && plane != opacityPlane) {
            float const opacity = (float)tuple[opacityPlane]/pamP->maxval;
            float const unadjusted = (float)tuple[plane]/pamP->maxval;

            adjustedForOpacity = 
                floatToSample(unadjusted * opacity, pamP->maxval);
        } else
            adjustedForOpacity = tuple[plane];
        
        accum[plane] += (double)adjustedForOpacity * weight;
    }
}



static void
generateOutputTuple(const struct pam * const pamP,
                    double             const accum[],
                    bool               const haveOpacity, 
                    unsigned int       const opacityPlane,
                    tuple *            const tupleP) {
/*----------------------------------------------------------------------------
  Convert the values accum[] accumulated for a pixel by
  outputOneResampledRow() to a bona fide PAM tuple as *tupleP,
  as described by *pamP.
-----------------------------------------------------------------------------*/
    unsigned int plane;

    for (plane = 0; plane < pamP->depth; ++plane) {
        float opacityAdjustedSample;

        if (haveOpacity && plane != opacityPlane) {
            if (accum[opacityPlane] < EPSILON) {
                assert(accum[plane] < EPSILON);
                opacityAdjustedSample = 0.0;
            } else 
                opacityAdjustedSample = accum[plane] / accum[opacityPlane];
        } else
            opacityAdjustedSample = accum[plane];

        (*tupleP)[plane] = floatToSample(opacityAdjustedSample, pamP->maxval);
    }
}



static void
outputOneResampledRow(const struct pam * const outpamP,
                      SCAN               const scanbuf,
                      WLIST              const YW,
                      const WLIST *      const XWeight,
                      tuple *            const line,
                      double *           const accum) {
/*----------------------------------------------------------------------------
   From the data in 'scanbuf' and weights in 'YW' and 'XWeight', 
   generate one output row for the image described by *outpamP and
   output it.

   An output pixel is a weighted average of the pixels in a certain
   rectangle of the input.  'YW' and 'XWeight' describe those weights
   for each column of the row we are to output.

   'line' and 'accum' are just working space that Caller provides us
   with to save us the time of allocating it.  'line' is at least big
   enough to hold an output row; 'weight' is at least outpamP->depth
   big.
-----------------------------------------------------------------------------*/
    unsigned int col;

    bool haveOpacity;           /* There is an opacity plane */
    unsigned int opacityPlane;  /* Plane number of opacity plane, if any */

    pnm_getopacity(outpamP, &haveOpacity, &opacityPlane);

    for (col = 0; col < outpamP->width; ++col) {
        WLIST const XW = XWeight[col];

        unsigned int i;
        {
            unsigned int plane;
            for (plane = 0; plane < outpamP->depth; ++plane)
                accum[plane] = 0.0;
        }
        
        for (i = 0; i < YW.nWeight; ++i) {
            int   const yp   = YW.Weight[i].position;
            float const yw   = YW.Weight[i].weight;
            int   const slot = yp % scanbuf.height;

            unsigned int j;
            
            for (j = 0; j < XW.nWeight; ++j) {
                int   const xp    = XW.Weight[j].position;
                tuple const tuple = scanbuf.line[slot].tuplerow[xp];
                
                addInPixel(outpamP, tuple, yw * XW.Weight[j].weight, 
                           haveOpacity, opacityPlane,
                           accum);
            }
        }
        generateOutputTuple(outpamP, accum, haveOpacity, opacityPlane, 
                            &line[col]);
    }
    pnm_writepamrow(outpamP, line);
}



static bool
scanbufContainsTheRows(SCAN  const scanbuf,
                       WLIST const rowWeights) {
/*----------------------------------------------------------------------------
   Return TRUE iff scanbuf 'scanbuf' contains every row mentioned in
   'rowWeights'.

   It might contain additional rows besides.
-----------------------------------------------------------------------------*/
    bool missingRow;
    unsigned int i;
    
    for (i = 0, missingRow = FALSE;
         i < rowWeights.nWeight && !missingRow;
        ++i) {
        unsigned int const inputRow = rowWeights.Weight[i].position;
        unsigned int const slot = inputRow % scanbuf.height;
            /* This is the number of the slot in the scanbuf that would
               have the input row in question if the scanbuf has the
               row at all.
            */
        if (scanbuf.line[slot].rowNumber != inputRow) {
            /* Nope, this slot has some other row or no row at all.
               So the row we're looking for isn't in the scanbuf.
            */
            missingRow = TRUE;
        }
    }
    return !missingRow;
}



static void
createWeightLists(struct pam *     const inpamP,
                  struct pam *     const outpamP,
                  basicFunction_t  const filterFunction,
                  double           const filterRadius,
                  basicFunction_t  const windowFunction,
                  WLIST **         const horizWeightP,
                  WLIST **         const vertWeightP,
                  unsigned int *   const maxRowWeightsP) {
/*----------------------------------------------------------------------------
   This is the function that actually does the resampling.  Note that it
   does it without ever looking at the source or target pixels!  It produces
   a simple set of numbers that Caller can blindly apply to the source 
   pixels to get target pixels.
-----------------------------------------------------------------------------*/
    struct filterFunction horizFilter, vertFilter;

    horizFilter = makeFilterFunction(
        (double)outpamP->width/inpamP->width,
        filterFunction, filterRadius, windowFunction);

    createWeightListSet(inpamP->width, outpamP->width, horizFilter, 
                        horizWeightP);
    
    vertFilter = makeFilterFunction(
        (double)outpamP->height/inpamP->height,
        filterFunction, filterRadius, windowFunction);

    createWeightListSet(inpamP->height, outpamP->height, vertFilter, 
                        vertWeightP);

    *maxRowWeightsP = ceil(2.0*(vertFilter.radius+EPSILON) + 1 + EPSILON);
}



static void
resample(struct pam *     const inpamP,
         struct pam *     const outpamP,
         basicFunction_t  const filterFunction,
         double           const filterRadius,
         basicFunction_t  const windowFunction,
         bool             const verbose,
         bool             const linear) {
/*---------------------------------------------------------------------------
  Resample the image in the input file, described by *inpamP,
  so as to create the image in the output file, described by *outpamP.
  
  Input and output differ by height, width, and maxval only.

  Use the resampling filter function 'filterFunction', applied over
  radius 'filterRadius'.
  
  The input file is positioned past the header, to the beginning of the
  raster.  The output file is too.
---------------------------------------------------------------------------*/
    int inputRow, outputRow;
    WLIST * horizWeight;
    WLIST * vertWeight;
    SCAN scanbuf;
    unsigned int maxRowWeights;

    tuple * line;
        /* This is just work space for outputOneSampleRow() */
    double * weight;
        /* This is just work space for outputOneSampleRow() */

    if (linear)
        pm_error("You cannot use the resampling scaling method on "
                 "linear input.");
  
    createWeightLists(inpamP, outpamP, filterFunction, filterRadius,
                      windowFunction, &horizWeight, &vertWeight,
                      &maxRowWeights);

    createScanBuf(inpamP, maxRowWeights, verbose, &scanbuf);

    if (verbose)
        resampleDimensionMessage(inpamP, outpamP);

    line = pnm_allocpamrow(outpamP);
    MALLOCARRAY_NOFAIL(weight, outpamP->depth);

    outputRow = 0;
    for (inputRow = 0; inputRow < inpamP->height; ++inputRow) {
        bool needMoreInput;
            /* We've output as much as we can using the rows that are in
               the scanbuf; it's time to move the window.  Or fill it in
               the first place.
            */
        unsigned int scanbufSlot;

        /* Read source row; add it to the scanbuf */
        scanbufSlot = inputRow % scanbuf.height;
        scanbuf.line[scanbufSlot].rowNumber = inputRow;
        pnm_readpamrow(inpamP, scanbuf.line[scanbufSlot].tuplerow);

        /* Output all the rows we can make out of the current contents of
           the scanbuf.  Might be none.
        */
        needMoreInput = FALSE;  /* initial assumption */
        while (outputRow < outpamP->height && !needMoreInput) {
            WLIST const rowWeights = vertWeight[outputRow];
                /* The description of what makes up our current output row;
                   i.e. what fractions of which input rows combine to create
                   this output row.
                */
            assert(rowWeights.nWeight <= scanbuf.height); 

            if (scanbufContainsTheRows(scanbuf, rowWeights)) {
                outputOneResampledRow(outpamP, scanbuf, rowWeights, 
                                      horizWeight, line, weight);
                ++outputRow;
            } else
                needMoreInput = TRUE;
        }
    }

    if (outputRow != outpamP->height)
        pm_error("INTERNAL ERROR: assembled only %u of the required %u "
                 "output rows.", outputRow, outpamP->height);

    pnm_freepamrow(line);
    destroyScanbuf(scanbuf);
    destroyWeightListSet(horizWeight, outpamP->width);
    destroyWeightListSet(vertWeight, outpamP->height);
}


/****************************/
/****************************/
/**** end of resampling *****/
/****************************/
/****************************/



static void
zeroNewRow(struct pam * const pamP,
           tuplen *     const tuplenrow) {
    
    unsigned int col;

    for (col = 0; col < pamP->width; ++col) {
        unsigned int plane;

        for (plane = 0; plane < pamP->depth; ++plane)
            tuplenrow[col][plane] = 0.0;
    }
}



static void
accumOutputCol(struct pam * const pamP,
               tuplen       const intuplen,
               float        const fraction, 
               tuplen       const accumulator) {
/*----------------------------------------------------------------------------
   Add fraction 'fraction' of the pixel indicated by 'intuplen' to the
   pixel accumulator 'accumulator'.

   'intuplen' and 'accumulator' are not a standard libnetpbm tuplen.
   It is proportional to light intensity.  The foreground color
   component samples are proportional to light intensity, and have
   opacity factored in.
-----------------------------------------------------------------------------*/
    unsigned int plane;

    for (plane = 0; plane < pamP->depth; ++plane)
        accumulator[plane] += fraction * intuplen[plane];
}



static void
horizontalScale(tuplen *     const inputtuplenrow, 
                tuplen *     const newtuplenrow,
                struct pam * const inpamP,
                struct pam * const outpamP,
                float        const xscale,
                float *      const stretchP) {
/*----------------------------------------------------------------------------
  Take the input row 'inputtuplenrow', decribed by *inpamP, and scale
  it by a factor of 'xscale', to create the output row 'newtuplenrow',
  described by *outpamP.

  Due to arithmetic imprecision, we may have to stretch slightly the
  contents of the last pixel of the output row to make a full pixel.
  Return as *stretchP the fraction of a pixel by which we had to
  stretch in this way.

  Assume maxval and depth of input and output are the same.
-----------------------------------------------------------------------------*/
    float fraccoltofill, fraccolleft;
    unsigned int col;
    unsigned int newcol;

    newcol = 0;
    fraccoltofill = 1.0;  /* Output column is "empty" now */

    zeroNewRow(outpamP, newtuplenrow);

    for (col = 0; col < inpamP->width; ++col) {
        /* Process one tuple from input ('inputtuplenrow') */
        fraccolleft = xscale;
        /* Output all columns, if any, that can be filled using information
           from this input column, in addition to what's already in the output
           column.
        */
        while (fraccolleft >= fraccoltofill) {
            /* Generate one output pixel in 'newtuplerow'.  It will
               consist of anything accumulated from prior input pixels
               in accumulator[], plus a fraction of the current input
               pixel.  
            */
            assert(newcol < outpamP->width);
            accumOutputCol(inpamP, inputtuplenrow[col], fraccoltofill,
                           newtuplenrow[newcol]);

            fraccolleft -= fraccoltofill;

            /* Set up to start filling next output column */
            ++newcol;
            fraccoltofill = 1.0;
        }
        /* There's not enough left in the current input pixel to fill up 
           a whole output column, so just accumulate the remainder of the
           pixel into the current output column.  Due to rounding, we may
           have a tiny bit of pixel left and have run out of output pixels.
           In that case, we throw away what's left.
        */
        if (fraccolleft > 0.0 && newcol < outpamP->width) {
            accumOutputCol(inpamP, inputtuplenrow[col], fraccolleft,
                           newtuplenrow[newcol]);
            fraccoltofill -= fraccolleft;
        }
    }

    if (newcol < outpamP->width-1 || newcol > outpamP->width)
        pm_error("Internal error: last column filled is %d, but %d "
                 "is the rightmost output column.",
                 newcol, outpamP->width-1);

    if (newcol < outpamP->width) {
        /* We were still working on the last output column when we 
           ran out of input columns.  This would be because of rounding
           down, and we should be missing only a tiny fraction of that
           last output column.  Just fill in the missing color with the
           color of the rightmost input pixel.
        */
        accumOutputCol(inpamP, inputtuplenrow[inpamP->width-1], 
                       fraccoltofill, newtuplenrow[newcol]);
        
        *stretchP = fraccoltofill;
    } else 
        *stretchP = 0.0;
}



static void
zeroAccum(struct pam * const pamP,
          tuplen *     const accumulator) {

    unsigned int plane;

    for (plane = 0; plane < pamP->depth; ++plane) {
        unsigned int col;
        for (col = 0; col < pamP->width; ++col)
            accumulator[col][plane] = 0.0;
    }
}



static void
accumOutputRow(struct pam * const pamP,
               tuplen *     const tuplenrow, 
               float        const fraction, 
               tuplen *     const accumulator) {
/*----------------------------------------------------------------------------
   Take 'fraction' times the samples in row 'tuplenrow' and add it to 
   'accumulator' in the same way as accumOutputCol().

   'fraction' is less than 1.0.
-----------------------------------------------------------------------------*/
    unsigned int plane;

    for (plane = 0; plane < pamP->depth; ++plane) {
        unsigned int col;
        for (col = 0; col < pamP->width; ++col)
            accumulator[col][plane] += fraction * tuplenrow[col][plane];
    }
}



static void
readARow(struct pam *             const pamP,
         tuplen *                 const tuplenRow,
         const pnm_transformMap * const transform) {
/*----------------------------------------------------------------------------
  Read a row from the input file described by *pamP, as values (for the
  foreground color) proportional to light intensity, with opacity
  included.

  By contrast, a simple libnetpbm read would give the same numbers you
  find in a PAM: gamma-adjusted values for the foreground color
  component and scaled as if opaque.  The latter means that full red
  would have a red intensity of 1.0 even if the pixel is only 75%
  opaque.  We, on the other hand, would return red intensity of .75 in
  that case.

  The opacity plane we return is the same as a simple libnetpbm read
  would return.

  We ASSUME that the transform 'transform' is that necessary to effect
  the conversion to intensity-linear values and normalize.  If it is
  NULL, we ASSUME that they already are intensity-proportional and just
  need to be normalized.
-----------------------------------------------------------------------------*/
    tuple * tupleRow;

    tupleRow = pnm_allocpamrow(pamP);

    pnm_readpamrow(pamP, tupleRow);

    pnm_normalizeRow(pamP, tupleRow, transform, tuplenRow);

    pnm_applyopacityrown(pamP, tuplenRow);

    pnm_freepamrow(tupleRow);
}



static void
writeARow(struct pam *             const pamP,
          tuplen *                 const tuplenRow,
          const pnm_transformMap * const transform) {
/*----------------------------------------------------------------------------
  Write a row to the output file described by *pamP, from values
  proportional to light intensity with opacity included (i.e. the same
  kind of number you would get form readARow()).

  We ASSUME that the transform 'transform' is that necessary to effect
  the conversion to brightness-linear unnormalized values.  If it is
  NULL, we ASSUME that they already are brightness-proportional and just
  need to be unnormalized.

  We destroy *tuplenRow in the process.
-----------------------------------------------------------------------------*/
    tuple * tupleRow;

    tupleRow = pnm_allocpamrow(pamP);

    pnm_unapplyopacityrown(pamP, tuplenRow);

    pnm_unnormalizeRow(pamP, tuplenRow, transform, tupleRow);

    pnm_writepamrow(pamP, tupleRow);

    pnm_freepamrow(tupleRow);
}



static void
issueStretchWarning(bool   const verbose, 
                    double const fracrowtofill) {

    /* We need another input row to fill up this
       output row, but there aren't any more.
       That's because of rounding down on our
       scaling arithmetic.  So we go ahead with
       the data from the last row we read, which
       amounts to stretching out the last output
       row.  
    */
    if (verbose)
        pm_message("%f of bottom row stretched due to "
                   "arithmetic imprecision", 
                   fracrowtofill);
}



static void
scaleHorizontallyAndOutputRow(struct pam *             const inpamP,
                              struct pam *             const outpamP,
                              tuplen *                 const rowAccumulator,
                              const pnm_transformMap * const transform,
                              tuplen *                 const newtuplenrow,
                              float                    const xscale,
                              unsigned int             const row,
                              bool                     const verbose) {
/*----------------------------------------------------------------------------
   Scale the row in 'rowAccumulator' horizontally by factor 'xscale'
   and output it.

   'newtuplenrow' is work space Caller provides us.  It is at least
   wide enough to hold one output row.
-----------------------------------------------------------------------------*/
    if (outpamP->width == inpamP->width)    
        /* shortcut X scaling */
        writeARow(outpamP, rowAccumulator, transform);
            /* This destroys 'rowAccumulator' */
    else {
        float stretch;
            
        horizontalScale(rowAccumulator, newtuplenrow, inpamP, outpamP,
                        xscale, &stretch);
            
        if (verbose && row == 0)
            pm_message("%f of right column stretched due to "
                       "arithmetic imprecision", 
                       stretch);
            
        writeARow(outpamP, newtuplenrow, transform);
            /* This destroys 'newtuplenrow' */
    }
}



static void
createTransforms(struct pam *              const inpamP,
                 struct pam *              const outpamP,
                 bool                      const assumeLinear,
                 const pnm_transformMap ** const inputTransformP,
                 const pnm_transformMap ** const outputTransformP) {

    if (assumeLinear) {
        *inputTransformP  = NULL;
        *outputTransformP = NULL;
    } else {
        *inputTransformP  = pnm_createungammatransform(inpamP);
        *outputTransformP = pnm_creategammatransform(outpamP);
    }
}



static void
destroyTransforms(const pnm_transformMap * const inputTransform,
                  const pnm_transformMap * const outputTransform) {

    if (inputTransform)
        free((void*)inputTransform);
    
    if (outputTransform)
        free((void*)outputTransform);
}



static void
scaleWithMixing(struct pam * const inpamP,
                struct pam * const outpamP,
                float        const xscale, 
                float        const yscale,
                bool         const assumeLinear,
                bool         const verbose) {
/*----------------------------------------------------------------------------
  Scale the image described by *inpamP by xscale horizontally and
  yscale vertically and write the result as the image described by
  *outpamP.

  The input file is positioned past the header, to the beginning of the
  raster.  The output file is too.

  Mix colors from input rows together in the output rows.

  'assumeLinear' means to assume that the sample values in the input
  image vary from standard PAM in that they are proportional to
  intensity, (This makes the computation a lot faster, so you might
  use this even if the samples are actually standard PAM, to get
  approximate but fast results).

-----------------------------------------------------------------------------*/
    /* Here's how we think of the color mixing scaling operation:  
       
       First, I'll describe scaling in one dimension.  Assume we have
       a one row image.  A raster row is ordinarily a sequence of
       discrete pixels which have no width and no distance between
       them -- only a sequence.  Instead, think of the raster row as a
       bunch of pixels 1 unit wide adjacent to each other.  For
       example, we are going to scale a 100 pixel row to a 150 pixel
       row.  Imagine placing the input row right above the output row
       and stretching it so it is the same size as the output row.  It
       still contains 100 pixels, but they are 1.5 units wide each.
       Our goal is to make the output row look as much as possible
       like the stretched input row, while observing that a pixel can
       be only one color.

       Output Pixel 0 is completely covered by Input Pixel 0, so we
       make Output Pixel 0 the same color as Input Pixel 0.  Output
       Pixel 1 is covered half by Input Pixel 0 and half by Input
       Pixel 1.  So we make Output Pixel 1 a 50/50 mix of Input Pixels
       0 and 1.  If you stand back far enough, input and output will
       look the same.

       This works for all scale factors, both scaling up and scaling down.
       
       For images with an opacity plane, imagine Input Pixel 0's
       foreground is fully opaque red (1,0,0,1), and Input Pixel 1 is
       fully transparent (foreground irrelevant) (0,0,0,0).  We make
       Output Pixel 0's foreground fully opaque red as before.  Output
       Pixel 1 is covered half by Input Pixel 0 and half by Input
       Pixel 1, so it is 50% opaque; but its foreground color is still
       red: (1,0,0,0.5).  The output foreground color is the opacity
       and coverage weighted average of the input foreground colors,
       and the output opacity is the coverage weighted average of the
       input opacities.

       This program always stretches or squeezes the input row to be the
       same length as the output row; The output row's pixels are always
       1 unit wide.

       The same thing works in the vertical direction.  We think of
       rows as stacked strips of 1 unit height.  We conceptually
       stretch the image vertically first (same process as above, but
       in place of a single-color pixels, we have a vector of colors).
       Then we take each row this vertical stretching generates and
       stretch it horizontally.  
    */

    tuplen * tuplenrow;     /* An input row */
    tuplen * newtuplenrow;  /* Working space */
    float rowsleft;
    /* The number of rows of output that need to be formed from the
       current input row (the one in tuplerow[]), less the number that 
       have already been formed (either in accumulator[]
       or output to the file).  This can be fractional because of the
       way we define rows as having height.
    */
    float fracrowtofill;
        /* The fraction of the current output row (the one in vertScaledRow[])
           that hasn't yet been filled in from an input row.
        */
    tuplen * rowAccumulator;
        /* The red, green, and blue color intensities so far accumulated
           from input rows for the current output row.  The ultimate value
           of this is an output row after vertical scaling, but before
           horizontal scaling.
        */
    int rowsread;
        /* Number of rows of the input file that have been read */
    int row;
    const pnm_transformMap * inputTransform;
    const pnm_transformMap * outputTransform;
    
    tuplenrow = pnm_allocpamrown(inpamP); 
    rowAccumulator = pnm_allocpamrown(inpamP);

    rowsread = 0;
    rowsleft = 0.0;
    fracrowtofill = 1.0;

    newtuplenrow = pnm_allocpamrown(outpamP);

    createTransforms(inpamP, outpamP, assumeLinear,
                     &inputTransform, &outputTransform);

    for (row = 0; row < outpamP->height; ++row) {
        /* First scale Y from tuplerow[] into rowAccumulator[]. */

        zeroAccum(inpamP, rowAccumulator);

        if (outpamP->height == inpamP->height) {
            /* shortcut Y scaling */
            readARow(inpamP, rowAccumulator, inputTransform);
        } else {
            while (fracrowtofill > 0) {
                if (rowsleft <= 0.0) {
                    if (rowsread < inpamP->height) {
                        readARow(inpamP, tuplenrow, inputTransform);
                        ++rowsread;
                    } else
                        issueStretchWarning(verbose, fracrowtofill);
                    rowsleft = yscale;
                }
                if (rowsleft < fracrowtofill) {
                    accumOutputRow(inpamP, tuplenrow, rowsleft,
                                   rowAccumulator);
                    fracrowtofill -= rowsleft;
                    rowsleft = 0.0;
                } else {
                    accumOutputRow(inpamP, tuplenrow, fracrowtofill,
                                   rowAccumulator);
                    rowsleft = rowsleft - fracrowtofill;
                    fracrowtofill = 0.0;
                }
            }
            fracrowtofill = 1.0;
        }
        /* 'rowAccumulator' now contains the contents of a single
           output row, but not yet horizontally scaled.  Scale it now
           horizontally and write it out.
        */
        scaleHorizontallyAndOutputRow(inpamP, outpamP, rowAccumulator,
                                      outputTransform, newtuplenrow, xscale,
                                      row, verbose);
            /* Destroys rowAccumulator */

    }
    destroyTransforms(inputTransform, outputTransform);
    pnm_freepamrown(rowAccumulator);
    pnm_freepamrown(newtuplenrow);
    pnm_freepamrown(tuplenrow);
}



static void
scaleWithoutMixing(const struct pam * const inpamP,
                   const struct pam * const outpamP,
                   float              const xscale, 
                   float              const yscale) {
/*----------------------------------------------------------------------------
  Scale the image described by *inpamP by xscale horizontally and
  yscale vertically and write the result as the image described by
  *outpamP.

  The input file is positioned past the header, to the beginning of the
  raster.  The output file is too.
  
  Don't mix colors from different input pixels together in the output
  pixels.  Each output pixel is an exact copy of some corresponding 
  input pixel.
-----------------------------------------------------------------------------*/
    tuple * tuplerow;  /* An input row */
    tuple * newtuplerow;
    int row;
    int rowInInput;

    tuplerow = pnm_allocpamrow(inpamP); 
    rowInInput = -1;

    newtuplerow = pnm_allocpamrow(outpamP);

    for (row = 0; row < outpamP->height; ++row) {
        int col;
        
        int const inputRow = (int) (row / yscale);

        for (; rowInInput < inputRow; ++rowInInput) 
            pnm_readpamrow(inpamP, tuplerow);
        
        for (col = 0; col < outpamP->width; ++col) {
            int const inputCol = (int) (col / xscale);
            
            pnm_assigntuple(inpamP, newtuplerow[col], tuplerow[inputCol]);
        }

        pnm_writepamrow(outpamP, newtuplerow);
    }
    pnm_freepamrow(tuplerow);
    pnm_freepamrow(newtuplerow);
}



int
main(int argc, char **argv ) {

    struct cmdlineInfo cmdline;
    FILE* ifP;
    struct pam inpam, outpam;
    float xscale, yscale;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFileName);

    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    outpam = inpam;  /* initial value */
    outpam.file = stdout;

    if (PNM_FORMAT_TYPE(inpam.format) == PBM_TYPE && !cmdline.nomix) {
        outpam.format = PGM_TYPE;
        outpam.maxval = PGM_MAXMAXVAL;
        pm_message("promoting from PBM to PGM");
    } else {
        outpam.format = inpam.format;
        outpam.maxval = inpam.maxval;
    }

    computeOutputDimensions(cmdline, inpam.height, inpam.width,
                            &outpam.height, &outpam.width);

    xscale = (float) outpam.width / inpam.width;
    yscale = (float) outpam.height / inpam.height;

    if (cmdline.verbose) {
        pm_message("Scaling by %f horizontally to %d columns.", 
                   xscale, outpam.width);
        pm_message("Scaling by %f vertically to %d rows.", 
                   yscale, outpam.height);
    }

    if (xscale * inpam.width < outpam.width - 1 ||
        yscale * inpam.height < outpam.height - 1) 
        pm_error("Arithmetic precision of this program is inadequate to "
                 "do the specified scaling.  Use a smaller input image "
                 "or a slightly different scale factor.");

    pnm_writepaminit(&outpam);

    if (cmdline.nomix) {
        if (cmdline.verbose)
            pm_message("Using nomix method");
        scaleWithoutMixing(&inpam, &outpam, xscale, yscale);
    } else if (!cmdline.filterFunction) {
        if (cmdline.verbose)
            pm_message("Using regular rescaling method");
        scaleWithMixing(&inpam, &outpam, xscale, yscale, 
                        cmdline.linear, cmdline.verbose);
    } else {
        if (cmdline.verbose)
            pm_message("Using general filter method");
        resample(&inpam, &outpam,
                 cmdline.filterFunction, cmdline.filterRadius,
                 cmdline.windowFunction, cmdline.verbose,
                 cmdline.linear);
    }
    pm_close(ifP);
    pm_close(stdout);
    
    return 0;
}
