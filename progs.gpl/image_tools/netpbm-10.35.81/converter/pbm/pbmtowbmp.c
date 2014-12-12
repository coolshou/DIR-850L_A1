/* pbmtowbmp.c - convert a portable bitmap to a Wireless Bitmap file

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

static void 
outputint(int i) {
  int c = 1;
  while(i & 0x7f << 7*c) c++;
  while(c > 1) putchar(0x80 | ((i >> 7*--c) & 0xff));
  putchar(i & 0x7f);
}



int 
main(int argc, char *argv[]) {
  FILE *f;
  bit **image;
  int rows, cols, row, col;
  int c, p;

  pbm_init(&argc, argv);
  if(argc > 2) {
    fprintf(stderr, "Copyright (C) 1999 Terje Sannum <terje@looplab.com>\n");
    pm_usage("[pbmfile]");
  }

  f = argc == 2 ? pm_openr(argv[1]) : stdin;
  image = pbm_readpbm(f, &cols, &rows);
  pm_close(f);

  /* Header */
  putchar(0); /* Type 0: B/W, no compression */
  putchar(0); /* FixHeaderField */
  /* Geometry */
  outputint(cols);
  outputint(rows);
  /* Image data */
  for(row = 0; row < rows; row++) {
    p = c = 0;
    for(col = 0; col < cols; col++) {
      if(image[row][col] == PBM_WHITE) c = c | (1 << (7-p));
      if(++p == 8) {
	putchar(c);
	p = c = 0;
      }
    }
    if(p) putchar(c);
  }
  
  return 0;
}

