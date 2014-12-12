/* main.c: main driver for autotrace -- convert bitmaps to splines. */

#include <string.h>
#include <assert.h>
#include <math.h>

#include "mallocvar.h"
#include "nstring.h"
#include "shhopt.h"
#include "pam.h"

#include "autotrace.h"
#include "message.h"
#include "logreport.h"
#include "output-svg.h"
#include "bitmap.h"

#define dot_printer_max_column 50
#define dot_printer_char '|'



static void
readImageToBitmap(FILE *            const ifP,
                  at_bitmap_type ** const bitmapPP) {

    at_bitmap_type * bitmapP;
    struct pam pam;
    tuple ** tuples;
    unsigned int row;
    tuple * row255;

    MALLOCVAR_NOFAIL(bitmapP);

    tuples = pnm_readpam(ifP, &pam, PAM_STRUCT_SIZE(tuple_type));

    bitmapP->width  = pam.width;
    bitmapP->height = pam.height;
    bitmapP->np     = pam.depth;

    MALLOCARRAY(bitmapP->bitmap, pam.width * pam.height * pam.depth);

    row255 = pnm_allocpamrow(&pam);

    for (row = 0; row < pam.height; ++row) {
        unsigned int col;

        pnm_scaletuplerow(&pam, row255, tuples[row], 255);
        
        for (col = 0; col < pam.width; ++col) {
            unsigned int plane;

            for (plane = 0; plane < pam.depth; ++plane) {
                unsigned int const bitmapIndex = 
                    (row * pam.width + col) * pam.depth + plane;
                bitmapP->bitmap[bitmapIndex] = row255[col][plane];
            }
        }
    }
    pnm_freepamrow(row255);
    pnm_freepamarray(tuples, &pam);
    
    *bitmapPP = bitmapP;
}



static void
dotPrinter(float  const percentage,
           void * const clientData) {

    int * const currentP = (int *)clientData;
    float const unit     = (float)1.0 / (float)(dot_printer_max_column) ;
    int   const maximum  = (int)(percentage / unit);
    
    while (*currentP < maximum) {
        fputc(dot_printer_char, stderr);
        (*currentP)++;
    }
}



static void
exceptionHandler(const char * const msg,
                 at_msg_type  const type,
                 void *       const data) {

    if (type == AT_MSG_FATAL)
        pm_error("%s", msg);
    else if (type == AT_MSG_WARNING)
        pm_message("%s", msg);
    else
        exceptionHandler("Wrong type of msg", AT_MSG_FATAL, NULL);
}



struct cmdlineInfo {
    const char * inputFileName;
    float        align_threshold;
    unsigned int backgroundSpec;
    pixel        background_color;
    unsigned int centerline;
    float        corner_always_threshold;
    unsigned int corner_surround;
    float        corner_threshold;
    unsigned int dpi;
    float        error_threshold;
    unsigned int filter_iterations;
    float        line_reversion_threshold;
    float        line_threshold;
    unsigned int log;
    unsigned int preserve_width;
    unsigned int remove_adjacent_corners;
    unsigned int tangent_surround;
    unsigned int report_progress;
    float        width_weight_factor;
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
    optEntry *option_def;
    /* Instructions to optParseOptions3 on how to parse our options. */
    optStruct3 opt;

    const char * background_colorOpt;
  
    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "align-threshold",     OPT_FLOAT,
            &cmdlineP->align_threshold,  NULL,                              0);
    OPTENT3(0, "background-color",    OPT_STRING,
            &background_colorOpt,        &cmdlineP->backgroundSpec,         0);
    OPTENT3(0, "centerline",          OPT_FLAG,
            NULL,                        &cmdlineP->centerline,             0);
    OPTENT3(0, "corner-always-threshold", OPT_FLOAT, 
            &cmdlineP->corner_always_threshold, NULL,                       0);
    OPTENT3(0, "corner-surround",     OPT_UINT,
            &cmdlineP->corner_surround,  NULL,                              0);
    OPTENT3(0, "corner-threshold",    OPT_FLOAT,
            &cmdlineP->corner_threshold, NULL,                              0);
    OPTENT3(0, "dpi",                 OPT_UINT,
            &cmdlineP->dpi,              NULL,                              0);
    OPTENT3(0, "error-threshold",     OPT_FLOAT,
            &cmdlineP->error_threshold,  NULL,                              0);
    OPTENT3(0, "filter-iterations",   OPT_UINT,
            &cmdlineP->filter_iterations, NULL,                             0);
    OPTENT3(0, "line-reversion-threshold", OPT_FLOAT,
            &cmdlineP->line_reversion_threshold, NULL,                    0);
    OPTENT3(0, "line-threshold",      OPT_FLOAT,
            &cmdlineP->line_threshold, NULL,                                0);
    OPTENT3(0, "log",                 OPT_FLAG,
            NULL,                         &cmdlineP->log,                   0);
    OPTENT3(0, "preserve-width",      OPT_FLAG,
            NULL,                         &cmdlineP->preserve_width,        0);
    OPTENT3(0, "remove-adjacent-corners", OPT_UINT,
            NULL,                       &cmdlineP->remove_adjacent_corners, 0);
    OPTENT3(0, "tangent-surround",    OPT_UINT,    
            &cmdlineP->tangent_surround, NULL,                              0);
    OPTENT3(0, "report-progress",     OPT_FLAG,
            NULL,                       &cmdlineP->report_progress,         0);
    OPTENT3(0, "width-weight-factor", OPT_FLOAT,    
            &cmdlineP->width_weight_factor, NULL,                           0);


    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;   /* We have no parms that are negative numbers */

    /* Set some defaults the lazy way (using multiple setting of variables) */

    cmdlineP->corner_always_threshold  = 60.0;
    cmdlineP->corner_surround          = 4;
    cmdlineP->corner_threshold         = 100.0;
    cmdlineP->error_threshold          = 2.0;
    cmdlineP->filter_iterations        = 4;
    cmdlineP->line_reversion_threshold = 0.01;
    cmdlineP->line_threshold           = 1.0;
    cmdlineP->tangent_surround         = 3;
    cmdlineP->width_weight_factor      = 6.0;

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
    /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (cmdlineP->backgroundSpec)
        cmdlineP->background_color = ppm_parsecolor(background_colorOpt, 255);

    if (argc-1 < 1)
        cmdlineP->inputFileName = "-";
    else {
        cmdlineP->inputFileName = argv[1];
        
        if (argc-1 > 1)
            pm_error("Too many arguments (%u).  The only non-option argument "
                     "is the input file name.", argc-1);
    }
}



static void
fitSplines(at_bitmap_type *             const bitmapP,
           struct cmdlineInfo           const cmdline,
           at_msg_func                        exceptionHandler,
           at_progress_func                   progressFunc,
           at_spline_list_array_type ** const splinesPP) {

    unsigned int progressStat;
    at_fitting_opts_type * fittingOptsP;

    progressStat = 0;
           
    fittingOptsP = at_fitting_opts_new();

    fittingOptsP->backgroundSpec           = cmdline.backgroundSpec;
    fittingOptsP->background_color         = cmdline.background_color;
    fittingOptsP->corner_always_threshold  = cmdline.corner_always_threshold;
    fittingOptsP->corner_surround          = cmdline.corner_surround;
    fittingOptsP->corner_threshold         = cmdline.corner_threshold;
    fittingOptsP->error_threshold          = cmdline.error_threshold;
    fittingOptsP->filter_iterations        = cmdline.filter_iterations;
    fittingOptsP->line_reversion_threshold = cmdline.line_reversion_threshold;
    fittingOptsP->line_threshold           = cmdline.line_threshold;
    fittingOptsP->remove_adjacent_corners  = cmdline.remove_adjacent_corners;
    fittingOptsP->tangent_surround         = cmdline.tangent_surround;
    fittingOptsP->centerline               = cmdline.centerline;
    fittingOptsP->preserve_width           = cmdline.preserve_width;
    fittingOptsP->width_weight_factor      = cmdline.width_weight_factor;

    *splinesPP = at_splines_new_full(bitmapP, fittingOptsP,
                                     exceptionHandler, NULL,
                                     progressFunc, &progressStat,
                                     NULL, NULL);

    at_fitting_opts_free(fittingOptsP);
}
  


static void
writeSplines(at_spline_list_array_type * const splinesP,
             struct cmdlineInfo          const cmdline,
             at_output_write_func              outputWriter,
             FILE *                      const ofP,
             at_msg_func                       exceptionHandler) {

    at_output_opts_type * outputOptsP;

    outputOptsP = at_output_opts_new();
    outputOptsP->dpi = cmdline.dpi;
    
    at_splines_write(outputWriter, ofP, outputOptsP,
                     splinesP, exceptionHandler, NULL);

    at_output_opts_free(outputOptsP);
}  



static const char *
filenameRoot(const char * const filename) {
/*----------------------------------------------------------------------------
   Return the root of the filename.  E.g. for /home/bryanh/foo.ppm,
   return 'foo'.
-----------------------------------------------------------------------------*/
    char * buffer;
    bool foundSlash;
    unsigned int slashPos;
    bool foundDot;
    unsigned int dotPos;
    unsigned int rootStart, rootEnd;
    unsigned int i, j;

    for (i = 0, foundSlash = FALSE; i < strlen(filename); ++i) {
        if (filename[i] == '/') {
            foundSlash = TRUE;
            slashPos = i;
        }
    }

    if (foundSlash)
        rootStart = slashPos + 1;
    else
        rootStart = 0;

    for (i = rootStart, foundDot = FALSE; i < strlen(filename); ++i) {
        if (filename[i] == '.') {
            foundDot = TRUE;
            dotPos = i;
        }
    }

    if (foundDot)
        rootEnd = dotPos;
    else
        rootEnd = strlen(filename);

    MALLOCARRAY(buffer, rootEnd - rootStart + 1);
    
    j = 0;
    for (i = rootStart; i < rootEnd; ++i)
        buffer[j++] = filename[i];

    buffer[j] = '\0';
    
    return buffer;
}



static void
openLogFile(FILE **      const logFileP,
            const char * const inputFileArg) {

    const char * logfileName;

    if (streq(inputFileArg, "-"))
        asprintfN(&logfileName, "pamtosvg.log");
    else {
        const char * inputRootName;

        inputRootName = filenameRoot(inputFileArg);
        if (inputRootName == NULL)
            pm_error("Can't find the root portion of file name '%s'",
                     inputFileArg);
    
        asprintfN(&logfileName, "%s.log", inputRootName);

        strfree(inputRootName);
    }

    *logFileP = pm_openw(logfileName);

    strfree(logfileName);
}
    


int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    at_bitmap_type * bitmapP;
    at_spline_list_array_type * splinesP;
    at_progress_func progressReporter;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFileName);

    if (cmdline.log)
        openLogFile(&log_file, cmdline.inputFileName);

    readImageToBitmap(ifP, &bitmapP);
    
    if (cmdline.report_progress) {
        progressReporter = dotPrinter;
        fprintf(stderr, "%-15s", cmdline.inputFileName);
    } else
        progressReporter = NULL;

    fitSplines(bitmapP, cmdline, exceptionHandler,
               progressReporter, &splinesP);

    writeSplines(splinesP, cmdline, output_svg_writer, stdout,
                 exceptionHandler);

    pm_close(stdout);
    pm_close(ifP);
    if (cmdline.log)
        pm_close(log_file);
    
    at_splines_free(splinesP);
    at_bitmap_free(bitmapP);

    if (cmdline.report_progress)
        fputs("\n", stderr);
    
    return 0;
}
