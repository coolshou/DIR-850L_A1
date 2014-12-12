/*
   Functions to allocate and deallocate structures and structure data
 
   By Jim Frost 1989.10.03, Bryan Henderson 2006.03.25.
 
   See COPYRIGHT file for copyright information.
*/

#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xdefs.h>  /* Needed by Xutil.h */
#include <X11/X.h>      /* Needed by Xutil.h */
#include <X11/Xlib.h>   /* Needed by Xutil.h */
#include <X11/Xutil.h>

#include "mallocvar.h"
#include "nstring.h"
#include "pm.h"
#include "ximageinfo.h"
#include "send.h"
#include "image.h"
#include "window.h"

/* A viewer object is something in which you can display an image.  You
   can display multiple images in the same viewer, sequentially.

   The viewer has a permanent window, called the viewport.  When you display
   an image, it has a second window, called the image window, which is a
   child of the viewport.
*/

struct viewer {
    Display *    dispP;
    int          scrn;
    Window       imageWin;
    Window       viewportWin;
    Colormap     imageColormap;
    Cursor       cursor;
    unsigned int xpos, ypos;
    unsigned int width, height;
    bool         fullscreen;
    bool         userChoseGeometry;
    Atom         deleteAtom;
    bool         blank;
        /* I'm just guessing, but it seems that a "paint" operation is
           necessary the first time a viewport is used, but not when
           displaying a new image in an old viewport.  I assume that's
           because the new viewport is blank, and that's what this
           value means.
        */
    Pixmap       pixmap;
};



static void
setXloadimageClassHint(Display * const dispP,
                       Window    const window) {

    XClassHint classhint;

    classhint.res_class = (char*)"Xloadimage";
    classhint.res_name  = (char*)"xloadimage";

    XSetClassHint(dispP, window, &classhint);
}



static void
setDeleteWindow(viewer * const viewerP) {

    Atom const protoAtom = XInternAtom(viewerP->dispP, "WM_PROTOCOLS", False);
    
    viewerP->deleteAtom = XInternAtom(viewerP->dispP,
                                      "WM_DELETE_WINDOW", False);
    
    if ((protoAtom != None) && (viewerP->deleteAtom != None))
        XChangeProperty(viewerP->dispP, viewerP->viewportWin,
                        protoAtom, XA_ATOM, 32, PropModeReplace,
                        (unsigned char *)&viewerP->deleteAtom, 1);
}



static void
getInitialViewerGeometry(const char *   const geometryString,
                         bool           const fullscreen,
                         Display *      const dispP,
                         int            const scrn,
                         unsigned int * const xposP,
                         unsigned int * const yposP,
                         unsigned int * const widthP,
                         unsigned int * const heightP,
                         bool *         const userChoseP) {

    unsigned int const defaultWidth  = 10;
    unsigned int const defaultHeight = 10;

    if (fullscreen) {
        *widthP  = DisplayWidth(dispP, scrn);
        *heightP = DisplayHeight(dispP, scrn);
        *xposP   = 0;
        *yposP   = 0;
        *userChoseP = TRUE;
    } else if (geometryString) {
        const char * defGeom;
        asprintfN(&defGeom, "%ux%u+0+0", defaultWidth, defaultHeight);
        XGeometry(dispP, scrn, geometryString, defGeom, 0, 1, 1, 0, 0,
                  (int *)xposP, (int *)yposP,
                  (int *)widthP, (int *)heightP);
        strfree(defGeom);
        *userChoseP = TRUE;
    } else {
        *widthP     = defaultWidth;
        *heightP    = defaultHeight;
        *xposP      = 0;
        *yposP      = 0;
        *userChoseP = FALSE;
    }
}



void
createViewer(viewer **     const viewerPP,
             Display *     const dispP,
             int           const scrn,
             const char *  const geometryString,
             bool          const fullscreen) {

    viewer * viewerP;
    
    XSetWindowAttributes  swa_view;

    MALLOCVAR_NOFAIL(viewerP);

    viewerP->dispP       = dispP;
    viewerP->scrn        = scrn;
    viewerP->fullscreen  = fullscreen;
    viewerP->imageWin    = 0;
    viewerP->viewportWin = 0;

    getInitialViewerGeometry(geometryString, fullscreen, dispP, scrn,
                             &viewerP->xpos, &viewerP->ypos,
                             &viewerP->width, &viewerP->height,
                             &viewerP->userChoseGeometry);

    viewerP->cursor = XCreateFontCursor(dispP, XC_watch);

    swa_view.background_pixel = WhitePixel(dispP, scrn);
    swa_view.backing_store    = NotUseful;
    swa_view.cursor           = viewerP->cursor;
    swa_view.event_mask       =
        ButtonPressMask | Button1MotionMask | KeyPressMask |
        StructureNotifyMask | EnterWindowMask | LeaveWindowMask;
    swa_view.save_under       = FALSE;
    
    viewerP->viewportWin =
        XCreateWindow(dispP, RootWindow(dispP, scrn),
                      viewerP->xpos, viewerP->ypos,
                      viewerP->width, viewerP->height, 0,
                      DefaultDepth(dispP, scrn), InputOutput,
                      DefaultVisual(dispP, scrn),
                      CWBackingStore | CWBackPixel | CWCursor |
                      CWEventMask | CWSaveUnder,
                      &swa_view);

    setXloadimageClassHint(viewerP->dispP, viewerP->viewportWin);
    
    setDeleteWindow(viewerP);

    viewerP->blank = TRUE;

    *viewerPP = viewerP;
}



static void
determineRepaintStrategy(viewer  *    const viewerP,
                         bool         const userWantsPixmap,
                         bool         const verbose,
                         XImageInfo * const ximageinfoP,
                         Pixmap *     const pixmapP) {
                        
    /* Decide how we're going to handle repaints.  We have three modes:
       use backing-store, use background pixmap, and use exposures.
       If the server supports backing-store, we enable it and use it.
       This really helps servers which are memory constrained.  If the
       server does not have backing-store, we try to send the image to
       a pixmap and use that as backing-store.  If that fails, we use
       exposures to blit the image (which is ugly but it works).
       
       'use_pixmap' forces background pixmap mode, which may improve
       performance.
    */

    ximageinfoP->drawable = viewerP->imageWin;
    if ((DoesBackingStore(ScreenOfDisplay(viewerP->dispP, viewerP->scrn)) ==
         NotUseful) ||
        userWantsPixmap) {
        *pixmapP = ximageToPixmap(viewerP->dispP, viewerP->imageWin,
                                  ximageinfoP);
        if (*pixmapP == None && verbose)
            pm_message("Cannot create image in server; "
                       "repaints will be ugly!");
    } else
        *pixmapP = None;
}



static void
setImageWindowAttr(Display * const dispP,
                   int       const scrn,
                   Window    const imageWindow,
                   Colormap  const cmap,
                   Pixmap    const pixmap) {

    /* build window attributes for the image window */

    XSetWindowAttributes swa_img;
    unsigned int         wa_mask_img;

    swa_img.bit_gravity  = NorthWestGravity;
    swa_img.save_under   = FALSE;
    swa_img.colormap     = cmap;
    swa_img.border_pixel = 0;

    wa_mask_img = 0;  /* initial value */

    if (pixmap == None) {
        /* No pixmap.  Must paint over the wire.  Ask for BackingStore
           to cut down on the painting.  But, ask for Exposures so we can
           paint both viewables and backingstore.
        */

        swa_img.background_pixel = WhitePixel(dispP,scrn);
        wa_mask_img |= CWBackPixel;
        swa_img.event_mask = ExposureMask;
        wa_mask_img |= CWEventMask;
        swa_img.backing_store = WhenMapped;
        wa_mask_img |= CWBackingStore;
    } else {
        /* We have a pixmap so tile the window.  to move the image we only
           have to move the window and the server should do the rest.
         */

        swa_img.background_pixmap = pixmap;
        wa_mask_img |= CWBackPixmap;
        swa_img.event_mask = 0; /* no exposures please */
        wa_mask_img |= CWEventMask;
        swa_img.backing_store = NotUseful;
        wa_mask_img |= CWBackingStore;
    }
    XChangeWindowAttributes(dispP, imageWindow, wa_mask_img, &swa_img);
}



static void
createImageWindow(viewer *      const viewerP,
                  XImageInfo *  const ximageInfoP,
                  Image *       const imageP,
                  Visual *      const visualP,
                  bool          const userWantsPixmap,
                  bool          const verbose) {

    XSetWindowAttributes  swa_img;

    swa_img.bit_gravity  = NorthWestGravity;
    swa_img.save_under   = FALSE;
    swa_img.colormap     = ximageInfoP->cmap;
    swa_img.border_pixel = 0;
    viewerP->imageWin = XCreateWindow(viewerP->dispP, viewerP->viewportWin,
                                      viewerP->xpos, viewerP->ypos,
                                      imageP->width, imageP->height, 0,
                                      ximageInfoP->depth, InputOutput, visualP,
                                      CWBitGravity | CWColormap | CWSaveUnder |
                                      CWBorderPixel, &swa_img);
    viewerP->imageColormap = ximageInfoP->cmap;
    setXloadimageClassHint(viewerP->dispP, viewerP->imageWin);

    determineRepaintStrategy(viewerP, userWantsPixmap, verbose, ximageInfoP,
                             &viewerP->pixmap);

    setImageWindowAttr(viewerP->dispP, viewerP->scrn,
                       viewerP->imageWin, ximageInfoP->cmap,
                       viewerP->pixmap);
}



static void
destroyImageWindow(viewer * const viewerP) {
    
    if (viewerP->imageWin) {
        if (viewerP->imageColormap &&
            (viewerP->imageColormap != DefaultColormap(viewerP->dispP, viewerP->scrn)))
            XFreeColormap(viewerP->dispP, viewerP->imageColormap);
        XDestroyWindow(viewerP->dispP, viewerP->imageWin);
    }
}
                       


static void
changeCursor(viewer *      const viewerP,
             unsigned int  const imageWidth,
             unsigned int  const imageHeight) {

    Cursor cursor;
    XSetWindowAttributes swa;

    if ((viewerP->width >= imageWidth) && (viewerP->height >= imageHeight))
        cursor = XCreateFontCursor(viewerP->dispP, XC_icon);
    else if ((viewerP->width < imageWidth) && (viewerP->height >= imageHeight))
        cursor = XCreateFontCursor(viewerP->dispP, XC_sb_h_double_arrow);
    else if ((viewerP->width >= imageWidth) && (viewerP->height < imageHeight))
        cursor = XCreateFontCursor(viewerP->dispP, XC_sb_v_double_arrow);
    else
        cursor = XCreateFontCursor(viewerP->dispP, XC_fleur);

    swa.cursor = cursor;
    XChangeWindowAttributes(viewerP->dispP, viewerP->viewportWin,
                            CWCursor, &swa);

    XFreeCursor(viewerP->dispP, viewerP->cursor);

    viewerP->cursor = cursor;
}



static void
placeImage(viewer * const viewerP,
           int      const width,
           int      const height,
           int *    const rxP,     /* input and output */
           int *    const ryP) {   /* input and output */

    int pixx, pixy;
    
    pixx = *rxP;
    pixy = *ryP;
    
    if (viewerP->width > width)
        pixx = (viewerP->width - width) / 2;
    else {
        if ((pixx < 0) && (pixx + width < viewerP->width))
            pixx = viewerP->width - width;
        if (pixx > 0)
            pixx = 0;
    }
    if (viewerP->height > height)
        pixy = (viewerP->height - height) / 2;
    else {
        if ((pixy < 0) && (pixy + height < viewerP->height))
            pixy = viewerP->height - viewerP->height;
        if (pixy > 0)
            pixy = 0;
    }
    XMoveWindow(viewerP->dispP, viewerP->imageWin, pixx, pixy);

    *rxP = pixx;
    *ryP = pixy;
}



static void
blitImage(XImageInfo * const ximageinfoP,
          unsigned int const width,
          unsigned int const height,
          int          const xArg,
          int          const yArg,
          int          const wArg,
          int          const hArg) {

    int w, h;
    int x, y;

    w = MIN(wArg, width);
    h = MIN(hArg, height);

    x = xArg;
    y = yArg;

    if (x < 0) {
        XClearArea(ximageinfoP->disp, ximageinfoP->drawable,
                   x, y, -x, h, False);
        w -= (0 - x);
        x = 0;
    }
    if (y < 0) {
        XClearArea(ximageinfoP->disp, ximageinfoP->drawable,
                   x, y, w, -y, False);
        h -= (0 - y);
        y = 0;
    }
    if (x + w > width) {
        XClearArea(ximageinfoP->disp, ximageinfoP->drawable,
                   x + width, y, x + w - width, h, False);
        w -= x + w - width;
    }
    if (y + h > height) {
        XClearArea(ximageinfoP->disp, ximageinfoP->drawable,
                   x, y + height, w, y + h - height, False);
        h -= y + h - height;
    }
    sendXImage(ximageinfoP, x, y, x, y, w, h);
}



void
destroyViewer(viewer * const viewerP) {
/*----------------------------------------------------------------------------
   Clean up static window.
-----------------------------------------------------------------------------*/
    if (viewerP->pixmap != None)
        XFreePixmap(viewerP->dispP, viewerP->pixmap);

    if (viewerP->imageWin)
        XDestroyWindow(viewerP->dispP, viewerP->imageWin);
    viewerP->imageWin = 0;
    
    if (viewerP->viewportWin)
        XDestroyWindow(viewerP->dispP, viewerP->viewportWin);
    
    viewerP->viewportWin = 0;

    XFreeCursor(viewerP->dispP, viewerP->cursor);

    free(viewerP);
}



static void
setViewportColormap(viewer *  const viewerP,
                    Visual *  const visualP) {
/*----------------------------------------------------------------------------
  Set the colormap and WM_COLORMAP_WINDOWS properly for the viewport.
-----------------------------------------------------------------------------*/
    static Atom cmap_atom = None;
    XSetWindowAttributes swa;
    Window cmap_windows[2];

    if (cmap_atom == None)
        cmap_atom = XInternAtom(viewerP->dispP, "WM_COLORMAP_WINDOWS", False);
    
    /* If the visual we're using is the same as the default visual (used by
       the viewport window) then we can set the viewport window to use the
       image's colormap.  This keeps most window managers happy.
    */
    
    if (visualP == DefaultVisual(viewerP->dispP, viewerP->scrn)) {
        swa.colormap = viewerP->imageColormap;
        XChangeWindowAttributes(viewerP->dispP, viewerP->viewportWin,
                                CWColormap, &swa);
        XDeleteProperty(viewerP->dispP, viewerP->viewportWin, cmap_atom);
    } else {
        /* Smart window managers can handle it when we use a different colormap
           in our subwindow so long as we set the WM_COLORMAP_WINDOWS property
           ala ICCCM.
        */
        cmap_windows[0] = viewerP->imageWin;
        cmap_windows[1] = viewerP->viewportWin;
        XChangeProperty(viewerP->dispP, viewerP->viewportWin,
                        cmap_atom, XA_WINDOW, 32,
                        PropModeReplace, (unsigned char *)cmap_windows, 2);
    }
}



static const char *
iconName(const char * const s) {
/*----------------------------------------------------------------------------
   Return an icon name suitable for an image titled 's'.

   s == NULL means untitled.
-----------------------------------------------------------------------------*/
    const char * retval;
    char buf[BUFSIZ];

    if (!s)
        STRSCPY(buf, "Unnamed");
    else {
        char * t;

        STRSCPY(buf, s);
        /* strip off stuff following 1st word.  This strips
           info added by processing functions too.
        */
        t = index(buf, ' ');
        if (t)
            *t = '\0';
    
        /* Strip off leading path.  if you don't use unix-style paths,
           You might want to change this.
        */
    
        t= rindex(buf, '/');
        if (t) {
            char * p;
            for (p = buf, ++t; *t; ++p, ++t)
                *p = *t;
            *p = '\0';
        }
        /* look for an extension and strip it off */
        t = index(buf, '.');
        if (t)
            *t = '\0';
    }

    retval = strdup(buf);
    if (retval == NULL)
        pm_error("Out of memory");
    return retval;
}



/* visual class to name table  */

static struct visual_class_name {
    int          class; /* numerical value of class */
    const char * name;  /* actual name of class */
} const VisualClassName[] = {
    {TrueColor,   "TrueColor"   } ,
    {DirectColor, "DirectColor" } ,
    {PseudoColor, "PseudoColor" } ,
    {StaticColor, "StaticColor" } ,
    {GrayScale,   "GrayScale"   } ,
    {StaticGray,  "StaticGray"  } ,
    {StaticGray,  "StaticGrey"  } ,
    {-1,          NULL          }
};



int
visualClassFromName(const char * const name) {

    unsigned int a;
    int class;
    bool found;
    
    for (a = 0, found = FALSE; VisualClassName[a].name; ++a) {
        if (STRCASEEQ(VisualClassName[a].name, name)) {
            /* Check for uniqueness.  We special-case StaticGray
               because we have two spellings but they are unique if
               we find either.
            */
            if (found && class != StaticGray)
                pm_error("'%s' does not uniquely describe a visual class",
                         name);

            class = VisualClassName[a].class;
            found = TRUE;
        }
    }
    if (!found)
        pm_error("'%s' is not a visual class name", name);

    return class;
}



static const char *
nameOfVisualClass(int const class) {

    unsigned int a;
    const char * name;
    bool found;

    for (a = 0, found = FALSE; VisualClassName[a].name; ++a) {
        if (VisualClassName[a].class == class) {
            name = VisualClassName[a].name;
            found = TRUE;
        }
    }
    if (found)
        return name;
    else
        return "[Unknown Visual Class]";
}



/* find the best visual of a particular class with a particular depth
 */

static Visual *
bestVisualOfClassAndDepth(Display *    const disp,
                          int          const scrn,
                          int          const class,
                          unsigned int const depth) {

    long const vinfoMask =
        VisualScreenMask | VisualClassMask | VisualDepthMask;

    Visual * best;
    XVisualInfo template;
    XVisualInfo * infoP;
    unsigned int nvisuals;

    best = NULL;  /* initial value */

    template.screen = scrn;
    template.class  = class;
    template.depth  = depth;
    infoP = XGetVisualInfo(disp, vinfoMask, &template, (int*)&nvisuals);
    if (infoP) {
        /* There are visuals with this depth */

        /* Not sure what to do if this gives more than one visual of a
           particular class and depth, so just return the first one.
        */

        best = infoP->visual;
        XFree((char *)infoP);
    }
    return best;
}



static void
bestVisual(Display *      const disp,
           int            const scrn,
           Image *        const imageP,
           Visual **      const rvisualPP,
           unsigned int * const rdepthP) {
/*----------------------------------------------------------------------------
  Try to determine the best available visual to use for a particular
  image
-----------------------------------------------------------------------------*/
    unsigned int  depth, a;
    Screen * screen;
    Visual * visualP;
    Visual * default_visualP;

    /* Figure out the best depth the server supports.  note that some servers
       (such as the HP 11.3 server) actually say they support some depths but
       have no visuals that support that depth.  Seems silly to me ...
    */

    depth = 0;
    screen = ScreenOfDisplay(disp, scrn);
    for (a = 0; a < screen->ndepths; ++a) {
        if (screen->depths[a].nvisuals &&
            ((!depth ||
              ((depth < imageP->depth) && (screen->depths[a].depth > depth)) ||
              ((screen->depths[a].depth >= imageP->depth) &&
               (screen->depths[a].depth < depth)))))
            depth = screen->depths[a].depth;
    }
    if (!depth) {
        /* this shouldn't happen */
        pm_message("bestVisual: didn't find any depths?!?");
        depth = DefaultDepth(disp, scrn);
    }
    
    /* given this depth, find the best possible visual
     */
    
    default_visualP = DefaultVisual(disp, scrn);
    switch (imageP->type) {
    case ITRUE: {
        /* If the default visual is DirectColor or TrueColor prioritize such
           that we use the default type if it exists at this depth
        */

        if (default_visualP->class == TrueColor) {
            visualP = bestVisualOfClassAndDepth(disp, scrn, TrueColor, depth);
            if (!visualP)
                visualP = bestVisualOfClassAndDepth(disp, scrn,
                                                    DirectColor, depth);
        } else {
            visualP = bestVisualOfClassAndDepth(disp, scrn, DirectColor,
                                                depth);
            if (!visualP)
                visualP = bestVisualOfClassAndDepth(disp, scrn, TrueColor,
                                                    depth);
        }
        
        if (!visualP || ((depth <= 8) &&
                         bestVisualOfClassAndDepth(disp, scrn, PseudoColor,
                                                   depth)))
            visualP = bestVisualOfClassAndDepth(disp, scrn, PseudoColor,
                                                depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, StaticColor,
                                                depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, GrayScale, depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, StaticGray, depth);
    } break;
        
    case IRGB: {
        /* if it's an RGB image, we want PseudoColor if we can get it */

        visualP = bestVisualOfClassAndDepth(disp, scrn, PseudoColor, depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, DirectColor,
                                                depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, TrueColor, depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, StaticColor,
                                                depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, GrayScale, depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, StaticGray, depth);
    } break;

    case IBITMAP: {
        visualP = bestVisualOfClassAndDepth(disp, scrn, PseudoColor, depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, StaticColor,
                                                depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, GrayScale, depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, StaticGray, depth);

        /* It seems pretty wasteful to use a TrueColor or DirectColor visual
           to display a bitmap (2-color) image, so we look for those last
        */

        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, DirectColor,
                                                depth);
        if (!visualP)
            visualP = bestVisualOfClassAndDepth(disp, scrn, TrueColor, depth);
    } break;
    }

    if (!visualP) { /* this shouldn't happen */
        pm_message("bestVisual: couldn't find one?!?");
        depth = DefaultDepth(disp, scrn);
        visualP = DefaultVisual(disp, scrn);
    }
    *rvisualPP = visualP;
    *rdepthP   = depth;
}



static void
bestVisualOfClass(Display *      const disp,
                  int            const scrn,
                  Image *        const image,
                  int            const visual_class,
                  Visual **      const rvisual,
                  unsigned int * const rdepth) {
/*----------------------------------------------------------------------------
  Given a visual class, try to find the best visual of that class at
  the best depth.  Return a null visual and depth if we couldn't find
  any visual of that type at any depth.
-----------------------------------------------------------------------------*/
  Visual       *visual;
  Screen       *screen;
  unsigned int  a, b, depth;

  /* loop through depths looking for a visual of a good depth which matches
   * our visual class.
   */

  screen= ScreenOfDisplay(disp, scrn);
  visual= (Visual *)NULL;
  depth= 0;
  for (a= 0; a < screen->ndepths; a++) {
    for (b= 0; b < screen->depths[a].nvisuals; b++) {
      if ((screen->depths[a].visuals[b].class == visual_class) &&
      (!depth ||
       ((depth < image->depth) && (screen->depths[a].depth > depth)) ||
       ((screen->depths[a].depth >= image->depth) &&
        (screen->depths[a].depth < depth)))) {
    depth= screen->depths[a].depth;
    visual= &(screen->depths[a].visuals[b]);
      }
    }
  }
  *rvisual= visual;
  *rdepth= depth;
}



static void
getImageDispDimensions(viewer *       const viewerP,
                       Image *        const imageP,
                       unsigned int * const widthP,
                       unsigned int * const heightP) {

    if (viewerP->userChoseGeometry) {
        *widthP  = viewerP->width;
        *heightP = viewerP->height;
    } else {
        unsigned int const displayWidth = 
            DisplayWidth(viewerP->dispP, viewerP->scrn);
        unsigned int const displayHeight =
            DisplayHeight(viewerP->dispP, viewerP->scrn);

        /* We don't use more than 90% of display real estate unless user
           explicitly asked for it.
        */
        *widthP = MIN(imageP->width, (unsigned)(displayWidth * 0.9));
        *heightP = MIN(imageP->height, (unsigned)(displayHeight * 0.9));
    }
}



static void
getVisualAndDepth(Image *        const imageP,
                  Display *      const dispP,
                  int            const scrn,
                  bool           const fit,
                  bool           const visualSpec,
                  unsigned int   const visualClass,
                  Visual **      const visualPP,
                  unsigned int * const depthP) {


    /* If the user told us to fit the colormap, we must use the default
       visual.
    */

    if (fit) {
        *visualPP = DefaultVisual(dispP, scrn);
        *depthP   = DefaultDepth(dispP, scrn);
    } else {
        Visual * visualP;
        unsigned int depth;

        visualP = NULL;
        if (!visualSpec) {
            /* Try to pick the best visual for the image. */

            bestVisual(dispP, scrn, imageP, &visualP, &depth);
        } else {
            /* Try to find a visual of the specified class */

            bestVisualOfClass(dispP, scrn, imageP, visualClass,
                              &visualP, &depth);
            if (!visualP) {
                bestVisual(dispP, scrn, imageP, &visualP, &depth);
                pm_message("Server does not provide %s visual, using %s",
                           nameOfVisualClass(visualClass),
                           nameOfVisualClass(visualP->class));
            }
        }
        *visualPP = visualP;
        *depthP   = depth;
    }
}



static void
setNormalSizeHints(viewer *     const viewerP,
                   Image *      const imageP) {
    
    XSizeHints sh;
    
    sh.width  = viewerP->width;
    sh.height = viewerP->height;
    if (viewerP->fullscreen) {
        sh.min_width  = sh.max_width  = viewerP->width;
        sh.min_height = sh.max_height = viewerP->height;
    } else {
        sh.min_width  = 1;
        sh.min_height = 1;
        sh.max_width  = imageP->width;
        sh.max_height = imageP->height;
    }
    sh.width_inc  = 1;
    sh.height_inc = 1;

    sh.flags = PMinSize | PMaxSize | PResizeInc;
    if (viewerP->userChoseGeometry)
        sh.flags |= USSize;
    else
        sh.flags |= PSize;

    if (viewerP->userChoseGeometry) {
        sh.x = viewerP->xpos;
        sh.y = viewerP->ypos;
        sh.flags |= USPosition;
    }
    XSetNormalHints(viewerP->dispP, viewerP->viewportWin, &sh);

    sh.min_width  = sh.max_width;
    sh.min_height = sh.max_height;
    XSetNormalHints(viewerP->dispP, viewerP->imageWin, &sh);
        /* Image doesn't shrink */
}



static void
setWMHints(viewer * const viewerP) {

    XWMHints wmh;

    wmh.input = TRUE;
    wmh.flags = InputHint;
    XSetWMHints(viewerP->dispP, viewerP->viewportWin, &wmh);
}


#define CTL_C '\003'

typedef enum exitReason {
    EXIT_NONE,
    EXIT_QUIT,
    EXIT_WM_KILL,
    EXIT_DESTROYED
} exitReason;



static void
run(viewer *     const viewerP,
    Image *      const imageP,
    XImageInfo * const ximageInfoP,
    int          const initPixx,
    int          const initPixy,
    bool         const install,
    exitReason * const exitReasonP) {

    int lastx, lasty;
    int pixx, pixy;
    union {
        XEvent              event;
        XAnyEvent           any;
        XButtonEvent        button;
        XKeyEvent           key;
        XConfigureEvent     configure;
        XExposeEvent        expose;
        XMotionEvent        motion;
        XResizeRequestEvent resize;
        XClientMessageEvent message;
    } event;
    exitReason exitReason;

    lastx = lasty = -1;
    pixx   = initPixx;
    pixy   = initPixy;

    exitReason = EXIT_NONE;  /* No reason to exit yet */

    while (exitReason == EXIT_NONE) {
        XNextEvent(viewerP->dispP, &event.event);

        switch (event.any.type) {
        case ButtonPress:
            if (event.button.button == 1) {
                lastx = event.button.x;
                lasty = event.button.y;
            }
            break;

        case KeyPress: {
            char buf[128];
            KeySym ks;
            XComposeStatus status;
            Status rc;
            
            rc = XLookupString(&event.key, buf, 128, &ks, &status);
            if (rc == 1) {
                char const ret = buf[0];
                char const lowerRet = tolower(ret);

                switch (lowerRet) {
                case CTL_C:
                case 'q':
                    exitReason = EXIT_QUIT;
                    break;
                }
            }
        } break;

        case MotionNotify: {
            int mousex, mousey;

            if (imageP->width  <= viewerP->width &&
                imageP->height <= viewerP->height) {
                /* we're AT&T */
            } else {
                mousex = event.button.x;
                mousey = event.button.y;
                while (XCheckTypedEvent(viewerP->dispP, MotionNotify,
                                        (XEvent*)&event)) {
                    mousex = event.button.x;
                    mousey = event.button.y;
                }
                pixx -= (lastx - mousex);
                pixy -= (lasty - mousey);
                lastx = mousex;
                lasty = mousey;
                placeImage(viewerP, imageP->width, imageP->height,
                           &pixx, &pixy);
            }
        } break;

        case ConfigureNotify:
            viewerP->width  = event.configure.width;
            viewerP->height = event.configure.height;

            placeImage(viewerP, imageP->width, imageP->height,
                       &pixx, &pixy);

            /* Configure the cursor to indicate which directions we can drag
             */

            changeCursor(viewerP, imageP->width, imageP->height);
            break;

        case DestroyNotify:
            exitReason = EXIT_DESTROYED;
            break;

        case Expose:
            blitImage(ximageInfoP, imageP->width, imageP->height,
                      event.expose.x, event.expose.y,
                      event.expose.width, event.expose.height);
            break;

        case EnterNotify:
            if (install)
                XInstallColormap(viewerP->dispP, ximageInfoP->cmap);
            break;

        case LeaveNotify:
            if (install)
                XUninstallColormap(viewerP->dispP, ximageInfoP->cmap);
            break;

        case ClientMessage:
            /* If we get a client message for the viewport window
               which has the value of the delete atom, it means the
               window manager wants us to die.
            */

            if ((event.message.window == viewerP->viewportWin) &&
                (event.message.data.l[0] == viewerP->deleteAtom)) {
                exitReason = EXIT_WM_KILL;
            }
            break;
        }
    }
    *exitReasonP = exitReason;
}



static int
retvalueFromExitReason(exitReason const exitReason) {

    int retval;

    switch (exitReason) {
    case EXIT_NONE:      assert(false); break;
    case EXIT_QUIT:      retval =  0;     break;
    case EXIT_WM_KILL:   retval = 10;     break;
    case EXIT_DESTROYED: retval = 20;     break;
    }
    return retval;
}



static void
resizeViewer(viewer *     const viewerP,
             unsigned int const newWidth,
             unsigned int const newHeight) {
    
    if (viewerP->width != newWidth || viewerP->height != newHeight) {
        XResizeWindow(viewerP->dispP, viewerP->viewportWin,
                      newWidth, newHeight);
        viewerP->width  = newWidth;
        viewerP->height = newHeight;
    }
}



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
                int *          const retvalP) {
    
    XImageInfo *     ximageInfoP;
    Visual *         visual;
    unsigned int     depth;
    bool             privateCmap;
    int              pixx, pixy;
    exitReason       exitReason;
    unsigned int     windowWidth, windowHeight;

    pixx = -1; pixy = -1;  /* initial values */

    getImageDispDimensions(viewerP, imageP, &windowWidth, &windowHeight);

    resizeViewer(viewerP, windowWidth, windowHeight);

    getVisualAndDepth(imageP, viewerP->dispP, viewerP->scrn,
                      fit, visualSpec, visualClass,
                      &visual, &depth);
    
    if (verbose && (visual != DefaultVisual(viewerP->dispP, viewerP->scrn)))
        pm_message("Using %s visual", nameOfVisualClass(visual->class));

    /* If we're in slideshow mode and the user told us to fit the colormap,
       free it here.
    */

    if (viewerP->blank) {
        /* For the first image we display we can use the default cmap.
           subsequent images use a private colormap (unless they're
           bitmaps) so we don't get color erosion when switching
           images.
        */

        if (fit) {
            XDestroyWindow(viewerP->dispP, viewerP->imageWin);
            viewerP->imageWin = 0;
            viewerP->imageColormap = 0;
            privateCmap = userWantsPrivateCmap;
        } else if (!BITMAPP(imageP))
            privateCmap = TRUE;
    } else
        privateCmap = userWantsPrivateCmap;

    ximageInfoP = imageToXImage(viewerP->dispP, viewerP->scrn, visual, depth,
                                imageP, privateCmap, fit, verbose);
    if (!ximageInfoP)
        pm_error("INTERNAL ERROR: Cannot convert Image to XImage");

    destroyImageWindow(viewerP);

    createImageWindow(viewerP, ximageInfoP, imageP, visual,
                      userWantsPixmap, verbose);

    if (title)
        XStoreName(viewerP->dispP, viewerP->viewportWin, title);
    else
        XStoreName(viewerP->dispP, viewerP->viewportWin, "Unnamed");

    {
        const char * const name = iconName(title);
        XSetIconName(viewerP->dispP, viewerP->viewportWin, name);
        strfree(name);
    }
    setNormalSizeHints(viewerP, imageP);

    setWMHints(viewerP);

    setViewportColormap(viewerP, visual);

    /* Map (display) windows */

    XMapWindow(viewerP->dispP, viewerP->imageWin);
    XMapWindow(viewerP->dispP, viewerP->viewportWin);

    /* Start displaying image */

    placeImage(viewerP, imageP->width, imageP->height, &pixx, &pixy);
    if (!viewerP->blank) {
        XResizeWindow(viewerP->dispP, viewerP->imageWin,
                      imageP->width, imageP->height);
        /* Clear the image window.  Ask for exposure if there is no tile. */
        XClearArea(viewerP->dispP, viewerP->imageWin, 0, 0, 0, 0,
                   (viewerP->pixmap == None));
        viewerP->blank = FALSE;
    }

    changeCursor(viewerP, imageP->width, imageP->height);

    /* Process X events, continuously */
    run(viewerP, imageP, ximageInfoP, pixx, pixy, install, &exitReason);

    freeXImage(imageP, ximageInfoP);

    *retvalP = retvalueFromExitReason(exitReason);
}
