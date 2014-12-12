/* pnmgamma.c - perform gamma correction on a PNM image
**
** Copyright (C) 1991 by Bill Davidson and Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <math.h>
#include <ctype.h>

#include "shhopt.h"
#include "mallocvar.h"
#include "pnm.h"

enum transferFunction {
    XF_EXP,
    XF_EXP_INVERSE,
    XF_BT709RAMP,
    XF_BT709RAMP_INVERSE,
    XF_SRGBRAMP,
    XF_SRGBRAMP_INVERSE,
    XF_BT709_TO_SRGB,
    XF_SRGB_TO_BT709
};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *filespec;  /* '-' if stdin */
    enum transferFunction transferFunction;
    float rgamma, ggamma, bgamma;
    unsigned int maxval;
    unsigned int makeNewMaxval;
};



static void
interpretOldArguments(int                  const argc,
                      char **              const argv,
                      float                const defaultGamma,
                      struct cmdlineInfo * const cmdlineP) {

    /* Use the old syntax wherein the gamma values come from arguments.
       If there is one argument, it's a gamma value for all three
       components.  If 3 arguments, it's separate gamma values.  If
       2, it's a single gamma value plus a file name.  If 4, it's
       separate gamma values plus a file name.
    */
    if (argc-1 == 0) {
        cmdlineP->rgamma = defaultGamma;
        cmdlineP->ggamma = defaultGamma;
        cmdlineP->bgamma = defaultGamma;
        cmdlineP->filespec = "-";
    } else if (argc-1 == 1) {
        cmdlineP->rgamma = atof(argv[1]);
        cmdlineP->ggamma = atof(argv[1]);
        cmdlineP->bgamma = atof(argv[1]);
        cmdlineP->filespec = "-";
    } else if (argc-1 == 2) {
        cmdlineP->rgamma = atof(argv[1]);
        cmdlineP->ggamma = atof(argv[1]);
        cmdlineP->bgamma = atof(argv[1]);
        cmdlineP->filespec = argv[2];
    } else if (argc-1 == 3) {
        cmdlineP->rgamma = atof(argv[1]);
        cmdlineP->ggamma = atof(argv[2]);
        cmdlineP->bgamma = atof(argv[3]);
        cmdlineP->filespec = "-";
    } else if (argc-1 == 4) {
        cmdlineP->rgamma = atof(argv[1]);
        cmdlineP->ggamma = atof(argv[2]);
        cmdlineP->bgamma = atof(argv[3]);
        cmdlineP->filespec = argv[4];
    } else 
        pm_error("Wrong number of arguments.  "
                 "You may have 0, 1, or 3 gamma values "
                 "plus zero or one filename");
        
    if (cmdlineP->rgamma <= 0.0 || 
        cmdlineP->ggamma <= 0.0 || 
        cmdlineP->bgamma <= 0.0 )
        pm_error("Invalid gamma value.  Must be positive floating point "
                 "number.");
}



static void
getGammaFromOpts(struct cmdlineInfo * const cmdlineP,
                 bool                 const gammaSpec,
                 float                const gammaOpt,
                 bool                 const rgammaSpec,
                 bool                 const ggammaSpec,
                 bool                 const bgammaSpec,
                 float                const defaultGamma) {

    if (gammaSpec)
        if (gammaOpt < 0.0)
            pm_error("Invalid gamma value.  "
                         "Must be positive floating point number.");
    
    if (rgammaSpec) {
        if (cmdlineP->rgamma < 0.0)
            pm_error("Invalid gamma value.  "
                     "Must be positive floating point number.");
    } else {
        if (gammaSpec)
            cmdlineP->rgamma = gammaOpt;
        else 
            cmdlineP->rgamma = defaultGamma;
    }
    if (ggammaSpec) {
        if (cmdlineP->ggamma < 0.0) 
            pm_error("Invalid gamma value.  "
                     "Must be positive floating point number.");
    } else {
        if (gammaSpec)
            cmdlineP->ggamma = gammaOpt;
        else 
            cmdlineP->ggamma = defaultGamma;
    }
    if (bgammaSpec) {
        if (cmdlineP->bgamma < 0.0)
            pm_error("Invalid gamma value.  "
                     "Must be positive floating point number.");
    } else {
        if (gammaSpec)
            cmdlineP->bgamma = gammaOpt;
        else
            cmdlineP->bgamma = defaultGamma;
    }
}



static void
parseCommandLine(int argc, char ** argv, 
                 struct cmdlineInfo * const cmdlineP) {

    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int bt709ramp, srgbramp, ungamma, bt709tosrgb, srgbtobt709;
    unsigned int bt709tolinear, lineartobt709;
    unsigned int gammaSpec, rgammaSpec, ggammaSpec, bgammaSpec;
    float gammaOpt;
    float defaultGamma;
    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "ungamma",       OPT_FLAG,   NULL,
            &ungamma,                 0);
    OPTENT3(0, "bt709tolinear", OPT_FLAG,   NULL,
            &bt709tolinear,           0);
    OPTENT3(0, "lineartobt709", OPT_FLAG,   NULL,
            &lineartobt709,           0);
    OPTENT3(0, "bt709ramp",     OPT_FLAG,   NULL,
            &bt709ramp,               0);
    OPTENT3(0, "cieramp",       OPT_FLAG,   NULL,
            &bt709ramp,               0);
    OPTENT3(0, "srgbramp",      OPT_FLAG,   NULL,
            &srgbramp,                0);
    OPTENT3(0, "bt709tosrgb",   OPT_FLAG,   NULL,
            &bt709tosrgb,             0);
    OPTENT3(0, "srgbtobt709",   OPT_FLAG,   NULL,
            &srgbtobt709,             0);
    OPTENT3(0, "maxval",        OPT_UINT,   &cmdlineP->maxval,
            &cmdlineP->makeNewMaxval, 0);
    OPTENT3(0, "gamma",         OPT_FLOAT,  &gammaOpt,
            &gammaSpec,               0);
    OPTENT3(0, "rgamma",        OPT_FLOAT,  &cmdlineP->rgamma,
            &rgammaSpec,              0);
    OPTENT3(0, "ggamma",        OPT_FLOAT,  &cmdlineP->ggamma,
            &ggammaSpec,              0);
    OPTENT3(0, "bgamma",        OPT_FLOAT,  &cmdlineP->bgamma,
            &bgammaSpec,              0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = TRUE; 

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (bt709tolinear + lineartobt709 + bt709ramp + srgbramp +
        bt709tosrgb + srgbtobt709 > 1)
        pm_error("You may specify only one function option");
    else {
        if (bt709tolinear) {
            if (ungamma)
                pm_error("You cannot specify -ungamma with -bt709tolinear");
            else
                cmdlineP->transferFunction = XF_BT709RAMP_INVERSE;
        } else if (lineartobt709) {
            if (ungamma)
                pm_error("You cannot specify -ungamma with -lineartobt709");
            else
                cmdlineP->transferFunction = XF_BT709RAMP;
        } else if (bt709tosrgb) {
            if (ungamma)
                pm_error("You cannot specify -ungamma with -bt709tosrgb");
            else
                cmdlineP->transferFunction = XF_BT709_TO_SRGB;
        } else if (srgbtobt709) {
            if (ungamma)
                pm_error("You cannot specify -ungamma with -srgbtobt709");
            else
                cmdlineP->transferFunction = XF_SRGB_TO_BT709;
        } else if (bt709ramp) {
            if (ungamma)
                cmdlineP->transferFunction = XF_BT709RAMP_INVERSE;
            else
                cmdlineP->transferFunction = XF_BT709RAMP;
        } else if (srgbramp) {
            if (ungamma)
                cmdlineP->transferFunction = XF_SRGBRAMP_INVERSE;
            else
                cmdlineP->transferFunction = XF_SRGBRAMP;
        } else {
            if (ungamma)
                cmdlineP->transferFunction = XF_EXP_INVERSE;
            else
                cmdlineP->transferFunction = XF_EXP;
        }
    }

    if (cmdlineP->makeNewMaxval) {
        if (cmdlineP->maxval > PNM_OVERALLMAXVAL)
            pm_error("Largest possible maxval is %u.  You specified %u",
                     PNM_OVERALLMAXVAL, cmdlineP->maxval);
    }

    switch (cmdlineP->transferFunction) {
    case XF_BT709RAMP:
    case XF_BT709RAMP_INVERSE:
    case XF_SRGB_TO_BT709:
        defaultGamma = 1.0/0.45;
        break;
    case XF_SRGBRAMP:
    case XF_SRGBRAMP_INVERSE:
    case XF_BT709_TO_SRGB:
        /* The whole function is often approximated with
           exponent 2.2 and no linear piece.  We do the linear
           piece, so we use the real exponent of 2.4.
        */
        defaultGamma = 2.4;
        break;
    case XF_EXP:
    case XF_EXP_INVERSE:
        defaultGamma = 2.2;
        break;
    }

    if (bt709tolinear || lineartobt709 || bt709tosrgb || srgbtobt709) {
        /* Use the new syntax wherein the gamma values come from options,
           not arguments.  So if there's an argument, it's a file name.
        */
        getGammaFromOpts(cmdlineP, gammaSpec, gammaOpt,
                         rgammaSpec, ggammaSpec, bgammaSpec, defaultGamma);

        if (argc-1 < 1)
            cmdlineP->filespec = "-";
        else {
            cmdlineP->filespec = argv[1];
            if (argc-1 > 1)
                pm_error("Too many arguments (%u).  With this function, there "
                         "is at most one argument:  the file name", argc-1);
        }
    } else {
        if (gammaSpec || rgammaSpec || ggammaSpec || bgammaSpec)
            pm_error("With this function, you specify the gamma values in "
                     "arguments, not with the -gamma, etc.");
        interpretOldArguments(argc, argv, defaultGamma, cmdlineP);
    }
}



static void
buildPowGamma(xelval       table[],
              xelval const maxval,
              xelval const newMaxval,
              double const gamma) {
/*----------------------------------------------------------------------------
   Build a gamma table of size maxval+1 for the given gamma value.
  
   This function depends on pow(3m).  If you don't have it, you can
   simulate it with '#define pow(x,y) exp((y)*log(x))' provided that
   you have the exponential function exp(3m) and the natural logarithm
   function log(3m).  I can't believe I actually remembered my log
   identities.
-----------------------------------------------------------------------------*/
    xelval i;
    double const oneOverGamma = 1.0 / gamma;

    for (i = 0 ; i <= maxval; ++i) {
        double const normalized = ((double) i) / maxval;
            /* Xel sample value normalized to 0..1 */
        double const v = pow(normalized, oneOverGamma);
        table[i] = MIN((xelval)(v * newMaxval + 0.5), newMaxval);  
            /* denormalize, round and clip */
    }
}



static void
buildBt709Gamma(xelval       table[],
                xelval const maxval,
                xelval const newMaxval,
                double const gamma) {
/*----------------------------------------------------------------------------
   Build a gamma table of size maxval+1 for the ITU Recommendation
   BT.709 gamma transfer function.

   'gamma' must be 1/0.45 for true Rec. 709.
-----------------------------------------------------------------------------*/
    double const oneOverGamma = 1.0 / gamma;
    xelval i;

    /* This transfer function is linear for sample values 0
       .. maxval*.018 and an exponential for larger sample values.
       The exponential is slightly stretched and translated, though,
       unlike the popular pure exponential gamma transfer function.
    */
    xelval const linearCutoff = (xelval) (maxval * 0.018 + 0.5);
    double const linearExpansion = 
        (1.099 * pow(0.018, oneOverGamma) - 0.099) / 0.018;
    double const maxvalScaler = (double)newMaxval/maxval;

    for (i = 0; i <= linearCutoff; ++i) 
        table[i] = i * linearExpansion * maxvalScaler + 0.5;
    for (; i <= maxval; ++i) {
        double const normalized = ((double) i) / maxval;
            /* Xel sample value normalized to 0..1 */
        double const v = 1.099 * pow(normalized, oneOverGamma) - 0.099;
        table[i] = MIN((xelval)(v * newMaxval + 0.5), newMaxval);  
            /* denormalize, round, and clip */
    }
}



static void
buildBt709GammaInverse(xelval       table[],
                       xelval const maxval,
                       xelval const newMaxval,
                       double const gamma) {
/*----------------------------------------------------------------------------
   Build a gamma table of size maxval+1 for the Inverse of the ITU
   Rec. BT.709 gamma transfer function.

   'gamma' must be 1/0.45 for true Rec. 709.
-----------------------------------------------------------------------------*/
    double const oneOverGamma = 1.0 / gamma;
    xelval i;

    /* This transfer function is linear for sample values 0
       .. maxval*.018 and an exponential for larger sample values.
       The exponential is slightly stretched and translated, though,
       unlike the popular pure exponential gamma transfer function.
    */

    xelval const linearCutoff = (xelval) (maxval * 0.018 + 0.5);
    double const linearCompression = 
        0.018 / (1.099 * pow(0.018, oneOverGamma) - 0.099);
    double const maxvalScaler = (double)newMaxval/maxval;

    for (i = 0; i <= linearCutoff / linearCompression; ++i) 
        table[i] = i * linearCompression * maxvalScaler + 0.5;

    for (; i <= maxval; ++i) {
        double const normalized = ((double) i) / maxval;
            /* Xel sample value normalized to 0..1 */
        double const v = pow((normalized + 0.099) / 1.099, gamma);
        table[i] = MIN((xelval)(v * newMaxval + 0.5), newMaxval);  
            /* denormalize, round, and clip */
    }
}



static void
buildSrgbGamma(xelval       table[],
               xelval const maxval,
               xelval const newMaxval,
               double const gamma) {
/*----------------------------------------------------------------------------
   Build a gamma table of size maxval+1 for the IEC SRGB gamma
   transfer function (Standard IEC 61966-2-1).

   'gamma' must be 2.4 for true SRGB
-----------------------------------------------------------------------------*/
    double const oneOverGamma = 1.0 / gamma;
    xelval i;

    /* This transfer function is linear for sample values 0
       .. maxval*.040405 and an exponential for larger sample values.
       The exponential is slightly stretched and translated, though,
       unlike the popular pure exponential gamma transfer function.
    */
    xelval const linearCutoff = (xelval) maxval * 0.0031308 + 0.5;
    double const linearExpansion = 
        (1.055 * pow(0.0031308, oneOverGamma) - 0.055) / 0.0031308;
    double const maxvalScaler = (double)newMaxval/maxval;

    for (i = 0; i <= linearCutoff; ++i) 
        table[i] = i * linearExpansion * maxvalScaler + 0.5;
    for (; i <= maxval; ++i) {
        double const normalized = ((double) i) / maxval;
            /* Xel sample value normalized to 0..1 */
        double const v = 1.055 * pow(normalized, oneOverGamma) - 0.055;
        table[i] = MIN((xelval)(v * newMaxval + 0.5), newMaxval);  
            /* denormalize, round, and clip */
    }
}



static void
buildSrgbGammaInverse(xelval       table[],
                      xelval const maxval,
                      xelval const newMaxval,
                      double const gamma) {
/*----------------------------------------------------------------------------
   Build a gamma table of size maxval+1 for the Inverse of the IEC SRGB gamma
   transfer function (Standard IEC 61966-2-1).

   'gamma' must be 2.4 for true SRGB
-----------------------------------------------------------------------------*/
    double const oneOverGamma = 1.0 / gamma;
    xelval i;

    /* This transfer function is linear for sample values 0
       .. maxval*.040405 and an exponential for larger sample values.
       The exponential is slightly stretched and translated, though,
       unlike the popular pure exponential gamma transfer function.
    */
    xelval const linearCutoff = (xelval) maxval * 0.0031308 + 0.5;
    double const linearCompression = 
        0.0031308 / (1.055 * pow(0.0031308, oneOverGamma) - 0.055);
    double const maxvalScaler = (double)newMaxval/maxval;

    for (i = 0; i <= linearCutoff / linearCompression; ++i) 
        table[i] = i * linearCompression * maxvalScaler + 0.5;
    for (; i <= maxval; ++i) {
        double const normalized = ((double) i) / maxval;
            /* Xel sample value normalized to 0..1 */
        double const v = pow((normalized + 0.055) / 1.055, gamma);
        table[i] = MIN((xelval)(v * newMaxval + 0.5), newMaxval);  
            /* denormalize, round, and clip */
    }
}



static void
buildBt709ToSrgbGamma(xelval       table[],
                      xelval const maxval,
                      xelval const newMaxval,
                      double const gammaSrgb) {
/*----------------------------------------------------------------------------
   Build a gamma table of size maxval+1 for the combination of the
   inverse of ITU Rec BT.709 and the forward SRGB gamma transfer
   functions.  I.e. this converts from Rec 709 to SRGB.

   'gammaSrgb' must be 2.4 for true SRGB.
-----------------------------------------------------------------------------*/
    double const oneOverGamma709  = 0.45;
    double const gamma709         = 1.0 / oneOverGamma709;
    double const oneOverGammaSrgb = 1.0 / gammaSrgb;
    double const normalizer       = 1.0 / maxval;

    /* This transfer function is linear for sample values 0
       .. maxval*.018 and an exponential for larger sample values.
       The exponential is slightly stretched and translated, though,
       unlike the popular pure exponential gamma transfer function.
    */

    xelval const linearCutoff709 = (xelval) (maxval * 0.018 + 0.5);
    double const linearCompression709 = 
        0.018 / (1.099 * pow(0.018, oneOverGamma709) - 0.099);

    double const linearCutoffSrgb = 0.0031308;
    double const linearExpansionSrgb = 
        (1.055 * pow(0.0031308, oneOverGammaSrgb) - 0.055) / 0.0031308;

    xelval i;

    for (i = 0; i <= maxval; ++i) {
        double const normalized = i * normalizer;
            /* Xel sample value normalized to 0..1 */
        double radiance;
        double srgb;

        if (i < linearCutoff709 / linearCompression709)
            radiance = normalized * linearCompression709;
        else
            radiance = pow((normalized + 0.099) / 1.099, gamma709);

        if (radiance < linearCutoffSrgb)
            srgb = radiance * linearExpansionSrgb;
        else
            srgb = 1.055 * pow(normalized, oneOverGammaSrgb) - 0.055;

        table[i] = srgb * newMaxval + 0.5;
    }
}



static void
buildSrgbToBt709Gamma(xelval       table[],
                      xelval const maxval,
                      xelval const newMaxval,
                      double const gamma709) {
/*----------------------------------------------------------------------------
   Build a gamma table of size maxval+1 for the combination of the
   inverse of SRGB and the forward ITU Rec BT.709 gamma transfer
   functions.  I.e. this converts from SRGB to Rec 709.

   'gamma709' must be 1/0.45 for true Rec. 709.
-----------------------------------------------------------------------------*/
    double const oneOverGamma709  = 1.0 / gamma709;
    double const gammaSrgb        = 2.4;
    double const oneOverGammaSrgb = 1.0 / gammaSrgb;
    double const normalizer       = 1.0 / maxval;

    /* This transfer function is linear for sample values 0
       .. maxval*.040405 and an exponential for larger sample values.
       The exponential is slightly stretched and translated, though,
       unlike the popular pure exponential gamma transfer function.
    */
    xelval const linearCutoffSrgb = (xelval) maxval * 0.0031308 + 0.5;
    double const linearCompressionSrgb = 
        0.0031308 / (1.055 * pow(0.0031308, oneOverGammaSrgb) - 0.055);

    xelval const linearCutoff709 = (xelval) (maxval * 0.018 + 0.5);
    double const linearExpansion709 = 
        (1.099 * pow(0.018, oneOverGamma709) - 0.099) / 0.018;

    xelval i;

    for (i = 0; i <= maxval; ++i) {
        double const normalized = i * normalizer;
            /* Xel sample value normalized to 0..1 */
        double radiance;
        double bt709;

        if (i < linearCutoffSrgb / linearCompressionSrgb)
            radiance = normalized * linearCompressionSrgb;
        else
            radiance = pow((normalized + 0.099) / 1.099, gammaSrgb);

        if (radiance < linearCutoff709)
            bt709 = radiance * linearExpansion709;
        else
            bt709 = 1.055 * pow(normalized, oneOverGamma709) - 0.055;

        table[i] = bt709 * newMaxval + 0.5;
    }
}



static void
createGammaTables(enum transferFunction const transferFunction,
                  xelval                const maxval,
                  xelval                const newMaxval,
                  double                const rgamma, 
                  double                const ggamma, 
                  double                const bgamma,
                  xelval **             const rtableP,
                  xelval **             const gtableP,
                  xelval **             const btableP) {

    /* Allocate space for the tables. */
    MALLOCARRAY(*rtableP, maxval+1);
    MALLOCARRAY(*gtableP, maxval+1);
    MALLOCARRAY(*btableP, maxval+1);
    if (*rtableP == NULL || *gtableP == NULL || *btableP == NULL)
        pm_error("Can't get memory to make gamma transfer tables");

    /* Build the gamma corection tables. */
    switch (transferFunction) {
    case XF_BT709RAMP: {
        buildBt709Gamma(*rtableP, maxval, newMaxval, rgamma);
        buildBt709Gamma(*gtableP, maxval, newMaxval, ggamma);
        buildBt709Gamma(*btableP, maxval, newMaxval, bgamma);
    } break;

    case XF_BT709RAMP_INVERSE: {
        buildBt709GammaInverse(*rtableP, maxval, newMaxval, rgamma);
        buildBt709GammaInverse(*gtableP, maxval, newMaxval, ggamma);
        buildBt709GammaInverse(*btableP, maxval, newMaxval, bgamma);
    } break;

    case XF_SRGBRAMP: {
        buildSrgbGamma(*rtableP, maxval, newMaxval, rgamma);
        buildSrgbGamma(*gtableP, maxval, newMaxval, ggamma);
        buildSrgbGamma(*btableP, maxval, newMaxval, bgamma);
    } break;

    case XF_SRGBRAMP_INVERSE: {
        buildSrgbGammaInverse(*rtableP, maxval, newMaxval, rgamma);
        buildSrgbGammaInverse(*gtableP, maxval, newMaxval, ggamma);
        buildSrgbGammaInverse(*btableP, maxval, newMaxval, bgamma);
    } break;

    case XF_EXP: {
        buildPowGamma(*rtableP, maxval, newMaxval, rgamma);
        buildPowGamma(*gtableP, maxval, newMaxval, ggamma);
        buildPowGamma(*btableP, maxval, newMaxval, bgamma);
    } break;

    case XF_EXP_INVERSE: {
        buildPowGamma(*rtableP, maxval, newMaxval, 1.0/rgamma);
        buildPowGamma(*gtableP, maxval, newMaxval, 1.0/ggamma);
        buildPowGamma(*btableP, maxval, newMaxval, 1.0/bgamma);
    } break;

    case XF_BT709_TO_SRGB: {
        buildBt709ToSrgbGamma(*rtableP, maxval, newMaxval, rgamma);
        buildBt709ToSrgbGamma(*gtableP, maxval, newMaxval, ggamma);
        buildBt709ToSrgbGamma(*btableP, maxval, newMaxval, bgamma);
    } break;

    case XF_SRGB_TO_BT709: {
        buildSrgbToBt709Gamma(*rtableP, maxval, newMaxval, rgamma);
        buildSrgbToBt709Gamma(*gtableP, maxval, newMaxval, ggamma);
        buildSrgbToBt709Gamma(*btableP, maxval, newMaxval, bgamma);
    } break;
    }
}



static void
convertRaster(FILE *   const ifP,
              FILE *   const ofP,
              int      const cols,
              int      const rows,
              xelval   const maxval,
              int      const format,
              xelval   const outputMaxval,
              int      const outputFormat,
              xelval * const rtable,
              xelval * const gtable,
              xelval * const btable) {

    xel * xelrow;
    unsigned int row;

    xelrow = pnm_allocrow(cols);

    for (row = 0; row < rows; ++row) {
        pnm_readpnmrow(ifP, xelrow, cols, maxval, format);

        pnm_promoteformatrow(xelrow, cols, maxval, format, 
                             maxval, outputFormat);

        switch (PNM_FORMAT_TYPE(outputFormat)) {
        case PPM_TYPE: {
            unsigned int col;
            for (col = 0; col < cols; ++col) {
                xelval const r = PPM_GETR(xelrow[col]);
                xelval const g = PPM_GETG(xelrow[col]);
                xelval const b = PPM_GETB(xelrow[col]);
                PPM_ASSIGN(xelrow[col], rtable[r], gtable[g], btable[b]);
            }
        } break;

        case PGM_TYPE: {
            unsigned int col;
            for (col = 0; col < cols; ++col) {
                xelval const xel = PNM_GET1(xelrow[col]);
                PNM_ASSIGN1(xelrow[col], gtable[xel]);
            }
        } break;
        default:
            pm_error("Internal error.  Impossible format type");
        }
        pnm_writepnmrow(ofP, xelrow, cols, outputMaxval, outputFormat, 0);
    }
    pnm_freerow(xelrow);
}



int
main(int argc, char *argv[]) {
    struct cmdlineInfo cmdline;
    FILE * ifP;
    xelval maxval;
    int rows, cols, format;
    xelval outputMaxval;
    int outputFormat;
    xelval * rtable;
    xelval * gtable;
    xelval * btable;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.filespec);

    pnm_readpnminit(ifP, &cols, &rows, &maxval, &format);

    if (PNM_FORMAT_TYPE(format) == PPM_TYPE)
        outputFormat = PPM_TYPE;
    else if (cmdline.rgamma != cmdline.ggamma 
             || cmdline.ggamma != cmdline.bgamma) 
        outputFormat = PPM_TYPE;
    else 
        outputFormat = PGM_TYPE;

    if (PNM_FORMAT_TYPE(format) != outputFormat) {
        if (outputFormat == PPM_TYPE)
            pm_message("Promoting to PPM");
        if (outputFormat == PGM_TYPE)
            pm_message("Promoting to PGM");
    }

    outputMaxval = cmdline.makeNewMaxval ? cmdline.maxval : maxval;

    createGammaTables(cmdline.transferFunction, maxval,
                      outputMaxval,
                      cmdline.rgamma, cmdline.ggamma, cmdline.bgamma,
                      &rtable, &gtable, &btable);

    pnm_writepnminit(stdout, cols, rows, outputMaxval, outputFormat, 0);

    convertRaster(ifP, stdout, cols, rows, maxval, format,
                  outputMaxval, outputFormat,
                  rtable, gtable, btable);

    pm_close(ifP);
    pm_close(stdout);

    return 0;
}
