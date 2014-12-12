/* pnmtopalm.c - read a PNM image and write a Palm Bitmap file
 *
 * Inspired by and using methods from ppmtoTbmp.c by Ian Goldberg
 * <iang@cs.berkeley.edu>, which was based on ppmtopuzz.c by Jef
 * Poskanzer, from the netpbm-1mar1994 package.
 *
 * Mods for multiple bits per pixel were added to ppmtoTbmp.c by
 * George Caswell <tetsujin@sourceforge.net> and Bill Janssen
 * <bill@janssen.org>.
 *
 * Major fixes and new capability added by Paul Bolle <pebolle@tiscali.nl>
 * in late 2004 / early 2005.
 *
 * See LICENSE file for licensing information.
 *
 *  
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "pnm.h"
#include "palm.h"
#include "shhopt.h"
#include "mallocvar.h"

enum compressionType {COMP_NONE, COMP_SCANLINE, COMP_RLE, COMP_PACKBITS};

struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input files */
    char *transparent;          /* -transparent value.  Null if unspec */
    unsigned int depth;         /* -depth value.  0 if unspec */
    unsigned int maxdepth;      /* -maxdepth value.  0 if unspec */
    enum compressionType compression;
    unsigned int verbose;
    unsigned int colormap;
    unsigned int offset;        /* -offset specified */
    unsigned int density;       /* screen density */
    unsigned int withdummy;
};



static void
parseCommandLine(int argc, char ** argv, struct cmdline_info *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optStruct3 opt;  /* set by OPTENT3 */
    optEntry *option_def;
    unsigned int option_def_index;

    unsigned int transSpec, depthSpec, maxdepthSpec, densitySpec;
    unsigned int scanline_compression, rle_compression, packbits_compression;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "transparent",      OPT_STRING, 
            &cmdlineP->transparent, &transSpec, 0);
    OPTENT3(0, "depth",            OPT_UINT, 
            &cmdlineP->depth,       &depthSpec, 0);
    OPTENT3(0, "maxdepth",         OPT_UINT, 
            &cmdlineP->maxdepth,    &maxdepthSpec, 0);
    OPTENT3(0, "scanline_compression", OPT_FLAG, 
            NULL,                   &scanline_compression, 0);
    OPTENT3(0, "rle_compression",  OPT_FLAG, 
            NULL,                   &rle_compression, 0);
    OPTENT3(0, "packbits_compression", OPT_FLAG, 
            NULL,                   &packbits_compression, 0);
    OPTENT3(0, "verbose",          OPT_FLAG, 
            NULL,                   &cmdlineP->verbose, 0);
    OPTENT3(0, "colormap",         OPT_FLAG, 
            NULL,                   &cmdlineP->colormap, 0);
    OPTENT3(0, "offset",           OPT_FLAG, 
            NULL,                   &cmdlineP->offset, 0);
    OPTENT3(0, "density",          OPT_UINT, 
            &cmdlineP->density,     &densitySpec, 0);
    OPTENT3(0, "withdummy",        OPT_FLAG, 
            NULL,                   &cmdlineP->withdummy, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE; /* We have some short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (depthSpec) {
        if (cmdlineP->depth != 1 && cmdlineP->depth != 2 
            && cmdlineP->depth != 4 && cmdlineP->depth != 8
            && cmdlineP->depth != 16)
            pm_error("invalid value for -depth: %u.  Valid values are "
                     "1, 2, 4, 8, and 16", cmdlineP->depth);
    } else
        cmdlineP->depth = 0;

    if (maxdepthSpec) {
        if (cmdlineP->maxdepth != 1 && cmdlineP->maxdepth != 2 
            && cmdlineP->maxdepth != 4 && cmdlineP->maxdepth != 8
            && cmdlineP->maxdepth != 16)
            pm_error("invalid value for -maxdepth: %u.  Valid values are "
                     "1, 2, 4, 8, and 16", cmdlineP->maxdepth);
    } else
        cmdlineP->maxdepth = 0;

    if (depthSpec && maxdepthSpec && 
        cmdlineP->depth > cmdlineP->maxdepth)
        pm_error("-depth value (%u) is greater than -maxdepth (%u) value.",
                 cmdlineP->depth, cmdlineP->maxdepth);

    if (!transSpec)
        cmdlineP->transparent = NULL;

    if (densitySpec) {
        if (cmdlineP->density != PALM_DENSITY_LOW &&
            cmdlineP->density != PALM_DENSITY_ONEANDAHALF &&
            cmdlineP->density != PALM_DENSITY_DOUBLE &&
            cmdlineP->density != PALM_DENSITY_TRIPLE &&
            cmdlineP->density != PALM_DENSITY_QUADRUPLE)
            pm_error("Invalid value for -density: %d.  Valid values are "
                     "%d, %d, %d, %d and %d.", cmdlineP->density, 
                     PALM_DENSITY_LOW, PALM_DENSITY_ONEANDAHALF,
                     PALM_DENSITY_DOUBLE, PALM_DENSITY_TRIPLE,
                     PALM_DENSITY_QUADRUPLE);
    } else
        cmdlineP->density = PALM_DENSITY_LOW;

    if (cmdlineP->density != PALM_DENSITY_LOW && cmdlineP->withdummy)
            pm_error("You can't specify -withdummy with -density value %u.  "
                     "It is valid only with low density (%u)",
                     cmdlineP->density, PALM_DENSITY_LOW);

    if (cmdlineP->withdummy && !cmdlineP->offset)
        pm_error("-withdummy does not make sense without -offset");

    if (scanline_compression + rle_compression + packbits_compression > 1)
        pm_error("You may specify only one of -scanline_compression, "
                 "-rle_compression, and -packbits_compression");
    else {
        if (scanline_compression)
            cmdlineP->compression = COMP_SCANLINE;
        else if (rle_compression)
            cmdlineP->compression = COMP_RLE;
        else if (packbits_compression)
            cmdlineP->compression = COMP_PACKBITS;
        else
            cmdlineP->compression = COMP_NONE;
    }
        
    if (argc-1 > 1)
        pm_error("This program takes at most 1 argument: the file name.  "
                 "You specified %d", argc-1);
    else if (argc-1 > 0) 
        cmdlineP->inputFilespec = argv[1];
    else
        cmdlineP->inputFilespec = "-";
}



static void
determinePalmFormat(unsigned int   const cols, 
                    unsigned int   const rows, 
                    xelval         const maxval, 
                    int            const format, 
                    xel **         const xels,
                    unsigned int   const specified_bpp,
                    unsigned int   const max_bpp, 
                    bool           const custom_colormap,
                    bool           const verbose,
                    unsigned int * const bppP, 
                    bool *         const directColorP, 
                    Colormap *     const colormapP) {

    if (PNM_FORMAT_TYPE(format) == PBM_TYPE) {
        if (custom_colormap)
            pm_error("You specified -colormap with a black and white input "
                     "image.  -colormap is valid only with color.");
        if (specified_bpp)
            *bppP = specified_bpp;
        else
            *bppP = 1;    /* no point in wasting bits */
        *directColorP = FALSE;
        *colormapP = NULL;
        if (verbose)
            pm_message("output is black and white");
    } else if (PNM_FORMAT_TYPE(format) == PGM_TYPE) {
        /* we can usually handle this one, but may not have enough
           pixels.  So check... */
        if (custom_colormap)
            pm_error("You specified -colormap with a black and white input"
                     "image.  -colormap is valid only with color.");
        if (specified_bpp)
            *bppP = specified_bpp;
        else if (max_bpp && (maxval >= (1 << max_bpp)))
            *bppP = max_bpp;
        else if (maxval > 16)
            *bppP = 4;
        else {
            /* scale to minimum number of bpp needed */
            for (*bppP = 1;  (1 << *bppP) < maxval;  *bppP *= 2)
                ;
        }
        if (*bppP > 4)
            *bppP = 4;
        if (verbose)
            pm_message("output is grayscale %d bits-per-pixel", *bppP);
        *directColorP = FALSE;
        *colormapP = NULL;
    } else if (PNM_FORMAT_TYPE(format) == PPM_TYPE) {

        /* We assume that we only get a PPM if the image cannot be
           represented as PBM or PGM.  There are two options here: either
           8-bit with a colormap, either the standard one or a custom one,
           or 16-bit direct color.  In the 8-bit case, if "custom_colormap"
           is specified (not recommended by Palm) we will put in our own
           colormap; otherwise we will assume that the colors have been
           mapped to the default Palm colormap by appropriate use of
           pnmquant.  We try for 8-bit color first, since it works on
           more PalmOS devices. 
        */
        if ((specified_bpp == 16) || 
            (specified_bpp == 0 && max_bpp == 16)) {
            /* we do the 16-bit direct color */
            *directColorP = TRUE;
            *colormapP = NULL;
            *bppP = 16;
        } else if (!custom_colormap) {
            /* standard indexed 8-bit color */
            *colormapP = palmcolor_build_default_8bit_colormap();
            *bppP = 8;
            if (((specified_bpp != 0) && (specified_bpp != 8)) ||
                ((max_bpp != 0) && (max_bpp < 8)))
                pm_error("Must use depth of 8 for color Palm Bitmap without "
                         "custom color table.");
            *directColorP = FALSE;
            if (verbose)
                pm_message("Output is color with default colormap at 8 bpp");
        } else {
            /* indexed 8-bit color with a custom colormap */
            *colormapP = 
                palmcolor_build_custom_8bit_colormap(rows, cols, xels);
            for (*bppP = 1; (1 << *bppP) < (*colormapP)->ncolors; *bppP *= 2);
            if (specified_bpp != 0) {
                if (specified_bpp >= *bppP)
                    *bppP = specified_bpp;
                else
                    pm_error("Too many colors for specified depth.  "
                             "Use pnmquant to reduce.");
            } else if ((max_bpp != 0) && (max_bpp < *bppP)) {
                pm_error("Too many colors for specified max depth.  "
                         "Use pnmquant to reduce.");
            }
            *directColorP = FALSE;
            if (verbose)
                pm_message("Output is color with custom colormap "
                           "with %d colors at %d bpp", 
                           (*colormapP)->ncolors, *bppP);
        }
    } else {
        pm_error("unknown format 0x%x on input file", (unsigned) format);
    }
}



static const char * 
formatName(int const format) {
    
    const char * retval;

    switch(PNM_FORMAT_TYPE(format)) {
    case PBM_TYPE: retval = "black and white"; break;
    case PGM_TYPE: retval = "grayscale";       break;
    case PPM_TYPE: retval = "color";           break;
    default:       retval = "???";             break;
    }
    return retval;
}

        

static void
findTransparentColor(char *         const colorSpec, 
                     pixval         const newMaxval,
                     bool           const directColor, 
                     pixval         const maxval, 
                     Colormap       const colormap,
                     xel *          const transcolorP, 
                     unsigned int * const transindexP) {

    *transcolorP = ppm_parsecolor(colorSpec, maxval);
    if (!directColor) {
        Color_s const temp_color = 
            ((((PPM_GETR(*transcolorP)*newMaxval) / maxval) << 16) 
             | (((PPM_GETG(*transcolorP)*newMaxval) / maxval) << 8)
             | ((PPM_GETB(*transcolorP)*newMaxval) / maxval));
        Color const found = 
            (bsearch(&temp_color,
                     colormap->color_entries, colormap->ncolors,
                     sizeof(Color_s), palmcolor_compare_colors));
        if (!found) {
            pm_error("Specified transparent color %s not found "
                     "in colormap.", colorSpec);
        } else
            *transindexP = (*found >> 24) & 0xFF;
    }
}



static unsigned int
bitmapVersion(unsigned int         const bpp,
              bool                 const colormap,
              bool                 const transparent,
              enum compressionType const compression,
              unsigned int         const density) {
/*----------------------------------------------------------------------------
   Return the version number of the oldest version that can represent
   the specified attributes.
-----------------------------------------------------------------------------*/
    unsigned int version;
    /* we need Version 1 if we use more than 1 bpp,
       Version 2 if we use compression or transparency,
       Version 3 if density is 108 or higher
    */
    if (density > PALM_DENSITY_LOW)
        version = 3;
    else if (transparent || compression != COMP_NONE)
        version = 2;
    else if (bpp > 1 || colormap)
        version = 1;
    else
        version = 0;

    return version;
}



static void
writeCommonHeader(unsigned int         const cols,
                  unsigned int         const rows,
                  unsigned int         const rowbytes,
                  enum compressionType const compression,
                  bool                 const colormap,
                  bool                 const transparent,
                  bool                 const directColor,
                  unsigned int         const bpp,
                  unsigned int         const version) {
/*----------------------------------------------------------------------------
   Write the first 10 bytes of the Palm Bitmap header.
   These are common to all encodings (versions 0, 1, 2 and 3).
-----------------------------------------------------------------------------*/
    unsigned short flags;

    if (cols > USHRT_MAX)
        pm_error("Too many columns for Palm Bitmap: %u", cols);
    pm_writebigshort(stdout, cols);    /* width */
    if (rows > USHRT_MAX)
        pm_error("Too many rows for Palm Bitmap: %u", rows);
    pm_writebigshort(stdout, rows);    /* height */
    if (rowbytes > USHRT_MAX)
        pm_error("Too many bytes per row for Palm Bitmap: %u", rowbytes);
    pm_writebigshort(stdout, rowbytes);

    flags = 0;  /* initial value */
    if (compression != COMP_NONE)
        flags |= PALM_IS_COMPRESSED_FLAG;
    if (colormap)
        flags |= PALM_HAS_COLORMAP_FLAG;
    if (transparent)
        flags |= PALM_HAS_TRANSPARENCY_FLAG;
    if (directColor)
        flags |= PALM_DIRECT_COLOR_FLAG;
    pm_writebigshort(stdout, flags);
    assert(bpp <= UCHAR_MAX);
    fputc(bpp, stdout);

    fputc(version, stdout);
}



static unsigned char 
compressionFieldValue(enum compressionType const compression) {

    unsigned char retval;

    switch (compression) {
    case COMP_SCANLINE:
        retval = PALM_COMPRESSION_SCANLINE;
        break;
    case COMP_RLE:
        retval = PALM_COMPRESSION_RLE;
        break;
    case COMP_PACKBITS:
        retval = PALM_COMPRESSION_PACKBITS;
        break;
    case COMP_NONE:
        retval = 0x00;  /* empty */
        break;
    }
    return retval;
}



static void
writeRemainingHeaderLow(unsigned int         const nextDepthOffset,
                        unsigned int         const transindex,
                        enum compressionType const compression,
                        unsigned int         const bpp) {
/*----------------------------------------------------------------------------
   Write last 6 bytes of a low density Palm Bitmap header. 
-----------------------------------------------------------------------------*/
    if (nextDepthOffset > USHRT_MAX)
        pm_error("Image too large for Palm Bitmap");

    pm_writebigshort(stdout, nextDepthOffset);

    if (bpp != 16) {
        assert(transindex <= UCHAR_MAX);
        fputc(transindex, stdout);    /* transparent index */
    } else
        fputc(0, stdout);    /* the DirectInfoType will hold this info */
    
    fputc(compressionFieldValue(compression), stdout);

    pm_writebigshort(stdout, 0);  /* reserved by Palm */
}



static void
writeRemainingHeaderHigh(unsigned int         const bpp,
                         enum compressionType const compression,
                         unsigned int         const density,
                         xelval               const maxval,
                         bool                 const transparent,
                         xel                  const transcolor,
                         unsigned int         const transindex,
                         unsigned int         const nextBitmapOffset) {
/*----------------------------------------------------------------------------
   Write last 16 bytes of a high density Palm Bitmap header. 
-----------------------------------------------------------------------------*/
    if ((nextBitmapOffset >> 31) > 1)
        pm_error("Image too large for Palm Bitmap.  nextBitmapOffset "
            "value doesn't fit in 4 bytes");

    fputc(0x18, stdout); /* size of this high density header */

    if (bpp != 16)
        fputc(PALM_FORMAT_INDEXED, stdout);
    else
        fputc(PALM_FORMAT_565, stdout);

    fputc(0x00, stdout); /* unused */

    fputc(compressionFieldValue(compression), stdout);

    pm_writebigshort(stdout, density);

    if (transparent) {
        if (bpp == 16) {
            /* Blind guess here */
            fputc(0, stdout);
            fputc((PPM_GETR(transcolor) * 255) / maxval, stdout);
            fputc((PPM_GETG(transcolor) * 255) / maxval, stdout);
            fputc((PPM_GETB(transcolor) * 255) / maxval, stdout);
        } else {
            assert(transindex <= UCHAR_MAX);
            fputc(0, stdout);
            fputc(0, stdout);
            fputc(0, stdout);
            fputc(transindex, stdout);   /* transparent index */
        }
    } else
        pm_writebiglong(stdout, 0);

    pm_writebiglong(stdout, nextBitmapOffset);
}



static void
writeDummy() {
/*----------------------------------------------------------------------------
   Write a dummy Palm Bitmap header.  This is a 16 byte header, of
   type version 1 and with (only) pixelSize set to 0xFF.

   An old viewer will see this as invalid due to the pixelSize, and stop
   reading the stream.  A new viewer will recognize this for what it is
   (a dummy header designed to stop old viewers from reading further in
   the stream) and continue reading the stream.  Presumably, what follows
   in the stream is understandable by a new viewer, but would confuse an
   old one.
-----------------------------------------------------------------------------*/
    pm_writebiglong(stdout, 0x00);
    pm_writebiglong(stdout, 0x00);
    fputc(0xFF, stdout);               /* pixelSize */
    fputc(0x01, stdout);               /* version */
    pm_writebigshort(stdout, 0x00); 
    pm_writebiglong(stdout, 0x00);
}



static void
writeColormap(bool         const explicitColormap,
              Colormap     const colormap,
              bool         const directColor,
              unsigned int const bpp,
              bool         const transparent,
              xel          const transcolor,
              xelval       const maxval,
              unsigned int const version) {
              
    /* if there's a colormap, write it out */
    if (explicitColormap) {
        unsigned int row;
        if (!colormap)
            pm_error("Internal error: user specified -colormap, but we did "
                     "not generate a colormap.");
        qsort (colormap->color_entries, colormap->ncolors,
               sizeof(Color_s), palmcolor_compare_indices);
        pm_writebigshort( stdout, colormap->ncolors );
        for (row = 0;  row < colormap->ncolors; ++row)
            pm_writebiglong (stdout, colormap->color_entries[row]);
    }

    if (directColor && (version < 3)) {
        /* write the DirectInfoType (8 bytes) */
        if (bpp == 16) {
            fputc(5, stdout);   /* # of bits of red */
            fputc(6, stdout);   /* # of bits of green */    
            fputc(5, stdout);   /* # of bits of blue */
            fputc(0, stdout);   /* reserved by Palm */
        } else
            pm_error("Don't know how to create %d bit DirectColor bitmaps.", 
                     bpp);
        if (transparent) {
            fputc(0, stdout);
            fputc((PPM_GETR(transcolor) * 255) / maxval, stdout);
            fputc((PPM_GETG(transcolor) * 255) / maxval, stdout);
            fputc((PPM_GETB(transcolor) * 255) / maxval, stdout);
        } else
            pm_writebiglong(stdout, 0);     /* no transparent color */
    }
}



static void
computeRawRowDirectColor(const xel *     const xelrow,
                         unsigned int    const cols,
                         xelval          const maxval,
                         unsigned char * const rowdata) {

    unsigned int col;
    unsigned char *outptr;
    
    for (col = 0, outptr = rowdata; col < cols; ++col) {
        unsigned int const color = 
            ((((PPM_GETR(xelrow[col])*31)/maxval) << 11) |
             (((PPM_GETG(xelrow[col])*63)/maxval) << 5) |
             ((PPM_GETB(xelrow[col])*31)/maxval));
        *outptr++ = (color >> 8) & 0xFF;
        *outptr++ = color & 0xFF;
    }
}



static void
computeRawRowNonDirect(const xel *     const xelrow,
                       unsigned int    const cols,
                       xelval          const maxval,
                       unsigned int    const bpp,
                       Colormap        const colormap,
                       unsigned int    const newMaxval,
                       unsigned char * const rowdata) {

    unsigned int col;
    unsigned char *outptr;
    unsigned char outbyte;
        /* Accumulated bits to be output */
    unsigned char outbit;
        /* The lowest bit number we want to access for this pixel */

    outbyte = 0x00;
    outptr = rowdata;

    for (outbit = 8 - bpp, col = 0; col < cols; ++col) {
        unsigned int color;
        if (!colormap) {
            /* we assume grayscale, and use simple scaling */
            color = (PNM_GET1(xelrow[col]) * newMaxval)/maxval;
            if (color > newMaxval)
                pm_error("oops.  Bug in color re-calculation code.  "
                         "color of %u.", color);
            color = newMaxval - color; /* note grayscale maps are inverted */
        } else {
            Color_s const temp_color =
                ((((PPM_GETR(xelrow[col])*newMaxval)/maxval)<<16) 
                 | (((PPM_GETG(xelrow[col])*newMaxval)/maxval)<<8)
                 | (((PPM_GETB(xelrow[col])*newMaxval)/maxval)));
            Color const found = (bsearch (&temp_color,
                                          colormap->color_entries, 
                                          colormap->ncolors,
                                          sizeof(Color_s), 
                                          palmcolor_compare_colors));
            if (!found) {
                pm_error("Color %d:%d:%d not found in colormap.  "
                         "Try using pnmquant to reduce the "
                         "number of colors.",
                         PPM_GETR(xelrow[col]), 
                         PPM_GETG(xelrow[col]), 
                         PPM_GETB(xelrow[col]));
            }
            color = (*found >> 24) & 0xFF;
        }

        if (color > newMaxval)
            pm_error("oops.  Bug in color re-calculation code.  "
                     "color of %u.", color);
        outbyte |= (color << outbit);
        if (outbit == 0) {
            /* Bit buffer is full.  Flush to to rowdata. */
            *outptr++ = outbyte;
            outbyte = 0x00;
            outbit = 8 - bpp;
        } else
            outbit -= bpp;
    }
    if ((cols % (8 / bpp)) != 0) {
        /* Flush bits remaining in the bit buffer to rowdata */
        *outptr++ = outbyte;
    }
}


struct seqBuffer {
/*----------------------------------------------------------------------------
   A buffer to which one can write bytes sequentially.
-----------------------------------------------------------------------------*/
    char * buffer;
    unsigned int allocatedSize;
    unsigned int occupiedSize;
};


static void
createBuffer(struct seqBuffer ** const bufferPP) {

    struct seqBuffer * bufferP;

    MALLOCVAR_NOFAIL(bufferP);

    bufferP->allocatedSize = 4096;
    MALLOCARRAY(bufferP->buffer, bufferP->allocatedSize);
    if (bufferP == NULL)
        pm_error("Unable to allocate %u bytes of buffer", 
                 bufferP->allocatedSize);
    bufferP->occupiedSize = 0;

    *bufferPP = bufferP;
}



static void
destroyBuffer(struct seqBuffer * const bufferP) {

    free(bufferP->buffer);
    free(bufferP);
}



static void
addByteToBuffer(struct seqBuffer * const bufferP,
                unsigned char      const newByte) {

    assert(bufferP->allocatedSize >= bufferP->occupiedSize);

    if (bufferP->allocatedSize == bufferP->occupiedSize) {
        bufferP->allocatedSize *= 2;
        REALLOCARRAY(bufferP->buffer, bufferP->allocatedSize);
        if (bufferP->buffer == NULL)
            pm_error("Couldn't (re)allocate %u bytes of memory "
                     "for buffer.", bufferP->allocatedSize);
    }
    bufferP->buffer[bufferP->occupiedSize++] = newByte;
}



static unsigned int
bufferLength(struct seqBuffer * const bufferP) {
    return bufferP->occupiedSize;
}



static void
writeOutBuffer(struct seqBuffer * const bufferP,
               FILE *             const fileP) {

    size_t bytesWritten;

    bytesWritten = fwrite(bufferP->buffer, sizeof(char), 
                          bufferP->occupiedSize, fileP);

    if (bytesWritten != bufferP->occupiedSize)
        pm_error("fwrite() failed to write out the buffer.");
}



static void
copyRowToBuffer(const unsigned char * const rowdata,
                unsigned int          const rowbytes,
                struct seqBuffer *    const rasterBufferP) {

    unsigned int pos;
    for (pos = 0; pos < rowbytes; ++pos)
        addByteToBuffer(rasterBufferP, rowdata[pos]);
} 



static void
scanlineCompressAndBufferRow(const unsigned char * const rowdata,
                             unsigned int          const rowbytes,
                             struct seqBuffer *    const rasterBufferP,
                             const unsigned char * const lastrow) {
/*----------------------------------------------------------------------------
   Take the raw Palm Bitmap row 'rowdata', which is 'rowbytes'
   columns, and add the scanline-compressed representation of it to
   the buffer with handle 'rasterBufferP'.

   'lastrow' is the raw contents of the row immediately before the one
   we're compressing -- i.e. we compress with respect to that row.  This
   function does not work on the first row of an image.
-----------------------------------------------------------------------------*/
    unsigned int pos;

    for (pos = 0;  pos < rowbytes;  pos += 8) {
        unsigned int const limit = MIN(rowbytes - pos, 8);

        unsigned char map;
            /* mask indicating which of the next 8 pixels are
               different from the previous row, and therefore present
               in the file immediately following the map byte.  
            */
        unsigned char differentPixels[8];
        unsigned char *outptr;
        unsigned char outbit;
            
        for (outbit = 0, map = 0x00, outptr = differentPixels;
             outbit < limit;  
             ++outbit) {
            if (!lastrow 
                || (lastrow[pos + outbit] != rowdata[pos + outbit])) {
                map |= (1 << (7 - outbit));
                *outptr++ = rowdata[pos + outbit];
            }
        }

        addByteToBuffer(rasterBufferP, map);
        {
            unsigned int j;
            for (j = 0; j < (outptr - differentPixels); ++j)
                addByteToBuffer(rasterBufferP, differentPixels[j]);
        }
    }
}



static void
rleCompressAndBufferRow(const unsigned char * const rowdata,
                        unsigned int          const rowbytes,
                        struct seqBuffer *    const rasterBufferP) {
/*----------------------------------------------------------------------------
   Take the raw Palm Bitmap row 'rowdata', which is 'rowbytes' bytes,
   and add the rle-compressed representation of it to the buffer with
   handle 'rasterBufferP'.
-----------------------------------------------------------------------------*/
    unsigned int pos;

    /* we output a count of the number of bytes a value is
       repeated, followed by that byte value 
    */
    pos = 0;
    while (pos < rowbytes) {
        unsigned int repeatcount;
        for (repeatcount = 1;  
             repeatcount < (rowbytes - pos) && repeatcount  < 255;  
             ++repeatcount)
            if (rowdata[pos + repeatcount] != rowdata[pos])
                break;

        addByteToBuffer(rasterBufferP, repeatcount);
        addByteToBuffer(rasterBufferP, rowdata[pos]);
        pos += repeatcount;
    }
}



static void
computeNextPackbitsRun(const unsigned char * const rowdata,
                       unsigned int          const rowbytes,
                       unsigned int          const startPos,
                       unsigned int *        const nextPosP,
                       unsigned char *       const output,
                       int *                 const countP) {

    unsigned int pos;
    int count;
    
    pos = startPos;
    count = 0;
    
    if (rowdata[pos] == rowdata[pos + 1]) {
        ++pos;
        --count;
        while ((count > -127) && (pos < (rowbytes - 1)) &&
               (rowdata[pos] == rowdata[pos + 1])) {
            ++pos;
            --count;
        }
        ++pos;  /* push pos past end of this run */
    } else {
        output[count] = rowdata[pos];
        ++pos;
        while ((count < 127) && (pos < (rowbytes - 1)) && 
               (rowdata[pos] != rowdata[pos + 1])) {
            ++count;
            output[count] = rowdata[pos];
            ++pos;
        }
        /* trailing literal */
        if ((count < 127) && (pos == (rowbytes - 1)) &&
            (rowdata[pos - 1] != rowdata[pos])) {
            ++count;
            output[count] = rowdata[pos];
            ++pos;
        }
    }
    *nextPosP = pos;
    *countP = count;
}



static void
addPackbitsRunToBuffer(const unsigned char * const rowdata,
                       unsigned int          const rowbytes,
                       unsigned int          const pos,
                       unsigned char *       const output,
                       int                   const count,
                       struct seqBuffer *    const rasterBufferP) {

    addByteToBuffer(rasterBufferP, (unsigned char)(signed char)count);
    if (count < 0) {
        addByteToBuffer(rasterBufferP, rowdata[pos - 1]);
    } else {
        unsigned int j;
        for (j = 0; j <= count; j++)
            addByteToBuffer(rasterBufferP, output[j]);
    }
    
    if (pos == (rowbytes - 1) && (rowdata[pos - 1] != rowdata[pos])) {
        /* orphaned byte, treat as literal */
        addByteToBuffer(rasterBufferP, 0);
        addByteToBuffer(rasterBufferP, rowdata[pos]);
    }
}



static void
packbitsCompressAndBufferRow(const unsigned char * const rowdata,
                             unsigned int          const rowbytes,
                             struct seqBuffer *    const rasterBufferP) {
/*----------------------------------------------------------------------------
   Take the raw Palm Bitmap row 'rowdata', which is 'rowbytess' bytes, and
   add the packbits-compressed representation of it to the buffer 
   with handle 'rasterBufferP'.
-----------------------------------------------------------------------------*/
    unsigned int position;
        /* byte position within the row */

    position = 0;  /* Start at beginning of row */

    while (position < rowbytes - 1) {
        unsigned char output[128];
        int count;

        computeNextPackbitsRun(rowdata, rowbytes, position, 
                               &position, output, &count);

        addPackbitsRunToBuffer(rowdata, rowbytes, position, output, count,
                               rasterBufferP);
    }
}



static void
bufferRowFromRawRowdata(const unsigned char *  const rowdata,
                        unsigned int           const rowbytes,
                        enum compressionType   const compression,
                        const unsigned char *  const lastrow,
                        struct seqBuffer *     const rasterBufferP) {
/*----------------------------------------------------------------------------
   Starting with a raw (uncompressed) Palm raster line, do the
   compression identified by 'compression' and add the compressed row
   to the buffer with handle 'rasterBufferP'.

   If 'compression' indicates scanline compression, 'lastrow' is the
   row immediately preceding this one in the image (and this function
   doesn't work on the first row of an image).  Otherwise, 'lastrow'
   is meaningless.
-----------------------------------------------------------------------------*/
    switch (compression) {
    case COMP_NONE:
        copyRowToBuffer(rowdata, rowbytes, rasterBufferP);
        break;
    case COMP_SCANLINE:
        scanlineCompressAndBufferRow(rowdata, rowbytes, rasterBufferP, 
                                     lastrow);
        break;
    case COMP_RLE:
        rleCompressAndBufferRow(rowdata, rowbytes, rasterBufferP);
        break;
    case COMP_PACKBITS:
        packbitsCompressAndBufferRow(rowdata, rowbytes, rasterBufferP);
        break;
    }
}



static void
bufferRow(const xel *          const xelrow,
          unsigned int         const cols,
          xelval               const maxval,
          unsigned int         const rowbytes,
          unsigned int         const bpp,
          unsigned int         const newMaxval,
          enum compressionType const compression,
          bool                 const directColor,
          Colormap             const colormap,
          unsigned char *      const rowdata,
          unsigned char *      const lastrow,
          struct seqBuffer *   const rasterBufferP) {
/*----------------------------------------------------------------------------
   Add a row of the Palm Bitmap raster to buffer 'rasterBufferP'.
   
   'xelrow' is the image contents of row.  It is 'cols' columns wide.

   If 'compression' indicates scanline compression, 'lastrow' is the
   row immediately preceding this one in the image (and this function
   doesn't work on the first row of an image).  Otherwise, 'lastrow'
   is meaningless.

   'rowdata' is a work buffer 'rowbytes' in size.
-----------------------------------------------------------------------------*/
    if (directColor)
        computeRawRowDirectColor(xelrow, cols, maxval, rowdata);
    else 
        computeRawRowNonDirect(xelrow, cols, maxval, bpp, colormap, newMaxval,
                               rowdata);

    bufferRowFromRawRowdata(rowdata, rowbytes, compression,
                            lastrow, rasterBufferP);
}



static void 
bufferRaster(xel **               const xels,
             unsigned int         const cols,
             unsigned int         const rows,
             xelval               const maxval,
             unsigned int         const rowbytes,
             unsigned int         const bpp,
             unsigned int         const newMaxval,
             enum compressionType const compression,
             bool                 const directColor,
             Colormap             const colormap,
             struct seqBuffer **  const rasterBufferPP) {
    
    unsigned char * rowdata;
    unsigned char * lastrow;
    unsigned int row;

    createBuffer(rasterBufferPP);
    
    MALLOCARRAY_NOFAIL(rowdata, rowbytes);
    MALLOCARRAY_NOFAIL(lastrow, rowbytes);

    /* And write out the data. */
    for (row = 0; row < rows; ++row) {
        bufferRow(xels[row], cols, maxval, rowbytes, bpp, newMaxval,
                  compression,
                  directColor, colormap, rowdata, row > 0 ? lastrow : NULL,
                  *rasterBufferPP);

        if (compression == COMP_SCANLINE)
            memcpy(lastrow, rowdata, rowbytes);
    }
    free(lastrow);
    free(rowdata);
}



static void
computeOffsetStuff(bool                 const offsetWanted,
                   unsigned int         const version,
                   bool                 const directColor,
                   enum compressionType const compression,
                   bool                 const colormap,
                   unsigned int         const colormapColorCount,
                   unsigned int         const sizePlusRasterSize,
                   unsigned int *       const nextDepthOffsetP,
                   unsigned int *       const nextBitmapOffsetP,
                   unsigned int *       const padBytesRequiredP) {
    
    if (offsetWanted) {
        /* Offset is measured in 4-byte words (double words in
           Intel/Microsoft terminology).  Account for header,
           colormap, and raster size and round up 
        */
        unsigned int const headerSize = ((version < 3) ? 16 : 24);
        unsigned int const colormapSize =
            (colormap ? (2 + colormapColorCount * 4) : 0);
        if (version < 3) {
            unsigned int const directSize = 
                (directColor && version < 3) ? 8 : 0; 
            if (compression != COMP_NONE && sizePlusRasterSize > USHRT_MAX)
                pm_error("Oversized compressed bitmap: %u bytes",
                         sizePlusRasterSize);
            *padBytesRequiredP = 4 - (sizePlusRasterSize + headerSize + 
                                      directSize + colormapSize) % 4;
            *nextDepthOffsetP = 
                (sizePlusRasterSize + headerSize + 
                 directSize + colormapSize + *padBytesRequiredP) / 4;
        } else {
            if (compression != COMP_NONE && (sizePlusRasterSize >> 31) > 1)
                pm_error("Oversized compressed bitmap: %u bytes",
                         sizePlusRasterSize);
            /* Does version 3 need padding? Probably won't hurt */
            *padBytesRequiredP = 4 - (sizePlusRasterSize + headerSize +
                                      colormapSize) % 4;
            *nextBitmapOffsetP = sizePlusRasterSize + headerSize + 
                colormapSize + *padBytesRequiredP;
        }
    } else {
        *padBytesRequiredP = 0;
        *nextDepthOffsetP = 0;
        *nextBitmapOffsetP = 0;
    }
}



static void
writeRasterSize(unsigned int const sizePlusRasterSize,
                unsigned int const version,
                FILE *       const fileP) {
/*----------------------------------------------------------------------------
   Write to file 'fileP' a raster size field for a Palm Bitmap version
   'version' header, indicating 'sizePlusRasterSize' bytes.
-----------------------------------------------------------------------------*/
    if (version < 3) 
        pm_writebigshort(fileP, sizePlusRasterSize);
    else
        pm_writebiglong(fileP, sizePlusRasterSize);
}



static void
writeBitmap(xel **               const xels,
            unsigned int         const cols,
            unsigned int         const rows,
            xelval               const maxval,
            unsigned int         const rowbytes,
            unsigned int         const bpp,
            unsigned int         const newMaxval,
            enum compressionType const compression,
            bool                 const transparent,
            bool                 const directColor,
            bool                 const offsetWanted,
            bool                 const hasColormap,
            Colormap             const colormap,
            unsigned int         const transindex,
            xel                  const transcolor,
            unsigned int         const version,
            unsigned int         const density,
            bool                 const withdummy) {
    
    unsigned int sizePlusRasterSize;
    unsigned int nextDepthOffset;
    unsigned int nextBitmapOffset;
        /* Offset from the beginning of the image we write to the beginning
           of the next one, assuming user writes another one following this
           one.
           nextDepthOffset is used in encodings 1, 2 and is in 4 byte words 
           nextBitmapOffset is used in encoding 3, is in 4 bytes 
        */
    unsigned int padBytesRequired;
        /* Number of bytes of padding we need to put after the image in
           order to align properly for User to add the next image to the
           stream.
        */
    struct seqBuffer * rasterBufferP;

    writeCommonHeader(cols, rows, rowbytes, compression, hasColormap, 
                      transparent, directColor, bpp, version);
    
    bufferRaster(xels, cols, rows, maxval, rowbytes, bpp, newMaxval,
                 compression, directColor, colormap, &rasterBufferP);

    /* rasterSize itself takes 2 or 4 bytes */
    if (version < 3)
        sizePlusRasterSize = 2 + bufferLength(rasterBufferP);
    else
        sizePlusRasterSize = 4 + bufferLength(rasterBufferP);
    
    computeOffsetStuff(offsetWanted, version, directColor, compression,
                       hasColormap, hasColormap ? colormap->ncolors : 0, 
                       sizePlusRasterSize,
                       &nextDepthOffset, &nextBitmapOffset,
                       &padBytesRequired);

    if (version < 3)
        writeRemainingHeaderLow(nextDepthOffset, transindex, compression, bpp);
    else
        writeRemainingHeaderHigh(bpp, compression, density,
                                 maxval, transparent, transcolor,
                                 transindex, nextBitmapOffset);

    writeColormap(hasColormap, colormap, directColor, bpp, 
                  transparent, transcolor, maxval, version);

    if (compression != COMP_NONE)
        writeRasterSize(sizePlusRasterSize, version, stdout);

    writeOutBuffer(rasterBufferP, stdout);

    destroyBuffer(rasterBufferP);

    {
        unsigned int i;
        for (i = 0; i < padBytesRequired; ++i)
            fputc(0x00, stdout);
    }

    if (withdummy)
        writeDummy();
}        



int 
main( int argc, char **argv ) {
    struct cmdline_info cmdline;
    unsigned int version;
    FILE* ifP;
    xel** xels;
    xel transcolor;
    unsigned int transindex;
    int rows, cols;
    unsigned int rowbytes;
    xelval maxval;
    int format;
    unsigned int bpp;
    bool directColor;
    unsigned int newMaxval;
    Colormap colormap;
    
    /* Parse default params */
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    xels = pnm_readpnm(ifP, &cols, &rows, &maxval, &format);
    pm_close(ifP);

    if (cmdline.verbose)
        pm_message("Input is %dx%d %s, maxval %d", 
                   cols, rows, formatName(format), maxval);
    
    determinePalmFormat(cols, rows, maxval, format, xels, cmdline.depth,
                        cmdline.maxdepth, cmdline.colormap, cmdline.verbose,
                        &bpp, &directColor, &colormap);

    newMaxval = (1 << bpp) - 1;

    if (cmdline.transparent) 
        findTransparentColor(cmdline.transparent, newMaxval, directColor,
                             maxval, colormap, &transcolor, &transindex);
    else 
        transindex = 0;

    rowbytes = ((cols + (16 / bpp -1)) / (16 / bpp)) * 2;    
        /* bytes per row - always a word boundary */

    version = bitmapVersion(bpp, cmdline.colormap, !!cmdline.transparent, 
                            cmdline.compression, cmdline.density);

    writeBitmap(xels, cols, rows, maxval,
                rowbytes, bpp, newMaxval, cmdline.compression,
                !!cmdline.transparent, directColor, cmdline.offset, 
                cmdline.colormap, colormap, transindex, transcolor,
                version, cmdline.density, cmdline.withdummy);
    
    return 0;
}
