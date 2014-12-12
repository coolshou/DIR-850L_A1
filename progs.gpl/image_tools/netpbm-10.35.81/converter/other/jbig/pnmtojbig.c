/*
    pnmtojbig - PNM to JBIG converter
  
    This program was derived from pbmtojbg.c in Markus Kuhn's
    JBIG-KIT package by Bryan Henderson on 2000.05.11

    The main difference is that this version uses the Netpbm libraries.

 */

/*
     The JBIG standard doesn't say which end of the scale is white and
     which end is black in a BIE.  It has a recommendation in terms of
     foreground and background (a concept which does not exist in the
     Netpbm formats) for single-plane images, and is silent for
     multi-plane images.

     Kuhn's implementation of the JBIG standard says if the BIE has a
     single plane, then in that plane a zero bit means white and a one
     bit means black.  But if it has multiple planes, a composite zero
     value means black and a composite maximal value means white.

     Actually, Kuhn's pbmtojbg doesn't even implement this, but rather
     bases the distinction on whether the input file was PBM or PGM.
     This means that if you convert a PGM file with maxval 1 to a JBIG
     file and then back, the result (which is a PBM file) is the
     inverse of what you started with.  Same if the PGM file has
     maxval > 1 but you use a -t option to write only one plane.  We
     assume this is just a bug in pbmtojpg and that hardly anybody does
     this.  So we adopt the implementation described above.

     This means that after jbg_split_planes() hands us a set of bitmap
     planes, if there is only one of them, we have to invert all the
     bits in it.

*/
  
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <jbig.h>

#include "pm_c_util.h"
#include "mallocvar.h"
#include "pnm.h"

static unsigned long total_length = 0;  
  /* used for determining output file length */

/*
 * malloc() with exception handler
 */
static void
*checkedmalloc(size_t n)
{
  void *p;
  
  if ((p = malloc(n)) == NULL) {
    fprintf(stderr, "Sorry, not enough memory available!\n");
    exit(1);
  }
  
  return p;
}


/*
 * Callback procedure which is used by JBIG encoder to deliver the
 * encoded data. It simply sends the bytes to the output file.
 */
static void data_out(unsigned char *start, size_t len, void *file)
{
  fwrite(start, len, 1, (FILE *) file);
  total_length += len;
  return;
}



static void
readPbm(FILE *            const fin,
        unsigned int      const cols,
        unsigned int      const rows,
        unsigned char *** const bitmapP) {

    unsigned int const bytes_per_line = pbm_packed_bytes(cols);

    unsigned char ** bitmap;

    /* Read the input image into bitmap[] */
    /* Shortcut for PBM */
    int row;
    bitmap = (unsigned char **) checkedmalloc(sizeof(unsigned char *));
    bitmap[0] = (unsigned char *) checkedmalloc(bytes_per_line * rows);
    
    for (row = 0; row < rows; row++)
        pbm_readpbmrow_packed(fin, &bitmap[0][row*bytes_per_line],
                              cols, RPBM_FORMAT);

    *bitmapP = bitmap;
} 



static void
readImage(FILE * const fin,
          unsigned int     const cols,
          unsigned int     const rows,
          xelval           const maxval,
          int              const format,
          unsigned int     const bpp,
          unsigned char ** const imageP) {
/*----------------------------------------------------------------------------
  Read the input image and put it into *imageP;

  Although the PBM case is separated, this logic works also for
  PBM, bpp=1.
-----------------------------------------------------------------------------*/
    unsigned char *image;
        /* This is a representation of the entire image with 'bpp' bytes per
           pixel.  The 'bpp' bytes for each pixel are arranged MSB first
           and its numerical value is the value from the PNM input.
           The pixels are laid out in row-major format in this rectangle.
           
           The point of this data structure is it is what jbg_split_planes()
           wants for input.
        */
    xel* pnm_row;
    unsigned int row;

    pnm_row = pnm_allocrow(cols);  /* row buffer */
    MALLOCARRAY_NOFAIL(image, cols * rows * bpp);
    
    for (row = 0; row < rows; ++row) {
        unsigned int col;
        pnm_readpnmrow(fin, pnm_row, cols, maxval, format);
        for (col = 0; col < cols; col++) {
            unsigned int j;
            /* Move each byte of the sample into image[], MSB first */
            for (j = 0; j < bpp; ++j)
                image[(((row*cols)+col) * bpp) + j] = (unsigned char)
                    PNM_GET1(pnm_row[col]) >> ((bpp-1-j) * 8);
        }
    }
    pnm_freerow(pnm_row);
    *imageP = image;
}
      


static void
convertImageToBitmap(unsigned char *   const image,
                     unsigned char *** const bitmapP,
                     unsigned int      const encode_planes,
                     unsigned int      const bytes_per_line,
                     unsigned int      const lines) {
    
    /* Convert image[] into bitmap[]  */
    
    unsigned char ** bitmap;
    unsigned int i;
    
    MALLOCARRAY_NOFAIL(bitmap, encode_planes);
    for (i = 0; i < encode_planes; ++i)
        MALLOCARRAY_NOFAIL(bitmap[i], bytes_per_line * lines);

    *bitmapP = bitmap;
}



static void
readPnm(FILE *            const fin,
        unsigned int      const cols,
        unsigned int      const rows,
        xelval            const maxval,
        int               const format,
        unsigned int      const bpp,
        unsigned int      const planes,
        unsigned int      const encode_planes,
        bool              const use_graycode,
        unsigned char *** const bitmapP) {

    unsigned int const bytes_per_line = pbm_packed_bytes(cols);

    unsigned char * image;
    unsigned char ** bitmap;

    readImage(fin, cols, rows, maxval, format, bpp, &image);

    convertImageToBitmap(image, &bitmap, encode_planes, bytes_per_line, rows);

    jbg_split_planes(cols, rows, planes, encode_planes, image, bitmap,
                     use_graycode);
    free(image);
    
    /* Invert the image if it is just one plane.  See top of this file
       for an explanation why.  Due to the separate handling of PBM,
       this is for exceptional PGM files.  
    */

    if (encode_planes == 1) {
        unsigned int row;
        for (row = 0; row < rows; ++row) {
            unsigned int i;
            for (i = 0; i < bytes_per_line; i++)
                bitmap[0][(row*bytes_per_line) + i] ^= 0xff;

            if (cols % 8 > 0) {   
                bitmap[0][ (row+1)*bytes_per_line  -1] >>= 8-cols%8;
                bitmap[0][ (row+1)*bytes_per_line  -1] <<= 8-cols%8;
            }
        }
    }
    *bitmapP = bitmap;
}



int
main(int argc, char **argv) {
    FILE *fin = stdin, *fout = stdout;
    const char *fnin = "<stdin>", *fnout = "<stdout>";
    int i;
    int all_args = 0, files = 0;
    int bpp, planes, encode_planes = -1;
    int cols, rows;
    xelval maxval;
    int format;
    unsigned char **bitmap;
    /* This is an array of the planes of the image.  Each plane is a
       two-dimensional array of pixels laid out in row-major format.
       format with each pixel being one bit.  A byte in the array 
       contains 8 pixels left to right, msb to lsb.
    */

    struct jbg_enc_state s;
    int verbose = 0, delay_at = 0, use_graycode = 1;
    long mwidth = 640, mheight = 480;
    int dl = -1, dh = -1, d = -1, l0 = -1, mx = -1;
    int options = JBG_TPDON | JBG_TPBON | JBG_DPON;
    int order = JBG_ILEAVE | JBG_SMID;

    pbm_init(&argc, argv);

    /* parse command line arguments */
    for (i = 1; i < argc; ++i) {
        int j;
        if (!all_args && argv[i][0] == '-') {
            if (argv[i][1] == '\0' && files == 0)
                ++files;
            else {
                for (j = 1; j > 0 && argv[i][j]; j++) {
                    switch(tolower(argv[i][j])) {
                    case '-' :
                        all_args = 1;
                        break;
                    case 'v':
                        verbose = 1;
                        break;
                    case 'b':
                        use_graycode = 0;
                        break;
                    case 'c':
                        delay_at = 1;
                        break;
                    case 'x':
                        if (++i >= argc)
                            pm_error("-x needs a value");
                        j = -1;
                        mwidth = atol(argv[i]);
                        break;
                    case 'y':
                        if (++i >= argc)
                            pm_error("-y needsa  value");
                        j = -1;
                        mheight = atol(argv[i]);
                        break;
                    case 'o':
                        if (++i >= argc)
                            pm_error("-o needs a value");
                        j = -1;
                        order = atoi(argv[i]);
                        break;
                    case 'p':
                        if (++i >= argc)
                            pm_error("-p needs a value");
                        j = -1;
                        options = atoi(argv[i]);
                        break;
                    case 'l':
                        if (++i >= argc)
                            pm_error("-l needs a value");
                        j = -1;
                        dl = atoi(argv[i]);
                        break;
                    case 'h':
                        if (++i >= argc)
                            pm_error("-h needs a value");
                        j = -1;
                        dh = atoi(argv[i]);
                        break;
                    case 'q':
                        d = 0;
                        break;
                    case 'd':
                        if (++i >= argc)
                            pm_error("-d needs a value");
                        j = -1;
                        d = atoi(argv[i]);
                        break;
                    case 's':
                        if (++i >= argc)
                            pm_error("-s needs a value");
                        j = -1;
                        l0 = atoi(argv[i]);
                        break;
                    case 't':
                        if (++i >= argc)
                            pm_error("-t needs a value");
                        j = -1;
                        encode_planes = atoi(argv[i]);
                        break;
                    case 'm':
                        if (++i >= argc)
                            pm_error("-m needs a value");
                        j = -1;
                        mx = atoi(argv[i]);
                        break;
                    default:
                        pm_error("Unrecognized option: %c", argv[i][j]);
                    }
                }
            }
        } else {
            switch (files++) {
            case 0:
                if (argv[i][0] != '-' || argv[i][1] != '\0') {
                    fnin = argv[i];
                    fin = fopen(fnin, "rb");
                    if (!fin) {
                        fprintf(stderr, "Can't open input file '%s", fnin);
                        perror("'");
                        exit(1);
                    }
                }
                break;
            case 1:
                fnout = argv[i];
                fout = fopen(fnout, "wb");
                if (!fout) {
                    fprintf(stderr, "Can't open input file '%s", fnout);
                    perror("'");
                    exit(1);
                }
                break;
            default:
                pm_error("too many non-option arguments");
            }
        }
    }

    pnm_readpnminit(fin, &cols, &rows, &maxval, &format);

    if (PNM_FORMAT_TYPE(format) != PGM_TYPE &&
        PNM_FORMAT_TYPE(format) != PBM_TYPE)
        pm_error("This program accepts PBM and PGM input only.  "
                 "Try Ppmtopgm.");

    planes = pm_maxvaltobits(maxval);

    /* In a JBIG file, maxvals are determined only by the number of planes,
       so must be a power of 2 minus 1
    */
  
    if ((1UL << planes)-1 != maxval) 
        pm_error("Input image has unacceptable maxval: %d.  JBIG files must "
                 "have a maxval which is a power of 2 minus 1.  Use "
                 "Ppmdepth to adjust the image's maxval", maxval);

    bpp = (planes + 7) / 8;

    if (encode_planes < 0 || encode_planes > planes)
        encode_planes = planes;

    if (bpp == 1 && PNM_FORMAT_TYPE(format) == PBM_TYPE)
        readPbm(fin, cols, rows, &bitmap);
    else
        readPnm(fin, cols, rows, maxval, format, bpp, 
                planes, encode_planes, use_graycode, 
                &bitmap);

    /* Apply JBIG algorithm and write BIE to output file */

  /* initialize parameter struct for JBIG encoder*/
    jbg_enc_init(&s, cols, rows, encode_planes, bitmap, data_out, fout);

    /* Select number of resolution layers either directly or based
   * on a given maximum size for the lowest resolution layer */
    if (d >= 0)
        jbg_enc_layers(&s, d);
    else
        jbg_enc_lrlmax(&s, mwidth, mheight);

  /* Specify a few other options (each is ignored if negative) */
    if (delay_at)
        options |= JBG_DELAY_AT;
    jbg_enc_lrange(&s, dl, dh);
    jbg_enc_options(&s, order, options, l0, mx, -1);

  /* now encode everything and send it to data_out() */
    jbg_enc_out(&s);

    /* give encoder a chance to free its temporary data structures */
    jbg_enc_free(&s);

    /* check for file errors and close fout */
    if (ferror(fout) || fclose(fout)) {
        fprintf(stderr, "Problem while writing output file '%s", fnout);
        perror("'");
        exit(1);
    }

    /* In case the user wants to know all the gory details ... */
    if (verbose) {
        fprintf(stderr, "Information about the created JBIG bi-level image entity "
                "(BIE):\n\n");
        fprintf(stderr, "              input image size: %ld x %ld pixel\n",
                s.xd, s.yd);
        fprintf(stderr, "                    bit planes: %d\n", s.planes);
        if (s.planes > 1)
            fprintf(stderr, "                      encoding: %s code, MSB first\n",
                    use_graycode ? "Gray" : "binary");
        fprintf(stderr, "                       stripes: %ld\n", s.stripes);
        fprintf(stderr, "   lines per stripe in layer 0: %ld\n", s.l0);
        fprintf(stderr, "  total number of diff. layers: %d\n", s.d);
        fprintf(stderr, "           lowest layer in BIE: %d\n", s.dl);
        fprintf(stderr, "          highest layer in BIE: %d\n", s.dh);
        fprintf(stderr, "             lowest layer size: %lu x %lu pixel\n",
                jbg_ceil_half(s.xd, s.d - s.dl), jbg_ceil_half(s.yd, s.d - s.dl));
        fprintf(stderr, "            highest layer size: %lu x %lu pixel\n",
                jbg_ceil_half(s.xd, s.d - s.dh), jbg_ceil_half(s.yd, s.d - s.dh));
        fprintf(stderr, "                   option bits:%s%s%s%s%s%s%s\n",
                s.options & JBG_LRLTWO  ? " LRLTWO" : "",
                s.options & JBG_VLENGTH ? " VLENGTH" : "",
                s.options & JBG_TPDON   ? " TPDON" : "",
                s.options & JBG_TPBON   ? " TPBON" : "",
                s.options & JBG_DPON    ? " DPON" : "",
                s.options & JBG_DPPRIV  ? " DPPRIV" : "",
                s.options & JBG_DPLAST  ? " DPLAST" : "");
        fprintf(stderr, "                    order bits:%s%s%s%s\n",
                s.order & JBG_HITOLO ? " HITOLO" : "",
                s.order & JBG_SEQ    ? " SEQ" : "",
                s.order & JBG_ILEAVE ? " ILEAVE" : "",
                s.order & JBG_SMID   ? " SMID" : "");
        fprintf(stderr, "           AT maximum x-offset: %d\n"
                "           AT maximum y-offset: %d\n", s.mx, s.my);
        fprintf(stderr, "         length of output file: %lu byte\n\n",
                total_length);
    }

    return 0;
}
