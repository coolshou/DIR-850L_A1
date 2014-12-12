/* escp2topbm.c - read an Epson ESC/P2 printer file and
**                 create a pbm file from the raster data,
**                 ignoring all other data.
**                 Can be regarded as a simple raster printer emulator
**                 with a RLE run length decoder.
**                 This program was made primarily for the test of pbmtoescp2
**
** Copyright (C) 2003 by Ulrich Walcher (u.walcher@gmx.de)
**                       and Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <string.h>

#include "pbm.h"
#include "mallocvar.h"

/* RLE decoder */
static unsigned int 
dec_epson_rle(unsigned        const int k, 
              unsigned        const char * in, 
              unsigned char * const out) {

    unsigned int i;
    unsigned int pos;
    unsigned int dpos;

    pos = 0;  /* initial value */
    dpos = 0; /* initial value */

    while (dpos < k) {
        if (in[pos] < 128) {
            for (i = 0; i < in[pos] + 1; ++i)
                out[dpos+i] = in[pos + i + 1];     
            /* copy through */
            pos += i + 1;
        } else {
            for (i = 0; i < 257 - in[pos]; ++i)
                out[dpos + i] = in[pos + 1];  
            /* inflate this run */
            pos += 2;
        }
        dpos += i;
    }
    return pos;        /* return number of treated input bytes */
}



int
main(int    argc,
     char * argv[]) {

    unsigned int const size = 4096; /* arbitrary value */

    FILE *ifP;
    unsigned int i, len, pos, opos, width, height;
    unsigned char *input, *output;
    const char * fileName;

    pbm_init(&argc, argv);

    MALLOCARRAY(input, size);
    MALLOCARRAY(output, size);
    
    if (input == NULL || output == NULL)
        pm_error("Cannot allocate memory");

    if (argc-1 > 1)
        pm_error("Too many arguments (%u).  Only argument is filename.",
                 argc-1);

    if (argc == 2)
        fileName = argv[1];
    else
        fileName = "-";

    ifP = pm_openr(fileName);

    /* read the whole file */
    len = 0;  /* initial value */
    for (i = 0; !feof(ifP); ++i) {
        size_t bytesRead;
        REALLOCARRAY(input, (i+1) * size);
        if (input == NULL)
            pm_error("Cannot allocate memory");
        bytesRead = fread(input + i * size, 1, size, ifP);
        len += bytesRead;
    }

    /* filter out raster data */
    height = 0;  /* initial value */
    pos = 0;     /* initial value */
    opos = 0;    /* initial value */

    while (pos < len) {
        /* only ESC sequences are regarded  */
        if (input[pos] == '\x1b' && input[pos+1] == '.') {
            unsigned int const k =
                input[pos+5] * ((input[pos+7] * 256 + input[pos+6] + 7) / 8);
            height += input[pos+5];
            width = input[pos+7] * 256 + input[pos+6];
            REALLOCARRAY(output, opos + k);
            if (output == NULL)
                pm_error("Cannot allocate memory");

            switch (input[pos+2]) {
            case 0:
                /* copy the data block */
                memcpy(output + opos, input + pos + 8, k);        
                pos += k + 8;
                opos += k;
                break;
            case 1: {
                /* inflate the data block */
                unsigned int l;
                l = dec_epson_rle(k,input+pos+8,output+opos);  
                pos += l + 8;
                opos += k;
            }
                break;
            default:
                pm_error("unknown compression mode");
                break;
            }
        }
        else
            ++pos;      /* skip bytes outside the ESCX sequence */
    }

    pbm_writepbminit(stdout, width, height, 0);
    fwrite(output, opos, 1, stdout);
    free(input); free(output);
    fclose(stdout); fclose(ifP);

    return 0;
}
