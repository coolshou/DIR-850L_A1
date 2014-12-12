#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include <X11/Xlib.h>

#include "pm_c_util.h"

struct Image;
struct viewer;
typedef struct viewer viewer;

int
visualClassFromName(const char * const name);

void
createViewer(viewer **     const viewerPP,
             Display *     const dispP,
             int           const scrn,
             const char *  const geometryString,
             bool          const fullscreen);

void
destroyViewer(viewer * const viewerP);

void
displayInViewer(viewer *       const viewerP,
                struct Image * const imageP,
                bool           const install,
                bool           const userWantsPrivateCmap,
                bool           const fit,
                bool           const userWantsPixmap,
                bool           const visualSpec,
                unsigned int   const visualClass,
                const char *   const title,
                bool           const verbose,
                int *          const retvalP);

#endif
