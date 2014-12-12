/* pnmtops.c - read a PNM image and produce a PostScript program.

   Copyright information is at end of file.

   We produce two main kinds of Postscript program:

      1) Use built in Postscript filters /ASCII85Decode, /ASCIIHexDecode,
         /RunLengthDecode, and /FlateDecode;

         We use methods we learned from Dirk Krause's program Bmeps and
         raster encoding code copied almost directly from Bmeps.

      2) Use our own filters and redefine /readstring .  This is aboriginal
         Netpbm code, from when Postscript was young.

   (2) is the default, because it's been working for ages and we have
   more confidence in it.  But (1) gives more options.  The user
   selects (1) with the -psfilter option.

   We also do a few other bold new things only when the user specifies
   -psfilter, because we're not sure they work for everyone.

   (I actually don't know Postscript, so some of this description, not to
   mention the code, may be totally bogus.)

   NOTE: it is possible to put transparency information in an
   encapsulated Postscript program.  Bmeps does this.  We don't.  It
   might be hard to do, because in Postscript, the transparency information
   goes in separate from the rest of the raster.
*/

#define _BSD_SOURCE  /* Make sure string.h contains strdup() */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <string.h>
#include <assert.h>
#include "pam.h"
#include "mallocvar.h"
#include "shhopt.h"
#include "nstring.h"
#include "bmepsoe.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFileName;  /* Filespecs of input file */
    float scale;
    unsigned int dpiX;     /* horiz component of DPI option */
    unsigned int dpiY;     /* vert component of DPI option */
    unsigned int width;              /* in 1/72 inch */
    unsigned int height;             /* in 1/72 inch */
    unsigned int mustturn;
    bool         canturn;
    unsigned int rle;
    bool         center;
    unsigned int imagewidth;         /* in 1/72 inch; zero if unspec */
    unsigned int imageheight;        /* in 1/72 inch; zero if unspec */
    unsigned int equalpixels;
    unsigned int setpage;
    bool         showpage;
    unsigned int level;
    unsigned int levelSpec;
    unsigned int psfilter;
    unsigned int flate;
    unsigned int ascii85;
    unsigned int dict;
    unsigned int vmreclaim;
    unsigned int verbose;
};


static bool verbose;


static void
parseDpi(const char *   const dpiOpt, 
         unsigned int * const dpiXP, 
         unsigned int * const dpiYP) {

    char *dpistr2;
    unsigned int dpiX, dpiY;

    dpiX = strtol(dpiOpt, &dpistr2, 10);
    if (dpistr2 == dpiOpt) 
        pm_error("Invalid value for -dpi: '%s'.  Must be either number "
                 "or NxN ", dpiOpt);
    else {
        if (*dpistr2 == '\0') {
            *dpiXP = dpiX;
            *dpiYP = dpiX;
        } else if (*dpistr2 == 'x') {
            char * dpistr3;

            dpistr2++;  /* Move past 'x' */
            dpiY = strtol(dpistr2, &dpistr3, 10);        
            if (dpistr3 != dpistr2 && *dpistr3 == '\0') {
                *dpiXP = dpiX;
                *dpiYP = dpiY;
            } else {
                pm_error("Invalid value for -dpi: '%s'.  Must be either "
                         "number or NxN", dpiOpt);
            }
        }
    }
}



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {

    unsigned int imagewidthSpec, imageheightSpec;
    float imagewidth, imageheight;
    unsigned int center, nocenter;
    unsigned int nosetpage;
    float width, height;
    unsigned int noturn;
    unsigned int showpage, noshowpage;
    const char *dpiOpt;
    unsigned int dpiSpec;

    optStruct3 opt;
    unsigned int option_def_index = 0;
    optEntry *option_def;

    MALLOCARRAY_NOFAIL(option_def, 100);

    OPTENT3(0, "scale",       OPT_FLOAT, &cmdlineP->scale, NULL,         0);
    OPTENT3(0, "dpi",         OPT_STRING, &dpiOpt,         &dpiSpec,     0);
    OPTENT3(0, "width",       OPT_FLOAT, &width,           NULL,         0);
    OPTENT3(0, "height",      OPT_FLOAT, &height,          NULL,         0);
    OPTENT3(0, "psfilter",    OPT_FLAG,  NULL, &cmdlineP->psfilter,      0);
    OPTENT3(0, "turn",        OPT_FLAG,  NULL, &cmdlineP->mustturn,      0);
    OPTENT3(0, "noturn",      OPT_FLAG,  NULL, &noturn,                  0);
    OPTENT3(0, "rle",         OPT_FLAG,  NULL, &cmdlineP->rle,           0);
    OPTENT3(0, "runlength",   OPT_FLAG,  NULL, &cmdlineP->rle,           0);
    OPTENT3(0, "ascii85",     OPT_FLAG,  NULL, &cmdlineP->ascii85,       0);
    OPTENT3(0, "center",      OPT_FLAG,  NULL, &center,                  0);
    OPTENT3(0, "nocenter",    OPT_FLAG,  NULL, &nocenter,                0);
    OPTENT3(0, "equalpixels", OPT_FLAG,  NULL, &cmdlineP->equalpixels,   0);
    OPTENT3(0, "imagewidth",  OPT_FLOAT, &imagewidth,  &imagewidthSpec,  0);
    OPTENT3(0, "imageheight", OPT_FLOAT, &imageheight, &imageheightSpec, 0);
    OPTENT3(0, "nosetpage",   OPT_FLAG,  NULL, &nosetpage,               0);
    OPTENT3(0, "setpage",     OPT_FLAG,  NULL, &cmdlineP->setpage,       0);
    OPTENT3(0, "noshowpage",  OPT_FLAG,  NULL, &noshowpage,              0);
    OPTENT3(0, "flate",       OPT_FLAG,  NULL, &cmdlineP->flate,         0);
    OPTENT3(0, "dict",        OPT_FLAG,  NULL, &cmdlineP->dict,          0);
    OPTENT3(0, "vmreclaim",   OPT_FLAG,  NULL, &cmdlineP->vmreclaim,     0);
    OPTENT3(0, "showpage",    OPT_FLAG,  NULL, &showpage,                0);
    OPTENT3(0, "verbose",     OPT_FLAG,  NULL, &cmdlineP->verbose,       0);
    OPTENT3(0, "level",       OPT_UINT, &cmdlineP->level, 
            &cmdlineP->levelSpec,              0);
    
    /* DEFAULTS */
    cmdlineP->scale = 1.0;
    width = 8.5;
    height = 11.0;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;
    opt.allowNegNum = FALSE;

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

    if (cmdlineP->mustturn && noturn)
        pm_error("You cannot specify both -turn and -noturn");
    if (center && nocenter)
        pm_error("You cannot specify both -center and -nocenter");
    if (showpage && noshowpage)
        pm_error("You cannot specify both -showpage and -noshowpage");
    if (cmdlineP->setpage && nosetpage)
        pm_error("You cannot specify both -setpage and -nosetpage");

    if (dpiSpec)
        parseDpi(dpiOpt, &cmdlineP->dpiX, &cmdlineP->dpiY);
    else {
        cmdlineP->dpiX = 300;
        cmdlineP->dpiY = 300;
    }

    cmdlineP->center  =  !nocenter;
    cmdlineP->canturn =  !noturn;
    cmdlineP->showpage = !noshowpage;
    
    cmdlineP->width  = width * 72;
    cmdlineP->height = height * 72;

    if (imagewidthSpec)
        cmdlineP->imagewidth = imagewidth * 72;
    else
        cmdlineP->imagewidth = 0;
    if (imageheightSpec)
        cmdlineP->imageheight = imageheight * 72;
    else
        cmdlineP->imageheight = 0;

    if (!cmdlineP->psfilter &&
        (cmdlineP->flate || cmdlineP->ascii85))
        pm_error("You must specify -psfilter in order to specify "
                 "-flate or -ascii85");

    if (argc-1 == 0) 
        cmdlineP->inputFileName = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->inputFileName = argv[1];

}


/*===========================================================================
  The native output encoder.  This is archaic and uses global variables.
  It is probably obsoleted by the bmeps output encoder; we just haven't
  had a chance to verify that yet.
===========================================================================*/

/*----------------------------------------------------------------------------
   The following global variables are the native output encoder state.
-----------------------------------------------------------------------------*/
static unsigned int itemsinline;
    /* The number of items in the line we are currently building */
static unsigned int bitsinitem;
    /* The number of bits filled so far in the item we are currently
       building 
    */
static unsigned int rlebitsinitem;
    /* The number of bits filled so far in the item we are currently
       building 
    */
static unsigned int bitspersample;
static unsigned int item, bitshift, items;
static unsigned int rleitem, rlebitshift;
static unsigned int repeat, itembuf[128], count, repeatitem, repeatcount;



static void
initNativeOutputEncoder(bool const rle, unsigned int const bitspersample) {
/*----------------------------------------------------------------------------
   Initialize the native output encoder.  Call this once per
   Postscript image that you will write with putitem(), before for the
   first putitem().

   We initialize the item putter state variables, which are the
   global variable defined above.
-----------------------------------------------------------------------------*/
    itemsinline = 0;
    items = 0;

    if (rle) {
        rleitem = 0;
        rlebitsinitem = 0;
        rlebitshift = 8 - bitspersample;
        repeat = 1;
        count = 0;
    } else {
        item = 0;
        bitsinitem = 0;
        bitshift = 8 - bitspersample;
    }

}



static void
putitem(void) {
    const char* const hexits = "0123456789abcdef";

    if (itemsinline == 30) {
        putchar('\n');
        itemsinline = 0;
    }
    putchar(hexits[item >> 4]);
    putchar(hexits[item & 15]);
    ++itemsinline;
    ++items;
    item = 0;
    bitsinitem = 0;
    bitshift = 8 - bitspersample;
}



static void
flushitem() {
    if (bitsinitem > 0)
        putitem();
}



static void 
putxelval(xelval const xv) {
    if (bitsinitem == 8)
        putitem();
    item += xv << bitshift;
    bitsinitem += bitspersample;
    bitshift -= bitspersample;
}



static void
rleputbuffer() {
    if (repeat) {
        item = 256 - count;
        putitem();
        item = repeatitem;
        putitem();
    } else {
        unsigned int i;
    
        item = count - 1;
        putitem();
        for (i = 0; i < count; ++i) {
            item = itembuf[i];
            putitem();
        }
    }
    repeat = 1;
    count = 0;
}



static void
rleputitem() {
    int i;

    if ( count == 128 )
        rleputbuffer();

    if ( repeat && count == 0 )
    { /* Still initializing a repeat buf. */
        itembuf[count] = repeatitem = rleitem;
        ++count;
    }
    else if ( repeat )
    { /* Repeating - watch for end of run. */
        if ( rleitem == repeatitem )
        { /* Run continues. */
            itembuf[count] = rleitem;
            ++count;
        }
        else
        { /* Run ended - is it long enough to dump? */
            if ( count > 2 )
            { /* Yes, dump a repeat-mode buffer and start a new one. */
                rleputbuffer();
                itembuf[count] = repeatitem = rleitem;
                ++count;
            }
            else
            { /* Not long enough - convert to non-repeat mode. */
                repeat = 0;
                itembuf[count] = repeatitem = rleitem;
                ++count;
                repeatcount = 1;
            }
        }
    }
    else
    { /* Not repeating - watch for a run worth repeating. */
        if ( rleitem == repeatitem )
        { /* Possible run continues. */
            ++repeatcount;
            if ( repeatcount > 3 )
            { /* Long enough - dump non-repeat part and start repeat. */
                count = count - ( repeatcount - 1 );
                rleputbuffer();
                count = repeatcount;
                for ( i = 0; i < count; ++i )
                    itembuf[i] = rleitem;
            }
            else
            { /* Not long enough yet - continue as non-repeat buf. */
                itembuf[count] = rleitem;
                ++count;
            }
        }
        else
        { /* Broken run. */
            itembuf[count] = repeatitem = rleitem;
            ++count;
            repeatcount = 1;
        }
    }

    rleitem = 0;
    rlebitsinitem = 0;
    rlebitshift = 8 - bitspersample;
}



static void 
rleputxelval(xelval const xv) {
    if (rlebitsinitem == 8)
        rleputitem();
    rleitem += xv << rlebitshift;
    rlebitsinitem += bitspersample;
    rlebitshift -= bitspersample;
}



static void
rleflush() {
    if (rlebitsinitem > 0)
        rleputitem();
    if (count > 0)
        rleputbuffer();
}


static void
flushNativeOutput(bool const rle) {
    if (rle)
        rleflush();
    else
        flushitem();
    printf("\n");
}
        
/*===========================================================================
  The BMEPS output encoder.
===========================================================================*/

/* This code is just a wrapper around the output encoder that is part of
   Bmeps, to give it better modularity.
*/

struct bmepsoe {
    Output_Encoder * oeP;
    int * rleBuffer;
    Byte * flateInBuffer;
    Byte * flateOutBuffer;
};



static void
createBmepsOutputEncoder(struct bmepsoe ** const bmepsoePP,
                         FILE *            const ofP,
                         bool              const rle,
                         bool              const flate,
                         bool              const ascii85) {

    unsigned int const FLATE_IN_SIZE = 16384;
    unsigned int const FLATE_OUT_SIZE = 17408;

    struct bmepsoe * bmepsoeP;
    int mode;

    MALLOCVAR_NOFAIL(bmepsoeP);
    MALLOCVAR_NOFAIL(bmepsoeP->oeP);
    MALLOCARRAY_NOFAIL(bmepsoeP->rleBuffer, 129);
    MALLOCARRAY_NOFAIL(bmepsoeP->flateInBuffer, FLATE_IN_SIZE);
    MALLOCARRAY_NOFAIL(bmepsoeP->flateOutBuffer, FLATE_OUT_SIZE);

    mode = 0;
    if (rle)
        mode |= OE_RL;
    if (flate)
        mode |= OE_FLATE;
    if (ascii85)
        mode |= OE_ASC85;

    oe_init(bmepsoeP->oeP, ofP, mode, 9, 
            bmepsoeP->rleBuffer, 
            bmepsoeP->flateInBuffer, FLATE_IN_SIZE,
            bmepsoeP->flateOutBuffer, FLATE_OUT_SIZE);

    *bmepsoePP = bmepsoeP;
}



static void
destroyBmepsOutputEncoder(struct bmepsoe * const bmepsoeP) {
    
    free(bmepsoeP->rleBuffer);
    free(bmepsoeP->flateInBuffer);
    free(bmepsoeP->flateOutBuffer);
    
    free(bmepsoeP);
}



static void
outputBmepsSample(struct bmepsoe * const bmepsoeP,
                  unsigned int     const sampleValue,
          unsigned int     const bitsPerSample) {

    if (bitsPerSample == 8)
        oe_byte_add(bmepsoeP->oeP, sampleValue);
    else {
        unsigned int m;

        for (m = 1 << (bitsPerSample-1); m != 0; m >>= 1)
            /* depends on oe_bit_add accepting any value !=0 as 1 */
            oe_bit_add(bmepsoeP->oeP, sampleValue & m); 
    }
}



static void
flushBmepsOutput(struct bmepsoe * const bmepsoeP) {
    oe_byte_flush(bmepsoeP->oeP);
}


/*============================================================================
   END OF OUTPUT ENCODERS
============================================================================*/


static void
computeImagePosition(int     const dpiX, 
                     int     const dpiY, 
                     int     const icols, 
                     int     const irows,
                     bool    const mustturn,
                     bool    const canturn,
                     bool    const center,
                     int     const pagewid, 
                     int     const pagehgt, 
                     float   const requestedScale,
                     float   const imagewidth,
                     float   const imageheight,
                     bool    const equalpixels,
                     float * const scolsP,
                     float * const srowsP,
                     float * const llxP, 
                     float * const llyP,
                     bool *  const turnedP ) {
/*----------------------------------------------------------------------------
   Determine where on the page the image is to go.  This means position,
   dimensions, and orientation.

   icols/irows are the dimensions of the PNM input in xels.

   'mustturn' means we are required to rotate the image.

   'canturn' means we may rotate the image if it fits better, but don't
   have to.

   *scolsP, *srowsP are the dimensions of the image in 1/72 inch.

   *llxP, *llyP are the coordinates, in 1/72 inch, of the lower left
   corner of the image on the page.

   *turnedP is true iff the image is to be rotated 90 degrees on the page.

   imagewidth/imageheight are the requested dimensions of the image on
   the page, in 1/72 inch.  Image will be as large as possible within
   those dimensions.  Zero means unspecified, so 'scale', 'pagewid',
   'pagehgt', 'irows', and 'icols' determine image size.

   'equalpixels' means the user wants one printed pixel per input pixel.
   It is inconsistent with imagewidth or imageheight != 0

   'requestedScale' is meaningful only when imageheight/imagewidth == 0
   and equalpixels == FALSE.  It tells how many inches the user wants
   72 pixels of input to occupy, if it fits on the page.
-----------------------------------------------------------------------------*/
    int cols, rows;
        /* Number of columns, rows of input xels in the output, as
           rotated if applicable
        */
    bool shouldturn;  /* The image fits the page better if we turn it */
    
    if (icols > irows && pagehgt > pagewid)
        shouldturn = TRUE;
    else if (irows > icols && pagewid > pagehgt)
        shouldturn = TRUE;
    else
        shouldturn = FALSE;

    if (mustturn || (canturn && shouldturn)) {
        *turnedP = TRUE;
        cols = irows;
        rows = icols;
    } else {
        *turnedP = FALSE;
        cols = icols;
        rows = irows;
    }
    if (equalpixels) {
        *scolsP = (72.0/dpiX)*cols;
        *srowsP = (72.0/dpiY)*rows;
    } else if (imagewidth > 0 || imageheight > 0) {
        float scale;

        if (imagewidth == 0)
            scale = (float) imageheight/rows;
        else if (imageheight == 0)
            scale = (float) imagewidth/cols;
        else
            scale = MIN((float)imagewidth/cols, (float)imageheight/rows);
        
        *scolsP = cols*scale;
        *srowsP = rows*scale;
    } else {
        /* He didn't give us a bounding box for the image so figure
           out output image size from other inputs.
        */
        const int devpixX = dpiX / 72.0 + 0.5;        
        const int devpixY = dpiY / 72.0 + 0.5;        
            /* How many device pixels make up 1/72 inch, rounded to
               nearest integer */
        const float pixfacX = 72.0 / dpiX * devpixX;  /* 1, approx. */
        const float pixfacY = 72.0 / dpiY * devpixY;  /* 1, approx. */
        float scale;

        scale = MIN(requestedScale, 
                    MIN((float)pagewid/cols, (float)pagehgt/rows));

        *scolsP = scale * cols * pixfacX;
        *srowsP = scale * rows * pixfacY;
        
        if (scale != requestedScale)
            pm_message("warning, image too large for page, rescaling to %g", 
                       scale );

        /* Before May 2001, Pnmtops enforced a 5% margin around the page.
           If the image would be too big to leave a 5% margin, Pnmtops would
           scale it down.  But people have images that are exactly the size
           of a page, e.g. because they created them with Sane's 'scanimage'
           program from a full page of input.  So we removed the gratuitous
           5% margin.  -Bryan.
        */
    }
    *llxP = (center) ? ( pagewid - *scolsP ) / 2 : 0;
    *llyP = (center) ? ( pagehgt - *srowsP ) / 2 : 0;


    if (verbose)
        pm_message("Image will be %3.2f points wide by %3.2f points high, "
                   "left edge %3.2f points from left edge of page, "
                   "bottom edge %3.2f points from top of page; "
                   "%sturned to landscape orientation",
                   *scolsP, *srowsP, *llxP, *llyP, *turnedP ? "" : "NOT ");
}



static void
determineDictionaryRequirement(bool           const userWantsDict,
                               bool           const psFilter,
                               unsigned int * const dictSizeP) {

    if (userWantsDict) {
        if (psFilter) {
            /* The Postscript this program generates to use built-in
               Postscript filters does not define any variables.
            */
            *dictSizeP = 0;
        } else
            *dictSizeP = 8;
    } else
        *dictSizeP = 0;
}



static void
defineReadstring(bool const rle) {
/*----------------------------------------------------------------------------
   Write to Standard Output Postscript statements to define /readstring.
-----------------------------------------------------------------------------*/
    if (rle) {
        printf("/rlestr1 1 string def\n");
        printf("/readrlestring {\n");             /* s -- nr */
        printf("  /rlestr exch def\n");           /* - */
        printf("  currentfile rlestr1 readhexstring pop\n");  /* s1 */
        printf("  0 get\n");                  /* c */
        printf("  dup 127 le {\n");               /* c */
        printf("    currentfile rlestr 0\n");         /* c f s 0 */
        printf("    4 3 roll\n");             /* f s 0 c */
        printf("    1 add  getinterval\n");           /* f s */
        printf("    readhexstring pop\n");            /* s */
        printf("    length\n");               /* nr */
        printf("  } {\n");                    /* c */
        printf("    256 exch sub dup\n");         /* n n */
        printf("    currentfile rlestr1 readhexstring pop\n");/* n n s1 */
        printf("    0 get\n");                /* n n c */
        printf("    exch 0 exch 1 exch 1 sub {\n");       /* n c 0 1 n-1*/
        printf("      rlestr exch 2 index put\n");
        printf("    } for\n");                /* n c */
        printf("    pop\n");                  /* nr */
        printf("  } ifelse\n");               /* nr */
        printf("} bind def\n");
        printf("/readstring {\n");                /* s -- s */
        printf("  dup length 0 {\n");             /* s l 0 */
        printf("    3 copy exch\n");              /* s l n s n l*/
        printf("    1 index sub\n");              /* s l n s n r*/
        printf("    getinterval\n");              /* s l n ss */
        printf("    readrlestring\n");            /* s l n nr */
        printf("    add\n");                  /* s l n */
        printf("    2 copy le { exit } if\n");        /* s l n */
        printf("  } loop\n");                 /* s l l */
        printf("  pop pop\n");                /* s */
        printf("} bind def\n");
    } else {
        printf("/readstring {\n");                /* s -- s */
        printf("  currentfile exch readhexstring pop\n");
        printf("} bind def\n");
    }
}



static void
setupReadstringNative(bool         const rle,
                      bool         const color,
                      unsigned int const icols, 
                      unsigned int const padright, 
                      unsigned int const bps) {
/*----------------------------------------------------------------------------
   Write to Standard Output statements to define /readstring and also
   arguments for it (/picstr or /rpicstr, /gpicstr, and /bpicstr).
-----------------------------------------------------------------------------*/
    unsigned int const bytesPerRow = (icols + padright) * bps / 8;

    defineReadstring(rle);
    
    if (color) {
        printf("/rpicstr %d string def\n", bytesPerRow);
        printf("/gpicstr %d string def\n", bytesPerRow);
        printf("/bpicstr %d string def\n", bytesPerRow);
    } else
        printf("/picstr %d string def\n", bytesPerRow);
}



static void
putFilters(unsigned int const postscriptLevel,
           bool         const rle,
           bool         const flate,
           bool         const ascii85,
           bool         const color) {

    assert(postscriptLevel > 1);
    
    if (ascii85)
        printf("/ASCII85Decode filter ");
    else 
        printf("/ASCIIHexDecode filter ");
    if (flate)
        printf("/FlateDecode filter ");
    if (rle) /* bmeps encodes rle before flate, so must decode after! */
        printf("/RunLengthDecode filter ");
}



static void
putReadstringNative(bool const color) {

    if (color) {
        printf("{ rpicstr readstring }\n");
        printf("{ gpicstr readstring }\n");
        printf("{ bpicstr readstring }\n");
    } else
        printf("{ picstr readstring }\n");
}



static void
putSetup(unsigned int const dictSize,
         bool         const psFilter,
         bool         const rle,
         bool         const color,
         unsigned int const icols,
         unsigned int const padright,
         unsigned int const bps) {
/*----------------------------------------------------------------------------
   Put the setup section in the Postscript program on Standard Output.
-----------------------------------------------------------------------------*/
    printf("%%%%BeginSetup\n");

    if (dictSize > 0)
        /* inputf {r,g,b,}pictsr readstring readrlestring rlestring */
        printf("%u dict begin\n", dictSize);
    
    if (!psFilter)
        setupReadstringNative(rle, color, icols, padright, bps);

    printf("%%%%EndSetup\n");
}



static void
putImage(bool const psFilter,
         bool const color) {
/*----------------------------------------------------------------------------
   Put the image/colorimage statement in the Postscript program on
   Standard Output.
-----------------------------------------------------------------------------*/
    if (color) {
        if (psFilter)
            printf("false 3\n");
        else
            printf("true 3\n");
        printf("colorimage");
    } else
        printf("image");
}



static void
putInitPsFilter(unsigned int const postscriptLevel,
                bool         const rle,
                bool         const flate,
                bool         const ascii85,
                bool         const color) {

    bool const filterTrue = TRUE;

    printf("{ currentfile ");

    putFilters(postscriptLevel, rle, flate, ascii85, color);

    putImage(filterTrue, color);
    
    printf(" } exec");
}



static void
putInitReadstringNative(bool const color) {

    bool const filterFalse = FALSE;

    putReadstringNative(color);
    
    putImage(filterFalse, color);
}



static void
putInit(unsigned int const postscriptLevel,
        char         const name[], 
        int          const icols, 
        int          const irows, 
        float        const scols, 
        float        const srows,
        float        const llx, 
        float        const lly,
        int          const padright, 
        int          const bps,
        int          const pagewid, 
        int          const pagehgt,
        bool         const color, 
        bool         const turned, 
        bool         const rle,
        bool         const flate,
        bool         const ascii85,
        bool         const setpage,
        bool         const psFilter,
        unsigned int const dictSize) {
/*----------------------------------------------------------------------------
   Write out to Standard Output the headers stuff for the Postscript
   program (everything up to the raster).
-----------------------------------------------------------------------------*/
    /* The numbers in the %! line often confuse people. They are NOT the
       PostScript language level.  The first is the level of the DSC comment
       spec being adhered to, the second is the level of the EPSF spec being
       adhered to.  It is *incorrect* to claim EPSF compliance if the file
       contains a setpagedevice.
    */
    printf("%%!PS-Adobe-3.0%s\n", setpage ? "" : " EPSF-3.0");
    printf("%%%%LanguageLevel: %u\n", postscriptLevel);
    printf("%%%%Creator: pnmtops\n");
    printf("%%%%Title: %s.ps\n", name);
    printf("%%%%Pages: 1\n");
    printf(
        "%%%%BoundingBox: %d %d %d %d\n",
        (int) llx, (int) lly,
        (int) (llx + scols + 0.5), (int) (lly + srows + 0.5));
    printf("%%%%EndComments\n");

    putSetup(dictSize, psFilter, rle, color, icols, padright, bps);

    printf("%%%%Page: 1 1\n");
    if (setpage)
        printf("<< /PageSize [ %d %d ] /ImagingBBox null >> setpagedevice\n",
               pagewid, pagehgt);
    printf("gsave\n");
    printf("%g %g translate\n", llx, lly);
    printf("%g %g scale\n", scols, srows);
    if (turned)
        printf("0.5 0.5 translate  90 rotate  -0.5 -0.5 translate\n");
    printf("%d %d %d\n", icols, irows, bps);
    printf("[ %d 0 0 -%d 0 %d ]\n", icols, irows, irows);

    if (psFilter)
        putInitPsFilter(postscriptLevel, rle, flate, ascii85, color);
    else
        putInitReadstringNative(color);

    printf("\n");
}



static void
putEnd(bool         const showpage, 
       bool         const psFilter,
       bool         const ascii85,
       unsigned int const dictSize,
       bool         const vmreclaim) {

    if (psFilter) {
        if (ascii85)
            printf("%s\n", "~>");
        else
            printf("%s\n", ">");
    } else {
        printf("currentdict /inputf undef\n");
        printf("currentdict /picstr undef\n");
        printf("currentdict /rpicstr undef\n");
        printf("currentdict /gpicstr undef\n");
        printf("currentdict /bpicstr undef\n");
    }

    if (dictSize > 0)
        printf("end\n");

    if (vmreclaim)
        printf("1 vmreclaim\n");

    printf("grestore\n");

    if (showpage)
        printf("showpage\n");
    printf("%%%%Trailer\n");
}



static void
computeDepth(xelval         const inputMaxval,
             unsigned int   const postscriptLevel, 
             unsigned int * const bitspersampleP,
             unsigned int * const psMaxvalP) {
/*----------------------------------------------------------------------------
   Figure out how many bits will represent each sample in the Postscript
   program, and the maxval of the Postscript program samples.  The maxval
   is just the maximum value allowable in the number of bits.
-----------------------------------------------------------------------------*/
    unsigned int const bitsRequiredByMaxval = pm_maxvaltobits(inputMaxval);

    if (bitsRequiredByMaxval <= 1)
        *bitspersampleP = 1;
    else if (bitsRequiredByMaxval <= 2)
        *bitspersampleP = 2;
    else if (bitsRequiredByMaxval <= 4)
        *bitspersampleP = 4;
    else        
        *bitspersampleP = 8;

    /* There is supposedly a 12 bits per pixel Postscript format, but
       what?  We produce a raster that is composed of bytes, each
       coded as a pair of hexadecimal characters and representing 8,
       4, 2, or 1 pixels.  We also have the RLE format, where some of
       those bytes are run lengths.
    */

    if (*bitspersampleP < bitsRequiredByMaxval) {
        if (bitsRequiredByMaxval <= 12 && postscriptLevel >= 2)
            pm_message("Maxval of input requires %u bit samples for full "
                       "resolution, and Postscript level %u is capable "
                       "of representing that many, but this program "
                       "doesn't know how.  So we are using %u",
                       bitsRequiredByMaxval, postscriptLevel, *bitspersampleP);
        else
            pm_message("Maxval of input requires %u bit samples for full "
                       "resolution, but we are using the Postscript level %u "
                       "maximum of %u",
                       bitsRequiredByMaxval, postscriptLevel, *bitspersampleP);
    }

    *psMaxvalP = pm_bitstomaxval(*bitspersampleP);

    if (verbose)
        pm_message("Input maxval is %u.  Postscript raster will have "
                   "%u bits per sample, so maxval = %u",
                   inputMaxval, *bitspersampleP, *psMaxvalP);
}    



static void
convertRowNative(struct pam * const pamP, 
                 tuple *      const tuplerow, 
                 unsigned int const psMaxval, 
                 bool         const rle, 
                 unsigned int const padright) {

    unsigned int plane;

    for (plane = 0; plane < pamP->depth; ++plane) {
        unsigned int col;
        for (col= 0; col < pamP->width; ++col) {
            sample const scaledSample = 
                pnm_scalesample(tuplerow[col][plane], pamP->maxval, psMaxval);

            if (rle)
                rleputxelval(scaledSample);
            else
                putxelval(scaledSample);
        }
        for (col = 0; col < padright; ++col)
            if (rle)
                rleputxelval(0);
        else
            putxelval(0);
        if (rle)
            rleflush();
    }
}



static void
convertRowPsFilter(struct pam *     const pamP,
                   tuple *          const tuplerow,
                   struct bmepsoe * const bmepsoeP,
                   unsigned int     const psMaxval) {

    unsigned int const bitsPerSample = pm_maxvaltobits(psMaxval);
    unsigned int const stragglers =
        (((bitsPerSample * pamP->depth) % 8) * pamP->width) % 8;
        /* Number of bits at the right edge that don't fill out a
           whole byte
        */

    unsigned int col;
    tuple scaledTuple;
    
    scaledTuple = pnm_allocpamtuple(pamP);

    for (col = 0; col < pamP->width; ++col) {
        unsigned int plane;
        pnm_scaletuple(pamP, scaledTuple, tuplerow[col], psMaxval);
        
        for (plane = 0; plane < pamP->depth; ++plane)
            outputBmepsSample(bmepsoeP, scaledTuple[plane], bitsPerSample);
    }
    if (stragglers > 0) {
        unsigned int i;
        for (i = stragglers; i < 8; ++i)
            oe_bit_add(bmepsoeP->oeP, 0);
    }
    pnm_freepamtuple(scaledTuple);
}



static void
selectPostscriptLevel(bool           const levelIsGiven,
                      unsigned int   const levelGiven,
                      bool           const color,
                      bool           const dict,
                      bool           const flate,
                      bool           const ascii85,
                      bool           const psFilter,
                      unsigned int * const postscriptLevelP) {

    unsigned int const maxPermittedLevel = 
        levelIsGiven ? levelGiven : UINT_MAX;
    unsigned int minPossibleLevel;

    /* Until we know, later in this function, that we needs certain
       features, we assume we can get by with classic Postscript Level 1:
    */
    minPossibleLevel = 1;

    /* Now we increase 'minPossibleLevel' as we notice that each of
       various features are required:
    */
    if (color) {
        minPossibleLevel = MAX(minPossibleLevel, 2);
        if (2 > maxPermittedLevel)
            pm_error("Color requires at least Postscript level 2");
    }
    if (flate) {
        minPossibleLevel = MAX(minPossibleLevel, 3);
        if (2 > maxPermittedLevel)
            pm_error("flate compression requires at least Postscript level 3");
    }
    if (ascii85) {
        minPossibleLevel = MAX(minPossibleLevel, 2);
        if (2 > maxPermittedLevel)
            pm_error("ascii85 encoding requires at least Postscript level 2");
    }
    if (psFilter) {
        minPossibleLevel = MAX(minPossibleLevel, 2);
        if (2 > maxPermittedLevel)
            pm_error("-psfilter requires at least Postscript level 2");
    }
    if (levelIsGiven)
        *postscriptLevelP = levelGiven;
    else
        *postscriptLevelP = minPossibleLevel;
}



static void
convertPage(FILE * const ifP, 
            int    const turnflag, 
            int    const turnokflag, 
            bool   const psFilter,
            bool   const rle, 
            bool   const flate,
            bool   const ascii85,
            bool   const setpage,
            bool   const showpage,
            bool   const center, 
            float  const scale,
            int    const dpiX, 
            int    const dpiY, 
            int    const pagewid, 
            int    const pagehgt,
            int    const imagewidth, 
            int    const imageheight, 
            bool   const equalpixels,
            char   const name[],
            bool   const dict,
            bool   const vmreclaim,
            bool   const levelIsGiven,
            bool   const levelGiven) {
    
    struct pam inpam;
    tuple* tuplerow;
    unsigned int padright;
        /* Number of bits we must add to the right end of each Postscript
           output line in order to have an integral number of bytes of output.
           E.g. at 2 bits per sample with 10 columns, this would be 4.
        */
    int row;
    unsigned int psMaxval;
        /* The maxval of the Postscript program */
    float scols, srows;
    float llx, lly;
    bool turned;
    bool color;
    unsigned int postscriptLevel;
    struct bmepsoe * bmepsoeP;
    unsigned int dictSize;
        /* Size of Postscript dictionary we should define */

    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));
    
    if (!STRSEQ(inpam.tuple_type, PAM_PBM_TUPLETYPE) &&
        !STRSEQ(inpam.tuple_type, PAM_PGM_TUPLETYPE) &&
        !STRSEQ(inpam.tuple_type, PAM_PPM_TUPLETYPE))
        pm_error("Unrecognized tuple type %s.  This program accepts only "
                 "PBM, PGM, PPM, and equivalent PAM input images", 
                 inpam.tuple_type);

    color = STRSEQ(inpam.tuple_type, PAM_PPM_TUPLETYPE);
    
    selectPostscriptLevel(levelIsGiven, levelGiven, color, 
                          dict, flate, ascii85, psFilter, &postscriptLevel);
    
    if (color)
        pm_message("generating color Postscript program.");

    computeDepth(inpam.maxval, postscriptLevel, &bitspersample, &psMaxval);
    {
        unsigned int const realBitsPerLine = inpam.width * bitspersample;
        unsigned int const paddedBitsPerLine = ((realBitsPerLine + 7) / 8) * 8;
        padright = (paddedBitsPerLine - realBitsPerLine) / bitspersample;
    }
    /* In positioning/scaling the image, we treat the input image as if
       it has a density of 72 pixels per inch.
    */
    computeImagePosition(dpiX, dpiY, inpam.width, inpam.height, 
                         turnflag, turnokflag, center,
                         pagewid, pagehgt, scale, imagewidth, imageheight,
                         equalpixels,
                         &scols, &srows, &llx, &lly, &turned);

    determineDictionaryRequirement(dict, psFilter, &dictSize);
    
    putInit(postscriptLevel, name, inpam.width, inpam.height, 
            scols, srows, llx, lly, padright, bitspersample, 
            pagewid, pagehgt, color,
            turned, rle, flate, ascii85, setpage, psFilter, dictSize);

    createBmepsOutputEncoder(&bmepsoeP, stdout, rle, flate, ascii85);
    initNativeOutputEncoder(rle, bitspersample);

    tuplerow = pnm_allocpamrow(&inpam);

    for (row = 0; row < inpam.height; ++row) {
        pnm_readpamrow(&inpam, tuplerow);
        if (psFilter)
            convertRowPsFilter(&inpam, tuplerow, bmepsoeP, psMaxval);
        else
            convertRowNative(&inpam, tuplerow, psMaxval, rle, padright);
    }

    pnm_freepamrow(tuplerow);

    if (psFilter)
        flushBmepsOutput(bmepsoeP);
    else
        flushNativeOutput(rle);

    destroyBmepsOutputEncoder(bmepsoeP);

    putEnd(showpage, psFilter, ascii85, dictSize, vmreclaim);
}



static const char *
basebasename(const char * const filespec) {
/*----------------------------------------------------------------------------
    Return filename up to first period
-----------------------------------------------------------------------------*/
    char const dirsep = '/';
    const char * const lastSlashPos = strrchr(filespec, dirsep);

    char * name;
    const char * filename;

    if (lastSlashPos)
        filename = lastSlashPos + 1;
    else
        filename = filespec;

    name = strdup(filename);
    if (name != NULL) {
        char * const dotPosition = strchr(name, '.');

        if (dotPosition)
            *dotPosition = '\0';
    }
    return name;
}



int
main(int argc, char * argv[]) {

    FILE* ifp;
    const char *name;  /* malloc'ed */
    struct cmdlineInfo cmdline;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    verbose = cmdline.verbose;

    ifp = pm_openr(cmdline.inputFileName);

    if (STREQ(cmdline.inputFileName, "-"))
        name = strdup("noname");
    else
        name = basebasename(cmdline.inputFileName);
    {
        int eof;  /* There are no more images in the input file */
        unsigned int imageSeq;

        /* I don't know if this works at all for multi-image PNM input.
           Before July 2000, it ignored everything after the first image,
           so this probably is at least as good -- it should be identical
           for a single-image file, which is the only kind which was legal
           before July 2000.

           Maybe there needs to be some per-file header and trailers stuff
           in the Postscript program, with some per-page header and trailer
           stuff inside.  I don't know Postscript.  - Bryan 2000.06.19.
        */

        eof = FALSE;  /* There is always at least one image */
        for (imageSeq = 0; !eof; ++imageSeq) {
            convertPage(ifp, cmdline.mustturn, cmdline.canturn, 
                        cmdline.psfilter,
                        cmdline.rle, cmdline.flate, cmdline.ascii85, 
                        cmdline.setpage, cmdline.showpage,
                        cmdline.center, cmdline.scale,
                        cmdline.dpiX, cmdline.dpiY,
                        cmdline.width, cmdline.height, 
                        cmdline.imagewidth, cmdline.imageheight, 
                        cmdline.equalpixels, name, 
                        cmdline.dict, cmdline.vmreclaim,
                        cmdline.levelSpec, cmdline.level);
            pnm_nextimage(ifp, &eof);
        }
    }
    strfree(name);

    pm_close(ifp);
    
    return 0;
}



/*
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
**
** -nocenter option added November 1993 by Wolfgang Stuerzlinger,
**  wrzl@gup.uni-linz.ac.at.
**
*/
