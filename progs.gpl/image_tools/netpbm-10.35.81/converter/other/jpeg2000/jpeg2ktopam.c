/*****************************************************************************
                              jpeg2kopam
******************************************************************************

  Convert a JPEG-2000 code stream image to a PNM or PAM

  By Bryan Henderson, San Jose CA  2002.10.26

*****************************************************************************/

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
/* Make sure strdup() is in string.h and int_fast32_t is in inttypes.h */
#define _XOPEN_SOURCE 600
#include <string.h>

#include "pam.h"
#include "shhopt.h"
#include "nstring.h"
#include "mallocvar.h"

#include <jasper/jasper.h>
#include "libjasper_compat.h"

enum compmode {COMPMODE_INTEGER, COMPMODE_REAL};

enum progression {PROG_LRCP, PROG_RLCP, PROG_RPCL, PROG_PCRL, PROG_CPRL};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    char * inputFilename;
    unsigned int debuglevel;  /* Jasper library debug level */
    unsigned int verbose;
};


static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that many of the strings that this function returns in the
   *cmdline_p structure are actually in the supplied argv array.  And
   sometimes, one of these strings is actually just a suffix of an entry
   in argv!
-----------------------------------------------------------------------------*/
    optEntry *option_def;
    optStruct3 opt;

    unsigned int debuglevelSpec;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "verbose",      OPT_FLAG,   NULL, 
            &cmdlineP->verbose,   0);
    OPTENT3(0, "debuglevel",   OPT_UINT,   &cmdlineP->debuglevel,
            &debuglevelSpec,      0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

    if (!debuglevelSpec)
        cmdlineP->debuglevel = 0;

    if (argc - 1 == 0)
        cmdlineP->inputFilename = strdup("-");  /* he wants stdin */
    else if (argc - 1 == 1)
        cmdlineP->inputFilename = strdup(argv[1]);
    else 
        pm_error("Too many arguments.  The only argument accepted\n"
                 "is the input file specification");

}



static void
readJpc(const char *   const inputFilename, 
        jas_image_t ** const jasperPP) {

    jas_image_t * jasperP;
    jas_stream_t *instream;
    const char * options;

    if ( strcmp(inputFilename, "-") == 0) {
        /* The input image is to be read from standard input. */
        instream = jas_stream_fdopen(fileno(stdin), "rb");
        if (instream == NULL)
            pm_error("error: cannot reopen standard input");
    } else {
        instream = jas_stream_fopen(inputFilename, "rb");
        if (instream == NULL )
            pm_error("cannot open input image file '%s'", inputFilename);
    } 

    if (jas_image_getfmt(instream) != jas_image_strtofmt((char*)"jpc"))
        pm_error("Input is not JPEG-2000 code stream");

    options = "";

    jasperP = jas_image_decode(instream, jas_image_strtofmt((char*)"jpc"), 
                               (char*)options);
    if (jasperP == NULL)
        pm_error("Unable to interpret JPEG-2000 input.  "
                 "The Jasper library jas_image_decode() subroutine failed.");

	jas_stream_close(instream);

    *jasperPP = jasperP;
}



static void
getRgbComponents(int jasperCmpnt[], jas_image_t * const jasperP) {

    {
        int const rc = 
            jas_image_getcmptbytype(jasperP,
                                    JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_RGB_R));
        if (rc < 0)
            pm_error("Input says it has RGB color space, but contains "
                     "no red component");
        else
            jasperCmpnt[PAM_RED_PLANE] = rc;

        if (jas_image_cmptsgnd(jasperP, rc)) 
            pm_error("Input image says it is RGB, but has signed values "
                     "for what should be the red intensities.");
    }
    {
        int const rc = 
            jas_image_getcmptbytype(jasperP,
                                    JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_RGB_G));
        if (rc < 0)
            pm_error("Input says it has RGB color space, but contains "
                     "no green component");
        else
            jasperCmpnt[PAM_GRN_PLANE] = rc;

        if (jas_image_cmptsgnd(jasperP, rc)) 
            pm_error("Input image says it is RGB, but has signed values "
                     "for what should be the green intensities.");
    }
    {
        int const rc = 
            jas_image_getcmptbytype(jasperP,
                                    JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_RGB_B));
        if (rc < 0)
            pm_error("Input says it has RGB color space, but contains "
                     "no blue component");
        else
            jasperCmpnt[PAM_BLU_PLANE] = rc;

        if (jas_image_cmptsgnd(jasperP, rc)) 
            pm_error("Input image says it is RGB, but has signed values "
                     "for what should be the blue intensities.");
    }
}            



static void
getGrayComponent(int * jasperCmptP, jas_image_t * const jasperP) {

    int const rc = 
        jas_image_getcmptbytype(jasperP,
                                JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_GRAY_Y));
    if (rc < 0)
        pm_error("Input says it has Grayscale color space, but contains "
                 "no gray intensity component");
    else
        *jasperCmptP = rc;
    if (jas_image_cmptsgnd(jasperP, 0)) 
        pm_error("Input image says it is grayscale, but has signed values "
                 "for what should be the gray levels.");
}



static void
validateComponentsAlike(jas_image_t * const jasperP) {
/*----------------------------------------------------------------------------
   JPC allows each component to have its own width and height.  But
   PAM requires all planes to the same shape.  So we validate now that
   all the channels are the same, and abort the program if not.
-----------------------------------------------------------------------------*/
    int cmptNo;
    
    for (cmptNo = 0; cmptNo < jas_image_numcmpts(jasperP); ++cmptNo) {
        if (jas_image_cmptwidth(jasperP, cmptNo) != 
            jas_image_cmptwidth(jasperP, 0))
            pm_message("Input image does not have components all the same "
                       "width.");
        if (jas_image_cmptheight(jasperP, cmptNo) != 
            jas_image_cmptheight(jasperP, 0))
            pm_message("Input image does not have components all the same "
                       "height.");
    }
}



static unsigned int
maxJasperComponentPrecision(jas_image_t * const jasperP) {

    int cmptNo;
    unsigned int max;
    
    max = 1;

    for (cmptNo = 0; cmptNo < jas_image_numcmpts(jasperP); ++cmptNo)
        max = MAX(max, jas_image_cmptprec(jasperP, cmptNo));

    return max;
}



static void
computeOutputParm(jas_image_t * const jasperP,
                  struct pam *  const outpamP,
                  int **        const jasperCmptNoP) {

    int * jasperCmptNo;
       /* Malloc'ed array.  jaspercmptNo[P] is the component number for use
          with the Jasper library that corresponds to Plane P of the PAM.
       */

	switch (jas_clrspc_fam(jas_image_clrspc(jasperP))) {
	case JAS_CLRSPC_FAM_GRAY:
        outpamP->depth = 1;
        MALLOCARRAY_NOFAIL(jasperCmptNo, 1);
        getGrayComponent(&jasperCmptNo[0], jasperP);
        if (jas_image_cmptprec(jasperP, jasperCmptNo[0]) == 1) {
            outpamP->format = RPBM_FORMAT;
            strcpy(outpamP->tuple_type, PAM_PBM_TUPLETYPE);
        } else {
            outpamP->format = RPGM_FORMAT;
            strcpy(outpamP->tuple_type, PAM_PGM_TUPLETYPE);
        }
        break;
	case JAS_CLRSPC_FAM_RGB:
        outpamP->depth = 3;
        MALLOCARRAY_NOFAIL(jasperCmptNo, 3);
        getRgbComponents(jasperCmptNo, jasperP);
        outpamP->format = RPPM_FORMAT;
        strcpy(outpamP->tuple_type, PAM_PPM_TUPLETYPE);
        break;
    default:
        outpamP->format = PAM_FORMAT;
        outpamP->depth = jas_image_numcmpts(jasperP);
        {
            unsigned int plane;

            MALLOCARRAY_NOFAIL(jasperCmptNo, outpamP->depth);
            for (plane = 0; plane < outpamP->depth; ++plane)
                jasperCmptNo[plane] = plane;
        }
        strcpy(outpamP->tuple_type, "");
        if (jas_image_cmptsgnd(jasperP, 0)) 
            pm_message("Warning: Input image has signed sample values.  "
                       "They will be represented in the PAM output in "
                       "two's complement.");
    }
    outpamP->plainformat = FALSE;

	outpamP->width = jas_image_cmptwidth(jasperP, 0);
	outpamP->height = jas_image_cmptheight(jasperP, 0);

    validateComponentsAlike(jasperP);

    {
        unsigned int const maxPrecision = maxJasperComponentPrecision(jasperP);

        outpamP->maxval = pm_bitstomaxval(maxPrecision);
        
        outpamP->bytes_per_sample = (maxPrecision + 7)/8;
    }
    *jasperCmptNoP = jasperCmptNo;
}



static void
createMatrices(struct pam * const outpamP, jas_matrix_t *** matrixP) {

    jas_matrix_t ** matrix; 
    unsigned int plane;

    MALLOCARRAY_NOFAIL(matrix, outpamP->depth);

    for (plane = 0; plane < outpamP->depth; ++plane) {
        matrix[plane] = jas_matrix_create(1, outpamP->width);

        if (matrix[plane] == NULL)
            pm_error("Unable to create matrix for plane %u.  "
                     "jas_matrix_create() failed.", plane);
    }   
    *matrixP = matrix;
}



static void
destroyMatrices(struct pam *    const outpamP, 
                jas_matrix_t ** const matrix ) {

    unsigned int plane;

    for (plane = 0; plane < outpamP->depth; ++plane)
        jas_matrix_destroy(matrix[plane]);
    free(matrix);
}    



static void
computeComponentMaxval(struct pam *  const outpamP,
                       jas_image_t * const jasperP,
                       int           const jasperCmpt[],
                       sample **     const jasperMaxvalP,
                       bool *        const singleMaxvalP) {
    
    sample * jasperMaxval;
    unsigned int plane;

    MALLOCARRAY(jasperMaxval, outpamP->depth);

    *singleMaxvalP = TRUE;  /* initial assumption */
    for (plane = 0; plane < outpamP->depth; ++plane) {
        jasperMaxval[plane] = 
            pm_bitstomaxval(jas_image_cmptprec(jasperP, jasperCmpt[plane]));
        if (jasperMaxval[plane] != jasperMaxval[0])
            *singleMaxvalP = FALSE;
    }
    *jasperMaxvalP = jasperMaxval;
}

                       

static void
copyRowSingleMaxval(jas_seqent_t ** const jasperRow,
                    tuple *         const tuplerow,
                    struct pam *    const outpamP) {
/*----------------------------------------------------------------------------
   Copy row from Jasper library representation to Netpbm library
   representation, assuming all Jasper components have the same precision,
   which corresponds to the maxval of the output PAM, which means we don't
   have to do any maxval scaling.

   This is significantly faster than copyRowAnyMaxval().
-----------------------------------------------------------------------------*/
    unsigned int col;
    
    for (col = 0; col < outpamP->width; ++col) {
        unsigned int plane;
        for (plane = 0; plane < outpamP->depth; ++plane) 
            tuplerow[col][plane] = jasperRow[plane][col];
    }
}



static void
copyRowAnyMaxval(jas_seqent_t ** const jasperRow,
                 tuple *         const tuplerow,
                 struct pam *    const outpamP,
                 sample          const jasperMaxval[]) {
/*----------------------------------------------------------------------------
   Copy row from Jasper library representation to Netpbm library
   representation, allowing for each Jasper library component to have a
   different precision (number of bits) and for those precisions to be
   unrelated to the PAM maxval.

   This is significantly slower than copyRowSingleMaxval().
-----------------------------------------------------------------------------*/
    unsigned int col;
            
    for (col = 0; col < outpamP->width; ++col) {
        unsigned int plane;
        for (plane = 0; plane < outpamP->depth; ++plane) 
            tuplerow[col][plane] = 
                jasperRow[plane][col] * 
                outpamP->maxval / jasperMaxval[plane];
    }
}



static void
convertToPamPnm(struct pam *  const outpamP,
                jas_image_t * const jasperP,
                int           const jasperCmptNo[]) {

    jas_matrix_t ** matrix;  /* malloc'ed */
        /* matrix[X] is the data for Plane X of the current row */
    sample * jasperMaxval;
    unsigned int row;
    tuple * tuplerow;
    jas_seqent_t ** jasperRow;   /* malloc'ed */
       /* A row of a plane of the raster from the Jasper library 
          This is an array of pointers into the 'matrix' data structures.
       */
    bool singleMaxval;

    createMatrices(outpamP, &matrix);

    computeComponentMaxval(outpamP, jasperP, jasperCmptNo,
                           &jasperMaxval, &singleMaxval);

    MALLOCARRAY(jasperRow, outpamP->depth);
    if (jasperRow == NULL)
        pm_error("Out of memory");

    tuplerow = pnm_allocpamrow(outpamP);

    for (row = 0; row < outpamP->height; ++row) {
        unsigned int plane;

        for (plane = 0; plane < outpamP->depth; ++plane) {
            int rc;
            rc = jas_image_readcmpt(jasperP, jasperCmptNo[plane], 0, row,
                                    outpamP->width, 1,
                                    matrix[plane]);
            if (rc != 0)
                pm_error("jas_image_readcmpt() of row %u plane %u "
                         "failed.", 
                         row, plane);
            jasperRow[plane] = jas_matrix_getref(matrix[plane], 0, 0);
        }
        if (singleMaxval) 
            copyRowSingleMaxval(jasperRow, tuplerow, outpamP);
        else
            copyRowAnyMaxval(jasperRow, tuplerow, outpamP, jasperMaxval);

        pnm_writepamrow(outpamP, tuplerow);
    }
    pnm_freepamrow(tuplerow);

    destroyMatrices(outpamP, matrix);

    free(jasperRow);
    free(jasperMaxval);
}



int
main(int argc, char **argv)
{
    struct cmdlineInfo cmdline;
    struct pam outpam;
    int * jasperCmpt;  /* malloc'ed */
       /* jaspercmpt[P] is the component number for use with the
          Jasper library that corresponds to Plane P of the PAM.  
       */
    jas_image_t * jasperP;

    pnm_init(&argc, argv);
    
    parseCommandLine(argc, argv, &cmdline);
    
    { 
        int rc;
        
        rc = jas_init();
        if ( rc != 0 )
            pm_error("Failed to initialize Jasper library.  "
                     "jas_init() returns rc %d", rc );
    }
    
    jas_setdbglevel(cmdline.debuglevel);
    
    readJpc(cmdline.inputFilename, &jasperP);

    outpam.file = stdout;
    outpam.size = sizeof(outpam);
    outpam.len  = PAM_STRUCT_SIZE(tuple_type);

    computeOutputParm(jasperP, &outpam, &jasperCmpt);

    pnm_writepaminit(&outpam);
    
    convertToPamPnm(&outpam, jasperP, jasperCmpt);
    
    free(jasperCmpt);
	jas_image_destroy(jasperP);

    pm_close(stdout);
    
    return 0;
}
