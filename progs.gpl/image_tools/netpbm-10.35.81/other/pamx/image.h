#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#include "pm_c_util.h"

#include "ximageinfo.h"

typedef struct rgbmap {
    unsigned int size;       /* size of RGB map */
    unsigned int used;       /* number of colors used in RGB map */
    bool         compressed; /* image uses colormap fully */
    Intensity *  red;        /* color values in X style */
    Intensity *  grn;
    Intensity *  blu;
} RGBMap;

typedef struct Image {
    unsigned int    type;   /* type of image */
    RGBMap          rgb;    /* RGB map of image if IRGB type */
    unsigned int    width;  /* width of image in pixels */
    unsigned int    height; /* height of image in pixels */
    unsigned int    depth;  /* depth of image in bits if IRGB type */
    unsigned int    pixlen; /* length of pixel if IRGB type */
    unsigned char * data;   /* data rounded to full byte for each row */
} Image;

#define IBAD    0 /* invalid image (used when freeing) */
#define IBITMAP 1 /* image is a bitmap */
#define IRGB    2 /* image is RGB */
#define ITRUE   3 /* image is true color */

#define BITMAPP(IMAGE) ((IMAGE)->type == IBITMAP)
#define RGBP(IMAGE)    ((IMAGE)->type == IRGB)
#define TRUEP(IMAGE)   ((IMAGE)->type == ITRUE)

#define TRUE_RED(PIXVAL)   (((PIXVAL) & 0xff0000) >> 16)
#define TRUE_GRN(PIXVAL) (((PIXVAL) & 0xff00) >> 8)
#define TRUE_BLU(PIXVAL)  ((PIXVAL) & 0xff)
#define RGB_TO_TRUE(R,G,B) \
  (((unsigned int)((R) & 0xff00) << 8) | ((unsigned int)(G) & 0xff00) | \
   ((unsigned int)(B) >> 8))

unsigned long
colorsToDepth(unsigned long const ncolors);

void
assertGoodImage(Image * const imageP);

Image *
newBitImage(unsigned int const width,
            unsigned int const height);

Image *
newRGBImage(unsigned int const width,
            unsigned int const height,
            unsigned int const depth);

Image *
newTrueImage(unsigned int const width,
             unsigned int const height);

void
freeImage(Image * const imageP);

unsigned int
depthToColors(unsigned int const depth);

extern unsigned short RedIntensity[];
extern unsigned short GreenIntensity[];
extern unsigned short BlueIntensity[];

static __inline__ unsigned int
colorIntensity(unsigned int const red,
               unsigned int const grn,
               unsigned int const blu) {
/*----------------------------------------------------------------------------
  Return the (approximate) intensity of a color.
-----------------------------------------------------------------------------*/
    return (RedIntensity[red / 256] +
            GreenIntensity[grn / 256] +
            BlueIntensity[blu / 256]);
}

Image *
pbmLoad(const char * const fullname,
        const char * const name,
        bool         const verbose);

#endif
