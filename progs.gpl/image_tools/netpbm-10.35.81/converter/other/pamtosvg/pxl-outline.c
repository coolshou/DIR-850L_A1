/* pxl-outline.c: find the outlines of a bitmap image; each outline is
   made up of one or more pixels; and each pixel participates via one
   or more edges.
*/

#include <assert.h>

#include "mallocvar.h"

#include "message.h"
#include "bitmap.h"
#include "bitmap.h"
#include "logreport.h"
#include "pxl-outline.h"

/* We consider each pixel to consist of four edges, and we travel along
   edges, instead of through pixel centers.  This is necessary for those
   unfortunate times when a single pixel is on both an inside and an
   outside outline.

   The numbers chosen here are not arbitrary; the code that figures out
   which edge to move to depends on particular values.  See the
   `TRY_PIXEL' macro in `edge.c'.  To emphasize this, I've written in the
   numbers we need for each edge value.  */

typedef enum
  {
    TOP = 1, LEFT = 2, BOTTOM = 3, RIGHT = 0, NO_EDGE = 4
  } edge_type;

/* This choice is also not arbitrary: starting at the top edge makes the
   code find outside outlines before inside ones, which is certainly
   what we want.  */
#define START_EDGE  top

typedef enum
  {
    NORTH = 0, NORTHWEST = 1, WEST = 2, SOUTHWEST = 3, SOUTH = 4,
    SOUTHEAST = 5, EAST = 6, NORTHEAST = 7
  } direction_type;

#define NUM_EDGES NO_EDGE

#define COMPUTE_DELTA(axis, dir)                        \
  ((dir) % 2 != 0                                       \
    ? COMPUTE_##axis##_DELTA ((dir) - 1)                \
      + COMPUTE_##axis##_DELTA (((dir) + 1) % 8)        \
    : COMPUTE_##axis##_DELTA (dir)                      \
  )

#define COMPUTE_ROW_DELTA(dir)                          \
  ((dir) == NORTH ? -1 : (dir) == SOUTH ? +1 : 0)

#define COMPUTE_COL_DELTA(dir)                  \
  ((dir) == WEST ? -1 : (dir) == EAST ? +1 : 0)

static void append_pixel_outline (pixel_outline_list_type *,
                                  pixel_outline_type);
static pixel_outline_type new_pixel_outline (void);
static void free_pixel_outline (pixel_outline_type *);
static void concat_pixel_outline (pixel_outline_type *,
                                  const pixel_outline_type*);
static bool is_marked_edge (edge_type, unsigned short, unsigned short, bitmap_type);

static void
mark_edge (edge_type e, unsigned short, unsigned short, bitmap_type *);

static bool is_marked_dir(unsigned short, unsigned short, direction_type, bitmap_type);
static bool is_other_dir_marked(unsigned short, unsigned short, direction_type, bitmap_type);
static void mark_dir(unsigned short, unsigned short, direction_type, bitmap_type *);

static unsigned
num_neighbors(unsigned short, unsigned short, bitmap_type);

#define CHECK_FATAL() if (at_exception_got_fatal(exp)) goto cleanup;
#define RETURN_IF_FATAL() if (at_exception_got_fatal(exp)) return;




static pixel
getBitmapColor(bitmap_type  const bitmap,
               unsigned int const row,
               unsigned int const col) {

    pixel pix;
    unsigned char *p = BITMAP_PIXEL (bitmap, row, col);
  
    if (bitmap.np >= 3)
        PPM_ASSIGN(pix, p[0], p[1], p[2]);
    else
        PPM_ASSIGN(pix, p[0], p[0], p[0]);

    return pix;
}




static void
append_outline_pixel(pixel_outline_type * const pixelOutlineP,
                     pm_pixelcoord        const coord) {
/*----------------------------------------------------------------------------
  Add a point to the pixel list.
-----------------------------------------------------------------------------*/

    O_LENGTH(*pixelOutlineP)++;
    REALLOCARRAY_NOFAIL(pixelOutlineP->data, O_LENGTH(*pixelOutlineP));

    O_COORDINATE(*pixelOutlineP, O_LENGTH(*pixelOutlineP) - 1) = coord;
}



/* We check to see if the edge of the pixel at position ROW and COL
   is an outline edge */

static bool
is_outline_edge (edge_type edge, bitmap_type bitmap,
                 unsigned short row, unsigned short col, pixel color,
                 at_exception_type * exp)
{
  /* If this pixel isn't of the same color, it's not part of the outline. */
  if (!PPM_EQUAL (getBitmapColor (bitmap, row, col), color))
    return false;

  switch (edge)
    {
    case LEFT:
      return (bool)
          (col == 0 ||
           !PPM_EQUAL (getBitmapColor (bitmap, row, col - 1), color));

    case TOP:
      return (bool)
          (row == 0 ||
           !PPM_EQUAL (getBitmapColor (bitmap, row - 1, col), color));

    case RIGHT:
        return (bool)
            (col == bitmap.width - 1
             || !PPM_EQUAL(getBitmapColor(bitmap, row, col + 1), color));

    case BOTTOM:
        return (bool)
            (row == bitmap.height - 1
             || !PPM_EQUAL(getBitmapColor (bitmap, row + 1, col), color));

    case NO_EDGE:
    default:
      LOG1 ("is_outline_edge: Bad edge value(%d)", edge);
      at_exception_fatal(exp, "is_outline_edge: Bad edge value");
    }

  return false; /* NOT REACHED */
}


/* Is this really an edge and is it still unmarked? */

static bool
is_unmarked_outline_edge(unsigned short row,
                         unsigned short col,
                         edge_type edge,
                         bitmap_type bitmap,
                         bitmap_type marked,
                         pixel color,
                         at_exception_type * exp)
{
  return
    (bool)(!is_marked_edge (edge, row, col, marked)
              && is_outline_edge (edge, bitmap, row, col, color, exp));
}


static bool
is_valid_dir(unsigned int   const row,
             unsigned int   const col,
             direction_type const dir,
             bitmap_type    const bitmap,
             bitmap_type    const marked) {
  
    return(!is_marked_dir(row, col, dir, marked)
           && COMPUTE_DELTA(ROW, dir)+row > 0
           && COMPUTE_DELTA(COL, dir)+col > 0
           && BITMAP_VALID_PIXEL(bitmap,
                                 COMPUTE_DELTA(ROW, dir) + row,
                                 COMPUTE_DELTA(COL, dir) + col)
           && PPM_EQUAL(getBitmapColor(bitmap,
                                       COMPUTE_DELTA(ROW, dir) + row,
                                       COMPUTE_DELTA(COL, dir) + col),
                        getBitmapColor(bitmap, row, col)));
}



static bool
next_unmarked_pixel(unsigned int *   const row,
                    unsigned int *   const col,
                    direction_type * const dir,
                    bitmap_type      const bitmap,
                    bitmap_type *    const marked) {

    unsigned int   const orig_row = *row;
    unsigned int   const orig_col = *col;
    direction_type const orig_dir = *dir;

    direction_type test_dir;
    pixel color;

    test_dir = *dir;  /* initial value */
    color = getBitmapColor(bitmap, *row, *col);

    do {
        if (is_valid_dir(orig_row, orig_col, test_dir, bitmap, *marked)) {
            *row = orig_row + COMPUTE_DELTA(ROW, test_dir);
            *col = orig_col + COMPUTE_DELTA(COL, test_dir);
            *dir = test_dir;
            break;
        }

        if (orig_dir == test_dir)
            test_dir = (orig_dir + 2) % 8;
        else if ((orig_dir + 2) % 8 == test_dir)
            test_dir = (orig_dir + 6) % 8;
        else if ((orig_dir + 6) % 8 == test_dir)
            test_dir = (orig_dir + 1) % 8;
        else if ((orig_dir + 1) % 8 == test_dir)
            test_dir = (orig_dir + 7) % 8;
        else if ((orig_dir + 7) % 8 == test_dir)
            test_dir = (orig_dir + 3) % 8;
        else if ((orig_dir + 3) % 8 == test_dir)
            test_dir = (orig_dir + 5) % 8;
        else if ((orig_dir + 5) % 8 == test_dir)
            break;
    } while (1);

    if ((*row != orig_row || *col != orig_col) && 
        (!(is_other_dir_marked(orig_row, orig_col, test_dir, *marked) &&
           is_other_dir_marked(orig_row + COMPUTE_DELTA(ROW, test_dir),
                               orig_col + COMPUTE_DELTA(COL, test_dir),
                               test_dir,*marked))))
        return true;
    else
        return false;
}



static pixel_outline_type
find_one_centerline(bitmap_type    const bitmap,
                    direction_type const original_dir,
                    unsigned int   const original_row,
                    unsigned int   const original_col,
                    bitmap_type *  const marked) {

    direction_type search_dir;
    unsigned int row, col;
    pixel_outline_type outline;

    outline = new_pixel_outline();
    outline.open  = false;
    outline.color = getBitmapColor(bitmap, original_row, original_col);

    /* Add the starting pixel to the output list, changing from bitmap
       to Cartesian coordinates and specifying the left edge so that
       the coordinates won't be adjusted.
    */
    {
        pm_pixelcoord pos;
        pos.col = col; pos.row = bitmap.height - row - 1;
        LOG2(" (%d,%d)", pos.col, pos.row);
        append_outline_pixel(&outline, pos);
    }
    search_dir = original_dir;  /* initial value */
    row = original_row;         /* initial value */
    col = original_col;         /* initial values */

    for ( ; ; ) {
        unsigned int const prev_row = row;
        unsigned int const prev_col = col;

        /* If there is no adjacent, unmarked pixel, we can't proceed
           any further, so return an open outline.
        */
        if (!next_unmarked_pixel(&row, &col, &search_dir, bitmap, marked)) {
            outline.open = true;
            break;
        }

        /* If we've moved to a new pixel, mark all edges of the previous
           pixel so that it won't be revisited.
        */
        if (!(prev_row == original_row && prev_col == original_col))
            mark_dir(prev_row, prev_col, search_dir, marked);
        mark_dir(row, col, (search_dir + 4) % 8, marked);

        /* If we've returned to the starting pixel, we're done. */
        if (row == original_row && col == original_col)
            break;

        
        {
            /* Add the new pixel to the output list. */
            pm_pixelcoord pos;
            pos.col = col; pos.row = bitmap.height - row - 1;
            LOG2(" (%d,%d)", pos.col, pos.row);
            append_outline_pixel(&outline, pos);
        }
    }
    mark_dir(original_row, original_col, original_dir, marked);

    return outline;
}



static bool
wrongDirection(unsigned int   const row,
               unsigned int   const col,
               direction_type const dir,
               bitmap_type    const bitmap,
               bitmap_type    const marked) {

    return (!is_valid_dir(row, col, dir, bitmap, marked)
            || (!is_valid_dir(COMPUTE_DELTA(ROW, dir) + row,
                              COMPUTE_DELTA(COL, dir) + col,
                              dir, bitmap, marked)
                && num_neighbors(row, col, bitmap) > 2)
            || num_neighbors(row, col, bitmap) > 4
            || num_neighbors(COMPUTE_DELTA(ROW, dir) + row,
                             COMPUTE_DELTA(COL, dir) + col, bitmap) > 4
            || (is_other_dir_marked(row, col, dir, marked)
                && is_other_dir_marked(row + COMPUTE_DELTA(ROW, dir),
                                       col + COMPUTE_DELTA(COL, dir),
                                       dir, marked)));
}



pixel_outline_list_type
find_centerline_pixels(bitmap_type         const bitmap,
                       pixel               const bg_color, 
                       at_progress_func          notify_progress,
                       void *              const progress_data,
                       at_testcancel_func        test_cancel,
                       void *              const testcancel_data,
                       at_exception_type * const exp) {

  pixel_outline_list_type outline_list;
  signed short row;
  bitmap_type marked = new_bitmap(bitmap.width, bitmap.height);
  unsigned int const max_progress = bitmap.height * bitmap.width;
    
  O_LIST_LENGTH(outline_list) = 0;
  outline_list.data = NULL;

  for (row = 0; row < bitmap.height; ++row) {
      signed short col;
      for (col = 0; col < bitmap.width; ) {
          bool           const clockwise = false;

          direction_type dir;
          pixel_outline_type outline;

          if (notify_progress)
              notify_progress((float)(row * bitmap.width + col) /
                              ((float) max_progress * (float)3.0),
                              progress_data);

		  if (PPM_EQUAL(getBitmapColor(bitmap, row, col), bg_color)) {
	          ++col;
			  continue;
          }

          dir = EAST;

          if (wrongDirection(row, col, dir, bitmap, marked)) {
              dir = SOUTHEAST;
              if (wrongDirection(row, col, dir, bitmap, marked)) {
                  dir = SOUTH;
                  if (wrongDirection(row, col, dir, bitmap, marked)) {
                      dir = SOUTHWEST;
                      if (wrongDirection(row, col, dir, bitmap, marked)) {
						  ++col;
						  continue;
                      }
                  }
              }
          }

          LOG2("#%u: (%sclockwise) ", O_LIST_LENGTH(outline_list),
               clockwise ? "" : "counter");

          outline = find_one_centerline(bitmap, dir, row, col, &marked);

          /* If the outline is open (i.e., we didn't return to the
             starting pixel), search from the starting pixel in the
             opposite direction and concatenate the two outlines.
          */

          if (outline.open) {
              pixel_outline_type partial_outline;
              bool okay = false;

              if (dir == EAST) {
                  dir = SOUTH;
                  okay = is_valid_dir(row, col, dir, bitmap, marked);
                  if (!okay) {
                      dir = SOUTHWEST;
                      okay = is_valid_dir(row, col, dir, bitmap, marked);
                      if (!okay) {
                          dir = SOUTHEAST;
                          okay = is_valid_dir(row, col, dir, bitmap, marked);
                      }
                  }
              } else if (dir == SOUTHEAST) {
                  dir = SOUTHWEST;
                  okay = is_valid_dir(row, col, dir, bitmap, marked);
                  if (!okay) {
                      dir = EAST;
                      okay=is_valid_dir(row, col, dir, bitmap, marked);
                      if (!okay) {
                          dir = SOUTH;
                          okay = is_valid_dir(row, col, dir, bitmap, marked);
                      }
                  }
              } else if (dir == SOUTH) {
                  dir = EAST;
                  okay = is_valid_dir(row, col, dir, bitmap, marked);
                  if (!okay) {
                      dir = SOUTHEAST;
                      okay = is_valid_dir(row, col, dir, bitmap, marked);
                      if (!okay) {
                          dir = SOUTHWEST;
                          okay = is_valid_dir(row, col, dir, bitmap, marked);
                      }
                  }
              } else if (dir == SOUTHWEST) {
                  dir = SOUTHEAST;
                  okay = is_valid_dir(row, col, dir, bitmap, marked);
                  if (!okay) {
                      dir = EAST;
                      okay = is_valid_dir(row, col, dir, bitmap, marked);
                      if (!okay) {
                          dir = SOUTH;
                          okay = is_valid_dir(row, col, dir, bitmap, marked);
                      }
                  }
              }
              if (okay) {
                  partial_outline =
                      find_one_centerline(bitmap, dir, row, col, &marked);
                  concat_pixel_outline(&outline, &partial_outline);
                  if (partial_outline.data)
                      free(partial_outline.data);
              } else
                  ++col;
          }        
            
          /* Outside outlines will start at a top edge, and move
             counterclockwise, and inside outlines will start at a
             bottom edge, and move clockwise.  This happens because of
             the order in which we look at the edges.
          */
          O_CLOCKWISE(outline) = clockwise;
          if (O_LENGTH(outline) > 1)
              append_pixel_outline(&outline_list, outline);
          LOG1("(%s)", (outline.open ? " open" : " closed"));
          LOG1(" [%u].\n", O_LENGTH(outline));
          if (O_LENGTH(outline) == 1)
              free_pixel_outline(&outline);
        }
  }
  if (test_cancel && test_cancel(testcancel_data)) {
      if (O_LIST_LENGTH (outline_list) != 0)
          free_pixel_outline_list (&outline_list);
  }
  free_bitmap(&marked);
  flush_log_output();
  return outline_list;
}



/* Add an outline to an outline list. */

static void
append_pixel_outline (pixel_outline_list_type *outline_list,
                      pixel_outline_type outline)
{
  O_LIST_LENGTH (*outline_list)++;
  REALLOCARRAY_NOFAIL(outline_list->data, outline_list->length);
  O_LIST_OUTLINE (*outline_list, O_LIST_LENGTH (*outline_list) - 1) = outline;
}


/* Free the list of outline lists. */

void
free_pixel_outline_list (pixel_outline_list_type *outline_list)
{
  unsigned this_outline;

  for (this_outline = 0; this_outline < outline_list->length; this_outline++)
    {
      pixel_outline_type o = outline_list->data[this_outline];
      free_pixel_outline (&o);
    }
  outline_list->length = 0;

  if (outline_list->data != NULL)
    {
      free (outline_list->data);
      outline_list->data = NULL;
    }

  flush_log_output ();
}


/* Return an empty list of pixels.  */


static pixel_outline_type
new_pixel_outline (void)
{
  pixel_outline_type pixel_outline;

  O_LENGTH (pixel_outline) = 0;
  pixel_outline.data = NULL;
  pixel_outline.open = false;

  return pixel_outline;
}

static void
free_pixel_outline (pixel_outline_type * outline)
{
  if (outline->data)
    {
      free (outline->data) ;
      outline->data   = NULL;
      outline->length = 0;
    }
}

/* Concatenate two pixel lists. The two lists are assumed to have the
   same starting pixel and to proceed in opposite directions therefrom. */

static void
concat_pixel_outline(pixel_outline_type *o1, const pixel_outline_type *o2)
{
  int src, dst;
  unsigned o1_length, o2_length;
  if (!o1 || !o2 || O_LENGTH(*o2) <= 1) return;

  o1_length = O_LENGTH(*o1);
  o2_length = O_LENGTH(*o2);
  O_LENGTH(*o1) += o2_length - 1;
  /* Resize o1 to the sum of the lengths of o1 and o2 minus one (because
     the two lists are assumed to share the same starting pixel). */
  REALLOCARRAY_NOFAIL(o1->data, O_LENGTH(*o1));
  /* Shift the contents of o1 to the end of the new array to make room
     to prepend o2. */
  for (src = o1_length - 1, dst = O_LENGTH(*o1) - 1; src >= 0; src--, dst--)
    O_COORDINATE(*o1, dst) = O_COORDINATE(*o1, src);
  /* Prepend the contents of o2 (in reverse order) to o1. */
  for (src = o2_length - 1, dst = 0; src > 0; src--, dst++)
    O_COORDINATE(*o1, dst) = O_COORDINATE(*o2, src);
}


/* If EDGE is not already marked, we mark it; otherwise, it's a fatal error.
   The position ROW and COL should be inside the bitmap MARKED. EDGE can be
   NO_EDGE. */

static void
mark_edge (edge_type edge, unsigned short row,
           unsigned short col, bitmap_type *marked)
{
  *BITMAP_PIXEL (*marked, row, col) |= 1 << edge;
}


/* Mark the direction of the pixel ROW/COL in MARKED. */

static void
mark_dir(unsigned short row, unsigned short col, direction_type dir, bitmap_type *marked)
{
  *BITMAP_PIXEL(*marked, row, col) |= 1 << dir;
}


/* Test if the direction of pixel at ROW/COL in MARKED is marked. */

static bool
is_marked_dir(unsigned short row, unsigned short col, direction_type dir, bitmap_type marked)
{
  return (bool)((*BITMAP_PIXEL(marked, row, col) & 1 << dir) != 0);
}


static bool
is_other_dir_marked(unsigned short row, unsigned short col, direction_type dir, bitmap_type marked)
{
  return (bool)((*BITMAP_PIXEL(marked, row, col) & (255 - (1 << dir) - (1 << ((dir + 4) % 8))) ) != 0);
}


/* Return the number of pixels adjacent to pixel ROW/COL that are black. */

static unsigned
num_neighbors(unsigned short row, unsigned short col, bitmap_type bitmap)
{
    unsigned dir, count = 0;
    pixel color = getBitmapColor(bitmap, row, col);
    for (dir = NORTH; dir <= NORTHEAST; dir++)
    {
	int delta_r = COMPUTE_DELTA(ROW, dir);
	int delta_c = COMPUTE_DELTA(COL, dir);
	unsigned int test_row = row + delta_r;
	unsigned int test_col = col + delta_c;
	if (BITMAP_VALID_PIXEL(bitmap, test_row, test_col)
	    && PPM_EQUAL(getBitmapColor(bitmap, test_row, test_col), color))
	    ++count;
    }
    return count;
}


/* Test if the edge EDGE at ROW/COL in MARKED is marked.  */

static bool
is_marked_edge (edge_type edge, unsigned short row, unsigned short col, bitmap_type marked)
{
  return
    (bool)(edge == NO_EDGE ? false : (*BITMAP_PIXEL (marked, row, col) & (1 << edge)) != 0);
}



static void
nextClockwisePointTop(bitmap_type         const bitmap,
                      edge_type *         const edge,
                      unsigned int *      const row,
                      unsigned int *      const col,
                      pixel               const color,
                      bitmap_type         const marked,
                      at_exception_type * const exp,
                      pm_pixelcoord *     const posP) {

    if ((*col >= 1 && !is_marked_edge(TOP, *row, *col-1, marked) &&
             is_outline_edge(TOP, bitmap, *row, *col-1, color, exp))) {

        /* WEST */

        *edge = TOP;
        --*col;
        posP->col = *col;
        posP->row = bitmap.height - *row;
        return;
    }

    RETURN_IF_FATAL();

    if ((*col >= 1 && *row >= 1 &&
         !is_marked_edge(RIGHT, *row-1, *col-1, marked) &&
         is_outline_edge(RIGHT, bitmap, *row-1, *col-1,
                         color, exp)) &&
        !(is_marked_edge(LEFT, *row-1, *col, marked) &&
          is_marked_edge(TOP, *row,*col-1, marked)) &&
        !(is_marked_edge(BOTTOM, *row-1, *col, marked) &&
          is_marked_edge(RIGHT, *row, *col-1, marked))) {

        /* NORTHWEST */

        *edge = RIGHT;
        --*col;
        --*row;
        posP->col = *col + 1;
        posP->row = bitmap.height - *row;
        return;
    } 

    RETURN_IF_FATAL();

    if ((!is_marked_edge(LEFT, *row, *col, marked)
         && is_outline_edge(LEFT, bitmap, *row, *col, color, exp))) {

        *edge = LEFT;
        posP->col = *col;
        posP->row = bitmap.height - *row - 1;
        return;
    }

    RETURN_IF_FATAL();

    *edge = NO_EDGE;
}



static void
nextClockwisePointRight(bitmap_type         const bitmap,
                        edge_type *         const edge,
                        unsigned int *      const row,
                        unsigned int *      const col,
                        pixel               const color,
                        bitmap_type         const marked,
                        at_exception_type * const exp,
                        pm_pixelcoord *     const posP) {

    if ((*row >= 1 &&
         !is_marked_edge(RIGHT, *row-1, *col, marked) &&
         is_outline_edge(RIGHT, bitmap, *row-1, *col, color, exp))) {

         /* NORTH */
        
        *edge = RIGHT;
        --*row;
        posP->col = *col+1;
        posP->row = bitmap.height - *row;
        return;
    }

    RETURN_IF_FATAL();
    
    if ((*col+1 < marked.width && *row >= 1 &&
         !is_marked_edge(BOTTOM, *row-1, *col+1, marked) &&
         is_outline_edge(BOTTOM, bitmap, *row-1, *col+1,
                         color, exp)) &&
        !(is_marked_edge(LEFT, *row, *col+1, marked) &&
          is_marked_edge(BOTTOM, *row-1, *col, marked)) &&
        !(is_marked_edge(TOP, *row, *col+1, marked) &&
          is_marked_edge(RIGHT, *row-1, *col, marked))) {

        /* NORTHEAST */
        *edge = BOTTOM;
        ++*col;
        --*row;
        posP->col = *col + 1;
        posP->row = bitmap.height - *row - 1;
        return;
    } 

    RETURN_IF_FATAL();

    if ((!is_marked_edge(TOP, *row, *col, marked) &&
         is_outline_edge(TOP, bitmap, *row, *col, color, exp))) {

        *edge = TOP;
        posP->col = *col;
        posP->row = bitmap.height - *row;
        return;
    }

    RETURN_IF_FATAL();

    *edge = NO_EDGE;
}



static void
nextClockwisePointBottom(bitmap_type         const bitmap,
                         edge_type *         const edge,
                         unsigned int *      const row,
                         unsigned int *      const col,
                         pixel               const color,
                         bitmap_type         const marked,
                         at_exception_type * const exp,
                         pm_pixelcoord *     const posP) {
    
    if ((*col+1 < marked.width &&
         !is_marked_edge(BOTTOM, *row, *col+1, marked) &&
         is_outline_edge(BOTTOM, bitmap, *row, *col+1, color, exp))) {

        /* EAST */

        *edge = BOTTOM;
        ++*col;
        posP->col = *col+1;
        posP->row = bitmap.height - *row - 1;
        return;
    }

    RETURN_IF_FATAL();

    if ((*col + 1 < marked.width && *row+1 < marked.height &&
         !is_marked_edge(LEFT, *row+1,*col+1, marked) &&
         is_outline_edge(LEFT, bitmap, *row+1, *col+1, color, exp)) &&
        !(is_marked_edge(TOP, *row+1, *col, marked) &&
          is_marked_edge(LEFT, *row, *col+1, marked)) &&
        !(is_marked_edge(RIGHT, *row+1, *col, marked) &&
          is_marked_edge(BOTTOM, *row, *col+1, marked))) {

        /* SOUTHEAST */

        *edge = LEFT;
        ++*col;
        ++*row;
        posP->col = *col;
        posP->row = bitmap.height - *row - 1;
        return;
    }

    RETURN_IF_FATAL();

    if ((!is_marked_edge(RIGHT, *row, *col, marked) &&
         is_outline_edge(RIGHT, bitmap, *row, *col, color, exp))) {

        *edge = RIGHT;
        posP->col = *col+1;
        posP->row = bitmap.height - *row;
        return;
    }

    RETURN_IF_FATAL();

    *edge = NO_EDGE;
}



static void
nextClockwisePointLeft(bitmap_type         const bitmap,
                       edge_type *         const edge,
                       unsigned int *      const row,
                       unsigned int *      const col,
                       pixel               const color,
                       bitmap_type         const marked,
                       at_exception_type * const exp,
                       pm_pixelcoord *     const posP) {

    if ((*row+1 < marked.height &&
         !is_marked_edge(LEFT, *row+1, *col, marked) &&
         is_outline_edge(LEFT, bitmap, *row+1, *col, color, exp))) {

        /* SOUTH */

        *edge = LEFT;
        ++*row;
        posP->col = *col;
        posP->row = bitmap.height - *row - 1;
        return;
    }

    RETURN_IF_FATAL();

    if ((*col >= 1 && *row+1 < marked.height &&
         !is_marked_edge(TOP, *row+1, *col-1, marked) &&
         is_outline_edge(TOP, bitmap, *row+1, *col-1, color, exp)) &&
        !(is_marked_edge(RIGHT, *row, *col-1, marked) &&
          is_marked_edge(TOP, *row+1, *col, marked)) &&
        !(is_marked_edge(BOTTOM, *row, *col-1, marked) &&
          is_marked_edge(LEFT, *row+1, *col, marked))) {
        
        /* SOUTHWEST */
        
        *edge = TOP;
        --*col;
        ++*row;
        posP->col = *col;
        posP->row = bitmap.height - *row;
        return;
    } 

    RETURN_IF_FATAL();

    if ((!is_marked_edge(BOTTOM, *row, *col, marked) &&
         is_outline_edge(BOTTOM, bitmap, *row, *col, color, exp))) {
        *edge = BOTTOM;
        posP->col = *col+1;
        posP->row = bitmap.height - *row - 1;
        return;
    }
    RETURN_IF_FATAL();

    *edge = NO_EDGE;
}



static void
nextClockwisePoint(bitmap_type         const bitmap,
                   edge_type *         const edge,
                   unsigned int *      const row,
                   unsigned int *      const col,
                   pixel               const color,
                   bitmap_type         const marked,
                   at_exception_type * const exp,
                   pm_pixelcoord *     const posP) {
    
    switch (*edge) {
    case TOP:
        nextClockwisePointTop(  bitmap, edge, row, col, color,
                                marked, exp, posP);
        break;
    case RIGHT: 
        nextClockwisePointRight(bitmap, edge, row, col, color,
                                marked, exp, posP);
        break;
    case BOTTOM: 
        nextClockwisePointBottom(bitmap, edge, row, col, color,
                                 marked, exp, posP);
        break;
    case LEFT: 
        nextClockwisePointLeft(  bitmap, edge, row, col, color,
                                 marked, exp, posP);
        break;
    case NO_EDGE:
        break;
    default:
        *edge = NO_EDGE;
        break;
    }
}



static void
nextCcwPointTop(bitmap_type         const bitmap,
                edge_type *         const edge,
                unsigned int *      const row,
                unsigned int *      const col,
                pixel               const color,
                bitmap_type         const marked,
                at_exception_type * const exp,
                pm_pixelcoord *     const posP) {

    if ((!is_marked_edge(LEFT, *row, *col, marked) &&
         is_outline_edge(LEFT,bitmap,*row,*col, color, exp))) {

        *edge = LEFT;
        posP->col = *col;
        posP->row = bitmap.height - *row - 1;
        return;
    }

    RETURN_IF_FATAL();

    if ((*col >= 1 &&
         !is_marked_edge(TOP, *row, *col-1, marked) &&
         is_outline_edge(TOP, bitmap, *row, *col-1, color, exp))) {

        /* WEST */

        *edge = TOP;
        --*col;
        posP->col = *col;
        posP->row = bitmap.height - *row;
        return;
    }

    RETURN_IF_FATAL();
    
    if ((*col >= 1 && *row >= 1 &&
         !is_marked_edge(RIGHT, *row-1, *col-1, marked) &&
         is_outline_edge(RIGHT, bitmap, *row-1, *col-1, color, exp))) {

        /* NORTHWEST */

        *edge = RIGHT;
        --*col;
        --*row;
        posP->col = *col + 1;
        posP->row = bitmap.height - *row;
        return;
    } 

    RETURN_IF_FATAL();

    *edge = NO_EDGE;
}



static void
nextCcwPointRight(bitmap_type         const bitmap,
                  edge_type *         const edge,
                  unsigned int *      const row,
                  unsigned int *      const col,
                  pixel               const color,
                  bitmap_type         const marked,
                  at_exception_type * const exp,
                  pm_pixelcoord *     const posP) {

    if ((!is_marked_edge(TOP, *row, *col, marked) &&
         is_outline_edge(TOP, bitmap, *row, *col, color, exp))) {

        *edge = TOP;
        posP->col = *col;
        posP->row = bitmap.height - *row;
        return;
    }

    RETURN_IF_FATAL();

    if ((*row >= 1 &&
         !is_marked_edge(RIGHT, *row-1, *col, marked) &&
         is_outline_edge(RIGHT, bitmap, *row-1, *col, color, exp))) {

        /* NORTH */
        
        *edge = RIGHT;
        --*row;
        posP->col = *col + 1;
        posP->row = bitmap.height - *row;
        return;
    }

    RETURN_IF_FATAL();

    if ((*col + 1 < marked.width && *row >= 1 &&
         !is_marked_edge(BOTTOM, *row-1, *col+1, marked) &&
         is_outline_edge(BOTTOM, bitmap, *row-1, *col+1, color, exp))) {

        /* NORTHEAST */

        *edge = BOTTOM;
        ++*col;
        ++*row;
        posP->col = *col + 1;
        posP->row = bitmap.height - *row - 1;
        return;
    }

    RETURN_IF_FATAL();

    *edge = NO_EDGE;
}



static void
nextCcwPointBottom(bitmap_type         const bitmap,
                   edge_type *         const edge,
                   unsigned int *      const row,
                   unsigned int *      const col,
                   pixel               const color,
                   bitmap_type         const marked,
                   at_exception_type * const exp,
                   pm_pixelcoord *     const posP) {

    if ((!is_marked_edge(RIGHT, *row, *col, marked) &&
         is_outline_edge(RIGHT, bitmap, *row, *col, color, exp))) {

        *edge = RIGHT;
        posP->col = *col + 1;
        posP->row = bitmap.height - *row;
        return;
    }

    RETURN_IF_FATAL();

    if ((*col + 1 < marked.width &&
         !is_marked_edge(BOTTOM, *row, *col+1, marked) &&
         is_outline_edge(BOTTOM, bitmap, *row, *col+1, color, exp))) {

        /* EAST */

        *edge = BOTTOM;
        ++*col;
        posP->col = *col + 1;
        posP->row = bitmap.height - *row - 1;
        return;
    }

    RETURN_IF_FATAL();

    if ((*col + 1 < marked.width && *row + 1 < marked.height &&
         !is_marked_edge(LEFT, *row+1, *col+1, marked) &&
         is_outline_edge(LEFT, bitmap, *row+1, *col+1, color, exp))) {

        /* SOUTHEAST */

        *edge = LEFT;
        ++*col;
        ++*row;
        posP->col = *col;
        posP->row = bitmap.height - *row - 1;
        return;
    }

    RETURN_IF_FATAL();
    
    *edge = NO_EDGE;
}



static void
nextCcwPointLeft(bitmap_type         const bitmap,
                 edge_type *         const edge,
                 unsigned int *      const row,
                 unsigned int *      const col,
                 pixel               const color,
                 bitmap_type         const marked,
                 at_exception_type * const exp,
                 pm_pixelcoord *     const posP) {


    if ((!is_marked_edge(BOTTOM, *row, *col, marked) &&
         is_outline_edge(BOTTOM, bitmap, *row, *col, color, exp))) {

        *edge = BOTTOM;
        posP->col = *col + 1;
        posP->row = bitmap.height - *row - 1;
        return;
    }

    RETURN_IF_FATAL();

    if ((*row + 1 < marked.height &&
         !is_marked_edge(LEFT, *row+1, *col, marked) &&
         is_outline_edge(LEFT, bitmap, *row+1, *col, color, exp))) {

        /* SOUTH */

        *edge = LEFT;
        ++*row;
        posP->col = *col;
        posP->row = bitmap.height - *row - 1;
        return;
    }

    RETURN_IF_FATAL();
    
    if ((*col >= 1 && *row + 1 < marked.height &&
         !is_marked_edge(TOP, *row+1, *col-1, marked) &&
         is_outline_edge(TOP, bitmap, *row+1, *col-1, color, exp))) {

        /* SOUTHWEST */

        *edge = TOP;
        --*col;
        ++*row;
        posP->col = *col;
        posP->row = bitmap.height - *row;
        return;
    }

    RETURN_IF_FATAL();

    *edge = NO_EDGE;
}

static void
nextCounterClockwisePoint(bitmap_type         const bitmap,
                          edge_type *         const edge,
                          unsigned int *      const row,
                          unsigned int *      const col,
                          pixel               const color,
                          bitmap_type         const marked,
                          at_exception_type * const exp,
                          pm_pixelcoord *     const posP) {

    switch (*edge) {
    case TOP:
        nextCcwPointTop(   bitmap, edge, row, col, color, marked, exp, posP);
        break;
    case RIGHT: 
        nextCcwPointRight( bitmap, edge, row, col, color, marked, exp, posP);
        break;
    case BOTTOM: 
        nextCcwPointBottom(bitmap, edge, row, col, color, marked, exp, posP);
        break;
    case LEFT: 
        nextCcwPointLeft(  bitmap, edge, row, col, color, marked, exp, posP);
        break;
    case NO_EDGE:
        break;
    default: 
        *edge = NO_EDGE;
        break;
    }
}



static void
nextPoint(bitmap_type         const bitmap,
          edge_type *         const edge,
          unsigned int *      const row,
          unsigned int *      const col,
          pm_pixelcoord *     const nextPointP,
          pixel               const color,
          bool                const clockwise,
          bitmap_type         const marked,
          at_exception_type * const exp) {

    if (!clockwise)
        nextClockwisePoint(       bitmap, edge, row, col, color,
                                  marked, exp, nextPointP);
    else
        nextCounterClockwisePoint(bitmap, edge, row, col, color,
                                  marked, exp, nextPointP);
}



static pixel_outline_type
find_one_outline(bitmap_type         const bitmap,
                 edge_type           const originalEdge,
                 unsigned int        const originalRow,
                 unsigned int        const originalCol,
                 bitmap_type *       const marked,
                 bool                const clockwise,
                 bool                const ignore,
                 at_exception_type * const exp) {
/*----------------------------------------------------------------------------
  Calculate one single outline.  We pass the position of the
  starting pixel and the starting edge.  We mark all edges we track along
  and append the outline pixels to the coordinate list.
-----------------------------------------------------------------------------*/
    unsigned int row;
    unsigned int col;
    edge_type    edge;

    pixel_outline_type outline;
    pm_pixelcoord pos;
    
    outline = new_pixel_outline();
    outline.color = getBitmapColor(bitmap, originalRow, originalCol);

    row  = originalRow;   /* initial value */
    col  = originalCol;   /* initial value */
    edge = originalEdge;  /* initial value */

    /* Set initial position */
    pos.col = col + ((edge == RIGHT) || (edge == BOTTOM) ? 1 : 0);
    pos.row = bitmap.height - row - 1 + 
        (edge == TOP || edge == RIGHT ? 1 : 0);

    do {
        /* Put this edge into the output list */
        if (!ignore) {
            LOG2(" (%d,%d)", pos.col, pos.row);
            append_outline_pixel(&outline, pos);
        }
        
        mark_edge(edge, row, col, marked);
        nextPoint(bitmap, &edge, &row, &col, &pos, outline.color, clockwise,
                  *marked, exp);
            /* edge, row, and col are both input and output in the above */
    } while (edge != NO_EDGE && !at_exception_got_fatal(exp));

    if (ignore || at_exception_got_fatal(exp))
        free_pixel_outline(&outline);

    return outline;
}



pixel_outline_list_type
find_outline_pixels(bitmap_type         const bitmap,
                    bool                const bg_spec,
                    pixel               const bg_color, 
                    at_progress_func          notify_progress,
                    void *              const progress_data,
                    at_testcancel_func        test_cancel,
                    void *              const testcancel_data,
                    at_exception_type * const exp) {
/*----------------------------------------------------------------------------
   Return a list of outlines in the image whose raster is 'bitmap'.

   The background color of the image is 'bg_color' if 'bg_spec' is true;
   otherwise, there is no background color.
-----------------------------------------------------------------------------*/
    /* We go through a bitmap TOP to BOTTOM, LEFT to RIGHT, looking for
       each pixel with an unmarked edge that we consider a starting point
       of an outline.  When we find one, we trace the outline and add it
       to the list, marking the edges in it as we go.
    */
    unsigned int const max_progress = bitmap.height * bitmap.width;
    
    pixel_outline_list_type outline_list;
    unsigned int row;
    bitmap_type marked;
    
    marked = new_bitmap (bitmap.width, bitmap.height);
    
    O_LIST_LENGTH(outline_list) = 0;
    outline_list.data = NULL;
    
    for (row = 0; row < bitmap.height; ++row) {
        unsigned int col;
        for (col = 0; col < bitmap.width; ++col) {
            pixel const color = getBitmapColor(bitmap, row, col);
            bool const is_background =
                bg_spec && PPM_EQUAL(color, bg_color);

            if (notify_progress)
                notify_progress((float)(row * bitmap.width + col) /
                                ((float) max_progress * (float)3.0),
                                progress_data);

            /* A valid edge can be TOP for an outside outline.
               Outside outlines are traced counterclockwise.
            */

            if (!is_background &&
                is_unmarked_outline_edge(row, col, TOP,
                                         bitmap, marked, color, exp)) {
                pixel_outline_type outline;
                
                CHECK_FATAL();   /* FREE(DONE) outline_list */
                
                LOG1("#%u: (counterclockwise)", O_LIST_LENGTH(outline_list));
                
                outline = find_one_outline(bitmap, TOP, row, col, &marked,
                                           false, false, exp);
                CHECK_FATAL();    /* FREE(DONE) outline_list */
                
                O_CLOCKWISE(outline) = false;
                append_pixel_outline(&outline_list, outline);
                
                LOG1(" [%u].\n", O_LENGTH (outline));
            } else
                CHECK_FATAL ();	/* FREE(DONE) outline_list */

            /* A valid edge can be BOTTOM for an inside outline.
               Inside outlines are traced clockwise.
            */
            if (row > 0) {
                pixel const colorAbove = getBitmapColor(bitmap, row-1, col);
                if (!(bg_spec && PPM_EQUAL(colorAbove, bg_color)) &&
                    is_unmarked_outline_edge(row-1, col, BOTTOM,
                                             bitmap, marked, colorAbove,exp)) {
                    CHECK_FATAL(); /* FREE(DONE) outline_list */
                    
                    /* This lines are for debugging only:*/
                    if (is_background) {
                        pixel_outline_type outline;
                    
                        LOG1("#%u: (clockwise)", O_LIST_LENGTH(outline_list));
                        
                        outline = find_one_outline(bitmap, BOTTOM, row-1, col,
                                                   &marked, true, false, exp);
                        CHECK_FATAL(); /* FREE(DONE) outline_list */
                        
                        O_CLOCKWISE(outline) = true;
                        append_pixel_outline(&outline_list, outline);
                        
                        LOG1(" [%u].\n", O_LENGTH(outline));
                    } else {
                        find_one_outline(bitmap, BOTTOM, row-1, col,
                                         &marked, true, true, exp);
                        CHECK_FATAL(); /* FREE(DONE) outline_list */
                    }
                } else
                    CHECK_FATAL();	/* FREE(DONE) outline_list */
            }
            if (test_cancel && test_cancel(testcancel_data)) {
                free_pixel_outline_list(&outline_list);
                goto cleanup;
            }
        }
    }
 cleanup:
    free_bitmap(&marked);
    flush_log_output();
    if (at_exception_got_fatal(exp))
        free_pixel_outline_list(&outline_list);

    return outline_list;
}

