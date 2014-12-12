#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* '-' if stdin */
    const char *transparent;  /* NULL if none */
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

    unsigned int transparentSpec;

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "verbose",     OPT_FLAG,   NULL,                  
            &cmdlineP->verbose,       0 );
    OPTENT3(0, "transparent", OPT_STRING, &cmdlineP->transparent,
            &transparentSpec,  0 );

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (!transparentSpec)
        cmdlineP->transparent = NULL;

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else
        pm_error("Too many arguments.  Program takes at most one argument: "
                 "input file name");
}




static void
pripix(struct pam * const pamP,
       tuple        const color, 
       unsigned int const rectWidth, 
       unsigned int const rectHeight, 
       tuple        const transparentColor) {

    if (rectWidth > 0 && rectHeight > 0) {
        printf("<TD VALIGN=CENTER ALIGN=CENTER");

        if (rectWidth != 1) 
            printf(" COLSPAN=%d", rectWidth);
        if (rectHeight != 1) 
            printf(" ROWSPAN=%d", rectHeight);

        if (transparentColor && 
            !pnm_tupleequal(pamP, color,transparentColor)) {
            /* No BGCOLOR attribute */
        } else {
            tuple const colorff = pnm_allocpamtuple(pamP);

            unsigned int r, g, b;

            pnm_scaletuple(pamP, colorff, color, 0xff);

            if (pamP->depth < 3)
                r = g = b = colorff[0];
            else {
                r = color[PAM_RED_PLANE];
                g = color[PAM_GRN_PLANE];
                b = color[PAM_BLU_PLANE];
            }
            printf(" BGCOLOR=#%02X%02X%02X", r, g, b);
        }
        printf(">");
        printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0>"
               "<TR><TD></TD></TR></TABLE>");
        printf("</TD>\n");
    }
}



static void
findSameColorRectangle(struct pam *   const pamP,
                       tuple **       const tuples,
                       unsigned int   const row,
                       unsigned int   const col,
                       unsigned int * const rectWidthP,
                       unsigned int * const rectHeightP) {
/*----------------------------------------------------------------------------
   Find the largest rectangle, in the image described by 'pam' and 
   'tuples', of uniform color, whose upper left corner is at (row, col).

   Return the width and height of that rectangle as *rectWidthP
   and *rectHeightP.
-----------------------------------------------------------------------------*/
    tuple const rectangleColor = tuples[row][col];

    unsigned int i;
    unsigned int mx, my;
    unsigned int cnx, cny;

    mx=0; my=0;
    cnx = pamP->width - col; cny = pamP->height - row;

    for (i=0; (!mx)||(!my); i++) {
        int j;
        /*fprintf(stderr,"\n[%d]",i);*/
        for (j=0; j<=i; j++) {
            if (!my) {
                if (i>=cny) 
                    my=cny;
                else
                    if (((!mx) || (j<mx)) && (j < cnx)) {
                        /*fprintf(stderr,"%d/%d ",j,i);*/
                        if (!pnm_tupleequal(pamP, tuples[row+i][col+j],
                                            rectangleColor)) 
                            my = i;
                    }
            }
            if (!mx) {
                if (i>=cnx) 
                    mx=cnx;
                else
                    if (((!my) || (j<my)) && (j < cny)) {
                        /*fprintf(stderr,"%d/%d ",i,j);*/
                        if (!pnm_tupleequal(pamP, tuples[row+j][col+i],
                                            rectangleColor)) 
                            mx = i;
                    }
            }
        }
    }
    *rectWidthP  = mx;
    *rectHeightP = my;
}



static bool **
allocOutputtedArray(unsigned int const width, unsigned int const height) {

    bool ** outputted;
    unsigned int row;

    MALLOCARRAY(outputted, height);
    if (outputted == NULL)
        pm_error("Unable to allocate space for 'outputted' array");

    for (row = 0; row < height; ++row) {
        unsigned int col;

        MALLOCARRAY(outputted[row], width);
        if (outputted[row] == NULL)
            pm_error("Unable to allocate space for 'outputted' array");

        for (col = 0; col < width ; ++col)
          outputted[row][col] = FALSE;

    }
    return outputted;
}



static void
freeOutputtedArray(bool ** const outputted, unsigned int const height) {

    unsigned int row;

    for (row = 0; row < height; ++row)
        free(outputted[row]);
}



static void
markOutputted(bool ** const outputted,
              unsigned int const upperLeftCol,
              unsigned int const upperLeftRow,
              unsigned int const width,
              unsigned int const height) {

    unsigned int const lowerRightCol = upperLeftCol + width;
    unsigned int const lowerRightRow = upperLeftRow + height;
    unsigned int row;
    
    for (row = upperLeftRow; row < lowerRightRow; ++row) {
        unsigned int col;
        for (col = upperLeftCol; col < lowerRightCol; ++col) 
            outputted[row][col] = TRUE;
    }
}



int
main(int argc, char **argv) {
    struct cmdlineInfo cmdline;
    FILE * ifP;
    struct pam inpam;
    tuple ** tuples;
    int row;
    unsigned int rectWidth, rectHeight;
    bool ** outputted;
    tuple transparentColor;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    tuples = pnm_readpam(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    if (cmdline.transparent) {
        pixel transcolor;
        transcolor = ppm_parsecolor(cmdline.transparent, inpam.maxval);
        transparentColor = pnm_allocpamtuple(&inpam);
        transparentColor[PAM_RED_PLANE] = PPM_GETR(transcolor);
        transparentColor[PAM_GRN_PLANE] = PPM_GETG(transcolor);
        transparentColor[PAM_BLU_PLANE] = PPM_GETB(transcolor);
    } else
        transparentColor = NULL;

    outputted = allocOutputtedArray(inpam.width, inpam.height);

    printf("<TABLE WIDTH=%d HEIGHT=%d BORDER=0 CELLSPACING=0 CELLPADDING=0>\n",
           inpam.width, inpam.height);
    for (row = 0; row < inpam.height; ++row) {
        int col;
        printf("<TR>\n");
        pripix(&inpam, tuples[row][0], 1, 1, transparentColor); 
        markOutputted(outputted, 0, row, 1, 1);

        for (col = 1; col < inpam.width; ++col) {
            if (!outputted[row][col]) {
                findSameColorRectangle(&inpam, tuples, row, col, 
                                       &rectWidth, &rectHeight);
                if (cmdline.verbose)
                    pm_message("[%d/%d] [%d/%d]",
                               col, row, rectWidth, rectHeight);
                pripix(&inpam, tuples[row][col], rectWidth, rectHeight, 
                       transparentColor);
                markOutputted(outputted, col, row, rectWidth, rectHeight);
            }
        }
        printf("</TR>\n");
    }
    printf("</TABLE>\n");

    if (transparentColor)
        pnm_freepamtuple(transparentColor);
    pnm_freepamarray(tuples, &inpam);
    freeOutputtedArray(outputted, inpam.height);

    exit(0);
}
