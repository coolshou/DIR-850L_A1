/*****************************************************************************
                                    bmptopnm.c
******************************************************************************
 
 Bmptopnm - Converts from a Microsoft Windows or OS/2 .BMP file to a
 PBM, PGM, or PPM file.

 This program was formerly called Bmptoppm (and generated only PPM output).
 The name was changed in March 2002.

 Copyright (C) 1992 by David W. Sanderson.
 
 Permission to use, copy, modify, and distribute this software and its
 documentation for any purpose and without fee is hereby granted,
 provided that the above copyright notice appear in all copies and
 that both that copyright notice and this permission notice appear
 in supporting documentation.  This software is provided "as is"
 without express or implied warranty.

*****************************************************************************/
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "pnm.h"
#include "shhopt.h"
#include "nstring.h"
#include "bmp.h"

/* MAXCOLORS is the maximum size of a color map in a BMP image */
#define MAXCOLORS       256

static xelval const bmpMaxval = 255;
    /* The maxval for intensity values in a BMP image -- either in a
       truecolor raster or in a colormap
    */

enum rowOrder {BOTTOMUP, TOPDOWN};

struct bitPosition {
    /* mask and shift count to describe a set of bits in a binary value.

       Example: if 16 bits are laid out as XRRRRRGGGGGBBBBB then the shift
       count for the R component is 10 and the mask is 0000000000011111.
    */
    unsigned int shift;
        /* How many bits right you have to shift the value to get the subject
           bits in the least significant bit positions.
        */
    unsigned int mask;
        /* Has one bits in positions where the subject bits are after
           shifting.
        */
};

struct pixelformat {
    /* The format of a pixel representation from the raster.  i.e. which 
       bits apply to red, green, blue, and transparency 
    */
    struct bitPosition red;
    struct bitPosition blu;
    struct bitPosition grn;
    struct bitPosition trn;
    
    bool conventionalBgr;
        /* This means that the above bit positions are just the conventional
           BGR format -- one byte Blue, one byte Green, one byte Red,
           no alpha.  Though it's totally redundant with the members above,
           this member speeds up computation:  We've never actually seen
           a BMP file that doesn't use conventional BGR, and it doesn't
           require any masking or shifting at all to interpret.
        */
};

struct bmpInfoHeader {
    enum rowOrder rowOrder;
    int cols;
    int rows;
    unsigned int cBitCount;
        /* Number of bits in the BMP file that each pixel occupies. */
    enum bmpClass class;
    bool bitFields;
        /* The raster values are arranged in arbitrary bit fields as
           described by the "mask" values in the header, rather than
           fixed formats.
        */
    unsigned int cmapsize;
        /* Size in bytes of the colormap (palette) in the BMP file.

           Zero means there is no colormap.
        */
    unsigned int imageSize;
        /* Size in bytes of the image data.  We only reference this 
           when the image is compressed. */    
    unsigned short cPlanes;
    unsigned long int compression;
    struct pixelformat pixelformat;
};



struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespecs of input files */
    int verbose;    /* -verbose option */
};

static const char *ifname;



static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdline_p) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optStruct *option_def = malloc(100*sizeof(optStruct));
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct2 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENTRY(0,   "verbose",     OPT_FLAG,   &cmdline_p->verbose,         0);
 
    /* Set the defaults */
    cmdline_p->verbose = FALSE;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions2(&argc, argv, opt, 0);
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (argc-1 == 0) 
        cmdline_p->input_filespec = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdline_p->input_filespec = argv[1];

}



static const char er_read[] = "%s: read error";

static int
GetByte(FILE * const fp) {

    int             v;

    if ((v = getc(fp)) == EOF)
        pm_error(er_read, ifname);

    return v;
}



static short
GetShort(FILE * const fp) {

    short           v;

    if (pm_readlittleshort(fp, &v) == -1)
        pm_error(er_read, ifname);

    return v;
}

static short
GetBigShort(FILE * const fp) {

    short           v;

    if (pm_readbigshort(fp, &v) == -1)
        pm_error(er_read, ifname);

    return v;
}



static long
GetLong(FILE * const fp) {

    long v;

    if (pm_readlittlelong(fp, &v) == -1)
        pm_error(er_read, ifname);

    return v;
}



typedef struct {
    long dummy[12];
} cieXyzTriple;

static cieXyzTriple
GetCieXyzTriple(FILE * const fp) {

    cieXyzTriple v;
    unsigned int i;

    for (i = 0; i < 12; ++i) 
        if (pm_readlittlelong(fp, &v.dummy[i]) == -1)
            pm_error(er_read, ifname);

    return v;
}



static void
readOffBytes(FILE * const fp, unsigned int const nbytes) {
/*----------------------------------------------------------------------------
   Read 'nbytes' from file 'fp'.  Abort program if read error.
-----------------------------------------------------------------------------*/
    int i;
    
    for(i = 0; i < nbytes; ++i) {
        int rc;
        rc = getc(fp);
        if (rc == EOF)
            pm_error(er_read, ifname);
    }
}



static void
BMPreadfileheader(FILE *         const ifP, 
                  unsigned int * const bytesReadP, 
                  unsigned int * const offBitsP) {

    unsigned short  xHotSpot;
    unsigned short  yHotSpot;
    unsigned long   offBits;
    unsigned long int fileSize;


    if (GetByte(ifP) != 'B')
        pm_error("'%s' is not a BMP file.  (It doesn't start with 'BM')",
                 ifname);
    if (GetByte(ifP) != 'M')
        pm_error("'%s' is not a BMP file.  (It doesn't start with 'BM')",
                 ifname);


    fileSize = GetLong(ifP);  /* This is not always reliable. */
    xHotSpot  = GetShort(ifP);
    yHotSpot  = GetShort(ifP);
    offBits   = GetLong(ifP);

    *offBitsP = offBits;

    *bytesReadP = 14;
}



static void
readOs2InfoHeader(FILE *                 const ifP,
                  struct bmpInfoHeader * const headerP) {

    headerP->class = C_OS2;

    headerP->cols = GetShort(ifP);
    headerP->rows = GetShort(ifP);
    headerP->rowOrder = BOTTOMUP;
    headerP->cPlanes = GetShort(ifP);
    headerP->cBitCount = GetShort(ifP);
    /* I actually don't know if the OS/2 BMP format allows
       cBitCount > 8 or if it does, what it means, but ppmtobmp
       creates such BMPs, more or less as a byproduct of creating
       the same for Windows BMP, so we interpret cBitCount > 8 the
       same as for Windows.
    */
    if (headerP->cBitCount <= 8)
        headerP->cmapsize = 1 << headerP->cBitCount;
    else if (headerP->cBitCount == 24)
        headerP->cmapsize = 0;
    /* There is a 16 bit truecolor format, but we don't know how the
       bits are divided among red, green, and blue, so we can't handle it.
    */
    else
        pm_error("Unrecognized bits per pixel in OS/2 BMP file header: %d",
                 headerP->cBitCount);
                 
    headerP->compression = COMP_RGB;
    
    pm_message("OS/2 BMP, %dx%dx%d",
               headerP->cols, headerP->rows, headerP->cBitCount);
}



static void
readWindowsBasic40ByteInfoHeader(FILE *                 const ifP,
                                 struct bmpInfoHeader * const headerP) {
/*----------------------------------------------------------------------------
   Read from the file stream 'ifP' the basic BMP Info header.  This does
   not include any Info header extension.  The Info header is the data
   that comes after the BMP file header.

   Return the information from the info header as *headerP.
-----------------------------------------------------------------------------*/
    int colorsimportant;   /* ColorsImportant value from header */
    int colorsused;        /* ColorsUsed value from header */

    headerP->class = C_WIN;

    headerP->cols = GetLong(ifP);
    {
        long const cy = GetLong(ifP);
        if (cy == 0)
            pm_error("Invalid BMP file: says height is zero");
        if (cy < 0) {
            headerP->rowOrder = TOPDOWN;
            headerP->rows = - cy;
        } else {
            headerP->rowOrder = BOTTOMUP;
            headerP->rows = cy;
        }
    }
    headerP->cPlanes = GetShort(ifP);
    headerP->cBitCount = GetShort(ifP);
 
    {
        unsigned long int const compression = GetLong(ifP);

        headerP->bitFields = (compression == COMP_BITFIELDS);
    
        if (compression != COMP_RGB && compression != COMP_BITFIELDS &&
            compression != COMP_RLE4 && compression != COMP_RLE8 ) 
            pm_error("Input is compressed.  Unsupported encoding method.\n"
                     "Compression type code = %ld", compression);
                     
        if ( (compression == COMP_RLE4 || compression == COMP_RLE8) &&
              headerP->rowOrder == TOPDOWN )                        
            pm_error("Invalid BMP header.  Top-down images cannot be compressed.");

        if ( (compression == COMP_RLE4 && headerP->cBitCount !=4) ||
             (compression == COMP_RLE8 && headerP->cBitCount !=8) )                        
            pm_error("Invalid BMP header.\n" 
                     "Compression type (%s) disagrees with number of bits per pixel (%u).",
                      compression == COMP_RLE4 ? "RLE4" : "RLE8",
                      headerP->cBitCount);

        headerP->compression = compression;             
    }
    /* And read the rest of the junk in the 40 byte header */
    headerP->imageSize = GetLong(ifP);   /* ImageSize */
    GetLong(ifP);   /* XpixelsPerMeter */
    GetLong(ifP);   /* YpixelsPerMeter */
    colorsused = GetLong(ifP);   /* ColorsUsed */
    /* See comments in bmp.h for info about the definition of the following
       word and its relationship to the color map size (headerP->cmapsize).
    */
    colorsimportant = GetLong(ifP);  /* ColorsImportant */

    if (headerP->cBitCount <= 8) {
        if (colorsused != 0) {
            if (colorsused > 1 << headerP->cBitCount)
                pm_error("Invalid BMP header.  Says %u bits per pixel, "
                         "but %d colors used", 
                         headerP->cBitCount, colorsused);
            else
                headerP->cmapsize = colorsused;
        } else 
            headerP->cmapsize = 1 << headerP->cBitCount;
    } else if (headerP->cBitCount == 24 || 
               headerP->cBitCount == 16 || 
               headerP->cBitCount == 32)
        headerP->cmapsize = 0;
    else
        pm_error("Unrecognized bits per pixel in Windows BMP file header: %d",
                 headerP->cBitCount);
}



static unsigned int
lsbZeroCount(unsigned int const mask)
/*----------------------------------------------------------------------------
   Return the number of consecutive zeroes in the mask 'mask', starting with
   the least significant bit and going up.  E.g. for 0x20, it would be 5.
   
   Use GCC built-in when available.
-----------------------------------------------------------------------------*/

#if ( defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 304) )
{
      return ( mask==0 ? sizeof(mask)*8 : __builtin_ctz(mask) );
}
#else
{
      unsigned int i=0;
    
      while (((mask >> i) & 0x1) == 0 && i < sizeof(mask)*8)
        ++i;

      return i;

}
#endif


static struct bitPosition
bitPositionFromMask(long const bmpMask) {
    struct bitPosition retval;

    retval.shift = lsbZeroCount(bmpMask);
    retval.mask  = bmpMask >> retval.shift;

    return retval;
}



static void
computeConventionalBgr(struct pixelformat * const fP,
                       unsigned int         const bitCount) {

    switch (bitCount) {
    case 24:
        fP->conventionalBgr =
            fP->red.shift ==  0 && fP->red.mask == 0xFF &&
            fP->grn.shift ==  8 && fP->grn.mask == 0xFF &&
            fP->blu.shift == 16 && fP->blu.mask == 0xFF &&
            fP->trn.mask == 0
            ;
        break;
    case 32:
        fP->conventionalBgr =
            fP->red.shift ==  8  && fP->red.mask == 0xFF &&
            fP->grn.shift == 16  && fP->grn.mask == 0xFF &&
            fP->blu.shift == 24  && fP->blu.mask == 0xFF &&
            fP->trn.mask == 0
            ;
        break;
    default:
        fP->conventionalBgr = FALSE;
    }
}



static struct pixelformat
defaultPixelformat(unsigned int const bitCount) {

    struct pixelformat retval;

    switch (bitCount) {
    case 16:
        retval.conventionalBgr = FALSE;
        retval.red.shift = 10;
        retval.grn.shift = 5;
        retval.blu.shift = 0;
        retval.trn.shift = 0;
        retval.red.mask = 0x1f;  /* 5 bits */
        retval.grn.mask = 0x1f;  /* 5 bits */
        retval.blu.mask = 0x1f;  /* 5 bits */
        retval.trn.mask = 0;
        break;
    case 24:
    case 32:
        retval.conventionalBgr = TRUE;
        retval.red.shift = 16;
        retval.grn.shift = 8;
        retval.blu.shift = 0;
        retval.trn.shift = 0;
        retval.red.mask = 0xff;  /* 8 bits */
        retval.grn.mask = 0xff;  /* 8 bits */
        retval.blu.mask = 0xff;  /* 8 bits */
        retval.trn.mask = 0;
        break;
    default:
        /* colormapped - masks are undefined */
        break;
    }

    return retval;
}



static void
readV4InfoHeaderExtension(FILE *                 const ifP, 
                          struct bmpInfoHeader * const headerP) {

    if (headerP->bitFields) {
        headerP->pixelformat.red = bitPositionFromMask(GetLong(ifP));
        headerP->pixelformat.grn = bitPositionFromMask(GetLong(ifP));
        headerP->pixelformat.blu = bitPositionFromMask(GetLong(ifP));
        headerP->pixelformat.trn = bitPositionFromMask(GetLong(ifP));

        computeConventionalBgr(&headerP->pixelformat, headerP->cBitCount);
    } else
        headerP->pixelformat = defaultPixelformat(headerP->cBitCount);

    GetLong(ifP);  /* Color space */
    GetCieXyzTriple(ifP);  /* Endpoints */
    GetLong(ifP);  /* GammaRed */
    GetLong(ifP);  /* GammaGreen */
    GetLong(ifP);  /* GammaBlue */
} 



static void
defaultV4InfoHeaderExtension(struct bmpInfoHeader * const headerP) {

    headerP->pixelformat = defaultPixelformat(headerP->cBitCount);

}



static void
readWindowsInfoHeader(FILE *                 const ifP, 
                      unsigned int           const cInfoHeaderSize,
                      struct bmpInfoHeader * const headerP) {

    /* There are 3 major formats of Windows
       BMP, identified by the 3 info header lengths.  The original
       one is 40 bytes.  The "V4 header" is 108 bytes and was
       new with Windows 95 and NT 4.0.  The "V5 header" is 124 bytes
       and was new with Windows 98 and Windows 2000.
    */
    readWindowsBasic40ByteInfoHeader(ifP, headerP);

    if (cInfoHeaderSize >= 108) 
        readV4InfoHeaderExtension(ifP, headerP);
    else 
        defaultV4InfoHeaderExtension(headerP);

    if (cInfoHeaderSize >= 124) {
        /* Read off the V5 info header extension. */
        GetLong(ifP);  /* Intent */
        GetLong(ifP);  /* ProfileData */
        GetLong(ifP);  /* ProfileSize */
        GetLong(ifP);  /* Reserved */
    }

    pm_message("Windows BMP, %dx%dx%d",
               headerP->cols, headerP->rows, headerP->cBitCount);
}



static void
BMPreadinfoheader(FILE *                 const ifP, 
                  unsigned int *         const bytesReadP,
                  struct bmpInfoHeader * const headerP) {

    unsigned int const cInfoHeaderSize = GetLong(ifP);

    switch (cInfoHeaderSize) {
    case 12:
        readOs2InfoHeader(ifP, headerP);
        break;
    case 40: 
    case 108:
    case 124:
        readWindowsInfoHeader(ifP, cInfoHeaderSize, headerP);
        break;
    default:
        pm_error("%s: unknown Info Header size: %u bytes", 
                 ifname, cInfoHeaderSize);
        break;
    }
    *bytesReadP = cInfoHeaderSize;
}



static void
BMPreadcolormap(FILE *         const ifP, 
                int            const class, 
                xel **         const colormapP, 
                unsigned int   const cmapsize,
                unsigned int * const bytesReadP) {
/*----------------------------------------------------------------------------
   Read the color map from the present position in the input BMP file
   *ifP.

   The map has 'cmapsize' entries in it.  cmapsize == 0 means there is
   no color map.

   We return a color map as *colormapP.  If there is no color map in the
   BMP, this is just an arbitrary color map.
 
   'class' is the class of BMP image - Windows or OS/2.
-----------------------------------------------------------------------------*/

    unsigned int i;

    xel * colormap;
    unsigned int bytesRead;

    colormap = pnm_allocrow(MAX(1,cmapsize));
    
    bytesRead = 0;  /* initial value */

    for (i = 0; i < cmapsize; ++i) {
        /* There is a document that says the bytes are ordered R,G,B,Z,
           but in practice it appears to be the following instead:
        */
        unsigned int r, g, b;
        
        b = GetByte(ifP);
        g = GetByte(ifP);
        r = GetByte(ifP);

        PNM_ASSIGN(colormap[i], r, g, b);

        bytesRead += 3;

        if (class == C_WIN) {
            GetByte(ifP);
            bytesRead += 1;
        }
    }
    *colormapP = colormap;
    *bytesReadP = bytesRead;
}



static void
extractBitFields(unsigned int       const rasterval,
                 struct pixelformat const pixelformat,
                 pixval             const maxval,
                 pixval *           const rP,
                 pixval *           const gP,
                 pixval *           const bP,
                 pixval *           const aP) {

    unsigned int const rbits = 
        (rasterval >> pixelformat.red.shift) & pixelformat.red.mask;
    unsigned int const gbits = 
        (rasterval >> pixelformat.grn.shift) & pixelformat.grn.mask;
    unsigned int const bbits = 
        (rasterval >> pixelformat.blu.shift) & pixelformat.blu.mask;
    unsigned int const abits = 
        (rasterval >> pixelformat.trn.shift) & pixelformat.trn.mask;
    
    *rP = pixelformat.red.mask ?
              (unsigned int) rbits * maxval / pixelformat.red.mask : 0;
    *gP = pixelformat.grn.mask ?
              (unsigned int) gbits * maxval / pixelformat.grn.mask : 0;
    *bP = pixelformat.blu.mask ?
              (unsigned int) bbits * maxval / pixelformat.blu.mask : 0;
    *aP = pixelformat.trn.mask ?
              (unsigned int) abits * maxval / pixelformat.trn.mask : 0;
}



static void
convertRow16(unsigned char      const bmprow[],
             xel                      xelrow[],
             int                const cols,
             struct pixelformat const pixelformat) {
    /* It's truecolor.  */

    unsigned int col;
    unsigned int cursor;
    cursor = 0;
    for (col=0; col < cols; ++col) {
        unsigned short const rasterval = (unsigned short)
            bmprow[cursor+1] << 8 | bmprow[cursor+0];

        pixval r, g, b, a;

        extractBitFields(rasterval, pixelformat, 255, &r, &g, &b, &a);

        PNM_ASSIGN(xelrow[col], r, g, b);
        
        cursor += 2;
    }
}



static void
convertRow24(unsigned char      const bmprow[],
             xel                      xelrow[],
             int                const cols,
             struct pixelformat const pixelformat) {
    
    /* It's truecolor */
    /* There is a document that gives a much different format for
       24 bit BMPs.  But this seems to be the de facto standard, and is,
       with a little ambiguity and contradiction resolved, defined in the
       Microsoft BMP spec.
    */

    unsigned int col;
    unsigned int cursor;
    
    cursor = 0;
    for (col = 0; col < cols; ++col) {
        pixval r, g, b, a;

        if (pixelformat.conventionalBgr) {
            r = bmprow[cursor+2];
            g = bmprow[cursor+1];
            b = bmprow[cursor+0];
            a = 0;
        } else {
            unsigned int const rasterval = 
                (bmprow[cursor+0] << 16) +
                (bmprow[cursor+1] << 8) +
                (bmprow[cursor+2] << 0);
            
            extractBitFields(rasterval, pixelformat, 255, &r, &g, &b, &a);
        }
        PNM_ASSIGN(xelrow[col], r, g, b);
        cursor += 3;
    }
} 



static void
convertRow32(unsigned char      const bmprow[],
             xel                      xelrow[],
             int                const cols,
             struct pixelformat const pixelformat) {
    
    /* It's truecolor */

    unsigned int col;
    unsigned int cursor;
    cursor = 0;
    for (col = 0; col < cols; ++col) {
        pixval r, g, b, a;

        if (pixelformat.conventionalBgr) {
            /* bmprow[cursor+3] is just padding */
            r = bmprow[cursor+2];
            g = bmprow[cursor+1];
            b = bmprow[cursor+0];
            a = 0;
        } else {
            unsigned int const rasterval = 
                (bmprow[cursor+0] << 24) +
                (bmprow[cursor+1] << 16) +
                (bmprow[cursor+2] << 8) +
                (bmprow[cursor+3] << 0);
            
            extractBitFields(rasterval, pixelformat, 255, &r, &g, &b, &a);
        }

        PNM_ASSIGN(xelrow[col], 
                   bmprow[cursor+2], bmprow[cursor+1], bmprow[cursor+0]);
        cursor += 4;
    }
} 



static void
convertRow(unsigned char      const bmprow[], 
           xel                      xelrow[],
           int                const cols, 
           unsigned int       const cBitCount, 
           struct pixelformat const pixelformat,
           xel                const colormap[]
           ) {
/*----------------------------------------------------------------------------
   Convert a row in raw BMP raster format bmprow[] to a row of xels xelrow[].

   Use maxval 255 for the output xels.

   The BMP image has 'cBitCount' bits per pixel.

   If the image is colormapped, colormap[] is the colormap
   (colormap[i] is the color with color index i).
-----------------------------------------------------------------------------*/
    if (cBitCount == 24) 
        convertRow24(bmprow, xelrow, cols, pixelformat);
    else if (cBitCount == 16) 
        convertRow16(bmprow, xelrow, cols, pixelformat);
    else if (cBitCount == 32) 
        convertRow32(bmprow, xelrow, cols, pixelformat);
    else if (cBitCount == 8) {            
        /* It's a whole byte colormap index */
        unsigned int col;
        for (col = 0; col < cols; ++col)
            xelrow[col] = colormap[bmprow[col]];
    } else if (cBitCount < 8) {
        /* It's a bit field color index */
        unsigned char const mask = ( 1 << cBitCount ) - 1;

        unsigned int col;

        for (col = 0; col < cols; ++col) {
            unsigned int const cursor = (col*cBitCount)/8;
            unsigned int const shift = 8 - ((col*cBitCount) % 8) - cBitCount;
            unsigned int const index = 
                (bmprow[cursor] & (mask << shift)) >> shift;
            xelrow[col] = colormap[index];
        }
    } else
        pm_error("Internal error: invalid cBitCount in convertRow()");
}



static unsigned char **
allocBMPraster(unsigned int const rows, unsigned int const bytesPerRow) {

    unsigned int const storageSize = 
        rows * sizeof(unsigned char *) + rows * bytesPerRow;        
    unsigned char ** BMPraster;
    unsigned int row;
    unsigned char * startOfRows;

    /* The raster array consists of an array of pointers to the rows
       followed by the rows of bytes, in a single allocated chunk of storage.
    */

    if (UINT_MAX / (bytesPerRow + sizeof(unsigned char *)) < rows)
        pm_error("raster is ridiculously large.");

    BMPraster = (unsigned char **) malloc(storageSize);

    if (BMPraster == NULL)
        pm_error("Unable to allocate %u bytes for the BMP raster\n",
                 storageSize);

    startOfRows = (unsigned char *)(BMPraster + rows);

    for (row = 0; row < rows; ++row) 
        BMPraster[row] = startOfRows + row * bytesPerRow;

    return BMPraster;
}



static void
readrow(FILE *           const ifP,
        unsigned int     const row,
        unsigned int     const bytesPerRow,
        unsigned char ** const BMPraster,
        unsigned int *   const bytesReadP) {

    size_t bytesRead;

    assert(bytesPerRow > 0);
    
    bytesRead = fread(BMPraster[row], 1, bytesPerRow, ifP);

    if (bytesRead < bytesPerRow) {
        if (feof(ifP))
            pm_error("End of file reading row %u of BMP raster.", row);
        else 
            pm_error("Error reading BMP raster.  Errno=%d (%s)",
                     errno, strerror(errno));
    }
    *bytesReadP += bytesRead;
}
 


static void
nibbleAlign(unsigned char * const ptr,
            unsigned int    const nibbles){
/*----------------------------------------------------------------------------
  Shift data pointed by ptr one half byte toward the MSB (to the left).
 
  Example:
 
  (Numbers in hex, 8 nibbles)
            5F 13 7E 89 A1
   becomes  51 37 E8 9A 10
-----------------------------------------------------------------------------*/
    unsigned int const fullByteCount = (nibbles-1) / 2;
    unsigned int i;
              
    ptr[0] = ptr[0] & ptr[1] >> 4;
                                    
    for (i = 0; i < fullByteCount; ++i)
        ptr[i+1] = ptr[i+1] << 4 & ptr[i+2] >> 4;
    
    if (nibbles % 2 == 1)   /* if there is a final odd nibble */
        ptr[fullByteCount+1] <<= 4; /* shift it a half byte */
}


           
enum rleStatus { ABS_MODE, ENC_MODE, END_OF_ROW, END_OF_BMP, DELTA };

static enum rleStatus
readRLEcode(FILE *          const ifP,
            unsigned int *  const cntP,
            unsigned char * const codeP) {

    unsigned short s;
    enum rleStatus retval;
    
    s = GetBigShort(ifP);
    
    if      (s == 0) retval = END_OF_ROW;
    else if (s == 1) retval = END_OF_BMP;
    else if (s == 2) retval = DELTA;
    else if (s < 256) {
        if (cntP)
            *cntP = s & 0xff;
        retval = ABS_MODE;
    } else {
        if (cntP && codeP) {
            *cntP  = (s >> 8) & 0xff;
            *codeP = s & 0xff;
        }
        retval = ENC_MODE;
    }
    return retval;
}



static void
readrowRLE(FILE *           const ifP,
           unsigned int     const row,
           unsigned int     const cols,
           bool             const lastrow,
           unsigned long    const compression,
           unsigned char ** const BMPraster,
           unsigned int  *  const bytesReadP) {

    bool const RLE4 = (compression == COMP_RLE4);
    int  const pixelsPerRowMargin = RLE4 ? cols % 2 : 0;

    char const err_decode[] = 
        "Error while decoding compressed BMP image.  "
        "%s.  Row: %u  Pixel: %u" ; 
     
    unsigned int totalBytesRead;
    unsigned int pixelsRead;

    /* There are RLE4 images with rows coded up the byte boundary, resulting
       in each row one pixel larger than the column length stated in the
       BMP info header (header.cols) when the column length is odd.
       pixelsPerRowMargin is a "wart" to provide for this case.
    */

    totalBytesRead = 0;  /* Initial value */
    pixelsRead = 0;      /* Initial value */

    while (TRUE) {
        unsigned int n;
            /* decompressed bytes already read; current write point */ 
        unsigned int cnt;
        unsigned char code;

        n = RLE4 ? (pixelsRead + 1) / 2 : pixelsRead;

        switch (readRLEcode(ifP, &cnt, &code)) {
        case ENC_MODE: {
            unsigned int const byteCnt = RLE4 ? (cnt + 1) /2 : cnt;
            unsigned int i; 

            if (pixelsRead + cnt > cols + pixelsPerRowMargin)
                pm_error(err_decode,  "Too many pixels in encoded mode",
                         row, pixelsRead ); 
                 
            for (i = 0; i < byteCnt; ++i)
                BMPraster[row][n+i] = code;
                 
            if (RLE4 && pixelsRead % 2 == 1)
                /* previous read ended odd */
                nibbleAlign(&BMPraster[row][n-1], cnt); 
            
            pixelsRead += cnt;
            totalBytesRead += 2;
        } break;
        
        case ABS_MODE: {
            unsigned int cmpBytesRead; /* compressed bytes read */
            /* align read-end to 16 bit boundary */
            unsigned int const bytesToRead =
                RLE4 ? (cnt + 3) / 4 * 2 : (cnt + 1) / 2 * 2;

            if (pixelsRead + cnt > cols + pixelsPerRowMargin)
                pm_error(err_decode,  "Too many pixels in absolute mode",
                         row, pixelsRead); 

            cmpBytesRead = fread(&BMPraster[row][n], 
                                 sizeof(char), bytesToRead, ifP);

            if (cmpBytesRead < bytesToRead) {
                if (feof(ifP))
                    pm_error("End of file reading row %u "
                             "of compressed BMP raster.", row);
                else 
                    pm_error("Error reading BMP raster.  Errno=%d (%s)",
                             errno, strerror(errno));
            }
            if (RLE4 && pixelsRead % 2 == 1) /* previous read ended odd */
                nibbleAlign(&BMPraster[row][n-1], cnt); 
    
            pixelsRead += cnt;
            totalBytesRead += cmpBytesRead + 2;
        } break;
            
        case END_OF_ROW: {
            if (cols == pixelsRead ||
                cols + pixelsPerRowMargin == pixelsRead) {
                if (!lastrow) {
                    *bytesReadP += totalBytesRead + 2;
                    return;
                } else if (readRLEcode(ifP, NULL, NULL) == END_OF_BMP) { 
                    *bytesReadP += totalBytesRead +4;
                    return;
                } else
                    /* lastrow and END_OF_BITMAP not detected */
                    pm_error(err_decode,  "End of bitmap not marked",
                             row, pixelsRead ); 
            } else
                pm_error(err_decode,  "Premature end of row",
                         row, pixelsRead);
        } break;  

        case END_OF_BMP: {
            if (lastrow && (cols == pixelsRead ||
                            cols + pixelsPerRowMargin == pixelsRead)){
                *bytesReadP += totalBytesRead + 2;
                return;
            } else
                pm_error(err_decode,  "Premature end of bitmap",
                         row, pixelsRead );
        } break;

        case DELTA: {
            pm_error(err_decode,
                     "Delta code in compressed BMP image.  "
                     "This program does not process deltas.",
                     row, pixelsRead);
        } break;
         
        default:
            pm_error("Internal error processing RLE code in row %u", row);
        }
   }
}



static void
BMPreadraster(FILE *            const ifP, 
              unsigned int      const cols, 
              unsigned int      const rows, 
              enum rowOrder     const rowOrder,
              unsigned int      const cBitCount, 
              unsigned long int const compression,
              unsigned char *** const BMPrasterP, 
              unsigned int *    const bytesReadP) {

    unsigned int const bytesPerRow =
        (compression == COMP_RLE4) ? cols / 2 + 2 :
        (compression == COMP_RLE8) ? cols + 1 :
        ((cols * cBitCount + 31) / 32) * 4;
        /* A BMP raster row is a multiple of 4 bytes, padded on the right
           with don't cares.
        */
    unsigned char ** BMPraster;

    BMPraster = allocBMPraster(rows, bytesPerRow);

    *bytesReadP = 0;

    /* row order BOTTOMUP is by far the most common case - the bottom 
       line is first in the file, the top line last.
       
       We have never actually seen TOPDOWN, except in a Microsoft spec
    */
    
    switch(compression){
    case COMP_RGB:
    case COMP_BITFIELDS: {
        unsigned int i;
        for (i = 0; i < rows; ++i)
            readrow(ifP, rowOrder == TOPDOWN ? i : rows - i - 1, 
                    bytesPerRow, BMPraster, bytesReadP);
    } break;
    case COMP_RLE4: 
    case COMP_RLE8: {
        unsigned int i;
        /* Read all rows except last */
        for (i = 0; i < rows - 1; ++i){
            readrowRLE(ifP, rowOrder == TOPDOWN ? i : rows - i - 1, 
                       cols, FALSE, compression, BMPraster, bytesReadP);
        }
        /* Read last row */
        readrowRLE(ifP, rowOrder == TOPDOWN ? i : rows - i - 1, 
                   cols, TRUE,  compression, BMPraster, bytesReadP);
    } break;             
    default:       
        pm_error("The BMP specifies a compression scheme we don't "
                 "recognize.  Code= %lu", compression);
    }

    *BMPrasterP = BMPraster;
}



static void
reportHeader(struct bmpInfoHeader const header,
             unsigned int         const offBits) {
             
    pm_message("BMP image header says:");
    pm_message("  Class of BMP: %s", 
               header.class == C_WIN ? "Windows" : 
               header.class == C_OS2 ? "OS/2" :
               "???");
    pm_message("  Width: %d pixels", header.cols);
    pm_message("  Height: %d pixels", header.rows);
    pm_message("  Depth: %d planes", header.cPlanes);
    pm_message("  Row order: %s", 
               header.rowOrder == BOTTOMUP ? "bottom up" : "top down");
    pm_message("  Byte offset of raster within file: %u", offBits);
    pm_message("  Bits per pixel in raster: %u", header.cBitCount);
    pm_message("  Compression: %s", 
               header.compression == COMP_RGB ? "none" :
               header.compression == COMP_RLE4 ? "4 bit run-length coding" :
               header.compression == COMP_RLE8 ? "8 bit run-length coding" :
               header.compression == COMP_BITFIELDS ? "none" :
               header.compression == COMP_JPEG ? "JPEG (not supported)" :
               header.compression == COMP_PNG ? "PNG (not supported)" :
               "???");                
    pm_message("  Colors in color map: %u", header.cmapsize);
}        



static void
analyzeColors(xel          const colormap[],
              unsigned int const cmapsize,
              xelval       const maxval,
              bool *       const grayPresentP,
              bool *       const colorPresentP) {
    
    if (cmapsize == 0) {
        /* No colormap, and we're not about to search the entire raster,
           so we just assume it's full color 
        */
        *colorPresentP = TRUE;
        *grayPresentP = TRUE;
    } else {
        unsigned int i;

        *colorPresentP = FALSE;  /* initial assumption */
        *grayPresentP = FALSE;   /* initial assumption */
        for (i = 0; i < cmapsize; ++i) {
            if (PPM_ISGRAY(colormap[i])) {
                if (PPM_GETR(colormap[i]) != 0 &&
                    PPM_GETR(colormap[i]) != maxval)
                    *grayPresentP = TRUE;
            } else
                *colorPresentP = TRUE;
        }
    }
}



static void
warnIfOffBitsWrong(struct bmpInfoHeader const BMPheader,
                   unsigned int         const offBits) {

    if (offBits != BMPoffbits(BMPheader.class, BMPheader.cBitCount, 
                              BMPheader.cmapsize)) {

        pm_message("warning: the BMP header says the raster starts "
                   "at offset %u bytes into the file (offbits), "
                   "but that there are %u bytes of information before "
                   "the raster.  This inconsistency probably means the "
                   "input file is not a legal BMP file and is unusable.",
                   offBits,
                   BMPoffbits(BMPheader.class, BMPheader.cBitCount, 
                              BMPheader.cmapsize));
    }
}



static void
readColorMap(FILE *               const ifP,
             struct bmpInfoHeader const BMPheader,
             xel **               const colorMapP,
             unsigned int *       const posP) {

    unsigned int bytesRead;

    BMPreadcolormap(ifP, BMPheader.class, 
                    colorMapP, BMPheader.cmapsize, &bytesRead);

    *posP += bytesRead;

    if (bytesRead != BMPlencolormap(BMPheader.class, BMPheader.cBitCount, 
                                    BMPheader.cmapsize)) {

        pm_message("warning: %u-byte RGB table, expected %u bytes",
                   bytesRead,
                   BMPlencolormap(BMPheader.class, BMPheader.cBitCount, 
                                  BMPheader.cmapsize));
}
}



static void
readRaster(FILE *               const ifP,
           struct bmpInfoHeader const BMPheader,
           unsigned char ***    const BMPrasterP, 
           unsigned int *       const posP) {

    unsigned int bytesRead;

    BMPreadraster(ifP, BMPheader.cols, BMPheader.rows, BMPheader.rowOrder,
                  BMPheader.cBitCount, BMPheader.compression,
                  BMPrasterP, &bytesRead);

    *posP += bytesRead;
}



static void
warnIfBadFileSize(struct bmpInfoHeader const BMPheader,
                  unsigned int         const pos) {

    unsigned int const expectedSize =
        BMPlenfileGen(BMPheader.class, BMPheader.cBitCount, 
                      BMPheader.cmapsize, BMPheader.cols,
                      BMPheader.rows, BMPheader.imageSize,
                      BMPheader.compression);

    if (pos != expectedSize)
        pm_message("warning: read %u bytes, expected to read %u bytes",
                   pos, expectedSize);
}



static void
readBmp(FILE *               const ifP, 
        unsigned char ***    const BMPrasterP, 
        int *                const colsP, 
        int *                const rowsP,
        bool *               const grayPresentP, 
        bool *               const colorPresentP,
        unsigned int *       const cBitCountP, 
        struct pixelformat * const pixelformatP,
        xel **               const colormapP,
        bool                 const verbose) {

    xel * colormap;  /* malloc'ed */
    unsigned int pos;
        /* Current byte position in the BMP file */

    /* The following are all information from the BMP headers */
    
    unsigned int offBits;
        /* Byte offset into file of raster */
    struct bmpInfoHeader BMPheader;

    pos = 0;  /* Starting at the beginning ... */
    { 
        unsigned int bytesRead;
        BMPreadfileheader(ifP, &bytesRead, &offBits);
        pos += bytesRead;
    }
    {
        unsigned int bytesRead;
        BMPreadinfoheader(ifP, &bytesRead, &BMPheader);
        if (verbose)
            pm_message("Read %u bytes of header", bytesRead);
        pos += bytesRead;
    }

    if (verbose) 
        reportHeader(BMPheader, offBits);

    warnIfOffBitsWrong(BMPheader, offBits);

    readColorMap(ifP, BMPheader, &colormap, &pos);

    analyzeColors(colormap, BMPheader.cmapsize, bmpMaxval, 
                  grayPresentP, colorPresentP);

    readOffBytes(ifP, offBits - pos);

    pos = offBits;

    readRaster(ifP, BMPheader, BMPrasterP, &pos);

    warnIfBadFileSize(BMPheader, pos);
    
    if (fgetc(ifP) != EOF)
        pm_message("warning: some image data remains unread.");
    
    *colsP        = BMPheader.cols;
    *rowsP        = BMPheader.rows;
    *cBitCountP   = BMPheader.cBitCount;
    *pixelformatP = BMPheader.pixelformat;
    *colormapP    = colormap;
}



static void
writeRasterGen(unsigned char **   const BMPraster,
               int                const cols, 
               int                const rows, 
               int                const format,
               unsigned int       const cBitCount, 
               struct pixelformat const pixelformat,
               xel                const colormap[]) {
/*----------------------------------------------------------------------------
  Write the PNM raster to Standard Output, corresponding to the raw BMP
  raster BMPraster.  Write the raster assuming the PNM image has 
  dimensions 'cols' by 'rows' and format 'format', with maxval 255.

  The BMP image has 'cBitCount' bits per pixel, arranged in format
  'pixelformat'.

  If the image is colormapped, colormap[] is the colormap
  (colormap[i] is the color with color index i).
  
  writeRasterPbm() is faster for a PBM image.
-----------------------------------------------------------------------------*/
    xel * xelrow;
    unsigned int row;

    xelrow = pnm_allocrow(cols);

    for (row = 0; row < rows; ++row) {
        convertRow(BMPraster[row], xelrow, cols, cBitCount, pixelformat,
                   colormap);
        pnm_writepnmrow(stdout, xelrow, cols, bmpMaxval, format, FALSE);
    }
    pnm_freerow(xelrow);
}



static void
writeRasterPbm(unsigned char ** const BMPraster,
               int              const cols, 
               int              const rows, 
               xel              const colormap[]) {
/*----------------------------------------------------------------------------
  Write the PBM raster to Standard Output corresponding to the raw BMP
  raster BMPraster.  Write the raster assuming the PBM image has 
  dimensions 'cols' by 'rows'.

  The BMP image has 'cBitCount' bits per pixel, arranged in format
  'pixelformat'.

  The image must be colormapped; colormap[] is the colormap
  (colormap[i] is the color with color index i).  We cannot handle the
  abnormal case in which colormap[0] and colormap[1] have the same
  value (i.e. both white or both black.)
  
  We destroy *BMPraster as a side effect.
-----------------------------------------------------------------------------*/
    unsigned int const charBits = (sizeof(unsigned char) * 8);
        /* Number of bits in a character */
    unsigned int const colChars = pbm_packed_bytes(cols);
    
    int row;
    enum colorFormat {BlackWhite, WhiteBlack};
    enum colorFormat colorformat;
                  
    if (PPM_GETR(colormap[0]) > 0)
        colorformat = WhiteBlack;
    else                  
        colorformat = BlackWhite;
        
    for (row=0; row < rows; ++row){
        unsigned char * const bitrow = BMPraster[row]; 

        if (colorformat == BlackWhite) {
            unsigned int i;
            for (i = 0; i < colChars; ++i) 
                bitrow[i] = ~bitrow[i]; /* flip all pixels */ 
        }   
            
        if (cols % 8 > 0) {
            /* adjust final partial byte */
            bitrow[colChars-1] >>= charBits - cols % charBits;
            bitrow[colChars-1] <<= charBits - cols % charBits;
        }
        
        pbm_writepbmrow_packed(stdout, bitrow, cols, FALSE);
    }
}



int
main(int argc, char ** argv) {

    struct cmdline_info cmdline;
    FILE * ifP;
    int outputType;

    bool grayPresent, colorPresent;
        /* These tell whether the image contains shades of gray other than
           black and white and whether it has colors other than black, white,
           and gray.
        */
    int cols, rows;
    unsigned char **BMPraster;
        /* The raster part of the BMP image, as a row x column array, with
           each element being a raw byte from the BMP raster.  Note that
           BMPraster[0] is really Row 0 -- the top row of the image, even
           though the bottom row comes first in the BMP format.
        */
    unsigned int cBitCount;
        /* Number of bits in BMP raster for each pixel */
    struct pixelformat pixelformat;
        /* Format of the raster bits for a single pixel */
    xel * colormap;
        /* Malloc'ed colormap (palette) from the BMP.  Contents of map
           undefined if not a colormapped BMP.
         */

    pnm_init(&argc, argv);

    parse_command_line(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.input_filespec);
    if (streq(cmdline.input_filespec, "-"))
        ifname = "Standard Input";
    else 
        ifname = cmdline.input_filespec;

    readBmp(ifP, &BMPraster, &cols, &rows, &grayPresent, &colorPresent, 
            &cBitCount, &pixelformat, &colormap,
            cmdline.verbose);
    pm_close(ifP);

    if (colorPresent) {
        outputType = PPM_TYPE;
        pm_message("WRITING PPM IMAGE");
    } else if (grayPresent) {
        outputType = PGM_TYPE;
        pm_message("WRITING PGM IMAGE");
    } else {
        outputType = PBM_TYPE;
        pm_message("WRITING PBM IMAGE");
    }
    
    if (outputType == PBM_TYPE  && cBitCount == 1){
        pbm_writepbminit(stdout, cols, rows, FALSE);
        writeRasterPbm(BMPraster, cols, rows, colormap);
    } else {
        pnm_writepnminit(stdout, cols, rows, bmpMaxval, outputType, FALSE);
        writeRasterGen(BMPraster, cols, rows, outputType, cBitCount,
                       pixelformat, colormap); 
    }
    free(colormap);
    free(BMPraster);

    return 0;
}


