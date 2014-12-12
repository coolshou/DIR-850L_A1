/*
 * ppmtobmp.c - Converts from a PPM file to a Microsoft Windows or OS/2
 * .BMP file.
 *
 * Copyright (C) 1992 by David W. Sanderson.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This software is provided "as is"
 * without express or implied warranty.
 *
 */

#define _BSD_SOURCE 1    /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <assert.h>
#include <string.h>
#include "bmp.h"
#include "ppm.h"
#include "shhopt.h"
#include "bitio.h"

#define MAXCOLORS 256

enum colortype {TRUECOLOR, PALETTE};

typedef struct {
/*----------------------------------------------------------------------------
   A color map for a BMP file.
-----------------------------------------------------------------------------*/
    unsigned int count;
        /* Number of colors in the map.  The first 'count' elements of these
           arrays are defined; all others are not.
        */
    colorhash_table cht;

    /* Indices in the following arrays are the same as in 'cht', above. */
    unsigned char red[MAXCOLORS];
    unsigned char grn[MAXCOLORS];
    unsigned char blu[MAXCOLORS];
} colorMap;



static void
freeColorMap(const colorMap * const colorMapP) {

    if (colorMapP->cht)
        ppm_freecolorhash(colorMapP->cht);
}



static struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    char *input_filename;
    int class;  /* C_WIN or C_OS2 */
    unsigned int bppSpec;
    unsigned int bpp;
} cmdline;


static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdline_p) {
/*----------------------------------------------------------------------------
   Note that many of the strings that this function returns in the
   *cmdline_p structure are actually in the supplied argv array.  And
   sometimes, one of these strings is actually just a suffix of an entry
   in argv!
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int windowsSpec, os2Spec;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3('w', "windows",   OPT_FLAG, NULL, &windowsSpec,            0);
    OPTENT3('o', "os2",       OPT_FLAG, NULL, &os2Spec,                0);
    OPTENT3(0,   "bpp",       OPT_UINT, &cmdline_p->bpp, 
            &cmdline_p->bppSpec,      0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

    if (windowsSpec && os2Spec) 
        pm_error("Can't specify both -windows and -os2 options.");
    else if (windowsSpec) 
        cmdline_p->class = C_WIN;
    else if (os2Spec)
        cmdline_p->class = C_OS2;
    else 
        cmdline_p->class = C_WIN;


    if (cmdline_p->bppSpec) {
        if (cmdline_p->bpp != 1 && cmdline_p->bpp != 4 && 
            cmdline_p->bpp != 8 && cmdline_p->bpp != 24)
        pm_error("Invalid -bpp value specified: %u.  The only values valid\n"
                 "in the BMP format are 1, 4, 8, and 24 bits per pixel",
                 cmdline_p->bpp);
    }

    if (argc - 1 == 0)
        cmdline_p->input_filename = strdup("-");  /* he wants stdin */
    else if (argc - 1 == 1)
        cmdline_p->input_filename = strdup(argv[1]);
    else 
        pm_error("Too many arguments.  The only argument accepted\n"
                 "is the input file specificaton");

}



static void
PutByte(FILE * const fp, unsigned char const v) {
    if (putc(v, fp) == EOF) 
        pm_error("Write of a byte to a file failed.");

    /* Note:  a Solaris/SPARC user reported on 2003.09.29 that the above
       putc() returned EOF when a former version of this code declared
       v as "char" instead of "unsigned char".  This was apparently due
       to a bug in his C library that caused 255 to look like -1 at some
       critical internal point.
    */
}



static void
PutShort(FILE * const fp, short const v) {
    if (pm_writelittleshort(fp, v) == -1) 
        pm_error("Write of a halfword to a file failed.");
}



static void
PutLong(FILE * const fp, long const v) {
    if (pm_writelittlelong(fp, v) == -1)
        pm_error("Write of a word to a file failed.");
}



/*
 * BMP writing
 */

static int
BMPwritefileheader(FILE *        const fp, 
                   int           const class, 
                   unsigned long const bitcount, 
                   unsigned long const x, 
                   unsigned long const y) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
-----------------------------------------------------------------------------*/
    PutByte(fp, 'B');
    PutByte(fp, 'M');

    /* cbSize */
    PutLong(fp, BMPlenfile(class, bitcount, -1, x, y));
    
    /* xHotSpot */
    PutShort(fp, 0);
    
    /* yHotSpot */
    PutShort(fp, 0);
    
    /* offBits */
    PutLong(fp, BMPoffbits(class, bitcount, -1));
    
    return 14;
}



static int
BMPwriteinfoheader(FILE *        const fp, 
                   int           const class, 
                   unsigned long const bitcount, 
                   unsigned long const x, 
                   unsigned long const y) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
----------------------------------------------------------------------------*/
    long cbFix;

    switch (class) {
    case C_WIN: {
        cbFix = 40;
        PutLong(fp, cbFix);

        PutLong(fp, x);         /* cx */
        PutLong(fp, y);         /* cy */
        PutShort(fp, 1);        /* cPlanes */
        PutShort(fp, bitcount); /* cBitCount */

        /*
         * We've written 16 bytes so far, need to write 24 more
         * for the required total of 40.
         */

        PutLong(fp, 0);   /* Compression */
        PutLong(fp, 0);   /* ImageSize */
        PutLong(fp, 0);   /* XpixelsPerMeter */
        PutLong(fp, 0);   /* YpixelsPerMeter */
        PutLong(fp, 0);   /* ColorsUsed */
        PutLong(fp, 0);   /* ColorsImportant */
    }
    break;
    case C_OS2: {
        cbFix = 12;
        PutLong(fp, cbFix);

        PutShort(fp, x);        /* cx */
        PutShort(fp, y);        /* cy */
        PutShort(fp, 1);        /* cPlanes */
        PutShort(fp, bitcount); /* cBitCount */
    }
    break;
    default:
        pm_error(er_internal, "BMPwriteinfoheader");
    }

    return cbFix;
}



static int
BMPwritergb(FILE * const fp, 
            int    const class, 
            pixval const R, 
            pixval const G, 
            pixval const B) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
-----------------------------------------------------------------------------*/
    switch (class) {
    case C_WIN:
        PutByte(fp, B);
        PutByte(fp, G);
        PutByte(fp, R);
        PutByte(fp, 0);
        return 4;
    case C_OS2:
        PutByte(fp, B);
        PutByte(fp, G);
        PutByte(fp, R);
        return 3;
    default:
        pm_error(er_internal, "BMPwritergb");
    }
    return -1;
}



static int
BMPwritecolormap(FILE *           const ifP, 
                 int              const class, 
                 int              const bpp,
                 const colorMap * const colorMapP) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
-----------------------------------------------------------------------------*/
    long const ncolors = (1 << bpp);

    unsigned int  nbyte;
    unsigned int  i;

    nbyte = 0;
    for (i = 0; i < colorMapP->count; ++i)
        nbyte += BMPwritergb(ifP, class,
                             colorMapP->red[i],
                             colorMapP->grn[i],
                             colorMapP->blu[i]);

    for (; i < ncolors; ++i)
        nbyte += BMPwritergb(ifP, class, 0, 0, 0);

    return nbyte;
}



static int
BMPwriterow_palette(FILE *          const fp, 
                    const pixel *   const row, 
                    unsigned long   const cx, 
                    unsigned short  const bpp, 
                    colorhash_table const cht) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
-----------------------------------------------------------------------------*/
    BITSTREAM    b;
    int retval;
    
    b = pm_bitinit(fp, "w");
    if (b == NULL)
        retval = -1;
    else {
        unsigned int nbyte;
        unsigned int x;
        bool         error;
        
        nbyte = 0;      /* initial value */
        error = FALSE;  /* initial value */
        
        for (x = 0; x < cx && !error; ++x) {
            int rc;
            rc = pm_bitwrite(b, bpp, ppm_lookupcolor(cht, &row[x]));
            if (rc == -1)
                error = TRUE;
            else
                nbyte += rc;
        }
        if (error)
            retval = -1;
        else {
            int rc;

            rc = pm_bitfini(b);
            if (rc == -1)
                retval = -1;
            else {
                nbyte += rc;
                
                /* Make sure we write a multiple of 4 bytes.  */
                while (nbyte % 4 != 0) {
                    PutByte(fp, 0);
                    ++nbyte;
                }
                retval = nbyte;
            }
        }
    }
    return retval;
}



static int
BMPwriterow_truecolor(FILE *        const fp, 
                      const pixel * const row, 
                      unsigned long const cols,
                      pixval        const maxval) {
/*----------------------------------------------------------------------------
  Write a row of a truecolor BMP image to the file 'fp'.  The row is 
  'row', which is 'cols' columns long.

  Return the number of bytes written.

  On error, issue error message and exit program.
-----------------------------------------------------------------------------*/
    /* This works only for 24 bits per pixel.  To implement this for the
       general case (which is only hypothetical -- this program doesn't
       write any truecolor images except 24 bit and apparently no one
       else does either), you would move this function into 
       BMPwriterow_palette, which writes arbitrary bit strings.  But
       that would be a lot slower and less robust.
    */

    int nbyte;  /* Number of bytes we have written to file so far */
    int col;  
        
    nbyte = 0;  /* initial value */
    for (col = 0; col < cols; col++) {
        /* We scale to the BMP maxval, which is always 255. */
        PutByte(fp, PPM_GETB(row[col]) * 255 / maxval);
        PutByte(fp, PPM_GETG(row[col]) * 255 / maxval);
        PutByte(fp, PPM_GETR(row[col]) * 255 / maxval);
        nbyte += 3;
    }

    /*
     * Make sure we write a multiple of 4 bytes.
     */
    while (nbyte % 4) {
        PutByte(fp, 0);
        nbyte++;
    }
    
    return nbyte;
}



static int
BMPwritebits(FILE *          const fp, 
             unsigned long   const cx, 
             unsigned long   const cy, 
             enum colortype  const colortype,
             unsigned short  const cBitCount, 
             const pixel **  const pixels, 
             pixval          const maxval,
             colorhash_table const cht) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
-----------------------------------------------------------------------------*/
    int  nbyte;
    long y;

    if (cBitCount > 24)
        pm_error("cannot handle cBitCount: %d", cBitCount);

    nbyte = 0;  /* initial value */

    /* The picture is stored bottom line first, top line last */

    for (y = cy - 1; y >= 0; --y) {
        int rc;
        if (colortype == PALETTE)
            rc = BMPwriterow_palette(fp, pixels[y], cx, 
                                     cBitCount, cht);
        else 
            rc = BMPwriterow_truecolor(fp, pixels[y], cx, maxval);

        if (rc == -1)
            pm_error("couldn't write row %ld", y);
        if (rc % 4 != 0)
            pm_error("row had bad number of bytes: %d", rc);
        nbyte += rc;
    }

    return nbyte;
}



static void
BMPEncode(FILE *           const ifP, 
          int              const class, 
          enum colortype   const colortype,
          int              const bpp,
          int              const x, 
          int              const y, 
          const pixel **   const pixels, 
          pixval           const maxval,
          const colorMap * const colorMapP) {
/*----------------------------------------------------------------------------
  Write a BMP file of the given class.
-----------------------------------------------------------------------------*/
    unsigned long nbyte;

    if (colortype == PALETTE)
        pm_message("Writing %d bits per pixel with a color palette", bpp);
    else
        pm_message("Writing %d bits per pixel truecolor (no palette)", bpp);

    nbyte = 0;  /* initial value */
    nbyte += BMPwritefileheader(ifP, class, bpp, x, y);
    nbyte += BMPwriteinfoheader(ifP, class, bpp, x, y);
    if (colortype == PALETTE)
        nbyte += BMPwritecolormap(ifP, class, bpp, colorMapP);

    if (nbyte != (BMPlenfileheader(class)
                  + BMPleninfoheader(class)
                  + BMPlencolormap(class, bpp, -1)))
        pm_error(er_internal, "BMPEncode 1");

    nbyte += BMPwritebits(ifP, x, y, colortype, bpp, pixels, maxval,
                          colorMapP->cht);
    if (nbyte != BMPlenfile(class, bpp, -1, x, y))
        pm_error(er_internal, "BMPEncode 2");
}



static void
makeBilevelColorMap(colorMap * const colorMapP) {

    colorMapP->count  = 2;
    colorMapP->cht    = NULL;
    colorMapP->red[0] = 0;
    colorMapP->grn[0] = 0;
    colorMapP->blu[0] = 0;
    colorMapP->red[1] = 255;
    colorMapP->grn[1] = 255;
    colorMapP->blu[1] = 255;
}



static void
BMPEncodePBM(FILE *           const ifP, 
             int              const class, 
             int              const cols, 
             int              const rows, 
             unsigned char ** const bitrow) {
/*----------------------------------------------------------------------------
  Write a bi-level BMP file of the given class.
-----------------------------------------------------------------------------*/
    /* Note:
       Only PBM input uses this routine.  Color images represented by 1 bpp via
       color palette use the general BMPEncode().
    */
    unsigned int const adjustedCols = (cols + 31) / 32 * 32;
    unsigned int const packedBytes  = adjustedCols / 8;

    unsigned long nbyte;
    colorMap bilevelColorMap;
    unsigned int row;
    
    /* colortype == PALETTE */
    pm_message("Writing 1 bit per pixel with a black-white palette");

    nbyte = 0;  /* initial value */
    nbyte += BMPwritefileheader(ifP, class, 1, cols, rows);
    nbyte += BMPwriteinfoheader(ifP, class, 1, cols, rows);

    makeBilevelColorMap(&bilevelColorMap);

    nbyte += BMPwritecolormap(ifP, class, 1, &bilevelColorMap);

    if (nbyte != (BMPlenfileheader(class)
                  + BMPleninfoheader(class)
                  + BMPlencolormap(class, 1, -1)))
        pm_error(er_internal, "BMPEncodePBM 1");
   
    for (row = 0; row < rows; ++row){
        size_t bytesWritten;

        bytesWritten = fwrite(bitrow[row], 1, packedBytes, ifP);
        if (bytesWritten != packedBytes){
            if (feof(ifP))
                pm_error("End of file writing row %u of BMP raster.", row);
            else 
                pm_error("Error writing BMP raster.  Errno=%d (%s)",
                         errno, strerror(errno));
        }  else
            nbyte += bytesWritten;
    }

    if (nbyte != BMPlenfile(class, 1, -1, cols, rows))
        pm_error(er_internal, "BMPEncodePBM 2");
}



static void
analyze_colors(const pixel **    const pixels, 
               int               const cols, 
               int               const rows, 
               pixval            const maxval, 
               int *             const minimum_bpp_p,
               colorMap *        const colorMapP) {
/*----------------------------------------------------------------------------
  Look at the colors in the image 'pixels' and compute values to use in
  representing those colors in a BMP image.  

  First of all, count the distinct colors.  Return as *minimum_bpp_p
  the minimum number of bits per pixel it will take to represent all
  the colors in BMP format.

  If there are few enough colors to represent with a palette, also
  return the number of colors as *colors_p and a suitable palette
  (colormap) and a hash table in which to look up indexes into that
  palette as *colorMapP.  Use only the first *colors_p entries of the
  map.

  If there are too many colors for a palette, return colorMapP->cht
  == NULL.

  Issue informational messages.
-----------------------------------------------------------------------------*/
    /* Figure out the colormap. */
    colorhist_vector chv;
    int colorCount;

    pm_message("analyzing colors...");
    chv = ppm_computecolorhist((pixel**)pixels, cols, rows, MAXCOLORS, 
                               &colorCount);
    colorMapP->count = colorCount;
    if (chv == NULL) {
        pm_message("More than %u colors found", MAXCOLORS);
        *minimum_bpp_p = 24;
        colorMapP->cht = NULL;
    } else {
        unsigned int const minbits = pm_maxvaltobits(colorMapP->count - 1);

        unsigned int i;

        pm_message("%u colors found", colorMapP->count);

        /* Only 1, 4, 8, and 24 are defined in the BMP spec we
           implement and other bpp's have in fact been seen to confuse
           viewers.  There is an extended BMP format that has 16 bpp
           too, but this program doesn't know how to generate that
           (see Bmptopnm.c, though).  
        */
        if (minbits == 1)
            *minimum_bpp_p = 1;
        else if (minbits <= 4)
            *minimum_bpp_p = 4;
        else if (minbits <= 8)
            *minimum_bpp_p = 8;
        else
            *minimum_bpp_p = 24;

        /*
         * Now scale the maxval to 255 as required by BMP format.
         */
        for (i = 0; i < colorMapP->count; ++i) {
            colorMapP->red[i] = (pixval) PPM_GETR(chv[i].color) * 255 / maxval;
            colorMapP->grn[i] = (pixval) PPM_GETG(chv[i].color) * 255 / maxval;
            colorMapP->blu[i] = (pixval) PPM_GETB(chv[i].color) * 255 / maxval;
        }
    
        /* And make a hash table for fast lookup. */
        colorMapP->cht = ppm_colorhisttocolorhash(chv, colorMapP->count);
        ppm_freecolorhist(chv);
    }
}



static void
choose_colortype_bpp(struct cmdline_info const cmdline,
                     unsigned int        const colors, 
                     unsigned int        const minimum_bpp,
                     enum colortype *    const colortype_p, 
                     unsigned int *      const bits_per_pixel_p) {

    if (!cmdline.bppSpec) {
        /* User has no preference as to bits per pixel.  Choose the
           smallest number possible for this image.
        */
        *bits_per_pixel_p = minimum_bpp;
    } else {
        if (cmdline.bpp < minimum_bpp)
            pm_error("There are too many colors in the image to "
                     "represent in the\n"
                     "number of bits per pixel you requested: %d.\n"
                     "You may use Ppmquant to reduce the number of "
                     "colors in the image.",
                     cmdline.bpp);
        else
            *bits_per_pixel_p = cmdline.bpp;
    }

    assert(*bits_per_pixel_p == 1 || 
           *bits_per_pixel_p == 4 || 
           *bits_per_pixel_p == 8 || 
           *bits_per_pixel_p == 24);

    if (*bits_per_pixel_p > 8) 
        *colortype_p = TRUECOLOR;
    else {
        *colortype_p = PALETTE;
    }
}



static void
doPbm(FILE *       const ifP,
      unsigned int const cols,
      unsigned int const rows,
      int          const format,
      int          const class,
      FILE *       const ofP) {
    
    /*  In the PBM case the raster is read directly from the input by 
        pbm_readpbmrow_packed.  The raster format is almost identical,
        except that BMP specifies rows to be zero-filled to 32 bit borders 
        and that in BMP the bottom row comes first in order.
    */

    int const CHARBITS = (sizeof(unsigned char)*8); 
    int const colChars = pbm_packed_bytes(cols);
    int const adjustedCols = (cols+31) /32 * 32;
    int const packedBytes  =  adjustedCols /8;

    unsigned char ** bitrow;    
    unsigned int row;

    bitrow = pbm_allocarray_packed(adjustedCols, rows);

    for (row = 0; row < rows; ++row) {
        unsigned char * const thisRow = bitrow[rows - row - 1];

        /* Clear end of each row */
        thisRow[packedBytes-1] = 0x00;
        thisRow[packedBytes-2] = 0x00;
        thisRow[packedBytes-3] = 0x00;
        thisRow[packedBytes-4] = 0x00;
        
        pbm_readpbmrow_packed(ifP, thisRow, cols, format);

        {
            unsigned int i;
            for (i = 0; i < colChars; ++i) 
                thisRow[i] = ~thisRow[i]; /* flip all pixels */
        }
        /* This may seem unnecessary, because the color palette 
           (RGB[] in BMPEncodePBM) can be inverted for the same effect.
           However we take this precaution, for there is indication that
           some BMP viewers may get confused with that.
        */

        if (cols % 8 >0) {
            /* adjust final partial byte */
            thisRow[colChars-1] >>= CHARBITS - cols % CHARBITS;
            thisRow[colChars-1] <<= CHARBITS - cols % CHARBITS;
        }
    }

    BMPEncodePBM(ofP, class, cols, rows, bitrow);
}            



static void
doPgmPpm(FILE * const ifP,
         unsigned int const cols,
         unsigned int const rows,
         pixval       const maxval,
         int          const ppmFormat,
         int          const class,
         FILE *       const ofP) {

    /* PGM and PPM.  The input image is read into a PPM array, scanned
       for color analysis and converted to a BMP raster.
       Logic works for PBM.
    */
    int minimumBpp;
    unsigned int bitsPerPixel;
    enum colortype colortype;
    unsigned int row;
    
    pixel ** pixels;
    colorMap colorMap;
    
    pixels = ppm_allocarray(cols, rows);
    
    for (row = 0; row < rows; ++row)
        ppm_readppmrow(ifP, pixels[row], cols, maxval, ppmFormat);
    
    analyze_colors((const pixel**)pixels, cols, rows, maxval, 
                   &minimumBpp, &colorMap);
    
    choose_colortype_bpp(cmdline, colorMap.count, minimumBpp, &colortype, 
                         &bitsPerPixel);
    
    BMPEncode(stdout, class, colortype, bitsPerPixel,
              cols, rows, (const pixel**)pixels, maxval, &colorMap);
    
    freeColorMap(&colorMap);
}



int
main(int argc, char **argv) {

    FILE * ifP;
    int rows;
    int cols;
    pixval maxval;
    int ppmFormat;

    ppm_init(&argc, argv);

    parse_command_line(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.input_filename);
    
    ppm_readppminit(ifP, &cols, &rows, &maxval, &ppmFormat);
    
    if (PPM_FORMAT_TYPE(ppmFormat) == PBM_TYPE)
        doPbm(ifP, cols, rows, ppmFormat, cmdline.class, stdout);
    else
        doPgmPpm(ifP, cols, rows, maxval, ppmFormat, cmdline.class, stdout);

    pm_close(ifP);
    pm_close(stdout);

    return 0;
}
