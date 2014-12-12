/*
 * pbmtextps.c -  render text into a bitmap using a postscript interpreter
 *
 * Copyright (C) 2002 by James McCann.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 *
 * PostScript is a registered trademark of Adobe Systems International.
 *
 * Additions by Bryan Henderson contributed to public domain by author.
 *
 */
#define _XOPEN_SOURCE   /* Make sure popen() is in stdio.h */
#define _BSD_SOURCE     /* Make sure stdrup() is in string.h */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pbm.h"
#include "nstring.h"
#include "shhopt.h"


#define BUFFER_SIZE 2048

static const char *gs_exe_path = 
#ifdef GHOSTSCRIPT_EXECUTABLE_PATH
GHOSTSCRIPT_EXECUTABLE_PATH;
#else
0;
#endif

static const char *pnmcrop_exe_path = 
#ifdef PNMCROP_EXECUTABLE_PATH
PNMCROP_EXECUTABLE_PATH;
#else
0;
#endif

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    int          res;         /* resolution, DPI */
    int          fontsize;    /* Size of font in points */
    const char * font;      /* Name of postscript font */
    float        stroke;
        /* Width of stroke in points (only for outline font) */
    unsigned int verbose;
    const char * text;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*---------------------------------------------------------------------------
  Note that the file spec array we return is stored in the storage that
  was passed to us as the argv array.
---------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
    /* Instructions to OptParseOptions2 on how to parse our options.
   */
    optStruct3 opt;

    unsigned int option_def_index;
    int i;
    char * text;
    int totaltextsize = 0;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "resolution", OPT_INT,    &cmdlineP->res,            NULL,  0);
    OPTENT3(0, "font",       OPT_STRING, &cmdlineP->font,           NULL,  0);
    OPTENT3(0, "fontsize",   OPT_INT,    &cmdlineP->fontsize,       NULL,  0);
    OPTENT3(0, "stroke",     OPT_FLOAT,  &cmdlineP->stroke,         NULL,  0);
    OPTENT3(0, "verbose",    OPT_FLAG,   NULL, &cmdlineP->verbose,         0);

    /* Set the defaults */
    cmdlineP->res = 150;
    cmdlineP->fontsize = 24;
    cmdlineP->font = "Times-Roman";
    cmdlineP->stroke = -1;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

    text = strdup("");
    totaltextsize = 1;

    for (i = 1; i < argc; i++) {
        if (i > 1) {
            totaltextsize += 1;
            text = realloc(text, totaltextsize);
            if (text == NULL)
                pm_error("out of memory");
            strcat(text, " ");
        } 
        totaltextsize += strlen(argv[i]);
        text = realloc(text, totaltextsize);
        if (text == NULL)
            pm_error("out of memory");
        strcat(text, argv[i]);
    }
    cmdlineP->text = text;
}



static const char *
construct_postscript(struct cmdlineInfo const cmdl) {

    const char * retval;
    const char * template;

    if (cmdl.stroke <= 0) 
        template = "/%s findfont\n%d scalefont\nsetfont\n12 36 moveto\n"
            "(%s) show\nshowpage\n";
    else 
        template = "/%s findfont\n%d scalefont\nsetfont\n12 36 moveto\n"
            "%f setlinewidth\n0 setgray\n"
            "(%s) true charpath\nstroke\nshowpage\n";

    if (cmdl.stroke < 0)
        asprintfN(&retval, template, cmdl.font, cmdl.fontsize, 
                  cmdl.text);
    else
        asprintfN(&retval, template, cmdl.font, cmdl.fontsize, 
                  cmdl.stroke, cmdl.text);

    return retval;
}



static const char *
gs_executable_name()
{
    static char buffer[BUFFER_SIZE];
    if(! gs_exe_path) {
        const char * const which = "which gs";
        FILE *f;
        memset(buffer, 0, BUFFER_SIZE);
        if(!(f = popen(which, "r")))
            pm_error("Can't find ghostscript");
        fread(buffer, 1, BUFFER_SIZE, f);
        if(buffer[strlen(buffer) - 1] == '\n')
            buffer[strlen(buffer) - 1] = 0;
        pclose(f);
        if(buffer[0] != '/' && buffer[0] != '.')
            pm_error("Can't find ghostscript");
    }
    else
        strcpy(buffer, gs_exe_path);

    return buffer;
}



static const char *
crop_executable_name()
{
    static char buffer[BUFFER_SIZE];
    if(! pnmcrop_exe_path) {
        const char * const which = "which pnmcrop";
        FILE *f;
        memset(buffer, 0, BUFFER_SIZE);
        if(!(f = popen(which, "r"))) {
            return 0;
        }
    
        fread(buffer, 1, BUFFER_SIZE, f);
        if(buffer[strlen(buffer) - 1] == '\n')
            buffer[strlen(buffer) - 1] = 0;
        pclose(f);
        if(buffer[0] != '/' && buffer[0] != '.') {
            buffer[0] = 0;
            pm_message("Can't find pnmcrop");
        }
    }
    else
        strcpy(buffer, pnmcrop_exe_path);

    return buffer;
}



static const char *
gsCommand(const char *       const psFname,
          const char *       const outputFilename, 
          struct cmdlineInfo const cmdline) {

    const char * retval;
    int const x = cmdline.res * 11;
    int const y = cmdline.res * (cmdline.fontsize * 2 + 72)  / 72.;
    asprintfN(&retval, "%s -g%dx%d -r%d -sDEVICE=pbm "
              "-sOutputFile=%s -q -dBATCH -dNOPAUSE %s </dev/null >/dev/null", 
              gs_executable_name(), x, y, cmdline.res, 
              outputFilename, psFname);
    return retval;
}



static const char *
cropCommand(const char * const inputFileName) {

    const char * retval;
    
    if (crop_executable_name()) {
        asprintfN(&retval, "%s -top -right %s", 
                  crop_executable_name(), inputFileName);
        if (retval == NULL)
            pm_error("Unable to allocate memory");
    } else
        retval = NULL;

    return retval;
}



static void
writeProgram(const char *       const psFname,
             struct cmdlineInfo const cmdline) {

    const char *ps;
    FILE * psfile;

    psfile = fopen(psFname, "w");
    if (psfile == NULL)
        pm_error("Can't open temp file '%s'.  Errno=%d (%s)",
                 psFname, errno, strerror(errno));

    ps = construct_postscript(cmdline);

    if (cmdline.verbose)
        pm_message("Postscript program = '%s'", ps);
        
    if (fwrite(ps, 1, strlen(ps), psfile) != strlen(ps))
        pm_error("Can't write postscript to temp file");

    fclose(psfile);

    strfree(ps);
}



static void
executeProgram(const char *       const psFname, 
               const char *       const outputFname,
               struct cmdlineInfo const cmdline) {

    const char * com;
    int rc;

    com = gsCommand(psFname, outputFname, cmdline);
    if (com == NULL)
        pm_error("Can't allocate memory for a 'ghostscript' command");
    
    if (cmdline.verbose)
        pm_message("Running Postscript interpreter '%s'", com);

    rc = system(com);
    if (rc != 0)
        pm_error("Failed to run Ghostscript process.  rc=%d", rc);

    strfree(com);
}



static void
cropToStdout(const char * const inputFileName,
             bool         const verbose) {

    const char * com;

    com = cropCommand(inputFileName);
    if (com == NULL) {
        /* No pnmcrop.  So don't crop. */
        pm_message("Can't find pnmcrop command, image will be large");
        asprintfN(&com, "cat %s", inputFileName);
        if (com == NULL) 
            pm_error("Unable to allocate memory.");
    } else {
        FILE *pnmcrop;

        if (verbose)
            pm_message("Running crop command '%s'", com);
        
        pnmcrop = popen(com, "r");
        if (pnmcrop == NULL)
            pm_error("Can't run pnmcrop process");
        else {
            char buf[2048];
            bool eof;

            eof = FALSE;
            
            while (!eof) {
                int bytesRead;

                bytesRead = fread(buf, 1, sizeof(buf), pnmcrop);
                if (bytesRead > 0) {
                    int rc;
                    rc = fwrite(buf, 1, bytesRead, stdout);
                    if (rc != bytesRead)
                        pm_error("Can't write to stdout");
                } else if (bytesRead == 0)
                    eof = TRUE;
                else
                    pm_error("Failed to read output of Pnmcrop process.  "
                             "Errno=%d (%s)", errno, strerror(errno));
            }
            fclose(pnmcrop);
        }
    }
    strfree(com);
}



static void
createOutputFile(struct cmdlineInfo const cmdline) {

    const char * const template = "./pstextpbm.%d.tmp.%s";
    
    const char * psFname;
    const char * uncroppedPbmFname;

    asprintfN(&psFname, template, getpid(), "ps");
    if (psFname == NULL)
        pm_error("Unable to allocate memory");
 
    writeProgram(psFname, cmdline);

    asprintfN(&uncroppedPbmFname, template, getpid(), "pbm");
    if (uncroppedPbmFname == NULL)
        pm_error("Unable to allocate memory");
 
    executeProgram(psFname, uncroppedPbmFname, cmdline);

    unlink(psFname);
    strfree(psFname);

    cropToStdout(uncroppedPbmFname, cmdline.verbose);

    unlink(uncroppedPbmFname);
    strfree(uncroppedPbmFname);
}



int 
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;

    pbm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    createOutputFile(cmdline);

    return 0;
}
