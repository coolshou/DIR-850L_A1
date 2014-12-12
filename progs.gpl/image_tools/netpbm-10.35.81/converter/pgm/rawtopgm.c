/* rawtopgm.c - convert raw grayscale bytes into a portable graymap
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <math.h>
#include "pgm.h"
#include "shhopt.h"

struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespec of input file */
    unsigned int headerskip;
    float rowskip;
    int bottomfirst;  /* the -bottomfirst/-bt option */
    int autosize;  /* User wants us to figure out the size */
    int width;
    int height;
    int bpp;
      /* bytes per pixel in input format.  1 or 2 */
    int littleendian;
      /* logical: samples in input are least significant byte first */
    int maxval;  /* -maxval option, or -1 if none */
};


static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdline_p) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optStruct *option_def = malloc(100*sizeof(optStruct));
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct2 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENTRY(0,   "bottomfirst",   OPT_FLAG,   &cmdline_p->bottomfirst,    0);
    OPTENTRY(0,   "bt",            OPT_FLAG,   &cmdline_p->bottomfirst,    0);
    OPTENTRY(0,   "topbottom",     OPT_FLAG,   &cmdline_p->bottomfirst,    0);
    OPTENTRY(0,   "tb",            OPT_FLAG,   &cmdline_p->bottomfirst,    0);
    OPTENTRY(0,   "headerskip",    OPT_UINT,   &cmdline_p->headerskip,     0);
    OPTENTRY(0,   "rowskip",       OPT_FLOAT,  &cmdline_p->rowskip,        0);
    OPTENTRY(0,   "bpp",           OPT_INT,    &cmdline_p->bpp,            0);
    OPTENTRY(0,   "littleendian",  OPT_FLAG,   &cmdline_p->littleendian,   0);
    OPTENTRY(0,   "maxval",        OPT_UINT,   &cmdline_p->maxval,         0);

    /* Set the defaults */
    cmdline_p->bottomfirst = FALSE;
    cmdline_p->headerskip = 0;
    cmdline_p->rowskip = 0.0;
    cmdline_p->bpp = 1;
    cmdline_p->littleendian = 0;
    cmdline_p->maxval = -1;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions2(&argc, argv, opt, 0);
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (argc-1 == 0) {
        cmdline_p->input_filespec = "-";
        cmdline_p->autosize = TRUE;
    } else if (argc-1 == 1) {
        cmdline_p->input_filespec = argv[1];
        cmdline_p->autosize = TRUE;
    } else if (argc-1 == 2) {
        cmdline_p->input_filespec = "-";
        cmdline_p->autosize = FALSE;
        cmdline_p->width = atoi(argv[1]);
        cmdline_p->height = atoi(argv[2]);
    } else if (argc-1 == 3) {
        cmdline_p->input_filespec = argv[3];
        cmdline_p->autosize = FALSE;
        cmdline_p->width = atoi(argv[1]);
        cmdline_p->height = atoi(argv[2]);
    } else
        pm_error("Program takes zero, one, two, or three arguments.  You "
                 "specified %d", argc-1);

    if (cmdline_p->bpp != 1 && cmdline_p->bpp != 2) 
        pm_error("Bytes per pixel (-bpp) must be 1 or 2.  You specified %d.",
                 cmdline_p->bpp);

    if (cmdline_p->maxval == 0)
        pm_error("Maxval (-maxval) may not be zero.");

    if (cmdline_p->maxval > 255 && cmdline_p->bpp == 1)
        pm_error("You have specified one byte per pixel, but a maxval "
                 "too large to fit in one byte: %d", cmdline_p->maxval);
    if (cmdline_p->maxval > 65535)
        pm_error("Maxval must be less than 65536.  You specified %d.",
                 cmdline_p->maxval);

    if (cmdline_p->width <= 0) 
        pm_error("Width must be a positive number.");
    if (cmdline_p->height <= 0) 
        pm_error("Height must be a positive number.");

    if (cmdline_p->rowskip && cmdline_p->autosize)
        pm_error("If you specify -rowskip, you must also give the image "
                 "dimensions.");
    if (cmdline_p->rowskip && cmdline_p->bottomfirst)
        pm_error("You canot specify both -rowskip and -bottomfirst.  This is "
                 "a limitation of this program.");

}



static void
compute_image_size(const struct cmdline_info cmdline, const long nread,
                   int * const rows_p, int * const cols_p) {

    if (cmdline.autosize) {
        int sqrt_trunc = 
            (int) sqrt((double) (nread-cmdline.headerskip));
        if (sqrt_trunc*sqrt_trunc+cmdline.headerskip != nread) 
            pm_error( "You must specify the dimensions of the image unless "
                      "it is a quadratic image.  This one is not quadratic: "
                      "The number of "
                      "pixels in the input is %ld, which is not a perfect "
                      "square.", nread-cmdline.headerskip);
        *rows_p = *cols_p = sqrt_trunc;
        pm_message( "Image size: %d cols, %d rows", *cols_p, *rows_p);
    } else {
        *rows_p = cmdline.height;
        *cols_p = cmdline.width;
    }
}



static void
skip_header(FILE *ifp, const int headerskip) {
    int i;

    for ( i = 0; i < headerskip; ++i ) {
        /* Read a byte out of the file */
        int val;
        val = getc( ifp );
        if ( val == EOF )
            pm_error("EOF / read error reading Byte %d in the header", i );
    }
}



static gray
read_from_file(FILE *ifp, const int bpp, const int row, const int col,
               const int littleendian) {
/*----------------------------------------------------------------------------
   Return the next sample value from the input file 'ifp', assuming the
   input stream is 'bpp' bytes per pixel (1 or 2).  In the case of two
   bytes, if 'littleendian', assume least significant byte is first.
   Otherwise, assume MSB first.
   
   In error messages, say this is Column 'col', Row 'row'.  Exit program if
   error.
-----------------------------------------------------------------------------*/
    gray retval;

    if (bpp == 1) {
        int val;
        val = getc(ifp);
        if (val == EOF)
            pm_error( "EOF / read error at Row %d Column %d",
                      row, col);
        retval = (gray) val;
    } else {
        short val;
        int rc;
        rc = littleendian ? 
            pm_readlittleshort(ifp, &val) : pm_readbigshort(ifp, &val);
        if (rc != 0)
            pm_error( "EOF / read error at Row %d Column %d",
                      row, col);
        retval = (gray) val;
    }
    return retval;
}



int
main(int argc, char *argv[] ) {

    struct cmdline_info cmdline;
    FILE* ifp;
    gray* grayrow;
    int rows, cols;
    gray maxval;
    char* buf;
    /* pixels_1 and pixels_2 are the array of pixels in the input buffer
       (assuming we are using an input buffer).  pixels_1 is the array
       as if the pixels are one byte each.  pixels_2 is the array as if
       they are two bytes each.
       */
    unsigned char *pixels_1;  
    unsigned short *pixels_2;
    long nread;
    int row;
    float toskip;

    pgm_init( &argc, argv );

    parse_command_line(argc, argv, &cmdline);

    ifp = pm_openr(cmdline.input_filespec);

    if (cmdline.autosize || cmdline.bottomfirst) {
        buf = pm_read_unknown_size( ifp, &nread );
        pixels_1 = (unsigned char *) buf;
        pixels_2 = (unsigned short *) buf;
    } else
        buf = NULL;

    compute_image_size(cmdline, nread, &rows, &cols);

    if (!buf)
        skip_header(ifp, cmdline.headerskip);

    toskip = 0.00001;

    if (cmdline.maxval == -1)
        maxval = (cmdline.bpp == 1 ? (gray) 255 : (gray) 65535);
    else
        maxval = (gray) cmdline.maxval;

    pgm_writepgminit( stdout, cols, rows, maxval, 0 );
    grayrow = pgm_allocrow( cols );

    for ( row = 0; row < rows; ++row) {
        int col;
        unsigned int rowpos; /* index of this row in pixel array */
        if (cmdline.bottomfirst)
            rowpos = (rows-row-1) * cols;
        else
            rowpos = row * cols;

        for ( col = 0; col < cols; ++col )
            if (buf) {
                if (cmdline.bpp == 1)
                    grayrow[col] = pixels_1[rowpos+col];
                else
                    grayrow[col] = pixels_2[rowpos+col];
            } else {
                grayrow[col] = read_from_file(ifp, cmdline.bpp, 
                                              row, col,
                                              cmdline.littleendian);
            }
        for ( toskip += cmdline.rowskip; toskip >= 1.0; toskip -= 1.0 ) {
            /* Note that if we're using a buffer, cmdline.rowskip is zero */
            int val;
            val = getc( ifp );
            if ( val == EOF )
                pm_error( "EOF / read error skipping bytes at the end "
                          "of Row %d.", row);
        }
        pgm_writepgmrow( stdout, grayrow, cols, maxval, 0 );
    }
    
    if (buf)
        free(buf);
    pm_close( ifp );
    pm_close( stdout );

    exit( 0 );
}
