/* pbmtog3.c - read a PBM image and produce a Group 3 FAX file
**
** Copyright (C) 1989 by Paul Haeberli <paul@manray.sgi.com>.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/*
   For specifications for Group 3 (G3) fax MH coding see ITU-T T.4
   This program generates only MH.  It is coded with future expansion for
   MR and MMR in mind.
*/

#include <assert.h>

#include "pbm.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "bitreverse.h"
#include "wordaccess.h"
#include "g3.h"

#define TC_MC 64

static bool const pbmtorl = 
#ifdef PBMTORL
    TRUE;
#else
    FALSE;
#endif


struct bitString {
    /* A string of bits, up to as many fit in a word. */
    unsigned int bitCount;
        /* The length of the bit string */
    wordint intBuffer;
        /* The bits are in the 'bitCount' least significant bit positions 
           of this number.  The rest of the bits of this number are always 
           zero.
        */

    /* Example:  The bit string 010100, on a machine with a 32 bit word,
       would be represented by bitCount = 6, intBuffer = 20
       (N.B. 20 = 00000000 00000000 00000000 00010100 in binary)
    */
};



struct outStream {
    struct bitString buffer;
    
    bool reverseBits;
};

/* This is a global variable for speed. */
static struct outStream out;


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFileName;
    unsigned int reversebits;
    unsigned int nofixedwidth;
    unsigned int verbose;
};



static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry * option_def;
        /* Instructions to OptParseOptions2 on how to parse our options.  */
    optStruct3 opt;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0,   "reversebits",      OPT_FLAG,  NULL, &cmdlineP->reversebits,
            0);
    OPTENT3(0,   "nofixedwidth",     OPT_FLAG,  NULL, &cmdlineP->nofixedwidth,
            0);
    OPTENT3(0,   "verbose",          OPT_FLAG,  NULL, &cmdlineP->verbose, 
            0);

    /* TODO
       Explicit fixed widths: -A4 -B4 -A3
    */

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = TRUE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 == 0) 
        cmdlineP->inputFileName = "-";
    else if (argc-1 != 1)
        pm_error("Program takes zero or one argument (filename).  You "
                 "specified %d", argc-1);
    else
        cmdlineP->inputFileName = argv[1];
}



static void
reversebuffer(unsigned char * const p, 
              unsigned int    const n) {

    unsigned int i;
    for (i = 0; i < n; ++i)
        p[i] = bitreverse[p[i]];
}



static struct bitString
makeBs(wordint      const bits, 
       unsigned int const bitCount) {

    struct bitString retval;
    retval.intBuffer = bits;
    retval.bitCount  = bitCount;

    return retval;
}

    

static __inline__ void
putbits(struct bitString const newBits) {
/*----------------------------------------------------------------------------
   Push the bits 'newBits' onto the right end of output buffer
   out.buffer (moving the bits already in the buffer left).

   Flush the buffer to stdout as necessary to make room.

   'newBits' must be shorter than a whole word.
   
   N.B. the definition of struct bitString requires upper bits to be zero.
-----------------------------------------------------------------------------*/
    unsigned int const spaceLeft = 
        sizeof(out.buffer.intBuffer)*8 - out.buffer.bitCount;
        /* Number of bits of unused space (at the high end) in buffer */

    assert(newBits.bitCount < sizeof(out.buffer.intBuffer) * 8);
    assert(newBits.intBuffer >> newBits.bitCount == 0);

    if (spaceLeft > newBits.bitCount) {
        /* New bits fit with bits to spare */
        out.buffer.intBuffer = 
            out.buffer.intBuffer << newBits.bitCount | newBits.intBuffer;
        out.buffer.bitCount += newBits.bitCount;
    } else { 
        /* New bits fill buffer.  We'll have to flush the buffer to stdout
           and put the rest of the bits in the new buffer.
        */
        unsigned int const nextBufBitCount = newBits.bitCount - spaceLeft;

        wordintBytes outbytes;
        size_t rc;

        wordintToBytes(&outbytes, 
                       (out.buffer.intBuffer << spaceLeft) 
                       | (newBits.intBuffer >> nextBufBitCount));
        if (out.reverseBits)
            reversebuffer(outbytes, sizeof(outbytes));
            
        rc = fwrite(outbytes, 1, sizeof(outbytes), stdout);
        if (rc != sizeof(outbytes))
            pm_error("Output error.  Unable to fwrite() to stdout");
        
        out.buffer.intBuffer = newBits.intBuffer & ((1<<nextBufBitCount) - 1); 
        out.buffer.bitCount = nextBufBitCount;
    }
}



static void 
initOutStream(bool const reverseBits) {
    out.buffer.intBuffer = 0;
    out.buffer.bitCount  = 0;
    out.reverseBits = reverseBits;
}



static __inline__ void
putcode(unsigned int const clr, 
        unsigned int const ix) {

    /* Note that this requires ttable to be aligned white entry, black
       entry, white, black, etc.  
    */
    putbits(makeBs(ttable[ix * 2 + clr].code, ttable[ix * 2 + clr].length));
}



static __inline__ void
putcode2(int const clr,
         int const ix) {
/*----------------------------------------------------------------------------
   Output Make-up code and Terminating code at once.

   For run lengths above TC_MC threshold (usually 64).

   The codes are combined here to avoid calculations in putbits()
   wordint is usually wide enough, with 32 or 64 bits.
   Provisions are made for 16 bit wordint (for debugging).

   Terminating code is max 12 bits, Make-up code is max 13 bits.
   (See ttable, mtable entries in pbmtog3.h)

   Also reduces object code size when putcode is compiled inline.
-----------------------------------------------------------------------------*/
    unsigned int const loIndex = ix % 64 * 2 + clr;
    unsigned int const hiIndex = ix / 64 * 2 + clr;

    if (sizeof(wordint) * 8 > 24) {
        unsigned int const l1 = ttable[loIndex].length;
        
        putbits(
            makeBs(mtable[hiIndex].code << l1 | ttable[loIndex].code,
                   mtable[hiIndex].length + l1)
            );
    } else { /* typically 16 bit wordint used for debugging */
        putbits(makeBs(mtable[hiIndex].code, mtable[hiIndex].length));
        putbits(makeBs(ttable[loIndex].code, ttable[loIndex].length));
    }
}



static __inline__ void
putspan_normal(bit          const color, 
               unsigned int const len) {

    if (len < TC_MC)
        putcode(color, len);
    else if (len < 2624)
        putcode2(color, len);
    else {  /* len >= 2624 : rare */
        unsigned int remainingLen;

        for (remainingLen = len;
             remainingLen >= 2624;
             remainingLen -= 2623) {

            putcode2(color, 2560+63);
            putcode(!color, 0);
        }
        if (remainingLen < TC_MC)
            putcode(color, remainingLen);
        else  /* TC_MC <= len < 2624 */
            putcode2(color, remainingLen);
    }
}



static __inline__ void
putspan(bit          const color, 
        unsigned int const len) {
/*----------------------------------------------------------------------------
   Put a span of 'len' pixels of color 'color' in the output.
-----------------------------------------------------------------------------*/
    if (pbmtorl) {
        if (len > 0) 
            printf("%c %d\n", color == PBM_WHITE ? 'W' : 'B', len);
    } else 
        putspan_normal(color, len);
}



static void
puteol(void) {

    if (pbmtorl)
        puts("EOL");
    else {
        struct bitString const eol = {12, 1};
            
        putbits(eol);
    }
}



/*
  PBM raw bitrow to inflection point array

  Write inflection (=color change) points into array milepost[].  
  It is easy to calculate run length from this.

  In milepost, a white-to-black (black-to-white) inflection point
  always has an even (odd) index.  A line starting with black is
  indicated by bitrow[0] == 0.

  WWWWWWWBBWWWWWWW ... = 7,2,7, ...
  BBBBBWBBBBBWBBBB ... = 0,5,1,5,1,4, ...

  Return the number of milepost elements written.
  Note that max number of entries into milepost = cols+1 .

  The inflection points are calculated like this:

   r1: 00000000000111111110011111000000
   r2: c0000000000011111111001111100000 0->carry
  xor: ?0000000000100000001010000100000

  The 1 bits in the xor above are the inflection points.
*/

static __inline__ void
convertRowToRunLengths(unsigned char * const bitrow, 
                       int             const cols, 
                       unsigned int *  const milepost,
                       unsigned int *  const lengthP) {

    unsigned int   const bitsPerWord  = sizeof(wordint) * 8;
    wordint      * const bitrowByWord = (wordint *) bitrow;
    int            const wordCount    = (cols + bitsPerWord - 1)/bitsPerWord; 
        /* Number of full and partial words in the row */
        

    if (cols % bitsPerWord != 0) {
        /* Clean final word in row.  For loop simplicity */
        wordint r1;
        r1 = bytesToWordint((unsigned char *)&bitrowByWord[wordCount - 1]);
        r1 >>= bitsPerWord - cols % bitsPerWord;
        r1 <<= bitsPerWord - cols % bitsPerWord;
        wordintToBytes((wordintBytes *)&bitrowByWord[wordCount - 1], r1);
    }
    {

        wordint carry;
        wordint r1, r2;
        unsigned int n;
        int i,c,k;

        for (i = carry = n = 0; i < wordCount; ++i) {
            r1 = r2 = bytesToWordint((unsigned char *)&bitrowByWord[i]);
            r2 = r1 ^ (carry << (bitsPerWord-1) | r2 >> 1);
            carry = r1 & 0x1;  k = 0;
            while (r2 != 0) {
                /* wordintClz(r2) reports most significant "1" bit of r2
                   counting from MSB = position 0.
                */
                c = wordintClz(r2);
                milepost[n++] = i * bitsPerWord + k + c;
                r2 <<= c++; r2 <<= 1;  k += c; 
            } 
        }
        if (n == 0 || milepost[n - 1] != cols) 
            milepost[n++] = cols;
        *lengthP = n;
    }
}



static void
padToDesiredWidth(unsigned int * const milepost,
                  unsigned int * const nRunP,
                  int            const existingCols,
                  int            const desiredCols) {

    if (existingCols < desiredCols) {
        /* adjustment for narrow input in fixed width mode
           nRun % 2 == 1 (0) means last (=rightmost) pixel is white (black)
           if white, extend the last span to outwidth
           if black, fill with a white span len (outwidth - readcols)
        */
        if (*nRunP % 2 == 0)
            ++*nRunP;
        milepost[*nRunP - 1] = desiredCols;
    }
}



int
main(int    argc,
     char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    unsigned char * bitrow;
       /* This is the bits of the current row, as read from the input and
           modified various ways at various points in the program.  It has
           a word of zero padding on the high (right) end for the convenience
           of code that accesses this buffer in word-size bites.
        */
     
    int rows;
    int cols;
    int readcols;
    int outwidth;
    int format;
    int row;
    unsigned int * milepost;

    pbm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
     
    ifP = pm_openr(cmdline.inputFileName);

    pbm_readpbminit(ifP, &cols, &rows, &format);
    if (cmdline.nofixedwidth)
        readcols = outwidth = cols;
    else {
        readcols = MIN(cols, 1728);
        outwidth = 1728;
    }

    MALLOCARRAY_NOFAIL(bitrow, pbm_packed_bytes(cols) + sizeof(wordint));

    MALLOCARRAY_NOFAIL(milepost, readcols + 1);

    initOutStream(cmdline.reversebits);
    puteol();

    for (row = 0; row < rows; ++row) {
        unsigned int nRun;  /* Number of runs in milepost[] */
        unsigned int p;
        unsigned int i;

        pbm_readpbmrow_packed(ifP, bitrow, cols, format);
        
        convertRowToRunLengths(bitrow, readcols, milepost, &nRun);
        
        padToDesiredWidth(milepost, &nRun, readcols, outwidth);

        for (i = p = 0; i < nRun; p = milepost[i++])
            putspan(i%2 == 0 ? PBM_WHITE : PBM_BLACK, milepost[i] - p);
        /* TODO 2-dimensional coding MR, MMR */
        puteol();
    }

    free(milepost);
    {
        unsigned int i;  
        for( i = 0; i < 6; ++i)
            puteol();
    }
    if (out.buffer.bitCount > 0) {
        /* flush final partial buffer */
        unsigned int const bytesToWrite = (out.buffer.bitCount+7)/8;
        
        unsigned char outbytes[sizeof(wordint)];
        size_t rc;
        wordintToBytes(&outbytes, 
                       out.buffer.intBuffer << (sizeof(out.buffer.intBuffer)*8 
                                                - out.buffer.bitCount));
        if (out.reverseBits)
            reversebuffer(outbytes, bytesToWrite);
        rc = fwrite(outbytes, 1, bytesToWrite, stdout);
        if (rc != bytesToWrite)
            pm_error("Output error");
    }
    pm_close(ifP);

    return 0;
}
