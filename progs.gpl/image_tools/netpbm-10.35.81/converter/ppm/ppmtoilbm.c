/* ppmtoilbm.c - read a portable pixmap and produce an IFF ILBM file
**
** Copyright (C) 1989 by Jef Poskanzer.
** Modified by Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
**  20/Jun/93:
**  - 24-bit capability (new options -24if, -24force)
**  - HAM8 capability (well, anything from HAM3 to HAM(MAXPLANES))
**  - now writes up to 8 (16) planes (new options -maxplanes, -fixplanes)
**  - colormap file (new option -map)
**  - write colormap only (new option -cmaponly)
**  - only writes CAMG chunk if it is a HAM-picture
**  29/Aug/93:
**  - operates row-by-row whenever possible
**  - faster colorscaling with lookup-table (~20% faster on HAM pictures)
**  - options -ham8 and -ham6 now imply -hamforce
**  27/Nov/93:
**  - byterun1 compression (this is now default) with new options:
**    -compress, -nocompress, -cmethod, -savemem
**  - floyd-steinberg error diffusion (for std+mapfile and HAM)
**  - new options: -lace and -hires --> write CAMG chunk
**  - LUT for luminance calculation (used by ppm_to_ham)
**  23/Oct/94:
**  - rework of mapfile handling
**  - added RGB8 & RGBN image types
**  - added maskplane and transparent color capability
**  - 24-bit & direct color modified to n-bit deep ILBM
**  - removed "-savemem" option
**  22/Feb/95:
**  - minor bugfixes
**  - fixed "-camg 0" behaviour: now writes a CAMG chunk with value 0
**  - "-24if" is now default
**  - "-mmethod" and "-cmethod" options accept numeric args and keywords
**  - direct color (DCOL) reimplemented
**  - mapfile useable for HAM
**  - added HAM colormap "fixed"
**  29/Mar/95:
**  - added HAM colormap "rgb4" and "rgb5" (compute with 4/5-bit table)
**  - added IFF text chunks
**
**  Feb 2010: afu
**  Added dimension check to prevent short int from overflowing.
**  
**
**  TODO:
**  - multipalette capability (PCHG chunk) for std and HAM
**
**
**           std   HAM  deep  cmap  RGB8  RGBN
**  -------+-----+-----+-----+-----+-----+-----
**  BMHD     yes   yes   yes   yes   yes   yes
**  CMAP     yes   (1)   no    yes   no    no
**  BODY     yes   yes   yes   no    yes   yes
**  CAMG     (2)   yes   (2)   no    yes   yes
**  nPlanes  1-16  3-16  3-48  0     25    13
**
**  (1): grayscale colormap
**  (2): only if "-lace", "-hires" or "-camg" option used
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <string.h>

#include "pm_c_util.h"
#include "mallocvar.h"
#include "ppm.h"
#include "ppmfloyd.h"
#include "pbm.h"
#include "ilbm.h"
#include "lum.h"

/*#define DEBUG*/

#define MODE_RGB8       6   /* RGB8: 8-bit RGB */
#define MODE_RGBN       5   /* RGBN: 4-bit RGB */
#define MODE_CMAP       4   /* ILBM: colormap only */
#define MODE_DCOL       3   /* ILBM: direct color */
#define MODE_DEEP       2   /* ILBM: deep (24-bit) */
#define MODE_HAM        1   /* ILBM: hold-and-modify (HAM) */
#define MODE_NONE       0   /* ILBM: colormapped */

#define HAMMODE_GRAY    0   /* HAM colormap: grayscale */
#define HAMMODE_FIXED   1   /* HAM colormap: 7 "rays" in RGB cube */
#define HAMMODE_MAPFILE 2   /* HAM colormap: loaded from mapfile */
#define HAMMODE_RGB4    3   /* HAM colormap: compute, 4bit RGB */
#define HAMMODE_RGB5    4   /* HAM colormap: compute, 5bit RGB */

#define ECS_MAXPLANES   5
#define ECS_HAMPLANES   6
#define AGA_MAXPLANES   8
#define AGA_HAMPLANES   8

#define HAMMAXPLANES    10  /* maximum planes for HAM */

#define DEF_MAXPLANES   ECS_MAXPLANES
#define DEF_HAMPLANES   ECS_HAMPLANES
#define DEF_COMPRESSION cmpByteRun1
#define DEF_DEEPPLANES  8
#define DEF_DCOLPLANES  5
#define DEF_IFMODE      MODE_DEEP

#define INT16MAX 32767

static void put_big_short ARGS((short s));
static void put_big_long ARGS((long l));
#define put_byte(b)     (void)(putc((unsigned char)(b), stdout))
static void write_bytes ARGS((unsigned char *buffer, int bytes));
static void ppm_to_ham ARGS((FILE *fp, int cols, int rows, int maxval, pixel *colormap, int colors, int cmapmaxval, int hamplanes));
static void ppm_to_deep ARGS((FILE *fp, int cols, int rows, int maxval, int bitspercolor));
static void ppm_to_dcol ARGS((FILE *fp, int cols, int rows, int maxval, DirectColor *dcol));
static void ppm_to_rgb8 ARGS((FILE *fp, int cols, int rows, int maxval));
static void ppm_to_rgbn ARGS((FILE *fp, int cols, int rows, int maxval));
static void ppm_to_std ARGS((FILE *fp, int cols, int rows, int maxval, pixel *colormap, int colors, int cmapmaxval, int maxcolors, int nPlanes));
static void ppm_to_cmap ARGS((pixel *colormap, int colors, int maxval));
static void write_bmhd ARGS((int cols, int rows, int nPlanes));
static void write_cmap ARGS((pixel *colormap, int colors, int maxval));
static long encode_row ARGS((FILE *outfile, rawtype *rawrow, int cols, int nPlanes));
static long encode_maskrow ARGS((FILE *outfile, rawtype *rawrow, int cols));
static int compress_row ARGS((int bytes));
static void store_bodyrow ARGS((unsigned char *row, int len));
static int runbyte1 ARGS((int bytes));
static pixel * next_pixrow ARGS((FILE *fp, int row));
static int * make_val_table ARGS((int oldmaxval, int newmaxval));
static void init_read ARGS((FILE *fp, int *colsP, int *rowsP, pixval *maxvalP, int *formatP, int readall));
static void write_body_rows ARGS((void));
static void write_camg ARGS((void));
static int  length_of_text_chunks ARGS((void));
static void write_text_chunks ARGS((void));
#define PAD(n)      (ODD(n) ? 1 : 0)    /* pad to a word */


/* global data */
static unsigned char *coded_rowbuf; /* buffer for uncompressed scanline */
static unsigned char *compr_rowbuf; /* buffer for compressed scanline */
static pixel **pixels;  /* PPM image (NULL for row-by-row operation) */
static pixel *pixrow;   
    /* current row in PPM image (pointer into pixels array, or buffer
       for row-by-row operation) 
    */

static long viewportmodes = 0;
static int slicesize = 1; 
    /* rows per slice for multipalette images - NOT USED */

static unsigned char compmethod = DEF_COMPRESSION;   /* default compression */
static unsigned char maskmethod = mskNone;

static pixel *transpColor = NULL;   /* transparent color */
static short  transpIndex = -1;     /* index of transparent color */

static short hammapmode = HAMMODE_GRAY;
static short sortcmap = 0;     /* sort colormap */

static FILE *maskfile = NULL;
static bit *maskrow = NULL;
static int maskcols, maskformat;
#define TOTALPLANES(nplanes) ((nplanes) + ((maskmethod == mskHasMask) ? 1 : 0))


#define ROWS_PER_BLOCK  1024
typedef struct bodyblock {
    int used;
    unsigned char *row[ROWS_PER_BLOCK];
    int            len[ROWS_PER_BLOCK];
    struct bodyblock *next;
} bodyblock;
static bodyblock firstblock = { 0 };
static bodyblock *cur_block = &firstblock;

static char *anno_chunk, *auth_chunk, *name_chunk, *text_chunk, *copyr_chunk;

/* flags */
static short compr_force = 0;   
    /* force compressed output, even if the image got larger  - NOT USED */
static short floyd = 0;         /* apply floyd-steinberg error diffusion */
static short gen_camg = 0;      /* write CAMG chunk */

#define WORSTCOMPR(bytes)       ((bytes) + (bytes)/128 + 1)
#define DO_COMPRESS             (compmethod != cmpNone)


/***** parse options and figure out what kind of ILBM to write *****/

static int get_int_val ARGS((char *string, char *option, int bot, int top));
static int get_compr_method ARGS((char *string));
static int get_mask_type ARGS((char *string));
static int get_hammap_mode ARGS((char *string));



#define NEWDEPTH(pix, table) PPM_ASSIGN((pix), (table)[PPM_GETR(pix)], (table)[PPM_GETG(pix)], (table)[PPM_GETB(pix)])


static void
report_too_many_colors(int         const ifmode,
                       int         const maxplanes,
                       int         const hamplanes,
                       DirectColor const dcol,
                       int         const deepbits) {
    
    int const maxcolors = 1 << maxplanes;

    switch( ifmode ) {
    case MODE_HAM:
        pm_message("too many colors for %d planes - "
                   "proceeding to write a HAM%d file", 
                   maxplanes, hamplanes);
        pm_message("if you want a non-HAM file, try doing a 'pnmquant %d'", 
                   maxcolors);
        break;
    case MODE_DCOL:
        pm_message("too many colors for %d planes - "
                   "proceeding to write a %d:%d:%d direct color ILBM", 
                   maxplanes, dcol.r, dcol.g, dcol.b);
        pm_message("if you want a non-direct color file, "
                   "try doing a 'pnmquant %d'", maxcolors);
        break;
    case MODE_DEEP:
        pm_message("too many colors for %d planes - "
                   "proceeding to write a %d-bit \'deep\' ILBM", 
                   maxplanes, deepbits*3);
        pm_message("if you want a non-deep file, "
                   "try doing a 'pnmquant %d'", 
                   maxcolors);
        break;
    default:
        pm_error("too many colors for %d planes - "
                 "try doing a 'pnmquant %d'", 
                 maxplanes, maxcolors);
        break;
    }
}


static int
get_int_val(string, option, bot, top)
    char *string, *option;
    int bot, top;
{
    int val;

    if( sscanf(string, "%d", &val) != 1 )
        pm_error("option \"%s\" needs integer argument", option);

    if( val < bot || val > top )
        pm_error("option \"%s\" argument value out of range (%d..%d)", 
                 option, bot, top);

    return val;
}


static int
get_compr_method(string)
    char *string;
{
    int retval;
    if( pm_keymatch(string, "none", 1) || pm_keymatch(string, "0", 1) )
        retval = cmpNone;
    else if( pm_keymatch(string, "byterun1", 1) || 
             pm_keymatch(string, "1", 1) )
        retval = cmpByteRun1;
    else 
        pm_error("unknown compression method: %s", string);
    return retval;
}


static int
get_mask_type(string)
    char *string;
{
    int retval;

    if( pm_keymatch(string, "none", 1) || pm_keymatch(string, "0", 1) )
        retval = mskNone;
    else
    if( pm_keymatch(string, "plane", 1) || 
        pm_keymatch(string, "maskplane", 1) ||
        pm_keymatch(string, "1", 1) )
        retval = mskHasMask;
    else
    if( pm_keymatch(string, "transparentcolor", 1) || 
        pm_keymatch(string, "2", 1) )
        retval = mskHasTransparentColor;
    else
    if( pm_keymatch(string, "lasso", 1) || pm_keymatch(string, "3", 1) )
        retval = mskLasso;
    else
        pm_error("unknown masking method: %s", string);
    return retval;
}


static int
get_hammap_mode(string)
    char *string;
{
    int retval;

    if( pm_keymatch(string, "grey", 1) || pm_keymatch(string, "gray", 1) )
        retval =  HAMMODE_GRAY;
    else
    if( pm_keymatch(string, "fixed", 1) )
        retval =  HAMMODE_FIXED;
    else
    if( pm_keymatch(string, "rgb4", 4) )
        retval = HAMMODE_RGB4;
    else
    if( pm_keymatch(string, "rgb5", 4) )
        retval = HAMMODE_RGB5;
    else 
        pm_error("unknown HAM colormap selection mode: %s", string);
    return retval;
}


/************ colormap file ************/

static void
ppm_to_cmap(colorrow, colors, maxval)
    pixel *colorrow;
    int colors;
    int maxval;
{
    int formsize, cmapsize;

    cmapsize = colors * 3;

    formsize =
        4 +                                 /* ILBM */
        4 + 4 + BitMapHeaderSize +          /* BMHD size header */
        4 + 4 + cmapsize + PAD(cmapsize) +  /* CMAP size colormap */
        length_of_text_chunks();

    put_big_long(ID_FORM);
    put_big_long(formsize);
    put_big_long(ID_ILBM);

    write_bmhd(0, 0, 0);
    write_text_chunks();
    write_cmap(colorrow, colors, maxval);
}

/************ HAM ************/

static long 
do_ham_body     ARGS((FILE *ifP, FILE *ofp, int cols, int rows, pixval maxval,
                pixval hammaxval, int nPlanes, pixel *cmap, int colors));


static int hcmp (const void *va, const void *vb);
static pixel *compute_ham_cmap ARGS((int cols, int rows, int maxval, 
                                     int maxcolors, int *colorsP, int hbits));


typedef struct {
    long count;
    pixval r, g, b;
} hentry;


static int
hcmp(const void *va, const void *vb)
{
    return(((hentry *)vb)->count - ((hentry *)va)->count);  
        /* reverse sort, highest count first */
}


static pixel *
compute_ham_cmap(cols, rows, maxval, maxcolors, colorsP, hbits)
    int cols, rows, maxval, maxcolors;
    int *colorsP;
    int hbits;
{
    int colors;
    hentry *hmap;
    pixel *cmap;
    pixval hmaxval;
    int i, r, g, b, col, row, *htable;
    unsigned long dist, maxdist;

    pm_message("initializing HAM colormap...");

    colors = 1<<(3*hbits);
    MALLOCARRAY(hmap, colors);
    if (hmap == NULL)
        pm_error("Unable to allocate memory for HAM colormap.");
    hmaxval = pm_bitstomaxval(hbits);

    i = 0;
    for( r = 0; r <= hmaxval; r++ ) {
        for( g = 0; g <= hmaxval; g++ ) {
            for( b = 0; b <= hmaxval; b++ ) {
                hmap[i].r = r; hmap[i].g = g; hmap[i].b = b;
                hmap[i].count = 0;
                i++;
            }
        }
    }

    htable = make_val_table(maxval, hmaxval);
    for( row = 0; row < rows; row++ ) {
        unsigned int col;
        for( col = 0; col < cols; ++col) {
            pixel const p = pixels[row][col];
            pixval const r = PPM_GETR(p);
            pixval const g = PPM_GETG(p);
            pixval const b = PPM_GETB(p);
            i = (htable[r]<<(2*hbits)) + (htable[g]<<hbits) + htable[b];
            hmap[i].count++;
        }
    }
    free(htable);

    qsort((void *)hmap, colors, sizeof(hentry), hcmp);
    for( i = colors-1; i >= 0; i-- ) {
        if( hmap[i].count )
            break;
    }
    colors = i+1;

    if( colors > maxcolors ) {
        pm_message("selecting HAM colormap from %d colors...", colors);
        for( maxdist = 1; ; maxdist++ ) {
            for( col = colors-1; col > 0; col-- ) {
                r = hmap[col].r; g = hmap[col].g; b = hmap[col].b;
                for( i = 0; i < col; i++ ) {
                    register int tmp;

                    tmp = hmap[i].r - r; dist = tmp * tmp;
                    tmp = hmap[i].g - g; dist += tmp * tmp;
                    tmp = hmap[i].b - b; dist += tmp * tmp;

                    if( dist <= maxdist ) {
                        int sum = hmap[i].count + hmap[col].count;

                        hmap[i].r = (hmap[i].r * hmap[i].count + 
                                     r * hmap[col].count + sum/2)/sum;
                        hmap[i].g = (hmap[i].g * hmap[i].count + 
                                     g * hmap[col].count + sum/2)/sum;
                        hmap[i].b = (hmap[i].b * hmap[i].count + 
                                     b * hmap[col].count + sum/2)/sum;
                        hmap[i].count = sum;

                        hmap[col] = hmap[i];    /* temp store */
                        for( tmp = i-1; 
                             tmp >= 0 && hmap[tmp].count < hmap[col].count; 
                             tmp-- )
                            hmap[tmp+1] = hmap[tmp];
                        hmap[tmp+1] = hmap[col];

                        for( tmp = col; tmp < colors-1; tmp++ )
                            hmap[tmp] = hmap[tmp+1];
                        if( --colors <= maxcolors )
                            goto out;
                        break;
                    }
                }
            }
#ifdef DEBUG
            pm_message("\tmaxdist=%ld: %d colors left", maxdist, colors);
#endif
        }
    }
out:
    pm_message("%d colors in HAM colormap", colors);

    cmap = ppm_allocrow(colors);
    *colorsP = colors;

    for( i = 0; i < colors; i++ ) {
        r = hmap[i].r; g = hmap[i].g; b = hmap[i].b;
        PPM_ASSIGN(cmap[i], r, g, b);
    }

    ppm_freerow(hmap);
    return cmap;
}


static void
ppm_to_ham(fp, cols, rows, maxval, colormap, colors, cmapmaxval, hamplanes)
    FILE *fp;
    int cols, rows, maxval;
    pixel *colormap;
    int colors, cmapmaxval, hamplanes;
{
    int hamcolors, nPlanes, i, hammaxval;
    long oldsize, bodysize, formsize, cmapsize;
    int *table = NULL;

    if( maskmethod == mskHasTransparentColor ) {
        pm_message("masking method '%s' not usable with HAM - "
                   "using '%s' instead",
                   mskNAME[mskHasTransparentColor], mskNAME[mskHasMask]);
        maskmethod = mskHasMask;
    }

    hamcolors = 1 << (hamplanes-2);
    hammaxval = pm_bitstomaxval(hamplanes-2);

    if( colors == 0 ) {
        /* no colormap, make our own */
        switch( hammapmode ) {
            case HAMMODE_GRAY:
                colors = hamcolors;
                MALLOCARRAY_NOFAIL(colormap, colors);
#ifdef DEBUG
                pm_message("generating grayscale colormap");
#endif
                table = make_val_table(hammaxval, MAXCOLVAL);
                for( i = 0; i < colors; i++ )
                    PPM_ASSIGN(colormap[i], table[i], table[i], table[i]);
                free(table);
                cmapmaxval = MAXCOLVAL;
                break;
            case HAMMODE_FIXED: {
                int entries, val;
                double step;

#ifdef DEBUG
                pm_message("generating rgb colormap");
#endif
                /* generate a colormap of 7 "rays" in an RGB color cube:
                        r, g, b, r+g, r+b, g+b, r+g+b
                   we need one colormap entry for black, so the number of
                   entries per ray is (maxcolors-1)/7 */

                entries = (hamcolors-1)/7;
                colors = 7*entries+1;
                MALLOCARRAY_NOFAIL(colormap, colors);
                step = (double)MAXCOLVAL / (double)entries;

                PPM_ASSIGN(colormap[0], 0, 0, 0);
                for( i = 1; i <= entries; i++ ) {
                    val = (int)((double)i * step);
                    PPM_ASSIGN(colormap[          i], val,   0,   0); /* r */
                    PPM_ASSIGN(colormap[  entries+i],   0, val,   0); /* g */
                    PPM_ASSIGN(colormap[2*entries+i],   0,   0, val); /* b */
                    PPM_ASSIGN(colormap[3*entries+i], val, val,   0); /* r+g */
                    PPM_ASSIGN(colormap[4*entries+i], val,   0, val); /* r+b */
                    PPM_ASSIGN(colormap[5*entries+i],   0, val, val); /* g+b */
                    PPM_ASSIGN(colormap[6*entries+i], val, val, val); /*r+g+b*/
                }
                cmapmaxval = MAXCOLVAL;
            }
            break;
            case HAMMODE_RGB4:
                colormap = compute_ham_cmap(cols, rows, maxval, hamcolors, 
                                            &colors, 4);
                cmapmaxval = 15;
                break;
            case HAMMODE_RGB5:
                colormap = compute_ham_cmap(cols, rows, maxval, 
                                            hamcolors, &colors, 5);
                cmapmaxval = 31;
                break;
            default:
                pm_error("ppm_to_ham(): unknown hammapmode - can't happen");
        }
    }
    else {
        hammapmode = HAMMODE_MAPFILE;
        if( colors > hamcolors ) {
            pm_message("colormap too large - using first %d colors", 
                       hamcolors);
            colors = hamcolors;
        }
    }

    if( cmapmaxval != maxval ) {
        int i, *table;
        pixel *newcmap;

        newcmap = ppm_allocrow(colors);
        table = make_val_table(cmapmaxval, maxval);
        for( i = 0; i < colors; i++ )
            PPM_ASSIGN(newcmap[i], 
                       table[PPM_GETR(colormap[i])], 
                       table[PPM_GETG(colormap[i])], 
                       table[PPM_GETB(colormap[i])]);
        free(table);
        ppm_freerow(colormap);
        colormap = newcmap;
    }
    if( sortcmap )
        ppm_sortcolorrow(colormap, colors, PPM_STDSORT);

    nPlanes = hamplanes;
    cmapsize = colors * 3;

    bodysize = oldsize = rows * TOTALPLANES(nPlanes) * RowBytes(cols);
    if( DO_COMPRESS ) {
        bodysize = do_ham_body(fp, NULL, cols, rows, maxval, 
                               hammaxval, nPlanes, colormap, colors);
        /*bodysize = do_ham_body(fp, NULL, cols, 
          rows, maxval, hammaxval, nPlanes, colbits, nocolor);*/
        if( bodysize > oldsize )
            pm_message("warning - %s compression increases BODY size "
                       "by %ld%%", 
                       cmpNAME[compmethod], 100*(bodysize-oldsize)/oldsize);
        else
            pm_message("BODY compression (%s): %ld%%", 
                       cmpNAME[compmethod], 100*(oldsize-bodysize)/oldsize);
    }


    formsize =
        4 +                                 /* ILBM */
        4 + 4 + BitMapHeaderSize +          /* BMHD size header */
        4 + 4 + CAMGChunkSize +             /* CAMG size viewportmodes */
        4 + 4 + cmapsize + PAD(cmapsize) +  /* CMAP size colormap */
        4 + 4 + bodysize + PAD(bodysize) +  /* BODY size data */
        length_of_text_chunks();

    put_big_long(ID_FORM);
    put_big_long(formsize);
    put_big_long(ID_ILBM);

    write_bmhd(cols, rows, nPlanes);
    write_text_chunks();
    write_camg();       /* HAM requires CAMG chunk */
    write_cmap(colormap, colors, maxval);

    /* write body */
    put_big_long(ID_BODY);
    put_big_long(bodysize);
    if( DO_COMPRESS )
        write_body_rows();
    else
        do_ham_body(fp, NULL, cols, rows, maxval, hammaxval, 
                    nPlanes, colormap, colors);
}


static long
#ifdef __STDC__
do_ham_body(FILE *ifP, FILE *ofp, int cols, int rows,
            pixval maxval, pixval hammaxval, int nPlanes,
            pixel *colormap, int colors)
#else
do_ham_body(ifP, ofp, cols, rows, maxval, hammaxval, nPlanes, colormap, colors)
    FILE *ifP, *ofp;
    int cols, rows;
    pixval maxval;      /* maxval of image color components */
    pixval hammaxval;   /* maxval of HAM color changes */
    int nPlanes;
    pixel *colormap;
    int colors;
#endif
{
    register int col, row, i;
    rawtype *raw_rowbuf;
    ppm_fs_info *fi = NULL;
    colorhash_table cht, cht2;
    long bodysize = 0;
    int *itoh;      /* table image -> ham */
    int usehash = 1;
    int colbits;
    int hamcode_red, hamcode_green, hamcode_blue;

    MALLOCARRAY_NOFAIL(raw_rowbuf, cols);

    cht = ppm_colorrowtocolorhash(colormap, colors);
    cht2 = ppm_alloccolorhash();
    colbits = pm_maxvaltobits(hammaxval);

    hamcode_red   = HAMCODE_RED << colbits;
    hamcode_green = HAMCODE_GREEN << colbits;
    hamcode_blue  = HAMCODE_BLUE << colbits;

    itoh = make_val_table(maxval, hammaxval);

    if( floyd )
        fi = ppm_fs_init(cols, maxval, 0);

    for( row = 0; row < rows; row++ ) {
        int noprev;
        int spr, spg, spb;   /* scaled values of previous pixel */
        int upr, upg, upb;   /* unscaled values of previous pixel, for floyd */
        pixel *prow;

        noprev = 1;
        prow = next_pixrow(ifP, row);
        for( col = ppm_fs_startrow(fi, prow); 
             col < cols; 
             col = ppm_fs_next(fi, col) ) {

            pixel const p = prow[col];

            /* unscaled values of current pixel */
            pixval const ur = PPM_GETR(p);
            pixval const ug = PPM_GETG(p);
            pixval const ub = PPM_GETB(p);

            /* scaled values of current pixel */
            int const sr = itoh[ur];
            int const sg = itoh[ug];
            int const sb = itoh[ub];

            i = ppm_lookupcolor(cht, &p);
            if( i == -1 ) { /* no matching color in cmap, find closest match */
                int ucr, ucg, ucb;  /* unscaled values of colormap entry */

                if(  hammapmode == HAMMODE_GRAY ) {
                    if( maxval <= 255 ) 
                        /* Use fast approximation to 
                           0.299 r + 0.587 g + 0.114 b. */
                        i = (int)ppm_fastlumin(p);
                    else 
                        /* Can't use fast approximation, 
                           so fall back on floats. 
                        */
                        i = (int)(PPM_LUMIN(p) + 0.5); 
                            /* -IUW added '+ 0.5' */
                    i = itoh[i];
                }
                else {
                    i = ppm_lookupcolor(cht2, &p);
                    if( i == -1 ) {
                        i = ppm_findclosestcolor(colormap, colors, &p);
                        if( usehash ) {
                            if( ppm_addtocolorhash(cht2, &p, i) < 0 ) {
                                pm_message("out of memory "
                                           "adding to hash table, "
                                           "proceeding without it");
                                usehash = 0;
                            }
                        }
                    }
                }
                ucr = PPM_GETR(colormap[i]); 
                ucg = PPM_GETG(colormap[i]); 
                ucb = PPM_GETB(colormap[i]);

                if( noprev ) {  /* no previous pixel, must use colormap */
                    raw_rowbuf[col] = i;    /* + (HAMCODE_CMAP << colbits) */
                    upr = ucr;          upg = ucg;          upb = ucb;
                    spr = itoh[upr];    spg = itoh[upg];    spb = itoh[upb];
                    noprev = 0;
                } else {
                    register long di, dr, dg, db;
                    int scr, scg, scb;   /* scaled values of colormap entry */

                    scr = itoh[ucr]; scg = itoh[ucg]; scb = itoh[ucb];

                    /* compute distances for the four options */
#if 1
                    dr = abs(sg - spg) + abs(sb - spb);
                    dg = abs(sr - spr) + abs(sb - spb);
                    db = abs(sr - spr) + abs(sg - spg);
                    di = abs(sr - scr) + abs(sg - scg) + abs(sb - scb);
#else
                    dr = (sg - spg)*(sg - spg) + (sb - spb)*(sb - spb);
                    dg = (sr - spr)*(sr - spr) + (sb - spb)*(sb - spb);
                    db = (sr - spr)*(sr - spr) + (sg - spg)*(sg - spg);
                    di = (sr - scr)*(sr - scr) + (sg - scg)*(sg - scg) +
                        (sb - scb)*(sb - scb);
#endif

                    if( di <= dr && di <= dg && di <= db ) {    
                        /* prefer colormap lookup */
                        raw_rowbuf[col] = i; 
                        upr = ucr;  upg = ucg;  upb = ucb;
                        spr = scr;  spg = scg;  spb = scb;
                    }
                    else
                    if( db <= dr && db <= dg ) {
                        raw_rowbuf[col] = sb + hamcode_blue; 
                        spb = sb;
                        upb = ub;
                    }
                    else
                    if( dr <= dg ) {
                        raw_rowbuf[col] = sr + hamcode_red;  
                        spr = sr;
                        upr = ur;
                    }
                    else {
                        raw_rowbuf[col] = sg + hamcode_green;
                        spg = sg;
                        upg = ug;
                    }
                }
            }
            else {  /* prefect match in cmap */
                raw_rowbuf[col] = i;    /* + (HAMCODE_CMAP << colbits) */
                upr = PPM_GETR(colormap[i]); 
                upg = PPM_GETG(colormap[i]); 
                upb = PPM_GETB(colormap[i]);
                spr = itoh[upr];            
                spg = itoh[upg];            
                spb = itoh[upb];
            }
            ppm_fs_update3(fi, col, upr, upg, upb);
        }
        bodysize += encode_row(ofp, raw_rowbuf, cols, nPlanes);
        if( maskmethod == mskHasMask )
            bodysize += encode_maskrow(ofp, raw_rowbuf, cols);
        ppm_fs_endrow(fi);
    }
    if( ofp && ODD(bodysize) )
        put_byte(0);

    free(itoh);

    /* clean up */
    free(raw_rowbuf);
    ppm_fs_free(fi);

    return bodysize;
}


/************ deep (24-bit) ************/

static long do_deep_body      ARGS((FILE *ifP, FILE *ofp, 
                                    int cols, int rows, 
                                    pixval maxval, int bitspercolor));

static void
ppm_to_deep(fp, cols, rows, maxval, bitspercolor)
    FILE *fp;
    int cols, rows, maxval, bitspercolor;
{
    int nPlanes;
    long bodysize, oldsize, formsize;

    if( maskmethod == mskHasTransparentColor ) {
        pm_message("masking method '%s' not usable with deep ILBM - "
                   "using '%s' instead",
                    mskNAME[mskHasTransparentColor], mskNAME[mskHasMask]);
        maskmethod = mskHasMask;
    }

    nPlanes = 3*bitspercolor;

    bodysize = oldsize = rows * TOTALPLANES(nPlanes) * RowBytes(cols);
    if( DO_COMPRESS ) {
        bodysize = do_deep_body(fp, NULL, cols, rows, maxval, bitspercolor);
        if( bodysize > oldsize )
            pm_message("warning - %s compression increases BODY size by %ld%%",
                       cmpNAME[compmethod], 100*(bodysize-oldsize)/oldsize);
        else
            pm_message("BODY compression (%s): %ld%%", 
                       cmpNAME[compmethod], 100*(oldsize-bodysize)/oldsize);
    }


    formsize =
        4 +                                 /* ILBM */
        4 + 4 + BitMapHeaderSize +          /* BMHD size header */
        4 + 4 + bodysize + PAD(bodysize) +  /* BODY size data */
        length_of_text_chunks();
    if( gen_camg )
        formsize += 4 + 4 + CAMGChunkSize;  /* CAMG size viewportmodes */

    put_big_long(ID_FORM);
    put_big_long(formsize);
    put_big_long(ID_ILBM);

    write_bmhd(cols, rows, nPlanes);
    write_text_chunks();
    if( gen_camg )
        write_camg();

    /* write body */
    put_big_long(ID_BODY);
    put_big_long(bodysize);
    if( DO_COMPRESS )
        write_body_rows();
    else
        do_deep_body(fp, stdout, cols, rows, maxval, bitspercolor);
}


static long
#if __STDC__
do_deep_body(FILE *ifP, FILE *ofp, int cols, int rows, pixval maxval, 
             int bitspercolor)
#else
do_deep_body(ifP, ofp, cols, rows, maxval, bitspercolor)
    FILE *ifP, *ofp;
    int cols, rows;
    pixval maxval;
    int bitspercolor;
#endif
{
    register int row, col;
    pixel *pP;
    int *table = NULL;
    long bodysize = 0;
    rawtype *redbuf, *greenbuf, *bluebuf;
    int newmaxval;

    MALLOCARRAY_NOFAIL(redbuf,   cols);
    MALLOCARRAY_NOFAIL(greenbuf, cols);
    MALLOCARRAY_NOFAIL(bluebuf,  cols);

    newmaxval = pm_bitstomaxval(bitspercolor);
    if( maxval != newmaxval ) {
        pm_message("maxval is not %d - automatically rescaling colors", 
                   newmaxval);
        table = make_val_table(maxval, newmaxval);
    }

    for( row = 0; row < rows; row++ ) {
        pP = next_pixrow(ifP, row);
        if( table ) {
            for( col = 0; col < cols; col++, pP++ ) {
                redbuf[col]     = table[PPM_GETR(*pP)];
                greenbuf[col]   = table[PPM_GETG(*pP)];
                bluebuf[col]    = table[PPM_GETB(*pP)];
            }
        }
        else {
            for( col = 0; col < cols; col++, pP++ ) {
                redbuf[col]     = PPM_GETR(*pP);
                greenbuf[col]   = PPM_GETG(*pP);
                bluebuf[col]    = PPM_GETB(*pP);
            }
        }
        bodysize += encode_row(ofp, redbuf,   cols, bitspercolor);
        bodysize += encode_row(ofp, greenbuf, cols, bitspercolor);
        bodysize += encode_row(ofp, bluebuf,  cols, bitspercolor);
        if( maskmethod == mskHasMask )
            bodysize += encode_maskrow(ofp, redbuf, cols);
    }
    if( ofp && ODD(bodysize) )
        put_byte(0);

    /* clean up */
    if( table )
        free(table);
    free(redbuf);
    free(greenbuf);
    free(bluebuf);

    return bodysize;
}


/************ direct color ************/

static long do_dcol_body      ARGS((FILE *ifP, FILE *ofp, int cols, int rows, 
                                    pixval maxval, DirectColor *dcol));

static void
ppm_to_dcol(fp, cols, rows, maxval, dcol)
    FILE *fp;
    int cols, rows, maxval;
    DirectColor *dcol;
{
    int nPlanes;
    long bodysize, oldsize, formsize;

    if( maskmethod == mskHasTransparentColor ) {
        pm_message("masking method '%s' not usable with deep ILBM - "
                   "using '%s' instead",
                   mskNAME[mskHasTransparentColor], mskNAME[mskHasMask]);
        maskmethod = mskHasMask;
    }

    nPlanes = dcol->r + dcol->g + dcol->b;

    bodysize = oldsize = rows * TOTALPLANES(nPlanes) * RowBytes(cols);
    if( DO_COMPRESS ) {
        bodysize = do_dcol_body(fp, NULL, cols, rows, maxval, dcol);
        if( bodysize > oldsize )
            pm_message("warning - %s compression increases BODY size by %ld%%",
                       cmpNAME[compmethod], 
                       100*(bodysize-oldsize)/oldsize);
        else
            pm_message("BODY compression (%s): %ld%%", cmpNAME[compmethod], 
                       100*(oldsize-bodysize)/oldsize);
    }


    formsize =
        4 +                                 /* ILBM */
        4 + 4 + BitMapHeaderSize +          /* BMHD size header */
        4 + 4 + DirectColorSize +           /* DCOL size dcol */
        4 + 4 + bodysize + PAD(bodysize) +  /* BODY size data */
        length_of_text_chunks();
    if( gen_camg )
        formsize += 4 + 4 + CAMGChunkSize;  /* CAMG size viewportmodes */

    put_big_long(ID_FORM);
    put_big_long(formsize);
    put_big_long(ID_ILBM);

    write_bmhd(cols, rows, nPlanes);
    write_text_chunks();

    put_big_long(ID_DCOL);
    put_big_long(DirectColorSize);
    put_byte(dcol->r);
    put_byte(dcol->g);
    put_byte(dcol->b);
    put_byte(0);    /* pad */

    if( gen_camg )
        write_camg();

    /* write body */
    put_big_long(ID_BODY);
    put_big_long(bodysize);
    if( DO_COMPRESS )
        write_body_rows();
    else
        do_dcol_body(fp, stdout, cols, rows, maxval, dcol);
}


static long
#if __STDC__
do_dcol_body(FILE *ifP, FILE *ofp, int cols, int rows, pixval maxval, 
             DirectColor *dcol)
#else
do_dcol_body(ifP, ofp, cols, rows, maxval, dcol)
    FILE *ifP, *ofp;
    int cols, rows;
    pixval maxval;
    DirectColor *dcol;
#endif
{
    register int row, col;
    pixel *pP;
    long bodysize = 0;
    rawtype *redbuf, *greenbuf, *bluebuf;
    int *redtable, *greentable, *bluetable;

    MALLOCARRAY_NOFAIL(redbuf,   cols);
    MALLOCARRAY_NOFAIL(greenbuf, cols);
    MALLOCARRAY_NOFAIL(bluebuf,  cols);

    redtable   = make_val_table(maxval, pm_bitstomaxval(dcol->r));
    greentable = make_val_table(maxval, pm_bitstomaxval(dcol->g));
    bluetable  = make_val_table(maxval, pm_bitstomaxval(dcol->b));

    for( row = 0; row < rows; row++ ) {
        pP = next_pixrow(ifP, row);
        for( col = 0; col < cols; col++, pP++ ) {
            redbuf[col]   = redtable[PPM_GETR(*pP)];
            greenbuf[col] = greentable[PPM_GETG(*pP)];
            bluebuf[col]  = bluetable[PPM_GETB(*pP)];
        }
        bodysize += encode_row(ofp, redbuf,   cols, dcol->r);
        bodysize += encode_row(ofp, greenbuf, cols, dcol->g);
        bodysize += encode_row(ofp, bluebuf,  cols, dcol->b);
        if( maskmethod == mskHasMask )
            bodysize += encode_maskrow(ofp, redbuf, cols);
    }
    if( ofp && ODD(bodysize) )
        put_byte(0);

    /* clean up */
    free(redtable);
    free(greentable);
    free(bluetable);
    free(redbuf);
    free(greenbuf);
    free(bluebuf);

    return bodysize;
}


/************ normal colormapped ************/

static long do_std_body     ARGS((FILE *ifP, FILE *ofp, int cols, int rows, 
                                  pixval maxval, pixel *colormap, 
                                  int colors, int nPlanes));

static void
ppm_to_std(fp, cols, rows, maxval, colormap, colors, cmapmaxval, 
           maxcolors, nPlanes)
    FILE *fp;
    int cols, rows, maxval;
    pixel *colormap;
    int cmapmaxval, colors, maxcolors, nPlanes;
{
    long formsize, cmapsize, bodysize, oldsize;

    if( maskmethod == mskHasTransparentColor ) {
        if( transpColor ) {
            transpIndex = 
                ppm_addtocolorrow(colormap, &colors, maxcolors, transpColor);
        }
        else
        if( colors < maxcolors )
            transpIndex = colors;

        if( transpIndex < 0 ) {
            pm_message("too many colors for masking method '%s' - "
                       "using '%s' instead",
                       mskNAME[mskHasTransparentColor], mskNAME[mskHasMask]);
            maskmethod = mskHasMask;
        }
    }

    if( cmapmaxval != maxval ) {
        int i, *table;
        pixel *newcmap;

        newcmap = ppm_allocrow(colors);
        table = make_val_table(cmapmaxval, maxval);
        for (i = 0; i < colors; ++i)
            PPM_ASSIGN(newcmap[i], 
                       table[PPM_GETR(colormap[i])], 
                       table[PPM_GETG(colormap[i])], 
                       table[PPM_GETB(colormap[i])]);
        free(table);
        colormap = newcmap;
    }
    if( sortcmap )
        ppm_sortcolorrow(colormap, colors, PPM_STDSORT);

    bodysize = oldsize = rows * TOTALPLANES(nPlanes) * RowBytes(cols);
    if( DO_COMPRESS ) {
        bodysize = do_std_body(fp, NULL, cols, rows, maxval, colormap, 
                               colors, nPlanes);
        if( bodysize > oldsize )
            pm_message("warning - %s compression increases BODY size by %ld%%",
                       cmpNAME[compmethod], 100*(bodysize-oldsize)/oldsize);
        else
            pm_message("BODY compression (%s): %ld%%", 
                       cmpNAME[compmethod], 100*(oldsize-bodysize)/oldsize);
    }

    cmapsize = colors * 3;

    formsize =
        4 +                                 /* ILBM */
        4 + 4 + BitMapHeaderSize +          /* BMHD size header */
        4 + 4 + cmapsize + PAD(cmapsize) +  /* CMAP size colormap */
        4 + 4 + bodysize + PAD(bodysize) +  /* BODY size data */
        length_of_text_chunks();
    if( gen_camg )
        formsize += 4 + 4 + CAMGChunkSize;  /* CAMG size viewportmodes */

    put_big_long(ID_FORM);
    put_big_long(formsize);
    put_big_long(ID_ILBM);

    write_bmhd(cols, rows, nPlanes);
    write_text_chunks();
    if( gen_camg )
        write_camg();
    write_cmap(colormap, colors, maxval);

    /* write body */
    put_big_long(ID_BODY);
    put_big_long(bodysize);
    if( DO_COMPRESS )
        write_body_rows();
    else
        do_std_body(fp, stdout, cols, rows, maxval, colormap, colors, nPlanes);
}


static long
#if __STDC__
do_std_body(FILE *ifP, FILE *ofp, int cols, int rows, pixval maxval,
            pixel *colormap, int colors, int nPlanes)
#else
do_std_body(ifP, ofp, cols, rows, maxval, colormap, colors, nPlanes)
    FILE *ifP, *ofp;
    int cols, rows;
    pixval maxval;
    pixel *colormap;
    int colors;
    int nPlanes;
#endif
{
    register int row, col, i;
    pixel *pP;
    rawtype *raw_rowbuf;
    ppm_fs_info *fi = NULL;
    long bodysize = 0;
    int usehash = 1;
    colorhash_table cht;

    MALLOCARRAY_NOFAIL(raw_rowbuf, cols);
    cht = ppm_colorrowtocolorhash(colormap, colors);
    if( floyd )
        fi = ppm_fs_init(cols, maxval, FS_ALTERNATE);

    for( row = 0; row < rows; row++ ) {
        pixel *prow;
        prow = next_pixrow(ifP, row);

        for( col = ppm_fs_startrow(fi, prow); 
             col < cols; 
             col = ppm_fs_next(fi, col) ) {
            pP = &prow[col];

            if( maskmethod == mskHasTransparentColor && 
                maskrow[col] == PBM_WHITE )
                i = transpIndex;
            else {
                /* Check hash table to see if we have already matched
                   this color. 
                */
                i = ppm_lookupcolor(cht, pP);
                if( i == -1 ) {
                    i = ppm_findclosestcolor(colormap, colors, pP);    
                        /* No; search colormap for closest match. */
                    if( usehash ) {
                        if( ppm_addtocolorhash(cht, pP, i) < 0 ) {
                            pm_message("out of memory adding to hash table, "
                                       "proceeding without it");
                            usehash = 0;
                        }
                    }
                }
            }
            raw_rowbuf[col] = i;
            ppm_fs_update(fi, col, &colormap[i]);
        }
        bodysize += encode_row(ofp, raw_rowbuf, cols, nPlanes);
        if( maskmethod == mskHasMask )
            bodysize += encode_maskrow(ofp, raw_rowbuf, cols);
        ppm_fs_endrow(fi);
    }
    if( ofp && ODD(bodysize) )
        put_byte(0);

    /* clean up */
    ppm_freecolorhash(cht);
    free(raw_rowbuf);
    ppm_fs_free(fi);

    return bodysize;
}

/************ RGB8 ************/

static void
ppm_to_rgb8(ifP, cols, rows, maxval)
    FILE *ifP;
    int cols, rows;
    int maxval;
{
    long bodysize, oldsize, formsize;
    pixel *pP;
    int *table = NULL;
    int row, col1, col2, compr_len, len;
    unsigned char *compr_row;

    maskmethod = 0;     /* no masking - RGB8 uses genlock bits */
    compmethod = 4;     /* RGB8 files are always compressed */
    MALLOCARRAY_NOFAIL(compr_row, cols * 4);

    if( maxval != 255 ) {
        pm_message("maxval is not 255 - automatically rescaling colors");
        table = make_val_table(maxval, 255);
    }

    oldsize = cols * rows * 4;
    bodysize = 0;
    for( row = 0; row < rows; row++ ) {
        pP = next_pixrow(ifP, row);
        compr_len = 0;
        for( col1 = 0; col1 < cols; col1 = col2 ) {
            col2 = col1 + 1;
            if( maskrow ) {
                while( col2 < cols && PPM_EQUAL(pP[col1], pP[col2]) && 
                       maskrow[col1] == maskrow[col2] )
                    col2++;
            }
            else {
                while( col2 < cols && PPM_EQUAL(pP[col1], pP[col2]) )
                    col2++;
            }
            len = col2 - col1;
            while( len ) {
                int count;
                count = (len > 127 ? 127 : len);
                len -= count;
                if( table ) {
                    compr_row[compr_len++] = table[PPM_GETR(pP[col1])];
                    compr_row[compr_len++] = table[PPM_GETG(pP[col1])];
                    compr_row[compr_len++] = table[PPM_GETB(pP[col1])];
                }
                else {
                    compr_row[compr_len++] = PPM_GETR(pP[col1]);
                    compr_row[compr_len++] = PPM_GETG(pP[col1]);
                    compr_row[compr_len++] = PPM_GETB(pP[col1]);
                }
                compr_row[compr_len] = count;
                if( maskrow && maskrow[col1] == PBM_WHITE )
                    compr_row[compr_len] |= 1<<7;     /* genlock bit */
                ++compr_len;
            }
        }
        store_bodyrow(compr_row, compr_len);
        bodysize += compr_len;
    }

    pm_message("BODY compression: %ld%%", 100*(oldsize-bodysize)/oldsize);

    formsize =
        4 +                                 /* RGB8 */
        4 + 4 + BitMapHeaderSize +          /* BMHD size header */
        4 + 4 + CAMGChunkSize +             /* CAMG size viewportmode */
        4 + 4 + bodysize + PAD(bodysize) +  /* BODY size data */
        length_of_text_chunks();

    /* write header */
    put_big_long(ID_FORM);
    put_big_long(formsize);
    put_big_long(ID_RGB8);

    write_bmhd(cols, rows, 25);
    write_text_chunks();
    write_camg();               /* RGB8 requires CAMG chunk */

    put_big_long(ID_BODY);
    put_big_long(bodysize);
    write_body_rows();
}


/************ RGBN ************/

static void
ppm_to_rgbn(ifP, cols, rows, maxval)
    FILE *ifP;
    int cols, rows;
    int maxval;
{
    long bodysize, oldsize, formsize;
    pixel *pP;
    int *table = NULL;
    int row, col1, col2, compr_len, len;
    unsigned char *compr_row;

    maskmethod = 0;     /* no masking - RGBN uses genlock bits */
    compmethod = 4;     /* RGBN files are always compressed */
    MALLOCARRAY_NOFAIL(compr_row, cols * 2);

    if( maxval != 15 ) {
        pm_message("maxval is not 15 - automatically rescaling colors");
        table = make_val_table(maxval, 15);
    }

    oldsize = cols * rows * 2;
    bodysize = 0;
    for( row = 0; row < rows; row++ ) {
        pP = next_pixrow(ifP, row);
        compr_len = 0;
        for( col1 = 0; col1 < cols; col1 = col2 ) {
            col2 = col1 + 1;
            if( maskrow ) {
                while( col2 < cols && PPM_EQUAL(pP[col1], pP[col2]) && 
                       maskrow[col1] == maskrow[col2] )
                    col2++;
            }
            else {
                while( col2 < cols && PPM_EQUAL(pP[col1], pP[col2]) )
                    col2++;
            }
            len = col2 - col1;
            while( len ) {
                int count;
                count = (len > 65535 ? 65535 : len);
                len -= count;
                if( table ) {
                    compr_row[compr_len]  = table[PPM_GETR(pP[col1])] << 4;
                    compr_row[compr_len] |= table[PPM_GETG(pP[col1])];
                    ++compr_len;
                    compr_row[compr_len]  = table[PPM_GETB(pP[col1])] << 4;
                }
                else {
                    compr_row[compr_len]  = PPM_GETR(pP[col1]) << 4;
                    compr_row[compr_len] |= PPM_GETG(pP[col1]);
                    ++compr_len;
                    compr_row[compr_len]  = PPM_GETB(pP[col1]) << 4;
                }
                if( maskrow && maskrow[col1] == PBM_WHITE )
                    compr_row[compr_len] |= 1<<3;   /* genlock bit */
                if( count <= 7 )
                    compr_row[compr_len++] |= count;  /* 3 bit repeat count */
                else {
                    ++compr_len;                  /* 3 bit repeat count = 0 */
                    if( count <= 255 )
                        compr_row[compr_len++] = (unsigned char)count;  
                            /* byte repeat count */
                    else {
                        compr_row[compr_len++] = (unsigned char)0;   
                            /* byte repeat count = 0 */
                        compr_row[compr_len++] = (count >> 8) & 0xff; 
                            /* word repeat count MSB */
                        compr_row[compr_len++] = count & 0xff;    
                            /* word repeat count LSB */
                    }
                }
            }
        }
        store_bodyrow(compr_row, compr_len);
        bodysize += compr_len;
    }

    pm_message("BODY compression: %ld%%", 100*(oldsize-bodysize)/oldsize);

    formsize =
        4 +                                 /* RGBN */
        4 + 4 + BitMapHeaderSize +          /* BMHD size header */
        4 + 4 + CAMGChunkSize +             /* CAMG size viewportmode */
        4 + 4 + bodysize + PAD(bodysize) +  /* BODY size data */
        length_of_text_chunks();

    /* write header */
    put_big_long(ID_FORM);
    put_big_long(formsize);
    put_big_long(ID_RGBN);

    write_bmhd(cols, rows, 13);
    write_text_chunks();
    write_camg();               /* RGBN requires CAMG chunk */

    put_big_long(ID_BODY);
    put_big_long(bodysize);
    write_body_rows();
}


/************ multipalette ************/

#ifdef ILBM_PCHG
static pixel *ppmslice[2];  /* need 2 for laced ILBMs, else 1 */

void ppm_to_pchg()
{
/*
    read first slice
    build a colormap from this slice
    select upto <maxcolors> colors
    build colormap from selected colors
    map slice to colormap
    write slice
    while( !finished ) {
        read next slice
        compute distances for each pixel and select upto
            <maxchangesperslice> unused colors in this slice
        modify selected colors to the ones with maximum(?) distance
        map slice to colormap
        write slice
    }


    for HAM use a different mapping:
        compute distance to closest color in colormap
        if( there is no matching color in colormap ) {
            compute distances for the three "modify" cases
            use the shortest distance from the four cases
        }
*/
}
#endif


/************ ILBM functions ************/

static int
length_of_text_chunks ARGS((void))
{
    int len, n;

    len = 0;
    if( anno_chunk ) {
        n = strlen(anno_chunk);
        len += 4 + 4 + n + PAD(n);      /* ID chunksize text */
    }
    if( auth_chunk ) {
        n = strlen(auth_chunk);
        len += 4 + 4 + n + PAD(n);      /* ID chunksize text */
    }
    if( name_chunk ) {
        n = strlen(name_chunk);
        len += 4 + 4 + n + PAD(n);      /* ID chunksize text */
    }
    if( copyr_chunk ) {
        n = strlen(copyr_chunk);
        len += 4 + 4 + n + PAD(n);      /* ID chunksize text */
    }
    if( text_chunk ) {
        n = strlen(text_chunk);
        len += 4 + 4 + n + PAD(n);      /* ID chunksize text */
    }
    return len;
}


static void
write_text_chunks ARGS((void))
{
    int n;

    if( anno_chunk ) {
        n = strlen(anno_chunk);
        put_big_long(ID_ANNO);
        put_big_long(n);
        write_bytes((unsigned char *)anno_chunk, n);
        if( ODD(n) )
            put_byte(0);
    }
    if( auth_chunk ) {
        n = strlen(auth_chunk);
        put_big_long(ID_AUTH);
        put_big_long(n);
        write_bytes((unsigned char *)auth_chunk, n);
        if( ODD(n) )
            put_byte(0);
    }
    if( copyr_chunk ) {
        n = strlen(copyr_chunk);
        put_big_long(ID_copy);
        put_big_long(n);
        write_bytes((unsigned char *)copyr_chunk, n);
        if( ODD(n) )
            put_byte(0);
    }
    if( name_chunk ) {
        n = strlen(name_chunk);
        put_big_long(ID_NAME);
        put_big_long(n);
        write_bytes((unsigned char *)name_chunk, n);
        if( ODD(n) )
            put_byte(0);
    }
    if( text_chunk ) {
        n = strlen(text_chunk);
        put_big_long(ID_TEXT);
        put_big_long(n);
        write_bytes((unsigned char *)text_chunk, n);
        if( ODD(n) )
            put_byte(0);
    }
}


static void
write_cmap(colormap, colors, maxval)
    pixel *colormap;
    int colors, maxval;
{
    int cmapsize, i;

    cmapsize = 3 * colors;

    /* write colormap */
    put_big_long(ID_CMAP);
    put_big_long(cmapsize);
    if( maxval != MAXCOLVAL ) {
        int *table;
        pm_message("maxval is not %d - automatically rescaling colors", 
                   MAXCOLVAL);
        table = make_val_table(maxval, MAXCOLVAL);
        for( i = 0; i < colors; i++ ) {
            put_byte(table[PPM_GETR(colormap[i])]);
            put_byte(table[PPM_GETG(colormap[i])]);
            put_byte(table[PPM_GETB(colormap[i])]);
        }
        free(table);
    }
    else {
        for( i = 0; i < colors; i++ ) {
            put_byte(PPM_GETR(colormap[i]));
            put_byte(PPM_GETG(colormap[i]));
            put_byte(PPM_GETB(colormap[i]));
        }
    }
    if( ODD(cmapsize) )
        put_byte(0);
}


static void
write_bmhd(cols, rows, nPlanes)
    int cols, rows, nPlanes;
{
    unsigned char xasp = 10, yasp = 10;

    if( viewportmodes & vmLACE )
        xasp *= 2;
    if( viewportmodes & vmHIRES )
        yasp *= 2;

    put_big_long(ID_BMHD);
    put_big_long(BitMapHeaderSize);

    put_big_short(cols);
    put_big_short(rows);
    put_big_short(0);                       /* x-offset */
    put_big_short(0);                       /* y-offset */
    put_byte(nPlanes);                      /* no of planes */
    put_byte(maskmethod);                   /* masking */
    put_byte(compmethod);                   /* compression */
    put_byte(BMHD_FLAGS_CMAPOK);            /* flags */
    if( maskmethod == mskHasTransparentColor )
        put_big_short(transpIndex);
    else
        put_big_short(0);
    put_byte(xasp);                         /* x-aspect */
    put_byte(yasp);                         /* y-aspect */
    put_big_short(cols);                    /* pageWidth */
    put_big_short(rows);                    /* pageHeight */
}


/* encode algorithm by Johan Widen (jw@jwdata.se) */
static const unsigned char bitmask[] = {1, 2, 4, 8, 16, 32, 64, 128};

static long
encode_row(outfile, rawrow, cols, nPlanes)
    FILE *outfile;  /* if non-NULL, write uncompressed row to this file */
    rawtype *rawrow;
    int cols, nPlanes;
{
    int plane, bytes;
    long retbytes = 0;

    bytes = RowBytes(cols);

    /* Encode and write raw bytes in plane-interleaved form. */
    for( plane = 0; plane < nPlanes; plane++ ) {
        register int col, cbit;
        register rawtype *rp;
        register unsigned char *cp;
        int mask;

        mask = 1 << plane;
        cbit = -1;
        cp = coded_rowbuf-1;
        rp = rawrow;
        for( col = 0; col < cols; col++, cbit--, rp++ ) {
            if( cbit < 0 ) {
                cbit = 7;
                *++cp = 0;
            }
            if( *rp & mask )
                *cp |= bitmask[cbit];
        }
        if( outfile ) {
            write_bytes(coded_rowbuf, bytes);
            retbytes += bytes;
        }
        else
            retbytes += compress_row(bytes);
    }
    return retbytes;
}


static long
encode_maskrow(ofp, rawrow, cols)
    FILE *ofp;
    rawtype *rawrow;
    int cols;
{
    int col;

    for( col = 0; col < cols; col++ ) {
        if( maskrow[col] == PBM_BLACK )
            rawrow[col] = 1;
        else
            rawrow[col] = 0;
    }
    return encode_row(ofp, rawrow, cols, 1);
}


static int
compress_row(bytes)
    int bytes;
{
    int newbytes;

    switch( compmethod ) {
        case cmpByteRun1:
            newbytes = runbyte1(bytes);
            break;
        default:
            pm_error("compress_row(): unknown compression method %d", 
                     compmethod);
    }
    store_bodyrow(compr_rowbuf, newbytes);

    return newbytes;
}


static void
store_bodyrow(row, len)
    unsigned char *row;
    int len;
{
    int idx = cur_block->used;
    if( idx >= ROWS_PER_BLOCK ) {
        MALLOCVAR_NOFAIL(cur_block->next);
        cur_block = cur_block->next;
        cur_block->used = idx = 0;
        cur_block->next = NULL;
    }
    MALLOCARRAY_NOFAIL(cur_block->row[idx], len);
    cur_block->len[idx] = len;
    memcpy(cur_block->row[idx], row, len);
    cur_block->used++;
}


static void
write_body_rows ARGS((void))
{
    bodyblock *b;
    int i;
    long total = 0;

    for( b = &firstblock; b != NULL; b = b->next ) {
        for( i = 0; i < b->used; i++ ) {
            write_bytes(b->row[i], b->len[i]);
            total += b->len[i];
        }
    }
    if( ODD(total) )
        put_byte(0);
}


static void
write_camg ARGS((void))
{
    put_big_long(ID_CAMG);
    put_big_long(CAMGChunkSize);
    put_big_long(viewportmodes);
}


/************ compression ************/


/* runbyte1 algorithm by Robert A. Knop (rknop@mop.caltech.edu) */
static int
runbyte1(size)
   int size;
{
    int in,out,count,hold;
    register unsigned char *inbuf = coded_rowbuf;
    register unsigned char *outbuf = compr_rowbuf;


    in=out=0;
    while( in<size ) {
        if( (in<size-1) && (inbuf[in]==inbuf[in+1]) ) {    
            /*Begin replicate run*/
            for( count=0, hold=in; 
                 in < size && inbuf[in] == inbuf[hold] && count < 128; 
                 in++, count++)
                ;
            outbuf[out++]=(unsigned char)(char)(-count+1);
            outbuf[out++]=inbuf[hold];
        }
        else {  /*Do a literal run*/
            hold=out; out++; count=0;
            while( ((in>=size-2)&&(in<size)) || 
                   ((in<size-2) && ((inbuf[in]!=inbuf[in+1])
                                    ||(inbuf[in]!=inbuf[in+2]))) ) {
                outbuf[out++]=inbuf[in++];
                if( ++count>=128 )
                    break;
            }
            outbuf[hold]=count-1;
        }
    }
    return(out);
}



/************ other utility functions ************/

static void
#if __STDC__
put_big_short(short s)
#else
put_big_short(s)
    short s;
#endif
{
    if ( pm_writebigshort( stdout, s ) == -1 )
        pm_error( "write error" );
}


static void
put_big_long(l)
    long l;
{
    if ( pm_writebiglong( stdout, l ) == -1 )
        pm_error( "write error" );
}


static void
write_bytes(buffer, bytes)
    unsigned char *buffer;
    int bytes;
{
    if( fwrite(buffer, 1, bytes, stdout) != bytes )
        pm_error("write error");
}


static int *
make_val_table(oldmaxval, newmaxval)
    int oldmaxval, newmaxval;
{
    int i;
    int *table;

    MALLOCARRAY_NOFAIL(table, oldmaxval + 1);
    for(i = 0; i <= oldmaxval; i++ )
        table[i] = (i * newmaxval + oldmaxval/2) / oldmaxval;

    return table;
}



static int  gFormat;
static int  gCols;
static int  gMaxval;

static void
init_read(fp, colsP, rowsP, maxvalP, formatP, readall)
    FILE *fp;
    int *colsP, *rowsP;
    pixval *maxvalP;
    int *formatP;
    int readall;
{
    ppm_readppminit(fp, colsP, rowsP, maxvalP, formatP);

    if( *rowsP >INT16MAX || *colsP >INT16MAX )
      pm_error ("Input image is too large.");

    if( readall ) {
        int row;

        pixels = ppm_allocarray(*colsP, *rowsP);
        for( row = 0; row < *rowsP; row++ )
            ppm_readppmrow(fp, pixels[row], *colsP, *maxvalP, *formatP);
        /* pixels = ppm_readppm(fp, colsP, rowsP, maxvalP); */
    }
    else {
        pixrow = ppm_allocrow(*colsP);
    }
    gCols = *colsP;
    gMaxval = *maxvalP;
    gFormat = *formatP;
}


static pixel *
next_pixrow(fp, row)
    FILE *fp;
    int row;
{
    if( pixels )
        pixrow = pixels[row];
    else {
        ppm_readppmrow(fp, pixrow, gCols, gMaxval, gFormat);
    }
    if( maskrow ) {
        int col;

        if( maskfile )
            pbm_readpbmrow(maskfile, maskrow, maskcols, maskformat);
        else {
            for( col = 0; col < gCols; col++ )
                maskrow[col] = PBM_BLACK;
        }
        if( transpColor ) {
            for( col = 0; col < gCols; col++ )
                if( PPM_EQUAL(pixrow[col], *transpColor) )
                    maskrow[col] = PBM_WHITE;
        }
    }
    return pixrow;
}



int
main(int argc, char ** argv) {

    FILE * ifP;
    int argn, rows, cols, format, nPlanes;
    int ifmode, forcemode, maxplanes, fixplanes, mode;
    int hamplanes;
    int deepbits;   /* bits per color component in deep ILBM */
    DirectColor dcol;
#define MAXCOLORS       (1<<maxplanes)
    pixval maxval;
    pixel * colormap;
    int colors = 0;
    pixval cmapmaxval;      /* maxval of colors in cmap */
    const char * mapfile;
    const char * transpname;

    ppm_init(&argc, argv);

    colormap = NULL;  /* initial value */
    ifmode = DEF_IFMODE; forcemode = MODE_NONE;
    maxplanes = DEF_MAXPLANES; fixplanes = 0;
    hamplanes = DEF_HAMPLANES;
    deepbits = DEF_DEEPPLANES;
    dcol.r = dcol.g = dcol.b = DEF_DCOLPLANES;
    mapfile = transpname = NULL;

    argn = 1;
    while( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' ) {
        if( pm_keymatch(argv[argn], "-ilbm", 5) ) {
            if( forcemode == MODE_RGB8 || forcemode == MODE_RGBN )
                forcemode = MODE_NONE;
        }
        else
        if( pm_keymatch(argv[argn], "-rgb8", 5) )
            forcemode = MODE_RGB8;
        else
        if( pm_keymatch(argv[argn], "-rgbn", 5) )
            forcemode = MODE_RGBN;
        else
        if( pm_keymatch(argv[argn], "-maxplanes", 4) || 
            pm_keymatch(argv[argn], "-mp", 3) ) {
            if( ++argn >= argc )
                pm_error("-maxplanes requires a value");
            maxplanes = get_int_val(argv[argn], argv[argn-1], 1, MAXPLANES);
            fixplanes = 0;
        }
        else
        if( pm_keymatch(argv[argn], "-fixplanes", 4) || 
            pm_keymatch(argv[argn], "-fp", 3) ) {
            if( ++argn >= argc )
                pm_error("-fixplanes requires a value");
            fixplanes = get_int_val(argv[argn], argv[argn-1], 1, MAXPLANES);
            maxplanes = fixplanes;
        }
        else
        if( pm_keymatch(argv[argn], "-mapfile", 4) ) {
            if( ++argn >= argc )
                pm_error("-mapfile requires a value");
            mapfile = argv[argn];
        }
        else
        if( pm_keymatch(argv[argn], "-mmethod", 3) ) {
            if( ++argn >= argc )
                pm_error("-mmethod requires a value");
            maskmethod = get_mask_type(argv[argn]);
            switch( maskmethod ) {
                case mskNone:
                case mskHasMask:
                case mskHasTransparentColor:
                    break;
                default:
                    pm_error("This program does not know how to handle "
                             "masking method '%s'", 
                             mskNAME[maskmethod]);
            }
        }
        else
        if( pm_keymatch(argv[argn], "-maskfile", 4) ) {
            if( ++argn >= argc )
                pm_error("-maskfile requires a value");
            maskfile = pm_openr(argv[argn]);
            if( maskmethod == mskNone )
                maskmethod = mskHasMask;
        }
        else
        if( pm_keymatch(argv[argn], "-transparent", 3) ) {
            if( ++argn >= argc )
                pm_error("-transparent requires a value");
            transpname = argv[argn];
            if( maskmethod == mskNone )
                maskmethod = mskHasTransparentColor;
        }
        else
        if( pm_keymatch(argv[argn], "-sortcmap", 5) )
            sortcmap = 1;
        else
        if( pm_keymatch(argv[argn], "-cmaponly", 3) ) {
            forcemode = MODE_CMAP;
        }
        else
        if( pm_keymatch(argv[argn], "-lace", 2) ) {
            slicesize = 2;
            viewportmodes |= vmLACE;
            gen_camg = 1;
        }
        else
        if( pm_keymatch(argv[argn], "-nolace", 4) ) {
            slicesize = 1;
            viewportmodes &= ~vmLACE;
        }
        else
        if( pm_keymatch(argv[argn], "-hires", 3) ) {
            viewportmodes |= vmHIRES;
            gen_camg = 1;
        }
        else
        if( pm_keymatch(argv[argn], "-nohires", 5) )
            viewportmodes &= ~vmHIRES;
        else
        if( pm_keymatch(argv[argn], "-camg", 5) ) {
            char *tail;
            long value = 0L;

            if( ++argn >= argc )
                pm_error("-camg requires a value");
            value = strtol(argv[argn], &tail, 16);
            /* TODO: should do some error checking here */
            viewportmodes |= value;
            gen_camg = 1;
        }
        else
        if( pm_keymatch(argv[argn], "-ecs", 2) ) {
            maxplanes = ECS_MAXPLANES;
            hamplanes = ECS_HAMPLANES;
        }
        else
        if( pm_keymatch(argv[argn], "-aga", 3) ) {
            maxplanes = AGA_MAXPLANES;
            hamplanes = AGA_HAMPLANES;
        }
        else
        if( pm_keymatch(argv[argn], "-hamplanes", 5) ) {
            if( ++argn >= argc )
                pm_error("-hamplanes requires a value");
            hamplanes = get_int_val(argv[argn], argv[argn-1], 3, HAMMAXPLANES);
        }
        else
        if( pm_keymatch(argv[argn], "-hambits", 5) ) {
            if( ++argn >= argc )
                pm_usage("-hambits requires a value");
            hamplanes = 
                get_int_val(argv[argn], argv[argn-1], 3, HAMMAXPLANES-2) +2;
        }
        else
        if( pm_keymatch(argv[argn], "-ham6", 5) ) {
            hamplanes = ECS_HAMPLANES;
            forcemode = MODE_HAM;
        }
        else
        if( pm_keymatch(argv[argn], "-ham8", 5) ) {
            hamplanes = AGA_HAMPLANES;
            forcemode = MODE_HAM;
        }
        else
        if( pm_keymatch(argv[argn], "-hammap", 5) ) {
            if( ++argn >= argc )
                pm_error("-hammap requires a value");
            hammapmode = get_hammap_mode(argv[argn]);
        }
        else
        if( pm_keymatch(argv[argn], "-hamif", 5) )
            ifmode = MODE_HAM;
        else
        if( pm_keymatch(argv[argn], "-nohamif", 7) ) {
            if( ifmode == MODE_HAM )
                ifmode = MODE_NONE;
        }
        else
        if( pm_keymatch(argv[argn], "-hamforce", 4) )
            forcemode = MODE_HAM;
        else
        if( pm_keymatch(argv[argn], "-nohamforce", 6) ) {
            if( forcemode == MODE_HAM )
                forcemode = MODE_NONE;
        }
        else
        if( pm_keymatch(argv[argn], "-24if", 4) ) {
            ifmode = MODE_DEEP;
            deepbits = 8;
        }
        else
        if( pm_keymatch(argv[argn], "-no24if", 6) ) {
            if( ifmode == MODE_DEEP )
                ifmode = MODE_NONE;
        }
        else
        if( pm_keymatch(argv[argn], "-24force", 3) ) {
            forcemode = MODE_DEEP;
            deepbits = 8;
        }
        else
        if( pm_keymatch(argv[argn], "-no24force", 5) ) {
            if( forcemode == MODE_DEEP )
                forcemode = MODE_NONE;
        }
        else
        if( pm_keymatch(argv[argn], "-deepplanes", 6) ) {
            if( ++argn >= argc )
                pm_error("-deepplanes requires a value");
            deepbits = get_int_val(argv[argn], argv[argn-1], 3, 3*MAXPLANES);
            if( deepbits % 3 != 0 )
                pm_error("option \"%s\" argument value must be divisible by 3",
                         argv[argn-1]);
            deepbits /= 3;
        }
        else
        if( pm_keymatch(argv[argn], "-deepbits", 6) ) {
            if( ++argn >= argc )
                pm_error("-deepbits requires a value");
            deepbits = get_int_val(argv[argn], argv[argn-1], 1, MAXPLANES);
        }
        else
        if( pm_keymatch(argv[argn], "-deepif", 6) )
            ifmode = MODE_DEEP;
        else
        if( pm_keymatch(argv[argn], "-nodeepif", 8) ) {
            if( ifmode == MODE_DEEP )
                ifmode = MODE_NONE;
        }
        else
        if( pm_keymatch(argv[argn], "-deepforce", 5) )
            forcemode = MODE_DEEP;
        else
        if( pm_keymatch(argv[argn], "-nodeepforce", 7) ) {
            if( forcemode == MODE_DEEP )
                forcemode = MODE_NONE;
        }
        else
        if( pm_keymatch(argv[argn], "-dcif", 4) )
            ifmode = MODE_DCOL;
        else
        if( pm_keymatch(argv[argn], "-nodcif", 6) ) {
            if( ifmode == MODE_DCOL )
                ifmode = MODE_NONE;
        }
        else
        if( pm_keymatch(argv[argn], "-dcforce", 4) )
            forcemode = MODE_DCOL;
        else
        if( pm_keymatch(argv[argn], "-nodcforce", 6) ) {
            if( forcemode == MODE_DCOL )
                forcemode = MODE_NONE;
        }
        else
        if( pm_keymatch(argv[argn], "-dcbits", 4) || 
            pm_keymatch(argv[argn], "-dcplanes", 4) ) {
            if( argc - argn < 4 )
                pm_error("-dcbits requires 4 arguments");
            dcol.r = get_int_val(argv[argn+1], argv[argn], 1, MAXPLANES);
            dcol.g = get_int_val(argv[argn+2], argv[argn], 1, MAXPLANES);
            dcol.b = get_int_val(argv[argn+3], argv[argn], 1, MAXPLANES);
            argn += 3;
        }
        else
        if( pm_keymatch(argv[argn], "-normal", 4) ) {
            ifmode = forcemode = MODE_NONE;
            compmethod = DEF_COMPRESSION;
        }
        else
        if( pm_keymatch(argv[argn], "-compress", 4) ) {
            compr_force = 1;
            if( compmethod == cmpNone )
#if DEF_COMPRESSION == cmpNone
                    compmethod = cmpByteRun1;
#else
                    compmethod = DEF_COMPRESSION;
#endif
        }
        else
        if( pm_keymatch(argv[argn], "-nocompress", 4) ) {
            compr_force = 0;
            compmethod = cmpNone;
        }
        else
        if( pm_keymatch(argv[argn], "-cmethod", 4) ) {
            if( ++argn >= argc )
                pm_error("-cmethod requires a value");
            compmethod = get_compr_method(argv[argn]);
        }
        else
        if( pm_keymatch(argv[argn], "-floyd", 3) || 
            pm_keymatch(argv[argn], "-fs", 3) )
            floyd = 1;
        else
        if( pm_keymatch(argv[argn], "-nofloyd", 5) || 
            pm_keymatch(argv[argn], "-nofs", 5) )
            floyd = 0;
        else
        if( pm_keymatch(argv[argn], "-annotation", 3) ) {
            if( ++argn >= argc )
                pm_error("-annotation requires a value");
            anno_chunk = argv[argn];
        }
        else
        if( pm_keymatch(argv[argn], "-author", 3) ) {
            if( ++argn >= argc )
                pm_error("-author requires a value");
            auth_chunk = argv[argn];
        }
        else
        if( pm_keymatch(argv[argn], "-copyright", 4) ) {
            if( ++argn >= argc )
                pm_error("-copyright requires a value");
            copyr_chunk = argv[argn];
        }
        else
        if( pm_keymatch(argv[argn], "-name", 3) ) {
            if( ++argn >= argc )
                pm_error("-name requires a value");
            name_chunk = argv[argn];
        }
        else
        if( pm_keymatch(argv[argn], "-text", 3) ) {
            if( ++argn >= argc )
                pm_error("-text requires a value");
            text_chunk = argv[argn];
        }
        else
            pm_error("invalid option: %s", argv[argn]);
        ++argn;
    }

    if( argn < argc ) {
        ifP = pm_openr(argv[argn]);
        ++argn;
    }
    else
        ifP = stdin;

    if( argn != argc )
        pm_error("Program takes no arguments.");

    mode = forcemode;
    switch(forcemode) {
        case MODE_HAM:
            if (hammapmode == HAMMODE_RGB4 || hammapmode == HAMMODE_RGB5)
                init_read(ifP, &cols, &rows, &maxval, &format, 1);
            else
                init_read(ifP, &cols, &rows, &maxval, &format, 0);
            break;
        case MODE_DCOL:
        case MODE_DEEP:
            mapfile = NULL;
            init_read(ifP, &cols, &rows, &maxval, &format, 0);
            break;
        case MODE_RGB8:
            mapfile = NULL;
            init_read(ifP, &cols, &rows, &maxval, &format, 0);
            break;
        case MODE_RGBN:
            mapfile = NULL;
            init_read(ifP, &cols, &rows, &maxval, &format, 0);
            break;
        case MODE_CMAP:
            /* Figure out the colormap. */
            pm_message("computing colormap...");
            colormap = ppm_mapfiletocolorrow(ifP, MAXCOLORS, &colors, 
                                             &cmapmaxval);
            if (colormap == NULL)
                pm_error("too many colors - try doing a 'pnmquant %d'", 
                         MAXCOLORS);
            pm_message("%d colors found", colors);
            break;
        default:
            if (mapfile)
                init_read(ifP, &cols, &rows, &maxval, &format, 0);
            else {
                init_read(ifP, &cols, &rows, &maxval, &format, 1);  
                    /* read file into memory */
                pm_message("computing colormap...");
                colormap = 
                    ppm_computecolorrow(pixels, cols, rows, MAXCOLORS, 
                                        &colors);
                if (colormap) {
                    cmapmaxval = maxval;
                    pm_message("%d colors found", colors);
                    nPlanes = pm_maxvaltobits(colors-1);
                    if (fixplanes > nPlanes)
                        nPlanes = fixplanes;
                } else {  /* too many colors */
                    mode = ifmode;
                    report_too_many_colors(ifmode, maxplanes, hamplanes,
                                           dcol, deepbits );
                }
            }
    }

    if (mapfile) {
        FILE * mapfp;

        pm_message("reading colormap file...");
        mapfp = pm_openr(mapfile);
        colormap = ppm_mapfiletocolorrow(mapfp, MAXCOLORS, &colors, 
                                         &cmapmaxval);
        pm_close(mapfp);
        if (colormap == NULL)
            pm_error("too many colors in mapfile for %d planes", maxplanes);
        if (colors == 0)
            pm_error("empty colormap??");
        pm_message("%d colors found in colormap", colors);
    }

    if (maskmethod != mskNone) {
        if (transpname) {
            MALLOCVAR_NOFAIL(transpColor);
            *transpColor = ppm_parsecolor(transpname, maxval);
        }
        if (maskfile) {
            int maskrows;
            pbm_readpbminit(maskfile, &maskcols, &maskrows, &maskformat);
            if (maskcols < cols || maskrows < rows)
                pm_error("maskfile too small - try scaling it");
            if (maskcols > cols || maskrows > rows)
                pm_message("warning - maskfile larger than image");
        } else
            maskcols = rows;
        maskrow = pbm_allocrow(maskcols);
    }

    if (mode != MODE_CMAP) {
        unsigned int i;
        MALLOCARRAY_NOFAIL(coded_rowbuf, RowBytes(cols));
        for (i = 0; i < RowBytes(cols); ++i)
            coded_rowbuf[i] = 0;
        if (DO_COMPRESS)
            MALLOCARRAY_NOFAIL(compr_rowbuf, WORSTCOMPR(RowBytes(cols)));
    }
    
    switch (mode) {
        case MODE_HAM:
            viewportmodes |= vmHAM;
            ppm_to_ham(ifP, cols, rows, maxval, 
                       colormap, colors, cmapmaxval, hamplanes);
            break;
        case MODE_DEEP:
            ppm_to_deep(ifP, cols, rows, maxval, deepbits);
            break;
        case MODE_DCOL:
            ppm_to_dcol(ifP, cols, rows, maxval, &dcol);
            break;
        case MODE_RGB8:
            ppm_to_rgb8(ifP, cols, rows, maxval);
            break;
        case MODE_RGBN:
            ppm_to_rgbn(ifP, cols, rows, maxval);
            break;
        case MODE_CMAP:
            ppm_to_cmap(colormap, colors, cmapmaxval);
            break;
        default:
            if (mapfile == NULL)
                floyd = 0;          /* would only slow down conversion */
            ppm_to_std(ifP, cols, rows, maxval, colormap, colors, 
                       cmapmaxval, MAXCOLORS, nPlanes);
            break;
    }
    pm_close(ifP);
    return 0;
}
