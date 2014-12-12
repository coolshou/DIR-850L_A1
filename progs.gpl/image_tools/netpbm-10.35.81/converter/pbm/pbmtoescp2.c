/* pbmtoescp2.c - read a portable bitmap and produce Epson ESC/P2 raster
**                 graphics output data for Epson Stylus printers
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

/* I used the Epson ESC/P Reference Manual (1997) in writing this. */

#include <string.h>

#include "pbm.h"
#include "shhopt.h"

static char const esc = 033;

struct cmdlineInfo {
    const char * inputFileName;
    unsigned int resolution;
    unsigned int compress;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {

    optStruct3 opt;
    unsigned int option_def_index = 0;
    optEntry *option_def = malloc(100*sizeof(optEntry));

    unsigned int compressSpec, resolutionSpec;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;
    opt.allowNegNum = FALSE;
    OPTENT3(0, "compress",     OPT_UINT,    &cmdlineP->compress,    
            &compressSpec, 0);
    OPTENT3(0, "resolution",   OPT_UINT,    &cmdlineP->resolution,  
            &resolutionSpec, 0);
    
    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
    
    if (argc-1 > 1)
        pm_error("Too many arguments: %d.  "
                 "Only argument is the filename", argc-1);

    if (compressSpec) {
        if (cmdlineP->compress != 0 && cmdlineP->compress != 1)
            pm_error("Invalid -compress value: %u.  Only 0 and 1 are valid.",
                     cmdlineP->compress);
    } else
        cmdlineP->compress = 1;

    if (resolutionSpec) {
        if (cmdlineP->resolution != 360 && cmdlineP->resolution != 180)
            pm_error("Invalid -resolution value: %u.  "
                     "Only 180 and 360 are valid.", cmdlineP->resolution);
    } else
        cmdlineP->resolution = 360;

    if (argc-1 == 1)
        cmdlineP->inputFileName = argv[1];
    else
        cmdlineP->inputFileName = "-";
}



static unsigned int
enc_epson_rle(unsigned int          const l, 
              const unsigned char * const src, 
              unsigned char *       const dest) {
/*----------------------------------------------------------------------------
  compress l data bytes from src to dest and return the compressed
  length
-----------------------------------------------------------------------------*/
    unsigned int i;      /* index */
    unsigned int state;  /* run state */
    unsigned int pos;    /* source position */
    unsigned int dpos;   /* destination position */

    pos = dpos = state  = 0;
    while ( pos < l )
    {
        for (i=0; i<128 && pos+i<l; i++)
            /* search for begin of a run, smallest useful run is 3
               equal bytes 
            */
            if(src[pos+i]==src[pos+i+1] && src[pos+i]==src[pos+i+2])
            {
                state=1;                       /* set run state */
                break;
            }
	if(i)
	{
        /* set counter byte for copy through */
        dest[dpos] = i-1;       
        /* copy data bytes before run begin or end cond. */
        memcpy(dest+dpos+1,src+pos,i);    
        pos+=i; dpos+=i+1;                 /* update positions */
	}
    if (state)
    {
        for (i=0; src[pos+i]==src[pos+i+1] && i<128 && pos+i<l; i++);
        /* found the runlength i */
        dest[dpos]   = 257-i;           /* set counter for byte repetition */
        dest[dpos+1] = src[pos];        /* set byte to be repeated */
        pos+=i; dpos+=2; state=0;       /* update positions, reset run state */
        }
    }
    return dpos;
}



int
main(int argc, char* argv[]) {

    FILE* ifP;
    int rows, cols;
    int format;
    unsigned int row, idx, len;
    unsigned int h, v;
    unsigned char *bytes, *cprbytes;
    struct cmdlineInfo cmdline;
    
    pbm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFileName);

    pbm_readpbminit(ifP, &cols, &rows, &format);

    bytes = malloc(24*pbm_packed_bytes(cols)+2);
    cprbytes = malloc(2*24*pbm_packed_bytes(cols));
    if (bytes == NULL || cprbytes == NULL)
        pm_error("Cannot allocate memory");

    h = v = 3600/cmdline.resolution;

    /* Set raster graphic mode. */
    printf("%c%c%c%c%c%c", esc, '(', 'G', 1, 0, 1);

    /* Set line spacing in units of 1/360 inches. */
    printf("%c%c%c", esc, '+', 24*h/10);

    /* Write out raster stripes 24 rows high. */
    for (row = 0; row < rows; row += 24) {
        unsigned int const linesThisStripe = (rows-row<24) ? rows%24 : 24;
        printf("%c%c%c%c%c%c%c%c", esc, '.', cmdline.compress, 
               v, h, linesThisStripe, 
               cols%256, cols/256);
        /* Read pbm rows, each padded to full byte */
        for (idx = 0; idx < 24 && row+idx < rows; ++idx)
            pbm_readpbmrow_packed(ifP,bytes+idx*pbm_packed_bytes(cols),
                                  cols,format);
        /* Write raster data. */
        if (cmdline.compress != 0) {
            /* compressed */
            len = enc_epson_rle(linesThisStripe * pbm_packed_bytes(cols), 
                                bytes, cprbytes);
            fwrite(cprbytes,len,1,stdout);
        } else
            /* uncompressed */
            fwrite(bytes, pbm_packed_bytes(cols), linesThisStripe, stdout);    

        if (rows-row >= 24) putchar('\n');
    }
    free(bytes); free(cprbytes);
    pm_close(ifP);

    /* Reset printer. */
    printf("%c%c", esc, '@');

    return 0;
}
