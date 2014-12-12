/*============================================================================
                               svgtopam
==============================================================================
  This is not useful today.  It is merely a stub from which someone who
  cares about SVG can build a full converter.

  The framework is all there; it should be just a matter of coding to
  add each of the SVG features to it.

  Today, the program works fine on an image that consists solely of
  <path> elements, which use only the "M", "L", and "z" commands.

  By Bryan Henderson, San Jose, California.  May 2006

  Contributed to the public domain.
============================================================================*/

#define _BSD_SOURCE  /* Make sure strdup() is in <string.h> */
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <libxml/xmlreader.h>

#include "pm_c_util.h"
#include "mallocvar.h"
#include "nstring.h"
#include "shhopt.h"
#include "pam.h"
#include "ppm.h"
#include "ppmdraw.h"


static bool traceDraw;

struct cmdlineInfo {
    const char * inputFileName;
    unsigned int trace;
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

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "trace",     OPT_FLAG,   NULL,                  
            &cmdlineP->trace,       0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;   /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
    /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 < 1)
        cmdlineP->inputFileName = "-";
    else {
        cmdlineP->inputFileName = argv[1];
        
        if (argc-1 > 1)
            pm_error("Too many arguments (%u).  The only non-option argument "
                     "is the input file name.", argc-1);
    }
}



/*============================================================================
   Wrappers for libxml2 routines.

   The difference is that these use conventional C data types and have
   shorter names.
=============================================================================*/

static const char *
getAttribute(xmlTextReaderPtr const xmlReaderP,
             const char *     const attributeName) {

    return (const char *)
        xmlTextReaderGetAttribute(xmlReaderP, (const xmlChar *)attributeName);
}



static const char *
currentNodeName(xmlTextReaderPtr const xmlReaderP) {

    return (const char *)xmlTextReaderConstName(xmlReaderP);
}



/*===========================================================================*/

#define OUTPUT_MAXVAL 255

typedef struct {
    unsigned int width;
    unsigned int height;
    pixel ** pixels;
    pixval maxval;
} canvas;

typedef struct {
    pixel fillColor;
} style;



typedef struct {
    const char * pathText;
        /* This is e.g. "M0 0 L1 1 L9 8 Z" */
    style        style;
        /* This is the style as given by a 'style' attribute of <path> */
    unsigned int pathTextLength;
        /* This is the length in characters of 'pathText'.  It's redundant
           with 'pathText' and exists for convenience.
        */
} path;

static void
createPath(const char * const pathText,
           style        const style,
           path **      const pathPP) {
/*----------------------------------------------------------------------------
   Create a path as described by a <path> element whose "style" attribute
   indicates style 'style' and whose "d" attribute indicates path data
   'pathText'.
-----------------------------------------------------------------------------*/
    bool error;
    path * pathP;
    
    MALLOCVAR(pathP);
    if (pathP == NULL)
        error = TRUE;
    else {
        pathP->style = style;

        pathP->pathText = strdup(pathText);
        if (pathP->pathText == NULL)
            error = TRUE;
        else {
            pathP->pathTextLength = strlen(pathP->pathText);

            error = FALSE;
        }
        if (error)
            free(pathP);
    }
    if (error )
        *pathPP = NULL;
    else
        *pathPP = pathP;
}



static void
destroyPath(path * const pathP) {
    
    assert(pathP->pathTextLength == strlen(pathP->pathText));

    strfree(pathP->pathText);

    free(pathP);
}



typedef struct {
    unsigned int x;
    unsigned int y;
} point;

static point
makePoint(unsigned int const x,
          unsigned int const y) {

    point p;
    
    p.x = x;
    p.y = y;
    
    return p;
}



typedef enum {
    PATH_MOVETO,
    PATH_LINETO,
    PATH_CLOSEPATH,
    PATH_CUBIC
} pathCommandVerb;

typedef struct {
    point dest;
} pathMovetoArgs;

typedef struct {
    /* Draw a line segment from current point to 'dest' */
    point dest;
} pathLinetoArgs;

typedef struct {
    /* Draw a cubic spline from current point to 'dest' with control points
       'ctl1' at the beginning of the curve and 'ctl2' at the end.

       I.e. it's a section of a cubic curve which passes through the
       current point and 'dest' and whose slope at the current point
       is that of the line through the current point and 'ctl1' and
       whose slope at 'dest' is that of the line through 'dest' and
       'ctl2';

       A cubic curve is a plot of a polynomial equation of degree 3
       (or less, for our purposes).
    */
    point dest;
    point ctl1;
    point ctl2;
} pathCubicArgs;

typedef struct {
    pathCommandVerb verb;
    union {
        pathMovetoArgs moveto;
        pathLinetoArgs lineto;
        pathCubicArgs  cubic;
    } args;
} pathCommand;



typedef struct {
/*----------------------------------------------------------------------------
   This is an object for reading through a path from beginning to end.
-----------------------------------------------------------------------------*/
    path *       pathP;
    unsigned int cursor;
} pathReader;

static void
createPathReader(path *        const pathP,
                 pathReader ** const pathReaderPP) {

    pathReader * pathReaderP;

    MALLOCVAR_NOFAIL(pathReaderP);

    pathReaderP->pathP = pathP;
    pathReaderP->cursor = 0;

    *pathReaderPP = pathReaderP;
}

static void
destroyPathReader(pathReader * const pathReaderP) {
    free(pathReaderP);
}



static void
skipWhiteSpace(pathReader * const pathReaderP) {
/*----------------------------------------------------------------------------
   Move the cursor over any white space where it now points.
-----------------------------------------------------------------------------*/
    const path * const pathP = pathReaderP->pathP;

    while (isspace(pathP->pathText[pathReaderP->cursor]) &&
           pathReaderP->cursor < pathP->pathTextLength)
        ++pathReaderP->cursor;
}



static void
getNumber(pathReader *   const pathReaderP,
          unsigned int * const numberP) {

    const path * const pathP          = pathReaderP->pathP;
    const char * const pathText       = pathP->pathText;
    size_t       const pathTextLength = pathP->pathTextLength;

    assert(!isspace(pathText[pathReaderP->cursor]));

    if (pathReaderP->cursor >= pathTextLength)
        pm_error("Path description ends where a number was expected.");
    else {
        unsigned int number;

        number = 0;  /* initial value */

        while (pathReaderP->cursor < pathTextLength &&
               isdigit(pathText[pathReaderP->cursor])) {
            number = 10 * number + (pathText[pathReaderP->cursor] - '0');
            ++pathReaderP->cursor;
        }
        *numberP = number;
    }
}



static void
getNextCommand(pathReader *  const pathReaderP,
               pathCommand * const pathCommandP,
               bool *        const endOfPathP) {

    const path * const pathP          = pathReaderP->pathP;
    const char * const pathText       = pathP->pathText;
    size_t       const pathTextLength = pathP->pathTextLength;

    skipWhiteSpace(pathReaderP);

    if (pathReaderP->cursor >= pathTextLength)
        *endOfPathP = true;
    else {
        switch (pathText[pathReaderP->cursor++]) {
        case 'M':
            pathCommandP->verb = PATH_MOVETO;
            skipWhiteSpace(pathReaderP);
            getNumber(pathReaderP, &pathCommandP->args.moveto.dest.x);
            skipWhiteSpace(pathReaderP);
            getNumber(pathReaderP, &pathCommandP->args.moveto.dest.y);
            break;
        case 'L':
            pathCommandP->verb = PATH_LINETO;
            skipWhiteSpace(pathReaderP);
            getNumber(pathReaderP, &pathCommandP->args.lineto.dest.x);
            skipWhiteSpace(pathReaderP);
            getNumber(pathReaderP, &pathCommandP->args.lineto.dest.y);
            break;
        case 'C':
            pathCommandP->verb = PATH_CUBIC;
            skipWhiteSpace(pathReaderP);
            getNumber(pathReaderP, &pathCommandP->args.cubic.ctl1.x);
            skipWhiteSpace(pathReaderP);
            getNumber(pathReaderP, &pathCommandP->args.cubic.ctl1.y);
            skipWhiteSpace(pathReaderP);
            getNumber(pathReaderP, &pathCommandP->args.cubic.ctl2.x);
            skipWhiteSpace(pathReaderP);
            getNumber(pathReaderP, &pathCommandP->args.cubic.ctl2.y);
            skipWhiteSpace(pathReaderP);
            getNumber(pathReaderP, &pathCommandP->args.cubic.dest.x);
            skipWhiteSpace(pathReaderP);
            getNumber(pathReaderP, &pathCommandP->args.cubic.dest.y);
            break;
        case 'z':
            pathCommandP->verb = PATH_CLOSEPATH;
            break;
        default:
            pm_error("Unrecognized command in <path>: '%c'",
                     pathText[pathReaderP->cursor++]);
        }
    }
}


static void
outlineObject(path *           const pathP,
              struct fillobj * const fillObjP) {
/*----------------------------------------------------------------------------
  Create a fill object, which contains and outline of the object and
  can be used with ppmd_fill() to fill the figure.  The outline is as
  described by *pathP.
-----------------------------------------------------------------------------*/
    pathReader * pathReaderP;
    bool endOfPath;
    point currentPos;
    point subpathStart;
        /* Point at which the current subpath starts */

    endOfPath = false;
    subpathStart = makePoint(0,0);
    currentPos = subpathStart;

    createPathReader(pathP, &pathReaderP);

    while (!endOfPath) {
        pathCommand pathCommand;
        getNextCommand(pathReaderP, &pathCommand, &endOfPath);
        if (!endOfPath) {
            switch (pathCommand.verb) {
            case PATH_MOVETO:
                if (traceDraw)
                    pm_message("Moving to (%u, %u)",
                               pathCommand.args.moveto.dest.x,
                               pathCommand.args.moveto.dest.y);
                subpathStart = pathCommand.args.moveto.dest;
                currentPos = subpathStart;
                break;
            case PATH_LINETO: {
                point const dest = pathCommand.args.lineto.dest;
                if (traceDraw)
                    pm_message("Lining to (%u, %u)", dest.x, dest.y);
                ppmd_line(NULL, 0, 0, 0,
                          currentPos.x, currentPos.y, dest.x, dest.y,
                          ppmd_fill_drawproc, fillObjP);
                currentPos = dest;
            } break;
            case PATH_CLOSEPATH:
                if (traceDraw)
                    pm_message("Closing.");
                ppmd_line(NULL, 0, 0, 0,
                          currentPos.x, currentPos.y,
                          subpathStart.x, subpathStart.y,
                          ppmd_fill_drawproc, fillObjP);
                currentPos = subpathStart;
                break;
            case PATH_CUBIC: {
                point const dest = pathCommand.args.cubic.dest;
                point const ctl1 = pathCommand.args.cubic.ctl1;
                point const ctl2 = pathCommand.args.cubic.ctl2;
                if (traceDraw)
                    pm_message("Doing cubic spline to (%u, %u)",
                               dest.x, dest.y);
                /* We need to write ppmd_spline4() */
                ppmd_spline4(NULL, 0, 0, 0,
                             currentPos.x, currentPos.y,
                             ctl1.x, ctl1.y,
                             ctl2.x, ctl2.y,
                             dest.x, dest.y,
                             ppmd_fill_drawproc, fillObjP);
                currentPos = dest;
            } break;
            }
        }
    }
    destroyPathReader(pathReaderP);
}



static void
drawPath(canvas * const canvasP,
         path *   const pathP) {
/*----------------------------------------------------------------------------
   Draw the path 'pathP' on the canvas 'canvasP'.
-----------------------------------------------------------------------------*/
    struct fillobj * fillObjP;

    if (traceDraw)
        pm_message("Drawing path '%s' with fill color (%u, %u, %u)",
                   pathP->pathText,
                   pathP->style.fillColor.r,
                   pathP->style.fillColor.g,
                   pathP->style.fillColor.b);

    fillObjP = ppmd_fill_create();

    outlineObject(pathP, fillObjP);

    ppmd_fill(canvasP->pixels, canvasP->width, canvasP->height,
              canvasP->maxval,
              fillObjP, 
              PPMD_NULLDRAWPROC, &pathP->style.fillColor);

    ppmd_fill_destroy(fillObjP);
}



static style
interpretStyle(const char * const styleAttr) {

    style style;

    char * buffer;

    char * p;

    buffer = strdup(styleAttr);
    if (buffer == NULL)
        pm_error("Could not get memory for a buffer to parse style attribute");

    p = &buffer[0];

    while (p) {
        const char * const token = strsepN(&p, ";");
        const char * strippedToken;
        const char * p;
        char * buffer;

        for (p = &token[0]; isspace(*p); ++p);
        
        strippedToken = p;

        buffer = strdup(strippedToken);

        if (strlen(strippedToken) > 0) {
            char * const colonPos = strchr(buffer, ':');

            if (colonPos == NULL)
                pm_error("There is no colon in the attribute specification "
                         "'%s' in the 'style' attribute of a <path> "
                         "element.", strippedToken);
            else {
                const char * const value = colonPos + 1;
                const char * const name  = &buffer[0];
                
                *colonPos = '\0';

                if (streq(name, "fill")) {
                    style.fillColor = ppm_parsecolor(value, OUTPUT_MAXVAL);
                } else if (streq(name, "stroke")) {
                    if (!streq(value, "none"))
                        pm_error("Value of 'stroke' attribute in the 'style' "
                                 "attribute of a <path> element is '%s'.  We "
                                 "understand only 'none'", value);
                } else
                    pm_error("Unrecognized attribute '%s' "
                             "in the 'style' attribute "
                             "of a <path> element", name);
            }
        }
        free(buffer);
    }

    free(buffer);

    return style;
}


static void
getPathAttributes(xmlTextReaderPtr const xmlReaderP,
                  style *          const styleP,
                  const char **    const pathP) {

    const char * const style = getAttribute(xmlReaderP, "style");
    const char * const d     = getAttribute(xmlReaderP, "d");

    *styleP = interpretStyle(style);
    *pathP = d;
}



static void
processSubPathNode(xmlTextReaderPtr const xmlReaderP,
                   bool *           const endOfPathP) {

    xmlReaderTypes const nodeType  = xmlTextReaderNodeType(xmlReaderP);

    *endOfPathP = FALSE;  /* initial assumption */

    switch (nodeType) {
    case XML_READER_TYPE_ELEMENT:
        pm_error("<path> contains a <%s> element.  <path> should have "
                 "no contents", currentNodeName(xmlReaderP));
        break;
    case XML_READER_TYPE_END_ELEMENT: {
        const char * nodeName = currentNodeName(xmlReaderP);
        if (streq(nodeName, "path"))
            *endOfPathP = TRUE;
        else
            pm_error("</%s> found where </path> expected", nodeName);
        } break;
    default:
        /* Just ignore whatever this is.  Contents of <path> are
           meaningless; all the information is in the attributes 
        */
        break;
    }
}



static void
processPathElement(xmlTextReaderPtr const xmlReaderP,
                   canvas *         const canvasP) {

    style style;
    const char * pathData;
    path * pathP;
    bool endOfPath;

    assert(xmlTextReaderNodeType(xmlReaderP) == XML_READER_TYPE_ELEMENT);
    assert(streq(currentNodeName(xmlReaderP), "path"));

    getPathAttributes(xmlReaderP, &style, &pathData);

    createPath(pathData, style, &pathP);

    if (pathP) {
        drawPath(canvasP, pathP);
        destroyPath(pathP);
    }

    endOfPath = xmlTextReaderIsEmptyElement(xmlReaderP);

    while (!endOfPath) {
        int rc;

        rc = xmlTextReaderRead(xmlReaderP);
        
        switch (rc) {
        case 1:
            processSubPathNode(xmlReaderP, &endOfPath);
            break;
        case 0:
            pm_error("Input file ends in the middle of a <path>");
            break;
        default:
            pm_error("xmlTextReaderRead() failed, rc=%d", rc);
        }
    }
}



static void
stringToUint(const char *   const string,
             unsigned int * const uintP,
             const char **  const errorP) {

    /* TODO: move this to nstring.c */

    if (strlen(string) == 0)
        asprintfN(errorP, "Value is a null string");
    else {
        char * tailptr;

        *uintP = strtoul(string, &tailptr, 10);

        if (*tailptr != '\0')
            asprintfN(errorP, "Non-numeric crap in string: '%s'", tailptr);
        else
            *errorP = NULL;
    }
}



static void
getSvgAttributes(xmlTextReaderPtr const xmlReaderP,
                 unsigned int *   const colsP,
                 unsigned int *   const rowsP) {

    const char * const width  = getAttribute(xmlReaderP, "width");
    const char * const height = getAttribute(xmlReaderP, "height");

    const char * error;

    stringToUint(width, colsP, &error);
    if (error) {
        pm_error("'width' attribute of <svg> has invalid value.  %s", error);
        strfree(error);
    }
    stringToUint(height, rowsP, &error);
    if (error) {
        pm_error("'height' attribute of <svg> has invalid value.  %s", error);
        strfree(error);
    }
}



static void
processSubSvgElement(xmlTextReaderPtr const xmlReaderP,
                     canvas *         const canvasP) {

    const char * const nodeName = currentNodeName(xmlReaderP);

    assert(xmlTextReaderNodeType(xmlReaderP) == XML_READER_TYPE_ELEMENT);
    
    if (streq(nodeName, "path"))
        processPathElement(xmlReaderP, canvasP);
    else
        pm_error("This image contains a <%s> element.  This program "
                 "understands only <path>!", nodeName);
}



static void
processSubSvgNode(xmlTextReaderPtr const xmlReaderP,
                  canvas *         const canvasP,
                  bool *           const endOfSvgP) {

    xmlReaderTypes const nodeType = xmlTextReaderNodeType(xmlReaderP);

    *endOfSvgP = FALSE;  /* initial assumption */

    switch (nodeType) {
    case XML_READER_TYPE_ELEMENT:
        processSubSvgElement(xmlReaderP, canvasP);
        break;
    case XML_READER_TYPE_END_ELEMENT: {
        const char * const nodeName = currentNodeName(xmlReaderP);
        if (streq(nodeName, "svg"))
            *endOfSvgP = TRUE;
        else
            pm_error("</%s> found where </svg> expected", nodeName);
    } break;
    default:
        /* Just ignore whatever this is */
        break;
    }
}



static void
createCanvas(unsigned int const width,
             unsigned int const height,
             pixval       const maxval,
             canvas **    const canvasPP) {

    canvas * canvasP;

    MALLOCVAR_NOFAIL(canvasP);

    canvasP->width  = width;
    canvasP->height = height;
    canvasP->pixels = ppm_allocarray(width, height);
    canvasP->maxval = maxval;

    *canvasPP = canvasP;
}



static void
destroyCanvas(canvas * const canvasP) {

    ppm_freearray(canvasP->pixels, canvasP->height);

    free(canvasP);
}



static void
writePam(FILE *   const ofP,
         canvas * const canvasP) {

    unsigned int row;
    struct pam pam;
    tuple * tuplerow;

    pam.size             = sizeof(pam);
    pam.len              = PAM_STRUCT_SIZE(tuple_type);
    pam.file             = ofP;
    pam.format           = PAM_FORMAT;
    pam.plainformat      = 0;
    pam.width            = canvasP->width;
    pam.height           = canvasP->height;
    pam.depth            = 3;
    pam.maxval           = OUTPUT_MAXVAL;
    strcpy(pam.tuple_type, PAM_PPM_TUPLETYPE);
    
    pnm_writepaminit(&pam);

    tuplerow = pnm_allocpamrow(&pam);

    for (row = 0; row < (unsigned)pam.height; ++row) {
        pixel * const pixelrow = canvasP->pixels[row];
        unsigned int col;

        for (col = 0; col < (unsigned)pam.width; ++col) {
            pixel const thisPixel = pixelrow[col];
            assert(pam.depth >= 3);

            tuplerow[col][PAM_RED_PLANE] = PPM_GETR(thisPixel);
            tuplerow[col][PAM_GRN_PLANE] = PPM_GETG(thisPixel);
            tuplerow[col][PAM_BLU_PLANE] = PPM_GETB(thisPixel);
        }
        pnm_writepamrow(&pam, tuplerow);
    }
    pnm_freepamrow(tuplerow);
}



static void
processSvgElement(xmlTextReaderPtr const xmlReaderP,
                  FILE *           const ofP) {

    unsigned int width, height;
    bool endOfSvg;
    canvas * canvasP;

    assert(xmlTextReaderNodeType(xmlReaderP) == XML_READER_TYPE_ELEMENT);
    assert(streq(currentNodeName(xmlReaderP), "svg"));

    getSvgAttributes(xmlReaderP, &width, &height);

    createCanvas(width, height, 255, &canvasP);

    endOfSvg = xmlTextReaderIsEmptyElement(xmlReaderP);

    while (!endOfSvg) {
        int rc;

        rc = xmlTextReaderRead(xmlReaderP);
        
        switch (rc) {
        case 1:
            processSubSvgNode(xmlReaderP, canvasP, &endOfSvg);
            break;
        case 0:
            pm_error("Input file ends in the middle of a <svg>");
            break;
        default:
            pm_error("xmlTextReaderRead() failed, rc=%d", rc);
        }
    }
    writePam(ofP, canvasP);

    destroyCanvas(canvasP);
}



static void
processTopLevelElement(xmlTextReaderPtr const xmlReaderP,
                       FILE *           const ofP) {

    const char * const nodeName = currentNodeName(xmlReaderP);

    assert(xmlTextReaderNodeType(xmlReaderP) == XML_READER_TYPE_ELEMENT);
    assert(xmlTextReaderDepth(xmlReaderP) == 0);

    if (!streq(nodeName, "svg"))
        pm_error("Not an SVG image.  This XML document consists of "
                 "a <%s> element, whereas an SVG image is an <svg> "
                 "element.", nodeName);
    else
        processSvgElement(xmlReaderP, ofP);
}



static void
processTopLevelNode(xmlTextReaderPtr const xmlReaderP,
                    FILE *           const ofP) {

    unsigned int   const depth    = xmlTextReaderDepth(xmlReaderP);
    xmlReaderTypes const nodeType = xmlTextReaderNodeType(xmlReaderP);

    assert(depth == 0);

    switch (nodeType) {
    case XML_READER_TYPE_ELEMENT:
        processTopLevelElement(xmlReaderP, ofP);
        break;
    default:
        /* Just ignore whatever this is */
        break;
    }
}



static void
processDocument(xmlTextReaderPtr const xmlReaderP,
                FILE *           const ofP) {

    bool eof;

    eof = false;

    while (!eof) {
        int rc;

        rc = xmlTextReaderRead(xmlReaderP);
        
        switch (rc) {
        case 1:
            processTopLevelNode(xmlReaderP, ofP);
            break;
        case 0:
            eof = true;
            break;
        default:
            pm_error("xmlTextReaderRead() failed, rc=%d", rc);
        }
    }
}



int
main(int argc, char **argv) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    xmlTextReaderPtr xmlReaderP;

    pnm_init(&argc, argv);

    xmlInitParser();

    LIBXML_TEST_VERSION;

    parseCommandLine(argc, argv, &cmdline);
    
    traceDraw = cmdline.trace;

    ifP = pm_openr(cmdline.inputFileName);

    xmlReaderP = xmlReaderForFd(fileno(ifP), "SVG_IMAGE", NULL, 0);

    if (xmlReaderP) {
        processDocument(xmlReaderP, stdout);

        /* xmlTextReaderIsValid() does not appear to work.  It always says
           the document is invalid
        */

        xmlFreeTextReader(xmlReaderP);
    } else
        pm_error("Failed to create xmlReader");

    xmlCleanupParser();

    return 0;
}
