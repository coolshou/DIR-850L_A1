/*****************************************************************************
                                pnmtojpeg
******************************************************************************
  This program is part of the Netpbm package.

  This program converts from the PNM formats to the JFIF format
  which is based on JPEG.

  This program is by Bryan Henderson on 2000.03.06, but is derived
  with permission from the program cjpeg, which is in the Independent
  Jpeg Group's JPEG library package.  Under the terms of that permission,
  redistribution of this software is restricted as described in the 
  file README.JPEG.

  Copyright (C) 1991-1998, Thomas G. Lane.

*****************************************************************************/

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <ctype.h>		/* to declare isdigit(), etc. */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
/* Note: jpeglib.h prerequires stdlib.h and ctype.h.  It should include them
   itself, but doesn't.
*/
#include <jpeglib.h>
#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"

#define EXIT_WARNING 2   /* Goes with EXIT_SUCCESS, EXIT_FAILURE in stdlib.h */

enum restart_unit {RESTART_MCU, RESTART_ROW, RESTART_NONE};
enum density_unit {DEN_UNSPECIFIED, DEN_DOTS_PER_INCH, DEN_DOTS_PER_CM};

struct density {
    enum density_unit unit;
        /* The units of density for 'horiz', 'vert' */
    unsigned short horiz;  /* Not 0 */
        /* Horizontal density, in units specified by 'unit' */
    unsigned short vert;   /* Not 0 */
        /* Same as 'horiz', but vertical */
};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
     */
    char *input_filespec;
    unsigned int verbose;
    unsigned int quality;
    unsigned int force_baseline;
    unsigned int progressive;
    unsigned int arith_code;
    J_DCT_METHOD dct_method;
    unsigned int grayscale;
    unsigned int rgb;
    long int max_memory_to_use;
    unsigned int trace_level;
    char *qslots;
    char *qtablefile;
    char *sample;
    char *scans;
    int smoothing_factor;
    unsigned int optimize;
    unsigned int restart_value;
    enum restart_unit restart_unit;
    char *restart;
    char *comment;              /* NULL if none */
    const char *exif_filespec;  /* NULL if none */
    unsigned int density_spec;
        /* boolean: JFIF should specify a density.  If false, 'density'
           is undefined.
        */
    struct density density;
};

static void 
interpret_maxmemory (const char * const maxmemory, 
                     long int * const max_memory_to_use_p) { 
    long int lval;
    char ch;
    
    if (maxmemory == NULL) {
        *max_memory_to_use_p = -1;  /* unspecified */
    } else if (sscanf(maxmemory, "%ld%c", &lval, &ch) < 1) {
        pm_error("Invalid value for --maxmemory option: '%s'.", maxmemory);
        exit(EXIT_FAILURE);
    } else {
        if (ch == 'm' || ch == 'M') lval *= 1000L;
        *max_memory_to_use_p = lval * 1000L;
    }
}



static void
interpret_restart(const char * const restart,
                  unsigned int * const restart_value_p,
                  enum restart_unit * const restart_unit_p) {
/*----------------------------------------------------------------------------
   Interpret the restart command line option.  Return values suitable
   for plugging into a jpeg_compress_struct to control compression.
-----------------------------------------------------------------------------*/
    if (restart == NULL) {
        /* No --restart option.  Set default */
        *restart_unit_p = RESTART_NONE;
    } else {
        /* Restart interval in MCU rows (or in MCUs with 'b'). */
        long lval;
        char ch;
        unsigned int matches;
        
        matches= sscanf(restart, "%ld%c", &lval, &ch);
        if (matches == 0) 
            pm_error("Invalid value for the --restart option : '%s'.",
                     restart);
        else {
            if (lval < 0 || lval > 65535L) {
                pm_error("--restart value %ld is out of range.", lval);
                exit(EXIT_FAILURE);
            } else {
                if (matches == 1) {
                    *restart_value_p = lval;
                    *restart_unit_p = RESTART_ROW;
                } else {
                    if (ch == 'b' || ch == 'B') {
                        *restart_value_p = lval;
                        *restart_unit_p = RESTART_MCU;
                    } else pm_error("Invalid --restart value '%s'.", restart);
                }
            }
        }
    }
}




static void
interpret_density(const char *        const densityString,
                  struct density *    const densityP) {
/*----------------------------------------------------------------------------
   Interpret the value of the "-density" option.
-----------------------------------------------------------------------------*/
    if (strlen(densityString) < 1)
        pm_error("-density value cannot be null.");
    else {
        char * unitName;  /* malloc'ed */
        int matched;
        int horiz, vert;

        unitName = malloc(strlen(densityString)+1);
    
        matched = sscanf(densityString, "%dx%d%s", &horiz, &vert, unitName);

        if (matched < 2)
            pm_error("Invalid format for density option value '%s'.  It "
                     "should follow the example '3x2' or '3x2dpi' or "
                     "'3x2dpcm'.", densityString);
        else {
            if (horiz <= 0 || horiz >= 1<<16)
                pm_error("Horizontal density %d is outside the range 1-65535",
                         horiz);
            else if (vert <= 0 || vert >= 1<<16)
                pm_error("Vertical density %d is outside the range 1-65535",
                         vert);
            else {
                densityP->horiz = horiz;
                densityP->vert  = vert;

                if (matched < 3) 
                    densityP->unit = DEN_UNSPECIFIED;
                else {
                    if (streq(unitName, "dpi") || streq(unitName, "DPI"))
                        densityP->unit = DEN_DOTS_PER_INCH;
                    else if (streq(unitName, "dpcm") ||
                             streq(unitName, "DPCM"))
                        densityP->unit = DEN_DOTS_PER_CM;
                    else
                        pm_error("Unrecognized unit '%s' in the density value "
                                 "'%s'.  I recognize only 'dpi' and 'dpcm'",
                                 unitName, densityString);
                }
            }
        }
        free(unitName);
    }
}



static void
parseCommandLine(const int argc, char ** argv,
                   struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that many of the strings that this function returns in the
   *cmdlineP structure are actually in the supplied argv array.  And
   sometimes, one of these strings is actually just a suffix of an entry
   in argv!

   On the other hand, unlike other option processing functions, we do
   not change argv at all.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    int i;  /* local loop variable */

    const char *dctval;
    const char *maxmemory;
    const char *restart;
    const char *density;

    unsigned int qualitySpec, smoothSpec;

    unsigned int option_def_index;

    int argc_parse;       /* argc, except we modify it as we parse */
    char ** argv_parse;
        /* argv, except we modify it as we parse */

    MALLOCARRAY(argv_parse, argc);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "verbose",     OPT_FLAG,   NULL, &cmdlineP->verbose,        0);
    OPTENT3(0, "quality",     OPT_UINT,   &cmdlineP->quality, 
            &qualitySpec,        0);
    OPTENT3(0, "baseline",    OPT_FLAG,   NULL, &cmdlineP->force_baseline, 0);
    OPTENT3(0, "progressive", OPT_FLAG,   NULL, &cmdlineP->progressive,    0);
    OPTENT3(0, "arithmetic",  OPT_FLAG,   NULL, &cmdlineP->arith_code,     0);
    OPTENT3(0, "dct",         OPT_STRING, &dctval, NULL,                    0);
    OPTENT3(0, "grayscale",   OPT_FLAG,   NULL, &cmdlineP->grayscale,      0);
    OPTENT3(0, "greyscale",   OPT_FLAG,   NULL, &cmdlineP->grayscale,      0);
    OPTENT3(0, "rgb",         OPT_FLAG,   NULL, &cmdlineP->rgb,            0);
    OPTENT3(0, "maxmemory",   OPT_STRING, &maxmemory, NULL,                 0);
    OPTENT3(0, "tracelevel",  OPT_UINT,   &cmdlineP->trace_level, NULL,    0);
    OPTENT3(0, "qslots",      OPT_STRING, &cmdlineP->qslots,      NULL,    0);
    OPTENT3(0, "qtables",     OPT_STRING, &cmdlineP->qtablefile,  NULL,    0);
    OPTENT3(0, "sample",      OPT_STRING, &cmdlineP->sample,      NULL,    0);
    OPTENT3(0, "scans",       OPT_STRING, &cmdlineP->scans,       NULL,    0);
    OPTENT3(0, "smooth",      OPT_UINT,   &cmdlineP->smoothing_factor, 
            &smoothSpec,  0);
    OPTENT3(0, "optimize",    OPT_FLAG,   NULL, &cmdlineP->optimize,       0);
    OPTENT3(0, "optimise",    OPT_FLAG,   NULL, &cmdlineP->optimize,       0);
    OPTENT3(0, "restart",     OPT_STRING, &restart, NULL,                   0);
    OPTENT3(0, "comment",     OPT_STRING, &cmdlineP->comment, NULL,        0);
    OPTENT3(0, "exif",        OPT_STRING, &cmdlineP->exif_filespec, NULL,  0);
    OPTENT3(0, "density",     OPT_STRING, &density, 
            &cmdlineP->density_spec, 0);

    /* Set the defaults */
    dctval = NULL;
    maxmemory = NULL;
    cmdlineP->trace_level = 0;
    cmdlineP->qslots = NULL;
    cmdlineP->qtablefile = NULL;
    cmdlineP->sample = NULL;
    cmdlineP->scans = NULL;
    restart = NULL;
    cmdlineP->comment = NULL;
    cmdlineP->exif_filespec = NULL;

    /* Make private copy of arguments for optParseOptions to corrupt */
    argc_parse = argc;
    for (i=0; i < argc; i++) argv_parse[i] = argv[i];

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc_parse, argv_parse, opt, sizeof(opt), 0);

    if (!qualitySpec)
        cmdlineP->quality = -1;  /* unspecified */

    if (!smoothSpec)
        cmdlineP->smoothing_factor = -1;

    if (cmdlineP->rgb && cmdlineP->grayscale)
        pm_error("You can't specify both -rgb and -grayscale");

    if (argc_parse - 1 == 0)
        cmdlineP->input_filespec = strdup("-");  /* he wants stdin */
    else if (argc_parse - 1 == 1)
        cmdlineP->input_filespec = strdup(argv_parse[1]);
    else 
        pm_error("Too many arguments.  The only argument accepted "
                 "is the input file specification.");
    if (dctval == NULL)
        cmdlineP->dct_method = JDCT_DEFAULT;
    else {
        if (streq(dctval, "int"))
            cmdlineP->dct_method = JDCT_ISLOW;
        else if (streq(dctval, "fast"))
            cmdlineP->dct_method = JDCT_IFAST;
        else if (streq(dctval, "float"))
            cmdlineP->dct_method = JDCT_FLOAT;
        else
            pm_error("Invalid value for the --dct option: '%s'.", dctval);
    }

    interpret_maxmemory(maxmemory, &cmdlineP->max_memory_to_use);
    interpret_restart(restart, &cmdlineP->restart_value,
                      &cmdlineP->restart_unit);
    if (cmdlineP->density_spec) 
        interpret_density(density, &cmdlineP->density);
    
    if (cmdlineP->smoothing_factor > 100)
        pm_error("Smoothing factor %d is greater than 100 (%%).",
                 cmdlineP->smoothing_factor);

    if (streq(cmdlineP->input_filespec, "=") &&
        cmdlineP->exif_filespec && 
        streq(cmdlineP->exif_filespec, "-"))

        pm_error("Cannot have both input image and exif header be from "
                 "Standard Input.");


    free(argv_parse);
}


static void
compute_rescaling_array(JSAMPLE ** const rescale_p, const pixval maxval,
                        const struct jpeg_compress_struct cinfo);
static void
convert_scanlines(struct jpeg_compress_struct * const cinfo_p, FILE * const input_file,
                  const pixval maxval, const int input_fmt,
                  JSAMPLE xlate_table[]);

static boolean read_quant_tables (j_compress_ptr cinfo, char * filename,
                                  int scale_factor, boolean force_baseline);

static boolean read_scan_script (j_compress_ptr cinfo, char * filename);

static boolean set_quant_slots (j_compress_ptr cinfo, char *arg);

static boolean set_sample_factors (j_compress_ptr cinfo, char *arg);


static void
report_compressor(const struct jpeg_compress_struct cinfo) {
    
    if (cinfo.scan_info == NULL)
        pm_message("No scan script is being used");
    else {
        int i;
        pm_message("A scan script with %d entries is being used:",
                   cinfo.num_scans);
        for (i = 0; i < cinfo.num_scans; i++) {
            int j;
            pm_message("    Scan %2d: Ss=%2d Se=%2d Ah=%2d Al=%2d  "
                       "%d components", 
                       i,
                       cinfo.scan_info[i].Ss,
                       cinfo.scan_info[i].Se,
                       cinfo.scan_info[i].Ah,
                       cinfo.scan_info[i].Al,
                       cinfo.scan_info[i].comps_in_scan
                       );
            for (j = 0; j < cinfo.scan_info[i].comps_in_scan; j++)
                pm_message("        Color component %d index: %d", j,
                           cinfo.scan_info[i].component_index[j]);
        }
    }
}



static void
setup_jpeg_source_parameters(struct jpeg_compress_struct * const cinfoP, 
                             int const width, int const height, 
                             int const format) {
/*----------------------------------------------------------------------------
   Set up in the compressor descriptor *cinfoP the description of the 
   source image as required by the compressor.
-----------------------------------------------------------------------------*/

    switch PNM_FORMAT_TYPE(format) {
    case PBM_TYPE:
    case PGM_TYPE:
        cinfoP->in_color_space = JCS_GRAYSCALE;
        cinfoP->input_components = 1;
        break;
    case PPM_TYPE:
        cinfoP->in_color_space = JCS_RGB; 
        cinfoP->input_components = 3;
        break;
    default:
        pm_error("INTERNAL ERROR; invalid format in "
                 "setup_jpeg_source_parameters()");
    }
}



static void
setup_jpeg_density(struct jpeg_compress_struct * const cinfoP, 
                   struct density                const density) {
/*----------------------------------------------------------------------------
   Set up in the compressor descriptor *cinfoP the density information
   'density'.
-----------------------------------------------------------------------------*/
    switch(density.unit) {
    case DEN_UNSPECIFIED:   cinfoP->density_unit = 0; break;
    case DEN_DOTS_PER_INCH: cinfoP->density_unit = 1; break;
    case DEN_DOTS_PER_CM:   cinfoP->density_unit = 2; break;
    }
    
    cinfoP->X_density = density.horiz;
    cinfoP->Y_density = density.vert;
}



static void
setup_jpeg(struct jpeg_compress_struct * const cinfoP,
           struct jpeg_error_mgr       * const jerrP,
           struct cmdlineInfo            const cmdline, 
           int                           const width,
           int                           const height,
           pixval                        const maxval,
           int                           const input_fmt,
           FILE *                        const output_file) {
  
    int quality;
    int q_scale_factor;

    /* Initialize the JPEG compression object with default error handling. */
    cinfoP->err = jpeg_std_error(jerrP);
    jpeg_create_compress(cinfoP);

    setup_jpeg_source_parameters(cinfoP, width, height, input_fmt);

    jpeg_set_defaults(cinfoP);

    cinfoP->data_precision = BITS_IN_JSAMPLE; 
        /* we always rescale data to this */
    cinfoP->image_width = (unsigned int) width;
    cinfoP->image_height = (unsigned int) height;

    cinfoP->arith_code = cmdline.arith_code;
    cinfoP->dct_method = cmdline.dct_method;
    if (cmdline.trace_level == 0 && cmdline.verbose) 
        cinfoP->err->trace_level = 1;
    else cinfoP->err->trace_level = cmdline.trace_level;
    if (cmdline.grayscale)
        jpeg_set_colorspace(cinfoP, JCS_GRAYSCALE);
    else if (cmdline.rgb)
        /* This is not legal if the input is not JCS_RGB too, i.e. it's PPM */
        jpeg_set_colorspace(cinfoP, JCS_RGB);
    else
        /* This default will be based on the in_color_space set above */
        jpeg_default_colorspace(cinfoP);
    if (cmdline.max_memory_to_use != -1)
        cinfoP->mem->max_memory_to_use = cmdline.max_memory_to_use;
    cinfoP->optimize_coding = cmdline.optimize;
    if (cmdline.quality == -1) {
        quality = 75;
        q_scale_factor = 100;
    } else {
        quality = cmdline.quality;
        q_scale_factor = jpeg_quality_scaling(cmdline.quality);
    }
    if (cmdline.smoothing_factor != -1) 
        cinfoP->smoothing_factor = cmdline.smoothing_factor;

    /* Set quantization tables for selected quality. */
    /* Some or all may be overridden if user specified --qtables. */
    jpeg_set_quality(cinfoP, quality, cmdline.force_baseline);
                   
    if (cmdline.qtablefile != NULL) {
        if (! read_quant_tables(cinfoP, cmdline.qtablefile,
                                q_scale_factor, cmdline.force_baseline)) 
            pm_error("Can't use quantization table file '%s'.",
                     cmdline.qtablefile);
    }
   
    if (cmdline.qslots != NULL) {
        if (! set_quant_slots(cinfoP, cmdline.qslots))
            pm_error("Bad quantization-table-selectors parameter string '%s'.", 
                     cmdline.qslots);
    }
          
    if (cmdline.sample != NULL) {
        if (! set_sample_factors(cinfoP, cmdline.sample))
            pm_error("Bad sample-factors parameters string '%s'.",
                     cmdline.sample);
    }

    if (cmdline.progressive)
        jpeg_simple_progression(cinfoP);

    if (cmdline.density_spec)
        setup_jpeg_density(cinfoP, cmdline.density);

    if (cmdline.scans != NULL) {
        if (! read_scan_script(cinfoP, cmdline.scans)) {
            pm_message("Error in scan script '%s'.", cmdline.scans);
        }
    }

    /* Specify data destination for compression */
    jpeg_stdio_dest(cinfoP, output_file);

    if (cmdline.verbose) report_compressor(*cinfoP);

    /* Start compressor */
    jpeg_start_compress(cinfoP, TRUE);

}



static void
write_exif_header(struct jpeg_compress_struct * const cinfoP,
                  const char * const exif_filespec) {
/*----------------------------------------------------------------------------
   Generate an APP1 marker in the JFIF output that is an Exif header.

   The contents of the Exif header are in the file with filespec 
   'exif_filespec' (file spec and contents are not validated).

   exif_filespec = "-" means Standard Input.

   If the file contains just two bytes of zero, don't write any marker
   but don't recognize any error either.
-----------------------------------------------------------------------------*/
    FILE * exif_file;
    unsigned short length;
    
    exif_file = pm_openr(exif_filespec);

    pm_readbigshort(exif_file, (short*)&length);

    if (length == 0) {
        /* Special value meaning "no header" */
    } else if (length < 3)
        pm_error("Invalid length %u at start of exif file", length);
    else {
        unsigned char * exif_data;
        int rc;
        size_t const data_length = length - 2;  
            /* Subtract 2 byte length field*/

        assert(data_length > 0);

        exif_data = malloc(data_length);
        if (exif_data == NULL)
            pm_error("Unable to allocate %d bytes for exif header buffer",
                     data_length);

        rc = fread(exif_data, 1, data_length, exif_file);

        if (rc != data_length)
            pm_error("Premature end of file on exif header file.  Should be "
                     "%d bytes of data, read only %d", data_length, rc);

        jpeg_write_marker(cinfoP, JPEG_APP0+1, 
                          (const JOCTET *) exif_data, data_length);

        free(exif_data);
    }
    
    pm_close(exif_file);
}

                  

static void
compute_rescaling_array(JSAMPLE ** const rescale_p, const pixval maxval,
                        const struct jpeg_compress_struct cinfo) {
/*----------------------------------------------------------------------------
   Compute the rescaling array for a maximum pixval of 'maxval'.
   Allocate the memory for it too.
-----------------------------------------------------------------------------*/
  const long half_maxval = maxval / 2;
  long val;

  *rescale_p = (JSAMPLE *)
    (cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_IMAGE,
                              (size_t) (((long) maxval + 1L) * 
                                        sizeof(JSAMPLE)));
  for (val = 0; val <= maxval; val++) {
    /* The multiplication here must be done in 32 bits to avoid overflow */
    (*rescale_p)[val] = (JSAMPLE) ((val*MAXJSAMPLE + half_maxval)/maxval);
  }
}



static void
translate_row(const pixel pnm_buffer[], 
              JSAMPLE jpeg_buffer[], 
              int const width, 
              int const input_components,
              const JSAMPLE translate[]) {
/*----------------------------------------------------------------------------
   Convert the input row, in pnm format, to an output row in JPEG compressor
   input format.

   This is a byte for byte copy, translated through the array 'translate'.
-----------------------------------------------------------------------------*/
  unsigned int column;
  /* I'm not sure why the JPEG library data structures don't have some kind
     of pixel data structure (such that a row buffer is an array of pixels,
     rather than an array of samples).  But because of this, we have to
     index jpeg_buffer the old fashioned way.
     */

  switch (input_components) {
  case 1:
      for (column = 0; column < width; column++) 
          jpeg_buffer[column] = translate[(int)PNM_GET1(pnm_buffer[column])];
      break;
  case 3:
      for (column = 0; column < width; column++) {
          jpeg_buffer[column*3+0] = 
              translate[(int)PPM_GETR(pnm_buffer[column])];
          jpeg_buffer[column*3+1] = 
              translate[(int)PPM_GETG(pnm_buffer[column])];
          jpeg_buffer[column*3+2] = 
              translate[(int)PPM_GETB(pnm_buffer[column])];
      }
      break;
  default:
      pm_error("INTERNAL ERROR: invalid number of input components in "
               "translate_row()");
  }

}



static void
convert_scanlines(struct jpeg_compress_struct * const cinfo_p,
                  FILE *                        const input_file,
                  pixval                        const maxval,
                  int                           const input_fmt,
                  JSAMPLE                             xlate_table[]){
/*----------------------------------------------------------------------------
   Read scan lines from the input file, which is already opened in the 
   netpbm library sense and ready for reading, and write them to the 
   output JPEG object.  Translate the pnm sample values to JPEG sample
   values through the thable xlate_table[].
-----------------------------------------------------------------------------*/
  xel * pnm_buffer;  
    /* contains the row of the input image currently being processed,
       in pnm_readpnmrow format
    */
  JSAMPARRAY buffer;
    /* Row 0 of this array contains the row of the output image currently 
       being processed, in JPEG compressor input format.  The array only
       has that one row.
    */

  /* Allocate the libpnm output and compressor input buffers */
  buffer = (*cinfo_p->mem->alloc_sarray)
    ((j_common_ptr) cinfo_p, JPOOL_IMAGE,
     (unsigned int) cinfo_p->image_width * cinfo_p->input_components, 
     (unsigned int) 1);
  
  pnm_buffer = pnm_allocrow(cinfo_p->image_width);

  while (cinfo_p->next_scanline < cinfo_p->image_height) {
    if (cinfo_p->err->trace_level > 1) 
        pm_message("Converting Row %d...", cinfo_p->next_scanline);
    pnm_readpnmrow(input_file, pnm_buffer, cinfo_p->image_width, 
                   maxval, input_fmt);
    translate_row(pnm_buffer, buffer[0], 
                  cinfo_p->image_width, cinfo_p->input_components,
                  xlate_table);
    jpeg_write_scanlines(cinfo_p, buffer, 1);
    if (cinfo_p->err->trace_level > 1) 
        pm_message("Done.");
  }

  pnm_freerow(pnm_buffer);
  /* Dont' worry about the compressor input buffer; it gets freed 
     automatically
  */

}

/*----------------------------------------------------------------------------
   The functions below here are essentially the file rdswitch.c from
   the JPEG library.  They perform the functions specifed by the following
   pnmtojpeg options:

   -qtables file          Read quantization tables from text file
   -scans file            Read scan script from text file
   -qslots N[,N,...]      Set component quantization table selectors
   -sample HxV[,HxV,...]  Set component sampling factors
-----------------------------------------------------------------------------*/

static int
text_getc (FILE * file)
/* Read next char, skipping over any comments (# to end of line) */
/* A comment/newline sequence is returned as a newline */
{
    register int ch;
  
    ch = getc(file);
    if (ch == '#') {
        do {
            ch = getc(file);
        } while (ch != '\n' && ch != EOF);
    }
    return ch;
}


static boolean
read_text_integer (FILE * file, long * result, int * termchar)
/* Read an unsigned decimal integer from a file, store it in result */
/* Reads one trailing character after the integer; returns it in termchar */
{
    register int ch;
    register long val;
  
    /* Skip any leading whitespace, detect EOF */
    do {
        ch = text_getc(file);
        if (ch == EOF) {
            *termchar = ch;
            return FALSE;
        }
    } while (isspace(ch));
  
    if (! isdigit(ch)) {
        *termchar = ch;
        return FALSE;
    }

    val = ch - '0';
    while ((ch = text_getc(file)) != EOF) {
        if (! isdigit(ch))
            break;
        val *= 10;
        val += ch - '0';
    }
    *result = val;
    *termchar = ch;
    return TRUE;
}


static boolean
read_quant_tables (j_compress_ptr cinfo, char * filename,
                   int scale_factor, boolean force_baseline)
/* Read a set of quantization tables from the specified file.
 * The file is plain ASCII text: decimal numbers with whitespace between.
 * Comments preceded by '#' may be included in the file.
 * There may be one to NUM_QUANT_TBLS tables in the file, each of 64 values.
 * The tables are implicitly numbered 0,1,etc.
 * NOTE: does not affect the qslots mapping, which will default to selecting
 * table 0 for luminance (or primary) components, 1 for chrominance components.
 * You must use -qslots if you want a different component->table mapping.
 */
{
    FILE * fp;
    int tblno, i, termchar;
    long val;
    unsigned int table[DCTSIZE2];

    if ((fp = fopen(filename, "rb")) == NULL) {
        pm_message("Can't open table file %s", filename);
        return FALSE;
    }
    tblno = 0;

    while (read_text_integer(fp, &val, &termchar)) { /* read 1st element of table */
        if (tblno >= NUM_QUANT_TBLS) {
            pm_message("Too many tables in file %s", filename);
            fclose(fp);
            return FALSE;
        }
        table[0] = (unsigned int) val;
        for (i = 1; i < DCTSIZE2; i++) {
            if (! read_text_integer(fp, &val, &termchar)) {
                pm_message("Invalid table data in file %s", filename);
                fclose(fp);
                return FALSE;
            }
            table[i] = (unsigned int) val;
        }
        jpeg_add_quant_table(cinfo, tblno, table, scale_factor, 
                             force_baseline);
        tblno++;
    }

    if (termchar != EOF) {
        pm_message("Non-numeric data in file %s", filename);
        fclose(fp);
        return FALSE;
    }

    fclose(fp);
    return TRUE;
}


static boolean
read_scan_integer (FILE * file, long * result, int * termchar)
/* Variant of read_text_integer that always looks for a non-space termchar;
 * this simplifies parsing of punctuation in scan scripts.
 */
{
    register int ch;

    if (! read_text_integer(file, result, termchar))
        return FALSE;
    ch = *termchar;
    while (ch != EOF && isspace(ch))
        ch = text_getc(file);
    if (isdigit(ch)) {		/* oops, put it back */
        if (ungetc(ch, file) == EOF)
            return FALSE;
        ch = ' ';
    } else {
        /* Any separators other than ';' and ':' are ignored;
         * this allows user to insert commas, etc, if desired.
         */
        if (ch != EOF && ch != ';' && ch != ':')
            ch = ' ';
    }
    *termchar = ch;
    return TRUE;
}


boolean
read_scan_script (j_compress_ptr cinfo, char * filename)
/* Read a scan script from the specified text file.
 * Each entry in the file defines one scan to be emitted.
 * Entries are separated by semicolons ';'.
 * An entry contains one to four component indexes,
 * optionally followed by a colon ':' and four progressive-JPEG parameters.
 * The component indexes denote which component(s) are to be transmitted
 * in the current scan.  The first component has index 0.
 * Sequential JPEG is used if the progressive-JPEG parameters are omitted.
 * The file is free format text: any whitespace may appear between numbers
 * and the ':' and ';' punctuation marks.  Also, other punctuation (such
 * as commas or dashes) can be placed between numbers if desired.
 * Comments preceded by '#' may be included in the file.
 * Note: we do very little validity checking here;
 * jcmaster.c will validate the script parameters.
 */
{
    FILE * fp;
    int nscans, ncomps, termchar;
    long val;
#define MAX_SCANS  100      /* quite arbitrary limit */
    jpeg_scan_info scans[MAX_SCANS];

    if ((fp = fopen(filename, "r")) == NULL) {
        pm_message("Can't open scan definition file %s", filename);
        return FALSE;
    }
    nscans = 0;

    while (read_scan_integer(fp, &val, &termchar)) {
        nscans++;  /* We got another scan */
        if (nscans > MAX_SCANS) {
            pm_message("Too many scans defined in file %s", filename);
            fclose(fp);
            return FALSE;
        }
        scans[nscans-1].component_index[0] = (int) val;
        ncomps = 1;
        while (termchar == ' ') {
            if (ncomps >= MAX_COMPS_IN_SCAN) {
                pm_message("Too many components in one scan in file %s", 
                           filename);
                fclose(fp);
                return FALSE;
            }
            if (! read_scan_integer(fp, &val, &termchar))
                goto bogus;
            scans[nscans-1].component_index[ncomps] = (int) val;
            ncomps++;
        }
        scans[nscans-1].comps_in_scan = ncomps;
        if (termchar == ':') {
            if (! read_scan_integer(fp, &val, &termchar) || termchar != ' ')
                goto bogus;
            scans[nscans-1].Ss = (int) val;
            if (! read_scan_integer(fp, &val, &termchar) || termchar != ' ')
                goto bogus;
            scans[nscans-1].Se = (int) val;
            if (! read_scan_integer(fp, &val, &termchar) || termchar != ' ')
                goto bogus;
            scans[nscans-1].Ah = (int) val;
            if (! read_scan_integer(fp, &val, &termchar))
                goto bogus;
            scans[nscans-1].Al = (int) val;
        } else {
            /* set non-progressive parameters */
            scans[nscans-1].Ss = 0;
            scans[nscans-1].Se = DCTSIZE2-1;
            scans[nscans-1].Ah = 0;
            scans[nscans-1].Al = 0;
        }
        if (termchar != ';' && termchar != EOF) {
        bogus:
            pm_message("Invalid scan entry format in file %s", filename);
            fclose(fp);
            return FALSE;
        }
    }

    if (termchar != EOF) {
        pm_message("Non-numeric data in file %s", filename);
        fclose(fp);
        return FALSE;
    }

    if (nscans > 0) {
        /* Stash completed scan list in cinfo structure.  NOTE: in
         * this program, JPOOL_IMAGE is the right lifetime for this
         * data, but if you want to compress multiple images you'd
         * want JPOOL_PERMANENT.  
         */
        const unsigned int scan_info_size = nscans * sizeof(jpeg_scan_info);
        jpeg_scan_info * const scan_info = 
            (jpeg_scan_info *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                        scan_info_size);
        memcpy(scan_info, scans, scan_info_size);
        cinfo->scan_info = scan_info;
        cinfo->num_scans = nscans;
    }

    fclose(fp);
    return TRUE;
}


static boolean
set_quant_slots (j_compress_ptr cinfo, char *arg)
/* Process a quantization-table-selectors parameter string, of the form
 *     N[,N,...]
 * If there are more components than parameters, the last value is replicated.
 */
{
    int val = 0;			/* default table # */
    int ci;
    char ch;

    for (ci = 0; ci < MAX_COMPONENTS; ci++) {
        if (*arg) {
            ch = ',';			/* if not set by sscanf, will be ',' */
            if (sscanf(arg, "%d%c", &val, &ch) < 1)
                return FALSE;
            if (ch != ',')		/* syntax check */
                return FALSE;
            if (val < 0 || val >= NUM_QUANT_TBLS) {
                pm_message("Invalid quantization table number: %d.  "
                           "JPEG quantization tables are numbered 0..%d",
                           val, NUM_QUANT_TBLS - 1);
                return FALSE;
            }
            cinfo->comp_info[ci].quant_tbl_no = val;
            while (*arg && *arg++ != ',') 
                /* advance to next segment of arg string */
                ;
        } else {
            /* reached end of parameter, set remaining components to last tbl*/
            cinfo->comp_info[ci].quant_tbl_no = val;
        }
    }
    return TRUE;
}


static boolean
set_sample_factors (j_compress_ptr cinfo, char *arg)
/* Process a sample-factors parameter string, of the form
 *     HxV[,HxV,...]
 * If there are more components than parameters, "1x1" is assumed for the rest.
 */
{
    int ci, val1, val2;
    char ch1, ch2;

    for (ci = 0; ci < MAX_COMPONENTS; ci++) {
        if (*arg) {
            ch2 = ',';		/* if not set by sscanf, will be ',' */
            if (sscanf(arg, "%d%c%d%c", &val1, &ch1, &val2, &ch2) < 3)
                return FALSE;
            if ((ch1 != 'x' && ch1 != 'X') || ch2 != ',') /* syntax check */
                return FALSE;
            if (val1 <= 0 || val1 > 4) {
                pm_message("Invalid sampling factor: %d.  " 
                           "JPEG sampling factors must be 1..4", val1);
                return FALSE;
            }
            if (val2 <= 0 || val2 > 4) {
                pm_message("Invalid sampling factor: %d.  "
                           "JPEG sampling factors must be 1..4", val2);
                return FALSE;
            }
            cinfo->comp_info[ci].h_samp_factor = val1;
            cinfo->comp_info[ci].v_samp_factor = val2;
            while (*arg && *arg++ != ',') 
                /* advance to next segment of arg string */
                ;
        } else {
            /* reached end of parameter, set remaining components 
               to 1x1 sampling */
            cinfo->comp_info[ci].h_samp_factor = 1;
            cinfo->comp_info[ci].v_samp_factor = 1;
        }
    }
    return TRUE;
}



int
main(int argc, char ** argv) {

    struct cmdlineInfo cmdline;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *input_file;
    FILE * output_file;
    int height;  
        /* height of the input image in rows, as specified by its header */
    int width;   
        /* width of the input image in columns, as specified by its header */
    pixval maxval;  
        /* maximum value of an input pixel component, as specified by header */
    int input_fmt;
        /* The input format, as determined by its header.  */
    JSAMPLE *rescale;         /* => maxval-remapping array, or NULL */
        /* This is an array that maps each possible pixval in the input to
           a new value such that while the range of the input values is
           0 .. maxval, the range of the output values is 0 .. MAXJSAMPLE.
        */

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    input_file = pm_openr(cmdline.input_filespec);
    free(cmdline.input_filespec);

    output_file = stdout;

    /* Open the pnm input */
    pnm_readpnminit(input_file, &width, &height, &maxval, &input_fmt);
    if (cmdline.verbose) {
        pm_message("Input file has format %c%c.\n"
                   "It has %d rows of %d columns of pixels "
                   "with max sample value of %d.", 
                   (char) (input_fmt/256), (char) (input_fmt % 256),
                   height, width, maxval);
    }

    setup_jpeg(&cinfo, &jerr, cmdline, width, height, maxval, input_fmt,
               output_file);

    compute_rescaling_array(&rescale, maxval, cinfo);

    if (cmdline.comment) 
        jpeg_write_marker(&cinfo, JPEG_COM, (const JOCTET *) cmdline.comment, 
                          strlen(cmdline.comment));

    if (cmdline.exif_filespec)
        write_exif_header(&cinfo, cmdline.exif_filespec);
    
    /* Translate and copy over the actual scanlines */
    convert_scanlines(&cinfo, input_file, maxval, input_fmt, rescale);

    /* Finish compression and release memory */
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    /* Close files, if we opened them */
    if (input_file != stdin)
        fclose(input_file);

    /* Program may have exited with non-zero completion code via
       various function calls above. 
    */
    return jerr.num_warnings > 0 ? EXIT_WARNING : EXIT_SUCCESS;
}
