/*
   This is derived from Dave Coffin's raw photo decoder, dcraw.c,
   Copyright 1997-2005 by Dave Coffin, dcoffin a cybercom o net.

   See the COPYRIGHT file in the same directory as this file for
   information on copyright and licensing.
 */


#define _BSD_SOURCE 1   /* Make sure string.h contains strcasecmp() */
#define _XOPEN_SOURCE  /* Make sure unistd.h contains swab() */

#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __CYGWIN__
#include <io.h>
#endif
#ifdef WIN32
  #include <winsock2.h>
  #pragma comment(lib, "ws2_32.lib")
  #define strcasecmp stricmp
  typedef __int64 INT64;
  static bool const have64BitArithmetic = true;
#else
  #include <unistd.h>
#endif

#include "mallocvar.h"
#include "shhopt.h"
#include "pam.h"

#include "global_variables.h"
#include "util.h"
#include "decode.h"
#include "identify.h"
#include "bayer.h"
#include "foveon.h"
#include "dng.h"

#if HAVE_INT64
   typedef int64_t INT64;
   static bool const have64BitArithmetic = true;
#else
   /* We define INT64 to something that lets the code compile, but we
      should not execute any INT64 code, because it will get the wrong
      result.  */
   typedef int INT64;
   static bool const have64BitArithmetic = false;
#endif

/*
   All global variables are defined here, and all functions that
   access them are prefixed with "CLASS".  Note that a thread-safe
   C++ class cannot have non-const static local variables.
 */
FILE * ifp;
short order;
char make[64], model[70], model2[64], *meta_data;
time_t timestamp;
int data_offset, meta_offset, meta_length;
int tiff_data_compression, kodak_data_compression;
int raw_height, raw_width, top_margin, left_margin;
int height, width, fuji_width, colors, tiff_samples;
int black, maximum, clip_max;
int iheight, iwidth, shrink;
int is_dng, is_canon, is_foveon, use_coeff, use_gamma;
int trim, flip, xmag, ymag;
int zero_after_ff;
unsigned filters;
unsigned short (*image)[4], white[8][8], curve[0x1000];
int fuji_secondary;
float cam_mul[4], pre_mul[4], coeff[3][4];
int histogram[3][0x2000];
jmp_buf failure;
bool use_secondary;
bool verbose;

#ifdef USE_LCMS
#include <lcms.h>
int profile_offset, profile_length;
#endif

#define CLASS

#define FORC3 for (c=0; c < 3; c++)
#define FORC4 for (c=0; c < colors; c++)

static void CLASS merror (const void *ptr, const char *where)
{
    if (ptr == NULL)
        pm_error ("Out of memory in %s", where);
}

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFileName;  /* "-" means Standard Input */
    float bright;
    float red_scale;
    float blue_scale;
    const char * profile;
    unsigned int identify_only;
    unsigned int verbose;
    unsigned int half_size;
    unsigned int four_color_rgb;
    unsigned int document_mode;
    unsigned int quick_interpolate;
    unsigned int use_auto_wb;
    unsigned int use_camera_wb;
    unsigned int use_camera_rgb;
    unsigned int use_secondary;
    unsigned int no_clip_color;
    unsigned int linear;
};


static struct cmdlineInfo cmdline;

static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that many of the strings that this function returns in the
   *cmdlineP structure are actually in the supplied argv array.  And
   sometimes, one of these strings is actually just a suffix of an entry
   in argv!
-----------------------------------------------------------------------------*/
    optStruct3 opt;
    optEntry *option_def;
    unsigned int option_def_index;
    unsigned int brightSpec, red_scaleSpec, blue_scaleSpec,
        profileSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;
    opt.allowNegNum = FALSE;

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "bright", 
            OPT_FLOAT,   &cmdlineP->bright,     &brightSpec, 0);
    OPTENT3(0, "red_scale", 
            OPT_FLOAT,   &cmdlineP->red_scale,  &red_scaleSpec, 0);
    OPTENT3(0, "blue_scale", 
            OPT_FLOAT,   &cmdlineP->blue_scale, &blue_scaleSpec, 0);
    OPTENT3(0, "profile", 
            OPT_STRING,  &cmdlineP->profile,    &profileSpec, 0);
    OPTENT3(0,   "identify_only",   
            OPT_FLAG,    NULL, &cmdlineP->identify_only, 0);
    OPTENT3(0,   "verbose",   
            OPT_FLAG,    NULL, &cmdlineP->verbose, 0);
    OPTENT3(0,   "half_size",   
            OPT_FLAG,    NULL, &cmdlineP->half_size, 0);
    OPTENT3(0,   "four_color_rgb",   
            OPT_FLAG,    NULL, &cmdlineP->four_color_rgb, 0);
    OPTENT3(0,   "document_mode",   
            OPT_FLAG,    NULL, &cmdlineP->document_mode, 0);
    OPTENT3(0,   "quick_interpolate",   
            OPT_FLAG,    NULL, &cmdlineP->quick_interpolate, 0);
    OPTENT3(0,   "balance_auto",   
            OPT_FLAG,    NULL, &cmdlineP->use_auto_wb, 0);
    OPTENT3(0,   "balance_camera",   
            OPT_FLAG,    NULL, &cmdlineP->use_camera_wb, 0);
    OPTENT3(0,   "use_secondary",   
            OPT_FLAG,    NULL, &cmdlineP->use_secondary, 0);
    OPTENT3(0,   "no_clip_color",   
            OPT_FLAG,    NULL, &cmdlineP->no_clip_color, 0);
    OPTENT3(0,   "rgb",   
            OPT_FLAG,    NULL, &cmdlineP->use_camera_rgb, 0);
    OPTENT3(0,   "linear",   
            OPT_FLAG,    NULL, &cmdlineP->linear, 0);

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

    if (!brightSpec)
        cmdlineP->bright = 1.0;
    if (!red_scaleSpec)
        cmdlineP->red_scale = 1.0;
    if (!blue_scaleSpec)
        cmdlineP->blue_scale = 1.0;
    if (!profileSpec)
        cmdlineP->profile = NULL;


    if (argc - 1 == 0)
        cmdlineP->inputFileName = strdup("-");  /* he wants stdin */
    else if (argc - 1 == 1)
        cmdlineP->inputFileName = strdup(argv[1]);
    else 
        pm_error("Too many arguments.  The only argument accepted "
                 "is the input file name");
}

  
/*
   Seach from the current directory up to the root looking for
   a ".badpixels" file, and fix those pixels now.
 */
static void CLASS bad_pixels()
{
  FILE *fp=NULL;
  char *fname, *cp, line[128];
  int len, time, row, col, r, c, rad, tot, n, fixed=0;

  if (!filters) return;
  for (len=16 ; ; len *= 2) {
    fname = malloc (len);
    if (!fname) return;
    if (getcwd (fname, len-12)) break;
    free (fname);
    if (errno != ERANGE) return;
  }
#ifdef WIN32
  if (fname[1] == ':')
    memmove (fname, fname+2, len-2);
  for (cp=fname; *cp; cp++)
    if (*cp == '\\') *cp = '/';
#endif
  cp = fname + strlen(fname);
  if (cp[-1] == '/') cp--;
  while (*fname == '/') {
    strcpy (cp, "/.badpixels");
    if ((fp = fopen (fname, "r"))) break;
    if (cp == fname) break;
    while (*--cp != '/');
  }
  free (fname);
  if (!fp) return;
  while (fgets (line, 128, fp)) {
    cp = strchr (line, '#');
    if (cp) *cp = 0;
    if (sscanf (line, "%d %d %d", &col, &row, &time) != 3) continue;
    if ((unsigned) col >= width || (unsigned) row >= height) continue;
    if (time > timestamp) continue;
    for (tot=n=0, rad=1; rad < 3 && n==0; rad++)
      for (r = row-rad; r <= row+rad; r++)
    for (c = col-rad; c <= col+rad; c++)
      if ((unsigned) r < height && (unsigned) c < width &&
        (r != row || c != col) && FC(r,c) == FC(row,col)) {
        tot += BAYER(r,c);
        n++;
      }
    BAYER(row,col) = tot/n;
    if (cmdline.verbose) {
      if (!fixed++)
          pm_message ("Fixed bad pixels at: %d,%d", col, row);
    }
  }
  fclose (fp);
}

static void CLASS scale_colors()
{
  int row, col, c, val, shift=0;
  int min[4], max[4], count[4];
  double sum[4], dmin;

  maximum -= black;
  if (cmdline.use_auto_wb || (cmdline.use_camera_wb && camera_red == -1)) {
    FORC4 min[c] = INT_MAX;
    FORC4 max[c] = count[c] = sum[c] = 0;
    for (row=0; row < height; row++)
      for (col=0; col < width; col++)
    FORC4 {
      val = image[row*width+col][c];
      if (!val) continue;
      if (min[c] > val) min[c] = val;
      if (max[c] < val) max[c] = val;
      val -= black;
      if (val > maximum-25) continue;
      if (val < 0) val = 0;
      sum[c] += val;
      count[c]++;
    }
    FORC4 pre_mul[c] = count[c] / sum[c];
  }
  if (cmdline.use_camera_wb && camera_red != -1) {
    FORC4 count[c] = sum[c] = 0;
    for (row=0; row < 8; row++)
      for (col=0; col < 8; col++) {
    c = FC(row,col);
    if ((val = white[row][col] - black) > 0)
      sum[c] += val;
    count[c]++;
      }
    val = 1;
    FORC4 if (sum[c] == 0) val = 0;
    if (val)
      FORC4 pre_mul[c] = count[c] / sum[c];
    else if (camera_red && camera_blue)
      memcpy (pre_mul, cam_mul, sizeof pre_mul);
    else
      pm_message ("Cannot use camera white balance.");
  }
  if (!use_coeff) {
    pre_mul[0] *= cmdline.red_scale;
    pre_mul[2] *= cmdline.blue_scale;
  }
  dmin = DBL_MAX;
  FORC4 if (dmin > pre_mul[c])
        dmin = pre_mul[c];
  FORC4 pre_mul[c] /= dmin;

  while (maximum << shift < 0x8000) shift++;
  FORC4 pre_mul[c] *= 1 << shift;
  maximum <<= shift;

  if (cmdline.linear || cmdline.bright < 1) {
      maximum *= cmdline.bright;
      if (maximum > 0xffff)
          maximum = 0xffff;
      FORC4 pre_mul[c] *= cmdline.bright;
  }
  if (cmdline.verbose) {
    fprintf (stderr, "Scaling with black=%d, pre_mul[] =", black);
    FORC4 fprintf (stderr, " %f", pre_mul[c]);
    fputc ('\n', stderr);
  }
  clip_max = cmdline.no_clip_color ? 0xffff : maximum;
  for (row=0; row < height; row++)
    for (col=0; col < width; col++)
      FORC4 {
    val = image[row*width+col][c];
    if (!val) continue;
    val -= black;
    val *= pre_mul[c];
    if (val < 0) val = 0;
    if (val > clip_max) val = clip_max;
    image[row*width+col][c] = val;
      }
}

/*
   This algorithm is officially called:

   "Interpolation using a Threshold-based variable number of gradients"

   described in http://www-ise.stanford.edu/~tingchen/algodep/vargra.html

   I've extended the basic idea to work with non-Bayer filter arrays.
   Gradients are numbered clockwise from NW=0 to W=7.
 */
static void CLASS vng_interpolate()
{
  static const signed char *cp, terms[] = {
    -2,-2,+0,-1,0,(char)0x01, -2,-2,+0,+0,1,(char)0x01, -2,-1,-1,+0,0,(char)0x01,
    -2,-1,+0,-1,0,(char)0x02, -2,-1,+0,+0,0,(char)0x03, -2,-1,+0,+1,1,(char)0x01,
    -2,+0,+0,-1,0,(char)0x06, -2,+0,+0,+0,1,(char)0x02, -2,+0,+0,+1,0,(char)0x03,
    -2,+1,-1,+0,0,(char)0x04, -2,+1,+0,-1,1,(char)0x04, -2,+1,+0,+0,0,(char)0x06,
    -2,+1,+0,+1,0,(char)0x02, -2,+2,+0,+0,1,(char)0x04, -2,+2,+0,+1,0,(char)0x04,
    -1,-2,-1,+0,0,(char)0x80, -1,-2,+0,-1,0,(char)0x01, -1,-2,+1,-1,0,(char)0x01,
    -1,-2,+1,+0,1,(char)0x01, -1,-1,-1,+1,0,(char)0x88, -1,-1,+1,-2,0,(char)0x40,
    -1,-1,+1,-1,0,(char)0x22, -1,-1,+1,+0,0,(char)0x33, -1,-1,+1,+1,1,(char)0x11,
    -1,+0,-1,+2,0,(char)0x08, -1,+0,+0,-1,0,(char)0x44, -1,+0,+0,+1,0,(char)0x11,
    -1,+0,+1,-2,1,(char)0x40, -1,+0,+1,-1,0,(char)0x66, -1,+0,+1,+0,1,(char)0x22,
    -1,+0,+1,+1,0,(char)0x33, -1,+0,+1,+2,1,(char)0x10, -1,+1,+1,-1,1,(char)0x44,
    -1,+1,+1,+0,0,(char)0x66, -1,+1,+1,+1,0,(char)0x22, -1,+1,+1,+2,0,(char)0x10,
    -1,+2,+0,+1,0,(char)0x04, -1,+2,+1,+0,1,(char)0x04, -1,+2,+1,+1,0,(char)0x04,
    +0,-2,+0,+0,1,(char)0x80, +0,-1,+0,+1,1,(char)0x88, +0,-1,+1,-2,0,(char)0x40,
    +0,-1,+1,+0,0,(char)0x11, +0,-1,+2,-2,0,(char)0x40, +0,-1,+2,-1,0,(char)0x20,
    +0,-1,+2,+0,0,(char)0x30, +0,-1,+2,+1,1,(char)0x10, +0,+0,+0,+2,1,(char)0x08,
    +0,+0,+2,-2,1,(char)0x40, +0,+0,+2,-1,0,(char)0x60, +0,+0,+2,+0,1,(char)0x20,
    +0,+0,+2,+1,0,(char)0x30, +0,+0,+2,+2,1,(char)0x10, +0,+1,+1,+0,0,(char)0x44,
    +0,+1,+1,+2,0,(char)0x10, +0,+1,+2,-1,1,(char)0x40, +0,+1,+2,+0,0,(char)0x60,
    +0,+1,+2,+1,0,(char)0x20, +0,+1,+2,+2,0,(char)0x10, +1,-2,+1,+0,0,(char)0x80,
    +1,-1,+1,+1,0,(char)0x88, +1,+0,+1,+2,0,(char)0x08, +1,+0,+2,-1,0,(char)0x40,
    +1,+0,+2,+1,0,(char)0x10
  }, chood[] = { -1,-1, -1,0, -1,+1, 0,+1, +1,+1, +1,0, +1,-1, 0,-1 };
  unsigned short (*brow[5])[4], *pix;
  int code[8][2][320], *ip, gval[8], gmin, gmax, sum[4];
  int row, col, shift, x, y, x1, x2, y1, y2, t, weight, grads, color, diag;
  int g, diff, thold, num, c;

  for (row=0; row < 8; row++) {     /* Precalculate for bilinear */
    for (col=1; col < 3; col++) {
      ip = code[row][col & 1];
      memset (sum, 0, sizeof sum);
      for (y=-1; y <= 1; y++)
    for (x=-1; x <= 1; x++) {
      shift = (y==0) + (x==0);
      if (shift == 2) continue;
      color = FC(row+y,col+x);
      *ip++ = (width*y + x)*4 + color;
      *ip++ = shift;
      *ip++ = color;
      sum[color] += 1 << shift;
    }
      FORC4
    if (c != FC(row,col)) {
      *ip++ = c;
      *ip++ = sum[c];
    }
    }
  }
  for (row=1; row < height-1; row++) {  /* Do bilinear interpolation */
    for (col=1; col < width-1; col++) {
      pix = image[row*width+col];
      ip = code[row & 7][col & 1];
      memset (sum, 0, sizeof sum);
      for (g=8; g--; ) {
    diff = pix[*ip++];
    diff <<= *ip++;
    sum[*ip++] += diff;
      }
      for (g=colors; --g; ) {
    c = *ip++;
    pix[c] = sum[c] / *ip++;
      }
    }
  }
  if (cmdline.quick_interpolate)
    return;

  for (row=0; row < 8; row++) {     /* Precalculate for VNG */
    for (col=0; col < 2; col++) {
      ip = code[row][col];
      for (cp=terms, t=0; t < 64; t++) {
    y1 = *cp++;  x1 = *cp++;
    y2 = *cp++;  x2 = *cp++;
    weight = *cp++;
    grads = *cp++;
    color = FC(row+y1,col+x1);
    if (FC(row+y2,col+x2) != color) continue;
    diag = (FC(row,col+1) == color && FC(row+1,col) == color) ? 2:1;
    if (abs(y1-y2) == diag && abs(x1-x2) == diag) continue;
    *ip++ = (y1*width + x1)*4 + color;
    *ip++ = (y2*width + x2)*4 + color;
    *ip++ = weight;
    for (g=0; g < 8; g++)
      if (grads & 1<<g) *ip++ = g;
    *ip++ = -1;
      }
      *ip++ = INT_MAX;
      for (cp=chood, g=0; g < 8; g++) {
    y = *cp++;  x = *cp++;
    *ip++ = (y*width + x) * 4;
    color = FC(row,col);
    if (FC(row+y,col+x) != color && FC(row+y*2,col+x*2) == color)
      *ip++ = (y*width + x) * 8 + color;
    else
      *ip++ = 0;
      }
    }
  }
  brow[4] = calloc (width*3, sizeof **brow);
  merror (brow[4], "vng_interpolate()");
  for (row=0; row < 3; row++)
    brow[row] = brow[4] + row*width;
  for (row=2; row < height-2; row++) {      /* Do VNG interpolation */
    for (col=2; col < width-2; col++) {
      pix = image[row*width+col];
      ip = code[row & 7][col & 1];
      memset (gval, 0, sizeof gval);
      while ((g = ip[0]) != INT_MAX) {      /* Calculate gradients */
    num = (diff = pix[g] - pix[ip[1]]) >> 31;
    gval[ip[3]] += (diff = ((diff ^ num) - num) << ip[2]);
    ip += 5;
    if ((g = ip[-1]) == -1) continue;
    gval[g] += diff;
    while ((g = *ip++) != -1)
      gval[g] += diff;
      }
      ip++;
      gmin = gmax = gval[0];            /* Choose a threshold */
      for (g=1; g < 8; g++) {
    if (gmin > gval[g]) gmin = gval[g];
    if (gmax < gval[g]) gmax = gval[g];
      }
      if (gmax == 0) {
    memcpy (brow[2][col], pix, sizeof *image);
    continue;
      }
      thold = gmin + (gmax >> 1);
      memset (sum, 0, sizeof sum);
      color = FC(row,col);
      for (num=g=0; g < 8; g++,ip+=2) {     /* Average the neighbors */
    if (gval[g] <= thold) {
      FORC4
        if (c == color && ip[1])
          sum[c] += (pix[c] + pix[ip[1]]) >> 1;
        else
          sum[c] += pix[ip[0] + c];
      num++;
    }
      }
      FORC4 {                   /* Save to buffer */
    t = pix[color];
    if (c != color) {
      t += (sum[c] - sum[color])/num;
      if (t < 0) t = 0;
      if (t > clip_max) t = clip_max;
    }
    brow[2][col][c] = t;
      }
    }
    if (row > 3)                /* Write buffer to image */
      memcpy (image[(row-2)*width+2], brow[0]+2, (width-4)*sizeof *image);
    for (g=0; g < 4; g++)
      brow[(g-1) & 3] = brow[g];
  }
  memcpy (image[(row-2)*width+2], brow[0]+2, (width-4)*sizeof *image);
  memcpy (image[(row-1)*width+2], brow[1]+2, (width-4)*sizeof *image);
  free (brow[4]);
}

#ifdef USE_LCMS
static void 
apply_profile(FILE *       const ifP,
              const char * const pfname)
{
  char *prof;
  cmsHPROFILE hInProfile=NULL, hOutProfile;
  cmsHTRANSFORM hTransform;

  if (pfname)
    hInProfile = cmsOpenProfileFromFile (pfname, "r");
  else if (profile_length) {
    prof = malloc (profile_length);
    merror (prof, "apply_profile()");
    fseek (ifP, profile_offset, SEEK_SET);
    fread (prof, 1, profile_length, ifP);
    hInProfile = cmsOpenProfileFromMem (prof, profile_length);
    free (prof);
  }
  if (!hInProfile) return;
  if (cmdline.verbose)
      pm_message( "Applying color profile...");
  maximum = 0xffff;
  use_gamma = use_coeff = 0;

  hOutProfile = cmsCreate_sRGBProfile();
  hTransform = cmsCreateTransform (hInProfile, TYPE_RGBA_16,
    hOutProfile, TYPE_RGBA_16, INTENT_PERCEPTUAL, 0);
  cmsDoTransform (hTransform, image, image, width*height);

  cmsDeleteTransform (hTransform);
  cmsCloseProfile (hInProfile);
  cmsCloseProfile (hOutProfile);
}
#else
static void 
apply_profile(FILE *       const ifP,
              const char * const pfname)
{
}
#endif

/*
   Convert the entire image to RGB colorspace and build a histogram.
 */
static void CLASS convert_to_rgb()
{
  int row, col, r, g, c=0;
  unsigned short *img;
  float rgb[3];

  if (cmdline.document_mode)
    colors = 1;
  memset (histogram, 0, sizeof histogram);
  for (row = trim; row < height-trim; row++)
    for (col = trim; col < width-trim; col++) {
      img = image[row*width+col];
      if (cmdline.document_mode)
    c = FC(row,col);
      if (colors == 4 && !use_coeff)    /* Recombine the greens */
    img[1] = (img[1] + img[3]) >> 1;
      if (colors == 1)          /* RGB from grayscale */
    for (r=0; r < 3; r++)
      rgb[r] = img[c];
      else if (use_coeff) {     /* RGB via coeff[][] */
    for (r=0; r < 3; r++)
      for (rgb[r]=g=0; g < colors; g++)
        rgb[r] += img[g] * coeff[r][g];
      } else                /* RGB from RGB (easy) */
    goto norgb;
      for (r=0; r < 3; r++) {
    if (rgb[r] < 0)        rgb[r] = 0;
    if (rgb[r] > clip_max) rgb[r] = clip_max;
    img[r] = rgb[r];
      }
norgb:
      for (r=0; r < 3; r++)
    histogram[r][img[r] >> 3]++;
    }
}

static void CLASS fuji_rotate()
{
  int i, wide, high, row, col;
  double step;
  float r, c, fr, fc;
  unsigned ur, uc;
  unsigned short (*img)[4], (*pix)[4];

  if (!fuji_width) return;
  if (cmdline.verbose)
    pm_message ("Rotating image 45 degrees...");
  fuji_width = (fuji_width + shrink) >> shrink;
  step = sqrt(0.5);
  wide = fuji_width / step;
  high = (height - fuji_width) / step;
  img = calloc (wide*high, sizeof *img);
  merror (img, "fuji_rotate()");

  for (row=0; row < high; row++)
    for (col=0; col < wide; col++) {
      ur = r = fuji_width + (row-col)*step;
      uc = c = (row+col)*step;
      if (ur > height-2 || uc > width-2) continue;
      fr = r - ur;
      fc = c - uc;
      pix = image + ur*width + uc;
      for (i=0; i < colors; i++)
    img[row*wide+col][i] =
      (pix[    0][i]*(1-fc) + pix[      1][i]*fc) * (1-fr) +
      (pix[width][i]*(1-fc) + pix[width+1][i]*fc) * fr;
    }
  free (image);
  width  = wide;
  height = high;
  image  = img;
  fuji_width = 0;
}

static void CLASS flip_image()
{
    unsigned *flag;
    int size, base, dest, next, row, col, temp;

    struct imageCell {
        unsigned char contents[8];
    };
    struct imageCell * img;
    struct imageCell hold;

    switch ((flip+3600) % 360) {
    case 270:  flip = 5;  break;
    case 180:  flip = 3;  break;
    case  90:  flip = 6;
    }
    img = (struct imageCell *) image;
    size = height * width;
    flag = calloc ((size+31) >> 5, sizeof *flag);
    merror (flag, "flip_image()");
    for (base = 0; base < size; base++) {
        if (flag[base >> 5] & (1 << (base & 31)))
            continue;
        dest = base;
        hold = img[base];
        while (1) {
            if (flip & 4) {
                row = dest % height;
                col = dest / height;
            } else {
                row = dest / width;
                col = dest % width;
            }
            if (flip & 2)
                row = height - 1 - row;
            if (flip & 1)
                col = width - 1 - col;
            next = row * width + col;
            if (next == base) break;
            flag[next >> 5] |= 1 << (next & 31);
            img[dest] = img[next];
            dest = next;
        }
        img[dest] = hold;
    }
    free (flag);
    if (flip & 4) {
        temp = height;
        height = width;
        width = temp;
        temp = ymag;
        ymag = xmag;
        xmag = temp;
    }
}

/*
   Write the image as an RGB PAM image
 */
static void CLASS write_pam_nonlinear (FILE *ofp)
{
  unsigned char lut[0x10000];
  int perc, c, val, total, i, row, col;
  float white=0, r;
  struct pam pam;
  tuple * tuplerow;

  pam.size   = sizeof(pam);
  pam.len    = PAM_STRUCT_SIZE(tuple_type);
  pam.file   = ofp;
  pam.width  = xmag*(width-trim*2);
  pam.height = ymag*(height-trim*2);
  pam.depth  = 3;
  pam.format = PAM_FORMAT;
  pam.maxval = 255;
  strcpy(pam.tuple_type, "RGB");

  pnm_writepaminit(&pam);

  tuplerow = pnm_allocpamrow(&pam);

  perc = width * height * 0.01;     /* 99th percentile white point */
  if (fuji_width) perc /= 2;
  FORC3 {
    for (val=0x2000, total=0; --val > 32; )
      if ((total += histogram[c][val]) > perc) break;
    if (white < val) white = val;
  }
  white *= 8 / cmdline.bright;
  for (i=0; i < 0x10000; i++) {
    r = i / white;
    val = 256 * ( !use_gamma ? r :
    r <= 0.018 ? r*4.5 : pow(r,0.45)*1.099-0.099 );
    if (val > 255) val = 255;
    lut[i] = val;
  }
  for (row=trim; row < height-trim; row++) {
      for (col=trim; col < width-trim; col++) {
          unsigned int plane;
          for (plane=0; plane < pam.depth; ++plane) {
              unsigned int copy;
              for (copy=0; copy < xmag; ++copy) {
                  unsigned int const pamcol = xmag*(col-trim)+copy;
                  tuplerow[pamcol][plane] = lut[image[row*width+col][plane]];
              }
          }
      }
      {
          unsigned int copy;
          for (copy=0; copy < ymag; ++copy)
              pnm_writepamrow(&pam, tuplerow);
      }
  }
  pnm_freepamrow(tuplerow);
}

/*
   Write the image to a 16-bit PAM file with linear color space
 */
static void CLASS write_pam_linear (FILE *ofp)
{
  int row;

  struct pam pam;
  tuple * tuplerow;

  if (maximum < 256) maximum = 256;

  pam.size   = sizeof(pam);
  pam.len    = PAM_STRUCT_SIZE(tuple_type);
  pam.file   = ofp;
  pam.width  = width-trim*2;
  pam.height = height-trim*2;
  pam.depth  = 3;
  pam.format = PAM_FORMAT;
  pam.maxval = MAX(maximum, 256);
  strcpy(pam.tuple_type, "RGB");

  pnm_writepaminit(&pam);

  tuplerow = pnm_allocpamrow(&pam);

  for (row = trim; row < height-trim; row++) {
      unsigned int col;
      for (col = trim; col < width-trim; col++) {
          unsigned int const pamCol = col - trim;
          unsigned int plane;
          for (plane = 0; plane < 3; ++plane)
              tuplerow[pamCol][plane] = image[row*width+col][plane];
      }
      pnm_writepamrow(&pam, tuplerow);
  }
  pnm_freepamrow(tuplerow);
}



static void CLASS
writePam(FILE * const ofP,
         bool   const linear) {

    if (linear)
        write_pam_linear(ofP);
    else
        write_pam_nonlinear(ofP);
}




static void CLASS
convertIt(FILE *    const ifP,
          FILE *    const ofP,
          loadRawFn const load_raw) {

    shrink = cmdline.half_size && filters;
    iheight = (height + shrink) >> shrink;
    iwidth  = (width  + shrink) >> shrink;
    image = calloc (iheight*iwidth*sizeof *image + meta_length, 1);
    merror (image, "main()");
    meta_data = (char *) (image + iheight*iwidth);
    if (cmdline.verbose)
        pm_message ("Loading %s %s image ...", make, model);

    use_secondary = cmdline.use_secondary;  /* for load_raw() */

    ifp = ifP;  /* Set global variable for (*load_raw)() */

    load_raw();
    bad_pixels();
    height = iheight;
    width  = iwidth;
    if (is_foveon) {
        if (cmdline.verbose)
            pm_message ("Foveon interpolation...");
        foveon_interpolate(coeff);
    } else {
        scale_colors();
    }
    if (shrink) filters = 0;
    trim = 0;
    if (filters && !cmdline.document_mode) {
        trim = 1;
        if (cmdline.verbose)
            pm_message ("%s interpolation...",
                        cmdline.quick_interpolate ? "Bilinear":"VNG");
        vng_interpolate();
    }
    fuji_rotate();
    apply_profile(ifP, cmdline.profile);
    if (cmdline.verbose)
        pm_message ("Converting to RGB colorspace...");
    convert_to_rgb();

    if (flip) {
        if (cmdline.verbose)
            pm_message ("Flipping image %c:%c:%c...",
                        flip & 1 ? 'H':'0', flip & 2 ? 'V':'0', 
                        flip & 4 ? 'T':'0');
        flip_image();
    }
    writePam(ofP, cmdline.linear);
}


int 
main (int argc, char **argv) {

    FILE * const ofP = stdout;

    FILE * ifP;
    int rc;
    loadRawFn load_raw;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    verbose = cmdline.verbose;

    ifP = pm_openr(cmdline.inputFileName);

    image = NULL;
    
    rc = identify(ifP,
                  cmdline.use_secondary, cmdline.use_camera_rgb,
                  cmdline.red_scale, cmdline.blue_scale,
                  cmdline.four_color_rgb, cmdline.inputFileName,
                  &load_raw);
    if (rc != 0)
        pm_error("Unable to identify the format of the input image");
    else {
        if (cmdline.identify_only) {
            pm_message ("Input is a %s %s image.", make, model);
        } else {
            if (cmdline.verbose)
                pm_message ("Input is a %s %s image.", make, model);
            convertIt(ifP, ofP, load_raw);
        }
    }
    pm_close(ifP);
    pm_close(ofP);
    free(image);

    return 0;
}



