/* pbmtolj.c - read a portable bitmap and produce a LaserJet bitmap file
**  
**  based on pbmtops.c
**
**  Michael Haberler HP Vienna mah@hpuviea.uucp
**                 mcvax!tuvie!mah
**  misfeatures: 
**      no positioning
**
**      Bug fix Dec 12, 1988 :
**              lines in putbit() reshuffled 
**              now runs OK on HP-UX 6.0 with X10R4 and HP Laserjet II
**      Bo Thide', Swedish Institute of Space Physics, Uppsala <bt@irfu.se>
**
**  Flags added December, 1993:
**      -noreset to suppress printer reset code
**      -float to suppress positioning code (such as it is)
**  Wim Lewis, Seattle <wiml@netcom.com>
**
** Copyright (C) 1988 by Jef Poskanzer and Michael Haberler.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "pbm.h"
#include "shhopt.h"
#include "mallocvar.h"
#include <string.h>
#include <assert.h>

static char *rowBuffer, *prevRowBuffer, *packBuffer, *deltaBuffer;
static int rowBufferSize, rowBufferIndex, prevRowBufferIndex;
static int packBufferSize, packBufferIndex;
static int deltaBufferSize, deltaBufferIndex;

static int item, bitsperitem, bitshift;



struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilename;
    unsigned int dpi;
    unsigned int copies;     /* number of copies */
    unsigned int floating;   /* suppress the ``ESC & l 0 E'' ? */
    unsigned int noreset;
    unsigned int pack;       /* use TIFF packbits compression */
    unsigned int delta;      /* use row-delta compression */
};


static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    unsigned int dpiSpec, copiesSpec, compressSpec;

    MALLOCARRAY(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0,   "resolution",  OPT_UINT, &cmdlineP->dpi, 
            &dpiSpec, 0);
    OPTENT3(0,   "copies",      OPT_UINT, &cmdlineP->copies,
            &copiesSpec, 0);
    OPTENT3(0,   "float",       OPT_FLAG, NULL,
            &cmdlineP->floating, 0);
    OPTENT3(0,   "noreset",     OPT_FLAG, NULL,
            &cmdlineP->noreset, 0);
    OPTENT3(0,   "packbits",    OPT_FLAG, NULL,
            &cmdlineP->pack, 0);
    OPTENT3(0,   "delta",       OPT_FLAG, NULL,
            &cmdlineP->delta, 0);
    OPTENT3(0,   "compress",    OPT_FLAG, NULL,
            &compressSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 == 0) 
        cmdlineP->inputFilename = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->inputFilename = argv[1];

    if (!dpiSpec)
        cmdlineP->dpi = 75;
    if (!copiesSpec)
        cmdlineP->copies = 1;
    if (compressSpec) {
        cmdlineP->pack = 1;
        cmdlineP->delta = 1;
    }
}



static void
allocateBuffers(unsigned int const cols) {

    rowBufferSize = (cols + 7) / 8;
    packBufferSize = rowBufferSize + (rowBufferSize + 127) / 128 + 1;
    deltaBufferSize = rowBufferSize + rowBufferSize / 8 + 10;

    MALLOCARRAY_NOFAIL(prevRowBuffer, rowBufferSize);
    MALLOCARRAY_NOFAIL(rowBuffer, rowBufferSize);
    MALLOCARRAY_NOFAIL(packBuffer, packBufferSize);
    MALLOCARRAY_NOFAIL(deltaBuffer, deltaBufferSize);
}



static void
freeBuffers(void) {

    free(deltaBuffer);
    free(packBuffer);
    free(rowBuffer);
    free(prevRowBuffer);
}



static void
putinit(struct cmdlineInfo const cmdline) {
    if (!cmdline.noreset) {
        /* Printer reset. */
        printf("\033E");
    }

    if (cmdline.copies > 1) {
        /* number of copies */
        printf("\033&l%dX", cmdline.copies);
    }
    if (!cmdline.floating) {
        /* Ensure top margin is zero */
        printf("\033&l0E");
    }

    /* Set raster graphics resolution */
    printf("\033*t%dR", cmdline.dpi);

    /* Start raster graphics, relative adressing */
    printf("\033*r1A");

    bitsperitem = 1;
    item = 0;
    bitshift = 7;
}



static void
putitem(void) {
    assert(rowBufferIndex < rowBufferSize);
    rowBuffer[rowBufferIndex++] = item;
    bitsperitem = 0;
    item = 0;
}



static void
putbit(bit const b) {

    if (b == PBM_BLACK)
        item += 1 << bitshift;

    --bitshift;

    if (bitsperitem == 8) {
        putitem();
        bitshift = 7;
    }
    ++bitsperitem;
}



static void
putflush(void) {
    if (bitsperitem > 1)
        putitem();
}



static void
putrest(bool const reset) {
    /* end raster graphics */
    printf("\033*rB");

    if (reset) {
        /* Printer reset. */
        printf("\033E");
    }
}



static void
packbits(void) {
    int ptr, litStart, runStart, thisByte, startByte, chew, spit;
    packBufferIndex = 0;
    ptr = 0;
    while (ptr < rowBufferIndex) {
        litStart = ptr;
        runStart = ptr;
        startByte = rowBuffer[ptr];
        ++ptr;
        while (ptr < rowBufferIndex) {
            thisByte = rowBuffer[ptr];
            if (thisByte != startByte) {
                if (ptr - runStart > 3) {
                    /* found literal after nontrivial run */
                    break;
                }
                startByte = thisByte;
                runStart = ptr;
            }
            ++ptr;
        }
        /*
          We drop out here after having found a [possibly empty]
          literal, followed by a [possibly degenerate] run of repeated
          bytes.  Degenerate runs can occur at the end of the scan line...
          there may be a "repeat" of 1 byte (which can't actually be 
          represented as a repeat) so we simply fold it into the previous
          literal.
        */
        if (runStart == rowBufferIndex - 1) {
            runStart = rowBufferIndex;
        }
        /*
          Spit out the leading literal if it isn't empty
        */
        chew = runStart - litStart;
        while (chew > 0) {
            spit = (chew > 127) ? 127 : chew;
            packBuffer[packBufferIndex++] = (char) (spit - 1);
            memcpy(packBuffer+packBufferIndex, rowBuffer+litStart, spit);
            packBufferIndex += spit;
            litStart += spit;
            chew -= spit;
        }
        /*
          Spit out the repeat, if it isn't empty
        */
        chew = ptr - runStart;
        while (chew > 0) {
            spit = (chew > 128) ? 128 : chew;
            if (chew == spit + 1) {
                spit--; /* don't leave a degenerate run at the end */
            }
            if (spit == 1) {
                fprintf(stderr, "packbits created a degenerate run!\n");
            }
            packBuffer[packBufferIndex++] = (char) -(spit - 1);
            packBuffer[packBufferIndex++] = startByte;
            chew -= spit;
        }
    }
}



static void
deltarow(void) {
    int burstStart, burstEnd, burstCode, mustBurst, ptr, skip, skipped, code;
    deltaBufferIndex = 0;
    if (memcmp(rowBuffer, prevRowBuffer, rowBufferIndex) == 0) {
        return; /* exact match, no deltas required */
    }
    ptr = 0;
    skipped = 0;
    burstStart = -1;
    burstEnd = -1;
    mustBurst = 0;
    while (ptr < rowBufferIndex) {
        skip = 0;
        if (ptr == 0 || skipped == 30 || rowBuffer[ptr] != prevRowBuffer[ptr]
            || (burstStart != -1 && ptr == rowBufferIndex - 1)) {
            /* we want to output this byte... */
            if (burstStart == -1) {
                burstStart = ptr;
            }
            if (ptr - burstStart == 7 || ptr == rowBufferIndex - 1) {
                /* we have to output it now... */
                burstEnd = ptr;
                mustBurst = 1;
            }
        } else {
            /* duplicate byte, we can skip it */
            if (burstStart != -1) {
                burstEnd = ptr - 1;
                mustBurst = 1;
            }
            skip = 1;
        }
        if (mustBurst) {
            burstCode = burstEnd - burstStart; 
                /* 0-7, means 1-8 bytes follow */
            code = (burstCode << 5) | skipped;
            deltaBuffer[deltaBufferIndex++] = (char) code;
            memcpy(deltaBuffer+deltaBufferIndex, rowBuffer+burstStart, 
                   burstCode + 1);
            deltaBufferIndex += burstCode + 1;
            burstStart = -1;
            burstEnd = -1;
            mustBurst = 0;
            skipped = 0;
        }
        if (skip) {
            ++skipped;
        }
        ++ptr;
    }
}



static void
findRightmostBlackCol(const bit *    const bitrow,
                      unsigned int   const cols,
                      bool *         const allWhiteP,
                      unsigned int * const blackColP) {

    int i;

    for (i = cols - 1; i >= 0 && bitrow[i] == PBM_WHITE; --i);

    if (i < 0)
        *allWhiteP = TRUE;
    else {
        *allWhiteP = FALSE;
        *blackColP = i;
    }
}



static void
convertRow(const bit *    const bitrow,
           unsigned int   const cols,
           bool           const pack,
           bool           const delta,
           bool *         const rowIsBlankP) {

    unsigned int rightmostBlackCol;
        
    findRightmostBlackCol(bitrow, cols, rowIsBlankP, &rightmostBlackCol);

    if (!*rowIsBlankP) {
        unsigned int const nzcol = rightmostBlackCol + 1;
            /* Number of columns excluding white right margin */
        unsigned int const rucols = ((nzcol + 7) / 8) * 8;
            /* 'nzcol' rounded up to nearest multiple of 8 */
        
        unsigned int col;

        memset(rowBuffer, 0, rowBufferSize);

        rowBufferIndex = 0;

        /* Generate the unpacked data */

        for (col = 0; col < nzcol; ++col)
            putbit(bitrow[col]);

        /* Pad out to a full byte with white */
        for (col = nzcol; col < rucols; ++col)
            putbit(0);

        putflush();

        /* Try optional compression algorithms */

        if (pack)
            packbits();
        else
            packBufferIndex = rowBufferIndex + 999;

        if (delta) {
            /* May need to temporarily bump the row buffer index up to
               whatever the previous line's was - if this line is shorter 
               than the previous would otherwise leave dangling cruft.
            */
            unsigned int const savedRowBufferIndex = rowBufferIndex;

            if (rowBufferIndex < prevRowBufferIndex)
                rowBufferIndex = prevRowBufferIndex;

            deltarow();

            rowBufferIndex = savedRowBufferIndex;
        } else
            deltaBufferIndex = packBufferIndex + 999;
    }
}


    
static void
printBlankRows(unsigned int const count) {

    if (count > 0) {
        unsigned int x;
        /* The code used to be this, but Charles Howes reports that
           this escape sequence does not exist on his HP Laserjet IIP
           plus, so we use the following less elegant code instead.
           
           printf("\033*b%dY", (*blankRowsP)); 
        */
        for (x = 0; x < count; ++x) 
            printf("\033*b0W");
        
        memset(prevRowBuffer, 0, rowBufferSize);
    }
}



static void
establishMode(int const newMode) {

    static int mode = -1;

    if (mode != newMode) {
        printf("\033*b%uM", newMode);
        mode = newMode;
    }
}



static void
printRow(void) {

    if (deltaBufferIndex < packBufferIndex &&
        deltaBufferIndex < rowBufferIndex) {
        assert(deltaBufferIndex <= deltaBufferSize);
        /*
          It's smallest when delta'ed
        */
        establishMode(3);

        printf("\033*b%dW", deltaBufferIndex);
        fwrite(deltaBuffer, 1, deltaBufferIndex, stdout);
    } else if (rowBufferIndex <= packBufferIndex) {
        assert (rowBufferIndex <= rowBufferSize);
        /*
          It didn't pack - send it unpacked
        */
        establishMode(0);

        printf("\033*b%dW", rowBufferIndex);
        fwrite(rowBuffer, 1, rowBufferIndex, stdout);
    } else {
        assert (packBufferIndex <= packBufferSize);
        /*
          It's smaller when packed
        */
        establishMode(2);

        printf("\033*b%dW", packBufferIndex);
        fwrite(packBuffer, 1, packBufferIndex, stdout);
    }
    memcpy(prevRowBuffer, rowBuffer, rowBufferSize);
    prevRowBufferIndex = rowBufferIndex;
}



static void
doPage(FILE *             const ifP, 
       struct cmdlineInfo const cmdline) {

    bit * bitrow;
    int rows, cols, format, row;
    unsigned int blankRows;
    bool rowIsBlank;

    pbm_readpbminit(ifP, &cols, &rows, &format);

    bitrow = pbm_allocrow(cols);

    allocateBuffers(cols);

    putinit(cmdline);

    blankRows = 0;
    prevRowBufferIndex = 0;
    memset(prevRowBuffer, 0, rowBufferSize);

    for (row = 0; row < rows; ++row) {
        pbm_readpbmrow(ifP, bitrow, cols, format);

        convertRow(bitrow, cols, cmdline.pack, cmdline.delta,
                   &rowIsBlank);

        if (rowIsBlank)
            ++blankRows;
        else {
            printBlankRows(blankRows);
            blankRows = 0;
            
            printRow();
        }
    }    
    printBlankRows(blankRows);
    blankRows = 0;

    putrest(!cmdline.noreset);

    freeBuffers();
    pbm_freerow(bitrow);
}



int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    bool eof;

    pbm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilename);

    eof = FALSE;
    while (!eof) {
        doPage(ifP, cmdline);
        pbm_nextimage(ifP, &eof);
    }

    pm_close(ifP);

    return 0;
}
