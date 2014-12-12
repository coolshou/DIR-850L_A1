/* ppmchange.c - change a given color to another
**
** Copyright (C) 1991 by Wilson H. Bent, Jr.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Modified by Alberto Accomazzi (alberto@cfa.harvard.edu).
**     28 Jan 94 -  Added multiple color substitution function.
*/

#include "ppm.h"
#include "shhopt.h"
#include "mallocvar.h"

#define TCOLS 256
#define SQRT3 1.73205080756887729352
    /* The square root of 3 */

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespecs of input files */
    int ncolors;      /* Number of valid entries in color0[], color1[] */
    char * oldcolorname[TCOLS];  /* colors user wants replaced */
    char * newcolorname[TCOLS];  /* colors with which he wants them replaced */
    int closeness;    
       /* -closeness option value.  Zero if no -closeness option */
    char * remainder_colorname;  
      /* Color user specified for -remainder.  Null pointer if he didn't
         specify -remainder.
      */
    unsigned int closeok;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    unsigned int closenessSpec, remainderSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "closeness",     OPT_UINT,
            &cmdlineP->closeness,           &closenessSpec,     0);
    OPTENT3(0, "remainder",     OPT_STRING,
            &cmdlineP->remainder_colorname, &remainderSpec,     0);
    OPTENT3(0, "closeok",       OPT_FLAG,
            NULL,                           &cmdlineP->closeok, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!remainderSpec)
        cmdlineP->remainder_colorname = NULL;

    if (!closenessSpec)
        cmdlineP->closeness = 0;

    if ((argc-1) % 2 == 0) 
        cmdlineP->input_filespec = "-";
    else
        cmdlineP->input_filespec = argv[argc-1];

    {
        int argn;
        cmdlineP->ncolors = 0;  /* initial value */
        for (argn = 1; 
             argn+1 < argc && cmdlineP->ncolors < TCOLS; 
             argn += 2) {
            cmdlineP->oldcolorname[cmdlineP->ncolors] = argv[argn];
            cmdlineP->newcolorname[cmdlineP->ncolors] = argv[argn+1];
            cmdlineP->ncolors++;
        }
    }
}



static double
sqrf(float const F) {
    return F*F;
}



static int 
colormatch(pixel const comparand, 
           pixel const comparator, 
           float const closeness) {
/*----------------------------------------------------------------------------
   Return true iff 'comparand' matches 'comparator' in color within the
   fuzz factor 'closeness'.
-----------------------------------------------------------------------------*/
    /* Fast path for usual case */
    if (closeness == 0)
        return PPM_EQUAL(comparand, comparator);

    return PPM_DISTANCE(comparand, comparator) <= sqrf(closeness);
}



static void
changeRow(const pixel * const inrow, 
          pixel *       const outrow, 
          int           const cols,
          int           const ncolors, 
          const pixel         colorfrom[], 
          const pixel         colorto[],
          bool          const remainder_specified, 
          pixel         const remainder_color, 
          float         const closeness) {
/*----------------------------------------------------------------------------
   Replace the colors in a single row.  There are 'ncolors' colors to 
   replace.  The to-replace colors are in the array colorfrom[], and the
   replace-with colors are in corresponding elements of colorto[].
   Iff 'remainder_specified' is true, replace all colors not mentioned
   in colorfrom[] with 'remainder_color'.  Use the closeness factor
   'closeness' in determining if something in the input row matches
   a color in colorfrom[].

   The input row is 'inrow'.  The output is returned as 'outrow', in
   storage which must be already allocated.  Both are 'cols' columns wide.
-----------------------------------------------------------------------------*/
    int col;

    for (col = 0; col < cols; ++col) {
        int i;
        int have_match; /* logical: It's a color user said to change */
        pixel newcolor;  
        /* Color to which we must change current pixel.  Undefined unless
           'have_match' is true.
        */

        have_match = FALSE;  /* haven't found a match yet */
        for (i = 0; i < ncolors && !have_match; ++i) {
            have_match = colormatch(inrow[col], colorfrom[i], closeness);
            newcolor = colorto[i];
        }
        if (have_match)
            outrow[col] = newcolor;
        else if (remainder_specified)
            outrow[col] = remainder_color;
        else
            outrow[col] = inrow[col];
    }
}



int
main(int argc, char *argv[]) {
    struct cmdlineInfo cmdline;
    FILE * ifP;
    int format;
    int rows, cols;
    pixval maxval;
    float closeness;
    int row;
    pixel* inrow;
    pixel* outrow;

    pixel oldcolor[TCOLS];  /* colors user wants replaced */
    pixel newcolor[TCOLS];  /* colors with which he wants them replaced */
    pixel remainder_color;
      /* Color user specified for -remainder.  Undefined if he didn't
         specify -remainder.
      */

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
    
    ifP = pm_openr(cmdline.input_filespec);

    ppm_readppminit(ifP, &cols, &rows, &maxval, &format);

    if (cmdline.remainder_colorname)
        remainder_color = ppm_parsecolor2(cmdline.remainder_colorname, maxval,
                                          cmdline.closeok);
    { 
        int i;
        for (i = 0; i < cmdline.ncolors; ++i) {
            oldcolor[i] = ppm_parsecolor2(cmdline.oldcolorname[i], maxval,
                                          cmdline.closeok);
            newcolor[i] = ppm_parsecolor2(cmdline.newcolorname[i], maxval,
                                          cmdline.closeok);
        }
    }
    closeness = SQRT3 * maxval * cmdline.closeness/100;

    ppm_writeppminit( stdout, cols, rows, maxval, 0 );
    inrow = ppm_allocrow(cols);
    outrow = ppm_allocrow(cols);

    /* Scan for the desired color */
    for (row = 0; row < rows; row++) {
        ppm_readppmrow(ifP, inrow, cols, maxval, format);

        changeRow(inrow, outrow, cols, cmdline.ncolors, oldcolor, newcolor,
                  cmdline.remainder_colorname != NULL,
                  remainder_color, closeness);

        ppm_writeppmrow(stdout, outrow, cols, maxval, 0);
    }

    pm_close(ifP);

    return 0;
}
