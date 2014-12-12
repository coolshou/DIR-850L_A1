/*
** pngtopnm.c -
** read a Portable Network Graphics file and produce a portable anymap
**
** Copyright (C) 1995,1998 by Alexander Lehmann <alex@hal.rhein-main.de>
**                        and Willem van Schaik <willem@schaik.com>
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** modeled after giftopnm by David Koblas and
** with lots of bits pasted from libpng.txt by Guy Eric Schalnat
*/

/* 
   BJH 20000408:  rename PPM_MAXMAXVAL to PPM_OVERALLMAXVAL
   BJH 20000303:  fix include statement so dependencies work out right.
*/
/* GRR 19991203:  moved VERSION to new version.h header file */

/* GRR 19990713:  fixed redundant freeing of png_ptr and info_ptr in setjmp()
 *  blocks and added "pm_close(ifp)" in each.  */

/* GRR 19990317:  declared "clobberable" automatic variables in convertpng()
 *  static to fix Solaris/gcc stack-corruption bug.  Also installed custom
 *  error-handler to avoid jmp_buf size-related problems (i.e., jmp_buf
 *  compiled with one size in libpng and another size here).  */

#ifndef PNMTOPNG_WARNING_LEVEL
#  define PNMTOPNG_WARNING_LEVEL 0   /* use 0 for backward compatibility, */
#endif                               /*  2 for warnings (1 == error) */

#include <math.h>
#include <float.h>
#include <png.h>    /* includes zlib.h and setjmp.h */
#define VERSION "2.37.4 (5 December 1999) +netpbm"

#include "pnm.h"
#include "mallocvar.h"
#include "nstring.h"
#include "shhopt.h"

#if PNG_LIBPNG_VER >= 10400
#error Your PNG library (<png.h>) is incompatible with this Netpbm source code.
#error You need either an older PNG library (older than 1.4)
#error newer Netpbm source code (at least 10.48)
#endif

typedef struct _jmpbuf_wrapper {
  jmp_buf jmpbuf;
} jmpbuf_wrapper;

/* GRR 19991205:  this is used as a test for pre-1999 versions of netpbm and
 *   pbmplus vs. 1999 or later (in which pm_close was split into two)
 */
#ifdef PBMPLUS_RAWBITS
#  define pm_closer pm_close
#  define pm_closew pm_close
#endif

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif
#ifndef NONE
#  define NONE 0
#endif

enum alpha_handling {ALPHA_NONE, ALPHA_ONLY, ALPHA_MIX};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* '-' if stdin */
    unsigned int verbose;
    enum alpha_handling alpha;
    const char * background;
    float gamma;  /* -1.0 means unspecified */
    const char * text;
    unsigned int time;
};


typedef struct pngcolor {
/*----------------------------------------------------------------------------
   A color in a format compatible with the PNG library.

   Note that the PNG library declares types png_color and png_color_16
   which are similar.
-----------------------------------------------------------------------------*/
    png_uint_16 r;
    png_uint_16 g;
    png_uint_16 b;
} pngcolor;


static png_uint_16 maxval;
static int verbose = FALSE;
static int mtime;
static jmpbuf_wrapper pngtopnm_jmpbuf_struct;


static void
parseCommandLine(int                 argc, 
                 char **             argv,
                 struct cmdlineInfo *cmdlineP ) {
/*----------------------------------------------------------------------------
   Parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int alphaSpec, mixSpec, backgroundSpec, gammaSpec, textSpec;

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "verbose",     OPT_FLAG,   NULL,                  
            &cmdlineP->verbose,       0);
    OPTENT3(0, "alpha",       OPT_FLAG,   NULL,                  
            &alphaSpec,               0);
    OPTENT3(0, "mix",         OPT_FLAG,   NULL,                  
            &mixSpec,                 0);
    OPTENT3(0, "background",  OPT_STRING, &cmdlineP->background,
            &backgroundSpec,          0);
    OPTENT3(0, "gamma",       OPT_FLOAT,  &cmdlineP->gamma,
            &gammaSpec,               0);
    OPTENT3(0, "text",        OPT_STRING, &cmdlineP->text,
            &textSpec,                0);
    OPTENT3(0, "time",        OPT_FLAG,   NULL,                  
            &cmdlineP->time,          0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (alphaSpec && mixSpec)
        pm_error("You cannot specify both -alpha and -mix");
    else if (alphaSpec)
        cmdlineP->alpha = ALPHA_ONLY;
    else if (mixSpec)
        cmdlineP->alpha = ALPHA_MIX;
    else
        cmdlineP->alpha = ALPHA_NONE;

    if (backgroundSpec && !mixSpec)
        pm_error("-background is useless without -mix");

    if (!backgroundSpec)
        cmdlineP->background = NULL;

    if (!gammaSpec)
        cmdlineP->gamma = -1.0;

    if (!textSpec)
        cmdlineP->text = NULL;

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else
        pm_error("Program takes at most one argument: input file name.  "
            "you specified %d", argc-1);
}




#define get_png_val(p) _get_png_val (&(p), info_ptr->bit_depth)

static png_uint_16
_get_png_val (png_byte ** const pp,
              int         const bit_depth) {

    png_uint_16 c;
    
    if (bit_depth == 16)
        c = (*((*pp)++)) << 8;
    else
        c = 0;

    c |= (*((*pp)++));
    
    return c;
}



static bool
isGrayscale(pngcolor const color) {

    return color.r == color.g && color.r == color.b;
}



static void 
setXel(xel *               const xelP, 
       pngcolor            const foreground,
       pngcolor            const background,
       enum alpha_handling const alpha_handling,
       png_uint_16         const alpha) {

    if (alpha_handling == ALPHA_ONLY) {
        PNM_ASSIGN1(*xelP, alpha);
    } else {
        if ((alpha_handling == ALPHA_MIX) && (alpha != maxval)) {
            double const opacity      = (double)alpha / maxval;
            double const transparency = 1.0 - opacity;

            pngcolor mix;

            mix.r = foreground.r * opacity + background.r * transparency + 0.5;
            mix.g = foreground.g * opacity + background.g * transparency + 0.5;
            mix.b = foreground.b * opacity + background.b * transparency + 0.5;
            PPM_ASSIGN(*xelP, mix.r, mix.g, mix.b);
        } else
            PPM_ASSIGN(*xelP, foreground.r, foreground.g, foreground.b);
    }
}



static png_uint_16
gamma_correct(png_uint_16 const v,
              float       const g) {

    if (g != -1.0)
        return (png_uint_16) (pow ((double) v / maxval, 
                                   (1.0 / g)) * maxval + 0.5);
    else
        return v;
}



#ifdef __STDC__
static int iscolor (png_color c)
#else
static int iscolor (c)
png_color c;
#endif
{
  return c.red != c.green || c.green != c.blue;
}

#ifdef __STDC__
static void save_text (png_info *info_ptr, FILE *tfp)
#else
static void save_text (info_ptr, tfp)
png_info *info_ptr;
FILE *tfp;
#endif
{
  int i, j, k;

  for (i = 0 ; i < info_ptr->num_text ; i++) {
    j = 0;
    while (info_ptr->text[i].key[j] != '\0' && info_ptr->text[i].key[j] != ' ')
      j++;    
    if (info_ptr->text[i].key[j] != ' ') {
      fprintf (tfp, "%s", info_ptr->text[i].key);
      for (j = strlen (info_ptr->text[i].key) ; j < 15 ; j++)
        putc (' ', tfp);
    } else {
      fprintf (tfp, "\"%s\"", info_ptr->text[i].key);
      for (j = strlen (info_ptr->text[i].key) ; j < 13 ; j++)
        putc (' ', tfp);
    }
    putc (' ', tfp); /* at least one space between key and text */
    
    for (j = 0 ; j < info_ptr->text[i].text_length ; j++) {
      putc (info_ptr->text[i].text[j], tfp);
      if (info_ptr->text[i].text[j] == '\n')
        for (k = 0 ; k < 16 ; k++)
          putc ((int)' ', tfp);
    }
    putc ((int)'\n', tfp);
  }
}

#ifdef __STDC__
static void show_time (png_info *info_ptr)
#else
static void show_time (info_ptr)
png_info *info_ptr;
#endif
{
    static const char * const month[] = {
        "", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

  if (info_ptr->valid & PNG_INFO_tIME) {
    pm_message ("modification time: %02d %s %d %02d:%02d:%02d",
                info_ptr->mod_time.day, month[info_ptr->mod_time.month],
                info_ptr->mod_time.year, info_ptr->mod_time.hour,
                info_ptr->mod_time.minute, info_ptr->mod_time.second);
  }
}

#ifdef __STDC__
static void pngtopnm_error_handler (png_structp png_ptr, png_const_charp msg)
#else
static void pngtopnm_error_handler (png_ptr, msg)
png_structp png_ptr;
png_const_charp msg;
#endif
{
  jmpbuf_wrapper  *jmpbuf_ptr;

  /* this function, aside from the extra step of retrieving the "error
   * pointer" (below) and the fact that it exists within the application
   * rather than within libpng, is essentially identical to libpng's
   * default error handler.  The second point is critical:  since both
   * setjmp() and longjmp() are called from the same code, they are
   * guaranteed to have compatible notions of how big a jmp_buf is,
   * regardless of whether _BSD_SOURCE or anything else has (or has not)
   * been defined. */

  pm_message("fatal libpng error: %s", msg);

  jmpbuf_ptr = png_get_error_ptr(png_ptr);
  if (jmpbuf_ptr == NULL) {
      /* we are completely hosed now */
      pm_error("EXTREMELY fatal error: jmpbuf unrecoverable; terminating.");
  }

  longjmp(jmpbuf_ptr->jmpbuf, 1);
}



static void
dump_png_info(png_info *info_ptr) {

    const char *type_string;
    const char *filter_string;

    switch (info_ptr->color_type) {
      case PNG_COLOR_TYPE_GRAY:
        type_string = "gray";
        break;

      case PNG_COLOR_TYPE_GRAY_ALPHA:
        type_string = "gray+alpha";
        break;

      case PNG_COLOR_TYPE_PALETTE:
        type_string = "palette";
        break;

      case PNG_COLOR_TYPE_RGB:
        type_string = "truecolor";
        break;

      case PNG_COLOR_TYPE_RGB_ALPHA:
        type_string = "truecolor+alpha";
        break;
    }

    switch (info_ptr->filter_type) {
    case PNG_FILTER_TYPE_BASE:
        asprintfN(&filter_string, "base filter");
        break;
    default:
        asprintfN(&filter_string, "unknown filter type %d", 
                  info_ptr->filter_type);
    }

    pm_message("reading a %ldw x %ldh image, %d bit%s",
               info_ptr->width, info_ptr->height,
               info_ptr->bit_depth, info_ptr->bit_depth > 1 ? "s" : "");
    pm_message("%s, %s, %s",
               type_string,
               info_ptr->interlace_type ? 
               "Adam7 interlaced" : "not interlaced",
               filter_string);
    pm_message("background {index, gray, red, green, blue} = "
               "{%d, %d, %d, %d, %d}",
               info_ptr->background.index,
               info_ptr->background.gray,
               info_ptr->background.red,
               info_ptr->background.green,
               info_ptr->background.blue);

    strfree(filter_string);

    if (info_ptr->valid & PNG_INFO_tRNS)
        pm_message("tRNS chunk (transparency): %u entries",
                   info_ptr->num_trans);
    else
        pm_message("tRNS chunk (transparency): not present");

    if (info_ptr->valid & PNG_INFO_gAMA)
        pm_message("gAMA chunk (image gamma): gamma = %4.2f", info_ptr->gamma);
    else
        pm_message("gAMA chunk (image gamma): not present");

    if (info_ptr->valid & PNG_INFO_sBIT)
        pm_message("sBIT chunk: present");
    else
        pm_message("sBIT chunk: not present");

    if (info_ptr->valid & PNG_INFO_cHRM)
        pm_message("cHRM chunk: present");
    else
        pm_message("cHRM chunk: not present");

    if (info_ptr->valid & PNG_INFO_PLTE)
        pm_message("PLTE chunk: %d entries", info_ptr->num_palette);
    else
        pm_message("PLTE chunk: not present");

    if (info_ptr->valid & PNG_INFO_bKGD)
        pm_message("bKGD chunk: present");
    else
        pm_message("bKGD chunk: not present");

    if (info_ptr->valid & PNG_INFO_hIST)
        pm_message("hIST chunk: present");
    else
        pm_message("hIST chunk: not present");

    if (info_ptr->valid & PNG_INFO_pHYs)
        pm_message("pHYs chunk: present");
    else
        pm_message("pHYs chunk: not present");

    if (info_ptr->valid & PNG_INFO_oFFs)
        pm_message("oFFs chunk: present");
    else
        pm_message("oFFs chunk: not present");

    if (info_ptr->valid & PNG_INFO_tIME)
        pm_message("tIME chunk: present");
    else
        pm_message("tIME chunk: not present");

    if (info_ptr->valid & PNG_INFO_pCAL)
        pm_message("pCAL chunk: present");
    else
        pm_message("pCAL chunk: not present");

    if (info_ptr->valid & PNG_INFO_sRGB)
        pm_message("sRGB chunk: present");
    else
        pm_message("sRGB chunk: not present");
}



static bool
isTransparentColor(pngcolor   const color,
                   png_info * const info_ptr,
                   double     const totalgamma) {
/*----------------------------------------------------------------------------
   Return TRUE iff pixels of color 'color' are supposed to be transparent
   everywhere they occur.  Assume it's an RGB image.
-----------------------------------------------------------------------------*/
    bool retval;

    if (info_ptr->valid & PNG_INFO_tRNS) {
        const png_color_16 * const transColorP = &info_ptr->trans_values;
    

        /* There seems to be a problem here: you can't compare real
           numbers for equality.  Also, I'm not sure the gamma
           corrected/uncorrected color spaces are right here.  
        */

        retval = 
            color.r == gamma_correct(transColorP->red,   totalgamma) &&
            color.g == gamma_correct(transColorP->green, totalgamma) &&
            color.b == gamma_correct(transColorP->blue,  totalgamma);
    } else 
        retval = FALSE;

    return retval;
}



#define SIG_CHECK_SIZE 4

static void
read_sig_buf(FILE * const ifP) {

    unsigned char sig_buf[SIG_CHECK_SIZE];
    size_t bytesRead;

    bytesRead = fread(sig_buf, 1, SIG_CHECK_SIZE, ifP);
    if (bytesRead != SIG_CHECK_SIZE)
        pm_error ("input file is empty or too short");

    if (png_sig_cmp(sig_buf, (png_size_t) 0, (png_size_t) SIG_CHECK_SIZE)
        != 0)
        pm_error ("input file is not a PNG file");
}



static void
setupGammaCorrection(png_struct * const png_ptr,
                     png_info *   const info_ptr,
                     float        const displaygamma,
                     float *      const totalgammaP) {

    if (displaygamma == -1.0)
        *totalgammaP = -1.0;
    else {
        float imageGamma;
        if (info_ptr->valid & PNG_INFO_gAMA)
            imageGamma = info_ptr->gamma;
        else {
            if (verbose)
                pm_message("PNG doesn't specify image gamma.  Assuming 1.0");
            imageGamma = 1.0;
        }

        if (fabs(displaygamma * imageGamma - 1.0) < .01) {
            *totalgammaP = -1.0;
            if (verbose)
                pm_message("image gamma %4.2f matches "
                           "display gamma %4.2f.  No conversion.",
                           imageGamma, displaygamma);
        } else {
            png_set_gamma(png_ptr, displaygamma, imageGamma);
            *totalgammaP = imageGamma * displaygamma;
            /* in case of gamma-corrections, sBIT's as in the
               PNG-file are not valid anymore 
            */
            info_ptr->valid &= ~PNG_INFO_sBIT;
            if (verbose)
                pm_message("image gamma is %4.2f, "
                           "converted for display gamma of %4.2f",
                           imageGamma, displaygamma);
        }
    }
}



static bool
paletteHasPartialTransparency(png_info * const info_ptr) {

    bool retval;

    if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
        if (info_ptr->valid & PNG_INFO_tRNS) {
            bool foundGray;
            unsigned int i;
            
            for (i = 0, foundGray = FALSE;
                 i < info_ptr->num_trans && !foundGray;
                 ++i) {
                if (info_ptr->trans[i] != 0 &&
                    info_ptr->trans[i] != maxval) {
                    foundGray = TRUE;
                }
            }
            retval = foundGray;
        } else
            retval = FALSE;
    } else
        retval = FALSE;

    return retval;
}



static void
setupSignificantBits(png_struct *        const png_ptr,
                     png_info *          const info_ptr,
                     enum alpha_handling const alpha,
                     png_uint_16 *       const maxvalP,
                     int *               const errorlevelP) {
/*----------------------------------------------------------------------------
  Figure out what maxval would best express the information in the PNG
  described by 'png_ptr' and 'info_ptr', with 'alpha' telling which
  information in the PNG we care about (image or alpha mask).

  Return the result as *maxvalP.
-----------------------------------------------------------------------------*/
    /* Initial assumption of maxval */
    if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
        if (alpha == ALPHA_ONLY) {
            if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY ||
                info_ptr->color_type == PNG_COLOR_TYPE_RGB)
                /* The alpha mask will be all opaque, so maxval 1 is plenty */
                *maxvalP = 1;
            else if (paletteHasPartialTransparency(info_ptr))
                /* Use same maxval as PNG transparency palette for simplicity*/
                *maxvalP = 255;
            else
                /* A common case, so we conserve bits */
                *maxvalP = 1;
        } else
            /* Use same maxval as PNG palette for simplicity */
            *maxvalP = 255;
    } else {
        *maxvalP = (1l << info_ptr->bit_depth) - 1;
    }

    /* sBIT handling is very tricky. If we are extracting only the
       image, we can use the sBIT info for grayscale and color images,
       if the three values agree. If we extract the transparency/alpha
       mask, sBIT is irrelevant for trans and valid for alpha. If we
       mix both, the multiplication may result in values that require
       the normal bit depth, so we will use the sBIT info only for
       transparency, if we know that only solid and fully transparent
       is used 
    */
    
    if (info_ptr->valid & PNG_INFO_sBIT) {
        switch (alpha) {
        case ALPHA_MIX:
            if (info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
                info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                break;
            if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
                (info_ptr->valid & PNG_INFO_tRNS)) {

                bool trans_mix;
                unsigned int i;
                trans_mix = TRUE;
                for (i = 0; i < info_ptr->num_trans; ++i)
                    if (info_ptr->trans[i] != 0 && info_ptr->trans[i] != 255) {
                        trans_mix = FALSE;
                        break;
                    }
                if (!trans_mix)
                    break;
            }

            /* else fall though to normal case */

        case ALPHA_NONE:
            if ((info_ptr->color_type == PNG_COLOR_TYPE_PALETTE ||
                 info_ptr->color_type == PNG_COLOR_TYPE_RGB ||
                 info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA) &&
                (info_ptr->sig_bit.red != info_ptr->sig_bit.green ||
                 info_ptr->sig_bit.red != info_ptr->sig_bit.blue) &&
                alpha == ALPHA_NONE) {
                pm_message("This program cannot handle "
                           "different bit depths for color channels");
                pm_message("writing file with %d bit resolution",
                           info_ptr->bit_depth);
                *errorlevelP = PNMTOPNG_WARNING_LEVEL;
            } else {
                if ((info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) &&
                    (info_ptr->sig_bit.red < 255)) {
                    unsigned int i;
                    for (i = 0; i < info_ptr->num_palette; ++i) {
                        info_ptr->palette[i].red   >>=
                            (8 - info_ptr->sig_bit.red);
                        info_ptr->palette[i].green >>=
                            (8 - info_ptr->sig_bit.green);
                        info_ptr->palette[i].blue  >>=
                            (8 - info_ptr->sig_bit.blue);
                    }
                    *maxvalP = (1l << info_ptr->sig_bit.red) - 1;
                    if (verbose)
                        pm_message ("image has fewer significant bits, "
                                    "writing file with %d bits per channel", 
                                    info_ptr->sig_bit.red);
                } else
                    if ((info_ptr->color_type == PNG_COLOR_TYPE_RGB ||
                         info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA) &&
                        (info_ptr->sig_bit.red < info_ptr->bit_depth)) {
                        png_set_shift (png_ptr, &(info_ptr->sig_bit));
                        *maxvalP = (1l << info_ptr->sig_bit.red) - 1;
                        if (verbose)
                            pm_message("image has fewer significant bits, "
                                       "writing file with %d "
                                       "bits per channel", 
                                       info_ptr->sig_bit.red);
                    } else 
                        if ((info_ptr->color_type == PNG_COLOR_TYPE_GRAY ||
                             info_ptr->color_type ==
                                 PNG_COLOR_TYPE_GRAY_ALPHA) &&
                            (info_ptr->sig_bit.gray < info_ptr->bit_depth)) {
                            png_set_shift (png_ptr, &(info_ptr->sig_bit));
                            *maxvalP = (1l << info_ptr->sig_bit.gray) - 1;
                            if (verbose)
                                pm_message("image has fewer significant bits, "
                                           "writing file with %d bits",
                                           info_ptr->sig_bit.gray);
                        }
            }
            break;

        case ALPHA_ONLY:
            if ((info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
                 info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA) && 
                (info_ptr->sig_bit.gray < info_ptr->bit_depth)) {
                png_set_shift (png_ptr, &(info_ptr->sig_bit));
                if (verbose)
                    pm_message ("image has fewer significant bits, "
                                "writing file with %d bits", 
                                info_ptr->sig_bit.alpha);
                *maxvalP = (1l << info_ptr->sig_bit.alpha) - 1;
            }
            break;

        }
    }
}



static bool
imageHasColor(png_info * const info_ptr) {

    bool retval;

    if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY ||
        info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)

        retval = FALSE;
    else if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
        bool foundColor;
        unsigned int i;
            
        for (i = 0, foundColor = FALSE;
             i < info_ptr->num_palette && !foundColor;
             ++i) {
            if (iscolor(info_ptr->palette[i]))
                foundColor = TRUE;
        }
        retval = foundColor;
    } else
        retval = TRUE;

    return retval;
}



static void
determineOutputType(png_info *          const info_ptr,
                    enum alpha_handling const alphaHandling,
                    pngcolor            const bgColor,
                    xelval              const maxval,
                    int *               const pnmTypeP) {

    if (alphaHandling != ALPHA_ONLY &&
        (imageHasColor(info_ptr) || !isGrayscale(bgColor)))
        *pnmTypeP = PPM_TYPE;
    else {
        if (maxval > 1)
            *pnmTypeP = PGM_TYPE;
        else
            *pnmTypeP = PBM_TYPE;
    }
}



static void
getBackgroundColor(png_info *        const info_ptr,
                   const char *      const requestedColor,
                   float             const totalgamma,
                   xelval            const maxval,
                   struct pngcolor * const bgColorP) {
/*----------------------------------------------------------------------------
   Figure out what the background color should be.  If the user requested
   a particular color ('requestedColor' not null), that's the one.
   Otherwise, if the PNG specifies a background color, that's the one.
   And otherwise, it's white.
-----------------------------------------------------------------------------*/
    if (requestedColor) {
        /* Background was specified from the command-line; we always
           use that.  I chose to do no gamma-correction in this case;
           which is a bit arbitrary.  
        */
        pixel const backcolor = ppm_parsecolor(requestedColor, maxval);

        bgColorP->r = PPM_GETR(backcolor);
        bgColorP->g = PPM_GETG(backcolor);
        bgColorP->b = PPM_GETB(backcolor);

    } else if (info_ptr->valid & PNG_INFO_bKGD) {
        /* didn't manage to get libpng to work (bugs?) concerning background
           processing, therefore we do our own.
        */
        switch (info_ptr->color_type) {
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            bgColorP->r = bgColorP->g = bgColorP->b = 
                gamma_correct(info_ptr->background.gray, totalgamma);
            break;
        case PNG_COLOR_TYPE_PALETTE: {
            png_color const rawBgcolor = 
                info_ptr->palette[info_ptr->background.index];
            bgColorP->r = gamma_correct(rawBgcolor.red, totalgamma);
            bgColorP->g = gamma_correct(rawBgcolor.green, totalgamma);
            bgColorP->b = gamma_correct(rawBgcolor.blue, totalgamma);
        }
        break;
        case PNG_COLOR_TYPE_RGB:
        case PNG_COLOR_TYPE_RGB_ALPHA: {
            png_color_16 const rawBgcolor = info_ptr->background;
            
            bgColorP->r = gamma_correct(rawBgcolor.red,   totalgamma);
            bgColorP->g = gamma_correct(rawBgcolor.green, totalgamma);
            bgColorP->b = gamma_correct(rawBgcolor.blue,  totalgamma);
        }
        break;
        }
    } else 
        /* when no background given, we use white [from version 2.37] */
        bgColorP->r = bgColorP->g = bgColorP->b = maxval;
}



static void
writePnm(FILE *              const ofP,
         xelval              const maxval,
         int                 const pnm_type,
         png_info *          const info_ptr,
         png_byte **         const png_image,
         pngcolor            const bgColor,
         enum alpha_handling const alpha_handling,
         double              const totalgamma) {
/*----------------------------------------------------------------------------
   Write a PNM of either the image or the alpha mask, according to
   'alpha_handling' that is in the PNG image described by 'info_ptr' and
   png_image.

   'pnm_type' and 'maxval' are of the output image.

   Use background color 'bgColor' in the output if the PNG is such that a
   background color is needed.
-----------------------------------------------------------------------------*/
    xel * xelrow;
    unsigned int row;

    if (verbose)
        pm_message ("writing a %s file (maxval=%u)",
                    pnm_type == PBM_TYPE ? "PBM" :
                    pnm_type == PGM_TYPE ? "PGM" :
                    pnm_type == PPM_TYPE ? "PPM" :
                    "UNKNOWN!", 
                    maxval);
    
    xelrow = pnm_allocrow(info_ptr->width);

    pnm_writepnminit(stdout, info_ptr->width, info_ptr->height, maxval,
                     pnm_type, FALSE);

    for (row = 0; row < info_ptr->height; ++row) {
        png_byte * png_pixelP;
        int col;

        png_pixelP = &png_image[row][0];  /* initial value */
        for (col = 0; col < info_ptr->width; ++col) {
            switch (info_ptr->color_type) {
            case PNG_COLOR_TYPE_GRAY: {
                pngcolor fgColor;
                fgColor.r = fgColor.g = fgColor.b = get_png_val(png_pixelP);
                setXel(&xelrow[col], fgColor, bgColor, alpha_handling,
                       ((info_ptr->valid & PNG_INFO_tRNS) &&
                        (fgColor.r == 
                         gamma_correct(info_ptr->trans_values.gray,
                                       totalgamma))) ?
                       0 : maxval);
            }
            break;

            case PNG_COLOR_TYPE_GRAY_ALPHA: {
                pngcolor fgColor;
                png_uint_16 alpha;

                fgColor.r = fgColor.g = fgColor.b = get_png_val(png_pixelP);
                alpha = get_png_val(png_pixelP);
                setXel(&xelrow[col], fgColor, bgColor, alpha_handling, alpha);
            }
            break;

            case PNG_COLOR_TYPE_PALETTE: {
                png_uint_16 const index        = get_png_val(png_pixelP);
                png_color   const paletteColor = info_ptr->palette[index];

                pngcolor fgColor;

                fgColor.r = paletteColor.red;
                fgColor.g = paletteColor.green;
                fgColor.b = paletteColor.blue;

                setXel(&xelrow[col], fgColor, bgColor, alpha_handling,
                       (info_ptr->valid & PNG_INFO_tRNS) &&
                       index < info_ptr->num_trans ?
                       info_ptr->trans[index] : maxval);
            }
            break;
                
            case PNG_COLOR_TYPE_RGB: {
                pngcolor fgColor;

                fgColor.r = get_png_val(png_pixelP);
                fgColor.g = get_png_val(png_pixelP);
                fgColor.b = get_png_val(png_pixelP);
                setXel(&xelrow[col], fgColor, bgColor, alpha_handling,
                       isTransparentColor(fgColor, info_ptr, totalgamma) ?
                       0 : maxval);
            }
            break;

            case PNG_COLOR_TYPE_RGB_ALPHA: {
                pngcolor fgColor;
                png_uint_16 alpha;

                fgColor.r = get_png_val(png_pixelP);
                fgColor.g = get_png_val(png_pixelP);
                fgColor.b = get_png_val(png_pixelP);
                alpha     = get_png_val(png_pixelP);
                setXel(&xelrow[col], fgColor, bgColor, alpha_handling, alpha);
            }
            break;

            default:
                pm_error ("unknown PNG color type: %d", info_ptr->color_type);
            }
        }
        pnm_writepnmrow(ofP, xelrow, info_ptr->width, maxval, pnm_type, FALSE);
    }
    pnm_freerow (xelrow);
}



static void 
convertpng(FILE *             const ifp, 
           FILE *             const tfp, 
           struct cmdlineInfo const cmdline,
           int *              const errorlevelP) {

    png_struct *png_ptr;
    png_info *info_ptr;
    png_byte **png_image;
    int x, y;
    int linesize;
    int pnm_type;
    pngcolor bgColor;
    float totalgamma;

    *errorlevelP = 0;

    read_sig_buf(ifp);

    png_ptr = png_create_read_struct(
        PNG_LIBPNG_VER_STRING,
        &pngtopnm_jmpbuf_struct, pngtopnm_error_handler, NULL);
    if (png_ptr == NULL)
        pm_error("cannot allocate main libpng structure (png_ptr)");

    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL)
        pm_error("cannot allocate LIBPNG structures");

    if (setjmp(pngtopnm_jmpbuf_struct.jmpbuf))
        pm_error ("setjmp returns error condition");

    png_init_io (png_ptr, ifp);
    png_set_sig_bytes (png_ptr, SIG_CHECK_SIZE);
    png_read_info (png_ptr, info_ptr);

    MALLOCARRAY(png_image, info_ptr->height);
    if (png_image == NULL) {
        png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
        pm_closer (ifp);
        pm_error ("couldn't allocate space for image");
    }

    if (info_ptr->bit_depth == 16)
        linesize = 2 * info_ptr->width;
    else
        linesize = info_ptr->width;

    if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        linesize *= 2;
    else
        if (info_ptr->color_type == PNG_COLOR_TYPE_RGB)
            linesize *= 3;
        else
            if (info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
                linesize *= 4;

    for (y = 0 ; y < info_ptr->height ; y++) {
        png_image[y] = malloc (linesize);
        if (png_image[y] == NULL) {
            for (x = 0 ; x < y ; x++)
                free (png_image[x]);
            free (png_image);
            png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
            pm_closer (ifp);
            pm_error ("couldn't allocate space for image");
        }
    }

    if (info_ptr->bit_depth < 8)
        png_set_packing (png_ptr);

    setupGammaCorrection(png_ptr, info_ptr, cmdline.gamma, &totalgamma);

    setupSignificantBits(png_ptr, info_ptr, cmdline.alpha,
                         &maxval, errorlevelP);

    getBackgroundColor(info_ptr, cmdline.background, totalgamma, maxval,
                       &bgColor);

    png_read_image (png_ptr, png_image);
    png_read_end (png_ptr, info_ptr);

    if (verbose)
        /* Note that some of info_ptr is not defined until png_read_end() 
       completes.  That's because it comes from chunks that are at the
       end of the stream.
    */
        dump_png_info(info_ptr);

    if (mtime)
        show_time (info_ptr);
    if (tfp)
        save_text (info_ptr, tfp);

    if (info_ptr->valid & PNG_INFO_pHYs) {
        float r;
        r = (float)info_ptr->x_pixels_per_unit / info_ptr->y_pixels_per_unit;
        if (r != 1.0) {
            pm_message ("warning - non-square pixels; "
                        "to fix do a 'pamscale -%cscale %g'",
                        r < 1.0 ? 'x' : 'y',
                        r < 1.0 ? 1.0 / r : r );
            *errorlevelP = PNMTOPNG_WARNING_LEVEL;
        }
    }

    determineOutputType(info_ptr, cmdline.alpha, bgColor, maxval, &pnm_type);

    writePnm(stdout, maxval, pnm_type, info_ptr, png_image, bgColor, 
             cmdline.alpha, totalgamma);

    fflush(stdout);
    for (y = 0 ; y < info_ptr->height ; y++)
        free (png_image[y]);
    free (png_image);
    png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
}



int 
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE *ifp, *tfp;
    int errorlevel;

    pnm_init (&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    verbose = cmdline.verbose;
    mtime = cmdline.time;

    ifp = pm_openr(cmdline.inputFilespec);

    if (cmdline.text)
        tfp = pm_openw(cmdline.text);
    else
        tfp = NULL;

    convertpng (ifp, tfp, cmdline, &errorlevel);

    if (tfp)
        pm_close(tfp);

    pm_close(ifp);
    pm_close(stdout);

    return errorlevel;
}
