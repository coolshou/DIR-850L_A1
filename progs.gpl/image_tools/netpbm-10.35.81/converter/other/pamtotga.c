/* pamtotga.c - read a portable pixmap and produce a TrueVision Targa file
**
** Copyright (C) 1989, 1991 by Mark Shand and Jef Poskanzer
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#define _BSD_SOURCE  /* Make sure string.h contains strdup() */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <string.h>

#include "pam.h"
#include "pammap.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"
#include "tga.h"

/* Max number of colors allowed for colormapped output. */
#define MAXCOLORS 256

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    char *outName;
    enum TGAbaseImageType imgType;
    bool defaultFormat;
    unsigned int norle;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Parse the program arguments (given by argc and argv) into a form
   the program can deal with more easily -- a cmdline_info structure.
   If the syntax is invalid, issue a message and exit the program via
   pm_error().

   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optStruct3 opt;  /* set by OPTENT3 */
    optEntry *option_def = malloc(100*sizeof(optEntry));
    unsigned int option_def_index;

    unsigned int outNameSpec;
    unsigned int cmap, mono, rgb;

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "name",       OPT_STRING, 
            &cmdlineP->outName, &outNameSpec, 0);
    OPTENT3(0,   "cmap",       OPT_FLAG, 
            NULL, &cmap, 0);
    OPTENT3(0,   "mono",       OPT_FLAG, 
            NULL, &mono, 0);
    OPTENT3(0,   "rgb",        OPT_FLAG, 
            NULL, &rgb, 0);
    OPTENT3(0,   "norle",      OPT_FLAG, 
            NULL, &cmdlineP->norle, 0);
    
    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (cmap + mono + rgb > 1)
        pm_error("You may specify only one of -cmap, -mono, and -rgb.");

    if (cmap + mono + rgb == 0)
        cmdlineP->defaultFormat = TRUE;
    else {
        cmdlineP->defaultFormat = FALSE;
    
        if (cmap)
            cmdlineP->imgType = TGA_MAP_TYPE;
        else if (mono)
            cmdlineP->imgType = TGA_MONO_TYPE;
        else if (rgb)
            cmdlineP->imgType = TGA_RGB_TYPE;
    }

    if (!outNameSpec)
        cmdlineP->outName = NULL;
    
    if (argc-1 == 0) 
        cmdlineP->inputFilespec = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->inputFilespec = argv[1];

}


static void
writeTgaHeader(struct ImageHeader const tgaHeader) {

    unsigned char flags;

    putchar(tgaHeader.IdLength);
    putchar(tgaHeader.CoMapType);
    putchar(tgaHeader.ImgType);
    putchar(tgaHeader.Index_lo);
    putchar(tgaHeader.Index_hi);
    putchar(tgaHeader.Length_lo);
    putchar(tgaHeader.Length_hi);
    putchar(tgaHeader.CoSize);
    putchar(tgaHeader.X_org_lo);
    putchar(tgaHeader.X_org_hi);
    putchar(tgaHeader.Y_org_lo);
    putchar(tgaHeader.Y_org_hi);
    putchar(tgaHeader.Width_lo);
    putchar(tgaHeader.Width_hi);
    putchar(tgaHeader.Height_lo);
    putchar(tgaHeader.Height_hi);
    putchar(tgaHeader.PixelSize);
    flags = (tgaHeader.AttBits & 0xf) | 
        ((tgaHeader.Rsrvd & 0x1) << 4) |
        ((tgaHeader.OrgBit & 0x1) << 5) | 
        ((tgaHeader.OrgBit & 0x3) << 6);
    putchar(flags);

    if (tgaHeader.IdLength > 0)
        fwrite(tgaHeader.Id, 1, (int) tgaHeader.IdLength, stdout);
}
    


static void
putPixel(struct pam *          const pamP,
         tuple                 const tuple, 
         enum TGAbaseImageType const imgType, 
         bool                  const withAlpha,
         tuplehash             const cht) {
/*----------------------------------------------------------------------------
   Write a single pixel of the TGA raster to Standard Output.  The
   pixel is to have color 'tuple'.  The raster has format 'imgType'.
   The color palette from which the specified color is to be drawn, if
   'imgType' indicates use of a color palette, is 'cht'.
-----------------------------------------------------------------------------*/
    if (imgType == TGA_MAP_TYPE) {
        int retval;
        int found;

        pnm_lookuptuple(pamP, cht, tuple, &found, &retval);
        if (!found)
            pm_error("Internal error: color not found in map that was "
                     "generated from all the colors in the image");
        putchar(retval);
    } else {
        if (imgType == TGA_RGB_TYPE && pamP->depth < 3) {
            /* Make RGB pixel out of a single input plane */
            unsigned int plane;
            
            for (plane = 0; plane < 3; ++plane) 
                putchar(pnm_scalesample(tuple[0], 
                                        pamP->maxval, TGA_MAXVAL));
        } else if (imgType == TGA_MONO_TYPE)
            putchar(pnm_scalesample(tuple[0], 
                                    pamP->maxval, TGA_MAXVAL));
        else {
            putchar(pnm_scalesample(tuple[PAM_BLU_PLANE], 
                                    pamP->maxval, TGA_MAXVAL));
            putchar(pnm_scalesample(tuple[PAM_GRN_PLANE], 
                                    pamP->maxval, TGA_MAXVAL));
            putchar(pnm_scalesample(tuple[PAM_RED_PLANE], 
                                    pamP->maxval, TGA_MAXVAL));
            if (withAlpha)
                putchar(pnm_scalesample(tuple[PAM_TRN_PLANE], 
                                        pamP->maxval, TGA_MAXVAL));
        }
    }
}



static void
putMapEntry(struct pam * const pamP, 
            tuple        const value, 
            int          const size) {

    if (size == 15 || size == 16) {
        /* 5 bits each of red, green, and blue.  Watch for byte order */

        tuple const tuple31 = pnm_allocpamtuple(pamP);

        pnm_scaletuple(pamP, tuple31, value, 31);
        {
            int const mapentry = 
                tuple31[PAM_BLU_PLANE] << 0 |
                tuple31[PAM_GRN_PLANE] << 5 |
                tuple31[PAM_RED_PLANE] << 10;
            
            putchar(mapentry % 256);
            putchar(mapentry / 256);
        }
        pnm_freepamtuple(tuple31);
    } else if (size == 8)
        putchar(pnm_scalesample(value[0], 
                                pamP->maxval, TGA_MAXVAL));
    else {
        /* Must be 24 or 32 */
        putchar(pnm_scalesample(value[PAM_BLU_PLANE], 
                                pamP->maxval, TGA_MAXVAL));
        putchar(pnm_scalesample(value[PAM_GRN_PLANE], 
                                pamP->maxval, TGA_MAXVAL));
        putchar(pnm_scalesample(value[PAM_RED_PLANE], 
                                    pamP->maxval, TGA_MAXVAL));
        if (size == 32)
            putchar(pnm_scalesample(value[PAM_TRN_PLANE], 
                                    pamP->maxval, TGA_MAXVAL));
    }
}



static void
computeRunlengths(struct pam * const pamP, 
                   tuple *      const tuplerow, 
                   int *        const runlength) {

    int col, start;

    /* Initialize all run lengths to 0.  (This is just an error check.) */
    for (col = 0; col < pamP->width; ++col)
        runlength[col] = 0;
    
    /* Find runs of identical pixels. */
    for ( col = 0; col < pamP->width; ) {
        start = col;
        do {
            ++col;
        } while ( col < pamP->width &&
                  col - start < 128 &&
                  pnm_tupleequal(pamP, tuplerow[col], tuplerow[start]));
        runlength[start] = col - start;
    }
    
    /* Now look for runs of length-1 runs, and turn them into negative runs. */
    for (col = 0; col < pamP->width; ) {
        if (runlength[col] == 1) {
            start = col;
            while (col < pamP->width &&
                   col - start < 128 &&
                   runlength[col] == 1 ) {
                runlength[col] = 0;
                ++col;
            }
            runlength[start] = - ( col - start );
        } else
            col += runlength[col];
    }
}



static void
computeOutName(struct cmdlineInfo const cmdline, 
               const char ** const outNameP) {
    
    char * workarea;

    if (cmdline.outName)
        workarea = strdup(cmdline.outName);
    else if (streq(cmdline.inputFilespec, "-"))
        workarea = NULL;
    else {
        char * cp;
        workarea = strdup(cmdline.inputFilespec);
        cp = strchr(workarea, '.');
        if (cp != NULL)
        	*cp = '\0';	/* remove extension */
    }
    
    if (workarea == NULL)
        *outNameP = NULL;
    else {
        /* Truncate the name to fit TGA specs */
        if (strlen(workarea) > IMAGEIDFIELDMAXSIZE)
            workarea[IMAGEIDFIELDMAXSIZE] = '\0';
        *outNameP = workarea;
    }
}



static void
validateTupleType(struct pam * const pamP) {

    if (streq(pamP->tuple_type, "RGB_ALPHA")) {
        if (pamP->depth < 4)
            pm_error("Invalid depth for tuple type RGB_ALPHA.  "
                     "Should have at least 4 planes, but has %d.", 
                     pamP->depth);
    } else if (streq(pamP->tuple_type, "RGB")) {
        if (pamP->depth < 3)
            pm_error("Invalid depth for tuple type RGB.  "
                     "Should have at least 3 planes, but has %d.", 
                     pamP->depth);
    } else if (streq(pamP->tuple_type, "GRAYSCALE")) {
    } else if (streq(pamP->tuple_type, "BLACKANDWHITE")) {
    } else 
        pm_error("Invalid type of input.  PAM tuple type is '%s'.  "
                 "This programs understands only RGB_ALPHA, RGB, GRAYSCALE, "
                 "and BLACKANDWHITE.", pamP->tuple_type);
}



static void
computeImageType_cht(struct pam *            const pamP,
                     struct cmdlineInfo      const cmdline, 
                     tuple **                const tuples,
                     enum TGAbaseImageType * const baseImgTypeP,
                     bool *                  const withAlphaP,
                     tupletable *            const chvP,
                     tuplehash *             const chtP, 
                     int *                   const ncolorsP) {

    unsigned int ncolors;
    enum TGAbaseImageType baseImgType;
    bool withAlpha;

    validateTupleType(pamP);

    withAlpha = (streq(pamP->tuple_type, "RGB_ALPHA"));

    if (cmdline.defaultFormat) {
        /* default the image type */
        if (withAlpha) {
            baseImgType = TGA_RGB_TYPE;
            *chvP = NULL;
        } else if (pamP->depth > 1) {
            pm_message("computing colormap...");
            *chvP = 
                pnm_computetuplefreqtable(pamP, tuples, MAXCOLORS, &ncolors);
            if (*chvP == NULL) {
                pm_message("Too many colors for colormapped TGA.  Doing RGB.");
                baseImgType = TGA_RGB_TYPE;
            } else 
                baseImgType = TGA_MAP_TYPE;
        } else {
            baseImgType = TGA_MONO_TYPE;
            *chvP = NULL;
        }
    } else {
        baseImgType = cmdline.imgType;

        if (baseImgType == TGA_MAP_TYPE) {
            if (withAlpha)
                pm_error("Can't do a colormap because image has transparency "
                         "information");
            pm_message("computing colormap...");
            *chvP = 
                pnm_computetuplefreqtable(pamP, tuples, MAXCOLORS, &ncolors);
            if (*chvP == NULL) 
                pm_error("Too many colors for colormapped TGA.  "
                         "Use 'pnmquant %d' to reduce the number of colors.", 
                         MAXCOLORS);
        } else
            *chvP = NULL;
        if (baseImgType == TGA_MONO_TYPE && pamP->depth > 1)
            pm_error("For Mono TGA output, input must be "
                     "GRAYSCALE or BLACKANDWHITE PAM or PBM or PGM");
    }
    
    if (baseImgType == TGA_MAP_TYPE) {
        pm_message("%d colors found.", ncolors);
        /* Make a hash table for fast color lookup. */
        *chtP = pnm_computetupletablehash(pamP, *chvP, ncolors);
    } else
        *chtP = NULL;

    *baseImgTypeP = baseImgType;
    *withAlphaP   = withAlpha;
    *ncolorsP     = ncolors;
}



static void
computeTgaHeader(struct pam *          const pamP,
                 enum TGAbaseImageType const baseImgType,
                 bool                  const withAlpha,
                 bool                  const rle, 
                 int                   const ncolors,
                 unsigned char         const orgBit,
                 const char *          const id,
                 struct ImageHeader *  const tgaHeaderP) {

    if (rle) {
        switch (baseImgType ) {
        case TGA_MONO_TYPE: tgaHeaderP->ImgType = TGA_RLEMono;          break;
        case TGA_MAP_TYPE:  tgaHeaderP->ImgType = TGA_RLEMap;           break;
        case TGA_RGB_TYPE:  tgaHeaderP->ImgType = TGA_RLERGB;           break;
        }
    } else {
        switch(baseImgType) {
        case TGA_MONO_TYPE: tgaHeaderP->ImgType = TGA_Mono;          break;
        case TGA_MAP_TYPE:  tgaHeaderP->ImgType = TGA_Map;           break;
        case TGA_RGB_TYPE:  tgaHeaderP->ImgType = TGA_RGB;           break;
        }
    }
    
    if (id) {
        tgaHeaderP->IdLength = strlen(id);
        tgaHeaderP->Id = strdup(id);
    } else
        tgaHeaderP->IdLength = 0;
    tgaHeaderP->Index_lo = 0;
    tgaHeaderP->Index_hi = 0;
    if (baseImgType == TGA_MAP_TYPE) {
        tgaHeaderP->CoMapType = 1;
        tgaHeaderP->Length_lo = ncolors % 256;
        tgaHeaderP->Length_hi = ncolors / 256;
        tgaHeaderP->CoSize = 8 * pamP->depth;
    } else {
        tgaHeaderP->CoMapType = 0;
        tgaHeaderP->Length_lo = 0;
        tgaHeaderP->Length_hi = 0;
        tgaHeaderP->CoSize = 0;
    }
    switch (baseImgType) {
    case TGA_MAP_TYPE:
        tgaHeaderP->PixelSize = 8;
        break;
    case TGA_RGB_TYPE:
        tgaHeaderP->PixelSize = 8 * MAX((withAlpha ? 4: 3), pamP->depth);
        break;
    case TGA_MONO_TYPE:
        tgaHeaderP->PixelSize = 8;
    }
    tgaHeaderP->X_org_lo = tgaHeaderP->X_org_hi = 0;
    tgaHeaderP->Y_org_lo = tgaHeaderP->Y_org_hi = 0;
    tgaHeaderP->Width_lo = pamP->width % 256;
    tgaHeaderP->Width_hi = pamP->width / 256;
    tgaHeaderP->Height_lo = pamP->height % 256;
    tgaHeaderP->Height_hi = pamP->height / 256;
    tgaHeaderP->AttBits = 0;
    tgaHeaderP->Rsrvd = 0;
    tgaHeaderP->IntrLve = 0;
    tgaHeaderP->OrgBit = orgBit;
}



static void
releaseTgaHeader(struct ImageHeader const tgaHeader) {

    if (tgaHeader.IdLength > 0)
        strfree(tgaHeader.Id);
}



static void 
writeTgaRaster(struct pam *          const pamP,
               tuple **              const tuples, 
               tuplehash             const cht,
               enum TGAbaseImageType const imgType,
               bool                  const withAlpha,
               bool                  const rle,
               unsigned char         const orgBit) {

    int* runlength;  /* malloc'ed */
    int row;

    if (rle)
        MALLOCARRAY(runlength, pamP->width);

    for (row = 0; row < pamP->height; ++row) {
        int realrow;
        realrow = (orgBit != 0) ? row : pamP->height - row - 1;
        if (rle) {
            int col;
            computeRunlengths(pamP, tuples[realrow], runlength);
            for (col = 0; col < pamP->width; ) {
                if (runlength[col] > 0) {
                    putchar(0x80 + runlength[col] - 1);
                    putPixel(pamP, tuples[realrow][col], imgType, withAlpha,
                             cht);
                    col += runlength[col];
                } else if (runlength[col] < 0) {
                    int i;
                    putchar(-runlength[col] - 1);
                    for (i = 0; i < -runlength[col]; ++i)
                        putPixel(pamP, tuples[realrow][col+i], 
                                 imgType, withAlpha, cht);
                    col += -runlength[col];
                } else
                    pm_error("Internal error: zero run length");
            }
        } else {
            int col;
            for (col = 0; col < pamP->width; ++col)
                putPixel(pamP, tuples[realrow][col], imgType, withAlpha, cht);
        }
    }
    if (rle)
        free(runlength);
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    tuple ** tuples;
    struct pam pam;
    int ncolors;
    tupletable chv;
    tuplehash cht;
    struct ImageHeader tgaHeader;
    enum TGAbaseImageType baseImgType;
    bool withAlpha;
    const char *outName;

    pnm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    computeOutName(cmdline, &outName);

    tuples = pnm_readpam(ifP, &pam, PAM_STRUCT_SIZE(tuple_type));
    pm_close(ifP);

    computeImageType_cht(&pam, cmdline, tuples, 
                         &baseImgType, &withAlpha, &chv, &cht, &ncolors);

    /* Do the Targa header */
    computeTgaHeader(&pam, baseImgType, withAlpha, !cmdline.norle,
                     ncolors, 0, outName, &tgaHeader);
    writeTgaHeader(tgaHeader);
    
    if (baseImgType == TGA_MAP_TYPE) {
        /* Write out the Targa colormap. */
        int i;
        for (i = 0; i < ncolors; ++i)
            putMapEntry(&pam, chv[i]->tuple, tgaHeader.CoSize);
    }

    writeTgaRaster(&pam, tuples, cht, baseImgType, withAlpha, 
                   !cmdline.norle, 0);

    if (cht)
        pnm_destroytuplehash(cht);
    if (chv)
        pnm_freetupletable(&pam, chv);

    releaseTgaHeader(tgaHeader);
    strfree(outName);
    pnm_freepamarray(tuples, &pam);

    return 0;
}
