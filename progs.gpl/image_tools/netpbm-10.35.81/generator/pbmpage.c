/***************************************************************************
                                pbmpage

  This program produces a printed page test pattern in PBM format.

  This was adapted from Tim Norman's 'pbmtpg' program, part of his
  'pbm2ppa' package, by Bryan Henderson on 2000.05.01.  The only
  change was to make it use the Netpbm libraries to generate the
  output.

  For copyright and licensing information, see the pbmtoppa program,
  which was also derived from the same package.
****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "pbm.h"

/* US is 8.5 in by 11 in */

#define USWIDTH  (5100)
#define USHEIGHT (6600)

/* A4 is 210 mm by 297 mm == 8.27 in by 11.69 in */

#define A4WIDTH  (4960)
#define A4HEIGHT (7016)


struct bitmap {
    int Width;      /* width and height in 600ths of an inch */
    int Height;
    int Pwidth;     /* width in bytes */
    char *bitmap;
};

static struct bitmap bitmap;



static void
setpixel(int const x,
         int const y,
         int const c) {

    int const charidx = y * bitmap.Pwidth + x/8;
    char const bitmask = 128 >> (x % 8);

    if (x < 0 || x >= bitmap.Width)
        return;
    if (y < 0 || y >= bitmap.Height)
        return;

    if (c)
        bitmap.bitmap[charidx] |= bitmask;
    else
        bitmap.bitmap[charidx] &= ~bitmask;
}



static void 
setplus(int x,int y,int s)
/*----------------------------------------------------------------------------
   Draw a black plus sign centered at (x,y) with arms 's' pixels long.  
   Leave the exact center of the plus white.
-----------------------------------------------------------------------------*/
{
  int i;

  for(i=0; i<s; i++)
  {
    setpixel(x+i,y,1);
    setpixel(x-i,y,1);
    setpixel(x,y+i,1);
    setpixel(x,y-i,1);
  }
}



static void 
setblock(int x,int y,int s)
{
  int i,j;

  for(i=0; i<s; i++)
    for(j=0; j<s; j++)
      setpixel(x+i,y+j,1);
}



static void 
setchar(int x,int y,char c)
{
  int xo,yo;
  static char charmap[10][5]= { { 0x3e, 0x41, 0x41, 0x41, 0x3e },
				{ 0x00, 0x42, 0x7f, 0x40, 0x00 },
				{ 0x42, 0x61, 0x51, 0x49, 0x46 },
				{ 0x22, 0x41, 0x49, 0x49, 0x36 },
				{ 0x18, 0x14, 0x12, 0x7f, 0x10 },
				{ 0x27, 0x45, 0x45, 0x45, 0x39 },
				{ 0x3e, 0x49, 0x49, 0x49, 0x32 },
				{ 0x01, 0x01, 0x61, 0x19, 0x07 },
				{ 0x36, 0x49, 0x49, 0x49, 0x36 },
				{ 0x26, 0x49, 0x49, 0x49, 0x3e } };

  if(c<='9' && c>='0')
    for(xo=0; xo<5; xo++)
      for(yo=0; yo<8; yo++)
	if((charmap[c-'0'][xo]>>yo)&1)
	  setblock(x+xo*3,y+yo*3,3);
}



static void 
setstring(int x,int y,char* s)
{
  char* p;
  int xo;

  for(xo=0, p=s; *p; xo+=21, p++)
    setchar(x+xo,y,*p);
}



static void 
setCG(int x,int y)
{
  int xo,yo,zo;

  for(xo=0; xo<=50; xo++)
  {
    yo=sqrt(50.0*50.0-xo*xo);
    setpixel(x+xo,y+yo,1);
    setpixel(x+yo,y+xo,1);
    setpixel(x-1-xo,y-1-yo,1);
    setpixel(x-1-yo,y-1-xo,1);
    setpixel(x+xo,y-1-yo,1);
    setpixel(x-1-xo,y+yo,1);
    for(zo=0; zo<yo; zo++)
    {
      setpixel(x+xo,y-1-zo,1);
      setpixel(x-1-xo,y+zo,1);
    }
  }
}



static void
outputPbm(FILE *        const file,
          struct bitmap const bitmap) {
/*----------------------------------------------------------------------------
  Create a pbm file containing the image from the global variable bitmap[].
-----------------------------------------------------------------------------*/
    int const forceplain = 0;
    bit *pbmrow;
    int row;
    int bitmap_cursor;
    
    pbm_writepbminit(file, bitmap.Width, bitmap.Height, forceplain);
  
    /* We round the allocated row space up to a multiple of 8 so the ugly
       fast code below can work.
       */
    pbmrow = pbm_allocrow(((bitmap.Width+7)/8)*8);
    
    bitmap_cursor = 0;
    for (row = 0; row < bitmap.Height; row++) {
        int col;
        for (col = 0; col < bitmap.Width;) {
            /* A little ugliness makes a big speed difference here. */
            pbmrow[col++] = bitmap.bitmap[bitmap_cursor] & (1<<7);
            pbmrow[col++] = bitmap.bitmap[bitmap_cursor] & (1<<6);
            pbmrow[col++] = bitmap.bitmap[bitmap_cursor] & (1<<5);
            pbmrow[col++] = bitmap.bitmap[bitmap_cursor] & (1<<4);
            pbmrow[col++] = bitmap.bitmap[bitmap_cursor] & (1<<3);
            pbmrow[col++] = bitmap.bitmap[bitmap_cursor] & (1<<2);
            pbmrow[col++] = bitmap.bitmap[bitmap_cursor] & (1<<1);
            pbmrow[col++] = bitmap.bitmap[bitmap_cursor] & (1<<0);
                
            bitmap_cursor++;
        }
        pbm_writepbmrow(file, pbmrow, bitmap.Width, forceplain); 
    }
    pbm_freerow(pbmrow);
    pm_close(file);
}


static void
framePerimeter(unsigned int const Width, 
               unsigned int const Height) {

    unsigned int x,y;

    /* Top edge */
    for (x = 0; x < Width; ++x)
        setpixel(x, 0, 1);

    /* Bottom edge */
    for (x = 0; x < Width; ++x)
        setpixel(x, Height-1, 1);

    /* Left edge */
    for (y = 0; y < Height; ++y)
        setpixel(0, y, 1);

    /* Right edge */
    for (y = 0; y < Height; ++y)
        setpixel(Width-1, y, 1);
}



int 
main(int argc,char** argv) {

    int TP=1;
    int x,y;
    char buf[128];
    int Width;      /* width and height in 600ths of an inch */
    int Height;

    pbm_init(&argc, argv);

    if (argc > 1 && strcmp(argv[1], "-a4") == 0) {
        Width = A4WIDTH;
        Height = A4HEIGHT;
        argc--;
        argv++;
    } else {
        Width = USWIDTH;
        Height = USHEIGHT;
    }

    bitmap.Width = Width;
    bitmap.Height = Height;
    bitmap.Pwidth = (Width + 7) / 8;
    bitmap.bitmap = malloc(bitmap.Pwidth * bitmap.Height);

    for (x = 0; x < bitmap.Pwidth * bitmap.Height; ++x)
        bitmap.bitmap[x] = 0x00;
    
    if (argc>1)
        TP = atoi(argv[1]);

    switch (TP) {
    case 1:
        framePerimeter(Width, Height);
        for (x = 0; x < Width; x += 100)
            for(y = 0; y < Height; y += 100)
                setplus(x, y, 4);
        for(x = 0; x < Width; x += 100) {
            sprintf(buf,"%d", x);
            setstring(x + 3, (Height/200) * 100 + 3, buf);
        }
        for (y=0; y < Height; y += 100) {
            sprintf(buf, "%d", y);
            setstring((Width/200) * 100 + 3, y + 3, buf);
        }
        for (x = 0; x < Width; x += 10)
            for (y = 0; y < Height; y += 100)
                setplus(x, y, ((x%100) == 50) ? 2 : 1);
        for (x=0; x < Width; x += 100)
            for (y = 0; y < Height; y += 10)
                setplus(x, y, ((y%100) == 50) ? 2 : 1);
        setCG(Width/2, Height/2);
        break;
    case 2:
        for (y = 0; y < 300; ++y)
            setpixel(Width/2, Height/2-y, 1);
        break;
    case 3:
        for (y = 0; y < 300; ++y) {
            setpixel(y, y, 1);
            setpixel(Width-1-y, Height-1-y, 1);
        }
        break;
    default:
        pm_error("unknown test pattern (%d)", TP);
    }

    outputPbm(stdout, bitmap);

    return 0;
}
