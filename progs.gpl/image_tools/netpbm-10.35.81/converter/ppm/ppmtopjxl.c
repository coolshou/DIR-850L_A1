/* ppmtopcl.c - convert portable pixmap into PCL language for HP PaintJet and
 *              PaintJet XL color printers
 * AJCD 12/3/91
 * 
 * usage:
 *       ppmtopcl [-nopack] [-gamma <n>] [-presentation] [-dark]
 *          [-diffuse] [-cluster] [-dither]
 *          [-xshift <s>] [-yshift <s>]
 *          [-xshift <s>] [-yshift <s>]
 *          [-xsize|-width|-xscale <s>] [-ysize|-height|-yscale <s>]
 *          [ppmfile]
 *
 */

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "pm_c_util.h"
#include "ppm.h"
#include "nstring.h"

#define MAXCOLORS 1024

const char * const usage="[-nopack] [-gamma <n>] [-presentation] [-dark]\n\
            [-diffuse] [-cluster] [-dither]\n\
            [-xshift <s>] [-yshift <s>]\n\
            [-xshift <s>] [-yshift <s>]\n\
            [-xsize|-width|-xscale <s>] [-ysize|-height|-yscale <s>]\n\
            [ppmfile]";

#define PCL_MAXWIDTH 2048
#define PCL_MAXHEIGHT 32767
#define PCL_MAXVAL 255

static int nopack = 0;
static int dark = 0;
static int diffuse = 0;
static int dither = 0;
static int cluster = 0;
static int xsize = 0;
static int ysize = 0;
static int xshift = 0;
static int yshift = 0;
static int quality = 0;
static double xscale = 0.0;
static double yscale = 0.0;
static double gamma_val = 0.0;

/* argument types */
#define DIM 0
#define REAL 1
#define BOOL 2
static const struct options {
    const char *name;
    int type;
    void *value;
} options[] = {
   {"-gamma",        REAL, &gamma_val },
   {"-presentation", BOOL, &quality },
   {"-width",        DIM,  &xsize },
   {"-xsize",        DIM,  &xsize },
   {"-height",       DIM,  &ysize },
   {"-ysize",        DIM,  &ysize },
   {"-xscale",       REAL, &xscale },
   {"-yscale",       REAL, &yscale },
   {"-xshift",       DIM,  &xshift },
   {"-yshift",       DIM,  &yshift },
   {"-dark",         BOOL, &dark },
   {"-diffuse",      BOOL, &diffuse },
   {"-dither",       BOOL, &dither },
   {"-cluster",      BOOL, &cluster },
   {"-nopack",       BOOL, &nopack },
};

#define putword(w) (putchar(((w)>>8) & 255), putchar((w) & 255))

static int 
bitsperpixel(int v) {
   int bpp = 0;
   while (v > 0) {  /* calculate # bits for value */
      ++bpp;
      v>>=1;
   }
   return (bpp);
}



static char *inrow = NULL;
static char *outrow = NULL;
/* "signed" was commented out below, but it caused warnings on an SGI 
   compiler, which defaulted to unsigned character.  2001.03.30 BJH */
static signed char *runcnt = NULL;

static void 
putbits(b, n) {
    /* put #n bits in b out, packing into bytes; n=0 flushes bits */
    /* n should never be > 8 */

   static int out = 0;
   static int cnt = 0;
   static int num = 0;
   static int pack = 0;
   if (n) {
      int xo = 0;
      int xc = 0;
      if (cnt+n > 8) {  /* overflowing current byte? */
	 xc = cnt + n - 8;
	 xo = (b & ~(-1 << xc)) << (8-xc);
	 n -= xc;
	 b >>= xc;
      }
      cnt += n;
      out |= (b & ~(-1 << n)) << (8-cnt);
      if (cnt >= 8) {
	 inrow[num++] = out;
	 out = xo;
	 cnt = xc;
      }
   } else { /* flush row */
      int i;
      if (cnt) {
	 inrow[num++] = out;
	 out = cnt = 0;
      }
      for (; num > 0 && inrow[num-1] == 0; num--); /* remove trailing zeros */
      printf("\033*b"); 
      if (num && !nopack) {            /* TIFF 4.0 packbits encoding */
	 int start = 0;
	 int next;
	 runcnt[start] = 0;
	 for (i = 1; i < num; i++) {
	    if (inrow[i] == inrow[i-1]) {
	       if (runcnt[start] <= 0 && runcnt[start] > -127)
		  runcnt[start]--;
	       else
		  runcnt[start = i] = 0;
	    } else {
	       if (runcnt[start] >= 0 && runcnt[start] < 127)
		  runcnt[start]++;
	       else
		  runcnt[start = i] = 0;
	    }
	 }
	 start = 0;
	 for (i = 0; i < num; i = next) {
	    int count = runcnt[i];
	    int from = i;
	    if (count >= 0) { /* merge two-byte runs */
	       for (;;) {
		  next = i+1+runcnt[i];
		  if(next >= num || runcnt[next] < 0 ||
		     count+runcnt[next]+1 > 127)
		     break;
		  count += runcnt[next]+1;
		  i = next;
	       }
	    }
	    next =  i + 1 + ((runcnt[i] < 0) ? -runcnt[i] : runcnt[i]);
	    if (next < num && count > 0 &&
		runcnt[next] < 0 && runcnt[next] > -127) {
	       count--;
	       next--;
	       runcnt[next] = runcnt[next+1]-1;
	    }
	    outrow[start++] = count;
	    if (count >= 0) {
	       while (count-- >= 0)
		  outrow[start++] = inrow[from++];
	    } else
	       outrow[start++] = inrow[from];
	 }
	 if (start < num) {
	    num = start;
	    if (!pack) {
	       printf("2m");
	       pack = 1;
	    }
	 } else {
	    if (pack) {
	       printf("0m");
	       pack = 0;
	    }
	 }
      }
      printf("%dW", num);
      for (i = 0; i < num; i++)
	 putchar(pack ? outrow[i] : inrow[i]);
      num = 0; /* new row */
   }
}

int
main(argc, argv)
     int argc;
     char *argv[];
{
   FILE *ifd;
   register pixel **pixels, *pixrow;
   register int  row, col, bpp, i;
   int rows, cols;
   pixval maxval;
   int bpr, bpg, bpb;
   int render;
   int colors, pclindex;
   colorhist_vector chv;
   colorhash_table cht;
   
   ppm_init( &argc, argv );

   while (argc > 1 && argv[1][0] == '-') {
      char *c;
      for (i = 0; i < sizeof(options)/sizeof(struct options); i++) {
	 if (pm_keymatch(argv[1], options[i].name,
			 MIN(strlen(argv[1]), strlen(options[i].name)))) {
	    switch (options[i].type) {
	    case DIM:
	       if (++argv, --argc == 1)
		  pm_usage(usage);
	       for (c = argv[1]; ISDIGIT(*c); c++);
	       if (c[0] == 'p' && c[1] == 't') /* points */
		  *(int *)(options[i].value) = atoi(argv[1])*10;
	       else if (c[0] == 'd' && c[1] == 'p') /* decipoints */
		  *(int *)(options[i].value) = atoi(argv[1]);
	       else if (c[0] == 'i' && c[1] == 'n') /* inches */
		  *(int *)(options[i].value) = atoi(argv[1])*720;
	       else if (c[0] == 'c' && c[1] == 'm') /* centimetres */
		  *(int *)(options[i].value) = atoi(argv[1])*283.46457;
	       else if (!c[0]) /* dots */
		  *(int *)(options[i].value) = atoi(argv[1])*4;
	       else
		  pm_error("illegal unit of measure %s", c);
	       break;
	    case REAL:
	       if (++argv, --argc == 1)
		  pm_usage(usage);
	       *(double *)(options[i].value) = atof(argv[1]);
	       break;
	    case BOOL:
	       *(int *)(options[i].value) = 1;
	       break;
	    }
	    break;
	 }
      }
      if (i >= sizeof(options)/sizeof(struct options))
	 pm_usage(usage);
      argv++; argc--;
   }
   if (argc > 2)
      pm_usage(usage);
   else if (argc == 2)
      ifd = pm_openr(argv[1]);
   else
      ifd = stdin ;

   /* validate arguments */
   if (diffuse+cluster+dither > 1)
      pm_error("only one of -diffuse, -dither and -cluster may be used");
   render = diffuse ? 4 : dither ? 3 : cluster ? 7 : 0;

   if (xsize != 0.0 && xscale != 0.0)
      pm_error("only one of -xsize and -xscale may be used");

   if (ysize != 0.0 && yscale != 0.0)
      pm_error("only one of -ysize and -yscale may be used");

   pixels = ppm_readppm( ifd, &cols, &rows, &maxval );
   pm_close( ifd );

   /* limit checks */
   if (cols > PCL_MAXWIDTH || rows > PCL_MAXHEIGHT)
      pm_error("image too large; reduce with ppmscale");
   if (maxval > PCL_MAXVAL)
      pm_error("color range too large; reduce with ppmcscale");

   /* Figure out the colormap. */
   fprintf( stderr, "(Computing colormap..." ); fflush( stderr );
   chv = ppm_computecolorhist( pixels, cols, rows, MAXCOLORS, &colors );
   if ( chv == (colorhist_vector) 0 )
      pm_error("too many colors; reduce with pnmquant");
   fprintf( stderr, "  Done.  %d colors found.)\n", colors );

   /* And make a hash table for fast lookup. */
   cht = ppm_colorhisttocolorhash( chv, colors );

   /* work out color downloading mode */
   pclindex = bitsperpixel(colors);
   if (pclindex > 8) /* can't use indexed mode */
      pclindex = 0;
   else
      switch (pclindex) { /* round up to 1,2,4,8 */
      case 0: /* direct mode (no palette) */
	 bpp = bitsperpixel(maxval); /* bits per pixel */
	 bpg = bpp; bpb = bpp;
	 bpp = (bpp*3+7)>>3;     /* bytes per pixel now */
	 bpr = (bpp<<3)-bpg-bpb; 
	 bpp *= cols;            /* bytes per row now */
	 break;
      case 5:         pclindex++;
      case 6:         pclindex++;
      case 3: case 7: pclindex++;
      default:
	 bpp = 8/pclindex;
	 bpp = (cols+bpp-1)/bpp;      /* bytes per row */
      }

   if ((inrow = (char *)malloc((unsigned)bpp)) == NULL ||
       (outrow = (char *)malloc((unsigned)bpp*2)) == NULL ||
       (runcnt = (signed char *)malloc((unsigned)bpp)) == NULL)
      pm_error("can't allocate space for row");

   /* set up image details */
   if (xscale != 0.0)
      xsize = cols * xscale * 4;
   if (yscale != 0.0)
      ysize = rows * yscale * 4;

#ifdef DEBUG
   fprintf(stderr, "dark    =%d\n", dark);
   fprintf(stderr, "diffuse =%d\n", diffuse);
   fprintf(stderr, "dither  =%d\n", dither);
   fprintf(stderr, "cluster =%d\n", cluster);
   fprintf(stderr, "quality =%d\n", quality);
   fprintf(stderr, "xsize   =%d\n", xsize);
   fprintf(stderr, "ysize   =%d\n", ysize);
   fprintf(stderr, "xshift  =%d\n", xshift);
   fprintf(stderr, "yshift  =%d\n", yshift);
   fprintf(stderr, "xscale  =%lf\n", xscale);
   fprintf(stderr, "yscale  =%lf\n", yscale);
   fprintf(stderr, "gamma   =%lf\n", gamma_val);
   fprintf(stderr, "pclindex   =%d\n", pclindex);
   fprintf(stderr, "nopack  =%d\n", nopack);
#endif

   /* write PCL header */
#if 0
   printf("\033&l26A");                         /* paper size */
#endif
   printf("\033*r%ds%dT", cols, rows);          /* source width, height */
   if (xshift != 0 || yshift != 0)
      printf("\033&a%+dh%+dV", xshift, yshift); /* xshift, yshift */
   if (quality)
      printf("\033*o%dQ", quality);             /* print quality */
   printf("\033*t");
   if (xsize == 0 && ysize == 0)
      printf("180r");                   /* resolution */
   else {                               /* destination width, height */
      if (xsize != 0)
	 printf("%dh", xsize);
      if (ysize != 0)
	 printf("%dv", ysize);
   }
   if (gamma_val != 0)
      printf("%.3fi", gamma_val);                    /* gamma correction */
   if (dark)
      printf("%dk", dark);              /* scaling algorithms */
   printf("%dJ", render);               /* rendering algorithm */
   printf("\033*v18W");                           /* configure image data */
      putchar(0); /* relative colors */
      putchar(pclindex ? 1 : 3); /* index/direct pixel mode */
      putchar(pclindex); /* ignored in direct pixel mode */
      if (pclindex) {
	 putchar(0);
	 putchar(0);
	 putchar(0);
      } else {
	 putchar(bpr); /* bits per red */
	 putchar(bpg); /* bits per green */
	 putchar(bpb); /* bits per blue */
      }
      putword(maxval); /* max red reference */
      putword(maxval); /* max green reference */
      putword(maxval); /* max blue reference */
      putword(0); /* min red reference */
      putword(0); /* min green reference */
      putword(0); /* min blue reference */
   if (pclindex) {                        /* set palette */
      for (i = 0; i < colors; i++) {
	 int r, g, b;
	 r = PPM_GETR( chv[i].color );
	 g = PPM_GETG( chv[i].color );
	 b = PPM_GETB( chv[i].color );
	 if (i == 0)
	    printf("\033*v");
	 if (r)
	    printf("%da", r);
	 if (g)
	    printf("%db", g);
	 if (b)
	    printf("%dc", b);
	 if (i == colors-1)
	    printf("%dI", i);    /* assign color index */
	 else
	    printf("%di", i);    /* assign color index */
      }
   }
   ppm_freecolorhist( chv );

   /* start raster graphics at CAP */
   printf("\033*r%dA", (xsize != 0 || ysize != 0) ? 3 : 1);

   for (row = 0; row < rows; row++) {
      if (pclindex) { /* indexed color mode */
	 int out, cnt;
	 out = cnt = 0;
	 for (col = 0, pixrow=pixels[row]; col < cols; col++, pixrow++) {
	    putbits(ppm_lookupcolor( cht, pixrow ), pclindex);
	 }
	 putbits(0, 0); /* flush row */
      } else { /* direct color mode */
	 for (col = 0, pixrow=pixels[row]; col < cols; col++, pixrow++) {
	    putbits(PPM_GETR( *pixrow ), bpr);
	    putbits(PPM_GETG( *pixrow ), bpg);
	    putbits(PPM_GETB( *pixrow ), bpb); /* don't need to flush */
	 }
	 putbits(0, 0); /* flush row */
      }
   }
   printf("\033*rC"); /* end raster graphics */
   exit(0);
}
