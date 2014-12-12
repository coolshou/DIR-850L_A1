/*
    jbigtopnm - JBIG to PNM converter
  
    This program was derived from jbgtopbm.c in Markus Kuhn's
    JBIG-KIT package by Bryan Henderson on 2000.05.11

    The main difference is that this version uses the Netpbm libraries.
  
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <jbig.h>
#include "pnm.h"

#define BUFSIZE 8192


static void
collect_image (unsigned char *data, size_t len, void *image) {
    static int cursor = 0;
    int i;

    for (i = 0; i < len; i++) {
        ((unsigned char *)image)[cursor++] = data[i];
    }
}



static void 
write_pnm (FILE *fout, const unsigned char * const image, const int bpp,
           const int rows, const int cols, const int maxval,
           const int format) {

    int row;
    xel *pnm_row;

    pnm_writepnminit(fout, cols, rows, maxval, format, 0);

    pnm_row = pnm_allocrow(cols);

    for (row = 0; row < rows; row++) {
        int col;
        for (col = 0; col < cols; col++) {
            int j;
            for (j = 0; j < bpp; j++)
                PNM_ASSIGN1(pnm_row[col], 
                            image[(((row*cols)+col) * bpp) + j]);
        }
        pnm_writepnmrow(fout, pnm_row, cols, maxval, format, 0);
    }
    
    pnm_freerow(pnm_row);
}



static void
write_raw_pbm(FILE * const fout, 
              const unsigned char * const binary_image,
              int                   const cols,
              int                   const rows) { 

    unsigned int const bytes_per_row = pbm_packed_bytes(cols);

    int row;

    pbm_writepbminit(fout, cols, rows, 0);

    for (row = 0; row < rows; ++row)
        pbm_writepbmrow_packed(fout, &binary_image[row*bytes_per_row], cols, 
                               0);
}



/*
 *
 */
static void 
diagnose_bie(FILE *f)
{
  unsigned char bih[20];
  int len;
  unsigned long xd, yd, l0;

  len = fread(bih, 1, 20, f);
  if (len < 20) {
    printf("Input file is %d < 20 bytes long and does therefore not "
	   "contain an intact BIE header!\n", len);
    return;
  }

  printf("Decomposition of BIH:\n\n  DL = %d\n  D  = %d\n  P  = %d\n"
	 "  -  = %d\n  XD = %lu\n  YD = %lu\n  L0 = %lu\n  MX = %d\n"
	 "  MY = %d\n",
	 bih[0], bih[1], bih[2], bih[3],
	 xd = ((unsigned long) bih[ 4] << 24) | ((unsigned long)bih[ 5] << 16)|
	 ((unsigned long) bih[ 6] <<  8) | ((unsigned long) bih[ 7]),
	 yd = ((unsigned long) bih[ 8] << 24) | ((unsigned long)bih[ 9] << 16)|
	 ((unsigned long) bih[10] <<  8) | ((unsigned long) bih[11]),
	 l0 = ((unsigned long) bih[12] << 24) | ((unsigned long)bih[13] << 16)|
	 ((unsigned long) bih[14] <<  8) | ((unsigned long) bih[15]),
	 bih[16], bih[17]);
  printf("  order   = %d %s%s%s%s%s\n", bih[18],
	 bih[18] & JBG_HITOLO ? " HITOLO" : "",
	 bih[18] & JBG_SEQ ? " SEQ" : "",
	 bih[18] & JBG_ILEAVE ? " ILEAVE" : "",
	 bih[18] & JBG_SMID ? " SMID" : "",
	 bih[18] & 0xf0 ? " other" : "");
  printf("  options = %d %s%s%s%s%s%s%s%s\n", bih[19],
	 bih[19] & JBG_LRLTWO ? " LRLTWO" : "",
	 bih[19] & JBG_VLENGTH ? " VLENGTH" : "",
	 bih[19] & JBG_TPDON ? " TPDON" : "",
	 bih[19] & JBG_TPBON ? " TPBON" : "",
	 bih[19] & JBG_DPON ? " DPON" : "",
	 bih[19] & JBG_DPPRIV ? " DPPRIV" : "",
	 bih[19] & JBG_DPLAST ? " DPLAST" : "",
	 bih[19] & 0x80 ? " other" : "");
  printf("\n  %lu stripes, %d layers, %d planes\n\n",
	 ((yd >> bih[1]) +  ((((1UL << bih[1]) - 1) & xd) != 0) + l0 - 1) / l0,
	 bih[1] - bih[0], bih[2]);

  return;
}


int main (int argc, char **argv)
{
    FILE *fin = stdin, *fout = stdout;
    const char *fnin = "<stdin>", *fnout = "<stdout>";
    int i, j, result;
    int all_args = 0, files = 0;
    struct jbg_dec_state s;
    char *buffer;
    unsigned char *p;
    size_t len, cnt;
    unsigned long xmax = 4294967295UL, ymax = 4294967295UL;
    int plane = -1, use_graycode = 1, diagnose = 0;

    pnm_init(&argc, argv);

    buffer = malloc(BUFSIZE);
    if (!buffer)
        pm_error("Sorry, not enough memory available!");

    /* parse command line arguments */
    for (i = 1; i < argc; i++) {
        if (!all_args && argv[i][0] == '-') {
            if (argv[i][1] == '\0' && files == 0)
                ++files;
            else {
                for (j = 1; j > 0 && argv[i][j]; j++) {
                    switch(tolower(argv[i][j])) {
                    case '-' :
                        all_args = 1;
                        break;
                    case 'b':
                        use_graycode = 0;
                        break;
                    case 'd':
                        diagnose = 1;
                        break;
                    case 'x':
                        if (++i >= argc)
                            pm_error("-x needs a value");
                        xmax = atol(argv[i]);
                        j = -1;
                        break;
                    case 'y':
                        if (++i >= argc)
                            pm_error("-y needs a value");
                        ymax = atol(argv[i]);
                        j = -1;
                        break;
                    case 'p':
                        if (++i >= argc)
                            pm_error("-p needs a value");
                        plane = atoi(argv[i]);
                        j = -1;
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
                    if (!fin)
                        pm_error("Can't open input file '%s'", fnin);
                }
                if (diagnose) {
                    diagnose_bie(fin);
                    exit(0);
                }
                break;
            case 1:
                fnout = argv[i];
                fout = fopen(fnout, "wb");
                if (!fout)
                    pm_error("Can't open output file '%s'", fnout);
                break;
            default:
                pm_error("Too many non-option arguments");
            }
        }
    }

    /* send input file to decoder */
    jbg_dec_init(&s);
    jbg_dec_maxsize(&s, xmax, ymax);
    result = JBG_EAGAIN;
    do {
        len = fread(buffer, 1, BUFSIZE, fin);
        if (!len) break;
        cnt = 0;
        p = (unsigned char *) buffer;
        while (len > 0 && (result == JBG_EAGAIN || result == JBG_EOK)) {
            result = jbg_dec_in(&s, p, len, &cnt);
            p += cnt;
            len -= cnt;
        }
    } while (result == JBG_EAGAIN || result == JBG_EOK);
    if (ferror(fin)) 
        pm_error("Problem while reading input file '%s", fnin);
    if (result != JBG_EOK && result != JBG_EOK_INTR) 
        pm_error("Problem with input file '%s': %s\n", 
                 fnin, jbg_strerror(result, JBG_EN));
    if (plane >= 0 && jbg_dec_getplanes(&s) <= plane) 
        pm_error("Image has only %d planes!\n", jbg_dec_getplanes(&s));

    {
        /* Write it out */

        int rows, cols;
        int maxval;
        int bpp;
        int plane_to_write;

        cols = jbg_dec_getwidth(&s);
        rows = jbg_dec_getheight(&s);
        maxval = pm_bitstomaxval(jbg_dec_getplanes(&s));
        bpp = (jbg_dec_getplanes(&s)+7)/8;

        if (jbg_dec_getplanes(&s) == 1) 
            plane_to_write = 0;
        else 
            plane_to_write = plane;

        if (plane_to_write >= 0) {
            /* Write just one plane */
            unsigned char * binary_image;

            pm_message("WRITING PBM FILE");

            binary_image=jbg_dec_getimage(&s, plane_to_write);
            write_raw_pbm(fout, binary_image, cols, rows);
        } else {
            unsigned char *image;
            pm_message("WRITING PGM FILE");

            /* Write out all the planes */
            /* What jbig.doc doesn't tell you is that jbg_dec_merge_planes
               delivers the image in chunks, in consecutive calls to 
               the data-out callback function.  And a row can span two
               chunks.
            */
            image = malloc(cols*rows*bpp);
            jbg_dec_merge_planes(&s, use_graycode, collect_image, image);
            write_pnm(fout, image, bpp, rows, cols, maxval, PGM_TYPE);
            free(image);
        }
    }
  
    pm_close(fout);

    jbg_dec_free(&s);

    return 0;
}
