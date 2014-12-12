/*=============================================================================
                                 pamdither
===============================================================================
  By Bryan Henderson, July 2006.

  Contributed to the public domain.

  This is meant to replace Ppmdither by Christos Zoulas, 1991.
=============================================================================*/

#include "mallocvar.h"
#include "shhopt.h"
#include "pam.h"

/* Besides having to have enough memory available, the limiting factor
   in the dithering matrix power is the size of the dithering value.
   We need 2*dith_power bits in an unsigned int.  We also reserve
   one bit to give headroom to do calculations with these numbers.
*/
#define MAX_DITH_POWER ((sizeof(unsigned int)*8 - 1) / 2)


/* COLOR():
 *	returns the index in the colormap for the
 *      r, g, b values specified.
 */
#define COLOR(r,g,b) (((r) * dith_ng + (g)) * dith_nb + (b))

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFileName;  /* File name of input file */
    const char * mapFileName;    /* File name of colormap file */
    unsigned int dim;
    unsigned int verbose;
};



static void
parseCommandLine (int argc, char ** argv,
                  struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry * option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int dimSpec, mapfileSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);
    
    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "dim",      OPT_UINT, 
            &cmdlineP->dim,    &dimSpec, 0);
    OPTENT3(0,   "mapfile",      OPT_STRING, 
            &cmdlineP->mapFilespec,    &mapfileSpec, 0);
    OPTENT3(0, "verbose",        OPT_FLAG,   NULL,                  
            &cmdlineP->verbose,        0 );

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (!dimSpec)
        cmdlineP->dim = 4;

    if (cmdlineP->dim > MAX_DITH_POWER)
        pm_error("Dithering matrix power %u (-dim) is too large.  "
                 "Must be <= %d",
                 dithPower, MAX_DITH_POWER);
        
    if (!mapfileSpec)
        pm_error("You must specify the -mapfile option.");

    if (argc-1 > 1)
        pm_error("Program takes at most one argument: the input file "
                 "specification.  "
                 "You specified %d arguments.", argc-1);
    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else
        cmdlineP->inputFilespec = argv[1];
}



static unsigned int
dither(sample       const p,
       sample       const maxval,
       unsigned int const d,
       unsigned int const ditheredMaxval,
       unsigned int const ditherMatrixArea) {
/*----------------------------------------------------------------------------
  Return the dithered brightness for a component of a pixel whose real 
  brightness for that component is 'p' based on a maxval of 'maxval'.
  The returned brightness is based on a maxval of ditheredMaxval.

  'd' is the entry in the dithering matrix for the position of this pixel
  within the dithered square.

  'ditherMatrixArea' is the area (number of pixels in) the dithered square.
-----------------------------------------------------------------------------*/
    unsigned int const ditherSquareMaxval = ditheredMaxval * ditherMatrixArea;
        /* This is the maxval for an intensity that an entire dithered
           square can represent.
        */
    pixval const pScaled = ditherSquareMaxval * p / maxval;
        /* This is the input intensity P expressed with a maxval of
           'ditherSquareMaxval'
        */
    
    /* Now we scale the intensity back down to the 'ditheredMaxval', and
       as that will involve rounding, we round up or down based on the position
       in the dithered square, as determined by 'd'
    */

    return (pScaled + d) / ditherMatrixArea;
}



static unsigned int
dithValue(unsigned int const y,
          unsigned int const x,
          unsigned int const dithPower) { 
/*----------------------------------------------------------------------------
  Return the value of a dither matrix which is 2 ** dithPower elements
  square at Row x, Column y.
  [graphics gems, p. 714]
-----------------------------------------------------------------------------*/
    unsigned int d;
        /*
          Think of d as the density. At every iteration, d is shifted
          left one and a new bit is put in the low bit based on x and y.
          If x is odd and y is even, or visa versa, then a bit is shifted in.
          This generates the checkerboard pattern seen in dithering.
          This quantity is shifted again and the low bit of y is added in.
          This whole thing interleaves a checkerboard pattern and y's bits
          which is what you want.
        */
    unsigned int i;

    for (i = 0, d = 0; i < dithPower; i++, x >>= 1, y >>= 1)
        d = (d << 2) | (((x & 1) ^ (y & 1)) << 1) | (y & 1);

    return(d);
}



static unsigned int **
dithMatrix(unsigned int const dithPower) {
/*----------------------------------------------------------------------------
   Create the dithering matrix for dimension 'dithDim'.

   Return it in newly malloc'ed storage.

   Note that we assume 'dith_dim' is small enough that the dith_mat_sz
   computed within fits in an int.  Otherwise, results are undefined.
-----------------------------------------------------------------------------*/
    unsigned int const dithDim = 1 << dithPower;

    unsigned int ** dithMat;

    assert(dithPower < sizeof(unsigned int) * 8);

    {
        unsigned int const dithMatSize = 
            (dithDim * sizeof(*dithMat)) + /* pointers */
            (dithDim * dithDim * sizeof(**dithMat)); /* data */
        
        dithMat = malloc(dithMatSize);
        
        if (dithMat == NULL) 
            pm_error("Out of memory.  "
                     "Cannot allocate %d bytes for dithering matrix.",
                     dithMatSize);
    }
    {
        unsigned int * const rowStorage = (unsigned int *)&dithMat[dithDim];
        unsigned int y;
        for (y = 0; y < dithDim; ++y)
            dithMat[y] = &rowStorage[y * dithDim];
    }
    {
        unsigned int y;
        for (y = 0; y < dithDim; ++y) {
            unsigned int x;
            for (x = 0; x < dithDim; ++x)
                dithMat[y][x] = dithValue(y, x, dithPower);
        }
    }
    return dithMat;
}

    

static void
ditherImage(struct pam   const inpam,
            tuple *      const colormap, 
            unsigned int const dithPower,
            struct pam   const outpam;
            tuple **     const inTuples,
            tuple ***    const outTuplesP) {

    unsigned int const dithDim = 1 << dithPower;
    unsigned int const ditherMatrixArea = SQR(dithDim);

    unsigned int const modMask = (dithDim - 1);
       /* And this into N to compute N % dithDim cheaply, since we
          know (though the compiler doesn't) that dithDim is a power of 2
       */
    unsigned int ** const ditherMatrix = dithMatrix(dithPower);

    tuple ** ouputTuples;
    unsigned int row; 

    assert(dithPower < sizeof(unsigned int) * 8);
    assert(UINT_MAX / dithDim >= dithDim);

    outTuples = ppm_allocpamarray(outpam);

    for (row = 0; row < inpam.height; ++row) {
        unsigned int col;
        for (col = 0; col < inpam.width; ++col) {
            unsigned int const d =
                ditherMatrix[row & modMask][(width-col-1) & modMask];
            tuple const inputTuple = inTuples[row][col];
            unsigned int dithered[3];

            unsigned int plane;

            assert(inpam.depth >= 3);

            for (plane = 0; plane < 3; ++plane)
                dithered[plane] =
                    dither(inputTuple[plane], inpam.maxval, d, outpam.maxval,
                           ditherMatrixArea);

            outTuples[row][col] = 
                colormap[COLOR(dithered[RED_PLANE],
                               dithered[GRN_PLANE],
                               dithered[BLU_PLANE])];
        }
    }
    free(ditherMatrix);
    *outTuplesP = outTuples;
}



static void
getColormap(const char * const mapFileName,
            tuple **     const colormapP) {

    TODO("write this");

}



int
main(int argc,
     char ** argv) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    tuple ** inTuples;        /* Input image */
    tuple ** outTuples;        /* Output image */
    tuple * colormap;
    int cols, rows;
    pixval maxval;  /* Maxval of the input image */

    struct pam outpamCommon;
        /* Describes the output images.  Width and height fields are
           not meaningful, because different output images might have
           different dimensions.  The rest of the information is common
           across all output images.
        */

    pnm_init(&argc, argv);

    parseCommandLine(&argc, &argv);

    pm_openr(cmdline.inputFileName);

    inTuples = pnm_readpam(ifP, &inpam, PAM_STRUCT_SIZE(allocation_depth));
    pm_close(ifP);

    getColormap(cmdline.mapFileName, &colormap);

    ditherImage(inpam, colormap, dithPower, inTuples, &outTuples);

    ppm_writeppm(stdout, opixels, cols, rows, outputMaxval, 0);
    pm_close(stdout);

    free(colormap);

    pnm_freepamarray(inTuples, &inpam);
    pnm_freepamarray(outTuples, &outpam);

    return 0;
}
