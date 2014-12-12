/* autotrace.h --- Autotrace API

  Copyright (C) 2000, 2001, 2002 Martin Weber

  The author can be contacted at <martweb@gmx.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifndef AUTOTRACE_H
#define AUTOTRACE_H

#include <stdio.h>

#include "point.h"
#include "pm_c_util.h"
#include "ppm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ===================================================================== *
 * Typedefs
 * ===================================================================== */

typedef struct _at_fitting_opts_type at_fitting_opts_type;
typedef struct _at_input_opts_type   at_input_opts_type;
typedef struct _at_output_opts_type  at_output_opts_type;
typedef struct _at_bitmap_type at_bitmap_type;
typedef struct _at_spline_type at_spline_type;
typedef struct _at_spline_list_type at_spline_list_type;
typedef struct _at_spline_list_array_type at_spline_list_array_type;

/* Third degree is the highest we deal with.  */
typedef enum _at_polynomial_degree
{
  AT_LINEARTYPE = 1, 
  AT_QUADRATICTYPE = 2, 
  AT_CUBICTYPE = 3, 
  AT_PARALLELELLIPSETYPE = 4,
  AT_ELLIPSETYPE = 5, 
  AT_CIRCLETYPE = 6 
  /* not the real number of points to define a
     circle but to distinguish between a cubic spline */
} at_polynomial_degree;

/* A Bezier spline can be represented as four points in the real plane:
   a starting point, ending point, and two control points.  The
   curve always lies in the convex hull defined by the four points.  It
   is also convenient to save the divergence of the spline from the
   straight line defined by the endpoints.  */
struct _at_spline_type
{
  float_coord v[4];	/* The control points.  */
  at_polynomial_degree degree;
  float linearity;
};

/* Each outline in a character is typically represented by many
   splines.  So, here is a list structure for that:  */
struct _at_spline_list_type
{
  at_spline_type *data;
  unsigned length;
  bool clockwise;
  pixel color;
  bool open;
};

/* Each character is in general made up of many outlines. So here is one
   more list structure.  */
struct _at_spline_list_array_type
{
  at_spline_list_type *data;
  unsigned length;

  /* splines bbox */
  unsigned short height, width;
  
  /* the values for following members are inherited from 
     at_fitting_opts_type */
  bool backgroundSpec;
  pixel background_color;
  bool centerline;
  bool preserve_width;
  float width_weight_factor;

};


/* Fitting option.
   With using at_fitting_opts_doc macro, the description of 
   each option could be get. e.g. at_fitting_opts_doc(background_color) */
struct _at_fitting_opts_type {
    bool backgroundSpec;
    pixel background_color;
    float corner_always_threshold;
    unsigned corner_surround;
    float corner_threshold;
    float error_threshold;
    unsigned filter_iterations;
    float line_reversion_threshold;
    float line_threshold;
    bool remove_adjacent_corners;
    unsigned tangent_surround;
    bool centerline;
    bool preserve_width;
    float width_weight_factor;
};

struct _at_output_opts_type
{
  int dpi;			/* DPI is used only in MIF output.*/
};

struct _at_bitmap_type
{
  unsigned short height;
  unsigned short width;
  unsigned char *bitmap;
  unsigned int np;
};

typedef enum _at_msg_type
{
  AT_MSG_FATAL = 1,
  AT_MSG_WARNING
} at_msg_type;

typedef
void (* at_msg_func) (const char * const msg,
                      at_msg_type  const msg_type,
                      void *       const client_data);

typedef 
int (*at_output_write_func) (FILE *                          const file,
                             const char *                    const name,
                             int                             const llx,
                             int                             const lly, 
                             int                             const urx,
                             int                             const ury,
                             at_output_opts_type *           const opts,
                             at_spline_list_array_type       const shape,
                             at_msg_func                           msg_func, 
                             void *                          const msg_data);

/*
 * Progress handler typedefs
 * 0.0 <= percentage <= 1.0
 */
typedef
void (*at_progress_func) (float const percentage,
                           void *  const client_data);

/*
 * Test cancel
 * Return true if auto-tracing should be stopped.
 */
typedef
bool (*at_testcancel_func) (void * const client_data);

/* ===================================================================== *
 * Functions
 * ===================================================================== */

/* --------------------------------------------------------------------- *
 * Fitting option related
 *
 * TODO: internal data access, copy
 * --------------------------------------------------------------------- */
at_fitting_opts_type * at_fitting_opts_new(void);
at_fitting_opts_type * at_fitting_opts_copy (at_fitting_opts_type * original); 
void at_fitting_opts_free(at_fitting_opts_type * opts);

/* TODO: Gettextize */
#define at_fitting_opts_doc(opt) _(at_doc__##opt)

/* --------------------------------------------------------------------- *
 * Output option related
 *
 * TODO: internal data access
 * --------------------------------------------------------------------- */
at_output_opts_type * at_output_opts_new(void);
at_output_opts_type * at_output_opts_copy(at_output_opts_type * original);
void at_output_opts_free(at_output_opts_type * opts);

/* --------------------------------------------------------------------- *
 * Spline related
 *
 * TODO: internal data access
 * --------------------------------------------------------------------- */
/* at_splines_new_full

   args:

   NOTIFY_PROGRESS is called repeatedly inside at_splines_new_full
   to notify the progress of the execution. This might be useful for 
   interactive applications. NOTIFY_PROGRESS is called following 
   format:

   NOTIFY_PROGRESS (percentage, progress_data);

   test_cancel is called repeatedly inside at_splines_new_full
   to test whether the execution is canceled or not.
   If test_cancel returns TRUE, execution of at_splines_new_full
   is stopped as soon as possible and returns NULL. If test_cancel 
   returns FALSE, nothing happens. test_cancel  is called following
   format:

   TEST_CANCEL (testcancel_data);
   
   NULL is valid value for notify_progress and/or test_cancel if 
   you don't need to know the progress of the execution and/or 
   cancel the execution */ 

at_spline_list_array_type * 
at_splines_new_full(at_bitmap_type *       const bitmap,
                    at_fitting_opts_type * const opts,
                    at_msg_func                  msg_func, 
                    void *                 const msg_data,
                    at_progress_func             notify_progress,
                    void *                 const progress_data,
                    at_testcancel_func           test_cancel,
                    void *                 const testcancel_data);

void 
at_splines_write(at_output_write_func                  output_writer,
                 FILE *                          const writeto,
                 at_output_opts_type *           const opts,
                 at_spline_list_array_type *     const splines,
                 at_msg_func                           msg_func,
                 void *                          const msg_data);

void
at_splines_free(at_spline_list_array_type * const splines);


/* --------------------------------------------------------------------- *
 * Misc
 * --------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
