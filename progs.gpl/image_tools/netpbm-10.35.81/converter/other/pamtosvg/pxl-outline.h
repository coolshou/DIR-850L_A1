/* pxl-outline.h: find a list of outlines which make up one character. */

#ifndef PXL_OUTLINE_H
#define PXL_OUTLINE_H

#include "ppm.h"

#include "autotrace.h"
#include "exception.h"
#include "bitmap.h"

/* This is a list of contiguous points on the bitmap.  */
typedef struct
{
  pm_pixelcoord * data;
  unsigned length;
  bool clockwise;
  pixel color;
  bool open;
} pixel_outline_type;


/* The Nth coordinate in the list.  */
#define O_COORDINATE(p_o, n) ((p_o).data[n])


/* The length of the list.  */
#define O_LENGTH(p_o) ((p_o).length)

/* Whether the outline moves clockwise or counterclockwise.  */
#define O_CLOCKWISE(p_o) ((p_o).clockwise)

/* Since a pixel outline is cyclic, the index of the next coordinate
   after the last is the first, and the previous coordinate before the
   first is the last.  */
#define O_NEXT(p_o, n) (((n) + 1) % O_LENGTH (p_o))
#define O_PREV(p_o, n) ((n) == 0				\
                         ? O_LENGTH (p_o) - 1			\
                         : (n) - 1)

/* And the character turns into a list of such lists.  */
typedef struct
{
  pixel_outline_type *data;
  unsigned length;
} pixel_outline_list_type;

/* The Nth list in the list of lists.  */
#define O_LIST_OUTLINE(p_o_l, n) ((p_o_l).data[n])

/* The length of the list of lists.  */
#define O_LIST_LENGTH(p_o_l) ((p_o_l).length)

/* Find all pixels on the outline in the character C.  */
pixel_outline_list_type
find_outline_pixels (bitmap_type         const type,
                     bool                const bg_spec,
                     pixel               const bg_color, 
                     at_progress_func          notify_progress,
                     void *              const progress_data,
                     at_testcancel_func        test_cancel,
                     void *              const testcancel_data,
                     at_exception_type * const exp);

/* Find all pixels on the center line of the character C.  */
pixel_outline_list_type
find_centerline_pixels (bitmap_type         const type,
                        pixel               const bg_color, 
                        at_progress_func          notify_progress,
                        void *              const progress_data,
                        at_testcancel_func        test_cancel,
                        void *              const testcancel_data,
                        at_exception_type * const exp);

/* Free the memory in the list.  */
extern void
free_pixel_outline_list (pixel_outline_list_type *);

#endif /* not PXL_OUTLINE_H */
