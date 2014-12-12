/* +-------------------------------------------------------------------+ */
/* | Copyright 1992, David Koblas.                                     | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

/*

    DON'T ADD NEW FUNCTION TO THIS PROGRAM.  ADD IT TO pamcomp.c INSTEAD.

*/



#define _BSD_SOURCE    /* Make sure strcasecmp() is in string.h */
#include <string.h>

#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"

enum horizPos {BEYONDLEFT, LEFT, CENTER, RIGHT, BEYONDRIGHT};
enum vertPos {ABOVE, TOP, MIDDLE, BOTTOM, BELOW};


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *underlyingFilespec;  /* '-' if stdin */
    const char *overlayFilespec;
    const char *alphaFilespec;
    const char *outputFilespec;  /* '-' if stdout */
    int xoff, yoff;   /* value of xoff, yoff options */
    float opacity;
    unsigned int alphaInvert;
    enum horizPos align;
    enum vertPos valign;
};




static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
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

    char *align, *valign;
    unsigned int xoffSpec, yoffSpec, alignSpec, valignSpec, opacitySpec,
        alphaSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "invert",     OPT_FLAG,   NULL,                  
            &cmdlineP->alphaInvert,       0 );
    OPTENT3(0, "xoff",       OPT_INT,    &cmdlineP->xoff,       
            &xoffSpec,       0 );
    OPTENT3(0, "yoff",       OPT_INT,    &cmdlineP->yoff,       
            &yoffSpec,       0 );
    OPTENT3(0, "opacity",    OPT_FLOAT, &cmdlineP->opacity,
            &opacitySpec,       0 );
    OPTENT3(0, "alpha",      OPT_STRING, &cmdlineP->alphaFilespec,
            &alphaSpec,  0 );
    OPTENT3(0, "align",      OPT_STRING, &align,
            &alignSpec,  0 );
    OPTENT3(0, "valign",     OPT_STRING, &valign,
            &valignSpec,  0 );

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (!xoffSpec)
        cmdlineP->xoff = 0;
    if (!yoffSpec)
        cmdlineP->yoff = 0;
    if (!alphaSpec)
        cmdlineP->alphaFilespec = NULL;

    if (alignSpec) {
        if (strcasecmp(align, "BEYONDLEFT") == 0)
            cmdlineP->align = BEYONDLEFT;
        else if (strcasecmp(align, "LEFT") == 0)
            cmdlineP->align = LEFT;
        else if (strcasecmp(align, "CENTER") == 0)
            cmdlineP->align = CENTER;
        else if (strcasecmp(align, "RIGHT") == 0)
            cmdlineP->align = RIGHT;
        else if (strcasecmp(align, "BEYONDRIGHT") == 0)
            cmdlineP->align = BEYONDRIGHT;
        else
            pm_error("Invalid value for align option: '%s'.  Only LEFT, "
                     "RIGHT, CENTER, BEYONDLEFT, and BEYONDRIGHT are valid.", 
                     align);
    } else 
        cmdlineP->align = LEFT;

    if (valignSpec) {
        if (strcasecmp(valign, "ABOVE") == 0)
            cmdlineP->valign = ABOVE;
        else if (strcasecmp(valign, "TOP") == 0)
            cmdlineP->valign = TOP;
        else if (strcasecmp(valign, "MIDDLE") == 0)
            cmdlineP->valign = MIDDLE;
        else if (strcasecmp(valign, "BOTTOM") == 0)
            cmdlineP->valign = BOTTOM;
        else if (strcasecmp(valign, "BELOW") == 0)
            cmdlineP->valign = BELOW;
        else
            pm_error("Invalid value for valign option: '%s'.  Only TOP, "
                     "BOTTOM, MIDDLE, ABOVE, and BELOW are valid.", 
                     align);
    } else 
        cmdlineP->valign = TOP;

    if (!opacitySpec) 
        cmdlineP->opacity = 1.0;

    if (argc-1 < 1)
        pm_error("Need at least one argument: file specification of the "
                 "overlay image.");

    cmdlineP->overlayFilespec = argv[1];

    if (argc-1 >= 2)
        cmdlineP->underlyingFilespec = argv[2];
    else
        cmdlineP->underlyingFilespec = "-";

    if (argc-1 >= 3)
        cmdlineP->outputFilespec = argv[3];
    else
        cmdlineP->outputFilespec = "-";

    if (argc-1 > 3)
        pm_error("Too many arguments.  Only acceptable arguments are: "
                 "overlay image, underlying image, output image");
}




static void
warnOutOfFrame( int const originLeft,
                int const originTop, 
                int const overCols,
                int const overRows,
                int const underCols,
                int const underRows ) {
    if (originLeft >= underCols)
        pm_message("WARNING: the overlay is entirely off the right edge "
                   "of the underlying image.  "
                   "It will not be visible in the result.  The horizontal "
                   "overlay position you selected is %d, "
                   "and the underlying image "
                   "is only %d pixels wide.", originLeft, underCols );
    else if (originLeft + overCols <= 0)
        pm_message("WARNING: the overlay is entirely off the left edge "
                   "of the underlying image.  "
                   "It will not be visible in the result.  The horizontal "
                   "overlay position you selected is %d and the overlay is "
                   "only %d pixels wide.", originLeft, overCols);
    else if (originTop >= underRows)
        pm_message("WARNING: the overlay is entirely off the bottom edge "
                   "of the underlying image.  "
                   "It will not be visible in the result.  The vertical "
                   "overlay position you selected is %d, "
                   "and the underlying image "
                   "is only %d pixels high.", originTop, underRows );
    else if (originTop + overRows <= 0)
        pm_message("WARNING: the overlay is entirely off the top edge "
                   "of the underlying image.  "
                   "It will not be visible in the result.  The vertical "
                   "overlay position you selected is %d and the overlay is "
                   "only %d pixels high.", originTop, overRows);
}



static void
computeOverlayPosition(const int underCols, const int underRows,
                       const int overCols, const int overRows,
                       const struct cmdlineInfo cmdline, 
                       int * const originLeftP,
                       int * const originTopP) {
/*----------------------------------------------------------------------------
   Determine where to overlay the overlay image, based on the options the
   user specified and the realities of the image dimensions.

   The origin may be outside the underlying image (so e.g. *originLeftP may
   be negative or > image width).  That means not all of the overlay image
   actually gets used.  In fact, there may be no overlap at all.
-----------------------------------------------------------------------------*/
    int xalign, yalign;

    switch (cmdline.align) {
    case BEYONDLEFT:  xalign = -overCols;              break;
    case LEFT:        xalign = 0;                      break;
    case CENTER:      xalign = (underCols-overCols)/2; break;
    case RIGHT:       xalign = underCols - overCols;   break;
    case BEYONDRIGHT: xalign = underCols;              break;
    }
    switch (cmdline.valign) {
    case ABOVE:       yalign = -overRows;              break;
    case TOP:         yalign = 0;                      break;
    case MIDDLE:      yalign = (underRows-overRows)/2; break;
    case BOTTOM:      yalign = underRows - overRows;   break;
    case BELOW:       yalign = underRows;              break;
    }
    *originLeftP = xalign + cmdline.xoff;
    *originTopP  = yalign + cmdline.yoff;

    warnOutOfFrame( *originLeftP, *originTopP, 
                    overCols, overRows, underCols, underRows );    
}



static pixval
composeComponents(pixval const compA, 
                  pixval const compB,
                  float  const distrib,
                  pixval const maxval) {
/*----------------------------------------------------------------------------
  Compose a single component of each of two pixels, with 'distrib' being
  the fraction of 'compA' in the result, 1-distrib the fraction of 'compB'.
  
  Both inputs are based on a maxval of 'maxval', and so is our result.
  
  Note that while 'distrib' in the straightforward case is always in
  [0,1], it can in fact be negative or greater than 1.  We clip the
  result as required to return a legal pixval.
-----------------------------------------------------------------------------*/
    return MIN(maxval, MAX(0, (int)compA * distrib +
                              (int)compB * (1.0 - distrib) + 
                              0.5
                          )
              );
}



static pixel
composePixels(pixel  const pixelA,
              pixel  const pixelB,
              float  const distrib,
              pixval const maxval) {
/*----------------------------------------------------------------------------
  Compose two pixels 'pixelA' and 'pixelB', with 'distrib' being the
  fraction of 'pixelA' in the result, 1-distrib the fraction of 'pixelB'.

  Both inputs are based on a maxval of 'maxval', and so is our result.
  
  Note that while 'distrib' in the straightforward case is always in
  [0,1], it can in fact be negative or greater than 1.  We clip the
  result as required to return a legal pixval.
-----------------------------------------------------------------------------*/
    pixel retval;

    pixval const red = 
        composeComponents(PPM_GETR(pixelA), PPM_GETR(pixelB), distrib, maxval);
    pixval const grn =
        composeComponents(PPM_GETG(pixelA), PPM_GETG(pixelB), distrib, maxval);
    pixval const blu = 
        composeComponents(PPM_GETB(pixelA), PPM_GETB(pixelB), distrib, maxval);

    PPM_ASSIGN(retval, red, grn, blu);

    return retval;
}



static void
composite(int      const originleft, 
          int      const origintop, 
          pixel ** const overlayImage, 
          int      const overlayCols, 
          int      const overlayRows,
          xelval   const overlayMaxval, 
          int      const overlayType,
          int      const cols, 
          int      const rows, 
          xelval   const maxval, 
          int      const type,
          gray **  const alpha, 
          gray     const alphaMax, 
          bool     const invertAlpha,
          float    const opacity,
          FILE *   const ifp, 
          FILE *   const ofp) {
/*----------------------------------------------------------------------------
   Overlay the overlay image 'overlayImage' onto the underlying image
   which is in file 'ifp', and output the composite to file 'ofp'.

   The underlying image file 'ifp' is positioned after its header.  The
   width, height, format, and maxval of the underlying image are 'cols',
   'rows', 'type', and 'maxval'.

   The width, height, format, and maxval of the overlay image are
   overlayCols, overlayRows, overlayType and overlayMaxval.

   'originleft' and 'origintop' are the coordinates in the underlying
   image plane where the top left corner of the overlay image is
   to go.  It is not necessarily inside the underlying image (in fact,
   may be negative).  Only the part of the overlay that actually intersects
   the underlying image, if any, gets into the output.

   Note that we modify the overlay image 'overlayImage' to change its
   format and maxval to the format and maxval of the output.
-----------------------------------------------------------------------------*/
    /* otype and oxmaxv are the type and maxval for the composed (output)
       image, and are derived from that of the underlying and overlay
       images.
    */
    int    const otype = (overlayType < type) ? type : overlayType;
    xelval const omaxv = pm_lcm(maxval, overlayMaxval, 1, PNM_OVERALLMAXVAL);

    int     row;
    xel     *pixelrow;

    pixelrow = pnm_allocrow(cols);

    if (overlayType != otype || overlayMaxval != omaxv) {
        pnm_promoteformat(overlayImage, overlayCols, overlayRows,
                          overlayMaxval, overlayType, omaxv, otype);
    }

    pnm_writepnminit(ofp, cols, rows, omaxv, otype, 0);

    for (row = 0; row < rows; ++row) {
        int col;

        /* Read a row and convert it to the output type */
        pnm_readpnmrow(ifp, pixelrow, cols, maxval, type);

        if (type != otype || maxval != omaxv)
            pnm_promoteformatrow(pixelrow, cols, maxval, type, omaxv, otype);

        /* Now overlay the overlay with alpha (if defined) */
        for (col = 0; col < cols; ++col) {
            int const ovlcol = col - originleft;
            int const ovlrow = row - origintop;

            double overlayWeight;

            if (ovlcol >= 0 && ovlcol < overlayCols &&
                ovlrow >= 0 && ovlrow < overlayRows) {

                if (alpha == NULL) {
                    overlayWeight = opacity;
                } else {
                    double alphaval;
                    alphaval = 
                        (double)alpha[ovlrow][ovlcol] / (double)alphaMax;
                    if (invertAlpha)
                        alphaval = 1.0 - alphaval;
                    overlayWeight = alphaval * opacity;
                }

                pixelrow[col] = composePixels(overlayImage[ovlrow][ovlcol],
                                              pixelrow[col], 
                                              overlayWeight, omaxv);
            }
        }
        pnm_writepnmrow(ofp, pixelrow, cols, omaxv, otype, 0);
    }
    pnm_freerow(pixelrow);
}



int
main(int argc, char *argv[]) {

    FILE    *ifp, *ofp;
    pixel   **image;
    int     imageCols, imageRows, imageType;
    xelval  imageMax;
    int     cols, rows, type;
    xelval  maxval;
    gray    **alpha;
    int     alphaCols, alphaRows;
    xelval  alphaMax;
    struct cmdlineInfo cmdline;
    int originLeft, originTop;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
        
    { /* Read the overlay image into 'image' */
        FILE *fp;
        fp = pm_openr(cmdline.overlayFilespec);
        image = 
            pnm_readpnm(fp, &imageCols, &imageRows, &imageMax, &imageType);
        pm_close(fp);
    }
    if (cmdline.alphaFilespec) {
        /* Read the alpha mask file into 'alpha' */
        FILE *fp = pm_openr(cmdline.alphaFilespec);
        alpha = pgm_readpgm(fp, &alphaCols, &alphaRows, &alphaMax);
        pm_close(fp);
            
        if (imageCols != alphaCols || imageRows != alphaRows)
            pm_error("Alpha map and overlay image are not the same size");
    } else
        alpha = NULL;

    ifp = pm_openr(cmdline.underlyingFilespec);

    ofp = pm_openw(cmdline.outputFilespec);

    pnm_readpnminit(ifp, &cols, &rows, &maxval, &type);

    computeOverlayPosition(cols, rows, imageCols, imageRows, 
                           cmdline, &originLeft, &originTop);

    composite(originLeft, originTop,
              image, imageCols, imageRows, imageMax, imageType, 
              cols, rows, maxval, type, 
              alpha, alphaMax, cmdline.alphaInvert, cmdline.opacity,
              ifp, ofp);

    pm_close(ifp);
    pm_close(ofp);

    /* If the program failed, it previously aborted with nonzero completion
       code, via various function calls.
    */
    return 0;
}


