/* ppmtoterm.c - convert a portable pixmap into an ISO 6429 (ANSI) color ascii
** text.
**
**  Copyright (C) 2002 by Ero Carrera (ecarrera@lightforge.com)
**  Partially based on,
**      ppmtoyuv.c by Marc Boucher,
**      ppmtolj.c by Jonathan Melvin and
**      ppmtogif.c by Jef Poskanzer
**
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** 14/Aug/2002: First version.
**
*/

#include <string.h>

#include "ppm.h"
#include "shhopt.h"


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    unsigned int verbose;
};



static void
parseCommandLine(int argc, char **argv,
                 struct cmdlineInfo *cmdlineP) {
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions3 on how to parse our options */
    optStruct3 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "verbose", OPT_FLAG, NULL, &cmdlineP->verbose, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    switch (argc-1) {
    case 0:
        cmdlineP->inputFilespec = "-";
        break;
    case 1:
        cmdlineP->inputFilespec = argv[1];
        break;
    case 2:
        break;
    }
}


#define ESC         "\x1B\x5B"
#define NUM_COLORS      128
#define MAX_ANSI_STR_LEN    16


static int __inline__ sqr(const int x) {
    return x*x;
}

/*
    Generates some sort of color palette mixing the available
    colors as different values of background, foreground & brightness.
*/
static int 
generate_palette(unsigned char rgb[NUM_COLORS][3], 
                 char ansi_code[NUM_COLORS][MAX_ANSI_STR_LEN]) {
    int code, col=0, cd2=0;
    
    memset((void *)rgb, 0, NUM_COLORS*3);
    memset((void *)ansi_code, 0, NUM_COLORS*MAX_ANSI_STR_LEN);

    for(col=cd2=0; cd2<8; cd2++) {
        unsigned int b;
        for(b=0;b<2;b++) {
            for(code=0; code<8; code++) {
                unsigned int c;
                for(c=0;c<3;c++) {
                    if(code&(1<<c)) {
                        rgb[col][c]=(192|(b?63:0));
                    }
                    if(cd2&(1<<c)) {
                        rgb[col][c]|=(128);
                    }
                }
                sprintf(ansi_code[col],
                        ESC"%dm"ESC"3%dm"ESC"4%dm",
                        b, code, cd2);
                col++;
            }
        }
    }
    return col;
}



int main(int argc, char **argv)
{
    FILE            *ifp;
    pixel           **pixels;
    int             rows, row, cols, col,
                    pal_len, i;
    pixval          maxval;
    struct cmdlineInfo
                    cmdline;
    unsigned char   rgb[NUM_COLORS][3];
    char            ansi_code[NUM_COLORS][MAX_ANSI_STR_LEN];

    
    ppm_init(&argc, argv);    

    parseCommandLine(argc, argv, &cmdline);

    ifp = pm_openr(cmdline.inputFilespec);
    
    pixels = ppm_readppm(ifp, &cols, &rows, &maxval);

    pm_close(ifp);
        
    pal_len=generate_palette(rgb, ansi_code);
    
    for (row = 0; row < rows; ++row) {
        for (col = 0; col < cols; col++) {
            pixval const r=(int)PPM_GETR(pixels[row][col])*255/maxval;
            pixval const g=(int)PPM_GETG(pixels[row][col])*255/maxval;
            pixval const b=(int)PPM_GETB(pixels[row][col])*255/maxval;
            int val, dist;
            
            /*
            The following loop calculates the index that
            corresponds to the minimum color distance
            between the given RGB values and the values
            available in the palette.
            */
            for(i=0, dist=sqr(255)*3, val=0; i<pal_len; i++) {
                pixval const pr=rgb[i][0];
                pixval const pg=rgb[i][1];
                pixval const pb=rgb[i][2];
                unsigned int j;
                if( (j=sqr(r-pr)+sqr(b-pb)+sqr(g-pg))<dist ) {
                    dist=j;
                    val=i;
                }
            }
            printf("%s%c", ansi_code[val],0xB1);
        }
        printf(ESC"\x30m\n");
    }
    printf(ESC"\x30m");

    ppm_freearray(pixels, rows);
    
    exit(0);
}
