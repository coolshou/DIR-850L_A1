/* yuvsplittoppm.c - construct a portable pixmap from 3 raw files:
** - basename.Y : The Luminance chunk at the size of the Image
** - basename.U : The Chrominance chunk U at 1/4
** - basename.V : The Chrominance chunk V at 1/4
** The subsampled U and V values are made by arithmetic mean.
**
** If ccir601 is defined, the produced YUV triples have been scaled again
** to fit into the smaller range of values for this standard.
**
** by Marcel Wijkstra <wijkstra@fwi.uva.nl>
**
** Based on ppmtoyuvsplit.c
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <string.h>
#include "ppm.h"
#include "nstring.h"
#include "shhopt.h"
#include "mallocvar.h"

/* x must be signed for the following to work correctly */
#define limit(x) (((x>0xffffff)?0xff0000:((x<=0xffff)?0:x&0xff0000))>>16)

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *filenameBase;
    unsigned int width;
    unsigned int height;
    unsigned int ccir601;
        /* Whether to create YUV in JFIF(JPEG) or CCIR.601(MPEG) scale */
};


static void
parseCommandLine(int                 argc, 
                 char **             argv,
                 struct cmdlineInfo *cmdlineP ) {
/*----------------------------------------------------------------------------
   Parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "ccir601",     OPT_FLAG,   NULL,                  
            &cmdlineP->ccir601,       0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (argc-1 !=3)
        pm_error("You must specify 3 arguments.  "
                 "You specified %d", argc-1);
    else {
        int width, height;
        cmdlineP->filenameBase = argv[1];
        width = atoi(argv[2]);
        if (width < 1)
            pm_error("Width must be at least 1.  You specified %d", width);
        height = atoi(argv[3]);
        if (height < 1)
            pm_error("Height must be at least 1.  You specified %d", height);
        cmdlineP->width  = width;
        cmdlineP->height = height;
    }
}



static void
computeTwoOutputRows(int             const cols,
                     bool            const ccir601,
                     unsigned char * const y1buf,
                     unsigned char * const y2buf,
                     unsigned char * const ubuf,
                     unsigned char * const vbuf,
                     pixel *         const pixelrow1,
                     pixel *         const pixelrow2) {
                     
    int col;

    for (col = 0; col < cols; col += 2) {
        long int r0,g0,b0,r1,g1,b1,r2,g2,b2,r3,g3,b3;
        long int  u,v,y0,y1,y2,y3,u0,u1,u2,u3,v0,v1,v2,v3;
        
        y0 = y1buf[col];
        y1 = y1buf[col+1];
        y2 = y2buf[col];
        y3 = y2buf[col+1];

        u = ubuf[col/2] - 128;
        v = vbuf[col/2] - 128;
        
        if (ccir601) {
            y0 = ((y0-16)*255)/219;
            y1 = ((y1-16)*255)/219;
            y2 = ((y2-16)*255)/219;
            y3 = ((y3-16)*255)/219;

            u  = (u*255)/224 ;
            v  = (v*255)/224 ;
        }
        /* mean the chroma for subsampling */
        
        u0=u1=u2=u3=u;
        v0=v1=v2=v3=v;


        /* The inverse of the JFIF RGB to YUV Matrix for $00010000 = 1.0

           [Y]   [65496        0   91880] [R]
           [U] = [65533   -22580  -46799] [G]
           [V]   [65537   116128      -8] [B]
           
        */

        r0 = 65536 * y0               + 91880 * v0;
        g0 = 65536 * y0 -  22580 * u0 - 46799 * v0;
        b0 = 65536 * y0 + 116128 * u0             ;
        
        r1 = 65536 * y1               + 91880 * v1;
        g1 = 65536 * y1 -  22580 * u1 - 46799 * v1;
        b1 = 65536 * y1 + 116128 * u1             ;
        
        r2 = 65536 * y2               + 91880 * v2;
        g2 = 65536 * y2 -  22580 * u2 - 46799 * v2;
        b2 = 65536 * y2 + 116128 * u2             ;
        
        r3 = 65536 * y3               + 91880 * v3;
        g3 = 65536 * y3 -  22580 * u3 - 46799 * v3;
        b3 = 65536 * y3 + 116128 * u3             ;
        
        r0 = limit(r0);
        r1 = limit(r1);
        r2 = limit(r2);
        r3 = limit(r3);
        g0 = limit(g0);
        g1 = limit(g1);
        g2 = limit(g2);
        g3 = limit(g3);
        b0 = limit(b0);
        b1 = limit(b1);
        b2 = limit(b2);
        b3 = limit(b3);

        PPM_ASSIGN(pixelrow1[col],   (pixval)r0, (pixval)g0, (pixval)b0);
        PPM_ASSIGN(pixelrow1[col+1], (pixval)r1, (pixval)g1, (pixval)b1);
        PPM_ASSIGN(pixelrow2[col],   (pixval)r2, (pixval)g2, (pixval)b2);
        PPM_ASSIGN(pixelrow2[col+1], (pixval)r3, (pixval)g3, (pixval)b3);
    }
}



int
main(int argc, char **argv) {

    struct cmdlineInfo cmdline;
    FILE *vf,*uf,*yf;
    int cols, rows;
    pixel *pixelrow1,*pixelrow2;
    int row;
    unsigned char  *y1buf,*y2buf,*ubuf,*vbuf;
    const char * ufname;
    const char * vfname;
    const char * yfname;

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
        
    asprintfN(&ufname, "%s.U", cmdline.filenameBase);
    asprintfN(&vfname, "%s.V", cmdline.filenameBase);
    asprintfN(&yfname, "%s.Y", cmdline.filenameBase);

    uf = pm_openr(ufname);
    vf = pm_openr(vfname);
    yf = pm_openr(yfname);

    ppm_writeppminit(stdout, cmdline.width, cmdline.height, 255, 0);

    if (cmdline.width % 2 != 0) {
        pm_message("Warning: odd width; last column ignored");
        cols = cmdline.width - 1;
    } else
        cols = cmdline.width;

    if (cmdline.height % 2 != 0) {
        pm_message("Warning: odd height; last row ignored");
        rows = cmdline.height - 1;
    } else 
        rows = cmdline.height;

    pixelrow1 = ppm_allocrow(cols);
    pixelrow2 = ppm_allocrow(cols);

    MALLOCARRAY_NOFAIL(y1buf, cmdline.width);
    MALLOCARRAY_NOFAIL(y2buf, cmdline.width);
    MALLOCARRAY_NOFAIL(ubuf,  cmdline.width/2);
    MALLOCARRAY_NOFAIL(vbuf,  cmdline.width/2);

    for (row = 0; row < rows; row += 2) {
        fread(y1buf, cmdline.width,   1, yf);
        fread(y2buf, cmdline.width,   1, yf);
        fread(ubuf,  cmdline.width/2, 1, uf);
        fread(vbuf,  cmdline.width/2, 1, vf);

        computeTwoOutputRows(cols, cmdline.ccir601,
                             y1buf, y2buf, ubuf, vbuf,
                             pixelrow1, pixelrow2);

        ppm_writeppmrow(stdout, pixelrow1, cols, (pixval) 255, 0);
        ppm_writeppmrow(stdout, pixelrow2, cols, (pixval) 255, 0);
    }
    pm_close(stdout);

    strfree(yfname);
    strfree(vfname);
    strfree(ufname);

    pm_close(yf);
    pm_close(uf);
    pm_close(vf);

    exit(0);
}
