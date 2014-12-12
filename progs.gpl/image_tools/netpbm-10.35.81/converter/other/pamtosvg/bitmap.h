/* bitmap.h: definition for a bitmap type.  No packing is done by
   default; each pixel is represented by an entire byte.  Among other
   things, this means the type can be used for both grayscale and binary
   images. */

#ifndef BITMAP_H
#define BITMAP_H

#include "autotrace.h"
#include <stdio.h>

/* at_ prefix removed version */
typedef at_bitmap_type bitmap_type;
#define BITMAP_PLANES(b)          AT_BITMAP_PLANES(b)
#define BITMAP_BITS(b)            AT_BITMAP_BITS(b)  
#define BITMAP_WIDTH(b)           AT_BITMAP_WIDTH(b)  
#define BITMAP_HEIGHT(b)          AT_BITMAP_HEIGHT(b) 

/* This is the pixel at [ROW,COL].  */
#define BITMAP_PIXEL(b, row, col)					\
  ((b).bitmap + ((row) * (b).width + (col)) * (b).np)

#define BITMAP_VALID_PIXEL(b, row, col)					\
   	((row) < (b).height && (col) < (b).width)

/* Allocate storage for the bits, set them all to white, and return an
   initialized structure.  */
extern bitmap_type new_bitmap (unsigned short width, unsigned short height);

/* Free that storage.  */
extern void free_bitmap (bitmap_type *);


at_bitmap_type * at_bitmap_new(unsigned short width,
			       unsigned short height,
			       unsigned int planes);
at_bitmap_type * at_bitmap_copy(at_bitmap_type * src);

/* We have to export functions that supports internal datum 
   access. Such functions might be useful for 
   at_bitmap_new user. */
unsigned short at_bitmap_get_width (at_bitmap_type * bitmap);
unsigned short at_bitmap_get_height (at_bitmap_type * bitmap);
unsigned short at_bitmap_get_planes (at_bitmap_type * bitmap);
void at_bitmap_free (at_bitmap_type * bitmap);

at_bitmap_type
at_bitmap_init(unsigned char * area,
	       unsigned short width,
	       unsigned short height,
	       unsigned int planes);

#endif /* not BITMAP_H */
