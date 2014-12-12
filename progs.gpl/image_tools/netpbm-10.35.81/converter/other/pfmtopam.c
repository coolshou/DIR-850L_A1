/*****************************************************************************
                                  pfmtopam
******************************************************************************
  This program converts a PFM (Portable Float Map) image to PAM.
  
  By Bryan Henderson, San Jose, CA April 2004.

  Contributed to the public domain by its author.

*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "pam.h"
#include "pm_gamma.h"
#include "shhopt.h"
#include "mallocvar.h"


struct cmdlineInfo {
    const char * inputFilespec;
    unsigned int verbose;
    sample maxval;
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
    unsigned int maxvalSpec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "maxval",   OPT_UINT, &cmdlineP->maxval, &maxvalSpec,        0);
    OPTENT3(0, "verbose",  OPT_FLAG, NULL,             &cmdlineP->verbose, 0);
  
    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;   /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
    /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (!maxvalSpec)
        cmdlineP->maxval = PNM_MAXMAXVAL;

    if (cmdlineP->maxval > PNM_OVERALLMAXVAL)
        pm_error("Maximum allowed -maxval is %u.  You specified %u",
                 PNM_OVERALLMAXVAL, (unsigned)cmdlineP->maxval);
    else if (cmdlineP->maxval == 0)
        pm_error("-maxval cannot be 0");

    /* Get the program parameters */

    if (argc-1 >= 1)
        cmdlineP->inputFilespec = argv[1];
    else
        cmdlineP->inputFilespec = "-";
    
    if (argc-1 > 1)
        pm_error("Program takes at most one argument:  the file name.  "
                 "You specified %d", argc-1);
}


enum endian {ENDIAN_BIG, ENDIAN_LITTLE};


static enum endian machineEndianness;



static enum endian
thisMachineEndianness(void) {
/*----------------------------------------------------------------------------
   Endianness is a component of the format in which a machine represents
   a number in memory or a register.  It is the only component of the format
   that varies among typical machines.

   Big endianness is the natural format.  In this format, if an integer is
   4 bytes, to be stored at memory address 100-103, the most significant 
   byte goes at 100, the next most significant at 101, and the least
   significant byte at 103.  This is natural because it matches the way
   humans read and write numbers.  I.e. 258 is stored as 0x00000102.

   Little endian is extremely common because it is used by IA32.  In the
   example above, the least significant byte goes first, so 258 would be
   stored as 0x02010000.

   You can extend this concept to floating point numbers, even though the
   bytes of a floating point number differ by more than significance.
-----------------------------------------------------------------------------*/
    short const testNumber = 0x0001;

    unsigned char * const storedNumber = (unsigned char *)&testNumber;
    enum endian endianness;
    
    if (storedNumber[0] == 0x01)
        endianness = ENDIAN_LITTLE;
    else
        endianness = ENDIAN_BIG;

    return endianness;
}



typedef union {
    unsigned char bytes[4];      /* as read from the file */
    float value;
        /* This is valid only if the pfmSample has the same endianness
           as the machine we're running on.
        */
} pfmSample;



static float
floatFromPfmSample(pfmSample   const sample, 
                   enum endian const pfmEndianness) {
/*----------------------------------------------------------------------------
   Type converter
-----------------------------------------------------------------------------*/
    if (machineEndianness == pfmEndianness) {
        return sample.value;
    } else {
        pfmSample rightEndianSample;
        unsigned int i, j;
        
        for (i = 0, j = sizeof(sample.bytes)-1; 
             i < sizeof(sample.bytes); 
             ++i, --j)
            
            rightEndianSample.bytes[i] = sample.bytes[j];

        return rightEndianSample.value;
    }
}



struct pfmHeader {
    unsigned int width;
    unsigned int height;
    bool color;
    float scaleFactor;
    enum endian endian;
};


static void
readPfmHeader(FILE *             const ifP,
              struct pfmHeader * const pfmHeaderP) {

    int firstChar;
    int secondChar;
    float scaleFactorEndian;

    firstChar = fgetc(ifP);
    if (firstChar == EOF)
        pm_error("Error reading first character of PFM file");
    secondChar = fgetc(ifP);
    if (secondChar == EOF)
        pm_error("Error reading second character of PFM file");

    if (firstChar != 'P' || (secondChar != 'F' && secondChar != 'f'))
        pm_error("First two characters of input file are '%c%c', but "
                 "for a valid PFM file, they must be 'PF' or 'Pf'.",
                 firstChar, secondChar);

    {
        int whitespace;

        whitespace = fgetc(ifP);
        if (whitespace == EOF)
            pm_error("Error reading third character of PFM file");

        if (!isspace(whitespace))
            pm_error("The 3rd character of the input file is not whitespace.");
    }
    {
        int rc;
        char whitespace;

        rc = fscanf(ifP, "%u %u%c", 
                    &pfmHeaderP->width, &pfmHeaderP->height, &whitespace);

        if (rc == EOF)
            pm_error("Error reading the width and height from input file.");
        else if (rc != 3)
            pm_error("Invalid input file format where width and height "
                     "are supposed to be (should be two positive decimal "
                     "integers separated by a space and followed by "
                     "white space)");
        
        if (!isspace(whitespace))
            pm_error("Invalid input file format -- '%c' instead of "
                     "white space after height", whitespace);

        if (pfmHeaderP->width == 0)
            pm_error("Invalid input file: image width is zero");
        if (pfmHeaderP->height == 0)
            pm_error("Invalid input file: image height is zero");
    }
    {
        int rc;
        char whitespace;

        rc = fscanf(ifP, "%f%c", &scaleFactorEndian, &whitespace);

        if (rc == EOF)
            pm_error("Error reading the scale factor from input file.");
        else if (rc != 2)
            pm_error("Invalid input file format where scale factor "
                     "is supposed to be (should be a floating point decimal "
                     "number followed by white space");
        
        if (!isspace(whitespace))
            pm_error("Invalid input file format -- '%c' instead of "
                     "white space after scale factor", whitespace);
    }

    pfmHeaderP->color = (secondChar == 'F');  
        /* 'PF' = RGB, 'Pf' = monochrome */

    if (scaleFactorEndian > 0.0) {
        pfmHeaderP->endian = ENDIAN_BIG;
        pfmHeaderP->scaleFactor = scaleFactorEndian;
    } else if (scaleFactorEndian < 0.0) {
        pfmHeaderP->endian = ENDIAN_LITTLE;
        pfmHeaderP->scaleFactor = - scaleFactorEndian;
    } else
        pm_error("Scale factor/endianness in PFM header is 0");
}


static void
dumpPfmHeader(struct pfmHeader const pfmHeader) {

    pm_message("width: %u, height: %u", pfmHeader.width, pfmHeader.height);
    pm_message("color: %s", pfmHeader.color ? "YES" : "NO");
    pm_message("endian: %s", 
               pfmHeader.endian == ENDIAN_BIG ? "BIG" : "LITTLE");
    pm_message("scale factor: %f", pfmHeader.scaleFactor);
}



static void
initPam(struct pam * const pamP, 
        int          const width, 
        int          const height, 
        bool         const color,
        sample       const maxval) {

    pamP->size        = sizeof(*pamP);
    pamP->len         = PAM_STRUCT_SIZE(tuple_type);
    pamP->file        = stdout;
    pamP->format      = PAM_FORMAT;
    pamP->plainformat = FALSE;
    pamP->width       = width;
    pamP->height      = height;
    pamP->maxval      = maxval;
    if (color) {
        pamP->depth = 3;
        strcpy(pamP->tuple_type, "RGB");
    } else {
        pamP->depth = 1;
        strcpy(pamP->tuple_type, "GRAYSCALE");
    }
}



static void
makePamRow(struct pam * const pamP,
           FILE *       const ifP,
           unsigned int const pfmRow,
           unsigned int const pfmSamplesPerRow,
           tuplen **    const tuplenArray,
           enum endian  const endian,
           float        const scaleFactor,
           pfmSample *  const pfmRowBuffer) {
/*----------------------------------------------------------------------------
   Make a PAM (tuple) row of the form described by *pamP, from the next
   row in the PFM file identified by 'ifP'.  Place it in the proper location
   in the tuple array 'tuplenArray'.
  
   'endian' is the endianness of the samples in the PFM file.

   'pfmRowNum' is the sequence number (starting at 0, which is the
   bottommost row of the image) of the row we are converting in the
   PFM file.

   Use 'pfmRowBuffer' as a work space; Caller must ensure this array
   has at least 'pfmSamplesPerRow' elements of space in it.
-----------------------------------------------------------------------------*/
    int      const row       = pamP->height - pfmRow - 1;
    tuplen * const tuplenRow = tuplenArray[row];

    int col;
    int pfmCursor;
    int rc;

    rc = fread(pfmRowBuffer, sizeof(pfmSample), pfmSamplesPerRow, ifP);
    if (rc != pfmSamplesPerRow)
        pm_error("End of file in the middle of row %d", pfmRow);

    pfmCursor = 0;

    for (col = 0; col < pamP->width; ++col) {
        /* The order of planes (R, G, B) is the same in PFM as in PAM. */
        unsigned int plane;
        for (plane = 0; plane < pamP->depth; ++plane) {
            float const val = 
                floatFromPfmSample(pfmRowBuffer[pfmCursor++], endian);
            tuplenRow[col][plane] = val / scaleFactor;
        }
    }
    assert(pfmCursor == pfmSamplesPerRow);
}



int
main(int argc, char **argv ) {

    struct cmdlineInfo cmdline;
    FILE* ifP;
    struct pam pam;
    struct pfmHeader pfmHeader;
    pfmSample * pfmRowBuffer;
    unsigned int pfmSamplesPerRow;
    unsigned pfmRow;
    tuplen ** tuplenArray;

    machineEndianness = thisMachineEndianness();

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    readPfmHeader(ifP, &pfmHeader);

    if (cmdline.verbose)
        dumpPfmHeader(pfmHeader);

    initPam(&pam, 
            pfmHeader.width, pfmHeader.height, pfmHeader.color,
            cmdline.maxval);

    tuplenArray = pnm_allocpamarrayn(&pam);

    pfmSamplesPerRow = pam.width * pam.depth;
    
    MALLOCARRAY_NOFAIL(pfmRowBuffer, pfmSamplesPerRow);

    /* PFMs are upside down like BMPs */
    for (pfmRow = 0; pfmRow < pam.height; ++pfmRow)
        makePamRow(&pam, ifP, pfmRow, pfmSamplesPerRow,
                   tuplenArray, pfmHeader.endian, pfmHeader.scaleFactor,
                   pfmRowBuffer);

    pnm_writepamn(&pam, tuplenArray);

    pnm_freepamarrayn(tuplenArray, &pam);
    free(pfmRowBuffer);
    
    pm_close(ifP);
    pm_close(pam.file);

    return 0;
}
