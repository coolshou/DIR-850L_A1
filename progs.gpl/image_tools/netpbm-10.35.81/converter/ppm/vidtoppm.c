/* Bryan got this from mm.ftp-cs.berkeley.edu from the package
   mpeg-encode-1.5b-src under the name vidtoppm.c on March 30, 2000.  
   The file was dated January 19, 1995.  

   This program does not use the netpbm libraries, but generates its
   output via the program rawtoppm.  If any work is ever done on it
   (or, more to the point, any interest ever expressed in it), it
   should be converted just to call ppmwrite(), etc. directly.

   There was no attached documentation, but the program appears to 
   convert from Parallax XVideo JPEG format to a sequence of PPM files.  It
   does this conversion by putting each frame in a window and then 
   reading it out of the window, using libXvid.

   Because it requires special libraries and there is no known
   requirement for it today, we are not including this program in the
   standard netpbm build.  But the source code is here in case someone
   is interested in it later.

*/

/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/* gcc -o playone playone.c -lX11 -lXvid -I/n/picasso/project/mm/xvideo/include
 */
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <XPlxExt.h>

#include "ppm.h"

usage (p)
char *p;

{
    fprintf (stderr, "Usage: %s filename width height start end outbase [quality]\n", p);
    exit (1);
}

static char buffer[300000];

Visual *
FindFullColorVisual (dpy, depth)
    Display *dpy;
    int *depth;
{
  XVisualInfo vinfo;
  XVisualInfo *vinfo_ret;
  int numitems, maxdepth;
  
  vinfo.class = TrueColor;
  
  vinfo_ret = XGetVisualInfo(dpy, VisualClassMask, &vinfo, &numitems);
  
  if (numitems == 0) return NULL;

  maxdepth = 0;
  while(numitems > 0) {
    if (vinfo_ret[numitems-1].depth > maxdepth) {
      maxdepth = vinfo_ret[numitems-1 ].depth;
    }
    numitems--;
  }
  XFree(vinfo_ret);

  if (maxdepth < 24) return NULL;

  if (XMatchVisualInfo(dpy, DefaultScreen(dpy), maxdepth, 
		       TrueColor, &vinfo)) {
    *depth = maxdepth;
    return vinfo.visual;
  }
  
  return NULL;
}

Window
CreateFullColorWindow (dpy, x, y, w, h)
    Display *dpy;
    int x, y, w, h;
{
    int depth;
    Visual *visual;
    XSetWindowAttributes xswa;
    unsigned int mask;
    unsigned int class;
    int screen;

    screen = XDefaultScreen(dpy);
    class = InputOutput;	/* Could be InputOnly */
    visual = FindFullColorVisual (dpy, &depth);
    if (visual == NULL) {
	return 0;
    }
    mask = CWBackPixel | CWColormap | CWBorderPixel;
    xswa.colormap = XCreateColormap(dpy, XRootWindow(dpy, screen),
		    visual, AllocNone);
    xswa.background_pixel = BlackPixel(dpy, DefaultScreen(dpy));
    xswa.border_pixel = WhitePixel(dpy, DefaultScreen(dpy));

    return XCreateWindow(dpy, RootWindow(dpy, screen), x, y, w, h,
	1, depth, class, visual, mask, &xswa);
}

main (argc, argv)
int argc;
char **argv;

{
  char *filename;
  Display *dpy;
  int screen;
  Window root;
  XEvent event;
  GC gc;
  Window win;
  XPlxCImage image;
  int size;
  char *qTable;
  FILE *inFile;
  FILE *outFile;
  extern char *malloc();
  int fd, r1, r2, i, j, r;
  int quality;
  int start, end;
  XImage *ximage;
  char *tdata;
  char *obase;
  char ofname[256];
  int height, width;
  char command[256];

  ppm_init(&argc, argv);

  if ((argc != 7) && (argc != 8))usage (argv[0]);
  filename = argv[1];

  width = atoi(argv[2]);
  height = atoi(argv[3]);

  start = atoi(argv[4]);
  end = atoi(argv[5]);

  if ((start < 1) || (end < start)) {
    perror ("Bad start and end values.");
    exit();
  }

  obase = argv[6];

  quality = 100;
  
  if (argc > 7)
    quality = atoi (argv[7]);
  
  dpy = XOpenDisplay (NULL);
  screen = DefaultScreen(dpy);
  root = DefaultRootWindow(dpy);
/*  gc = DefaultGC(dpy, screen); */
/*  win = XCreateSimpleWindow (dpy, root, 0, 0, width, height,
			     0, NULL, NULL);
*/
  win = CreateFullColorWindow(dpy, 0, 0, width+4, height+4);
  gc = XCreateGC(dpy, win, 0, NULL);

  if (!win ) {
    perror ("Unable to create window");
    exit(1);
  }

  XMapWindow (dpy, win);
  XSelectInput (dpy, win, ExposureMask |ButtonPressMask);

  size = MakeQTables(quality, &qTable);
  XPlxPutTable(dpy, win, gc, qTable, size, 0);
  XPlxPutTable(dpy, win, gc, qTable, size, 1);
  XPlxVideoTag (dpy, win, gc, PLX_VIDEO);
  
  inFile = fopen(filename, "rb");
  if (inFile == NULL) {
    perror (filename);
    exit (1);
  }
  fd = fileno(inFile);
  wait(2);

  for (i=0; i<start; i++) {
    if (read (fd, &image, sizeof(image)) != sizeof(image)) {
      perror("End of file.");
      exit();
    }
    image.data = buffer;
    if (read (fd, buffer, image.size) != image.size) {
      perror("End of file.");
      exit();
    }
  }
    
  for (i=start; i<=end; i++) {
    fprintf(stdout, "GRABBING FRAME %d\n", i);

    if (read (fd, &image, sizeof(image)) != sizeof(image)) {
      perror("End of file.");
      exit();
    }
    image.data = buffer;
    if (read (fd, buffer, image.size) != image.size) {
      perror("End of file.");
      exit();
    }
    
    XPlxPutCImage (dpy, win, gc, &image, 0, 0, image.width,
		   image.height, 0, 0, width+2, height+2, 1);

    XFlush(dpy);

    ximage = XGetImage(dpy, win, 0, 0, width, height, 0x00ffffff,
		       ZPixmap);
    
    if (i == 0) {
      fprintf(stderr, "Depth %d\n", ximage->depth);
      fprintf(stderr, "Height: %d Width: %d\n", height, width );
    }
    tdata = ximage->data;


    sprintf(ofname, "%s%d.ppm", obase, i);
    outFile = fopen("/tmp/foobar", "wb");
    if (!outFile) {
      perror("Couldn't open output file.");
    }

    for (r=0; r<height; r++) {
      for (j=0; j<width; j++) {
	fputc(*(tdata+3), outFile);
	fputc(*(tdata+2), outFile);
	fputc(*(tdata+1), outFile);
	tdata += 4;
      }
    }

    fclose(outFile);

    free(tdata);

    sprintf(command, "rawtoppm %d %d < /tmp/foobar > %s",
	    width, height, ofname);
    system(command);
  }
}
