/* pgmtopbm.c - read a portable graymap and write a portable bitmap
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

#include <assert.h>
#include "pgm.h"
#include "dithers.h"
#include "mallocvar.h"
#include "shhopt.h"

enum halftone {QT_FS, QT_THRESH, QT_DITHER8, QT_CLUSTER, QT_HILBERT};


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *  inputFilespec;
    enum halftone halftone;
    unsigned int  clumpSize;
    unsigned int  clusterRadius;  
        /* Defined only for halftone == QT_CLUSTER */
    float         threshval;
};




static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    unsigned int floydOpt, hilbertOpt, thresholdOpt, dither8Opt,
        cluster3Opt, cluster4Opt, cluster8Opt;
    unsigned int valueSpec, clumpSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "floyd",     OPT_FLAG,  NULL, &floydOpt,     0);
    OPTENT3(0, "fs",        OPT_FLAG,  NULL, &floydOpt,     0);
    OPTENT3(0, "threshold", OPT_FLAG,  NULL, &thresholdOpt, 0);
    OPTENT3(0, "hilbert",   OPT_FLAG,  NULL, &hilbertOpt,   0);
    OPTENT3(0, "dither8",   OPT_FLAG,  NULL, &dither8Opt,   0);
    OPTENT3(0, "d8",        OPT_FLAG,  NULL, &dither8Opt,   0);
    OPTENT3(0, "cluster3",  OPT_FLAG,  NULL, &cluster3Opt,  0);
    OPTENT3(0, "c3",        OPT_FLAG,  NULL, &cluster3Opt,  0);
    OPTENT3(0, "cluster4",  OPT_FLAG,  NULL, &cluster4Opt,  0);
    OPTENT3(0, "c4",        OPT_FLAG,  NULL, &cluster4Opt,  0);
    OPTENT3(0, "cluster8",  OPT_FLAG,  NULL, &cluster8Opt,  0);
    OPTENT3(0, "c8",        OPT_FLAG,  NULL, &cluster8Opt,  0);
    OPTENT3(0, "value",     OPT_FLOAT, &cmdlineP->threshval, 
            &valueSpec, 0);
    OPTENT3(0, "clump",     OPT_UINT,  &cmdlineP->clumpSize, 
            &clumpSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (floydOpt + thresholdOpt + hilbertOpt + dither8Opt + 
        cluster3Opt + cluster4Opt + cluster8Opt == 0)
        cmdlineP->halftone = QT_FS;
    else if (floydOpt + thresholdOpt + dither8Opt + 
        cluster3Opt + cluster4Opt + cluster8Opt > 1)
        pm_error("No cannot specify more than one halftoning type");
    else {
        if (floydOpt)
            cmdlineP->halftone = QT_FS;
        else if (thresholdOpt)
            cmdlineP->halftone = QT_THRESH;
        else if (hilbertOpt)
            cmdlineP->halftone = QT_HILBERT;
        else if (dither8Opt)
            cmdlineP->halftone = QT_DITHER8;
        else if (cluster3Opt) {
            cmdlineP->halftone = QT_CLUSTER;
            cmdlineP->clusterRadius = 3;
        } else if (cluster4Opt) {
            cmdlineP->halftone = QT_CLUSTER;
            cmdlineP->clusterRadius = 4;
        } else if (cluster8Opt) {
            cmdlineP->halftone = QT_CLUSTER;
            cmdlineP->clusterRadius = 8;
        } else 
            pm_error("INTERNAL ERROR.  No halftone option");
    }

    if (!valueSpec)
        cmdlineP->threshval = 0.5;
    else {
        if (cmdlineP->threshval < 0.0)
            pm_error("-value cannot be negative.  You specified %f",
                     cmdlineP->threshval);
        if (cmdlineP->threshval > 1.0)
            pm_error("-value cannot be greater than one.  You specified %f",
                     cmdlineP->threshval);
    }
            
    if (!clumpSpec)
        cmdlineP->clumpSize = 5;
    else {
        if (cmdlineP->clumpSize < 2)
            pm_error("-clump must be at least 2.  You specified %u",
                     cmdlineP->clumpSize);
    }

    if (argc-1 > 1)
        pm_error("Too many arguments (%d).  There is at most one "
                 "non-option argument:  the file name",
                 argc-1);
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else
        cmdlineP->inputFilespec = "-";
}



/* Hilbert curve tracer */

#define MAXORD 18

static int hil_order,hil_ord;
static int hil_turn;
static int hil_dx,hil_dy;
static int hil_x,hil_y;
static int hil_stage[MAXORD];
static int hil_width,hil_height;

static void 
init_hilbert(int const w, 
             int const h) {
/*----------------------------------------------------------------------------
  Initialize the Hilbert curve tracer 
-----------------------------------------------------------------------------*/
    int big,ber;
    hil_width = w;
    hil_height = h;
    big = w > h ? w : h;
    for (ber = 2, hil_order = 1; ber < big; ber <<= 1, hil_order++);
    if (hil_order > MAXORD)
        pm_error("Sorry, hilbert order is too large");
    hil_ord = hil_order;
    hil_order--;
}



static int 
hilbert(int * const px, int * const py) {
/*----------------------------------------------------------------------------
  Return non-zero if got another point
-----------------------------------------------------------------------------*/
    int temp;
    if (hil_ord > hil_order) {
        /* have to do first point */

        hil_ord--;
        hil_stage[hil_ord] = 0;
        hil_turn = -1;
        hil_dy = 1;
        hil_dx = hil_x = hil_y = 0;
        *px = *py = 0;
        return 1;
    }

    /* Operate the state machine */
    for(;;)  {
        switch (hil_stage[hil_ord]) {
        case 0:
            hil_turn = -hil_turn;
            temp = hil_dy;
            hil_dy = -hil_turn * hil_dx;
            hil_dx = hil_turn * temp;
            if (hil_ord > 0) {
                hil_stage[hil_ord] = 1;
                hil_ord--;
                hil_stage[hil_ord]=0;
                continue;
            }
        case 1:
            hil_x += hil_dx;
            hil_y += hil_dy;
            if (hil_x < hil_width && hil_y < hil_height) {
                hil_stage[hil_ord] = 2;
                *px = hil_x;
                *py = hil_y;
                return 1;
            }
        case 2:
            hil_turn = -hil_turn;
            temp = hil_dy;
            hil_dy = -hil_turn * hil_dx;
            hil_dx = hil_turn * temp;
            if (hil_ord > 0) { 
                /* recurse */

                hil_stage[hil_ord] = 3;
                hil_ord--;
                hil_stage[hil_ord]=0;
                continue;
            }
        case 3:
            hil_x += hil_dx;
            hil_y += hil_dy;
            if (hil_x < hil_width && hil_y < hil_height) {
                hil_stage[hil_ord] = 4;
                *px = hil_x;
                *py = hil_y;
                return 1;
            }
        case 4:
            if (hil_ord > 0) {
                /* recurse */
                hil_stage[hil_ord] = 5;
                hil_ord--;
                hil_stage[hil_ord]=0;
                continue;
            }
        case 5:
            temp = hil_dy;
            hil_dy = -hil_turn * hil_dx;
            hil_dx = hil_turn * temp;
            hil_turn = -hil_turn;
            hil_x += hil_dx;
            hil_y += hil_dy;
            if (hil_x < hil_width && hil_y < hil_height) {
                hil_stage[hil_ord] = 6;
                *px = hil_x;
                *py = hil_y;
                return 1;
            }
        case 6:
            if (hil_ord > 0) {
                /* recurse */
                hil_stage[hil_ord] = 7;
                hil_ord--;
                hil_stage[hil_ord]=0;
                continue;
            }
        case 7:
            temp = hil_dy;
            hil_dy = -hil_turn * hil_dx;
            hil_dx = hil_turn * temp;
            hil_turn = -hil_turn;
            /* Return from a recursion */
            if (hil_ord < hil_order)
                hil_ord++;
            else
                return 0;
        }
    }
}



static void doHilbert(FILE *       const ifP,
                      unsigned int const clump_size) {
/*----------------------------------------------------------------------------
  Use hilbert space filling curve dithering
-----------------------------------------------------------------------------*/
    /*
     * This is taken from the article "Digital Halftoning with
     * Space Filling Curves" by Luiz Velho, proceedings of
     * SIGRAPH '91, page 81.
     *
     * This is not a terribly efficient or quick version of
     * this algorithm, but it seems to work. - Graeme Gill.
     * graeme@labtam.labtam.OZ.AU
     *
     */

    int cols, rows;
    gray maxval;
    gray **grays;
    bit **bits;
    int end;
    int *x,*y;
    int sum = 0;

    grays = pgm_readpgm(ifP, &cols,&rows, &maxval);
    bits = pbm_allocarray(cols,rows);

    MALLOCARRAY(x, clump_size);
    MALLOCARRAY(y, clump_size);
    if (x == NULL  || y == NULL)
        pm_error("out of memory");
    init_hilbert(cols,rows);

    end = clump_size;
    while (end == clump_size) {
        int i;
        /* compute the next clust co-ordinates along hilbert path */
        for (i = 0; i < end; i++) {
            if (hilbert(&x[i],&y[i])==0)
                end = i;    /* we reached the end */
        }
        /* sum levels */
        for (i = 0; i < end; i++)
            sum += grays[y[i]][x[i]];
        /* dither half and half along path */
        for (i = 0; i < end; i++) {
            if (sum >= maxval) {
                bits[y[i]][x[i]] = PBM_WHITE;
                sum -= maxval;
            } else
                bits[y[i]][x[i]] = PBM_BLACK;
        }
    }
    pbm_writepbm(stdout, bits, cols, rows, 0);
}



struct converter {
    void (*convertRow)(struct converter * const converterP,
                       unsigned int       const row,
                       gray                     grayrow[], 
                       bit                      bitrow[]);
    void (*destroy)(struct converter * const converterP);
    unsigned int cols;
    gray maxval;
    void * stateP;
};



unsigned int const fs_scale      = 1024;
unsigned int const half_fs_scale = 512;

struct fsState {
    long* thiserr;
    long* nexterr;
    bool fs_forward;
    long threshval;  /* Threshold gray value, scaled by FS_SCALE */
};


static void
fsConvertRow(struct converter * const converterP,
             unsigned int       const row,
             gray                     grayrow[],
             bit                      bitrow[]) {

    struct fsState * const stateP = converterP->stateP;

    long * const thiserr = stateP->thiserr;
    long * const nexterr = stateP->nexterr;

    bit* bP;
    gray* gP;

    unsigned int limitcol;
    unsigned int col;
    
    for (col = 0; col < converterP->cols + 2; ++col)
        nexterr[col] = 0;
    if (stateP->fs_forward) {
        col = 0;
        limitcol = converterP->cols;
        gP = grayrow;
        bP = bitrow;
    } else {
        col = converterP->cols - 1;
        limitcol = -1;
        gP = &(grayrow[col]);
        bP = &(bitrow[col]);
    }
    do {
        long sum;
        sum = ((long) *gP * fs_scale) / converterP->maxval + 
            thiserr[col + 1];
        if (sum >= stateP->threshval) {
            *bP = PBM_WHITE;
            sum = sum - stateP->threshval - half_fs_scale;
        } else
            *bP = PBM_BLACK;
        
        if (stateP->fs_forward) {
            thiserr[col + 2] += (sum * 7) / 16;
            nexterr[col    ] += (sum * 3) / 16;
            nexterr[col + 1] += (sum * 5) / 16;
            nexterr[col + 2] += (sum    ) / 16;
            
            ++col;
            ++gP;
            ++bP;
        } else {
            thiserr[col    ] += (sum * 7) / 16;
            nexterr[col + 2] += (sum * 3) / 16;
            nexterr[col + 1] += (sum * 5) / 16;
            nexterr[col    ] += (sum    ) / 16;
            
            --col;
            --gP;
            --bP;
        }
    } while (col != limitcol);
    
    stateP->thiserr = nexterr;
    stateP->nexterr = thiserr;
    stateP->fs_forward = ! stateP->fs_forward;
}



static void
fsDestroy(struct converter * const converterP) {
    free(converterP->stateP);
}



static struct converter
createFsConverter(unsigned int const cols, 
                  gray         const maxval,
                  float        const threshFraction) {

    struct fsState * stateP;
    struct converter converter;

    MALLOCVAR_NOFAIL(stateP);

    /* Initialize Floyd-Steinberg error vectors. */
    MALLOCARRAY_NOFAIL(stateP->thiserr, cols + 2);
    MALLOCARRAY_NOFAIL(stateP->nexterr, cols + 2);
    srand((int)(time(NULL) ^ getpid()));

    {
        /* (random errors in [-fs_scale/8 .. fs_scale/8]) */
        unsigned int col;
        for (col = 0; col < cols + 2; ++col)
            stateP->thiserr[col] = 
                (long)(rand() % fs_scale - half_fs_scale) / 4;
    }

    stateP->fs_forward = TRUE;
    stateP->threshval = threshFraction * fs_scale;
    converter.stateP = stateP;
    converter.cols = cols;
    converter.maxval = maxval;
    converter.convertRow = &fsConvertRow;
    converter.destroy = &fsDestroy;

    return converter;
}



struct threshState {
    gray threshval;
};


static void
threshConvertRow(struct converter * const converterP,
                 unsigned int       const row,
                 gray                     grayrow[],
                 bit                      bitrow[]) {
    
    struct threshState * const stateP = converterP->stateP;

    unsigned int col;
    for (col = 0; col < converterP->cols; ++col)
        if (grayrow[col] >= stateP->threshval)
            bitrow[col] = PBM_WHITE;
        else
            bitrow[col] = PBM_BLACK;
}



static void
threshDestroy(struct converter * const converterP) {
    free(converterP->stateP);
}



static struct converter
createThreshConverter(unsigned int const cols, 
                      gray         const maxval,
                      float        const threshFraction) {

    struct threshState * stateP;
    struct converter converter;

    MALLOCVAR_NOFAIL(stateP);

    converter.cols       = cols;
    converter.maxval     = maxval;
    converter.convertRow = &threshConvertRow;
    converter.destroy    = &threshDestroy;

    stateP->threshval    = ROUNDU(maxval * threshFraction);
    converter.stateP     = stateP;

    return converter;
}



static void
dither8ConvertRow(struct converter * const converterP,
                  unsigned int       const row,
                  gray                     grayrow[],
                  bit                      bitrow[]) {

    unsigned int col;

    for (col = 0; col < converterP->cols; ++col)
        if (grayrow[col] > dither8[row % 16][col % 16])
            bitrow[col] = PBM_WHITE;
        else
            bitrow[col] = PBM_BLACK;
}



static struct converter
createDither8Converter(unsigned int const cols, 
                       gray         const maxval) {

    struct converter converter;

    unsigned int row;

    converter.cols = cols;
    converter.convertRow = &dither8ConvertRow;
    converter.destroy = NULL;

    /* Scale dither matrix. */
    for (row = 0; row < 16; ++row) {
        unsigned int col;
        for (col = 0; col < 16; ++col)
            dither8[row][col] = dither8[row][col] * maxval / 256;
    }
    return converter;
}



struct clusterState {
    unsigned int radius;
    int ** clusterMatrix;
};



static void
clusterConvertRow(struct converter * const converterP,
                  unsigned int       const row,
                  gray                     grayrow[],
                  bit                      bitrow[]) {

    struct clusterState * const stateP = converterP->stateP;
    unsigned int const diameter = 2 * stateP->radius;

    unsigned int col;

    for (col = 0; col < converterP->cols; ++col)
        if (grayrow[col] >
            stateP->clusterMatrix[row % diameter][col % diameter])
            bitrow[col] = PBM_WHITE;
        else
            bitrow[col] = PBM_BLACK;
}



static void
clusterDestroy(struct converter * const converterP) {

    struct clusterState * const stateP = converterP->stateP;
    unsigned int const diameter = 2 * stateP->radius;

    unsigned int row;

    for (row = 0; row < diameter; ++row)
        free(stateP->clusterMatrix[row]);

    free(stateP->clusterMatrix);
    
    free(stateP);
}



static struct converter
createClusterConverter(unsigned int const radius,
                       unsigned int const cols, 
                       gray         const maxval) {
    
    int const clusterNormalizer = radius * radius * 2;
    unsigned int const diameter = 2 * radius;

    struct converter converter;
    struct clusterState * stateP;
    unsigned int row;

    converter.cols = cols;
    converter.convertRow = &clusterConvertRow;
    converter.destroy = &clusterDestroy;

    MALLOCVAR_NOFAIL(stateP);

    stateP->radius = radius;

    MALLOCARRAY_NOFAIL(stateP->clusterMatrix, diameter);
    for (row = 0; row < diameter; ++row) {
        unsigned int col;

        MALLOCARRAY_NOFAIL(stateP->clusterMatrix[row], diameter);
        
        for (col = 0; col < diameter; ++col) {
            int val;
            switch (radius) {
            case 3: val = cluster3[row][col]; break;
            case 4: val = cluster4[row][col]; break;
            case 8: val = cluster8[row][col]; break;
            default:
                pm_error("INTERNAL ERROR: invalid radius");
            }
            stateP->clusterMatrix[row][col] = val * maxval / clusterNormalizer;
        }
    }            

    converter.stateP = stateP;

    return converter;
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE* ifP;
    gray* grayrow;
    bit* bitrow;

    pgm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    if (cmdline.halftone == QT_HILBERT)
        doHilbert(ifP, cmdline.clumpSize);
    else {
        struct converter converter;
        int cols, rows;
        gray maxval;
        int format;
        int row;

        pgm_readpgminit(ifP, &cols, &rows, &maxval, &format);
        
        pbm_writepbminit(stdout, cols, rows, 0);

        switch (cmdline.halftone) {
        case QT_FS:
            converter = createFsConverter(cols, maxval, cmdline.threshval);
            break;
        case QT_THRESH:
            converter = createThreshConverter(cols, maxval, cmdline.threshval);
            break;
        case QT_DITHER8: 
            converter = createDither8Converter(cols, maxval); 
            break;
        case QT_CLUSTER: 
            converter = 
                createClusterConverter(cmdline.clusterRadius, cols, maxval);
            break;
        case QT_HILBERT: 
                pm_error("INTERNAL ERROR: halftone is QT_HILBERT where it "
                         "shouldn't be.");
                break;
        }

        grayrow = pgm_allocrow(cols);
        bitrow  = pbm_allocrow(cols);

        for (row = 0; row < rows; ++row) {
            pgm_readpgmrow(ifP, grayrow, cols, maxval, format);

            converter.convertRow(&converter, row, grayrow, bitrow);
            
            pbm_writepbmrow(stdout, bitrow, cols, 0);
        }
        pbm_freerow(bitrow);
        pgm_freerow(grayrow);

        if (converter.destroy)
            converter.destroy(&converter);
    }

    pm_close(ifP);

    return 0;
}
