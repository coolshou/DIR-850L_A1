/* 
   fill an image area with a particular pixel value
 
   By Jim Frost 1989.10.02, Bryan Henderson 2006.03.25.
 
   See COPYRIGHT file for copyright information.
*/

#include "pm.h"
#include "image.h"
#include "valtomem.h"
#include "fill.h"

void
fill(Image * const imageP,
     unsigned int const fx,
     unsigned int const fy,
     unsigned int const fw,
     unsigned int const fh,
     Pixel        const pixval) {

    assertGoodImage(imageP);
    switch(imageP->type) {
    case IBITMAP: {
        unsigned int const linelen = (imageP->width +7)/ 8;
        unsigned int const start = (fx +7) / 8;
        unsigned char const startmask = 0x80 >> (fx % 8);

        unsigned int y;
        unsigned char * lineptr;

        for (y = fy, lineptr = imageP->data + linelen * fy;
             y < fy + fh;
             ++y, lineptr += linelen) {

            unsigned int x;
            unsigned char mask;
            unsigned char * pixptr;

            mask = startmask;
            pixptr = lineptr + start;

            for (x = fx; x < fw; ++x) {
                if (pixval)
                    *pixptr |= mask;
                else
                    *pixptr &= ~mask;
                if (!(mask >>= 1)) {
                    mask = 0x80;
                    ++pixptr;
                }
            }
        }
    } break;
        
  case IRGB:
    case ITRUE: {
        unsigned int const linelen= imageP->width * imageP->pixlen;
        unsigned int const start = imageP->pixlen * fx;

        unsigned int y;
        unsigned char * lineptr;

        for (y = fy, lineptr = imageP->data + (linelen * fy);
             y < fy + fh;
             ++y, lineptr += linelen) {

            unsigned int x;
            unsigned char * pixptr;

            pixptr = lineptr + start;
            for (x = fx, pixptr = lineptr + start;
                 x < fw;
                 ++x, pixptr += imageP->pixlen) {
                valToMem(pixval, pixptr, imageP->pixlen);
            }
        }
    } break;
    default:
        pm_error("INTERNAL ERROR: Impossible image type %u", imageP->type);
    }
}
