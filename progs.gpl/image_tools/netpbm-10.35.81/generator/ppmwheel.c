/* ppmwheel.c - create a color circle of a specified size
**
** This was adapted by Bryan Henderson in January 2003 from ppmcirc.c by
** Peter Kirchgessner:
**
** Copyright (C) 1995 by Peter Kirchgessner.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <string.h>
#include <math.h>

#include "ppm.h"

#ifndef PI
#define PI  3.14159265358979323846
#endif

#ifndef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif

static void 
hsv_rgb(double const in_h, double const in_s, double const in_v, 
        double * const r, double * const g, double * const b) {
/*----------------------------------------------------------------------------
   This is a stripped down hsv->rgb converter that works only for
   Saturation of zero.
-----------------------------------------------------------------------------*/
    double h, s, v;

    h = in_h < 0.0 ? 0.0 : in_h > 360.0 ? 360.0 : in_h;

    v = in_v < 0.0 ? 0.0 : in_v > 1.0 ? 1.0 : in_v;

    s = in_s < 0.0 ? 0.0 : in_s > 1.0 ? 1.0 : in_s;

    if (s != 0.0)
        pm_error("Internal error: non-zero saturation");

    if (h <= 60.0) {          /* from red to yellow */
        *r = 1.0;
        *g = h / 60.0;
        *b = 0.0;
    } else if ( h <= 120.0 ) {   /* from yellow to green */
        *r = 1.0 - (h - 60.0) / 60.0;
        *g = 1.0;
        *b = 0.0;
    } else if ( h <= 180.0 ) {   /* from green to cyan */
        *r = 0.0;
        *g = 1.0;
        *b = (h - 120.0) / 60.0;
    } else if ( h <= 240.0 ) {    /* from cyan to blue */
        *r = 0.0;
        *g = 1.0 - (h - 180.0) / 60.0;
        *b = 1.0;
    } else if ( h <= 300.0) {    /* from blue to magenta */
        *r = (h - 240.0) / 60.0;
        *g = 0.0;
        *b = 1.0;
    } else {                      /* from magenta to red */
        *r = 1.0;
        *g = 0.0;
        *b = 1.0 - (h - 300.0) / 60.0;
    }

    if ( v >= 0.5) {
        v = 2.0 - 2.0 * v;
        v = sqrt (v);
        *r = 1.0 + v * (*r - 1.0);
        *g = 1.0 + v * (*g - 1.0);
        *b = 1.0 + v * (*b - 1.0);
    } else {
        v *= 2.0;
        v = sqrt (sqrt ( sqrt (v)));
        *r *= v;
        *g *= v;
        *b *= v;
    }
}


int
main(int argc, char *argv[]) {
    pixel *orow;
    int rows, cols;
    pixval maxval;
    unsigned int row;
    unsigned int xcenter, ycenter, radius;
    long diameter;
    char * tailptr;

    ppm_init( &argc, argv );

    if (argc-1 != 1)
        pm_error("Program takes one argument:  diameter of color wheel");

    diameter = strtol(argv[1], &tailptr, 10);
    if (strlen(argv[1]) == 0 || *tailptr != '\0')
        pm_error("You specified an invalid diameter: '%s'", argv[1]);
    if (diameter <= 0)
        pm_error("Diameter must be positive.  You specified %ld.", diameter);
    if (diameter < 4)
        pm_error("Diameter must be at least 4.  You specified %ld", diameter);

    cols = rows = diameter;
    
    orow = ppm_allocrow(cols);

    maxval = PPM_MAXMAXVAL;
    ppm_writeppminit(stdout, cols, rows, maxval, 0);

    radius = diameter/2 - 1;

    xcenter = cols / 2;
    ycenter = rows / 2;

    for (row = 0; row < rows; ++row) {
        unsigned int col;
        for (col = 0; col < cols; ++col) {
            double const dx = (int)col - (int)xcenter;
            double const dy = (int)row - (int)ycenter;
            double const dist = sqrt(dx*dx + dy*dy);

            pixval r, g, b;

            if (dist > radius) {
                r = g = b = maxval;
            } else {
                double hue, sat, val;
                double dr, dg, db;

                hue = atan2(dx, dy) / PI * 180.0;
                if (hue < 0.0) 
                    hue = 360.0 + hue;
                sat = 0.0;
                val = dist / radius;

                hsv_rgb(hue, sat, val, &dr, &dg, &db);

                r = (pixval)(maxval * dr);
                g = (pixval)(maxval * dg);
                b = (pixval)(maxval * db);
            }
            PPM_ASSIGN (orow[col], r, g, b );
        }
        ppm_writeppmrow(stdout, orow, cols, maxval, 0);
    }
    pm_close(stdout);
    exit(0);
}
