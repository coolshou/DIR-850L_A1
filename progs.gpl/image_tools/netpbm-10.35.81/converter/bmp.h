/*\
 * $Id: bmp.h,v 1.3 1992/11/24 19:39:56 dws Exp dws $
 * 
 * bmp.h - routines to calculate sizes of parts of BMP files
 *
 * Some fields in BMP files contain offsets to other parts
 * of the file.  These routines allow us to calculate these
 * offsets, so that we can read and write BMP files without
 * the need to fseek().
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
 * $Log: bmp.h,v $
 * Revision 1.3  1992/11/24  19:39:56  dws
 * Added copyright.
 *
 * Revision 1.2  1992/11/17  02:13:37  dws
 * Adjusted a string's name.
 *
 * Revision 1.1  1992/11/16  19:54:44  dws
 * Initial revision
 *
\*/

/* 
   There is a specification of BMP (2003.07.24) at:

     http://msdn.microsoft.com/library/default.asp?url=/library/en-us/gdi/bitmaps_4v1h.asp

   There is a better written specification of the Windows BMP format
   in (2000.06.08) <http://www.daubnet.com/formats/BMP.html>.
   However, the "Windows BMP" format used in practice is much closer
   to the Microsoft definition.  The Microsoft spec was not known for
   Netpbm development until 2003.07.24.

   The ColorsImportant field is defined in the daubnet spec as "Number of
   important colors.  0 = all"  That is the entire definition.  The
   spec also says the number of entries in the color map is a function
   of the BitCount field alone.

   But Marc Moorcroft says (2000.07.23) that he found two BMP files
   some time ago that had a color map whose number of entries was not
   as specified and was in fact the value of ColorsImportant.

   And Bill Janssen actually produced some BMPs in January 2001 that
   appear to have the size of the colormap determined by ColorsUsed.
   They have 8 bits per pixel in the raster, but ColorsUsed is 4 and
   there are in fact 4 entries in the color map.  He got these from
   the Palm emulator for Windows, using the "Save Screen" menu 
   option.

   Bmptoppm had, for a few releases in 2000, code by Marc to use
   ColorsImportant as the color map size unless it was zero, in which
   case it determined color map size as specified.  The current
   thinking is that there are probably more BMPs that need to be
   interpreted per the spec than that need to be interpreted Marc's
   way.  

   But in light of Janssen's discovery, we have made the assumption
   since February 2001 that when ColorsUsed is zero, the colormap size
   is as specified, and when it is nonzero, the colormap size is given
   by ColorsUsed.

   But we were also assuming that if ColorsUsed is nonzero, the image
   is colormapped.  We heard from "Ron & Bes Vantreese"
   <eaglesun@aggienetwork.com> in February 2003 that his understanding
   of the format was that ColorsUsed == 2**24 is appropriate for a
   non-colormapped (24 bit) BMP, and images he created that way caused
   trouble for Bmptopnm.  So since then, we look at ColorsUsed only if
   we know because bits per pixel <= 8 that it is a colormapped image.

*/

#ifndef BMP_H_INCLUDED
#define BMP_H_INCLUDED

#include "pm.h"  /* For pm_error() */

enum bmpClass {C_WIN=1, C_OS2=2};

static char const er_internal[] = "%s: internal error!";

/* Values of the "compression" field of the BMP info header */
#define COMP_RGB       0
#define COMP_RLE8      1
#define COMP_RLE4      2
#define COMP_BITFIELDS 3
#define COMP_JPEG      4
#define COMP_PNG       5

static __inline__ unsigned int
BMPlenfileheader(enum bmpClass const class) {

    unsigned int retval;

    switch (class) {
    case C_WIN: retval = 14; break;
    case C_OS2: retval = 14; break;
    }
    return retval;
}



static __inline__ unsigned int
BMPleninfoheader(enum bmpClass const class) {

    unsigned int retval;

    switch (class) {
    case C_WIN: retval = 40; break;
    case C_OS2: retval = 12; break;
    }
    return retval;
}



static __inline__ unsigned int
BMPlencolormap(enum bmpClass const class,
               unsigned int  const bitcount, 
               unsigned int  const cmapsize) {

    unsigned int lenrgb;
    unsigned int lencolormap;

    if (bitcount < 1)
        pm_error(er_internal, "BMPlencolormap");
    else if (bitcount > 8) 
        lencolormap = 0;
    else {
        switch (class) {
        case C_WIN: lenrgb = 4; break;
        case C_OS2: lenrgb = 3; break;
        }

        if (!cmapsize) 
            lencolormap = (1 << bitcount) * lenrgb;
        else 
            lencolormap = cmapsize * lenrgb;
    }
    return lencolormap;
}



static __inline__ unsigned int
BMPlenline(enum bmpClass const class,
           unsigned int  const bitcount, 
           unsigned int  const x) {
/*----------------------------------------------------------------------------
  length, in bytes, of a line of the image

  Each row is padded on the right as needed to make it a multiple of 4
  bytes int.  This appears to be true of both OS/2 and Windows BMP
  files.
-----------------------------------------------------------------------------*/
    unsigned int bitsperline;
    unsigned int retval;

    bitsperline = x * bitcount;  /* tentative */

    /*
     * if bitsperline is not a multiple of 32, then round
     * bitsperline up to the next multiple of 32.
     */
    if ((bitsperline % 32) != 0)
        bitsperline += (32 - (bitsperline % 32));

    if ((bitsperline % 32) != 0) {
        pm_error(er_internal, "BMPlenline");
        retval = 0;
    } else 
        /* number of bytes per line == bitsperline/8 */
        retval = bitsperline >> 3;

    return retval;
}



static __inline__ unsigned int
BMPlenbits(enum bmpClass const class,
           unsigned int  const bitcount, 
           unsigned int  const x,
           unsigned int  const y) {
/*----------------------------------------------------------------------------
  Return the number of bytes used to store the image bits
  for an uncompressed image.
-----------------------------------------------------------------------------*/
    return y * BMPlenline(class, bitcount, x);
}



static __inline__ unsigned int
BMPoffbits(enum bmpClass const class,
           unsigned int  const bitcount, 
           unsigned int  const cmapsize) {
/*----------------------------------------------------------------------------
  return the offset to the BMP image bits.
-----------------------------------------------------------------------------*/
    return BMPlenfileheader(class)
        + BMPleninfoheader(class)
        + BMPlencolormap(class, bitcount, cmapsize);
}


static __inline__ unsigned int
BMPlenfileGen(enum bmpClass     const class,
              unsigned int      const bitcount, 
              unsigned int      const cmapsize,
              unsigned int      const x,
              unsigned int      const y,
              unsigned int      const imageSize,
              unsigned long int const compression) {
/*----------------------------------------------------------------------------
  Return the size of the BMP file in bytes.
-----------------------------------------------------------------------------*/
    unsigned int retval;

    switch (compression) {
    case COMP_RGB:
    case COMP_BITFIELDS:
        retval =
            BMPoffbits(class, bitcount, cmapsize) +
            BMPlenbits(class, bitcount, x, y);
        break;
    default:
        retval = BMPoffbits(class, bitcount, cmapsize) + imageSize;
    }
    return retval;
}



static __inline__ unsigned int
BMPlenfile(enum bmpClass const class,
           unsigned int  const bitcount, 
           unsigned int  const cmapsize,
           unsigned int  const x,
           unsigned int  const y) {
/*----------------------------------------------------------------------------
  return the size of the BMP file in bytes; no compression
-----------------------------------------------------------------------------*/
    return BMPlenfileGen(class, bitcount, cmapsize, x, y, 0, COMP_RGB);
}

#endif
