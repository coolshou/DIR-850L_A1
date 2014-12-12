/* ppmtopcx.c - convert a portable pixmap to PCX
**
** Copyright (C) 1994 by Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
** based on ppmtopcx.c by Michael Davidson
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** 11/Dec/94: first version
** 12/Dec/94: added handling of "packed" format (16 colors or less)
*/
#include <assert.h>

#include "ppm.h"
#include "shhopt.h"
#include "mallocvar.h"

#define MAXCOLORS       256

#define PCX_MAGIC       0x0a            /* PCX magic number             */
#define PCX_256_COLORS  0x0c            /* magic number for 256 colors  */
#define PCX_MAXVAL      (pixval)255


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* '-' if stdin */
    unsigned int truecolor;   /* -24bit option */
    unsigned int use_8_bit; /* -8bit option */
    unsigned int planes;    /* zero means minimum */
    unsigned int packed;
    unsigned int verbose;
    unsigned int stdpalette;
    const char * palette;   /* NULL means none */
    int xpos;
    int ypos;
};



struct pcxCmapEntry {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

static struct pcxCmapEntry
pcxCmapEntryFromPixel(pixel const colorPixel) {

    struct pcxCmapEntry retval;

    retval.r = PPM_GETR(colorPixel);
    retval.g = PPM_GETG(colorPixel);
    retval.b = PPM_GETB(colorPixel);

    return retval;
}



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int planesSpec, xposSpec, yposSpec, paletteSpec;

    MALLOCARRAY(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "24bit",     OPT_FLAG,   NULL,                  
            &cmdlineP->truecolor,    0);
    OPTENT3(0, "8bit",      OPT_FLAG,   NULL,    
            &cmdlineP->use_8_bit,    0);
    OPTENT3(0, "planes",    OPT_UINT,   &cmdlineP->planes, 
            &planesSpec,             0);
    OPTENT3(0, "packed",    OPT_FLAG,   NULL,                  
            &cmdlineP->packed,       0);
    OPTENT3(0, "verbose",   OPT_FLAG,   NULL,                  
            &cmdlineP->verbose,      0);
    OPTENT3(0, "stdpalette", OPT_FLAG,  NULL,
            &cmdlineP->stdpalette,   0);
    OPTENT3(0, "palette",    OPT_STRING, &cmdlineP->palette,
            &paletteSpec,   0);
    OPTENT3(0, "xpos",  OPT_INT, &cmdlineP->xpos, &xposSpec,   0);
    OPTENT3(0, "ypos",  OPT_INT, &cmdlineP->ypos, &yposSpec,   0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (!xposSpec)
        cmdlineP->xpos = 0;
    else if (cmdlineP->xpos < -32767 || cmdlineP->xpos > 32768)
        pm_error("-xpos value (%d) is outside acceptable range "
                 "(-32767, 32768)", cmdlineP->xpos);

    if (!yposSpec)
        cmdlineP->ypos = 0;
    else if (cmdlineP->ypos < -32767 || cmdlineP->ypos > 32768)
        pm_error("-ypos value (%d) is outside acceptable range "
                 "(-32767, 32768)", cmdlineP->ypos);

    if (!planesSpec)
        cmdlineP->planes = 0;  /* 0 means minimum */

    if (planesSpec) {
        if (cmdlineP->planes > 4 || cmdlineP->planes < 1)
            pm_error("The only possible numbers of planes are 1-4.  "
                     "You specified %u", cmdlineP->planes);
        if (cmdlineP->packed)
            pm_error("-planes is meaningless with -packed.");
        if (cmdlineP->truecolor)
            pm_error("-planes is meaningless with -24bit");
        if (cmdlineP->use_8_bit)
            pm_error("-planes is meaningless with -8bit");
    }
    
    if (paletteSpec && cmdlineP->stdpalette)
        pm_error("You can't specify both -palette and -stdpalette");

    if (!paletteSpec)
        cmdlineP->palette = NULL;

    if (cmdlineP->use_8_bit && cmdlineP->truecolor) 
        pm_error("You cannot specify both -8bit and -truecolor");

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else
        pm_error("Program takes at most one argument "
                 "(input file specification).  You specified %d",
                 argc-1);
}



/*
 * Write out a two-byte little-endian word to the PCX file
 */
static void
Putword(int    const w, 
        FILE * const fp) {

    int rc;
    
    rc = pm_writelittleshort(fp, w);

    if (rc != 0)
        pm_error("Error writing integer to output file");
}



/*
 * Write out a byte to the PCX file
 */
static void
Putbyte(int    const b, 
        FILE * const fp) {

    int rc;

    rc = fputc(b & 0xff, fp);
    if (rc == EOF)
        pm_error("Error writing byte to output file.");
}



static const unsigned char bitmask[] = {1, 2, 4, 8, 16, 32, 64, 128};


static void
extractPlane(unsigned char * const rawrow, 
             int             const cols, 
             unsigned char * const buf, 
             int             const plane) {
/*----------------------------------------------------------------------------
   From the image row 'rawrow', which is an array of 'cols' palette indices
   (as unsigned 8 bit integers), extract plane number 'plane' and return
   it at 'buf'.

   E.g. Plane 2 is all the 2nd bits from the palette indices, packed so
   that each byte represents 8 columns.
-----------------------------------------------------------------------------*/
    unsigned int const planeMask = 1 << plane;

    unsigned int col;
    int cbit;  /* Significance of bit representing current column in output */
    unsigned char *cp;  /* Ptr to next output byte to fill */
    unsigned char byteUnderConstruction;
    
    cp = buf;  /* initial value */

    cbit = 7;
    byteUnderConstruction = 0x00;
    for (col = 0; col < cols; ++col) {
        if (rawrow[col] & planeMask)
            byteUnderConstruction |= (1 << cbit);

        --cbit;
        if (cbit < 0) {
            /* We've filled a byte.  Output it and start the next */
            *cp++ = byteUnderConstruction;
            cbit = 7;
            byteUnderConstruction = 0x00;
        }
    }
    if (cbit < 7)
        /* A byte was partially built when we ran out of columns (the number
           of columns is not a multiple of 8.  Output the partial byte.
        */
        *cp++ = byteUnderConstruction;
}



static void
PackBits(unsigned char * const rawrow, 
         int             const width, 
         unsigned char * const buf, 
         int             const bits) {

    int x, i, shift;

    shift = i = -1;

    for (x = 0; x < width; ++x) {
        if (shift < 0) {
            shift = 8 - bits;
            buf[++i] = 0;
        }

        buf[i] |= (rawrow[x] << shift);
        shift -= bits;
    }
}



static void
write_header(FILE *              const fp, 
             int                 const cols, 
             int                 const rows, 
             int                 const BitsPerPixel, 
             int                 const Planes, 
             struct pcxCmapEntry const cmap16[],
             unsigned int        const xPos, 
             unsigned int        const yPos) {

    int i, BytesPerLine;

    Putbyte(PCX_MAGIC, fp);        /* .PCX magic number            */
    Putbyte(0x05, fp);             /* PC Paintbrush version        */
    Putbyte(0x01, fp);             /* .PCX run length encoding     */
    Putbyte(BitsPerPixel, fp);     /* bits per pixel               */
    
    Putword(xPos, fp);             /* x1   - image left            */
    Putword(yPos, fp);             /* y1   - image top             */
    Putword(xPos+cols-1, fp);      /* x2   - image right           */
    Putword(yPos+rows-1, fp);      /* y2   - image bottom          */

    Putword(cols, fp);             /* horizontal resolution        */
    Putword(rows, fp);             /* vertical resolution          */

    /* Write out the Color Map for images with 16 colors or less */
    if (cmap16)
        for (i = 0; i < 16; ++i) {
            Putbyte(cmap16[i].r, fp);
            Putbyte(cmap16[i].g, fp);
            Putbyte(cmap16[i].b, fp);
        }
    else {
        unsigned int i;
        for (i = 0; i < 16; ++i) {
            Putbyte(0, fp);
            Putbyte(0, fp);
            Putbyte(0, fp);
        }
    }
    Putbyte(0, fp);                /* reserved byte                */
    Putbyte(Planes, fp);           /* number of color planes       */

    BytesPerLine = ((cols * BitsPerPixel) + 7) / 8;
    Putword(BytesPerLine, fp);    /* number of bytes per scanline */

    Putword(1, fp);                /* palette info                 */

    {
        unsigned int i;
        for (i = 0; i < 58; ++i)        /* fill to end of header        */
            Putbyte(0, fp);
    }
}



static void
PCXEncode(FILE *                const fp, 
          const unsigned char * const buf, 
          int                   const Size) {

    const unsigned char * const end = buf + Size;

    const unsigned char * currentP;
    int previous, count;

    currentP = buf;
    previous = *currentP++;
    count    = 1;

    while (currentP < end) {
        unsigned char const c = *currentP++;
        if (c == previous && count < 63)
            ++count;
        else {
            if (count > 1 || (previous & 0xc0) == 0xc0) {
                count |= 0xc0;
                Putbyte ( count , fp );
            }
            Putbyte(previous, fp);
            previous = c;
            count   = 1;
        }
    }

    if (count > 1 || (previous & 0xc0) == 0xc0) {
        count |= 0xc0;
        Putbyte ( count , fp );
    }
    Putbyte(previous, fp);
}



static unsigned int
indexOfColor(colorhash_table const cht,
             pixel           const color) {
/*----------------------------------------------------------------------------
   Return the index in the palette described by 'cht' of the color 'color'.

   Abort program with error message if the color is not in the palette.
-----------------------------------------------------------------------------*/

    int const rc = ppm_lookupcolor(cht, &color);
            
    if (rc < 0)
        pm_error("Image contains color which is not "
                 "in the palette: %u/%u/%u", 
                 PPM_GETR(color), PPM_GETG(color), PPM_GETB(color));

    return rc;
}



static void
ppmTo16ColorPcx(pixel **            const pixels, 
                int                 const cols, 
                int                 const rows, 
                struct pcxCmapEntry const pcxcmap[], 
                int                 const colors, 
                colorhash_table     const cht, 
                bool                const packbits,
                unsigned int        const planesRequested,
                unsigned int        const xPos,
                unsigned int        const yPos) {

    int Planes, BytesPerLine, BitsPerPixel;
    unsigned char *indexRow;  /* malloc'ed */
        /* indexRow[x] is the palette index of the pixel at column x of
           the row currently being processed
        */
    unsigned char *planesrow; /* malloc'ed */
        /* This is the input for a single row to the compressor */
    int row;

    if (packbits) {
        Planes = 1;
        if (colors > 4)        BitsPerPixel = 4;
        else if (colors > 2)   BitsPerPixel = 2;
        else                   BitsPerPixel = 1;
    } else {
        BitsPerPixel = 1;
        if (planesRequested)
            Planes = planesRequested;
        else {
            if (colors > 8)        Planes = 4;
            else if (colors > 4)   Planes = 3;
            else if (colors > 2)   Planes = 2;
            else                   Planes = 1;
        }
    }
    BytesPerLine = ((cols * BitsPerPixel) + 7) / 8;
    MALLOCARRAY_NOFAIL(indexRow, cols);
    MALLOCARRAY_NOFAIL(planesrow, BytesPerLine);

    write_header(stdout, cols, rows, BitsPerPixel, Planes, pcxcmap, 
                 xPos, yPos);
    for (row = 0; row < rows; ++row) {
        int col;
        for (col = 0; col < cols; ++col)
            indexRow[col] = indexOfColor(cht, pixels[row][col]);

        if (packbits) {
            PackBits(indexRow, cols, planesrow, BitsPerPixel);
            PCXEncode(stdout, planesrow, BytesPerLine);
        } else {
            unsigned int plane;
            for (plane = 0; plane < Planes; ++plane) {
                extractPlane(indexRow, cols, planesrow, plane);
                PCXEncode(stdout, planesrow, BytesPerLine);
            }
        }
    }
    free(planesrow);
    free(indexRow);
}



static void
ppmTo256ColorPcx(pixel **            const pixels, 
                 int                 const cols, 
                 int                 const rows, 
                 struct pcxCmapEntry const pcxcmap[], 
                 int                 const colors, 
                 colorhash_table     const cht,
                 unsigned int        const xPos, 
                 unsigned int        const yPos) {

    int row;
    unsigned int i;
    unsigned char *rawrow;

    rawrow = (unsigned char *)pm_allocrow(cols, sizeof(unsigned char));

    /* 8 bits per pixel, 1 plane */
    write_header(stdout, cols, rows, 8, 1, NULL, xPos, yPos);
    for (row = 0; row < rows; ++row) {
        int col;
        for (col = 0; col < cols; ++col)
            rawrow[col] = indexOfColor(cht, pixels[row][col]);
        PCXEncode(stdout, rawrow, cols);
    }
    Putbyte(PCX_256_COLORS, stdout);
    for (i = 0; i < MAXCOLORS; ++i) {
        Putbyte(pcxcmap[i].r, stdout);
        Putbyte(pcxcmap[i].g, stdout);
        Putbyte(pcxcmap[i].b, stdout);
    }
    pm_freerow((void*)rawrow);
}



static void
ppmToTruecolorPcx(pixel **     const pixels, 
                  int          const cols, 
                  int          const rows, 
                  pixval       const maxval,
                  unsigned int const xPos, 
                  unsigned int const yPos) {

    unsigned char *redrow, *greenrow, *bluerow;
    int col, row;

    redrow   = (unsigned char *)pm_allocrow(cols, sizeof(unsigned char));
    greenrow = (unsigned char *)pm_allocrow(cols, sizeof(unsigned char));
    bluerow  = (unsigned char *)pm_allocrow(cols, sizeof(unsigned char));

    /* 8 bits per pixel, 3 planes */
    write_header(stdout, cols, rows, 8, 3, NULL, xPos, yPos);
    for( row = 0; row < rows; row++ ) {
        register pixel *pP = pixels[row];
        for( col = 0; col < cols; col++, pP++ ) {
            if( maxval != PCX_MAXVAL ) {
                redrow[col]   = (long)PPM_GETR(*pP) * PCX_MAXVAL / maxval;
                greenrow[col] = (long)PPM_GETG(*pP) * PCX_MAXVAL / maxval;
                bluerow[col]  = (long)PPM_GETB(*pP) * PCX_MAXVAL / maxval;
            }
            else {
                redrow[col]   = PPM_GETR(*pP);
                greenrow[col] = PPM_GETG(*pP);
                bluerow[col]  = PPM_GETB(*pP);
            }
        }
        PCXEncode(stdout, redrow, cols);
        PCXEncode(stdout, greenrow, cols);
        PCXEncode(stdout, bluerow, cols);
    }
    pm_freerow((void*)bluerow);
    pm_freerow((void*)greenrow);
    pm_freerow((void*)redrow);
}



static const struct pcxCmapEntry 
stdPalette[] = {
    {   0,   0,   0 },
    {   0,   0, 170 },
    {   0, 170,   0 },
    {   0, 170, 170 },
    { 170,   0,   0 },
    { 170,   0, 170 },
    { 170, 170,   0 },
    { 170, 170, 170 },
    {  85,  85,  85 },
    {  85,  85, 255 },
    {  85, 255,  85 },
    {  85, 255, 255 },
    { 255,  85,  85 },
    { 255,  85, 255 },
    { 255, 255,  85 },
    { 255, 255, 255 }
};



static void
putPcxColorInHash(colorhash_table const cht,
                  pixel           const newPcxColor,
                  unsigned int    const newColorIndex,
                  pixval          const maxval) {

    pixel ppmColor;
        /* Same color as 'newPcxColor', but at the PPM image's color
           resolution: 'maxval'
        */
    int rc;

    PPM_DEPTH(ppmColor, newPcxColor, PCX_MAXVAL, maxval);
        
    rc = ppm_lookupcolor(cht, &ppmColor);

    if (rc == -1)
        /* This color is not in the hash yet, so we just add it */
        ppm_addtocolorhash(cht, &ppmColor, newColorIndex);
    else {
        /* This color is already in the hash.  That's because the
           subject image has less color resolution than PCX (i.e.
           'maxval' is less than PCX_MAXVAL), and two distinct
           colors in the standard palette are indistinguishable at
           subject image color resolution.
           
           So we have to figure out wether color 'newPcxColor' or
           'existingPcxColor' is a better match for 'ppmColor'.
        */

        unsigned int const existingColorIndex = rc;

        pixel idealPcxColor;
        pixel existingPcxColor;

        PPM_DEPTH(idealPcxColor, ppmColor, maxval, PCX_MAXVAL);
        
        PPM_ASSIGN(existingPcxColor, 
                   stdPalette[existingColorIndex].r,
                   stdPalette[existingColorIndex].g,
                   stdPalette[existingColorIndex].b);

        if (PPM_DISTANCE(newPcxColor, idealPcxColor) <
            PPM_DISTANCE(existingPcxColor, idealPcxColor)) {
            /* The new PCX color is a better match.  Make it the new
               translation of image color 'ppmColor'.
            */
            ppm_delfromcolorhash(cht, &ppmColor);
            ppm_addtocolorhash(cht, &ppmColor, newColorIndex);
        }
    }
}



static void
generateStandardPalette(struct pcxCmapEntry ** const pcxcmapP,
                        pixval                 const maxval,
                        colorhash_table *      const chtP,
                        int *                  const colorsP) {

    unsigned int const stdPaletteSize = 16;
    unsigned int colorIndex;
    struct pcxCmapEntry * pcxcmap;
    colorhash_table cht;

    MALLOCARRAY_NOFAIL(pcxcmap, MAXCOLORS);
    
    *pcxcmapP = pcxcmap;

    cht = ppm_alloccolorhash();

    for (colorIndex = 0; colorIndex < stdPaletteSize; ++colorIndex) {
        pixel pcxColor;
            /* The color of this colormap entry, in PCX resolution */

        pcxcmap[colorIndex] = stdPalette[colorIndex];
        
        PPM_ASSIGN(pcxColor, 
                   stdPalette[colorIndex].r,
                   stdPalette[colorIndex].g,
                   stdPalette[colorIndex].b);

        putPcxColorInHash(cht, pcxColor, colorIndex, maxval);
    }

    *chtP = cht;
    *colorsP = stdPaletteSize;
}
    


static void
readPpmPalette(const char *   const paletteFileName,
               pixel       (* const ppmPaletteP)[], 
               unsigned int * const paletteSizeP) {

    FILE * pfP;
    pixel ** pixels;
    int cols, rows;
    pixval maxval;
    
    pfP = pm_openr(paletteFileName);

    pixels = ppm_readppm(pfP, &cols, &rows, &maxval);

    pm_close(pfP);
    
    *paletteSizeP = rows * cols;
    if (*paletteSizeP > MAXCOLORS) 
        pm_error("ordered palette image contains %d pixels.  Maximum is %d",
                 *paletteSizeP, MAXCOLORS);

    {
        int j;
        int row;
        j = 0;  /* initial value */
        for (row = 0; row < rows; ++row) {
            int col;
            for (col = 0; col < cols; ++col) 
                (*ppmPaletteP)[j++] = pixels[row][col];
        }
    }
    ppm_freearray(pixels, rows);
}        



static void
readPaletteFromFile(struct pcxCmapEntry ** const pcxcmapP,
                    const char *           const paletteFileName,
                    pixval                 const maxval,
                    colorhash_table *      const chtP,
                    int *                  const colorsP) {

    unsigned int colorIndex;
    pixel ppmPalette[MAXCOLORS];
    unsigned int paletteSize;
    struct pcxCmapEntry * pcxcmap;
    colorhash_table cht;

    readPpmPalette(paletteFileName, &ppmPalette, &paletteSize);

    MALLOCARRAY_NOFAIL(pcxcmap, MAXCOLORS);
    
    *pcxcmapP = pcxcmap;

    cht = ppm_alloccolorhash();

    for (colorIndex = 0; colorIndex < paletteSize; ++colorIndex) {
        pixel pcxColor;
            /* The color of this colormap entry, in PCX resolution */

        pcxcmap[colorIndex] = pcxCmapEntryFromPixel(ppmPalette[colorIndex]);
        
        PPM_ASSIGN(pcxColor, 
                   ppmPalette[colorIndex].r,
                   ppmPalette[colorIndex].g,
                   ppmPalette[colorIndex].b);

        putPcxColorInHash(cht, pcxColor, colorIndex, maxval);
    }

    *chtP = cht;
    *colorsP = paletteSize;
}
    


static void
moveBlackToIndex0(colorhist_vector const chv,
                  int              const colors) {
/*----------------------------------------------------------------------------
   If black is in the palette, make it at Index 0.
-----------------------------------------------------------------------------*/
    pixel blackPixel;
    unsigned int i;
    bool blackPresent;

    PPM_ASSIGN(blackPixel, 0, 0, 0);

    blackPresent = FALSE;  /* initial assumption */

    for (i = 0; i < colors; ++i)
        if (PPM_EQUAL(chv[i].color, blackPixel))
            blackPresent = TRUE;
            
    if (blackPresent) {
        /* We use a trick here.  ppm_addtocolorhist() always adds to the
           beginning of the table and if the color is already elsewhere in
           the table, removes it.
        */
        int colors2;
        colors2 = colors;
        ppm_addtocolorhist(chv, &colors2, MAXCOLORS, &blackPixel, 0, 0);
        assert(colors2 == colors);
    }
}



static void
makePcxColormapFromImage(pixel **               const pixels,
                         int                    const cols,
                         int                    const rows,
                         pixval                 const maxval,
                         struct pcxCmapEntry ** const pcxcmapP,
                         colorhash_table *      const chtP,
                         int *                  const colorsP,
                         bool *                 const tooManyColorsP) {
/*----------------------------------------------------------------------------
   Make a colormap (palette) for the PCX header that can be used
   for the image described by 'pixels', 'cols', 'rows', and 'maxval'.

   Return it in newly malloc'ed storage and return its address as
   *pcxcmapP.

   Also return a lookup hash to relate a color in the image to the
   appropriate index in *pcxcmapP.  Return that in newly malloc'ed 
   storage as *chtP.

   Iff there are too many colors to do that (i.e. more than 256), 
   return *tooManyColorsP == TRUE.
-----------------------------------------------------------------------------*/
    int colors;
    colorhist_vector chv;

    pm_message("computing colormap...");

    chv = ppm_computecolorhist(pixels, cols, rows, MAXCOLORS, &colors);
    if (chv == NULL)
        *tooManyColorsP = TRUE;
    else {
        int i;
        struct pcxCmapEntry * pcxcmap;

        *tooManyColorsP = FALSE;

        pm_message("%d colors found", colors);
        
        moveBlackToIndex0(chv, colors);

        MALLOCARRAY_NOFAIL(pcxcmap, MAXCOLORS);

        *pcxcmapP = pcxcmap;

        for (i = 0; i < colors; ++i) {
            pixel p;

            PPM_DEPTH(p, chv[i].color, maxval, PCX_MAXVAL);

            pcxcmap[i].r = PPM_GETR(p);
            pcxcmap[i].g = PPM_GETG(p);
            pcxcmap[i].b = PPM_GETB(p);
        }

        /* Fill it out with black */
        for ( ; i < MAXCOLORS; ++i) {
            pcxcmap[i].r = 0;
            pcxcmap[i].g = 0;
            pcxcmap[i].b = 0;
        }

        *chtP = ppm_colorhisttocolorhash(chv, colors);

        *colorsP = colors;

        ppm_freecolorhist(chv);
    }
}



static void 
ppmToPalettePcx(pixel **            const pixels, 
                int                 const cols, 
                int                 const rows,
                pixval              const maxval,
                unsigned int        const xPos, 
                unsigned int        const yPos,
                struct pcxCmapEntry const pcxcmap[],
                colorhash_table     const cht,
                int                 const colors,
                bool                const packbits,
                unsigned int        const planes,
                bool                const use_8_bit) {
    
    /* convert image */
    if( colors <= 16 && !use_8_bit )
        ppmTo16ColorPcx(pixels, cols, rows, pcxcmap, colors, cht, 
                        packbits, planes, xPos, yPos);
    else
        ppmTo256ColorPcx(pixels, cols, rows, pcxcmap, colors, cht,
                         xPos, yPos);
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE* ifP;
    int rows, cols;
    pixval maxval;
    pixel **pixels;
    struct pcxCmapEntry * pcxcmap;
    colorhash_table cht;
    bool truecolor;
    int colors;

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);
    pixels = ppm_readppm(ifP, &cols, &rows, &maxval);
    pm_close(ifP);

    if (cmdline.truecolor)
        truecolor = TRUE;
    else {
        if (cmdline.stdpalette) {
            truecolor = FALSE;
            generateStandardPalette(&pcxcmap, maxval, &cht, &colors);
        } else if (cmdline.palette) {
            truecolor = FALSE;
            readPaletteFromFile(&pcxcmap, cmdline.palette, maxval, 
                                &cht, &colors);
        } else {
            bool tooManyColors;
            makePcxColormapFromImage(pixels, cols, rows, maxval,
                                     &pcxcmap, &cht, &colors,
                                     &tooManyColors);
            
            if (tooManyColors) {
                pm_message("too many colors - writing a 24bit PCX file");
                pm_message("if you want a non-24bit file, "
                           " a 'pnmquant %d'", MAXCOLORS);
                truecolor = TRUE;
            } else
                truecolor = FALSE;
        }
    }

    if (truecolor)
        ppmToTruecolorPcx(pixels, cols, rows, maxval, 
                          cmdline.xpos, cmdline.ypos);
    else {
        ppmToPalettePcx(pixels, cols, rows, maxval, 
                        cmdline.xpos, cmdline.ypos,
                        pcxcmap, cht, colors, cmdline.packed, 
                        cmdline.planes, cmdline.use_8_bit);
        
        ppm_freecolorhash(cht);
        free(pcxcmap);
    }
    return 0;
}
