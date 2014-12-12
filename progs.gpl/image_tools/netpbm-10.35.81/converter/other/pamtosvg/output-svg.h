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

#ifndef OUTPUT_SVG_H
#define OUTPUT_SVG_H

int
output_svg_writer(FILE *                    const file,
                  const char *              const name,
                  int                       const llx,
                  int                       const lly,
                  int                       const urx,
                  int                       const ury, 
                  at_output_opts_type *     const opts,
                  at_spline_list_array_type const shape,
                  at_msg_func                     msg_func, 
                  void *                    const msg_data);


#endif /* not OUTPUT_SVG_H */

