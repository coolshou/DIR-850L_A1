/* yuvtoppm.c - convert Abekas YUV bytes into a portable pixmap
**
** by Marc Boucher
** Internet: marc@PostImage.COM
** 
** Based on Example Conversion Program, A60/A64 Digital Video Interface
** Manual, page 69
**
** Uses integer arithmetic rather than floating point for better performance
**
** Copyright (C) 1991 by DHD PostImage Inc.
** Copyright (C) 1987 by Abekas Video Systems Inc.
** Copyright (C) 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "ppm.h"
#include "mallocvar.h"

/* x must be signed for the following to work correctly */
#define limit(x) ((((x)>0xffffff)?0xff0000:(((x)<=0xffff)?0:(x)&0xff0000))>>16)

int
main(argc, argv)
	char          **argv;
{
	FILE           *ifp;
	pixel          *pixrow;
	int             argn, rows, cols, row;
	const char     * const usage = "<width> <height> [yuvfile]";
    struct yuv {
        /* This is an element of a YUV file.  It describes two 
           side-by-side pixels.
        */
        unsigned char u;
        unsigned char y1;
        unsigned char v;
        unsigned char y2;
    } *yuvbuf;

	ppm_init(&argc, argv);

	argn = 1;

	if (argn + 2 > argc)
		pm_usage(usage);

	cols = atoi(argv[argn++]);
	rows = atoi(argv[argn++]);
	if (cols <= 0 || rows <= 0)
		pm_usage(usage);

	if (argn < argc) {
		ifp = pm_openr(argv[argn]);
		++argn;
	} else
		ifp = stdin;

	if (argn != argc)
		pm_usage(usage);

    if (cols % 2 != 0) {
        pm_error("Number of columns (%d) is odd.  A YUV image must have an "
                 "even number of columns.", cols);
    }

	ppm_writeppminit(stdout, cols, rows, (pixval) 255, 0);
	pixrow = ppm_allocrow(cols);
    MALLOCARRAY(yuvbuf, (cols+1)/2);
    if (yuvbuf == NULL)
        pm_error("Unable to allocate YUV buffer for %d columns.", cols);

	for (row = 0; row < rows; ++row) {
		int    col;

		fread(yuvbuf, cols * 2, 1, ifp);

		for (col = 0; col < cols; col += 2) {
            /* Produce two pixels in pixrow[] */
            int y1, u, v, y2, r, g, b;

			u = yuvbuf[col/2].u-128;

            y1 = yuvbuf[col/2].y1 - 16;
			if (y1 < 0) y1 = 0;

            v = yuvbuf[col/2].v - 128;

            y2 = yuvbuf[col/2].y2 - 16;
			if (y2 < 0) y2 = 0;

			r = 104635 * v;
			g = -25690 * u + -53294 * v;
			b = 132278 * u;

			y1*=76310; y2*=76310;

			PPM_ASSIGN(pixrow[col], limit(r+y1), limit(g+y1), limit(b+y1));
			PPM_ASSIGN(pixrow[col+1], limit(r+y2), limit(g+y2), limit(b+y2));
		}
		ppm_writeppmrow(stdout, pixrow, cols, (pixval) 255, 0);
	}
    free(yuvbuf);
    ppm_freerow(pixrow);
	pm_close(ifp);
	pm_close(stdout);

	exit(0);
}
