/* ppmtolj.c - convert a portable pixmap to an HP PCL 5 color image
**
** Copyright (C) 2000 by Jonathan Melvin (jonathan.melvin@heywood.co.uk)
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

static int compress_row_delta (unsigned char *op, unsigned char *prev_op, 
                               unsigned char *cp, int bufsize);

#define C_RESET                 "\033E"
#define C_PRESENTATION          "\033*r%dF"
#define C_PRESENTATION_LOGICAL  0
#define C_PRESENTATION_PHYSICAL 3
#define C_GAMMA                 "\033*t%dI"
#define C_IMAGE_WIDTH           "\033*r%dS"
#define C_IMAGE_HEIGHT          "\033*r%dT"
#define C_DATA_PLANES           "\033*r%dU"
#define C_TRANS_MODE            "\033*b%dM"
#define C_TRANS_MODE_STD        0 /*compression modes*/
#define C_TRANS_MODE_RLE        1 /*no good for rgb*/
#define C_TRANS_MODE_TIFF       2 /*no good for rgb*/
#define C_TRANS_MODE_DELTA      3 /*only on to use for rgb values*/
#define C_CONFIG_IMAGE_DATA     "\033*v6W"
#define C_SEND_ROW              "\033*b%dW"
#define C_BEGIN_RASTER          "\033*r%dA"
#define C_BEGIN_RASTER_CUR      1
#define C_END_RASTER            "\033*r%dC"
#define C_END_RASTER_UNUSED     0
#define C_RESOLUTION            "\033*t%dR"
#define C_RESOLUTION_300DPI     300
#define C_MOVE_X                "\033*p+%dX"
#define C_MOVE_Y                "\033*p+%dY"
#define C_LEFT_MARGIN           "\033*r%dA"
#define C_Y_OFFSET              "\033*b%dY"


/*
 * delta encoding. 
 */
/*
op row buffer
prev_op     previous row buffer
bufsize     length of row
cp          buffer for compressed data
*/
static int
compress_row_delta(op, prev_op, cp, bufsize)
unsigned char *op, *prev_op, *cp;
int bufsize;
{
    int burstStart, burstEnd, burstCode, mustBurst, ptr, skip, skipped, code;
    int deltaBufferIndex = 0;
    if (memcmp(op, prev_op , bufsize/*rowBufferIndex*/) == 0) 
        return 0; /* exact match, no deltas required */

    ptr = 0;
    skipped = 0;
    burstStart = -1;
    burstEnd = -1;
    mustBurst = 0;
    while (ptr < bufsize/*rowBufferIndex*/) 
    {
        skip = 0;
        if (ptr == 0 || skipped == 30 || op[ptr] != prev_op[ptr] ||
            (burstStart != -1 && ptr == bufsize - 1)) 
        {
            /* we want to output this byte... */
            if (burstStart == -1) 
            {
                burstStart = ptr;
            }
            if (ptr - burstStart == 7 || ptr == bufsize - 1) 
            {
                /* we have to output it now... */
                burstEnd = ptr;
                mustBurst = 1;
            }
        } 
        else 
        {
            /* duplicate byte, we can skip it */
            if (burstStart != -1) 
            {
                burstEnd = ptr - 1;
                mustBurst = 1;
            }
            skip = 1;
        }
        if (mustBurst) 
        {
            burstCode = burstEnd - burstStart; /* 0-7 means 1-8 bytes follow */
            code = (burstCode << 5) | skipped;
            cp[deltaBufferIndex++] = (char) code;
            memcpy(cp+deltaBufferIndex, op+burstStart, burstCode + 1);
            deltaBufferIndex += burstCode + 1;
            burstStart = -1;
            burstEnd = -1;
            mustBurst = 0;
            skipped = 0;
        }
        if (skip) 
        {
            skipped ++;
        }
        ptr ++;
    }
    return deltaBufferIndex;
}


int main(int argc, char *argv[]) {
    pixel * pixelrow;
    FILE *ifp;
    int argn, rows, cols, r, c, k;
    pixval maxval;
    unsigned char *obuf, *op, *cbuf, *previous_obuf;
    int format;
    int gamma = 0;
    int mode = C_TRANS_MODE_STD;
    int currentmode = 0;
    int floating = 0;  /* suppress the ``ESC & l 0 E'' ? */
    int resets = 3;    /* bit mask for when to emit printer reset seq */

    int resolution = C_RESOLUTION_300DPI;

    char CID[6] =  { 0, 3, 0, 8, 8, 8 }; 
        /*data for the configure image data command*/

    const char * const usage = "[-noreset][-float][-delta][-gamma <val>] [-resolution N] "
        "[ppmfile]\n\tresolution = [75|100|150|300|600] (dpi)";

    ppm_init( &argc, argv );

    argn = 1;
    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
        if( pm_keymatch( argv[argn], "-resolution", 2 ) && argn + 1 < argc )
        {
            ++argn;
            if ( argn == argc || sscanf( argv[argn], "%d", &resolution ) != 1 )
                pm_usage( usage );
        }
        else if ( pm_keymatch(argv[argn],"-gamma",2) && argn + 1 < argc )
        {
            ++argn;
            if ( sscanf( argv[argn], "%d",&gamma ) != 1 )
                pm_usage( usage );
        }
        else if (pm_keymatch(argv[argn],"-delta",2))
            mode = C_TRANS_MODE_DELTA;
        else if (pm_keymatch(argv[argn],"-float",2))
            floating = 1;
        else if (pm_keymatch(argv[argn],"-noreset",2))
            resets = 0;

        else
            pm_usage( usage );
        ++argn;
    }

    if ( argn < argc )
    {
        ifp = pm_openr( argv[argn] );
        ++argn;
    }
    else
        ifp = stdin;

    if ( argn != argc )
        pm_usage( usage );

    ppm_readppminit( ifp, &cols, &rows, &maxval, &format );
    pixelrow = ppm_allocrow( cols );

    obuf = (unsigned char *) pm_allocrow(cols * 3, sizeof(unsigned char));
    cbuf = (unsigned char *) pm_allocrow(cols * 6, sizeof(unsigned char));
    if (mode == C_TRANS_MODE_DELTA)
    {
        previous_obuf = 
            (unsigned char *) pm_allocrow(cols * 3, sizeof(unsigned char));
        memset(previous_obuf, 0, cols * 3);
    }

    if(resets & 1)
    {
        /* Printer reset. */
        printf(C_RESET);
    }

    if(!floating)
    {
        /* Ensure top margin is zero */
        printf("\033&l0E");
    }

    /*Set Presentation mode*/
    (void) printf(C_PRESENTATION, C_PRESENTATION_PHYSICAL);
    /* Set the resolution */
    (void) printf(C_RESOLUTION, resolution);
    /* Set raster height*/
    (void) printf(C_IMAGE_HEIGHT, rows);
    /* Set raster width*/
    (void) printf(C_IMAGE_WIDTH, cols);
    /* set left margin to current x pos*/
    /*(void) printf(C_LEFT_MARGIN, 1);*/
    /* set the correct color mode */
    (void) printf(C_CONFIG_IMAGE_DATA);
    (void) fwrite(CID, 1, 6, stdout);
    /* Start raster graphics */
    (void) printf(C_BEGIN_RASTER, C_BEGIN_RASTER_CUR);  /*posscale);*/
    /* set Y offset to 0 */
    (void) printf(C_Y_OFFSET, 0);
/*  
    if (xoff)
        (void) printf(C_MOVE_X, xoff);
    if (yoff)
        (void) printf(C_MOVE_Y, yoff);
*/
    /* Set raster compression */
    (void) printf(C_TRANS_MODE, mode);
    currentmode = mode;
    
    if(gamma)
        (void) printf(C_GAMMA,   gamma);
    
    for (r = 0; r < rows; r++)
    {
        ppm_readppmrow(ifp, pixelrow, cols, maxval, format);

        /* get a row of data with 3 bytes per pixel */
        for (c = 0, op = &obuf[-1]; c < cols; c++)
        {
            ++op;
            *op = (PPM_GETR(pixelrow[c])*255)/maxval;
            ++op;
            *op = (PPM_GETG(pixelrow[c])*255)/maxval;
            ++op;
            *op = (PPM_GETB(pixelrow[c])*255)/maxval;
        }
        ++op;
        k = op - obuf; /*size of row*/
        /*compress the row if required*/
        if(mode == C_TRANS_MODE_STD)
        {/*no compression*/
            op = obuf;
        }

        if(mode ==  C_TRANS_MODE_DELTA)
        {/*delta compression*/
            int newmode = 0;
            int deltasize = 
                compress_row_delta(obuf, previous_obuf, cbuf, cols*3);
            if(deltasize >= k)/*normal is best?*/
            {
                op = obuf;
            }
            else /*delta is best*/
            {
                k = deltasize;
                op = cbuf;
                newmode = C_TRANS_MODE_DELTA;
            }
            memcpy(previous_obuf, obuf, cols*3);

            if(currentmode != newmode)
            {
                (void) printf(C_TRANS_MODE, newmode);
                currentmode = newmode;
            }
        }

        (void) printf(C_SEND_ROW, k);
        (void) fwrite(op, 1, k, stdout);
        
    }

    (void) printf(C_END_RASTER, C_END_RASTER_UNUSED);
    if(resets & 2)
    {
        /* Printer reset. */
        printf(C_RESET);
    }
    pm_close( ifp );
    ppm_freerow(pixelrow);

    return 0;
}
