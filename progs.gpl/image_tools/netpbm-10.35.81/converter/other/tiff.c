/*============================================================================
                                 tiff.c
==============================================================================

  Stuff specific to the TIFF format -- essentially and extension to libtiff.

============================================================================*/

#include <string.h>

#ifdef VMS
#ifdef SYSV
#undef SYSV
#endif
#include <tiffioP.h>
#endif
#include <tiffio.h>

#include "pm_c_util.h"
#include "nstring.h"
#include "mallocvar.h"
#include "pm.h"
#include "pm_tiff.h"



static uint32
number(const char * const value,
       tagvalmap    const tagvallist[]) {
    
    char * ep;
    long num;

    if (strlen(value) == 0)
        pm_error("null string where numeric tag value or enumerated tag "
                 "value name expected");
    
    num = strtol(value, &ep, 10);
    if (*ep != '\0') {
        /* It's not a numeric string, so it must be an enumerated value name */
        unsigned int i;
        for (i = 0; tagvallist[i].name; ++i) {
            if (STRCASEEQ(value, tagvallist[i].name))
                return tagvallist[i].value;
        }
        pm_error("'%s' is neither a number nor a valid value name", value);
        return 0; /* avoid compiler warning */
    } else
        return num;
}



static double
dnumber(const char * const value) {

    char * ep;
    double num;
    
    num = strtod(value, &ep);
    if (ep == value || *ep != '\0')
        pm_error("Bad floating point number %s", value);

    return num;
}



#if 0
static void
putByte(TIFF *          const tifP,
        unsigned int    const tagnum,
        const char *    const value,
        tagvalmap const tagvallist[]) {

    uint16 const num = number(value, tagvallist);

    TIFFSetField(tifP, tagnum, num);
}
#endif



static void
putAscii(TIFF *       const tifP,
         unsigned int const tagnum,
         const char * const value,
         tagvalmap    const tagvallist[]) {

    TIFFSetField(tifP, tagnum, value);
}



static void
putShort(TIFF *       const tifP,
         unsigned     const tagnum,
         const char * const value,
         tagvalmap    const tagvallist[]) {

    uint16 const num = number(value, tagvallist);

    TIFFSetField(tifP, tagnum, num);
}



static void
putLong(TIFF *       const tifP,
        unsigned int const tagnum,
        const char * const value,
        tagvalmap    const tagvallist[]) {

    uint32 const num = number(value, tagvallist);

    TIFFSetField(tifP, tagnum, num);
}



static void
putRational(TIFF *       const tifP,
            unsigned     const tagnum,
            const char * const value,
            tagvalmap    const tagvallist[]) {

    float const num = dnumber(value);

    TIFFSetField(tifP, tagnum, num);
}



static void
putCountBytes(TIFF *       const tifP,
              unsigned     const tagnum,
              const char * const value,
              tagvalmap    const tagvallist[]) {

    uint32 const len = strlen(value)/2;

    char * data;
    uint32 i;
    bool   error;

    MALLOCARRAY_NOFAIL(data, len);

    for (i = 0, error = FALSE; !error && i < len; ++i) {
        unsigned int val;
        int    nb;
        int    rc;
        rc = sscanf(&value[i*2], "%2x%n", &val, &nb);
        if (rc != 1 || nb != 2)
            error = TRUE;
        else
            data[i] = val;
    }
    if (!error)
        TIFFSetField(tifP, tagnum, len, (void *)data);

    free(data);
}



#define TV(p,a) { #a, p##a, }

static tagvalmap const 
tvm_compression[] = {
    TV(COMPRESSION_,NONE),
    TV(COMPRESSION_,CCITTRLE),
    TV(COMPRESSION_,CCITTFAX3),
    TV(COMPRESSION_,CCITTFAX4),
    TV(COMPRESSION_,LZW),
    TV(COMPRESSION_,OJPEG),
    TV(COMPRESSION_,JPEG),
    TV(COMPRESSION_,NEXT),
    TV(COMPRESSION_,CCITTRLEW),
    TV(COMPRESSION_,PACKBITS),
    TV(COMPRESSION_,THUNDERSCAN),
    TV(COMPRESSION_,IT8CTPAD),
    TV(COMPRESSION_,IT8LW),
    TV(COMPRESSION_,IT8MP),
    TV(COMPRESSION_,IT8BL),
    TV(COMPRESSION_,PIXARFILM),
    TV(COMPRESSION_,PIXARLOG),
    TV(COMPRESSION_,DEFLATE),
    TV(COMPRESSION_,ADOBE_DEFLATE),
    TV(COMPRESSION_,DCS),
    TV(COMPRESSION_,JBIG),
    TV(COMPRESSION_,SGILOG),
    TV(COMPRESSION_,SGILOG24),
    { NULL, 0, },
};

static tagvalmap const
tvm_faxmode[] = {
    TV(FAXMODE_,CLASSIC),
    TV(FAXMODE_,NORTC),
    TV(FAXMODE_,NOEOL),
    TV(FAXMODE_,BYTEALIGN),
    TV(FAXMODE_,WORDALIGN),
    TV(FAXMODE_,CLASSF),
    { NULL, 0, },
};

static tagvalmap const
tvm_fillorder[] = {
    TV(FILLORDER_,MSB2LSB),
    TV(FILLORDER_,LSB2MSB),
    { NULL, 0, },
};

static tagvalmap
const tvm_group3options[] = {
    TV(GROUP3OPT_,2DENCODING),
    TV(GROUP3OPT_,UNCOMPRESSED),
    TV(GROUP3OPT_,FILLBITS),
    { NULL, 0, },
};

static tagvalmap
const tvm_group4options[] = {
    TV(GROUP4OPT_,UNCOMPRESSED),
    { NULL, 0, },
};

static tagvalmap
const tvm_inkset[] = {
    TV(INKSET_,CMYK),
    { NULL, 0, },
};

static tagvalmap
const tvm_orientation[] = {
    TV(ORIENTATION_,TOPLEFT),
    TV(ORIENTATION_,TOPRIGHT),
    TV(ORIENTATION_,BOTRIGHT),
    TV(ORIENTATION_,BOTLEFT),
    TV(ORIENTATION_,LEFTTOP),
    TV(ORIENTATION_,RIGHTTOP),
    TV(ORIENTATION_,RIGHTBOT),
    TV(ORIENTATION_,LEFTBOT),
    { NULL, 0, },
};

static tagvalmap const
tvm_photometric[] = {
    TV(PHOTOMETRIC_,MINISWHITE),
    TV(PHOTOMETRIC_,MINISBLACK),
    TV(PHOTOMETRIC_,RGB),
    TV(PHOTOMETRIC_,PALETTE),
    TV(PHOTOMETRIC_,MASK),
    TV(PHOTOMETRIC_,SEPARATED),
    TV(PHOTOMETRIC_,YCBCR),
    TV(PHOTOMETRIC_,CIELAB),
    TV(PHOTOMETRIC_,LOGL),
    TV(PHOTOMETRIC_,LOGLUV),
    { NULL, 0, },
};

static tagvalmap const
tvm_planarconfig[] = {
    TV(PLANARCONFIG_,CONTIG),
    TV(PLANARCONFIG_,SEPARATE),
    { NULL, 0, },
};

static tagvalmap const
tvm_resolutionunit[] = {
    TV(RESUNIT_,NONE),
    TV(RESUNIT_,INCH),
    TV(RESUNIT_,CENTIMETER),
    { NULL, 0, },
};

static tagvalmap const
tvm_sampleformat[] = {
    TV(SAMPLEFORMAT_,UINT),
    TV(SAMPLEFORMAT_,INT),
    TV(SAMPLEFORMAT_,IEEEFP),
    TV(SAMPLEFORMAT_,VOID),
    { NULL, 0, },
};

static tagvalmap const
tvm_subfiletype[] = {
    TV(FILETYPE_,REDUCEDIMAGE),
    TV(FILETYPE_,PAGE),
    TV(FILETYPE_,MASK),
    { NULL, 0, },
};

static tagvalmap const
tvm_threshholding[] =
{
    TV(THRESHHOLD_,BILEVEL),
    TV(THRESHHOLD_,HALFTONE),
    TV(THRESHHOLD_,ERRORDIFFUSE),
    { NULL, 0, },
};
#undef TV

/*
 * For full functionality, some tags have an implied count
 * (eg. colormap, dotrange). See TIFFGetField(3).
 * Also, some are arrays.
 */
#define DECL(a,p,c) { #a, TIFFTAG_##a, p, c, }

static tagDefinition const
tagDefinitions[] = {
    DECL(ARTIST,                  putAscii,    NULL),
    DECL(BADFAXLINES,             putLong,     NULL),
    DECL(BITSPERSAMPLE,           putShort,    NULL),
    DECL(CELLLENGTH,              NULL,        NULL),
    DECL(CELLWIDTH,               NULL,        NULL),
    DECL(CLEANFAXDATA,            putShort,    NULL),
    DECL(COLORMAP,                NULL,        NULL),
    DECL(COLORRESPONSEUNIT,       NULL,        NULL),
    DECL(COMPRESSION,             putShort,    tvm_compression),
    DECL(CONSECUTIVEBADFAXLINES,  putLong,     NULL),
    DECL(COPYRIGHT,               putAscii,    NULL),
    DECL(DATATYPE,                putShort,    NULL),
    DECL(DATETIME,                putAscii,    NULL),
    DECL(DCSBALANCEARRAY,         NULL,        NULL),
    DECL(DCSCALIBRATIONFD,        NULL,        NULL),
    DECL(DCSCLIPRECTANGLE,        NULL,        NULL),
    DECL(DCSCORRECTMATRIX,        NULL,        NULL),
    DECL(DCSGAMMA,                NULL,        NULL),
    DECL(DCSHUESHIFTVALUES,       NULL,        NULL),
    DECL(DCSIMAGERTYPE,           NULL,        NULL),
    DECL(DCSINTERPMODE,           NULL,        NULL),
    DECL(DCSTOESHOULDERPTS,       NULL,        NULL),
    DECL(DOCUMENTNAME,            putAscii,    NULL),
    DECL(DOTRANGE,                NULL,        NULL),
    DECL(EXTRASAMPLES,            NULL,        NULL),
    DECL(FAXFILLFUNC,             NULL,        NULL),
    DECL(FAXMODE,                 putLong,     tvm_faxmode),
    DECL(FAXRECVPARAMS,           NULL,        NULL),
    DECL(FAXRECVTIME,             NULL,        NULL),
    DECL(FAXSUBADDRESS,           NULL,        NULL),
    DECL(FEDEX_EDR,               NULL,        NULL),
    DECL(FILLORDER,               putShort,    tvm_fillorder),
    DECL(FRAMECOUNT,              NULL,        NULL),
    DECL(FREEBYTECOUNTS,          NULL,        NULL),
    DECL(FREEOFFSETS,             NULL,        NULL),
    DECL(GRAYRESPONSECURVE,       NULL,        NULL),
    DECL(GRAYRESPONSEUNIT,        NULL,        NULL),
    DECL(GROUP3OPTIONS,           putLong,     tvm_group3options),
    DECL(GROUP4OPTIONS,           putLong,     tvm_group4options),
    DECL(HALFTONEHINTS,           NULL,        NULL),
    DECL(HOSTCOMPUTER,            putAscii,    NULL),
    DECL(ICCPROFILE,              putCountBytes, NULL),
    DECL(IMAGEDEPTH,              putLong,     NULL),
    DECL(IMAGEDESCRIPTION,        putAscii,    NULL),
    DECL(IMAGELENGTH,             putLong,     NULL),
    DECL(IMAGEWIDTH,              putLong,     NULL),
    DECL(INKNAMES,                putAscii,    NULL),
    DECL(INKSET,                  putShort,    tvm_inkset),
    DECL(IT8BITSPEREXTENDEDRUNLENGTH, NULL,    NULL),
    DECL(IT8BITSPERRUNLENGTH,     NULL,        NULL),
    DECL(IT8BKGCOLORINDICATOR,    NULL,        NULL),
    DECL(IT8BKGCOLORVALUE,        NULL,        NULL),
    DECL(IT8COLORCHARACTERIZATION, NULL,       NULL),
    DECL(IT8COLORSEQUENCE,        NULL,        NULL),
    DECL(IT8COLORTABLE,           NULL,        NULL),
    DECL(IT8HEADER,               NULL,        NULL),
    DECL(IT8IMAGECOLORINDICATOR,  NULL,        NULL),
    DECL(IT8IMAGECOLORVALUE,      NULL,        NULL),
    DECL(IT8PIXELINTENSITYRANGE,  NULL,        NULL),
    DECL(IT8RASTERPADDING,        NULL,        NULL),
    DECL(IT8SITE,                 NULL,        NULL),
    DECL(IT8TRANSPARENCYINDICATOR, NULL,       NULL),
    DECL(JBIGOPTIONS,             NULL,        NULL),
    DECL(JPEGACTABLES,            NULL,        NULL),
    DECL(JPEGCOLORMODE,           NULL,        NULL),
    DECL(JPEGDCTABLES,            NULL,        NULL),
    DECL(JPEGIFBYTECOUNT,         NULL,        NULL),
    DECL(JPEGIFOFFSET,            NULL,        NULL),
    DECL(JPEGLOSSLESSPREDICTORS,  NULL,        NULL),
    DECL(JPEGPOINTTRANSFORM,      NULL,        NULL),
    DECL(JPEGPROC,                NULL,        NULL),
    DECL(JPEGQTABLES,             NULL,        NULL),
    DECL(JPEGQUALITY,             NULL,        NULL),
    DECL(JPEGRESTARTINTERVAL,     NULL,        NULL),
    DECL(JPEGTABLES,              NULL,        NULL),
    DECL(JPEGTABLESMODE,          NULL,        NULL),
    DECL(MAKE,                    putAscii,    NULL),
    DECL(MATTEING,                putShort,    NULL),
    DECL(MAXSAMPLEVALUE,          putShort,    NULL),
    DECL(MINSAMPLEVALUE,          putShort,    NULL),
    DECL(MODEL,                   putAscii,    NULL),
    DECL(NUMBEROFINKS,            NULL,        NULL),
    DECL(ORIENTATION,             putShort,    tvm_orientation),
    DECL(OSUBFILETYPE,            NULL,        NULL),
    DECL(PAGENAME,                putAscii,    NULL),
    DECL(PAGENUMBER,              NULL,        NULL),
    DECL(PHOTOMETRIC,             putShort,    tvm_photometric),
    DECL(PHOTOSHOP,               NULL,        NULL),
    DECL(PIXAR_FOVCOT,            NULL,        NULL),
    DECL(PIXAR_IMAGEFULLLENGTH,   NULL,        NULL),
    DECL(PIXAR_IMAGEFULLWIDTH,    NULL,        NULL),
    DECL(PIXAR_MATRIX_WORLDTOCAMERA, NULL,     NULL),
    DECL(PIXAR_MATRIX_WORLDTOSCREEN, NULL,     NULL),
    DECL(PIXAR_TEXTUREFORMAT,     NULL,        NULL),
    DECL(PIXAR_WRAPMODES,         NULL,        NULL),
    DECL(PIXARLOGDATAFMT,         NULL,        NULL),
    DECL(PIXARLOGQUALITY,         NULL,        NULL),
    DECL(PLANARCONFIG,            putShort,    tvm_planarconfig),
    DECL(PREDICTOR,               putShort,    NULL),
    DECL(PRIMARYCHROMATICITIES,   NULL,        NULL),
    DECL(REFERENCEBLACKWHITE,     NULL,        NULL),
    DECL(REFPTS,                  NULL,        NULL),
    DECL(REGIONAFFINE,            NULL,        NULL),
    DECL(REGIONTACKPOINT,         NULL,        NULL),
    DECL(REGIONWARPCORNERS,       NULL,        NULL),
    DECL(RESOLUTIONUNIT,          putShort,    tvm_resolutionunit),
    DECL(RICHTIFFIPTC,            NULL,        NULL),
    DECL(ROWSPERSTRIP,            putLong,     NULL),
    DECL(SAMPLEFORMAT,            putShort,    tvm_sampleformat),
    DECL(SAMPLESPERPIXEL,         putShort,    NULL),
    DECL(SGILOGDATAFMT,           NULL,        NULL),
    DECL(SMAXSAMPLEVALUE,         NULL,        NULL),
    DECL(SMINSAMPLEVALUE,         NULL,        NULL),
    DECL(SOFTWARE,                putAscii,    NULL),
    DECL(STONITS,                 NULL,        NULL),
    DECL(STRIPBYTECOUNTS,         NULL,        NULL),
    DECL(STRIPOFFSETS,            NULL,        NULL),
    DECL(SUBFILETYPE,             putLong,     tvm_subfiletype),
    DECL(SUBIFD,                  NULL,        NULL),
    DECL(TARGETPRINTER,           putAscii,    NULL),
    DECL(THRESHHOLDING,           putShort,    tvm_threshholding),
    DECL(TILEBYTECOUNTS,          NULL,        NULL),
    DECL(TILEDEPTH,               putLong,     NULL),
    DECL(TILELENGTH,              putLong,     NULL),
    DECL(TILEOFFSETS,             NULL,        NULL),
    DECL(TILEWIDTH,               putLong,     NULL),
    DECL(TRANSFERFUNCTION,        NULL,        NULL),
    DECL(WHITEPOINT,              NULL,        NULL),
    DECL(WRITERSERIALNUMBER,      NULL,        NULL),
    DECL(XPOSITION,               putRational, NULL),
    DECL(XRESOLUTION,             putRational, NULL),
    DECL(YCBCRCOEFFICIENTS,       NULL,        NULL),
    DECL(YCBCRPOSITIONING,        NULL,        NULL),
    DECL(YCBCRSUBSAMPLING,        NULL,        NULL),
    DECL(YPOSITION,               putRational, NULL),
    DECL(YRESOLUTION,             putRational, NULL),
    DECL(ZIPQUALITY,              NULL,        NULL),
};
#undef DECL



const tagDefinition *
tagDefFind(const char * const name) {

    unsigned int i;

    for (i = 0;
         i < ARRAY_SIZE(tagDefinitions) && tagDefinitions[i].name;
         ++i) {
        if (STRCASEEQ(tagDefinitions[i].name, name))
            return &tagDefinitions[i];
    }

    return NULL;
}
