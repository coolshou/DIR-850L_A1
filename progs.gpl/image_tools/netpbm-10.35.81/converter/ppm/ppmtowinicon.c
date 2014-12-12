/* ppmtowinicon.c - read portable pixmap file(s) and write a MS Windows .ico
**
** Copyright (C) 2000 by Lee Benfield - lee@benf.org
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <math.h>
#include <string.h>

#include "winico.h"
#include "ppm.h"
#include "mallocvar.h"
#include "shhopt.h"
#include "nstring.h"

#define MAJVERSION 0
#define MINVERSION 3

#define MAXCOLORS 256

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    unsigned int iconCount;
    const char **inputFilespec;  /* '-' if stdin; malloc'ed array */
    const char **andpgmFilespec;    /* NULL if unspecified; malloc'ed array */
    const char *output;     /* '-' if unspecified */
    unsigned int truetransparent;
    unsigned int verbose;
};


static bool verbose;

static int      file_offset = 0; /* not actually used, but useful for debug. */
static FILE *   ofp;

static void
parseCommandLine(int                 argc, 
                 char **             argv,
                 struct cmdlineInfo *cmdlineP ) {
/*----------------------------------------------------------------------------
   Parse program command line described in Unix standard form by argc
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

    unsigned int outputSpec, andpgms;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "output",     OPT_STRING, &cmdlineP->output,
            &outputSpec,                   0);
    OPTENT3(0, "andpgms",    OPT_FLAG,   NULL,
            &andpgms,                      0);
    OPTENT3(0, "truetransparent", OPT_FLAG,   NULL,
            &cmdlineP->truetransparent,    0);
    OPTENT3(0, "verbose",    OPT_STRING, NULL,
            &cmdlineP->verbose,            0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!outputSpec)
        cmdlineP->output = "-";


    if (!andpgms) {
        if (argc-1 == 0) {
            cmdlineP->iconCount = 1;
            MALLOCARRAY_NOFAIL(cmdlineP->inputFilespec, cmdlineP->iconCount);
            cmdlineP->inputFilespec[0] = "-";
        } else {
            unsigned int iconIndex;

            cmdlineP->iconCount = argc-1;
            MALLOCARRAY_NOFAIL(cmdlineP->inputFilespec, cmdlineP->iconCount);
            for (iconIndex = 0; iconIndex < cmdlineP->iconCount; ++iconIndex)
                cmdlineP->inputFilespec[iconIndex] = argv[iconIndex+1];
        }
        {
            unsigned int iconIndex;
            MALLOCARRAY_NOFAIL(cmdlineP->andpgmFilespec, cmdlineP->iconCount);
            for (iconIndex = 0; iconIndex < cmdlineP->iconCount; ++iconIndex)
                cmdlineP->andpgmFilespec[iconIndex] = NULL;
        }
    } else {
        if (argc-1 < 2)
            pm_error("with -andpgms, you must specify at least two "
                     "arguments: image file name and and mask file name.  "
                     "You specified %d", argc-1);
        else if ((argc-1)/2*2 != (argc-1))
            pm_error("with -andpgms, you must specify an even number of "
                     "arguments.  You specified %d", argc-1);
        else {
            unsigned int iconIndex;
            cmdlineP->iconCount = (argc-1)/2;
            MALLOCARRAY_NOFAIL(cmdlineP->inputFilespec, cmdlineP->iconCount);
            MALLOCARRAY_NOFAIL(cmdlineP->andpgmFilespec, cmdlineP->iconCount);
            for (iconIndex = 0; iconIndex < cmdlineP->iconCount; ++iconIndex) {
                cmdlineP->inputFilespec[iconIndex] = argv[1 + iconIndex*2];
                cmdlineP->andpgmFilespec[iconIndex] = argv[2 + iconIndex*2];
            }
        }
    }

}



static void 
PutByte(int const v) {
   
   if (putc(v, ofp) == EOF)
       pm_error("Unable to write byte to output file.");
}
   


static void 
PutShort(short const v) {
   
   if (pm_writelittleshort(ofp, v) == -1)
       pm_error("Unable to write short integer to output file");
}
   


static void 
PutLong(long const v) {
   
   if (pm_writelittlelong(ofp, v) == -1)
       pm_error("Unable to write long integer to output file");
}


   
/*
 * These have no purpose but to wrapper the Byte, Short & Long 
 * functions.
 */
static void 
writeU1 (u1 const v) {
   file_offset++;
   PutByte(v);
}

static  void 
writeU2 (u2 const v) {
   file_offset +=2;
   PutShort(v);
}

static void 
writeU4 (u4 const v) {
   file_offset += 4;
   PutLong(v);
}

static MS_Ico 
createIconFile (void) {
   MS_Ico MSIconData;
   
   MALLOCVAR_NOFAIL(MSIconData);

   MSIconData->reserved     = 0;
   MSIconData->type         = 1;
   MSIconData->count        = 0;
   MSIconData->entries      = NULL;
   return MSIconData;
}


/* create andBitmap from pgm */

static ICON_bmp 
createAndBitmap (gray ** const ba, int const cols, int const rows,
                 gray const maxval) {
   /*
    * How wide should the u1 string for each row be?
    * each byte is 8 pixels, but must be a multiple of 4 bytes.
    */
   ICON_bmp icBitmap;
   int xBytes,y,x;
   int wt = cols;
   u1 ** rowData;

   MALLOCVAR_NOFAIL(icBitmap);

   wt >>= 3;
   if (wt & 3) {
      wt = (wt & ~3) + 4;
   }
   xBytes = wt;
   MALLOCARRAY_NOFAIL(rowData, rows);
   icBitmap->xBytes = xBytes;
   icBitmap->data   = rowData;
   icBitmap->size   = xBytes * rows;
   for (y=0;y<rows;y++) {
      u1 * row;
      int byteOn = 0;
      int bitOn = 128;

      MALLOCARRAY_NOFAIL(row, xBytes);

      memset (row, 0, xBytes);
      rowData[rows-y-1] = row;
      /* 
       * Check there's a bit array, otherwise we're just faking this...
       */
      if (ba) {
     for (x=0;x<cols;x++) {
            /* Black (bit clear) is transparent in PGM alpha maps,
             * in ICO bit *set* is transparent.
             */
            if (ba[y][x] <= maxval/2) row[byteOn] |= bitOn;

        if (bitOn == 1) {
           byteOn++;
           bitOn = 128;
        } else {
           bitOn >>= 1;
        }
     }
      }
   }
   return icBitmap;
}


/*
 * Depending on if the image is stored as 1bpp, 4bpp or 8bpp, the 
 * encoding mechanism is different.
 * 
 * I didn't re-use the code from ppmtobmp since I need to keep the
 * bitmaps in memory till I've loaded all ppms.
 * 
 * 8bpp => 1 byte/palette index.
 * 4bpp => High Nibble, Low Nibble
 * 1bpp => 1 palette value per bit, high bit 1st.
 */
static ICON_bmp 
create1Bitmap (pixel ** const pa, int const cols, int const rows, 
               colorhash_table const cht) {
   /*
    * How wide should the u1 string for each row be?
    * each byte is 8 pixels, but must be a multiple of 4 bytes.
    */
   ICON_bmp icBitmap;
   int xBytes,y,x;
   int wt = cols;
   u1 ** rowData;

   MALLOCVAR_NOFAIL(icBitmap);
   
   wt >>= 3;
   if (wt & 3) {
      wt = (wt & ~3) + 4;
   }
   xBytes = wt;
   MALLOCARRAY_NOFAIL(rowData, rows);
   icBitmap->xBytes = xBytes;
   icBitmap->data   = rowData;
   icBitmap->size   = xBytes * rows;
   for (y=0;y<rows;y++) {
      u1 * row;
      int byteOn = 0;
      int bitOn = 128;
      int value;
      
      MALLOCARRAY_NOFAIL(row, xBytes);
      memset (row, 0, xBytes);
      rowData[rows-y-1] = row;
      /* 
       * Check there's a pixel array, otherwise we're just faking this...
       */
      if (pa) {
     for (x=0;x<cols;x++) {
        /*
         * So we've got a colorhash_table with two colors in it.
         * Which is black?!
         * 
         * Unless the hashing function changes, 0's black.
         */
        value = ppm_lookupcolor(cht, &pa[y][x]);
        if (!value) {
           /* leave black. */
        } else {
           row[byteOn] |= bitOn;
        }
        if (bitOn == 1) {
           byteOn++;
           bitOn = 128;
        } else {
           bitOn >>= 1;
        }
     }
      }
   }
   return icBitmap;
}


static ICON_bmp 
create4Bitmap (pixel ** const pa, int const cols, int const rows,
               colorhash_table const cht) {
   /*
    * How wide should the u1 string for each row be?
    * each byte is 8 pixels, but must be a multiple of 4 bytes.
    */
   ICON_bmp icBitmap;
   int xBytes,y,x;
   int wt = cols;
   u1 ** rowData;

   MALLOCVAR_NOFAIL(icBitmap);

   wt >>= 1;
   if (wt & 3) {
      wt = (wt & ~3) + 4;
   }
   xBytes = wt;
   MALLOCARRAY_NOFAIL(rowData, rows);
   icBitmap->xBytes = xBytes;
   icBitmap->data   = rowData;
   icBitmap->size   = xBytes * rows;

   for (y=0;y<rows;y++) {
      u1 * row;
      int byteOn = 0;
      int nibble = 1;   /* high nibble = 1, low nibble = 0; */
      int value;

      MALLOCARRAY_NOFAIL(row, xBytes);

      memset (row, 0, xBytes);
      rowData[rows-y-1] = row;
      /* 
       * Check there's a pixel array, otherwise we're just faking this...
       */
      if (pa) {
     for (x=0;x<cols;x++) {
        value = ppm_lookupcolor(cht, &pa[y][x]);
        /*
         * Shift it, if we're putting it in the high nibble.
         */
        if (nibble) {
           value <<= 4;
        }
        row[byteOn] |= value;
        if (nibble) {
           nibble = 0;
        } else {
           nibble = 1;
           byteOn++;
        }
     }
      }
   }
   return icBitmap;
}



static ICON_bmp 
create8Bitmap (pixel ** const pa, int const cols, int const rows,
               colorhash_table const cht) {
   /*
    * How wide should the u1 string for each row be?
    * each byte is 8 pixels, but must be a multiple of 4 bytes.
    */
   ICON_bmp icBitmap;
   int xBytes,y,x;
   int wt = cols;
   u1 ** rowData;

   MALLOCVAR_NOFAIL(icBitmap);

   if (wt & 3) {
      wt = (wt & ~3) + 4;
   }
   xBytes = wt;
   MALLOCARRAY_NOFAIL(rowData, rows);
   icBitmap->xBytes = xBytes;
   icBitmap->data   = rowData;
   icBitmap->size   = xBytes * rows;

   for (y=0;y<rows;y++) {
      u1 * row;

      MALLOCARRAY_NOFAIL(row, xBytes);
      memset (row, 0, xBytes);
      rowData[rows-y-1] = row;
      /* 
       * Check there's a pixel array, otherwise we're just faking this...
       */
      if (pa) {
     for (x=0;x<cols;x++) {
        row[x] = ppm_lookupcolor(cht, &pa[y][x]);
     }
      }
   }
   return icBitmap;
}



static IC_InfoHeader 
createInfoHeader(IC_Entry const entry, ICON_bmp const xbmp,
                 ICON_bmp const abmp) {
   IC_InfoHeader ih;
   
   MALLOCVAR_NOFAIL(ih);

   ih->size          = 40;
   ih->width         = entry->width;
   ih->height        = entry->height * 2;  
   ih->planes        = 1;  
   ih->bitcount      = entry->bitcount;
   ih->compression   = 0;
   ih->imagesize     = entry->width * entry->height * 8 / entry->bitcount;
   ih->x_pixels_per_m= 0;
   ih->y_pixels_per_m= 0;
   ih->colors_used   = 1 << entry->bitcount;
   ih->colors_important = 0;
   return ih;
}



static IC_Palette 
createCleanPalette(void) {
   IC_Palette palette;
   int x;
   
   MALLOCVAR_NOFAIL(palette);

   MALLOCARRAY_NOFAIL(palette->colors, MAXCOLORS);
   for (x=0;x<MAXCOLORS;x++ ){
      palette->colors[x] = NULL;
   }
   return palette;
}



static void 
addColorToPalette(IC_Palette const palette, int const i,
                  int const r, int const g, int const b) {

    MALLOCVAR_NOFAIL(palette->colors[i]);

    palette->colors[i]->red      = r;
    palette->colors[i]->green    = g;
    palette->colors[i]->blue     = b;
    palette->colors[i]->reserved = 0;
}



static ICON_bmp 
createBitmap (int const bpp, pixel ** const pa, 
              int const cols, int const rows, 
              colorhash_table const cht) {
    
    ICON_bmp retval;
    const int assumed_bpp = (pa == NULL) ? 1 : bpp;

    switch (assumed_bpp) {
    case 1:
        retval = create1Bitmap (pa,cols,rows,cht);
        break;
    case 4:
        retval = create4Bitmap (pa,cols,rows,cht);
        break;
    case 8:
    default:
        retval = create8Bitmap (pa,cols,rows,cht);
        break;
    }
    return retval;
}



static void
makePalette(pixel **          const xorPPMarray,
            int               const xorCols,
            int               const xorRows,
            pixval            const xorMaxval,
            IC_Palette *      const paletteP,
            colorhash_table * const xorChtP,
            int *             const colorsP,
            const char **     const errorP) {
   /*
    * Figure out the colormap and turn it into the appropriate GIF
    * colormap - this code's pretty much straight from ppmtobpm
    */
    colorhist_vector xorChv;
    unsigned int i;
    int colors;
    IC_Palette palette = createCleanPalette();

    if (verbose) pm_message("computing colormap...");
    xorChv = ppm_computecolorhist(xorPPMarray, xorCols, xorRows, MAXCOLORS, 
                                  &colors);
    if (xorChv == NULL)
        asprintfN(errorP,
                  "image has too many colors - try doing a 'pnmquant %d'",
                  MAXCOLORS);
    else {
        *errorP = NULL;

        if (verbose) pm_message("%d colors found", colors);
        
        if (verbose && (xorMaxval > 255))
            pm_message("maxval is not 255 - automatically rescaling colors");
        for (i = 0; i < colors; ++i) {
            if (xorMaxval == 255) {
                addColorToPalette(palette,i,
                                  PPM_GETR(xorChv[i].color),
                                  PPM_GETG(xorChv[i].color),
                                  PPM_GETB(xorChv[i].color));
            } else {
                addColorToPalette(palette,i,
                                  PPM_GETR(xorChv[i].color) * 255 / xorMaxval,
                                  PPM_GETG(xorChv[i].color) * 255 / xorMaxval,
                                  PPM_GETB(xorChv[i].color) * 255 / xorMaxval);
            }
        }
        
        /* And make a hash table for fast lookup. */
        *xorChtP = ppm_colorhisttocolorhash(xorChv, colors);
        
        ppm_freecolorhist(xorChv);
        
        *paletteP = palette;
        *colorsP = colors;
    }
}



static void
getOrFakeAndMap(const char *      const andPgmFname,
                int               const xorCols,
                int               const xorRows,
                gray ***          const andPGMarrayP,
                pixval *          const andMaxvalP,
                colorhash_table * const andChtP,
                const char **     const errorP) {

    int andRows, andCols;
    
    if (!andPgmFname) {
        /* He's not supplying a bitmap for 'and'.  Fake the bitmap. */
        *andPGMarrayP = NULL;
        *andMaxvalP   = 1;
        *andChtP      = NULL;
        *errorP       = NULL;
    } else {
        FILE * andfile;
        andfile = pm_openr(andPgmFname);
        *andPGMarrayP = pgm_readpgm(andfile, &andCols, &andRows, andMaxvalP);
        pm_close(andfile);

        if ((andCols != xorCols) || (andRows != xorRows)) {
            asprintfN(errorP,
                      "And mask and image have different dimensions "
                     "(%d x %d vs %d x %d).  Aborting.",
                     andCols, xorCols, andRows, xorRows);
        } else
            *errorP = NULL;
    }
}



static void
blackenTransparentAreas(pixel ** const xorPPMarray,
                        int      const cols,
                        int      const rows,
                        gray **  const andPGMarray,
                        pixval   const andMaxval) {

    unsigned int row;

    if (verbose) pm_message("Setting transparent pixels to black");

    for (row = 0; row < rows; ++row) {
        unsigned int col;
        for (col = 0; col < cols; ++col) {
            if (andPGMarray[row][col] < andMaxval)
                /* It's not opaque here; make it black */
                PPM_ASSIGN(xorPPMarray[row][col], 0, 0, 0);
        }
    }
}



static void 
addEntryToIcon(MS_Ico       const MSIconData, 
               const char * const xorPpmFname,
               const char * const andPgmFname,
               bool         const trueTransparent) {

    IC_Entry entry;
    FILE * xorfile;
    pixel ** xorPPMarray;
    gray ** andPGMarray;
    ICON_bmp xorBitmap;
    ICON_bmp andBitmap;
    int rows, cols;
    int bpp, colors;
    int entry_cols;
    IC_Palette palette;
    colorhash_table  xorCht;
    colorhash_table  andCht; 
    const char * error;
   
    pixval xorMaxval;
    gray andMaxval;

    MALLOCVAR_NOFAIL(entry);

   /*
    * Read the xor PPM.
    */
    xorfile = pm_openr(xorPpmFname);
    xorPPMarray = ppm_readppm(xorfile, &cols, &rows, &xorMaxval);
    pm_close(xorfile);
    /*
    * Since the entry uses 1 byte to hold the width and height of the icon, the
    * image can't be more than 256 x 256.
    */
    if (rows > 255 || cols > 255) {
        pm_error("Max size for a icon is 255 x 255 (1 byte fields).  "
                 "%s is %d x %d", xorPpmFname, cols, rows);
    }
   
    if (verbose) pm_message("read PPM: %dw x %dh, maxval = %d", 
                            cols, rows, xorMaxval);

    makePalette(xorPPMarray, cols, rows, xorMaxval, 
                &palette, &xorCht, &colors, &error);

    if (error)
        pm_error("Unable to make palette for '%s'.  %s", xorPpmFname, error);
   /*
    * All the icons I found seemed to pad the palette to the max entries
    * for that bitdepth.
    * 
    * The spec indicates this isn't neccessary, but I'll follow this behaviour
    * just in case.
    */
    if (colors < 3) {
        bpp = 1;
        entry_cols = 2;
    } else if (colors < 17) {
        bpp = 4;
        entry_cols = 16;
    } else {
        bpp = 8;
        entry_cols = 256;
    }

    getOrFakeAndMap(andPgmFname, cols, rows,
                    &andPGMarray, &andMaxval, &andCht, &error);
    if (error)
        pm_error("Error in and map for '%s'.  %s", xorPpmFname, error);

    if (andPGMarray && trueTransparent)
        blackenTransparentAreas(xorPPMarray, cols, rows, 
                                andPGMarray, andMaxval);

    xorBitmap = createBitmap(bpp, xorPPMarray, cols, rows, xorCht);
    andBitmap = createAndBitmap(andPGMarray, cols, rows, andMaxval);
    /*
     * Fill in the entry data fields.
    */
    entry->width         = cols;
    entry->height        = rows;
    entry->color_count   = entry_cols;
    entry->reserved      = 0;
    entry->planes        = 1;
    /* 
    * all the icons I looked at ignored this value...
    */
    entry->bitcount      = bpp;
    entry->ih            = createInfoHeader(entry, xorBitmap, andBitmap);
    entry->colors        = palette->colors;
    entry->size_in_bytes = 
        xorBitmap->size + andBitmap->size + 40 + (4 * entry->color_count);
    if (verbose) 
        pm_message("entry->size_in_bytes = %d + %d + %d = %d",
                   xorBitmap->size ,andBitmap->size, 
                   40, entry->size_in_bytes );
    /*
    * We don't know the offset ATM, set to 0 for now.
    * Have to calculate this at the end.
    */
    entry->file_offset   = 0;
    entry->xorBitmapOut  = xorBitmap->data;
    entry->andBitmapOut  = andBitmap->data;
    entry->xBytesXor     = xorBitmap->xBytes;
    entry->xBytesAnd     = andBitmap->xBytes;  
    /*
    * Add the entry to the entries array.
    */
    ++MSIconData->count;
    /* Perhaps I should allocate ahead, and take fewer trips to the well. */
    REALLOCARRAY(MSIconData->entries, MSIconData->count);
    MSIconData->entries[MSIconData->count-1] = entry;
}



static void 
writeIC_Entry (IC_Entry const entry) {
   writeU1(entry->width);
   writeU1(entry->height);
   writeU1(entry->color_count); /* chops 256->0 on its own.. */
   writeU1(entry->reserved);
   writeU2(entry->planes);
   writeU2(entry->bitcount);
   writeU4(entry->size_in_bytes);
   writeU4(entry->file_offset);
}



static void 
writeIC_InfoHeader (IC_InfoHeader const ih) {
   writeU4(ih->size);
   writeU4(ih->width);
   writeU4(ih->height);
   writeU2(ih->planes);
   writeU2(ih->bitcount);
   writeU4(ih->compression);
   writeU4(ih->imagesize);
   writeU4(ih->x_pixels_per_m);
   writeU4(ih->y_pixels_per_m);
   writeU4(ih->colors_used);
   writeU4(ih->colors_important);
}



static void 
writeIC_Color (IC_Color const col) {
   /* Since the ppm might not have as many colors in it as we'd like,
    * (2, 16, 256), stick 0 in the gaps.
    * 
    * This means that we lose palette information, but that can't be
    * helped.  
    */
   if (col == NULL) {
      writeU1(0);
      writeU1(0);
      writeU1(0);
      writeU1(0);
   } else {
      writeU1(col->blue);
      writeU1(col->green);
      writeU1(col->red);
      writeU1(col->reserved);
   }
}



static void
writeBitmap(u1 ** const bitmap, int const xBytes, int const height) {
   int y;
   for (y = 0;y<height;y++) {
      fwrite (bitmap[y],1,xBytes,ofp);
      file_offset += xBytes;
   }
}



static void 
writeMS_Ico(MS_Ico       const MSIconData, 
            const char * const outFname) {
    int x,y;
   
    ofp = pm_openw(outFname);

    writeU2(MSIconData->reserved);
    writeU2(MSIconData->type);
    writeU2(MSIconData->count);
    for (x=0;x<MSIconData->count;x++) writeIC_Entry(MSIconData->entries[x]);
    for (x=0;x<MSIconData->count;x++) {
        writeIC_InfoHeader(MSIconData->entries[x]->ih);
        for (y=0;y<(MSIconData->entries[x]->color_count);y++) {
            writeIC_Color(MSIconData->entries[x]->colors[y]);
        }
        if (verbose) pm_message("writing xor bitmap");
        writeBitmap(MSIconData->entries[x]->xorBitmapOut,
                    MSIconData->entries[x]->xBytesXor,
                    MSIconData->entries[x]->height);
        if (verbose) pm_message("writing and bitmap");
        writeBitmap(MSIconData->entries[x]->andBitmapOut,
                    MSIconData->entries[x]->xBytesAnd,
                    MSIconData->entries[x]->height);
    }
    fclose(ofp);
}



int 
main(int argc, char ** argv) {

    struct cmdlineInfo cmdline;

    MS_Ico const MSIconDataP = createIconFile();
    unsigned int iconIndex;
    unsigned int offset;
   
    ppm_init (&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    verbose = cmdline.verbose;

    for (iconIndex = 0; iconIndex < cmdline.iconCount; ++iconIndex) {
        addEntryToIcon(MSIconDataP, cmdline.inputFilespec[iconIndex],
                       cmdline.andpgmFilespec[iconIndex], 
                       cmdline.truetransparent);
    }
    /*
     * Now we have to go through and calculate the offsets.
     * The first infoheader starts at 6 + count*16 bytes.
     */
    offset = (MSIconDataP->count * 16) + 6;
    for (iconIndex = 0; iconIndex < MSIconDataP->count; ++iconIndex) {
        IC_Entry entry = MSIconDataP->entries[iconIndex];
        entry->file_offset = offset;
        /* 
         * Increase the offset by the size of this offset & data.
         * this includes the size of the color data.
         */
        offset += entry->size_in_bytes;
    }
    /*
     * And now, we have to actually SAVE the .ico!
     */
    writeMS_Ico(MSIconDataP, cmdline.output);

    free(cmdline.inputFilespec);
    free(cmdline.andpgmFilespec);

    return 0;
}

