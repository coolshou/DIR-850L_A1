/* fit.h: convert the pixel representation to splines. */

#ifndef FIT_H_INCLUDED
#define FIT_H_INCLUDED

#include "autotrace.h"
#include "image-proc.h"
#include "pxl-outline.h"
#include "spline.h"
#include "exception.h"

/* See fit.c for descriptions of these variables, all of which can be
   set using options. 
*/
typedef at_fitting_opts_type fitting_opts_type;

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
                        spline_list_array_type * const splineListArrayP);

/* Get a new set of fitting options */
fitting_opts_type
new_fitting_opts(void);

#endif
