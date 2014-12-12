/*============================================================================
                              pnmindex   
==============================================================================

  build a visual index of a bunch of PNM images

  This used to be a C shell program, and then a BASH program.  Neither
  were portable enough, and the program is too complex for either of
  those languages anyway, so now it's in C.

  By Bryan Henderson 2005.04.24.

  Contributed to the public domain by its author.

============================================================================*/

#define _BSD_SOURCE   /* Make sure strdup is in string.h */

#include <assert.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>


#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    unsigned int  inputFileCount;
    const char ** inputFileName;
    unsigned int  size;
    unsigned int  across;
    unsigned int  colors;
    unsigned int  black;
    unsigned int  noquant;
    const char *  title;
    unsigned int  verbose;
};

static bool verbose;



static void PM_GNU_PRINTF_ATTR(1,2)
systemf(const char * const fmt,
        ...) {

    va_list varargs;
    
    size_t dryRunLen;
    
    va_start(varargs, fmt);
    
    vsnprintfN(NULL, 0, fmt, varargs, &dryRunLen);

    va_end(varargs);

    if (dryRunLen + 1 < dryRunLen)
        /* arithmetic overflow */
        pm_error("Command way too long");
    else {
        size_t const allocSize = dryRunLen + 1;
        char * shellCommand;
        shellCommand = malloc(allocSize);
        if (shellCommand == NULL)
            pm_error("Can't get storage for %u-character command",
                     allocSize);
        else {
            va_list varargs;
            size_t realLen;
            int rc;

            va_start(varargs, fmt);

            vsnprintfN(shellCommand, allocSize, fmt, varargs, &realLen);
                
            assert(realLen == dryRunLen);
            va_end(varargs);

            if (verbose)
                pm_message("shell cmd: %s", shellCommand);

            rc = system(shellCommand);
            if (rc != 0)
                pm_error("shell command '%s' failed.  rc %d",
                         shellCommand, rc);
            
            strfree(shellCommand);
        }
    }
}
        


static void
parseCommandLine(int argc, char ** argv, 
                 struct cmdlineInfo * const cmdlineP) {

    unsigned int option_def_index;
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int quant;
    unsigned int sizeSpec, colorsSpec, acrossSpec, titleSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "black",       OPT_FLAG,   NULL,                  
            &cmdlineP->black,         0);
    OPTENT3(0, "noquant",     OPT_FLAG,   NULL,                  
            &cmdlineP->noquant,       0);
    OPTENT3(0, "quant",       OPT_FLAG,   NULL,                  
            &quant,                   0);
    OPTENT3(0, "verbose",     OPT_FLAG,   NULL,                  
            &cmdlineP->verbose,       0);
    OPTENT3(0, "size",        OPT_UINT,   &cmdlineP->size,
            &sizeSpec,                0);
    OPTENT3(0, "colors",      OPT_UINT,   &cmdlineP->colors,
            &colorsSpec,              0);
    OPTENT3(0, "across",      OPT_UINT,   &cmdlineP->across,
            &acrossSpec,              0);
    OPTENT3(0, "title",       OPT_STRING, &cmdlineP->title,
            &titleSpec,               0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE; 

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (quant && cmdlineP->noquant)
        pm_error("You can't specify both -quant and -noquat");

    if (!colorsSpec)
        cmdlineP->colors = 256;
    
    if (!sizeSpec)
        cmdlineP->size = 100;

    if (!acrossSpec)
        cmdlineP->across = 6;

    if (!titleSpec)
        cmdlineP->title = NULL;

    if (colorsSpec && cmdlineP->noquant)
        pm_error("-colors doesn't make any sense with -noquant");

    if (argc-1 < 1)
        pm_error("You must name at least one file that contains an image "
                 "to go into the index");

    cmdlineP->inputFileCount = argc-1;

    MALLOCARRAY_NOFAIL(cmdlineP->inputFileName, cmdlineP->inputFileCount);

    {
        unsigned int i;
        for (i = 0; i < cmdlineP->inputFileCount; ++i) {
            cmdlineP->inputFileName[i] = strdup(argv[i+1]);
            if (cmdlineP->inputFileName[i] == NULL)
                pm_error("Unable to allocate memory for a file name");
        }
    }
}



static void
freeCmdline(struct cmdlineInfo const cmdline) {

    unsigned int i;

    for (i = 0; i < cmdline.inputFileCount; ++i)
        strfree(cmdline.inputFileName[i]);

    free(cmdline.inputFileName);

}



static void
makeTempDir(const char ** const tempDirP) {

    const char * const tmpdir = getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";

    const char * mytmpdir;
    int rc;

    asprintfN(&mytmpdir, "%s/pnmindex_%d", tmpdir, getpid());

    rc = mkdir(mytmpdir, 0700);
    if (rc != 0)
        pm_error("Unable to create temporary file directory '%s'.  mkdir() "
                 "fails with errno %d (%s)",
                 mytmpdir, errno, strerror(errno));

    *tempDirP = mytmpdir;
}



static void
removeTempDir(const char * const tempDir) {

    int rc;

    rc = rmdir(tempDir);
    if (rc != 0)
        pm_error("Failed to remove temporary file directory '%s'.  "
                 "rmdir() fails with errno %d (%s)",
                 tempDir, errno, strerror(errno));
}


static const char *
rowFileName(const char * const dirName,
            unsigned int const row) {

    const char * fileName;
    
    asprintfN(&fileName, "%s/pi.%u", dirName, row);
    
    return fileName;
}



static void
makeTitle(const char * const title,
          unsigned int const rowNumber,
          bool         const blackBackground,
          const char * const tempDir) {

    const char * const invertStage = blackBackground ? "| pnminvert " : "";

    const char * fileName;

    fileName = rowFileName(tempDir, rowNumber);

    /* This quoting is not adequate.  We really should do this without
       a shell at all.
    */
    systemf("pbmtext \"%s\" "
            "%s"
            "> %s", 
            title, invertStage, fileName);

    strfree(fileName);
}



static void
copyImage(const char * const inputFileName,
          const char * const outputFileName) {

    systemf("cat %s > %s", inputFileName, outputFileName);
} 



static void
copyScaleQuantImage(const char * const inputFileName,
                    const char * const outputFileName,
                    int          const format,
                    unsigned int const size,
                    unsigned int const quant,
                    unsigned int const colors) {

    const char * scaleCommand;

    switch (PNM_FORMAT_TYPE(format)) {
    case PBM_TYPE:
        asprintfN(&scaleCommand, 
                  "pamscale -quiet -xysize %u %u %s "
                  "| pgmtopbm > %s",
                  size, size, inputFileName, outputFileName);
        break;
        
    case PGM_TYPE:
        asprintfN(&scaleCommand, 
                  "pamscale -quiet -xysize %u %u %s >%s",
                  size, size, inputFileName, outputFileName);
        break;
        
    case PPM_TYPE:
        if (quant)
            asprintfN(&scaleCommand, 
                      "pamscale -quiet -xysize %u %u %s "
                      "| pnmquant -quiet %u > %s",
                      size, size, inputFileName, colors, outputFileName);
        else
            asprintfN(&scaleCommand, 
                      "pamscale -quiet -xysize %u %u %s >%s",
                      size, size, inputFileName, outputFileName);
        break;
    default:
        pm_error("Unrecognized Netpbm format: %d", format);
    }

    systemf("%s", scaleCommand);

    strfree(scaleCommand);
}



static int
formatTypeMax(int const typeA,
              int const typeB) {

    if (typeA == PPM_TYPE || typeB == PPM_TYPE)
        return PPM_TYPE; 
    else if (typeA == PGM_TYPE || typeB == PGM_TYPE)
        return PGM_TYPE;
    else
        return PBM_TYPE;
}



static const char *
thumbnailFileName(const char * const dirName,
                  unsigned int const row,
                  unsigned int const col) {

    const char * fileName;
    
    asprintfN(&fileName, "%s/pi.%u.%u", dirName, row, col);
    
    return fileName;
}



static const char *
thumbnailFileList(const char * const dirName,
                  unsigned int const row,
                  unsigned int const cols) {

    unsigned int const maxListSize = 4096;

    char * list;
    unsigned int col;

    list = malloc(maxListSize);
    if (list == NULL)
        pm_error("Unable to allocate %u bytes for file list", maxListSize);

    list[0] = '\0';
    
    for (col = 0; col < cols; ++col) {
        const char * const fileName = thumbnailFileName(dirName, row, col);

        if (strlen(list) + strlen(fileName) + 1 > maxListSize - 1)
            pm_error("File name list too long for this program to handle.");
        else {
            strcat(list, " ");
            strcat(list, fileName);
        }
        strfree(fileName);
    }

    return list;
}



static void
makeImageFile(const char * const thumbnailFileName,
              const char * const inputFileName,
              bool         const blackBackground,
              const char * const outputFileName) {

    const char * const blackWhiteOpt = blackBackground ? "-black" : "-white";
    const char * const invertStage = blackBackground ? "| pnminvert " : "";

    systemf("pbmtext \"%s\" "
            "%s"
            "| pnmcat %s -topbottom %s - "
            "> %s",
            inputFileName, invertStage, blackWhiteOpt, 
            thumbnailFileName, outputFileName);
}    



static void
makeThumbnail(const char *  const inputFileName,
              unsigned int  const size,
              bool          const black,
              bool          const quant,
              unsigned int  const colors,
              const char *  const tempDir,
              unsigned int  const row,
              unsigned int  const col,
              int *         const formatP) {

    FILE * ifP;
    int imageCols, imageRows, format;
    xelval maxval;
    const char * tmpfile;
    const char * fileName;
        
    ifP = pm_openr(inputFileName);
    pnm_readpnminit(ifP, &imageCols, &imageRows, &maxval, &format);
    pm_close(ifP);
    
    asprintfN(&tmpfile, "%s/pi.tmp", tempDir);

    if (imageCols < size && imageRows < size)
        copyImage(inputFileName, tmpfile);
    else
        copyScaleQuantImage(inputFileName, tmpfile, format, 
                            size, quant, colors);

    fileName = thumbnailFileName(tempDir, row, col);
        
    makeImageFile(tmpfile, inputFileName, black, fileName);

    unlink(tmpfile);

    strfree(fileName);
    strfree(tmpfile);

    *formatP = format;
}
        


static void
unlinkThumbnailFiles(const char * const dirName,
                     unsigned int const row,
                     unsigned int const cols) {

    unsigned int col;
    
    for (col = 0; col < cols; ++col) {
        const char * const fileName = thumbnailFileName(dirName, row, col);

        unlink(fileName);

        strfree(fileName);
    }
}



static void
unlinkRowFiles(const char * const dirName,
               unsigned int const rows) {

    unsigned int row;
    
    for (row = 0; row < rows; ++row) {
        const char * const fileName = rowFileName(dirName, row);

        unlink(fileName);

        strfree(fileName);
    }
}



static void
combineIntoRowAndDelete(unsigned int const row,
                        unsigned int const cols,
                        int          const maxFormatType,
                        bool         const blackBackground,
                        bool         const quant,
                        unsigned int const colors,
                        const char * const tempDir) {

    const char * const blackWhiteOpt = blackBackground ? "-black" : "-white";

    const char * fileName;
    const char * quantStage;
    const char * fileList;
    
    fileName = rowFileName(tempDir, row);

    unlink(fileName);

    if (maxFormatType == PPM_TYPE && quant)
        asprintfN(&quantStage, "| pnmquant -quiet %u ", colors);
    else
        quantStage = strdup("");

    fileList = thumbnailFileList(tempDir, row, cols);

    systemf("pnmcat %s -leftright -jbottom %s "
            "%s"
            ">%s",
            blackWhiteOpt, fileList, quantStage, fileName);

    strfree(fileList);
    strfree(quantStage);
    strfree(fileName);

    unlinkThumbnailFiles(tempDir, row, cols);
}



static const char *
rowFileList(const char * const dirName,
            unsigned int const rows) {

    unsigned int const maxListSize = 4096;

    unsigned int row;
    char * list;

    list = malloc(maxListSize);
    if (list == NULL)
        pm_error("Unable to allocate %u bytes for file list", maxListSize);

    list[0] = '\0';

    for (row = 0; row < rows; ++row) {
        const char * const fileName = rowFileName(dirName, row);

        if (strlen(list) + strlen(fileName) + 1 > maxListSize - 1)
            pm_error("File name list too long for this program to handle.");
        
        else {
            strcat(list, " ");
            strcat(list, fileName);
        }
        strfree(fileName);
    }

    return list;
}



static void
writeRowsAndDelete(unsigned int const rows,
                   int          const maxFormatType,
                   bool         const blackBackground,
                   bool         const quant,
                   unsigned int const colors,
                   const char * const tempDir) {

    const char * const blackWhiteOpt = blackBackground ? "-black" : "-white";

    const char * quantStage;
    const char * fileList;
    
    if (maxFormatType == PPM_TYPE && quant)
        asprintfN(&quantStage, "| pnmquant -quiet %u ", colors);
    else
        quantStage = strdup("");

    fileList = rowFileList(tempDir, rows);

    systemf("pnmcat %s -topbottom %s %s",
            blackWhiteOpt, fileList, quantStage);

    strfree(fileList);
    strfree(quantStage);

    unlinkRowFiles(tempDir, rows);
}



int
main(int argc, char *argv[]) {
    struct cmdlineInfo cmdline;
    const char * tempDir;
    int maxFormatType;
    unsigned int colsInRow;
    unsigned int rowsDone;
    unsigned int i;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    verbose = cmdline.verbose;
    
    makeTempDir(&tempDir);

    maxFormatType = PBM_TYPE;
    colsInRow = 0;
    rowsDone = 0;

    if (cmdline.title)
        makeTitle(cmdline.title, rowsDone++, cmdline.black, tempDir);

    for (i = 0; i < cmdline.inputFileCount; ++i) {
        const char * const inputFileName = cmdline.inputFileName[i];

        int format;

        makeThumbnail(inputFileName, cmdline.size, cmdline.black, 
                      !cmdline.noquant, cmdline.colors, tempDir,
                      rowsDone, colsInRow, &format);

        maxFormatType = formatTypeMax(maxFormatType, PNM_FORMAT_TYPE(format));

        ++colsInRow;
        if (colsInRow >= cmdline.across || i == cmdline.inputFileCount-1) {
            combineIntoRowAndDelete(
                rowsDone, colsInRow, maxFormatType,
                cmdline.black, !cmdline.noquant, cmdline.colors,
                tempDir);
            ++rowsDone;
            colsInRow = 0;
        }
    }

    writeRowsAndDelete(rowsDone, maxFormatType, cmdline.black,
                       !cmdline.noquant, cmdline.colors, tempDir);

    removeTempDir(tempDir);

    freeCmdline(cmdline);

    pm_close(stdout);

    return 0;
}
