/* wbmptopbm.c - convert a wbmp file to a portable bitmap

   This is derived for Netpbm from the pbmwbmp package from 
   <http://www.looplab.com/wap/tools> on 2000.06.06.
   
   The specifications for the wbmp format are part of the Wireless 
   Application Environment specification at
   <http://www.wapforum.org/what/technical.htm>.

** Copyright (C) 1999 Terje Sannum <terje@looplab.com>.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "pbm.h"

static int 
readc(FILE *f) {
  int c = fgetc(f);
  if(c == EOF) pm_error("EOF / read error");
  return c;
}



static int 
readint(FILE *f) {
  int c=0, pos=0, sum=0;
  do {
    c = readc(f);
    sum = (sum << 7*pos++) | (c & 0x7f);
  } while(c & 0x80);
  return sum;
}



static void 
readheader(int h, FILE *f) {
  int c,i;
  switch(h & 0x60) {
  case 0x00:
    /* Type 00: read multi-byte bitfield */
    do c=readc(f); while(c & 0x80);
    break;
  case 0x60:
    /* Type 11: read name/value pair */
    for(i=0; i < ((h & 0x70) >> 4) + (h & 0x0f); i++) c=readc(f);
    break;
  }
}



static bit **
readwbmp(FILE *f, int *cols, int *rows) {
  int i,j,k,row,c;
  bit **image;
  /* Type */
  c = readint(f);
  if(c != 0) pm_error("Unrecognized WBMP type");
  /* Headers */
  c = readc(f); /* FixHeaderField */
  while(c & 0x80) { /* ExtHeaderFields */
    c = readc(f);
    readheader(c, f);
  }
  /* Geometry */
  *cols = readint(f);
  *rows = readint(f);
  image = pbm_allocarray(*cols, *rows);
  /* read image */
  row = *cols/8;
  if(*cols%8) row +=1;
  for(i=0; i<*rows; i++) {
    for(j=0; j<row; j++) {
      c=readc(f);
      for(k=0; k<8 && j*8+k<*cols; k++) {
	image[i][j*8+k] = c & (0x80 >> k) ? PBM_WHITE : PBM_BLACK;
      }
    }
  }
  return image;
}



int 
main(int argc, char *argv[]) {
  FILE *f;
  bit **image;
  int rows, cols;

  pbm_init(&argc, argv);
  if(argc > 2) {
    fprintf(stderr, "Copyright (C) 1999 Terje Sannum <terje@looplab.com>\n");
    pm_usage("[wbmpfile]");
  }

  f = argc == 2 ? pm_openr(argv[1]) : stdin;
  image = readwbmp(f, &cols, &rows);
  pm_close(f);
  pbm_writepbm(stdout, image, cols, rows, 0);
  pm_close(stdout);
  return 0;
}

