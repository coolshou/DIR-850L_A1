/*
** pnmtotiff.c - converts a PNM image to a TIFF (Tagged Image File) image
**
** Derived by Jef Poskanzer from ras2tif.c, which is:
**
** Copyright (c) 1990 by Sun Microsystems, Inc.
**
** Author: Patrick J. Naughton
** naughton@wind.sun.com
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted,
** provided that the above copyright notice appear in all copies and that
** both that copyright notice and this permission notice appear in
** supporting documentation.
**
** This file is provided AS IS with no warranties of any kind.  The author
** shall have no liability with respect to the infringement of copyrights,
** trade secrets or any patents by this file or any part thereof.  In no
** event will the author be liable for any lost revenue or profits or
** other special, indirect and consequential damages.
*/

#define _XOPEN_SOURCE    /* Make sure stdio.h contains fileno() */
#define _BSD_SOURCE      /* Make sure string.h contains strcasecmp() */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#ifdef VMS
#ifdef SYSV
#undef SYSV
#endif
#include <tiffioP.h>
#endif
/* tiffio.h has a weird problem on AIX.  tiffio.h wrongly typedefs
   "int32".  That's wrong because such a name is likely to be used in
   other parts of the program that includes tiffio.h.  And in fact, on
   AIX, <inttypes.h> defines it in some cases.  One case in which <inttypes.h>
   does _not_ define int32 is where _XOPEN_SOURCE is declared.  So we always
   do that when including tiffio.h.  06.01.05
*/
#include <tiffio.h>

#include <string.h>

#include "pm_c_util.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"
#include "pam.h"
#include "pammap.h"
#include "pm_tiff.h"

#define MAXCOLORS 256

/* 2001.09.03: Old tiff.h doesn't contain these values: */
#ifndef COMPRESSION_ADOBE_DEFLATE
#define COMPRESSION_ADOBE_DEFLATE 8
#endif

struct sizeset {
    bool b1, b2, b4, b8;
};


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespecs of input files */
    int compression;
        /* COMPRESSION Tiff tag value, that corresponds to the compression
           option the user specified, or -1 if he didn't specify any.
           The -tag option doesn't count.
        */
    int fillorder;     /* FILLORDER Tiff tag value */
    int g3options;     /* G3OPTIONS Tiff tag value or 0 for none */
    int predictor;     /* PREDICTOR Tiff tag value or -1 for none */
    int rowsperstrip;  /* -1 = unspecified */
    unsigned int minisblack;   /* logical: User wants MINISBLACK photometric */
    unsigned int miniswhite;   /* logical: User wants MINISWHITE photometric */
    unsigned int truecolor;    /* logical: User wants truecolor, not cmap */
    unsigned int color;        /* logical: assume not grayscale */
    float xresolution; /* XRESOLUTION Tiff tag value or -1 for none */
    float yresolution; /* YRESOLUTION Tiff tag value or -1 for none */
    int resolutionUnit;  /* RESOLUTIONUNIT Tiff tag value */
    struct sizeset indexsizeAllowed;
    /* Which bit widths are allowable in a raster of palette indices */
    unsigned int verbose;
    unsigned int append;
    float resolution;  /* X and Y resolution */
    struct optNameValue * taglist;
};



static void
validateTagList(struct optNameValue const taglist[]) {

    unsigned int i;
    for (i = 0; taglist[i].name; ++i) {
        const char * const tagName = taglist[i].name;
        const tagDefinition * tagDefP = tagDefFind(tagName);
        
        if (!tagDefP)
            pm_error("Unknown tag name '%s'", tagName);
        else {
            /* We don't allow the user to set directly any tag that we
               control explicitly.  These are normally tags that have to be
               consistent with other stuff we put in the image.  But in some
               cases, they're just tags we had special options for before
               -tag existed.  The latter should probably be converted
               eventually to prefer -tag.
            */
            switch (tagDefP->tagnum) {
            case TIFFTAG_IMAGEWIDTH:
            case TIFFTAG_IMAGELENGTH:
            case TIFFTAG_BITSPERSAMPLE:
            case TIFFTAG_COMPRESSION:
            case TIFFTAG_GROUP3OPTIONS:
            case TIFFTAG_PREDICTOR:
            case TIFFTAG_PHOTOMETRIC:
            case TIFFTAG_FILLORDER:
            case TIFFTAG_SAMPLESPERPIXEL:
            case TIFFTAG_ROWSPERSTRIP:
            case TIFFTAG_PLANARCONFIG:
            case TIFFTAG_COLORMAP:
            case TIFFTAG_RESOLUTIONUNIT:
            case TIFFTAG_XRESOLUTION:
            case TIFFTAG_YRESOLUTION:
                pm_error("You cannot specify a '%s' tag with -tag.  "
                         "Pamtotiff controls that internally or via other "
                         "options.", tagName);
            }
        }
    }
}



static void
parseCommandLine(int                        argc,
                 char **              const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry * option_def;

    optStruct3 opt;

    unsigned int none, packbits, lzw, g3, g4, msb2lsb, lsb2msb, opt_2d, fill;
    unsigned int flate, adobeflate;
    char * indexbits;
    char * resolutionUnit;

    unsigned int predictorSpec, rowsperstripSpec, xresolutionSpec,
        yresolutionSpec, indexbitsSpec, resolutionUnitSpec, tagSpec;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "verbose",      OPT_FLAG,   NULL, &cmdlineP->verbose,       0);
    OPTENT3(0, "append",       OPT_FLAG,   NULL, &cmdlineP->append,        0);
    OPTENT3(0, "none",         OPT_FLAG,   NULL, &none,                    0);
    OPTENT3(0, "packbits",     OPT_FLAG,   NULL, &packbits,                0);
    OPTENT3(0, "lzw",          OPT_FLAG,   NULL, &lzw,                     0);
    OPTENT3(0, "g3",           OPT_FLAG,   NULL, &g3,                      0);
    OPTENT3(0, "g4",           OPT_FLAG,   NULL, &g4,                      0);
    OPTENT3(0, "flate",        OPT_FLAG,   NULL, &flate,                   0);
    OPTENT3(0, "adobeflate",   OPT_FLAG,   NULL, &adobeflate,              0);
    OPTENT3(0, "msb2lsb",      OPT_FLAG,   NULL, &msb2lsb,                 0);
    OPTENT3(0, "lsb2msb",      OPT_FLAG,   NULL, &lsb2msb,                 0);
    OPTENT3(0, "2d",           OPT_FLAG,   NULL, &opt_2d,                  0);
    OPTENT3(0, "fill",         OPT_FLAG,   NULL, &fill,                    0);
    OPTENT3(0, "minisblack",   OPT_FLAG,   NULL, &cmdlineP->minisblack,    0);
    OPTENT3(0, "mb",           OPT_FLAG,   NULL, &cmdlineP->minisblack,    0);
    OPTENT3(0, "miniswhite",   OPT_FLAG,   NULL, &cmdlineP->miniswhite,    0);
    OPTENT3(0, "mw",           OPT_FLAG,   NULL, &cmdlineP->miniswhite,    0);
    OPTENT3(0, "truecolor",    OPT_FLAG,   NULL, &cmdlineP->truecolor,     0);
    OPTENT3(0, "color",        OPT_FLAG,   NULL, &cmdlineP->color,         0);
    OPTENT3(0, "predictor",    OPT_UINT,   &cmdlineP->predictor,    
            &predictorSpec,    0);
    OPTENT3(0, "rowsperstrip", OPT_UINT,   &cmdlineP->rowsperstrip, 
            &rowsperstripSpec, 0);
    OPTENT3(0, "xresolution",  OPT_FLOAT,  &cmdlineP->xresolution,  
            &xresolutionSpec,  0);
    OPTENT3(0, "yresolution",  OPT_FLOAT,  &cmdlineP->yresolution,  
            &yresolutionSpec,  0);
    OPTENT3(0, "resolutionunit", OPT_STRING, &resolutionUnit,
            &resolutionUnitSpec,    0);
    OPTENT3(0, "indexbits",    OPT_STRING,   &indexbits, 
            &indexbitsSpec,    0);
    OPTENT3(0, "tag",          OPT_NAMELIST, &cmdlineP->taglist, &tagSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
    /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (none + packbits + lzw + g3 + g4 + flate + adobeflate > 1)
        pm_error("You specified more than one compression option.  "
                 "Only one of -none, -packbits, -lze, -g3, and -g4 "
                 "is allowed.");
    
    if (none)
        cmdlineP->compression = COMPRESSION_NONE;
    else if (packbits)
        cmdlineP->compression = COMPRESSION_PACKBITS;
    else if (lzw)
        cmdlineP->compression = COMPRESSION_LZW;
    else if (g3)
        cmdlineP->compression = COMPRESSION_CCITTFAX3;
    else if (g4)
        cmdlineP->compression = COMPRESSION_CCITTFAX4;
    else if (adobeflate)
        cmdlineP->compression = COMPRESSION_ADOBE_DEFLATE;
    else if (flate)
        cmdlineP->compression = COMPRESSION_DEFLATE;
    else
        cmdlineP->compression = COMPRESSION_NONE;
    
    if (msb2lsb + lsb2msb > 1)
        pm_error("You specified both -msb2lsb and -lsb2msb.  "
                 "These are conflicting options.");

    if (msb2lsb)
        cmdlineP->fillorder = FILLORDER_MSB2LSB;
    else if (lsb2msb)
        cmdlineP->fillorder = FILLORDER_LSB2MSB;
    else 
        cmdlineP->fillorder = FILLORDER_MSB2LSB;
    

    if (cmdlineP->miniswhite && cmdlineP->minisblack)
        pm_error("You cannot specify both -miniswhite and -minisblack");

    cmdlineP->g3options = 0;  /* initial value */
    if (opt_2d)
        cmdlineP->g3options |= GROUP3OPT_2DENCODING;
    if (fill)
        cmdlineP->g3options |= GROUP3OPT_FILLBITS;

    if (predictorSpec) {
        if (cmdlineP->predictor != 1 && cmdlineP->predictor != 2)
            pm_error("-predictor may be only 1 or 2.  You specified %d.", 
                     cmdlineP->predictor);
    } else
        cmdlineP->predictor = -1;

    if (rowsperstripSpec) {
        if (cmdlineP->rowsperstrip < 1)
            pm_error("-rowsperstrip must be positive.  You specified %d.",
                     cmdlineP->rowsperstrip);
    } else
        cmdlineP->rowsperstrip = -1;

    if (xresolutionSpec) {
        if (cmdlineP->xresolution < 1)
            pm_error("-xresolution must be positive.  You specified %f.",
                     cmdlineP->xresolution);
    } else
        cmdlineP->xresolution = -1;

    if (yresolutionSpec) {
        if (cmdlineP->yresolution < 1)
            pm_error("-yresolution must be positive.  You specified %f.",
                     cmdlineP->yresolution);
    } else
        cmdlineP->yresolution = -1;

    if (resolutionUnitSpec) {
        if (streq(resolutionUnit, "inch"))
            cmdlineP->resolutionUnit = RESUNIT_INCH;
        else if (streq(resolutionUnit, "in"))
            cmdlineP->resolutionUnit = RESUNIT_INCH;
        else if (streq(resolutionUnit, "centimeter"))
            cmdlineP->resolutionUnit = RESUNIT_CENTIMETER;
        else if (streq(resolutionUnit, "cm"))
            cmdlineP->resolutionUnit = RESUNIT_CENTIMETER;
        else if (streq(resolutionUnit, "none"))
            cmdlineP->resolutionUnit = RESUNIT_NONE;
        else if (streq(resolutionUnit, "no"))
            cmdlineP->resolutionUnit = RESUNIT_NONE;
        else
            pm_error("The only acceptable values for -resolutionunit are "
                     "inch, centimeter, none, in, cm, and no.  "
                     "You specified '%s'.", resolutionUnit);
    } else
        cmdlineP->resolutionUnit = RESUNIT_INCH;

    if (indexbitsSpec) {
        if (strstr(indexbits, "1"))
            cmdlineP->indexsizeAllowed.b1 = TRUE;
        else
            cmdlineP->indexsizeAllowed.b1 = FALSE;
        if (strstr(indexbits, "2"))
            cmdlineP->indexsizeAllowed.b2 = TRUE;
        else
            cmdlineP->indexsizeAllowed.b2 = FALSE;
        if (strstr(indexbits, "4"))
            cmdlineP->indexsizeAllowed.b4 = TRUE;
        else
            cmdlineP->indexsizeAllowed.b4 = FALSE;
        if (strstr(indexbits, "8"))
            cmdlineP->indexsizeAllowed.b8 = TRUE;
        else
            cmdlineP->indexsizeAllowed.b8 = FALSE;
    } else {
        cmdlineP->indexsizeAllowed.b1 = FALSE;
        cmdlineP->indexsizeAllowed.b2 = FALSE;
        cmdlineP->indexsizeAllowed.b4 = FALSE;
        cmdlineP->indexsizeAllowed.b8 = TRUE;
    }

    if (tagSpec)
        validateTagList(cmdlineP->taglist);
    else {
        MALLOCARRAY_NOFAIL(cmdlineP->taglist, 1);
        cmdlineP->taglist[0].name = NULL;
        cmdlineP->taglist[0].value = NULL;
    }

    if (argc-1 == 0) 
        cmdlineP->input_filespec = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->input_filespec = argv[1];
}



static void
putSample(sample           const s,
          sample           const maxval,
          sample           const tiff_maxval,
          unsigned int     const bitspersample,
          unsigned char ** const tPP) {

    xelval s2;

    s2 = s;
    if (maxval != tiff_maxval)
        s2 = s * tiff_maxval / maxval;
    if (bitspersample > 8) {
        *((unsigned short *)(*tPP)) = s2;
        (*tPP) += sizeof(short);
    } else
        *(*tPP)++ = s2 & 0xff;
}




/* Note: PUTSAMPLE doesn't work if bitspersample is 1-4. */

#define PUTSAMPLE putSample(s, maxval, tiff_maxval, bitspersample, &tP);



static void
fillRowOfSubBytePixels(struct pam *    const pamP,
                       const tuple *   const tuplerow,
                       unsigned char * const buf,
                       unsigned short  const tiff_maxval,
                       unsigned short  const photometric,
                       int             const fillorder,
                       int             const bitspersample,
                       tuplehash       const cht) {
/*----------------------------------------------------------------------------
  This subroutine deals with cases where multiple pixels are packed into
  a single byte of the Tiff output image, i.e. bits per pixel < 8.
-----------------------------------------------------------------------------*/
    int col;
    int bitshift;
        /* The number of bits we have to shift a pixel value left to line
           it up with where the current pixel goes in the current byte of
           the output buffer.  
        */
    int const firstbitshift = 
        (fillorder == FILLORDER_MSB2LSB) ? 8 - bitspersample : 0;
        /* The value of 'bitshift' for the first pixel into a
           byte of the output buffer.  (MSB2LSB is normal).
        */
    sample s;
        /* The value of the current sample */
    unsigned char* tP;
        /* The address of the byte in the output buffer in which
           the current pixel goes.
        */
    unsigned char byte;
        /* The under-construction value of the byte pointed to by
           tP, above.
        */
                
    bitshift = firstbitshift;
    byte = 0;
    for (col = 0, tP = buf; col < pamP->width; ++col) {
        if (cht == NULL) {
            /* It's grayscale */
            s = tuplerow[col][0];
            if (pamP->maxval != tiff_maxval )
                s = (long) s * tiff_maxval / pamP->maxval;
 
            if (photometric == PHOTOMETRIC_MINISWHITE)
                s = tiff_maxval - s;
        } else {
            /* It's a colormapped raster */
            int si;
            int found;
            pnm_lookuptuple(pamP, cht, tuplerow[col], &found, &si);
            if (!found)
                pm_error("INTERNAL ERROR.  We made a color map, and a "
                         "color map we need is not in it!.  "
                         "col=%d  r=%lu g=%lu b=%lu",
                         col, tuplerow[col][0], tuplerow[col][1],
                         tuplerow[col][2]);
            s = (unsigned char) si;
        }
        byte |= s << bitshift;
        if (fillorder == FILLORDER_MSB2LSB)
            bitshift -= bitspersample;  /* normal case */
        else
            bitshift += bitspersample;
        if (bitshift < 0 || bitshift > 7) {
            *tP++ = byte;
            bitshift = firstbitshift;
            byte = 0;
        }
    }
    if (bitshift != firstbitshift)
        *tP++ = byte;
}



static void
fillRowOfWholeBytePixels(struct pam *    const pamP,
                         tuple *         const tuplerow,
                         unsigned char * const buf,
                         unsigned short  const photometric,
                         unsigned short  const tiffMaxval,
                         unsigned int    const bitsPerSample) {

    unsigned int col;
    unsigned char * tP;
    unsigned int planes;
    
    if (photometric == PHOTOMETRIC_RGB)
        planes = pamP->depth;
    else
        /* Write only the first plane from the PAM, even if there
           are more.
        */
        planes = 1;

    for (col = 0, tP = buf; col < pamP->width; ++col) {
        unsigned int plane;
        for (plane = 0; plane < planes; ++plane) {
            putSample(tuplerow[col][plane], pamP->maxval,
                      tiffMaxval, bitsPerSample, &tP);
            /* Advances tP */
        }
    }
} 



static void
writeScanLines(struct pam *   const pamP,
               TIFF *         const tif, 
               tuplehash      const cht,
               unsigned short const tiffMaxval,
               unsigned short const bitspersample, 
               unsigned short const photometric,
               int            const bytesperrow, 
               int            const fillorder) {
/*----------------------------------------------------------------------------
   Write out the raster for the input image described by 'pamP', whose
   file is positioned to the raster of the image.
-----------------------------------------------------------------------------*/
    tuple * tuplerow;  /* malloc'ed */
    unsigned char * buf; /* malloc'ed */
    int row;

    /* The man page for TIFFWriteScanLine doesn't tell the format of
       it's 'buf' parameter, but here it is: Its format depends on the
       bits per pixel of the TIFF image.  If it's 16, 'buf' is an
       array of short (16 bit) integers, one per raster column.  If
       it's 8, 'buf' is an array of characters (8 bit integers), one
       per image column.  If it's less than 8, it's an array of characters,
       each of which represents 1-8 raster columns, packed
       into it in the order specified by the TIFF image's fill order,
       with don't-care bits on the right such that each byte contains only
       whole pixels.

       In all cases, the array elements are in order left to right going
       from low array indices to high array indices.
    */
    MALLOCARRAY(buf, bytesperrow);

    if (buf == NULL)
        pm_error("can't allocate memory for row buffer");

    tuplerow = pnm_allocpamrow(pamP);
    
    for (row = 0; row < pamP->height; ++row) {
        int col;

        pnm_readpamrow(pamP, tuplerow);

        if (cht == NULL) {
            /* It's a direct, non-colormapped raster */

            if (bitspersample == 8 || bitspersample == 16)
                fillRowOfWholeBytePixels(pamP, tuplerow, buf, photometric,
                                         tiffMaxval, bitspersample);
            else
                fillRowOfSubBytePixels(pamP, tuplerow, buf,
                                       tiffMaxval, photometric,
                                       fillorder, bitspersample, NULL);
        } else {
            /* It's a colormapped raster */
            if (bitspersample == 8) {
                for (col = 0; col < pamP->width; ++col) {
                    int si;
                    int found;
                    
                    pnm_lookuptuple(pamP, cht, tuplerow[col], &found, &si);
                    
                    if (!found)
                        pm_error("INTERNAL ERROR.  We made a color map, and a "
                                 "color map we need is not in it!  "
                                 "col=%d  r=%lu g=%lu b=%lu",
                                 col, tuplerow[col][0], tuplerow[col][1],
                                 tuplerow[col][2]);
                    buf[col] = (unsigned char) si;
                }
            } else {
                fillRowOfSubBytePixels(pamP, tuplerow, buf, 0, 0, fillorder,
                                       bitspersample, cht);
            }
        }
        if (TIFFWriteScanline(tif, buf, row, 0) < 0)
            pm_error("failed a scanline write on row %d", row);
    }
    pnm_freepamrow(tuplerow);
    free(buf);
}



static void
analyzeColorsInRgbInput(struct pam *        const pamP,
                        struct cmdlineInfo  const cmdline,
                        int                 const maxcolors, 
                        tupletable *        const chvP, 
                        unsigned int *      const colorsP, 
                        bool *              const grayscaleP) {
/*----------------------------------------------------------------------------
   Same as analyzeColors(), except assuming input image has R/G/B tuples.
-----------------------------------------------------------------------------*/
    if (cmdline.color && cmdline.truecolor) {
        *chvP = NULL;
        *grayscaleP = FALSE;
    } else {
        tupletable chv;
        bool grayscale;

        pm_message("computing colormap...");
        chv = pnm_computetuplefreqtable2(pamP, NULL, maxcolors,
                                         pamP->maxval,
                                         colorsP);
        if (chv == NULL) {
            grayscale = FALSE;
        } else {
            unsigned int i;
            pm_message("%u color%s found", 
                       *colorsP, *colorsP == 1 ? "" : "s");
            grayscale = TRUE;  /* initial assumption */
            for (i = 0; i < *colorsP && grayscale; ++i) {
                if (!pnm_rgbtupleisgray(chv[i]->tuple))
                    grayscale = FALSE;
            }
        }
        *grayscaleP = grayscale;

        if (grayscale || cmdline.truecolor) {
            *chvP = NULL;
            if (chv)
                pnm_freetupletable(pamP, chv);
        } else {
            /* He wants a colormap.  But can we give it to him? */
            if (chv)
                *chvP = chv;
            else {
                pm_message("Too many colors - "
                           "proceeding to write a 24-bit RGB file.");
                pm_message("If you want an 8-bit palette file, "
                           "try doing a 'pnmquant %d'.",
                           maxcolors);
                *chvP = NULL;
            }
        }
    }
}



static void
analyzeColors(struct pam *        const pamP,
              struct cmdlineInfo  const cmdline,
              int                 const maxcolors, 
              tupletable *        const chvP, 
              unsigned int *      const colorsP, 
              bool *              const grayscaleP) {
/*----------------------------------------------------------------------------
   Analyze the colors in the input image described by 'pamP', whose file
   is positioned to the raster. 

   If the colors, combined with command line options 'cmdline', indicate
   a colormapped TIFF should be generated, return as *chvP the address
   of a color map (in newly malloc'ed space).  If a colormapped TIFF is
   not indicated, return *chvP == NULL.

   Return *grayscaleP == true iff the image should be stored as a grayscale
   image (which means the image is monochromatic and the user doesn't
   insist on color format).

   Leave the file position undefined.
-----------------------------------------------------------------------------*/
    if (pamP->depth >= 3)
        /* Assume it's a color-capable input image
           (tuple type RGB or RGB_ALPHA)
        */
        analyzeColorsInRgbInput(pamP, cmdline, maxcolors,
                                chvP, colorsP, grayscaleP);
    else {
        *chvP = NULL;
        *grayscaleP = TRUE;
    }
}



static void
computeRasterParm(struct pam *     const pamP,
                  tupletable       const chv, 
                  int              const colors, 
                  bool             const grayscale,
                  int              const compression,
                  bool             const minisblack,
                  bool             const miniswhite,
                  struct sizeset   const indexsizeAllowed,
                  unsigned short * const samplesperpixelP,
                  unsigned short * const bitspersampleP,
                  unsigned short * const photometricP,
                  int            * const bytesperrowP) {
/*----------------------------------------------------------------------------
   Compute the parameters of the raster portion of the TIFF image.

   'minisblack' and 'miniswhite' mean the user requests the corresponding
   photometric.  Both FALSE means user has no explicit requirement.
-----------------------------------------------------------------------------*/
    unsigned short defaultPhotometric;
    /* The photometric we use if the user specified no preference */

    /* We determine the bits per sample by the maxval, except that if
       it would be more than 8, we just use 16.  I don't know if bits
       per sample between 8 and 16 are legal, but they aren't very
       nice in any case.  If users want them, we should provide an
       option.  It is not clear why we don't use bits per pixel < 8
       for RGB images.  Note that code to handle maxvals <= 255 was
       written long before maxval > 255 was possible and there are
       backward compatibility requirements.  
    */

    if (pamP->depth == 1 && pamP->maxval == 1) {
        *samplesperpixelP = 1;
        *bitspersampleP = 1;
        if ((compression == COMPRESSION_CCITTFAX3) ||
            (compression == COMPRESSION_CCITTFAX4))
            defaultPhotometric = PHOTOMETRIC_MINISWHITE;
        else
            defaultPhotometric = PHOTOMETRIC_MINISBLACK;
    } else {
        if (chv) {
            *samplesperpixelP = 1;  /* Pixel is just the one index value */
            *bitspersampleP = 
                colors <=   2 && indexsizeAllowed.b1 ? 1 :
                colors <=   4 && indexsizeAllowed.b2 ? 2 :
                colors <=  16 && indexsizeAllowed.b4 ? 4 :
                colors <= 256 && indexsizeAllowed.b8 ? 8 :
                0;
            if (*bitspersampleP == 0)
                pm_error("Your -indexbits option is insufficient for the "
                         "%d colors in this image.", colors);

            defaultPhotometric = PHOTOMETRIC_PALETTE;
        } else {
            if (grayscale) {
                /* We'll write just the first plane regardless of how many are
                   in the input PAM.
                */
                *samplesperpixelP = 1;
                *bitspersampleP =
                    pamP->maxval > 255 ? 16 : pm_maxvaltobits(pamP->maxval);
                defaultPhotometric = PHOTOMETRIC_MINISBLACK;
            } else {
                *samplesperpixelP = pamP->depth;
                *bitspersampleP = pamP->maxval > 255 ? 16 : 8;
                defaultPhotometric = PHOTOMETRIC_RGB;
            }
        }
    }

    if (miniswhite)
        *photometricP = PHOTOMETRIC_MINISWHITE;
    else if (minisblack)
        *photometricP = PHOTOMETRIC_MINISBLACK;
    else
        *photometricP = defaultPhotometric;

    {
        unsigned int const samplesPerRow = pamP->width * *samplesperpixelP;

        if (*bitspersampleP < 8) {
            int samplesperbyte;
            samplesperbyte = 8 / *bitspersampleP;
            *bytesperrowP =
                (samplesPerRow + samplesperbyte-1) / samplesperbyte;
        } else {
            if (UINT_MAX / *bitspersampleP < samplesPerRow)
                pm_error("Too many bytes per row to compute");
            *bytesperrowP = (samplesPerRow * *bitspersampleP) / 8;
        }
    }
}



static void
validateSeekableOutputFile(int          const ofd,
                           const char * const outFileName) {
/*----------------------------------------------------------------------------
   Validate that the file attached to file descriptor 'ofd' is capable
   of seeking.  If not, fail the program.

   This is useful because the TIFF library requires seekable output and
   fails with an unhelpful error message about a file I/O error if it is
   not.  We, on the other hand, give a helpful error message.

   We leave the file positioned to the beginning.
-----------------------------------------------------------------------------*/
    int rc;

    /* We'd like to get the current position and leave the file positioned
       where we found it.  But that entails the mess with some systems
       having 32 bit file offsets and some having 64 bit file offsets.
    */

    /* Some files can seek ahead, but not back.  So we test for the
       more difficult backward seek.
    */
    lseek(ofd, 1, SEEK_SET);
    rc = lseek(ofd, 0, SEEK_SET);
            
    if (rc < 0)
        pm_error("Output file (%s) is not seekable.  lseek() returned "
                 "errno %d (%s).  "
                 "The TIFF library can write only to "
                 "a seekable file.", 
                 outFileName, errno, strerror(errno));
}



static void
createTiffGenerator(int          const ofd, 
                    const char * const outFileName,
                    bool         const append,
                    TIFF **      const tifPP) {

    const char * option;

    /* Before 10.12 (November 2002), we set O_NONBLOCK here:

       fcntl( 1, F_SETFL, O_NONBLOCK ) ; 
   
       I have no idea why.  The comment attached said, 

         acooke dec99 - otherwise blocks on read inside 
         next line (Linux i386) 
    */

    validateSeekableOutputFile(ofd, outFileName);

    if (append)
        option = "a";
    else
        option = "w";

    *tifPP = TIFFFdOpen(ofd, outFileName, option);
    if (*tifPP == NULL)
        pm_error("error opening standard output as TIFF file.  "
                 "TIFFFdOpen() failed.");
}



static void
destroyTiffGenerator(TIFF * const tifP) {

    TIFFFlushData(tifP);
    TIFFClose(tifP);
}



static void
createTiffColorMap(struct pam *       const pamP,
                   unsigned int       const bitspersample,
                   tupletable         const chv,
                   unsigned int       const colors,
                   unsigned short *** const tiffColorMapP) {

    unsigned int const colorMapSize = 1 << bitspersample;
    unsigned short ** tiffColorMap;
    unsigned int plane;
    unsigned int i;
    
    MALLOCARRAY_NOFAIL(tiffColorMap, pamP->depth);
    for (plane = 0; plane < pamP->depth; ++plane)
        MALLOCARRAY_NOFAIL(tiffColorMap[plane], colorMapSize);
    
    for (i = 0; i < colorMapSize; ++i) {
        unsigned int plane;
        for (plane = 0; plane < pamP->depth; ++plane) {
            if (i < colors)
                tiffColorMap[plane][i] =
                    chv[i]->tuple[plane] * 65535L / pamP->maxval;
            else
                tiffColorMap[plane][i] = 0;
        }
    }
    *tiffColorMapP = tiffColorMap;
}
        


static void
destroyTiffColorMap(struct pam *      const pamP,
                    unsigned short ** const tiffColorMap) {

    unsigned int plane;

    for (plane = 0; plane < pamP->depth; ++plane)
        free(tiffColorMap[plane]);

    free(tiffColorMap);
}



static void
setTagListFields(const struct optNameValue * const taglist,
                 TIFF *                      const tifP) {

    unsigned int i;

    for (i = 0; taglist[i].name; ++i) {
        const tagDefinition * const tagDefP = tagDefFind(taglist[i].name);
        
        if (tagDefP->put)
            tagDefP->put(tifP, tagDefP->tagnum, taglist[i].value,
                         tagDefP->choices);
    }
}



static void
setTiffFields(TIFF *              const tifP,
              struct cmdlineInfo  const cmdline,
              struct pam *        const pamP,
              unsigned short      const bitspersample,
              unsigned short      const photometric,
              unsigned short      const samplesperpixel,
              unsigned short **   const tiffColorMap,
              const char *        const inputFileDescription,
              struct optNameValue const taglist[]) {

    /* TODO: It would be cleaner to set all tags in a tag list and
       then call setTagListFields() on that list.  That way,
       TIFFSetField() is in only one place and all tags get set the
       same way and we don't have this icky setting of default and
       then resetting with user choice (e.g. DOCUMENTNAME).
    */

    /* Set TIFF parameters. */
    TIFFSetField(tifP, TIFFTAG_IMAGEWIDTH,    pamP->width);
    TIFFSetField(tifP, TIFFTAG_IMAGELENGTH,   pamP->height);
    TIFFSetField(tifP, TIFFTAG_BITSPERSAMPLE, bitspersample);
    if (cmdline.compression != -1)
        TIFFSetField(tifP, TIFFTAG_COMPRESSION, cmdline.compression);
    if (cmdline.compression == COMPRESSION_CCITTFAX3 && cmdline.g3options != 0)
        TIFFSetField( tifP, TIFFTAG_GROUP3OPTIONS, cmdline.g3options);
    if (cmdline.compression == COMPRESSION_LZW && cmdline.predictor != -1)
        TIFFSetField( tifP, TIFFTAG_PREDICTOR, cmdline.predictor);
    TIFFSetField(tifP, TIFFTAG_PHOTOMETRIC, photometric);
    if (cmdline.fillorder != -1)
        TIFFSetField(tifP, TIFFTAG_FILLORDER, cmdline.fillorder);
    TIFFSetField(tifP, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
    if (cmdline.rowsperstrip != -1)
        TIFFSetField(tifP, TIFFTAG_ROWSPERSTRIP, cmdline.rowsperstrip);
    else
        TIFFSetField(tifP, TIFFTAG_ROWSPERSTRIP,
                     TIFFDefaultStripSize(tifP, 0));
    if (cmdline.xresolution != -1 ||
        cmdline.yresolution != -1 ||
        cmdline.resolutionUnit != -1) {
        TIFFSetField(tifP, TIFFTAG_RESOLUTIONUNIT,
                     cmdline.resolutionUnit != -1 ?
                     cmdline.resolutionUnit : RESUNIT_NONE);
    }
    if (cmdline.xresolution > 0)
        TIFFSetField(tifP, TIFFTAG_XRESOLUTION, cmdline.xresolution);
    if ( cmdline.yresolution > 0)
        TIFFSetField(tifP, TIFFTAG_YRESOLUTION, cmdline.yresolution);
    TIFFSetField(tifP, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    if (tiffColorMap)
        TIFFSetField(tifP, TIFFTAG_COLORMAP,
                     tiffColorMap[PAM_RED_PLANE],
                     tiffColorMap[PAM_GRN_PLANE],
                     tiffColorMap[PAM_BLU_PLANE]);


    TIFFSetField(tifP, TIFFTAG_DOCUMENTNAME,     inputFileDescription);
    TIFFSetField(tifP, TIFFTAG_IMAGEDESCRIPTION, "converted PNM file");
 
    /* Some of taglist[] overrides defaults we set above.  But taglist[]
       is defined not to specify any tag types that are not purely user
       choice.
    */
    setTagListFields(taglist, tifP);
}



static void
convertImage(FILE *             const ifP,
             TIFF *             const tifP,
             const char *       const inputFileDescription,
             struct cmdlineInfo const cmdline) {

    tupletable chv;
    tuplehash cht;
    unsigned short ** tiffColorMap;  /* malloc'ed */
    struct pam pam;
    unsigned int colors;
    bool grayscale;
    unsigned short photometric;
    unsigned short samplesperpixel;
    unsigned short bitspersample;
    unsigned short tiff_maxval;
    /* This is the maxval of the samples in the tiff file.  It is 
       determined solely by the bits per sample ('bitspersample').
       */
    int bytesperrow;
    pm_filepos rasterPos;

    pnm_readpaminit(ifP, &pam, PAM_STRUCT_SIZE(tuple_type));

    pm_tell2(ifP, &rasterPos, sizeof(rasterPos));

    analyzeColors(&pam, cmdline, MAXCOLORS, &chv, &colors, &grayscale);

    /* Go back to beginning of raster */
    pm_seek2(ifP, &rasterPos, sizeof(rasterPos));  

    /* Figure out TIFF parameters. */

    computeRasterParm(&pam, chv, colors, grayscale, 
                      cmdline.compression,
                      cmdline.minisblack, cmdline.miniswhite,
                      cmdline.indexsizeAllowed,
                      &samplesperpixel, &bitspersample, &photometric,
                      &bytesperrow);

    tiff_maxval = pm_bitstomaxval(bitspersample);

    if (!chv) {
        cht = NULL;
        tiffColorMap = NULL;
    } else {
        createTiffColorMap(&pam, bitspersample, chv, colors, &tiffColorMap);

        /* Convert color vector to color hash table, for fast lookup. */
        cht = pnm_computetupletablehash(&pam, chv, colors);
        pnm_freetupletable(&pam, chv);
    }

    setTiffFields(tifP, cmdline, &pam, bitspersample, photometric,
                  samplesperpixel, tiffColorMap, inputFileDescription,
                  cmdline.taglist);

    writeScanLines(&pam, tifP, cht,
                   tiff_maxval, bitspersample, photometric, bytesperrow, 
                   cmdline.fillorder);

    if (tiffColorMap)
        destroyTiffColorMap(&pam, tiffColorMap);
}



static void
validateReadableStdout(void) {
/*----------------------------------------------------------------------------
  We validate that Standard Output is readable and fail the program if
  it isn't.

  This is useful because there are situations in which the TIFF library
  must read the output file and if it can't, it fails with an unhelpful
  error message about a file I/O error.  We, on the other hand, produce
  a helpful error message.
-----------------------------------------------------------------------------*/
    int flags;

    flags = fcntl(STDOUT_FILENO, F_GETFL);

    if (flags < 0) {
        /* We couldn't get the flags.  So just assume the file's OK */
    } else {
        if ((flags & O_RDONLY) || (flags & O_RDWR)) {
            /* File is readable.  All is well. */
        } else
            pm_error("Standard Output is not opened for reading.  "
                     "In order to create a multi-image TIFF stream, "
                     "Standard Output must be both readable and writable.");
    }
}



int
main(int argc, char *argv[]) {
    struct cmdlineInfo cmdline;
    const char * inputFileDescription;
    FILE* ifP;
    TIFF* tifP;
    bool eof;
    unsigned int imageSeq;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
    
    ifP = pm_openr_seekable(cmdline.input_filespec);

    if (streq(cmdline.input_filespec, "-"))
        inputFileDescription = "Standard Input";
    else 
        inputFileDescription = cmdline.input_filespec;

    if (cmdline.append)
        validateReadableStdout();

    createTiffGenerator(STDOUT_FILENO, "Standard Output", cmdline.append,
                        &tifP);

    eof = FALSE;  /* initial assumption */
    imageSeq = 0;

    while (!eof) {
        bool success;

        if (cmdline.verbose)
            pm_message("Converting Image %u", imageSeq);

        pnm_nextimage(ifP, &eof);

        if (!eof) {
            if (imageSeq > 0)
                validateReadableStdout();

            convertImage(ifP, tifP, inputFileDescription, cmdline);
            
            success = TIFFWriteDirectory(tifP);
            if (!success)
                pm_error("Unable to write TIFF image %u to file.  "
                         "tifWriteDirectory() failed.", imageSeq);
            ++imageSeq;
        }
    }

    destroyTiffGenerator(tifP);
    pm_close(ifP);

    return 0;
}
