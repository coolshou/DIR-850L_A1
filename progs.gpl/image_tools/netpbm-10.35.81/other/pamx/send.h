#ifndef SEND_H_INCLUDED
#define SEND_H_INCLUDED

#include <X11/Xlib.h>

#include "pm_c_util.h"

struct Image;

void
sendXImage(XImageInfo * const ximageinfoP,
           int          const src_x,
           int          const src_y,
           int          const dst_x,
           int          const dst_y,
           unsigned int const w,
           unsigned int const h);

void
freeXImage(struct Image * const imageP,
           XImageInfo *   const ximageinfoP);

XImageInfo *
imageToXImage(Display *      const disp,
              int            const scrn,
              Visual *       const visualP, /* visual to use */
              unsigned int   const ddepth, /* depth of the visual to use */
              struct Image * const origImageP,
              bool           const userWantsPrivateCmap,
              bool           const userWantsFit,
              bool           const verbose);

Pixmap
ximageToPixmap(Display *    const disp,
               Window       const parent,
               XImageInfo * const ximageinfo);

#endif
