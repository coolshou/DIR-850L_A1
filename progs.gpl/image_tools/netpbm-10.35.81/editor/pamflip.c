/* pamflip.c - perform one or more flip operations on a Netpbm image
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/*
   transformGen() is the general transformation function.
   
   The following are enhancements for specific cases:
   
     transformRowByRowPbm()
     transformRowsBottomTopPbm()
     transformRowByRowNonPbm()
     transformRowsBottomTopNonPbm()
     transformPbm()
   
   Although we use transformGen() only when none of the enhancement
   functions apply, it is capable of handling all cases.  (Only that it
   is slow, and uses more memory.)  In the same manner, transformPbm() is
   capable of handling all pbm transformations and transformRowByRowNonPbm()
   transformRowsBottomTomNonPbm() are capable of handling pbm.


   There is some fancy virtual memory management in transformGen() to avoid
   page thrashing when you flip a large image in a columns-for-rows
   way (e.g. -transpose).
   
   The page thrashing we're trying to avoid could happen because the
   output of the transformation is stored in an array of tuples in
   virtual memory.  A tuple array is stored in column-first order,
   meaning that all the columns of particular row are contiguous, the
   next row is next to that, etc.  If you fill up that array by
   filling in Column 0 sequentially in every row from top to bottom,
   you will touch a lot of different virtual memory pages, and every
   one has to be paged in as you touch it.

   If the number of virtual memory pages you touch exceeds the amount
   of real memory the process can get, then by the time you hit the bottom
   of the tuple array, the pages that hold the top are already paged out.
   So if you go back and do Column 1 from top to bottom, you will again
   touch lots of pages and have to page in every one of them.  Do this 
   for 100 columns, and you might page in every page in the array 100 times
   each, putting a few bytes in the page each time.

   That is very expensive.  Instead, you'd like to keep the same pages in
   real memory as long as possible and fill them up as much as you can 
   before paging them out and working on a new set of pages.  You can do
   that by doing Column 0 from top to say Row 10, then Column 1 from top
   to Row 10, etc. all the way across the image.  Assuming 10 rows fits
   in real memory, you will keep the virtual memory for the first 10 rows
   of the tuple array in real memory until you've filled them in completely.
   Now you go back and do Column 0 from Row 11 to Row 20, then Column 1
   from Row 11 to Row 20, and so on.

   So why are we even trying to fill in column by column instead of just
   filling in row by row?  Because we're reading an input image row by row
   and transforming it in such a way that a row of the input becomes
   a column of the output.  In order to fill in a whole row of the output,
   we'd have to read a whole column of the input, and then we have the same
   page thrashing problem in the input.

   So the optimal procedure is to do N output rows in each pass, where
   N is the largest number of rows we can fit in real memory.  In each
   pass, we read certain columns of every row of the input and output
   every column of certain rows of the output.  The output area for
   the rows in the pass gets paged in once during the pass and then
   never again.  Note that some pages of every row of the input get
   paged in once in each pass too.  As each input page is referenced
   only in one burst, input pages do not compete with output pages for
   real memory -- the working set is the output pages, which get referenced
   cyclically.

   This all worked when we used the pnm xel format, but now that we
   use the pam tuple format, there's an extra memory reference that
   could be causing trouble.  Because tuples have varying depth, a pam
   row is an array of pointers to the tuples.  To access a tuple, we
   access the tuple pointer, then the tuple.  We could probably do better,
   because the samples are normally in the memory immediately following
   the tuple pointers, so we could compute where a tuple's samples live
   without actually loading the tuple address from memory.  I.e. the 
   address of the tuple for Column 5 of Row 9 of a 3-deep 100-wide
   image is (void*)tuples[9] + 100 * sizeof(tuple*) + 5*(3*sizeof(sample)).
*/

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <limits.h>
#include <string.h>

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"
#include "bitreverse.h"

enum xformType {LEFTRIGHT, TOPBOTTOM, TRANSPOSE};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    unsigned int xformCount;
        /* Number of transforms in the 'xformType' array */
    enum xformType xformList[10];
        /* Array of transforms to be applied, in order */
    unsigned int availableMemory;
    unsigned int pageSize;
    unsigned int verbose;
};



static void
parseXformOpt(const char *     const xformOpt,
              unsigned int  *  const xformCountP,
              enum xformType * const xformList) {
/*----------------------------------------------------------------------------
   Translate the -xform option string into an array of transform types.

   Return the array as xformList[], which is preallocated for at least
   10 elements.
-----------------------------------------------------------------------------*/
    unsigned int xformCount;
    char * xformOptWork;
    char * cursor;
    bool eol;
    
    xformOptWork = strdup(xformOpt);
    cursor = &xformOptWork[0];
    
    eol = FALSE;    /* initial value */
    xformCount = 0; /* initial value */
    while (!eol && xformCount < 10) {
        const char * token;
        token = strsepN(&cursor, ",");
        if (token) {
            if (streq(token, "leftright"))
                xformList[xformCount++] = LEFTRIGHT;
            else if (streq(token, "topbottom"))
                xformList[xformCount++] = TOPBOTTOM;
            else if (streq(token, "transpose"))
                xformList[xformCount++] = TRANSPOSE;
            else if (streq(token, ""))
            { /* ignore it */}
            else
                pm_error("Invalid transform type in -xform option: '%s'",
                         token );
        } else
            eol = TRUE;
    }
    free(xformOptWork);

    *xformCountP = xformCount;
}



static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int lr, tb, xy, r90, r270, r180, null;
    unsigned int memsizeSpec, pagesizeSpec, xformSpec;
    unsigned int memsizeOpt;
    const char *xformOpt;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "lr",        OPT_FLAG,    NULL, &lr,      0);
    OPTENT3(0, "leftright", OPT_FLAG,    NULL, &lr,      0);
    OPTENT3(0, "tb",        OPT_FLAG,    NULL, &tb,      0);
    OPTENT3(0, "topbottom", OPT_FLAG,    NULL, &tb,      0);
    OPTENT3(0, "xy",        OPT_FLAG,    NULL, &xy,      0);
    OPTENT3(0, "transpose", OPT_FLAG,    NULL, &xy,      0);
    OPTENT3(0, "r90",       OPT_FLAG,    NULL, &r90,     0);
    OPTENT3(0, "rotate90",  OPT_FLAG,    NULL, &r90,     0);
    OPTENT3(0, "ccw",       OPT_FLAG,    NULL, &r90,     0);
    OPTENT3(0, "r180",      OPT_FLAG,    NULL, &r180,    0);
    OPTENT3(0, "rotate180", OPT_FLAG,    NULL, &r180,    0);
    OPTENT3(0, "r270",      OPT_FLAG,    NULL, &r270,    0);
    OPTENT3(0, "rotate270", OPT_FLAG,    NULL, &r270,    0);
    OPTENT3(0, "cw",        OPT_FLAG,    NULL, &r270,    0);
    OPTENT3(0, "null",      OPT_FLAG,    NULL, &null,    0);
    OPTENT3(0, "verbose",   OPT_FLAG,    NULL, &cmdlineP->verbose,       0);
    OPTENT3(0, "memsize",   OPT_UINT,    &memsizeOpt, 
            &memsizeSpec,       0);
    OPTENT3(0, "pagesize",  OPT_UINT,    &cmdlineP->pageSize,
            &pagesizeSpec,      0);
    OPTENT3(0, "xform",     OPT_STRING,  &xformOpt, 
            &xformSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We don't parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (lr + tb + xy + r90 + r180 + r270 + null > 1)
        pm_error("You may specify only one type of flip.");
    if (lr + tb + xy + r90 + r180 + r270 + null == 1) {
        if (lr) {
            cmdlineP->xformCount = 1;
            cmdlineP->xformList[0] = LEFTRIGHT;
        } else if (tb) {
            cmdlineP->xformCount = 1;
            cmdlineP->xformList[0] = TOPBOTTOM;
        } else if (xy) {
            cmdlineP->xformCount = 1;
            cmdlineP->xformList[0] = TRANSPOSE;
        } else if (r90) {
            cmdlineP->xformCount = 2;
            cmdlineP->xformList[0] = TRANSPOSE;
            cmdlineP->xformList[1] = TOPBOTTOM;
        } else if (r180) {
            cmdlineP->xformCount = 2;
            cmdlineP->xformList[0] = LEFTRIGHT;
            cmdlineP->xformList[1] = TOPBOTTOM;
        } else if (r270) {
            cmdlineP->xformCount = 2;
            cmdlineP->xformList[0] = TRANSPOSE;
            cmdlineP->xformList[1] = LEFTRIGHT;
        } else if (null) {
            cmdlineP->xformCount = 0;
        }
    } else if (xformSpec) 
        parseXformOpt(xformOpt, &cmdlineP->xformCount, cmdlineP->xformList);
    else
        pm_error("You must specify an option such as -topbottom to indicate "
                 "what kind of flip you want.");

    if (memsizeSpec) {
        if (memsizeOpt > UINT_MAX / 1024 / 1024)
            pm_error("-memsize value too large: %u MiB.  Maximum this program "
                     "can handle is %u MiB", 
                     memsizeOpt, UINT_MAX / 1024 / 1024);
        cmdlineP->availableMemory = memsizeOpt * 1024 *1024;
    } else
        cmdlineP->availableMemory = UINT_MAX;

    if (!pagesizeSpec)
        cmdlineP->pageSize = 4*1024;         

    if (argc-1 == 0) 
        cmdlineP->inputFilespec = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->inputFilespec = argv[1];
}



struct xformMatrix {
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
};



static void
leftright(struct xformMatrix * const xformP) {
    xformP->a = - xformP->a;
    xformP->c = - xformP->c;
    xformP->e = - xformP->e + 1;
}



static void
topbottom(struct xformMatrix * const xformP) {
    xformP->b = - xformP->b;
    xformP->d = - xformP->d;
    xformP->f = - xformP->f + 1;
}



static void
swap(int * const xP, int * const yP) {

    int const t = *xP;

    *xP = *yP;
    *yP = t;
}



static void
transpose(struct xformMatrix * const xformP) {
    swap(&xformP->a, &xformP->b);
    swap(&xformP->c, &xformP->d);
    swap(&xformP->e, &xformP->f);
}



static void
computeXformMatrix(struct xformMatrix * const xformP, 
                   unsigned int         const xformCount,
                   enum xformType             xformType[]) {

    struct xformMatrix const nullTransform = {1, 0, 0, 1, 0, 0};

    unsigned int i;

    *xformP = nullTransform;   /* initial value */

    for (i = 0; i < xformCount; ++i) {
        switch (xformType[i]) {
        case LEFTRIGHT: 
            leftright(xformP);
            break;
        case TOPBOTTOM:
            topbottom(xformP);
            break;
        case TRANSPOSE:
            transpose(xformP);
            break;
        }
    }
}



static void
bitOrderReverse(unsigned char * const bitrow, 
                unsigned int    const cols) {
/*----------------------------------------------------------------------------
  Reverse the bits in a packed pbm row (1 bit per pixel).  I.e. the leftmost
  bit becomes the rightmost, etc.
-----------------------------------------------------------------------------*/
    unsigned int const lastfullByteIdx = cols/8 - 1;

    if (cols == 0 || bitrow == NULL )
        pm_error("Invalid arguments passed to bitOrderReverse");

    if (cols <= 8)
        bitrow[0] = bitreverse[bitrow[0]] << (8-cols);
    else if (cols % 8 == 0) {
        unsigned int i, j;
        for (i = 0, j = lastfullByteIdx; i <= j; ++i, --j) {
            unsigned char const t = bitreverse[bitrow[j]]; 
            bitrow[j] = bitreverse[bitrow[i]];
            bitrow[i] = t;
        }
    } else {
        unsigned int const m = cols % 8; 

        unsigned int i, j;
            /* Cursors into bitrow[].  i moves from left to center;
               j moves from right to center as bits of bitrow[] are exchanged.
            */
        unsigned char th, tl;  /* 16 bit temp ( th << 8 | tl ) */
        tl = 0;
        for (i = 0, j = lastfullByteIdx+1; i <= lastfullByteIdx/2; ++i, --j) {
            th = bitreverse[bitrow[i]];
            bitrow[i] =
                bitreverse[0xff & ((bitrow[j-1] << 8 | bitrow[j]) >> (8-m))];
            bitrow[j] = 0xff & ((th << 8 | tl) >> m);
            tl = th;
        }
        if (i == j) 
            /* bitrow[] has an odd number of bytes (an even number of
               full bytes; lastfullByteIdx is odd), so we did all but
               the center byte above.  We do the center byte now.
            */
            bitrow[j] = 0xff & ((bitreverse[bitrow[i]] << 8 | tl) >> m);
    }
}



static void
transformRowByRowPbm(struct pam * const inpamP, 
                     struct pam * const outpamP,
                     bool         const reverse) {
/*----------------------------------------------------------------------------
  Transform a PBM image either by flipping it left for right, or just leaving
  it alone, as indicated by 'reverse'.

  Process the image one row at a time and use fast packed PBM bit
  reverse algorithm (where required).
-----------------------------------------------------------------------------*/
    unsigned char * bitrow;
    unsigned int row;

    bitrow = pbm_allocrow_packed(outpamP->width); 

    pbm_writepbminit(outpamP->file, outpamP->width, outpamP->height, 0);

    for (row = 0; row < inpamP->height; ++row) {
        pbm_readpbmrow_packed(inpamP->file,  bitrow, inpamP->width,
                              inpamP->format);

        if (reverse)
            bitOrderReverse(bitrow, inpamP->width);

        pbm_writepbmrow_packed(outpamP->file, bitrow, outpamP->width, 0);
    }
    pbm_freerow_packed(bitrow);
}



static void
transformRowByRowNonPbm(struct pam * const inpamP, 
                        struct pam * const outpamP,
                        bool         const reverse) {
/*----------------------------------------------------------------------------
  Flip an image left for right or leave it alone.

  Process one row at a time.

  This works on any image, but is slower and uses more memory than the
  PBM-only transformRowByRowPbm().
-----------------------------------------------------------------------------*/
    tuple * tuplerow;
    tuple * newtuplerow;
        /* This is not a full tuple row.  It is either an array of pointers
           to the tuples in 'tuplerow' (in reverse order) or just 'tuplerow'
           itself.
        */
    tuple * scratchTuplerow;
    
    unsigned int row;
    
    tuplerow = pnm_allocpamrow(inpamP);
    
    if (reverse) {
        /* Set up newtuplerow[] to point to the tuples of tuplerow[] in
           reverse order.
        */
        unsigned int col;
        
        MALLOCARRAY_NOFAIL(scratchTuplerow, inpamP->width);

        for (col = 0; col < inpamP->width; ++col) 
            scratchTuplerow[col] = tuplerow[inpamP->width - col - 1];
        newtuplerow = scratchTuplerow;
    } else {
        scratchTuplerow = NULL;
        newtuplerow = tuplerow;
    }
    pnm_writepaminit(outpamP);

    for (row = 0; row < inpamP->height ; ++row) {
        pnm_readpamrow(inpamP, tuplerow);
        pnm_writepamrow(outpamP, newtuplerow);
    }
    
    if (scratchTuplerow)
        free(scratchTuplerow);
    pnm_freepamrow(tuplerow);
}



static void
transformRowByRow(struct pam * const inpamP,
                  struct pam * const outpamP,
                  bool         const reverse,
                  bool         const verbose) {

    if (verbose)
        pm_message("Transforming row by row, top to bottom");

    switch (PNM_FORMAT_TYPE(inpamP->format)) {
    case PBM_TYPE:
        transformRowByRowPbm(inpamP, outpamP, reverse);
        break;
    default:
        transformRowByRowNonPbm(inpamP, outpamP, reverse);
        break;
    }
} 



static void
transformRowsBottomTopPbm(struct pam * const inpamP,
                          struct pam * const outpamP,
                          bool         const reverse) { 
/*----------------------------------------------------------------------------
  Flip a PBM image top for bottom.  Iff 'reverse', also flip it left for right.

  Read complete image into memory in packed PBM format; Use fast
  packed PBM bit reverse algorithm (where required).
-----------------------------------------------------------------------------*/
    unsigned int const rows=inpamP->height;

    unsigned char ** bitrow;
    int row;
        
    bitrow = pbm_allocarray_packed(outpamP->width, outpamP->height);
        
    for (row = 0; row < rows; ++row)
        pbm_readpbmrow_packed(inpamP->file, bitrow[row], 
                              inpamP->width, inpamP->format);

    pbm_writepbminit(outpamP->file, outpamP->width, outpamP->height, 0);

    for (row = 0; row < rows; ++row) {
        if (reverse) 
            bitOrderReverse(bitrow[rows-row-1], inpamP->width);

        pbm_writepbmrow_packed(outpamP->file, bitrow[rows - row - 1],
                               outpamP->width, 0);
    }
    pbm_freearray_packed(bitrow, outpamP->height);
}



static void
transformRowsBottomTopNonPbm(struct pam * const inpamP, 
                             struct pam * const outpamP,
                             bool         const reverse) {
/*----------------------------------------------------------------------------
  Read complete image into memory as a tuple array.

  This can do any transformation except a column-for-row transformation,
  on any type of image, but is slower and uses more memory than the 
  PBM-only transformRowsBottomTopPbm().
-----------------------------------------------------------------------------*/
    tuple** tuplerows;
    tuple * scratchTuplerow;
        /* This is not a full tuple row -- just an array of pointers to
           the tuples in 'tuplerows'.
        */
    unsigned int row;

    if (reverse)
        MALLOCARRAY_NOFAIL(scratchTuplerow, inpamP->width);
    else
        scratchTuplerow = NULL;
  
    tuplerows = pnm_allocpamarray(outpamP);

    for (row = 0; row < inpamP->height ; ++row)
        pnm_readpamrow(inpamP, tuplerows[row]);

    pnm_writepaminit(outpamP);

    for (row = 0; row < inpamP->height ; ++row) {
        tuple * newtuplerow;
        tuple * const tuplerow = tuplerows[inpamP->height - row - 1];
        if (reverse) {
            unsigned int col;
            newtuplerow = scratchTuplerow;
            for (col = 0; col < inpamP->width; ++col) 
                newtuplerow[col] = tuplerow[inpamP->width - col - 1];
        } else
            newtuplerow = tuplerow;
        pnm_writepamrow(outpamP, newtuplerow);
    }

    if (scratchTuplerow)
        free(scratchTuplerow);
    
    pnm_freepamarray(tuplerows, outpamP);
}



static void
transformRowsBottomTop(struct pam * const inpamP,
                       struct pam * const outpamP,
                       bool         const reverse,
                       bool         const verbose) {

    if (PNM_FORMAT_TYPE(inpamP->format) == PBM_TYPE) {
        if (verbose)
            pm_message("Transforming PBM row by row, bottom to top");
        transformRowsBottomTopPbm(inpamP, outpamP, reverse);
    } else {
        if (verbose)
            pm_message("Transforming non-PBM row by row, bottom to top");
        transformRowsBottomTopNonPbm(inpamP, outpamP, reverse);
    }
}



static void __inline__
transformPoint(int                const col, 
               int                const newcols,
               int                const row, 
               int                const newrows, 
               struct xformMatrix const xform, 
               int *              const newcolP, 
               int *              const newrowP ) {
/*----------------------------------------------------------------------------
   Compute the location in the output of a pixel that is at row 'row',
   column 'col' in the input.  Assume the output image is 'newcols' by
   'newrows' and the transformation is as described by 'xform'.

   Return the output image location of the pixel as *newcolP and *newrowP.
-----------------------------------------------------------------------------*/
    /* The transformation is:
     
                 [ a b 0 ]
       [ x y 1 ] [ c d 0 ] = [ x2 y2 1 ]
                 [ e f 1 ]
    */
    *newcolP = xform.a * col + xform.c * row + xform.e * (newcols - 1);
    *newrowP = xform.b * col + xform.d * row + xform.f * (newrows - 1);
}



static void
transformPbm(struct pam *       const inpamP,
             struct pam *       const outpamP,
             struct xformMatrix const xform) { 
/*----------------------------------------------------------------------------
   This is the same as transformGen, except that it uses less 
   memory, since the PBM buffer format uses one bit per pixel instead
   of twelve bytes + pointer space

   This can do any PBM transformation, but is slower and uses more
   memory than the more restricted transformRowByRowPbm() and
   transformRowsBottomTopPbm().
-----------------------------------------------------------------------------*/
    bit* bitrow;
    bit** newbits;
    int row;
            
    bitrow = pbm_allocrow(inpamP->width);
    newbits = pbm_allocarray(pbm_packed_bytes(outpamP->width), 
                             outpamP->height);
            
    /* Initialize entire array to zeroes.  One bits will be or'ed in later */
    for (row = 0; row < outpamP->height; ++row) {
        int col;
        for (col = 0; col < pbm_packed_bytes(outpamP->width); ++col) 
             newbits[row][col] = 0; 
    }
    
    for (row = 0; row < inpamP->height; ++row) {
        int col;
        pbm_readpbmrow(inpamP->file, bitrow, inpamP->width, inpamP->format);
        for (col = 0; col < inpamP->width; ++col) {
            int newcol, newrow;
            transformPoint(col, outpamP->width, row, outpamP->height, xform,
                           &newcol, &newrow);
            newbits[newrow][newcol/8] |= bitrow[col] << (7 - newcol % 8);
                /* Use of "|=" patterned after pbm_readpbmrow_packed. */
         }
    }
    
    pbm_writepbminit(outpamP->file, outpamP->width, outpamP->height, 0);
    for (row = 0; row < outpamP->height; ++row)
        pbm_writepbmrow_packed(outpamP->file, newbits[row], outpamP->width, 
                               0);
    
    pbm_freearray(newbits, outpamP->height);
    pbm_freerow(bitrow);
}



static unsigned int 
optimalSegmentSize(struct xformMatrix  const xform, 
                   struct pam *        const pamP,
                   unsigned int        const availableMemory,
                   unsigned int        const pageSize) {
/*----------------------------------------------------------------------------
   Compute the maximum number of columns that can be transformed, one row
   at a time, without causing page thrashing.

   See comments at the top of this file for an explanation of the kind
   of page thrashing using segments avoids.

   'availableMemory' is the amount of real memory in bytes that this
   process should expect to be able to use.

   'pageSize' is the size of a page in bytes.  A page means the unit that
   is paged in or out.
   
   'pamP' describes the storage required to represent a row of the
   output array.
-----------------------------------------------------------------------------*/
    unsigned int segmentSize;

    if (xform.b == 0)
        segmentSize = pamP->width;
    else {
        unsigned int const otherNeeds = 200*1024;
            /* A wild guess at how much real memory is needed by the program
               for purposes other than the output tuple array.
            */
        if (otherNeeds > availableMemory)
            segmentSize = pamP->width;  /* Can't prevent thrashing */
        else {
            unsigned int const availablePages = 
                (availableMemory - otherNeeds) / pageSize;
            if (availablePages <= 1)
                segmentSize = pamP->width; /* Can't prevent thrashing */
            else {
                unsigned int const bytesPerRow = 
                    pamP->width * pamP->depth * pamP->bytes_per_sample;
                unsigned int rowsPerPage = 
                    MAX(1, (pageSize + (pageSize/2)) / bytesPerRow);
                    /* This is how many consecutive rows we can touch
                       on average while staying within the same page.  
                    */
                segmentSize = availablePages * rowsPerPage;
            }
        }
    }    
    return segmentSize;
}



static void
transformNonPbm(struct pam *       const inpamP,
                struct pam *       const outpamP,
                struct xformMatrix const xform,
                unsigned int       const segmentSize,
                bool               const verbose) {
/*----------------------------------------------------------------------------
  Do the transform using "pam" library functions, as opposed to "pbm"
  ones.

  Assume input file is positioned to the raster (just after the
  header).

  'segmentSize' is the number of columns we are to process in each
  pass.  We do each segment going from left to right.  For each
  segment, we do each row, going from top to bottom.  For each row of
  the segment, we do each column, going from left to right.  (The
  reason Caller wants it done by segments is to improve virtual memory
  reference locality.  See comments at the top of this file).

  if 'segmentSize' is less than the whole image, ifP must be a seekable
  file.

  This can do any transformation, but is slower and uses more memory
  than the PBM-only transformPbm().
-----------------------------------------------------------------------------*/
    pm_filepos imagepos;
        /* The input file position of the raster.  But defined only if
           segment size is less than whole image.
        */
    tuple* tuplerow;
    tuple** newtuples;
    unsigned int startCol;

    tuplerow = pnm_allocpamrow(inpamP);
    newtuples = pnm_allocpamarray(outpamP);
    
    if (segmentSize < inpamP->width)
        pm_tell2(inpamP->file, &imagepos, sizeof(imagepos));

    for (startCol = 0; startCol < inpamP->width; startCol += segmentSize) {
        /* Do one set of columns which is small enough not to cause
           page thrashing.
        */
        unsigned int const endCol = MIN(inpamP->width, startCol + segmentSize);
        unsigned int row;

        if (verbose)
            pm_message("Transforming Columns %u up to %u", 
                       startCol, endCol);

        if (startCol > 0)
            /* Go back to read from Row 0 again */
            pm_seek2(inpamP->file, &imagepos, sizeof(imagepos));

        for (row = 0; row < inpamP->height; ++row) {
            unsigned int col;
            pnm_readpamrow(inpamP, tuplerow);

            for (col = startCol; col < endCol; ++col) {
                int newcol, newrow;
                transformPoint(col, outpamP->width, row, outpamP->height, 
                               xform,
                               &newcol, &newrow);
                pnm_assigntuple(inpamP, newtuples[newrow][newcol],
                                tuplerow[col]);
            }
        }
    }
    
    pnm_writepam(outpamP, newtuples);
    
    pnm_freepamarray(newtuples, outpamP);
    pnm_freepamrow(tuplerow);
}



static void
transformGen(struct pam *       const inpamP,
             struct pam *       const outpamP,
             struct xformMatrix const xform,
             unsigned int       const availableMemory,
             unsigned int       const pageSize,
             bool               const verbose) {
/*----------------------------------------------------------------------------
  Produce the transformed output on Standard Output.

  Assume input file is positioned to the raster (just after the
  header).

  This can transform any image in any way, but is slower and uses more
  memory than the more restricted transformRowByRow() and
  transformRowsBottomTop().
-----------------------------------------------------------------------------*/
    unsigned int const segmentSize = 
        optimalSegmentSize(xform, outpamP, availableMemory, pageSize);
    
    switch (PNM_FORMAT_TYPE(inpamP->format)) {
    case PBM_TYPE: 
        transformPbm(inpamP, outpamP, xform);
        break;
    default:
        if (segmentSize < outpamP->width) {
            if (verbose && xform.b !=0)
                pm_message("Transforming %u columns of %u total at a time", 
                           segmentSize, outpamP->width);
            else
                pm_message("Transforming entire image at once");
        }
        transformNonPbm(inpamP, outpamP, xform, segmentSize, verbose);
        break;
    }
}



int
main(int argc, char * argv[]) {
    struct cmdlineInfo cmdline;
    struct pam inpam;
    struct pam outpam;
    FILE* ifP;
    struct xformMatrix xform;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    if (cmdline.availableMemory < UINT_MAX)
        ifP = pm_openr_seekable(cmdline.inputFilespec);
    else
        ifP = pm_openr(cmdline.inputFilespec);
    
    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    computeXformMatrix(&xform, cmdline.xformCount, cmdline.xformList);

    outpam = inpam;  /* initial value */

    outpam.file = stdout;
    outpam.width  = abs(xform.a) * inpam.width + abs(xform.c) * inpam.height;
    outpam.height = abs(xform.b) * inpam.width + abs(xform.d) * inpam.height;
    
    if (xform.b == 0 && xform.d == 1 && xform.f == 0)
        /* In this case Row N of the output is based only on Row N of
           the input, so we can transform row by row and avoid
           in-memory buffering altogether.  
        */
        transformRowByRow(&inpam, &outpam, xform.a == -1, cmdline.verbose);
    else if (xform.b == 0 && xform.c == 0) 
        /* In this case, Row N of the output is based only on Row ~N of the
           input.  We need all the rows in memory, but have to pass
           through them only twice, so there is no page thrashing concern.
        */
        transformRowsBottomTop(&inpam, &outpam, xform.a == -1, 
                               cmdline.verbose);
    else
        /* This is a colum-for-row type of transformation, which requires
           complex traversal of an in-memory image.
        */
        transformGen(&inpam, &outpam, xform,
                     cmdline.availableMemory, cmdline.pageSize, 
                     cmdline.verbose);

    pm_close(inpam.file);
    pm_close(outpam.file);
    
    return 0;
}
