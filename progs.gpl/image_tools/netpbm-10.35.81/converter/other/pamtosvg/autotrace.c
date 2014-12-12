/* autotrace.c --- Autotrace API

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

#include "mallocvar.h"

#include "autotrace.h"
#include "exception.h"

#include "fit.h"
#include "bitmap.h"
#include "spline.h"

#include "image-header.h"
#include "image-proc.h"
#include "thin-image.h"


#define AT_DEFAULT_DPI 72

at_fitting_opts_type *
at_fitting_opts_new(void)
{
  at_fitting_opts_type * opts;
  MALLOCVAR_NOFAIL(opts);
  return opts;
}

at_fitting_opts_type *
at_fitting_opts_copy (at_fitting_opts_type * original)
{
  at_fitting_opts_type * new_opts;
  if (original == NULL)
    return NULL;

  new_opts = at_fitting_opts_new ();
  *new_opts = *original;
  new_opts->backgroundSpec = original->backgroundSpec;
  new_opts->background_color = original->background_color;
  return new_opts;
}

void 
at_fitting_opts_free(at_fitting_opts_type * opts)
{
  free(opts);
}

at_output_opts_type *
at_output_opts_new(void)
{
  at_output_opts_type * opts;
  MALLOCVAR_NOFAIL(opts);
  opts->dpi          = AT_DEFAULT_DPI;
  return opts;
}

at_output_opts_type *
at_output_opts_copy(at_output_opts_type * original)
{
  at_output_opts_type * opts =  at_output_opts_new();
  *opts = *original;
  return opts;
}

void
at_output_opts_free(at_output_opts_type * opts)
{
  free(opts);
}

/* at_splines_new_full modifies its 'bitmap' argument
   when it does the thin_image thing.
*/
at_spline_list_array_type * 
at_splines_new_full(at_bitmap_type *       const bitmap,
                    at_fitting_opts_type * const opts,
                    at_msg_func                  msg_func, 
                    void *                 const msg_data,
                    at_progress_func             notify_progress,
                    void *                 const progress_data,
                    at_testcancel_func           test_cancel,
                    void *                 const testcancel_data) {

    at_spline_list_array_type * retval;
    image_header_type image_header;
    pixel_outline_list_type pixelOutlineList;
    at_exception_type exp;
    distance_map_type distanceMap;
    bool haveDistMap;

    exp = at_exception_new(msg_func, msg_data);

    image_header.width  = at_bitmap_get_width(bitmap);
    image_header.height = at_bitmap_get_height(bitmap);

    if (opts->centerline) {
        if (opts->preserve_width) {
            /* Preserve line width prior to thinning. */
            bool const paddedTrue = true;
            distanceMap = new_distance_map(*bitmap, 255, paddedTrue, &exp);
            haveDistMap = true;
        } else
            haveDistMap = false;
        thin_image(bitmap, opts->backgroundSpec, opts->background_color, &exp);
    } else
        haveDistMap = false;

    if (at_exception_got_fatal(&exp))
        retval = NULL;
    else {
        if (opts->centerline) {
            pixel background_color;

            if (opts->backgroundSpec) 
                background_color = opts->background_color;
            else
                PPM_ASSIGN(background_color, 255, 255, 255);
            
            pixelOutlineList =
                find_centerline_pixels(*bitmap, background_color, 
                                       notify_progress, progress_data,
                                       test_cancel, testcancel_data, &exp);
        } else
            pixelOutlineList =
                find_outline_pixels(*bitmap,
                                    opts->backgroundSpec,
                                    opts->background_color, 
                                    notify_progress, progress_data,
                                    test_cancel, testcancel_data, &exp);

        if (at_exception_got_fatal(&exp) ||
            (test_cancel && test_cancel(testcancel_data)))
            retval = NULL;
        else {
            at_spline_list_array_type * splinesP;
        
            MALLOCVAR_NOFAIL(splinesP); 
            fit_outlines_to_splines(pixelOutlineList, opts,
                                    haveDistMap ? &distanceMap : NULL,
                                    image_header.width,
                                    image_header.height,
                                    &exp,
                                    notify_progress, progress_data,
                                    test_cancel, testcancel_data,
                                    splinesP);

            if (at_exception_got_fatal(&exp) ||
                (test_cancel && test_cancel(testcancel_data)))
                retval = NULL;
            else {
                if (notify_progress)
                    notify_progress(1.0, progress_data);

                retval = splinesP;
            }
            free_pixel_outline_list(&pixelOutlineList);
        }
        if (haveDistMap)
            free_distance_map(&distanceMap);
    }
    return retval;
}



void 
at_splines_write(at_output_write_func                  outputWriter,
                 FILE *                          const writeto,
                 at_output_opts_type *           const optsArg,
                 at_spline_list_array_type *     const splinesP,
                 at_msg_func                           msgFunc,
                 void *                          const msgData) {

    at_output_opts_type * optsP;
    bool newOpts;
    int llx, lly, urx, ury;
    llx = 0;
    lly = 0;
    urx = splinesP->width;
    ury = splinesP->height;
    
    if (optsArg == NULL) {
        newOpts = true;
        optsP   = at_output_opts_new();
    } else {
        newOpts = false;
        optsP   = optsArg;
    }
    (*outputWriter)(writeto, "DUMMYFILENAME",
                    llx, lly, urx, ury, optsP, *splinesP,
                    msgFunc, msgData);
    if (newOpts)
        at_output_opts_free(optsP);
}



void 
at_splines_free(at_spline_list_array_type * const splines) {

    free_spline_list_array(splines);
    free(splines);
}
