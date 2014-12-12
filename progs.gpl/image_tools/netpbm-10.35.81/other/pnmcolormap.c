/******************************************************************************
                               pnmcolormap.c
*******************************************************************************

  Create a colormap file (a PPM image containing one pixel of each of a set
  of colors).  Base the set of colors on an input image.

  For PGM input, do the equivalent for grayscale and produce a PGM graymap.

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

#include <math.h>

#include "pm_config.h"
#include "pam.h"
#include "pammap.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"

enum methodForLargest {LARGE_NORM, LARGE_LUM};

enum methodForRep {REP_CENTER_BOX, REP_AVERAGE_COLORS, REP_AVERAGE_PIXELS};

typedef struct box* boxVector;
struct box {
    int ind;
    int colors;
    int sum;
};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    unsigned int allcolors;  /* boolean: select all colors from the input */
    unsigned int newcolors;    
        /* Number of colors argument; meaningless if allcolors true */
    enum methodForLargest methodForLargest; 
        /* -spreadintensity/-spreadluminosity options */
    enum methodForRep methodForRep;
        /* -center/-meancolor/-meanpixel options */
    unsigned int sort;
    unsigned int square;
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
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int spreadbrightness, spreadluminosity;
    unsigned int center, meancolor, meanpixel;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "spreadbrightness", OPT_FLAG,   
            NULL,                       &spreadbrightness, 0);
    OPTENT3(0,   "spreadluminosity", OPT_FLAG,   
            NULL,                       &spreadluminosity, 0);
    OPTENT3(0,   "center",           OPT_FLAG,   
            NULL,                       &center,           0);
    OPTENT3(0,   "meancolor",        OPT_FLAG,   
            NULL,                       &meancolor,        0);
    OPTENT3(0,   "meanpixel",        OPT_FLAG,   
            NULL,                       &meanpixel,        0);
    OPTENT3(0, "sort",     OPT_FLAG,   NULL,                  
            &cmdlineP->sort,       0 );
    OPTENT3(0, "square",   OPT_FLAG,   NULL,                  
            &cmdlineP->square,       0 );
    OPTENT3(0, "verbose",   OPT_FLAG,   NULL,                  
            &cmdlineP->verbose,       0 );

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */


    if (spreadbrightness && spreadluminosity) 
        pm_error("You cannot specify both -spreadbrightness and "
                 "spreadluminosity.");
    if (spreadluminosity)
        cmdlineP->methodForLargest = LARGE_LUM;
    else
        cmdlineP->methodForLargest = LARGE_NORM;

    if (center + meancolor + meanpixel > 1)
        pm_error("You can specify only one of -center, -meancolor, and "
                 "-meanpixel.");
    if (meancolor)
        cmdlineP->methodForRep = REP_AVERAGE_COLORS;
    else if (meanpixel) 
        cmdlineP->methodForRep = REP_AVERAGE_PIXELS;
    else
        cmdlineP->methodForRep = REP_CENTER_BOX;

    if (argc-1 > 2)
        pm_error("Program takes at most two arguments: number of colors "
                 "and input file specification.  "
                 "You specified %d arguments.", argc-1);
    else {
        if (argc-1 < 2)
            cmdlineP->inputFilespec = "-";
        else
            cmdlineP->inputFilespec = argv[2];

        if (argc-1 < 1)
            pm_error("You must specify the number of colors in the "
                     "output as an argument.");
        else {
            if (strcmp(argv[1], "all") == 0)
                cmdlineP->allcolors = TRUE;
            else {
                char * tail;
                long int const newcolors = strtol(argv[1], &tail, 10);
                if (*tail != '\0')
                    pm_error("The number of colors argument '%s' is not "
                             "a number or 'all'", argv[1]);
                else if (newcolors < 1)
                    pm_error("The number of colors must be positive");
                else if (newcolors == 1)
                    pm_error("The number of colors must be greater than 1.");
                else {
                    cmdlineP->newcolors = newcolors;
                    cmdlineP->allcolors = FALSE;
                }
            }
        }
    }
}



#ifndef LITERAL_FN_DEF_MATCH
static qsort_comparison_fn compareplane;
#endif

static unsigned int compareplanePlane;
    /* This is a parameter to compareplane().  We use this global variable
       so that compareplane() can be called by qsort(), to compare two
       tuples.  qsort() doesn't pass any arguments except the two tuples.
    */
static int
compareplane(const void * const arg1, 
             const void * const arg2) {

    const struct tupleint * const * const comparandPP  = arg1;
    const struct tupleint * const * const comparatorPP = arg2;

    return (*comparandPP)->tuple[compareplanePlane] -
        (*comparatorPP)->tuple[compareplanePlane];
}



#ifndef LITERAL_FN_DEF_MATCH
static qsort_comparison_fn sumcompare;
#endif

static int
sumcompare(const void * const b1, const void * const b2) {
    return(((boxVector)b2)->sum - ((boxVector)b1)->sum);
}




/*
** Here is the fun part, the median-cut colormap generator.  This is based
** on Paul Heckbert's paper "Color Image Quantization for Frame Buffer
** Display", SIGGRAPH '82 Proceedings, page 297.
*/

static tupletable2
newColorMap(unsigned int const newcolors,
            unsigned int const depth) {

    tupletable2 colormap;
    unsigned int i;
    struct pam pam;

    pam.depth = depth;

    colormap.table = pnm_alloctupletable(&pam, newcolors);

    for (i = 0; i < newcolors; ++i) {
        unsigned int plane;
        for (plane = 0; plane < depth; ++plane) 
            colormap.table[i]->tuple[plane] = 0;
    }
    colormap.size = newcolors;

    return colormap;
}



static boxVector
newBoxVector(int const colors, int const sum, int const newcolors) {

    boxVector bv;
    MALLOCARRAY(bv, newcolors);
    if (bv == NULL)
        pm_error("out of memory allocating box vector table");

    /* Set up the initial box. */
    bv[0].ind = 0;
    bv[0].colors = colors;
    bv[0].sum = sum;

    return bv;
}



static void
findBoxBoundaries(tupletable2  const colorfreqtable,
                  unsigned int const depth,
                  unsigned int const boxStart,
                  unsigned int const boxSize,
                  sample             minval[],
                  sample             maxval[]) {
/*----------------------------------------------------------------------------
  Go through the box finding the minimum and maximum of each
  component - the boundaries of the box.
-----------------------------------------------------------------------------*/
    unsigned int plane;
    unsigned int i;

    for (plane = 0; plane < depth; ++plane) {
        minval[plane] = colorfreqtable.table[boxStart]->tuple[plane];
        maxval[plane] = minval[plane];
    }
    
    for (i = 1; i < boxSize; ++i) {
        unsigned int plane;
        for (plane = 0; plane < depth; ++plane) {
            sample const v = colorfreqtable.table[boxStart + i]->tuple[plane];
            if (v < minval[plane]) minval[plane] = v;
            if (v > maxval[plane]) maxval[plane] = v;
        } 
    }
}



static unsigned int
largestByNorm(sample minval[], sample maxval[], unsigned int const depth) {
    
    unsigned int largestDimension;
    unsigned int plane;

    sample largestSpreadSoFar = 0;  
    largestDimension = 0;
    for (plane = 0; plane < depth; ++plane) {
        sample const spread = maxval[plane]-minval[plane];
        if (spread > largestSpreadSoFar) {
            largestDimension = plane;
            largestSpreadSoFar = spread;
        }
    }
    return largestDimension;
}



static unsigned int 
largestByLuminosity(sample minval[], sample maxval[], 
                    unsigned int const depth) {
/*----------------------------------------------------------------------------
   This subroutine presumes that the tuple type is either 
   BLACKANDWHITE, GRAYSCALE, or RGB (which implies pamP->depth is 1 or 3).
   To save time, we don't actually check it.
-----------------------------------------------------------------------------*/
    unsigned int retval;

    if (depth == 1)
        retval = 0;
    else {
        /* An RGB tuple */
        unsigned int largestDimension;
        unsigned int plane;
        double largestSpreadSoFar;

        largestSpreadSoFar = 0.0;

        for (plane = 0; plane < 3; ++plane) {
            double const spread = 
                pnm_lumin_factor[plane] * (maxval[plane]-minval[plane]);
            if (spread > largestSpreadSoFar) {
                largestDimension = plane;
                largestSpreadSoFar = spread;
            }
        }
        retval = largestDimension;
    }
    return retval;
}



static void
centerBox(int          const boxStart,
          int          const boxSize,
          tupletable2  const colorfreqtable, 
          unsigned int const depth,
          tuple        const newTuple) {

    unsigned int plane;

    for (plane = 0; plane < depth; ++plane) {
        int minval, maxval;
        unsigned int i;

        minval = maxval = colorfreqtable.table[boxStart]->tuple[plane];
        
        for (i = 1; i < boxSize; ++i) {
            int const v = colorfreqtable.table[boxStart + i]->tuple[plane];
            minval = MIN( minval, v);
            maxval = MAX( maxval, v);
        }
        newTuple[plane] = (minval + maxval) / 2;
    }
}



static void
averageColors(int          const boxStart,
              int          const boxSize,
              tupletable2  const colorfreqtable, 
              unsigned int const depth,
              tuple        const newTuple) {

    unsigned int plane;

    for (plane = 0; plane < depth; ++plane) {
        sample sum;
        int i;

        sum = 0;

        for (i = 0; i < boxSize; ++i) 
            sum += colorfreqtable.table[boxStart+i]->tuple[plane];

        newTuple[plane] = sum / boxSize;
    }
}



static void
averagePixels(int          const boxStart,
              int          const boxSize,
              tupletable2  const colorfreqtable, 
              unsigned int const depth,
              tuple        const newTuple) {

    unsigned int n;
        /* Number of tuples represented by the box */
    unsigned int plane;
    unsigned int i;

    /* Count the tuples in question */
    n = 0;  /* initial value */
    for (i = 0; i < boxSize; ++i)
        n += colorfreqtable.table[boxStart + i]->value;


    for (plane = 0; plane < depth; ++plane) {
        sample sum;
        int i;

        sum = 0;

        for (i = 0; i < boxSize; ++i) 
            sum += colorfreqtable.table[boxStart+i]->tuple[plane]
                * colorfreqtable.table[boxStart+i]->value;

        newTuple[plane] = sum / n;
    }
}



static tupletable2
colormapFromBv(unsigned int      const newcolors,
               boxVector         const bv, 
               unsigned int      const boxes,
               tupletable2       const colorfreqtable, 
               unsigned int      const depth,
               enum methodForRep const methodForRep) {
    /*
    ** Ok, we've got enough boxes.  Now choose a representative color for
    ** each box.  There are a number of possible ways to make this choice.
    ** One would be to choose the center of the box; this ignores any structure
    ** within the boxes.  Another method would be to average all the colors in
    ** the box - this is the method specified in Heckbert's paper.  A third
    ** method is to average all the pixels in the box. 
    */
    tupletable2 colormap;
    unsigned int bi;

    colormap = newColorMap(newcolors, depth);

    for (bi = 0; bi < boxes; ++bi) {
        switch (methodForRep) {
        case REP_CENTER_BOX: 
            centerBox(bv[bi].ind, bv[bi].colors, colorfreqtable, depth, 
                      colormap.table[bi]->tuple);
            break;
        case REP_AVERAGE_COLORS:
            averageColors(bv[bi].ind, bv[bi].colors, colorfreqtable, depth,
                          colormap.table[bi]->tuple);
            break;
        case REP_AVERAGE_PIXELS:
            averagePixels(bv[bi].ind, bv[bi].colors, colorfreqtable, depth,
                          colormap.table[bi]->tuple);
            break;
        default:
            pm_error("Internal error: invalid value of methodForRep: %d",
                     methodForRep);
        }
    }
    return colormap;
}



static unsigned int
freqTotal(tupletable2 const freqtable) {
    
    unsigned int i;
    unsigned int sum;

    sum = 0;

    for (i = 0; i < freqtable.size; ++i)
        sum += freqtable.table[i]->value;

    return sum;
}


static void
splitBox(boxVector             const bv, 
         unsigned int *        const boxesP, 
         unsigned int          const bi,
         tupletable2           const colorfreqtable, 
         unsigned int          const depth,
         enum methodForLargest const methodForLargest) {
/*----------------------------------------------------------------------------
   Split Box 'bi' in the box vector bv (so that bv contains one more box
   than it did as input).  Split it so that each new box represents about
   half of the pixels in the distribution given by 'colorfreqtable' for 
   the colors in the original box, but with distinct colors in each of the
   two new boxes.

   Assume the box contains at least two colors.
-----------------------------------------------------------------------------*/
    unsigned int const boxStart = bv[bi].ind;
    unsigned int const boxSize  = bv[bi].colors;
    unsigned int const sm       = bv[bi].sum;

    sample * minval;  /* malloc'ed array */
    sample * maxval;  /* malloc'ed array */

    unsigned int largestDimension;
        /* number of the plane with the largest spread */
    unsigned int medianIndex;
    int lowersum;
        /* Number of pixels whose value is "less than" the median */

    MALLOCARRAY_NOFAIL(minval, depth);
    MALLOCARRAY_NOFAIL(maxval, depth);

    findBoxBoundaries(colorfreqtable, depth, boxStart, boxSize, 
                      minval, maxval);

    /* Find the largest dimension, and sort by that component.  I have
       included two methods for determining the "largest" dimension;
       first by simply comparing the range in RGB space, and second by
       transforming into luminosities before the comparison.
    */
    switch (methodForLargest) {
    case LARGE_NORM: 
        largestDimension = largestByNorm(minval, maxval, depth);
        break;
    case LARGE_LUM: 
        largestDimension = largestByLuminosity(minval, maxval, depth);
        break;
    }
                                                    
    /* TODO: I think this sort should go after creating a box,
       not before splitting.  Because you need the sort to use
       the REP_CENTER_BOX method of choosing a color to
       represent the final boxes 
    */

    /* Set the gross global variable 'compareplanePlane' as a
       parameter to compareplane(), which is called by qsort().
    */
    compareplanePlane = largestDimension;
    qsort((char*) &colorfreqtable.table[boxStart], boxSize, 
          sizeof(colorfreqtable.table[boxStart]), 
          compareplane);
            
    {
        /* Now find the median based on the counts, so that about half
           the pixels (not colors, pixels) are in each subdivision.  */

        unsigned int i;

        lowersum = colorfreqtable.table[boxStart]->value; /* initial value */
        for (i = 1; i < boxSize - 1 && lowersum < sm/2; ++i) {
            lowersum += colorfreqtable.table[boxStart + i]->value;
        }
        medianIndex = i;
    }
    /* Split the box, and sort to bring the biggest boxes to the top.  */

    bv[bi].colors = medianIndex;
    bv[bi].sum = lowersum;
    bv[*boxesP].ind = boxStart + medianIndex;
    bv[*boxesP].colors = boxSize - medianIndex;
    bv[*boxesP].sum = sm - lowersum;
    ++(*boxesP);
    qsort((char*) bv, *boxesP, sizeof(struct box), sumcompare);

    free(minval); free(maxval);
}



static void
mediancut(tupletable2           const colorfreqtable, 
          unsigned int          const depth,
          int                   const newcolors,
          enum methodForLargest const methodForLargest,
          enum methodForRep     const methodForRep,
          tupletable2 *         const colormapP) {
/*----------------------------------------------------------------------------
   Compute a set of only 'newcolors' colors that best represent an
   image whose pixels are summarized by the histogram
   'colorfreqtable'.  Each tuple in that table has depth 'depth'.
   colorfreqtable.table[i] tells the number of pixels in the subject image
   have a particular color.

   As a side effect, sort 'colorfreqtable'.
-----------------------------------------------------------------------------*/
    boxVector bv;
    unsigned int bi;
    unsigned int boxes;
    bool multicolorBoxesExist;
        /* There is at least one box that contains at least 2 colors; ergo,
           there is more splitting we can do.
        */

    bv = newBoxVector(colorfreqtable.size, freqTotal(colorfreqtable),
                      newcolors);
    boxes = 1;
    multicolorBoxesExist = (colorfreqtable.size > 1);

    /* Main loop: split boxes until we have enough. */
    while (boxes < newcolors && multicolorBoxesExist) {
        /* Find the first splittable box. */
        for (bi = 0; bi < boxes && bv[bi].colors < 2; ++bi);
        if (bi >= boxes)
            multicolorBoxesExist = FALSE;
        else 
            splitBox(bv, &boxes, bi, colorfreqtable, depth, methodForLargest);
    }
    *colormapP = colormapFromBv(newcolors, bv, boxes, colorfreqtable, depth,
                                methodForRep);

    free(bv);
}




static void
validateCompatibleImage(struct pam * const inpamP,
                        struct pam * const firstPamP,
                        unsigned int const imageSeq) {

    if (inpamP->depth != firstPamP->depth)
        pm_error("Image %u depth (%u) is not the same as Image 0 (%u)",
                 imageSeq, inpamP->depth, firstPamP->depth);
    if (inpamP->maxval != firstPamP->maxval)
        pm_error("Image %u maxval (%u) is not the same as Image 0 (%u)",
                 imageSeq,
                 (unsigned)inpamP->maxval, (unsigned)firstPamP->maxval);
    if (inpamP->format != firstPamP->format)
        pm_error("Image %u format (%d) is not the same as Image 0 (%d)",
                 imageSeq, inpamP->format, firstPamP->format);
    if (!STREQ(inpamP->tuple_type, firstPamP->tuple_type))
        pm_error("Image %u tuple type (%s) is not the same as Image 0 (%s)",
                 imageSeq, inpamP->tuple_type, firstPamP->tuple_type);
}



static void
addImageColorsToHash(struct pam *   const pamP,
                     tuplehash      const tuplehash,
                     unsigned int * const colorCountP) {

    tuple * tuplerow;
    unsigned int row;
    
    tuplerow = pnm_allocpamrow(pamP);

    for (row = 0; row < pamP->height; ++row) {
        unsigned int col;

        pnm_readpamrow(pamP, tuplerow);

        for (col = 0; col < pamP->width; ++col) {
            bool firstOccurrence;

            pnm_addtuplefreqoccurrence(pamP, tuplerow[col], tuplehash,
                                       &firstOccurrence);

            if (firstOccurrence)
                ++(*colorCountP);
        }
    }
    pnm_freepamrow(tuplerow);
}



static void
computeHistogram(FILE *         const ifP,
                 int *          const formatP,
                 struct pam *   const freqPamP,
                 tupletable2 *  const colorfreqtableP) {
/*----------------------------------------------------------------------------
  Make a histogram of the colors in the image stream in the file '*ifP'.
  
  Return as *freqPamP a description of the tuple values in the histogram.
  Only the fields of *freqPamP that describe individual tuples are
  meaningful (depth, maxval, tuple type);

  As a fringe benefit, also return the format of the input file as
  *formatP.
----------------------------------------------------------------------------*/
    unsigned int imageSeq;
    struct pam firstPam;
    tuplehash tuplehash;
    unsigned int colorCount;
    bool eof;
    
    pm_message("making histogram...");

    tuplehash = pnm_createtuplehash();
    colorCount = 0;

    eof = FALSE;

    for (imageSeq = 0; !eof; ++imageSeq) {
        struct pam inpam;
        
        pm_message("Scanning image %u", imageSeq);

        pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

        if (imageSeq == 0)
            firstPam = inpam;
        else
            validateCompatibleImage(&inpam, &firstPam, imageSeq);
    
        addImageColorsToHash(&inpam, tuplehash, &colorCount);

        pm_message("%u colors so far", colorCount);

        pnm_nextimage(ifP, &eof);
    }
    colorfreqtableP->table =
        pnm_tuplehashtotable(&firstPam, tuplehash, colorCount);
    colorfreqtableP->size = colorCount;

    pnm_destroytuplehash(tuplehash);

    pm_message("%u colors found", colorfreqtableP->size);
    
    freqPamP->size   = sizeof(*freqPamP);
    freqPamP->len    = PAM_STRUCT_SIZE(tuple_type);
    freqPamP->maxval = firstPam.maxval;
    freqPamP->bytes_per_sample = pnm_bytespersample(freqPamP->maxval);
    freqPamP->depth  = firstPam.depth;
    STRSCPY(freqPamP->tuple_type, firstPam.tuple_type);
    
    *formatP = firstPam.format;
}



static void
computeColorMapFromInput(FILE *                const ifP,
                         bool                  const allColors, 
                         int                   const reqColors, 
                         enum methodForLargest const methodForLargest,
                         enum methodForRep     const methodForRep,
                         int *                 const formatP,
                         struct pam *          const freqPamP,
                         tupletable2 *         const colormapP) {
/*----------------------------------------------------------------------------
   Produce a colormap containing the best colors to represent the
   image stream in file 'ifP'.  Figure it out using the median cut
   technique.

   The colormap will have 'reqcolors' or fewer colors in it, unless
   'allcolors' is true, in which case it will have all the colors that
   are in the input.

   The colormap has the same maxval as the input.

   Put the colormap in newly allocated storage as a tupletable2 
   and return its address as *colormapP.  Return the number of colors in
   it as *colorsP and its maxval as *colormapMaxvalP.

   Return the characteristics of the input file as
   *formatP and *freqPamP.  (This information is not really
   relevant to our colormap mission; just a fringe benefit).
-----------------------------------------------------------------------------*/
    tupletable2 colorfreqtable;

    computeHistogram(ifP, formatP, freqPamP, &colorfreqtable);
    
    if (allColors) {
        *colormapP = colorfreqtable;
    } else {
        if (colorfreqtable.size <= reqColors) {
            pm_message("Image already has few enough colors (<=%d).  "
                       "Keeping same colors.", reqColors);
            *colormapP = colorfreqtable;
        } else {
            pm_message("choosing %d colors...", reqColors);
            mediancut(colorfreqtable, freqPamP->depth,
                      reqColors, methodForLargest, methodForRep,
                      colormapP);
            pnm_freetupletable2(freqPamP, colorfreqtable);
        }
    }
}



static void
sortColormap(tupletable2  const colormap, 
             sample       const depth) {
/*----------------------------------------------------------------------------
   Sort the colormap in place, in order of ascending Plane 0 value, 
   the Plane 1 value, etc.

   Use insertion sort.
-----------------------------------------------------------------------------*/
    int i;
    
    pm_message("Sorting %u colors...", colormap.size);

    for (i = 0; i < colormap.size; ++i) {
        int j;
        for (j = i+1; j < colormap.size; ++j) {
            unsigned int plane;
            bool iIsGreater, iIsLess;

            iIsGreater = FALSE; iIsLess = FALSE;
            for (plane = 0; 
                 plane < depth && !iIsGreater && !iIsLess; 
                 ++plane) {
                if (colormap.table[i]->tuple[plane] >
                    colormap.table[j]->tuple[plane])
                    iIsGreater = TRUE;
                else if (colormap.table[i]->tuple[plane] <
                         colormap.table[j]->tuple[plane])
                    iIsLess = TRUE;
            }            
            if (iIsGreater) {
                for (plane = 0; plane < depth; ++plane) {
                    sample const temp = colormap.table[i]->tuple[plane];
                    colormap.table[i]->tuple[plane] =
                        colormap.table[j]->tuple[plane];
                    colormap.table[j]->tuple[plane] = temp;
                }
            }
        }    
    }
}



static void 
colormapToSquare(struct pam * const pamP,
                 tupletable2  const colormap,
                 tuple ***    const outputRasterP) {
    {
        unsigned int const intsqrt = (int)sqrt((float)colormap.size);
        if (intsqrt * intsqrt == colormap.size) 
            pamP->width = intsqrt;
        else 
            pamP->width = intsqrt + 1;
    }
    {
        unsigned int const intQuotient = colormap.size / pamP->width;
        if (pamP->width * intQuotient == colormap.size)
            pamP->height = intQuotient;
        else
            pamP->height = intQuotient + 1;
    }
    {
        tuple ** outputRaster;
        unsigned int row;
        unsigned int colormapIndex;
        
        outputRaster = pnm_allocpamarray(pamP);

        colormapIndex = 0;  /* initial value */
        
        for (row = 0; row < pamP->height; ++row) {
            unsigned int col;
            for (col = 0; col < pamP->width; ++col) {
                unsigned int plane;
                for (plane = 0; plane < pamP->depth; ++plane) {
                    outputRaster[row][col][plane] = 
                        colormap.table[colormapIndex]->tuple[plane];
                }
                if (colormapIndex < colormap.size-1)
                    ++colormapIndex;
            }
        }
        *outputRasterP = outputRaster;
    } 
}



static void 
colormapToSingleRow(struct pam * const pamP,
                    tupletable2  const colormap,
                    tuple ***    const outputRasterP) {

    tuple ** outputRaster;
    unsigned int col;
    
    pamP->width = colormap.size;
    pamP->height = 1;
    
    outputRaster = pnm_allocpamarray(pamP);
    
    for (col = 0; col < pamP->width; ++col) {
        int plane;
        for (plane = 0; plane < pamP->depth; ++plane)
            outputRaster[0][col][plane] = colormap.table[col]->tuple[plane];
    }
    *outputRasterP = outputRaster;
}



static void
colormapToImage(int                const format,
                const struct pam * const colormapPamP,
                tupletable2        const colormap,
                bool               const sort,
                bool               const square,
                struct pam *       const outpamP, 
                tuple ***          const outputRasterP) {
/*----------------------------------------------------------------------------
   Create a tuple array and pam structure for an image which includes
   one pixel of each of the colors in the colormap 'colormap'.

   May rearrange the contents of 'colormap'.
-----------------------------------------------------------------------------*/
    outpamP->size             = sizeof(*outpamP);
    outpamP->len              = PAM_STRUCT_SIZE(tuple_type);
    outpamP->format           = format,
    outpamP->plainformat      = FALSE;
    outpamP->depth            = colormapPamP->depth;
    outpamP->maxval           = colormapPamP->maxval;
    outpamP->bytes_per_sample = pnm_bytespersample(outpamP->maxval);
    STRSCPY(outpamP->tuple_type, colormapPamP->tuple_type);

    if (sort)
        sortColormap(colormap, outpamP->depth);

    if (square) 
        colormapToSquare(outpamP, colormap, outputRasterP);
    else 
        colormapToSingleRow(outpamP, colormap, outputRasterP);
}



int
main(int argc, char * argv[] ) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    int format;
    struct pam colormapPam;
    struct pam outpam;
    tuple ** colormapRaster;
    tupletable2 colormap;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    computeColorMapFromInput(ifP,
                             cmdline.allcolors, cmdline.newcolors, 
                             cmdline.methodForLargest, 
                             cmdline.methodForRep,
                             &format, &colormapPam, &colormap);

    pm_close(ifP);

    colormapToImage(format, &colormapPam, colormap,
                    cmdline.sort, cmdline.square, &outpam, &colormapRaster);

    if (cmdline.verbose)
        pm_message("Generating %u x %u image", outpam.width, outpam.height);

    outpam.file = stdout;
    
    pnm_writepam(&outpam, colormapRaster);
    
    pnm_freetupletable2(&colormapPam, colormap);

    pnm_freepamarray(colormapRaster, &outpam);

    pm_close(stdout);

    return 0;
}
