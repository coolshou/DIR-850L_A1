#define _BSD_SOURCE
    /* Make sure strcasecmp is in string.h */
#define _XOPEN_SOURCE
    /* Make sure putenv is in stdlib.h */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#ifdef HAVE_JPEG
#include <jpeglib.h>
#endif

#include "pm.h"
#include "mallocvar.h"

#include "global_variables.h"
#include "util.h"
#include "decode.h"
#include "bayer.h"
#include "ljpeg.h"
#include "dng.h"

#include "camera.h"

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

#ifndef LONG_BIT
#define LONG_BIT (8 * sizeof (long))
#endif

#define FORC3 for (c=0; c < 3; c++)
#define FORC4 for (c=0; c < colors; c++)

static void 
merror (const void *ptr, const char *where) 
{
    if (ptr == NULL)
        pm_error ("Out of memory in %s", where);
}




static void  
adobe_copy_pixel (int row, int col, unsigned short **rp, bool use_secondary)
{
  unsigned r=row, c=col;

  if (fuji_secondary && use_secondary) (*rp)++;
  if (filters) {
    if (fuji_width) {
      r = row + fuji_width - 1 - (col >> 1);
      c = row + ((col+1) >> 1);
    }
    if (r < height && c < width)
      BAYER(r,c) = **rp < 0x1000 ? curve[**rp] : **rp;
    *rp += 1 + fuji_secondary;
  } else
    for (c=0; c < tiff_samples; c++) {
      image[row*width+col][c] = **rp < 0x1000 ? curve[**rp] : **rp;
      (*rp)++;
    }
  if (fuji_secondary && use_secondary) (*rp)--;
}

void
adobe_dng_load_raw_lj()
{
  int save, twide, trow=0, tcol=0, jrow, jcol;
  struct jhead jh;
  unsigned short *rp;

  while (1) {
    save = ftell(ifp);
    fseek (ifp, get4(ifp), SEEK_SET);
    if (!ljpeg_start (ifp, &jh)) break;
    if (trow >= raw_height) break;
    if (jh.high > raw_height-trow)
    jh.high = raw_height-trow;
    twide = jh.wide;
    if (filters) twide *= jh.clrs;
    else         colors = jh.clrs;
    if (fuji_secondary) twide /= 2;
    if (twide > raw_width-tcol)
    twide = raw_width-tcol;

    for (jrow=0; jrow < jh.high; jrow++) {
      ljpeg_row (&jh);
      for (rp=jh.row, jcol=0; jcol < twide; jcol++)
    adobe_copy_pixel (trow+jrow, tcol+jcol, &rp, use_secondary);
    }
    fseek (ifp, save+4, SEEK_SET);
    if ((tcol += twide) >= raw_width) {
      tcol = 0;
      trow += jh.high;
    }
    free (jh.row);
  }
}



void
adobe_dng_load_raw_nc()
{
    unsigned short *pixel, *rp;
    int row, col;

    pixel = calloc (raw_width * tiff_samples, sizeof *pixel);
    merror (pixel, "adobe_dng_load_raw_nc()");
    for (row=0; row < raw_height; row++) {
        read_shorts (ifp, pixel, raw_width * tiff_samples);
        for (rp=pixel, col=0; col < raw_width; col++)
            adobe_copy_pixel (row, col, &rp, use_secondary);
    }
    free (pixel);
}



static int nikon_curve_offset;

void
nikon_compressed_load_raw(void)
{
    static const unsigned char nikon_tree[] = {
        0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,
        5,4,3,6,2,7,1,0,8,9,11,10,12
    };
    int csize, row, col, i, diff;
    unsigned short vpred[4], hpred[2], *curve;

    init_decoder();
    make_decoder (nikon_tree, 0);

    fseek (ifp, nikon_curve_offset, SEEK_SET);
    read_shorts (ifp, vpred, 4);
    csize = get2(ifp);
    curve = calloc (csize, sizeof *curve);
    merror (curve, "nikon_compressed_load_raw()");
    read_shorts (ifp, curve, csize);

    fseek (ifp, data_offset, SEEK_SET);
    getbits(ifp, -1);

    for (row=0; row < height; row++)
        for (col=0; col < raw_width; col++)
        {
            diff = ljpeg_diff (first_decode);
            if (col < 2) {
                i = 2*(row & 1) + (col & 1);
                vpred[i] += diff;
                hpred[col] = vpred[i];
            } else
                hpred[col & 1] += diff;
            if ((unsigned) (col-left_margin) >= width) continue;
            diff = hpred[col & 1];
            if (diff >= csize) diff = csize-1;
            BAYER(row,col-left_margin) = curve[diff];
        }
    maximum = curve[csize-1];
    free (curve);
}

void
nikon_load_raw()
{
  int irow, row, col, i;

  getbits(ifp, -1);
  for (irow=0; irow < height; irow++) {
    row = irow;
    if (model[0] == 'E') {
      row = irow * 2 % height + irow / (height/2);
      if (row == 1 && atoi(model+1) < 5000) {
    fseek (ifp, 0, SEEK_END);
    fseek (ifp, ftell(ifp)/2, SEEK_SET);
    getbits(ifp, -1);
      }
    }
    for (col=0; col < raw_width; col++) {
      i = getbits(ifp, 12);
      if ((unsigned) (col-left_margin) < width)
    BAYER(row,col-left_margin) = i;
      if (tiff_data_compression == 34713 && (col % 10) == 9)
    getbits(ifp, 8);
    }
  }
}

/*
   Figure out if a NEF file is compressed.  These fancy heuristics
   are only needed for the D100, thanks to a bug in some cameras
   that tags all images as "compressed".
 */
int
nikon_is_compressed()
{
  unsigned char test[256];
  int i;

  if (tiff_data_compression != 34713)
    return 0;
  if (strcmp(model,"D100"))
    return 1;
  fseek (ifp, data_offset, SEEK_SET);
  fread (test, 1, 256, ifp);
  for (i=15; i < 256; i+=16)
    if (test[i]) return 1;
  return 0;
}

/*
   Returns 1 for a Coolpix 990, 0 for a Coolpix 995.
 */
int
nikon_e990()
{
  int i, histo[256];
  const unsigned char often[] = { 0x00, 0x55, 0xaa, 0xff };

  memset (histo, 0, sizeof histo);
  fseek (ifp, 2064*1540*3/4, SEEK_SET);
  for (i=0; i < 2000; i++)
    histo[fgetc(ifp)]++;
  for (i=0; i < 4; i++)
    if (histo[often[i]] > 400)
      return 1;
  return 0;
}

/*
   Returns 1 for a Coolpix 2100, 0 for anything else.
 */
int
nikon_e2100()
{
  unsigned char t[12];
  int i;

  fseek (ifp, 0, SEEK_SET);
  for (i=0; i < 1024; i++) {
    fread (t, 1, 12, ifp);
    if (((t[2] & t[4] & t[7] & t[9]) >> 4
    & t[1] & t[6] & t[8] & t[11] & 3) != 3)
      return 0;
  }
  return 1;
}

/*
   Separates a Pentax Optio 33WR from a Nikon E3700.
 */
int
pentax_optio33()
{
  int i, sum[] = { 0, 0 };
  unsigned char tail[952];

  fseek (ifp, -sizeof tail, SEEK_END);
  fread (tail, 1, sizeof tail, ifp);
  for (i=0; i < sizeof tail; i++)
    sum[(i>>2) & 1] += tail[i];
  return sum[0] < sum[1]*4;
}

/*
   Separates a Minolta DiMAGE Z2 from a Nikon E4300.
 */
int
minolta_z2()
{
  int i;
  char tail[424];

  fseek (ifp, -sizeof tail, SEEK_END);
  fread (tail, 1, sizeof tail, ifp);
  for (i=0; i < sizeof tail; i++)
    if (tail[i]) return 1;
  return 0;
}

void
nikon_e2100_load_raw()
{
  unsigned char   data[3432], *dp;
  unsigned short pixel[2288], *pix;
  int row, col;

  for (row=0; row <= height; row+=2) {
    if (row == height) {
      fseek (ifp, ((width==1616) << 13) - (-ftell(ifp) & -2048), SEEK_SET);
      row = 1;
    }
    fread (data, 1, width*3/2, ifp);
    for (dp=data, pix=pixel; pix < pixel+width; dp+=12, pix+=8) {
      pix[0] = (dp[2] >> 4) + (dp[ 3] << 4);
      pix[1] = (dp[2] << 8) +  dp[ 1];
      pix[2] = (dp[7] >> 4) + (dp[ 0] << 4);
      pix[3] = (dp[7] << 8) +  dp[ 6];
      pix[4] = (dp[4] >> 4) + (dp[ 5] << 4);
      pix[5] = (dp[4] << 8) +  dp[11];
      pix[6] = (dp[9] >> 4) + (dp[10] << 4);
      pix[7] = (dp[9] << 8) +  dp[ 8];
    }
    for (col=0; col < width; col++)
      BAYER(row,col) = (pixel[col] & 0xfff);
  }
}

void
nikon_e950_load_raw()
{
  int irow, row, col;

  getbits(ifp, -1);
  for (irow=0; irow < height; irow++) {
    row = irow * 2 % height;
    for (col=0; col < width; col++)
      BAYER(row,col) = getbits(ifp, 10);
    for (col=28; col--; )
      getbits(ifp, 8);
  }
  maximum = 0x3dd;
}

/*
   The Fuji Super CCD is just a Bayer grid rotated 45 degrees.
 */
void
fuji_s2_load_raw()
{
  unsigned short pixel[2944];
  int row, col, r, c;

  fseek (ifp, (2944*24+32)*2, SEEK_CUR);
  for (row=0; row < 2144; row++) {
    read_shorts(ifp, pixel, 2944);
    for (col=0; col < 2880; col++) {
      r = row + ((col+1) >> 1);
      c = 2143 - row + (col >> 1);
      BAYER(r,c) = pixel[col];
    }
  }
}

void
fuji_s3_load_raw()
{
  unsigned short pixel[4352];
  int row, col, r, c;

  fseek (ifp, (4352*2+32)*2, SEEK_CUR);
  for (row=0; row < 1440; row++) {
    read_shorts(ifp, pixel, 4352);
    for (col=0; col < 4288; col++) {
      r = 2143 + row - (col >> 1);
      c = row + ((col+1) >> 1);
      BAYER(r,c) = pixel[col];
    }
  }
}

static void  fuji_common_load_raw (int ncol, int icol, int nrow)
{
  unsigned short pixel[2048];
  int row, col, r, c;

  for (row=0; row < nrow; row++) {
    read_shorts(ifp, pixel, ncol);
    for (col=0; col <= icol; col++) {
      r = icol - col + (row >> 1);
      c = col + ((row+1) >> 1);
      BAYER(r,c) = pixel[col];
    }
  }
}

void
fuji_s5000_load_raw()
{
  fseek (ifp, (1472*4+24)*2, SEEK_CUR);
  fuji_common_load_raw (1472, 1423, 2152);
}

void
fuji_s7000_load_raw()
{
  fuji_common_load_raw (2048, 2047, 3080);
}

/*
   The Fuji Super CCD SR has two photodiodes for each pixel.
   The secondary has about 1/16 the sensitivity of the primary,
   but this ratio may vary.
 */
void
fuji_f700_load_raw()
{
  unsigned short pixel[2944];
  int row, col, r, c, val;

  for (row=0; row < 2168; row++) {
    read_shorts(ifp, pixel, 2944);
    for (col=0; col < 1440; col++) {
      r = 1439 - col + (row >> 1);
      c = col + ((row+1) >> 1);
      val = pixel[col+16 + use_secondary*1472];
      BAYER(r,c) = val;
    }
  }
}

void
rollei_load_raw()
{
  unsigned char pixel[10];
  unsigned iten=0, isix, i, buffer=0, row, col, todo[16];

  isix = raw_width * raw_height * 5 / 8;
  while (fread (pixel, 1, 10, ifp) == 10) {
    for (i=0; i < 10; i+=2) {
      todo[i]   = iten++;
      todo[i+1] = pixel[i] << 8 | pixel[i+1];
      buffer    = pixel[i] >> 2 | buffer << 6;
    }
    for (   ; i < 16; i+=2) {
      todo[i]   = isix++;
      todo[i+1] = buffer >> (14-i)*5;
    }
    for (i=0; i < 16; i+=2) {
      row = todo[i] / raw_width - top_margin;
      col = todo[i] % raw_width - left_margin;
      if (row < height && col < width)
    BAYER(row,col) = (todo[i+1] & 0x3ff);
    }
  }
  maximum = 0x3ff;
}

void
phase_one_load_raw()
{
  int row, col, a, b;
  unsigned short *pixel, akey, bkey;

  fseek (ifp, 8, SEEK_CUR);
  fseek (ifp, get4(ifp) + 296, SEEK_CUR);
  akey = get2(ifp);
  bkey = get2(ifp);
  fseek (ifp, data_offset + 12 + top_margin*raw_width*2, SEEK_SET);
  pixel = calloc (raw_width, sizeof *pixel);
  merror (pixel, "phase_one_load_raw()");
  for (row=0; row < height; row++) {
    read_shorts(ifp, pixel, raw_width);
    for (col=0; col < raw_width; col+=2) {
      a = pixel[col+0] ^ akey;
      b = pixel[col+1] ^ bkey;
      pixel[col+0] = (b & 0xaaaa) | (a & 0x5555);
      pixel[col+1] = (a & 0xaaaa) | (b & 0x5555);
    }
    for (col=0; col < width; col++)
      BAYER(row,col) = pixel[col+left_margin];
  }
  free (pixel);
}

void
ixpress_load_raw()
{
  unsigned short pixel[4090];
  int row, col;

  order = 0x4949;
  fseek (ifp, 304 + 6*2*4090, SEEK_SET);
  for (row=height; --row >= 0; ) {
    read_shorts(ifp, pixel, 4090);
    for (col=0; col < width; col++)
      BAYER(row,col) = pixel[width-1-col];
  }
}

void
leaf_load_raw()
{
  unsigned short *pixel;
  int r, c, row, col;

  pixel = calloc (raw_width, sizeof *pixel);
  merror (pixel, "leaf_load_raw()");
  for (r=0; r < height-32; r+=32)
    FORC3 for (row=r; row < r+32; row++) {
      read_shorts(ifp, pixel, raw_width);
      for (col=0; col < width; col++)
    image[row*width+col][c] = pixel[col];
    }
  free (pixel);
}

/*
   For this function only, raw_width is in bytes, not pixels!
 */
void
packed_12_load_raw()
{
  int row, col;

  getbits(ifp, -1);
  for (row=0; row < height; row++) {
    for (col=0; col < width; col++)
      BAYER(row,col) = getbits(ifp, 12);
    for (col = width*3/2; col < raw_width; col++)
      getbits(ifp, 8);
  }
}

void
unpacked_load_raw()
{
  unsigned short *pixel;
  int row, col;

  pixel = calloc (raw_width, sizeof *pixel);
  merror (pixel, "unpacked_load_raw()");
  for (row=0; row < height; row++) {
    read_shorts(ifp, pixel, raw_width);
    for (col=0; col < width; col++)
      BAYER(row,col) = pixel[col];
  }
  free (pixel);
}

void
olympus_e300_load_raw()
{
  unsigned char  *data,  *dp;
  unsigned short *pixel, *pix;
  int dwide, row, col;

  dwide = raw_width * 16 / 10;
  data = malloc (dwide + raw_width*2);
  merror (data, "olympus_e300_load_raw()");
  pixel = (unsigned short *) (data + dwide);
  for (row=0; row < height; row++) {
    fread (data, 1, dwide, ifp);
    for (dp=data, pix=pixel; pix < pixel+raw_width; dp+=3, pix+=2) {
      if (((dp-data) & 15) == 15) dp++;
      pix[0] = dp[1] << 8 | dp[0];
      pix[1] = dp[2] << 4 | dp[1] >> 4;
    }
    for (col=0; col < width; col++)
      BAYER(row,col) = (pixel[col] & 0xfff);
  }
  free (data);
}

void
olympus_cseries_load_raw()
{
  int irow, row, col;

  for (irow=0; irow < height; irow++) {
    row = irow * 2 % height + irow / (height/2);
    if (row < 2) {
      fseek (ifp, data_offset - row*(-width*height*3/4 & -2048), SEEK_SET);
      getbits(ifp, -1);
    }
    for (col=0; col < width; col++)
      BAYER(row,col) = getbits(ifp, 12);
  }
}

void
eight_bit_load_raw()
{
  unsigned char *pixel;
  int row, col;

  pixel = calloc (raw_width, sizeof *pixel);
  merror (pixel, "eight_bit_load_raw()");
  for (row=0; row < height; row++) {
    fread (pixel, 1, raw_width, ifp);
    for (col=0; col < width; col++)
      BAYER(row,col) = pixel[col];
  }
  free (pixel);
  maximum = 0xff;
}

void
casio_qv5700_load_raw()
{
  unsigned char  data[3232],  *dp;
  unsigned short pixel[2576], *pix;
  int row, col;

  for (row=0; row < height; row++) {
    fread (data, 1, 3232, ifp);
    for (dp=data, pix=pixel; dp < data+3220; dp+=5, pix+=4) {
      pix[0] = (dp[0] << 2) + (dp[1] >> 6);
      pix[1] = (dp[1] << 4) + (dp[2] >> 4);
      pix[2] = (dp[2] << 6) + (dp[3] >> 2);
      pix[3] = (dp[3] << 8) + (dp[4]     );
    }
    for (col=0; col < width; col++)
      BAYER(row,col) = (pixel[col] & 0x3ff);
  }
  maximum = 0x3fc;
}

void
nucore_load_raw()
{
  unsigned short *pixel;
  int irow, row, col;

  pixel = calloc (width, 2);
  merror (pixel, "nucore_load_raw()");
  for (irow=0; irow < height; irow++) {
    read_shorts(ifp, pixel, width);
    row = irow/2 + height/2 * (irow & 1);
    for (col=0; col < width; col++)
      BAYER(row,col) = pixel[col];
  }
  free (pixel);
}

static int  radc_token (int tree)
{
  int t;
  static struct decode *dstart[18], *dindex;
  static const int *s, source[] = {
    1,1, 2,3, 3,4, 4,2, 5,7, 6,5, 7,6, 7,8,
    1,0, 2,1, 3,3, 4,4, 5,2, 6,7, 7,6, 8,5, 8,8,
    2,1, 2,3, 3,0, 3,2, 3,4, 4,6, 5,5, 6,7, 6,8,
    2,0, 2,1, 2,3, 3,2, 4,4, 5,6, 6,7, 7,5, 7,8,
    2,1, 2,4, 3,0, 3,2, 3,3, 4,7, 5,5, 6,6, 6,8,
    2,3, 3,1, 3,2, 3,4, 3,5, 3,6, 4,7, 5,0, 5,8,
    2,3, 2,6, 3,0, 3,1, 4,4, 4,5, 4,7, 5,2, 5,8,
    2,4, 2,7, 3,3, 3,6, 4,1, 4,2, 4,5, 5,0, 5,8,
    2,6, 3,1, 3,3, 3,5, 3,7, 3,8, 4,0, 5,2, 5,4,
    2,0, 2,1, 3,2, 3,3, 4,4, 4,5, 5,6, 5,7, 4,8,
    1,0, 2,2, 2,-2,
    1,-3, 1,3,
    2,-17, 2,-5, 2,5, 2,17,
    2,-7, 2,2, 2,9, 2,18,
    2,-18, 2,-9, 2,-2, 2,7,
    2,-28, 2,28, 3,-49, 3,-9, 3,9, 4,49, 5,-79, 5,79,
    2,-1, 2,13, 2,26, 3,39, 4,-16, 5,55, 6,-37, 6,76,
    2,-26, 2,-13, 2,1, 3,-39, 4,16, 5,-55, 6,-76, 6,37
  };

  if (free_decode == first_decode)
    for (s=source, t=0; t < 18; t++) {
      dstart[t] = free_decode;
      s = make_decoder_int (s, 0);
    }
  if (tree == 18) {
    if (model[2] == '4')
      return (getbits(ifp, 5) << 3) + 4; /* DC40 */
    else
      return (getbits(ifp, 6) << 2) + 2; /* DC50 */
  }
  for (dindex = dstart[tree]; dindex->branch[0]; )
    dindex = dindex->branch[getbits(ifp, 1)];
  return dindex->leaf;
}

#define FORYX for (y=1; y < 3; y++) for (x=col+1; x >= col; x--)

#define PREDICTOR (c ? (buf[c][y-1][x] + buf[c][y][x+1]) / 2 \
: (buf[c][y-1][x+1] + 2*buf[c][y-1][x] + buf[c][y][x+1]) / 4)

void
kodak_radc_load_raw()
{
  int row, col, tree, nreps, rep, step, i, c, s, r, x, y, val;
  short last[3] = { 16,16,16 }, mul[3], buf[3][3][386];

  init_decoder();
  getbits(ifp, -1);
  for (i=0; i < sizeof(buf)/sizeof(short); i++)
    buf[0][0][i] = 2048;
  for (row=0; row < height; row+=4) {
    for (i=0; i < 3; i++)
      mul[i] = getbits(ifp, 6);
    FORC3 {
      val = ((0x1000000/last[c] + 0x7ff) >> 12) * mul[c];
      s = val > 65564 ? 10:12;
      x = ~(-1 << (s-1));
      val <<= 12-s;
      for (i=0; i < sizeof(buf[0])/sizeof(short); i++)
    buf[c][0][i] = (buf[c][0][i] * val + x) >> s;
      last[c] = mul[c];
      for (r=0; r <= !c; r++) {
    buf[c][1][width/2] = buf[c][2][width/2] = mul[c] << 7;
    for (tree=1, col=width/2; col > 0; ) {
      if ((tree = radc_token(tree))) {
        col -= 2;
        if (tree == 8)
          FORYX buf[c][y][x] = radc_token(tree+10) * mul[c];
        else
          FORYX buf[c][y][x] = radc_token(tree+10) * 16 + PREDICTOR;
      } else
        do {
          nreps = (col > 2) ? radc_token(9) + 1 : 1;
          for (rep=0; rep < 8 && rep < nreps && col > 0; rep++) {
        col -= 2;
        FORYX buf[c][y][x] = PREDICTOR;
        if (rep & 1) {
          step = radc_token(10) << 4;
          FORYX buf[c][y][x] += step;
        }
          }
        } while (nreps == 9);
    }
    for (y=0; y < 2; y++)
      for (x=0; x < width/2; x++) {
        val = (buf[c][y+1][x] << 4) / mul[c];
        if (val < 0) val = 0;
        if (c)
          BAYER(row+y*2+c-1,x*2+2-c) = val;
        else
          BAYER(row+r*2+y,x*2+y) = val;
      }
    memcpy (buf[c][0]+!c, buf[c][2], sizeof buf[c][0]-2*!c);
      }
    }
    for (y=row; y < row+4; y++)
      for (x=0; x < width; x++)
    if ((x+y) & 1) {
      val = (BAYER(y,x)-2048)*2 + (BAYER(y,x-1)+BAYER(y,x+1))/2;
      if (val < 0) val = 0;
      BAYER(y,x) = val;
    }
  }
  maximum = 0x1fff;     /* wild guess */
}

#undef FORYX
#undef PREDICTOR

#ifndef HAVE_JPEG
void kodak_jpeg_load_raw() {}
#else

static boolean
fill_input_buffer (j_decompress_ptr cinfo)
{
  static char jpeg_buffer[4096];
  size_t nbytes;

  nbytes = fread (jpeg_buffer, 1, 4096, ifp);
  swab (jpeg_buffer, jpeg_buffer, nbytes);
  cinfo->src->next_input_byte = jpeg_buffer;
  cinfo->src->bytes_in_buffer = nbytes;
  return TRUE;
}

void
kodak_jpeg_load_raw()
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPARRAY buf;
  JSAMPLE (*pixel)[3];
  int row, col;

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);
  jpeg_stdio_src (&cinfo, ifp);
  cinfo.src->fill_input_buffer = fill_input_buffer;
  jpeg_read_header (&cinfo, TRUE);
  jpeg_start_decompress (&cinfo);
  if ((cinfo.output_width      != width  ) ||
      (cinfo.output_height*2   != height ) ||
      (cinfo.output_components != 3      )) {
    pm_error ("incorrect JPEG dimensions");
  }
  buf = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr) &cinfo, JPOOL_IMAGE, width*3, 1);

  while (cinfo.output_scanline < cinfo.output_height) {
    row = cinfo.output_scanline * 2;
    jpeg_read_scanlines (&cinfo, buf, 1);
    pixel = (void *) buf[0];
    for (col=0; col < width; col+=2) {
      BAYER(row+0,col+0) = pixel[col+0][1] << 1;
      BAYER(row+1,col+1) = pixel[col+1][1] << 1;
      BAYER(row+0,col+1) = pixel[col][0] + pixel[col+1][0];
      BAYER(row+1,col+0) = pixel[col][2] + pixel[col+1][2];
    }
  }
  jpeg_finish_decompress (&cinfo);
  jpeg_destroy_decompress (&cinfo);
  maximum = 0xff << 1;
}

#endif

void
kodak_dc120_load_raw()
{
  static const int mul[4] = { 162, 192, 187,  92 };
  static const int add[4] = {   0, 636, 424, 212 };
  unsigned char pixel[848];
  int row, shift, col;

  for (row=0; row < height; row++) {
    fread (pixel, 848, 1, ifp);
    shift = row * mul[row & 3] + add[row & 3];
    for (col=0; col < width; col++)
      BAYER(row,col) = (unsigned short) pixel[(col + shift) % 848];
  }
  maximum = 0xff;
}

void
kodak_dc20_coeff (float const juice)
{
  static const float my_coeff[3][4] =
  { {  2.25,  0.75, -1.75, -0.25 },
    { -0.25,  0.75,  0.75, -0.25 },
    { -0.25, -1.75,  0.75,  2.25 } };
  static const float flat[3][4] =
  { {  1, 0,   0,   0 },
    {  0, 0.5, 0.5, 0 },
    {  0, 0,   0,   1 } };
  int r, g;

  for (r=0; r < 3; r++)
    for (g=0; g < 4; g++)
      coeff[r][g] = my_coeff[r][g] * juice + flat[r][g] * (1-juice);
  use_coeff = 1;
}

void
kodak_easy_load_raw()
{
  unsigned char *pixel;
  unsigned row, col, icol;

  if (raw_width > width)
    black = 0;
  pixel = calloc (raw_width, sizeof *pixel);
  merror (pixel, "kodak_easy_load_raw()");
  for (row=0; row < height; row++) {
    fread (pixel, 1, raw_width, ifp);
    for (col=0; col < raw_width; col++) {
      icol = col - left_margin;
      if (icol < width)
    BAYER(row,icol) = (unsigned short) curve[pixel[col]];
      else
    black += curve[pixel[col]];
    }
  }
  free (pixel);
  if (raw_width > width)
    black /= (raw_width - width) * height;
  if (!strncmp(model,"DC2",3))
    black = 0;
  maximum = curve[0xff];
}

void
kodak_compressed_load_raw()
{
  unsigned char c, blen[256];
  unsigned short raw[6];
  unsigned row, col, len, save, i, israw=0, bits=0, pred[2];
  INT64 bitbuf=0;
  int diff;

  assert(have64BitArithmetic);

  for (row=0; row < height; row++)
    for (col=0; col < width; col++)
    {
      if ((col & 255) == 0) {       /* Get the bit-lengths of the */
    len = width - col;      /* next 256 pixel values      */
    if (len > 256) len = 256;
    save = ftell(ifp);
    for (israw=i=0; i < len; i+=2) {
      c = fgetc(ifp);
      if ((blen[i+0] = c & 15) > 12 ||
          (blen[i+1] = c >> 4) > 12 )
        israw = 1;
    }
    bitbuf = bits = pred[0] = pred[1] = 0;
    if (len % 8 == 4) {
      bitbuf  = fgetc(ifp) << 8;
      bitbuf += fgetc(ifp);
      bits = 16;
    }
    if (israw)
      fseek (ifp, save, SEEK_SET);
      }
      if (israw) {          /* If the data is not compressed */
    switch (col & 7) {
      case 0:
        read_shorts(ifp, raw, 6);
        diff = raw[0] >> 12 << 8 | raw[2] >> 12 << 4 | raw[4] >> 12;
        break;
      case 1:
        diff = raw[1] >> 12 << 8 | raw[3] >> 12 << 4 | raw[5] >> 12;
        break;
      default:
        diff = raw[(col & 7) - 2] & 0xfff;
    }
      } else {              /* If the data is compressed */
    len = blen[col & 255];      /* Number of bits for this pixel */
    if (bits < len) {       /* Got enough bits in the buffer? */
      for (i=0; i < 32; i+=8)
        bitbuf += (INT64) fgetc(ifp) << (bits+(i^8));
      bits += 32;
    }
    diff = bitbuf & (0xffff >> (16-len));  /* Pull bits from buffer */
    bitbuf >>= len;
    bits -= len;
    if ((diff & (1 << (len-1))) == 0)
      diff -= (1 << len) - 1;
    pred[col & 1] += diff;
    diff = pred[col & 1];
      }
      BAYER(row,col) = curve[diff];
    }
}

void
kodak_yuv_load_raw()
{
  unsigned char c, blen[384];
  unsigned row, col, len, bits=0;
  INT64 bitbuf=0;
  int i, li=0, si, diff, six[6], y[4], cb=0, cr=0, rgb[3];
  unsigned short *ip;

  assert(have64BitArithmetic);

  for (row=0; row < height; row+=2)
    for (col=0; col < width; col+=2) {
      if ((col & 127) == 0) {
    len = (width - col + 1) * 3 & -4;
    if (len > 384) len = 384;
    for (i=0; i < len; ) {
      c = fgetc(ifp);
      blen[i++] = c & 15;
      blen[i++] = c >> 4;
    }
    li = bitbuf = bits = y[1] = y[3] = cb = cr = 0;
    if (len % 8 == 4) {
      bitbuf  = fgetc(ifp) << 8;
      bitbuf += fgetc(ifp);
      bits = 16;
    }
      }
      for (si=0; si < 6; si++) {
    len = blen[li++];
    if (bits < len) {
      for (i=0; i < 32; i+=8)
        bitbuf += (INT64) fgetc(ifp) << (bits+(i^8));
      bits += 32;
    }
    diff = bitbuf & (0xffff >> (16-len));
    bitbuf >>= len;
    bits -= len;
    if ((diff & (1 << (len-1))) == 0)
      diff -= (1 << len) - 1;
    six[si] = diff;
      }
      y[0] = six[0] + y[1];
      y[1] = six[1] + y[0];
      y[2] = six[2] + y[3];
      y[3] = six[3] + y[2];
      cb  += six[4];
      cr  += six[5];
      for (i=0; i < 4; i++) {
    ip = image[(row+(i >> 1))*width + col+(i & 1)];
    rgb[0] = y[i] + cr;
    rgb[1] = y[i];
    rgb[2] = y[i] + cb;
    FORC3 if (rgb[c] > 0) ip[c] = curve[rgb[c]];
      }
    }
  maximum = 0xe74;
}

static void  sony_decrypt (unsigned *data, int len, int start, int key)
{
  static uint32_t pad[128];
  unsigned int p;

  if (start) {
    for (p=0; p < 4; p++)
      pad[p] = key = key * 48828125 + 1;
    pad[3] = pad[3] << 1 | (pad[0]^pad[2]) >> 31;
    for (p=4; p < 127; p++)
      pad[p] = (pad[p-4]^pad[p-2]) << 1 | (pad[p-3]^pad[p-1]) >> 31;

    /* Convert to big-endian */

    for (p=0; p < 127; p++) {
        union {
            unsigned char bytes[4];
            uint32_t word;
        } u;

        u.bytes[0] = pad[p] >> 24;
        u.bytes[1] = pad[p] >> 16;
        u.bytes[2] = pad[p] >>  8;
        u.bytes[3] = pad[p] >>  0;
        
        pad[p] = u.word;
    }
  }
  while (len--)
    *data++ ^= pad[p++ & 0x7f] = pad[(p+1) & 0x7f] ^ pad[(p+65) & 0x7f];
}

void
sony_load_raw()
{
  unsigned char head[40];
  struct pixel {
      unsigned char bytes[2];
  };
  struct pixel * pixelrow;
  unsigned i, key, row, col;

  fseek (ifp, 200896, SEEK_SET);
  fseek (ifp, (unsigned) fgetc(ifp)*4 - 1, SEEK_CUR);
  order = 0x4d4d;
  key = get4(ifp);
  fseek (ifp, 164600, SEEK_SET);
  fread (head, 1, 40, ifp);
  sony_decrypt ((void *) head, 10, 1, key);
  for (i=26; i-- > 22; )
    key = key << 8 | head[i];
  fseek (ifp, data_offset, SEEK_SET);
  MALLOCARRAY(pixelrow, raw_width);
  merror (pixelrow, "sony_load_raw()");
  for (row=0; row < height; row++) {
    fread (pixelrow, 2, raw_width, ifp);
    sony_decrypt ((void *) pixelrow, raw_width/2, !row, key);
    for (col=9; col < left_margin; col++)
      black += pixelrow[col].bytes[0] * 256 + pixelrow[col].bytes[1];
    for (col=0; col < width; col++)
      BAYER(row,col) =
          pixelrow[col+left_margin].bytes[0] * 256 +
          pixelrow[col+left_margin].bytes[1];
  }
  free (pixelrow);
  if (left_margin > 9)
    black /= (left_margin-9) * height;
  maximum = 0x3ff0;
}

void
parse_minolta(FILE * const ifp)
{
  int save, tag, len, offset, high=0, wide=0;

  fseek (ifp, 4, SEEK_SET);
  offset = get4(ifp) + 8;
  while ((save=ftell(ifp)) < offset) {
    tag = get4(ifp);
    len = get4(ifp);
    switch (tag) {
      case 0x505244:                /* PRD */
    fseek (ifp, 8, SEEK_CUR);
    high = get2(ifp);
    wide = get2(ifp);
    break;
      case 0x574247:                /* WBG */
    get4(ifp);
    camera_red  = get2(ifp);
    camera_red /= get2(ifp);
    camera_blue = get2(ifp);
    camera_blue = get2(ifp) / camera_blue;
    break;
      case 0x545457:                /* TTW */
    parse_tiff(ifp, ftell(ifp));
    }
    fseek (ifp, save+len+8, SEEK_SET);
  }
  raw_height = high;
  raw_width  = wide;
  data_offset = offset;
}

/*
   CIFF block 0x1030 contains an 8x8 white sample.
   Load this into white[][] for use in scale_colors().
 */
static void  ciff_block_1030()
{
  static const unsigned short key[] = { 0x410, 0x45f3 };
  int i, bpp, row, col, vbits=0;
  unsigned long bitbuf=0;

  get2(ifp);
  if (get4(ifp) != 0x80008) return;
  if (get4(ifp) == 0) return;
  bpp = get2(ifp);
  if (bpp != 10 && bpp != 12) return;
  for (i=row=0; row < 8; row++)
    for (col=0; col < 8; col++) {
      if (vbits < bpp) {
    bitbuf = bitbuf << 16 | (get2(ifp) ^ key[i++ & 1]);
    vbits += 16;
      }
      white[row][col] =
    bitbuf << (LONG_BIT - vbits) >> (LONG_BIT - bpp);
      vbits -= bpp;
    }
}

/*
   Parse a CIFF file, better known as Canon CRW format.
 */
void 
parse_ciff(FILE * const ifp, 
           int    const offset, 
           int    const length)
{
  int tboff, nrecs, i, type, len, roff, aoff, save, wbi=-1;
  static const int remap[] = { 1,2,3,4,5,1 };
  static const int remap_10d[] = { 0,1,3,4,5,6,0,0,2,8 };
  static const int remap_s70[] = { 0,1,2,9,4,3,6,7,8,9,10,0,0,0,7,0,0,8 };
  unsigned short key[] = { 0x410, 0x45f3 };

  if (strcmp(model,"Canon PowerShot G6") &&
      strcmp(model,"Canon PowerShot S70") &&
      strcmp(model,"Canon PowerShot Pro1"))
    key[0] = key[1] = 0;
  fseek (ifp, offset+length-4, SEEK_SET);
  tboff = get4(ifp) + offset;
  fseek (ifp, tboff, SEEK_SET);
  nrecs = get2(ifp);
  for (i = 0; i < nrecs; i++) {
    type = get2(ifp);
    len  = get4(ifp);
    roff = get4(ifp);
    aoff = offset + roff;
    save = ftell(ifp);
    if (type == 0x080a) {       /* Get the camera make and model */
      fseek (ifp, aoff, SEEK_SET);
      fread (make, 64, 1, ifp);
      fseek (ifp, aoff+strlen(make)+1, SEEK_SET);
      fread (model, 64, 1, ifp);
    }
    if (type == 0x102a) {       /* Find the White Balance index */
      fseek (ifp, aoff+14, SEEK_SET);   /* 0=auto, 1=daylight, 2=cloudy ... */
      wbi = get2(ifp);
      if (((!strcmp(model,"Canon EOS DIGITAL REBEL") ||
        !strcmp(model,"Canon EOS 300D DIGITAL"))) && wbi == 6)
    wbi++;
    }
    if (type == 0x102c) {       /* Get white balance (G2) */
      if (!strcmp(model,"Canon PowerShot G1") ||
      !strcmp(model,"Canon PowerShot Pro90 IS")) {
    fseek (ifp, aoff+120, SEEK_SET);
    white[0][1] = get2(ifp);
    white[0][0] = get2(ifp);
    white[1][0] = get2(ifp);
    white[1][1] = get2(ifp);
      } else {
    fseek (ifp, aoff+100, SEEK_SET);
    goto common;
      }
    }
    if (type == 0x0032) {       /* Get white balance (D30 & G3) */
      if (!strcmp(model,"Canon EOS D30")) {
    fseek (ifp, aoff+72, SEEK_SET);
common:
    camera_red   = get2(ifp) ^ key[0];
    camera_red   =(get2(ifp) ^ key[1]) / camera_red;
    camera_blue  = get2(ifp) ^ key[0];
    camera_blue /= get2(ifp) ^ key[1];
      } else if (!strcmp(model,"Canon PowerShot G6") ||
         !strcmp(model,"Canon PowerShot S70")) {
    fseek (ifp, aoff+96 + remap_s70[wbi]*8, SEEK_SET);
    goto common;
      } else if (!strcmp(model,"Canon PowerShot Pro1")) {
    fseek (ifp, aoff+96 + wbi*8, SEEK_SET);
    goto common;
      } else {
    fseek (ifp, aoff+80 + (wbi < 6 ? remap[wbi]*8 : 0), SEEK_SET);
    if (!camera_red)
      goto common;
      }
    }
    if (type == 0x10a9) {       /* Get white balance (D60) */
      if (!strcmp(model,"Canon EOS 10D"))
    wbi = remap_10d[wbi];
      fseek (ifp, aoff+2 + wbi*8, SEEK_SET);
      camera_red  = get2(ifp);
      camera_red /= get2(ifp);
      camera_blue = get2(ifp);
      camera_blue = get2(ifp) / camera_blue;
    }
    if (type == 0x1030 && (wbi == 6 || wbi == 15)) {
      fseek (ifp, aoff, SEEK_SET);  /* Get white sample */
      ciff_block_1030();
    }
    if (type == 0x1031) {       /* Get the raw width and height */
      fseek (ifp, aoff+2, SEEK_SET);
      raw_width  = get2(ifp);
      raw_height = get2(ifp);
    }
    if (type == 0x180e) {       /* Get the timestamp */
      fseek (ifp, aoff, SEEK_SET);
      timestamp = get4(ifp);
    }
    if (type == 0x580e)
      timestamp = len;
    if (type == 0x1810) {       /* Get the rotation */
      fseek (ifp, aoff+12, SEEK_SET);
      flip = get4(ifp);
    }
    if (type == 0x1835) {       /* Get the decoder table */
      fseek (ifp, aoff, SEEK_SET);
      crw_init_tables (get4(ifp));
    }
    if (type >> 8 == 0x28 || type >> 8 == 0x30) /* Get sub-tables */
      parse_ciff(ifp, aoff, len);
    fseek (ifp, save, SEEK_SET);
  }
  if (wbi == 0 && !strcmp(model,"Canon EOS D30"))
    camera_red = -1;            /* Use my auto WB for this photo */
}

void
parse_rollei(FILE * const ifp)
{
  char line[128], *val;
  int tx=0, ty=0;
  struct tm t;
  time_t ts;

  fseek (ifp, 0, SEEK_SET);
  do {
    fgets (line, 128, ifp);
    if ((val = strchr(line,'=')))
      *val++ = 0;
    else
      val = line + strlen(line);
    if (!strcmp(line,"DAT"))
      sscanf (val, "%d.%d.%d", &t.tm_mday, &t.tm_mon, &t.tm_year);
    if (!strcmp(line,"TIM"))
      sscanf (val, "%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);
    if (!strcmp(line,"HDR"))
      data_offset = atoi(val);
    if (!strcmp(line,"X  "))
      raw_width = atoi(val);
    if (!strcmp(line,"Y  "))
      raw_height = atoi(val);
    if (!strcmp(line,"TX "))
      tx = atoi(val);
    if (!strcmp(line,"TY "))
      ty = atoi(val);
  } while (strncmp(line,"EOHD",4));
  t.tm_year -= 1900;
  t.tm_mon -= 1;
  putenv((char*)"TZ=");
  if ((ts = mktime(&t)) > 0)
    timestamp = ts;
  data_offset += tx * ty * 2;
  strcpy (make, "Rollei");
  strcpy (model,"d530flex");
}



void
parse_mos(FILE * const ifp, 
          int    const offset)
{
    char data[40];
    int skip, from, i, neut[4];

    fseek (ifp, offset, SEEK_SET);
    while (1) {
        fread (data, 1, 8, ifp);
        if (strcmp(data,"PKTS")) break;
        fread (data, 1, 40, ifp);
        skip = get4(ifp);
        from = ftell(ifp);
#ifdef USE_LCMS
        if (!strcmp(data,"icc_camera_profile")) {
            profile_length = skip;
            profile_offset = from;
        }
#endif
        if (!strcmp(data,"NeutObj_neutrals")) {
            for (i=0; i < 4; i++)
                fscanf (ifp, "%d", neut+i);
            camera_red  = (float) neut[2] / neut[1];
            camera_blue = (float) neut[2] / neut[3];
        }
        parse_mos(ifp, from);
        fseek (ifp, skip+from, SEEK_SET);
    }
}

void
nikon_e950_coeff()
{
  int r, g;
  static const float my_coeff[3][4] =
  { { -1.936280,  1.800443, -1.448486,  2.584324 },
    {  1.405365, -0.524955, -0.289090,  0.408680 },
    { -1.204965,  1.082304,  2.941367, -1.818705 } };

  for (r=0; r < 3; r++)
    for (g=0; g < 4; g++)
      coeff[r][g] = my_coeff[r][g];
  use_coeff = 1;
}



static double getrat()
{
  double num = get4(ifp);
  return num / get4(ifp);
}



static void 
parse_makernote(FILE * const ifp)
{
  unsigned base=0, offset=0, entries, tag, type, len, save;
  static const int size[] = { 1,1,1,2,4,8,1,1,2,4,8,4,8 };
  short sorder;
  char buf[10];
/*
   The MakerNote might have its own TIFF header (possibly with
   its own byte-order!), or it might just be a table.
 */
  sorder = order;
  fread (buf, 1, 10, ifp);
  if (!strncmp (buf,"KC" ,2) ||     /* these aren't TIFF format */
      !strncmp (buf,"MLY",3)) return;
  if (!strcmp (buf,"Nikon")) {
    base = ftell(ifp);
    order = get2(ifp);
    if (get2(ifp) != 42) goto quit;
    offset = get4(ifp);
    fseek (ifp, offset-8, SEEK_CUR);
  } else if (!strncmp (buf,"FUJIFILM",8) ||
         !strcmp  (buf,"Panasonic")) {
    order = 0x4949;
    fseek (ifp,  2, SEEK_CUR);
  } else if (!strcmp (buf,"OLYMP") ||
         !strcmp (buf,"LEICA") ||
         !strcmp (buf,"EPSON"))
    fseek (ifp, -2, SEEK_CUR);
  else if (!strcmp (buf,"AOC") ||
       !strcmp (buf,"QVC"))
    fseek (ifp, -4, SEEK_CUR);
  else fseek (ifp, -10, SEEK_CUR);

  entries = get2(ifp);
  while (entries--) {
    tag  = get2(ifp);
    type = get2(ifp);
    len  = get4(ifp);
    save = ftell(ifp);
    if (len * size[type < 13 ? type:0] > 4)
      fseek (ifp, get4(ifp)+base, SEEK_SET);

    if (tag == 0xc && len == 4) {
      camera_red  = getrat();
      camera_blue = getrat();
    }
    if (tag == 0x14 && len == 2560 && type == 7) {
      fseek (ifp, 1248, SEEK_CUR);
      goto get2_256;
    }
    if (strstr(make,"PENTAX")) {
      if (tag == 0x1b) tag = 0x1018;
      if (tag == 0x1c) tag = 0x1017;
    }
    if (tag == 0x8c)
      nikon_curve_offset = ftell(ifp) + 2112;
    if (tag == 0x96)
      nikon_curve_offset = ftell(ifp) + 2;
    if (tag == 0x97) {
      if (!strcmp(model,"NIKON D100 ")) {
    fseek (ifp, 72, SEEK_CUR);
    camera_red  = get2(ifp) / 256.0;
    camera_blue = get2(ifp) / 256.0;
      } else if (!strcmp(model,"NIKON D2H")) {
    fseek (ifp, 10, SEEK_CUR);
    goto get2_rggb;
      } else if (!strcmp(model,"NIKON D70")) {
    fseek (ifp, 20, SEEK_CUR);
    camera_red  = get2(ifp);
    camera_red /= get2(ifp);
    camera_blue = get2(ifp);
    camera_blue/= get2(ifp);
      }
    }
    if (tag == 0xe0 && len == 17) {
      get2(ifp);
      raw_width  = get2(ifp);
      raw_height = get2(ifp);
    }
    if (tag == 0x200 && len == 4)
      black = (get2(ifp)+get2(ifp)+get2(ifp)+get2(ifp))/4;
    if (tag == 0x201 && len == 4) {
      camera_red  = get2(ifp);
      camera_red /= get2(ifp);
      camera_blue = get2(ifp);
      camera_blue = get2(ifp) / camera_blue;
    }
    if (tag == 0x401 && len == 4) {
      black = (get4(ifp)+get4(ifp)+get4(ifp)+get4(ifp))/4;
    }
    if (tag == 0xe80 && len == 256 && type == 7) {
      fseek (ifp, 48, SEEK_CUR);
      camera_red  = get2(ifp) * 508 * 1.078 / 0x10000;
      camera_blue = get2(ifp) * 382 * 1.173 / 0x10000;
    }
    if (tag == 0xf00 && len == 614 && type == 7) {
      fseek (ifp, 188, SEEK_CUR);
      goto get2_256;
    }
    if (tag == 0x1017)
      camera_red  = get2(ifp) / 256.0;
    if (tag == 0x1018)
      camera_blue = get2(ifp) / 256.0;
    if (tag == 0x2011 && len == 2) {
get2_256:
      order = 0x4d4d;
      camera_red  = get2(ifp) / 256.0;
      camera_blue = get2(ifp) / 256.0;
    }
    if (tag == 0x4001) {
      fseek (ifp, strstr(model,"EOS-1D") ? 68:50, SEEK_CUR);
get2_rggb:
      camera_red  = get2(ifp);
      camera_red /= get2(ifp);
      camera_blue = get2(ifp);
      camera_blue = get2(ifp) / camera_blue;
    }
    fseek (ifp, save+4, SEEK_SET);
  }
quit:
  order = sorder;
}



static void
get_timestamp(FILE * const ifp)
{
/*
   Since the TIFF DateTime string has no timezone information,
   assume that the camera's clock was set to Universal Time.
 */
  struct tm t;
  time_t ts;

  if (fscanf (ifp, "%d:%d:%d %d:%d:%d", &t.tm_year, &t.tm_mon,
    &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
    return;
  t.tm_year -= 1900;
  t.tm_mon -= 1;
  putenv((char*)"TZ=UTC");   /* Remove this to assume local time */
  if ((ts = mktime(&t)) > 0)
    timestamp = ts;
}

static void 
parse_exif(FILE * const ifp, int base)
{
  int entries, tag, type, len, val, save;

  entries = get2(ifp);
  while (entries--) {
    tag  = get2(ifp);
    type = get2(ifp);
    len  = get4(ifp);
    val  = get4(ifp);
    save = ftell(ifp);
    fseek (ifp, base+val, SEEK_SET);
    if (tag == 0x9003 || tag == 0x9004)
      get_timestamp(ifp);
    if (tag == 0x927c) {
      if (!strncmp(make,"SONY",4))
    data_offset = base+val+len;
      else
    parse_makernote(ifp);
    }
    fseek (ifp, save, SEEK_SET);
  }
}

static int 
parse_tiff_ifd(FILE * const ifp, int base, int level)
{
  unsigned entries, tag, type, len, plen=16, save;
  int done=0, use_cm=0, cfa, i, j, c;
  static const int size[] = { 1,1,1,2,4,8,1,1,2,4,8,4,8 };
  char software[64];
  static const int flip_map[] = { 0,1,3,2,4,6,7,5 };
  unsigned char cfa_pat[16], cfa_pc[] = { 0,1,2,3 }, tab[256];
  unsigned short scale[4];
  double dblack, cc[4][4], cm[4][3];
  double ab[]={ 1,1,1,1 }, asn[] = { 0,0,0,0 }, xyz[] = { 1,1,1 };

  for (j=0; j < 4; j++)
    for (i=0; i < 4; i++)
      cc[j][i] = i == j;
  entries = get2(ifp);
  if (entries > 512) return 1;
  while (entries--) {
    tag  = get2(ifp);
    type = get2(ifp);
    len  = get4(ifp);
    save = ftell(ifp);
    if (tag > 50700 && tag < 50800) done = 1;
    if (len * size[type < 13 ? type:0] > 4)
      fseek (ifp, get4(ifp)+base, SEEK_SET);
    switch (tag) {
      case 0x11:
    camera_red  = get4(ifp) / 256.0;
    break;
      case 0x12:
    camera_blue = get4(ifp) / 256.0;
    break;
      case 0x100:           /* ImageWidth */
    if (strcmp(make,"Canon") || level)
      raw_width = type==3 ? get2(ifp) : get4(ifp);
    break;
      case 0x101:           /* ImageHeight */
    if (strcmp(make,"Canon") || level)
      raw_height = type==3 ? get2(ifp) : get4(ifp);
    break;
      case 0x102:           /* Bits per sample */
    fuji_secondary = len == 2;
    if (level) maximum = (1 << get2(ifp)) - 1;
    break;
      case 0x103:           /* Compression */
    tiff_data_compression = get2(ifp);
    break;
      case 0x106:           /* Kodak color format */
    kodak_data_compression = get2(ifp);
    break;
      case 0x10f:           /* Make */
    fgets (make, 64, ifp);
    break;
      case 0x110:           /* Model */
    fgets (model, 64, ifp);
    break;
      case 0x111:           /* StripOffset */
    data_offset = get4(ifp);
    break;
      case 0x112:           /* Orientation */
    flip = flip_map[(get2(ifp)-1) & 7];
    break;
      case 0x115:           /* SamplesPerPixel */
    tiff_samples = get2(ifp);
    break;
      case 0x131:           /* Software tag */
    fgets (software, 64, ifp);
    if (!strncmp(software,"Adobe",5))
      make[0] = 0;
    break;
      case 0x132:           /* DateTime tag */
    get_timestamp(ifp);
    break;
      case 0x144:           /* TileOffsets */
    if (level) {
      data_offset = ftell(ifp);
    } else {
      strcpy (make, "Leaf");
      data_offset = get4(ifp);
    }
    break;
      case 0x14a:           /* SubIFD tag */
    if (len > 2 && !is_dng && !strcmp(make,"Kodak"))
        len = 2;
    while (len--) {
      i = ftell(ifp);
      fseek (ifp, get4(ifp)+base, SEEK_SET);
      if (parse_tiff_ifd(ifp, base, level+1)) break;
      fseek (ifp, i+4, SEEK_SET);
    }
    break;
      case 33405:           /* Model2 */
    fgets (model2, 64, ifp);
    break;
      case 33422:           /* CFAPattern */
    if ((plen=len) > 16) plen = 16;
    fread (cfa_pat, 1, plen, ifp);
    for (colors=cfa=i=0; i < plen; i++) {
      colors += !(cfa & (1 << cfa_pat[i]));
      cfa |= 1 << cfa_pat[i];
    }
    if (cfa == 070) memcpy (cfa_pc,"\003\004\005",3);   /* CMY */
    if (cfa == 072) memcpy (cfa_pc,"\005\003\004\001",4);   /* GMCY */
    goto guess_cfa_pc;
      case 34665:           /* EXIF tag */
    fseek (ifp, get4(ifp)+base, SEEK_SET);
    parse_exif(ifp, base);
    break;
      case 50706:           /* DNGVersion */
    is_dng = 1;
    if (flip == 7) flip = 4;    /* Adobe didn't read the TIFF spec. */
    break;
      case 50710:           /* CFAPlaneColor */
    if (len > 4) len = 4;
    colors = len;
    fread (cfa_pc, 1, colors, ifp);
guess_cfa_pc:
    FORC4 tab[cfa_pc[c]] = c;
    for (i=16; i--; )
      filters = filters << 2 | tab[cfa_pat[i % plen]];
    break;
      case 50711:           /* CFALayout */
    if (get2(ifp) == 2) {
      fuji_width = (raw_width+1)/2;
      filters = 0x49494949;
    }
    break;
      case 0x123:
      case 0x90d:
      case 50712:           /* LinearizationTable */
    if (len > 0x1000)
        len = 0x1000;
    read_shorts(ifp, curve, len);
    for (i=len; i < 0x1000; i++)
      maximum = curve[i] = curve[i-1];
    break;
      case 50714:           /* BlackLevel */
      case 50715:           /* BlackLevelDeltaH */
      case 50716:           /* BlackLevelDeltaV */
    for (dblack=i=0; i < len; i++)
      dblack += getrat();
    black += dblack/len + 0.5;
    break;
      case 50717:           /* WhiteLevel */
    maximum = get2(ifp);
    break;
      case 50718:           /* DefaultScale */
    for (i=0; i < 4; i++)
      scale[i] = get4(ifp);
    if (scale[1]*scale[2] == 2*scale[0]*scale[3]) ymag = 2;
    break;
      case 50721:           /* ColorMatrix1 */
      case 50722:           /* ColorMatrix2 */
    FORC4 for (j=0; j < 3; j++)
      cm[c][j] = getrat();
    use_cm = 1;
    break;
      case 50723:           /* CameraCalibration1 */
      case 50724:           /* CameraCalibration2 */
    for (i=0; i < colors; i++)
      FORC4 cc[i][c] = getrat();    
      case 50727:           /* AnalogBalance */
    FORC4 ab[c] = getrat();
    break;
      case 50728:           /* AsShotNeutral */
    FORC4 asn[c] = getrat();
    break;
      case 50729:           /* AsShotWhiteXY */
    xyz[0] = getrat();
    xyz[1] = getrat();
    xyz[2] = 1 - xyz[0] - xyz[1];
    }
    fseek (ifp, save+4, SEEK_SET);
  }
  for (i=0; i < colors; i++)
    FORC4 cc[i][c] *= ab[i];
  if (use_cm)
    dng_coeff (cc, cm, xyz);
  if (asn[0])
    FORC4 pre_mul[c] = 1 / asn[c];
  if (!use_cm)
    FORC4 pre_mul[c] /= cc[c][c];

  if (is_dng || level) return done;

  if ((raw_height & 1) && !strncmp (make,"OLYMPUS",7))
       raw_height++;

  if (make[0] == 0 && raw_width == 680 && raw_height == 680) {
    strcpy (make, "Imacon");
    strcpy (model,"Ixpress");
  }
  return done;
}

void
parse_tiff(FILE * const ifp, int base)
{
  int doff;

  fseek (ifp, base, SEEK_SET);
  order = get2(ifp);
  if (order != 0x4949 && order != 0x4d4d) return;
  get2(ifp);
  while ((doff = get4(ifp))) {
    fseek (ifp, doff+base, SEEK_SET);
    if (parse_tiff_ifd(ifp, base, 0)) break;
  }
  if (!is_dng && !strncmp(make,"Kodak",5)) {
    fseek (ifp, 12+base, SEEK_SET);
    parse_tiff_ifd(ifp, base, 2);
  }
}



/*
   Many cameras have a "debug mode" that writes JPEG and raw
   at the same time.  The raw file has no header, so try to
   to open the matching JPEG file and read its metadata.
 */
void
parse_external_jpeg(const char * const ifname)
{
    const char *file, *ext;
    char * jfile;
    char * jext;
    char * jname;

    ext  = strrchr (ifname, '.');
    file = strrchr (ifname, '/');
    if (!file) file = strrchr (ifname, '\\');
    if (!file) file = ifname-1;
    file++;
    if (strlen(ext) != 4 || ext-file != 8) return;
    jname = malloc (strlen(ifname) + 1);
    merror (jname, "parse_external()");
    strcpy (jname, ifname);
    jfile = jname + (file - ifname);
    jext  = jname + (ext  - ifname);
    if (strcasecmp (ext, ".jpg")) {
        strcpy (jext, isupper(ext[1]) ? ".JPG":".jpg");
        memcpy (jfile, file+4, 4);
        memcpy (jfile+4, file, 4);
    } else
        while (isdigit(*--jext)) {
            if (*jext != '9') {
                (*jext)++;
                break;
            }
            *jext = '0';
        }
    if (strcmp (jname, ifname)) {
        FILE * ifP;
        ifP = fopen (jname, "rb");
        if (ifP) {
            if (verbose)
                pm_message ("Reading metadata from %s...", jname);
            parse_tiff(ifP, 12);
            fclose (ifP);
        }
    }
    if (!timestamp)
        pm_message ( "Failed to read metadata from %s", jname);
    free (jname);
}

