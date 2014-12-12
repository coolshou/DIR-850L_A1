#ifndef WINICO_H_INCLUDED
#define WINICO_H_INCLUDED

/* A specification for the Windows icon format is at   (2000.06.08)

   http://www.daubnet.com/formats/ICO.html

*/

typedef unsigned char      u1;
typedef unsigned short int u2;
typedef unsigned int       u4;

typedef struct MS_Ico_ *          MS_Ico;
typedef struct IC_Entry_ *        IC_Entry;
typedef struct IC_InfoHeader_ *   IC_InfoHeader;
typedef struct IC_Color_ *        IC_Color;
/* Not part of the spec, but useful in constructing the icon. */
typedef struct IC_Palette_ *      IC_Palette;
typedef struct ICON_bmp_ *        ICON_bmp;

struct MS_Ico_ {
   u2 reserved;
   u2 type;
   u2 count;
   IC_Entry * entries;
};


struct IC_Entry_ {
   u1 width;
   u1 height;
   /*
    * color_count is actually a byte (u1)... but 0 = 256, so I've used a short (u2).
    */
   u2 color_count;
   u1 reserved;
   u2 planes;
   u2 bitcount;
   u4 size_in_bytes;
   u4 file_offset;
   IC_InfoHeader ih;
   IC_Color * colors;
   /*
    * Below here, I have useful fields which aren't in the spec, but 
    * save having to keep stoopid amounts of global data.
    */
   u1 * andBitmap;        /* Used in reader. */
   u1 * xorBitmap;
   int xBytesXor;         /* Not used in reading, but saved for writing. */
   int xBytesAnd;         /* Not used in reading, but saved for writing. */
   u1 ** andBitmapOut;    /* it's just easier to use a 2d array in the code.*/
   u1 ** xorBitmapOut;    /* Sorry! :) */
};

struct IC_InfoHeader_ {
   u4 size;
   u4 width;
   u4 height;
   u2 planes;
   u2 bitcount;
   u4 compression;
   u4 imagesize;
   u4 x_pixels_per_m;
   u4 y_pixels_per_m;
   u4 colors_used;
   u4 colors_important;
};

struct IC_Color_ {
   u1 red;
   u1 green;
   u1 blue;
   u1 reserved;
};

struct IC_Palette_ {
   u4 col_amount;
   IC_Color * colors;
};

struct ICON_bmp_ {
   int xBytes;
   u4 size;    /* just col_amount * height, but save calculating too often. */
   u1 ** data;
};

#endif
