/* ppmbrighten.c - allow user control over Value and Saturation of PPM file
**
** Copyright (C) 1989 by Jef Poskanzer.
** Copyright (C) 1990 by Brian Moffet.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "ppm.h"
#include "shhopt.h"
#include "mallocvar.h"

#define MULTI   1000

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* '-' if stdin */
    float saturation;
    float value;
    unsigned int normalize;
};




static void
parseCommandLine (int argc, char ** argv,
                  struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int saturationSpec, valueSpec;
    int saturationOpt, valueOpt;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "saturation",  OPT_INT,    &saturationOpt,
            &saturationSpec,      0 );
    OPTENT3(0, "value",       OPT_INT,    &valueOpt,
            &valueSpec,           0 );
    OPTENT3(0, "normalize",   OPT_FLAG,   NULL,
            &cmdlineP->normalize, 0 );


    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */
    
    if (saturationSpec) {
        if (saturationOpt < -100)
            pm_error("Saturation reduction cannot be more than 100%%.  "
                     "You specified %d", saturationOpt);
        else
            cmdlineP->saturation = 1.0 + (float)saturationOpt / 100;
    } else
        cmdlineP->saturation = 1.0;

    if (valueSpec) {
        if (valueOpt < -100)
            pm_error("Value reduction cannot be more than 100%%.  "
                     "You specified %d", valueOpt);
        else
            cmdlineP->value = 1.0 + (float)valueOpt / 100;
    } else
        cmdlineP->value = 1.0;

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else
        pm_error("Program takes at most one argument:  file specification");
}



static __inline__ unsigned int
mod(int const dividend, unsigned int const divisor) {

    int remainder = dividend % divisor;

    if (remainder < 0)
        return divisor + remainder;
    else 
        return (unsigned int) remainder;
}



static void 
RGBtoHSV(pixel          const color,
         pixval         const maxval,
         unsigned int * const hP, 
         unsigned int * const sP, 
         unsigned int * const vP) {

    unsigned int const R = (MULTI * PPM_GETR(color) + maxval - 1) / maxval;
    unsigned int const G = (MULTI * PPM_GETG(color) + maxval - 1) / maxval;
    unsigned int const B = (MULTI * PPM_GETB(color) + maxval - 1) / maxval;

    unsigned int s, v;
    unsigned int t;
    unsigned int sector;

    v = MAX(R, MAX(G, B));

    t = MIN(R, MIN(G, B));

    if (v == 0)
        s = 0;
    else
        s = ((v - t)*MULTI)/v;

    if (s == 0)
        sector = 0;
    else {
        unsigned int const cr = (MULTI * (v - R))/(v - t);
        unsigned int const cg = (MULTI * (v - G))/(v - t);
        unsigned int const cb = (MULTI * (v - B))/(v - t);

        if (R == v)
            sector = mod((int)(cb - cg), 6*MULTI);
        else if (G == v)
            sector = mod((int)((2*MULTI) + cr - cb), 6*MULTI);
        else if (B == v)
            sector = mod((int)((4*MULTI) + cg - cr), 6*MULTI);
        else
            pm_error("Internal error: neither r, g, nor b is maximum");
    }

    *hP = sector * 60;
    *sP = s;
    *vP = v;
}



static void
HSVtoRGB(unsigned int   const h, 
         unsigned int   const s, 
         unsigned int   const v, 
         pixval         const maxval,
         pixel *        const colorP) {
    
    unsigned int R, G, B;

    if (s == 0) {
        R = v;
        G = v;
        B = v;
    } else {
        unsigned int const sectorSize = 60 * MULTI;
            /* Color wheel is divided into six 60 degree sectors. */
        unsigned int const sector = (h/sectorSize);
            /* The sector in which our color resides.  Value is in 0..5 */
        unsigned int const f = (h - sector*sectorSize)/60;
            /* The fraction of the way the color is from one side of
               our sector to the other side, going clockwise.  Value is
               in [0, MULTI).
            */
        unsigned int const m = (v * (MULTI - s)) / MULTI;
        unsigned int const n = (v * (MULTI - (s * f)/MULTI)) / MULTI;
        unsigned int const k = (v * (MULTI - (s * (MULTI - f))/MULTI)) / MULTI;

        switch (sector) {
        case 0:
            R = v;
            G = k;
            B = m;
            break;
        case 1:
            R = n;
            G = v;
            B = m;
            break;
        case 2:
            R = m;
            G = v;
            B = k;
            break;
        case 3:
            R = m;
            G = n;
            B = v;
            break;
        case 4:
            R = k;
            G = m;
            B = v;
            break;
        case 5:
            R = v;
            G = m;
            B = n;
            break;
        default:
            pm_error("Invalid H value passed to HSVtoRGB: %u/%u", h, MULTI);
        }
    }
    PPM_ASSIGN(*colorP, 
               (R * maxval) / MULTI,
               (G * maxval) / MULTI,
               (B * maxval) / MULTI);
}



static void
getMinMax(FILE *         const ifP,
          int            const cols,
          int            const rows,
          pixval         const maxval,
          int            const format,
          unsigned int * const minValueP,
          unsigned int * const maxValueP) {

    pixel * pixelrow;
    unsigned int minValue, maxValue;
    int row;

    pixelrow = ppm_allocrow(cols);

    maxValue = 0;
    minValue = MULTI;
    for (row = 0; row < rows; ++row) {
        unsigned int col;
        ppm_readppmrow(ifP, pixelrow, cols, maxval, format);
        for (col = 0; col < cols; ++col) {
            unsigned int H, S, V;

            RGBtoHSV(pixelrow[col], maxval, &H, &S, &V);
            maxValue = MAX(maxValue, V);
            minValue = MIN(minValue, V);
        }
    }
    ppm_freerow(pixelrow);

    *minValueP = minValue;
    *maxValueP = maxValue;
}



int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE *ifP;
    pixval minValue, maxValue;
    pixel *pixelrow;
    pixval maxval;
    int rows, cols, format, row;

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    if (cmdline.normalize)
        ifP = pm_openr_seekable(cmdline.inputFilespec);
    else
        ifP = pm_openr(cmdline.inputFilespec);

    ppm_readppminit(ifP, &cols, &rows, &maxval, &format);

    if (cmdline.normalize) {
        pm_filepos rasterPos;
        pm_tell2(ifP, &rasterPos, sizeof(rasterPos));
        getMinMax(ifP, cols, rows, maxval, format, &minValue, &maxValue);
        pm_seek2(ifP, &rasterPos, sizeof(rasterPos));
        pm_message("Minimum value %u%% of full intensity "
                   "being remapped to zero.",
                   (minValue*100+MULTI/2)/MULTI);
        pm_message("Maximum value %u%% of full intensity "
                   "being remapped to full.",
                   (maxValue*100+MULTI/2)/MULTI);
    }

    pixelrow = ppm_allocrow(cols);

    ppm_writeppminit(stdout, cols, rows, maxval, 0);

    for (row = 0; row < rows; ++row) {
        unsigned int col;
        ppm_readppmrow(ifP, pixelrow, cols, maxval, format);
        for (col = 0; col < cols; ++col) {
            unsigned int H, S, V;

            RGBtoHSV(pixelrow[col], maxval, &H, &S, &V);
            
            if (cmdline.normalize) {
                V -= minValue;
                V = (V * MULTI) /
                    (MULTI - (minValue+MULTI-maxValue));
            }

            S = MIN(MULTI, (unsigned int) (S * cmdline.saturation + 0.5));
            V = MIN(MULTI, (unsigned int) (V * cmdline.value + 0.5));

            HSVtoRGB(H, S, V, maxval, &pixelrow[col]);
        }

        ppm_writeppmrow(stdout, pixelrow, cols, maxval, 0);
    }
    ppm_freerow(pixelrow);

    pm_close(ifP);

    /* If the program failed, it previously aborted with nonzero completion
       code, via various function calls.
    */
    return 0;
}
