/* pamtouil.c - convert PBM, PGM, PPM, or PPM+alpha to Motif UIL icon file
**
** Bryan Henderson converted ppmtouil to pamtouil on 2002.05.04.
**
** Jef Poskanzer derived pamtouil from ppmtoxpm, which is
** Copyright (C) 1990 by Mark W. Snitily.
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
#include <ctype.h>
#include <string.h>
#include "pam.h"
#include "pammap.h"
#include "colorname.h"
#include "shhopt.h"
#include "nstring.h"

/* Max number of colors allowed in ppm input. */
#define MAXCOLORS 256

/* Lower bound and upper bound of character-pixels printed in UIL output. */
#define LOW_CHAR '`'
#define HIGH_CHAR '~'

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input files */
    char *outname;         /* output filename, less "_icon" */
    unsigned int verbose;
};



typedef struct {    /* character-pixel mapping */
    const char* cixel;    /* character string printed for pixel */
    const char* rgbname;  /* ascii rgb color, either mnemonic or #rgb value */
    const char* uilname;  /* same, with spaces replaced by underbars */
    bool        transparent;
} cixel_map;



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.  The outname array is in newly
   malloc'ed storage.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc( 100*sizeof( optEntry ) );
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    unsigned int outnameSpec;
    const char *outnameOpt;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "name",       OPT_STRING, &outnameOpt, 
            &outnameSpec,       0);
    OPTENT3(0, "verbose",    OPT_FLAG,   NULL, 
            &cmdlineP->verbose, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 == 0)
        cmdlineP->inputFilespec = "-";  /* stdin */
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else 
        pm_error("Too many arguments (%d).  "
                 "Only need one: the input filespec", argc-1);

    if (outnameSpec) {
        char * barPos;

        cmdlineP->outname = strdup(outnameOpt);

        /* Remove trailing "_icon" */
        barPos = strrchr(cmdlineP->outname, '_');
        if (barPos && STREQ(barPos, "_icon")) 
            *barPos = '\0';
    } else {
        if (STREQ(cmdlineP->inputFilespec, "-"))
            cmdlineP->outname = strdup("noname");
        else {
            char * dotPos;

            cmdlineP->outname = strdup(cmdlineP->inputFilespec);

            /* remove extension */
            dotPos = strrchr(cmdlineP->outname, '.');
            if (dotPos)
                *dotPos = '\0';
        }
    }
}




static char*
genNumstr(int  const number, 
          int  const base, 
          char const low_char, 
          int  const digits ) {
/*----------------------------------------------------------------------------
  Generate a string 'digits' characters long in newly malloc'ed
  storage that is the number 'number' displayed in base 'base'.  Fill it on
  the left with zero digits and truncate on the left if it doesn't fit.

  Use the characters 'low_char' to 'low_char' + 'base' to represent the
  digits 0 to 'base'. 
-----------------------------------------------------------------------------*/
    char* str;
    char* p;
    int d;
    int i;

    /* Allocate memory for printed number.  Abort if error. */
    str = (char*) malloc(digits + 1);
    if (str == NULL)
        pm_error("out of memory allocating number string");

    /* Generate characters starting with least significant digit. */
    i = number;
    p = str + digits;
    *p-- = '\0';    /* nul terminate string */
    while (p >= str) {
        d = i % base;
        i /= base;
        *p-- = low_char + d;
    }
    return str;
}



static const char * 
uilName(const char * const rgbname, bool const transparent) {
/*----------------------------------------------------------------------------
   Return a string in newly malloc'ed storage which is an appropriate 
   color name for the UIL palette.  It is the same as the rgb name,
   except that blanks are replaced by underscores, and if 'transparent'
   is true, it is "background color".  The latter is a strange name of
   a color, but it works pretty much the same in the UIL colortable() value.
-----------------------------------------------------------------------------*/
    char * output;

    if (transparent)
        output = strdup("background color");
    else {
        int i;

        output = malloc(strlen(rgbname) + 5 + 1);
        if (output == NULL)
            pm_error( "out of memory allocating color name" );
        
        for (i = 0; i < strlen(rgbname); ++i) {
            if (rgbname[i] == ' ')
                output[i] = '_';
            else
                output[i] = rgbname[i];
        }
        output[strlen(rgbname)] = '\0';
    }

    return output;
}



static void
genCmap(struct pam *   const pamP,
        tupletable     const chv, 
        unsigned int   const ncolors, 
        cixel_map            cmap[MAXCOLORS], 
        unsigned int * const charsppP,
        bool           const verbose) {

    unsigned int const base = (int) HIGH_CHAR - (int) LOW_CHAR + 1;
    char* colorname;
    unsigned int colorIndex;
    {
        /* Figure out how many characters per pixel we'll be using.
        ** Don't want to be forced to link with libm.a, so using a
        ** division loop rather than a log function.  
        */
        unsigned int i;
        for (*charsppP = 0, i = ncolors; i > 0; ++(*charsppP))
            i /= base;
    }

    /* Generate the character-pixel string and the rgb name for each colormap
    ** entry.
    */
    for (colorIndex = 0; colorIndex < ncolors; ++colorIndex) {
        bool nameAlreadyInCmap;
        unsigned int indexOfName;
        bool transparent;
        int j;
        
        if (pamP->depth-1 < PAM_TRN_PLANE)
            transparent = FALSE;
        else 
            transparent = 
                chv[colorIndex]->tuple[PAM_TRN_PLANE] < pamP->maxval/2;

        /* Generate color name string. */
        colorname = pam_colorname(pamP, chv[colorIndex]->tuple, 
                                  PAM_COLORNAME_ENGLISH);

        /* We may have already assigned a character code to this color
           name/transparency because the same color name can apply to
           two different colors because we said we wanted the closest
           matching color that has an English name, and we recognize
           only one transparent color.  If that's the case, we just
           make a cross-reference.  
        */
        nameAlreadyInCmap = FALSE;   /* initial assumption */
        for (j = 0; j < colorIndex; ++j) {
            if (cmap[j].rgbname != NULL && 
                STREQ(colorname, cmap[j].rgbname) &&
                cmap[j].transparent == transparent) {
                nameAlreadyInCmap = TRUE;
                indexOfName = j;
            }
        }
        if (nameAlreadyInCmap) {
            /* Make the entry a cross-reference to the earlier entry */
            cmap[colorIndex].uilname = NULL;
            cmap[colorIndex].rgbname = NULL;
            cmap[colorIndex].cixel = cmap[indexOfName].cixel;
        } else {
            cmap[colorIndex].uilname = uilName(colorname, transparent);
            cmap[colorIndex].rgbname = strdup(colorname);
            if (cmap[colorIndex].rgbname == NULL)
                pm_error( "out of memory allocating color name" );
            
            cmap[colorIndex].transparent = transparent;
            
            /* Generate color value characters. */
            cmap[colorIndex].cixel = 
                genNumstr(colorIndex, base, LOW_CHAR, *charsppP);
            if (verbose)
                pm_message("Adding color '%s' %s = '%s' to UIL colormap",
                           cmap[colorIndex].rgbname, 
                           cmap[colorIndex].transparent ? "TRANS" : "OPAQUE",
                           cmap[colorIndex].cixel);
        }
    }
}



static void
writeUilHeader(const char * const outname) {
    /* Write out the UIL header. */
    printf( "module %s\n", outname );
    printf( "version = 'V1.0'\n" );
    printf( "names = case_sensitive\n" );
    printf( "include file 'XmAppl.uil';\n" );
}



static void
writeColormap(const char * const outname, 
              cixel_map          cmap[MAXCOLORS], 
              unsigned int const ncolors) {

    {
        int i;

        /* Write out the colors. */
        printf("\n");
        printf("value\n");
        for (i = 0; i < ncolors; ++i)
            if (cmap[i].uilname != NULL && !cmap[i].transparent) 
                printf("    %s : color( '%s' );\n", 
                       cmap[i].uilname, cmap[i].rgbname );
    }
    {
        /* Write out the ascii colormap. */

        int i;
        bool printedOne;

        printf("\n");
        printf("value\n");
        printf("  %s_rgb : color_table (\n", outname);
        printedOne = FALSE; 
        for (i = 0; i < ncolors; ++i)
            if (cmap[i].uilname != NULL) {
                if (printedOne)
                    printf(",\n");
                printf("    %s = '%s'", cmap[i].uilname, cmap[i].cixel);
                printedOne = TRUE;
            }     
        printf("\n");
        printf("    );\n");
    }
}



static void
writeRaster(struct pam *  const pamP, 
            tuple **      const tuples,
            const char *  const outname,
            cixel_map           cmap[MAXCOLORS], 
            unsigned int  const ncolors,
            tuplehash     const cht, 
            unsigned int  const charspp) {
    
    int row;
    /* Write out the ascii character-pixel image. */

    printf("\n");
    printf("%s_icon : exported icon( color_table = %s_rgb,\n",
           outname, outname);
    for (row = 0; row < pamP->height; ++row) {
        int col;

        printf("    '");
        for (col = 0; col < pamP->width; ++col) {
            int colorIndex;
            int found;
            if ((col * charspp) % 70 == 0 && col > 0)
                printf( "\\\n" );       /* line continuation */
            pnm_lookuptuple(pamP, cht, tuples[row][col], &found, &colorIndex);
            if (!found)
                pm_error("INTERNAL ERROR: color not found in colormap");
            printf("%s", cmap[colorIndex].cixel);
        }
        if (row != pamP->height - 1)
            printf("',\n");
        else
            printf("'\n"); 
    }
    printf(");\n");
    printf("\n");
}


static void
freeString(const char * const s) {
/*----------------------------------------------------------------------------
   This is just free(), but with type checking for const char *.
-----------------------------------------------------------------------------*/
    free((void *)s);
}



static void
freeCmap(cixel_map cmap[], unsigned int const ncolors) {

    int i;

    for (i = 0; i < ncolors; ++i) {
        cixel_map const cmapEntry = cmap[i];
        if (cmapEntry.uilname)
            freeString(cmapEntry.uilname);
        if (cmapEntry.rgbname)
            freeString(cmapEntry.rgbname);
    }
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    struct pam pam;   /* Input PAM image */
    FILE* ifP;
    tuple** tuples;
    unsigned int ncolors;
    tuplehash cht;
    tupletable chv;
    cixel_map cmap[MAXCOLORS];
    unsigned int charspp;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);
    tuples = pnm_readpam(ifP, &pam, PAM_STRUCT_SIZE(tuple_type));
    pm_close(ifP);

    pm_message("computing colormap...");

    chv = pnm_computetuplefreqtable(&pam, tuples, MAXCOLORS, &ncolors);
    if (chv == NULL)
        pm_error("too many colors - try doing a 'pnmquant %u'", MAXCOLORS);
    if (cmdline.verbose)
        pm_message("%u colors found", ncolors);

    /* Make a hash table for fast color lookup. */
    cht = pnm_computetupletablehash(&pam, chv, ncolors);

    /* Now generate the character-pixel colormap table. */
    pm_message("looking up color names, assigning character codes...");
    genCmap(&pam, chv, ncolors, cmap, &charspp, cmdline.verbose);

    pm_message("generating UIL...");
    writeUilHeader(cmdline.outname);

    writeColormap(cmdline.outname, cmap, ncolors);

    writeRaster(&pam, tuples, cmdline.outname, cmap, ncolors, cht, charspp);

    printf("end module;\n");

    free(cmdline.outname);
    freeCmap(cmap, ncolors);

    return 0;
}
