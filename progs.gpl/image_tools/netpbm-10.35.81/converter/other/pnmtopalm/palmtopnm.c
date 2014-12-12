/******************************************************************************
                             palmtopnm
*******************************************************************************
  By Bryan Henderson, San Jose, California, June 2004.

  Inspired by and using methods from Tbmptopnm by Ian Goldberg
  <iang@cs.berkeley.edu>, and Bill Janssen <bill@janssen.org>.

  Major fixes and new capability added by Paul Bolle <pebolle@tiscali.nl>
  in late 2004 / early 2005.

  Bryan's work is contributed to the public domain by its author.
******************************************************************************/

#include <string.h>
#include <assert.h>

#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"

#include "palm.h"



enum palmCompressionType {
    COMPRESSION_NONE, 
    COMPRESSION_RLE, 
    COMPRESSION_SCANLINE,
    COMPRESSION_PACKBITS
};

struct palmHeader {
    unsigned short cols;
    unsigned short rows;
    unsigned short bytesPerRow;
    unsigned short flags;
    bool           directColor;
        /* The header indicates a direct color raster, either by flag
           (the old way) or by pixel format (the new way)
        */
    bool           hasColormap;
    bool           hasTransparency;
    unsigned char  pixelSizeCode;
    unsigned int   pixelSize;
    unsigned char  version;
    unsigned int   transparentIndex;
    enum palmCompressionType compressionType;
    /* version 3 encoding specific */
    unsigned char  size;
    unsigned char  pixelFormat;
    unsigned short density;
    unsigned long  transparentValue;
};



struct directPixelFormat {
    unsigned int redbits;
    unsigned int greenbits;
    unsigned int bluebits;
};



struct directColorInfo {
    struct directPixelFormat pixelFormat;
    Color_s                  transparentColor;
};




struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilespec;
    unsigned int verbose;
    unsigned int rendition;
    unsigned int showhist;
    unsigned int transparent;
};


static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc( 100*sizeof( optEntry ) );
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int renditionSpec;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "verbose",     OPT_FLAG, NULL,
            &cmdlineP->verbose,  0);
    OPTENT3(0, "showhist",    OPT_FLAG, NULL,
            &cmdlineP->showhist, 0);
    OPTENT3(0, "transparent",    OPT_FLAG, NULL,
            &cmdlineP->transparent, 0);
    OPTENT3(0, "rendition",  OPT_UINT, &cmdlineP->rendition, 
            &renditionSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (renditionSpec) {
        if (cmdlineP->rendition < 1)
            pm_error("The -rendition value must be at least 1");
    } else 
        cmdlineP->rendition = 1;
    
    if (cmdlineP->transparent && cmdlineP->showhist)
        pm_error("You can't specify -showhist with -transparent");

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else {
        cmdlineP->inputFilespec = argv[1];
        if (argc-1 > 1)
            pm_error("Too many arguments (%d).  The only non-option "
                     "argument is the file name", argc-1);
    }
}



static xelval *
createGraymap(unsigned int const ncolors, 
              xelval       const maxval) {
    int i;
    xelval *map;

    MALLOCARRAY_NOFAIL(map, ncolors);
    for (i = 0; i < ncolors; ++i) {
        map[i] = maxval - (i * maxval) / (ncolors - 1);
    }
    return map;
}



static void 
skipbytes(FILE *       const ifP, 
          unsigned int const nbytes) {

    unsigned char buf[256];
    unsigned int n;
    size_t bytesRead;

    n = nbytes;  /* initial value */

    while (n > 0) {
        if (n > sizeof(buf)) {
            bytesRead = fread(buf, sizeof(char), sizeof(buf), ifP);
            if (bytesRead != sizeof(buf))
               pm_error("Error reading Palm file.  Short read.");
            n -= sizeof(buf);
        } else {
            bytesRead = fread(buf, sizeof(char), n, ifP);
            if (bytesRead != n)
               pm_error("Error reading Palm file.  Short read.");
            n = 0;
        }
    }    
}



static void
interpretCompression(unsigned char              const compressionValue,
                     enum palmCompressionType * const compressionTypeP) {
    
    switch (compressionValue) {
    case PALM_COMPRESSION_RLE:
        *compressionTypeP = COMPRESSION_RLE;
        break;
    case PALM_COMPRESSION_SCANLINE:
        *compressionTypeP = COMPRESSION_SCANLINE;
        break;
    case PALM_COMPRESSION_PACKBITS:
        *compressionTypeP = COMPRESSION_PACKBITS;
        break;
    case PALM_COMPRESSION_NONE:
        /* according to the spec this is not possible */
        *compressionTypeP = COMPRESSION_NONE;
        break;
    default:
        pm_error("The Palm image header has an unrecognized value for "
                 "compression type: 0x%02x", (unsigned)compressionValue);
    }
}



static void
readRestOfHeaderVersion3(FILE *           const ifP,
                         unsigned int     const pixelSize,
                         unsigned char *  const sizeP,
                         unsigned char *  const pixelFormatP,
                         unsigned char *  const compressionTypeP,
                         short *          const densityP,
                         unsigned int *   const transparentIndexP,
                         long *           const transparentValueP,
                         long *           const nextBitmapOffsetP,
                         short *          const nextDepthOffsetP) {

    unsigned char unused;
    
    pm_readcharu(ifP, sizeP);
    /* should be 0x18, but I can't see why we should really care */
    if (*sizeP != 0x18)
        pm_message("Strange value for Palm bitmap header size: %hu", *sizeP);

    pm_readcharu(ifP, pixelFormatP);
    if (*pixelFormatP != PALM_FORMAT_INDEXED &&
        *pixelFormatP != PALM_FORMAT_565)
        pm_error("Unrecognized pixelformat type: %u", *pixelFormatP);
    
    pm_readcharu(ifP, &unused);


    pm_readcharu(ifP, compressionTypeP);

    pm_readbigshort(ifP, densityP);
    /* the specs imply that 0x00 is not valid */
    if (*densityP != PALM_DENSITY_LOW &&
        *densityP != PALM_DENSITY_ONEANDAHALF &&
        *densityP != PALM_DENSITY_DOUBLE &&
        *densityP != PALM_DENSITY_TRIPLE &&
        *densityP != PALM_DENSITY_QUADRUPLE)
        pm_error("Invalid value for -density: %d.", *densityP);
 
    pm_readbiglong(ifP, transparentValueP);
    if (pixelSize < 16)
        *transparentIndexP = *transparentValueP;
    else
        *transparentIndexP = 0;
 
    pm_readbiglong(ifP, nextBitmapOffsetP);
    
    /* version < 3 specific */
    *nextDepthOffsetP = 0;
}



static void
readRestOfHeaderOld(FILE *           const ifP,
                    unsigned char *  const sizeP,
                    unsigned char *  const pixelFormatP,
                    unsigned char *  const compressionTypeP,
                    short *          const densityP,
                    unsigned int *   const transparentIndexP,
                    long *           const transparentValueP,
                    long *           const nextBitmapOffsetP,
                    short *          const nextDepthOffsetP) {
    
    short pad;
    unsigned char transparentIndex;
    
    pm_readbigshort(ifP, nextDepthOffsetP);
    pm_readcharu(ifP, &transparentIndex);
    *transparentIndexP = transparentIndex;
 
    pm_readcharu(ifP,compressionTypeP);
    
    pm_readbigshort(ifP, &pad); /* reserved by Palm as of 8/9/00 */
    
    /* version 3 specific */
    *sizeP = 0;
    *pixelFormatP = 0;
    *densityP = 0;
    *transparentValueP = 0;
    *nextBitmapOffsetP = 0;
}



static void
interpretHeader(struct palmHeader * const palmHeaderP,
                short               const cols,
                short               const rows,
                short               const bytesPerRow,
                short               const flags,
                unsigned char       const pixelSizeCode,
                unsigned int        const pixelSize,
                unsigned char       const version,
                unsigned char       const size,
                unsigned char       const pixelFormat,
                short               const density,
                long                const transparentValue,
                unsigned int        const transparentIndex,
                unsigned char       const compressionType) {
    
    palmHeaderP->cols = cols;
    palmHeaderP->rows = rows;
    palmHeaderP->bytesPerRow = bytesPerRow;
    palmHeaderP->flags = flags;  /* Just for diagnostics */
    palmHeaderP->hasColormap = !!(flags & PALM_HAS_COLORMAP_FLAG);
    palmHeaderP->hasTransparency = !!(flags & PALM_HAS_TRANSPARENCY_FLAG);
    palmHeaderP->pixelSizeCode = pixelSizeCode;
    palmHeaderP->pixelSize = pixelSize;
    palmHeaderP->version = version;
    palmHeaderP->size = size;
    palmHeaderP->pixelFormat = pixelFormat;
    palmHeaderP->density = density;
    palmHeaderP->transparentValue = transparentValue;
    palmHeaderP->transparentIndex = transparentIndex;

    if (palmHeaderP->version == 3 && ((flags & PALM_DIRECT_COLOR_FLAG) &&
                                      (pixelFormat != PALM_FORMAT_565)))
        /* There's no directColorInfoType section in a version 3 Palm Bitmap
           so we also need PALM_FORMAT_565 for this flag to make sense
        */
        pm_error("PALM_DIRECT_COLOR_FLAG is set but pixelFormat is not"
                 "PALM_FORMAT_565.");
        
    palmHeaderP->directColor = ((flags & PALM_DIRECT_COLOR_FLAG) || 
                                palmHeaderP->pixelFormat == PALM_FORMAT_565);
    
    if (flags & PALM_IS_COMPRESSED_FLAG) 
        interpretCompression(compressionType,
                             &palmHeaderP->compressionType);
    else
        palmHeaderP->compressionType = COMPRESSION_NONE;
}



static void
readHeader(FILE *              const ifP,
           unsigned int        const requestedRendition,
           struct palmHeader * const palmHeaderP) {
/*----------------------------------------------------------------------------
   Read the Palm Bitmap header from the file 'ifP'.  Read past all
   renditions up to 'requestedRendition' and read the header of that
   rendition.  Return the information contained in the header as *palmHeaderP.
-----------------------------------------------------------------------------*/
    bool gotHeader;
    unsigned int currentRendition;

    gotHeader = FALSE;
    currentRendition = 1;
    while (!gotHeader) {
        short cols, rows, bytesPerRow, flags, nextDepthOffset, density;
        unsigned char pixelSizeCode, version, compressionType, 
            size, pixelFormat;
        long transparentValue, nextBitmapOffset;
        unsigned int pixelSize, transparentIndex;

        pm_readbigshort(ifP, &cols);
        pm_readbigshort(ifP, &rows);
        pm_readbigshort(ifP, &bytesPerRow);
        pm_readbigshort(ifP, &flags);
 
        pm_readcharu(ifP, &pixelSizeCode);
        pixelSize = pixelSizeCode == 0 ? 1 : pixelSizeCode;
        if (pixelSizeCode != 0x00 &&
            pixelSizeCode != 0x01 &&
            pixelSizeCode != 0x02 &&
            pixelSizeCode != 0x04 &&
            pixelSizeCode != 0x08 &&
            pixelSizeCode != 0x10 &&
            pixelSizeCode != 0xFF)
            pm_error("Invalid value for bits per pixel: %u.", pixelSizeCode);

        if ((bytesPerRow * 8) < (cols * pixelSize))
            pm_error("%u bytes per row is not valid with %u columns and %u "
                     "bits per pixel.", bytesPerRow, cols, pixelSize);

        pm_readcharu(ifP, &version);
        if (version > 3) 
            pm_error("Unknown encoding version type: %d", version);
        else if (version == 3)
            readRestOfHeaderVersion3(ifP, pixelSize,
                                     &size, &pixelFormat, &compressionType,
                                     &density, &transparentIndex, 
                                     &transparentValue, &nextBitmapOffset,
                                     &nextDepthOffset);
        else
            readRestOfHeaderOld(ifP,
                                &size, &pixelFormat, &compressionType,
                                &density, &transparentIndex,
                                &transparentValue, &nextBitmapOffset,
                                &nextDepthOffset);

        if (currentRendition < requestedRendition) {
             if (version < 3 && nextDepthOffset == 0 && pixelSizeCode != 0xFF) 
                 pm_error("Not enough renditions in the input Palm Bitmap "
                          "to extract the %dth", requestedRendition);
             if (version == 3 && nextBitmapOffset == 0) 
                 pm_error("Not enough renditions in the input Palm Bitmap "
                          "to extract the %dth", requestedRendition);
             /* nextDepthOffset is calculated in 4 byte words
                from the beginning of this bitmap (so it equals its size) 
             */
             if (version < 3 && pixelSizeCode != 0xFF )
                 skipbytes(ifP, (nextDepthOffset*4)-16);
             else if (version == 3)
                 /* FIXME rewrite skipbytes to accept longs? */
                 skipbytes(ifP, (short) nextBitmapOffset-24); 
             if (pixelSizeCode != 0xFF)
                 ++currentRendition;
        } else if (pixelSizeCode != 0xFF) {
            gotHeader = TRUE;
            
            interpretHeader(palmHeaderP,
                            cols, rows, bytesPerRow, flags, pixelSizeCode,
                            pixelSize, version, size, pixelFormat, density,
                            transparentValue, transparentIndex, 
                            compressionType);
        }
    }
}



static const char *
yesno(bool const arg) {

    if (arg)
        return "YES";
    else
        return "NO";
}


static void
reportPalmHeader(struct palmHeader      const palmHeader,
                 struct directColorInfo const directColorInfo) {

    const char *ctype;

    switch (palmHeader.compressionType) {
    case COMPRESSION_RLE:
        ctype = "rle (Palm OS 3.5)";
        break;
    case COMPRESSION_SCANLINE:
        ctype = "scanline (Palm OS 2.0)";
        break;
    case COMPRESSION_PACKBITS:
        ctype = "packbits (Palm OS 4.0)";
        break;
    case COMPRESSION_NONE:
        ctype = "none";
        break;
    }
    pm_message("Dimensions: %hu columns x %hu rows",
               palmHeader.cols, palmHeader.rows);
    pm_message("Row layout: %hu bytes per row, %hu bits per pixel",
               palmHeader.bytesPerRow, palmHeader.pixelSize);
    pm_message("Pixel Size code: %hu", palmHeader.pixelSizeCode);
    pm_message("Flags: 0x%04hx", palmHeader.flags);
    pm_message("  Direct Color: %s", yesno(palmHeader.directColor));
    pm_message("  Colormap:     %s", yesno(palmHeader.hasColormap));
    pm_message("  Transparency: %s", yesno(palmHeader.hasTransparency));
    pm_message("Version %d", palmHeader.version);
    if (palmHeader.hasTransparency) {
        if (palmHeader.directColor) {
            /* Copied from doTransparent(...) */
            Color_s const color = directColorInfo.transparentColor;
            pm_message("Transparent value: #%02x%02x%02x", 
                       (unsigned int)((color >> 16) & 0xFF), 
                       (unsigned int)((color >>  8) & 0xFF), 
                       (unsigned int)((color >>  0) & 0xFF));
        } else
            pm_message("Transparent index: %u", palmHeader.transparentIndex);
    }
    pm_message("Compression type: %s", ctype);
    if (palmHeader.version == 3)
        pm_message("Density: %d", palmHeader.density);
}



static void
determineOutputFormat(struct palmHeader const palmHeader,
                      int *             const formatP,
                      xelval *          const maxvalP) {

    if (palmHeader.directColor) {
        *formatP = PPM_TYPE;
        *maxvalP = 255;
    } else if (palmHeader.hasColormap) {
        *formatP = PPM_TYPE;
        *maxvalP = 255;
    } else if (palmHeader.pixelSize == 1) {
        *formatP = PBM_TYPE;
        *maxvalP = 1;
    } else if (palmHeader.pixelSize >= 8) {
        *formatP = PPM_TYPE;
        *maxvalP = pm_bitstomaxval(palmHeader.pixelSize);
    } else {
        *formatP = PGM_TYPE;
        *maxvalP = pm_bitstomaxval(palmHeader.pixelSize);
    }
}



static void
readRgbFormat(FILE *                     const ifP,
              struct directPixelFormat * const pixelFormatP) {

    unsigned char r, g, b;

    pm_readcharu(ifP, &r);
    pm_readcharu(ifP, &g);
    pm_readcharu(ifP, &b);
    
    if (r != 5 || g != 6 || b != 5)
        pm_error("This image has a direct color pixel format of "
                 "%u red, %u green, %u blue bits.  This program "
                 "can handle only 5, 6, 5.", r, g, b);
    else {
        pixelFormatP->redbits   = r;
        pixelFormatP->greenbits = g;
        pixelFormatP->bluebits  = b;
    }
}



static void
readDirectTransparentColor(FILE *    const ifP,
                           Color_s * const colorP) {

    unsigned char r, g, b;

    pm_readcharu(ifP, &r);
    pm_readcharu(ifP, &g);
    pm_readcharu(ifP, &b);

    *colorP = (r << 16) | (g << 8) | (b << 0);
}



static void
readDirectInfoType(FILE *                   const ifP,
                   struct palmHeader        const palmHeader,
                   struct directColorInfo * const directInfoTypeP) {
/*----------------------------------------------------------------------------
   Read the Palm Bitmap Direct Info Type section, if any.

   The Direct Info Type section is a section of a pre-Version 3 direct
   color Palm Bitmap that tells how to interpret the direct color
   raster.

   Return an undefined value as *directInfoTypeP if there is no such
   section in this Palm Bitmap.
-----------------------------------------------------------------------------*/
    if ((palmHeader.directColor) && palmHeader.pixelSize != 16)
        pm_error("The image is of the direct color type, but has %u "
                 "bits per pixel.  The only kind of direct color images "
                 "this program understands are 16 bit ones.", 
                 palmHeader.pixelSize);

    if (palmHeader.version == 3) {
        /* All direct color info is in the header, because it'sversion
           3 encoding.  No Direct Info Type section.  
        */
    } else {
        if (palmHeader.directColor) {
            unsigned char padding;

            readRgbFormat(ifP, &directInfoTypeP->pixelFormat);

            pm_readcharu(ifP, &padding);
            pm_readcharu(ifP, &padding);

            readDirectTransparentColor(ifP, 
                                       &directInfoTypeP->transparentColor);
        } else {
            /* Not a direct color image; no Direct Info Type section. */
        }
    }
}



static void
readColormap(FILE *            const ifP,
             struct palmHeader const palmHeader,
             Colormap *        const colormapP) {
/*----------------------------------------------------------------------------
   Read the colormap, if any from the Palm Bitmap.

   If the image described by 'palmHeader' doesn't have a colormap,
   return an undefined value as *colormapP.
-----------------------------------------------------------------------------*/
    if (palmHeader.hasColormap)
        *colormapP = palmcolor_read_colormap(ifP);
}



static void
getColorInfo(struct palmHeader        const palmHeader,
             struct directColorInfo   const directInfoType,
             Colormap                 const colormapFromImage,
             Colormap *               const colormapP,
             unsigned int *           const ncolorsP,
             struct directColorInfo * const directColorInfoP) {
/*----------------------------------------------------------------------------
   Gather color encoding information from the various sources.

   Note that 'directInfoType' and 'colormapFromImage' are meaningful only
   with certain values of 'palmHeader'.

   If it's a version 3 direct color, the pixel format must be "565".
-----------------------------------------------------------------------------*/
    if (palmHeader.version == 3 && palmHeader.directColor) {
        *colormapP = NULL;

        assert(palmHeader.pixelFormat == PALM_FORMAT_565);

        directColorInfoP->pixelFormat.redbits   = 5;
        directColorInfoP->pixelFormat.greenbits = 6;
        directColorInfoP->pixelFormat.bluebits  = 5;
        directColorInfoP->transparentColor = 
            /* See convertRowToPnmDirect for this trick

               This will break once maxval isn't always set 255 for
               directColor
            */
            ((((palmHeader.transparentValue >> 11) & 0x1F) * 255 / 0x1F)
             << 16) |
            ((((palmHeader.transparentValue >>  5) & 0x3F) * 255 / 0x3F)
             <<  8) |
            ((((palmHeader.transparentValue >>  0) & 0x1F) * 255 / 0x1F)
             <<  0);
    } else if (palmHeader.directColor) {
        *colormapP = NULL;
        *directColorInfoP = directInfoType;
    } else if (palmHeader.hasColormap)
        *colormapP = colormapFromImage;
    else if (palmHeader.pixelSize >= 8) {
        Colormap colormap;
        colormap = palmcolor_build_default_8bit_colormap();
        qsort(colormap->color_entries, colormap->ncolors, 
              sizeof(Color_s), palmcolor_compare_indices);
        *colormapP = colormap;
    } else
        *colormapP = NULL;

    *ncolorsP = 1 << palmHeader.pixelSize;
}



static void
doTransparent(FILE *                 const ofP,
              bool                   const hasTransparency,
              bool                   const directColor,
              unsigned char          const transparentIndex,
              unsigned char          const pixelSize,
              Colormap               const colormap,
              struct directColorInfo const directColorInfo) {
/*----------------------------------------------------------------------------
   Generate a PNM comment on *ofP telling what color in the raster is
   supposed to be transparent.

   Note that PNM itself doesn't have any way to represent transparency.
   (But this program could be converted to a PAM program and use the
   RGB_ALPHA and GRAYSCALE_ALPHA tuple types).
-----------------------------------------------------------------------------*/
    if (hasTransparency) {
        if (colormap) {
            Color_s const color = transparentIndex << 24;
            Color const actualColor = (bsearch(&color,
                                               colormap->color_entries, 
                                               colormap->ncolors,
                                               sizeof(color), 
                                               palmcolor_compare_indices));
            fprintf(ofP, "#%02x%02x%02x\n", 
                   (unsigned int) ((*actualColor >> 16) & 0xFF),
                   (unsigned int) ((*actualColor >>  8) & 0xFF), 
                   (unsigned int) ((*actualColor >>  0) & 0xFF));
        } else if (directColor) {
            Color_s const color = directColorInfo.transparentColor;
            fprintf(ofP, "#%02x%02x%02x\n", 
                   (unsigned int)((color >> 16) & 0xFF), 
                   (unsigned int)((color >>  8) & 0xFF), 
                   (unsigned int)((color >>  0) & 0xFF));
        } else {
            unsigned int const maxval = pm_bitstomaxval(pixelSize);
            unsigned int const grayval = 
                ((maxval - transparentIndex) * 256) / maxval;
            fprintf(ofP, "#%02x%02x%02x\n", grayval, grayval, grayval);
        }
    }
}



static void
createHistogram(unsigned int    const ncolors,
                unsigned int ** const seenP) {

    unsigned int * seen;

    MALLOCARRAY(seen, ncolors);
    if (!seen)
        pm_error("Can't allocate array for keeping track of "
                 "how many pixels of each of %u colors are in the image.", 
                 ncolors);

    {    
        /* Initialize the counter for each color to zero */
        unsigned int i;
        for (i = 0; i < ncolors; ++i)
            seen[i] = 0;
    }
    *seenP = seen;
}



static void
readScanlineRow(FILE *          const ifP,
                unsigned char * const palmrow,
                unsigned char * const lastrow,
                unsigned int    const bytesPerRow,
                bool            const firstRow) {

    unsigned int j;

    for (j = 0; j < bytesPerRow; j += 8) {
        unsigned char diffmask;
            /* A mask telling whether each of the 8 raster bytes indexed
               j through j+7 is the same as in the previous row ('lastrow')
               or is to be read from the file.  Bit 0 of the mask refers
               to byte j, Bit 1 to byte j + 1, etc.
            */
        unsigned int byteCount;
            /* How many bytes are covered by 'diffmask'.  Normally 8, but
               at the end of the row, could be less.
            */
        unsigned int k;

        pm_readcharu(ifP, &diffmask);
        byteCount = MIN(bytesPerRow - j, 8);
        
        for (k = 0; k < byteCount; ++k) {
            /* the first row cannot be compressed */
            if (firstRow || ((diffmask & (1 << (7 - k))) != 0)) {
                unsigned char inval;
                pm_readcharu(ifP, &inval);
                palmrow[j + k] = inval;
            } else
                palmrow[j + k] = lastrow[j + k];
        }
    }
    memcpy(lastrow, palmrow, bytesPerRow);
}



static void
readRleRow(FILE *          const ifP,
           unsigned char * const palmrow,
           unsigned int    const bytesPerRow) {

    unsigned int j;

    for (j = 0;  j < bytesPerRow; ) {
        unsigned char incount;
        unsigned char inval;

        pm_readcharu(ifP, &incount);
        if (incount == 0)
            pm_error("Invalid (zero) count in RLE compression.");
        if (j + incount > bytesPerRow)
            pm_error("Bytes in RLE compressed row exceed bytes per row.  "
                     "Bytes per row is %u.  A run length of %u bytes "
                     "pushes the bytes in this row up to %u bytes (and then "
                     "we gave up).", bytesPerRow, incount, j + incount);
        pm_readcharu(ifP, &inval);
        memset(palmrow + j, inval, incount);
        j += incount;
    }
} 



static void
readPackBitsRow16(FILE *          const ifP,
                  unsigned char * const palmrow,
                  unsigned int    const bytesPerRow) {

    /*  From the Palm OS Programmer's API Reference:
  
        Although the [...] spec is byte-oriented, the 16-bit algorithm is
        identical [to the 8-bit algorithm]: just substitute "word" for "byte".
    */
    unsigned int j;

    for (j = 0;  j < bytesPerRow; ) {
        char incount;
        pm_readchar(ifP, &incount);
        if (incount < 0) { 
            /* How do we handle incount == -128 ? */
            unsigned int const runlength = (-incount + 1) * 2;
            unsigned int k;
            unsigned short inval;
            pm_readlittleshortu(ifP, &inval);
            for (k = 0; (k < runlength) && (j + k + 1 < bytesPerRow); k += 2) {
                memcpy(palmrow + j + k, &inval, 2);
            }
            j += runlength;
        } else {
            /* We just read the stream of shorts as a stream of chars */
            unsigned int const nonrunlength = (incount + 1) * 2;
            unsigned int k;
            for (k = 0; (k < nonrunlength) && (j + k < bytesPerRow); ++k) {
                unsigned char inval;
                pm_readcharu(ifP, &inval);
                palmrow[j + k] = inval;
            }
            j += nonrunlength;
        }
        if (j > bytesPerRow) 
            pm_error("Bytes in PackBits compressed row exceed bytes per row.  "
                     "Bytes per row is %u.  "
                     "The bytes in this row were pushed up to %u bytes "
                     "(and then we gave up).", bytesPerRow, j);
    }
} 



static void
readPackBitsRow(FILE *          const ifP,
                unsigned char * const palmrow,
                unsigned int    const bytesPerRow) {

    unsigned int j;

    for (j = 0;  j < bytesPerRow; ) {
        char incount;
        pm_readchar(ifP, &incount);
        if (incount < 0) { 
            /* How do we handle incount == -128 ? */
            unsigned int const runlength = -incount + 1;
            unsigned char inval;
            pm_readcharu(ifP, &inval);
            if (j + runlength < bytesPerRow) 
                memset(palmrow + j, inval, runlength);
            j += runlength;
        } else {
            unsigned int const nonrunlength = incount + 1;
            unsigned int k;
            for (k = 0; k < nonrunlength && j + k < bytesPerRow; ++k) {
                unsigned char inval;
                pm_readcharu(ifP, &inval);
                palmrow[j + k] = inval;
            }
            j += nonrunlength;
        }
        if (j > bytesPerRow) 
            pm_error("Bytes in PackBits compressed row exceed bytes per row.  "
                     "Bytes per row is %u.  "
                     "The bytes in this row were pushed up to %u bytes "
                     "(and then we gave up).", bytesPerRow, j);
    }
} 



static void
readUncompressedRow(FILE *          const ifP,
                    unsigned char * const palmrow,
                    unsigned int    const bytesPerRow) {

    int bytesRead;
    
    bytesRead = fread(palmrow, 1, bytesPerRow, ifP);
    if (bytesRead != bytesPerRow)
        pm_error("Error reading Palm file.  Short read.");
}



static void
readDecompressedRow(FILE *                   const ifP,
                    unsigned char *          const palmrow,
                    unsigned char *          const lastrow,
                    enum palmCompressionType const compressionType,
                    unsigned int             const bytesPerRow,
                    unsigned int             const pixelSize,
                    bool                     const firstRow) {
/*----------------------------------------------------------------------------
   Read a row from Palm file 'ifP', in uncompressed form (i.e. decompress if
   necessary).  Assume the row contains 'bytesPerRow' uncompressed bytes,
   compressed according to 'compressionType'.  Return the data at 'palmrow'.

   'firstRow' means decompress it as if it is the first row of the image
   (some compression schemes transform the first row differently from the
   rest, because each row depends on the row before it).

   If 'compressionType' is COMPRESSION_SCANLINE, (which means
   transformation of a row depends on the contents of the row before
   it), then 'lastRow' is as input the uncompressed contents of the
   previous row (undefined if 'firstRow' is true).  In that case, we
   modify 'lastrow' to contain a copy of 'palmrow' (so Caller can
   conveniently use it to read the next row).

   If 'compressionType' is not COMPRESSION_SCANLINE, 'lastrow' is
   undefined both as input and output.
-----------------------------------------------------------------------------*/
    switch (compressionType) {
    case COMPRESSION_RLE:
        readRleRow(ifP, palmrow, bytesPerRow);
        break;
    case COMPRESSION_SCANLINE:
        readScanlineRow(ifP, palmrow, lastrow, bytesPerRow, firstRow);
        break;
    case COMPRESSION_PACKBITS:
        if (pixelSize != 16)
            readPackBitsRow(ifP, palmrow, bytesPerRow);
        else
            readPackBitsRow16(ifP, palmrow, bytesPerRow);
        break;
    case COMPRESSION_NONE:
        readUncompressedRow(ifP, palmrow, bytesPerRow);
        break;
    }
}



static void
convertRowToPnmDirect(const unsigned char * const palmrow,
                      xel *                 const xelrow,
                      unsigned int          const cols,
                      xelval                const maxval,
                      unsigned int *        const seen) {

    /* There's a problem with this.  Take the Palm 16-bit
       direct color.  That's 5 bits for the red, 6 for the
       green, and 5 for the blue.  So what should the MAXVAL
       be?  I decided to use 255 (8 bits) for everything,
       since that's the theoretical max of the number of bits
       in any one color, according to Palm.  So the Palm color
       0xFFFF (white) would be red=0x1F, green=0x3F, and
       blue=0x1F.  How do we promote those colors?  Simple
       shift would give us R=248,G=252,B=248; which is
       slightly green.  Hardly seems right.
       
       So I've perverted the math a bit.  Each color value is
       multiplied by 255, then divided by either 31 (red or
       blue) or 63 (green).  That's the right way to do it
       anyway.  
    */

    const unsigned char *inbyte;
    unsigned int j;
    
    for (inbyte = palmrow, j = 0;  j < cols;  ++j) {
        unsigned int inval;
        inval = *inbyte++ << 8;
        inval |= *inbyte++;
        
        if (seen)
            ++seen[inval];
        
        PPM_ASSIGN(xelrow[j], 
                   (((inval >> 11) & 0x1F) * maxval) / 0x1F, 
                   (((inval >>  5) & 0x3F) * maxval) / 0x3F, 
                   (((inval >>  0) & 0x1F) * maxval) / 0x1F
            );
    }
}



static void
convertRowToPnmNotDirect(const unsigned char * const palmrow,
                         xel *                 const xelrow,
                         unsigned int          const cols,
                         Colormap              const colormap,
                         xelval *              const graymap,
                         unsigned int *        const seen,
                         unsigned int          const pixelSize) {

    unsigned int const mask = (1 << pixelSize) - 1;

    const unsigned char *inbyte;
    unsigned int inbit;
    unsigned int j;
    
    inbit = 8 - pixelSize;
    inbyte = palmrow;
    for (j = 0; j < cols; ++j) {
        short const color = ((*inbyte) & (mask << inbit)) >> inbit;
        if (seen)
            ++seen[color];
        
        if (colormap) {
            Color_s const color2      = color << 24;
            Color   const actualColor = (bsearch (&color2,
                                                  colormap->color_entries, 
                                                  colormap->ncolors,
                                                  sizeof(color2), 
                                                  palmcolor_compare_indices));
            PPM_ASSIGN(xelrow[j], 
                       (*actualColor >> 16) & 0xFF, 
                       (*actualColor >>  8) & 0xFF, 
                       (*actualColor >>  0) & 0xFF);
        } else
            PNM_ASSIGN1(xelrow[j], graymap[color]);
        
        if (!inbit) {
            ++inbyte;
            inbit = 8 - pixelSize;
        } else
            inbit -= pixelSize;
    }
}



static void
writePnm(FILE *            const ofP,
         struct palmHeader const palmHeader,
         FILE *            const ifP,
         Colormap          const colormap,
         xelval *          const graymap,
         unsigned int      const nColors,
         int               const format,
         xelval            const maxval,
         unsigned int **   const seenP) {

    int const cols = palmHeader.cols;
    int const rows = palmHeader.rows;

    unsigned char * palmrow;
    unsigned char * lastrow;
    xel *           xelrow;
    unsigned int *  seen;
    unsigned int    row;
    
    pnm_writepnminit(ofP, cols, rows, maxval, format, 0);
    xelrow = pnm_allocrow(cols);

    /* Read the picture data, one row at a time */
    MALLOCARRAY_NOFAIL(palmrow, palmHeader.bytesPerRow);
    MALLOCARRAY_NOFAIL(lastrow, palmHeader.bytesPerRow); 
    
    if (seenP) {
        createHistogram(nColors, &seen);
        *seenP = seen;
    } else
        seen = NULL;

    /* We should actually use compressedDataSizeNN for checking the sanity 
       of the data we're reading ... 
    */
    if (palmHeader.compressionType != COMPRESSION_NONE) {
        if (palmHeader.version < 3) {
            short compressedDataSize16;
            pm_readbigshort(ifP, &compressedDataSize16);
        } else {
            long compressedDataSize32;
            pm_readbiglong(ifP, &compressedDataSize32);
        }
    }

    for (row = 0; row < rows; ++row) {
        readDecompressedRow(ifP, palmrow, lastrow, 
                            palmHeader.compressionType, 
                            palmHeader.bytesPerRow,
                            palmHeader.pixelSize,
                            row == 0);

        if (palmHeader.directColor) {
            assert(palmHeader.pixelSize == 16);
            convertRowToPnmDirect(palmrow, xelrow, cols, maxval, seen);
        } else
            convertRowToPnmNotDirect(palmrow, xelrow, cols, colormap, graymap,
                                     seen, palmHeader.pixelSize);

        pnm_writepnmrow(ofP, xelrow, cols, maxval, format, 0);
    }
    free(lastrow);
    free(palmrow);
    pnm_freerow(xelrow);
}



static void
showHistogram(unsigned int * const seen,
              Colormap       const colormap,
              const xelval * const graymap,
              unsigned int   const ncolors) {

    unsigned int colorIndex;
    
    for (colorIndex = 0;  colorIndex < ncolors; ++colorIndex) {
        if (!colormap)
            pm_message("%.3d -> %.3d:  %d", 
                       colorIndex, graymap[colorIndex], seen[colorIndex]);
        else {
            Color_s const color = colorIndex << 24;
            Color const actualColor = (bsearch(&color,
                                               colormap->color_entries, 
                                               colormap->ncolors,
                                               sizeof(color), 
                                               palmcolor_compare_indices));
            if (actualColor)
                pm_message("%.3d -> %ld,%ld,%ld:  %d", colorIndex,
                           (*actualColor >> 16) & 0xFF,
                           (*actualColor >> 8) & 0xFF,
                           (*actualColor & 0xFF), seen[colorIndex]);
        }
    }
}



int
main(int argc, char **argv) {

    struct cmdlineInfo cmdline;

    FILE* ifP;
    struct palmHeader palmHeader;
    struct directColorInfo directInfoType;
    Colormap colormapFromImage;
    Colormap colormap;
    struct directColorInfo directColorInfo;
    int format;
    xelval maxval;
    unsigned int nColors;

    /* Parse default params */
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    readHeader(ifP, cmdline.rendition, &palmHeader);

    readDirectInfoType(ifP, palmHeader, &directInfoType);
   
    readColormap(ifP, palmHeader, &colormapFromImage);
    
    determineOutputFormat(palmHeader, &format, &maxval);
    
    getColorInfo(palmHeader, directInfoType, colormapFromImage,
                 &colormap, &nColors, &directColorInfo);

    if (cmdline.verbose)
        reportPalmHeader(palmHeader, directColorInfo);

    if (cmdline.transparent)
        doTransparent(stdout, 
                      palmHeader.hasTransparency, palmHeader.directColor,
                      palmHeader.transparentIndex, 
                      palmHeader.pixelSize, colormap, directColorInfo);
    else {
        unsigned int * seen;
        xelval * graymap;

        graymap = createGraymap(nColors, maxval);

        writePnm(stdout,
                 palmHeader, ifP, colormap, graymap, nColors, format, maxval, 
                 cmdline.showhist ? &seen : NULL);
        
        if (cmdline.showhist)
            showHistogram(seen, colormap, graymap, nColors);
        
        free(graymap);
    }
    pm_close(ifP);

    return 0;
}
