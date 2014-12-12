/*
** pnmtopng.c -
** read a portable anymap and produce a Portable Network Graphics file
**
** derived from pnmtorast.c (c) 1990,1991 by Jef Poskanzer and some
** parts derived from ppmtogif.c by Marcel Wijkstra <wijkstra@fwi.uva.nl>
**
** Copyright (C) 1995-1998 by Alexander Lehmann <alex@hal.rhein-main.de>
**                        and Willem van Schaik <willem@schaik.com>
** Copyright (C) 1999,2001 by Greg Roelofs <newt@pobox.com>
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/* This Netpbm version of Pnmtopng was derived from the independently
   distributed program of the same name, Version 2.37.6 (21 July 2001).
*/

/* A performance note: This program reads one row at a time because
   the whole image won't fit in memory always.  When you realize that
   in a Netpbm xel array a one bit pixel can take 96 bits of memory,
   it's easy to see that an ordinary fax could deplete your virtual
   memory and even if it didn't, it might deplete your real memory and
   iterating through the array would cause thrashing.  This program
   iterates through the image multiple times.  

   So instead, we read the image into memory one row at a time, into a
   single row buffer.  We use Netpbm's pm_openr_seekable() facility to
   access the file.  That facility copies the file into a temporary
   file if it isn't seekable, so we always end up with a file that we
   can rewind and reread multiple times.

   This shouldn't cause I/O delays because the entire image ought to fit
   in the system's I/O cache (remember that the file is a lot smaller than
   the xel array you'd get by doing a pnm_readpnm() of it).

   However, it does introduce some delay because of all the system calls 
   required to read the file.  A future enhancement might read the entire
   file into an xel array in some cases, and read one row at a time in 
   others, depending on the needs of the particular use.

   We do still read the entire alpha mask (if there is one) into a
   'gray' array, rather than access it one row at a time.  

   Before May 2001, we did in fact read the whole image into an xel array,
   and we got complaints.  Before April 2000, it wasn't as big a problem
   because xels were only 24 bits.  Now they're 96.
*/
   
#define GRR_GRAY_PALETTE_FIX

#ifndef PNMTOPNG_WARNING_LEVEL
#  define PNMTOPNG_WARNING_LEVEL 0   /* use 0 for backward compatibility, */
#endif                               /*  2 for warnings (1 == error) */

#include <assert.h>
#include <string.h> /* strcat() */
#include <limits.h>
#include <png.h>    /* includes zlib.h and setjmp.h */
#include "pnm.h"
#include "pngtxt.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"
#include "version.h"

#if PNG_LIBPNG_VER >= 10400
#error Your PNG library (<png.h>) is incompatible with this Netpbm source code.
#error You need either an older PNG library (older than 1.4)
#error newer Netpbm source code (at least 10.47.04)
#endif


struct zlibCompression {
    /* These are parameters that describe a form of zlib compression.
       Values have the same meaning as the similarly named arguments to
       zlib's deflateInit2().  See zlib.h.
    */
    unsigned int levelSpec;
    unsigned int level;
    unsigned int memLevelSpec;
    unsigned int mem_level;
    unsigned int strategySpec;
    unsigned int strategy;
    unsigned int windowBitsSpec;
    unsigned int window_bits;
    unsigned int methodSpec;
    unsigned int method;
    unsigned int bufferSizeSpec;
    unsigned int buffer_size;
};

struct chroma {
    float wx;
    float wy;
    float rx;
    float ry;
    float gx;
    float gy;
    float bx;
    float by;
};

struct phys {
    int x;
    int y;
    int unit;
};

typedef struct cahitem {
    xel color;
    gray alpha;
    int value;
    struct cahitem * next;
} cahitem;

typedef cahitem ** coloralphahash_table;

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *  inputFilename;  /* '-' if stdin */
    const char *  alpha;
    unsigned int  verbose;
    unsigned int  downscale;
    unsigned int  interlace;
    const char *  transparent;  /* NULL if none */
    const char *  background;   /* NULL if none */
    unsigned int  gammaSpec;
    float         gamma;        /* Meaningless if !gammaSpec */
    unsigned int  hist;
    unsigned int  rgbSpec;
    struct chroma rgb;          /* Meaningless if !rgbSpec */
    unsigned int  sizeSpec;
    struct phys   size;         /* Meaningless if !sizeSpec */
    const char *  text;         /* NULL if none */
    const char *  ztxt;         /* NULL if none */
    unsigned int  modtimeSpec;
    time_t        modtime;      /* Meaningless if !modtimeSpec */
    const char *  palette;      /* NULL if none */
    int           filterSet;
    unsigned int  force;
    unsigned int  libversion;
    unsigned int  compressionSpec;
    struct zlibCompression zlibCompression;
};



typedef struct _jmpbuf_wrapper {
  jmp_buf jmpbuf;
} jmpbuf_wrapper;

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif
#ifndef NONE
#  define NONE 0
#endif
#define MAXCOLORS 256
#define MAXPALETTEENTRIES 256

/* PALETTEMAXVAL is the maxval used in a PNG palette */
#define PALETTEMAXVAL 255

#define PALETTEOPAQUE 255
#define PALETTETRANSPARENT 0

static bool verbose;

static jmpbuf_wrapper pnmtopng_jmpbuf_struct;
static int errorlevel;



static void
parseSizeOpt(const char *  const sizeOpt,
             struct phys * const sizeP) {

    int count;
    
    count = sscanf(sizeOpt, "%d %d %d", &sizeP->x, &sizeP->y, &sizeP->unit);

    if (count != 3)
        pm_error("Invalid syntax for the -size option value '%s'.  "
                 "Should be 3 integers: x, y, and unit code", sizeOpt);
}



static void
parseRgbOpt(const char *    const rgbOpt,
            struct chroma * const rgbP) {

    int count;
    
    count = sscanf(rgbOpt, "%f %f %f %f %f %f %f %f",
                   &rgbP->wx, &rgbP->wy,
                   &rgbP->rx, &rgbP->ry,
                   &rgbP->gx, &rgbP->gy,
                   &rgbP->bx, &rgbP->by);

    if (count != 6)
        pm_error("Invalid syntax for the -rgb option value '%s'.  "
                 "Should be 6 floating point number: "
                 "x and y for each of white, red, green, and blue",
                 rgbOpt);
}



static void
parseModtimeOpt(const char * const modtimeOpt,
                time_t *     const modtimeP) {

    struct tm brokenTime;
    int year;
    int month;
    int count;

    count = sscanf(modtimeOpt, "%d-%d-%d %d:%d:%d",
                   &year,
                   &month,
                   &brokenTime.tm_mday,
                   &brokenTime.tm_hour,
                   &brokenTime.tm_min,
                   &brokenTime.tm_sec);

    if (count != 6)
        pm_error("Invalid value for -modtime '%s'.   It should have "
                 "the form [yy]yy-mm-dd hh:mm:ss.", modtimeOpt);
    
    if (year < 0)
        pm_error("Year is negative in -modtime value '%s'", modtimeOpt);
    if (year > 9999)
        pm_error("Year is more than 4 digits in -modtime value '%s'",
                 modtimeOpt);
    if (month < 0)
        pm_error("Month is negative in -modtime value '%s'", modtimeOpt);
    if (month > 12)
        pm_error("Month is >12 in -modtime value '%s'", modtimeOpt);
    if (brokenTime.tm_mday < 0)
        pm_error("Day of month is negative in -modtime value '%s'",
                 modtimeOpt);
    if (brokenTime.tm_mday > 31)
        pm_error("Day of month is >31 in -modtime value '%s'", modtimeOpt);
    if (brokenTime.tm_hour < 0)
        pm_error("Hour is negative in -modtime value '%s'", modtimeOpt);
    if (brokenTime.tm_hour > 23)
        pm_error("Hour is >23 in -modtime value '%s'", modtimeOpt);
    if (brokenTime.tm_min < 0)
        pm_error("Minute is negative in -modtime value '%s'", modtimeOpt);
    if (brokenTime.tm_min > 59)
        pm_error("Minute is >59 in -modtime value '%s'", modtimeOpt);
    if (brokenTime.tm_sec < 0)
        pm_error("Second is negative in -modtime value '%s'", modtimeOpt);
    if (brokenTime.tm_sec > 59)
        pm_error("Second is >59 in -modtime value '%s'", modtimeOpt);

    brokenTime.tm_mon = month - 1;
    if (year >= 1900)
        brokenTime.tm_year = year - 1900;
    else
        brokenTime.tm_year = year;

    /* Note that mktime() considers brokeTime to be in local time.
       This is what we want, since we got it from a user.  User should
       set his local time zone to UTC if he wants absolute time.
    */
    *modtimeP = mktime(&brokenTime);
}



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

    unsigned int alphaSpec, transparentSpec, backgroundSpec;
    unsigned int textSpec, ztxtSpec, paletteSpec;
    unsigned int filterSpec;

    unsigned int nofilter, sub, up, avg, paeth, filter;
    unsigned int chroma, phys, time;
    const char * size;
    const char * rgb;
    const char * modtime;
    const char * compMethod;
    const char * compStrategy;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "alpha",            OPT_STRING,    &cmdlineP->alpha,
            &alphaSpec,            0);
    OPTENT3(0, "transparent",      OPT_STRING,    &cmdlineP->transparent,
            &transparentSpec,      0);
    OPTENT3(0, "background",       OPT_STRING,    &cmdlineP->background,
            &backgroundSpec,       0);
    OPTENT3(0, "rgb",              OPT_STRING,    &rgb,
            &cmdlineP->rgbSpec,    0);
    OPTENT3(0, "size",             OPT_STRING,    &size,
            &cmdlineP->sizeSpec,   0);
    OPTENT3(0, "text",             OPT_STRING,    &cmdlineP->text,
            &textSpec,             0);
    OPTENT3(0, "ztxt",             OPT_STRING,    &cmdlineP->ztxt,
            &ztxtSpec,             0);
    OPTENT3(0, "modtime",          OPT_STRING,    &modtime,
            &cmdlineP->modtimeSpec,0);
    OPTENT3(0, "palette",          OPT_STRING,    &cmdlineP->palette,
            &paletteSpec,          0);
    OPTENT3(0, "compression",      OPT_UINT,
            &cmdlineP->zlibCompression.level,
            &cmdlineP->zlibCompression.levelSpec,            0);
    OPTENT3(0, "comp_mem_level",   OPT_UINT,
            &cmdlineP->zlibCompression.mem_level,
            &cmdlineP->zlibCompression.memLevelSpec,         0);
    OPTENT3(0, "comp_strategy",    OPT_STRING,    &compStrategy,
            &cmdlineP->zlibCompression.strategySpec,         0);
    OPTENT3(0, "comp_window_bits", OPT_UINT,
            &cmdlineP->zlibCompression.window_bits,
            &cmdlineP->zlibCompression.windowBitsSpec,       0);
    OPTENT3(0, "comp_method",      OPT_STRING,    &compMethod,
            &cmdlineP->zlibCompression.methodSpec,           0);
    OPTENT3(0, "comp_buffer_size", OPT_UINT,
            &cmdlineP->zlibCompression.buffer_size,
            &cmdlineP->zlibCompression.bufferSizeSpec,       0);
    OPTENT3(0, "gamma",            OPT_FLOAT,     &cmdlineP->gamma,
            &cmdlineP->gammaSpec,  0);
    OPTENT3(0, "hist",             OPT_FLAG,      NULL,
            &cmdlineP->hist,       0);
    OPTENT3(0, "downscale",        OPT_FLAG,      NULL,
            &cmdlineP->downscale,  0);
    OPTENT3(0, "interlace",        OPT_FLAG,      NULL,
            &cmdlineP->interlace,  0);
    OPTENT3(0, "force",            OPT_FLAG,      NULL,
            &cmdlineP->force,      0);
    OPTENT3(0, "libversion",       OPT_FLAG,      NULL,
            &cmdlineP->libversion, 0);
    OPTENT3(0, "verbose",          OPT_FLAG,      NULL,
            &cmdlineP->verbose,    0);
    OPTENT3(0, "nofilter",         OPT_FLAG,      NULL,
            &nofilter,             0);
    OPTENT3(0, "sub",              OPT_FLAG,      NULL,
            &sub,                  0);
    OPTENT3(0, "up",               OPT_FLAG,      NULL,
            &up,                   0);
    OPTENT3(0, "avg",              OPT_FLAG,      NULL,
            &avg,                  0);
    OPTENT3(0, "paeth",            OPT_FLAG,      NULL,
            &paeth,                0);
    OPTENT3(0, "filter",           OPT_INT,       &filter,
            &filterSpec,           0);
    OPTENT3(0, "verbose",          OPT_FLAG,      NULL,
            &cmdlineP->verbose,    0);
    OPTENT3(0, "chroma",           OPT_FLAG,      NULL,
            &chroma,               0);
    OPTENT3(0, "phys",             OPT_FLAG,      NULL,
            &phys,                 0);
    OPTENT3(0, "time",             OPT_FLAG,      NULL,
            &time,                 0);


    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (chroma)
        pm_error("The -chroma option no longer exists.  Use -rgb instead.");
    if (phys)
        pm_error("The -phys option no longer exists.  Use -size instead.");
    if (time)
        pm_error("The -time option no longer exists.  Use -modtime instead.");

    if (alphaSpec + transparentSpec > 1)
        pm_error("You may not specify both -alpha and -transparent");
    if (!alphaSpec)
        cmdlineP->alpha = NULL;
    if (!transparentSpec)
        cmdlineP->transparent = NULL;
    if (!backgroundSpec)
        cmdlineP->background = NULL;
    if (!textSpec)
        cmdlineP->text = NULL;
    if (!ztxtSpec)
        cmdlineP->ztxt = NULL;
    if (!paletteSpec)
        cmdlineP->palette = NULL;
    
    if (filterSpec + nofilter + sub + up + avg + paeth > 1)
        pm_error("You may specify at most one of "
                 "-nofilter, -sub, -up, -avg, -paeth, and -filter");
    
    if (filterSpec) {
        if (filter < 0 || filter > 4)
            pm_error("-filter is obsolete.  Use -nofilter, -sub, -up, -avg, "
                     "and -paeth options instead.");
        else
            switch (filter) {
            case 0: cmdlineP->filterSet = PNG_FILTER_NONE;  break;
            case 1: cmdlineP->filterSet = PNG_FILTER_SUB;   break;
            case 2: cmdlineP->filterSet = PNG_FILTER_UP;    break;
            case 3: cmdlineP->filterSet = PNG_FILTER_AVG;   break;
            case 4: cmdlineP->filterSet = PNG_FILTER_PAETH; break;
            }
    } else {
        if (nofilter)
            cmdlineP->filterSet = PNG_FILTER_NONE;
        else if (sub)
            cmdlineP->filterSet = PNG_FILTER_SUB;
        else if (up)
            cmdlineP->filterSet = PNG_FILTER_UP;
        else if (avg)
            cmdlineP->filterSet = PNG_FILTER_AVG;
        else if (paeth)
            cmdlineP->filterSet = PNG_FILTER_PAETH;
        else
            cmdlineP->filterSet = PNG_FILTER_NONE;
    }
    
    if (cmdlineP->sizeSpec)
        parseSizeOpt(size, &cmdlineP->size);

    if (cmdlineP->rgbSpec)
        parseRgbOpt(rgb, &cmdlineP->rgb);
    
    if (cmdlineP->modtimeSpec)
        parseModtimeOpt(modtime, &cmdlineP->modtime);

    if (cmdlineP->zlibCompression.levelSpec &&
        cmdlineP->zlibCompression.level > 9)
        pm_error("-compression value must be from 0 (no compression) "
                 "to 9 (maximum compression).  You specified %u",
                 cmdlineP->zlibCompression.level);

    if (cmdlineP->zlibCompression.memLevelSpec) {
        if (cmdlineP->zlibCompression.mem_level  < 1 ||
            cmdlineP->zlibCompression.mem_level > 9)
        pm_error("-comp_mem_level value must be from 1 (minimum memory usage) "
                 "to 9 (maximum memory usage).  You specified %u",
                 cmdlineP->zlibCompression.mem_level);
    }

    if (cmdlineP->zlibCompression.methodSpec) {
        if (STREQ(compMethod, "deflated"))
            cmdlineP->zlibCompression.method = Z_DEFLATED;
        else
            pm_error("The only valid value for -method is 'deflated'.  "
                     "You specified '%s'", compMethod);
    }

    if (cmdlineP->zlibCompression.strategySpec) {
        if (STREQ(compStrategy, "huffman_only"))
            cmdlineP->zlibCompression.strategy = Z_HUFFMAN_ONLY;
        else if (STREQ(compStrategy, "filtered"))
            cmdlineP->zlibCompression.strategy = Z_FILTERED;
        else
            pm_error("Valid values for -strategy are 'huffman_only' and "
                     "filtered.  You specified '%s'", compStrategy);
    }


    if (argc-1 < 1)
        cmdlineP->inputFilename = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFilename = argv[1];
    else
        pm_error("Program takes at most one argument:  input file name");
}



static png_color_16
xelToPngColor_16(xel const input, 
                 xelval const maxval, 
                 xelval const pngMaxval) {
    png_color_16 retval;

    xel scaled;
    
    PPM_DEPTH(scaled, input, maxval, pngMaxval);

    retval.red   = PPM_GETR(scaled);
    retval.green = PPM_GETG(scaled);
    retval.blue  = PPM_GETB(scaled);
    retval.gray  = PNM_GET1(scaled);

    return retval;
}



static void
closestColorInPalette(pixel          const targetColor, 
                      pixel                palette_pnm[],
                      unsigned int   const paletteSize,
                      unsigned int * const bestIndexP,
                      unsigned int * const bestMatchP) {
    
    unsigned int paletteIndex;
    unsigned int bestIndex;
    unsigned int bestMatch;

    assert(paletteSize > 0);

    bestMatch = UINT_MAX;
    for (paletteIndex = 0; paletteIndex < paletteSize; ++paletteIndex) {
        unsigned int const dist = 
            PPM_DISTANCE(palette_pnm[paletteIndex], targetColor);

        if (dist < bestMatch) {
            bestMatch = dist;
            bestIndex = paletteIndex;
        }
    }
    if (bestIndexP != NULL)
        *bestIndexP = bestIndex;
    if (bestMatchP != NULL)
        *bestMatchP = bestMatch;
}



/* We really ought to make this hash function actually depend upon
   the "a" argument; we just don't know a decent prime number off-hand.
*/
#define HASH_SIZE 20023
#define hashpixelalpha(p,a) ((((long) PPM_GETR(p) * 33023 + \
                               (long) PPM_GETG(p) * 30013 + \
                               (long) PPM_GETB(p) * 27011 ) \
                              & 0x7fffffff ) % HASH_SIZE )

static coloralphahash_table
alloccoloralphahash(void)  {
    coloralphahash_table caht;
    int i;

    MALLOCARRAY(caht,HASH_SIZE);
    if (caht == NULL)
        pm_error( "out of memory allocating hash table" );

    for (i = 0; i < HASH_SIZE; ++i)
        caht[i] = NULL;

    return caht;
}


static void
freecoloralphahash(coloralphahash_table const caht) {
    int i;

    for (i = 0; i < HASH_SIZE; ++i) {
        cahitem * p;
        cahitem * next;
        for (p = caht[i]; p; p = next) {
            next = p->next;
            free(p);
        }
    }
    free(caht);
}



static void
addtocoloralphahash(coloralphahash_table const caht,
                    pixel *              const colorP,
                    gray *               const alphaP,
                    int                  const value) {

    int hash;
    cahitem * itemP;

    MALLOCVAR(itemP);
    if (itemP == NULL)
        pm_error("Out of memory building hash table");
    hash = hashpixelalpha(*colorP, *alphaP);
    itemP->color = *colorP;
    itemP->alpha = *alphaP;
    itemP->value = value;
    itemP->next = caht[hash];
    caht[hash] = itemP;
}



static int
lookupColorAlpha(coloralphahash_table const caht,
                 const pixel *        const colorP,
                 const gray *         const alphaP) {

    int hash;
    cahitem * p;

    hash = hashpixelalpha(*colorP, *alphaP);
    for (p = caht[hash]; p; p = p->next)
        if (PPM_EQUAL(p->color, *colorP) && p->alpha == *alphaP)
            return p->value;

    return -1;
}



static void
pnmtopng_error_handler(png_structp     const png_ptr,
                       png_const_charp const msg) {

  jmpbuf_wrapper  *jmpbuf_ptr;

  /* this function, aside from the extra step of retrieving the "error
   * pointer" (below) and the fact that it exists within the application
   * rather than within libpng, is essentially identical to libpng's
   * default error handler.  The second point is critical:  since both
   * setjmp() and longjmp() are called from the same code, they are
   * guaranteed to have compatible notions of how big a jmp_buf is,
   * regardless of whether _BSD_SOURCE or anything else has (or has not)
   * been defined. */

  fprintf(stderr, "pnmtopng:  fatal libpng error: %s\n", msg);
  fflush(stderr);

  jmpbuf_ptr = png_get_error_ptr(png_ptr);
  if (jmpbuf_ptr == NULL) {         /* we are completely hosed now */
    fprintf(stderr,
      "pnmtopng:  EXTREMELY fatal error: jmpbuf unrecoverable; terminating.\n");
    fflush(stderr);
    exit(99);
  }

  longjmp(jmpbuf_ptr->jmpbuf, 1);
}


/* The following variables belong to getChv() and freeChv() */
static bool getChv_computed = FALSE;
static colorhist_vector getChv_chv;



static void
getChv(FILE *             const ifP, 
       pm_filepos         const rasterPos,
       int                const cols, 
       int                const rows, 
       xelval             const maxval,
       int                const format, 
       int                const maxColors, 
       colorhist_vector * const chvP,
       unsigned int *     const colorsP) {
/*----------------------------------------------------------------------------
   Return a list of all the colors in a libnetpbm image and the number of
   times they occur.  The image is in the seekable file 'ifP', whose
   raster starts at position 'rasterPos' of the file.  The image's properties
   are 'cols', 'rows', 'maxval', and 'format'.

   Return the number of colors as *colorsP.  Return the details of the 
   colors in newly malloc'ed storage, and its address as *chvP.  If
   there are more than 'maxColors' colors, though, just return NULL as
   *chvP and leave *colorsP undefined.

   Don't spend the time to read the file if this subroutine has been called
   before.  In that case, just assume the inputs are all the same and return
   the previously computed information.  Ick.

   *chvP is in static program storage.
-----------------------------------------------------------------------------*/
    static unsigned int getChv_colors;

    if (!getChv_computed) {
        int colorCount;
        if (verbose) 
            pm_message ("Finding colors in input image...");

        pm_seek2(ifP, &rasterPos, sizeof(rasterPos));
        getChv_chv = ppm_computecolorhist2(ifP, cols, rows, maxval, format, 
                                           maxColors, &colorCount);
        
        getChv_colors = colorCount;

        if (verbose) {
            if (getChv_chv)
                pm_message("%u colors found", getChv_colors);
            else
                pm_message("Too many colors (more than %u) found", maxColors);
        }
        getChv_computed = TRUE;
    }
    *chvP = getChv_chv;
    *colorsP = getChv_colors;
}



static void freeChv(void) {

    if (getChv_computed)
        if (getChv_chv)
            ppm_freecolorhist(getChv_chv);

    getChv_computed = FALSE;
}



static bool
pgmBitsAreRepeated(unsigned int const repeatedSize,
                   FILE *       const ifP,
                   pm_filepos   const rasterPos, 
                   int          const cols,
                   int          const rows,
                   xelval       const maxval,
                   int          const format) {
/*----------------------------------------------------------------------------
   Return TRUE iff all the samples in the image in file 'ifP',
   described by 'cols', 'rows', 'maxval', and 'format', consist in the
   rightmost 'repeatedSize' * 2 bits of two identical sets of
   'repeatedSize' bits.

   The file has arbitrary position, but the raster is at file position
   'rasterPos'.

   E.g. for repeatedSize = 2, a sample value of 0xaa would qualify.
   So would 0x0a.

   Leave the file positioned where we found it.
-----------------------------------------------------------------------------*/
    unsigned int const mask2 = (1 << repeatedSize*2) - 1;
    unsigned int const mask1 = (1 << repeatedSize) - 1;

    bool mayscale;
    unsigned int row;
    xel * xelrow;

    xelrow = pnm_allocrow(cols);
    
    pm_seek2(ifP, &rasterPos, sizeof(rasterPos));

    mayscale = TRUE;  /* initial assumption */

    for (row = 0; row < rows && mayscale; ++row) {
        unsigned int col;
        pnm_readpnmrow(ifP, xelrow, cols, maxval, format);
        for (col = 0; col < cols && mayscale; ++col) {
            xelval const testbits2 = PNM_GET1(xelrow[col]) & mask2;
                /* The bits of interest in the sample */
            xelval const testbits1 = testbits2 & mask1;
                /* The lower half of the bits of interest in the sample */
            if (((testbits1 << repeatedSize) | testbits1) != testbits2)
                mayscale = FALSE;
        }
    }
    pnm_freerow(xelrow);

    return mayscale;
}



static void
meaningful_bits_pgm(FILE *         const ifP, 
                    pm_filepos     const rasterPos, 
                    int            const cols,
                    int            const rows,
                    xelval         const maxval,
                    int            const format,
                    unsigned int * const retvalP) {
/*----------------------------------------------------------------------------
   In the PGM raster with maxval 'maxval' at file offset 'rasterPos'
   in file 'ifp', the samples may be composed of groups of 1, 2, 4, or 8
   bits repeated.  This would be the case if the image were converted
   at some point from a 2 bits-per-pixel image to an 8-bits-per-pixel
   image, for example.

   If this is the case, we find out and find out how small these repeated
   groups of bits are and return the number of bits.
-----------------------------------------------------------------------------*/
    unsigned int maxMeaningfulBits;
        /* progressive estimate of the maximum number of meaningful
           (nonrepeated) bits in the samples.
        */

    maxMeaningfulBits = pm_maxvaltobits(maxval);  /* initial value */

    if (maxval == 0xffff || maxval == 0xff || maxval == 0xf || maxval == 0x3) {
        if (maxMeaningfulBits == 16) {
            if (pgmBitsAreRepeated(8,
                                   ifP, rasterPos, cols, rows, maxval, format))
                maxMeaningfulBits = 8;
        }
        if (maxMeaningfulBits == 8) {
            if (pgmBitsAreRepeated(4,
                                   ifP, rasterPos, cols, rows, maxval, format))
                maxMeaningfulBits = 4;
        }
        if (maxMeaningfulBits == 4) {
            if (pgmBitsAreRepeated(2,
                                   ifP, rasterPos, cols, rows, maxval, format))
                maxMeaningfulBits = 2;
        }
        if (maxMeaningfulBits == 2) {
            if (pgmBitsAreRepeated(1,
                                   ifP, rasterPos, cols, rows, maxval, format))
                maxMeaningfulBits = 1;
        }
    }
    *retvalP = maxMeaningfulBits;
}



static void
meaningful_bits_ppm(FILE *         const ifp, 
                    pm_filepos     const rasterPos, 
                    int            const cols,
                    int            const rows,
                    xelval         const maxval,
                    int            const format,
                    unsigned int * const retvalP) {
/*----------------------------------------------------------------------------
   In the PPM raster with maxval 'maxval' at file offset 'rasterPos'
   in file 'ifp', the samples may be composed of groups of 8
   bits repeated twice.  This would be the case if the image were converted
   at some point from a 8 bits-per-pixel image to an 16-bits-per-pixel
   image, for example.

   We return the smallest number of bits we can take from the right of
   a sample without losing information (8 or all).
-----------------------------------------------------------------------------*/
    int mayscale;
    unsigned int row;
    xel * xelrow;
    unsigned int maxMeaningfulBits;
        /* progressive estimate of the maximum number of meaningful
           (nonrepeated) bits in the samples.
        */

    xelrow = pnm_allocrow(cols);

    maxMeaningfulBits = pm_maxvaltobits(maxval);

    if (maxval == 65535) {
        mayscale = TRUE;   /* initial assumption */
        pm_seek2(ifp, &rasterPos, sizeof(rasterPos));
        for (row = 0; row < rows && mayscale; ++row) {
            unsigned int col;
            pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
            for (col = 0; col < cols && mayscale; ++col) {
                xel const p = xelrow[col];
                if ((PPM_GETR(p) & 0xff) * 0x101 != PPM_GETR(p) ||
                    (PPM_GETG(p) & 0xff) * 0x101 != PPM_GETG(p) ||
                    (PPM_GETB(p) & 0xff) * 0x101 != PPM_GETB(p))
                    mayscale = FALSE;
            }
        }
        if (mayscale)
            maxMeaningfulBits = 8;
    }
    pnm_freerow(xelrow);

    *retvalP = maxMeaningfulBits;
}



static void
tryTransparentColor(FILE *     const ifp, 
                    pm_filepos const rasterPos, 
                    int        const cols, 
                    int        const rows, 
                    xelval     const maxval,
                    int        const format, 
                    gray **    const alphaMask,
                    gray       const alphaMaxval,
                    pixel      const transcolor,
                    bool *     const singleColorIsTransP) {

    int const pnm_type = PNM_FORMAT_TYPE(format);

    xel * xelrow;
    bool singleColorIsTrans;
        /* So far, it looks like a single color is uniquely transparent */
    int row;

    xelrow = pnm_allocrow(cols);

    pm_seek2(ifp, &rasterPos, sizeof(rasterPos));

    singleColorIsTrans = TRUE;  /* initial assumption */
        
    for (row = 0; row < rows && singleColorIsTrans; ++row) {
        int col;
        pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
        for (col = 0 ; col < cols && singleColorIsTrans; ++col) {
            if (alphaMask[row][col] == 0) { /* transparent */
                /* If we have a second transparent color, we're
                   disqualified
                */
                if (pnm_type == PPM_TYPE) {
                    if (!PPM_EQUAL(xelrow[col], transcolor))
                        singleColorIsTrans = FALSE;
                } else {
                    if (PNM_GET1(xelrow[col]) != PNM_GET1(transcolor))
                        singleColorIsTrans = FALSE;
                }
            } else if (alphaMask[row][col] != alphaMaxval) {
                /* Here's an area of the mask that is translucent.  That
                   disqualified us.
                */
                singleColorIsTrans = FALSE;
            } else {
                /* Here's an area of the mask that is opaque.  If it's
                   the same color as our candidate transparent color,
                   that disqualifies us.
                */
                if (pnm_type == PPM_TYPE) {
                    if (PPM_EQUAL(xelrow[col], transcolor))
                        singleColorIsTrans = FALSE;
                } else {
                    if (PNM_GET1(xelrow[col]) == PNM_GET1(transcolor))
                        singleColorIsTrans = FALSE;
                }
            }
        }
    }  
    pnm_freerow(xelrow);
}



static void
analyzeAlpha(FILE *     const ifp, 
             pm_filepos const rasterPos, 
             int        const cols, 
             int        const rows, 
             xelval     const maxval,
             int        const format, 
             gray **    const alphaMask,
             gray       const alphaMaxval,
             bool *     const allOpaqueP,
             bool *     const singleColorIsTransP, 
             pixel*     const alphaTranscolorP) {
/*----------------------------------------------------------------------------
  Get information about the alpha mask, in combination with the masked
  image, that Caller can use to choose the most efficient way to
  represent the information in the alpha mask in a PNG.  Simply
  putting the alpha mask in the PNG is a last resort.  But if the mask
  says all opaque, we can simply omit any mention of transparency
  instead -- default is opaque.  And if the mask makes all the pixels
  of a certain color fully transparent and every other pixel opaque,
  we can simply identify that color in the PNG.

  We have to do this before any scaling occurs, since alpha is only
  possible with 8 and 16-bit.
-----------------------------------------------------------------------------*/
    xel * xelrow;
    bool foundTransparentPixel;
        /* We found a pixel in the image where the alpha mask says it is
           transparent.
        */
    pixel transcolor;
        /* Color of the transparent pixel mentioned above. */
    
    xelrow = pnm_allocrow(cols);

    {
        int row;
        /* Find a candidate transparent color -- the color of any pixel in the
           image that the alpha mask says should be transparent.
        */
        foundTransparentPixel = FALSE;  /* initial assumption */
        pm_seek2(ifp, &rasterPos, sizeof(rasterPos));
        for (row = 0 ; row < rows && !foundTransparentPixel ; ++row) {
            int col;
            pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
            for (col = 0; col < cols && !foundTransparentPixel; ++col) {
                if (alphaMask[row][col] == 0) {
                    foundTransparentPixel = TRUE;
                    transcolor = pnm_xeltopixel(xelrow[col], format);
                }
            }
        }
    }

    pnm_freerow(xelrow);

    if (foundTransparentPixel) {
        *allOpaqueP = FALSE;
        tryTransparentColor(ifp, rasterPos, cols, rows, maxval, format,
                            alphaMask, alphaMaxval, transcolor,
                            singleColorIsTransP);
        *alphaTranscolorP = transcolor;
    } else {
        *allOpaqueP   = TRUE;
        *singleColorIsTransP = FALSE;
    }
}



static void
findRedundantBits(FILE *         const ifp, 
                  int            const rasterPos, 
                  int            const cols,
                  int            const rows,
                  xelval         const maxval,
                  int            const format,
                  bool           const alpha,
                  bool           const force,
                  unsigned int * const meaningfulBitsP) {
/*----------------------------------------------------------------------------
   Find out if we can use just a subset of the bits from each input
   sample.  Often, people create an image with e.g. 8 bit samples from
   one that has e.g. only 4 bit samples by scaling by 256/16, which is
   the same as repeating the bits.  E.g.  1011 becomes 10111011.  We
   detect this case.  We return as *meaningfulBitsP the minimum number
   of bits, starting from the least significant end, that contain
   original information.
-----------------------------------------------------------------------------*/
  if (!alpha && PNM_FORMAT_TYPE(format) == PGM_TYPE && !force) 
      meaningful_bits_pgm(ifp, rasterPos, cols, rows, maxval, format,
                          meaningfulBitsP);
  else if (PNM_FORMAT_TYPE(format) == PPM_TYPE && !force)
      meaningful_bits_ppm(ifp, rasterPos, cols, rows, maxval, format,
                          meaningfulBitsP);
  else 
      *meaningfulBitsP = pm_maxvaltobits(maxval);

  if (verbose && *meaningfulBitsP != pm_maxvaltobits(maxval))
      pm_message("Using only %d rightmost bits of input samples.  The "
                 "rest are redundant.", *meaningfulBitsP);
}



static void
readOrderedPalette(FILE *         const pfp,
                   xel                  ordered_palette[], 
                   unsigned int * const ordered_palette_size_p) {

    xel ** xels;
    int cols, rows;
    xelval maxval;
    int format;
    
    if (verbose)
        pm_message("reading ordered palette (colormap)...");

    xels = pnm_readpnm(pfp, &cols, &rows, &maxval, &format);
    
    if (PNM_FORMAT_TYPE(format) != PPM_TYPE) 
        pm_error("ordered palette must be a PPM file, not type %d", format);

    *ordered_palette_size_p = rows * cols;
    if (*ordered_palette_size_p > MAXCOLORS) 
        pm_error("ordered-palette image contains %d pixels.  Maximum is %d",
                 *ordered_palette_size_p, MAXCOLORS);
    if (verbose)
        pm_message("%u colors found", *ordered_palette_size_p);

    {
        unsigned int j;
        unsigned int row;
        j = 0;  /* initial value */
        for (row = 0; row < rows; ++row) {
            int col;
            for (col = 0; col < cols; ++col) 
                ordered_palette[j++] = xels[row][col];
        }
    }
    pnm_freearray(xels, rows);
}        



static void
compute_nonalpha_palette(colorhist_vector const chv,
                         int              const colors,
                         pixval           const maxval,
                         FILE *           const pfp,
                         pixel                  palette_pnm[],
                         unsigned int *   const paletteSizeP,
                         gray                   trans_pnm[],
                         unsigned int *   const transSizeP) {
/*----------------------------------------------------------------------------
   Compute the palette corresponding to the color set 'chv'
   (consisting of 'colors' distinct colors) assuming a pure-color (no
   transparency) palette.

   If 'pfp' is non-null, assume it's a PPM file and read the palette
   from that.  Make sure it contains the same colors as the palette
   we computed ourself would have.  Caller supplied the file because he
   wants the colors in a particular order in the palette.
-----------------------------------------------------------------------------*/
    unsigned int colorIndex;
    
    xel ordered_palette[MAXCOLORS];
    unsigned int ordered_palette_size;

    if (pfp) {
        readOrderedPalette(pfp, ordered_palette, &ordered_palette_size);

        if (colors != ordered_palette_size) 
            pm_error("sizes of ordered palette (%d) "
                     "and existing palette (%d) differ",
                     ordered_palette_size, colors);
        
        /* Make sure the ordered palette contains all the colors in
           the image 
        */
        for (colorIndex = 0; colorIndex < colors; colorIndex++) {
            int j;
            bool found;
            
            found = FALSE;
            for (j = 0; j < ordered_palette_size && !found; ++j) {
                if (PNM_EQUAL(ordered_palette[j], chv[colorIndex].color)) 
                    found = TRUE;
            }
            if (!found) 
                pm_error("failed to find color (%d, %d, %d), which is in the "
                         "input image, in the ordered palette",
                         PPM_GETR(chv[colorIndex].color),
                         PPM_GETG(chv[colorIndex].color),
                         PPM_GETB(chv[colorIndex].color));
        }
        /* OK, the ordered palette passes muster as a palette; go ahead
           and return it as the palette.
        */
        for (colorIndex = 0; colorIndex < colors; ++colorIndex)
            palette_pnm[colorIndex] = ordered_palette[colorIndex];
    } else {
        for (colorIndex = 0; colorIndex < colors; ++colorIndex) 
            palette_pnm[colorIndex] = chv[colorIndex].color;
    }
    *paletteSizeP = colors;
    *transSizeP = 0;
}



static bool
isPowerOfTwoOrZero(unsigned int const arg) {

    unsigned int i;
    unsigned int mask;
    unsigned int nOneBit;

    for (i = 0, mask = 0x1, nOneBit = 0;
         i < sizeof(arg) * 8;
         ++i, mask <<= 1) {

        if (arg & mask)
            ++nOneBit;
        if (nOneBit > 1)
            return false;
    }
    return true;
}



static void
addColorAlphaPair(gray **        const alphasOfColor,
                  unsigned int * const alphasOfColorCnt,
                  unsigned int   const colorIndex,
                  gray           const alpha) {
/*----------------------------------------------------------------------------
   Add the pair (colorIndex, alpha) to the palette
   (alphasOfColor, alphasOfColorCnt).
-----------------------------------------------------------------------------*/
    unsigned int const colorCnt = alphasOfColorCnt[colorIndex];

    if (isPowerOfTwoOrZero(colorCnt)) {
        /* We've filled the current memory allocation.  Expand. */

        REALLOCARRAY(alphasOfColor[colorIndex], MAX(1, colorCnt * 2));

        if (alphasOfColor[colorIndex] == NULL)
            pm_error("Out of memory allocating color/alpha palette space "
                     "for %u alpha values for color index %u",
                     colorCnt * 2, colorIndex);
    }

    alphasOfColor[colorIndex][colorCnt] = alpha;
    ++alphasOfColorCnt[colorIndex];
}



static void
freeAlphasOfColor(gray **      const alphasOfColor,
                  unsigned int const colorCount) {

    unsigned int colorIndex;

    for (colorIndex = 0; colorIndex < colorCount; ++colorIndex)
        free(alphasOfColor[colorIndex]);
}



static void
computeUnsortedAlphaPalette(FILE *           const ifP,
                            int              const cols,
                            int              const rows,
                            xelval           const maxval,
                            int              const format,
                            pm_filepos       const rasterPos,
                            gray **          const alphaMask,
                            colorhist_vector const chv,
                            int              const colors,
                            unsigned int     const maxPaletteEntries,
                            gray *                 alphasOfColor[],
                            unsigned int           alphasFirstIndex[],
                            unsigned int           alphasOfColorCnt[],
                            bool *           const tooBigP) {
/*----------------------------------------------------------------------------
   Read the image at position 'rasterPos' in file *ifP, which is a PNM
   described by 'cols', 'rows', 'maxval', and 'format'.

   Using the alpha mask 'alpha_mask' and color map 'chv' (of size 'colors')
   for the image, construct a palette of (color index, alpha) ordered pairs 
   for the image, as follows.

   The alpha/color palette is the set of all ordered pairs of
   (color,alpha) in the PNG, including the background color.  The
   actual palette is an array.  Each array element contains a color
   index from the color palette and an alpha value.  All the elements
   with the same color index are contiguous.  alphasFirstIndex[x] is
   the index in the alpha/color palette of the first element that has
   color index x.  alphasOfColorCnt[x] is the number of elements that
   have color index x.  alphasOfColor[x][y] is the yth alpha value
   that appears with color index x (in order of appearance).

   The palette we produce does not go out of its way to include the
   background color; unless the background color is also in the image,
   Caller will have to add it.

   To save time, we give up as soon as we know there are more than
   'maxPaletteEntries' in the palette.  We return *tooBigP indicating
   whether that was the case.
-----------------------------------------------------------------------------*/
    colorhash_table cht;
    int colorIndex;
    bool tooBig;
    int row;
    xel * xelrow;
    unsigned int alphaColorPairCnt;

    cht = ppm_colorhisttocolorhash(chv, colors);

    for (colorIndex = 0; colorIndex < colors; ++colorIndex) {
        alphasOfColor[colorIndex] = NULL;
        alphasOfColorCnt[colorIndex] = 0;
    }
 
    pm_seek2(ifP, &rasterPos, sizeof(rasterPos));

    xelrow = pnm_allocrow(cols);

    tooBig = false;  /* initial assumption */

    for (row = 0; row < rows && !tooBig; ++row) {
        unsigned int col;
        pnm_readpnmrow(ifP, xelrow, cols, maxval, format);
        pnm_promoteformatrow(xelrow, cols, maxval, format, maxval, PPM_TYPE);

        for (col = 0; col < cols; ++col) {
            unsigned int i;
            int const colorIndex = ppm_lookupcolor(cht, &xelrow[col]);
            for (i = 0 ; i < alphasOfColorCnt[colorIndex] ; ++i) {
                if (alphaMask[row][col] == alphasOfColor[colorIndex][i])
                    break;
            }
            if (i == alphasOfColorCnt[colorIndex]) {
                if (alphaColorPairCnt >= maxPaletteEntries) {
                    tooBig = true;
                    break;
                } else {
                    addColorAlphaPair(alphasOfColor, alphasOfColorCnt,
                                      colorIndex, alphaMask[row][col]);
                    ++alphaColorPairCnt;
                }
            }
        }
    }
    if (tooBig)
        freeAlphasOfColor(alphasOfColor, colors);
    else {
        unsigned int i;
        alphasFirstIndex[0] = 0;
        for (i = 1; i < colors; ++i)
            alphasFirstIndex[i] = alphasFirstIndex[i-1] +
                alphasOfColorCnt[i-1];
    }
    pnm_freerow(xelrow);
    ppm_freecolorhash(cht);

    *tooBigP = tooBig;
}



static void
sortAlphaPalette(gray *         const alphas_of_color[],
                 unsigned int   const alphas_first_index[],
                 unsigned int   const alphas_of_color_cnt[],
                 unsigned int   const colors,
                 gray           const alphaMaxval,
                 unsigned int         mapping[],
                 unsigned int * const transSizeP) {
/*----------------------------------------------------------------------------
   Remap the palette indices so opaque entries are last.

   This is _not_ a sort in place -- we do not modify our inputs.

   alphas_of_color[], alphas_first_index[], and alphas_of_color_cnt[]
   describe an unsorted PNG (alpha/color) palette.  We generate
   mapping[] such that mapping[x] is the index into the sorted PNG
   palette of the alpha/color pair whose index is x in the unsorted
   PNG palette.  This mapping sorts the palette so that opaque entries
   are last.
-----------------------------------------------------------------------------*/
    unsigned int bot_idx;
    unsigned int top_idx;
    unsigned int colorIndex;
    
    /* We start one index at the bottom of the palette index range
       and another at the top.  We run through the unsorted palette,
       and when we see an opaque entry, we map it to the current top
       cursor and bump it down.  When we see a non-opaque entry, we map 
       it to the current bottom cursor and bump it up.  Because the input
       and output palettes are the same size, the two cursors should meet
       right when we process the last entry of the unsorted palette.
    */    
    bot_idx = 0;
    top_idx = alphas_first_index[colors-1] + alphas_of_color_cnt[colors-1] - 1;
    
    for (colorIndex = 0;  colorIndex < colors;  ++colorIndex) {
        unsigned int j;
        for (j = 0; j < alphas_of_color_cnt[colorIndex]; ++j) {
            unsigned int const paletteIndex = 
                alphas_first_index[colorIndex] + j;
            if (alphas_of_color[colorIndex][j] == alphaMaxval)
                mapping[paletteIndex] = top_idx--;
            else
                mapping[paletteIndex] = bot_idx++;
        }
    }
    /* indices should have just crossed paths */
    if (bot_idx != top_idx + 1) {
        pm_error ("internal inconsistency: "
                  "remapped bot_idx = %u, top_idx = %u",
                  bot_idx, top_idx);
    }
    *transSizeP = bot_idx;
}



static void
compute_alpha_palette(FILE *         const ifP, 
                      int            const cols,
                      int            const rows,
                      xelval         const maxval,
                      int            const format,
                      pm_filepos     const rasterPos,
                      gray **        const alpha_mask,
                      gray           const alphaMaxval,
                      pixel                palette_pnm[],
                      gray                 trans_pnm[],
                      unsigned int * const paletteSizeP,
                      unsigned int * const transSizeP,
                      bool *         const tooBigP) {
/*----------------------------------------------------------------------------
   Return the palette of color/alpha pairs for the image indicated by
   'ifP', 'cols', 'rows', 'maxval', 'format', and 'rasterPos'.
   alpha_mask[] is the Netpbm-style alpha mask for the image.

   Return the palette as the arrays palette_pnm[] and trans_pnm[].
   The ith entry in the palette is the combination of palette_pnm[i],
   which defines the color, and trans_pnm[i], which defines the
   transparency.

   Return the number of entries in the palette as *paletteSizeP.

   The palette is sorted so that the opaque entries are last, and we return
   *transSizeP as the number of non-opaque entries.

   palette[] and trans[] are allocated by the caller to at least 
   MAXPALETTEENTRIES elements.

   If there are more than MAXPALETTEENTRIES color/alpha pairs in the image, 
   don't return any palette information -- just return *tooBigP == TRUE.
-----------------------------------------------------------------------------*/
    colorhist_vector chv;
    unsigned int colors;

    gray * alphas_of_color[MAXPALETTEENTRIES];
    unsigned int alphas_first_index[MAXPALETTEENTRIES];
    unsigned int alphas_of_color_cnt[MAXPALETTEENTRIES];
 
    getChv(ifP, rasterPos, cols, rows, maxval, format, MAXCOLORS, 
           &chv, &colors);

    assert(colors < ARRAY_SIZE(alphas_of_color));

    computeUnsortedAlphaPalette(ifP, cols, rows, maxval, format, rasterPos,
                                alpha_mask, chv, colors,
                                MAXPALETTEENTRIES,
                                alphas_of_color,
                                alphas_first_index,
                                alphas_of_color_cnt,
                                tooBigP);

    if (!*tooBigP) {
        unsigned int mapping[MAXPALETTEENTRIES];
            /* Sorting of the alpha/color palette.  mapping[x] is the
               index into the sorted PNG palette of the alpha/color
               pair whose index is x in the unsorted PNG palette.
               This mapping sorts the palette so that opaque entries
               are last.  
            */

        *paletteSizeP = colors == 0 ?
            0 :
            alphas_first_index[colors-1] + alphas_of_color_cnt[colors-1];
        assert(*paletteSizeP <= MAXPALETTEENTRIES);

        /* Make the opaque palette entries last */
        sortAlphaPalette(alphas_of_color, alphas_first_index,
                         alphas_of_color_cnt, colors, alphaMaxval,
                         mapping, transSizeP);

        {
            unsigned int colorIndex;

            for (colorIndex = 0; colorIndex < colors; ++colorIndex) {
                unsigned int j;
                for (j = 0; j < alphas_of_color_cnt[colorIndex]; ++j) {
                    unsigned int const paletteIndex = 
                        alphas_first_index[colorIndex] + j;
                    palette_pnm[mapping[paletteIndex]] = chv[colorIndex].color;
                    trans_pnm[mapping[paletteIndex]] = 
                    alphas_of_color[colorIndex][j];
                }
            }
        }
        freeAlphasOfColor(alphas_of_color, colors);
    }
} 



static void
makeOneColorTransparentInPalette(xel            const transColor, 
                                 bool           const exact,
                                 pixel                palette_pnm[],
                                 unsigned int   const paletteSize,
                                 gray                 trans_pnm[],
                                 unsigned int * const transSizeP) {
/*----------------------------------------------------------------------------
   Find the color 'transColor' in the color/alpha palette defined by
   palette_pnm[], paletteSize, trans_pnm[] and *transSizeP.  

   Make that entry fully transparent.

   Rearrange the palette so that that entry is first.  (The PNG compressor
   can do a better job when the opaque entries are all last in the 
   color/alpha palette).

   If the specified color is not there and exact == TRUE, return
   without changing anything, but issue a warning message.  If it's
   not there and exact == FALSE, just find the closest color.

   We assume every entry in the palette is opaque upon entry.

   A valid palette has at least one color.
-----------------------------------------------------------------------------*/
    unsigned int transparentIndex;
    unsigned int distance;

    assert(paletteSize > 0);
    
    if (*transSizeP != 0)
        pm_error("Internal error: trying to make a color in the palette "
                 "transparent where there already is one.");

    closestColorInPalette(transColor, palette_pnm, paletteSize, 
                          &transparentIndex, &distance);

    if (distance != 0 && exact) {
        pm_message("specified transparent color not present in palette; "
                   "ignoring -transparent");
        errorlevel = PNMTOPNG_WARNING_LEVEL;
    } else {        
        /* Swap this with the first entry in the palette */
        pixel tmp;
    
        tmp = palette_pnm[transparentIndex];
        palette_pnm[transparentIndex] = palette_pnm[0];
        palette_pnm[0] = tmp;
        
        /* Make it transparent */
        trans_pnm[0] = PGM_TRANSPARENT;
        *transSizeP = 1;
        if (verbose) {
            pixel const p = palette_pnm[0];
            pm_message("Making all occurences of color (%u, %u, %u) "
                       "transparent.",
                       PPM_GETR(p), PPM_GETG(p), PPM_GETB(p));
        }
    }
}



static void
findOrAddBackgroundInPalette(pixel          const backColor, 
                             pixel                palette_pnm[], 
                             unsigned int * const paletteSizeP,
                             unsigned int * const backgroundIndexP) {
/*----------------------------------------------------------------------------
  Add the background color 'backColor' to the palette, unless
  it's already in there.  If it's not present and there's no room to
  add it, choose a background color that's already in the palette,
  as close to 'backColor' as possible.

  If we add an entry to the palette, make it opaque.  But in searching the 
  existing palette, ignore transparency.

  Note that PNG specs say that transparency of the background is meaningless;
  i.e. a viewer must ignore the transparency of the palette entry when 
  using the background color.

  Return the palette index of the background color as *backgroundIndexP.
-----------------------------------------------------------------------------*/
    int backgroundIndex;  /* negative means not found */
    unsigned int paletteIndex;

    backgroundIndex = -1;
    for (paletteIndex = 0; 
         paletteIndex < *paletteSizeP; 
         ++paletteIndex) 
        if (PPM_EQUAL(palette_pnm[paletteIndex], backColor))
            backgroundIndex = paletteIndex;

    if (backgroundIndex >= 0) {
        /* The background color is already in the palette. */
        *backgroundIndexP = backgroundIndex;
        if (verbose) {
            pixel const p = palette_pnm[*backgroundIndexP];
            pm_message("background color (%u, %u, %u) appears in image.",
                       PPM_GETR(p), PPM_GETG(p), PPM_GETB(p));
        }
    } else {
        /* Try to add the background color, opaque, to the palette. */
        if (*paletteSizeP < MAXCOLORS) {
            /* There's room, so just add it to the end of the palette */

            /* Because we're not expanding the transparency palette, this
               entry is not in it, and is thus opaque.
            */
            *backgroundIndexP = (*paletteSizeP)++;
            palette_pnm[*backgroundIndexP] = backColor;
            if (verbose) {
                pixel const p = palette_pnm[*backgroundIndexP];
                pm_message("added background color (%u, %u, %u) to palette.",
                           PPM_GETR(p), PPM_GETG(p), PPM_GETB(p));
            }
        } else {
            closestColorInPalette(backColor, palette_pnm, *paletteSizeP,
                                  backgroundIndexP, NULL);
            errorlevel = PNMTOPNG_WARNING_LEVEL;
            {
                pixel const p = palette_pnm[*backgroundIndexP];
                pm_message("no room in palette for background color; "
                           "using closest match (%u, %u, %u) instead",
                           PPM_GETR(p), PPM_GETG(p), PPM_GETB(p));
            }
        }
    }
}



static void 
buildColorLookup(pixel                   palette_pnm[], 
                 unsigned int      const paletteSize,
                 colorhash_table * const chtP) {
/*----------------------------------------------------------------------------
   Create a colorhash_table out of the palette described by
   palette_pnm[] (which has 'paletteSize' entries) so one can look up
   the palette index of a given color.

   Where the same color appears twice in the palette, the lookup table
   finds an arbitrary one of them.  We don't consider transparency of
   palette entries, so if the same color appears in the palette once
   transparent and once opaque, the lookup table finds an arbitrary one
   of those two.
-----------------------------------------------------------------------------*/
    colorhash_table const cht = ppm_alloccolorhash();
    unsigned int paletteIndex;

    for (paletteIndex = 0; paletteIndex < paletteSize; ++paletteIndex) {
        ppm_addtocolorhash(cht, &palette_pnm[paletteIndex], paletteIndex);
    }
    *chtP = cht;
}



static void 
buildColorAlphaLookup(pixel              palette_pnm[], 
                      unsigned int const paletteSize,
                      gray               trans_pnm[], 
                      unsigned int const transSize,
                      gray         const alphaMaxval,
                      coloralphahash_table * const cahtP) {
    
    coloralphahash_table const caht = alloccoloralphahash();

    unsigned int paletteIndex;

    for (paletteIndex = 0; paletteIndex < paletteSize; ++paletteIndex) {
        gray paletteTrans;

        if (paletteIndex < transSize)
            paletteTrans = alphaMaxval;
        else
            paletteTrans = trans_pnm[paletteIndex];

        addtocoloralphahash(caht, &palette_pnm[paletteIndex],
                            &trans_pnm[paletteIndex], paletteIndex);
    }
    *cahtP = caht;
}



static void
tryAlphaPalette(FILE *         const ifP,
                int            const cols,
                int            const rows,
                xelval         const maxval,
                int            const format,
                pm_filepos     const rasterPos,
                gray **        const alpha_mask,
                gray           const alphaMaxval,
                FILE *         const pfP,
                pixel *        const palette_pnm,
                unsigned int * const paletteSizeP,
                gray *         const trans_pnm,
                unsigned int * const transSizeP,
                const char **  const impossibleReasonP) {
/*----------------------------------------------------------------------------
   Try to make an alpha palette as 'trans_pnm', size *transSizeP.

   If it's impossible, return as *impossibleReasonP newly malloced storage
   containing text that tells why.  But if we succeed, return
   *impossibleReasonP == NULL.
-----------------------------------------------------------------------------*/
    bool tooBig;
    if (pfP)
        pm_error("This program is not capable of generating "
                 "a PNG with transparency when you specify "
                 "the palette with -palette.");

    compute_alpha_palette(ifP, cols, rows, maxval, format, 
                          rasterPos,  alpha_mask, alphaMaxval,
                          palette_pnm, trans_pnm, 
                          paletteSizeP, transSizeP, &tooBig);
    if (tooBig) {
        asprintfN(impossibleReasonP,
                  "too many color/transparency pairs "
                  "(more than the PNG maximum of %u", 
                  MAXPALETTEENTRIES);
    } else
        *impossibleReasonP = NULL;
} 



static void
computePixelWidth(int            const pnm_type,
                  unsigned int   const pnm_meaningful_bits,
                  bool           const alpha,
                  unsigned int * const bitsPerSampleP,
                  unsigned int * const bitsPerPixelP) {

    unsigned int bitsPerSample, bitsPerPixel;

    if (pnm_type == PPM_TYPE || alpha) {
        /* PNG allows only depths of 8 and 16 for a truecolor image 
           and for a grayscale image with an alpha channel.
          */
        if (pnm_meaningful_bits > 8)
            bitsPerSample = 16;
        else 
            bitsPerSample = 8;
    } else {
        /* A grayscale, non-colormapped, no-alpha PNG may have any 
             bit depth from 1 to 16
          */
        if (pnm_meaningful_bits > 8)
            bitsPerSample = 16;
        else if (pnm_meaningful_bits > 4)
            bitsPerSample = 8;
        else if (pnm_meaningful_bits > 2)
            bitsPerSample = 4;
        else if (pnm_meaningful_bits > 1)
            bitsPerSample = 2;
        else
            bitsPerSample = 1;
    }
    if (alpha) {
        if (pnm_type == PPM_TYPE)
            bitsPerPixel = 4 * bitsPerSample;
        else
            bitsPerPixel = 2 * bitsPerSample;
    } else {
        if (pnm_type == PPM_TYPE)
            bitsPerPixel = 3 * bitsPerSample;
        else
            bitsPerPixel = bitsPerSample;
    }
    if (bitsPerPixelP)
        *bitsPerPixelP = bitsPerPixel;
    if (bitsPerSampleP)
        *bitsPerSampleP = bitsPerSample;
}



static unsigned int
paletteIndexBits(unsigned int const nColors) {
/*----------------------------------------------------------------------------
  Return the number of bits that a palette index in the PNG will
  occupy given that the palette has 'nColors' colors in it.  It is 1,
  2, 4, or 8 bits.
  
  If 'nColors' is not a valid PNG palette size, return 0.
-----------------------------------------------------------------------------*/
    unsigned int retval;

    if (nColors < 1)
        retval = 0;
    else if (nColors <= 2)
        retval = 1;
    else if (nColors <= 4)
        retval = 2;
    else if (nColors <= 16)
        retval = 4;
    else if (nColors <= 256)
        retval = 8;
    else
        retval = 0;

    return retval;
}



static void
computeColorMap(FILE *         const ifP,
                pm_filepos     const rasterPos,
                int            const cols,
                int            const rows,
                xelval         const maxval,
                int            const format,
                bool           const force,
                FILE *         const pfP,
                bool           const alpha,
                bool           const transparent,
                pixel          const transcolor,
                bool           const transexact,
                bool           const background,
                pixel          const backcolor,
                gray **        const alpha_mask,
                gray           const alphaMaxval,
                unsigned int   const pnm_meaningful_bits,
                /* Outputs */
                pixel *        const palette_pnm,
                unsigned int * const paletteSizeP,
                gray *         const trans_pnm,
                unsigned int * const transSizeP,
                unsigned int * const backgroundIndexP,
                const char **  const noColormapReasonP) {
/*---------------------------------------------------------------------------
  Determine whether to do a colormapped or truecolor PNG and if
  colormapped, compute the full PNG palette -- both color and
  transparency.

  If we decide to do truecolor, we return as *noColormapReasonP a text
  description of why, in newly malloced memory.  If we decide to go
  with colormapped, we return *noColormapReasonP == NULL.

  In the colormapped case, we return the palette as arrays
  palette_pnm[] and trans_pnm[], allocated by Caller, with sizes
  *paletteSizeP and *transSizeP.

  'background' means the image is to have a background color, and that
  color is 'backcolor'.  'backcolor' is meaningless when 'background'
  is false.

  If the image is to have a background color, we return the palette index
  of that color as *backgroundIndexP.
-------------------------------------------------------------------------- */
    if (force)
        asprintfN(noColormapReasonP, "You requested no color map");
    else if (maxval > PALETTEMAXVAL)
        asprintfN(noColormapReasonP, "The maxval of the input image (%u) "
                  "exceeds the PNG palette maxval (%u)", 
                  maxval, PALETTEMAXVAL);
    else {
        unsigned int bitsPerPixel;
        computePixelWidth(PNM_FORMAT_TYPE(format), pnm_meaningful_bits, alpha,
                          NULL, &bitsPerPixel);

        if (!pfP && bitsPerPixel == 1)
            /* No palette can beat 1 bit per pixel -- no need to waste time
               counting the colors.
            */
            asprintfN(noColormapReasonP, "pixel is already only 1 bit");
        else {
            /* We'll have to count the colors ('colors') to know if a
               palette is possible and desirable.  Along the way, we'll
               compute the actual set of colors (chv) too, and then create
               the palette itself if we decide we want one.
            */
            colorhist_vector chv;
            unsigned int colors;
            
            getChv(ifP, rasterPos, cols, rows, maxval, format, MAXCOLORS, 
                   &chv, &colors);

            if (chv == NULL) {
                asprintfN(noColormapReasonP, 
                          "More than %u colors found -- too many for a "
                          "colormapped PNG", MAXCOLORS);
            } else {
                /* There are few enough colors that a palette is possible */
                if (bitsPerPixel <= paletteIndexBits(colors) && !pfP)
                    asprintfN(noColormapReasonP, 
                              "palette index for %u colors would be "
                              "no smaller than the indexed value (%u bits)", 
                              colors, bitsPerPixel);
                else {
                    unsigned int paletteSize;
                    unsigned int transSize;
                    if (alpha)
                        tryAlphaPalette(ifP, cols, rows, maxval, format,
                                        rasterPos, alpha_mask, alphaMaxval,
                                        pfP,
                                        palette_pnm, &paletteSize, 
                                        trans_pnm, &transSize,
                                        noColormapReasonP);

                    else {
                        *noColormapReasonP = NULL;

                        compute_nonalpha_palette(chv, colors, maxval, pfP,
                                                 palette_pnm, &paletteSize, 
                                                 trans_pnm, &transSize);
    
                        if (transparent)
                            makeOneColorTransparentInPalette(
                                transcolor, transexact, 
                                palette_pnm, paletteSize, trans_pnm, 
                                &transSize);
                    }
                    if (!*noColormapReasonP) {
                        if (background)
                            findOrAddBackgroundInPalette(
                                backcolor, palette_pnm, &paletteSize,
                                backgroundIndexP);
                        *paletteSizeP = paletteSize;
                        *transSizeP   = transSize;
                    }
                }
            }
            freeChv();
        }
    }
}



static void computeColorMapLookupTable(
    bool                   const colorMapped,
    pixel                        palette_pnm[],
    unsigned int           const palette_size,
    gray                         trans_pnm[],
    unsigned int           const trans_size,
    bool                   const alpha,
    xelval                 const alpha_maxval,
    colorhash_table *      const chtP,
    coloralphahash_table * const cahtP) {
/*----------------------------------------------------------------------------
   Compute applicable lookup tables for the palette index.  If there's no
   alpha mask, this is just a standard Netpbm colorhash_table.  If there's
   an alpha mask, it is the slower Pnmtopng-specific 
   coloralphahash_table.

   If a lookup table is not applicable to the image, return NULL as
   its address.  (If the image is not colormapped, both will be NULL).
-----------------------------------------------------------------------------*/
    if (colorMapped) {
        if (alpha) {
            buildColorAlphaLookup(palette_pnm, palette_size, 
                                  trans_pnm, trans_size, alpha_maxval, cahtP);
            *chtP = NULL;
        } else { 
            buildColorLookup(palette_pnm, palette_size, chtP);
            *cahtP = NULL;
        }
        if (verbose)
            pm_message("PNG palette has %u entries, %u of them non-opaque",
                       palette_size, trans_size);
    } else {
        *chtP = NULL;
        *cahtP = NULL;
    }
}



static void
computeRasterWidth(bool           const colorMapped,
                   unsigned int   const palette_size,
                   int            const pnm_type,
                   unsigned int   const pnm_meaningful_bits,
                   bool           const alpha,
                   unsigned int * const bitsPerSampleP,
                   unsigned int * const bitsPerPixelP) {
/*----------------------------------------------------------------------------
   Compute the number of bits per raster sample and per raster pixel:
   *bitsPerSampleP and *bitsPerPixelP.  Note that a raster element may be a
   palette index, or a gray value or color with or without alpha mask.
-----------------------------------------------------------------------------*/
    if (colorMapped) {
        /* The raster element is a palette index */
        if (palette_size <= 2)
            *bitsPerSampleP = 1;
        else if (palette_size <= 4)
            *bitsPerSampleP = 2;
        else if (palette_size <= 16)
            *bitsPerSampleP = 4;
        else
            *bitsPerSampleP = 8;
        *bitsPerPixelP = *bitsPerSampleP;
        if (verbose)
            pm_message("Writing %d-bit color indexes", *bitsPerSampleP);
    } else {
        /* The raster element is an explicit pixel -- color and transparency */
        computePixelWidth(pnm_type, pnm_meaningful_bits, alpha,
                          bitsPerSampleP, bitsPerPixelP);

        if (verbose)
            pm_message("Writing %d bits per component per pixel", 
                       *bitsPerSampleP);
    }
}


static void
createPngPalette(pixel              palette_pnm[], 
                 unsigned int const paletteSize, 
                 pixval       const maxval,
                 gray               trans_pnm[],
                 unsigned int const transSize,
                 gray               alpha_maxval,
                 png_color          palette[],
                 png_byte           trans[]) {
/*----------------------------------------------------------------------------
   Create the data structure to be passed to the PNG compressor to represent
   the palette -- the whole palette, color + transparency.

   This is basically just a maxval conversion from the Netpbm-format
   equivalents we get as input.
-----------------------------------------------------------------------------*/
    unsigned int i;

    for (i = 0; i < paletteSize; ++i) {
        pixel p;
        PPM_DEPTH(p, palette_pnm[i], maxval, PALETTEMAXVAL);
        palette[i].red   = PPM_GETR(p);
        palette[i].green = PPM_GETG(p);
        palette[i].blue  = PPM_GETB(p);
    }

    for (i = 0; i < transSize; ++i) {
        unsigned int const newmv = PALETTEMAXVAL;
        unsigned int const oldmv = alpha_maxval;
        trans[i] = (trans_pnm[i] * newmv + (oldmv/2)) / oldmv;
    }
}



static void
setCompressionSize(png_struct * const png_ptr,
                   int const    buffer_size) {

#if PNG_LIBPNG_VER >= 10009
    png_set_compression_buffer_size(png_ptr, buffer_size);
#else
    pm_error("Your PNG library cannot set the compression buffer size.  "
             "You need at least Version 1.0.9 of Libpng; you have Version %s",
             PNG_LIBPNG_VER_STRING);
#endif
}



static void
setZlibCompression(png_struct *           const png_ptr,
                   struct zlibCompression const zlibCompression) {

    if (zlibCompression.levelSpec)
        png_set_compression_level(png_ptr, zlibCompression.level);

    if (zlibCompression.memLevelSpec)
        png_set_compression_mem_level(png_ptr, zlibCompression.mem_level);

    if (zlibCompression.strategySpec)
        png_set_compression_strategy(png_ptr, zlibCompression.strategy);

    if (zlibCompression.windowBitsSpec)
        png_set_compression_window_bits(png_ptr, zlibCompression.window_bits);

    if (zlibCompression.methodSpec)
        png_set_compression_method(png_ptr, zlibCompression.method);

    if (zlibCompression.bufferSizeSpec) {
        setCompressionSize(png_ptr, zlibCompression.buffer_size);
    }
}
                  


static void
makePngLine(png_byte *           const line,
            const xel *          const xelrow,
            unsigned int         const cols,
            xelval               const maxval,
            bool                 const alpha,
            gray *               const alpha_mask,
            colorhash_table      const cht,
            coloralphahash_table const caht,
            png_info *           const info_ptr,
            xelval               const png_maxval,
            unsigned int         const depth) {
            
    unsigned int col;
    png_byte *pp;

    pp = line;  /* start at beginning of line */
    for (col = 0; col < cols; ++col) {
        xel p_png;
        xel const p = xelrow[col];
        PPM_DEPTH(p_png, p, maxval, png_maxval);
        if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY ||
            info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            if (depth == 16)
                *pp++ = PNM_GET1(p_png) >> 8;
            *pp++ = PNM_GET1(p_png) & 0xff;
        } else if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
            unsigned int paletteIndex;
            if (alpha)
                paletteIndex = lookupColorAlpha(caht, &p, &alpha_mask[col]);
            else
                paletteIndex = ppm_lookupcolor(cht, &p);
            *pp++ = paletteIndex;
        } else if (info_ptr->color_type == PNG_COLOR_TYPE_RGB ||
                   info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
            if (depth == 16)
                *pp++ = PPM_GETR(p_png) >> 8;
            *pp++ = PPM_GETR(p_png) & 0xff;
            if (depth == 16)
                *pp++ = PPM_GETG(p_png) >> 8;
            *pp++ = PPM_GETG(p_png) & 0xff;
            if (depth == 16)
                *pp++ = PPM_GETB(p_png) >> 8;
            *pp++ = PPM_GETB(p_png) & 0xff;
        } else
            pm_error("INTERNAL ERROR: undefined color_type");
                
        if (info_ptr->color_type & PNG_COLOR_MASK_ALPHA) {
            int const png_alphaval = (int)
                alpha_mask[col] * (float) png_maxval / maxval + 0.5;
            if (depth == 16)
                *pp++ = png_alphaval >> 8;
            *pp++ = png_alphaval & 0xff;
        }
    }
}



static void
writeRaster(png_struct *         const png_ptr,
            png_info *           const info_ptr,
            FILE *               const ifP,
            pm_filepos           const rasterPos,
            unsigned int         const cols,
            unsigned int         const rows,
            xelval               const maxval,
            int                  const format,
            xelval               const png_maxval,
            unsigned             const int depth,
            bool                 const alpha,
            gray **              const alpha_mask,
            colorhash_table      const cht,
            coloralphahash_table const caht
            ) {
/*----------------------------------------------------------------------------
   Write the PNG raster via compressor *png_ptr, reading the PNM raster
   from file *ifP, position 'rasterPos'.

   The PNG raster consists of IDAT chunks.

   'alpha_mask' is defined only if 'alpha' is true.
-----------------------------------------------------------------------------*/
    xel * xelrow;
    png_byte * line;
    unsigned int pass;

    xelrow = pnm_allocrow(cols);

    /* max: 3 color channels, one alpha channel, 16-bit */
    MALLOCARRAY(line, cols * 8);
    if (line == NULL)
        pm_error("out of memory allocating PNG row buffer");

    for (pass = 0; pass < png_set_interlace_handling(png_ptr); ++pass) {
        unsigned int row;
        pm_seek2(ifP, &rasterPos, sizeof(rasterPos));
        for (row = 0; row < rows; ++row) {
            pnm_readpnmrow(ifP, xelrow, cols, maxval, format);
            pnm_promoteformatrow(xelrow, cols, maxval, format, maxval,
                                 PPM_TYPE);
            
            makePngLine(line, xelrow, cols, maxval,
                        alpha, alpha ? alpha_mask[row] : NULL,
                        cht, caht, info_ptr, png_maxval, depth);

            png_write_row(png_ptr, line);
        }
    }
    pnm_freerow(xelrow);
}



static void
doGamaChunk(struct cmdlineInfo const cmdline,
            png_info *         const info_ptr) {
            
    if (cmdline.gammaSpec) {
        /* gAMA chunk */
        info_ptr->valid |= PNG_INFO_gAMA;
        info_ptr->gamma = cmdline.gamma;
    }
}



static void
doChrmChunk(struct cmdlineInfo const cmdline,
            png_info *         const info_ptr) {

    if (cmdline.rgbSpec) {
        /* cHRM chunk */
        info_ptr->valid |= PNG_INFO_cHRM;

        info_ptr->x_white = cmdline.rgb.wx;
        info_ptr->y_white = cmdline.rgb.wy;
        info_ptr->x_red   = cmdline.rgb.rx;
        info_ptr->y_red   = cmdline.rgb.ry;
        info_ptr->x_green = cmdline.rgb.gx;
        info_ptr->y_green = cmdline.rgb.gy;
        info_ptr->x_blue  = cmdline.rgb.bx;
        info_ptr->y_blue  = cmdline.rgb.by;
    }
}



static void
doPhysChunk(struct cmdlineInfo const cmdline,
            png_info *         const info_ptr) {

    if (cmdline.sizeSpec) {
        /* pHYS chunk */
        info_ptr->valid |= PNG_INFO_pHYs;

        info_ptr->x_pixels_per_unit = cmdline.size.x;
        info_ptr->y_pixels_per_unit = cmdline.size.y;
        info_ptr->phys_unit_type    = cmdline.size.unit;
    }
}




static void
doTimeChunk(struct cmdlineInfo const cmdline,
            png_info *         const info_ptr) {

    if (cmdline.modtimeSpec) {
        /* tIME chunk */
        info_ptr->valid |= PNG_INFO_tIME;

        png_convert_from_time_t(&info_ptr->mod_time, cmdline.modtime);
    }
}



static void
doSbitChunk(png_info * const pngInfoP,
            xelval     const pngMaxval,
            xelval     const maxval,
            bool       const alpha,
            xelval     const alphaMaxval) {

    if (pngInfoP->color_type != PNG_COLOR_TYPE_PALETTE &&
        (pngMaxval > maxval || (alpha && pngMaxval > alphaMaxval))) {

        /* We're writing in a bit depth that doesn't match the maxval
           of the input image and the alpha mask.  So we write an sBIT
           chunk to tell what the original image's maxval was.  The
           sBit chunk doesn't let us specify any maxval -- only powers
           of two minus one.  So we pick the power of two minus one
           which is greater than or equal to the actual input maxval.
           
           PNG also doesn't let an sBIT chunk indicate a maxval
           _greater_ than the the PNG maxval.  The designers probably
           did not conceive of the case where that would happen.  The
           case is this: We detected redundancy in the bits so were
           able to store fewer bits than the user provided.  But since
           PNG doesn't allow it, we don't attempt to create such an
           sBIT chunk.
        */

        pngInfoP->valid |= PNG_INFO_sBIT;

        {
            int const sbitval = pm_maxvaltobits(MIN(maxval, pngMaxval));

            if (pngInfoP->color_type & PNG_COLOR_MASK_COLOR) {
                pngInfoP->sig_bit.red   = sbitval;
                pngInfoP->sig_bit.green = sbitval;
                pngInfoP->sig_bit.blue  = sbitval;
            } else
                pngInfoP->sig_bit.gray = sbitval;
            
            if (verbose)
                pm_message("Writing sBIT chunk with bits = %d", sbitval);
        }
        if (pngInfoP->color_type & PNG_COLOR_MASK_ALPHA) {
            pngInfoP->sig_bit.alpha =
                pm_maxvaltobits(MIN(alphaMaxval, pngMaxval));
            if (verbose)
                pm_message("  alpha bits = %d", pngInfoP->sig_bit.alpha);
        }
    }
}



static void 
convertpnm(struct cmdlineInfo const cmdline,
           FILE *             const ifp,
           FILE *             const afp,
           FILE *             const pfp,
           FILE *             const tfp,
           int *              const errorLevelP
    ) {
/*----------------------------------------------------------------------------
   Design note:  It's is really a modularity violation that we have
   all the command line parameters as an argument.  We do it because we're
   lazy -- it takes a great deal of work to carry all that information as
   separate arguments -- and it's only a very small violation.
-----------------------------------------------------------------------------*/
  xel p;
  int rows, cols, format;
  xelval maxval;
      /* The maxval of the input image */
  xelval png_maxval;
      /* The maxval of the samples in the PNG output 
         (must be 1, 3, 7, 15, 255, or 65535)
      */
  pixel transcolor;
      /* The color that is to be transparent, with maxval equal to that
         of the input image.
      */
  int transexact;  
    /* boolean: the user wants only the exact color he specified to be
       transparent; not just something close to it.
    */
  int transparent;
  bool alpha;
    /* There will be an alpha mask */
  unsigned int pnm_meaningful_bits;
  pixel backcolor;
      /* The background color, with maxval equal to that of the input
         image.
      */
  png_struct *png_ptr;
  png_info *info_ptr;

  bool colorMapped;
  pixel palette_pnm[MAXCOLORS];
  png_color palette[MAXCOLORS];
      /* The color part of the color/alpha palette passed to the PNG
         compressor 
      */
  unsigned int palette_size;

  gray trans_pnm[MAXCOLORS];
  png_byte  trans[MAXCOLORS];
      /* The alpha part of the color/alpha palette passed to the PNG
         compressor 
      */
  unsigned int trans_size;

  colorhash_table cht;
  coloralphahash_table caht;

  unsigned int background_index;
      /* Index into palette[] of the background color. */

  png_uint_16 histogram[MAXCOLORS];
  gray alpha_maxval;
  int alpha_rows;
  int alpha_cols;
  const char * noColormapReason;
      /* The reason that we shouldn't make a colormapped PNG, or NULL if
         we should.  malloc'ed null-terminated string.
      */
  unsigned int depth;
      /* The number of bits per sample in the (uncompressed) png 
         raster -- if the raster contains palette indices, this is the
         number of bits in the index.
      */
  unsigned int fulldepth;
      /* The total number of bits per pixel in the (uncompressed) png
         raster, including all channels 
      */
  pm_filepos rasterPos;  
      /* file position in input image file of start of image (i.e. after
         the header)
      */
  xel *xelrow;    /* malloc'ed */
      /* The row of the input image currently being processed */

  int pnm_type;
  xelval maxmaxval;
  gray ** alpha_mask;

  /* these guys are initialized to quiet compiler warnings: */
  maxmaxval = 255;
  alpha_mask = NULL;
  depth = 0;
  errorlevel = 0;

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
    &pnmtopng_jmpbuf_struct, pnmtopng_error_handler, NULL);
  if (png_ptr == NULL) {
    pm_closer (ifp);
    pm_error ("cannot allocate main libpng structure (png_ptr)");
  }

  info_ptr = png_create_info_struct (png_ptr);
  if (info_ptr == NULL) {
    png_destroy_write_struct (&png_ptr, (png_infopp)NULL);
    pm_closer (ifp);
    pm_error ("cannot allocate libpng info structure (info_ptr)");
  }

  if (setjmp (pnmtopng_jmpbuf_struct.jmpbuf)) {
    png_destroy_write_struct (&png_ptr, &info_ptr);
    pm_closer (ifp);
    pm_error ("setjmp returns error condition (1)");
  }

  pnm_readpnminit (ifp, &cols, &rows, &maxval, &format);
  pm_tell2(ifp, &rasterPos, sizeof(rasterPos));
  pnm_type = PNM_FORMAT_TYPE (format);

  xelrow = pnm_allocrow(cols);

  if (verbose) {
    if (pnm_type == PBM_TYPE)    
      pm_message ("reading a PBM file (maxval=%d)", maxval);
    else if (pnm_type == PGM_TYPE)    
      pm_message ("reading a PGM file (maxval=%d)", maxval);
    else if (pnm_type == PPM_TYPE)    
      pm_message ("reading a PPM file (maxval=%d)", maxval);
  }

  if (pnm_type == PGM_TYPE)
    maxmaxval = PGM_OVERALLMAXVAL;
  else if (pnm_type == PPM_TYPE)
    maxmaxval = PPM_OVERALLMAXVAL;

  if (cmdline.transparent) {
      const char * transstring2;  
          /* The -transparent value, but with possible leading '=' removed */
      if (cmdline.transparent[0] == '=') {
          transexact = 1;
          transstring2 = &cmdline.transparent[1];
      } else {
          transexact = 0;
          transstring2 = cmdline.transparent;
      }  
      /* We do this funny PPM_DEPTH thing instead of just passing 'maxval'
         to ppm_parsecolor() because ppm_parsecolor() does a cheap maxval
         scaling, and this is more precise.
      */
      PPM_DEPTH (transcolor, ppm_parsecolor(transstring2, maxmaxval),
                 maxmaxval, maxval);
  }
  if (cmdline.alpha) {
    pixel alpha_transcolor;
    bool alpha_can_be_transparency_index;
    bool all_opaque;

    if (verbose)
      pm_message ("reading alpha-channel image...");
    alpha_mask = pgm_readpgm (afp, &alpha_cols, &alpha_rows, &alpha_maxval);

    if (alpha_cols != cols || alpha_rows != rows) {
      png_destroy_write_struct (&png_ptr, &info_ptr);
      pm_closer (ifp);
      pm_error ("dimensions for image and alpha mask do not agree");
    }
    analyzeAlpha(ifp, rasterPos, cols, rows, maxval, format, 
                 alpha_mask, alpha_maxval, &all_opaque,
                 &alpha_can_be_transparency_index, &alpha_transcolor);

    if (alpha_can_be_transparency_index && !cmdline.force) {
      if (verbose)
        pm_message ("converting alpha mask to transparency index");
      alpha = FALSE;
      transparent = 2;
      transcolor = alpha_transcolor;
    } else if (all_opaque) {
        alpha = FALSE;
        transparent = -1;
    } else {
      alpha = TRUE;
      transparent = -1;
    }
  } else {
      /* Though there's no alpha_mask, we still need an alpha_maxval for
         use with trans[], which can have stuff in it if the user specified
         a transparent color.
      */
      alpha = FALSE;
      alpha_maxval = 255;
      transparent = cmdline.transparent ? 1 : -1;
  }
  if (cmdline.background) 
      PPM_DEPTH(backcolor, ppm_parsecolor(cmdline.background, maxmaxval), 
                maxmaxval, maxval);;

  /* first of all, check if we have a grayscale image written as PPM */

  if (pnm_type == PPM_TYPE && !cmdline.force) {
      unsigned int row;
      bool isgray;

      isgray = TRUE;  /* initial assumption */
      pm_seek2(ifp, &rasterPos, sizeof(rasterPos));
      for (row = 0; row < rows && isgray; ++row) {
          unsigned int col;
          pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
          for (col = 0; col < cols && isgray; ++col) {
              p = xelrow[col];
              if (PPM_GETR(p) != PPM_GETG(p) || PPM_GETG(p) != PPM_GETB(p))
                  isgray = FALSE;
          }
      }
      if (isgray)
          pnm_type = PGM_TYPE;
  }

  /* handle `odd' maxvalues */

  if (maxval > 65535 && !cmdline.downscale) {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      pm_closer(ifp);
      pm_error("can only handle files up to 16-bit "
               "(use -downscale to override");
  }

  findRedundantBits(ifp, rasterPos, cols, rows, maxval, format, alpha,
                    cmdline.force, &pnm_meaningful_bits);
  
  computeColorMap(ifp, rasterPos, cols, rows, maxval, format,
                  cmdline.force, pfp,
                  alpha, transparent >= 0, transcolor, transexact, 
                  !!cmdline.background, backcolor,
                  alpha_mask, alpha_maxval, pnm_meaningful_bits,
                  palette_pnm, &palette_size, trans_pnm, &trans_size,
                  &background_index, &noColormapReason);

  if (noColormapReason) {
      if (pfp)
          pm_error("You specified a particular palette, but this image "
                   "cannot be represented by any palette.  %s",
                   noColormapReason);
      if (verbose)
          pm_message("Not using color map.  %s", noColormapReason);
      strfree(noColormapReason);
      colorMapped = FALSE;
  } else
      colorMapped = TRUE;
  
  computeColorMapLookupTable(colorMapped, palette_pnm, palette_size,
                             trans_pnm, trans_size, alpha, alpha_maxval,
                             &cht, &caht);

  computeRasterWidth(colorMapped, palette_size, pnm_type, 
                     pnm_meaningful_bits, alpha,
                     &depth, &fulldepth);
  if (verbose)
    pm_message ("writing a%s %d-bit %s%s file%s",
                fulldepth == 8 ? "n" : "", fulldepth,
                colorMapped ? "palette": 
                (pnm_type == PPM_TYPE ? "RGB" : "gray"),
                alpha ? (colorMapped ? "+transparency" : "+alpha") : "",
                cmdline.interlace ? " (interlaced)" : "");

  /* now write the file */

  png_maxval = pm_bitstomaxval(depth);

  if (setjmp (pnmtopng_jmpbuf_struct.jmpbuf)) {
    png_destroy_write_struct (&png_ptr, &info_ptr);
    pm_closer (ifp);
    pm_error ("setjmp returns error condition (2)");
  }

  png_init_io (png_ptr, stdout);
  info_ptr->width = cols;
  info_ptr->height = rows;
  info_ptr->bit_depth = depth;

  if (colorMapped)
    info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
  else if (pnm_type == PPM_TYPE)
    info_ptr->color_type = PNG_COLOR_TYPE_RGB;
  else
    info_ptr->color_type = PNG_COLOR_TYPE_GRAY;

  if (alpha && info_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
    info_ptr->color_type |= PNG_COLOR_MASK_ALPHA;

  info_ptr->interlace_type = cmdline.interlace;

  doGamaChunk(cmdline, info_ptr);

  doChrmChunk(cmdline, info_ptr);

  doPhysChunk(cmdline, info_ptr);

  if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {

    /* creating PNG palette  (PLTE and tRNS chunks) */

    createPngPalette(palette_pnm, palette_size, maxval,
                     trans_pnm, trans_size, alpha_maxval, 
                     palette, trans);
    info_ptr->valid |= PNG_INFO_PLTE;
    info_ptr->palette = palette;
    info_ptr->num_palette = palette_size;
    if (trans_size > 0) {
        info_ptr->valid |= PNG_INFO_tRNS;
        info_ptr->trans = trans;
        info_ptr->num_trans = trans_size;   /* omit opaque values */
    }
    /* creating hIST chunk */
    if (cmdline.hist) {
        colorhist_vector chv;
        unsigned int colors;
        colorhash_table cht;
        
        getChv(ifp, rasterPos, cols, rows, maxval, format, MAXCOLORS, 
               &chv, &colors);

        cht = ppm_colorhisttocolorhash (chv, colors);
                
        { 
            unsigned int i;
            for (i = 0 ; i < MAXCOLORS; ++i) {
                int const chvIndex = ppm_lookupcolor(cht, &palette_pnm[i]);
                if (chvIndex == -1)
                    histogram[i] = 0;
                else
                    histogram[i] = chv[chvIndex].value;
            }
        }

        ppm_freecolorhash(cht);

        info_ptr->valid |= PNG_INFO_hIST;
        info_ptr->hist = histogram;
        if (verbose)
            pm_message("histogram created");
    }
  } else { /* color_type != PNG_COLOR_TYPE_PALETTE */
    if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY ||
        info_ptr->color_type == PNG_COLOR_TYPE_RGB) {
        if (transparent > 0) {
            info_ptr->valid |= PNG_INFO_tRNS;
            info_ptr->trans_values = 
                xelToPngColor_16(transcolor, maxval, png_maxval);
        }
    } else {
        /* This is PNG_COLOR_MASK_ALPHA.  Transparency will be handled
           by the alpha channel, not a transparency color.
        */
    }
    if (verbose) {
        if (info_ptr->valid && PNG_INFO_tRNS) 
            pm_message("Transparent color {gray, red, green, blue} = "
                       "{%d, %d, %d, %d}",
                       info_ptr->trans_values.gray,
                       info_ptr->trans_values.red,
                       info_ptr->trans_values.green,
                       info_ptr->trans_values.blue);
        else
            pm_message("No transparent color");
    }
  }

  /* bKGD chunk */
  if (cmdline.background) {
      info_ptr->valid |= PNG_INFO_bKGD;
      if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
          info_ptr->background.index = background_index;
      } else {
          info_ptr->background = 
              xelToPngColor_16(backcolor, maxval, png_maxval);
          if (verbose)
              pm_message("Writing bKGD chunk with background color "
                         " {gray, red, green, blue} = {%d, %d, %d, %d}",
                         info_ptr->background.gray, 
                         info_ptr->background.red, 
                         info_ptr->background.green, 
                         info_ptr->background.blue ); 
      }
  }

  doSbitChunk(info_ptr, png_maxval, maxval, alpha, alpha_maxval);

  /* tEXT and zTXT chunks */
  if (cmdline.text || cmdline.ztxt)
      pnmpng_read_text(info_ptr, tfp, !!cmdline.ztxt, cmdline.verbose);

  doTimeChunk(cmdline, info_ptr);

  if (cmdline.filterSet != 0)
      png_set_filter(png_ptr, 0, cmdline.filterSet);

  setZlibCompression(png_ptr, cmdline.zlibCompression);

  /* write the png-info struct */
  png_write_info(png_ptr, info_ptr);

  if (cmdline.text || cmdline.ztxt)
      /* prevent from being written twice with png_write_end */
      info_ptr->num_text = 0;

  if (cmdline.modtime)
      /* prevent from being written twice with png_write_end */
      info_ptr->valid &= ~PNG_INFO_tIME;

  /* let libpng take care of, e.g., bit-depth conversions */
  png_set_packing (png_ptr);

  writeRaster(png_ptr, info_ptr, ifp, rasterPos, cols, rows, maxval, format,
              png_maxval, depth, alpha, alpha_mask, cht, caht);

  png_write_end (png_ptr, info_ptr);


#if 0
  /* The following code may be intended to solve some segfault problem
     that arises with png_destroy_write_struct().  The latter is the
     method recommended in the libpng documentation and this program 
     will not compile under Cygwin because the Windows DLL for libpng
     does not contain png_write_destroy() at all.  Since the author's
     comment below does not make it clear what the segfault issue is,
     we cannot consider it.  -Bryan 00.09.15
*/

  png_write_destroy (png_ptr);
  /* flush first because free(png_ptr) can segfault due to jmpbuf problems
     in png_write_destroy */
  fflush (stdout);
  free (png_ptr);
  free (info_ptr);
#else
  png_destroy_write_struct(&png_ptr, &info_ptr);
#endif

  pnm_freerow(xelrow);

  if (cht)
      ppm_freecolorhash(cht);
  if (caht)
      freecoloralphahash(caht);

  *errorLevelP = errorlevel;
}



static void
displayVersion() {

    fprintf(stderr,"Pnmtopng version %s.\n", NETPBM_VERSION);

    /* We'd like to display the version of libpng with which we're
       linked, as we do for zlib, but it isn't practical.
       While libpng is capable of telling you what it's level
       is, different versions of it do it two different ways: with
       png_libpng_ver or with png_get_header_ver.  So we have to be
       compiled for a particular version just to find out what
       version it is! It's not worth having a link failure, much
       less a compile failure, if we choose wrong.
       png_get_header_ver is not in anything older than libpng 1.0.2a
       (Dec 1998).  png_libpng_ver is not there in libraries built
       without USE_GLOBAL_ARRAYS.  Cygwin versions are normally built
       without USE_GLOBAL_ARRAYS.  -bjh 2002.06.17.
    */
    fprintf(stderr, "   Compiled with libpng %s.\n",
            PNG_LIBPNG_VER_STRING);
    fprintf(stderr, "   Pnmtopng (not libpng) compiled with zlib %s.\n",
            ZLIB_VERSION);
    fprintf(stderr, "\n");
}



int 
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    FILE * afP;
    FILE * pfP;
    FILE * tfP;

    int errorlevel;
    
    pnm_init (&argc, argv);
    
    parseCommandLine(argc, argv, &cmdline);
    
    if (cmdline.libversion) {
        displayVersion();
        return 0;
    }
    verbose = cmdline.verbose;
    
    ifP = pm_openr_seekable(cmdline.inputFilename);
    
    if (cmdline.alpha)
        afP = pm_openr(cmdline.alpha);
    else
        afP = NULL;
    
    if (cmdline.palette)
        pfP = pm_openr(cmdline.palette);
    else
        pfP = NULL;
    
    if (cmdline.text)
        tfP = pm_openr(cmdline.text);
    else if (cmdline.ztxt)
        tfP = pm_openr(cmdline.ztxt);
    else
        tfP = NULL;

    convertpnm(cmdline, ifP, afP, pfP, tfP, &errorlevel);
    
    if (afP)
        pm_close(afP);
    if (pfP)
        pm_close(pfP);
    if (tfP)
        pm_close(tfP);

    pm_close(ifP);
    pm_close(stdout);

    return errorlevel;
}
