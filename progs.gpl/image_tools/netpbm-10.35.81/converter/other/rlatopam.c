/** rlatopam.c - read Alias/Wavefront RLA or RPF file
 **
 ** Copyright (C) 2005 Matte World Digital
 **
 ** Author: Simon Walton
 **
 ** Permission to use, copy, modify, and distribute this software and its
 ** documentation for any purpose and without fee is hereby granted, provided
 ** that the above copyright notice appear in all copies and that both that
 ** copyright notice and this permission notice appear in supporting
 ** documentation.  This software is provided "as is" without express or
 ** implied warranty.
 **/


#include <string.h>
#include <errno.h>

#include "shhopt.h"
#include "mallocvar.h"
#include "pam.h"
#include "rla.h"

static int * offsets;
static bool is_float;
static bool has_matte;

static unsigned int depth;
static unsigned int width;
static unsigned int height;
static unsigned int chanBits;
static short storageType;
static struct pam outpam;
static unsigned int numChan;


static void
parseCommandLine(int           const argc,
                 char **       const argv,
                 const char ** const inputFileNameP) {

    if (argc-1 < 1)
        *inputFileNameP = "-";
    else {
        *inputFileNameP = argv[1];

        if (argc-1 > 1)
            pm_error("There is at most one argument - input file name.  "
                     "You specified %u", argc-1);
    }
}



static bool littleEndian;


static void
determineEndianness(void) {

    union {
        unsigned char bytes[2];
        unsigned short number;
    } u;

    u.number = 1;

    littleEndian = (u.bytes[0] == 1);
}



static unsigned short
byteswap(unsigned short const input) {

    return (input << 8) | (input >> 8);
}



static void
read_header(FILE *   const ifP,
            rlahdr * const hdrP) {

    rlahdr hdr;
    size_t bytesRead;
    
    fseek (ifP, 0, SEEK_SET);
    
    /* Here we have a hack.  The bytes in the file are almost in the
       same format as the compiler stores 'hdr' in memory.  The only
       difference is that the compiler may store the integer values
       in the obscene little-endian format.  So we just read the whole
       header into 'hdr' as if it were the right format, and then
       correct the integer values by swapping their bytes.

       The _right_ way to do this is to read the file one field at a time,
       using pm_readbigshort() where appropriate.
    */

    bytesRead = fread(&hdr, sizeof hdr, 1, ifP);
    if (bytesRead != 1)
        pm_error("Unexpected EOF on input file.");

    if (littleEndian) {
        hdr.window.left          = byteswap(hdr.window.left);
        hdr.window.right         = byteswap(hdr.window.right);
        hdr.window.bottom        = byteswap(hdr.window.bottom);
        hdr.window.top           = byteswap(hdr.window.top);
        hdr.active_window.left   = byteswap(hdr.active_window.left);
        hdr.active_window.right  = byteswap(hdr.active_window.right);
        hdr.active_window.bottom = byteswap(hdr.active_window.bottom);
        hdr.active_window.top    = byteswap(hdr.active_window.top);
        hdr.storage_type         = byteswap(hdr.storage_type);
        hdr.num_chan             = byteswap(hdr.num_chan);
        hdr.num_matte            = byteswap(hdr.num_matte);
        hdr.revision             = byteswap(hdr.revision);
        hdr.chan_bits            = byteswap(hdr.chan_bits);
        hdr.matte_type           = byteswap(hdr.matte_type);
        hdr.matte_bits           = byteswap(hdr.matte_bits);
    }

    if (hdr.revision != 0xfffe && hdr.revision != 0xfffd)
        pm_error("Invalid file header.  \"revision\" field should contain "
                 "0xfffe or 0xfffd, but contains 0x%04x", hdr.revision);

    *hdrP = hdr;
}



static void
decodeFP(unsigned char * const in,
         unsigned char * const out,
         int             const width,
         int             const stride) {

    unsigned int x;
    unsigned char * inputCursor;
    unsigned char * outputCursor;

    inputCursor = &in[0];

    for (x = 0; x < width; ++x) {
        union {char bytes [4]; float fv;} fi;
        unsigned short val;

        if (littleEndian) {
            fi.bytes [3] = *inputCursor++;
            fi.bytes [2] = *inputCursor++;
            fi.bytes [1] = *inputCursor++;
            fi.bytes [0] = *inputCursor++;
        } else {
            fi.bytes [0] = *inputCursor++;
            fi.bytes [1] = *inputCursor++;
            fi.bytes [2] = *inputCursor++;
            fi.bytes [3] = *inputCursor++;
        }

        val = fi.fv > 1 ? 65535 : (fi.fv < 0 ? 0 :
                                   (unsigned short) (65535 * fi.fv + .5));
        outputCursor[0] = val >> 8; outputCursor[1] = val & 0xff;
        outputCursor += stride;
    }
}



static unsigned char *
decode(unsigned char * const input,
       unsigned char * const output,
       int             const xFile,
       int             const xImage,
       int             const stride) {

    int x;
    unsigned int bytes;
    unsigned int useX;
    unsigned char * inputCursor;
    unsigned char * outputCursor;

    inputCursor = &input[0];
    outputCursor = &output[0];
    x = xFile;
    bytes = 0;
    useX = 0;
    
    while (x > 0) {
        int count;

        count = *(signed char *)inputCursor++;
        ++bytes;

        if (count >= 0) {
            /* Repeat pixel value (count + 1) times. */
            while (count-- >= 0) {
                if (useX < xImage) {
                    *outputCursor = *inputCursor;
                    outputCursor += stride;
                }
                --x;
                ++useX;
            }
            ++inputCursor;
            ++bytes;
        } else {
            /* Copy (-count) unencoded values. */
            for (count = -count; count > 0; --count) {
                if (useX < xImage) {
                    *outputCursor = *inputCursor;
                    outputCursor += stride;
                }
                ++inputCursor;
                ++bytes;
                --x;
                ++useX;
            }
        }
    }
    return inputCursor;
}



static void
decode_row(FILE *          const ifP,
           int             const row,
           unsigned char * const rb) {

    static unsigned char * read_buffer = NULL;
    unsigned int chan;
    int rc;

    if (!read_buffer) {
        MALLOCARRAY(read_buffer, width * 4);
        if (read_buffer == 0)
            pm_error("Unable to get memory for read_buffer");
    }

    rc = fseek (ifP, offsets [height - row - 1], SEEK_SET);
    if (rc != 0)
        pm_error("fseek() failed with errno %d (%s)",
                 errno, strerror(errno));

    for (chan = 0; chan < outpam.depth; ++chan) {
        unsigned short length;
        size_t bytesRead;
        
        pm_readbigshortu(ifP, &length);
        if (length > width * 4)
            pm_error("Line too long - row %u, channel %u", row, chan);

        bytesRead = fread(read_buffer, 1, length, ifP);
        if (bytesRead != length)
            pm_error("EOF encountered unexpectedly");

        if (is_float)
            decodeFP(read_buffer, rb + chan * 2, width, outpam.depth * 2);
        else if (depth > 8) {
            /* Hi byte */
            unsigned char * const newpos =
                decode(read_buffer, rb + chan * 2, width, width,
                       outpam.depth * 2);
            /* Lo byte */
            decode(newpos, rb + chan * 2 + 1, width, width,
                   outpam.depth * 2);
        } else
            decode(read_buffer, rb + chan, width, width, outpam.depth); 
    }
}



static void
getHeaderInfo(FILE *         const ifP,
              unsigned int * const widthP,
              unsigned int * const heightP,
              unsigned int * const depthP,
              bool *         const hasMatteP,
              unsigned int * const chanBitsP,
              short *        const storageType) {
    
    rlahdr hdr;
    int width, height;

    read_header(ifP, &hdr);

    height = hdr.active_window.top - hdr.active_window.bottom + 1;
    width  = hdr.active_window.right - hdr.active_window.left + 1;
    if (width <=0)
        pm_error("Invalid input image.  It says its width isn't positive");
    if (height <=0)
        pm_error("Invalid input image.  It says its height isn't positive");

    *widthP  = width;
    *heightP = height;

    if (hdr.num_chan != 1 && hdr.num_chan != 3)
        pm_error ("Input image has bad number of channels: %d.  "
                  "Should be 1 or 3.",
                  hdr.num_chan);

    *depthP = hdr.chan_bits <= 8 ? 8 : 16;

    *hasMatteP = (hdr.num_matte > 0);
    *chanBitsP = hdr.chan_bits;
}



static void
readOffsetArray(FILE *       const ifP,
                int **       const offsetsP,
                unsigned int const height) {

    int * offsets;
    unsigned int row;

    MALLOCARRAY(offsets, height);
    if (offsets == NULL)
        pm_error("Unable to allocate memory for the offsets array");

    for (row = 0; row < height; ++row) {
        long l;
        pm_readbiglong(ifP, &l);
        offsets[row] = l;
    }
    *offsetsP = offsets;
}



static void
destroyOffsetArray(int * const offsets) {

    free(offsets);
}



static void
readAndWriteRaster(FILE *             const ifP,
                   const struct pam * const outpamP) {

    unsigned char * rowBuffer;
    tuple * tuplerow;
    unsigned int row;

    /* Hold one row of all image planes */
    rowBuffer = calloc(1, width * outpamP->depth * 4);
    if (rowBuffer == NULL)
        pm_error("Unable to allocate memor for row buffer.");

    tuplerow = pnm_allocpamrow(outpamP);

    for (row = 0; row < height; ++row) {
        unsigned int col;
        unsigned char * rbP;

        decode_row(ifP, row, rowBuffer);
        for (col = 0, rbP = rowBuffer; col < width; ++col) {
            unsigned int chan;
            for (chan = 0; chan < outpamP->depth; ++chan) {
                if (depth > 8) {
                    tuplerow[col][chan] = 256 * rbP[0] + rbP[1];
                    rbP += 2;
                } else {
                    tuplerow[col][chan] = *rbP;
                    rbP += 1;
                }
            }
        }
        pnm_writepamrow(outpamP, tuplerow);
    }
    pnm_freepamrow(tuplerow);
    free(rowBuffer);
}



int
main(int    argc,
     char * argv[]) {

    const char * inputFileName;
    FILE * ifP;

    pnm_init(&argc, argv);

    determineEndianness();

    parseCommandLine(argc, argv, &inputFileName);

    ifP = pm_openr_seekable(inputFileName);

    getHeaderInfo(ifP, &width, &height, &depth, &has_matte,
                  &chanBits, &storageType);

    outpam.size   = outpam.len = sizeof (struct pam);
    outpam.file   = stdout;
    outpam.format = PAM_FORMAT;
    outpam.height = height;
    outpam.width  = width;
    outpam.depth  = numChan + (has_matte ? 1 : 0);
    outpam.maxval = (1 << (chanBits > 16 ? 
                           (9 + (chanBits - 1) % 8)
                                /* Take top 2 of 3 or 4 bytes */
                           : chanBits)) - 1;

    /* Most apps seem to assume 32 bit integer is really floating point */
    if (chanBits == 32 || storageType == 4) {
        is_float = TRUE;
        outpam.maxval = 65535;
        depth = 16;
    }

    outpam.bytes_per_sample = depth / 8;
    strcpy(outpam.tuple_type, (numChan == 3 ? "RGB" : "GRAYSCALE"));
    if (has_matte)
        strcat(outpam.tuple_type, "A");

    readOffsetArray(ifP, &offsets, height);

    pnm_writepaminit(&outpam);

    readAndWriteRaster(ifP, &outpam);

    destroyOffsetArray(offsets);
    
    pm_close(ifP);

    return 0; 
}
