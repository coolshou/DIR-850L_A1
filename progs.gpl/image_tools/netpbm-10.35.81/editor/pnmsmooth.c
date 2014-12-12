/* pnmsmooth.c - smooth out an image by replacing each pixel with the 
**               average of its width x height neighbors.
**
** Version 2.0   December 5, 1994
**
** Copyright (C) 1994 by Mike Burns (burns@chem.psu.edu)
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/*
  Written by Mike Burns December 5, 1994 and called Version 2.0.
  Based on ideas from a shell script by Jef Poskanzer, 1989, 1991.
  The shell script had no options.
*/


#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include "pm_c_util.h"
#include "mallocvar.h"
#include "shhopt.h"
#include "nstring.h"
#include "pnm.h"


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    unsigned int width;
    unsigned int height;
    const char * dump;
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
    optEntry * option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int widthSpec, heightSpec, dumpSpec, sizeSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);
    
    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "dump",          OPT_STRING,   
            &cmdlineP->dump,            &dumpSpec, 0);
    OPTENT3(0,   "width",         OPT_UINT,
            &cmdlineP->width,           &widthSpec, 0);
    OPTENT3(0,   "height",        OPT_UINT,
            &cmdlineP->height,          &heightSpec, 0);
    OPTENT3(0,   "size",          OPT_FLAG,
            NULL,                       &sizeSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (!widthSpec)
        cmdlineP->width = 3;

    if (!heightSpec)
        cmdlineP->height = 3;

    if (!dumpSpec)
        cmdlineP->dump = NULL;

    if (sizeSpec) {
        /* -size is strictly for backward compatibility.  This program
           used to use a different command line processor and had
           irregular syntax in which the -size option had two values,
           e.g. "-size <width> <height>" And the options had to go
           before the arguments.  So an old pnmsmooth command looks to us
           like a command with the -size flag option and the first two
           arguments being the width and height.
        */

        if (widthSpec || heightSpec)
            pm_error("-size is obsolete.  Use -width and -height instead");

        if (argc-1 > 3)
            pm_error("Too many arguments.  With -size, there are at most "
                     "3 arguments.");
        else if (argc-1 < 2)
            pm_error("Not enough arguments.  With -size, the first two "
                     "arguments are width and height");
        else {
            cmdlineP->width  = atoi(argv[1]);
            cmdlineP->height = atoi(argv[2]);
            
            if (argc-1 < 3)
                cmdlineP->inputFilespec = "-";
            else
                cmdlineP->inputFilespec = argv[3];
        }
    } else {
        if (argc-1 > 1)
            pm_error("Program takes at most one argument: the input file "
                     "specification.  "
                     "You specified %d arguments.", argc-1);
        if (argc-1 < 1)
            cmdlineP->inputFilespec = "-";
        else
            cmdlineP->inputFilespec = argv[1];
    }
    if (cmdlineP->width % 2 != 1)
        pm_error("The convolution matrix must have an odd number of columns.  "
                 "You specified %u", cmdlineP->width);

    if (cmdlineP->height % 2 != 1)
        pm_error("The convolution matrix must have an odd number of rows.  "
                 "You specified %u", cmdlineP->height);
}



static void
writeConvolutionImage(FILE *       const cofp,
                      unsigned int const cols,
                      unsigned int const rows,
                      int          const format) {

    xelval const convmaxval = rows * cols * 2;
        /* normalizing factor for our convolution matrix */
    xelval const g = rows * cols + 1;
        /* weight of all pixels in our convolution matrix */
    int row;
    xel *outputrow;

    if (convmaxval > PNM_OVERALLMAXVAL)
        pm_error("The convolution matrix is too large.  "
                 "Width x Height x 2\n"
                 "must not exceed %d and it is %d.",
                 PNM_OVERALLMAXVAL, convmaxval);

    pnm_writepnminit(cofp, cols, rows, convmaxval, format, 0);
    outputrow = pnm_allocrow(cols);

    for (row = 0; row < rows; ++row) {
        unsigned int col;
        for (col = 0; col < cols; ++col)
            PNM_ASSIGN1(outputrow[col], g);
        pnm_writepnmrow(cofp, outputrow, cols, convmaxval, format, 0);
    }
    pnm_freerow(outputrow);
}



static void
runPnmconvol(const char * const inputFilespec,
             const char * const convolutionImageFilespec) {

    /* fork a Pnmconvol process */
    pid_t rc;

    rc = fork();
    if (rc < 0)
        pm_error("fork() failed.  errno=%d (%s)", errno, strerror(errno));
    else if (rc == 0) {
        /* child process executes following code */

        execlp("pnmconvol",
               "pnmconvol", convolutionImageFilespec, inputFilespec,
               NULL);

        pm_error("error executing pnmconvol command.  errno=%d (%s)",
                 errno, strerror(errno));
    } else {
        /* This is the parent */
        pid_t const childPid = rc;

        int status;

        /* wait for child to finish */
        while (wait(&status) != childPid);
    }
}



int
main(int argc, char ** argv) {

    struct cmdlineInfo cmdline;
    FILE * convFileP;
    const char * tempfileName;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    if (cmdline.dump)
        convFileP = pm_openw(cmdline.dump);
    else
        pm_make_tmpfile(&convFileP, &tempfileName);
        
    writeConvolutionImage(convFileP, cmdline.width, cmdline.height,
                          PGM_FORMAT);

    pm_close(convFileP);

    if (cmdline.dump) {
        /* We're done.  Convolution image is in user's file */
    } else {
        runPnmconvol(cmdline.inputFilespec, tempfileName);

        unlink(tempfileName);
        strfree(tempfileName);
    }
    return 0;
}
