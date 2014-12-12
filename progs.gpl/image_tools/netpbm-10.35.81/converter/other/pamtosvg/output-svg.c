/* output-svg.h - output in SVG format

   Copyright (C) 1999, 2000, 2001 Bernhard Herzog

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#include "spline.h"
#include "output-svg.h"



static void
outSplineList(FILE *           const fileP,
              spline_list_type const splineList,
              unsigned int     const height) {
              
    unsigned splineSeq;
        
    for (splineSeq = 0;
         splineSeq < SPLINE_LIST_LENGTH(splineList);
         ++splineSeq) {
        
        spline_type const spline = SPLINE_LIST_ELT(splineList, splineSeq);
        
        if (SPLINE_DEGREE(spline) == LINEARTYPE) {
            fprintf(fileP, "L%g %g",
                    END_POINT(spline).x, height - END_POINT(spline).y);
        } else
            fprintf(fileP, "C%g %g %g %g %g %g",
                    CONTROL1(spline).x, height  - CONTROL1(spline).y,
                    CONTROL2(spline).x, height  - CONTROL2(spline).y,
                    END_POINT(spline).x, height - END_POINT(spline).y);
    }
}



static void
out_splines(FILE *                 const fileP,
            spline_list_array_type const shape,
            unsigned int           const height) {

    unsigned listSeq;
    pixel lastColor;
    
    PPM_ASSIGN(lastColor, 0, 0, 0);
    
    for (listSeq = 0;
         listSeq < SPLINE_LIST_ARRAY_LENGTH(shape);
         ++listSeq) {
        
        spline_list_type const splineList =
            SPLINE_LIST_ARRAY_ELT(shape, listSeq);
        spline_type const first = SPLINE_LIST_ELT(splineList, 0);

        if (listSeq == 0 || !PPM_EQUAL(splineList.color, lastColor)) {
            if (listSeq > 0) {
                /* Close previous <path> element */
                if (!(shape.centerline || splineList.open))
                    fputs("z", fileP);
                fputs("\"/>\n", fileP);
            }
            /* Open new <path> element */
            fprintf(fileP, "<path style=\"%s:#%02x%02x%02x; %s:none;\" d=\"",
                    (shape.centerline || splineList.open) ? "stroke" : "fill",
                    PPM_GETR(splineList.color),
                    PPM_GETG(splineList.color),
                    PPM_GETB(splineList.color),
                    (shape.centerline || splineList.open) ? "fill" : "stroke");
        }
        fprintf(fileP, "M%g %g",
                START_POINT(first).x, height - START_POINT(first).y);

        outSplineList(fileP, splineList, height);

        lastColor = splineList.color;
    }

    if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0) {
        spline_list_type const lastSplineList =
            SPLINE_LIST_ARRAY_ELT(shape, SPLINE_LIST_ARRAY_LENGTH(shape)-1);

        if (!(shape.centerline || lastSplineList.open))
            fputs("z", fileP);

        /* Close last <path> element */
        fputs("\"/>\n", fileP);
    }
}



int
output_svg_writer(FILE *                    const fileP,
                  const char *              const name,
                  int                       const llx,
                  int                       const lly,
                  int                       const urx,
                  int                       const ury, 
                  at_output_opts_type *     const opts,
                  at_spline_list_array_type const shape,
                  at_msg_func                     msg_func, 
                  void *                    const msg_data) {

    int const width  = urx - llx;
    int const height = ury - lly;

    fputs("<?xml version=\"1.0\" standalone=\"yes\"?>\n", fileP);

    fprintf(fileP, "<svg width=\"%d\" height=\"%d\">\n", width, height);

    out_splines(fileP, shape, height);

    fputs("</svg>\n", fileP);
    
    return 0;
}
