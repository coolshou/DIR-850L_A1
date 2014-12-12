#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include "pam.h"
#include "mallocvar.h"

struct cmdlineInfo {
    const char * inputFileName;
};



static void
processCommandLine(int                  const argc,
                   char *               const argv[],
                   struct cmdlineInfo * const cmdlineP) {
        
    if (argc-1 < 1)
        cmdlineP->inputFileName = "-";
    else {
        cmdlineP->inputFileName = argv[1];

        if (argc-1 > 1)
            pm_error("Too many arguments.  "
                     "The only argument is the input file name.");
    }
}



static void
convertPamToPng(const struct pam * const pamP,
                const tuple *      const tuplerow,
                png_byte *         const pngRow) {
    
    unsigned int col;
    
    for (col = 0; col < pamP->width; ++col) {
        unsigned int plane;
        
        for (plane = 0; plane < 4; ++plane)
            pngRow[4 * col + plane] = tuplerow[col][plane];
    }
}



static void
writeRaster(const struct pam * const pamP,
            png_struct *       const pngP) {
    
    tuple * tupleRow;
    png_byte * pngRow;
    
    tupleRow = pnm_allocpamrow(pamP);
    MALLOCARRAY(pngRow, pamP->width * 4);

    if (pngRow == NULL)
        pm_error("Unable to allocate space for PNG pixel row.");
    else {
        unsigned int row;
        for (row = 0; row < pamP->height; ++row) {
            pnm_readpamrow(pamP, tupleRow);
            
            convertPamToPng(pamP, tupleRow, pngRow);
            
            png_write_row(pngP, pngRow);
        }
        free(pngRow);
    }
    pnm_freepamrow(tupleRow);
}



static void
pngErrorHandler(png_struct * const pngP,
                const char * const message) {

    pm_error("Error generating PNG image.  libpng says: %s", message);
}



static void
writePng(const struct pam * const pamP,
         FILE *             const ofP) {

    png_struct * pngP;
    png_info * infoP;

    pngP = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!pngP)
        pm_error("Could not allocate png struct.");

    png_set_error_fn(pngP, NULL, &pngErrorHandler, NULL);

    infoP = png_create_info_struct(pngP);
    if (!infoP)
        pm_error("Could not allocate PNG info structure");
    else {
        infoP->width      = pamP->width;
        infoP->height     = pamP->height;
        infoP->bit_depth  = 8;
        infoP->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        
        png_init_io(pngP, ofP);

        png_write_info(pngP, infoP);
        
        writeRaster(pamP, pngP);

        png_write_end(pngP, infoP);
        
        png_destroy_write_struct(&pngP, &infoP);
    }
}
    


int
main(int argc, char * argv[]) {

    FILE * ifP;
    struct cmdlineInfo cmdline;
    struct pam pam;

    pnm_init(&argc, argv);

    processCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFileName);

    pnm_readpaminit(ifP, &pam, PAM_STRUCT_SIZE(tuple_type));
    
    if (pam.depth < 4)
        pm_error("PAM must have depth at least 4 (red, green, blue, alpha).  "
                 "This one has depth %u", pam.depth);
        
    if (pam.maxval != 255)
        pm_error("PAM must have maxval 255.  This one has %lu", pam.maxval);

    writePng(&pam, stdout);

    pm_close(ifP);

    return 0;
}
