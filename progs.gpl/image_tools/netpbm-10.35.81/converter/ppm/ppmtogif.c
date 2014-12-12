/* ppmtogif.c - read a portable pixmap and produce a GIF file
**
** Based on GIFENCOD by David Rowley <mgardi@watdscu.waterloo.edu>.A
** Lempel-Zim compression based on "compress".
**
** Modified by Marcel Wijkstra <wijkstra@fwi.uva.nl>
**
** The non-LZW GIF generation stuff was adapted from the Independent
** JPEG Group's djpeg on 2001.09.29.  The uncompressed output subroutines
** are derived directly from the corresponding subroutines in djpeg's
** wrgif.c source file.  Its copyright notice say:

 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
   The reference README file is README.JPEG in the Netpbm package.
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** The Graphics Interchange Format(c) is the Copyright property of
** CompuServe Incorporated.  GIF(sm) is a Service Mark property of
** CompuServe Incorporated.
*/

/* TODO: merge the LZW and uncompressed subroutines.  They are separate
   only because they had two different lineages and the code is too
   complicated for me quickly to rewrite it.
*/
#include <assert.h>
#include <string.h>

#include "mallocvar.h"
#include "shhopt.h"
#include "ppm.h"

#define MAXCMAPSIZE 256

static unsigned int const gifMaxval = 255;

static bool verbose;
/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
typedef int code_int;

typedef long int          count_int;


struct cmap {
    /* This is the information for the GIF colormap (aka palette). */

    int red[MAXCMAPSIZE], green[MAXCMAPSIZE], blue[MAXCMAPSIZE];
        /* These arrays arrays map a color index, as is found in
           the raster part of the GIF, to an intensity value for the indicated
           RGB component.
        */
    int perm[MAXCMAPSIZE], permi[MAXCMAPSIZE];
        /* perm[i] is the position in the sorted colormap of the color which
           is at position i in the unsorted colormap.  permi[] is the inverse
           function of perm[].
        */
    int cmapsize;
        /* Number of entries in the GIF colormap.  I.e. number of colors
           in the image, plus possibly one fake transparency color.
        */
    int transparent;
        /* color index number in GIF palette of the color that is to be
           transparent.  -1 if no color is transparent.
        */
    colorhash_table cht;
        /* A hash table that relates a PPM pixel value to to a pre-sort
           GIF colormap index.
        */
    pixval maxval;
        /* The maxval for the colors in 'cht'. */
};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespec of input file */
    const char *alpha_filespec;  /* Filespec of alpha file; NULL if none */
    const char *alphacolor;      /* -alphacolor option value or default */
    unsigned int interlace; /* -interlace option value */
    unsigned int sort;     /* -sort option value */
    const char *mapfile;        /* -mapfile option value.  NULL if none. */
    const char *transparent;    /* -transparent option value.  NULL if none. */
    const char *comment;        /* -comment option value; NULL if none */
    unsigned int nolzw;    /* -nolzw option */
    unsigned int verbose;
};


static void
handleLatex2htmlHack(void) {
/*----------------------------------------------------------------------------
  This program used to put out a "usage" message when it saw an option
  it didn't understand.  Latex2html's configure program does a
  ppmtogif -h (-h was never a valid option) to elicit that message and
  then parses the message to see if it included the strings
  "-interlace" and "-transparent".  That way it knows if the
  'ppmtogif' program it found has those options or not.  I don't think
  any 'ppmtogif' you're likely to find today lacks those options, but
  latex2html checks anyway, and we don't want it to conclude that we
  don't have them.

  So we issue a special error message just to trick latex2html into
  deciding that we have -interlace and -transparent options.  The function
  is not documented in the man page.  We would like to see Latex2html 
  either stop checking or check like configure programs usually do -- 
  try the option and see if you get success or failure.

  -Bryan 2001.11.14
-----------------------------------------------------------------------------*/
     pm_error("latex2html, you should just try the -interlace and "
             "-transparent options to see if they work instead of "
             "expecting a 'usage' message from -h");
}



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Parse the program arguments (given by argc and argv) into a form
   the program can deal with more easily -- a cmdline_info structure.
   If the syntax is invalid, issue a message and exit the program via
   pm_error().

   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry * option_def;  /* malloc'ed */
    optStruct3 opt;  /* set by OPTENT3 */
    unsigned int option_def_index;

    unsigned int latex2htmlhack;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "interlace",   OPT_FLAG,   
            NULL,                       &cmdlineP->interlace, 0);
    OPTENT3(0,   "sort",        OPT_FLAG,   
            NULL,                       &cmdlineP->sort, 0);
    OPTENT3(0,   "nolzw",       OPT_FLAG,   
            NULL,                       &cmdlineP->nolzw, 0);
    OPTENT3(0,   "mapfile",     OPT_STRING, 
            &cmdlineP->mapfile,        NULL, 0);
    OPTENT3(0,   "transparent", OPT_STRING, 
            &cmdlineP->transparent,    NULL, 0);
    OPTENT3(0,   "comment",     OPT_STRING, 
            &cmdlineP->comment,        NULL, 0);
    OPTENT3(0,   "alpha",       OPT_STRING, 
            &cmdlineP->alpha_filespec, NULL, 0);
    OPTENT3(0,   "alphacolor",  OPT_STRING, 
            &cmdlineP->alphacolor,     NULL, 0);
    OPTENT3(0,   "h",           OPT_FLAG, 
            NULL,                       &latex2htmlhack, 0);
    OPTENT3(0,   "verbose",     OPT_FLAG, 
            NULL,                       &cmdlineP->verbose, 0);
    
    /* Set the defaults */
    cmdlineP->mapfile = NULL;
    cmdlineP->transparent = NULL;  /* no transparency */
    cmdlineP->comment = NULL;      /* no comment */
    cmdlineP->alpha_filespec = NULL;      /* no alpha file */
    cmdlineP->alphacolor = "rgb:0/0/0";      
        /* We could say "black" here, but then we depend on the color names
           database existing.
        */

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (latex2htmlhack) 
        handleLatex2htmlHack();

    if (argc-1 == 0) 
        cmdlineP->input_filespec = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->input_filespec = argv[1];

    if (cmdlineP->alpha_filespec && cmdlineP->transparent)
        pm_error("You cannot specify both -alpha and -transparent.");
}



/*
 * Write out a word to the GIF file
 */
static void
Putword(int const w, FILE * const fp) {

    fputc( w & 0xff, fp );
    fputc( (w / 256) & 0xff, fp );
}


static int
closestcolor(pixel         const color,
             pixval        const maxval,
             struct cmap * const cmapP) {
/*----------------------------------------------------------------------------
   Return the pre-sort colormap index of the color in the colormap *cmapP
   that is closest to the color 'color', whose maxval is 'maxval'.

   Also add 'color' to the colormap hash, with the colormap index we
   are returning.  Caller must ensure that the color is not already in
   there.
-----------------------------------------------------------------------------*/
    unsigned int i;
    unsigned int imin, dmin;

    pixval const r = PPM_GETR(color) * gifMaxval / maxval;
    pixval const g = PPM_GETG(color) * gifMaxval / maxval;
    pixval const b = PPM_GETB(color) * gifMaxval / maxval;

    dmin = SQR(255) * 3;
    imin = 0;
    for (i=0;i < cmapP->cmapsize; i++) {
        int const d = SQR(r-cmapP->red[i]) + 
            SQR(g-cmapP->green[i]) + 
            SQR(b-cmapP->blue[i]);
        if (d < dmin) {
            dmin = d;
            imin = i; 
        } 
    }
    ppm_addtocolorhash(cmapP->cht, &color, cmapP->permi[imin]);

    return cmapP->permi[imin];
}



enum pass {MULT8PLUS0, MULT8PLUS4, MULT4PLUS2, MULT2PLUS1};


struct pixelCursor {
    unsigned int width;
        /* Width of the image, in columns */
    unsigned int height;
        /* Height of the image, in rows */
    bool interlace;
        /* We're accessing the image in interlace fashion */
    unsigned int nPixelsLeft;
        /* Number of pixels left to be read */
    unsigned int curCol;
        /* Location of pointed-to pixel, column */
    unsigned int curRow;
        /* Location of pointed-to pixel, row */
    enum pass pass;
        /* The interlace pass.  Undefined if !interlace */
};



static struct pixelCursor pixelCursor;
    /* Current location in the input pixels.  */


static void
initPixelCursor(unsigned int const width,
                unsigned int const height,
                bool         const interlace) {

    pixelCursor.width       = width;
    pixelCursor.height      = height;
    pixelCursor.interlace   = interlace;
    pixelCursor.pass        = MULT8PLUS0;
    pixelCursor.curCol      = 0;
    pixelCursor.curRow      = 0;
    pixelCursor.nPixelsLeft = width * height;
}



static void
getPixel(pixel **           const pixels,
         pixval             const inputMaxval,
         gray **            const alpha,
         gray               const alphaThreshold, 
         struct cmap *      const cmapP,
         struct pixelCursor const pixelCursor,
         int *              const retvalP) {
/*----------------------------------------------------------------------------
   Return as *retvalP the colormap index of the pixel at location
   pointed to by 'pixelCursor' in the PPM raster 'pixels', using
   colormap *cmapP.
-----------------------------------------------------------------------------*/
    unsigned int const x = pixelCursor.curCol;
    unsigned int const y = pixelCursor.curRow;

    int colorindex;

    if (alpha && alpha[y][x] < alphaThreshold)
        colorindex = cmapP->transparent;
    else {
        int presortColorindex;

        presortColorindex = ppm_lookupcolor(cmapP->cht, &pixels[y][x]);
        if (presortColorindex == -1)
            presortColorindex = 
                closestcolor(pixels[y][x], inputMaxval, cmapP);
        colorindex = cmapP->perm[presortColorindex];
    }
    *retvalP = colorindex;
}



static void
bumpRowInterlace(struct pixelCursor * const pixelCursorP) {
/*----------------------------------------------------------------------------
   Move *pixelCursorP to the next row in the interlace pattern.
-----------------------------------------------------------------------------*/
    /* There are 4 passes:
       MULT8PLUS0: Rows 8, 16, 24, 32, etc.
       MULT8PLUS4: Rows 4, 12, 20, 28, etc.
       MULT4PLUS2: Rows 2, 6, 10, 14, etc.
       MULT2PLUS1: Rows 1, 3, 5, 7, 9, etc.
    */
    
    switch (pixelCursorP->pass) {
    case MULT8PLUS0:
        pixelCursorP->curRow += 8;
        break;
    case MULT8PLUS4:
        pixelCursorP->curRow += 8;
        break;
    case MULT4PLUS2:
        pixelCursorP->curRow += 4;
        break;
    case MULT2PLUS1:
        pixelCursorP->curRow += 2;
        break;
    }
    /* Set the proper pass for the next read.  Note that if there are
       more than 4 rows, the sequence of passes is sequential, but
       when there are fewer than 4, we may skip e.g. from MULT8PLUS0
       to MULT4PLUS2.
    */
    while (pixelCursorP->curRow >= pixelCursorP->height) {
        switch (pixelCursorP->pass) {
        case MULT8PLUS0:
            pixelCursorP->pass = MULT8PLUS4;
            pixelCursorP->curRow = 4;
            break;
        case MULT8PLUS4:
            pixelCursorP->pass = MULT4PLUS2;
            pixelCursorP->curRow = 2;
            break;
        case MULT4PLUS2:
            pixelCursorP->pass = MULT2PLUS1;
            pixelCursorP->curRow = 1;
            break;
        case MULT2PLUS1:
            /* We've read the entire image; pass and current row are
               now undefined.
            */
            pixelCursorP->curRow = 0;
            break;
        }
    }
}



static void
bumpPixel(struct pixelCursor * const pixelCursorP) {
/*----------------------------------------------------------------------------
   Bump *pixelCursorP to point to the next pixel to go into the GIF

   Must not call when there are no pixels left.
-----------------------------------------------------------------------------*/
    assert(pixelCursorP->nPixelsLeft > 0);

    /* Move one column to the right */
    ++pixelCursorP->curCol;
    
    if (pixelCursorP->curCol >= pixelCursorP->width) {
        /* That pushed us past the end of a row. */
        /* Reset to the left edge ... */
        pixelCursorP->curCol = 0;
        
        /* ... of the next row */
        if (!pixelCursorP->interlace)
            /* Go to the following row */
            ++pixelCursorP->curRow;
        else
            bumpRowInterlace(pixelCursorP);
    }
    --pixelCursorP->nPixelsLeft;
}



static int
gifNextPixel(pixel **      const pixels,
             pixval        const inputMaxval,
             gray **       const alpha,
             gray          const alphaThreshold, 
             struct cmap * const cmapP) {
/*----------------------------------------------------------------------------
   Return the pre-sort color index (index into the unsorted GIF color map)
   of the next pixel to be processed from the input image.

   'alpha_threshold' is the gray level such that a pixel in the alpha
   map whose value is less that that represents a transparent pixel
   in the output.
-----------------------------------------------------------------------------*/
    int retval;

    if (pixelCursor.nPixelsLeft == 0 )
        retval = EOF;
    else {
        getPixel(pixels, inputMaxval, alpha, alphaThreshold, cmapP, 
                 pixelCursor, &retval);

        bumpPixel(&pixelCursor);
    }
    return retval;
}



static void
write_transparent_color_index_extension(FILE *fp, const int Transparent) {
/*----------------------------------------------------------------------------
   Write out extension for transparent color index.
-----------------------------------------------------------------------------*/

    fputc( '!', fp );
    fputc( 0xf9, fp );
    fputc( 4, fp );
    fputc( 1, fp );
    fputc( 0, fp );
    fputc( 0, fp );
    fputc( Transparent, fp );
    fputc( 0, fp );
}



static void
write_comment_extension(FILE *fp, const char comment[]) {
/*----------------------------------------------------------------------------
   Write out extension for a comment
-----------------------------------------------------------------------------*/
    char *segment;
    
    fputc('!', fp);   /* Identifies an extension */
    fputc(0xfe, fp);  /* Identifies a comment */

    /* Write it out in segments no longer than 255 characters */
    for (segment = (char *) comment; 
         segment < comment+strlen(comment); 
         segment += 255) {

        const int length_this_segment = MIN(255, strlen(segment));

        fputc(length_this_segment, fp);

        fwrite(segment, 1, length_this_segment, fp);
    }

    fputc(0, fp);   /* No more comment blocks in this extension */
}



/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/

/*
 * General DEFINEs
 */

#define BITS    12

#define HSIZE  5003            /* 80% occupancy */

#ifdef NO_UCHAR
 typedef char   char_type;
#else /*NO_UCHAR*/
 typedef        unsigned char   char_type;
#endif /*NO_UCHAR*/

/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */
#include <ctype.h>

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

static code_int const maxmaxcode = (code_int)1 << BITS;
    /* should NEVER generate this code */
#define MAXCODE(n_bits)        (((code_int) 1 << (n_bits)) - 1)

static long htab [HSIZE];
static unsigned short codetab [HSIZE];
#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i)        ((char_type*)(htab))[i]
#define de_stack               ((char_type*)&tab_suffixof((code_int)1<<BITS))

static code_int free_ent = 0;                  /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg = 0;

static int offset;
static long int in_count = 1;            /* length of input */
static long int out_count = 0;           /* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static int ClearCode;
static int EOFCode;

/***************************************************************************
*                          BYTE OUTPUTTER                 
***************************************************************************/

typedef struct {
    FILE * fileP;  /* The file to which to output */
    unsigned int count;
        /* Number of bytes so far in the current data block */
    unsigned char buffer[256];
        /* The current data block, under construction */
} byteBuffer;



static byteBuffer *
byteBuffer_create(FILE * const fileP) {

    byteBuffer * byteBufferP;

    MALLOCVAR_NOFAIL(byteBufferP);

    byteBufferP->fileP = fileP;
    byteBufferP->count = 0;

    return byteBufferP;
}



static void
byteBuffer_destroy(byteBuffer * const byteBufferP) {

    free(byteBufferP);
}



static void
byteBuffer_flush(byteBuffer * const byteBufferP) {
/*----------------------------------------------------------------------------
   Write the current data block to the output file, then reset the current 
   data block to empty.
-----------------------------------------------------------------------------*/
    if (byteBufferP->count > 0 ) {
        if (verbose)
            pm_message("Writing %u byte block", byteBufferP->count);
        fputc(byteBufferP->count, byteBufferP->fileP);
        fwrite(byteBufferP->buffer, 1, byteBufferP->count, byteBufferP->fileP);
        byteBufferP->count = 0;
    }
}



static void
byteBuffer_flushFile(byteBuffer * const byteBufferP) {
    
    fflush(byteBufferP->fileP);
    
    if (ferror(byteBufferP->fileP))
        pm_error("error writing output file");
}



static void
byteBuffer_out(byteBuffer *  const byteBufferP,
               unsigned char const c) {
/*----------------------------------------------------------------------------
  Add a byte to the end of the current data block, and if it is now 254
  characters, flush the data block to the output file.
-----------------------------------------------------------------------------*/
    byteBufferP->buffer[byteBufferP->count++] = c;
    if (byteBufferP->count >= 254)
        byteBuffer_flush(byteBufferP);
}



struct gif_dest {
    /* This structure controls output of uncompressed GIF raster */

    byteBuffer * byteBufferP;  /* Where the full bytes go */

    /* State for packing variable-width codes into a bitstream */
    int n_bits;         /* current number of bits/code */
    int maxcode;        /* maximum code, given n_bits */
    int cur_accum;      /* holds bits not yet output */
    int cur_bits;       /* # of bits in cur_accum */

    /* State for GIF code assignment */
    int ClearCode;      /* clear code (doesn't change) */
    int EOFCode;        /* EOF code (ditto) */
    int code_counter;   /* counts output symbols */
};



static unsigned long const masks[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
                                       0x001F, 0x003F, 0x007F, 0x00FF,
                                       0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                                       0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

typedef struct {
    byteBuffer * byteBufferP;
    unsigned int initBits;
    unsigned int n_bits;                        /* number of bits/code */
    code_int maxcode;                  /* maximum code, given n_bits */
    unsigned long curAccum;
    int curBits;
} codeBuffer;



static codeBuffer *
codeBuffer_create(FILE *       const ofP,
                  unsigned int const initBits) {

    codeBuffer * codeBufferP;

    MALLOCVAR_NOFAIL(codeBufferP);

    codeBufferP->initBits    = initBits;
    codeBufferP->n_bits      = codeBufferP->initBits;
    codeBufferP->maxcode     = MAXCODE(codeBufferP->n_bits);
    codeBufferP->byteBufferP = byteBuffer_create(ofP);
    codeBufferP->curAccum    = 0;
    codeBufferP->curBits     = 0;

    return codeBufferP;
}



static void
codeBuffer_destroy(codeBuffer * const codeBufferP) {

    byteBuffer_destroy(codeBufferP->byteBufferP);

    free(codeBufferP);
}



static void
codeBuffer_output(codeBuffer * const codeBufferP,
                  code_int     const code) {
/*----------------------------------------------------------------------------
   Output one GIF code to the file, through the code buffer.

   The code is represented as n_bits bits in the file -- the lower
   n_bits bits of 'code'.

   If the code is EOF, flush the code buffer to the file.

   In some cases, change n_bits and recalculate maxcode to go with it.
-----------------------------------------------------------------------------*/
    /*
      Algorithm:
      Maintain a BITS character long buffer (so that 8 codes will
      fit in it exactly).  Use the VAX insv instruction to insert each
      code in turn.  When the buffer fills up empty it and start over.
    */
    
    codeBufferP->curAccum &= masks[codeBufferP->curBits];

    if (codeBufferP->curBits > 0)
        codeBufferP->curAccum |= ((long)code << codeBufferP->curBits);
    else
        codeBufferP->curAccum = code;

    codeBufferP->curBits += codeBufferP->n_bits;

    while (codeBufferP->curBits >= 8) {
        byteBuffer_out(codeBufferP->byteBufferP,
                       codeBufferP->curAccum & 0xff);
        codeBufferP->curAccum >>= 8;
        codeBufferP->curBits -= 8;
    }

    if (clear_flg) {
        codeBufferP->n_bits = codeBufferP->initBits;
        codeBufferP->maxcode = MAXCODE(codeBufferP->n_bits);
        clear_flg = 0;
    } else if (free_ent > codeBufferP->maxcode) {
        /* The next entry is going to be too big for the code size, so
           increase it, if possible.
        */
        ++codeBufferP->n_bits;
        if (codeBufferP->n_bits == BITS)
            codeBufferP->maxcode = maxmaxcode;
        else
            codeBufferP->maxcode = MAXCODE(codeBufferP->n_bits);
    }
    
    if (code == EOFCode) {
        /* We're at EOF.  Output the possible partial byte in the buffer */
        if (codeBufferP->curBits > 0) {
            byteBuffer_out(codeBufferP->byteBufferP,
                           codeBufferP->curAccum & 0xff);
            codeBufferP->curBits = 0;
        }
        byteBuffer_flush(codeBufferP->byteBufferP);
        
        byteBuffer_flushFile(codeBufferP->byteBufferP);
    }
}



static void
cl_hash(long const hsize) {
    /* reset code table */

    long const m1 = -1;

    long * htab_p;
    long i;

    htab_p = htab + hsize;  /* initial value */

    i = hsize - 16;
    do {                            /* might use Sys V memset(3) here */
        *(htab_p-16) = m1;
        *(htab_p-15) = m1;
        *(htab_p-14) = m1;
        *(htab_p-13) = m1;
        *(htab_p-12) = m1;
        *(htab_p-11) = m1;
        *(htab_p-10) = m1;
        *(htab_p-9) = m1;
        *(htab_p-8) = m1;
        *(htab_p-7) = m1;
        *(htab_p-6) = m1;
        *(htab_p-5) = m1;
        *(htab_p-4) = m1;
        *(htab_p-3) = m1;
        *(htab_p-2) = m1;
        *(htab_p-1) = m1;
        htab_p -= 16;
    } while ((i -= 16) >= 0);

    for (i += 16; i > 0; --i)
        *--htab_p = m1;
}



static void
cl_block(codeBuffer * const codeBufferP) {
/*----------------------------------------------------------------------------
  Clear out the hash table
-----------------------------------------------------------------------------*/
    cl_hash(HSIZE);
    free_ent = ClearCode + 2;
    clear_flg = 1;
    
    codeBuffer_output(codeBufferP, (code_int)ClearCode);
}



static void
write_raster_LZW(pixel **      const pixels,
                 pixval        const input_maxval,
                 gray **       const alpha,
                 gray          const alpha_maxval, 
                 struct cmap * const cmapP, 
                 int           const initBits,
                 FILE *        const ofP) {
/*----------------------------------------------------------------------------
   Write the raster to file 'ofP'.

   The raster to write is 'pixels', which has maxval 'input_maxval',
   modified by alpha mask 'alpha', which has maxval 'alpha_maxval'.

   Use the colormap 'cmapP' to generate the raster ('pixels' is 
   composed of RGB samples; the GIF raster is colormap indices).

   Write the raster using LZW compression.
-----------------------------------------------------------------------------*/
    gray const alpha_threshold = (alpha_maxval + 1) / 2;
        /* gray levels below this in the alpha mask indicate transparent
           pixels in the output image.
        */
    code_int ent;
    code_int disp;
    int hshift;
    bool eof;
    codeBuffer * codeBufferP;
    
    codeBufferP = codeBuffer_create(ofP, initBits);
    
    /*
     * Set up the necessary values
     */
    offset = 0;
    out_count = 0;
    clear_flg = 0;
    in_count = 1;

    ClearCode = (1 << (initBits - 1));
    EOFCode = ClearCode + 1;
    free_ent = ClearCode + 2;

    ent = gifNextPixel(pixels, input_maxval, alpha, alpha_threshold, cmapP);

    {
        long fcode;
        hshift = 0;
        for (fcode = HSIZE; fcode < 65536L; fcode *= 2L)
            ++hshift;
        hshift = 8 - hshift;                /* set hash code range bound */
    }
    cl_hash(HSIZE);            /* clear hash table */

    codeBuffer_output(codeBufferP, (code_int)ClearCode);

    eof = FALSE;
    while (!eof) {
        int gifpixel;
            /* The value for the pixel in the GIF image.  I.e. the colormap
               index.  Or -1 to indicate "no more pixels."
            */
        gifpixel = gifNextPixel(pixels, 
                                input_maxval, alpha, alpha_threshold, cmapP);
        if (gifpixel == EOF) eof = TRUE;
        if (!eof) {
            long const fcode = (long) (((long) gifpixel << BITS) + ent);
            code_int i;
                /* xor hashing */

            ++in_count;

            i = (((code_int)gifpixel << hshift) ^ ent);    

            if (HashTabOf (i) == fcode) {
                ent = CodeTabOf (i);
                continue;
            } else if ((long)HashTabOf(i) < 0)      /* empty slot */
                goto nomatch;
            disp = HSIZE - i;        /* secondary hash (after G. Knott) */
            if (i == 0)
                disp = 1;
        probe:
            if ((i -= disp) < 0)
                i += HSIZE;

            if (HashTabOf(i) == fcode) {
                ent = CodeTabOf(i);
                continue;
            }
            if ((long)HashTabOf(i) > 0)
                goto probe;
        nomatch:
            codeBuffer_output(codeBufferP, (code_int)ent);
            ++out_count;
            ent = gifpixel;
            if (free_ent < maxmaxcode) {
                CodeTabOf(i) = free_ent++; /* code -> hashtable */
                HashTabOf(i) = fcode;
            } else
                cl_block(codeBufferP);
        }
    }
    /* Put out the final code. */
    codeBuffer_output(codeBufferP, (code_int)ent);
    ++out_count;
    codeBuffer_output(codeBufferP, (code_int) EOFCode);

    codeBuffer_destroy(codeBufferP);
}



/* Routine to convert variable-width codes into a byte stream */

static void
outputUncompressed(struct gif_dest * const dinfoP,
                   int               const code) {

    /* Emit a code of n_bits bits */
    /* Uses cur_accum and cur_bits to reblock into 8-bit bytes */
    dinfoP->cur_accum |= ((int) code) << dinfoP->cur_bits;
    dinfoP->cur_bits += dinfoP->n_bits;

    while (dinfoP->cur_bits >= 8) {
        byteBuffer_out(dinfoP->byteBufferP, dinfoP->cur_accum & 0xFF);
        dinfoP->cur_accum >>= 8;
        dinfoP->cur_bits -= 8;
    }
}


static void
writeRasterUncompressedInit(FILE *            const ofP,
                            struct gif_dest * const dinfoP, 
                            int               const i_bits) {
/*----------------------------------------------------------------------------
   Initialize pseudo-compressor
-----------------------------------------------------------------------------*/

    /* init all the state variables */
    dinfoP->n_bits = i_bits;
    dinfoP->maxcode = MAXCODE(dinfoP->n_bits);
    dinfoP->ClearCode = (1 << (i_bits - 1));
    dinfoP->EOFCode = dinfoP->ClearCode + 1;
    dinfoP->code_counter = dinfoP->ClearCode + 2;
    /* init output buffering vars */
    dinfoP->byteBufferP = byteBuffer_create(ofP);
    dinfoP->cur_accum = 0;
    dinfoP->cur_bits = 0;
    /* GIF specifies an initial Clear code */
    outputUncompressed(dinfoP, dinfoP->ClearCode);
}



static void
writeRasterUncompressedPixel(struct gif_dest * const dinfoP, 
                             unsigned int      const colormapIndex) {
/*----------------------------------------------------------------------------
   "Compress" one pixel value and output it as a symbol.

   'colormapIndex' must be less than dinfoP->n_bits wide.
-----------------------------------------------------------------------------*/
    assert(colormapIndex >> dinfoP->n_bits == 0);

    outputUncompressed(dinfoP, colormapIndex);
    /* Issue Clear codes often enough to keep the reader from ratcheting up
     * its symbol size.
     */
    if (dinfoP->code_counter < dinfoP->maxcode) {
        ++dinfoP->code_counter;
    } else {
        outputUncompressed(dinfoP, dinfoP->ClearCode);
        dinfoP->code_counter = dinfoP->ClearCode + 2;	/* reset the counter */
    }
}



static void
writeRasterUncompressedTerm(struct gif_dest * const dinfoP) {

    outputUncompressed(dinfoP, dinfoP->EOFCode);

    if (dinfoP->cur_bits > 0)
        byteBuffer_out(dinfoP->byteBufferP, dinfoP->cur_accum & 0xFF);

    byteBuffer_flush(dinfoP->byteBufferP);

    byteBuffer_destroy(dinfoP->byteBufferP);
}



static void
writeRasterUncompressed(FILE *         const ofP, 
                        pixel **       const pixels,
                        pixval         const inputMaxval,
                        gray **        const alpha,
                        gray           const alphaMaxval, 
                        struct cmap *  const cmapP, 
                        int            const initBits) {
/*----------------------------------------------------------------------------
   Write the raster to file 'ofP'.
   
   Same as write_raster_LZW(), except written out one code per
   pixel (plus some clear codes), so no compression.  And no use
   of the LZW patent.
-----------------------------------------------------------------------------*/
    gray const alphaThreshold = (alphaMaxval + 1) / 2;
        /* gray levels below this in the alpha mask indicate transparent
           pixels in the output image.
        */
    bool eof;

    struct gif_dest gifDest;

    writeRasterUncompressedInit(ofP, &gifDest, initBits);

    eof = FALSE;
    while (!eof) {
        int gifpixel;
            /* The value for the pixel in the GIF image.  I.e. the colormap
               index.  Or -1 to indicate "no more pixels."
            */
        gifpixel = gifNextPixel(pixels, 
                                inputMaxval, alpha, alphaThreshold, cmapP);
        if (gifpixel == EOF)
            eof = TRUE;
        else
            writeRasterUncompressedPixel(&gifDest, gifpixel);
    }
    writeRasterUncompressedTerm(&gifDest);
}



/******************************************************************************
 *
 * GIF Specific routines
 *
 *****************************************************************************/

static void
writeGifHeader(FILE * const fp,
               int const Width, int const Height, 
               int const GInterlace, int const Background, 
               int const BitsPerPixel, struct cmap * const cmapP,
               const char comment[]) {

    int B;
    int const Resolution = BitsPerPixel;
    int const ColorMapSize = 1 << BitsPerPixel;

    /* Write the Magic header */
    if (cmapP->transparent != -1 || comment)
        fwrite("GIF89a", 1, 6, fp);
    else
        fwrite("GIF87a", 1, 6, fp);

    /* Write out the screen width and height */
    Putword( Width, fp );
    Putword( Height, fp );

    /* Indicate that there is a global color map */
    B = 0x80;       /* Yes, there is a color map */

    /* OR in the resolution */
    B |= (Resolution - 1) << 4;

    /* OR in the Bits per Pixel */
    B |= (BitsPerPixel - 1);

    /* Write it out */
    fputc( B, fp );

    /* Write out the Background color */
    fputc( Background, fp );

    /* Byte of 0's (future expansion) */
    fputc( 0, fp );

    {
        /* Write out the Global Color Map */
        /* Note that the Global Color Map is always a power of two colors
           in size, but *cmapP could be smaller than that.  So we pad with
           black.
        */
        int i;
        for ( i=0; i < ColorMapSize; ++i ) {
            if ( i < cmapP->cmapsize ) {
                fputc( cmapP->red[i], fp );
                fputc( cmapP->green[i], fp );
                fputc( cmapP->blue[i], fp );
            } else {
                fputc( 0, fp );
                fputc( 0, fp );
                fputc( 0, fp );
            }
        }
    }
        
    if ( cmapP->transparent >= 0 ) 
        write_transparent_color_index_extension(fp, cmapP->transparent);

    if ( comment )
        write_comment_extension(fp, comment);
}



static void
writeImageHeader(FILE *       const ofP,
                 unsigned int const leftOffset,
                 unsigned int const topOffset,
                 unsigned int const gWidth,
                 unsigned int const gHeight,
                 unsigned int const gInterlace,
                 unsigned int const initCodeSize) {

    Putword(leftOffset, ofP);
    Putword(topOffset,  ofP);
    Putword(gWidth,     ofP);
    Putword(gHeight,    ofP);

    /* Write out whether or not the image is interlaced */
    if (gInterlace)
        fputc(0x40, ofP);
    else
        fputc(0x00, ofP);

    /* Write out the initial code size */
    fputc(initCodeSize, ofP);
}



static void
gifEncode(FILE *        const ofP, 
          pixel **      const pixels,
          pixval        const inputMaxval,
          int           const gWidth,
          int           const gHeight, 
          gray **       const alpha,
          gray          const alphaMaxval,
          int           const gInterlace,
          int           const background, 
          int           const bitsPerPixel,
          struct cmap * const cmapP,
          char          const comment[],
          bool          const nolzw) {

    unsigned int const leftOffset = 0;
    unsigned int const topOffset  = 0;

    unsigned int const initCodeSize = bitsPerPixel <= 1 ? 2 : bitsPerPixel;
        /* The initial code size */

    if (gWidth > 65535)
        pm_error("Image width %u too large for GIF format.  (Max 65535)",
                 gWidth);  
    
    if (gHeight > 65535)
        pm_error("Image height %u too large for GIF format.  (Max 65535)",
                 gHeight);  

    writeGifHeader(ofP, gWidth, gHeight, gInterlace, background,
                   bitsPerPixel, cmapP, comment);

    /* Write an Image separator */
    fputc(',', ofP);

    writeImageHeader(ofP, leftOffset, topOffset, gWidth, gHeight, gInterlace,
                     initCodeSize);

    initPixelCursor(gWidth, gHeight, gInterlace);

    /* Write the actual raster */
    if (nolzw)
        writeRasterUncompressed(ofP, pixels, 
                                inputMaxval, alpha, alphaMaxval, cmapP, 
                                initCodeSize + 1);
    else
        write_raster_LZW(pixels, 
                         inputMaxval, alpha, alphaMaxval, cmapP, 
                         initCodeSize + 1, ofP);

    /* Write out a zero length data block (to end the series) */
    fputc(0, ofP);

    /* Write the GIF file terminator */
    fputc(';', ofP);
}



static int
compute_transparent(const char colorarg[], 
                    struct cmap * const cmapP) {
/*----------------------------------------------------------------------------
   Figure out the color index (index into the colormap) of the color
   that is to be transparent in the GIF.

   colorarg[] is the string that specifies the color the user wants to
   be transparent (e.g. "red", "#fefefe").  Its maxval is the maxval
   of the colormap.  'cmap' is the full colormap except that its
   'transparent' component isn't valid.

   colorarg[] is a standard Netpbm color specification, except that
   may have a "=" prefix, which means it specifies a particular exact
   color, as opposed to without the "=", which means "the color that
   is closest to this and actually in the image."

   Return -1 if colorarg[] specifies an exact color and that color is not
   in the image.  Also issue an informational message.
-----------------------------------------------------------------------------*/
    int retval;

    const char *colorspec;
    bool exact;
    int presort_colorindex;
    pixel transcolor;

    if (colorarg[0] == '=') {
        colorspec = &colorarg[1];
        exact = TRUE;
    } else {
        colorspec = colorarg;
        exact = FALSE;
    }
        
    transcolor = ppm_parsecolor((char*)colorspec, cmapP->maxval);
    presort_colorindex = ppm_lookupcolor(cmapP->cht, &transcolor);
    
    if (presort_colorindex != -1)
        retval = cmapP->perm[presort_colorindex];
    else if (!exact)
        retval = cmapP->perm[closestcolor(transcolor, cmapP->maxval, cmapP)];
    else {
        retval = -1;
        pm_message(
            "Warning: specified transparent color does not occur in image.");
    }
    return retval;
}



static void
sort_colormap(int const sort, struct cmap * const cmapP) {
/*----------------------------------------------------------------------------
   Sort (in place) the colormap *cmapP.

   Create the perm[] and permi[] mappings for the colormap.

   'sort' is logical:  true means to sort the colormap by red intensity,
   then by green intensity, then by blue intensity.  False means a null
   sort -- leave it in the same order in which we found it.
-----------------------------------------------------------------------------*/
    int * const Red = cmapP->red;
    int * const Blue = cmapP->blue;
    int * const Green = cmapP->green;
    int * const perm = cmapP->perm;
    int * const permi = cmapP->permi;
    unsigned int const cmapsize = cmapP->cmapsize;
    
    int i;

    for (i=0; i < cmapsize; i++)
        permi[i] = i;

    if (sort) {
        pm_message("sorting colormap");
        for (i=0; i < cmapsize; i++) {
            int j;
            for (j=i+1; j < cmapsize; j++)
                if (((Red[i]*MAXCMAPSIZE)+Green[i])*MAXCMAPSIZE+Blue[i] >
                    ((Red[j]*MAXCMAPSIZE)+Green[j])*MAXCMAPSIZE+Blue[j]) {
                    int tmp;
                    
                    tmp=permi[i]; permi[i]=permi[j]; permi[j]=tmp;
                    tmp=Red[i]; Red[i]=Red[j]; Red[j]=tmp;
                    tmp=Green[i]; Green[i]=Green[j]; Green[j]=tmp;
                    tmp=Blue[i]; Blue[i]=Blue[j]; Blue[j]=tmp; } }
    }

    for (i=0; i < cmapsize; i++)
        perm[permi[i]] = i;
}



static void
normalize_to_255(colorhist_vector const chv, struct cmap * const cmapP) {
/*----------------------------------------------------------------------------
   With a PPM color histogram vector 'chv' as input, produce a colormap
   of integers 0-255 as output in *cmapP.
-----------------------------------------------------------------------------*/
    int i;
    pixval const maxval = cmapP->maxval;

    if ( maxval != 255 )
        pm_message(
            "maxval is not 255 - automatically rescaling colors" );

    for ( i = 0; i < cmapP->cmapsize; ++i ) {
        if ( maxval == 255 ) {
            cmapP->red[i] = (int) PPM_GETR( chv[i].color );
            cmapP->green[i] = (int) PPM_GETG( chv[i].color );
            cmapP->blue[i] = (int) PPM_GETB( chv[i].color );
        } else {
            cmapP->red[i] = (int) PPM_GETR( chv[i].color ) * 255 / maxval;
            cmapP->green[i] = (int) PPM_GETG( chv[i].color ) * 255 / maxval;
            cmapP->blue[i] = (int) PPM_GETB( chv[i].color ) * 255 / maxval;
        }
    }
}



static void add_to_colormap(struct cmap * const cmapP, 
                            const char *  const colorspec, 
                            int *         const new_indexP) {
/*----------------------------------------------------------------------------
  Add a new entry to the colormap.  Make the color that specified by
  'colorspec', and return the index of the new entry as *new_indexP.

  'colorspec' is a color specification given by the user, e.g.
  "red" or "rgb:ff/03.0d".  The maxval for this color specification is
  that for the colormap *cmapP.
-----------------------------------------------------------------------------*/
    pixel const transcolor = ppm_parsecolor((char*)colorspec, cmapP->maxval);
    
    *new_indexP = cmapP->cmapsize++; 

    cmapP->red[*new_indexP] = PPM_GETR(transcolor);
    cmapP->green[*new_indexP] = PPM_GETG(transcolor); 
    cmapP->blue[*new_indexP] = PPM_GETB(transcolor); 
}



static void
colormap_from_file(const char filespec[], unsigned int const maxcolors,
                   colorhist_vector * const chvP, pixval * const maxvalP,
                   int * const colorsP) {
/*----------------------------------------------------------------------------
   Read a colormap from the PPM file filespec[].  Return the color histogram
   vector (which is practically a colormap) of the input image as *cvhP
   and the maxval for that histogram as *maxvalP.
-----------------------------------------------------------------------------*/
    FILE *mapfile;
    int cols, rows;
    pixel ** colormap_ppm;

    mapfile = pm_openr(filespec);
    colormap_ppm = ppm_readppm(mapfile, &cols, &rows, maxvalP);
    pm_close(mapfile);
    
    /* Figure out the colormap from the <mapfile>. */
    pm_message("computing other colormap...");
    *chvP = 
        ppm_computecolorhist(colormap_ppm, cols, rows, maxcolors, colorsP);
    
    ppm_freearray(colormap_ppm, rows); 
}



static void
get_alpha(const char * const alpha_filespec, int const cols, int const rows,
          gray *** const alphaP, gray * const maxvalP) {

    if (alpha_filespec) {
        int alpha_cols, alpha_rows;
        *alphaP = pgm_readpgm(pm_openr(alpha_filespec),
                              &alpha_cols, &alpha_rows, maxvalP);
        if (alpha_cols != cols || alpha_rows != rows)
            pm_error("alpha mask is not the same dimensions as the "
                     "input file (alpha is %dW x %dH; image is %dW x %dH)",
                     alpha_cols, alpha_rows, cols, rows);
    } else 
        *alphaP = NULL;
}



static void
compute_ppm_colormap(pixel ** const pixels, int const cols, int const rows,
                     int const input_maxval, bool const have_alpha, 
                     const char * const mapfile, colorhist_vector * const chvP,
                     colorhash_table * const chtP,
                     pixval * const colormap_maxvalP, 
                     int * const colorsP) {
/*----------------------------------------------------------------------------
   Compute a colormap, PPM style, for the image 'pixels', which is
   'cols' by 'rows' with maxval 'input_maxval'.  If 'mapfile' is
   non-null, Use the colors in that (PPM) file for the color map
   instead of the colors in 'pixels'.

   Return the colormap as *chvP and *chtP.  Return the maxval for that
   colormap as *colormap_maxvalP.

   While we're at it, count the colors and validate that there aren't
   too many.  Return the count as *colorsP.  In determining if there are
   too many, allow one slot for a fake transparency color if 'have_alpha'
   is true.  If there are too many, issue an error message and abort the
   program.
-----------------------------------------------------------------------------*/
    unsigned int maxcolors;
        /* The most colors we can tolerate in the image.  If we have
           our own made-up entry in the colormap for transparency, it
           isn't included in this count.
        */

    if (have_alpha)
        maxcolors = MAXCMAPSIZE - 1;
    else
        maxcolors = MAXCMAPSIZE;

    if (mapfile) {
        /* Read the colormap from a separate colormap file. */
        colormap_from_file(mapfile, maxcolors, chvP, colormap_maxvalP, 
                           colorsP);
    } else {
        /* Figure out the color map from the input file */
        pm_message("computing colormap...");
        *chvP = ppm_computecolorhist(pixels, cols, rows, maxcolors, colorsP); 
        *colormap_maxvalP = input_maxval;
    }

    if (*chvP == NULL)
        pm_error("too many colors - try doing a 'pnmquant %d'", maxcolors);
    pm_message("%d colors found", *colorsP);

    /* And make a hash table for fast lookup. */
    *chtP = ppm_colorhisttocolorhash(*chvP, *colorsP);
}



int
main(int argc, char *argv[]) {
    struct cmdlineInfo cmdline;
    FILE * ifP;
    int rows, cols;
    int BitsPerPixel;
    pixel ** pixels;   /* The input image, in PPM format */
    pixval input_maxval;  /* Maxval for 'pixels' */
    gray ** alpha;     /* The supplied alpha mask; NULL if none */
    gray alpha_maxval; /* Maxval for 'alpha' */

    struct cmap cmap;
        /* The colormap, with all its accessories */
    colorhist_vector chv;
    int fake_transparent;
        /* colormap index of the fake transparency color we're using to
           implement the alpha mask.  Undefined if we're not doing an alpha
           mask.
        */

    ppm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    verbose = cmdline.verbose;

    ifP = pm_openr(cmdline.input_filespec);

    pixels = ppm_readppm(ifP, &cols, &rows, &input_maxval);

    pm_close(ifP);

    get_alpha(cmdline.alpha_filespec, cols, rows, &alpha, &alpha_maxval);

    compute_ppm_colormap(pixels, cols, rows, input_maxval, (alpha != NULL), 
                         cmdline.mapfile, 
                         &chv, &cmap.cht, &cmap.maxval, &cmap.cmapsize);

    /* Now turn the ppm colormap into the appropriate GIF colormap. */

    normalize_to_255(chv, &cmap);

    ppm_freecolorhist(chv);

    if (alpha) {
        /* Add a fake entry to the end of the colormap for transparency.  
           Make its color black. 
        */
        add_to_colormap(&cmap, cmdline.alphacolor, &fake_transparent);
    }
    sort_colormap(cmdline.sort, &cmap);

    BitsPerPixel = pm_maxvaltobits(cmap.cmapsize-1);

    if (alpha) {
        cmap.transparent = cmap.perm[fake_transparent];
    } else {
        if (cmdline.transparent)
            cmap.transparent = 
                compute_transparent(cmdline.transparent, &cmap);
        else 
            cmap.transparent = -1;
    }
    /* All set, let's do it. */
    gifEncode(stdout, pixels, input_maxval, cols, rows, 
              alpha, alpha_maxval, 
              cmdline.interlace, 0, BitsPerPixel, &cmap, cmdline.comment,
              cmdline.nolzw);

    ppm_freearray(pixels, rows);
    if (alpha)
        pgm_freearray(alpha, rows);

    fclose(stdout);

    return 0;
}
