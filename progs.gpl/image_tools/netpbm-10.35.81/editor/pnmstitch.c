/*
 * Copyright (c) 2002 Mark Salyzyn
 * All rights reserved.
 *
 * TERMS AND CONDITIONS OF USE
 *
 * Redistribution and use in source form, with or without modification, are
 * permitted provided that redistributions of source code must retain the
 * above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * This software is provided `as is' by Mark Salyzyn and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose, are disclaimed. In no
 * event shall Mark Salyzyn be liable for any direct, indirect, incidental,
 * special, exemplary or consequential damages (including, but not limited to,
 * procurement of substitute goods or services; loss of use, data, or profits;
 * or business interruptions) however caused and on any theory of liability,
 * whether in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even if
 * advised of the possibility of such damage.
 *
 * Any restrictions or encumberances added to this source code or derivitives,
 * is prohibited.
 *
 *  Name: pnmstitch.c
 *  Description: Automated panoramic stitcher.
 *      Many digital Cameras have a panorama mode where they hold on to
 *    the right hand side of an image, shifted to the left hand side of their
 *    view screen for subsequent pictures, facilitating the manual stitching
 *    up of a panoramic shot. These same cameras are shipped with software
 *    to manually or automatically stitch images together into the composite
 *    image. However, these programs are dedicated for a specific OS.
 *    In the pnmstitch program, it analyzes the match between the images,
 *    generates a transform, processes the transform on the images, and
 *    blends the overlapping regions. In addition, there is an output filter
 *    to process automatic cropping of the resultant image.
 *      The stitching software here works by constraining the right
 *    hand side of the right hand image as `fixed' per-se, after offset
 *    evaluation and only the left hand side of the right hand image is
 *    mangled. Thus, the algorithm is optimized for stitching a right hand
 *    image to the right hand half (half being a loose term) of the left hand
 *    image.
 *  Author: Mark Salyzyn <mark@bohica.net>  June 2002
 *  Version: 0.0.4
 *
 *  Modifications: 0.0.4  July 31 2002 Mark Salyzyn <mark@bohica.net>
 *                                  &  Bryan Henderson <bryanh@giraffe-data.com>
 *      - FreeBSD port.
 *      - merge changes to incorporate into netpbm tree.
 *  Modifications: 0.0.3  July 27 2002  Mark Salyzyn <mark@bohica.net>
 *                                  &   "George M. Sipe" <geo@sipe.org>
 *      - Deal with subtle differences between BSD and GNU getopt
 *        facilitating the Linux port.
 *  Modifications: 0.0.2  July 25 2002  Mark Salyzyn <mark@bohica.net>
 *      - RotateSliver needs to use higher resolution match.
 *      - RotateCrop code interfered with StraightThrough code
 *        resulting in an incorrect pnm image output.
 *  Modifications: 0.0.1  July 18 2002  Mark Salyzyn <mark@bohica.net>
 *      - Added BiLinearSliver, RotateSliver and HorizontalCrop
 *
 *  ToDo:
 *      - Split this into multiple files ... nah, keep it in one to
 *        keep it from polluting the netpbm tree.
 *      - Add and refine the videorbits algorithm. One piece of public
 *        domain software that is pnm aware is called videorbits. It
 *        needs considerably more overlap than the Digital Cameras set
 *        up (of course, anyone can to a panorama with more overlap
 *        with our without the feature) as it uses what is called video
 *        flow to generate the match. It is designed more for a series
 *        of images from a video camera. videorbits has three programs,
 *        one generates the transform, the next processes the
 *        transform, and the final blends the images together much as
 *        this one piece program does.
 *      - speedups in matching algorithm
 *      - refinement in accuracy of matching algorithm
 *      - Add RotateCrop filter algorithm (in-memory copy of image,
 *        detect least loss horizontal crop on a rotated image).
 *      - pnmstitch should be generalized to handle transformation
 *        occuring on the left image, currently it blends assuming
 *        that there is no transformation effects on the left image.
 *      - user selectable blending algorithms?
 */

#define _BSD_SOURCE 1   /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "pm_c_util.h"
#include "shhopt.h"
#include "nstring.h"
#include "mallocvar.h"
#include "pam.h"

/*
 *  Structures
 */

/*
 *  Image structure
 */
typedef struct {
    const char * name;  /* File Name                */
    struct pam   pam;   /* netpbm image description */
    tuple **     tuple; /* in-memory copy of image  */
} Image;

/*
 *  Output class
 *  The following methods and data allocations are used for output filter.
 */
typedef struct output {
    /* name */
    const char * Name;
    /* methods */
    bool      (* Alloc)(struct output * me, const char * file,
                        unsigned int width, unsigned int height,
                        struct pam * prototype);
    void      (* DeAlloc)(struct output * me);
    tuple    *(* Row)(struct output * me, unsigned row);
    void      (* FlushRow)(struct output * me, unsigned row);
    void      (* FlushImage)(struct output * me);
    /* data */
    Image      * image;
    void       * extra;
} Output;

extern Output OutputMethods[];

/*
 *  Stitching class
 *  The following methods and data allocations are used for operations
 *  surrounding stitching of an image.
 */
typedef struct stitcher {
    /* name */
    const char * Name;
    /* methods */
    bool      (* Alloc)(struct stitcher *me);
    void      (* DeAlloc)(struct stitcher *me);
    void      (* Constrain)(struct stitcher *me, int x, int y,
                            int width, int height);
        /* Set transformation parameter constraints.  This affects the
           function of a future 'Match' method execution.
        */
    bool      (* Match)(struct stitcher *me, Image * Left, Image * Right);
        /* Determine the transformation parameters for the stitching.
           I.e. determine the parameters that affect future invocations
           of the transformation methods below.  You must execute a 
           'Match' before executing any of the transformation methods.
        */
    /*-----------------------------------------------------------------------
      The transformation methods answer the question, "Which pixel in the left
      image and which pixel in the right image contribute to the pixel at
      Column X, Column Y of the output?

      If there is no pixel in the left image that contributes to the output
      pixel in question, the methods return column or row numbers outside
      the bounds of the left image (possibly negative).  Likewise for the 
      right image.
    */
    float     (* XLeft)(struct stitcher *me, int x, int y);
        /* column number of the pixel from the left image */
    float     (* YLeft)(struct stitcher *me, int x, int y);
        /* row number of the pixel from the left image */
    float     (* XRight)(struct stitcher *me, int x, int y);
        /* column number of the pixel from the right image */
    float     (* YRight)(struct stitcher *me, int x, int y);
        /* row number of the pixel from the left image */
    /*----------------------------------------------------------------------*/
    /* Output methods */
    void      (* Output)(struct stitcher *me, FILE * fp);
    /* private data */
    int          x, y, width, height;
    /* For a Linear Sliver stitcher, 'x' and 'y' are simply the offset you
       add to an output location to get the location in the right image of the
       pixel that corresponds to that output pixel.
    */
    float      * parms;
} Stitcher;

extern Stitcher StitcherMethods[];

/*
 *  Prototypes
 */
static int pnmstitch(const char * const left,
                     const char * const right,
                     const char * const out,
                     int          const x,
                     int          const y,
                     int          const width,
                     int          const height,
                     const char * const stitcher,
                     const char * const filter);

struct cmdlineInfo {
    /*
     * All the information the user supplied in the command line,
     * in a form easy for the program to use.
     */
    const char * leftFilespec;   /* '-' if stdin */
    const char * rightFilespec;  /* '-' if stdin */
    const char * outputFilespec; /* '-' if stdout */
    const char * stitcher;
    const char * filter;
    int          width;
    int          height;
    int          xrightpos;
    int          yrightpos;
    unsigned int verbose;
};

static char minus[] = "-";

static void
parseCommandLine ( int argc, char ** argv,
                   struct cmdlineInfo *cmdlineP )
{
/*----------------------------------------------------------------------------
   parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc( 100*sizeof( optEntry ) );
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    char *outputOpt;
    unsigned int widthSpec, heightSpec, outputSpec, 
        xrightposSpec, yrightposSpec, stitcherSpec, filterSpec;

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "width",       OPT_UINT,   &cmdlineP->width, 
            &widthSpec,         0);
    OPTENT3(0, "height",      OPT_UINT,   &cmdlineP->height, 
            &heightSpec,        0);
    OPTENT3(0, "verbose",     OPT_FLAG,   NULL,                  
            &cmdlineP->verbose, 0 );
    OPTENT3(0, "output",      OPT_STRING, &outputOpt, 
            &outputSpec,        0);
    OPTENT3(0, "xrightpos",   OPT_UINT,   &cmdlineP->xrightpos, 
            &xrightposSpec,     0);
    OPTENT3(0, "yrightpos",   OPT_UINT,   &cmdlineP->yrightpos, 
            &yrightposSpec,     0);
    OPTENT3(0, "stitcher",    OPT_STRING, &cmdlineP->stitcher, 
            &stitcherSpec,      0);
    OPTENT3(0, "filter",      OPT_STRING, &cmdlineP->filter, 
            &filterSpec,        0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!widthSpec) {
        cmdlineP->width = INT_MAX;
    }
    if (!heightSpec) {
        cmdlineP->height = INT_MAX;
    }
    if (!xrightposSpec) {
        cmdlineP->xrightpos = INT_MAX;
    }
    if (!yrightposSpec) {
        cmdlineP->yrightpos = INT_MAX;
    }
    if (!stitcherSpec) {
        cmdlineP->stitcher = "BiLinearSliver";
    }
    if (!filterSpec) {
        cmdlineP->filter = "StraightThrough";
    }

    if (argc-1 > 3) {
        pm_error("Program takes at most three arguments: left, right, and "
                 "output file specifications.  You specified %d", argc-1);
        /* NOTREACHED */
    } else {
        if (argc-1 == 0) {
            cmdlineP->leftFilespec = minus;
            cmdlineP->rightFilespec = minus;
        } else if (argc-1 == 1) {
            cmdlineP->leftFilespec = minus;
            cmdlineP->rightFilespec = argv[1];
        } else {
            cmdlineP->leftFilespec = argv[1];
            cmdlineP->rightFilespec = argv[2];
        }
        if (argc-1 == 3 && outputSpec) {
            pm_error("You cannot specify --output and also name the "
                     "output file with the 3rd argument.");
            /* NOTREACHED */
        } else if (argc-1 == 3) {
            cmdlineP->outputFilespec = argv[3];
        } else if (outputSpec) {
            cmdlineP->outputFilespec = outputOpt;
        } else {
            cmdlineP->outputFilespec = minus;
        }
    }
} /* parseCommandLine() - end */

static int  verbose;

/*
 *  Parse the command line, call pnmstitch to perform work.
 */
int
main (int argc, char **argv)
{
    struct cmdlineInfo cmdline;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    verbose = cmdline.verbose;

    return pnmstitch (cmdline.leftFilespec, 
                      cmdline.rightFilespec, 
                      cmdline.outputFilespec, 
                      cmdline.xrightpos, 
                      cmdline.yrightpos, 
                      cmdline.width, 
                      cmdline.height, 
                      cmdline.stitcher, 
                      cmdline.filter);
} /* main() - end */



/*
 *  allocate a clear image structure.
 */
static Image *
allocate_image(void)
{
    Image * retVal;
    
    MALLOCVAR(retVal);

    if (retVal != NULL) 
        memset (retVal, 0, (unsigned)sizeof(Image));

    return retVal;
}



/*
 *  free an image structure.
 */
static void
free_image(Image * image)
{
    if (image->name) {
        strfree(image->name);
        image->name = NULL;     
    }
    if (image->tuple) {
        pnm_freepamarray(image->tuple, &image->pam);
        image->tuple = NULL; 
    }
    if (image->pam.file) {
        fclose (image->pam.file);
        image->pam.file = NULL;
    }
    free (image);
}



static void
openWithPossibleExtension(const char *  const baseName,
                          FILE **       const ifPP,
                          const char ** const filenameP) {

    /* list of possible extensions for input file */
    const char * const extlist[] = { 
        "", ".pnm", ".pam", ".pgm", ".pbm", ".ppm" 
    };

    FILE * ifP;
    unsigned int extIndex;

    ifP = NULL;   /* initial value -- no file opened yet */

    for (extIndex = 0; extIndex < ARRAY_SIZE(extlist) && !ifP; ++extIndex) {
        
        const char * trialName;
        
        asprintfN(&trialName, "%s%s", baseName, extlist[extIndex]);
        
        ifP = fopen(trialName, "rb");
        
        if (ifP)
            *filenameP = trialName;
        else
            strfree(trialName);
    }
    if (!ifP) 
        pm_error ("Failed to open input file named '%s' "
                  "or '%s' with one of various common extensions.", 
                  baseName, baseName);
    
    *ifPP = ifP;
}



/*
 *  Create an image object for the PNM/PAM image in the file name 'name'
 *  This includes reading the entire image.
 */
static Image *
readinit(const char * const name)
{
    Image * const image = allocate_image();
    Image * retVal;

    if (image == NULL) 
        retVal = NULL;
    else {
        FILE * ifP;

        if (strcmp(name,minus) == 0) {
            ifP = stdin;
            image->name = strdup("<stdin>");
        } else {
            openWithPossibleExtension(name, &ifP, &image->name);
        }
        image->tuple = pnm_readpam(ifP, &(image->pam), 
                                   PAM_STRUCT_SIZE(tuple_type));
        fclose (ifP);
        image->pam.file = NULL;

        if (image->tuple == NULL) {
            free_image(image);
            retVal = NULL;
        } else
            retVal = image;
    }
    return retVal;
} /* readinit() - end */

/*
 *  Prepare an image to be output.
 *      Too bad we can't help them and add a .pnm on the filename,
 *      since this would be `bad'. on readinit we can check if the .pnm
 *      needs to be added ...
 */
static bool
writeinit(Image * image)
{
    if (strcmp(image->name,minus) == 0) {
        image->pam.file = stdout;
        strfree(image->name);
        image->name = strdup("<stdout>");
    } else {
        image->pam.file = pm_openw(image->name);
    }
    return TRUE;
} /* writeinit() - end */

/*
 *  Compare a subimage to an image.
 *  The most time consuming actions surround this subroutine.
 *  Return the magnitude of the difference between the specified region
 *  in image 'left' and the specified region in image 'right'.
 *  The magnitude is defined as the sum of the squares of the differences
 *  between intensities of each of the 3 colors over all the pixels.
 *
 *  The region in the left image is the rectangle with top left corner at
 *  Column 'lx', Row 'ly', with dimensions 'width' columns by 'height'
 *  rows.  The region in the right image is a rectangle the same dimensions
 *  with upper left corner at Column 'rx', Row 'ry'.
 *
 *  Caller must ensure that the regions indicated are entirely within the
 *  their respective images.
 */
static unsigned long
regionDifference(Image * left,
                 int     lx,
                 int     ly,
                 Image * right,
                 int     rx,
                 int     ry,
                 int     width,
                 int     height)
{
    unsigned long total;
    unsigned      row;
    
    total = 0;  /* initial value */

    for (row = 0; row < height; ++row) {
        unsigned column;

        for (column = 0; column < width; ++column) {
            unsigned plane;

            for (plane = 0; plane < left->pam.depth; ++plane) {
                sample const leftSample = left->tuple[row][column][plane];
                sample const rightSample = right->tuple[row][column][plane];
                total += SQR(leftSample - rightSample);
            }
        }
    }
    return total;
}



/*
 *  Generate a recent sorted histogram of best matches.
 */
typedef struct {
    unsigned long total;
    int           x;
    int           y;
} Best;
/* Arbitrary, except 9 points surrounding a hot spot plus one layer more */
#define NUM_BEST 9

/*
 *  Allocate the Best structure.
 */
static Best *
allocate_best(void)
{
    Best * retVal;

    MALLOCARRAY(retVal, NUM_BEST);

    if (retVal != NULL) {
        unsigned int    i;
        for (i = 0; i < NUM_BEST; ++i) {
            retVal[i].total = ULONG_MAX;
            retVal[i].x = INT_MAX;
            retVal[i].y = INT_MAX;
        }
    }
    return retVal;
}

/*
 *  Free the Best structure
 */
#define free_best(best) free(best)

/*
 *  Placement helper for the Best structure.
 */
static void
update_best(Best * best, unsigned long total, int x, int y)
{
    int i;
    if (best[NUM_BEST-1].total <= total) {
        return;
    }
    for (i = NUM_BEST - 1; i > 0; --i) {
        if (best[i-1].total < total) {
            break;
        }
        best[i] = best[i-1];
    }
    best[i].total = total;
    best[i].x = x;
    best[i].y = y;
}

/*
 *  Print helper for the Best structure.
 */
static void
pr_best(Best * const best)
{
    int i;
    if (best != (Best *)NULL)
    for (i = 0; i < NUM_BEST; ++i) {
        fprintf (stderr, " (%d,%d)%lu",
          best[i].x, best[i].y, best[i].total);
    }
} /* pr_best() - end */

static void *
findObject(const char * const name, void * start, unsigned size)
{
    char  ** object;
    char  ** best;
    unsigned length;

    if (name == (char *)NULL) {
        return start;
    }
    for (length = 0, best = (char **)NULL, object = (char **)start;
      *object != (char *)NULL;
      object = (char **)(((char *)object) + size)) {
        const char * np      = name;
        char       * op      = *object;
        unsigned     matched = 0;

        /* case insensitive match */
        while ((*np != '\0') && (*op != '\0')
         && ((*np == *op) || (tolower(*np) == tolower(*op)))) {
            ++np;
            ++op;
            ++matched;
        }
        if ((*np == '\0') && (*op == '\0')) {
            break;
        }
        if ((matched >= length) && (*np == '\0')) {
            if (matched == length) {
                best = (char **)NULL;
            } else {
                best = object;
            }
            length = matched;
        }
    }
    if (*object == (char *)NULL) {
        object = best;
    }
    if (object == (char **)NULL) {
        fprintf (stderr,
          "Unknown driver \"%s\". available drivers are:\n", name);
        for (object = (char **)start;
          *object != (char *)NULL;
          object = (char **)(((char *)object) + size)) {
            fprintf (stderr, "\t%s%s\n", *object,
              (object == (char **)start) ? " (default)" : "");
        }
    }
    return object;
}

/*
 *  The general wrapper for both the Output and the Stitcher algorithms.
 */

/* Determine the mask corners for both Left and Right images */
static void
determineMaskCorners(Stitcher * Stitch,
                     Image    * Left,
                     Image    * Right,
                     int        xp[],
                     int        yp[])
{
    int i;

    xp[0] = xp[1] = xp[4] = xp[5] = 0;
    yp[0] = yp[2] = yp[4] = yp[6] = 0;
    xp[2] = xp[3] = Left->pam.width;
    yp[1] = yp[3] = Left->pam.height;
    xp[6] = xp[7] = Right->pam.width;
    yp[5] = yp[7] = Right->pam.height;
    for (i = 0; i < 8; ++i) {
        int     x, y, xx, yy, count = 65536; /* max iterations */
        float (*X)(Stitcher *me, int x, int y);
        float (*Y)(Stitcher *me, int x, int y);

        if (i < 4) {
            X = Stitch->XLeft;
            Y = Stitch->YLeft;
        } else {
            X = Stitch->XRight;
            Y = Stitch->YRight;
        }
        x = xp[i];
        y = yp[i];
        /* will not work if rotated 90o or if gain > 10 */
        do {
            xx = ((*X)(Stitch, xp[i], yp[i]) + 0.5) - x;
            if (xx < 0) {
                if (xx > -100) {
                    ++xp[i];
                } else {
                    xp[i] -= xx / 10;
                }
            } else if (xx > 0) {
                if (xx < 100) {
                    --xp[i];
                } else {
                    xp[i] -= xx / 10;
                }
            }
            yy = ((*Y)(Stitch, xp[i], yp[i]) + 0.5) - y;
            if (yy < 0) {
                if (yy > -100) {
                    ++yp[i];
                } else {
                    yp[i] -= yy / 10;
                }
            } else if (yy > 0) {
                if (yy < 100) {
                    --yp[i];
                } else {
                    yp[i] -= yy / 10;
                }
            }
        } while (((xx != 0) || (yy != 0)) && (--count != 0));
    }
    if (verbose) {
        (*(Stitch->Output))(Stitch, stderr);
        if (verbose > 2) {
            static char quotes[] = "'\0\0\"\0\0'\"\0\"\"";
            fprintf (stderr, " Left:");
            for (i = 0; i < 8; ++i) {
                if (i == 4) {
                    fprintf (stderr, "\n Right:");
                }
                fprintf (stderr, " x%s,y%s=%d,%d",
                  &quotes[(i%4)*3], &quotes[(i%4)*3],
                  xp[i], yp[i]);
            }
        }
    }
} /* determineMaskCorners() - end */

static void
calculateXyWidthHeight(int         xp[],
                       int         yp[],
                       int * const xP,
                       int * const yP,
                       int * const widthP,
                       int * const heightP)
{
    int x, y, width, height, i;

    /* Calculate generic x,y left top corner, and the width and height */
    x = xp[0];
    y = yp[0];
    width = height = 0;
    for (i = 1; i < 8; ++i) {
        if (xp[i] < x) {
            width += x - xp[i];
            x = xp[i];
        } else if ((x + width) < xp[i]) {
            width = xp[i] - x;
        }
        if (yp[i] < y) {
            height += y - yp[i];
            y = yp[i];
        } else if ((y + height) < yp[i]) {
            height = yp[i] - y;
        }
    }
    *xP = x; *yP = y;
    *widthP = width; *heightP = height;
} /* calculateXyWidthHeight() - end */

static void
printPlan(int xp[], int yp[], Image * Left, Image * Right)
{
    /* Calculate Left image transformed bounds */
    int X, Y, W, H, i;

    X = xp[0];
    Y = yp[0];
    W = H = 0;
    for (i = 1; i < 4; ++i) {
        if (xp[i] < X) {
            W += X - xp[i];
            X = xp[i];
        } else if ((X + W) < xp[i]) {
            W = xp[i] - X;
        }
        if (yp[i] < Y) {
            H += Y - yp[i];
            Y = yp[i];
        } else if ((Y + H) < yp[i]) {
            H = yp[i] - Y;
        }
    }
    fprintf (stderr,
      "%s[%u,%u=>%d,%d](%d,%d)",
      Left->name, Left->pam.width, Left->pam.height,
      W, H, X, Y);
    X = xp[i];
    Y = yp[i];
    W = H = 0;
    for (++i; i < 8; ++i) {
        if (xp[i] < X) {
            W += X - xp[i];
            X = xp[i];
        } else if ((X + W) < xp[i]) {
            W = xp[i] - X;
        }
        if (yp[i] < Y) {
            H += Y - yp[i];
            Y = yp[i];
        } else if ((Y + H) < yp[i]) {
            H = yp[i] - Y;
        }
    }
    fprintf (stderr,
      "+%s[%u,%u=>%d,%d](%d,%d)",
      Right->name, Right->pam.width, Right->pam.height,
      W, H, X, Y);
} /* printPlan() - end */



static void
stitchOnePixel(Image *    const Left,
               Image *    const Right,
               struct pam const outpam,
               int        const row,
               int        const column,
               int        const y,
               int        const right_row,
               int        const right_column,
               unsigned * const firstRightP,
               tuple      const outPixel) {
               
    unsigned plane;

    for (plane = 0; plane < outpam.depth; ++plane) {
        sample leftPixel, rightPixel;
        /* Left `mix' is easy to find */
        leftPixel = (column < Left->pam.width)
                    ? (y < 0)
                        ? ((row < -y) || (row >= (Left->pam.height - y)))
                           ? 0
                           : Left->tuple[row + y][column][plane]
                        : (row < Left->pam.height)
                          ? Left->tuple[row][column][plane]
                          : 0
                     : 0;
        rightPixel = 0;
        if (right_column >= 0) {
            rightPixel = Right->tuple[right_row][right_column][plane];
            if ((rightPixel > 0) && (*firstRightP == 0)) 
                *firstRightP = column;
        }
        if (leftPixel == 0) {
            leftPixel = rightPixel;
        } else if ((*firstRightP <= column)
                   && (column < Left->pam.width)
                   && (rightPixel > 0)) {
            /* blend 7/8 over half of stitch */
            int const w = Left->pam.width - *firstRightP;
            if (column < (*firstRightP + w/2)) {
                int const v = (w * 4) / 7;
                leftPixel = (sample)(
                    ((leftPixel
                      * (unsigned long)(*firstRightP + v - column))
                     + (rightPixel
                        * (unsigned long)(column - *firstRightP)))
                    / (unsigned long)v);
            } else {
                int const v = w * 4;
                leftPixel = (sample)(
                    ((leftPixel 
                      * (unsigned long)(Left->pam.width - column))
                     + (rightPixel
                        * (unsigned long)(column - Left->pam.width + v)))
                    / (unsigned long)v);
            }
        }
        outPixel[plane] = leftPixel;
    }
}



static void
stitchOneRow(Image *    const Left,
             Image *    const Right,
             Output *   const Out,
             Stitcher * const Stitch,
             int        const row,
             int        const y) {

    /*
     *  We scale the overlap of the left and right images, we need to
     * discover and hold on to the left edge of the right image to
     * determine the rate at which we blend. Most (7/8) of the blending
     * occurs in the first half of the overlap to reduce the occurences
     * of blending artifacts. If there is no overlap, the image present
     * has no blending activity, this is determined by the black
     * background and is not through an alpha layer to help reduce
     * storage needs. The algorithm below is complicated most by
     * the blending determinations, overlapping a left untransformed
     * image with a right transformed image with a black background is
     * all that remains.
     */
    /*
     * Normalize transformation against origin, the
     * transformation algorithm was in reference to the right
     * hand side of the left hand image before.
     */
    tuple * const Row = (*(Out->Row))(Out,row);

    unsigned column, firstRight;

    firstRight = 0;  /* initial value */

    for (column = 0; column < Out->image->pam.width; ++column) {
        int right_row, right_column;

        right_row = -1;
        right_column = (*(Stitch->XRight))(Stitch, column,
                                           (y < 0) ? (row + y) : row) + 0.5;
        if ((0 <= right_column)
            && (right_column < Right->pam.width)) {
            right_row = (*(Stitch->YRight))(Stitch, column,
                                            (y < 0) ? (row + y) : row) + 0.5;
            if ((right_row < 0)
                || (Right->pam.height <= right_row)) {
                right_column = -1;
                right_row    = -1;
            }
        } else 
            right_column = -1;

        /* Create the pixel at column 'column' of row 'row' of the
           output 'Out': Row[column].
        */
        stitchOnePixel(Left, Right, Out->image->pam, row, column, y, 
                       right_row, right_column, &firstRight, Row[column]);
    }
}



static void 
stitchit(Image *      const Left, 
         Image *      const Right, 
         const char * const outfilename,
         const char * const filter,
         Stitcher *   const Stitch,
         int *        const retvalP) {

    Output * const Out = findObject(filter, &OutputMethods[0],
                                    sizeof(OutputMethods[0]));
    unsigned   row;
    int        xp[8], yp[8], x, y, width, height;
    
    if ((Out == (Output *)NULL) || (Out->Name == (char *)NULL)) 
        *retvalP = -2;
    else {
        if (verbose)
            fprintf (stderr, "Selected %s output filter algorithm\n",
                     Out->Name);

        /* Determine the mask corners for both Left and Right images */
        determineMaskCorners(Stitch, Left, Right, xp, yp);

        /* Output the combined images */
                
        /* Calculate generic x,y left top corner, and the width and height */
        calculateXyWidthHeight(xp, yp, &x, &y, &width, &height);
                
        if (verbose) 
            printPlan(xp, yp, Left, Right);
    
        if (!(*(Out->Alloc))(Out, outfilename, width, height, &Left->pam))
            *retvalP = -9;
        else {
            if (verbose) {
                fprintf (stderr,
                         "=%s[%u,%u=>%d,%d](%d,%d)\n",
                         Out->image->name, Out->image->pam.width,
                         Out->image->pam.height, width, height, x, y);
            }
            for (row = 0; row < Out->image->pam.height; row++) {
                /* Generate row number 'row' of the output image 'Out' */
                stitchOneRow(Left, Right, Out, Stitch, row, y);
                (*(Out->FlushRow))(Out,row);
            }
            (*(Out->FlushImage))(Out);
            (*(Out->DeAlloc))(Out);
        
            *retvalP = 0;
        }
    }
}



static int
pnmstitch(const char * const leftfilename,
          const char * const rightfilename,
          const char * const outfilename,
          int          const reqx,
          int          const reqy,
          int          const reqWidth,
          int          const reqHeight,
          const char * const stitcher,
          const char * const filter)
{
    Stitcher * const Stitch = findObject(stitcher, &StitcherMethods[0],
                                         sizeof(StitcherMethods[0]));
    Image    * Left;
    Image    * Right;
    int        retval;

    if ((Stitch == (Stitcher *)NULL) || (Stitch->Name == (char *)NULL)) 
        retval = -1;
    else {
        if (verbose) 
            fprintf (stderr, "Selected %s stitcher algorithm\n",
                     Stitch->Name);

        /* Left hand image read into memory */
        Left = readinit(leftfilename);
        if (Left == NULL)
            retval = -3;
        else {
            /* Right hand image read into memory */
            Right = readinit(rightfilename);
            if (Right == NULL)
                retval = -4;
            else {
                if (Left->pam.depth != Right->pam.depth) {
                    fprintf(stderr, "Images should have matching depth.  "
                            "The left image has depth %d, "
                            "while the right has depth %d.", 
                            Left->pam.depth, Right->pam.depth);
                    retval = -5;
                } else if (Left->pam.maxval != Right->pam.maxval) {
                    fprintf (stderr,
                             "Images should have matching maxval.  "
                             "The left image has maxval %u, "
                             "while the right has maxval %u.",
                             (unsigned)Left->pam.maxval, 
                             (unsigned)Right->pam.maxval);
                    retval = -6;
                } else if ((*(Stitch->Alloc))(Stitch) == FALSE) 
                    retval = -7;
                else {
                    (*(Stitch->Constrain))(Stitch, reqx, reqy, 
                                           reqWidth, reqHeight);

                    if ((*(Stitch->Match))(Stitch, Left, Right) == FALSE) 
                        retval = -8;
                    else 
                        stitchit(Left, Right, outfilename, filter, Stitch, 
                                 &retval);
                }
                free_image(Right);
            }
            free_image(Left);
        }
    }
    return retval;
}



/* Output Methods */

/* Helper methods */

static void
OutputDeAlloc(Output * me)
{
    if (me->image != (Image *)NULL) {
        /* Free up resources */
        free_image (me->image);
        me->image = (Image *)NULL;
    }
    if (me->extra != (void *)NULL) {
        free (me->extra);
        me->extra = (void *)NULL;
    }
} /* OutputDeAlloc() - end */

static bool
OutputAlloc(Output     * const me,
            const char * const file,
            unsigned int const width,
            unsigned int const height,
            struct pam * const prototype)
{
    /* Output the combined images */
    me->extra = (void *)NULL;
    me->image = allocate_image();
    if (me->image == (Image *)NULL) {
        return FALSE;
    }
    me->image->pam = *prototype;
    me->image->pam.width = width;
    me->image->pam.height = height;
    /* Give the output a name */
    me->image->name = strdup(file);
    /* Initialize output arrays */
    if (writeinit(me->image) == FALSE) {
        OutputDeAlloc(me);
        return FALSE;
    }
    return TRUE;
} /* OutputAlloc() - end */

/* StraightThrough output method */

static void
StraightThroughDeAlloc(Output * me)
{
    /* Trick the proper freeing of resouces on the Output Image */
    me->image->pam.height = 1;
    OutputDeAlloc(me);
} /* StraightThroughDeAlloc() - end */

static bool
StraightThroughAlloc(Output     * const me,
                     const char * const file,
                     unsigned int const width,
                     unsigned int const height,
                     struct pam * const prototype)
{
    if (OutputAlloc(me, file, width, height, prototype) == FALSE) {
        StraightThroughDeAlloc(me);
    }
    /* Trick the proper allocation of resouces on the Output Image */
    me->image->pam.height = 1;
    me->image->tuple = pnm_allocpamarray(&me->image->pam);
    if (me->image->tuple == (tuple **)NULL) {
        StraightThroughDeAlloc(me);
        return FALSE;
    }
    me->image->pam.height = height;
    pnm_writepaminit(&me->image->pam);
    return TRUE;
} /* StraightThroughAlloc() - end */

static tuple *
StraightThroughRow(Output * me, unsigned row)
{
    UNREFERENCED_PARAMETER(row);
    return me->image->tuple[0];
} /* StraightThroughRow() - end */

static void
StraightThroughFlushRow(Output * me, unsigned row)
{
    UNREFERENCED_PARAMETER(row);
    if (me->image != (Image *)NULL) {
        pnm_writepamrow(&me->image->pam, me->image->tuple[0]);
    }
} /* StraightThroughFlushRow() - end */

static void
StraightThroughFlushImage(Output * me)
{
    UNREFERENCED_PARAMETER(me);
} /* StraightThroughFlushImage() - end */

/* Horizontal Crop output method */

#define HorizontalCropDeAlloc StraightThroughDeAlloc

typedef struct {
    int state;
    int lostInSpace;
} HorizontalCropExtra;

static bool
HorizontalCropAlloc(Output     * const me,
                    const char * const file,
                    unsigned int const width,
                    unsigned int const height,
                    struct pam * const prototype)
{
    unsigned long pos;

    if (StraightThroughAlloc(me, file, width, height, prototype) == FALSE) {
        return FALSE;
    }
    me->extra = (void *)malloc(sizeof(HorizontalCropExtra));
    if (me->extra == (void *)NULL) {
        HorizontalCropDeAlloc(me);
        return FALSE;
    }
    memset (me->extra, 0, sizeof(HorizontalCropExtra));
    /* Test if we can seek, important since we rewrite the header */
    pos = ftell(me->image->pam.file);
    if ((fseek(me->image->pam.file, 1L, SEEK_SET) != 0)
     || (ftell(me->image->pam.file) != 1L)) {
        fprintf (stderr, "%s needs to output to a seekable entity\n",
          me->Name);
    }
    (void)fseek(me->image->pam.file, pos, SEEK_SET);
    return TRUE;
} /* HorizontalCropAlloc() - end */

#define HorizontalCropRow StraightThroughRow

static void
HorizontalCropFlushRow(Output * me, unsigned row)
{
    unsigned column;
    unsigned threshold;
#   define HorizontalCropThreshold 4
    UNREFERENCED_PARAMETER(row);

    if (me->image == (Image *)NULL) {
        return;
    }
    if (((HorizontalCropExtra *)(me->extra))->state == 2) {
        ((HorizontalCropExtra *)(me->extra))->lostInSpace++;
        return;
    }
    /* Any pitch black pixels? */
    threshold = HorizontalCropThreshold;
    for (column = 0; column < me->image->pam.width; ++column) {
        unsigned plane = 0;
        while (me->image->tuple[0][column][plane] == (sample)0) {
            if (++plane >= me->image->pam.depth) {
                if (--threshold == 0) {
                    if (((HorizontalCropExtra *)(me->extra))->state == 1) {
                        ((HorizontalCropExtra *)(me->extra))->state = 2;
                    }
                    ((HorizontalCropExtra *)(me->extra))->lostInSpace++;
                    return;
                }
            }
        }
        if (plane < me->image->pam.depth) {
            threshold = HorizontalCropThreshold;
        }
    }
    ((HorizontalCropExtra *)(me->extra))->state = 1;
    pnm_writepamrow(&me->image->pam, me->image->tuple[0]);
} /* HorizontalCropFlushRow() - end */

static void
HorizontalCropFlushImage(Output * me)
{
    me->image->pam.height -= ((HorizontalCropExtra *)(me->extra))->lostInSpace;
    if (verbose) {
        fprintf (stderr, "%s has set image size to %d x %d\n",
          me->Name, me->image->pam.width, me->image->pam.height);
    }
    if (fseek(me->image->pam.file, 0L, SEEK_SET) == 0) {
        pnm_writepaminit(&me->image->pam);
    } else {
        fprintf (stderr,
          "%s failed to seek to beginning to rewrite the header\n",
          me->Name);
    }
} /* HorizontalCropFlushImage() - end */

/* Rotate Crop output method */

#define RotateCropDeAlloc OutputDeAlloc

static bool
RotateCropAlloc(Output     * const me,
                const char * const file,
                unsigned int const width,
                unsigned int const height,
                struct pam * const prototype)
{
    if (OutputAlloc(me, file, width, height, prototype) == FALSE) {
        RotateCropDeAlloc(me);
    }
    me->image->tuple = pnm_allocpamarray(&me->image->pam);
    if (me->image->tuple == (tuple **)NULL) {
        RotateCropDeAlloc(me);
        return FALSE;
    }
    return TRUE;
} /* RotateCropAlloc() - end */

static tuple *
RotateCropRow(Output * me, unsigned row)
{
    return me->image->tuple[row];
} /* RotateCropRow() - end */

static void
RotateCropFlushRow(Output * me, unsigned row)
{
    UNREFERENCED_PARAMETER(me);
    UNREFERENCED_PARAMETER(row);
} /* RotateCropFlushRow() - end */

/*
 *  Algorithm under construction.
 *
 */
static void
RotateCropFlushImage(Output * me)
{
    /* Cop Out for now ... */
    pnm_writepam(&me->image->pam, me->image->tuple);
} /* RotateCropFlushImage() - end */

/* Output Method Table */

Output OutputMethods[] = {
    { "StraightThrough", StraightThroughAlloc, StraightThroughDeAlloc,
      StraightThroughRow, StraightThroughFlushRow,
      StraightThroughFlushImage },
    { "HorizontalCrop", HorizontalCropAlloc, HorizontalCropDeAlloc,
      HorizontalCropRow, HorizontalCropFlushRow, HorizontalCropFlushImage },
    { "RotateCrop (unimplemented)", RotateCropAlloc, RotateCropDeAlloc,
      RotateCropRow, RotateCropFlushRow, RotateCropFlushImage },
    { (char *)NULL }
};

/* Stitcher Methods */

/* These names are for the 8 parameters of a stitch, in any of the 3
   methods this program presently implements.  Each is a subscript in
   the parms[] array for the Stitcher object that represents a linear
   stitching method.  
   
   There are also other sets of names for the 8 parameters, such as
   Rotate_a.  I don't know why.  Maybe historical.
*/

#define Sliver_A   0
#define Sliver_B   1
#define Sliver_C   2
#define Sliver_D   3
#define Sliver_xp  4
#define Sliver_yp  5
#define Sliver_xpp 6
#define Sliver_ypp 7

/* Linear Stitcher Methods */

static void
LinearDeAlloc(Stitcher * me)
{
    if (me->parms != (float *)NULL) {
        free (me->parms);
        me->parms = (float *)NULL;
    }
} /* LinearDeAlloc() - end */

static bool
LinearAlloc(Stitcher * me)
{
    bool retval;

    MALLOCARRAY(me->parms, 8);
    if (me->parms == NULL) 
        retval = FALSE;
    else {
        /* Constraints unset */
        me->x = INT_MAX;
        me->y = INT_MAX;
        me->width = INT_MAX;
        me->height = INT_MAX;
        /* Unity transform matrix */
        me->parms[Sliver_A]   = 1.0;
        me->parms[Sliver_B]   = 0.0;
        me->parms[Sliver_C]   = 0.0;
        me->parms[Sliver_D]   = 0.0;
        me->parms[Sliver_xp]  = 0.0;
        me->parms[Sliver_yp]  = 1.0;
        me->parms[Sliver_xpp] = 0.0;
        me->parms[Sliver_ypp] = 0.0;
        retval = TRUE;
    }
    return retval;
}



static void
LinearConstrain(Stitcher * me, int x, int y, int width, int height)
{
    me->x = x;
    me->y = y;
    me->width = width;
    me->height = height;
} /* LinearConstrain() - end */

/*
 *  First pass is to find an approximate match. To do so, we take a
 *  width sliver of the left hand side of the right image and compare
 *  the sample to the left hand image. Accuracy is honored over speed.
 *  The image overlap is expected between 7/16 to 1/16 in the horizontal
 *  position, and a minumum of 5/8 in the vertical dimension.
 *
 *  Blind alleys:
 *      - reduced resolution can match in totally wrong regions,
 *        as such it can not be used to improve the speed by
 *        getting close, then fine tuning at full resolution.
 *      - vector (color) average of sample matched to running
 *        vector average on left image in an attempt to improve
 *        positional accuracy of a reduced resolution image
 *        produced even more artifacts.
 *      - A complete boxed sliver did not find a minima, as for
 *        too large or too small of a square sample. heuristics
 *        show that it works between 1/128 to 1/16 of the total
 *        image dimension. Smaller, of course, improves speed,
 *        but has the possibility of less accuracy.
 *
 *  Transformation parameters
 *      x=x.+a
 *      y=y'+b
 *  Where x,y represents the original point, and x.,y.
 *  represents the transformed point. Thus:
 *
 * Transformed image:
 * ((x'+x")/2,(y'+y"-H)/2)               ((x'+x"+2W)/2,(y'+y"-H)/2)
 * ((x'+x")/2,(y'+y"+H)/2)               ((x'+x"+2W)/2,(y'+y"+H)/2
 *
 * Corresponding to Original (dot) image:
 * (0,0)                 (Right->pam.width,0)
 * (0,Right->pam.height) (Right->pam.width,Right->pam.height)
 *
 *  Our matching data points are centered on x.=width/2, and
 * scan for transformation results with a variety of y. values:
 *  x=a*width/2+by.+c*width*y./2+d
 *  y=e*width/2+fy.+g*width*y./2+h
 * we set:
 *  A=b+c*width/2
 *  B=a*width/2+d
 *  C=f+g*width/2
 *  D=e*width/2+h
 * thus simplifying to:
 *  x=Ay.+B
 *  y=Cy.+D
 * adding in a weighting factor of w, the error equation is:
 *   2                       2           2
 *  E(A,B,C,D)=w * ((Ay.+B-x) + (Cy.+D-y))
 * thus
 *    2
 *  dE(A)=2wy.(Ay.+B-x) => 0=A{wy.y. + B{wy. - {wy.x
 *    2
 *      dE(B)=2w(Ay.+B-x)   => 0=A{wy.   + B{w   - {wx
 *      A=({wy.x{w-{wx{wy.)/({wy.y.{w-{wy.{wy.)
 *      B=({wx-A{wy.)/{w
 * and
 *    2
 *      dE(C)=2wy.(Cy.+D-y) => 0=C{wy.y. + D{wy. - {wy.y
 *    2
 *      dE(D)=2w(Cy.+D-y)   => 0=C{wy.   + D{w   - {wy
 *      C=({wy.y{w-{wy{wy.)/({wy.y.{w-{wy.{wy.)
 *      D=({wy-C{wy.)/{w
 * requiring us to collect:
 *   {wy.x=sumydotx
 *     {wx=sumx
 *        {wy.=sumydot
 *      {wy.y.=sumydotydot
 *          {w=sum
 *       {wy.y=sumydoty
 *         {wy=sumy
 * Once we have A, B, C and D, we can calculate the x',y' and x",y"
 * values as follows (based on geometric interpolation from the above
 * constraints):
 *  x'=AH/2+B-AHW/(2W-width)
 *  y'=CH/2+D-CHW/(2W-width)
 *  x"=AH/2+B+AHW/(2W-width)
 *  y"=CH/2+D+CHW/(2W-width)
 * These two points can be used either in the Linear or the BiLinear to
 * establish a transform.
 */

#define IMAGE_PORTION 64
#define SKIP_SLIVER   1

/* Following global variables are for use by SliverMatch() */
static unsigned long starPeriod;
    /* The number of events between printing of a "*" progress
       indicator.  
    */
static unsigned long starCount;
    /* The number of events until the next * progress indicator needs to be
       printed.
    */

static void
starEvent() {
    
    if (--starCount == 0) {
        starCount = starPeriod;
        fprintf (stderr, "*");
    }
}


static void
starInit(unsigned long const period) {
    starPeriod = period;
    starCount = period;
}


static void
starResetPeriod(unsigned long const period) {
    starPeriod = period;
    if (starCount > period)
        starCount = period;
}

static void
findBestMatches(Image *  const Left,
                Image *  const Right,
                int      const x,
                int      const y,
                int      const width,
                int      const height,
                int      const offY,
                unsigned const Xmin,
                unsigned const Xmax,
                int      const Ymin,
                int      const Ymax,
                Best           best[NUM_BEST]) { 
/*----------------------------------------------------------------------------
  Compare the rectangle 'width' columns by 'height' rows with upper
  left corner at Column 'x', Row y+offY in image 'Right' to a bunch of
  rectangles of the same size in image 'Left' and generate a list of the
  rectangles in 'Left' that best match the one in 'Right'.

  The specific rectangles in 'Left' we examine are those with upper left
  corner (X,Y+offY) where X is in [Xmin, Xmax) and Y is in [Ymin, Ymax).

  We return the ordered list of best matches as best[].

  Caller must ensure that each of the rectangles in question is fully
  contained with its respective image.
-----------------------------------------------------------------------------*/
    unsigned X, Y;
    /* Exhaustively find the best match */
    for (X = Xmin; X < Xmax; ++X) {
        int const widthOfOverlap = X - Left->pam.width;
        for (Y = Ymin; Y < Ymax; ++Y) {
            unsigned long difference = regionDifference(
                Left, X, Y + offY,
                Right, x, y + offY,
                width, height);
            update_best(best, difference, widthOfOverlap, Y + offY);
            starEvent();
        }
    }
}



static void
allocate_best_array(Best *** const bestP, unsigned const bestSize) {

    Best ** best;
    unsigned int i;

    MALLOCARRAY(best, bestSize);
    if (best == NULL)
        pm_error("No memory for Best array");
    
    for (i = 0; i < bestSize; ++i) 
        best[i] = allocate_best();
    *bestP = best;
}



static void determineXYRange(Stitcher * const me,
                             Image *    const Left,
                             Image *    const Right,
                             unsigned * const XminP,
                             unsigned * const XmaxP,
                             int *      const YminP,
                             int *      const YmaxP) {
    
    if (me->x == INT_MAX) {
        *XmaxP = Left->pam.width - me->width;
        /* I can't bring myself to go half way */
        *XminP = Left->pam.width - (7 * Right->pam.width / 16);
    } else {
        *XminP = me->x;
        *XmaxP = me->x + 1;
    }
    if (me->y == INT_MAX) {
        /* Middle 1/4 */
        *YminP = Left->pam.height * 3 / 8;
        *YmaxP = Left->pam.height - (*YminP) - me->height;
    } else {
        *YminP = me->y;
        *YmaxP = me->y + 1;
    }
    if (verbose) 
        pm_message("Test %d<x<%d %d<y<%d", *XminP, *XmaxP, *YminP, *YmaxP);
}



/*
 *  Find the weighted best line fit using the left hand margin of the
 * right hand image.
 */
static bool
SliverMatch(Stitcher * me, Image * Left, Image * Right,
            unsigned image_portion, unsigned skip_sliver)
{
    /* up/down 3/10, make sure has an odd number of members */
    unsigned const bestSize = 
        1 + 2 * ((image_portion * 3) / (10 * skip_sliver));
    Best       ** best; /* malloc'ed array of Best * */
    float         sumydotx, sumx, sumydot, sum;
    float         sumydoty, sumy, sumydotydot;
    int           yDiff;
    unsigned      X, Xmin, Xmax, num, xmin, xmax;
    int           x, y, Y, Ymin, Ymax, in, ymin, ymax;

    /* Harry Sticks Geeses */
    if (me->width == INT_MAX) {
        me->width = Right->pam.width / image_portion;
    }
    if ((me->width > (Right->pam.width/2))
     || (me->width > (Left->pam.width/2))) {
        pm_error ("stitch sample too wide %d\n", me->width);
        /* NOTREACHED */
    }
    if (me->height == INT_MAX) {
        me->height = Right->pam.height / image_portion;
    }
    if ((me->height > Right->pam.height)
     || (me->height > Left->pam.height)) {
        pm_error ("stitch sample too high %d\n", me->height);
        /* NOTREACHED */
    }
    yDiff = (Right->pam.height * skip_sliver) / image_portion;
    starInit((unsigned long)-1L);

    allocate_best_array(&best, bestSize);

    determineXYRange(me, Left, Right, &Xmin, &Xmax, &Ymin, &Ymax);

    /* Find the best */
    if ((verbose == 1) || (verbose == 2)) {
        fprintf (stderr, "%79s|\r|", "");
        starInit((unsigned long)
                 (
                     (unsigned long)(Xmax - Xmin)
                     * (unsigned long)(Ymax - Ymin)
                     ) * (unsigned long)bestSize
                 / 78L);
    }

    /* A point in the middle of the right image */
    x = 0;
    y = (Right->pam.height - me->height) / 2;
    /*
     *  Exhaustively search for the best match, improvements
     * in the algorithm here, if any, would give us the best
     * bang for the buck when it comes to improving performance.
         */
        /*
         *  First pass through the right hand images to determine
         * which are good candidate (top 90 percentile) for content of
         * features that we may have a chance of testing with.
         */
    {
        float   minf, maxf;
        float * features;

        MALLOCARRAY(features, bestSize);
        minf = maxf = 0.0;
        for (in = 0; in < bestSize; ++in) {
            int const offY = yDiff * (in - (bestSize/2));
            float SUM[3], SUMSQ[3];
            int plane;
            for (plane = 0; plane < MIN(Right->pam.depth,3); ++plane) {
                SUM[plane] = SUMSQ[plane] = 0.0;
            }
            for (X = x; X < (x + me->width); ++X) {
                for (Y = y + offY; Y < (y + offY + me->height); ++Y) {
                    for (plane = 0; 
                         plane < MIN(Right->pam.depth,3); 
                         ++plane) {
                        sample point = Right->tuple[Y][X][plane];
                        SUM[plane]   += point;
                        SUMSQ[plane] += point * point;
                    }
                }
            }
            /* How many features */
            features[in] = 0.0;
            for (plane = 0; plane < MIN(Right->pam.depth,3); ++plane) {
                features[in] += SUMSQ[plane] - 
                    (SUM[plane]*SUM[plane]/(float)(me->width*me->height));
            }
            if ((minf == 0.0) || (features[in] < minf)) {
                minf = features[in];
            }
            if ((maxf == 0.0) || (features[in] > maxf)) {
                maxf = features[in];
            }
        }
        /* Select 90% in the contrast range */
        minf = (minf + maxf) / 10;
        for (in = 0; in < bestSize; ++in) {
            if (features[in] < minf) {
                free_best(best[in]);
                best[in] = (Best *)NULL;
            }
        }
    }
    /* Loop through the constraints to find the best match */
    sumydotx=sumx=sumydot=sumydotydot=sum=sumydoty=sumy=0.0;
    xmin = UINT_MAX;
    xmax = 0;
    ymin = INT_MAX;
    ymax = INT_MIN;
    in = num = 0;
    for (;;) {
        float w;
        int offY = yDiff * (in - (bestSize/2));
        /* See if this one to be skipped because of too few features */
        if (best[in] == (Best *)NULL) {
            if (in > (bestSize/2)) {
                in = bestSize - in;
            } else if (in < (bestSize/2)) {
                in = (bestSize-1) - in;
            } else {
                break;
            }
            if ((verbose == 1) || (verbose == 2))
                for (X = Xmin; X < Xmax; ++X) {
                    for (Y = Ymin; Y < Ymax; ++Y) 
                        starEvent();
                }
            continue;
        }
        findBestMatches(Left, Right, x, y, me->width, me->height, 
                        offY, Xmin, Xmax, Ymin, Ymax,
                        best[in]);
        /* slop (noise in NUM_BEST) */
        {
            float SUMx, SUMxx, SUMy, SUMyy, SUMw;
            unsigned i;
            SUMx = SUMxx = SUMy = SUMyy = SUMw = 0.0;
            for (i = 0; i < NUM_BEST; ++i) {
                /* best[in][i] describes the ith closest region in the right
                   image to the region in the left image whose top corner is
                   at (x, y+offY).
                */
                float const w2 = (best[in][i].total > 0)
                    ? (1.0 / (float)best[in][i].total)
                    : 1.0;
                SUMx  += w2 * best[in][i].x;
                SUMy  += w2 * best[in][i].y;
                SUMxx += w2 * (best[in][i].x * best[in][i].x);
                SUMyy += w2 * (best[in][i].y * best[in][i].y);
                SUMw  += w2;
            }
            /* Find our weighted error */
            w = SUMw
                / ((SUMxx - (SUMx*SUMx)/SUMw) + (SUMyy - (SUMy*SUMy)/SUMw));
        }
        /* magnify slop */
        w *= w;
        Y = y + offY;
        sumy        += w * best[in][0].y;
        sumx        += w * best[in][0].x;
        sum         += w;
        sumydot     += w * Y;
        sumydotydot += w * Y * Y;
        sumydoty    += w * Y * best[in][0].y;
        sumydotx    += w * Y * best[in][0].x;
        /* Calculate the best fit line for these matches */
        me->parms[Sliver_C] = ((sumydotydot * sum)
                               - (sumydot * sumydot));
        if (me->parms[Sliver_C] == 0.0) {
            me->parms[Sliver_A] = 0.0;
        } else {
            me->parms[Sliver_A] = ((sumydotx * sum)
                                   - (sumx * sumydot))
                / me->parms[Sliver_C];
            me->parms[Sliver_C] = ((sumydoty * sum)
                                   - (sumy * sumydot))
                / me->parms[Sliver_C];
        }
        if (sum == 0.0) {
            me->parms[Sliver_B] = me->parms[Sliver_D] = 0;
        } else {
            me->parms[Sliver_B] = (sumx
                                   - (me->parms[Sliver_A] * sumydot))
                / sum;
            me->parms[Sliver_D] = (sumy
                                   - (me->parms[Sliver_C] * sumydot))
                / sum;
        }
        if (verbose > 2) {
            fprintf (stderr, "%.4g*(%d,%d)@(%d,%d)\n",
                     w, best[in][0].x + Left->pam.width, best[in][0].y,
                     me->width / 2, Y);
        }
        /* Record history of limits */
        if ((best[in][0].x + Left->pam.width) < xmin) {
            xmin = best[in][0].x + Left->pam.width;
        }
        if (xmax < (best[in][0].x + Left->pam.width)) {
            xmax = best[in][0].x + Left->pam.width;
        }
        if ((best[in][0].y - offY) < ymin) {
            ymin = best[in][0].y - offY;
        }
        if (ymax < (best[in][0].y - offY)) {
            ymax = best[in][0].y - offY;
        }
        /* Lets restrict the search a bit now */
        if (++num > 1) {
            if (me->x == INT_MAX) {
                int newXmin, newXmax, hold;
                /* Use the formula to determine the bounds */
                newXmin = (int)(me->parms[Sliver_B] + 0.5)
                    + Left->pam.width;
                newXmax = (int)((me->parms[Sliver_A]
                                 * (float)Left->pam.height)
                                + me->parms[Sliver_B] + 0.5)
                    + Left->pam.width;
                if (newXmax < newXmin) {
                    hold = newXmin;
                    newXmin = newXmax;
                    newXmax = hold;
                }
                /* Trust little ... */
                hold = (3 * newXmin - newXmax) / 2;
                newXmax = (3 * newXmax - newXmin) / 2;
                newXmin = hold;
                /* Don't go inside history */
                if (xmin < newXmin) {
                    newXmin = xmin + Left->pam.width;
                }
                if (newXmax < xmax) {
                    newXmax = xmax + Left->pam.width;
                }
                /* If it is `wacky' drop it */
                if ((newXmax - Xmax) < (Right->pam.width / 3)) {
                    /* Now upgrade new minimum and maximum */
                    hold = Xmin;
                    if ((Xmin < newXmin) && (newXmin < Xmax)) {
                        hold = newXmin;
                    }
                    if ((Xmin < newXmax) && (newXmax < Xmax)) {
                        Xmax = newXmax;
                    }
                    Xmin = hold;
                }
            }
            if (me->y == INT_MAX) {
                int newYmin, newYmax, hold;
                float tmp;
                /* Use the formula to determine the bounds */
                newYmin = tmp = me->parms[Sliver_D]
                    + ((float)(Left->pam.height + 1))
                    / 2;
                newYmax = (int)((me->parms[Sliver_C]
                                 * (float)Left->pam.height)
                                + tmp) - Left->pam.height;
                if (newYmax < newYmin) {
                    hold = newYmin;
                    newYmin = newYmax;
                    newYmax = hold;
                }
                /* Trust little ... */
                hold = (3 * newYmin - newYmax) / 2;
                newYmax = (3 * newYmax - newYmin) / 2;
                newYmin = hold;
                /* Don't go inside history */
                if (ymin < newYmin) {
                    newYmin = ymin;
                }
                if (newYmax < ymax) {
                    newYmax = ymax;
                }
                /* Now upgrade new minimum and maximum */
                hold = Ymin;
                if ((Ymin < newYmin) && (newYmin < Ymax)) {
                    hold = newYmin;
                }
                if ((Ymin < newYmax) && (newYmax < Ymax)) {
                    Ymax = newYmax;
                }
                Ymin = hold;
            }
            if ((verbose == 1) || (verbose == 2)) {
                starResetPeriod((unsigned long)(
                    ((unsigned long)(Xmax - Xmin)
                    * (unsigned long)(Ymax - Ymin))
                                * (unsigned long)bestSize
                                / 78L));
            }
        }
        if (in > (bestSize/2)) {
            in = bestSize - in;
        } else if (in < (bestSize/2)) {
            in = (bestSize-1) - in;
        } else {
            break;
        }
    }
    if ((verbose == 1) || (verbose == 2)) {
        fprintf (stderr, "\n");
    }
    if (verbose > 2) {
        fprintf (stderr, "Up  ");
        pr_best(best[bestSize-1]);
        fprintf (stderr, "\nMid ");
        pr_best(best[bestSize/2]);
        fprintf (stderr, "\nDown");
        pr_best(best[0]);
        fprintf (stderr, "\n");
    }

    if (verbose) {
        if (verbose > 1) {
            fprintf (stderr,
                     "[y=%.4g [x=%.4g [=%.4g [y.=%.4g "
                     "[y.y.=%.4g [y.y=%.4g [y.x=%.4g\n",
                     sumy, sumx, sum, sumydot, sumydotydot,
                     sumydoty, sumydotx);
        }
        fprintf (stderr, "x=%.4gY%+.4g\ny=%.4gY%+.4g\n",
                 me->parms[Sliver_A], me->parms[Sliver_B],
                 me->parms[Sliver_C], me->parms[Sliver_D]);
    }
    /*
         *  Free up resources
         */
    for (in = 0; in < bestSize; ++in) {
        if (best[in] != (Best *)NULL) {
            free_best (best[in]);
            best[in] = (Best *)NULL;
        }
    }
    free(best);

        /* Calculate x',y' and x",y" from best fit line formula */
    sum = (float)(Right->pam.width * Right->pam.height)
        / (float)(2 * Right->pam.width - me->width);
    me->parms[Sliver_xpp] = me->parms[Sliver_A] * sum;
    me->parms[Sliver_ypp] = me->parms[Sliver_C] * sum;
    sumx = me->parms[Sliver_A] * (Right->pam.height / 2)
        + me->parms[Sliver_B];
    sumx += Left->pam.width;
    sumy = me->parms[Sliver_C] * (Right->pam.height / 2)
        + me->parms[Sliver_D];
    me->parms[Sliver_xp] = sumx - me->parms[Sliver_xpp];
    me->parms[Sliver_yp] = sumy - me->parms[Sliver_ypp];
    me->parms[Sliver_xpp] += sumx;
    me->parms[Sliver_ypp] += sumy;
    if (verbose > 1) {
        fprintf (stderr, "x',y'=%.4g,%.4g x\",y\"=%.4g,%.4g\n",
                 me->parms[Sliver_xp], me->parms[Sliver_yp],
                 me->parms[Sliver_xpp], me->parms[Sliver_ypp]);
    }
    return TRUE;
} /* SliverMatch() - end */

/* These are not used.  Perhaps they are forerunners of the more
   expressive Sliver_A, etc. names.
*/
#define Linear_a   0
#define Linear_b   1
#define Linear_c   2
#define Linear_d   3
#define Linear_e   4
#define Linear_f   5
#define Linear_g   6
#define Linear_h   7

static bool
LinearMatch(Stitcher * me, Image * Left, Image * Right)
{
    if (SliverMatch(me, Left, Right, IMAGE_PORTION, SKIP_SLIVER * 8) 
        == FALSE) {
        return FALSE;
    }

    me->x = - (me->parms[Sliver_xp] + me->parms[Sliver_xpp] + 1) / 2;
    me->y = - (me->parms[Sliver_yp] + me->parms[Sliver_ypp]
          + (1 - Left->pam.height)) / 2;

    if (verbose) 
        pm_message("LinearMatch translation parameters are (%d,%d)",
                   me->x, me->y);

    return TRUE;
} /* LinearMatch() - end */

/*
 *  Transformation parameters
 *      left  x' = x
 *      left  y' = y
 *      right x' = x + me->x
 *      right y' = y + me->y
 */
static float
LinearXLeft(Stitcher * me, int x, int y)
{
    UNREFERENCED_PARAMETER(y);
    return x;
} /* LinearXLeft() - end */

static float
LinearYLeft(Stitcher * me, int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    return y;
} /* LinearYLeft() - end */

static float
LinearXRight(Stitcher * me, int x, int y)
{
    UNREFERENCED_PARAMETER(y);
    return (x + me->x);
} /* LinearXRight() - end */

static float
LinearYRight(Stitcher * me, int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    return (y + me->y);
} /* LinearYRight() - end */

static void
LinearOutput(Stitcher * me, FILE * fp)
{
    fprintf (fp, "x'=x%+d\ny'=y%+d\n", me->x, me->y);
} /* LinearOutput() - end */

/* BiLinear Stitcher Methods */

static void
BiLinearDeAlloc(Stitcher * me)
{
    LinearDeAlloc(me);
}



static bool
BiLinearAlloc(Stitcher * me)
{
    return LinearAlloc(me);
}



static void
BiLinearConstrain(Stitcher * me, int x, int y, int width, int height)
{
    LinearConstrain(me, x, y, width, height);
    if (x != INT_MAX) {
        me->parms[3] -= x;
    }
    if (y != INT_MAX) {
        me->parms[7] -= y;
    }
} /* BiLinearConstrain() - end */

/*
 *  First pass is to find an approximate match. To do so, we take a
 *  width sliver of the left hand side of the right image and compare
 *  the sample to the left hand image. Accuracy is honored over speed.
 *  The image overlap is expected between 7/16 to 1/16 in the horizontal
 *  position, and a minumum of 5/8 in the vertical dimension.
 *
 *  Blind alleys:
 *      - Tried a simpler constraint for right side to be `back'
 *        to image, twisted too much sometimes:
 *         . . .
 *         W=aW+bH+cWH+d
 *         H=eW+fH+gWH+h
 *         W=aW+d
 *         0=eW+h
 *        Solve the equations resulted in:
 *         a = W/(W-x") - cy"
 *         b = -Wc
 *         c = W/((x"-W)(y'-y"))
 *         d = (1-a)W
 *         e = y'(y"x'+W-Hx'-x"y")/(x'y"x"-Wx'y"-Wy'x"-WWy'+x'y'x"-Wx'y'-W)
 *         f = 1 - Wg
 *         g = (e + (H-y")/(x"-W))/y"
 *         h = -We
 *        Results left here for historical reasons.
 *
 *  Transformation parameters
 *      x=ax.+by.+cx.y.+d
 *      y=ex.+fy.+gx.y.+h
 *  Where x,y represents the original point, and x.,y.
 *  represents the transformed point. Thus:
 *
 * Transformed image:
 * (x',y')               (x'",y'")
 * (x",y")               (x"",y"")
 *
 * Corresponding to Original (dot) image:
 * (0,0)                 (Right->pam.width,0)
 * (0,Right->pam.height) (Right->pam.width,Right->pam.height)
 *
 * Define:
 *      H=Right->pam.height
 *      W=Right->pam.width
 * Given that I want a flat presentation that both reduces the distortion
 * necessary on an image, reduces the cropping losses, and flattens out the
 * spherical or orbit distortions; it was chosen to constrain the right side
 * in the middle horizontal, and pivot the left side in that middle (hopefully
 * minimally) and to allow the image only vertical and horizontal location
 * placement. Rotating the entire image could increase cropping losses
 * especially if the focus was not down the center of the image on a
 * graduated field causing the distortion to accumulate in subsequent
 * images. Trapezoidal would cause the distortion to accumulate in subsequent
 * images as well, resetting to `square' gradually towards the right would
 * allow the next image to restart a match placing the distortions mainly
 * in the stitching zone where averaging and the slight expectation of
 * artifacts would minimize the effects. These constraints can be explained
 * mathematically as the following:
 *  x'" + x"" - 2W = x' + x"
 *       y'" + y"" = y' + y"
 *                 x'" = x""
 *         y'" + H = y""
 * resulting in the right side of the image being completely explained by the
 * placement of the left hand side:
 *  x'"=(x'+x"+2W)/2
 *  y'"=(y'+y"-H)/2
 *  x""=(x'+x"+2W)/2
 *  y""=(y'+y"+H)/2
 *
 * Describing the `X' polygon using geometry and ratios:
 *  X=A(y-(y'+y")/2)(x-(x'+x"+2W)/2) + (x - (x'+x")/2))
 *    A=2(x"-x')/((y'-y")(x'-x"-2W))
 *  a=2(y'(x'-x")-W(y'-y"))/((y'-y")(x'-x"-2W))
 *  b=(x'-x")(x'+x"+2W)/((y'-y")(x'-x"-2W))
 *  c=2(x"-x')/((y'-y")(x'-x"-2W))
 *  d=(2W(x"y'-x'y")+y'(x"x"-x'x'))/((y'-y")(x'-x"-2W))
 *
 * Describing the `Y' polygon using geometry and ratios (note use of X rather
 * than x, this has the effect of linearalizing the polygon).
 *  Y=((y'-y"+H)/W(y'-y"))(y-(y'+y")/2)(X-HW/(y'-y"+H)) + H/2
 *  e=(y'+y")(y'-y"+H)/2W(y"-y')
 *  f=H/(y"-y')
 *  g=(y'-y"+H)/W(y'-y")
 *  h=Hy'/(y'-y")
 *
 * FYI: Reverse transform using the same formula style is:
 *  a=(x"-x'+2W)/2W
 *  b=(x"-x')/H
 *  c=(x'-x")/WH
 *  d=x'
 *  e=(y"-y'-H)/2W
 *  f=(y"-y')/H
 *  g=(y'-y"+H)/WH
 *  h=y'
 */

#define BiLinear_a   0
#define BiLinear_b   1
#define BiLinear_c   2
#define BiLinear_d   3
#define BiLinear_e   4
#define BiLinear_f   5
#define BiLinear_g   6
#define BiLinear_h   7

static bool
BiLinearMatch(Stitcher * me, Image * Left, Image * Right)
{
    float xp, yp, xpp, ypp;

    if (SliverMatch(me, Left, Right, IMAGE_PORTION, SKIP_SLIVER) == FALSE) {
        return FALSE;
    }
    /* If too wacky, flatten out */
    xp  = me->parms[Sliver_xp];
    yp  = me->parms[Sliver_yp];
    xpp = me->parms[Sliver_xpp];
    ypp = me->parms[Sliver_ypp];
    if ((me->parms[Sliver_A] < -0.3)
     || (0.3 < me->parms[Sliver_A])) {
        xp = xpp = (xp + xpp) / 2;
    }
    if ((me->parms[Sliver_C] < 0.6)
     || (1.5 < me->parms[Sliver_D])) {
        yp = (yp + ypp - (float)Right->pam.height) / 2;
        ypp = yp + Right->pam.height;
    }

    /*
     *  Calculate any necessary transformations on the
     * right image to improve the stitching match. We have Done a
     * weighted best fit line on the points we have collected
     * thus far, now translate this to the constrained
     * transformation equations.
     */
    /* a = y"-y' */
    me->parms[BiLinear_a] = ypp-yp;
    /* c = x'-x" */
    me->parms[BiLinear_c] = xp-xpp;
    /* d = (y"-y')(x"-x'+2W) = (y'-y")(x'-x"-2W) */
    me->parms[BiLinear_d] = me->parms[BiLinear_a]
                          * ((float)
                             (2*Right->pam.width)-me->parms[BiLinear_c]);
    /* a = 2(y'(x'-x")+W(y"-y'))/((y'-y")(x'-x"-2W)) */
    me->parms[BiLinear_a] = 2*(yp*me->parms[BiLinear_c]
                          + me->parms[BiLinear_a]*(float)(Right->pam.width))
                          / me->parms[BiLinear_d];
    /* b = (x'-x")(x'+x"+2W)/((y'-y")(x'-x"-2W)) */
    me->parms[BiLinear_b] = me->parms[BiLinear_c]
                          * (xp+xpp+(float)(2*Right->pam.width))
                          / me->parms[BiLinear_d];
    /* c = -2(x'-x")/((y'-y")(x'-x"-2W)) */
    me->parms[BiLinear_c]*= -2/me->parms[BiLinear_d];
    /* d = (2W(x"y'-x'y")+y'(x"x"-x'x'))/((y'-y")(x'-x"-2W)) */
    me->parms[BiLinear_d] = ((xpp*yp-xp*ypp)*(float)(2*Right->pam.width)
                          + yp*(xpp*xpp-xp*xp))
                          / me->parms[BiLinear_d];

    /* f = y"-y' */
    me->parms[BiLinear_f] = ypp-yp;
    /* g = (y"-y'-H)/W(y"-y') */
    me->parms[BiLinear_g] = (me->parms[BiLinear_f]-(float)Right->pam.height)
                          / me->parms[BiLinear_f]
                          / (float)Right->pam.width;
    /* e = (y'+y")(y'-y"+H)/2W(y"-y') = -g(y'+y")/2 */
    me->parms[BiLinear_e] = (yp+ypp)*me->parms[BiLinear_g]/-2;
    /* f=H/(y"-y') */
    me->parms[BiLinear_f] = ((float)Right->pam.height)
                          / me->parms[BiLinear_f];
    /* h = Hy'/(y'-y") = -fy' */
    me->parms[BiLinear_h] = -yp*me->parms[BiLinear_f];

    return TRUE;
} /* BiLinearMatch() - end */

/*
 *  Transformation parameters
 *      x`=x
 *      y`=y
 */
#define BiLinearXLeft LinearXLeft
#define BiLinearYLeft LinearYLeft

/*
 *  Transformation parameters
 *      x`=ax+by+cxy+d
 *      y`=ex`+fy+gx`y+h
 */
static float
BiLinearXRight(Stitcher * me, int x, int y)
{
    return (me->parms[BiLinear_a] * x) + (me->parms[BiLinear_b] * y)
         + (me->parms[BiLinear_c] * (x * y)) + me->parms[BiLinear_d];
} /* BiLinearXRight() - end */

static float
BiLinearYRight(Stitcher * me, int x, int y)
{
    /* A little trick I learned from a biker */
    float X = BiLinearXRight(me, x, y);
    return (me->parms[BiLinear_e] * X) + (me->parms[BiLinear_f] * y)
         + (me->parms[BiLinear_g] * (X * y)) + me->parms[BiLinear_h];
} /* BiLinearYRight() - end */

static void
BiLinearOutput(Stitcher * me, FILE * fp)
{
    fprintf (fp,
      "x'=%.6gx%+.6gy%+.6gxy%+.6g\ny'=%.6gx'%+.6gy%+.6gx'y%+.6g\n",
      me->parms[BiLinear_a], me->parms[BiLinear_b], me->parms[BiLinear_c],
      me->parms[BiLinear_d], me->parms[BiLinear_e], me->parms[BiLinear_f],
      me->parms[BiLinear_g], me->parms[BiLinear_h]);
} /* BiLinearOutput() - end */

/* Rotate Stitcher Methods */

#define RotateDeAlloc   BiLinearDeAlloc

#define RotateAlloc     BiLinearAlloc

#define RotateConstrain BiLinearConstrain

/*
 *  First pass is to utilize the SliverMatch.
 *
 *  Transformation parameters
 *      x=ax.+by.+d
 *      y=ex.+fy.+h
 *  Where x,y represents the original point, and x.,y.
 *  represents the transformed point. Thus:
 *
 * Transformed image:
 * (x',y')               (x'",y'")
 * (x",y")               (x"",y"")
 *
 * Corresponding to Original (dot) image:
 * (0,0)                 (Right->pam.width,0)
 * (0,Right->pam.height) (Right->pam.width,Right->pam.height)
 *
 * Define:
 *      H=Right->pam.height
 *      W=Right->pam.width
 *
 */

#define Rotate_a   0
#define Rotate_b   1
#define Rotate_c   2
#define Rotate_d   3
#define Rotate_e   4
#define Rotate_f   5
#define Rotate_g   6
#define Rotate_h   7

static bool
RotateMatch(Stitcher * me, Image * Left, Image * Right)
{
    float xp, yp, xpp, ypp;

    if (SliverMatch(me, Left, Right, IMAGE_PORTION, SKIP_SLIVER) == FALSE) {
        return FALSE;
    }
    xp  = me->parms[Sliver_xp];
    yp  = me->parms[Sliver_yp];
    xpp = me->parms[Sliver_xpp];
    ypp = me->parms[Sliver_ypp];

    me->parms[Rotate_c] = (xp - xpp);
    me->parms[Rotate_c]*= me->parms[Rotate_c];
    me->parms[Rotate_g] = (yp - ypp);
    me->parms[Rotate_g]*= me->parms[Rotate_g];
    me->parms[Rotate_a] = me->parms[Rotate_f] = sqrt(me->parms[Rotate_g]
                                              / (me->parms[Rotate_c]
                                                + me->parms[Rotate_g]));
    me->parms[Rotate_b] = me->parms[Rotate_e] = sqrt(me->parms[Rotate_c]
                                              / (me->parms[Rotate_c]
                                                + me->parms[Rotate_g]));
    if (xp < xpp) {
        me->parms[Rotate_b] = -me->parms[Rotate_b];
    } else {
        me->parms[Rotate_e] = -me->parms[Rotate_e];
    }
    /* negative (for reverse transform below) xp & yp set for unity gain */
    xp = ((me->parms[Rotate_b] * (float)Right->pam.height) + xp + xpp) / -2;
    yp = ((me->parms[Rotate_f] * (float)Right->pam.height) - yp - ypp) / 2;
    me->parms[Rotate_d] = xp * me->parms[Rotate_a] + yp * me->parms[Rotate_b];
    me->parms[Rotate_h] = xp * me->parms[Rotate_e] + yp * me->parms[Rotate_f];
    return TRUE;
} /* RotateMatch() - end */

/*
 *  Transformation parameters
 *      x`=x
 *      y`=y
 */
#define RotateXLeft BiLinearXLeft
#define RotateYLeft BiLinearYLeft

/*
 *  Transformation parameters
 *      x`=ax+by+d
 *      y`=ex+fy+h
 */

static float
RotateXRight(Stitcher * me, int x, int y)
{
    return (me->parms[Rotate_a] * x) + (me->parms[Rotate_b] * y)
          + me->parms[Rotate_d];
} /* RotateXRight() - end */

static float
RotateYRight(Stitcher * me, int x, int y)
{
    return (me->parms[Rotate_e] * x) + (me->parms[Rotate_f] * y)
          + me->parms[Rotate_h];
} /* RotateYRight() - end */

static void
RotateOutput(Stitcher * me, FILE * fp)
{
    fprintf (fp,
      "x'=%.6gx%+.6gy%+.6g\ny'=%.6gx%+.6gy%+.6g\n",
      me->parms[Rotate_a], me->parms[Rotate_b], me->parms[Rotate_d],
      me->parms[Rotate_e], me->parms[Rotate_f], me->parms[Rotate_h]);
} /* RotateOutput() - end */

/* Stitcher Method Table */

Stitcher StitcherMethods[] = {
    { "RotateSliver", RotateAlloc, RotateDeAlloc, RotateConstrain,
      RotateMatch, RotateXLeft, RotateYLeft, RotateXRight,
      RotateYRight, RotateOutput },
    { "BiLinearSliver", BiLinearAlloc, BiLinearDeAlloc, BiLinearConstrain,
      BiLinearMatch, BiLinearXLeft, BiLinearYLeft, BiLinearXRight,
      BiLinearYRight, BiLinearOutput },
    { "LinearSliver", LinearAlloc, LinearDeAlloc, LinearConstrain,
      LinearMatch, LinearXLeft, LinearYLeft, LinearXRight,
      LinearYRight, LinearOutput },
    { (char *)NULL }
};
