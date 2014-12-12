
/***************************************************************************

    PBMTOMDA: Convert portable bitmap to Microdesign area
    Copyright (C) 1999,2004 John Elliott <jce@seasip.demon.co.uk>

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

******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "pbm.h"
#include "mallocvar.h"

/* I'm being somewhat conservative in the PBM -> MDA translation. I output 
 * only the MD2 format and don't allow RLE over the ends of lines.
 */

typedef unsigned char mdbyte;

static FILE *infile;
static mdbyte header[128];
static int bInvert = 0;
static int bScale  = 0;

/* Encode 8 pixels as a byte */

static mdbyte 
encode(bit ** const bits, int const row, int const col)
{
    int n;
    int mask;
    mdbyte b;

    mask = 0x80;   /* initial value */
    b = 0;  /* initial value */

    for (n = 0; n < 8; n++) {
        if (bits[row][col+n] == PBM_BLACK) b |= mask;
        mask = mask >> 1;
    }
    return b;
}

/* Translate a pbm to MD2 format, one row at a time */

static void 
do_translation(bit ** const bits, 
               int    const nOutCols, 
               int    const nOutRows,
               int    const nInRows)
{
    int row;
    mdbyte *mdrow;  /* malloc'ed */

    int const step = bScale ? 2 : 1;

    MALLOCARRAY(mdrow, nOutCols);

    if (mdrow == NULL)
        pm_error("Not enough memory for conversion.");

    for (row = 0; row < nOutRows; row+=step)
    {
        int col;
        int x1;

        /* Encode image into non-compressed bitmap */
        for (col = 0; col < nOutCols; ++col) {
            mdbyte b;

            if (row < nInRows)
                b = encode(bits, row, col*8);
            else
                b = 0xff;  /* All black */

            mdrow[col] = bInvert ? b : ~b;
        }

        /* Encoded. Now RLE it */
        for (col = 0; col < nOutCols; )
        {
            mdbyte const b = mdrow[col];

            if (b != 0xFF && b != 0) /* Normal byte */
            {
                putchar(b);
                ++col;
            }
            else    /* RLE a run of 0s or 0xFFs */
            {
                for (x1 = col; x1 < nOutCols; x1++)
                {
                    if (mdrow[x1] != b) break;
                    if (x1 - col > 256) break;
                }
                x1 -= col;    /* x1 = no. of repeats */
                if (x1 == 256) x1 = 0;
                putchar(b);
                putchar(x1);
                col += x1;        
            }   
        }
    }
    free(mdrow);
}


static void usage(char *s)
{        
    printf("pbmtomda v1.01, Copyright (C) 1999,2004 John Elliott <jce@seasip.demon.co.uk>\n"
         "This program is redistributable under the terms of the GNU General Public\n"
                 "License, version 2 or later.\n\n"
                 "Usage: %s [ -d ] [ -i ] [ -- ] [ infile ]\n\n"
                 "-d: Halve height (to compensate for the PCW aspect ratio)\n"
                 "-i: Invert colors\n"
                 "--: No more options (use if filename begins with a dash)\n",
        s);

    exit(0);
}

int main(int argc, char **argv)
{
    int nOutRowsUnrounded;  /* Before rounding up to multiple of 4 */
    int nOutCols, nOutRows;
    int nInCols, nInRows;
    bit **bits;
    int rc;

    int n, optstop = 0;
    char *fname = NULL;

    pbm_init(&argc, argv);

    /* Output v2-format MDA images. Simulate MDA header...
     * 2004-01-11: Hmm. Apparently some (but not all) MDA-reading 
     * programs insist on the program identifier being exactly 
     * 'MicroDesignPCW'. The spec does not make this clear. */
    strcpy((char*) header, ".MDAMicroDesignPCWv1.00\r\npbm2mda\r\n");

    for (n = 1; n < argc; n++)
    {
        if (argv[n][0] == '-' && !optstop)
        {   
            if (argv[n][1] == 'd' || argv[n][1] == 'D') bScale = 1;
            if (argv[n][1] == 'i' || argv[n][1] == 'I') bInvert = 1;
            if (argv[n][1] == 'h' || argv[n][1] == 'H') usage(argv[0]);
            if (argv[n][1] == '-' && argv[n][2] == 0 && !fname)     /* "--" */
            {
                optstop = 1;
            }
            if (argv[n][1] == '-' && (argv[n][2] == 'h' || argv[n][2] == 'H')) usage(argv[0]);
        }
        else if (argv[n][0] && !fname)  /* Filename */
        {
            fname = argv[n];
        }
    }

    if (fname) infile = pm_openr(fname);
    else       infile = stdin;

    bits = pbm_readpbm(infile, &nInCols, &nInRows);
    
    nOutRowsUnrounded = bScale ? nInRows/2 : nInRows;

    nOutRows = ((nOutRowsUnrounded + 3) / 4) * 4;
        /* MDA wants rows a multiple of 4 */   
    nOutCols = nInCols / 8;

    rc = fwrite(header, 1, 128, stdout);
    if (rc < 128)
        pm_error("Unable to write header to output file.  errno=%d (%s)",
                 errno, strerror(errno));

    pm_writelittleshort(stdout, nOutRows);
    pm_writelittleshort(stdout, nOutCols);

    do_translation(bits, nOutCols, nOutRows, nInRows);

    pm_close(infile);
    fflush(stdout);
    pbm_freearray(bits, nInRows);
    
    return 0;
}
