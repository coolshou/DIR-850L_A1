/* This code is licensed to the public by its copyright owners under GPL. */

#define _XOPEN_SOURCE   /* get M_PI */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "mallocvar.h"
#include "pm.h"
#include "global_variables.h"
#include "decode.h"
#include "foveon.h"

#if HAVE_INT64
   typedef int64_t INT64;
   static bool const have64BitArithmetic = true;
#else
   /* We define INT64 to something that lets the code compile, but we
      should not execute any INT64 code, because it will get the wrong
      result.
   */
   typedef int INT64;
   static bool const have64BitArithmetic = false;
#endif


#define FORC3 for (c=0; c < 3; c++)
#define FORC4 for (c=0; c < colors; c++)



static char *  
foveon_gets(int    const offset, 
            char * const str, 
            int    const len) {

    unsigned int i;
    fseek (ifp, offset, SEEK_SET);
    for (i=0; i < len-1; ++i) {
        /* It certains seems wrong that we're reading a 16 bit integer
           and assigning it to char, but that's what Dcraw does.
        */
        unsigned short val;
        pm_readlittleshortu(ifp, &val);
        str[i] = val;
        if (str[i] == 0)
            break;
    }
    str[i] = 0;
    return str;
}



void 
parse_foveon(FILE * const ifp) {
    long fliplong;
    long pos;
    long magic;
    long junk;
    long entries;

    fseek (ifp, 36, SEEK_SET);
    pm_readlittlelong(ifp, &fliplong);
    flip = fliplong;
    fseek (ifp, -4, SEEK_END);
    pm_readlittlelong(ifp, &pos);
    fseek (ifp, pos, SEEK_SET);
    pm_readlittlelong(ifp, &magic);
    if (magic != 0x64434553) 
        return; /* SECd */
    pm_readlittlelong(ifp, &junk);
    pm_readlittlelong(ifp, &entries);
    while (entries--) {
        long off, len, tag;
        long save;
        long pent;
        int i, poff[256][2];
        char name[64];
        long sec_;

        pm_readlittlelong(ifp, &off);
        pm_readlittlelong(ifp, &len);
        pm_readlittlelong(ifp, &tag);
            
        save = ftell(ifp);
        fseek (ifp, off, SEEK_SET);
        pm_readlittlelong(ifp, &sec_);
        if (sec_ != (0x20434553 | (tag << 24))) return;
        switch (tag) {
        case 0x47414d49:          /* IMAG */
            if (data_offset) 
                break;
            data_offset = off + 28;
            fseek (ifp, 12, SEEK_CUR);
            {
                long wlong, hlong;
                pm_readlittlelong(ifp, &wlong);
                pm_readlittlelong(ifp, &hlong);
                raw_width  = wlong;
                raw_height = hlong;
            }
            break;
        case 0x464d4143:          /* CAMF */
            meta_offset = off + 24;
            meta_length = len - 28;
            if (meta_length > 0x20000)
                meta_length = 0x20000;
            break;
        case 0x504f5250:          /* PROP */
            pm_readlittlelong(ifp, &junk);
            pm_readlittlelong(ifp, &pent);
            fseek (ifp, 12, SEEK_CUR);
            off += pent*8 + 24;
            if (pent > 256) pent=256;
            for (i=0; i < pent*2; i++) {
                long x;
                pm_readlittlelong(ifp, &x);
                poff[0][i] = off + x*2;
            }
            for (i=0; i < pent; i++) {
                foveon_gets (poff[i][0], name, 64);
                if (!strcmp (name, "CAMMANUF"))
                    foveon_gets (poff[i][1], make, 64);
                if (!strcmp (name, "CAMMODEL"))
                    foveon_gets (poff[i][1], model, 64);
                if (!strcmp (name, "WB_DESC"))
                    foveon_gets (poff[i][1], model2, 64);
                if (!strcmp (name, "TIME"))
                    timestamp = atoi (foveon_gets (poff[i][1], name, 64));
            }
        }
        fseek (ifp, save, SEEK_SET);
    }
    is_foveon = 1;
}



void  
foveon_coeff(bool * const useCoeffP,
             float        coeff[3][4]) {

    static const float foveon[3][3] =
    { {  1.4032, -0.2231, -0.1016 },
      { -0.5263,  1.4816,  0.0170 },
      { -0.0112,  0.0183,  0.9113 } };

    int i, j;

    for (i=0; i < 3; ++i)
        for (j=0; j < 3; ++j)
            coeff[i][j] = foveon[i][j];
    *useCoeffP = 1;
}



static void  
foveon_decoder (unsigned int const huff[1024], 
                unsigned int const code) {

    struct decode *cur;
    int len;
    unsigned int code2;

    cur = free_decode++;
    if (free_decode > first_decode+2048) {
        pm_error ("decoder table overflow");
    }
    if (code) {
        unsigned int i;
        for (i=0; i < 1024; i++) {
            if (huff[i] == code) {
                cur->leaf = i;
                return;
            }
        }
    }
    if ((len = code >> 27) > 26) 
        return;
    code2 = (len+1) << 27 | (code & 0x3ffffff) << 1;

    cur->branch[0] = free_decode;
    foveon_decoder (huff, code2);
    cur->branch[1] = free_decode;
    foveon_decoder (huff, code2+1);
}



static void  
foveon_load_camf() {
    unsigned int i, val;
    long key;

    fseek (ifp, meta_offset, SEEK_SET);
    pm_readlittlelong(ifp, &key);
    fread (meta_data, 1, meta_length, ifp);
    for (i=0; i < meta_length; i++) {
        key = (key * 1597 + 51749) % 244944;
        assert(have64BitArithmetic);
        val = key * (INT64) 301593171 >> 24;
        meta_data[i] ^= ((((key << 8) - val) >> 1) + val) >> 17;
    }
}



void  
foveon_load_raw() {

    struct decode *dindex;
    short diff[1024], pred[3];
    unsigned int huff[1024], bitbuf=0;
    int row, col, bit=-1, c, i;

    assert(have64BitArithmetic);

    for (i=0; i < 1024; ++i)
        pm_readlittleshort(ifp, &diff[i]);

    for (i=0; i < 1024; ++i) {
        long x;
        pm_readlittlelong(ifp, &x);
        huff[i] = x;
    }
    init_decoder();
    foveon_decoder (huff, 0);

    for (row=0; row < height; row++) {
        long junk;
        memset (pred, 0, sizeof pred);
        if (!bit) 
            pm_readlittlelong(ifp, &junk);
        for (col=bit=0; col < width; col++) {
            FORC3 {
                for (dindex=first_decode; dindex->branch[0]; ) {
                    if ((bit = (bit-1) & 31) == 31)
                        for (i=0; i < 4; i++)
                            bitbuf = (bitbuf << 8) + fgetc(ifp);
                    dindex = dindex->branch[bitbuf >> bit & 1];
                }
                pred[c] += diff[dindex->leaf];
            }
            FORC3 image[row*width+col][c] = pred[c];
        }
    }
    foveon_load_camf();
    maximum = clip_max = 0xffff;
}



static int
sget4(char const s[]) {
    return s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
}



static char *  
foveon_camf_param (const char * const block, 
                   const char * const param) {
    unsigned idx, num;
    char *pos, *cp, *dp;

    for (idx=0; idx < meta_length; idx += sget4(pos+8)) {
        pos = meta_data + idx;
        if (strncmp (pos, "CMb", 3)) break;
        if (pos[3] != 'P') continue;
        if (strcmp (block, pos+sget4(pos+12))) continue;
        cp = pos + sget4(pos+16);
        num = sget4(cp);
        dp = pos + sget4(cp+4);
        while (num--) {
            cp += 8;
            if (!strcmp (param, dp+sget4(cp)))
                return dp+sget4(cp+4);
        }
    }
    return NULL;
}



static void *  
foveon_camf_matrix (int                dim[3], 
                    const char * const name) {

    unsigned i, idx, type, ndim, size, *mat;
    char *pos, *cp, *dp;

    for (idx=0; idx < meta_length; idx += sget4(pos+8)) {
        pos = meta_data + idx;
        if (strncmp (pos, "CMb", 3)) break;
        if (pos[3] != 'M') continue;
        if (strcmp (name, pos+sget4(pos+12))) continue;
        dim[0] = dim[1] = dim[2] = 1;
        cp = pos + sget4(pos+16);
        type = sget4(cp);
        if ((ndim = sget4(cp+4)) > 3) break;
        dp = pos + sget4(cp+8);
        for (i=ndim; i--; ) {
            cp += 12;
            dim[i] = sget4(cp);
        }
        if ((size = dim[0]*dim[1]*dim[2]) > meta_length/4) break;
        MALLOCARRAY(mat, size);
        if (mat == NULL)
            pm_error("Unable to allocate memory for size=%d", size);
        for (i=0; i < size; i++)
            if (type && type != 6)
                mat[i] = sget4(dp + i*4);
            else
                mat[i] = sget4(dp + i*2) & 0xffff;
        return mat;
    }
    pm_message ("'%s' matrix not found!", name);
    return NULL;
}



static int  
foveon_fixed (void *       const ptr, 
              int          const size, 
              const char * const name) {
    void *dp;
    int dim[3];

    dp = foveon_camf_matrix (dim, name);
    if (!dp) return 0;
    memcpy (ptr, dp, size*4);
    free (dp);
    return 1;
}

static float  foveon_avg (unsigned short *pix, int range[2], float cfilt)
{
    int i;
    float val, min=FLT_MAX, max=-FLT_MAX, sum=0;

    for (i=range[0]; i <= range[1]; i++) {
        sum += val = 
            (short)pix[i*4] + ((short)pix[i*4]-(short)pix[(i-1)*4]) * cfilt;
        if (min > val) min = val;
        if (max < val) max = val;
    }
    return (sum - min - max) / (range[1] - range[0] - 1);
}

static short *foveon_make_curve (double max, double mul, double filt)
{
    short *curve;
    int size;
    unsigned int i;
    double x;

    size = 4*M_PI*max / filt;
    MALLOCARRAY(curve, size+1);
    if (curve == NULL)
        pm_error("Out of memory for %d-element curve array", size);

    curve[0] = size;
    for (i=0; i < size; ++i) {
        x = i*filt/max/4;
        curve[i+1] = (cos(x)+1)/2 * tanh(i*filt/mul) * mul + 0.5;
    }
    return curve;
}

static void foveon_make_curves
(short **curvep, float dq[3], float div[3], float filt)
{
    double mul[3], max=0;
    int c;

    FORC3 mul[c] = dq[c]/div[c];
    FORC3 if (max < mul[c]) max = mul[c];
    FORC3 curvep[c] = foveon_make_curve (max, mul[c], filt);
}

static int  foveon_apply_curve (short *curve, int i)
{
    if (abs(i) >= (unsigned short)curve[0]) return 0;
    return i < 0 ? -(unsigned short)curve[1-i] : (unsigned short)curve[1+i];
}

void  
foveon_interpolate(float coeff[3][4]) {

    static const short hood[] = { 
        -1,-1, -1,0, -1,1, 0,-1, 0,1, 1,-1, 1,0, 1,1 };
    short *pix, prev[3], *curve[8], (*shrink)[3];
    float cfilt=0.8, ddft[3][3][2], ppm[3][3][3];
    float cam_xyz[3][3], correct[3][3], last[3][3], trans[3][3];
    float chroma_dq[3], color_dq[3], diag[3][3], div[3];
    float (*black)[3], (*sgain)[3], (*sgrow)[3];
    float fsum[3], val, frow, num;
    int row, col, c, i, j, diff, sgx, irow, sum, min, max, limit;
    int dim[3], dscr[2][2], (*smrow[7])[3], total[4], ipix[3];
    int work[3][3], smlast, smred, smred_p=0, dev[3];
    int satlev[3], keep[4], active[4];
    unsigned *badpix;
    double dsum=0, trsum[3];
    char str[128], *cp;

    foveon_fixed (dscr, 4, "DarkShieldColRange");
    foveon_fixed (ppm[0][0], 27, "PostPolyMatrix");
    foveon_fixed (ddft[1][0], 12, "DarkDrift");
    foveon_fixed (&cfilt, 1, "ColumnFilter");
    foveon_fixed (satlev, 3, "SaturationLevel");
    foveon_fixed (keep, 4, "KeepImageArea");
    foveon_fixed (active, 4, "ActiveImageArea");
    foveon_fixed (chroma_dq, 3, "ChromaDQ");
    foveon_fixed (color_dq, 3,
                  foveon_camf_param ("IncludeBlocks", "ColorDQ") ?
                  "ColorDQ" : "ColorDQCamRGB");

    if (!(cp = foveon_camf_param ("WhiteBalanceIlluminants", model2)))
    { pm_message ( "Invalid white balance \"%s\"", model2);
    return; }
    foveon_fixed (cam_xyz, 9, cp);
    foveon_fixed (correct, 9,
                  foveon_camf_param ("WhiteBalanceCorrections", model2));
    memset (last, 0, sizeof last);
    for (i=0; i < 3; i++)
        for (j=0; j < 3; j++)
            FORC3 last[i][j] += correct[i][c] * cam_xyz[c][j];

    sprintf (str, "%sRGBNeutral", model2);
    if (foveon_camf_param ("IncludeBlocks", str))
        foveon_fixed (div, 3, str);
    else {
#define LAST(x,y) last[(i+x)%3][(c+y)%3]
        for (i=0; i < 3; i++)
            FORC3 diag[c][i] = LAST(1,1)*LAST(2,2) - LAST(1,2)*LAST(2,1);
#undef LAST
        FORC3 div[c] = 
            diag[c][0]*0.3127 + diag[c][1]*0.329 + diag[c][2]*0.3583;
    }
    num = 0;
    FORC3 if (num < div[c]) num = div[c];
    FORC3 div[c] /= num;

    memset (trans, 0, sizeof trans);
    for (i=0; i < 3; i++)
        for (j=0; j < 3; j++)
            FORC3 trans[i][j] += coeff[i][c] * last[c][j] * div[j];
    FORC3 trsum[c] = trans[c][0] + trans[c][1] + trans[c][2];
    dsum = (6*trsum[0] + 11*trsum[1] + 3*trsum[2]) / 20;
    for (i=0; i < 3; i++)
        FORC3 last[i][c] = trans[i][c] * dsum / trsum[i];
    memset (trans, 0, sizeof trans);
    for (i=0; i < 3; i++)
        for (j=0; j < 3; j++)
            FORC3 trans[i][j] += (i==c ? 32 : -1) * last[c][j] / 30;

    foveon_make_curves (curve, color_dq, div, cfilt);
    FORC3 chroma_dq[c] /= 3;
    foveon_make_curves (curve+3, chroma_dq, div, cfilt);
    FORC3 dsum += chroma_dq[c] / div[c];
    curve[6] = foveon_make_curve (dsum, dsum, cfilt);
    curve[7] = foveon_make_curve (dsum*2, dsum*2, cfilt);

    sgain = foveon_camf_matrix (dim, "SpatialGain");
    if (!sgain) return;
    sgrow = calloc (dim[1], sizeof *sgrow);
    sgx = (width + dim[1]-2) / (dim[1]-1);

    black = calloc (height, sizeof *black);
    for (row=0; row < height; row++) {
        for (i=0; i < 6; i++)
            ddft[0][0][i] = ddft[1][0][i] +
                row / (height-1.0) * (ddft[2][0][i] - ddft[1][0][i]);
        FORC3 black[row][c] =
            ( foveon_avg (image[row*width]+c, dscr[0], cfilt) +
              foveon_avg (image[row*width]+c, dscr[1], cfilt) * 3
              - ddft[0][c][0] ) / 4 - ddft[0][c][1];
    }
    memcpy (black, black+8, sizeof *black*8);
    memcpy (black+height-11, black+height-22, 11*sizeof *black);
    memcpy (last, black, sizeof last);

    for (row=1; row < height-1; row++) {
        FORC3 if (last[1][c] > last[0][c]) {
            if (last[1][c] > last[2][c])
                black[row][c] = 
                    (last[0][c] > last[2][c]) ? last[0][c]:last[2][c];
        } else
            if (last[1][c] < last[2][c])
                black[row][c] = 
                    (last[0][c] < last[2][c]) ? last[0][c]:last[2][c];
        memmove (last, last+1, 2*sizeof last[0]);
        memcpy (last[2], black[row+1], sizeof last[2]);
    }
    FORC3 black[row][c] = (last[0][c] + last[1][c])/2;
    FORC3 black[0][c] = (black[1][c] + black[3][c])/2;

    val = 1 - exp(-1/24.0);
    memcpy (fsum, black, sizeof fsum);
    for (row=1; row < height; row++)
        FORC3 fsum[c] += black[row][c] =
            (black[row][c] - black[row-1][c])*val + black[row-1][c];
    memcpy (last[0], black[height-1], sizeof last[0]);
    FORC3 fsum[c] /= height;
    for (row = height; row--; )
        FORC3 last[0][c] = black[row][c] =
            (black[row][c] - fsum[c] - last[0][c])*val + last[0][c];

    memset (total, 0, sizeof total);
    for (row=2; row < height; row+=4)
        for (col=2; col < width; col+=4) {
            FORC3 total[c] += (short) image[row*width+col][c];
            total[3]++;
        }
    for (row=0; row < height; row++)
        FORC3 black[row][c] += fsum[c]/2 + total[c]/(total[3]*100.0);

    for (row=0; row < height; row++) {
        for (i=0; i < 6; i++)
            ddft[0][0][i] = ddft[1][0][i] +
                row / (height-1.0) * (ddft[2][0][i] - ddft[1][0][i]);
        pix = (short*)image[row*width];
        memcpy (prev, pix, sizeof prev);
        frow = row / (height-1.0) * (dim[2]-1);
        if ((irow = frow) == dim[2]-1) irow--;
        frow -= irow;
        for (i=0; i < dim[1]; i++)
            FORC3 sgrow[i][c] = sgain[ irow   *dim[1]+i][c] * (1-frow) +
                sgain[(irow+1)*dim[1]+i][c] *    frow;
        for (col=0; col < width; col++) {
            FORC3 {
                diff = pix[c] - prev[c];
                prev[c] = pix[c];
                ipix[c] = pix[c] + floor ((diff + (diff*diff >> 14)) * cfilt
                                          - ddft[0][c][1] 
                                          - ddft[0][c][0] 
                                            * ((float) col/width - 0.5)
                                          - black[row][c] );
            }
            FORC3 {
                work[0][c] = ipix[c] * ipix[c] >> 14;
                work[2][c] = ipix[c] * work[0][c] >> 14;
                work[1][2-c] = ipix[(c+1) % 3] * ipix[(c+2) % 3] >> 14;
            }
            FORC3 {
                for (val=i=0; i < 3; i++)
                    for (  j=0; j < 3; j++)
                        val += ppm[c][i][j] * work[i][j];
                ipix[c] = floor ((ipix[c] + floor(val)) *
                                 ( sgrow[col/sgx  ][c] * (sgx - col%sgx) +
                                   sgrow[col/sgx+1][c] * (col%sgx) ) / sgx / 
                                 div[c]);
                if (ipix[c] > 32000) ipix[c] = 32000;
                pix[c] = ipix[c];
            }
            pix += 4;
        }
    }
    free (black);
    free (sgrow);
    free (sgain);

    if ((badpix = foveon_camf_matrix (dim, "BadPixels"))) {
        for (i=0; i < dim[0]; i++) {
            col = (badpix[i] >> 8 & 0xfff) - keep[0];
            row = (badpix[i] >> 20       ) - keep[1];
            if ((unsigned)(row-1) > height-3 || (unsigned)(col-1) > width-3)
                continue;
            memset (fsum, 0, sizeof fsum);
            for (sum=j=0; j < 8; j++)
                if (badpix[i] & (1 << j)) {
                    FORC3 fsum[c] += 
                        image[(row+hood[j*2])*width+col+hood[j*2+1]][c];
                    sum++;
                }
            if (sum) FORC3 image[row*width+col][c] = fsum[c]/sum;
        }
        free (badpix);
    }

    /* Array for 5x5 Gaussian averaging of red values */
    smrow[6] = calloc (width*5, sizeof **smrow);
    if (smrow[6] == NULL)
        pm_error("Out of memory");
    for (i=0; i < 5; i++)
        smrow[i] = smrow[6] + i*width;

    /* Sharpen the reds against these Gaussian averages */
    for (smlast=-1, row=2; row < height-2; row++) {
        while (smlast < row+2) {
            for (i=0; i < 6; i++)
                smrow[(i+5) % 6] = smrow[i];
            pix = (short*)image[++smlast*width+2];
            for (col=2; col < width-2; col++) {
                smrow[4][col][0] =
                    (pix[0]*6 + (pix[-4]+pix[4])*4 + pix[-8]+pix[8] + 8) >> 4;
                pix += 4;
            }
        }
        pix = (short*)image[row*width+2];
        for (col=2; col < width-2; col++) {
            smred = ( 6 *  smrow[2][col][0]
                      + 4 * (smrow[1][col][0] + smrow[3][col][0])
                      +      smrow[0][col][0] + smrow[4][col][0] + 8 ) >> 4;
            if (col == 2)
                smred_p = smred;
            i = pix[0] + ((pix[0] - ((smred*7 + smred_p) >> 3)) >> 3);
            if (i > 32000) i = 32000;
            pix[0] = i;
            smred_p = smred;
            pix += 4;
        }
    }

    /* Adjust the brighter pixels for better linearity */
    FORC3 {
        i = satlev[c] / div[c];
        if (maximum > i) maximum = i;
    }
    clip_max = maximum;
    limit = maximum * 9 >> 4;
    for (pix=(short*)image[0]; pix < (short *) image[height*width]; pix+=4) {
        if (pix[0] <= limit || pix[1] <= limit || pix[2] <= limit)
            continue;
        min = max = pix[0];
        for (c=1; c < 3; c++) {
            if (min > pix[c]) min = pix[c];
            if (max < pix[c]) max = pix[c];
        }
        i = 0x4000 - ((min - limit) << 14) / limit;
        i = 0x4000 - (i*i >> 14);
        i = i*i >> 14;
        FORC3 pix[c] += (max - pix[c]) * i >> 14;
    }
    /*
      Because photons that miss one detector often hit another,
      the sum R+G+B is much less noisy than the individual colors.
      So smooth the hues without smoothing the total.
    */
    for (smlast=-1, row=2; row < height-2; row++) {
        while (smlast < row+2) {
            for (i=0; i < 6; i++)
                smrow[(i+5) % 6] = smrow[i];
            pix = (short*)image[++smlast*width+2];
            for (col=2; col < width-2; col++) {
                FORC3 smrow[4][col][c] = (pix[c-4]+2*pix[c]+pix[c+4]+2) >> 2;
                pix += 4;
            }
        }
        pix = (short*)image[row*width+2];
        for (col=2; col < width-2; col++) {
            FORC3 dev[c] = -foveon_apply_curve (curve[7], pix[c] -
                                                ((smrow[1][col][c] + 
                                                  2*smrow[2][col][c] + 
                                                  smrow[3][col][c]) >> 2));
            sum = (dev[0] + dev[1] + dev[2]) >> 3;
            FORC3 pix[c] += dev[c] - sum;
            pix += 4;
        }
    }
    for (smlast=-1, row=2; row < height-2; row++) {
        while (smlast < row+2) {
            for (i=0; i < 6; i++)
                smrow[(i+5) % 6] = smrow[i];
            pix = (short*)image[++smlast*width+2];
            for (col=2; col < width-2; col++) {
                FORC3 smrow[4][col][c] =
                    (pix[c-8]+pix[c-4]+pix[c]+pix[c+4]+pix[c+8]+2) >> 2;
                pix += 4;
            }
        }
        pix = (short*)image[row*width+2];
        for (col=2; col < width-2; col++) {
            for (total[3]=375, sum=60, c=0; c < 3; c++) {
                for (total[c]=i=0; i < 5; i++)
                    total[c] += smrow[i][col][c];
                total[3] += total[c];
                sum += pix[c];
            }
            if (sum < 0) sum = 0;
            j = total[3] > 375 ? (sum << 16) / total[3] : sum * 174;
            FORC3 pix[c] += foveon_apply_curve (curve[6],
                                                ((j*total[c] + 0x8000) >> 16) 
                                                - pix[c]);
            pix += 4;
        }
    }

    /* Transform the image to a different colorspace */
    for (pix=(short*)image[0]; pix < (short *) image[height*width]; pix+=4) {
        FORC3 pix[c] -= foveon_apply_curve (curve[c], pix[c]);
        sum = (pix[0]+pix[1]+pix[1]+pix[2]) >> 2;
        FORC3 pix[c] -= foveon_apply_curve (curve[c], pix[c]-sum);
        FORC3 {
            for (dsum=i=0; i < 3; i++)
                dsum += trans[c][i] * pix[i];
            if (dsum < 0)  dsum = 0;
            if (dsum > 24000) dsum = 24000;
            ipix[c] = dsum + 0.5;
        }
        FORC3 pix[c] = ipix[c];
    }

    /* Smooth the image bottom-to-top and save at 1/4 scale */
    MALLOCARRAY(shrink, (width/4) * (height/4));
    if (shrink == NULL)
        pm_error("Out of memory allocating 1/4 scale array");

    for (row = height/4; row > 0; --row) {
        for (col=0; col < width/4; ++col) {
            ipix[0] = ipix[1] = ipix[2] = 0;
            for (i=0; i < 4; i++)
                for (j=0; j < 4; j++)
                    FORC3 ipix[c] += image[(row*4+i)*width+col*4+j][c];
            FORC3
                if (row+2 > height/4)
                    shrink[row*(width/4)+col][c] = ipix[c] >> 4;
                else
                    shrink[row*(width/4)+col][c] =
                        (shrink[(row+1)*(width/4)+col][c]*1840 + 
                         ipix[c]*141 + 2048) >> 12;
        }
    }
    /* From the 1/4-scale image, smooth right-to-left */
    for (row=0; row < (height & ~3); ++row) {
        ipix[0] = ipix[1] = ipix[2] = 0;
        if ((row & 3) == 0)
            for (col = width & ~3 ; col--; )
                FORC3 smrow[0][col][c] = ipix[c] =
                    (shrink[(row/4)*(width/4)+col/4][c]*1485 + 
                     ipix[c]*6707 + 4096) >> 13;

        /* Then smooth left-to-right */
        ipix[0] = ipix[1] = ipix[2] = 0;
        for (col=0; col < (width & ~3); col++)
            FORC3 smrow[1][col][c] = ipix[c] =
                (smrow[0][col][c]*1485 + ipix[c]*6707 + 4096) >> 13;

        /* Smooth top-to-bottom */
        if (row == 0)
            memcpy (smrow[2], smrow[1], sizeof **smrow * width);
        else
            for (col=0; col < (width & ~3); col++)
                FORC3 smrow[2][col][c] =
                    (smrow[2][col][c]*6707 + smrow[1][col][c]*1485 + 4096) 
                        >> 13;

        /* Adjust the chroma toward the smooth values */
        for (col=0; col < (width & ~3); col++) {
            for (i=j=30, c=0; c < 3; c++) {
                i += smrow[2][col][c];
                j += image[row*width+col][c];
            }
            j = (j << 16) / i;
            for (sum=c=0; c < 3; c++) {
                ipix[c] = foveon_apply_curve(
                    curve[c+3],
                    ((smrow[2][col][c] * j + 0x8000) >> 16) - 
                    image[row*width+col][c]);
                sum += ipix[c];
            }
            sum >>= 3;
            FORC3 {
                i = image[row*width+col][c] + ipix[c] - sum;
                if (i < 0) i = 0;
                image[row*width+col][c] = i;
            }
        }
    }
    free (shrink);
    free (smrow[6]);
    for (i=0; i < 8; i++)
        free (curve[i]);

    /* Trim off the black border */
    active[1] -= keep[1];
    active[3] -= 2;
    i = active[2] - active[0];
    for (row = 0; row < active[3]-active[1]; row++)
        memcpy (image[row*i], image[(row+active[1])*width+active[0]],
                i * sizeof *image);
    width = i;
    height = row;
}
