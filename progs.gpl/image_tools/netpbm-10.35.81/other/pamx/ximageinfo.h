#ifndef XIMAGEINFO_H_INCLUDED
#define XIMAGEINFO_H_INCLUDED

#include <X11/Xlib.h>



typedef unsigned long  Pixel;     /* what X thinks a pixel is */
typedef unsigned short Intensity; /* what X thinks an RGB intensity is */

typedef struct {
    /* This struct holds the X-client side bits for a rendered image.  */
    Display * disp;       /* destination display */
    int       scrn;       /* destination screen */
    int       depth;      /* depth of drawable we want/have */
    Drawable  drawable;   /* drawable to send image to */
    Pixel     foreground;
        /* foreground and background pixels for mono images */
    Pixel     background;
    Colormap  cmap;       /* colormap used for image */
    GC        gc;         /* cached gc for sending image */
    XImage *  ximageP;     /* ximage structure */
} XImageInfo;

#endif
