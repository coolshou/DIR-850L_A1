/* pnmtoxwd.c - read a portable anymap and produce a color X11 window dump
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <string.h>
#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"
#include "x11wd.h"


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input file */
    unsigned int pseudodepth;
    unsigned int directcolor;
};



static void 
parseCommandLine(int argc, 
                 char ** argv, 
                 struct cmdlineInfo  * const cmdlineP) {
/* --------------------------------------------------------------------------
   Parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
--------------------------------------------------------------------------*/
    optEntry *option_def;
    /* Instructions to optParseOptions3 on how to parse our options. */
    optStruct3 opt;

    unsigned int option_def_index;
    unsigned int depthSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);
  
    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "directcolor", OPT_FLAG,    NULL,   &cmdlineP->directcolor,  0);
    OPTENT3(0, "pseudodepth",    OPT_UINT,  &cmdlineP->pseudodepth,    
            &depthSpec,          0);
  
    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;   /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
    /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!depthSpec)
        cmdlineP->pseudodepth = 8;
    else {
        if (cmdlineP->pseudodepth < 1)
            pm_error("-pseudodepth option value must be at least 1.  "
                     "You specified %u", cmdlineP->pseudodepth);
        else if (cmdlineP->pseudodepth > 16)
            pm_error("-pseudodepth option value must be at most 16.  "
                     "You specified %u", cmdlineP->pseudodepth);
    }

    if (argc-1 == 0) 
        cmdlineP->inputFilespec = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->inputFilespec = argv[1];
}



static void
setupX11Header(X11WDFileHeader * const h11P,
               const char * const dumpname,
               unsigned int const cols,
               unsigned int const rows,
               int          const format,
               bool         const direct,
               bool         const grayscale,
               unsigned int const colors,
               unsigned int const pseudodepth) {

    /* Set up the header. */

    h11P->header_size = sizeof(*h11P) + (xwdval) strlen(dumpname) + 1;
    h11P->file_version = X11WD_FILE_VERSION;
    h11P->pixmap_format = ZPixmap;
    h11P->pixmap_width = cols;
    h11P->pixmap_height = rows;
    h11P->xoffset = 0;
    h11P->byte_order = MSBFirst;
    h11P->bitmap_bit_order = MSBFirst;
    h11P->window_width = cols;
    h11P->window_height = rows;
    h11P->window_x = 0;
    h11P->window_y = 0;
    h11P->window_bdrwidth = 0;

    if (direct) {
        h11P->pixmap_depth = 24;
        h11P->bitmap_unit = 32;
        h11P->bitmap_pad = 32;
        h11P->bits_per_pixel = 32;
        h11P->visual_class = DirectColor;
        h11P->colormap_entries = 256;
        h11P->ncolors = 256;
        h11P->red_mask = 0xff0000;
        h11P->green_mask = 0xff00;
        h11P->blue_mask = 0xff;
        h11P->bytes_per_line = cols * 4;
    } else {  /* pseudocolor -- i.e. regular paletted raster */
        if (grayscale) {
            if (PNM_FORMAT_TYPE(format) == PBM_TYPE) {
                h11P->pixmap_depth = 1;
                h11P->bits_per_pixel = 1;
                h11P->colormap_entries = colors;
                h11P->bytes_per_line = (cols + 7) / 8;
            } else {
                h11P->pixmap_depth = pseudodepth;
                h11P->bits_per_pixel = pseudodepth;
                h11P->colormap_entries = colors;
                h11P->bytes_per_line = cols;
            }
            h11P->bitmap_unit = 8;
            h11P->bitmap_pad = 8;
            h11P->visual_class = StaticGray;
            h11P->red_mask = 0;
            h11P->green_mask = 0;
            h11P->blue_mask = 0;
        } else {
            h11P->pixmap_depth = pseudodepth;
            h11P->bits_per_pixel = pseudodepth;
            h11P->visual_class = PseudoColor;
            h11P->colormap_entries = 1 << pseudodepth;
            h11P->red_mask = 0;
            h11P->green_mask = 0;
            h11P->blue_mask = 0;
            h11P->bytes_per_line = cols;
            h11P->bitmap_unit = 8;
            h11P->bitmap_pad = 8;
        }
        h11P->ncolors = colors;
    }
    h11P->bits_per_rgb = h11P->pixmap_depth;
}




static void
writeX11Header(X11WDFileHeader const h11,
               FILE *          const ofP) {

    /* Write out the header in big-endian order. */

    pm_writebiglong(ofP, h11.header_size);
    pm_writebiglong(ofP, h11.file_version);
    pm_writebiglong(ofP, h11.pixmap_format);
    pm_writebiglong(ofP, h11.pixmap_depth);
    pm_writebiglong(ofP, h11.pixmap_width);
    pm_writebiglong(ofP, h11.pixmap_height);
    pm_writebiglong(ofP, h11.xoffset);
    pm_writebiglong(ofP, h11.byte_order);
    pm_writebiglong(ofP, h11.bitmap_unit);
    pm_writebiglong(ofP, h11.bitmap_bit_order);
    pm_writebiglong(ofP, h11.bitmap_pad);
    pm_writebiglong(ofP, h11.bits_per_pixel);
    pm_writebiglong(ofP, h11.bytes_per_line);
    pm_writebiglong(ofP, h11.visual_class);
    pm_writebiglong(ofP, h11.red_mask);
    pm_writebiglong(ofP, h11.green_mask);
    pm_writebiglong(ofP, h11.blue_mask);
    pm_writebiglong(ofP, h11.bits_per_rgb);
    pm_writebiglong(ofP, h11.colormap_entries);
    pm_writebiglong(ofP, h11.ncolors);
    pm_writebiglong(ofP, h11.window_width);
    pm_writebiglong(ofP, h11.window_height);
    pm_writebiglong(ofP, h11.window_x);
    pm_writebiglong(ofP, h11.window_y);
    pm_writebiglong(ofP, h11.window_bdrwidth);
}



static void
writePseudoColormap(FILE *           const ofP,
                    colorhist_vector const chv,
                    unsigned int     const colors,
                    bool             const grayscale,
                    bool             const backwardMap,
                    xelval           const maxval) {
    /* Write out the colormap, big-endian order. */
    
    X11XColor color;
    unsigned int i;

    color.flags = 7;
    color.pad = 0;
    for (i = 0; i < colors; ++i) {
        color.num = i;
        if (grayscale) {
            /* Stupid hack because xloadimage and xwud disagree on
               how to interpret bitmaps. 
            */
            if (backwardMap)
                color.red = (long) (colors-1-i) * 65535L / (colors - 1);
            else
                color.red = (long) i * 65535L / (colors - 1);
            
            color.green = color.red;
            color.blue = color.red;
        } else {
            color.red = PPM_GETR(chv[i].color);
            color.green = PPM_GETG(chv[i].color);
            color.blue = PPM_GETB(chv[i].color);
            if (maxval != 65535L) {
                color.red = (long) color.red * 65535L / maxval;
                color.green = (long) color.green * 65535L / maxval;
                color.blue = (long) color.blue * 65535L / maxval;
            }
        }
        pm_writebiglong( ofP, color.num);
        pm_writebigshort(ofP, color.red);
        pm_writebigshort(ofP, color.green);
        pm_writebigshort(ofP, color.blue);
        putc(color.flags, ofP);
        putc(color.pad,   ofP);
    }
}



static void
writeDirectColormap(FILE * const ofP) {
/*----------------------------------------------------------------------------
   Write the XWD colormap.

   We use a constant (independent of input) color map which simply has
   each of the values 0-255, scaled to 16 bits, for each of the three
   maps
-----------------------------------------------------------------------------*/
    X11XColor color;
    unsigned int i;

    color.flags = 7;
    color.pad = 0;

    for (i = 0; i < 256; ++i) {
        color.red   = (short)(i << 8 | i);
        color.green = (short)(i << 8 | i);
        color.blue  = (short)(i << 8 | i);
        color.num   = i << 16 | i << 8 | i;
        
        pm_writebiglong( ofP, color.num);
        pm_writebigshort(ofP, color.red);
        pm_writebigshort(ofP, color.green);
        pm_writebigshort(ofP, color.blue);
        putc(color.flags, ofP);
        putc(color.pad,   ofP);
    }
}



static void
writeRowDirect(FILE *       const ofP,
               xel *        const xelrow,
               unsigned int const cols,
               int          const format,
               long         const xmaxval,
               xelval       const maxval) {
    
    switch (PNM_FORMAT_TYPE(format)) {
    case PPM_TYPE: {
        unsigned int col;
        for (col = 0; col < cols; ++col) {
            unsigned long const ul = 
                ((PPM_GETR(xelrow[col]) * xmaxval / maxval) << 16) |
                ((PPM_GETG(xelrow[col]) * xmaxval / maxval) << 8) |
                (PPM_GETB(xelrow[col]) * xmaxval / maxval);
            pm_writebiglong(ofP, ul);
        }
    }
    break;

    default: {
        unsigned int col;
        for (col = 0; col < cols; ++col) {
            unsigned long const val = PNM_GET1(xelrow[col]);
            unsigned long const ul =
                ((val * xmaxval / maxval) << 16) |
                ((val * xmaxval / maxval) << 8) |
                (val * xmaxval / maxval);
            pm_writebiglong(ofP, ul);
        }
        break;
    }
    }
}



static void
writeRowGrayscale(FILE *       const ofP,
                  xel *        const xelrow,
                  unsigned int const cols,
                  xelval       const maxval,
                  bool         const backwardMap,
                  unsigned int const bitsPerPixel) {

    xelval bigger_maxval;
    int bitshift;
    unsigned char byte;
    unsigned int col;
    
    bigger_maxval = pm_bitstomaxval(bitsPerPixel);
    bitshift = 8 - bitsPerPixel;
    byte = 0;
    for (col = 0; col < cols; ++col) {
        xelval s;
        s = PNM_GET1(xelrow[col]);
        if (backwardMap)
            s = 1 - s;
        
        if (maxval != bigger_maxval)
            s = (long) s * bigger_maxval / maxval;
        byte |= s << bitshift;
        bitshift -= bitsPerPixel;
        if (bitshift < 0) {
            putc(byte, stdout);
            bitshift = 8 - bitsPerPixel;
            byte = 0;
        }
    }
    if (bitshift < 8 - bitsPerPixel)
        putc(byte, ofP);
}



static void
writeRowPseudoColor(FILE *          const ofP,
                    xel *           const xelrow,
                    unsigned int    const cols,
                    colorhash_table const cht) {
                       
    unsigned int col;

    for (col = 0; col < cols; ++col)
        putc(ppm_lookupcolor(cht, &xelrow[col]), ofP);
}



static void
writeRaster(FILE *           const ofP,
            xel **           const xels,
            unsigned int     const cols,
            unsigned int     const rows,
            int              const format,
            xelval           const maxval,
            long             const xmaxval,
            colorhist_vector const chv,
            unsigned int     const colors,
            bool             const direct,
            bool             const grayscale,
            bool             const backwardMap,
            unsigned int     const bitsPerPixel) {

    unsigned int row;
    colorhash_table cht;

    if (chv)
        cht = ppm_colorhisttocolorhash(chv, colors);
            
    for (row = 0; row < rows; ++row) {
        if (direct)
            writeRowDirect(ofP, xels[row], cols, format, xmaxval, maxval);
        else {
            if (grayscale)
                writeRowGrayscale(ofP, xels[row], cols, maxval, backwardMap,
                                  bitsPerPixel);
            else
                writeRowPseudoColor(ofP, xels[row], cols, cht);
        }
    }
}



int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE* ifP;
    xel ** xels;
    int rows, cols, format, colors;
    xelval maxval;
    long xmaxval;
    colorhist_vector chv;
    X11WDFileHeader h11;
    bool direct, grayscale;
    const char * dumpname;
    bool backwardMap;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    xels = pnm_readpnm(ifP, &cols, &rows, &maxval, &format);
    xmaxval = (1 << cmdline.pseudodepth) - 1;
    pm_close(ifP);
    
    if (cmdline.directcolor) {
        direct = TRUE;
        grayscale = FALSE;
        chv = NULL;
    } else {
        /* Figure out the colormap. */
        switch (PNM_FORMAT_TYPE(format)) {
        case PPM_TYPE:
            pm_message("computing colormap...");
            chv = ppm_computecolorhist(xels, cols, rows, xmaxval+1, &colors);
            if (!chv) {
                pm_message("Too many colors - "
                           "proceeding to write a 24-bit DirectColor "
                           "dump file.  If you want PseudoColor, "
                           "try doing a 'pnmquant %ld'.",
                           xmaxval);
                direct = TRUE;
            } else {
                pm_message("%d colors found", colors);
                direct = FALSE;
            }
            grayscale = FALSE;
            break;

        case PBM_TYPE:
            chv = NULL;
            direct = FALSE;
            grayscale = TRUE;
            colors = 2;
            break;
        case PGM_TYPE:
            chv = NULL;
            direct = FALSE;
            grayscale = TRUE;
            colors = xmaxval + 1;
            break;
        default:
            pm_error("INTERNAL ERROR: impossible format type");
        }
    }

    if (STREQ(cmdline.inputFilespec, "-"))
        dumpname = "stdin";
    else {
        if (strlen(cmdline.inputFilespec) > XWDVAL_MAX - sizeof(h11) - 1)
            pm_error("Input file name is ridiculously long.");
        else
            dumpname = cmdline.inputFilespec;
    }

    setupX11Header(&h11, dumpname, cols, rows, format, 
                   direct, grayscale, colors,
                   cmdline.pseudodepth);

    writeX11Header(h11, stdout);

    /* Write out the dump name. */
    fwrite(dumpname, 1, strlen(dumpname) + 1, stdout);

    backwardMap = PNM_FORMAT_TYPE(format) == PBM_TYPE;

    if (direct)
        writeDirectColormap(stdout);
    else
        writePseudoColormap(stdout, chv, colors, 
                            grayscale, backwardMap, maxval);
    
    writeRaster(stdout, xels, cols, rows, format, maxval, xmaxval, 
                chv, colors, direct, grayscale,
                backwardMap, h11.bits_per_pixel);

    return 0;
}
