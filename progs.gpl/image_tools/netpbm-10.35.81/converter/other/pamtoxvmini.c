/*=============================================================================
                                    pamtoxvmini
===============================================================================
   Convert Netpbm image to XV mini thumbnail.

   Written by Bryan Henderson in April 2006 and contributed to the public
   domain.
=============================================================================*/

#include <assert.h>
#include <limits.h>
#include <string.h>

#include "pm_c_util.h"
#include "nstring.h"
#include "pam.h"
#include "pammap.h"

typedef struct xvPalette {
    unsigned int red[256];
    unsigned int grn[256];
    unsigned int blu[256];
} xvPalette;


struct cmdlineInfo {
    const char * inputFileName;
};



static void
parseCommandLine(int const argc,
                 char *    argv[],
                 struct cmdlineInfo * const cmdlineP) {

    if (argc-1 < 1)
        cmdlineP->inputFileName = "-";
    else {
        cmdlineP->inputFileName = argv[1];

        if (argc-1 > 1)
            pm_error("Too many arguments: %u.  Only argument is optional "
                     "input file name.", argc-1);
    }
}



static void
makeXvPalette(xvPalette * const xvPaletteP) {

    unsigned int paletteIndex;
    unsigned int r;

    paletteIndex = 0;

    for (r = 0; r < 8; ++r) {
        unsigned int g;
        for (g = 0; g < 8; ++g) {
            unsigned int b;
            for (b = 0; b < 4; ++b) {
                xvPaletteP->red[paletteIndex] = (r*255)/7;
                xvPaletteP->grn[paletteIndex] = (g*255)/7;
                xvPaletteP->blu[paletteIndex] = (b*255)/3;
                ++paletteIndex;
            }
        }
    }

}



static void
writeXvHeader(FILE *       const ofP,
              unsigned int const cols,
              unsigned int const rows) {
           
    fprintf(ofP, "P7 332\n");

    fprintf(ofP, "# Created by Pamtoxvmini\n");
    fprintf(ofP, "#END_OF_COMMENTS\n");

    /* I don't know what the maxval number (3rd field) means here, since
       the maxvals are fixed at red=7, grn=7, blu=3.  We used to have
       it put the maxval of the input image there.  That generated an
       output that Xv choked on when the input maxval was 65535.
    */

    fprintf(ofP, "%u %u 255\n", cols, rows);
}



static void
findClosestColor(struct pam *      const pamP,
                 tuple             const tuple, 
                 const xvPalette * const xvPaletteP,
                 unsigned int *    const paletteIndexP) {
/*----------------------------------------------------------------------------
   Find the color in the palette *xvPaletteP that is closest to the color
   'tuple' and return its index in the palette.
-----------------------------------------------------------------------------*/
    unsigned int paletteIndex;
    unsigned int bestPaletteIndex;
    unsigned int bestDistanceSoFar;

    /* An entry condition is that the tuple have the same form as the
       colors in the XV palette:
    */
    assert(pamP->depth >= 3);
    assert(pamP->maxval == 255);

    bestPaletteIndex = 0;
    bestDistanceSoFar = UINT_MAX;

    for (paletteIndex = 0; paletteIndex < 256; ++paletteIndex) {
        unsigned int const tupleRed = tuple[PAM_RED_PLANE];
        unsigned int const tupleGrn = tuple[PAM_GRN_PLANE];
        unsigned int const tupleBlu = tuple[PAM_BLU_PLANE];
        
        unsigned int const paletteRed = xvPaletteP->red[paletteIndex];
        unsigned int const paletteGrn = xvPaletteP->grn[paletteIndex];
        unsigned int const paletteBlu = xvPaletteP->blu[paletteIndex];

        unsigned int const distance = 
            SQR((int)tupleRed - (int)paletteRed) +
            SQR((int)tupleGrn - (int)paletteGrn) +
            SQR((int)tupleBlu - (int)paletteBlu);

        if (distance < bestDistanceSoFar) {
            bestDistanceSoFar = distance;
            bestPaletteIndex = paletteIndex;
        }
    }
    *paletteIndexP = bestPaletteIndex;
}



static void
getPaletteIndexThroughCache(struct pam *      const pamP,
                            tuple             const tuple,
                            const xvPalette * const xvPaletteP,
                            tuplehash         const paletteHash,
                            unsigned int *    const paletteIndexP) {
/*----------------------------------------------------------------------------
   Return as *paletteIndexP the index into the palette *xvPaletteP of
   the color that most closely resembles the color 'tuple'.

   Use the hash table *paletteIndexP as a cache to speed up the search.
   If the tuple-index association is in *paletteIndexP, use it.  If not,
   find it the hard way and add it to *palettedIndexP for the next guy.
-----------------------------------------------------------------------------*/
    bool found;
    int paletteIndex;

    pnm_lookuptuple(pamP, paletteHash, tuple, &found, &paletteIndex);
    if (found)
        *paletteIndexP = paletteIndex;
    else {
        bool fits;
        findClosestColor(pamP, tuple, xvPaletteP, paletteIndexP);
        
        pnm_addtotuplehash(pamP, paletteHash, tuple, *paletteIndexP, &fits);
        
        if (!fits)
            pm_error("Can't get memory for palette hash.");
    }
}

    

static void
writeXvRaster(struct pam * const pamP,
              xvPalette *  const xvPaletteP,
              FILE *       const ofP) {
/*----------------------------------------------------------------------------
   Write out the XV image, from the Netpbm input file ifP, which is
   positioned to the raster.

   The XV raster contains palette indices into the palette *xvPaletteP.

   If there is any color in the image which is not in the palette, we
   fail the program.  We really should use the closest color in the palette
   instead.
-----------------------------------------------------------------------------*/
    tuplehash paletteHash;
    tuple * tuplerow;
    unsigned int row;
    unsigned char * xvrow;
    struct pam scaledPam;

    paletteHash = pnm_createtuplehash();

    tuplerow = pnm_allocpamrow(pamP);
    xvrow = (unsigned char*)pm_allocrow(pamP->width, 1);

    scaledPam = *pamP;
    scaledPam.maxval = 255;

    for (row = 0; row < pamP->height; ++row) {
        unsigned int col;

        pnm_readpamrow(pamP, tuplerow);
        pnm_scaletuplerow(pamP, tuplerow, tuplerow, scaledPam.maxval);
        pnm_makerowrgb(&scaledPam, tuplerow);

        for (col = 0; col < scaledPam.width; ++col) {
            unsigned int paletteIndex;

            getPaletteIndexThroughCache(&scaledPam, tuplerow[col], xvPaletteP,
                                        paletteHash, &paletteIndex);

            assert(paletteIndex < 256);

            xvrow[col] = paletteIndex;
        }
        fwrite(xvrow, 1, scaledPam.width, ofP);
    }

    pm_freerow((char*)xvrow);
    pnm_freepamrow(tuplerow);

    pnm_destroytuplehash(paletteHash);
}



int 
main(int    argc,
     char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    struct pam pam;
    xvPalette xvPalette;
 
    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFileName);

    makeXvPalette(&xvPalette);

    pnm_readpaminit(ifP, &pam, PAM_STRUCT_SIZE(allocation_depth));

    pnm_setminallocationdepth(&pam, 3);

    writeXvHeader(stdout, pam.width, pam.height);
    
    writeXvRaster(&pam, &xvPalette, stdout);

    pm_close(ifP);

    return 0;
}
