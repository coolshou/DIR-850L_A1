/*****************************************************************************
                               pamtodjvurle
******************************************************************************
  This program converts a PAM image to DjVu Color RLE format.
  
  By Bryan Henderson, San Jose, CA April 2004.

  Contributed to the public domain by its author.

  This work is inspired by Ppmtodjvurle, written by Scott Pakin
  <scott+pbm@pakin.org> in March 2004.  Bryan took the requirements of
  the DjVu Color RLE format and the technique for generating the format
  (but not code) from that program.

*****************************************************************************/
#include <stdio.h>
#include <assert.h>

#include "pam.h"
#include "pammap.h"
#include "shhopt.h"


struct cmdlineInfo {
    const char * inputFilespec;
    const char * transparent;
    unsigned int showcolormap;
};



static void 
parseCommandLine(int argc, 
                 char ** argv, 
                 struct cmdlineInfo  * const cmdlineP) {
/* --------------------------------------------------------------------------
   Parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
--------------------------------------------------------------------------*/
    optEntry *option_def = malloc( 100*sizeof( optEntry ) );
    /* Instructions to optParseOptions3 on how to parse our options. */
    optStruct3 opt;
  
    unsigned int option_def_index;
    unsigned int transparentSpec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "transparent",   OPT_STRING, &cmdlineP->transparent, 
            &transparentSpec,        0);
    OPTENT3(0, "showcolormap",  OPT_FLAG, NULL,
            &cmdlineP->showcolormap,        0);
  
    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;   /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
    /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (!transparentSpec)
        cmdlineP->transparent = "white";

    /* Get the program parameters */

    if (argc-1 >= 1)
        cmdlineP->inputFilespec = argv[1];
    else
        cmdlineP->inputFilespec = "-";
    
    if (argc-1 > 1)
        pm_error("Program takes at most one argument:  the file name.  "
                 "You specified %d", argc-1);
}



static void
computeColorMap(struct pam *   const pamP,
                tuple **       const tupleArray,
                unsigned int * const numColorsP,
                tupletable *   const colormapP,
                tuplehash *    const colorhashP,
                bool           const show) {

    unsigned int numColors;
    tupletable colormap;

    colormap = pnm_computetuplefreqtable(pamP, tupleArray, 0, &numColors);
    if (numColors > 0xFF0)
        pm_error("too many colors; "
                 "use pnmquant to reduce to no more than %u colors", 0xFF0);
    
    if (show) {
        unsigned int colorIndex;
        fprintf(stderr, "Color map:\n");
        fprintf(stderr, "    Index Color\n");
        for (colorIndex = 0; colorIndex < numColors; ++colorIndex) {
            unsigned int plane;
            fprintf(stderr, "    %5u   ", colorIndex);
            for (plane = 0; plane < pamP->depth; ++plane)
                fprintf(stderr, "%3lu ", colormap[colorIndex]->tuple[plane]);
            fprintf(stderr, "\n");
        }
    }

    *colorhashP = pnm_computetupletablehash(pamP, colormap, numColors);

    *numColorsP = numColors;
    *colormapP  = colormap;
}



static void
makeDjvurleHeader(FILE *       const ofP,
                  struct pam * const pamP,
                  unsigned int const numColors,
                  tupletable   const colormap) {
    
    unsigned int colorIndex;

    fprintf(ofP, "R6\n");
    fprintf(ofP, "%d %d %d\n", pamP->width, pamP->height, numColors);

    for (colorIndex = 0; colorIndex < numColors; ++colorIndex) {
        sample red, grn, blu;

        if (pamP->depth >= 3) {
            red = colormap[colorIndex]->tuple[PAM_RED_PLANE];
            grn = colormap[colorIndex]->tuple[PAM_GRN_PLANE];
            blu = colormap[colorIndex]->tuple[PAM_BLU_PLANE];
        } else
            red = grn = blu = colormap[colorIndex]->tuple[0];
        
        fputc(pnm_scalesample(red, pamP->maxval, 255), ofP);
        fputc(pnm_scalesample(grn, pamP->maxval, 255), ofP);
        fputc(pnm_scalesample(blu, pamP->maxval, 255), ofP);
    }
}



static bool
colorEqual(tuple        comparand,
           unsigned int comparandDepth,
           tuple        comparator) {

    /* comparator has depth 3 */

    if (comparandDepth >= 3)
        return (comparand[0] == comparator[0] &&
                comparand[1] == comparator[1] &&
                comparand[2] == comparator[2]);
    else
        return (comparand[0] == comparator[0] &&
                comparand[0] == comparator[1] &&
                comparand[0] == comparator[2]);
}



static void 
writeRleRun(FILE *       const ofP, 
            struct pam * const pamP,
            tuple        const color, 
            int          const count,
            tuplehash    const colorhash,
            tuple        const transcolor) {
/*----------------------------------------------------------------------------
  Write one DjVu Color RLE run to the file 'ofP'.  The run is 
  'count' pixels of color 'color', using the color index given by
  'colorhash' and assuming 'transcolor' is the transparent color.
  
  'transcolor' is a 3-deep tuple with the same maxval as the image.
-----------------------------------------------------------------------------*/
    uint32n rlevalue;         /* RLE-encoded color/valuex */
    int index;

    if (count > 0) {
        if (colorEqual(color, pamP->depth, transcolor))
            index = 0xFFF;
        else {
            int found;
            pnm_lookuptuple(pamP, colorhash, color, &found, &index);
            assert(found);
        }
        rlevalue = (index << 20) | count;
      
        pm_writebiglong(ofP, rlevalue);
    }
}



static void
writeDjvurleRow(FILE *       const ofP,
                struct pam * const pamP,
                tuple *      const tupleRow,
                tuplehash    const colorhash,
                tuple        const transcolor) {

    unsigned int col;
    unsigned int runlength;
    tuple prevpixel;        /* Previous pixel seen */

    prevpixel = tupleRow[0];
    runlength = 0;
    
    for (col = 0; col < pamP->width; ++col) {
        tuple const newpixel = tupleRow[col];      /* Current pixel color */
        
        if (pnm_tupleequal(pamP, newpixel, prevpixel))
            /* This is a continuation of the current run */
            ++runlength;
          else {
              /* The run is over.  Write it out and start a run of the next
                 color.
              */
              writeRleRun(ofP, pamP, prevpixel, runlength, 
                          colorhash, transcolor);
              runlength = 1;
              prevpixel = newpixel;
          }
        if (runlength >= (1<<20)-1) {
            /* Can't make the run any longer.  Write it out and start a
               new run.
            */
            writeRleRun(ofP, pamP, prevpixel, runlength, 
                        colorhash, transcolor);
            runlength = 1;
        }
    }
    /* Write the last run we started */
    writeRleRun(ofP, pamP, prevpixel, runlength, colorhash, transcolor);
}



int 
main(int argc, char *argv[]) {

    FILE * const rlefile = stdout;

    struct cmdlineInfo cmdline;
    FILE *ifP;                 /* Input (Netpbm) file */
    struct pam pam;            /* Description of the image */
    tuple ** tupleArray;       /* The image raster */
    tupletable colormap;       /* List of all of the colors used */
    unsigned int numColors;    /* Number of unique colors in the color map */
    tuplehash colorhash; 
        /* Mapping from color to index into colormap[] */
    tuple transcolor;
        /* Color that should be considered transparent */

    pnm_init (&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    tupleArray = pnm_readpam(ifP, &pam, PAM_STRUCT_SIZE(tuple_type));

    transcolor = pnm_parsecolor(cmdline.transparent, pam.maxval);
    
    computeColorMap(&pam, tupleArray, &numColors, &colormap, &colorhash,
                    cmdline.showcolormap);
    
    makeDjvurleHeader(rlefile, &pam, numColors, colormap);

    /* Write the raster */

    {
        unsigned int row;
        for (row = 0; row < pam.height; ++row)
            writeDjvurleRow(rlefile, &pam, tupleArray[row], colorhash, 
                            transcolor);
    }
    /* Clean up */
    
    pnm_freepamarray(tupleArray, &pam);
    pnm_freetupletable(&pam, colormap);
    pnm_destroytuplehash(colorhash);
    pnm_freepamtuple(transcolor);
    pm_close(ifP);

    return 0;
}
