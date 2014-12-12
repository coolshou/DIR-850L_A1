/*
 
  Send an Image to an X pixmap


  By Jim Frost 1989.10.02, Bryan Henderson 2006.03.25.
 
  Copyright 1989, 1990, 1991 Jim Frost.
  See COPYRIGHT file for copyright information.
*/

#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "pm_c_util.h"
#include "pm.h"
#include "mallocvar.h"
#include "ximageinfo.h"
#include "valtomem.h"
#include "image.h"
#include "send.h"

#define TRUE_TO_15BIT(PIXEL)     \
    ((((PIXEL) & 0xf80000) >> 9) | \
    (((PIXEL) & 0x00f800) >> 6) | \
    (((PIXEL) & 0x0000f8) >> 3))

#define RED_INTENSITY(P)   (((P) & 0x7c00) >> 10)
#define GREEN_INTENSITY(P) (((P) & 0x03e0) >> 5)
#define BLUE_INTENSITY(P)   ((P) & 0x001f)
#define PM_SCALE(a, b, c) (long)((a) * (c))/(b)


static bool GotError;

static int
pixmapErrorTrap(Display *     const disp,
                XErrorEvent * const  pErrorEvent) {

#define MAXERRORLEN 100
    char buf[MAXERRORLEN+1];
    GotError = 1;
    XGetErrorText(disp, pErrorEvent->error_code, buf, MAXERRORLEN);
    pm_message("serial #%ld (request code %d) Got Error '%s'",
               pErrorEvent->serial,
               pErrorEvent->request_code,
               buf);
    return 0;
}



Pixmap
ximageToPixmap(Display *    const disp,
               Window       const parent,
               XImageInfo * const ximageinfoP) {

    XErrorHandler old_handler;
    Pixmap        pixmap;
  
    GotError = FALSE;
    old_handler = XSetErrorHandler(pixmapErrorTrap);
    XSync(disp, False);
    pixmap= XCreatePixmap(disp, parent,
                          ximageinfoP->ximageP->width,
                          ximageinfoP->ximageP->height,
                          ximageinfoP->depth);
    XSetErrorHandler(old_handler);
    if (GotError)
        return None;
    ximageinfoP->drawable = pixmap;
    sendXImage(ximageinfoP, 0, 0, 0, 0,
               ximageinfoP->ximageP->width, ximageinfoP->ximageP->height);

    return pixmap;
}



/* find the best pixmap depth supported by the server for a particular
 * visual and return that depth.
 *
 * this is complicated by R3's lack of XListPixmapFormats so we fake it
 * by looking at the structure ourselves.
 */

static unsigned int
bitsPerPixelAtDepth(Display *    const disp,
                    int          const scrn,
                    unsigned int const depth) {

#if XlibSpecificationRelease < 4 /* the way things were */
  unsigned int a;

  for (a= 0; a < disp->nformats; a++)
    if (disp->pixmap_format[a].depth == depth)
      return(disp->pixmap_format[a].bits_per_pixel);

#else /* the way things should be */
  XPixmapFormatValues *xf;
  unsigned int nxf, a;

  xf = XListPixmapFormats(disp, (int *)&nxf);
  for (a = 0; a < nxf; a++)
    if (xf[a].depth == depth)
      return(xf[a].bits_per_pixel);
#endif

  /* this should never happen; if it does, we're in trouble
   */

  fprintf(stderr, "bitsPerPixelAtDepth: Can't find pixmap depth info!\n");
  exit(1);
}
     


static Image *
itrueToRGB(Image *      const imageP,
           unsigned int const ddepth) {

    int y, x, num_pixels, colors;
    unsigned long pixel_counts[32786];
    unsigned long pixel_array[32786];
    Pixel pixval;
    unsigned char * pixel;
    unsigned char * dpixel;
    Image * newImageP;
    
    newImageP = newRGBImage(imageP->width, imageP->height, ddepth);

    colors = 1 << ddepth;
  
    memset(pixel_counts, 0, 32768 * sizeof(unsigned long));
  
    pixel= imageP->data;
    for (y= 0; y < imageP->height; y++) {
        unsigned int x;
        for (x= 0; x < imageP->width; x++) {
            unsigned int const z = TRUE_TO_15BIT(memToVal(pixel, 3));
            pixel_counts[z]++;
            pixel += 3;
        }
    }
    num_pixels = 0;
    for (x = 0; x < 32768; ++x) {
        if (pixel_counts[x] > 0) {
            unsigned long const red = RED_INTENSITY(x);
            unsigned long const grn = GREEN_INTENSITY(x);
            unsigned long const blu = BLUE_INTENSITY(x);
            pixel_counts[x] = num_pixels;
            *(newImageP->rgb.red + num_pixels) = red<<11;
            *(newImageP->rgb.grn + num_pixels) = grn<<11; 
            *(newImageP->rgb.blu + num_pixels) =  blu<<11;
            pixel_array[num_pixels++] = (short)x;
            if (num_pixels > colors)
                break;
        }
    }    

    pixel = imageP->data;
    dpixel = newImageP->data;
    
    for (y = 0; y < imageP->height; ++y) {
        unsigned int x;
        for (x = 0; x < imageP->width; ++x) {
            unsigned int const z = TRUE_TO_15BIT(memToVal(pixel, 3));
            pixval = pixel_counts[z];
            valToMem(pixval, dpixel, newImageP->pixlen);
            pixel += 3;
            dpixel += newImageP->pixlen;
        }
    }
    newImageP->rgb.used = num_pixels;
    newImageP->rgb.compressed = 1;

    return newImageP;
}



static void
makeUsableVisual(Image *      const origImageP,
                 Visual *     const visualP,
                 unsigned int const ddepth,
                 Image **     const newImagePP) {

    /* process image based on type of visual to which we're sending */

    switch (origImageP->type) {
    case ITRUE:
        switch (visualP->class) {
        case TrueColor:
        case DirectColor:
            /* goody goody */
            *newImagePP = origImageP;
            break;
        case PseudoColor:
            *newImagePP = itrueToRGB(origImageP, ddepth);
            if (*newImagePP == NULL)
                pm_error("Unable to convert for Pseudocolor.");
            break;
        default:
            pm_error("INTERNAL ERROR: impossible visual class %u",
                     visualP->class);
        }
        break;
        
    case IRGB:
        switch(visualP->class) {
        case TrueColor:
        case DirectColor:
            /* no problem, we handle this just fine */
            *newImagePP = origImageP;
            break;
        default:
            pm_error("INTERNAL ERROR: impossible visual class %u",
                     visualP->class);
        }
        
    case IBITMAP:
        /* no processing ever needs to be done for bitmaps */
        *newImagePP = origImageP;
        break;
    }
}    



static void
makeColorMap1(Display *  const disp,
              int        const scrn,
              Visual *   const visualP,
              Colormap * const cmapP,
              Pixel **   const redvalueP,
              Pixel **   const grnvalueP,
              Pixel **   const bluvalueP) {
    
    Pixel * redvalue;
    Pixel * grnvalue;
    Pixel * bluvalue;
    Pixel pixval;
    unsigned int redcolors, grncolors, blucolors;
    unsigned int redstep, grnstep, blustep;
    unsigned int redbottom, grnbottom, blubottom;
    unsigned int redtop, grntop, blutop;
    unsigned int a;
            
    MALLOCARRAY_NOFAIL(redvalue, 256);
    MALLOCARRAY_NOFAIL(grnvalue, 256);
    MALLOCARRAY_NOFAIL(bluvalue, 256);
            
    if (visualP == DefaultVisual(disp, scrn))
        *cmapP = DefaultColormap(disp, scrn);
    else
        *cmapP = XCreateColormap(disp, RootWindow(disp, scrn),
                                 visualP, AllocNone);
            
 retry_direct: /* tag we hit if a DirectColor allocation fails on
                * default colormap */
            
    /* calculate number of distinct colors in each band */
            
    redcolors = grncolors = blucolors = 1;
    for (pixval = 1; pixval; pixval <<= 1) {
        if (pixval & visualP->red_mask)
            redcolors <<= 1;
        if (pixval & visualP->green_mask)
            grncolors <<= 1;
        if (pixval & visualP->blue_mask)
            blucolors <<= 1;
    }
            
    /* sanity check */
            
    if ((redcolors > visualP->map_entries) ||
        (grncolors > visualP->map_entries) ||
        (blucolors > visualP->map_entries)) {
        pm_message("Warning: inconsistency in color information "
                   "(this may be ugly)");
    }
            
    redstep= 256 / redcolors;
    grnstep= 256 / grncolors;
    blustep= 256 / blucolors;
    redbottom = grnbottom = blubottom= 0;
    for (a = 0; a < visualP->map_entries; ++a) {
        XColor xcolor;
        Status rc;
        if (redbottom < 256)
            redtop = redbottom + redstep;
        if (grnbottom < 256)
            grntop = grnbottom + grnstep;
        if (blubottom < 256)
            blutop = blubottom + blustep;
                
        xcolor.flags = DoRed | DoGreen | DoBlue;
        xcolor.red   = (redtop - 1) << 8;
        xcolor.green = (grntop - 1) << 8;
        xcolor.blue  = (blutop - 1) << 8;
        rc = XAllocColor(disp, *cmapP, &xcolor);
        if (rc == 0) {
            /* Allocation failed.  If it's for a DirectColor default
               visual then we should create a private colormap
               and try again.
            */

            if ((visualP->class == DirectColor) &&
                (visualP == DefaultVisual(disp, scrn))) {
                *cmapP = XCreateColormap(disp, RootWindow(disp, scrn),
                                         visualP, AllocNone);
                goto retry_direct;
            }
                    
            /* something completely unexpected happened */
                    
            pm_error("INTERNAL ERROR: XAllocColor failed on a "
                     "TrueColor/Directcolor visual");
        }
                
        /* fill in pixel values for each band at this intensity */
                
        while ((redbottom < 256) && (redbottom < redtop))
            redvalue[redbottom++] = xcolor.pixel & visualP->red_mask;
        while ((grnbottom < 256) && (grnbottom < grntop))
            grnvalue[grnbottom++] = xcolor.pixel & visualP->green_mask;
        while ((blubottom < 256) && (blubottom < blutop))
            bluvalue[blubottom++] = xcolor.pixel & visualP->blue_mask;
    }
    *redvalueP   = redvalue;
    *grnvalueP   = grnvalue;
    *bluvalueP   = bluvalue;
}


 
static void
allocColorCells(Display *      const disp,
                Colormap       const cmap,
                Pixel *        const colorIndex,
                unsigned int   const colorCount,
                unsigned int * const cellCountP) {

    bool outOfCells;
    unsigned int cellCount;
    
    outOfCells = false;  /* initial value */
    cellCount = 0;       /* initial value */
    while (cellCount < colorCount && !outOfCells) {
        Status rc;
        rc = XAllocColorCells(disp, cmap, FALSE, NULL, 0,
                              &colorIndex[cellCount++], 1);
        if (rc == 0)
            outOfCells = true;
    }
    *cellCountP = cellCount;
}

    


static void
makeColorMap2(Display *  const disp,
              int        const scrn,
              Visual *   const visualP,
              RGBMap     const rgb,
              bool       const userWantsPrivateCmap,
              bool       const userWantsFit,
              bool       const verbose,
              Colormap * const cmapP,
              Pixel **   const colorIndexP) {

    bool privateCmap;
    bool fit;
    bool newmap;
    Pixel * colorIndex;

    MALLOCARRAY_NOFAIL(colorIndex, rgb.used);
        
    /* 'privateCmap' is invalid if not a dynamic visual */
        
    switch (visualP->class) {
    case StaticColor:
    case StaticGray:
        privateCmap = TRUE;
    default:
        privateCmap = userWantsPrivateCmap;
    }
        
    /* get the colormap to use. */
        
    if (privateCmap) { /* user asked us to use a private cmap */
        newmap = TRUE;
        fit = FALSE;
    } else if ((visualP == DefaultVisual(disp, scrn)) ||
               (visualP->class == StaticGray) ||
               (visualP->class == StaticColor) ||
               (visualP->class == TrueColor) ||
               (visualP->class == DirectColor)) {
            
        unsigned int a;

        fit = userWantsFit;

        /* if we're using the default visual, try to alloc colors
           shareable.  otherwise we're using a static visual and
           should treat it accordingly.
        */
            
        if (visualP == DefaultVisual(disp, scrn))
            *cmapP = DefaultColormap(disp, scrn);
        else
            *cmapP = XCreateColormap(disp, RootWindow(disp, scrn),
                                     visualP, AllocNone);
        newmap = FALSE;
            
        /* allocate colors shareable (if we can) */
            
        for (a = 0; a < rgb.used; ++a) {
            Status rc;
            XColor  xcolor;

            xcolor.flags = DoRed | DoGreen | DoBlue;
            xcolor.red   = rgb.red[a];
            xcolor.green = rgb.grn[a];
            xcolor.blue  = rgb.blu[a];
            rc = XAllocColor(disp, *cmapP, &xcolor);
            if (rc == 0) {
                if ((visualP->class == StaticColor) ||
                    (visualP->class == StaticGray) ||
                    (visualP->class == TrueColor) ||
                    (visualP->class == DirectColor)) {
                    pm_error("XAllocColor failed on a static visual");
                } else {
                    /* We can't allocate the colors shareable so
                       free all the colors we had allocated and
                       create a private colormap (or fit into the
                       default cmap if `fit' is true).
                    */
                    XFreeColors(disp, *cmapP, colorIndex, a, 0);
                    newmap = TRUE;
                    break;
                }
            }
            colorIndex[a] = xcolor.pixel;
        }
    } else {
        newmap = TRUE;
        fit    = FALSE;
    }
        
    if (newmap) {
        /* Either create a new colormap or fit the image into the
           one we have.  To create a new one, we create a private
           cmap and allocate the colors writable.  Fitting the
           colors is harder; we have to:

           1. grab the server so no one can goof with the colormap.
           2. count the available colors using XAllocColorCells.
           3. free the colors we just allocated.
           4. reduce the depth of the image to fit.
           5. allocate the colors again shareable.
           6. ungrab the server and continue on our way.
               
           Someone should shoot the people who designed X color allocation.
        */
            
        unsigned int a;

        if (fit) {
            if (verbose)
                pm_message("Fitting image into default colormap");
            XGrabServer(disp);
        } else {
            if (verbose)
                pm_message("Using private colormap");
                
            /* create new colormap */
                
            *cmapP = XCreateColormap(disp, RootWindow(disp, scrn),
                                     visualP, AllocNone);
        }
            
        allocColorCells(disp, *cmapP, colorIndex, rgb.used, &a);

        if (fit) {
            if (a > 0)
                XFreeColors(disp, *cmapP, colorIndex, a, 0);
            if (a <= 2)
                pm_error("Cannot fit into default colormap");
        }
            
        if (a == 0)
            pm_error("Color allocation failed!");
            
        if (fit) {
            unsigned int a;
            for (a = 0; a < rgb.used; ++a) {
                XColor xcolor;
                xcolor.flags = DoRed | DoGreen | DoBlue;
                xcolor.red   = rgb.red[a];
                xcolor.green = rgb.grn[a];
                xcolor.blue  = rgb.blu[a];
                
                if (!XAllocColor(disp, *cmapP, &xcolor))
                    pm_error("XAllocColor failed while fitting colormap!");
                colorIndex[a] = xcolor.pixel;
            }
            XUngrabServer(disp);
        } else {
            unsigned int b;
            for (b = 0; b < a; ++b) {
                XColor xcolor;
                xcolor.flags = DoRed | DoGreen | DoBlue;
                xcolor.pixel = colorIndex[b];
                xcolor.red   = rgb.red[b];
                xcolor.green = rgb.grn[b];
                xcolor.blue  = rgb.blu[b];
                XStoreColor(disp, *cmapP, &xcolor);
            }
        }
    }
    *colorIndexP = colorIndex;
}



static void
doColorAllocation(XImageInfo * const ximageinfoP,
                  Display *    const disp,
                  int          const scrn,
                  Visual *     const visualP,
                  Image *      const imageP,
                  bool         const userWantsPrivateCmap,
                  bool         const userWantsFit,
                  bool         const verbose,
                  Pixel **     const colorIndexP,
                  Pixel **     const redvalP,
                  Pixel **     const grnvalP,
                  Pixel **     const bluvalP) {
    
    if ((visualP->class == TrueColor || visualP->class == DirectColor) &&
        !BITMAPP(imageP)) {
        makeColorMap1(disp, scrn, visualP, &ximageinfoP->cmap,
                      redvalP, grnvalP, bluvalP);
        *colorIndexP = NULL;
    } else {
        makeColorMap2(disp, scrn, visualP, imageP->rgb,
                      userWantsPrivateCmap, userWantsFit, verbose,
                      &ximageinfoP->cmap, colorIndexP);
        
        *redvalP = *grnvalP = *bluvalP = NULL;
    }
}
    



static void
makeXImage(XImageInfo * const ximageinfoP,
           Display *    const disp,
           int          const scrn,
           Visual *     const visualP,
           unsigned int const ddepth,
           Image *      const imageP,
           Pixel        const colorIndex[],
           Pixel        const redvalue[],
           Pixel        const grnvalue[],
           Pixel        const bluvalue[],
           bool         const verbose) {
/*----------------------------------------------------------------------------
  Create an XImage and related colormap based on the image type we
  have.
-----------------------------------------------------------------------------*/
    if (verbose)
        pm_message("Building XImage...");

    switch (imageP->type) {
    case IBITMAP: {
        unsigned int const byteCount =
            (imageP->width + 7) / 8 * imageP->height;
        unsigned char * data;

        /* we copy the data to be more consistent */

        MALLOCARRAY(data, byteCount);
        if (data == NULL)
            pm_error("Can't allocate space for %u byte image", byteCount);
        bcopy(imageP->data, data, byteCount);

        ximageinfoP->ximageP =
            XCreateImage(disp, visualP, 1, XYBitmap,
                         0, (char*)data, imageP->width, imageP->height, 8, 0);
        ximageinfoP->depth = ddepth;
        ximageinfoP->foreground = colorIndex[1];
        ximageinfoP->background = colorIndex[0];
        ximageinfoP->ximageP->bitmap_bit_order = MSBFirst;
        ximageinfoP->ximageP->byte_order = MSBFirst;
    } break;

    case IRGB:
    case ITRUE: {
        /* Modify image data to match visual and colormap */
        
        unsigned int const dbits = bitsPerPixelAtDepth(disp, scrn, ddepth);
        unsigned int const dpixlen = (dbits + 7) / 8;

        ximageinfoP->depth = ddepth;
        
        switch (visualP->class) {
        case DirectColor:
        case TrueColor: {
            unsigned char * data;
            unsigned char * destptr;
            unsigned char * srcptr;
        
            ximageinfoP->ximageP =
                XCreateImage(disp, visualP, ddepth, ZPixmap, 0,
                             NULL, imageP->width, imageP->height, 8, 0);
            MALLOCARRAY(data, imageP->width * imageP->height * dpixlen);
            if (data == NULL)
                pm_error("Unable to allocate space for %u x %u x %u image",
                         imageP->width, imageP->height, dpixlen);
            ximageinfoP->ximageP->data = (char*)data;
            destptr = data;
            srcptr = imageP->data;
            switch (imageP->type) {
            case ITRUE: {
                unsigned int y;
                for (y= 0; y < imageP->height; ++y) {
                    unsigned int x;
                    for (x= 0; x < imageP->width; ++x) {
                        Pixel const pixval = memToVal(srcptr, imageP->pixlen);
                        Pixel const newpixval =
                            redvalue[TRUE_RED(pixval)] |
                            grnvalue[TRUE_GRN(pixval)] |
                            bluvalue[TRUE_BLU(pixval)];
                        valToMem(newpixval, destptr, dpixlen);
                        srcptr += imageP->pixlen;
                        destptr += dpixlen;
                    }
                }
            } break;
            case IRGB: {
                unsigned int y;
                for (y= 0; y < imageP->height; ++y) {
                    unsigned int x;
                    for (x = 0; x < imageP->width; ++x) {
                        Pixel const pixval = memToVal(srcptr, imageP->pixlen);
                        Pixel const newpixval =
                            redvalue[imageP->rgb.red[pixval] >> 8] |
                            grnvalue[imageP->rgb.grn[pixval] >> 8] |
                            bluvalue[imageP->rgb.blu[pixval] >> 8];
                        valToMem(newpixval, destptr, dpixlen);
                        srcptr += imageP->pixlen;
                        destptr += dpixlen;
                    }
                }
            } break;
            default: /* something's broken */
                pm_error("INTERNAL ERROR: Unexpected image type %u for "
                         "DirectColor/TrueColor visual!", imageP->type);
            }
            ximageinfoP->ximageP->byte_order = MSBFirst;
                /* Trust me, I know what I'm talking about */
        } break;

        default: {
            
            /* only IRGB images make it this far. */

            /* If our XImage doesn't have modulus 8 bits per pixel,
               it's unclear how to pack bits so we instead use an
               XYPixmap image.  This is slower.
            */

            if (dbits % 8) {
                unsigned int const linelen = (imageP->width + 7) / 8;
                unsigned int const size =
                    imageP->width * imageP->height * dpixlen;
                unsigned char * data;
                unsigned char * destptr;
                unsigned char * srcptr;
                Pixel pixval;
                unsigned int a;

                ximageinfoP->ximageP =
                    XCreateImage(disp, visualP, ddepth, XYPixmap, 0,
                                 NULL, imageP->width, imageP->height, 8, 0);

                MALLOCARRAY(data, size);
                if (data == NULL)
                    pm_error("Unable to allocate space for %u x %x x %u "
                             "image", imageP->width, imageP->height, dpixlen);
                ximageinfoP->ximageP->data = (char*)data;
                memset(data, 0, size);
                ximageinfoP->ximageP->bitmap_bit_order = MSBFirst;
                ximageinfoP->ximageP->byte_order = MSBFirst;
                for (a= 0; a < dbits; ++a) {
                    Pixel const pixmask = 1 << a;
                    unsigned char * const destdata = 
                        data + ((ddepth - a - 1) * imageP->height * linelen);

                    unsigned int y;

                    srcptr = imageP->data;
                    for (y= 0; y < imageP->height; ++y) {
                        unsigned int x;
                        unsigned char mask;
                        destptr = destdata + (y * linelen);
                        *destptr = 0;
                        mask = 0x80;
                        for (x= 0; x < imageP->width; ++x) {
                            pixval = memToVal(srcptr, imageP->pixlen);
                            srcptr += imageP->pixlen;
                            if (colorIndex[pixval] & pixmask)
                                *destptr |= mask;
                            mask >>= 1;
                            if (mask == 0) {
                                mask = 0x80;
                                ++destptr;
                            }
                        }
                    }
                }
            } else {
                unsigned int const dpixlen =
                    (ximageinfoP->ximageP->bits_per_pixel + 7) / 8;

                unsigned char * data;
                unsigned char * srcptr;
                unsigned char * destptr;
                unsigned int y;

                ximageinfoP->ximageP =
                    XCreateImage(disp, visualP, ddepth, ZPixmap, 0,
                                 NULL, imageP->width, imageP->height, 8, 0);

                MALLOCARRAY(data, imageP->width * imageP->height * dpixlen);
                if (data == NULL)
                    pm_error("Failed to allocate space for %u x %u x %u image",
                             imageP->width, imageP->height, dpixlen);
                ximageinfoP->ximageP->data = (char*)data;
                ximageinfoP->ximageP->byte_order= MSBFirst;
                    /* Trust me, i know what I'm talking about */
                srcptr = imageP->data;
                destptr = data;
                for (y= 0; y < imageP->height; ++y) {
                    unsigned int x;
                    for (x= 0; x < imageP->width; x++) {
                        valToMem(colorIndex[memToVal(srcptr, imageP->pixlen)],
                                 destptr, dpixlen);
                        srcptr += imageP->pixlen;
                        destptr += dpixlen;
                    }
                }
            }
        } break;
        }   
    } break;
    }
    if (verbose)
        pm_message("done");
}



XImageInfo *
imageToXImage(Display *    const disp,
              int          const scrn,
              Visual *     const visualP, /* visual to use */
              unsigned int const ddepth, /* depth of the visual to use */
              Image *      const origImageP,
              bool         const userWantsPrivateCmap,
              bool         const userWantsFit,
              bool         const verbose) {

    XImageInfo * ximageinfoP;
    Image * imageP;
    Pixel * colorIndex;
    Pixel * redvalue;
    Pixel * grnvalue;
    Pixel * bluvalue;

    assertGoodImage(origImageP);
  
    MALLOCVAR_NOFAIL(ximageinfoP);
    ximageinfoP->disp = disp;
    ximageinfoP->scrn = scrn;
    ximageinfoP->depth = 0;
    ximageinfoP->drawable = None;
    ximageinfoP->foreground = ximageinfoP->background = 0;
    ximageinfoP->gc = NULL;
    ximageinfoP->ximageP = NULL;
  
    makeUsableVisual(origImageP, visualP, ddepth, &imageP);

    assertGoodImage(imageP);

    doColorAllocation(ximageinfoP, disp, scrn, visualP, imageP,
                      userWantsPrivateCmap, userWantsFit, verbose,
                      &colorIndex, &redvalue, &grnvalue, &bluvalue);

    makeXImage(ximageinfoP, disp, scrn, visualP, ddepth, imageP,
               colorIndex, redvalue, grnvalue, bluvalue, verbose);

    if (colorIndex)
        free(colorIndex);
    if (redvalue) {
        free(redvalue);
        free(grnvalue);
        free(bluvalue);
    }
    if (imageP != origImageP)
        freeImage(imageP);
    
    return ximageinfoP;
}



void
sendXImage(XImageInfo * const ximageinfoP,
           int          const src_x,
           int          const src_y,
           int          const dst_x,
           int          const dst_y,
           unsigned int const w,
           unsigned int const h) {
/*----------------------------------------------------------------------------
  Given an XImage and a drawable, move a rectangle from the Ximage
  to the drawable.
-----------------------------------------------------------------------------*/
    XGCValues gcv;

    /* build and cache the GC */
    
    if (!ximageinfoP->gc) {
        gcv.function = GXcopy;
        if (ximageinfoP->ximageP->depth == 1) {
            gcv.foreground = ximageinfoP->foreground;
            gcv.background= ximageinfoP->background;
            ximageinfoP->gc =
                XCreateGC(ximageinfoP->disp, ximageinfoP->drawable,
                          GCFunction | GCForeground | GCBackground,
                          &gcv);
        }else
            ximageinfoP->gc =
                XCreateGC(ximageinfoP->disp, ximageinfoP->drawable,
                          GCFunction, &gcv);
    }
    
    XPutImage(ximageinfoP->disp, ximageinfoP->drawable, ximageinfoP->gc,
              ximageinfoP->ximageP, src_x, src_y, dst_x, dst_y, w, h);
}



void
freeXImage(Image *      const imageP,
           XImageInfo * const ximageinfoP) {
/*----------------------------------------------------------------------------
  free up anything cached in the local Ximage structure.
-----------------------------------------------------------------------------*/
    if (ximageinfoP->gc)
        XFreeGC(ximageinfoP->disp, ximageinfoP->gc);
    free(ximageinfoP->ximageP->data);
    ximageinfoP->ximageP->data = NULL;
    XDestroyImage(ximageinfoP->ximageP);

    free(ximageinfoP);
}
