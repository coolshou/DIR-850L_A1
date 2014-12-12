/*===========================================================================
                            g3topbm
=============================================================================

  This program reads a Group 3 FAX file and produces a PBM image.

  Bryan Henderson wrote this on August 5, 2004 and contributed it to 
  the public domain.

  This program is designed to be a drop-in replacement for the program
  of the same name that was distributed with Pbmplus and Netpbm since
  1989, written by Paul Haeberli <paul@manray.sgi.com>.

  Bryan used ideas on processing G3 data from Haeberli's code, but did
  not use any of the code.

  Others have modified the program since Bryan's initial work, each
  contributing their work to the public domain.
===========================================================================*/

#include "pbm.h"
#include "shhopt.h"
#include "nstring.h"
#include "mallocvar.h"
#include "g3.h"
#include "bitreverse.h"

#define MAXCOLS 10800
#define MAXROWS 14400   /* this allows up to two pages of image */

#define WHASHA 3510
#define WHASHB 1178

#define BHASHA 293
#define BHASHB 2695

#define HASHSIZE 1021

static g3TableEntry * whash[HASHSIZE];
static g3TableEntry * bhash[HASHSIZE];


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilespec;  /* Filespec of input file */
    unsigned int reversebits;
    unsigned int kludge;
    unsigned int stretch;
    unsigned int stop_error;
    unsigned int expectedLineSize;
};



static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry * option_def;  /* malloc'ed */
        /* Instructions to OptParseOptions3 on how to parse our options.  */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int widthSpec, paper_sizeSpec;
    const char * paperSize;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "reversebits",      OPT_FLAG,  NULL, &cmdlineP->reversebits,
            0);
    OPTENT3(0, "kludge",           OPT_FLAG,  NULL, &cmdlineP->kludge,
            0);
    OPTENT3(0, "stretch",          OPT_FLAG,  NULL, &cmdlineP->stretch, 
            0);
    OPTENT3(0, "stop_error",       OPT_FLAG,  NULL, &cmdlineP->stop_error, 
            0);
    OPTENT3(0, "width",            OPT_UINT,  &cmdlineP->expectedLineSize,
            &widthSpec,                0);
    OPTENT3(0, "paper_size",       OPT_STRING, &paperSize,
            &paper_sizeSpec,           0);
    
    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (widthSpec && paper_sizeSpec)
        pm_error("You can't specify both -width and -paper_size");

    if (widthSpec) {
        if (cmdlineP->expectedLineSize < 1)
            pm_error("-width must be at least 1");
    } else if (paper_sizeSpec) {
        if (STRCASEEQ(paperSize, "A6"))
            cmdlineP->expectedLineSize = 864;
        else if (STRCASEEQ(paperSize, "A5"))
            cmdlineP->expectedLineSize = 1216;
        else if (STRCASEEQ(paperSize, "A4"))
            cmdlineP->expectedLineSize = 1728;
        else if (STRCASEEQ(paperSize, "B4"))
            cmdlineP->expectedLineSize = 2048;
        else if (STRCASEEQ(paperSize, "A3"))
            cmdlineP->expectedLineSize = 2432;
        else
            pm_error("Unrecognized value for -paper_size '%s'.  "
                     "We recognize only A3, A4, A5, A6, and B4.",
                     paperSize);
    } else
        cmdlineP->expectedLineSize = 0;

    if (argc-1 == 0) 
        cmdlineP->inputFilespec = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->inputFilespec = argv[1];
}



struct bitStream {

    FILE * fileP;
    bool reversebits;
    int shdata;
        /* 8-bit buffer for rawgetbit(). */
    unsigned int shbit;
        /* single bit mask for the bit of 'shdata' that is next in the stream.
           zero when 'shdata' is empty.
        */
    unsigned int zeroBitCount;
        /* Number of consecutive zero bits the stream has seen.  Note that
           because an EOL mark ends in a one bit, this starts over for each
           line.
        */
};



static void
readBit(struct bitStream * const bitStreamP,
        unsigned int *     const bitP,
        const char **      const errorP) {
/*----------------------------------------------------------------------------
   Return the next raw bit from the G3 input stream.

   Do not call this outside of the bit stream object; Caller is responsible
   for maintaining object state.
-----------------------------------------------------------------------------*/
    *errorP = NULL;  /* initial assumption */

    if ((bitStreamP->shbit & 0xff) == 0) {
        bitStreamP->shdata = getc(bitStreamP->fileP);
        if (bitStreamP->shdata == EOF)
            asprintfN(errorP, "EOF or error reading file");
        else {
            bitStreamP->shbit = 0x80;
            if ( bitStreamP->reversebits )
                bitStreamP->shdata = bitreverse[ bitStreamP->shdata ];
            }
    }

    if (bitStreamP->shdata & bitStreamP->shbit)
        *bitP = 1;
    else
        *bitP = 0;

    bitStreamP->shbit >>= 1;
}



static void
readBitAndDetectEol(struct bitStream * const bitStreamP,
                    unsigned int *     const bitP,
                    bool *             const eolP,
                    const char **      const errorP) {
/*----------------------------------------------------------------------------
   Same as readBit(), but iff the bit read is the final bit of an EOL
   mark, return *eolP == TRUE.
-----------------------------------------------------------------------------*/
    readBit(bitStreamP, bitP, errorP);
    if (!*errorP) {
        bool eol;

        eol = FALSE;  /* initial assumption */
        if (*bitP == 0)
            ++bitStreamP->zeroBitCount;
        else {
            if (bitStreamP->zeroBitCount >= 11)
                eol = TRUE;
            bitStreamP->zeroBitCount = 0;
        }
        *eolP = eol;
    }
}


static void
initBitStream(struct bitStream * const bitStreamP,
              FILE *             const fileP,
              bool               const reversebits) {
    
    bitStreamP->fileP        = fileP;
    bitStreamP->reversebits  = reversebits;
    bitStreamP->shbit        = 0x00;
    bitStreamP->zeroBitCount = 0;
}



static void
skipToNextLine(struct bitStream * const bitStreamP) {

    bool eol;
    const char * error;

    eol = FALSE;
    error = NULL;
    
    while (!eol && !error) {
        unsigned int bit;
        
        readBitAndDetectEol(bitStreamP, &bit, &eol, &error);
    }
}



static void
addtohash(g3TableEntry *     hash[], 
          g3TableEntry       table[], 
          unsigned int const n, 
          int          const a, 
          int          const b) {
    
    unsigned int i;

    for (i = 0; i < n; ++i) {
        g3TableEntry * const teP = &table[i*2];
        unsigned int const pos =
            ((teP->length + a) * (teP->code + b)) % HASHSIZE;
        if (hash[pos])
            pm_error("internal error: addtohash fatal hash collision");
        hash[pos] = teP;
    }
}



static g3TableEntry*
hashfind(g3TableEntry *       hash[], 
         int          const length, 
         int          const code, 
         int          const a, 
         int          const b) {

    unsigned int pos;
    g3TableEntry * te;

    pos = ((length + a) * (code + b)) % HASHSIZE;
    te = hash[pos];
    return ((te && te->length == length && te->code == code) ? te : 0);
}



static void
buildHashes(g3TableEntry * (*whashP)[HASHSIZE],
            g3TableEntry * (*bhashP)[HASHSIZE]) {

    unsigned int i;

    for (i = 0; i < HASHSIZE; ++i)
        (*whashP)[i] = (*bhashP)[i] = NULL;

    addtohash(*whashP, &ttable[0], 64, WHASHA, WHASHB);
    addtohash(*whashP, &mtable[2], 40, WHASHA, WHASHB);

    addtohash(*bhashP, &ttable[1], 64, BHASHA, BHASHB);
    addtohash(*bhashP, &mtable[3], 40, BHASHA, BHASHB);

}



static void
makeRowWhite(bit *        const bitrow,
             unsigned int const cols) {

    unsigned int col;
    for (col = 0; col < MAXCOLS; ++col)
        bitrow[col] = PBM_WHITE;
}



static g3TableEntry *
g3code(unsigned int const curcode,
       unsigned int const curlen,
       bit          const color) {

    g3TableEntry * retval;

    switch (color) {
    case PBM_WHITE:
        if (curlen < 4)
            retval = NULL;
        else
            retval = hashfind(whash, curlen, curcode, WHASHA, WHASHB);
        break;
    case PBM_BLACK:
        if (curlen < 2)
            retval = NULL;
        else
            retval = hashfind(bhash, curlen, curcode, BHASHA, BHASHB);
        break;
    default:
        pm_error("INTERNAL ERROR: color is not black or white");
    }
    return retval;
}



enum g3tableId {TERMWHITE, TERMBLACK, MKUPWHITE, MKUPBLACK};



static void
processG3Code(g3TableEntry * const teP,
              bit *          const bitrow,
              unsigned int * const colP,
              bit *          const colorP,
              unsigned int * const countP) {
              
    enum g3tableId const teId =
        (teP > mtable ? 2 : 0) + (teP - ttable) % 2;

    unsigned int teCount;
    
    switch(teId) {
    case TERMWHITE: teCount = (teP - ttable    ) / 2;      break;
    case TERMBLACK: teCount = (teP - ttable - 1) / 2;      break;
    case MKUPWHITE: teCount = (teP - mtable    ) / 2 * 64; break;
    case MKUPBLACK: teCount = (teP - mtable - 1) / 2 * 64; break;
    }

    switch (teId) {
    case TERMWHITE:
    case TERMBLACK: {
        unsigned int runLengthSoFar;
        unsigned int col;
        
        col = *colP;
        runLengthSoFar = MIN(*countP + teCount, MAXCOLS - col);

        if (runLengthSoFar > 0) {
            if (*colorP == PBM_WHITE) {
                /* Row was initialized to white, so we just skip */
                col += runLengthSoFar;
            } else {
                unsigned int i;
                for (i = 0; i < runLengthSoFar; ++i)
                    bitrow[col++] = PBM_BLACK;
            }
        }
        *colorP = !*colorP;
        *countP = 0;
        *colP   = col;
    } break;
    case MKUPWHITE:
    case MKUPBLACK:
        *countP += teCount;
        break;
    default:
        pm_error("Can't happen");
    }
}



static void
formatBadCodeException(const char ** const exceptionP,
                       unsigned int  const col,
                       unsigned int  const curlen,
                       unsigned int  const curcode) {

    asprintfN(exceptionP,
        "bad code word at Column %u.  "
        "No prefix of the %u bits 0x%x matches any recognized "
        "code word and no code words longer than 12 bits are "
        "defined.  ",
        col, curlen, curcode);
}



static void
readFaxRow(struct bitStream * const bitStreamP,
           bit *              const bitrow,
           unsigned int *     const lineLengthP,
           const char **      const exceptionP,
           const char **      const errorP) {
/*----------------------------------------------------------------------------
  Read one line of G3 fax from the bit stream *bitStreamP into 
  bitrow[].  Return the length of the line, in pixels, as *lineLengthP.

  If there's a problem with the line, return as much of it as we can,
  advance the input stream past the next EOL mark, and put a text
  description of the problem in newly malloc'ed storage at
  *exceptionP.  If there's no problem, return *exceptionP = NULL.

  We guarantee that we make progress through the input stream.

  Iff there is an error, return a text description of it in newly
  malloc'ed storage at *errorP and all other specified behavior 
  (including return values) is unspecified.
-----------------------------------------------------------------------------*/
    unsigned int col;
    unsigned int curlen;  
        /* Number of bits we've read so far for the code we're currently 
           reading
        */
    unsigned int curcode; 
        /* What we've assembled so far of the code we're currently reading */
    unsigned int count;
        /* Number of consecutive pixels of the same color */
    bit currentColor;
        /* The color of the current run of pixels */
    g3TableEntry * te;
        /* Address of structure that describes the current G3 code */
    bool done;

    makeRowWhite(bitrow, MAXCOLS);  /* initialize row */

    col = 0;
    curlen = 0;
    curcode = 0;
    currentColor = PBM_WHITE;
    count = 0;
    *exceptionP = NULL;
    *errorP = NULL;
    done = FALSE;

    while (!done) {
        if (col >= MAXCOLS) {
            asprintfN(exceptionP, "Line is too long for this program to "
                      "handle -- longer than %u columns", MAXCOLS);
            done = TRUE;
        } else {
            unsigned int bit;
            bool eol;
            const char * error;

            readBitAndDetectEol(bitStreamP, &bit, &eol, &error);
            if (error) {
                if (col > 0)
                    /* We got at least some of the row, so it's only an
                       exception, not a fatal error.
                    */
                    *exceptionP = error;
                else
                    *errorP = error;
                done = TRUE;
            } else if (eol)
                done = TRUE;
            else {
                curcode = (curcode << 1) | bit;
                curlen++;
            
                if (curlen > 13) {
                    formatBadCodeException(exceptionP, col, curlen, curcode);
                    done = TRUE;
                } else if (curcode != 0) {
                    te = g3code(curcode, curlen, currentColor);
                    
                    if (te) {
                        processG3Code(te, bitrow, &col, &currentColor, &count);
                        
                        curcode = 0;
                        curlen = 0;
                    }
                }
            }
        }
    }
    if (*exceptionP)
        skipToNextLine(bitStreamP);

    *lineLengthP = col;
}



static void
freeBits(bit **       const bits,
         unsigned int const rows,
         bool         const stretched) {

    unsigned int row;

    for (row = 0; row < rows; ++row) {
        if (stretched && row % 2 == 1) {
            /* This is just a pointer to the previous row; don't want to
               free it twice.
            */
        } else 
            pbm_freerow(bits[row]);
    }
    free(bits);
}



static void
handleRowException(const char * const exception,
                   const char * const error,
                   unsigned int const row,
                   bool         const tolerateErrors) {


    if (exception) {
        if (tolerateErrors)
            pm_message("Problem reading Row %u.  Skipping rest of row.  %s",
                       row, exception);
        else
            pm_error("Problem reading Row %u.  Aborting.  %s", row, exception);
        strfree(exception);
    }

    if (error) {
        if (tolerateErrors)
            pm_message("Unable to read Row %u.  Skipping rest of image.  %s",
                       row, error);
        else
            pm_error("Unable to read Row %u.  Aborting.  %s", row, error);
        strfree(error);
    }
}



typedef struct {
    unsigned int expectedLineSize;
        /* The size that lines are supposed to be.  Zero means we're happy
           with any size.
        */
    unsigned int maxLineSize;
        /* The maximum line size we have seen so far, or zero if we have
           not seen any lines yet.
        */
    bool warned;
        /* We have warned the user that he has a line length problem */
    bool tolerateErrors;
        /* Try to continue when we detect a line size error, as opposed to
           aborting the program.
        */
} lineSizeAnalyzer;



static void
initializeLineSizeAnalyzer(lineSizeAnalyzer * const analyzerP,
                           unsigned int       const expectedLineSize,
                           bool               const tolerateErrors) {

    analyzerP->expectedLineSize = expectedLineSize;
    analyzerP->tolerateErrors   = tolerateErrors;

    analyzerP->maxLineSize = 0;
    analyzerP->warned      = FALSE;
}



static void
analyzeLineSize(lineSizeAnalyzer * const analyzerP,
                unsigned int       const thisLineSize) {

    const char * error;

    if (analyzerP->expectedLineSize &&
        thisLineSize != analyzerP->expectedLineSize)
        asprintfN(&error, "Image contains a line of %u pixels.  "
                  "You specified lines should be %u pixels.",
                  thisLineSize, analyzerP->expectedLineSize);
    else {
        if (analyzerP->maxLineSize && thisLineSize != analyzerP->maxLineSize)
            asprintfN(&error, "There are at least two different "
                      "line lengths in this image, "
                      "%u pixels and %u pixels.  "
                      "This is a violation of the G3 standard.  ",
                      thisLineSize, analyzerP->maxLineSize);
        else
            error = NULL;
    }

    if (error) {
        if (analyzerP->tolerateErrors) {
            if (!analyzerP->warned) {
                pm_message("Warning: %s.", error);
                analyzerP->warned = TRUE;
            }
        } else
            pm_error("%s", error);

        strfree(error);
    }
    analyzerP->maxLineSize = MAX(thisLineSize, analyzerP->maxLineSize);
}



/* An empty line means EOF.  An ancient comment in the code said there
   is supposed to 6 EOL marks in a row to indicate EOF, but the code
   checked for 3 and considered 2 in row just to mean a zero length
   line.  Starting in Netpbm 10.24 (August 2004), we assume there is
   no valid reason to have an empty line and recognize EOF as any
   empty line.  Alternatively, we could read off and ignore two empty
   lines without a 3rd.  
*/

static void
readFax(struct bitStream * const bitStreamP,
        bool               const stretch,
        unsigned int       const expectedLineSize,
        bool               const tolerateErrors,
        bit ***            const bitsP,
        unsigned int *     const colsP,
        unsigned int *     const rowsP) {

    lineSizeAnalyzer lineSizeAnalyzer;
    bit ** bits;
    const char * error;
    bool eof;
    unsigned int row;
    
    MALLOCARRAY_NOFAIL(bits, MAXROWS);

    initializeLineSizeAnalyzer(&lineSizeAnalyzer,
                               expectedLineSize, tolerateErrors);

    eof = FALSE;
    error = NULL;
    row = 0;

    while (!eof && !error) {
        unsigned int lineSize;

        if (row >= MAXROWS)
            asprintfN(&error, "Image is too tall.  This program can "
                      "handle at most %u rows", MAXROWS);
        else {
            const char * exception;

            bits[row] = pbm_allocrow(MAXCOLS);
            readFaxRow(bitStreamP, bits[row], &lineSize, &exception, &error);

            handleRowException(exception, error, row, tolerateErrors);

            if (!error) {
                if (lineSize == 0) {
                    /* EOF.  See explanation above */
                    eof = TRUE;
                } else {
                    analyzeLineSize(&lineSizeAnalyzer, lineSize);
                    
                    if (stretch) {
                        ++row;
                        if (row >= MAXROWS)
                            asprintfN(&error, "Image is too tall.  This "
                                      "program can handle at most %u rows "
                                      "after stretching", MAXROWS);
                        else
                            bits[row] = bits[row-1];
                    }
                    ++row;
                }
            }
        }
    }
    *rowsP  = row;
    *colsP  = lineSizeAnalyzer.maxLineSize;
    *bitsP  = bits;
}



int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    struct bitStream bitStream;
    unsigned int rows, cols;
    bit ** bits;

    pbm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    initBitStream(&bitStream, ifP, cmdline.reversebits);

    if (cmdline.kludge) {
        /* Skip extra lines to get in sync. */
        skipToNextLine(&bitStream);
        skipToNextLine(&bitStream);
        skipToNextLine(&bitStream);
    }
    skipToNextLine(&bitStream);

    buildHashes(&whash, &bhash);

    readFax(&bitStream, cmdline.stretch, cmdline.expectedLineSize,
            !cmdline.stop_error, 
            &bits, &cols, &rows);

    pm_close(ifP);

    pbm_writepbm(stdout, bits, cols, rows, 0);
    pm_close(stdout);

    freeBits(bits, rows, cmdline.stretch);

    return 0;
}
