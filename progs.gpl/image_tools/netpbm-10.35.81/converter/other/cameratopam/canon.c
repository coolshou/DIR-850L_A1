#include <string.h>
#include "mallocvar.h"
#include "pm.h"
#include "global_variables.h"
#include "util.h"
#include "decode.h"
#include "bayer.h"
#include "canon.h"


void 
canon_600_load_raw(void) {
    unsigned char  data[1120], *dp;
    unsigned short pixel[896], *pix;
    int irow, orow, col;

    for (irow=orow=0; irow < height; irow++)
    {
        fread (data, 1120, 1, ifp);
        for (dp=data, pix=pixel; dp < data+1120; dp+=10, pix+=8)
        {
            pix[0] = (dp[0] << 2) + (dp[1] >> 6    );
            pix[1] = (dp[2] << 2) + (dp[1] >> 4 & 3);
            pix[2] = (dp[3] << 2) + (dp[1] >> 2 & 3);
            pix[3] = (dp[4] << 2) + (dp[1]      & 3);
            pix[4] = (dp[5] << 2) + (dp[9]      & 3);
            pix[5] = (dp[6] << 2) + (dp[9] >> 2 & 3);
            pix[6] = (dp[7] << 2) + (dp[9] >> 4 & 3);
            pix[7] = (dp[8] << 2) + (dp[9] >> 6    );
        }
        for (col=0; col < width; col++)
            BAYER(orow,col) = pixel[col];
        for (col=width; col < 896; col++)
            black += pixel[col];
        if ((orow+=2) > height)
            orow = 1;
    }
    black /= (896 - width) * height;
    maximum = 0x3ff;
}



void
canon_a5_load_raw(void) {
    unsigned char  data[1940], *dp;
    unsigned short pixel[1552], *pix;
    int row, col;

    for (row=0; row < height; row++) {
        fread (data, raw_width * 10 / 8, 1, ifp);
        for (dp=data, pix=pixel; pix < pixel+raw_width; dp+=10, pix+=8)
        {
            pix[0] = (dp[1] << 2) + (dp[0] >> 6);
            pix[1] = (dp[0] << 4) + (dp[3] >> 4);
            pix[2] = (dp[3] << 6) + (dp[2] >> 2);
            pix[3] = (dp[2] << 8) + (dp[5]     );
            pix[4] = (dp[4] << 2) + (dp[7] >> 6);
            pix[5] = (dp[7] << 4) + (dp[6] >> 4);
            pix[6] = (dp[6] << 6) + (dp[9] >> 2);
            pix[7] = (dp[9] << 8) + (dp[8]     );
        }
        for (col=0; col < width; col++)
            BAYER(row,col) = (pixel[col] & 0x3ff);
        for (col=width; col < raw_width; col++)
            black += pixel[col] & 0x3ff;
    }
    if (raw_width > width)
        black /= (raw_width - width) * height;
    maximum = 0x3ff;
}



/*
   Return 0 if the image starts with compressed data,
   1 if it starts with uncompressed low-order bits.

   In Canon compressed data, 0xff is always followed by 0x00.
 */
static int
canon_has_lowbits()
{
    unsigned char test[0x4000];
    int ret=1, i;

    fseek (ifp, 0, SEEK_SET);
    fread (test, 1, sizeof test, ifp);
    for (i=540; i < sizeof test - 1; i++)
        if (test[i] == 0xff) {
            if (test[i+1]) return 1;
            ret=0;
        }
    return ret;
}



void 
canon_compressed_load_raw(void) {
    unsigned short *pixel, *prow;
    int lowbits, i, row, r, col, save, val;
    unsigned irow, icol;
    struct decode *decode, *dindex;
    int block, diffbuf[64], leaf, len, diff, carry=0, pnum=0, base[2];
    unsigned char c;

    MALLOCARRAY(pixel, raw_width*8);
    if (pixel == NULL)
        pm_error("Unable to allocate space for %u pixels", raw_width*8);
    lowbits = canon_has_lowbits();
    if (!lowbits) maximum = 0x3ff;
    fseek (ifp, 540 + lowbits*raw_height*raw_width/4, SEEK_SET);
    zero_after_ff = 1;
    getbits(ifp, -1);
    for (row = 0; row < raw_height; row += 8) {
        for (block=0; block < raw_width >> 3; block++) {
            memset (diffbuf, 0, sizeof diffbuf);
            decode = first_decode;
            for (i=0; i < 64; i++ ) {
                for (dindex=decode; dindex->branch[0]; )
                    dindex = dindex->branch[getbits(ifp, 1)];
                leaf = dindex->leaf;
                decode = second_decode;
                if (leaf == 0 && i) break;
                if (leaf == 0xff) continue;
                i  += leaf >> 4;
                len = leaf & 15;
                if (len == 0) continue;
                diff = getbits(ifp, len);
                if ((diff & (1 << (len-1))) == 0)
                    diff -= (1 << len) - 1;
                if (i < 64) diffbuf[i] = diff;
            }
            diffbuf[0] += carry;
            carry = diffbuf[0];
            for (i=0; i < 64; i++ ) {
                if (pnum++ % raw_width == 0)
                    base[0] = base[1] = 512;
                pixel[(block << 6) + i] = ( base[i & 1] += diffbuf[i] );
            }
        }
        if (lowbits) {
            save = ftell(ifp);
            fseek (ifp, 26 + row*raw_width/4, SEEK_SET);
            for (prow=pixel, i=0; i < raw_width*2; i++) {
                c = fgetc(ifp);
                for (r=0; r < 8; r+=2, prow++) {
                    val = (*prow << 2) + ((c >> r) & 3);
                    if (raw_width == 2672 && val < 512) val += 2;
                    *prow = val;
                }
            }
            fseek (ifp, save, SEEK_SET);
        }
        for (r=0; r < 8; r++) {
            irow = row - top_margin + r;
            if (irow >= height) continue;
            for (col = 0; col < raw_width; col++) {
                icol = col - left_margin;
                if (icol < width)
                    BAYER(irow,icol) = pixel[r*raw_width+col];
                else
                    black += pixel[r*raw_width+col];
            }
        }
    }
    free (pixel);
    if (raw_width > width)
        black /= (raw_width - width) * height;
}

