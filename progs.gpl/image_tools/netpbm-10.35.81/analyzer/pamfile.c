/* pamfile.c - describe a portable anymap
**
** Copyright (C) 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "mallocvar.h"
#include "nstring.h"
#include "shhopt.h"
#include "pam.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    int inputFileCount;  /* Number of input files */
    const char ** inputFilespec;  /* Filespecs of input files */
    unsigned int allimages;  /* -allimages or -count */
    unsigned int count;      /* -count */
    unsigned int comments;   /* -comments */
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to as as the argv array.
-----------------------------------------------------------------------------*/
    optEntry * option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);
    
    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "allimages", OPT_FLAG,  NULL, &cmdlineP->allimages,   0);
    OPTENT3(0,   "count",     OPT_FLAG,  NULL, &cmdlineP->count,       0);
    OPTENT3(0,   "comments",  OPT_FLAG,  NULL, &cmdlineP->comments,    0);

    opt.opt_table     = option_def;
    opt.short_allowed = FALSE; /* We have no short (old-fashioned) options */
    opt.allowNegNum   = FALSE; /* We have no parms that are negative numbers */
    
    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others */

    cmdlineP->inputFilespec = (const char **)&argv[1];
    cmdlineP->inputFileCount = argc - 1;
}



static void
dumpHeader(struct pam const pam) {

    switch (pam.format) {
    case PAM_FORMAT:
        printf("PAM, %d by %d by %d maxval %ld\n", 
               pam.width, pam.height, pam.depth, pam.maxval);
        printf("    Tuple type: %s\n", pam.tuple_type);
        break;

	case PBM_FORMAT:
        printf("PBM plain, %d by %d\n", pam.width, pam.height );
        break;

	case RPBM_FORMAT:
        printf("PBM raw, %d by %d\n", pam.width, pam.height);
        break;

	case PGM_FORMAT:
        printf("PGM plain, %d by %d  maxval %ld\n", 
               pam.width, pam.height, pam.maxval);
        break;

	case RPGM_FORMAT:
        printf("PGM raw, %d by %d  maxval %ld\n", 
               pam.width, pam.height, pam.maxval);
        break;

	case PPM_FORMAT:
        printf("PPM plain, %d by %d  maxval %ld\n", 
               pam.width, pam.height, pam.maxval);
        break;

	case RPPM_FORMAT:
        printf("PPM raw, %d by %d  maxval %ld\n", 
               pam.width, pam.height, pam.maxval);
        break;
    }
}



static void
dumpComments(const char * const comments) {

    const char * p;
    bool startOfLine;
    
    printf("Comments:\n");

    for (p = &comments[0], startOfLine = TRUE; *p; ++p) {
        if (startOfLine)
            printf("  #");
        
        fputc(*p, stdout);
        
        if (*p == '\n')
            startOfLine = TRUE;
        else
            startOfLine = FALSE;
    }
    if (!startOfLine)
        fputc('\n', stdout);
}



static void
doOneImage(const char * const name,
           unsigned int const imageDoneCount,
           FILE *       const fileP,
           bool         const allimages,
           bool         const justCount,
           bool         const wantComments,
           bool *       const eofP) {
                    
    struct pam pam;
    const char * comments;
    enum pm_check_code checkRetval;

    pam.comment_p = &comments;

    pnm_readpaminit(fileP, &pam, PAM_STRUCT_SIZE(comment_p));
        
    if (!justCount) {
        if (allimages)
            printf("%s:\tImage %d:\t", name, imageDoneCount);
        else 
            printf("%s:\t", name);
            
        dumpHeader(pam);
        if (wantComments)
            dumpComments(comments);
    }
    strfree(comments);

    pnm_checkpam(&pam, PM_CHECK_BASIC, &checkRetval);
    if (allimages) {
        tuple * tuplerow;
        unsigned int row;
        
        tuplerow = pnm_allocpamrow(&pam);
        
        for (row = 0; row < pam.height; ++row) 
            pnm_readpamrow(&pam, tuplerow);
        
        pnm_freepamrow(tuplerow);
        
        pnm_nextimage(fileP, eofP);
    }
}



static void
describeOneFile(const char * const name,
                FILE *       const fileP,
                bool         const allimages,
                bool         const justCount,
                bool         const wantComments) {
/*----------------------------------------------------------------------------
   Describe one image stream (file).  Its name, for purposes of display,
   is 'name'.  The file is open as *fileP and positioned to the beginning.

   'allimages' means report on every image in the stream and read all of
   every image from it, as opposed to reading just the header of the first
   image and reporting just on that.

   'justCount' means don't tell anything about the stream except how
   many images are in it.  Pretty useless without 'allimages'.

   'wantComments' means to show the comments from the image header.
   Meaningless with 'justCount'.
-----------------------------------------------------------------------------*/
    unsigned int imageDoneCount;
        /* Number of images we've processed so far */
    bool eof;
    
    eof = FALSE;
    imageDoneCount = 0;

    while (!eof && (imageDoneCount < 1 || allimages)) {
        doOneImage(name, imageDoneCount, fileP,
                   allimages, justCount, wantComments,
                   &eof);
        ++imageDoneCount;
    }
    if (justCount)
        printf("%s:\t%u images\n", name, imageDoneCount);
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    if (cmdline.inputFileCount == 0)
        describeOneFile("stdin", stdin, cmdline.allimages || cmdline.count,
                        cmdline.count, cmdline.comments);
    else {
        unsigned int i;
        for (i = 0; i < cmdline.inputFileCount; ++i) {
            FILE * ifP;
            ifP = pm_openr(cmdline.inputFilespec[i]);
            describeOneFile(cmdline.inputFilespec[i], ifP, 
                            cmdline.allimages || cmdline.count,
                            cmdline.count, cmdline.comments);
            pm_close(ifP);
	    }
	}
    
    return 0;
}
