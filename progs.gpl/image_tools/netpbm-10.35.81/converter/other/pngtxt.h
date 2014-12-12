#ifndef PNGTXT_H_INCLUDED
#define PNGTXT_H_INCLUDED

#include "pm_c_util.h"
#include <png.h>

void 
pnmpng_read_text (png_info * const info_ptr, 
                  FILE *     const tfp, 
                  bool const ztxt,
                  bool const verbose);

#endif
