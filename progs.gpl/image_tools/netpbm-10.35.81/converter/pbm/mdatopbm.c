
/***************************************************************************

    MDATOPBM: Convert Microdesign area to portable bitmap
    Copyright (C) 1999 John Elliott <jce@seasip.demon.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    See the file mdaspec.txt for a specification of the MDA format.
******************************************************************************/

#include <string.h>
#include <stdio.h>
#include "pbm.h"
#include "mallocvar.h"

/* Simple MDA -> portable bitmap converter */

typedef unsigned char mdbyte;   /* Must be exactly one byte */

static FILE *infile;            /* Input file */
static mdbyte header[128];      /* MDA file header */
static bit **data;          /* PBM image */
static mdbyte *mdrow;           /* MDA row after decompression (MD3 only) */
static int bInvert = 0;     /* Invert image? */
static int bScale  = 0;     /* Scale image? */
static int bAscii  = 0;     /* Ouput ASCII PBM? */
static int nInRows, nInCols;        /* Height, width of input (rows x bytes) */
static int nOutCols, nOutRows;      /* Height, width of output (rows x bytes) */

static mdbyte 
getbyte(void) {
    /* Read a byte from the input stream, with error trapping */
    int b;

    b = fgetc(infile);

    if (b == EOF) pm_error("Unexpected end of MDA file\n");
    
    return (mdbyte)b;
}



static void 
render_byte(int *col, int *xp, int *yp, int b) {

/* Convert a byte to 8 cells in the destination bitmap 
 *
 * *col = source column
 * *xp  = destination column
 * *yp  = destination row
 *  b   = byte to draw
 *
 * Will update *col, *xp and *yp to point to the next bit of the row.
 */

    int mask = 0x80;
    int n;
    int y3 = *yp;

    if (bScale) y3 *= 2;

    if (y3 >= nOutRows) return;

    for (n = 0; n < 8; ++n) {
        if (bInvert) data[y3][*xp] = (b & mask) ? PBM_BLACK : PBM_WHITE;
        else         data[y3][*xp] = (b & mask) ? PBM_WHITE : PBM_BLACK;
        mask = mask >> 1;
        if (bScale) data[y3+1][*xp] = data[y3][*xp];
        ++(*xp);
    }
    ++(*col);       /* Next byte */
    if ((*col) >= nInCols) {
        /* Onto next line? */
        *col = 0;
        *xp = 0;
        ++(*yp);
    }
}


static void 
md2_trans(void) {
    /* Convert a MicroDesign 2 area to PBM */
    /* MD2 has RLE encoding that may go over */

    int x1, y1, col;    /* multiple lines. */
    mdbyte b;
    int c;

    x1 = y1 = col = 0;

    while (y1 < nInRows) {
        b = getbyte();
    
        if (b == 0 || b == 0xFF) {
            /* RLE sequence */
            c = getbyte();
            if (c == 0) c = 256;
            while (c > 0) { 
                render_byte(&col, &x1, &y1, b); 
                --c; 
            }
        }
        else 
            render_byte(&col, &x1, &y1, b);    /* Not RLE */
    }
}



static void 
md3_trans(void) {
    /* Convert MD3 file. MD3 are encoded as rows, and 
       there are three types. 
    */
    int x1, y1, col;
    mdbyte b;
    int c, d, n;

    for (y1 = 0; y1 < nInRows; ++y1) {
        b = getbyte();   /* Row type */
        switch(b)  {
        case 0: /* All the same byte */
            c = getbyte();
            for (n = 0; n < nInCols; n++) 
                mdrow[n] = c;
            break;
            
        case 1:      /* Encoded data */
        case 2: col = 0; /* Encoded as XOR with previous row */
            while (col < nInCols) {
                c = getbyte();
                if (c >= 129) {
                    /* RLE sequence */
                    c = 257 - c;
                    d = getbyte();
                    for (n = 0; n < c; ++n) {
                        if (b == 1) 
                            mdrow[col++] = d;
                        else 
                            mdrow[col++] ^= d;
                    }   
                } else {
                    /* not RLE sequence */
                        ++c;
                        for (n = 0; n < c; ++n) {
                            d = getbyte();
                            if (b == 1) 
                                mdrow[col++] = d;
                            else
                                mdrow[col++] ^= d;
                        }
                } 
            }
        }
        /* Row loaded. Convert it. */
        x1 = 0; col = 0; 
        for (n = 0; n < nInCols; ++n) {
            d  = y1;
            render_byte(&col, &x1, &d, mdrow[n]);
        }
    }
}



static void 
usage(char *s) {        
    printf("mdatopbm v1.00, Copyright (C) 1999 "
           "John Elliott <jce@seasip.demon.co.uk>\n"
           "This program is redistributable under the terms of "
           "the GNU General Public\n"
           "License, version 2 or later.\n\n"
           "Usage: %s [ -a ] [ -d ] [ -i ] [ -- ] [ infile ]\n\n"
           "-a: Output an ASCII pbm file\n"
           "-d: Double height (to compensate for the PCW aspect ratio)\n"
           "-i: Invert colors\n"
           "--: No more options (use if filename begins with a dash)\n",
           s);

    exit(0);
}



int 
main(int argc, char **argv) {
    int n, optstop = 0;
    char *fname = NULL;

    pbm_init(&argc, argv);

    /* Parse options */

    for (n = 1; n < argc; ++n) {
        if (argv[n][0] == '-' && !optstop) {   
            if (argv[n][1] == 'a' || argv[n][1] == 'A') bAscii = 1;
            if (argv[n][1] == 'd' || argv[n][1] == 'D') bScale = 1;
            if (argv[n][1] == 'i' || argv[n][1] == 'I') bInvert = 1;
            if (argv[n][1] == 'h' || argv[n][1] == 'H') usage(argv[0]);
            if (argv[n][1] == '-' && argv[n][2] == 0 && !fname) {
                /* "--" */
                optstop = 1;
            }
            if (argv[n][1] == '-' && (argv[n][2] == 'h' || argv[n][2] == 'H'))
                usage(argv[0]);
        }
        else if (argv[n][0] && !fname) {
            /* Filename */
            fname = argv[n];
        }
    }

    if (fname) 
        infile = pm_openr(fname);
    else
        infile = stdin;

    /* Read MDA file header */

    if (fread(header, 1, 128, infile) < 128)
        pm_error("Not a .MDA file\n");

    if (strncmp((char*) header, ".MDA", 4) && 
        strncmp((char*) header, ".MDP", 4))
        pm_error("Not a .MDA file\n");

    {
        short yy;
        pm_readlittleshort(infile, &yy); nInRows = yy;
        pm_readlittleshort(infile, &yy); nInCols = yy;
    }
    
    nOutCols = 8 * nInCols;
    nOutRows = nInRows;
    if (bScale) 
        nOutRows *= 2;

    data = pbm_allocarray(nOutCols, nOutRows);
    
    MALLOCARRAY_NOFAIL(mdrow, nInCols);

    if (header[21] == '0') 
        md2_trans();
    else
        md3_trans();

    pbm_writepbm(stdout, data, nInCols*8, nOutRows, bAscii);

    if (infile != stdin) 
        pm_close(infile);
    fflush(stdout);
    pbm_freearray(data, nOutRows);
    free(mdrow);

    return 0;
}
