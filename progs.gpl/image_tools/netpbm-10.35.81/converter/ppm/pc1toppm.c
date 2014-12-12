/* pc1toppm.c - read a Degas Elite PC1 file and produce a PPM

 Copyright (C) 1991 by Steve Belczyk (seb3@gte.com) and Jef Poskanzer.
 Copyright (C) 2004 by Roine Gustafsson (roine@users.sourceforge.net)

 Converted to modern Netpbm code style by Bryan Henderson April 2004.
 That work is contributed to the public domain by Bryan.

 Permission to use, copy, modify, and distribute this software and its
 documentation for any purpose and without fee is hereby granted, provided
 that the above copyright notice appear in all copies and that both that
 copyright notice and this permission notice appear in supporting
 documentation.  This software is provided "as is" without express or
 implied warranty.

 Algorithm for PC1 compression from "gimp-degas" GIMP plugin 
 by Markus F.X.J. Oberhumer

*/

#include "ppm.h"

/* The code has not been tested for other resolutions than 320x200 */
static unsigned int const rows   = 200;
static unsigned int const cols   = 320;
static unsigned int const planes = 4;  /* 4 bits for 16 colors */
static pixval       const maxval = 7;

unsigned int const colsPerBlock = 16;

#define BLOCKS (cols/colsPerBlock)
#define ROWBYTES (cols * planes / 8)
#define BLOCKBYTES (colsPerBlock / 8)
#define PLANEBYTES (ROWBYTES / planes)
#define ROWSHORTS (cols * planes / 16)
#define BLOCKSHORTS (colsPerBlock * planes / 16)
#define PLANESHORTS (ROWSHORTS / planes)



static void
readPalette(FILE *   const ifP, 
            pixel (* const palP)[]) {

    /* Read the palette. */
    unsigned int i;
    for (i = 0; i < 16; ++i) {
        unsigned short j;
        pm_readbigshortu(ifP, &j);
        PPM_ASSIGN((*palP)[i],
                   (j & 0x700) >> 8,
                   (j & 0x070) >> 4,
                   (j & 0x007) >> 0);
    }
}



static void
processStretch(unsigned int     const countbyte,
               FILE *           const ifP,
               unsigned int *   const colP,
               unsigned char ** const lineposP) {

    if (countbyte <= 127) {
        /* countbyte+1 literal bytes follows */

        unsigned int const count = countbyte + 1;
        unsigned int i;

        if (*colP + count > ROWBYTES)
            pm_error("Error in PC1 file.  "
                     "A literal stretch extends beyond the end of a row");

        for (i = 0; i < count; ++i) {
            *(*lineposP)++ = getc(ifP);
            ++(*colP);
        }
    } else {
        /* next byte repeated 257-countbyte times */
                
        unsigned char const duplicated_color = getc(ifP);
        unsigned int  const count = 257 - countbyte;
        unsigned int i;

        if (*colP + count > ROWBYTES)
            pm_error("Error in PC1 file.  "
                     "A run extends beyond the end of a row.");
        for (i = 0; i < count; ++i) {
            *(*lineposP)++ = duplicated_color;
            ++(*colP);
        }
    }
}



static void
readPc1(FILE * const ifP,
        pixel (*palP)[],
        unsigned char (*bufferP)[]) {

    unsigned short j;
    unsigned int row;
    unsigned char* bufferCursor;

    /* Check resolution word */
    pm_readbigshortu(ifP, &j);
    if (j != 0x8000)
        pm_error("This is not a PC1 file.  "
                 "The first two bytes are 0x%04X instead of 0x8000", j);

    readPalette(ifP, palP);

    /* Read the screen data */
    bufferCursor = &(*bufferP)[0];
    for (row = 0; row < rows; ++row) {
        unsigned int col;

        for (col = 0; col < ROWBYTES;) {
            unsigned int const countbyte = getc(ifP);

            processStretch(countbyte, ifP, &col, &bufferCursor);
	    }
	}
}



static void
reInterleave(unsigned char     const buffer[],
             unsigned short (* const screenP)[]) {
    
    /* The buffer is in one plane for each line, to optimize packing */
    /* Re-interleave to match the Atari screen layout                */

    unsigned short * const screen = *screenP;
    unsigned int row;

    for (row = 0; row < rows; ++row) {
        unsigned int block;
        for (block = 0; block < BLOCKS; ++block) {
            unsigned int plane;
            for (plane = 0; plane < planes; ++plane) {
                unsigned int const blockIndex = 
                    row*ROWBYTES + plane*PLANEBYTES + block*BLOCKBYTES;

                screen[row*ROWSHORTS + block*BLOCKSHORTS + plane] = 
                    (buffer[blockIndex+0] << 8) + (buffer[blockIndex+1]);
            }
        }
    }
}



static void
writePpm(FILE *         const ofP,
         unsigned short const screen[],
         pixel          const palette[]) {

    pixel* pixelrow;
    unsigned int row;

    ppm_writeppminit(ofP, cols, rows, maxval, 0);
    pixelrow = ppm_allocrow(cols);

    for (row = 0; row < rows; ++row) {
        /* Each row is arranged into blocks of 16 columns.  Each block
           is represented by 4 shorts (16 bit integers).  The
           first is for Plane 0, the second for Plane 1, etc.  Each short
           contains the bits for that plane for each of the 16 columns,
           arranged from most signficant bit to least in increasing column
           number.
        */
        unsigned int col0ScreenIndex = cols/16*planes * row;

        unsigned int col;
        for (col = 0; col < cols; ++col) {
            unsigned int const ind = col0ScreenIndex + col/16 * planes;
            unsigned int const b = 0x8000 >> (col % 16);

            unsigned int plane;
            unsigned int colorIndex;

            colorIndex = 0;
            for (plane = 0; plane < planes; ++plane)
                if (b & screen[ind+plane])
                    colorIndex |= (1 << plane);
            pixelrow[col] = palette[colorIndex];
        }
        ppm_writeppmrow(ofP, pixelrow, cols, maxval, 0);
    }
    ppm_freerow(pixelrow);
}



int
main(int argc, char ** argv) {

    const char * inputFilename;
    FILE* ifP;
    pixel palette[16];  /* Degas palette */
    static unsigned short screen[32000/2];   
        /* simulates the Atari's video RAM */
    static unsigned char buffer[32000];   
        /* simulates the Atari's video RAM */

    ppm_init(&argc, argv);

    if (argc-1 == 0)
        inputFilename = "-";
    else if (argc-1 == 1)
        inputFilename = argv[1];
    else
        pm_error("There is at most one argument to this program:  "
                 "the input file name.  You specified %d", argc-1);

    ifP = pm_openr(inputFilename);

    readPc1(ifP, &palette, &buffer);

    pm_close(ifP);

    reInterleave(buffer, &screen);

    writePpm(stdout, screen, palette);

    pm_close(stdout);

    return 0;
}
