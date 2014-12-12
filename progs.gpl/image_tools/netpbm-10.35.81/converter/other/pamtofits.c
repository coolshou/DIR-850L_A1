/* pnmtofits.c - read a PNM image and produce a FITS file
**
** Copyright (C) 1989 by Wilson H. Bent (whb@hoh-2.att.com).
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Modified by Alberto Accomazzi (alberto@cfa.harvard.edu), Dec 1, 1992.
**
** Added PPM input capability; the program is renamed pnmtofits.
** This program produces files with NAXIS = 2 if input file is in PBM
** or PGM format, and NAXIS = 3, NAXIS3 = 3 if input file is a PPM file.
** Data is written out as either 8 bits/pixel or 16 bits/pixel integers,
** depending on the value of maxval in the input file.
** Flags -max, -min can be used to set DATAMAX, DATAMIN, BSCALE and BZERO
** in the FITS header, but do not cause the data to be rescaled.
*/

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "mallocvar.h"
#include "shhopt.h"
#include "nstring.h"
#include "pam.h"

struct cmdlineInfo {
    const char * inputFileName;
    unsigned int maxSpec;
    float max;
    float min;
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
    optEntry *option_def;
    /* Instructions to optParseOptions3 on how to parse our options. */
    optStruct3 opt;

    unsigned int minSpec;
    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "min",     OPT_FLOAT,
            &cmdlineP->min,  &minSpec,                              0);
    OPTENT3(0, "max",     OPT_FLOAT,
            &cmdlineP->max,  &cmdlineP->maxSpec,                    0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;   /* We have no parms that are negative numbers */

    /* Set some defaults the lazy way (using multiple setting of variables) */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
    /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!minSpec)
        cmdlineP->min = 0.0;

    if (cmdlineP->maxSpec) {
        if (cmdlineP->max <= cmdlineP->min)
            pm_error("-max must be greater than min (%f).  You specified %f",
                     cmdlineP->min, cmdlineP->max);
    }

    if (argc-1 < 1)
        cmdlineP->inputFileName = "-";
    else {
        cmdlineP->inputFileName = argv[1];
        
        if (argc-1 > 1)
            pm_error("Too many arguments (%u).  The only non-option argument "
                     "is the input file name.", argc-1);
    }
}




static void
writeHeaderCard(const char * const s) {
/*----------------------------------------------------------------------------
   Write the string 's', padded with spaces to 80 characters.
-----------------------------------------------------------------------------*/
    const char * card;

    asprintfN(&card, "%-80.80s", s);

    fwrite(card, sizeof(card[0]), 80, stdout);

    strfree(card);
}



static void
padToMultipleOf36Cards(unsigned int const nCardsAlreadyWritten) {

    /* pad with blanks cards to multiple of 36 cards */

    unsigned int const npadCard = 36 - (nCardsAlreadyWritten % 36);
    unsigned int i;
    
    for (i = 0; i < npadCard; ++i)
        writeHeaderCard("");
}



static void
writeFitsHeader(int    const bitpix,
                int    const planes,
                int    const cols,
                int    const rows,
                double const bscale,
                double const fitsBzero,
                double const datamax,
                double const datamin) {

    char buffer[80+1];
    unsigned int cardsWritten;
                
    cardsWritten = 0;  /* initial value */

    sprintf(buffer, "%-20.20s%10.10s", "SIMPLE  =", "T");
    writeHeaderCard(buffer);
    ++cardsWritten;

    sprintf(buffer, "%-20.20s%10d", "BITPIX  =", bitpix);
    writeHeaderCard(buffer);
    ++cardsWritten;

    sprintf(buffer, "%-20.20s%10d", "NAXIS   =", (planes == 3) ? 3 : 2);
    writeHeaderCard(buffer);
    ++cardsWritten;

    sprintf(buffer, "%-20.20s%10d", "NAXIS1  =", cols);
    writeHeaderCard(buffer);
    ++cardsWritten;

    sprintf(buffer, "%-20.20s%10d", "NAXIS2  =", rows);
    writeHeaderCard(buffer);
    ++cardsWritten;

    if (planes == 3) {
        sprintf(buffer, "%-20.20s%10d", "NAXIS3  =", 3);
        writeHeaderCard(buffer);
        ++cardsWritten;
    }

    sprintf(buffer, "%-18.18s%12.5E", "BSCALE  =", bscale);
    writeHeaderCard(buffer);
    ++cardsWritten;
    
    sprintf(buffer, "%-18.18s%12.5E", "BZERO   =", fitsBzero);
    writeHeaderCard(buffer);
    ++cardsWritten;

    sprintf(buffer, "%-18.18s%12.5E", "DATAMAX =", datamax);
    writeHeaderCard(buffer);
    ++cardsWritten;

    sprintf(buffer, "%-18.18s%12.5E", "DATAMIN =", datamin);
    writeHeaderCard(buffer);
    ++cardsWritten;

    writeHeaderCard("HISTORY Created by pnmtofits.");
    ++cardsWritten;

    writeHeaderCard("END");
    ++cardsWritten;

    padToMultipleOf36Cards(cardsWritten);
}



static void
writeRaster(struct pam * const pamP,
            tuple **     const tuples,
            unsigned int const bitpix,
            int          const offset) {

    unsigned int plane;

    for (plane = 0; plane < pamP->depth; ++plane) {
        unsigned int row;
        for (row = 0; row < pamP->height; ++row) {
            unsigned int col;
            for (col = 0; col < pamP->width; ++col) {
                if (bitpix == 16) {
                    /* 16 bit FITS samples are signed integers */
                    int const fitsSample =
                        (int)tuples[row][col][plane] - offset;
                    pm_writebigshort(stdout, (short)fitsSample);
                } else
                    /* 8 bit FITS samples are unsigned integers */
                    putchar(tuples[row][col][plane] - offset);
            }
        }
    }
    {
        /* pad raster to 36 cards with nulls */
        unsigned int const bytesWritten =
            pamP->height * pamP->width * pamP->depth * bitpix/8;
        unsigned int const npad = (36*80) - (bytesWritten % (36*80));
        unsigned int i;

        for (i = 0; i < npad; ++i)
            putchar('\0');
    }
}



int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    tuple ** tuples;
    struct pam pam;
    unsigned int bitpix;
    double datamin, datamax, bscale, fitsBzero;
    int pnmSampleOffsetFromFits;
        /* This is what you add to a FITS sample (raster) value in order
           to get the PNM sample value which it represents.  Note that in
           the default case, that PNM sample value is also the FITS "physical"
           value, but user options can change that.
        */
    
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFileName);

    tuples = pnm_readpam(ifP, &pam, PAM_STRUCT_SIZE(tuple_type));

    datamin = cmdline.min;

    if (cmdline.maxSpec)
        datamax = cmdline.max;
    else {
        if (pam.maxval <= cmdline.min)
            pm_error("You must specify -max greater than -min (%f).  "
                     "max defaults to the maxval, which is %u",
                     cmdline.min, (unsigned)pam.maxval);
        datamax = pam.maxval;
    }

    assert(datamax > datamin);

    bscale = (datamax - datamin) / pam.maxval;
    
    if (pam.maxval > 255) {
        bitpix = 16;
        /* Because 16 bit FITS samples are signed, we have to do a 2**15
           offset to get any possible unsigned 16 bit PNM sample into a FITS
           sample.
        */
        fitsBzero = 1 << 15;
        pnmSampleOffsetFromFits = 1 << 15;
    } else {
        bitpix = 8;
        fitsBzero = datamin;
        /* Both 8 bit FITS samples and PNM samples are unsigned 8 bits, so
           we make them identical.
        */
        pnmSampleOffsetFromFits = 0;
    }

    fitsBzero = datamin + pnmSampleOffsetFromFits;
    pm_close(ifP);

    writeFitsHeader(bitpix, pam.depth, pam.width, pam.height,
                    bscale, fitsBzero, datamax, datamin);

    writeRaster(&pam, tuples, bitpix, pnmSampleOffsetFromFits);

    return 0;
}
