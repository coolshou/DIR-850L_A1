/*----------------------------------------------------------------------------
                              pamcomp
-----------------------------------------------------------------------------
   This program composes two images together, with optional translucence.

   This program is derived from (and replaces) Pnmcomp, whose origin is
   as follows:

       Copyright 1992, David Koblas.                                    
         Permission to use, copy, modify, and distribute this software  
         and its documentation for any purpose and without fee is hereby
         granted, provided that the above copyright notice appear in all
         copies and that both that copyright notice and this permission 
         notice appear in supporting documentation.  This software is   
         provided "as is" without express or implied warranty.          

   No code from the original remains in the present version.  The
   January 2004 version was coded entirely by Bryan Henderson.
   Bryan has contributed his work to the public domain.

   The current version is derived from the January 2004 version, with
   additional work by multiple authors.
-----------------------------------------------------------------------------*/

#define _BSD_SOURCE    /* Make sure strcasecmp() is in string.h */
#include <string.h>
#include <math.h>

#include "pam.h"
#include "pm_gamma.h"
#include "shhopt.h"
#include "mallocvar.h"

enum horizPos {BEYONDLEFT, LEFT, CENTER, RIGHT, BEYONDRIGHT};
enum vertPos {ABOVE, TOP, MIDDLE, BOTTOM, BELOW};


enum sampleScale {INTENSITY_SAMPLE, GAMMA_SAMPLE};
/* This indicates a scale for a PAM sample value.  INTENSITY_SAMPLE means
   the value is proportional to light intensity; GAMMA_SAMPLE means the 
   value is gamma-adjusted as defined in the PGM/PPM spec.  In both
   scales, the values are continuous and normalized to the range 0..1.

   This scale has no meaning if the PAM is not a visual image.  
*/

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
    unsigned int linear;
};



static void
parseCommandLine(int                        argc, 
                 char **                    argv,
                 struct cmdlineInfo * const cmdlineP ) {
/*----------------------------------------------------------------------------
   Parse program command line described in Unix standard form by argc
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
            &cmdlineP->alphaInvert,       0);
    OPTENT3(0, "xoff",       OPT_INT,    &cmdlineP->xoff,       
            &xoffSpec,                    0);
    OPTENT3(0, "yoff",       OPT_INT,    &cmdlineP->yoff,       
            &yoffSpec,                    0);
    OPTENT3(0, "opacity",    OPT_FLOAT, &cmdlineP->opacity,
            &opacitySpec,                 0);
    OPTENT3(0, "alpha",      OPT_STRING, &cmdlineP->alphaFilespec,
            &alphaSpec,                   0);
    OPTENT3(0, "align",      OPT_STRING, &align,
            &alignSpec,                   0);
    OPTENT3(0, "valign",     OPT_STRING, &valign,
            &valignSpec,                  0);
    OPTENT3(0, "linear",     OPT_FLAG,    NULL,       
            &cmdlineP->linear,            0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
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




static int
commonFormat(int const formatA,
             int const formatB) {
/*----------------------------------------------------------------------------
   Return a viable format for the result of composing the two formats
   'formatA' and 'formatB'.
-----------------------------------------------------------------------------*/
    int retval;

    int const typeA = PAM_FORMAT_TYPE(formatA);
    int const typeB = PAM_FORMAT_TYPE(formatB);
    
    if (typeA == PAM_TYPE || typeB == PAM_TYPE)
        retval = PAM_FORMAT;
    else if (typeA == PPM_TYPE || typeB == PPM_TYPE)
        retval = PPM_FORMAT;
    else if (typeA == PGM_TYPE || typeB == PGM_TYPE)
        retval = PGM_FORMAT;
    else if (typeA == PBM_TYPE || typeB == PBM_TYPE)
        retval = PBM_FORMAT;
    else {
        /* Results are undefined for this case, so we do a hail Mary. */
        retval = formatA;
    }
    return retval;
}



static void
commonTupletype(const char * const tupletypeA, 
                const char * const tupletypeB, 
                char *       const tupletypeOut,
                unsigned int const size) {

    if (strncmp(tupletypeA, "RGB", 3) == 0 ||
        strncmp(tupletypeB, "RGB", 3) == 0)
        strncpy(tupletypeOut, "RGB", size);
    else if (strncmp(tupletypeA, "GRAYSCALE", 9) == 0 ||
        strncmp(tupletypeB, "GRAYSCALE", 9) == 0)
        strncpy(tupletypeOut, "GRAYSCALE", size);
    else if (strncmp(tupletypeA, "BLACKANDWHITE", 13) == 0 ||
        strncmp(tupletypeB, "BLACKANDWHITE", 13) == 0)
        strncpy(tupletypeOut, "BLACKANDWHITE", size);
    else
        /* Results are undefined for this case, so we do a hail Mary. */
        strncpy(tupletypeOut, tupletypeA, size);
}



static void
determineOutputType(struct pam * const composedPamP,
                    struct pam * const underlayPamP,
                    struct pam * const overlayPamP) {

    composedPamP->height = underlayPamP->height;
    composedPamP->width  = underlayPamP->width;

    composedPamP->format = commonFormat(underlayPamP->format, 
                                        overlayPamP->format);
    composedPamP->plainformat = FALSE;
    commonTupletype(underlayPamP->tuple_type, overlayPamP->tuple_type,
                    composedPamP->tuple_type, 
                    sizeof(composedPamP->tuple_type));

    composedPamP->maxval = pm_lcm(underlayPamP->maxval, overlayPamP->maxval, 
                                  1, PNM_OVERALLMAXVAL);

    if (strcmp(composedPamP->tuple_type, "RGB") == 0)
        composedPamP->depth = 3;
    else if (strcmp(composedPamP->tuple_type, "GRAYSCALE") == 0)
        composedPamP->depth = 1;
    else if (strcmp(composedPamP->tuple_type, "BLACKANDWHITE") == 0)
        composedPamP->depth = 1;
    else
        /* Results are undefined for this case, so we just do something safe */
        composedPamP->depth = MIN(underlayPamP->depth, overlayPamP->depth);
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
validateComputableHeight(int const originTop, 
                         int const overRows) {

    if (originTop < 0) {
        if (originTop < -INT_MAX)
            pm_error("Overlay starts too far above the underlay image to be "
                     "computable.  Overlay can be at most %d rows above "
                     "the underlay.", INT_MAX);
    } else {
        if (INT_MAX - originTop <= overRows)
            pm_error("Too many total rows involved to be computable.  "
                     "You must have a shorter overlay image or compose it "
                     "higher on the underlay image.");
    }
}



static void
computeOverlayPosition(int                const underCols, 
                       int                const underRows,
                       int                const overCols, 
                       int                const overRows,
                       struct cmdlineInfo const cmdline, 
                       int *              const originLeftP,
                       int *              const originTopP) {
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

    validateComputableHeight(*originTopP, overRows);

    warnOutOfFrame(*originLeftP, *originTopP, 
                   overCols, overRows, underCols, underRows);    
}



static sample
composeComponents(sample           const compA, 
                  sample           const compB,
                  float            const distrib,
                  sample           const maxval,
                  enum sampleScale const sampleScale) {
/*----------------------------------------------------------------------------
  Compose a single component of each of two pixels, with 'distrib' being
  the fraction of 'compA' in the result, 1-distrib the fraction of 'compB'.
  
  The inputs and result are based on a maxval of 'maxval'.
  
  Note that while 'distrib' in the straightforward case is always in
  [0,1], it can in fact be negative or greater than 1.  We clip the
  result as required to return a legal sample value.
-----------------------------------------------------------------------------*/
    sample retval;

    if (fabs(1.0-distrib) < .001)
        /* Fast path for common case */
        retval = compA;
    else {
        if (sampleScale == INTENSITY_SAMPLE) {
            sample const mix = 
                ROUNDU(compA * distrib + compB * (1.0 - distrib));
            retval = MIN(maxval, MAX(0, mix));
        } else {
            float const compANormalized = (float)compA/maxval;
            float const compBNormalized = (float)compB/maxval;
            float const compALinear = pm_ungamma709(compANormalized);
            float const compBLinear = pm_ungamma709(compBNormalized);
            float const mix = 
                compALinear * distrib + compBLinear * (1.0 - distrib);
            sample const sampleValue = ROUNDU(pm_gamma709(mix) * maxval);
            retval = MIN(maxval, MAX(0, sampleValue));
        }
    }
    return retval;
}



static void
overlayPixel(tuple            const overlayTuple,
             struct pam *     const overlayPamP,
             tuple            const underlayTuple,
             struct pam *     const underlayPamP,
             tuplen           const alphaTuplen,
             bool             const invertAlpha,
             bool             const overlayHasOpacity,
             unsigned int     const opacityPlane,
             tuple            const composedTuple,
             struct pam *     const composedPamP,
             float            const masterOpacity,
             enum sampleScale const sampleScale) {

    float overlayWeight;

    overlayWeight = masterOpacity;  /* initial value */
    
    if (overlayHasOpacity)
        overlayWeight *= (float)
            overlayTuple[opacityPlane] / overlayPamP->maxval;
    
    if (alphaTuplen) {
        float const alphaval = 
            invertAlpha ? (1.0 - alphaTuplen[0]) : alphaTuplen[0];
        overlayWeight *= alphaval;
    }

    {
        unsigned int plane;
        
        for (plane = 0; plane < composedPamP->depth; ++plane)
            composedTuple[plane] = 
                composeComponents(overlayTuple[plane], underlayTuple[plane], 
                                  overlayWeight,
                                  composedPamP->maxval,
                                  sampleScale);
    }
}



static void
adaptRowToOutputFormat(struct pam * const inpamP,
                       tuple *      const tuplerow,
                       struct pam * const outpamP) {
/*----------------------------------------------------------------------------
   Convert the row in 'tuplerow', which is in a format described by
   *inpamP, to the format described by *outpamP.

   'tuplerow' must have enough allocated depth to do this.
-----------------------------------------------------------------------------*/
    pnm_scaletuplerow(inpamP, tuplerow, tuplerow, outpamP->maxval);

    if (strncmp(outpamP->tuple_type, "RGB", 3) == 0)
        pnm_makerowrgb(inpamP, tuplerow);
}



static void
composite(int          const originleft, 
          int          const origintop, 
          struct pam * const underlayPamP,
          struct pam * const overlayPamP,
          struct pam * const alphaPamP,
          bool         const invertAlpha,
          float        const masterOpacity,
          struct pam * const composedPamP,
          bool         const assumeLinear) {
/*----------------------------------------------------------------------------
   Overlay the overlay image in the array 'overlayImage', described by
   *overlayPamP, onto the underlying image from the input image file
   as described by *underlayPamP, output the composite to the image
   file as described by *composedPamP.

   Apply the overlay image with transparency described by the array
   'alpha' and *alphaPamP.

   The underlying image is positioned after its header.

   'originleft' and 'origintop' are the coordinates in the underlying
   image plane where the top left corner of the overlay image is to
   go.  It is not necessarily inside the underlying image (in fact,
   may be negative).  Only the part of the overlay that actually
   intersects the underlying image, if any, gets into the output.
-----------------------------------------------------------------------------*/
    enum sampleScale const sampleScale = 
        assumeLinear ? INTENSITY_SAMPLE : GAMMA_SAMPLE;

    int underlayRow;  /* NB may be negative */
    int overlayRow;   /* NB may be negative */
    tuple * composedTuplerow;
    tuple * underlayTuplerow;
    tuple * overlayTuplerow;
    tuplen * alphaTuplerown;
    bool overlayHasOpacity;
    unsigned int opacityPlane;

    pnm_getopacity(overlayPamP, &overlayHasOpacity, &opacityPlane);

    composedTuplerow = pnm_allocpamrow(composedPamP);
    underlayTuplerow = pnm_allocpamrow(underlayPamP);
    overlayTuplerow  = pnm_allocpamrow(overlayPamP);
    if (alphaPamP)
        alphaTuplerown = pnm_allocpamrown(alphaPamP);

    pnm_writepaminit(composedPamP);

    for (underlayRow = MIN(0, origintop), overlayRow = MIN(0, -origintop);
         underlayRow < MAX(underlayPamP->height, 
                           origintop + overlayPamP->height);
         ++underlayRow, ++overlayRow) {

        if (overlayRow >= 0 && overlayRow < overlayPamP->height) {
            pnm_readpamrow(overlayPamP, overlayTuplerow);
            adaptRowToOutputFormat(overlayPamP, overlayTuplerow, composedPamP);
            if (alphaPamP)
                pnm_readpamrown(alphaPamP, alphaTuplerown);
        }
        if (underlayRow >= 0 && underlayRow < underlayPamP->height) {
            pnm_readpamrow(underlayPamP, underlayTuplerow);
            adaptRowToOutputFormat(underlayPamP, underlayTuplerow, 
                                   composedPamP);

            if (underlayRow < origintop || 
                underlayRow >= origintop + overlayPamP->height) {
            
                /* Overlay image does not touch this underlay row. */

                pnm_writepamrow(composedPamP, underlayTuplerow);
            } else {
                unsigned int col;
                for (col = 0; col < composedPamP->width; ++col) {
                    int const ovlcol = col - originleft;

                    if (ovlcol >= 0 && ovlcol < overlayPamP->width) {
                        tuplen const alphaTuplen = 
                            alphaPamP ? alphaTuplerown[ovlcol] : NULL;

                        overlayPixel(overlayTuplerow[ovlcol], overlayPamP,
                                     underlayTuplerow[col], underlayPamP,
                                     alphaTuplen, invertAlpha,
                                     overlayHasOpacity, opacityPlane,
                                     composedTuplerow[col], composedPamP,
                                     masterOpacity, sampleScale);
                    } else
                        /* Overlay image does not touch this column. */
                        pnm_assigntuple(composedPamP, composedTuplerow[col],
                                        underlayTuplerow[col]);
                }
                pnm_writepamrow(composedPamP, composedTuplerow);
            }
        }
    }
    pnm_freepamrow(composedTuplerow);
    pnm_freepamrow(underlayTuplerow);
    pnm_freepamrow(overlayTuplerow);
    if (alphaPamP)
        pnm_freepamrown(alphaTuplerown);
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE * underlayFileP;
    FILE * overlayFileP;
    FILE * alphaFileP;
    struct pam underlayPam;
    struct pam overlayPam;
    struct pam alphaPam;
    struct pam composedPam;
    int originLeft, originTop;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    overlayFileP = pm_openr(cmdline.overlayFilespec);
    pnm_readpaminit(overlayFileP, &overlayPam, 
                    PAM_STRUCT_SIZE(allocation_depth));
    if (cmdline.alphaFilespec) {
        alphaFileP = pm_openr(cmdline.alphaFilespec);
        pnm_readpaminit(alphaFileP, &alphaPam, 
                        PAM_STRUCT_SIZE(allocation_depth));

        if (overlayPam.width != alphaPam.width || 
            overlayPam.height != overlayPam.height)
            pm_error("Opacity map and overlay image are not the same size");
    } else
        alphaFileP = NULL;

    underlayFileP = pm_openr(cmdline.underlyingFilespec);

    pnm_readpaminit(underlayFileP, &underlayPam, 
                    PAM_STRUCT_SIZE(allocation_depth));

    computeOverlayPosition(underlayPam.width, underlayPam.height, 
                           overlayPam.width,  overlayPam.height,
                           cmdline, &originLeft, &originTop);

    composedPam.size             = sizeof(composedPam);
    composedPam.len              = PAM_STRUCT_SIZE(allocation_depth);
    composedPam.allocation_depth = 0;
    composedPam.file             = pm_openw(cmdline.outputFilespec);

    determineOutputType(&composedPam, &underlayPam, &overlayPam);

    pnm_setminallocationdepth(&underlayPam, composedPam.depth);
    pnm_setminallocationdepth(&overlayPam,  composedPam.depth);
    
    composite(originLeft, originTop,
              &underlayPam, &overlayPam, alphaFileP ? &alphaPam : NULL,
              cmdline.alphaInvert, cmdline.opacity,
              &composedPam, cmdline.linear);

    if (alphaFileP)
        pm_close(alphaFileP);
    pm_close(overlayFileP);
    pm_close(underlayFileP);
    pm_close(composedPam.file);

    /* If the program failed, it previously aborted with nonzero completion
       code, via various function calls.
    */
    return 0;
}
