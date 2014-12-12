/* winicontoppm.c - read a MS Windows .ico file and write portable pixmap(s)
**
** Copyright (C) 2000,2003 by Lee Benfield - lee@benf.org
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Changes:
** 
** 03/2003 - Added 24+32 bpp capability.
*/

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <math.h>
#include <string.h>
#include <assert.h>

#include "ppm.h"
#include "shhopt.h"
#include "nstring.h"
#include "mallocvar.h"
#include "winico.h"

#define MAJVERSION 0
#define MINVERSION 4

static int file_offset = 0;    /* not actually used, but useful for debug */
static const char     er_read[] = "%s: read error";
static const char *   infname;
static FILE *   ifp;

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilespec;
    const char * outputFilespec;
    unsigned int allicons;
    unsigned int bestqual;
    unsigned int writeands;
    unsigned int multippm;
    unsigned int verbose;
};




static void
parseCommandLine ( int argc, char ** argv,
                   struct cmdlineInfo *cmdlineP ) {
/*----------------------------------------------------------------------------
   parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "allicons",     OPT_FLAG,   NULL,                  
            &cmdlineP->allicons,       0 );
    OPTENT3(0, "bestqual",     OPT_FLAG,   NULL,                  
            &cmdlineP->bestqual,       0 );
    OPTENT3(0, "writeands",    OPT_FLAG,   NULL,                  
            &cmdlineP->writeands,      0 );
    OPTENT3(0, "multippm",     OPT_FLAG,   NULL,                  
            &cmdlineP->multippm,       0 );
    OPTENT3(0, "verbose",      OPT_FLAG,   NULL,                  
            &cmdlineP->verbose,        0 );

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (argc-1 < 1) 
        cmdlineP->inputFilespec = "-";
    else
        cmdlineP->inputFilespec = argv[1];

    if (argc-1 < 2) {
        cmdlineP->outputFilespec = "-";
        
        if (cmdlineP->writeands || cmdlineP->allicons)
            pm_error("If you specify the -writeands or -allicons option, "
                     "you must also specify an output file name argument.");
    } else
        cmdlineP->outputFilespec = argv[2];

    if (argc-1 > 2)
        pm_error("Too many arguments (%d).  Input filespec and "
                 "output filespec are the only possible arguments.",
                 argc-1);
}




static int 
GetByte(void) {
    int v;
   
    if ((v = getc(ifp)) == EOF)
    {
        pm_error(er_read, infname);
    }
   
    return v;
}
   
static short 
GetShort(void) {
    short v;
   
    if (pm_readlittleshort(ifp, &v) == -1)
    {
        pm_error(er_read, infname);
    }
   
    return v;
}
   
static long 
GetLong(void) {
    long v;
   
    if (pm_readlittlelong(ifp, &v) == -1)
    {
        pm_error(er_read, infname);
    }
   
    return v;
}
   


/*
 * These have no purpose but to wrapper the Byte, Short & Long 
 * functions.
 */
static u1 
readU1 (void) {
    file_offset++;
    return GetByte();
}

static u1 * 
readU1String (int length)
{
   
    u1 * string;
    
    MALLOCARRAY(string, length + 1);
    if (string == NULL)
        pm_error("out of memory");

    fread(string,sizeof(u1),length,ifp);
    string[length] = 0;
    file_offset += length * sizeof(u1);
    return string;
}

static u2 
readU2 (void) {
    file_offset +=2;
    return GetShort();
}

static u4 
readU4 (void) {
    file_offset += 4;
    return GetLong();
}

static IC_Entry 
readICEntry (void) 
{
    IC_Entry entry;

    MALLOCVAR(entry);

    if (entry == NULL)
        pm_error("Unable to allcoate memory for IC entry");

    entry->width         = readU1();
    entry->height        = readU1();
    entry->color_count   = readU1();
    entry->reserved      = readU1();
    entry->planes        = readU2();
    entry->bitcount      = readU2();
    entry->size_in_bytes = readU4();
    entry->file_offset   = readU4();
    entry->colors        = NULL;
    entry->ih            = NULL;
    entry->xorBitmap     = NULL;
    entry->andBitmap     = NULL;
    
    return entry;
}



static IC_InfoHeader 
readInfoHeader (IC_Entry entry) 
{
    IC_InfoHeader ih;

    MALLOCVAR(ih);
    
    if (ih == NULL)
        pm_error("Unable to allocate memory for info header");

    ih->size            = readU4();
    ih->width           = readU4();
    ih->height          = readU4();
    ih->planes          = readU2();
    ih->bitcount        = readU2();
    ih->compression     = readU4();
    ih->imagesize       = readU4();
    ih->x_pixels_per_m  = readU4();
    ih->y_pixels_per_m  = readU4();
    ih->colors_used     = readU4();
    ih->colors_important = readU4();
    
    if (!entry->bitcount) entry->bitcount = ih->bitcount;
    if (entry->color_count == 0 && 
        entry->bitcount <= 8) entry->color_count = 256;
    if (ih->compression) {
        pm_error("Can't handle compressed icons");
    }
    return ih;
}

/*
 * I don't know why this isn't the same as the spec, it just <b>isn't</b>
 * The colors honestly seem to be stored BGR.  Bizarre.
 * 
 * I've checked this in the BMP code for bmptoppm and the gimp.  Guess the
 * spec I have is just plain wrong.
 */
static IC_Color 
readICColor (void) 
{
    IC_Color col;

    MALLOCVAR(col);

    if (col == NULL)
        pm_error("Unable to allocate memory for color");

    col->blue     = readU1();
    col->green    = readU1();
    col->red      = readU1();
    col->reserved = readU1();
    return col;
}
   


/*
 * Depending on if the image is stored as 1bpp, 4bpp or 8bpp, the 
 * encoding mechanism is different.
 * 
 * 8bpp => 1 byte/palette index.
 * 4bpp => High Nibble, Low Nibble
 * 1bpp => 1 palette value per bit, high bit 1st.
 */
static u1 * 
read1Bitmap (int width, int height) 
{
    int tmp;
    int xBytes;
    u1 * bitmap;
    int wt = width;

    MALLOCARRAY(bitmap, width * height);
    if (bitmap == NULL)
        pm_error("out of memory");

    wt >>= 3;
    if (wt & 3) {
        wt = (wt & ~3) + 4;
    }
    xBytes = wt;
    for (tmp = 0; tmp<height; tmp++ ) {
        int x;
        int rowByte = 0;
        int xOrVal = 128;
        u1 * row = readU1String(xBytes);
        for (x = 0; x< width; x++) {
            *(bitmap+((height-tmp-1)*width) + (x)) = 
                (row[rowByte] & xOrVal) / xOrVal;
            if (xOrVal == 1) {
                xOrVal = 128;
                rowByte++;
            } else {
                xOrVal >>= 1;
            }
        }
        free(row);
    }
    return bitmap;
}


   
static u1 * 
read4Bitmap (int width, int height) 
{
    int tmp;
    u1 * bitmap;

    int wt = width;
    int xBytes;

    MALLOCARRAY(bitmap, width * height);
    if (bitmap == NULL)
        pm_error("out of memory");


    wt >>= 1;
    if (wt & 3) {
        wt = (wt & ~3) + 4;
    }
    xBytes = wt;
    for (tmp = 0; tmp<height ; tmp++ ) {
        int rowByte = 0;
        int bottom = 1;
        int x;
        u1 * row = readU1String(xBytes);
        for (x = 0; x< width; x++) {
            /*
             * 2 nibbles, 2 values.
             */
            if (bottom) {
                *(bitmap+((height-tmp-1)*width) + (x)) = 
                    (row[rowByte] & 0xF0) >> 4;
            } else {
                *(bitmap+((height-tmp-1)*width) + (x)) = (row[rowByte] & 0xF);
                rowByte++;
            }
            bottom = !bottom;
        }
    free(row);
    }
    return bitmap;
}


   
static u1 * 
read8Bitmap (int width, int height) 
{
    int tmp;
    unsigned int xBytes;
    unsigned int wt = width;
    u1 * bitmap;
   
    MALLOCARRAY(bitmap, width * height);
    if (bitmap == NULL)
        pm_error("out of memory");

    if (wt & 3) {
        wt = (wt & ~3) + 4;
    }
    xBytes = wt;
    for (tmp = 0; tmp<height ; tmp++ ) {
        int rowByte = 0;
        int x;
        u1 * row = readU1String(xBytes);
        for ( x = 0; x< width; x++) {
            *(bitmap+((height-tmp-1)*width) + (x)) = row[rowByte];
            rowByte++;
        }
        free(row);
    }
    return bitmap;
}



/*
 * Read a true color bitmap. (24/32 bits)
 * 
 * The output routine deplanarizes it for us, we keep it flat here.
 */
static u1 *
readXBitmap (int const width, 
             int const height, 
             int const bpp) {
    int          const bytes  = bpp >> 3;
    unsigned int const xBytes = width * bytes;

    u1 * bitmap;
        /* remember - bmp (dib) stored upside down, so reverse */

    MALLOCARRAY(bitmap, bytes * width * height);
    if (bitmap == NULL)
        pm_error("out of memory allocating bitmap array");

    {
        unsigned int i;
        u1 * bitcurptr;

        for (i = 0, bitcurptr = &bitmap[bytes * width * (height-1)];
             i < height; 
             ++i, bitcurptr -= xBytes) {

            u1 * const row = readU1String(xBytes);
            memcpy(bitcurptr, row, xBytes);
            free(row);
        }
    }
    return bitmap;
}



static MS_Ico 
readIconFile (bool const verbose) {
    int iter,iter2;

    MS_Ico MSIconData;

    MALLOCVAR(MSIconData);
   
    /*
     * reserved - should equal 0.
     */
    MSIconData->reserved = readU2();
    /*
     * Type - should equal 1
     */
    MSIconData->type     = readU2();
    /*
     * count - no of icons in file..
     */
    MSIconData->count    = readU2();
    /*
     * Allocate "count" array of entries.
     */
    if (verbose) 
        pm_message("Icon file contains %d icons.", MSIconData->count);

    MALLOCARRAY(MSIconData->entries, MSIconData->count);
    if (MSIconData->entries == NULL)
        pm_error("out of memory");
    /*
     * Read in each of the entries
     */
    for (iter = 0;iter < MSIconData->count ; iter++ ) {
        MSIconData->entries[iter] = readICEntry();
    }
    /* After that, we have to read in the infoheader, color map (if
     * any) and the actual bit/pix maps for the icons.  
     */
    if (verbose) 
        fprintf (stderr,"#\tColors\tBPP\tWidth\tHeight\n");
    for (iter = 0;iter < MSIconData->count ; iter++ ) {
        int bpp;
        MSIconData->entries[iter]->ih =
            readInfoHeader (MSIconData->entries[iter]);
       
        /* What's the bits per pixel? */
        bpp = MSIconData->entries[iter]->bitcount; 
        /* Read the palette, if appropriate */
        switch (bpp) {
        case 24:
        case 32:
            /* 24/32 bpp icon has no palette */
            break;
        default:
            MALLOCARRAY(MSIconData->entries[iter]->colors, 
                        MSIconData->entries[iter]->color_count);
            if (MSIconData->entries[iter]->colors == NULL)
                pm_error("out of memory");

            for (iter2 = 0;
                 iter2 < MSIconData->entries[iter]->color_count ; 
                 iter2++ ) {
                MSIconData->entries[iter]->colors[iter2] = readICColor();
            }
            break;
        }
        if (verbose) {
            char cols_text[10];
            sprintf (cols_text, "%d", MSIconData->entries[iter]->color_count);
            fprintf (stderr,
                     "%d\t%s\t%d\t%d\t%d\n", iter,
                     MSIconData->entries[iter]->color_count ? 
                     cols_text : "TRUE",
                     bpp, MSIconData->entries[iter]->width, 
                     MSIconData->entries[iter]->height);
        }
        /* Pixels are stored bottom-up, left-to-right. Pixel lines are
         * padded with zeros to end on a 32bit (4byte) boundary. Every
         * line will have the same number of bytes. Color indices are
         * zero based, meaning a pixel color of 0 represents the first
         * color table entry, a pixel color of 255 (if there are that
         * many) represents the 256th entry.  
         * 
         * 24+32 bit (16 is an abomination, which I'll avoid, and expect
         * no-one to mind) are stored 1byte/plane with a spare (alpha?)
         * byte for 32 bit.
         */
        {
            /*
             * Read XOR Bitmap
             */
            switch (bpp) {
            case 1:
                MSIconData->entries[iter]->xorBitmap = 
                    read1Bitmap(MSIconData->entries[iter]->width,
                                MSIconData->entries[iter]->height);
                break;
            case 4:
                MSIconData->entries[iter]->xorBitmap = 
                    read4Bitmap(MSIconData->entries[iter]->width,
                                MSIconData->entries[iter]->height);
                break;
            case 8:
                MSIconData->entries[iter]->xorBitmap = 
                    read8Bitmap(MSIconData->entries[iter]->width,
                                MSIconData->entries[iter]->height);
                break;
            case 24:
            case 32:
                MSIconData->entries[iter]->xorBitmap = 
                    readXBitmap(MSIconData->entries[iter]->width,
                                MSIconData->entries[iter]->height,bpp);
                break;
            default:
                pm_error("Uncatered bit depth %d",bpp);
            }
            /*
             * Read AND Bitmap
             */
            MSIconData->entries[iter]->andBitmap = 
                read1Bitmap(MSIconData->entries[iter]->width,
                            MSIconData->entries[iter]->height);
        }
      
    }
    return MSIconData;
}



static char * 
trimOutputName(const char inputName[])
{
    /*
     * Just trim off the final ".ppm", if there is one, else return as is.
     * oh, for =~ ... :)
     */
    char * outFile = strdup(inputName);
    if (STREQ(outFile + (strlen (outFile) - 4), ".ppm")) {
        *(outFile + (strlen (outFile) - 4)) = 0;
    }
    return outFile;

}



static int 
getBestQualityIcon(MS_Ico MSIconData)
{
    int x,best,best_size,best_bpp,bpp,size;
    IC_Entry entry;

    best_size = best_bpp = 0;
    for (x = 0; x < MSIconData->count; x++) {
        entry =  MSIconData->entries[x];
        size = entry->width * entry->height;
        bpp  = entry->bitcount ? entry->bitcount : entry->ih->bitcount;
        if (size > best_size) {
            best = x;
            best_size = size;
        } else if (size == best_size && bpp > best_bpp) {
            best = x;
            best_bpp = bpp;
        }
    }
    return best;
}

static void
writeXors(FILE *   const multiOutF,
          char           outputFileBase[], 
          IC_Entry const entry,
          int      const entryNum,
          bool     const multiple, 
          bool     const xor) {
/*----------------------------------------------------------------------------
   Write an "xor" image (i.e. the main image) out.

   'multiple' means this is one of multiple images that are being written.
   'entryNum' is the sequence number within the winicon file of the image
   we are writing.

   'xor' means to include "xor" in the output file name.

   if 'multiOutF' is non-null, it is the stream descriptor of an open
   stream to which we are to write the image.  If it is null, 
   we are to open a file using outputFileBase[] and 'entryNum' and 'xor'
   to derive its name, and close it afterward.
-----------------------------------------------------------------------------*/
    FILE * outF;
    pixel ** ppm_array;
    int row;
    int pel_size;
    const char *outputFile;
    int maxval;
    int forcetext;

    if (multiOutF) {
        outF = multiOutF;
        outputFile = strdup("");
    } else {
        if (outputFileBase) {
            if (multiple) {
                asprintfN(&outputFile, "%s%s_%d.ppm",
                          outputFileBase,(xor ? "_xor" : ""), entryNum);
            } else { 
                asprintfN(&outputFile, "%s%s.ppm",
                          outputFileBase,(xor ? "_xor" : ""));
            }
        } else
            outputFile = strdup("-");
        
        outF = pm_openw(outputFile);
    }
    /* 
     * allocate an array to save the bmp data into.
     * note that entry->height will be 1/2 entry->ih->height,
     * as the latter adds "and" and "xor" height.
     */
    ppm_array = ppm_allocarray(entry->width, entry->height);
    for (row=0; row < entry->height; row++) {
        u1 * xorRow;
        int col;
        switch (entry->bitcount) {
        case 24:
        case 32:
            pel_size = entry->bitcount >> 3;
            xorRow = entry->xorBitmap + row * entry->width * pel_size;
            for (col=0; col < entry->width*pel_size;col+=pel_size) {
                PPM_ASSIGN(ppm_array[row][col/pel_size],
                           xorRow[col+2],xorRow[col+1],xorRow[col]);
            }
            break;
        default:
            xorRow = entry->xorBitmap + row * entry->width;
            for (col=0; col < entry->width; col++) {
                int colorIndex;
                IC_Color color;
                colorIndex  = xorRow[col];
                color = entry->colors[colorIndex];
                PPM_ASSIGN(ppm_array[row][col],
                           color->red,color->green,color->blue);
            }
            break;
        }
    }    
    
    maxval = 255;
    forcetext = 0;

    ppm_writeppm(outF,ppm_array,entry->width, entry->height, 
                 (pixval) maxval, forcetext);
    ppm_freearray(ppm_array,entry->height);

    strfree(outputFile);
    
    if (!multiOutF) 
        pm_close(outF);
}
            


static void
writeAnds(FILE * const multiOutF, 
          char outputFileBase[], IC_Entry const entry, int const entryNum, 
          bool multiple) {
/*----------------------------------------------------------------------------
   Write the "and" image (i.e. the alpha mask) of the image 'IC_Entry' out.

   'multiple' means this is one of multiple images that are being written.
   'entryNum' is the sequence number within the winicon file of the image
   we are writing.

   if 'multiOutF' is non-null, it is the stream descriptor of an open
   stream to which we are to write the image.  If it is null, 
   we are to open a file using outputFileBase[] and 'entryNum' and 'xor'
   to derive its name, and close it afterward.
-----------------------------------------------------------------------------*/
    FILE * outF;
    bit ** pbm_array;
    u1 * andRow;
    int row;

    if (multiOutF)
        outF = multiOutF;
    else {
        const char *outputFile;

        assert(outputFileBase);

        if (multiple) 
            asprintfN(&outputFile, "%s_and_%d.pbm", outputFileBase, entryNum);
        else 
            asprintfN(&outputFile, "%s_and.pbm", outputFileBase);
        outF = pm_openw(outputFile);
        strfree(outputFile);
    }
    pbm_array = pbm_allocarray(entry->width, entry->height);
    for (row=0; row < entry->height; row++) {
        int col;
        andRow = entry->andBitmap + row * entry->width;
        for (col=0; col < entry->width; col++) {
            /* Note: black is transparent in a Netpbm alpha mask */
            pbm_array[row][col] = andRow[col] ? PBM_BLACK: PBM_WHITE;
        }
    }

    pbm_writepbm(outF, pbm_array, entry->width, entry->height, 0);

    pbm_freearray(pbm_array, entry->height);
    if (!multiOutF)
        pm_close (outF);     
}



static void
openMultiXor(char          outputFileBase[], 
             bool    const writeands,
             FILE ** const multiOutFP) {

    const char *outputFile;

    if (outputFileBase) {
        asprintfN(&outputFile, "%s%s.ppm",
                  outputFileBase, (writeands ? "_xor" : ""));
    } else
        outputFile = strdup("-");

    /*
     * Open the output file now, it'll stay open the whole time.
     */
    *multiOutFP = pm_openw(outputFile);

    strfree(outputFile);
}



static void
openMultiAnd(char outputFileBase[], FILE ** const multiAndOutFP) {

    const char *outputFile;

    assert(outputFileBase);

    asprintfN(&outputFile, "%s_and.pbm", outputFileBase);
    
    *multiAndOutFP = pm_openw(outputFile);

    strfree(outputFile);
}

static void free_iconentry(IC_Entry entry) {
    int x;
    if (entry->colors && entry->color_count) {
        for (x=0;x<entry->color_count;x++) free(entry->colors[x]);
        free(entry->colors);
    }
    if (entry->andBitmap) free(entry->andBitmap);
    if (entry->xorBitmap) free(entry->xorBitmap);
    if (entry->ih) free(entry->ih);
    free(entry);
}

static void free_icondata(MS_Ico MSIconData)
{
    int x;
    for (x=0;x<MSIconData->count;x++) {
    free_iconentry(MSIconData->entries[x]);
    }
    free(MSIconData);
}


int 
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    int startEntry, endEntry;
    MS_Ico MSIconData;
    char * outputFileBase;
    FILE * multiOutF;
    FILE * multiAndOutF;
   
    ppm_init (&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    if (cmdline.bestqual && cmdline.allicons)
        pm_message("-bestqual doesn't make sense with -allicons.  "
                   "Ignoring -bestqual.");
   
    if (STREQ(cmdline.outputFilespec, "-"))
        outputFileBase = NULL;
    else
        outputFileBase = trimOutputName(cmdline.outputFilespec);

    ifp = pm_openr(cmdline.inputFilespec);

    infname = cmdline.inputFilespec;

    MSIconData = readIconFile(cmdline.verbose);
    /*
     * Now we've read the icon file in (Hopefully! :)
     * Go through each of the entries, and write out files of the
     * form
     * 
     * fname_0_xor.ppm
     * fname_0_and.ppm
     * 
     * (or to stdout, depending on args parsing above).
     */
    /*
     * If allicons is set, we want everything, if not, just go through once.
     */
    startEntry = 0;
    if (cmdline.allicons) {
        endEntry = MSIconData->count;
    } else {
        endEntry = 1;
    }
    /*
     * If bestqual is set, find the icon with highest size & bpp.
     */
    if (cmdline.bestqual) {
        startEntry = getBestQualityIcon(MSIconData);
        endEntry = startEntry+1;
    }
   
    if (cmdline.multippm) 
        openMultiXor(outputFileBase, cmdline.writeands, &multiOutF);
    else
        multiOutF = NULL;

    if (cmdline.writeands && cmdline.multippm) 
        openMultiAnd(outputFileBase, &multiAndOutF);
    else
        multiAndOutF = NULL;

    {
        int entryNum;

        for (entryNum = startEntry ; entryNum < endEntry ; entryNum++ ) {
            IC_Entry const entry = MSIconData->entries[entryNum];

            writeXors(multiOutF, outputFileBase, entry, entryNum, 
                      cmdline.allicons, cmdline.writeands);
            if (cmdline.writeands)
                writeAnds(multiAndOutF, outputFileBase, 
                          entry, entryNum, cmdline.allicons);
        }
    }
    if (multiOutF)
        pm_close (multiOutF);    
    if (multiAndOutF)
        pm_close(multiAndOutF);
    
    /* free up the image data here. */
    free_icondata(MSIconData);
    return 0;
}
