/* bitmap.c: operations on bitmaps. */

#include <string.h>

#include "mallocvar.h"

#include "bitmap.h"

at_bitmap_type *
at_bitmap_new(unsigned short width,
              unsigned short height,
              unsigned int planes) {

    at_bitmap_type * bitmap;

    MALLOCVAR_NOFAIL(bitmap); 

    *bitmap = at_bitmap_init(NULL, width, height, planes);

    return bitmap;
}



at_bitmap_type *
at_bitmap_copy(at_bitmap_type * src)
{
    at_bitmap_type * dist;
    unsigned short width, height, planes;

    width  = at_bitmap_get_width(src);
    height = at_bitmap_get_height(src);
    planes = at_bitmap_get_planes(src);
    
    dist = at_bitmap_new(width, height, planes);
    memcpy(dist->bitmap, 
           src->bitmap, 
           width * height * planes * sizeof(unsigned char));
    return dist;
}



at_bitmap_type
at_bitmap_init(unsigned char * area,
               unsigned short width,
               unsigned short height,
               unsigned int planes) {

    at_bitmap_type bitmap;
    
    if (area)
        bitmap.bitmap = area;
    else {
        if (width * height == 0)
            bitmap.bitmap = NULL;
        else {
            MALLOCARRAY(bitmap.bitmap, width * height * planes);
            if (bitmap.bitmap == NULL)
                pm_error("Unable to allocate %u x %u x %u bitmap array",
                         width, height, planes);
            bzero(bitmap.bitmap,
                  width * height * planes * sizeof(unsigned char));
        }
    }
    
    bitmap.width  = width;
    bitmap.height = height;
    bitmap.np     =  planes;

    return bitmap;  
}

void 
at_bitmap_free (at_bitmap_type * bitmap)
{
    free_bitmap (bitmap);
    free(bitmap);
}

unsigned short
at_bitmap_get_width (at_bitmap_type * bitmap)
{
    return bitmap->width;
}

unsigned short
at_bitmap_get_height (at_bitmap_type * bitmap)
{
    return bitmap->height;
}

unsigned short
at_bitmap_get_planes (at_bitmap_type * bitmap)
{
    return bitmap->np;
}



bitmap_type
new_bitmap (unsigned short width, unsigned short height)
{
    return at_bitmap_init(NULL,width,height,1);
}

/* Free the storage that is allocated for a bitmap.  On the other hand,
   the bitmap might not have any storage allocated for it if it is zero
   in either dimension; in that case, don't free it.  */

void
free_bitmap (bitmap_type *b)
{
    if (b->bitmap != NULL)
        free (b->bitmap);
}
