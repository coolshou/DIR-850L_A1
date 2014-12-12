/* fit.c: turn a bitmap representation of a curve into a list of splines.
    Some of the ideas, but not the code, comes from the Phoenix thesis.
   See README for the reference.

   The code was partially derived from limn.

   Copyright (C) 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <assert.h>

#include "pm_c_util.h"
#include "mallocvar.h"

#include "autotrace.h"
#include "fit.h"
#include "message.h"
#include "logreport.h"
#include "spline.h"
#include "vector.h"
#include "curve.h"
#include "pxl-outline.h"
#include "epsilon-equal.h"

#define CUBE(x) ((x) * (x) * (x))

/* We need to manipulate lists of array indices.  */

typedef struct index_list
{
  unsigned *data;
  unsigned length;
} index_list_type;

/* The usual accessor macros.  */
#define GET_INDEX(i_l, n)  ((i_l).data[n])
#define INDEX_LIST_LENGTH(i_l)  ((i_l).length)
#define GET_LAST_INDEX(i_l)  ((i_l).data[INDEX_LIST_LENGTH (i_l) - 1])

static void append_index (index_list_type *, unsigned);
static void free_index_list (index_list_type *);
static index_list_type new_index_list (void);
static void remove_adjacent_corners (index_list_type *, unsigned, bool,
                     at_exception_type * exception);
static void filter (curve_type, fitting_opts_type *);
static void find_vectors
  (unsigned const, pixel_outline_type const, vector_type * const, vector_type * const, unsigned const);
static float find_error (curve_type, spline_type, unsigned *,
               at_exception_type * exception);
static vector_type find_half_tangent (curve_type, bool start, unsigned *, unsigned);
static void find_tangent (curve_type, bool, bool, unsigned);
static void remove_knee_points (curve_type const, bool const);
static void set_initial_parameter_values (curve_type);
static float distance (float_coord, float_coord);


static pm_pixelcoord
real_to_int_coord(float_coord const real_coord) {
/*----------------------------------------------------------------------------
  Turn an real point into a integer one.
-----------------------------------------------------------------------------*/

    pm_pixelcoord int_coord;

    int_coord.col = ROUND(real_coord.x);
    int_coord.row = ROUND(real_coord.y);
    
    return int_coord;
}


static void
appendCorner(index_list_type *  const cornerListP,
             unsigned int       const pixelSeq,
             pixel_outline_type const outline,
             float              const angle,
             char               const logType) {

    pm_pixelcoord const coord = O_COORDINATE(outline, pixelSeq);

    append_index(cornerListP, pixelSeq);
    LOG4(" (%d,%d)%c%.3f", coord.col, coord.row, logType, angle);
}



static void
lookAheadForBetterCorner(pixel_outline_type  const outline,
                         unsigned int        const basePixelSeq,
                         float               const baseCornerAngle,
                         unsigned int        const cornerSurround,
                         unsigned int        const cornerAlwaysThreshold,
                         unsigned int *      const highestExaminedP,
                         float *             const bestCornerAngleP,
                         unsigned int *      const bestCornerIndexP,
                         index_list_type *   const equallyGoodListP,
                         index_list_type *   const cornerListP,
                         at_exception_type * const exceptionP) {
/*----------------------------------------------------------------------------
   'basePixelSeq' is the sequence position within 'outline' of a pixel
   that has a sufficiently small angle (to wit 'baseCornerAngle') to
   be a corner.  We look ahead in 'outline' for an even better one.
   We'll look up to 'cornerSurround' pixels ahead.

   We return the pixel sequence of the best corner we find (which could
   be the base) as *bestCornerIndexP.  Its angle is *bestCornerAngleP.

   We return as *highestExaminedP the pixel sequence of the last pixel
   we examined in our search (Caller can use this information to avoid
   examining them again).

   And we have this really dirty side effect: If we encounter any
   corner whose angle is less than 'cornerAlwaysThreshold', we add
   that to the list *cornerListP along the way.
-----------------------------------------------------------------------------*/
    float bestCornerAngle;
    unsigned bestCornerIndex;
    index_list_type equallyGoodList;
    unsigned int q;
    unsigned int i;

    bestCornerIndex = basePixelSeq;     /* initial assumption */
    bestCornerAngle = baseCornerAngle;    /* initial assumption */
    
    equallyGoodList = new_index_list();
    
    q = basePixelSeq;
    i = basePixelSeq + 1;  /* Start with the next pixel */
    
    while (i < bestCornerIndex + cornerSurround &&
           i < O_LENGTH(outline) &&
           !at_exception_got_fatal(exceptionP)) {

        vector_type inVector, outVector;
        float cornerAngle;
        
        /* Check the angle.  */

        q = i % O_LENGTH(outline);
        find_vectors(q, outline, &inVector, &outVector, cornerSurround);
        cornerAngle = Vangle(inVector, outVector, exceptionP);
        if (!at_exception_got_fatal(exceptionP)) {
            /* Perhaps the angle is sufficiently small that we want to
               consider this a corner, even if it's not the best
               (unless we've already wrapped around in the search, in
               which case we have already added the corner, and we
               don't want to add it again).
            */
            if (cornerAngle <= cornerAlwaysThreshold && q >= basePixelSeq)
                appendCorner(cornerListP, q, outline, cornerAngle, '\\');

            if (epsilon_equal(cornerAngle, bestCornerAngle))
                append_index(&equallyGoodList, q);
            else if (cornerAngle < bestCornerAngle) {
                bestCornerAngle = cornerAngle;
                /* We want to check `cornerSurround' pixels beyond the
                   new best corner.
                */
                i = bestCornerIndex = q;
                free_index_list(&equallyGoodList);
                equallyGoodList = new_index_list();
            }
            ++i;
        }
    }
    *bestCornerAngleP = bestCornerAngle;
    *bestCornerIndexP = bestCornerIndex;
    *equallyGoodListP = equallyGoodList;
    *highestExaminedP = q;
}



static void
establishCornerSearchLimits(pixel_outline_type  const outline,
                            fitting_opts_type * const fittingOptsP,
                            unsigned int *      const firstP,
                            unsigned int *      const lastP) {
/*----------------------------------------------------------------------------
   Determine where in the outline 'outline' we should look for corners.
-----------------------------------------------------------------------------*/
    assert(O_LENGTH(outline) >= 1);
    assert(O_LENGTH(outline) - 1 >= fittingOptsP->corner_surround);

    *firstP = 0;
    *lastP  = O_LENGTH(outline) - 1;
    if (outline.open) {
        *firstP += fittingOptsP->corner_surround;
        *lastP  -= fittingOptsP->corner_surround;
    }
}



static void
removeAdjacent(index_list_type *   const cornerListP,
               pixel_outline_type  const outline,
               fitting_opts_type * const fittingOptsP,
               at_exception_type * const exception) {
               
    /* We never want two corners next to each other, since the
       only way to fit such a ``curve'' would be with a straight
       line, which usually interrupts the continuity dreadfully.
    */

    if (INDEX_LIST_LENGTH(*cornerListP) > 0)
        remove_adjacent_corners(
            cornerListP,
            O_LENGTH(outline) - (outline.open ? 2 : 1),
            fittingOptsP->remove_adjacent_corners,
            exception);
}



static index_list_type
find_corners(pixel_outline_type  const outline,
             fitting_opts_type * const fittingOptsP,
             at_exception_type * const exceptionP) {

    /* We consider a point to be a corner if (1) the angle defined by
       the `corner_surround' points coming into it and going out from
       it is less than `corner_threshold' degrees, and no point within
       `corner_surround' points has a smaller angle; or (2) the angle
       is less than `corner_always_threshold' degrees.
    */
    unsigned int p;
    unsigned int firstPixelSeq, lastPixelSeq;
    index_list_type cornerList;

    cornerList = new_index_list();

    if (O_LENGTH(outline) <= fittingOptsP->corner_surround * 2 + 1)
        return cornerList;

    establishCornerSearchLimits(outline, fittingOptsP,
                                &firstPixelSeq, &lastPixelSeq);
    
    /* Consider each pixel on the outline in turn.  */
    for (p = firstPixelSeq; p <= lastPixelSeq;) {
        vector_type inVector, outVector;
        float cornerAngle;

        /* Check if the angle is small enough.  */
        find_vectors(p, outline, &inVector, &outVector,
                     fittingOptsP->corner_surround);
        cornerAngle = Vangle(inVector, outVector, exceptionP);
        if (at_exception_got_fatal(exceptionP))
            goto cleanup;

        if (fabs(cornerAngle) <= fittingOptsP->corner_threshold) {
            /* We want to keep looking, instead of just appending the
               first pixel we find with a small enough angle, since there
               might be another corner within `corner_surround' pixels, with
               a smaller angle.  If that is the case, we want that one.

               If we come across a corner that is just as good as the
               best one, we should make it a corner, too.  This
               happens, for example, at the points on the `W' in some
               typefaces, where the "points" are flat.
            */
            float bestCornerAngle;
            unsigned bestCornerIndex;
            index_list_type equallyGoodList;
            unsigned int q;

            if (cornerAngle <= fittingOptsP->corner_always_threshold)
                /* The angle is sufficiently small that we want to
                   consider this a corner, even if it's not the best.
                */
                appendCorner(&cornerList, p, outline, cornerAngle, '\\');

            lookAheadForBetterCorner(outline, p, cornerAngle,
                                     fittingOptsP->corner_surround,
                                     fittingOptsP->corner_always_threshold,
                                     &q,
                                     &bestCornerAngle, &bestCornerIndex,
                                     &equallyGoodList,
                                     &cornerList,
                                     exceptionP);

            if (at_exception_got_fatal(exceptionP))
                goto cleanup;

            /* `q' is the index of the last point lookAhead checked.
               He added the corner if `bestCornerAngle' is less than
               `corner_always_threshold'.  If we've wrapped around, we
               added the corner on the first pass.  Otherwise, we add
               the corner now.
            */
            if (bestCornerAngle > fittingOptsP->corner_always_threshold
                && bestCornerIndex >= p) {

                unsigned int j;
                
                appendCorner(&cornerList, bestCornerIndex,
                             outline, bestCornerAngle, '/');
                
                for (j = 0; j < INDEX_LIST_LENGTH (equallyGoodList); ++j)
                    appendCorner(&cornerList, GET_INDEX(equallyGoodList, j),
                                 outline, bestCornerAngle, '@');
            }
            free_index_list(&equallyGoodList);

            /* If we wrapped around in our search, we're done;
               otherwise, we move on to the pixel after the highest
               one we just checked.
            */
            p = (q < p) ? O_LENGTH(outline) : q + 1;
        } else
            ++p;
    }
    removeAdjacent(&cornerList, outline, fittingOptsP, exceptionP);

cleanup:  
    return cornerList;
}



static void
makeOutlineOneCurve(pixel_outline_type const outline,
                    curve_list_type *  const curveListP) {
/*----------------------------------------------------------------------------
   Add to *curveListP a single curve that represents the outline 'outline'.
-----------------------------------------------------------------------------*/
    curve_type curve;
    unsigned int pixelSeq;

    curve = new_curve();
    
    for (pixelSeq = 0; pixelSeq < O_LENGTH(outline); ++pixelSeq)
        append_pixel(curve, O_COORDINATE(outline, pixelSeq));
    
    if (curveListP->open)
        CURVE_CYCLIC(curve) = false;
    else
        CURVE_CYCLIC(curve) = true;
    
    /* Make it a one-curve cycle */
    NEXT_CURVE(curve)     = curve;
    PREVIOUS_CURVE(curve) = curve;

    append_curve(curveListP, curve);
}



static void
addCurveStartingAtCorner(pixel_outline_type const outline,
                         index_list_type    const cornerList,
                         unsigned int       const cornerSeq,
                         curve_list_type *  const curveListP) {

    unsigned int const cornerPixelSeq = GET_INDEX(cornerList, cornerSeq);
    
    unsigned int lastPixelSeq;
    curve_type curve;
    unsigned int pixelSeq;
    
    if (cornerSeq + 1 >= cornerList.length)
        /* No more corners, so we go through the end of the outline. */
        lastPixelSeq = O_LENGTH(outline) - 1;
    else
        /* Go through the next corner */
        lastPixelSeq = GET_INDEX(cornerList, cornerSeq + 1);
    
    curve = new_curve();

    for (pixelSeq = cornerPixelSeq; pixelSeq <= lastPixelSeq; ++pixelSeq)
        append_pixel(curve, O_COORDINATE(outline, pixelSeq));
    
    {
        /* Add curve to end of chain */
        if (!CURVE_LIST_EMPTY(*curveListP)) {
            curve_type const previousCurve = LAST_CURVE_LIST_ELT(*curveListP);
            NEXT_CURVE(previousCurve) = curve;
            PREVIOUS_CURVE(curve)     = previousCurve;
        }
    }
    append_curve(curveListP, curve);
}



static void
divideOutlineWithCorners(pixel_outline_type const outline,
                         index_list_type    const cornerList,
                         curve_list_type *  const curveListP) {
/*----------------------------------------------------------------------------
   Divide the outline 'outline' into curves at the corner points
   'cornerList' and add each curve to *curveListP.

   Each curve contains the corners at each end.

   The last curve is special.  It consists of the pixels (inclusive)
   between the last corner and the end of the outline, and the
   beginning of the outline and the first corner.

   We link the curves in a chain.  If the outline (and therefore the
   curve list) is closed, the chain is a cycle of all the curves.  If
   it is open, the chain is a linear chain of all the curves except
   the last one (the one that goes from the last corner to the first
   corner).

   Assume there is at least one corner.
-----------------------------------------------------------------------------*/
    unsigned int const firstCurveSeq = CURVE_LIST_LENGTH(*curveListP);
        /* Index in curve list of the first curve we add */
    unsigned int cornerSeq;

    assert(cornerList.length > 0);

    if (outline.open) {
        /* Start with a curve that contains the point up to the first
           corner
        */
        curve_type curve;
        unsigned int pixelSeq;
        
        curve = new_curve();

        for (pixelSeq = 0; pixelSeq <= GET_INDEX(cornerList, 0); ++pixelSeq)
            append_pixel(curve, O_COORDINATE(outline, pixelSeq));

        append_curve(curveListP, curve);
    } else
        /* We'll pick up the pixels before the first corner at the end */

    /* Add to the list a curve that starts at each corner and goes
       through the following corner, or the end of the outline if
       there is no following corner.  Do it in order of the corners.
    */
    for (cornerSeq = 0; cornerSeq < cornerList.length; ++cornerSeq)
        addCurveStartingAtCorner(outline, cornerList, cornerSeq, curveListP);

    if (!outline.open) {
        /* Come around to the start of the curve list -- add the pixels
           before the first corner to the last curve, and chain the last
           curve to the first one.
        */
        curve_type const firstCurve =
            CURVE_LIST_ELT(*curveListP, firstCurveSeq);
        curve_type const lastCurve  =
            LAST_CURVE_LIST_ELT(*curveListP);

        unsigned int pixelSeq;

        for (pixelSeq = 0; pixelSeq <= GET_INDEX(cornerList, 0); ++pixelSeq)
            append_pixel(lastCurve, O_COORDINATE(outline, pixelSeq));

        NEXT_CURVE(lastCurve)      = firstCurve;
        PREVIOUS_CURVE(firstCurve) = lastCurve;
    }
}



static curve_list_array_type
split_at_corners(pixel_outline_list_type const pixel_list,
                 fitting_opts_type *     const fitting_opts,
                 at_exception_type *     const exception) {
/*----------------------------------------------------------------------------
   Find the corners in PIXEL_LIST, the list of points.  (Presumably we
   can't fit a single spline around a corner.)  The general strategy
   is to look through all the points, remembering which we want to
   consider corners.  Then go through that list, producing the
   curve_list.  This is dictated by the fact that PIXEL_LIST does not
   necessarily start on a corner---it just starts at the character's
   first outline pixel, going left-to-right, top-to-bottom.  But we
   want all our splines to start and end on real corners.

   For example, consider the top of a capital `C' (this is in cmss20):
                     x
                     ***********
                  ******************

   PIXEL_LIST will start at the pixel below the `x'.  If we considered
   this pixel a corner, we would wind up matching a very small segment
   from there to the end of the line, probably as a straight line, which
   is certainly not what we want.

   PIXEL_LIST has one element for each closed outline on the character.
   To preserve this information, we return an array of curve_lists, one
   element (which in turn consists of several curves, one between each
   pair of corners) for each element in PIXEL_LIST.
-----------------------------------------------------------------------------*/
    unsigned outlineSeq;
    curve_list_array_type curve_array;

    curve_array = new_curve_list_array();

    LOG("\nFinding corners:\n");

    for (outlineSeq = 0;
         outlineSeq < O_LIST_LENGTH(pixel_list);
         ++outlineSeq) {

        pixel_outline_type const outline =
            O_LIST_OUTLINE(pixel_list, outlineSeq);

        index_list_type corner_list;
        curve_list_type curve_list;

        curve_list = new_curve_list();

        CURVE_LIST_CLOCKWISE(curve_list) = O_CLOCKWISE(outline);
        curve_list.color = outline.color;
        curve_list.open  = outline.open;

        LOG1("#%u:", outlineSeq);
        
        /* If the outline does not have enough points, we can't do
           anything.  The endpoints of the outlines are automatically
           corners.  We need at least `corner_surround' more pixels on
           either side of a point before it is conceivable that we might
           want another corner.
        */
        if (O_LENGTH(outline) > fitting_opts->corner_surround * 2 + 2)
            corner_list = find_corners(outline, fitting_opts, exception);

        else {
            int const surround = (O_LENGTH(outline) - 3) / 2;
            if (surround >= 2) {
                unsigned int const save_corner_surround =
                    fitting_opts->corner_surround;
                fitting_opts->corner_surround = surround;
                corner_list = find_corners(outline, fitting_opts, exception);
                fitting_opts->corner_surround = save_corner_surround;
            } else {
                corner_list.length = 0;
                corner_list.data = NULL;
            }
        }

        if (corner_list.length == 0)
            /* No corners.  Use all of the pixel outline as the one curve. */
            makeOutlineOneCurve(outline, &curve_list);
        else
            divideOutlineWithCorners(outline, corner_list, &curve_list);
        
        LOG1(" [%u].\n", corner_list.length);
        free_index_list(&corner_list);

        /* And now add the just-completed curve list to the array.  */
        append_curve_list(&curve_array, curve_list);
    }

    return curve_array;
}



static void
removeKnees(curve_list_type const curveList) {
/*----------------------------------------------------------------------------
  Remove the extraneous ``knee'' points before filtering.  Since the
  corners have already been found, we don't need to worry about
  removing a point that should be a corner.
-----------------------------------------------------------------------------*/
    unsigned int curveSeq;
    
    LOG("\nRemoving knees:\n");
    for (curveSeq = 0; curveSeq < curveList.length; ++curveSeq) {
        LOG1("#%u:", curveSeq);
        remove_knee_points(CURVE_LIST_ELT(curveList, curveSeq),
                           CURVE_LIST_CLOCKWISE(curveList));
    }
}
    


static void
computePointWeights(curve_list_type     const curveList,
                    fitting_opts_type * const fittingOptsP,
                    distance_map_type * const distP) {

    unsigned int const height = distP->height;

    unsigned int curveSeq;
    
    for (curveSeq = 0; curveSeq < curveList.length; ++curveSeq) {
        unsigned pointSeq;
        curve_type const curve = CURVE_LIST_ELT(curveList, curveSeq);
        for (pointSeq = 0; pointSeq < CURVE_LENGTH(curve); ++pointSeq) {
            float_coord * const coordP = &CURVE_POINT(curve, pointSeq);
            unsigned int x = coordP->x;
            unsigned int y = height - (unsigned int)coordP->y - 1;
            
            float width, w;

            /* Each (x, y) is a point on the skeleton of the curve, which
               might be offset from the true centerline, where the width
               is maximal.  Therefore, use as the local line width the
               maximum distance over the neighborhood of (x, y). 
            */
            width = distP->d[y][x];  /* initial value */
            if (y - 1 >= 0) {
                if ((w = distP->d[y-1][x]) > width)
                    width = w;
                if (x - 1 >= 0) {
                    if ((w = distP->d[y][x-1]) > width)
                        width = w;
                    if ((w = distP->d[y-1][x-1]) > width)
                        width = w;
                }
                if (x + 1 < distP->width) {
                    if ((w = distP->d[y][x+1]) > width)
                        width = w;
                    if ((w = distP->d[y-1][x+1]) > width)
                        width = w;
                }
            }
            if (y + 1 < height) {
                if ((w = distP->d[y+1][x]) > width)
                    width = w;
                if (x - 1 >= 0 && (w = distP->d[y+1][x-1]) > width)
                    width = w;
                if (x + 1 < distP->width && (w = distP->d[y+1][x+1]) > width)
                    width = w;
            }
            coordP->z = width * (fittingOptsP->width_weight_factor);
        }
    }
}



static void
filterCurves(curve_list_type     const curveList,
             fitting_opts_type * const fittingOptsP) {
             
    unsigned int curveSeq;

    LOG("\nFiltering curves:\n");

    for (curveSeq = 0; curveSeq < curveList.length; ++curveSeq) {
        LOG1("#%u: ", curveSeq);
        filter(CURVE_LIST_ELT(curveList, curveSeq), fittingOptsP);
    }
}



static void
logSplinesForCurve(unsigned int     const curveSeq,
                   spline_list_type const curveSplines) {

    unsigned int splineSeq;

    LOG1("Fitted splines for curve #%u:\n", curveSeq);
    for (splineSeq = 0;
         splineSeq < SPLINE_LIST_LENGTH(curveSplines);
         ++splineSeq) {
        LOG1("  %u: ", splineSeq);
        if (log_file)
            print_spline(log_file, SPLINE_LIST_ELT(curveSplines, splineSeq));
    }
}     
       


static void
change_bad_lines(spline_list_type *        const spline_list,
                 const fitting_opts_type * const fitting_opts) {

/* Unfortunately, we cannot tell in isolation whether a given spline
   should be changed to a line or not.  That can only be known after the
   entire curve has been fit to a list of splines.  (The curve is the
   pixel outline between two corners.)  After subdividing the curve, a
   line may very well fit a portion of the curve just as well as the
   spline---but unless a spline is truly close to being a line, it
   should not be combined with other lines.  */

  unsigned this_spline;
  bool found_cubic = false;
  unsigned length = SPLINE_LIST_LENGTH (*spline_list);

  LOG1 ("\nChecking for bad lines (length %u):\n", length);

  /* First see if there are any splines in the fitted shape.  */
  for (this_spline = 0; this_spline < length; this_spline++)
    {
      if (SPLINE_DEGREE (SPLINE_LIST_ELT (*spline_list, this_spline)) ==
       CUBICTYPE)
        {
          found_cubic = true;
          break;
        }
    }

  /* If so, change lines back to splines (we haven't done anything to
     their control points, so we only have to change the degree) unless
     the spline is close enough to being a line.  */
  if (found_cubic)
    for (this_spline = 0; this_spline < length; this_spline++)
      {
        spline_type s = SPLINE_LIST_ELT (*spline_list, this_spline);

        if (SPLINE_DEGREE (s) == LINEARTYPE)
          {
            LOG1 ("  #%u: ", this_spline);
            if (SPLINE_LINEARITY (s) > fitting_opts->line_reversion_threshold)
              {
                LOG ("reverted, ");
                SPLINE_DEGREE (SPLINE_LIST_ELT (*spline_list, this_spline))
                  = CUBICTYPE;
              }
            LOG1 ("linearity %.3f.\n", SPLINE_LINEARITY (s));
          }
      }
    else
      LOG ("  No lines.\n");
}



static bool
spline_linear_enough(spline_type *             const spline,
                     curve_type                const curve,
                     const fitting_opts_type * const fitting_opts) {

/* Supposing that we have accepted the error, another question arises:
   would we be better off just using a straight line?  */

  float A, B, C;
  unsigned this_point;
  float dist = 0.0, start_end_dist, threshold;

  LOG ("Checking linearity:\n");

  A = END_POINT(*spline).x - START_POINT(*spline).x;
  B = END_POINT(*spline).y - START_POINT(*spline).y;
  C = END_POINT(*spline).z - START_POINT(*spline).z;

  start_end_dist = (float) (SQR(A) + SQR(B) + SQR(C));
  LOG1 ("start_end_distance is %.3f.\n", sqrt(start_end_dist));

  LOG3 ("  Line endpoints are (%.3f, %.3f, %.3f) and ", START_POINT(*spline).x, START_POINT(*spline).y, START_POINT(*spline).z);
  LOG3 ("(%.3f, %.3f, %.3f)\n", END_POINT(*spline).x, END_POINT(*spline).y, END_POINT(*spline).z);

  /* LOG3 ("  Line is %.3fx + %.3fy + %.3f = 0.\n", A, B, C); */

  for (this_point = 0; this_point < CURVE_LENGTH (curve); this_point++)
    {
      float a, b, c, w;
      float t = CURVE_T (curve, this_point);
      float_coord spline_point = evaluate_spline (*spline, t);

      a = spline_point.x - START_POINT(*spline).x;
      b = spline_point.y - START_POINT(*spline).y;
      c = spline_point.z - START_POINT(*spline).z;
      w = (A*a + B*b + C*c) / start_end_dist;

      dist += (float)sqrt(SQR(a-A*w) + SQR(b-B*w) + SQR(c-C*w));
    }
  LOG1 ("  Total distance is %.3f, ", dist);

  dist /= (CURVE_LENGTH (curve) - 1);
  LOG1 ("which is %.3f normalized.\n", dist);

  /* We want reversion of short curves to splines to be more likely than
     reversion of long curves, hence the second division by the curve
     length, for use in `change_bad_lines'.  */
  SPLINE_LINEARITY (*spline) = dist;
  LOG1 ("  Final linearity: %.3f.\n", SPLINE_LINEARITY (*spline));
  if (start_end_dist * (float) 0.5 > fitting_opts->line_threshold)
    threshold = fitting_opts->line_threshold;
  else
    threshold = start_end_dist * (float) 0.5;
  LOG1 ("threshold is %.3f .\n", threshold);
  if (dist < threshold)
    return true;
  else
    return false;
}



/* Forward declaration for recursion */

static spline_list_type *
fitCurve(curve_type                const curve,
         const fitting_opts_type * const fitting_opts,
         at_exception_type *       const exception);



static spline_list_type *
fit_with_line(curve_type const curve) {
/*----------------------------------------------------------------------------
  This routine returns the curve fitted to a straight line in a very
  simple way: make the first and last points on the curve be the
  endpoints of the line.  This simplicity is justified because we are
  called only on very short curves.
-----------------------------------------------------------------------------*/
    spline_type line;

    LOG("Fitting with straight line:\n");

    SPLINE_DEGREE(line) = LINEARTYPE;
    START_POINT(line) = CONTROL1(line) = CURVE_POINT(curve, 0);
    END_POINT(line) = CONTROL2(line) = LAST_CURVE_POINT(curve);

    /* Make sure that this line is never changed to a cubic.  */
    SPLINE_LINEARITY(line) = 0;

    if (log_file) {
        LOG("  ");
        print_spline(log_file, line);
    }

    return new_spline_list_with_spline(line);
}



#define B0(t) CUBE ((float) 1.0 - (t))
#define B1(t) ((float) 3.0 * (t) * SQR ((float) 1.0 - (t)))
#define B2(t) ((float) 3.0 * SQR (t) * ((float) 1.0 - (t)))
#define B3(t) CUBE (t)

static spline_type
fit_one_spline(curve_type          const curve, 
               at_exception_type * const exception) {
/*----------------------------------------------------------------------------
   Our job here is to find alpha1 (and alpha2), where t1_hat (t2_hat) is
   the tangent to CURVE at the starting (ending) point, such that:

   control1 = alpha1 * t1_hat + starting point
   control2 = alpha2 * t2_hat + ending_point

   and the resulting spline (starting_point .. control1 and control2 ..
   ending_point) minimizes the least-square error from CURVE.

   See pp.57--59 of the Phoenix thesis.

   The B?(t) here corresponds to B_i^3(U_i) there.
   The Bernshte\u in polynomials of degree n are defined by
   B_i^n(t) = { n \choose i } t^i (1-t)^{n-i}, i = 0..n
-----------------------------------------------------------------------------*/
    /* Since our arrays are zero-based, the `C0' and `C1' here correspond
       to `C1' and `C2' in the paper. 
    */
    float X_C1_det, C0_X_det, C0_C1_det;
    float alpha1, alpha2;
    spline_type spline;
    vector_type start_vector, end_vector;
    unsigned i;
    vector_type * A;
    vector_type t1_hat;
    vector_type t2_hat;
    float C[2][2] = { { 0.0, 0.0 }, { 0.0, 0.0 } };
    float X[2] = { 0.0, 0.0 };

    t1_hat = *CURVE_START_TANGENT(curve);  /* initial value */
    t2_hat = *CURVE_END_TANGENT(curve);    /* initial value */

    MALLOCARRAY_NOFAIL(A, CURVE_LENGTH(curve) * 2);

    START_POINT(spline) = CURVE_POINT(curve, 0);
    END_POINT(spline)   = LAST_CURVE_POINT(curve);
    start_vector = make_vector(START_POINT(spline));
    end_vector   = make_vector(END_POINT(spline));

    for (i = 0; i < CURVE_LENGTH(curve); ++i) {
        A[(i << 1) + 0] = Vmult_scalar(t1_hat, B1(CURVE_T(curve, i)));
        A[(i << 1) + 1] = Vmult_scalar(t2_hat, B2(CURVE_T(curve, i)));
    }

    for (i = 0; i < CURVE_LENGTH(curve); ++i) {
        vector_type temp, temp0, temp1, temp2, temp3;
        vector_type * Ai = A + (i << 1);

        C[0][0] += Vdot(Ai[0], Ai[0]);
        C[0][1] += Vdot(Ai[0], Ai[1]);
        /* C[1][0] = C[0][1] (this is assigned outside the loop)  */
        C[1][1] += Vdot(Ai[1], Ai[1]);

        /* Now the right-hand side of the equation in the paper.  */
        temp0 = Vmult_scalar(start_vector, B0(CURVE_T(curve, i)));
        temp1 = Vmult_scalar(start_vector, B1(CURVE_T(curve, i)));
        temp2 = Vmult_scalar(end_vector, B2(CURVE_T(curve, i)));
        temp3 = Vmult_scalar(end_vector, B3(CURVE_T(curve, i)));

        temp = make_vector(
            Vsubtract_point(CURVE_POINT(curve, i),
                            Vadd(temp0, Vadd(temp1, Vadd(temp2, temp3)))));

        X[0] += Vdot(temp, Ai[0]);
        X[1] += Vdot(temp, Ai[1]);
    }
    free(A);

    C[1][0] = C[0][1];
    
    X_C1_det = X[0] * C[1][1] - X[1] * C[0][1];
    C0_X_det = C[0][0] * X[1] - C[0][1] * X[0];
    C0_C1_det = C[0][0] * C[1][1] - C[1][0] * C[0][1];
    if (C0_C1_det == 0.0) {
        LOG ("zero determinant of C0*C1");
        at_exception_fatal(exception, "zero determinant of C0*C1");
        goto cleanup;
    }

    alpha1 = X_C1_det / C0_C1_det;
    alpha2 = C0_X_det / C0_C1_det;

    CONTROL1(spline) = Vadd_point(START_POINT(spline),
                                  Vmult_scalar(t1_hat, alpha1));
    CONTROL2(spline) = Vadd_point(END_POINT(spline),
                                  Vmult_scalar(t2_hat, alpha2));
    SPLINE_DEGREE(spline) = CUBICTYPE;

cleanup:
    return spline;
}



static void
logSplineFit(spline_type const spline) {

    if (SPLINE_DEGREE(spline) == LINEARTYPE)
        LOG("  fitted to line:\n");
    else
        LOG("  fitted to spline:\n");
    
    if (log_file) {
        LOG ("    ");
        print_spline (log_file, spline);
    }
}



static spline_list_type *
fit_with_least_squares(curve_type                const curve,
                       const fitting_opts_type * const fitting_opts,
                       at_exception_type *       const exception) {
/*----------------------------------------------------------------------------
  The least squares method is well described in Schneider's thesis.
  Briefly, we try to fit the entire curve with one spline.  If that
  fails, we subdivide the curve. 
-----------------------------------------------------------------------------*/
    float error;
    float best_error;
    spline_type spline;
    spline_type best_spline;
    spline_list_type * spline_list;
    unsigned int worst_point;
    float previous_error;
    
    best_error = FLT_MAX;  /* initial value */
    previous_error = FLT_MAX;  /* initial value */
    spline_list = NULL;  /* initial value */
    worst_point = 0;  /* initial value */

    LOG ("\nFitting with least squares:\n");
    
    /* Phoenix reduces the number of points with a ``linear spline
       technique''.  But for fitting letterforms, that is
       inappropriate.  We want all the points we can get.
    */
    
    /* It makes no difference whether we first set the `t' values or
       find the tangents.  This order makes the documentation a little
       more coherent.
    */

    LOG("Finding tangents:\n");
    find_tangent(curve, /* to_start */ true,  /* cross_curve */ false,
                 fitting_opts->tangent_surround);
    find_tangent(curve, /* to_start */ false, /* cross_curve */ false,
                 fitting_opts->tangent_surround);

    set_initial_parameter_values(curve);

    /* Now we loop, subdividing, until CURVE has been fit.  */
    while (true) {
        float error;

        spline = fit_one_spline(curve, exception);
        best_spline = spline;
        if (at_exception_got_fatal(exception))
            goto cleanup;

        logSplineFit(spline);
        
        if (SPLINE_DEGREE(spline) == LINEARTYPE)
            break;

        error = find_error(curve, spline, &worst_point, exception);
        if (error <= previous_error) {
            best_error  = error;
            best_spline = spline;
        }
        break;
    }

    if (SPLINE_DEGREE(spline) == LINEARTYPE) {
        spline_list = new_spline_list_with_spline(spline);
        LOG1("Accepted error of %.3f.\n", error);
        return spline_list;
    }

    /* Go back to the best fit.  */
    spline = best_spline;
    error = best_error;

    if (error < fitting_opts->error_threshold && !CURVE_CYCLIC(curve)) {
        /* The points were fitted with a spline.  We end up here
           whenever a fit is accepted.  We have one more job: see if
           the ``curve'' that was fit should really be a straight
           line.
        */
        if (spline_linear_enough(&spline, curve, fitting_opts)) {
            SPLINE_DEGREE(spline) = LINEARTYPE;
            LOG("Changed to line.\n");
        }
        spline_list = new_spline_list_with_spline(spline);
        LOG1("Accepted error of %.3f.\n", error);
    } else {
        /* We couldn't fit the curve acceptably, so subdivide.  */
        unsigned subdivision_index;
        spline_list_type * left_spline_list;
        spline_list_type * right_spline_list;
        curve_type left_curve, right_curve;

        left_curve  = new_curve();
        right_curve = new_curve();

        /* Insert 'left_curve', then 'right_curve' after 'curve' in the list */
        NEXT_CURVE(right_curve) = NEXT_CURVE(curve);
        PREVIOUS_CURVE(right_curve) = left_curve;
        NEXT_CURVE(left_curve) = right_curve;
        PREVIOUS_CURVE(left_curve) = curve;
        NEXT_CURVE(curve) = left_curve;

        LOG1("\nSubdividing (error %.3f):\n", error);
        LOG3("  Original point: (%.3f,%.3f), #%u.\n",
             CURVE_POINT (curve, worst_point).x,
             CURVE_POINT (curve, worst_point).y, worst_point);
        subdivision_index = worst_point;
        LOG3 ("  Final point: (%.3f,%.3f), #%u.\n",
              CURVE_POINT (curve, subdivision_index).x,
              CURVE_POINT (curve, subdivision_index).y, subdivision_index);

        /* The last point of the left-hand curve will also be the first
           point of the right-hand curve.  */
        CURVE_LENGTH(left_curve)  = subdivision_index + 1;
        CURVE_LENGTH(right_curve) = CURVE_LENGTH(curve) - subdivision_index;
        left_curve->point_list = curve->point_list;
        right_curve->point_list = curve->point_list + subdivision_index;

        /* We want to use the tangents of the curve which we are
           subdividing for the start tangent for left_curve and the
           end tangent for right_curve.
        */
        CURVE_START_TANGENT(left_curve) = CURVE_START_TANGENT(curve);
        CURVE_END_TANGENT(right_curve)  = CURVE_END_TANGENT(curve);

        /* We have to set up the two curves before finding the tangent at
           the subdivision point.  The tangent at that point must be the
           same for both curves, or noticeable bumps will occur in the
           character.  But we want to use information on both sides of the
           point to compute the tangent, hence cross_curve = true.
        */
        find_tangent(left_curve, /* to_start_point: */ false,
                     /* cross_curve: */ true, fitting_opts->tangent_surround);
        CURVE_START_TANGENT(right_curve) = CURVE_END_TANGENT(left_curve);

        /* Now that we've set up the curves, we can fit them.  */
        left_spline_list = fitCurve(left_curve, fitting_opts, exception);
        if (at_exception_got_fatal(exception))
            /* TODO: Memory allocated for left_curve and right_curve
               will leak.*/
            goto cleanup;

        right_spline_list = fitCurve(right_curve, fitting_opts, exception);
        /* TODO: Memory allocated for left_curve and right_curve
           will leak.*/
        if (at_exception_got_fatal(exception))
            goto cleanup;
        
        /* Neither of the subdivided curves could be fit, so fail.  */
        if (left_spline_list == NULL && right_spline_list == NULL)
            return NULL;

        /* Put the two together (or whichever of them exist).  */
        spline_list = new_spline_list();

        if (left_spline_list == NULL) {
            LOG1("Could not fit spline to left curve (%lx).\n",
                 (unsigned long) left_curve);
            at_exception_warning(exception, "Could not fit left spline list");
        } else {
            concat_spline_lists(spline_list, *left_spline_list);
            free_spline_list(*left_spline_list);
            free(left_spline_list);
        }
        
        if (right_spline_list == NULL) {
            LOG1("Could not fit spline to right curve (%lx).\n",
                 (unsigned long) right_curve);
            at_exception_warning(exception, "Could not fit right spline list");
        } else {
            concat_spline_lists(spline_list, *right_spline_list);
            free_spline_list(*right_spline_list);
            free(right_spline_list);
        }
        if (CURVE_END_TANGENT(left_curve))
            free(CURVE_END_TANGENT(left_curve));
        free(left_curve);
        free(right_curve);
    }
cleanup:

    return spline_list;
}



static spline_list_type *
fitCurve(curve_type                const curve,
         const fitting_opts_type * const fittingOptsP,
         at_exception_type *       const exception) {
/*----------------------------------------------------------------------------
  Transform a set of locations to a list of splines (the fewer the
  better).  We are guaranteed that CURVE does not contain any corners.
  We return NULL if we cannot fit the points at all.
-----------------------------------------------------------------------------*/
    spline_list_type * fittedSplinesP;

    if (CURVE_LENGTH(curve) < 2) {
        LOG("Tried to fit curve with less than two points");
        at_exception_warning(exception, 
                             "Tried to fit curve with less than two points");
        fittedSplinesP = NULL;
    } else if (CURVE_LENGTH(curve) < 4)
        fittedSplinesP = fit_with_line(curve);
    else
        fittedSplinesP =
            fit_with_least_squares(curve, fittingOptsP, exception);

    return fittedSplinesP;
}



static void
fitCurves(curve_list_type           const curveList,
          pixel                     const color,
          const fitting_opts_type * const fittingOptsP,
          spline_list_type *        const splinesP,
          at_exception_type *       const exceptionP) {
          
    spline_list_type curveListSplines;
    unsigned int curveSeq;

    curveListSplines = empty_spline_list();
    
    curveListSplines.open      = curveList.open;
    curveListSplines.clockwise = curveList.clockwise;
    curveListSplines.color     = color;

    for (curveSeq = 0;
         curveSeq < curveList.length && !at_exception_got_fatal(exceptionP);
         ++curveSeq) {

        curve_type const currentCurve = CURVE_LIST_ELT(curveList, curveSeq);

        spline_list_type * curveSplinesP;

        LOG1("\nFitting curve #%u:\n", curveSeq);

        curveSplinesP = fitCurve(currentCurve, fittingOptsP, exceptionP);
        if (!at_exception_got_fatal(exceptionP)) {
            if (curveSplinesP == NULL) {
                LOG1("Could not fit curve #%u", curveSeq);
                at_exception_warning(exceptionP, "Could not fit curve");
            } else {
                logSplinesForCurve(curveSeq, *curveSplinesP);
                
                /* After fitting, we may need to change some would-be lines
                   back to curves, because they are in a list with other
                   curves.
                */
                change_bad_lines(curveSplinesP, fittingOptsP);
                
                concat_spline_lists(&curveListSplines, *curveSplinesP);
                free_spline_list(*curveSplinesP);
                free(curveSplinesP);
            }
        }
    }
    if (at_exception_got_fatal(exceptionP))
        free_spline_list(curveListSplines);
    else
        *splinesP = curveListSplines;
}
    


static void
logFittedSplines(spline_list_type const curve_list_splines) {

    unsigned int splineSeq;

    LOG("\nFitted splines are:\n");
    for (splineSeq = 0;
         splineSeq < SPLINE_LIST_LENGTH(curve_list_splines);
         ++splineSeq) {
        LOG1("  %u: ", splineSeq);
        print_spline(log_file,
                     SPLINE_LIST_ELT(curve_list_splines, splineSeq));
    }
}



static void
fitCurveList(curve_list_type     const curveList,
             fitting_opts_type * const fittingOptsP,
             distance_map_type * const dist,
             pixel               const color,
             spline_list_type *  const splineListP,
             at_exception_type * const exception) {
/*----------------------------------------------------------------------------
  Fit the list of curves CURVE_LIST to a list of splines, and return
  it.  CURVE_LIST represents a single closed paths, e.g., either the
  inside or outside outline of an `o'.
-----------------------------------------------------------------------------*/
    curve_type curve;
    spline_list_type curveListSplines;

    removeKnees(curveList);

    if (dist != NULL)
        computePointWeights(curveList, fittingOptsP, dist);
    
    /* We filter all the curves in 'curveList' at once; otherwise, we
       would look at an unfiltered curve when computing tangents.
    */
    filterCurves(curveList, fittingOptsP);

    /* Make the first point in the first curve also be the last point in
       the last curve, so the fit to the whole curve list will begin and
       end at the same point.  This may cause slight errors in computing
       the tangents and t values, but it's worth it for the continuity.
       Of course we don't want to do this if the two points are already
       the same, as they are if the curve is cyclic.  (We don't append it
       earlier, in `split_at_corners', because that confuses the
       filtering.)  Finally, we can't append the point if the curve is
       exactly three points long, because we aren't adding any more data,
       and three points isn't enough to determine a spline.  Therefore,
       the fitting will fail.
    */
    curve = CURVE_LIST_ELT(curveList, 0);
    if (CURVE_CYCLIC(curve))
        append_point(curve, CURVE_POINT(curve, 0));

    /* Finally, fit each curve in the list to a list of splines.  */

    fitCurves(curveList, color, fittingOptsP, &curveListSplines, exception);
    if (!at_exception_got_fatal(exception)) {
        if (log_file)
            logFittedSplines(curveListSplines);
        *splineListP = curveListSplines;
    }
}



static void
fitCurvesToSplines(curve_list_array_type    const curveArray,
                   fitting_opts_type *      const fittingOptsP,
                   distance_map_type *      const dist,
                   unsigned short           const width,
                   unsigned short           const height,
                   at_exception_type *      const exception,
                   at_progress_func               notifyProgress, 
                   void *                   const progressData,
                   at_testcancel_func             testCancel,
                   void *                   const testcancelData,
                   spline_list_array_type * const splineListArrayP) {
    
    unsigned splineListSeq;
    bool cancelled;
    spline_list_array_type splineListArray;

    splineListArray = new_spline_list_array();
    splineListArray.centerline          = fittingOptsP->centerline;
    splineListArray.preserve_width      = fittingOptsP->preserve_width;
    splineListArray.width_weight_factor = fittingOptsP->width_weight_factor;
    splineListArray.backgroundSpec      = fittingOptsP->backgroundSpec;
    splineListArray.background_color    = fittingOptsP->background_color;
    /* Set dummy values. Real value is set in upper context. */
    splineListArray.width  = width;
    splineListArray.height = height;
    
    for (splineListSeq = 0, cancelled = false;
         splineListSeq < CURVE_LIST_ARRAY_LENGTH(curveArray) &&
             !at_exception_got_fatal(exception) && !cancelled;
         ++splineListSeq) {

        curve_list_type const curveList =
            CURVE_LIST_ARRAY_ELT(curveArray, splineListSeq);
      
        spline_list_type curveSplineList;

        if (notifyProgress)
            notifyProgress((((float)splineListSeq)/
                            ((float)CURVE_LIST_ARRAY_LENGTH(curveArray) *
                             (float)3.0) + (float)0.333),
                           progressData);
        if (testCancel && testCancel(testcancelData))
            cancelled = true;

        LOG1("\nFitting curve list #%u:\n", splineListSeq);

        fitCurveList(curveList, fittingOptsP, dist, curveList.color,
                     &curveSplineList, exception);
        if (!at_exception_got_fatal(exception))
            append_spline_list(&splineListArray, curveSplineList);
    }
    *splineListArrayP = splineListArray;
}



void
fit_outlines_to_splines(pixel_outline_list_type  const pixelOutlineList,
                        fitting_opts_type *      const fittingOptsP,
                        distance_map_type *      const dist,
                        unsigned short           const width,
                        unsigned short           const height,
                        at_exception_type *      const exception,
                        at_progress_func               notifyProgress, 
                        void *                   const progressData,
                        at_testcancel_func             testCancel,
                        void *                   const testcancelData,
                        spline_list_array_type * const splineListArrayP) {
/*----------------------------------------------------------------------------
   Transform a list of pixels in the outlines of the original character to
   a list of spline lists fitted to those pixels.
-----------------------------------------------------------------------------*/
    curve_list_array_type const curveListArray =
        split_at_corners(pixelOutlineList, fittingOptsP, exception);

    fitCurvesToSplines(curveListArray, fittingOptsP, dist, width, height,
                       exception, notifyProgress, progressData,
                       testCancel, testcancelData, splineListArrayP);


    free_curve_list_array(&curveListArray, notifyProgress, progressData);
    
    flush_log_output();
}




static void
find_vectors(unsigned int       const test_index,
             pixel_outline_type const outline,
             vector_type *      const in,
             vector_type *      const out,
             unsigned int       const corner_surround) {
/*----------------------------------------------------------------------------
  Return the difference vectors coming in and going out of the outline
  OUTLINE at the point whose index is TEST_INDEX.  In Phoenix,
  Schneider looks at a single point on either side of the point we're
  considering.  That works for him because his points are not touching.
  But our points *are* touching, and so we have to look at
  `corner_surround' points on either side, to get a better picture of
  the outline's shape.
-----------------------------------------------------------------------------*/
    int i;
    unsigned n_done;
    pm_pixelcoord const candidate = O_COORDINATE(outline, test_index);

    in->dx  = in->dy  = in->dz  = 0.0;
    out->dx = out->dy = out->dz = 0.0;
    
    /* Add up the differences from p of the `corner_surround' points
       before p.
    */
    for (i = O_PREV(outline, test_index), n_done = 0;
         n_done < corner_surround;
         i = O_PREV(outline, i), ++n_done)
        *in = Vadd(*in, IPsubtract(O_COORDINATE(outline, i), candidate));
    
    /* And the points after p. */
    for (i = O_NEXT (outline, test_index), n_done = 0;
         n_done < corner_surround;
         i = O_NEXT(outline, i), ++n_done)
        *out = Vadd(*out, IPsubtract(O_COORDINATE(outline, i), candidate));
}



/* Remove adjacent points from the index list LIST.  We do this by first
   sorting the list and then running through it.  Since these lists are
   quite short, a straight selection sort (e.g., p.139 of the Art of
   Computer Programming, vol.3) is good enough.  LAST_INDEX is the index
   of the last pixel on the outline, i.e., the next one is the first
   pixel. We need this for checking the adjacency of the last corner.

   We need to do this because the adjacent corners turn into
   two-pixel-long curves, which can only be fit by straight lines.  */

static void
remove_adjacent_corners (index_list_type *list, unsigned last_index,
             bool remove_adj_corners,
             at_exception_type * exception)
             
{
  unsigned j;
  unsigned last;
  index_list_type new_list = new_index_list ();

  for (j = INDEX_LIST_LENGTH (*list) - 1; j > 0; j--)
    {
      unsigned search;
      unsigned temp;
      /* Find maximal element below `j'.  */
      unsigned max_index = j;

      for (search = 0; search < j; search++)
        if (GET_INDEX (*list, search) > GET_INDEX (*list, max_index))
          max_index = search;

      if (max_index != j)
        {
          temp = GET_INDEX (*list, j);
          GET_INDEX (*list, j) = GET_INDEX (*list, max_index);
          GET_INDEX (*list, max_index) = temp;
      
      /* xx -- really have to sort?  */
      LOG ("needed exchange");
      at_exception_warning(exception, "needed exchange");
        }
    }

  /* The list is sorted.  Now look for adjacent entries.  Each time
     through the loop we insert the current entry and, if appropriate,
     the next entry.  */
  for (j = 0; j < INDEX_LIST_LENGTH (*list) - 1; j++)
    {
      unsigned current = GET_INDEX (*list, j);
      unsigned next = GET_INDEX (*list, j + 1);

      /* We should never have inserted the same element twice.  */
      /* assert (current != next); */

      if ((remove_adj_corners) && ((next == current + 1) || (next == current)))
        j++;

      append_index (&new_list, current);
    }

  /* Don't append the last element if it is 1) adjacent to the previous
     one; or 2) adjacent to the very first one.  */
  last = GET_LAST_INDEX (*list);
  if (INDEX_LIST_LENGTH (new_list) == 0
      || !(last == GET_LAST_INDEX (new_list) + 1
           || (last == last_index && GET_INDEX (*list, 0) == 0)))
    append_index (&new_list, last);

  free_index_list (list);
  *list = new_list;
}

/* A ``knee'' is a point which forms a ``right angle'' with its
   predecessor and successor.  See the documentation (the `Removing
   knees' section) for an example and more details.

   The argument CLOCKWISE tells us which direction we're moving.  (We
   can't figure that information out from just the single segment with
   which we are given to work.)

   We should never find two consecutive knees.

   Since the first and last points are corners (unless the curve is
   cyclic), it doesn't make sense to remove those.  */

/* This evaluates to true if the vector V is zero in one direction and
   nonzero in the other.  */
#define ONLY_ONE_ZERO(v)                                                \
  (((v).dx == 0.0 && (v).dy != 0.0) || ((v).dy == 0.0 && (v).dx != 0.0))

/* There are four possible cases for knees, one for each of the four
   corners of a rectangle; and then the cases differ depending on which
   direction we are going around the curve.  The tests are listed here
   in the order of upper left, upper right, lower right, lower left.
   Perhaps there is some simple pattern to the
   clockwise/counterclockwise differences, but I don't see one.  */
#define CLOCKWISE_KNEE(prev_delta, next_delta)                                                  \
  ((prev_delta.dx == -1.0 && next_delta.dy == 1.0)                                              \
   || (prev_delta.dy == 1.0 && next_delta.dx == 1.0)                                    \
   || (prev_delta.dx == 1.0 && next_delta.dy == -1.0)                                   \
   || (prev_delta.dy == -1.0 && next_delta.dx == -1.0))

#define COUNTERCLOCKWISE_KNEE(prev_delta, next_delta)                                   \
  ((prev_delta.dy == 1.0 && next_delta.dx == -1.0)                                              \
   || (prev_delta.dx == 1.0 && next_delta.dy == 1.0)                                    \
   || (prev_delta.dy == -1.0 && next_delta.dx == 1.0)                                   \
   || (prev_delta.dx == -1.0 && next_delta.dy == -1.0))



static void
remove_knee_points(curve_type const curve,
                   bool       const clockwise) {

      unsigned const offset = (CURVE_CYCLIC(curve) == true) ? 0 : 1;
      curve_type const trimmed_curve = copy_most_of_curve(curve);

      pm_pixelcoord previous;
      unsigned i;

      if (!CURVE_CYCLIC(curve))
          append_pixel(trimmed_curve,
                       real_to_int_coord(CURVE_POINT(curve, 0)));

      previous = real_to_int_coord(CURVE_POINT(curve,
                                               CURVE_PREV(curve, offset)));

      for (i = offset; i < CURVE_LENGTH (curve) - offset; ++i) {
          pm_pixelcoord const current =
              real_to_int_coord(CURVE_POINT(curve, i));
          pm_pixelcoord const next =
              real_to_int_coord(CURVE_POINT(curve, CURVE_NEXT(curve, i)));
          vector_type const prev_delta = IPsubtract(previous, current);
          vector_type const next_delta = IPsubtract(next, current);

          if (ONLY_ONE_ZERO(prev_delta) && ONLY_ONE_ZERO(next_delta)
              && ((clockwise && CLOCKWISE_KNEE(prev_delta, next_delta))
                  || (!clockwise
                      && COUNTERCLOCKWISE_KNEE(prev_delta, next_delta))))
              LOG2(" (%d,%d)", current.col, current.row);
          else {
              previous = current;
              append_pixel(trimmed_curve, current);
          }
      }

      if (!CURVE_CYCLIC(curve))
          append_pixel(trimmed_curve,
                       real_to_int_coord(LAST_CURVE_POINT(curve)));

      if (CURVE_LENGTH(trimmed_curve) == CURVE_LENGTH(curve))
          LOG(" (none)");

      LOG(".\n");

      free_curve(curve);
      *curve = *trimmed_curve;
      free(trimmed_curve);      /* free_curve? --- Masatake */
}



/* Smooth the curve by adding in neighboring points.  Do this
   `filter_iterations' times.  But don't change the corners.  */

static void
filter (curve_type curve, fitting_opts_type *fitting_opts)
{
  unsigned iteration, this_point;
  unsigned offset = (CURVE_CYCLIC (curve) == true) ? 0 : 1;
  float_coord prev_new_point;

  /* We must have at least three points---the previous one, the current
     one, and the next one.  But if we don't have at least five, we will
     probably collapse the curve down onto a single point, which means
     we won't be able to fit it with a spline.  */
  if (CURVE_LENGTH (curve) < 5)
    {
      LOG1 ("Length is %u, not enough to filter.\n", CURVE_LENGTH (curve));
      return;
    }

  prev_new_point.x = FLT_MAX;
  prev_new_point.y = FLT_MAX;
  prev_new_point.z = FLT_MAX;

  for (iteration = 0; iteration < fitting_opts->filter_iterations;
   iteration++)
    {
      curve_type newcurve = copy_most_of_curve (curve);
      bool collapsed = false;

      /* Keep the first point on the curve.  */
      if (offset)
        append_point (newcurve, CURVE_POINT (curve, 0));

      for (this_point = offset; this_point < CURVE_LENGTH (curve) - offset;
           this_point++)
        {
          vector_type in, out, sum;
          float_coord new_point;

          /* Calculate the vectors in and out, computed by looking at n points
             on either side of this_point. Experimental it was found that 2 is
             optimal. */

          signed int prev, prevprev; /* have to be signed */
          unsigned int next, nextnext;
          float_coord candidate = CURVE_POINT (curve, this_point);

          prev = CURVE_PREV (curve, this_point);
          prevprev = CURVE_PREV (curve, prev);
          next = CURVE_NEXT (curve, this_point);
          nextnext = CURVE_NEXT (curve, next);

          /* Add up the differences from p of the `surround' points
             before p.  */
          in.dx = in.dy = in.dz = 0.0;

          in = Vadd (in, Psubtract (CURVE_POINT (curve, prev), candidate));
          if (prevprev >= 0)
              in = Vadd (in, Psubtract (CURVE_POINT (curve, prevprev), candidate));

          /* And the points after p.  Don't use more points after p than we
             ended up with before it.  */
          out.dx = out.dy = out.dz = 0.0;

          out = Vadd (out, Psubtract (CURVE_POINT (curve, next), candidate));
          if (nextnext < CURVE_LENGTH (curve))
              out = Vadd (out, Psubtract (CURVE_POINT (curve, nextnext), candidate));

          /* Start with the old point.  */
          new_point = candidate;
          sum = Vadd (in, out);
          /* We added 2*n+2 points, so we have to divide the sum by 2*n+2 */
          new_point.x += sum.dx / 6;
          new_point.y += sum.dy / 6;
          new_point.z += sum.dz / 6;
          if (fabs (prev_new_point.x - new_point.x) < 0.3
              && fabs (prev_new_point.y - new_point.y) < 0.3
              && fabs (prev_new_point.z - new_point.z) < 0.3)
            {
              collapsed = true;
              break;
            }


          /* Put the newly computed point into a separate curve, so it
             doesn't affect future computation (on this iteration).  */
          append_point (newcurve, prev_new_point = new_point);
        }

      if (collapsed)
    free_curve (newcurve);
      else
    {
          /* Just as with the first point, we have to keep the last point.  */
          if (offset)
        append_point (newcurve, LAST_CURVE_POINT (curve));
      
          /* Set the original curve to the newly filtered one, and go again.  */
          free_curve (curve);
          *curve = *newcurve;
    }
      free (newcurve);
    }

  log_curve (curve, false);
}



/* Find reasonable values for t for each point on CURVE.  The method is
   called chord-length parameterization, which is described in Plass &
   Stone.  The basic idea is just to use the distance from one point to
   the next as the t value, normalized to produce values that increase
   from zero for the first point to one for the last point.  */

static void
set_initial_parameter_values (curve_type curve)
{
  unsigned p;

  LOG ("\nAssigning initial t values:\n  ");

  CURVE_T (curve, 0) = 0.0;

  for (p = 1; p < CURVE_LENGTH (curve); p++)
    {
      float_coord point = CURVE_POINT (curve, p),
                           previous_p = CURVE_POINT (curve, p - 1);
      float d = distance (point, previous_p);
      CURVE_T (curve, p) = CURVE_T (curve, p - 1) + d;
    }

  assert (LAST_CURVE_T (curve) != 0.0);

  for (p = 1; p < CURVE_LENGTH (curve); p++)
    CURVE_T (curve, p) = CURVE_T (curve, p) / LAST_CURVE_T (curve);

  log_entire_curve (curve);
}

/* Find an approximation to the tangent to an endpoint of CURVE (to the
   first point if TO_START_POINT is true, else the last).  If
   CROSS_CURVE is true, consider points on the adjacent curve to CURVE.

   It is important to compute an accurate approximation, because the
   control points that we eventually decide upon to fit the curve will
   be placed on the half-lines defined by the tangents and
   endpoints...and we never recompute the tangent after this.  */

static void
find_tangent (curve_type curve, bool to_start_point, bool cross_curve,
  unsigned tangent_surround)
{
  vector_type tangent;
  vector_type **curve_tangent = (to_start_point == true) ? &(CURVE_START_TANGENT (curve))
                                               : &(CURVE_END_TANGENT (curve));
  unsigned n_points = 0;

  LOG1 ("  tangent to %s: ", (to_start_point == true) ? "start" : "end");

  if (*curve_tangent == NULL)
    {
        MALLOCVAR_NOFAIL(*curve_tangent);
      do
        {
          tangent = find_half_tangent (curve, to_start_point, &n_points,
            tangent_surround);

          if ((cross_curve == true) || (CURVE_CYCLIC (curve) == true))
            {
              curve_type adjacent_curve
                = (to_start_point == true) ? PREVIOUS_CURVE (curve) : NEXT_CURVE (curve);
              vector_type tangent2
                = (to_start_point == false) ? find_half_tangent (adjacent_curve, true, &n_points,
                tangent_surround) : find_half_tangent (adjacent_curve, true, &n_points,
                tangent_surround);

              LOG3 ("(adjacent curve half tangent (%.3f,%.3f,%.3f)) ",
                tangent2.dx, tangent2.dy, tangent2.dz);
              tangent = Vadd (tangent, tangent2);
            }
          tangent_surround--;

        }
      while (tangent.dx == 0.0 && tangent.dy == 0.0);

      assert (n_points > 0);
      **curve_tangent = Vmult_scalar (tangent, (float)(1.0 / n_points));
      if ((CURVE_CYCLIC (curve) == true) && CURVE_START_TANGENT (curve))
          *CURVE_START_TANGENT (curve) = **curve_tangent;
      if  ((CURVE_CYCLIC (curve) == true) && CURVE_END_TANGENT (curve))
          *CURVE_END_TANGENT (curve) = **curve_tangent;
    }
  else
    LOG ("(already computed) ");

  LOG3 ("(%.3f,%.3f,%.3f).\n", (*curve_tangent)->dx, (*curve_tangent)->dy, (*curve_tangent)->dz);
}

/* Find the change in y and change in x for `tangent_surround' (a global)
   points along CURVE.  Increment N_POINTS by the number of points we
   actually look at.  */

static vector_type
find_half_tangent (curve_type c, bool to_start_point, unsigned *n_points,
  unsigned tangent_surround)
{
  unsigned p;
  int factor = to_start_point ? 1 : -1;
  unsigned tangent_index = to_start_point ? 0 : c->length - 1;
  float_coord tangent_point = CURVE_POINT (c, tangent_index);
  vector_type tangent = { 0.0, 0.0 };
  unsigned int surround;

  if ((surround = CURVE_LENGTH (c) / 2) > tangent_surround)
    surround = tangent_surround;

  for (p = 1; p <= surround; p++)
    {
      int this_index = p * factor + tangent_index;
      float_coord this_point;

      if (this_index < 0 || this_index >= (int) c->length)
        break;

      this_point = CURVE_POINT (c, p * factor + tangent_index);

      /* Perhaps we should weight the tangent from `this_point' by some
         factor dependent on the distance from the tangent point.  */
      tangent = Vadd (tangent,
                      Vmult_scalar (Psubtract (this_point, tangent_point),
                                    (float) factor));
      (*n_points)++;
    }

  return tangent;
}

/* When this routine is called, we have computed a spline representation
   for the digitized curve.  The question is, how good is it?  If the
   fit is very good indeed, we might have an error of zero on each
   point, and then WORST_POINT becomes irrelevant.  But normally, we
   return the error at the worst point, and the index of that point in
   WORST_POINT.  The error computation itself is the Euclidean distance
   from the original curve CURVE to the fitted spline SPLINE.  */

static float
find_error (curve_type curve, spline_type spline, unsigned *worst_point,
        at_exception_type * exception)
{
  unsigned this_point;
  float total_error = 0.0;
  float worst_error = FLT_MIN;

  *worst_point = CURVE_LENGTH (curve) + 1;   /* A sentinel value.  */

  for (this_point = 0; this_point < CURVE_LENGTH (curve); this_point++)
    {
      float_coord curve_point = CURVE_POINT (curve, this_point);
      float t = CURVE_T (curve, this_point);
      float_coord spline_point = evaluate_spline (spline, t);
      float this_error = distance (curve_point, spline_point);
      if (this_error >= worst_error)
        {
         *worst_point = this_point;
          worst_error = this_error;
        }
      total_error += this_error;
    }

  if (*worst_point == CURVE_LENGTH (curve) + 1)
    { /* Didn't have any ``worst point''; the error should be zero.  */
      *worst_point = 0;
      if (epsilon_equal (total_error, 0.0))
        LOG ("  Every point fit perfectly.\n");
      else
    {
      LOG("No worst point found; something is wrong");
      at_exception_warning(exception, "No worst point found; something is wrong");
    }
    }
  else
    {
      if (epsilon_equal (total_error, 0.0))
        LOG ("  Every point fit perfectly.\n");
      else
        {
          LOG5 ("  Worst error (at (%.3f,%.3f,%.3f), point #%u) was %.3f.\n",
              CURVE_POINT (curve, *worst_point).x,
              CURVE_POINT (curve, *worst_point).y,
              CURVE_POINT (curve, *worst_point).z, *worst_point, worst_error);
          LOG1 ("  Total error was %.3f.\n", total_error);
          LOG2 ("  Average error (over %u points) was %.3f.\n",
              CURVE_LENGTH (curve), total_error / CURVE_LENGTH (curve));
        }
    }

  return worst_error;
}


/* Lists of array indices (well, that is what we use it for).  */

static index_list_type
new_index_list (void)
{
  index_list_type index_list;

  index_list.data = NULL;
  INDEX_LIST_LENGTH (index_list) = 0;

  return index_list;
}

static void
free_index_list (index_list_type *index_list)
{
  if (INDEX_LIST_LENGTH (*index_list) > 0)
    {
      free (index_list->data);
      index_list->data = NULL;
      INDEX_LIST_LENGTH (*index_list) = 0;
    }
}

static void
append_index (index_list_type *list, unsigned new_index)
{
  INDEX_LIST_LENGTH (*list)++;
  REALLOCARRAY_NOFAIL(list->data, INDEX_LIST_LENGTH(*list));
  list->data[INDEX_LIST_LENGTH (*list) - 1] = new_index;
}


/* Return the Euclidean distance between P1 and P2.  */

static float
distance (float_coord p1, float_coord p2)
{
  float x = p1.x - p2.x, y = p1.y - p2.y, z = p1.z - p2.z;
  return (float) sqrt (SQR(x) + SQR(y) + SQR(z));
}
