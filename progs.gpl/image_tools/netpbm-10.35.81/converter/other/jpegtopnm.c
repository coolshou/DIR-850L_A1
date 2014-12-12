/*****************************************************************************
                                jpegtopnm
******************************************************************************
  This program is part of the Netpbm package.

  This program converts from the JFIF format, which is based on JPEG, to
  the fundamental ppm or pgm format (depending on whether the JFIF 
  image is gray scale or color).

  This program is by Bryan Henderson on 2000.03.20, but is derived
  with permission from the program djpeg, which is in the Independent
  Jpeg Group's JPEG library package.  Under the terms of that permission,
  redistribution of this software is restricted as described in the 
  file README.JPEG.

  Copyright (C) 1991-1998, Thomas G. Lane.

  TODO:

    For CMYK and YCCK JPEG input, optionally produce a 4-deep PAM
    output containing CMYK values.  Define a tupletype for this.
    Extend pamtoppm to convert this to ppm using the standard
    transformation.

    See if additional decompressor options effects signficant speedup.
    grayscale output of color image, downscaling, color quantization, and
    dithering are possibilities.  Djpeg's man page says they make it faster.

  IMPLEMENTATION NOTE - PRECISION

    There are two versions of the JPEG library.  One handles only JPEG
    files with 8 bit samples; the other handles only 12 bit files.
    This program may be compiled and linked against either, and run
    dynamically linked to either at runtime independently.  It uses
    the precision information from the file header.  Note that when
    the input has 12 bit precision, this program generates PPM files
    with two-byte samples, but when the input has 8 bit precision, it
    generates PPM files with one-byte samples.  One should use
    Pnmdepth to reduce precision to 8 bits if one-byte-sample output
    is essential.

  IMPLEMENTATION NOTE - EXIF

    See http://exif.org.  See the programs Exifdump
    (http://topo.math.u-psud.fr/~bousch/exifdump.py) and Jhead
    (http://www.sentex.net/~mwandel/jhead).

    
*****************************************************************************/

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <ctype.h>		/* to declare isprint() */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
/* Note: jpeglib.h prerequires stdlib.h and ctype.h.  It should include them
   itself, but doesn't.
*/
#include <jpeglib.h>
#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"
#include "exif.h"
#include "jpegdatasource.h"

#define EXIT_WARNING 2  /* Goes with EXIT_FAILURE, EXIT_SUCCESS in stdlib.h */

enum inklevel {NORMAL, ADOBE, GUESS};
   /* This describes image samples that represent ink levels.  NORMAL
      means 0 is no ink; ADOBE means 0 is maximum ink.  GUESS means we
      don't know what 0 means, so we have to guess from information in 
      the image.
      */

enum colorspace {
    /* These are the color spaces in which we can get pixels from the
       JPEG decompressor.  We include only those that are possible
       given our particular inputs to the decompressor.  The
       decompressor is theoretically capable of other, e.g. YCCK.
       Unlike the JPEG library, this type distinguishes between the
       Adobe and non-Adobe style of CMYK samples.  
    */
    GRAYSCALE_COLORSPACE,
    RGB_COLORSPACE, 
    CMYK_NORMAL_COLORSPACE, 
    CMYK_ADOBE_COLORSPACE
    };

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    char *input_filespec;
    char *exif_filespec;
        /* Filespec in which to save EXIF information.  NULL means don't
           save.  "-" means standard output
        */
    unsigned int verbose;
    unsigned int nosmooth;
    J_DCT_METHOD dct_method;
    long int max_memory_to_use;
    unsigned int trace_level;
    enum inklevel inklevel;
    unsigned int comments;
    unsigned int dumpexif;
    unsigned int multiple;
};


static bool displayComments;
    /* User wants comments from the JPEG to be displayed */

static void 
interpret_maxmemory (bool         const maxmemorySpec,
                     const char * const maxmemory, 
                     long int *   const max_memory_to_use_p) { 
/*----------------------------------------------------------------------------
   Interpret the "maxmemory" command line option.
-----------------------------------------------------------------------------*/
    long int lval;
    char ch;
    
    if (!maxmemorySpec) {
        *max_memory_to_use_p = -1;  /* unspecified */
    } else if (sscanf(maxmemory, "%ld%c", &lval, &ch) < 1) {
        pm_error("Invalid value for --maxmemory option: '%s'.", maxmemory);
    } else {
        if (ch == 'm' || ch == 'M') lval *= 1000L;
        *max_memory_to_use_p = lval * 1000L;
    }
}


static void
interpret_adobe(const int adobe, const int notadobe, 
                enum inklevel * const inklevel_p) {
/*----------------------------------------------------------------------------
   Interpret the adobe/notadobe command line options
-----------------------------------------------------------------------------*/
    if (adobe && notadobe)
        pm_error("You cannot specify both -adobe and -notadobe options.");
    else {
        if (adobe)
            *inklevel_p = ADOBE;
        else if (notadobe)
            *inklevel_p = NORMAL;
        else 
            *inklevel_p = GUESS;
    }
}



static void
parse_command_line(const int argc, char ** argv,
                   struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that many of the strings that this function returns in the
   *cmdlineP structure are actually in the supplied argv array.  And
   sometimes, one of these strings is actually just a suffix of an entry
   in argv!

   On the other hand, unlike other option processing functions, we do
   not change argv at all.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    int i;  /* local loop variable */

    char *maxmemory;
    char *dctval;
    unsigned int adobe, notadobe;

    unsigned int tracelevelSpec, exifSpec, dctvalSpec, maxmemorySpec;
    unsigned int option_def_index;

    int argc_parse;       /* argc, except we modify it as we parse */
    char ** argv_parse;

    MALLOCARRAY_NOFAIL(option_def, 100);
    MALLOCARRAY_NOFAIL(argv_parse, argc);
    
    /* argv, except we modify it as we parse */

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "verbose",     OPT_FLAG,   NULL, &cmdlineP->verbose,       0);
    OPTENT3(0, "dct",         OPT_STRING, &dctval,
            &dctvalSpec, 0);
    OPTENT3(0, "maxmemory",   OPT_STRING, &maxmemory,
            &maxmemorySpec, 0); 
    OPTENT3(0, "nosmooth",    OPT_FLAG,   NULL, &cmdlineP->nosmooth,      0);
    OPTENT3(0, "tracelevel",  OPT_UINT,   &cmdlineP->trace_level,   
            &tracelevelSpec, 0);
    OPTENT3(0, "adobe",       OPT_FLAG,   NULL, &adobe,                   0);
    OPTENT3(0, "notadobe",    OPT_FLAG,   NULL, &notadobe,                0);
    OPTENT3(0, "comments",    OPT_FLAG,   NULL, &cmdlineP->comments,      0);
    OPTENT3(0, "exif",        OPT_STRING, &cmdlineP->exif_filespec, 
            &exifSpec, 0);
    OPTENT3(0, "dumpexif",    OPT_FLAG,   NULL, &cmdlineP->dumpexif,      0);
    OPTENT3(0, "multiple",    OPT_FLAG,   NULL, &cmdlineP->multiple,      0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    /* Make private copy of arguments for optParseOptions to corrupt */
    argc_parse = argc;
    for (i=0; i < argc; i++) argv_parse[i] = argv[i];

    optParseOptions3( &argc_parse, argv_parse, opt, sizeof(opt), 0);
        /* Uses and sets argc_parse, argv_parse, 
           and some of *cmdlineP and others. */

    if (!tracelevelSpec)
        cmdlineP->trace_level = 0;

    if (!exifSpec)
        cmdlineP->exif_filespec = NULL;

    if (argc_parse - 1 == 0)
        cmdlineP->input_filespec = strdup("-");  /* he wants stdin */
    else if (argc_parse - 1 == 1)
        cmdlineP->input_filespec = strdup(argv_parse[1]);
    else 
        pm_error("Too many arguments.  The only argument accepted "
                 "is the input file specification");

    if (!dctvalSpec)
        cmdlineP->dct_method = JDCT_DEFAULT;
    else {
        if (STREQ(dctval, "int"))
            cmdlineP->dct_method = JDCT_ISLOW;
        else if (STREQ(dctval, "fast"))
            cmdlineP->dct_method = JDCT_IFAST;
        else if (STREQ(dctval, "float"))
            cmdlineP->dct_method = JDCT_FLOAT;
        else pm_error("Invalid value for the --dct option: '%s'.", dctval);
    }

    interpret_maxmemory(maxmemorySpec, maxmemory, 
                        &cmdlineP->max_memory_to_use);

    interpret_adobe(adobe, notadobe, &cmdlineP->inklevel);

    free(argv_parse);
}


/*
 * Marker processor for COM and interesting APPn markers.
 * This replaces the library's built-in processor, which just skips the marker.
 * We want to print out the marker as text, to the extent possible.
 * Note this code relies on a non-suspending data source.
 */

#if 0
static unsigned int
jpeg_getc (j_decompress_ptr cinfo)
/* Read next byte */
{
  struct jpeg_source_mgr * datasrc = cinfo->src;

  if (datasrc->bytes_in_buffer == 0) {
      if (! (*datasrc->fill_input_buffer) (cinfo)) 
          pm_error("Can't suspend here.");
  }
  datasrc->bytes_in_buffer--;
  return GETJOCTET(*datasrc->next_input_byte++);
}


static boolean
print_text_marker (j_decompress_ptr cinfo) {
/*----------------------------------------------------------------------------
   This is a routine that you can register with the Jpeg decompressor
   with e.g.

     jpeg_set_marker_processor(cinfoP, JPEG_APP0 + app_type, 
                               print_text_marker);

  The decompressor then calls it when it encounters a miscellaneous marker
  of the specified type (e.g. APP1).  At that time, the jpeg input stream
  is positioned to the marker contents -- first 2 bytes of length information,
  MSB first, where the length includes those two bytes, then the data.
  
  We just get and print the contents of the marker.

  This routine is no longer used; it is kept as an example in case we want
  to use it in the future.  Instead, we use jpeg_save_markers() and have
  the Jpeg library store all the markers in memory for our later access.
-----------------------------------------------------------------------------*/
    const boolean traceit = (cinfo->err->trace_level >= 1);
    const boolean display_value = 
        traceit || (cinfo->unread_marker == JPEG_COM && displayComments);
    
    INT32 length;
    unsigned int ch;
    unsigned int lastch = 0;
    
    length = jpeg_getc(cinfo) << 8;
    length += jpeg_getc(cinfo);
    length -= 2;			/* discount the length word itself */

    if (traceit) {
        if (cinfo->unread_marker == JPEG_COM)
            fprintf(stderr, "Comment, length %ld:\n", (long) length);
        else			/* assume it is an APPn otherwise */
            fprintf(stderr, "APP%d, length %ld:\n",
                    cinfo->unread_marker - JPEG_APP0, (long) length);
    }
    
    if (cinfo->unread_marker == JPEG_COM && displayComments)
        fprintf(stderr, "COMMENT: ");
    
    while (--length >= 0) {
        ch = jpeg_getc(cinfo);
        if (display_value) {
            /* Emit the character in a readable form.
             * Nonprintables are converted to \nnn form,
             * while \ is converted to \\.
             * Newlines in CR, CR/LF, or LF form will be printed as one 
             * newline.
             */
            if (ch == '\r') {
              fprintf(stderr, "\n");
            } else if (ch == '\n') {
                if (lastch != '\r')
                    fprintf(stderr, "\n");
            } else if (ch == '\\') {
                fprintf(stderr, "\\\\");
            } else if (isprint(ch)) {
                putc(ch, stderr);
            } else {
                fprintf(stderr, "\\%03o", ch);
            }
          lastch = ch;
        }
    }
    
    if (display_value)
        fprintf(stderr, "\n");
    
    return TRUE;
}
#endif



static void
print_marker(struct jpeg_marker_struct const marker) {

    if (marker.original_length != marker.data_length) {
        /* This should be impossible, because we asked for up to 65535
           bytes, and the jpeg spec doesn't allow anything bigger than that.
        */
        pm_message("INTERNAL ERROR: %d of %d bytes of marker were "
                   "saved.", marker.data_length, marker.original_length);
    }

    {
        int i;
        JOCTET lastch;

        lastch = 0;
        for (i = 0; i < marker.data_length; i++) {
            /* Emit the character in a readable form.
             * Nonprintables are converted to \nnn form,
             * while \ is converted to \\.
             * Newlines in CR, CR/LF, or LF form will be printed as one 
             * newline.
             */
            if (marker.data[i] == '\r') {
                fprintf(stderr, "\n");
            } else if (marker.data[i] == '\n') {
                if (lastch != '\r')
                    fprintf(stderr, "\n");
            } else if (marker.data[i] == '\\') {
                fprintf(stderr, "\\\\");
            } else if (isprint(marker.data[i])) {
                putc(marker.data[i], stderr);
            } else {
                fprintf(stderr, "\\%03o", marker.data[i]);
            }
            lastch = marker.data[i];
        }
        fprintf(stderr, "\n");
    }
}


typedef struct rgb {unsigned int r; unsigned int g; unsigned int b;} rgb_type;


static rgb_type *
read_rgb(JSAMPLE *ptr, const enum colorspace color_space, 
         const unsigned int maxval) {
/*----------------------------------------------------------------------------
  Return the RGB triple corresponding to the color of the JPEG pixel at
  'ptr', which is in color space 'color_space'.  

  Assume 'maxval' is the maximum sample value in the input pixel, and also
  use it for the maximum sample value in the return value.
-----------------------------------------------------------------------------*/
    static rgb_type rgb;  /* Our return value */

    switch (color_space) {
    case RGB_COLORSPACE: {
        rgb.r = GETJSAMPLE(*(ptr+0));
        rgb.g = GETJSAMPLE(*(ptr+1)); 
        rgb.b = GETJSAMPLE(*(ptr+2)); 
    }
        break;
    case CMYK_NORMAL_COLORSPACE: {
        const int c = GETJSAMPLE(*(ptr+0));
        const int m = GETJSAMPLE(*(ptr+1));
        const int y = GETJSAMPLE(*(ptr+2));
        const int k = GETJSAMPLE(*(ptr+3));

        /* I swapped m and y below, because they looked wrong.
           -Bryan 2000.08.20
           */
        rgb.r = ((maxval-k)*(maxval-c))/maxval;
        rgb.g = ((maxval-k)*(maxval-m))/maxval;
        rgb.b = ((maxval-k)*(maxval-y))/maxval;
    }
        break;
    case CMYK_ADOBE_COLORSPACE: {
        const int c = GETJSAMPLE(*(ptr+0));
        const int m = GETJSAMPLE(*(ptr+1));
        const int y = GETJSAMPLE(*(ptr+2));
        const int k = GETJSAMPLE(*(ptr+3));

        rgb.r = (k*c)/maxval;
        rgb.g = (k*m)/maxval;
        rgb.b = (k*y)/maxval;
    }
        break;
    default:
        pm_error("Internal error: unknown color space %d passed to "
                 "read_rgb().", (int) color_space);
    }
    return(&rgb);
}



/* pnmbuffer is declared global because it would be improper to pass a
   pointer to it as input to copy_pixel_row(), since it isn't
   logically a parameter of the operation, but rather is private to
   copy_pixel_row().  But it would be impractical to allocate and free
   the storage with every call to copy_pixel_row().
*/
static xel *pnmbuffer;      /* Output buffer.  Input to pnm_writepnmrow() */

static void
copy_pixel_row(const JSAMPROW jpegbuffer, const int width, 
               const unsigned int samples_per_pixel, 
               const enum colorspace color_space,
               const unsigned int maxval,
               FILE * const output_file, const int output_type) {
  JSAMPLE *ptr;
  unsigned int output_cursor;     /* Cursor into output buffer 'pnmbuffer' */

  ptr = jpegbuffer;  /* Start at beginning of input row */

  for (output_cursor = 0; output_cursor < width; output_cursor++) {
      xel current_pixel;
      if (samples_per_pixel >= 3) {
          const rgb_type * const rgb_p = read_rgb(ptr, color_space, maxval);
          PPM_ASSIGN(current_pixel, rgb_p->r, rgb_p->g, rgb_p->b);
      } else {
          PNM_ASSIGN1(current_pixel, GETJSAMPLE(*ptr));
      }
      ptr += samples_per_pixel;  /* move to next pixel of input */
      pnmbuffer[output_cursor] = current_pixel;
  }
  pnm_writepnmrow(output_file, pnmbuffer, width,
                  maxval, output_type, FALSE);
}



static void
set_color_spaces(const J_COLOR_SPACE jpeg_color_space,
                 int * const output_type_p,
                 J_COLOR_SPACE * const out_color_space_p) {
/*----------------------------------------------------------------------------
   Decide what type of output (PPM or PGM) we shall generate and what 
   color space we must request from the JPEG decompressor, based on the
   color space of the input JPEG image, 'jpeg_color_space'.

   Write to stderr a message telling which type we picked.

   Exit the process with EXIT_FAILURE completion code and a message to
   stderr if the input color space is beyond our capability.
-----------------------------------------------------------------------------*/
    /* Note that the JPEG decompressor is not capable of translating
       CMYK or YCCK to RGB, but can translate YCCK to CMYK.
    */

    switch (jpeg_color_space) {
    case JCS_UNKNOWN:
        pm_error("Input JPEG image has 'unknown' color space "
                 "(JCS_UNKNOWN).\n"
                 "We cannot interpret this image.");
        break;
    case JCS_GRAYSCALE:
        *output_type_p = PGM_TYPE;
        *out_color_space_p = JCS_GRAYSCALE;
        break;
    case JCS_RGB:
        *output_type_p = PPM_TYPE;
        *out_color_space_p = JCS_RGB;
        break;
    case JCS_YCbCr:
        *output_type_p = PPM_TYPE;
        *out_color_space_p = JCS_RGB;
        /* Design note:  We found this YCbCr->RGB conversion increases
           user mode CPU time by 2.5%.  2002.10.12
        */
        break;
    case JCS_CMYK:
        *output_type_p = PPM_TYPE;
        *out_color_space_p = JCS_CMYK;
        break;
    case JCS_YCCK:
        *output_type_p = PPM_TYPE;
        *out_color_space_p = JCS_CMYK;
        break;
    default:
        pm_error("Internal error: unknown color space code %d passed "
                 "to set_color_spaces().", jpeg_color_space);
    }
    pm_message("WRITING %s FILE", 
               *output_type_p == PPM_TYPE ? "PPM" : "PGM");
}



static const char *
colorspace_name(const J_COLOR_SPACE jpeg_color_space) {

    const char *retval;

    switch(jpeg_color_space) {
    case JCS_UNKNOWN: retval = "JCS_UNKNOWN"; break;
    case JCS_GRAYSCALE: retval= "JCS_GRAYSCALE"; break;
    case JCS_RGB: retval = "JCS_RGB"; break;
    case JCS_YCbCr: retval = "JCS_YCbCr"; break;
    case JCS_CMYK: retval = "JCS_CMYK"; break;
    case JCS_YCCK: retval = "JCS_YCCK"; break;
    default: retval = "invalid"; break;
    };
    return(retval);
}



static void
print_verbose_info_about_header(struct jpeg_decompress_struct const cinfo){

    struct jpeg_marker_struct * markerP;

    pm_message("input color space is %d (%s)\n", 
               cinfo.jpeg_color_space, 
               colorspace_name(cinfo.jpeg_color_space));

    /* Note that raw information about marker, including marker type code,
       was already printed by the jpeg library, due to the jpeg library
       trace level >= 1.  Our job is to interpret it a little bit.
    */
    if (cinfo.marker_list)
        pm_message("Miscellaneous markers (excluding APP0, APP12) "
                   "in header:");
    else
        pm_message("No miscellaneous markers (excluding APP0, APP12) "
                   "in header");
    for (markerP = cinfo.marker_list; markerP; markerP = markerP->next) {
        if (markerP->marker == JPEG_COM)
            pm_message("Comment marker (COM):");
        else if (markerP->marker >= JPEG_APP0 && 
                 markerP->marker <= JPEG_APP0+15)
            pm_message("Miscellaneous marker type APP%d:", 
                       markerP->marker - JPEG_APP0);
        else
            pm_message("Miscellaneous marker of unknown type (0x%X):",
                       markerP->marker);
        
        print_marker(*markerP);
    }
}



static void
beginJpegInput(struct jpeg_decompress_struct * const cinfoP,
               const boolean verbose, 
               const J_DCT_METHOD dct_method, 
               const int max_memory_to_use, 
               const boolean nosmooth) {
/*----------------------------------------------------------------------------
   Read the JPEG header, create decompressor object (and
   allocate memory for it), set up decompressor.
-----------------------------------------------------------------------------*/
    /* Read file header, set default decompression parameters */
    jpeg_read_header(cinfoP, TRUE);

    cinfoP->dct_method = dct_method;
    if (max_memory_to_use != -1)
        cinfoP->mem->max_memory_to_use = max_memory_to_use;
    if (nosmooth)
        cinfoP->do_fancy_upsampling = FALSE;
    
}



static void
print_comments(struct jpeg_decompress_struct const cinfo) {
    
    struct jpeg_marker_struct * markerP;

    for (markerP = cinfo.marker_list;
         markerP; markerP = markerP->next) 
        if (markerP->marker == JPEG_COM) {
            pm_message("COMMENT:");
            print_marker(*markerP);
        }
}



static void
print_exif_info(struct jpeg_marker_struct const marker) {
/*----------------------------------------------------------------------------
   Dump as informational messages the contents of the Jpeg miscellaneous
   marker 'marker', assuming it is an Exif header.
-----------------------------------------------------------------------------*/
    ImageInfo_t imageInfo;
    const char * error;

    assert(marker.data_length >= 6);

    process_EXIF(marker.data+6, marker.data_length-6, 
                 &imageInfo, FALSE, &error);

    if (error) {
        pm_message("EXIF header is invalid.  %s", error);
        strfree(error);
    } else
        ShowImageInfo(&imageInfo);
}



static boolean
is_exif(struct jpeg_marker_struct const marker) {
/*----------------------------------------------------------------------------
   Return true iff the JPEG miscellaneous marker 'marker' is an Exif 
   header.
-----------------------------------------------------------------------------*/
    boolean retval;
    
    if (marker.marker == JPEG_APP0+1) {
        if (marker.data_length >=6 && memcmp(marker.data, "Exif", 4) == 0)
            retval = TRUE;
        else retval = FALSE;
    }
    else retval = FALSE;

    return retval;
}



static void
dump_exif(struct jpeg_decompress_struct const cinfo) {
/*----------------------------------------------------------------------------
   Dump as informational messages the contents of all EXIF headers in
   the image, interpreted.  An EXIF header is an APP1 marker.
-----------------------------------------------------------------------------*/
    struct jpeg_marker_struct * markerP;
    boolean found_one;

    found_one = FALSE;  /* initial value */

    for (markerP = cinfo.marker_list;
         markerP; markerP = markerP->next) 
        if (is_exif(*markerP)) {
            pm_message("EXIF INFO:");
            print_exif_info(*markerP);
            found_one = TRUE;
        }
    if (!found_one)
        pm_message("No EXIF info in image.");
}



static void
save_exif(struct jpeg_decompress_struct const cinfo, 
          const char *                  const exif_filespec) {
/*----------------------------------------------------------------------------
  Write the contents of the first Exif header in the image into the
  file with filespec 'exif_filespec'.  Start with the two byte length
  field.  If 'exif_filespec' is "-", write to standard output.

  If there is no Exif header in the image, write just zero, as a two
  byte pure binary integer.
-----------------------------------------------------------------------------*/
    FILE * exif_file;
    struct jpeg_marker_struct * markerP;

    exif_file = pm_openw(exif_filespec);

    for (markerP = cinfo.marker_list; 
         markerP && !is_exif(*markerP);
         markerP = markerP->next);

    if (markerP) {
        pm_writebigshort(exif_file, markerP->data_length+2);
        if (ferror(exif_file))
            pm_error("Write of Exif header to %s failed on first byte.",
                     exif_filespec);
        else {
            int rc;

            rc = fwrite(markerP->data, 1, markerP->data_length, exif_file);
            if (rc != markerP->data_length)
                pm_error("Write of Exif header to '%s' failed.  Wrote "
                         "length successfully, but then failed after "
                         "%d characters of data.", exif_filespec, rc);
        }
    } else {
        /* There is no Exif header in the image */
        pm_writebigshort(exif_file, 0);
        if (ferror(exif_file))
            pm_error("Write of Exif header file '%s' failed.", exif_filespec);
    }
    pm_close(exif_file);
}



static void
tellDetails(struct jpeg_decompress_struct const cinfo,
            xelval                        const maxval,
            int                           const output_type) {

    print_verbose_info_about_header(cinfo);

    pm_message("Input image data precision = %d bits", 
               cinfo.data_precision);
    pm_message("Output file will have format %c%c "
               "with max sample value of %d.", 
               (char) (output_type/256), (char) (output_type % 256),
               maxval);
}  



static enum colorspace
computeColorSpace(struct jpeg_decompress_struct * const cinfoP,
                  enum inklevel                   const inklevel) {
    
    enum colorspace colorSpace;

    if (cinfoP->out_color_space == JCS_GRAYSCALE)
        colorSpace = GRAYSCALE_COLORSPACE;
    else if (cinfoP->out_color_space == JCS_RGB)
        colorSpace = RGB_COLORSPACE;
    else if (cinfoP->out_color_space == JCS_CMYK) {
        switch (inklevel) {
        case ADOBE:
            colorSpace = CMYK_ADOBE_COLORSPACE; break;
        case NORMAL:
            colorSpace = CMYK_NORMAL_COLORSPACE; break;
        case GUESS:
            colorSpace = CMYK_ADOBE_COLORSPACE; break;
        }
    } else
        pm_error("Internal error: unacceptable output color space from "
                 "JPEG decompressor.");

    return colorSpace;
}



static void
convertImage(FILE *                          const ofP, 
             struct cmdlineInfo              const cmdline,
             struct jpeg_decompress_struct * const cinfoP) {

    int output_type;
        /* The type of output file, PGM or PPM.  Value is either PPM_TYPE
           or PGM_TYPE, which conveniently also pass as format values
           PPM_FORMAT and PGM_FORMAT.
        */
    JSAMPROW jpegbuffer;  /* Input buffer.  Filled by jpeg_scanlines() */
    unsigned int maxval;  
        /* The maximum value of a sample (color component), both in the input
           and the output.
        */
    enum colorspace color_space;
        /* The color space of the pixels coming out of the JPEG decompressor */

    beginJpegInput(cinfoP, cmdline.verbose, 
                   cmdline.dct_method, 
                   cmdline.max_memory_to_use, cmdline.nosmooth);
                   
    set_color_spaces(cinfoP->jpeg_color_space, &output_type, 
                     &cinfoP->out_color_space);

    maxval = (1 << cinfoP->data_precision) - 1;

    if (cmdline.verbose) 
        tellDetails(*cinfoP, maxval, output_type);

    /* Calculate output image dimensions so we can allocate space */
    jpeg_calc_output_dimensions(cinfoP);

    jpegbuffer = ((*cinfoP->mem->alloc_sarray)
                  ((j_common_ptr) cinfoP, JPOOL_IMAGE,
                   cinfoP->output_width * cinfoP->output_components, 
                   (JDIMENSION) 1)
        )[0];

    /* Start decompressor */
    jpeg_start_decompress(cinfoP);

    if (ofP)
        /* Write pnm output header */
        pnm_writepnminit(ofP, cinfoP->output_width, cinfoP->output_height,
                         maxval, output_type, FALSE);

    pnmbuffer = pnm_allocrow(cinfoP->output_width);
    
    color_space = computeColorSpace(cinfoP, cmdline.inklevel);

    /* Process data */
    while (cinfoP->output_scanline < cinfoP->output_height) {
        jpeg_read_scanlines(cinfoP, &jpegbuffer, 1);
        if (ofP)
            copy_pixel_row(jpegbuffer, cinfoP->output_width, 
                           cinfoP->out_color_components,
                           color_space, maxval, ofP, output_type);
    }

    if (cmdline.comments)
        print_comments(*cinfoP);
    if (cmdline.dumpexif)
        dump_exif(*cinfoP);
    if (cmdline.exif_filespec)
        save_exif(*cinfoP, cmdline.exif_filespec);

    pnm_freerow(pnmbuffer);

    /* Finish decompression and release decompressor memory. */
    jpeg_finish_decompress(cinfoP);
}




static void
saveMarkers(struct jpeg_decompress_struct * const cinfoP) {

    unsigned int app_type;
    /* Get all the miscellaneous markers (COM and APPn) saved for our
       later access.
    */
    jpeg_save_markers(cinfoP, JPEG_COM, 65535);
    for (app_type = 0; app_type <= 15; ++app_type) {
        if (app_type == 0 || app_type == 14) {
            /* The jpeg library uses APP0 and APP14 internally (see
               libjpeg.doc), so we don't mess with those.
            */
        } else
            jpeg_save_markers(cinfoP, JPEG_APP0 + app_type, 65535);
    }
}



static void
convertImages(FILE *                          const ofP,
              struct cmdlineInfo              const cmdline,
              struct jpeg_decompress_struct * const cinfoP,
              struct sourceManager *          const sourceManagerP) {
              
    if (cmdline.multiple) {
        unsigned int imageSequence;
        for (imageSequence = 0; dsDataLeft(sourceManagerP); ++imageSequence) {
            if (cmdline.verbose)
                pm_message("Reading Image %u", imageSequence);
            convertImage(ofP, cmdline, cinfoP);
        }
    } else {
        if (dsDataLeft(sourceManagerP))
            convertImage(ofP, cmdline, cinfoP);
        else
            pm_error("Input stream is empty");
    }
}



int
main(int argc, char **argv) {
    FILE * ofP;
    struct cmdlineInfo cmdline;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct sourceManager * sourceManagerP;

    pnm_init(&argc, argv);

    parse_command_line(argc, argv, &cmdline);

    if (cmdline.exif_filespec && STREQ(cmdline.exif_filespec, "-"))
        /* He's got exif going to stdout, so there can be no image output */
        ofP = NULL;
    else
        ofP = stdout;

    displayComments = cmdline.comments;

    /* Initialize the JPEG decompression object with default error handling. */
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    if (cmdline.trace_level == 0 && cmdline.verbose) 
        cinfo.err->trace_level = 1;
    else 
        cinfo.err->trace_level = cmdline.trace_level;
    
    saveMarkers(&cinfo);

    sourceManagerP = dsCreateSource(cmdline.input_filespec);

    cinfo.src = dsJpegSourceMgr(sourceManagerP);

    convertImages(ofP, cmdline, &cinfo, sourceManagerP);

    jpeg_destroy_decompress(&cinfo);

    if (ofP) {
        int rc;
        rc = fclose(ofP);
        if (rc == EOF) 
            pm_error("Error writing output file.  Errno = %s (%d).",
                     strerror(errno), errno);
    }

    dsDestroySource(sourceManagerP);

    free(cmdline.input_filespec);
  
    exit(jerr.num_warnings > 0 ? EXIT_WARNING : EXIT_SUCCESS);
}

