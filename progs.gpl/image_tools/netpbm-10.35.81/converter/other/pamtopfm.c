/*****************************************************************************
                                  pamtopfm
******************************************************************************
  This program converts a PAM image to PFM (Portable Float Map).
  
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
#include "nstring.h"

enum endian {ENDIAN_BIG, ENDIAN_LITTLE};

struct cmdlineInfo {
    const char * inputFilespec;
    unsigned int verbose;
    enum endian endian;
    float scale;
};



static enum endian machineEndianness;



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
    char * endianOpt;
    unsigned int endianSpec, scaleSpec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "endian",   OPT_STRING, &endianOpt, &endianSpec,        0);
    OPTENT3(0, "scale",    OPT_FLOAT,  &cmdlineP->scale, &scaleSpec,   0);
  
    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;   /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
    /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (endianSpec) {
        if (streq(endianOpt, "big"))
            cmdlineP->endian = ENDIAN_BIG;
        else if (streq(endianOpt, "little"))
            cmdlineP->endian = ENDIAN_LITTLE;
        else
            pm_error("Invalid value '%s' for -endian.  "
                     "Must be 'big' or 'little'.", endianOpt);
    } else
        cmdlineP->endian = machineEndianness;

    if (!scaleSpec) {
        cmdlineP->scale = 1.0;
    }
    if (cmdlineP->scale == 0.0)
        pm_error("Scale factor cannot be zero");

    /* Get the program parameters */

    if (argc-1 >= 1)
        cmdlineP->inputFilespec = argv[1];
    else
        cmdlineP->inputFilespec = "-";
    
    if (argc-1 > 1)
        pm_error("Program takes at most one argument:  the file name.  "
                 "You specified %d", argc-1);
}



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



typedef struct {
    unsigned char bytes[4];
} pfmSample;



static void
floatToPfmSample(float       const input,
                 pfmSample *       outputP,
                 enum endian const pfmEndianness) {
/*----------------------------------------------------------------------------
   Type converter
-----------------------------------------------------------------------------*/
    if (machineEndianness == pfmEndianness) {
        *(float *)outputP->bytes = input;
    } else {
        unsigned char reversed[sizeof(pfmSample)];
        unsigned int i, j;

        *(float *)reversed = input;
        
        for (i = 0, j = sizeof(pfmSample)-1; 
             i < sizeof(pfmSample); 
             ++i, --j)
            
            outputP->bytes[i] = reversed[j];
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
writePfmHeader(FILE *           const ofP,
               struct pfmHeader const pfmHeader) {

    const char * const magic = pfmHeader.color ? "PF" : "Pf";
    float const scaleFactorEndian = 
        pfmHeader.endian == ENDIAN_BIG ? 
            pfmHeader.scaleFactor :
            - pfmHeader.scaleFactor;

    fprintf(ofP, "%s\n",    magic);
    fprintf(ofP, "%u %u\n", pfmHeader.width, pfmHeader.height);
    fprintf(ofP, "%f\n",    scaleFactorEndian);
}



static void
writePfmRow(struct pam * const pamP,
            FILE *       const ofP,
            unsigned int const pfmRow,
            unsigned int const pfmSamplesPerRow,
            tuplen **    const tuplenArray,
            enum endian  const endian,
            float        const scaleFactor,
            pfmSample *  const pfmRowBuffer) {

    int const row = pamP->height - pfmRow - 1;
    tuplen * const tuplenRow = tuplenArray[row];

    int col;
    int pfmCursor;
    int rc;

    pfmCursor = 0;  /* initial value */

    for (col = 0; col < pamP->width; ++col) {
        /* The order of planes (R, G, B) is the same in PFM as in PAM. */
        unsigned int plane;
        for (plane = 0; plane < pamP->depth; ++plane) {
            pfmSample val;
            floatToPfmSample(tuplenRow[col][plane] * scaleFactor, 
                             &val, endian);
            pfmRowBuffer[pfmCursor++] = val;
        }
    }
    assert(pfmCursor == pfmSamplesPerRow);

    rc = fwrite(pfmRowBuffer, sizeof(pfmSample), pfmSamplesPerRow, ofP);
    if (rc != pfmSamplesPerRow)
        pm_error("Unable to write to output file in the middle of row %d", 
                 pfmRow);


}



static struct pfmHeader
makePfmHeader(const struct pam * const pamP,
              float              const scaleFactor,
              enum endian        const endian) {
    
    struct pfmHeader pfmHeader;
    
    pfmHeader.width  = pamP->width;
    pfmHeader.height = pamP->height;

    if (strncmp(pamP->tuple_type, "RGB", 3) == 0)
        pfmHeader.color = TRUE;
    else if (strncmp(pamP->tuple_type, "GRAYSCALE", 9) == 0)
        pfmHeader.color = FALSE;
    else if (strncmp(pamP->tuple_type, "BLACKANDWHITE", 13) == 0)
        pfmHeader.color = FALSE;
    else
        pm_error("Invalid PAM input.  Tuple type is '%s'.  "
                 "We understand only RGB* and GRAYSCALE*", pamP->tuple_type);

    pfmHeader.scaleFactor = scaleFactor;
    pfmHeader.endian = endian;
        
    return pfmHeader;
}


int
main(int argc, char **argv ) {

    struct cmdlineInfo cmdline;
    FILE* ifP;
    struct pam pam;
    pfmSample * pfmRowBuffer;
    unsigned int pfmSamplesPerRow;
    unsigned int pfmRow;
    tuplen ** tuplenArray;

    pnm_init(&argc, argv);

    machineEndianness = thisMachineEndianness();

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    tuplenArray = pnm_readpamn(ifP, &pam, PAM_STRUCT_SIZE(tuple_type));

    writePfmHeader(stdout, 
                   makePfmHeader(&pam, cmdline.scale, cmdline.endian));

    pfmSamplesPerRow = pam.width * pam.depth;
    
    MALLOCARRAY_NOFAIL(pfmRowBuffer, pfmSamplesPerRow);

    /* PFMs are upside down like BMPs */
    for (pfmRow = 0; pfmRow < pam.height; ++pfmRow)
        writePfmRow(&pam, stdout, pfmRow, pfmSamplesPerRow,
                    tuplenArray, cmdline.endian, cmdline.scale,
                    pfmRowBuffer);

    pnm_freepamarrayn(tuplenArray, &pam);
    free(pfmRowBuffer);
    
    pm_close(stdout);
    pm_close(pam.file);

    return 0;
}
