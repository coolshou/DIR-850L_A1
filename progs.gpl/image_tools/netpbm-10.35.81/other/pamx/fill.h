#ifndef FILL_H_INCLUDED
#define FILL_H_INCLUDED

#include "ximageinfo.h"

struct Image;

void
fill(struct Image * const imageP,
     unsigned int   const fx,
     unsigned int   const fy,
     unsigned int   const fw,
     unsigned int   const fh,
     Pixel          const pixval);

#endif
