/******************************************************************************
                               pnmremap.c
*******************************************************************************

  Replace colors in an input image with colors from a given colormap image.

  For PGM input, do the equivalent.

  By Bryan Henderson, San Jose, CA 2001.12.17

  Derived from ppmquant, originally by Jef Poskanzer.

  Copyright (C) 1989, 1991 by Jef Poskanzer.
  Copyright (C) 2001 by Bryan Henderson.

  Permission to use, copy, modify, and distribute this software and its
  documentation for any purpose and without fee is hereby granted, provided
  that the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.  This software is provided "as is" without express or
  implied warranty.
******************************************************************************/

#include <limits.h>
#include <math.h>
#include <assert.h>

#include "pm_c_util.h"
#include "mallocvar.h"
#include "nstring.h"
#include "shhopt.h"
#include "pam.h"
#include "pammap.h"

#define MAXCOLORS 32767u

enum missingMethod {
    MISSING_FIRST,
    MISSING_SPECIFIED,
    MISSING_CLOSE
};

#define FS_SCALE 1024

struct fserr {
    long** thiserr;
    long** nexterr;
    bool fsForward;
};


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilespec;  /* Filespec of input file */
    const char * mapFilespec;    /* Filespec of colormap file */
    unsigned int floyd;   /* Boolean: -floyd/-fs option */
    enum missingMethod missingMethod;
    char * missingcolor;      
        /* -missingcolor value.  Null if not specified */
    unsigned int verbose;
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
    optEntry * option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int nofloyd, firstisdefault;
    unsigned int missingSpec, mapfileSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);
    
    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "floyd",        OPT_FLAG,   
            NULL,                       &cmdlineP->floyd, 0);
    OPTENT3(0,   "fs",           OPT_FLAG,   
            NULL,                       &cmdlineP->floyd, 0);
    OPTENT3(0,   "nofloyd",      OPT_FLAG,   
            NULL,                       &nofloyd, 0);
    OPTENT3(0,   "nofs",         OPT_FLAG,   
            NULL,                       &nofloyd, 0);
    OPTENT3(0,   "firstisdefault", OPT_FLAG,   
            NULL,                       &firstisdefault, 0);
    OPTENT3(0,   "mapfile",      OPT_STRING, 
            &cmdlineP->mapFilespec,    &mapfileSpec, 0);
    OPTENT3(0,   "missingcolor", OPT_STRING, 
            &cmdlineP->missingcolor,   &missingSpec, 0);
    OPTENT3(0, "verbose",        OPT_FLAG,   NULL,                  
            &cmdlineP->verbose,        0 );

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    cmdlineP->missingcolor = NULL;  /* default value */
    
    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (cmdlineP->floyd && nofloyd)
        pm_error("You cannot specify both -floyd and -nofloyd options.");

    if (firstisdefault && missingSpec)
        pm_error("You cannot specify both -missing and -firstisdefault.");

    if (firstisdefault)
        cmdlineP->missingMethod = MISSING_FIRST;
    else if (missingSpec)
        cmdlineP->missingMethod = MISSING_SPECIFIED;
    else
        cmdlineP->missingMethod = MISSING_CLOSE;

    if (!mapfileSpec)
        pm_error("You must specify the -mapfile option.");

    if (argc-1 > 1)
        pm_error("Program takes at most one argument: the input file "
                 "specification.  "
                 "You specified %d arguments.", argc-1);
    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else
        cmdlineP->inputFilespec = argv[1];
}



static void
rgbToDepth1(const struct pam * const pamP,
            tuple *            const tupleRow) {
    
    unsigned int col;

    for (col = 0; col < pamP->width; ++col) {
        unsigned int plane;
        double grayvalue;
        grayvalue = 0.0;  /* initial value */
        for (plane = 0; plane < pamP->depth; ++plane)
            grayvalue += pnm_lumin_factor[plane] * tupleRow[col][plane];
        tupleRow[col][0] = (sample) (grayvalue + 0.5);
    }
}



static void
grayscaleToDepth3(const struct pam * const pamP,
                  tuple *            const tupleRow) {
    
    unsigned int col;

    assert(pamP->allocation_depth >= 3);

    for (col = 0; col < pamP->width; ++col) {
        tupleRow[col][1] = tupleRow[col][0];
        tupleRow[col][2] = tupleRow[col][0];
    }
}



static void
adjustDepth(const struct pam * const pamP,
            tuple *            const tupleRow,
            unsigned int       const newDepth) {
/*----------------------------------------------------------------------------
   Change the depth of the raster row tupleRow[] of the image
   described by 'pamP' to newDepth.

   We don't change the memory allocation; tupleRow[] must already have
   space allocated for at least 'newDepth' planes.  When we're done,
   all but the first 'newDepth' planes are meaningless, but the space is
   still there.

   The only depth changes we know how to do are:

     - from tuple type RGB, depth 3 to depth 1 

       We change it to grayscale or black and white.

     - from tuple type GRAYSCALE or BLACKANDWHITE depth 1 to depth 3.

       We change it to RGB.

   For any other depth change request, we issue an error message and abort
   the program.
-----------------------------------------------------------------------------*/
    if (newDepth != pamP->depth) {

        if (stripeq(pamP->tuple_type, "RGB")) {
            if (newDepth != 1) {
                pm_error("Map image depth of %u differs from input image "
                         "depth of %u, and the tuple type is RGB.  "
                         "The only depth to which I know how to convert "
                         "an RGB tuple is 1.",
                         newDepth, pamP->depth);
            } else
                rgbToDepth1(pamP, tupleRow);
        } else if (stripeq(pamP->tuple_type, "GRAYSCALE") ||
                   stripeq(pamP->tuple_type, "BLACKANDWHITE")) {
            if (newDepth != 3) {
                pm_error("Map image depth of %u differs from input image "
                         "depth of %u, and the tuple type is GRAYSCALE "
                         "or BLACKANDWHITE.  "
                         "The only depth to which I know how to convert "
                         "a GRAYSCALE or BLACKANDWHITE tuple is 3.",
                         newDepth, pamP->depth);
            } else
                grayscaleToDepth3(pamP, tupleRow);
        } else {
            pm_error("Map image depth of %u differs from input image depth "
                     "of %u, and the input image does not have a tuple type "
                     "that I know how to convert to the map depth.  "
                     "I can convert RGB, GRAYSCALE, and BLACKANDWHITE.  "
                     "The input image is '%.*s'.",
                     newDepth, pamP->depth, 
                     (int)sizeof(pamP->tuple_type), pamP->tuple_type);
        }
    }
}




static void
computeColorMapFromMap(struct pam *   const mappamP, 
                       tuple **       const maptuples, 
                       tupletable *   const colormapP,
                       unsigned int * const newcolorsP) {
/*----------------------------------------------------------------------------
   Produce a colormap containing the colors that we will use in the output.

   Make it include exactly those colors that are in the image
   described by *mappamP and maptuples[][].

   Return the number of colors in the returned colormap as *newcolorsP.
-----------------------------------------------------------------------------*/
    unsigned int colors; 

    if (mappamP->width == 0 || mappamP->height == 0)
        pm_error("colormap file contains no pixels");

    *colormapP = 
        pnm_computetuplefreqtable(mappamP, maptuples, MAXCOLORS, &colors);
    if (*colormapP == NULL)
        pm_error("too many colors in colormap!");
    pm_message("%d colors found in colormap", colors);
    *newcolorsP = colors;
}



static void
initFserr(struct pam *   const pamP,
          struct fserr * const fserrP) {
/*----------------------------------------------------------------------------
   Initialize the Floyd-Steinberg error vectors
-----------------------------------------------------------------------------*/
    unsigned int plane;

    unsigned int const fserrSize = pamP->width + 2;

    MALLOCARRAY(fserrP->thiserr, pamP->depth);
    if (fserrP->thiserr == NULL)
        pm_error("Out of memory allocating Floyd-Steinberg structures "
                 "for depth %u", pamP->depth);
    MALLOCARRAY(fserrP->nexterr, pamP->depth);
    if (fserrP->nexterr == NULL)
        pm_error("Out of memory allocating Floyd-Steinberg structures "
                 "for depth %u", pamP->depth);
    
    for (plane = 0; plane < pamP->depth; ++plane) {
        MALLOCARRAY(fserrP->thiserr[plane], fserrSize);
        if (fserrP->thiserr[plane] == NULL)
            pm_error("Out of memory allocating Floyd-Steinberg structures "
                     "for Plane %u, size %u", plane, fserrSize);
        MALLOCARRAY(fserrP->nexterr[plane], fserrSize);
        if (fserrP->nexterr[plane] == NULL)
            pm_error("Out of memory allocating Floyd-Steinberg structures "
                     "for Plane %u, size %u", plane, fserrSize);
    }

    srand((int)(time(0) ^ getpid()));

    {
        int col;

        for (col = 0; col < fserrSize; ++col) {
            unsigned int plane;
            for (plane = 0; plane < pamP->depth; ++plane) 
                fserrP->thiserr[plane][col] = 
                    rand() % (FS_SCALE * 2) - FS_SCALE;
                    /* (random errors in [-1 .. 1]) */
        }
    }
    fserrP->fsForward = TRUE;
}



static void
floydInitRow(struct pam * const pamP, struct fserr * const fserrP) {

    int col;
    
    for (col = 0; col < pamP->width + 2; ++col) {
        unsigned int plane;
        for (plane = 0; plane < pamP->depth; ++plane) 
            fserrP->nexterr[plane][col] = 0;
    }
}



static void
floydAdjustColor(struct pam *   const pamP, 
                 tuple          const tuple, 
                 struct fserr * const fserrP, 
                 int            const col) {
/*----------------------------------------------------------------------------
  Use Floyd-Steinberg errors to adjust actual color.
-----------------------------------------------------------------------------*/
    unsigned int plane;

    for (plane = 0; plane < pamP->depth; ++plane) {
        long int const s =
            tuple[plane] + fserrP->thiserr[plane][col+1] / FS_SCALE;
        tuple[plane] = MIN(pamP->maxval, MAX(0,s));
    }
}



static void
floydPropagateErr(struct pam *   const pamP, 
                  struct fserr * const fserrP, 
                  int            const col, 
                  tuple          const oldtuple, 
                  tuple          const newtuple) {
/*----------------------------------------------------------------------------
  Propagate Floyd-Steinberg error terms.

  The error is due to substituting the tuple value 'newtuple' for the
  tuple value 'oldtuple' (both described by *pamP).  The error terms
  are meant to be used to introduce a compensating error into the
  future selection of tuples nearby in the image.
-----------------------------------------------------------------------------*/
    unsigned int plane;
    for (plane = 0; plane < pamP->depth; ++plane) {
        long const newSample = newtuple[plane];
        long const oldSample = oldtuple[plane];
        long const err = (oldSample - newSample) * FS_SCALE;
            
        if (fserrP->fsForward) {
            fserrP->thiserr[plane][col + 2] += ( err * 7 ) / 16;
            fserrP->nexterr[plane][col    ] += ( err * 3 ) / 16;
            fserrP->nexterr[plane][col + 1] += ( err * 5 ) / 16;
            fserrP->nexterr[plane][col + 2] += ( err     ) / 16;
        } else {
            fserrP->thiserr[plane][col    ] += ( err * 7 ) / 16;
            fserrP->nexterr[plane][col + 2] += ( err * 3 ) / 16;
            fserrP->nexterr[plane][col + 1] += ( err * 5 ) / 16;
            fserrP->nexterr[plane][col    ] += ( err     ) / 16;
        }
    }
}



static void
floydSwitchDir(struct pam * const pamP, struct fserr * const fserrP) {

    unsigned int plane;

    for (plane = 0; plane < pamP->depth; ++plane) {
        long * const temperr = fserrP->thiserr[plane];
        fserrP->thiserr[plane] = fserrP->nexterr[plane];
        fserrP->nexterr[plane] = temperr;
    }
    fserrP->fsForward = ! fserrP->fsForward;
}



struct colormapFinder {
/*----------------------------------------------------------------------------
   This is an object that finds a color in a colormap.  The methods
   'searchColormapClose' and 'searchColormapExact' belong to it.

   This object ought to encompass the hash table as well some day and
   possibly encapsulate the color map altogether and just be an object
   that opaquely maps input colors to output colors.
-----------------------------------------------------------------------------*/
    tupletable colormap;
    unsigned int colors;
        /* Number of colors in 'colormap'.  At least 1 */
    unsigned int distanceDivider;
        /* The value by which our intermediate distance calculations
           have to be divided to make sure we don't overflow our
           unsigned int data structure.
           
           To the extent 'distanceDivider' is greater than 1, closest
           color results will be approximate -- there could
           conceivably be a closer one that we miss.
        */
};



static void
createColormapFinder(struct pam *             const pamP,
                     tupletable               const colormap,
                     unsigned int             const colors,
                     struct colormapFinder ** const colormapFinderPP) {

    struct colormapFinder * colormapFinderP;

    MALLOCVAR_NOFAIL(colormapFinderP);

    colormapFinderP->colormap = colormap;
    colormapFinderP->colors = colors;

    {
        unsigned int const maxHandleableSqrDiff = 
            (unsigned int)UINT_MAX / pamP->depth;
        
        if (SQR(pamP->maxval) > maxHandleableSqrDiff)
            colormapFinderP->distanceDivider = (unsigned int)
                (SQR(pamP->maxval) / maxHandleableSqrDiff + 0.1 + 1.0);
                /* The 0.1 is a fudge factor to keep us out of rounding 
                   trouble.  The 1.0 effects a round-up.
                */
        else
            colormapFinderP->distanceDivider = 1;
    }
    *colormapFinderPP = colormapFinderP;
}



static void
destroyColormapFinder(struct colormapFinder * const colormapFinderP) {

    free(colormapFinderP);
}



static void
searchColormapClose(struct pam *            const pamP,
                    tuple                   const tuple,
                    struct colormapFinder * const colorFinderP,
                    int *                   const colormapIndexP) {
/*----------------------------------------------------------------------------
   Search the colormap indicated by *colorFinderP for the color closest to
   that of tuple 'tuple'.  Return its index as *colormapIndexP.

   *pamP describes the tuple 'tuple' and *colorFinderP has to be
   compatible with it (i.e. the tuples in the color map must also be
   described by *pamP).

   We compute distance between colors simply as the cartesian distance
   between them in the RGB space.  An alternative would be to look at
   the chromaticities and luminosities of the colors.  In experiments
   in 2003, we found that this was actually worse in many cases.  One
   might think that two colors are closer if they have similar hues
   than when they are simply different brightnesses of the same hue.
   Human subjects asked to compare two colors normally say so.  But
   when replacing the color of a pixel in an image, the luminosity is
   much more important, because you need to retain the luminosity
   relationship between adjacent pixels.  If you replace a pixel with
   one that has the same chromaticity as the original, but much
   darker, it may stand out among its neighbors in a way the original
   pixel did not.  In fact, on an image with blurred edges, we saw
   ugly effects at the edges when we substituted colors using a
   chromaticity-first color closeness formula.
-----------------------------------------------------------------------------*/
    unsigned int i;
    unsigned int dist;
        /* The closest distance we've found so far between the value of
           tuple 'tuple' and a tuple in the colormap.  This is measured as
           the square of the cartesian distance between the tuples, except
           that it's divided by 'distanceDivider' to make sure it will fit
           in an unsigned int.
        */

    dist = UINT_MAX;  /* initial value */

    assert(colorFinderP->colors > 0);

    for (i = 0; i < colorFinderP->colors; ++i) {
        unsigned int newdist;  /* candidate for new 'dist' value */
        unsigned int plane;

        newdist = 0;

        for (plane=0; plane < pamP->depth; ++plane) {
            newdist += 
                SQR(tuple[plane] - colorFinderP->colormap[i]->tuple[plane]) 
                / colorFinderP->distanceDivider;
        }
        if (newdist < dist) {
            *colormapIndexP = i;
            dist = newdist;
        }
    }
}



static void
searchColormapExact(struct pam *            const pamP,
                    struct colormapFinder * const colorFinderP,
                    tuple                   const tuple,
                    int *                   const colormapIndexP,
                    bool *                  const foundP) {
/*----------------------------------------------------------------------------
   Search the colormap indicated by *colorFinderP for the color of
   tuple 'tuple'.  If it's in the map, return its index as
   *colormapIndexP and return *foundP == TRUE.  Otherwise, return
   *foundP = FALSE.

   *pamP describes the tuple 'tuple' and *colorFinderP has to be
   compatible with it (i.e. the tuples in the color map must also be
   described by *pamP).
-----------------------------------------------------------------------------*/
    unsigned int i;
    bool found;
    
    found = FALSE;  /* initial value */
    for (i = 0; i < colorFinderP->colors && !found; ++i) {
        unsigned int plane;
        found = TRUE;  /* initial assumption */
        for (plane=0; plane < pamP->depth; ++plane) 
            if (tuple[plane] != colorFinderP->colormap[i]->tuple[plane]) 
                found = FALSE;
        if (found) 
            *colormapIndexP = i;
    }
    *foundP = found;
}



static void
lookupThroughHash(struct pam *            const pamP, 
                  tuple                   const tuple, 
                  bool                    const needExactMatch,
                  struct colormapFinder * const colorFinderP,
                  tuplehash               const colorhash,       
                  int *                   const colormapIndexP,
                  bool *                  const usehashP) {
/*----------------------------------------------------------------------------
   Look up the color of tuple 'tuple' in the color map indicated by
   'colorFinderP' and, if it's in there, return its index as
   *colormapIndexP.  If not, return *colormapIndexP == -1.

   Both the tuple 'tuple' and the colors in color map 'colormap' are
   described by *pamP.

   If 'needExactMatch' isn't true, we find the closest color in the color map,
   and never return *colormapIndex == -1.

   lookaside at the hash table 'colorhash' to possibly avoid the cost of
   a full lookup.  If we do a full lookup, we add the result to 'colorhash'
   unless *usehashP is false, and if that makes 'colorhash' full, we set
   *usehashP false.
-----------------------------------------------------------------------------*/
    int found;

    /* Check hash table to see if we have already matched this color. */
    pnm_lookuptuple(pamP, colorhash, tuple, &found, colormapIndexP);
    if (!found) {
        /* No, have to do a full lookup */
        if (needExactMatch) {
            bool found;

            searchColormapExact(pamP, colorFinderP, tuple,
                                colormapIndexP, &found);
            if (!found)
                *colormapIndexP = -1;
        } else 
            searchColormapClose(pamP, tuple, colorFinderP, colormapIndexP);
        if (*usehashP) {
            bool fits;
            pnm_addtotuplehash(pamP, colorhash, tuple, *colormapIndexP, 
                               &fits);
            if (!fits) {
                pm_message("out of memory adding to hash table; "
                           "proceeding without it");
                *usehashP = FALSE;
            }
        }
    }
}



static void
convertRow(struct pam *            const pamP, 
           tuple                         tuplerow[],
           tupletable              const colormap,
           struct colormapFinder * const colorFinderP,
           tuplehash               const colorhash, 
           bool *                  const usehashP,
           bool                    const floyd, 
           tuple                   const defaultColor,
           struct fserr *          const fserrP,
           unsigned int *          const missingCountP) {
/*----------------------------------------------------------------------------
  Replace the colors in row tuplerow[] (described by *pamP) with the
  new colors.

  Use and update the Floyd-Steinberg state *fserrP.

  Return the number of pixels that were not matched in the color map as
  *missingCountP.

  *colorFinderP is a color finder based on 'colormap' -- it tells us what
  index of 'colormap' corresponds to a certain color.
-----------------------------------------------------------------------------*/
    int col;
    int limitcol;
        /* The column at which to stop processing the row.  If we're scanning
           forwards, this is the rightmost column.  If we're scanning 
           backward, this is the leftmost column.
        */
    
    if (floyd)
        floydInitRow(pamP, fserrP);

    *missingCountP = 0;  /* initial value */
    
    if ((!floyd) || fserrP->fsForward) {
        col = 0;
        limitcol = pamP->width;
    } else {
        col = pamP->width - 1;
        limitcol = -1;
    }
    do {
        int colormapIndex;
            /* Index into the colormap of the replacement color, or -1 if
               there is no usable color in the color map.
            */

        if (floyd) 
            floydAdjustColor(pamP, tuplerow[col], fserrP, col);

        lookupThroughHash(pamP, tuplerow[col], 
                          !!defaultColor, colorFinderP, 
                          colorhash, &colormapIndex, usehashP);
        if (floyd)
            floydPropagateErr(pamP, fserrP, col, tuplerow[col], 
                              colormap[colormapIndex]->tuple);

        if (colormapIndex == -1) {
            ++*missingCountP;

            assert(defaultColor);  // Otherwise, lookup would have succeeded

            pnm_assigntuple(pamP, tuplerow[col], defaultColor);
        } else 
            pnm_assigntuple(pamP, tuplerow[col], 
                            colormap[colormapIndex]->tuple);

        if (floyd && !fserrP->fsForward) 
            --col;
        else
            ++col;
    } while (col != limitcol);

    if (floyd)
        floydSwitchDir(pamP, fserrP);
}



static void
copyRaster(struct pam *   const inpamP, 
           struct pam *   const outpamP,
           tupletable     const colormap, 
           unsigned int   const colormapSize,
           bool           const floyd, 
           tuple          const defaultColor, 
           unsigned int * const missingCountP) {

    tuplehash const colorhash = pnm_createtuplehash();
    struct colormapFinder * colorFinderP;
    bool usehash;
    struct fserr fserr;
    tuple * tuplerow = pnm_allocpamrow(inpamP);
    int row;

    if (outpamP->maxval != inpamP->maxval && defaultColor)
        pm_error("The maxval of the colormap (%u) is not equal to the "
                 "maxval of the input image (%u).  This is allowable only "
                 "if you are doing an approximate mapping (i.e. you don't "
                 "specify -firstisdefault or -missingcolor)",
                 (unsigned int)outpamP->maxval, (unsigned int)inpamP->maxval);

    usehash = TRUE;

    createColormapFinder(outpamP, colormap, colormapSize, &colorFinderP);

    if (floyd)
        initFserr(inpamP, &fserr);

    *missingCountP = 0;  /* initial value */

    for (row = 0; row < inpamP->height; ++row) {
        unsigned int missingCount;

        pnm_readpamrow(inpamP, tuplerow);

        /* The following modify tuplerow, to make it consistent with
           *outpamP instead of *inpamP.
        */
        assert(inpamP->allocation_depth >= outpamP->depth);
        pnm_scaletuplerow(inpamP, tuplerow, tuplerow, outpamP->maxval);
        adjustDepth(inpamP, tuplerow, outpamP->depth); 

        /* The following both consults and adds to 'colorhash' and
           its associated 'usehash'.  It modifies 'tuplerow' too.
        */
        convertRow(outpamP, tuplerow, colormap, colorFinderP, colorhash, 
                   &usehash,
                   floyd, defaultColor, &fserr, &missingCount);
        
        *missingCountP += missingCount;
        
        pnm_writepamrow(outpamP, tuplerow);
    }
    destroyColormapFinder(colorFinderP);
    pnm_freepamrow(tuplerow);
    pnm_destroytuplehash(colorhash);
}



static void
remap(FILE *             const ifP,
      const struct pam * const outpamCommonP,
      tupletable         const colormap, 
      unsigned int       const colormapSize,
      bool               const floyd,
      tuple              const defaultColor,
      bool               const verbose) {
/*----------------------------------------------------------------------------
   Remap the pixels from the raster on *ifP to the 'colormapSize' colors in
   'colormap'.

   Where the input pixel's color is in the map, just use that for the output.
   Where it isn't, use 'defaultColor', except if that is NULL, use the
   closest color in the map to the input color.

   But if 'floyd' is true and 'defaultColor' is NULL, also do Floyd-Steinberg
   dithering on the output so the aggregate color of a region is about the
   same as that of the input even though the individual pixels have different
   colors.
-----------------------------------------------------------------------------*/
    bool eof;
    eof = FALSE;
    while (!eof) {
        struct pam inpam, outpam;
        unsigned int missingCount;
            /* Number of pixels that were mapped to 'defaultColor' because
               they weren't present in the color map.
            */

        pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(allocation_depth));
    
        outpam = *outpamCommonP;
        outpam.width  = inpam.width;
        outpam.height = inpam.height;

        pnm_writepaminit(&outpam);

        /* Set up so input buffers have extra space as needed to
           convert the input to the output depth.
        */
        pnm_setminallocationdepth(&inpam, outpam.depth);
    
        copyRaster(&inpam, &outpam, colormap, colormapSize, floyd,
                   defaultColor, &missingCount);
        
        if (verbose)
            pm_message("%u pixels not matched in color map", missingCount);
        
        pnm_nextimage(ifP, &eof);
    }
}



static void
processMapFile(const char *   const mapFileName,
               struct pam *   const outpamCommonP,
               tupletable *   const colormapP,
               unsigned int * const colormapSizeP,
               tuple *        const firstColorP) {

    FILE * mapfile;
    struct pam mappam;
    tuple ** maptuples;
    tuple firstColor;

    mapfile = pm_openr(mapFileName);
    maptuples = pnm_readpam(mapfile, &mappam, PAM_STRUCT_SIZE(tuple_type));
    pm_close(mapfile);

    computeColorMapFromMap(&mappam, maptuples, colormapP, colormapSizeP);

    firstColor = pnm_allocpamtuple(&mappam);
    pnm_assigntuple(&mappam, firstColor, maptuples[0][0]);
    *firstColorP = firstColor;

    pnm_freepamarray(maptuples, &mappam);

    *outpamCommonP = mappam; 
    outpamCommonP->file = stdout;
}



static void
getSpecifiedMissingColor(struct pam * const pamP,
                         const char * const colorName,
                         tuple *      const specColorP) {

    tuple specColor;
                             
    specColor = pnm_allocpamtuple(pamP);

    if (colorName) {
        pixel const color = ppm_parsecolor(colorName, pamP->maxval);
        if (pamP->depth == 3) {
            specColor[PAM_RED_PLANE] = PPM_GETR(color);
            specColor[PAM_GRN_PLANE] = PPM_GETG(color);
            specColor[PAM_BLU_PLANE] = PPM_GETB(color);
        } else if (pamP->depth == 1) {
            specColor[0] = PPM_LUMIN(color);
        } else {
            pm_error("You may not use -missing with a colormap that is not "
                     "of depth 1 or 3.  Yours has depth %u",
                     pamP->depth);
        }
    }
    *specColorP = specColor;
}



int
main(int argc, char * argv[] ) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    struct pam outpamCommon;
        /* Describes the output images.  Width and height fields are
           not meaningful, because different output images might have
           different dimensions.  The rest of the information is common
           across all output images.
        */
    tupletable colormap;
    unsigned int colormapSize;
    tuple specColor;
        /* A tuple of the color the user specified to use for input colors
           that are not in the colormap.  Arbitrary tuple if he didn't
           specify any.
        */
    tuple firstColor;
        /* A tuple of the first color present in the map file */
    tuple defaultColor;
        /* The color to which we will map an input color that is not in the
           colormap.  NULL if we are not to map such a color to a particular
           color (i.e. we'll choose an approximate match from the map).
        */

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    processMapFile(cmdline.mapFilespec, &outpamCommon,
                   &colormap, &colormapSize, &firstColor);

    getSpecifiedMissingColor(&outpamCommon, cmdline.missingcolor, &specColor);

    switch (cmdline.missingMethod) {
    case MISSING_CLOSE:
        defaultColor = NULL;
        break;
    case MISSING_FIRST:
        defaultColor = firstColor;
        break;
    case MISSING_SPECIFIED:
        defaultColor = specColor;
        break;
    }

    remap(ifP, &outpamCommon, colormap, colormapSize, 
          cmdline.floyd, defaultColor,
          cmdline.verbose);

    pnm_freepamtuple(firstColor);
    pnm_freepamtuple(specColor);

    pm_close(stdout);

    pm_close(ifP);

    return 0;
}
