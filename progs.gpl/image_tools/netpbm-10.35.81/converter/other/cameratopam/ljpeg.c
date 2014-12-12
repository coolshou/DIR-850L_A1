#define _BSD_SOURCE    /* Make sure string.h containst strcasecmp() */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "pm.h"

#include "global_variables.h"
#include "util.h"
#include "decode.h"
#include "bayer.h"

#include "ljpeg.h"


/*
   Not a full implementation of Lossless JPEG, just
   enough to decode Canon, Kodak and Adobe DNG images.
 */

int  
ljpeg_start (FILE * ifp, struct jhead *jh)
{
  int i, tag, len;
  unsigned char data[256], *dp;

  init_decoder();
  for (i=0; i < 4; i++)
    jh->huff[i] = free_decode;
  fread (data, 2, 1, ifp);
  if (data[0] != 0xff || data[1] != 0xd8) return 0;
  do {
    fread (data, 2, 2, ifp);
    tag =  data[0] << 8 | data[1];
    len = (data[2] << 8 | data[3]) - 2;
    if (tag <= 0xff00 || len > 255) return 0;
    fread (data, 1, len, ifp);
    switch (tag) {
      case 0xffc3:
    jh->bits = data[0];
    jh->high = data[1] << 8 | data[2];
    jh->wide = data[3] << 8 | data[4];
    jh->clrs = data[5];
    break;
      case 0xffc4:
    for (dp = data; dp < data+len && *dp < 4; ) {
      jh->huff[*dp] = free_decode;
      dp = make_decoder (++dp, 0);
    }
    }
  } while (tag != 0xffda);
  jh->row = calloc (jh->wide*jh->clrs, 2);
  if (jh->row == NULL)
      pm_error("Out of memory in ljpeg_start()");
  for (i=0; i < 4; i++)
    jh->vpred[i] = 1 << (jh->bits-1);
  zero_after_ff = 1;
  getbits(ifp, -1);
  return 1;
}

int 
ljpeg_diff (struct decode *dindex)
{
  int len, diff;

  while (dindex->branch[0])
    dindex = dindex->branch[getbits(ifp, 1)];
  diff = getbits(ifp, len = dindex->leaf);
  if ((diff & (1 << (len-1))) == 0)
    diff -= (1 << len) - 1;
  return diff;
}

void
ljpeg_row (struct jhead *jh)
{
  int col, c, diff;
  unsigned short *outp=jh->row;

  for (col=0; col < jh->wide; col++)
    for (c=0; c < jh->clrs; c++) {
      diff = ljpeg_diff (jh->huff[c]);
      *outp = col ? outp[-jh->clrs]+diff : (jh->vpred[c] += diff);
      outp++;
    }
}

void  
lossless_jpeg_load_raw(void)
{
  int jwide, jrow, jcol, val, jidx, i, row, col;
  struct jhead jh;
  int min=INT_MAX;

  if (!ljpeg_start (ifp, &jh)) return;
  jwide = jh.wide * jh.clrs;

  for (jrow=0; jrow < jh.high; jrow++) {
    ljpeg_row (&jh);
    for (jcol=0; jcol < jwide; jcol++) {
      val = curve[jh.row[jcol]];
      jidx = jrow*jwide + jcol;
      if (raw_width == 5108) {
    i = jidx / (1680*jh.high);
    if (i < 2) {
      row = jidx / 1680 % jh.high;
      col = jidx % 1680 + i*1680;
    } else {
      jidx -= 2*1680*jh.high;
      row = jidx / 1748;
      col = jidx % 1748 + 2*1680;
    }
      } else if (raw_width == 3516) {
    row = jidx / 1758;
    col = jidx % 1758;
    if (row >= raw_height) {
      row -= raw_height;
      col += 1758;
    }
      } else {
    row = jidx / raw_width;
    col = jidx % raw_width;
      }
      if ((unsigned) (row-top_margin) >= height) continue;
      if ((unsigned) (col-left_margin) < width) {
    BAYER(row-top_margin,col-left_margin) = val;
    if (min > val) min = val;
      } else
    black += val;
    }
  }
  free (jh.row);
  if (raw_width > width)
    black /= (raw_width - width) * height;
  if (!strcasecmp(make,"KODAK"))
    black = min;
}


