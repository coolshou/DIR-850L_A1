/******************************************************************************
                               ppmsvgalib
*******************************************************************************
   Display a PPM image on a Linux console using Svgalib.

   By Bryan Henderson, San Jose CA 2002.01.06.

   Contributed to the public domain.
   
******************************************************************************/

#define _XOPEN_SOURCE    /* Make sure modern signal stuff is in signal.h */
#include <stdio.h>
#include <vga.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "ppm.h"
#include "shhopt.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    unsigned int mode;
    unsigned int verbose;
};



static void
parseCommandLine (int argc, char ** argv,
                  struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc( 100*sizeof( optEntry ) );
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int modeSpec;

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "mode",         OPT_UINT,
            &cmdlineP->mode,   &modeSpec, 0);
    OPTENT3(0,   "verbose",      OPT_FLAG,
            NULL,   &cmdlineP->verbose,   0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (!modeSpec)
        pm_error("You must specify the -mode option.");

    if (argc-1 > 1)
        pm_error("Program takes at most one argument: the input file "
                 "specification.  "
                 "You specified %d arguments.", argc-1);
    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else
        cmdlineP->inputFilespec = argv[1];
}



static void
displayImage(FILE * const ifP, 
             int    const cols, 
             int    const rows,
             pixval const maxval, 
             int    const format,
             int    const originCol,
             int    const originRow) {
/*----------------------------------------------------------------------------
   Draw the PPM image which is in file 'ifP', which is positioned after the
   PPM header on the screen with its upper left corner (originCol, originRow).

   The image is 'cols' x 'rows' with maxval 'maxval' and PNM format 'format'.

   Svgalib is initialized and the mode selected.

   The image fits on the screen.
-----------------------------------------------------------------------------*/
    unsigned int svgalibMaxval = 255;
        /* This is the maxval for intensity values passed to Svgalib */
    unsigned int row;
    pixel * pixelrow;

    pixelrow = ppm_allocrow(cols);

    /* Implementation note:  It might be faster to use 
       vga_drawscansegment() instead of vga_drawpixel()
    */

    for (row = 0; row < rows; ++row) {
        unsigned int col;
        ppm_readppmrow(ifP, pixelrow, cols, maxval, format);
        for (col = 0; col < cols; ++col) {
            pixel const p = pixelrow[col];
            int const red = PPM_GETR(p) * svgalibMaxval / maxval;
            int const grn = PPM_GETG(p) * svgalibMaxval / maxval;
            int const blu = PPM_GETB(p) * svgalibMaxval / maxval;

            vga_setrgbcolor(red, grn, blu);
            vga_drawpixel(originCol + col, originRow + row);
        }
    }
    ppm_freerow(pixelrow);
}



static void
sigintHandler(int const signal) {
/*----------------------------------------------------------------------------
   This is a signal handler for the SIGINT signal (Control-C).

   It does nothing; The handler exists only to replace the default action,
   which is to terminate the process.  Though the handler does nothing,
   the signal still causes the wait() system call, assuming it's in progress,
   to terminate so that this program can terminate.
-----------------------------------------------------------------------------*/
}



static void 
waitforSigint(void) {

    struct sigaction oldsigaction;
    struct sigaction newsigaction;
    int rc;
    
    newsigaction.sa_handler = &sigintHandler;
    sigemptyset(&newsigaction.sa_mask);
    newsigaction.sa_flags = 0;
    rc = sigaction(SIGINT, &newsigaction, &oldsigaction);
    if (rc != 0)
        pm_error("Unable to set up SIGINTR signal handler.  Errno=%d (%s)",
                 errno, strerror(errno));

    pause();  /* Wait for a signal, e.g. control-C */

    sigaction(SIGINT, &oldsigaction, NULL);
}



static void
display(FILE * const ifP, 
        int    const cols, 
        int    const rows, 
        pixval const maxval, 
        int    const format, 
        int    const videoMode, 
        bool   const verbose) {

    int xmax, ymax;
    vga_modeinfo *modeinfo;

    modeinfo = vga_getmodeinfo(videoMode);
    
    if (verbose) {
        pm_message("Screen Width: %d  Height: %d  Colors: %d",
                   modeinfo->width,
                   modeinfo->height,
                   modeinfo->colors);
        pm_message("DisplayStartRange: %xh  Maxpixels: %d  Blit: %s",
                   modeinfo->startaddressrange,
                   modeinfo->maxpixels,
                   modeinfo->haveblit ? "YES" : "NO");
    }

    if (modeinfo->colors <= 256)
        pm_error("This video mode has %d or fewer colors, which means "
                 "it is colormapped (aka paletted, aka pseudocolor).  "
                 "This program cannot drive colormapped modes.", 
                 modeinfo->colors);

    if (cols > modeinfo->width)
        pm_error("Image is too wide (%d columns) for screen (%d columns).  "
                 "Use Pamcut to select part to display.", 
                 cols, modeinfo->width);
    if (rows > modeinfo->height)
        pm_error("Image is too tall (%d rows) for screen (%d rows).  "
                 "Use Pamcut to select part to display.",
                 rows, modeinfo->height);
    
    /* The program must not terminate after we set the video mode and before
       we reset it to text mode.  Note that vga_setmode() sets up handlers
       for signals such as SIGINT that attempt to restore modes and then exit
       the program.
    */

    vga_setmode(videoMode);

    vga_screenoff();

    xmax = vga_getxdim() - 1;
    ymax = vga_getydim() - 1;

    /* Draw white border */

    vga_setcolor(vga_white());
    vga_drawline(0, 0, xmax, 0);
    vga_drawline(xmax, 0, xmax, ymax);
    vga_drawline(xmax, ymax, 0, ymax);
    vga_drawline(0, ymax, 0, 0);

    vga_screenon();

    {
        int const originCol = (modeinfo->width - cols) / 2;
        int const originRow = (modeinfo->height - rows) / 2;
        displayImage(ifP, cols, rows, maxval, format, originCol, originRow);
    }

    waitforSigint();

    vga_setmode(TEXT);
}



int 
main(int argc, char *argv[]) {

    FILE * ifP;
    struct cmdlineInfo cmdline;
    int cols, rows;
    pixval maxval;
    int format;
    int rc;

    ppm_init( &argc, argv );

    rc = vga_init();         /* Initialize. */
    if (rc < 0)
        pm_error("Svgalib unable to allocate a virtual console.");

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    ppm_readppminit(ifP, &cols, &rows, &maxval, &format);

    {
        enum pm_check_code checkResult;
        ppm_check(ifP, PM_CHECK_BASIC, format, cols, rows, maxval, 
                  &checkResult);
    }

    if (vga_hasmode(cmdline.mode))
        display(ifP, cols, rows, maxval, format, 
                cmdline.mode, cmdline.verbose);
    else {
        pm_error("Svgalib video mode #%d not available.  Either the "
                 "video controller isn't capable of that mode or the "
                 "Svgalib video driver doesn't know how to use it.",
                 cmdline.mode);
    }

    pm_close(ifP);

    return 0;
}
