#include <assert.h>
#include <string.h>

#include "shhopt.h"
#include "pam.h"

enum function {FN_ADD, FN_SUBTRACT, FN_MULTIPLY, FN_DIVIDE, FN_DIFFERENCE,
               FN_MINIMUM, FN_MAXIMUM, FN_MEAN, FN_COMPARE,
               FN_AND, FN_OR, FN_NAND, FN_NOR, FN_XOR,
               FN_SHIFTLEFT, FN_SHIFTRIGHT
              };

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input1Filespec;  
    const char *input2Filespec;  
    enum function function;
};



static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    
    unsigned int addSpec, subtractSpec, multiplySpec, divideSpec,
        differenceSpec,
        minimumSpec, maximumSpec, meanSpec, compareSpec,
        andSpec, orSpec, nandSpec, norSpec, xorSpec,
        shiftleftSpec, shiftrightSpec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "add",         OPT_FLAG,   NULL, &addSpec,        0);
    OPTENT3(0, "subtract",    OPT_FLAG,   NULL, &subtractSpec,   0);
    OPTENT3(0, "multiply",    OPT_FLAG,   NULL, &multiplySpec,   0);
    OPTENT3(0, "divide",      OPT_FLAG,   NULL, &divideSpec,     0);
    OPTENT3(0, "difference",  OPT_FLAG,   NULL, &differenceSpec, 0);
    OPTENT3(0, "minimum",     OPT_FLAG,   NULL, &minimumSpec,    0);
    OPTENT3(0, "maximum",     OPT_FLAG,   NULL, &maximumSpec,    0);
    OPTENT3(0, "mean",        OPT_FLAG,   NULL, &meanSpec,       0);
    OPTENT3(0, "compare",     OPT_FLAG,   NULL, &compareSpec,    0);
    OPTENT3(0, "and",         OPT_FLAG,   NULL, &andSpec,        0);
    OPTENT3(0, "or",          OPT_FLAG,   NULL, &orSpec,         0);
    OPTENT3(0, "nand",        OPT_FLAG,   NULL, &nandSpec,       0);
    OPTENT3(0, "nor",         OPT_FLAG,   NULL, &norSpec,        0);
    OPTENT3(0, "xor",         OPT_FLAG,   NULL, &xorSpec,        0);
    OPTENT3(0, "shiftleft",   OPT_FLAG,   NULL, &shiftleftSpec,  0);
    OPTENT3(0, "shiftright",  OPT_FLAG,   NULL, &shiftrightSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (addSpec + subtractSpec + multiplySpec + divideSpec + differenceSpec +
        minimumSpec + maximumSpec + meanSpec + compareSpec +
        andSpec + orSpec + nandSpec + norSpec + xorSpec +
        shiftleftSpec + shiftrightSpec > 1)
        pm_error("You may specify only one function");

    if (argc-1 != 2)
        pm_error("You must specify two arguments:  the files which are "
                 "the operands of the "
                 "dyadic function.  You specified %d", argc-1);
    else {
        cmdlineP->input1Filespec = argv[1];
        cmdlineP->input2Filespec = argv[2];
    }

    if (addSpec)
        cmdlineP->function = FN_ADD;
    else if (subtractSpec)
        cmdlineP->function = FN_SUBTRACT;
    else if (multiplySpec)
        cmdlineP->function = FN_MULTIPLY;
    else if (divideSpec)
        cmdlineP->function = FN_DIVIDE;
    else if (differenceSpec)
        cmdlineP->function = FN_DIFFERENCE;
    else if (minimumSpec)
        cmdlineP->function = FN_MINIMUM;
    else if (maximumSpec)
        cmdlineP->function = FN_MAXIMUM;
    else if (meanSpec)
        cmdlineP->function = FN_MEAN;
    else if (compareSpec)
        cmdlineP->function = FN_COMPARE;
    else if (andSpec)
        cmdlineP->function = FN_AND;
    else if (orSpec)
        cmdlineP->function = FN_OR;
    else if (nandSpec)
        cmdlineP->function = FN_NAND;
    else if (norSpec)
        cmdlineP->function = FN_NOR;
    else if (xorSpec)
        cmdlineP->function = FN_XOR;
    else if (shiftleftSpec)
        cmdlineP->function = FN_SHIFTLEFT;
    else if (shiftrightSpec)
        cmdlineP->function = FN_SHIFTRIGHT;
    else
        pm_error("You must specify a function (e.g. '-add')");
}        



enum category {
    CATEGORY_FRACTIONAL_ARITH,
        /* Arithmetic in which each sample represents a the fraction
           sample/maxval.
        */
    CATEGORY_BITSTRING,
        /* And, Or, etc.  Maxval isn't a scale factor at all; it's a mask. */
    CATEGORY_SHIFT
        /* Left argument is a bit string, but right argument is a whole
           number (left maxval is a mask; right maxval is meaningless).
        */
};



static enum category
functionCategory(enum function const function) {

    enum category retval;
    
    switch (function) {
    case FN_ADD:
    case FN_SUBTRACT:
    case FN_DIFFERENCE:
    case FN_MINIMUM:
    case FN_MAXIMUM:
    case FN_MEAN:
    case FN_COMPARE:
    case FN_MULTIPLY:
    case FN_DIVIDE:
        retval = CATEGORY_FRACTIONAL_ARITH;
        break;
    case FN_AND:
    case FN_OR:
    case FN_NAND:
    case FN_NOR:
    case FN_XOR:
        retval = CATEGORY_BITSTRING;
        break;
    case FN_SHIFTLEFT:
    case FN_SHIFTRIGHT:
        retval = CATEGORY_SHIFT;
        break;
    }
    return retval;
}



static void
computeOutputType(struct pam *  const outpamP,
                  struct pam    const inpam1,
                  struct pam    const inpam2,
                  enum function const function) {

    outpamP->size        = sizeof(struct pam);
    outpamP->len         = PAM_STRUCT_SIZE(tuple_type);
    outpamP->file        = stdout;
    outpamP->format      = MAX(inpam1.format, inpam2.format);
    outpamP->plainformat = FALSE;
    outpamP->height      = inpam1.height;
    outpamP->width       = inpam1.width;
    outpamP->depth       = MAX(inpam1.depth, inpam2.depth);

    switch (functionCategory(function)) {    
    case CATEGORY_FRACTIONAL_ARITH:
        if (function == FN_COMPARE)
            outpamP->maxval = 2;
        else
            outpamP->maxval = MAX(inpam1.maxval, inpam2.maxval);
        break;
    case CATEGORY_BITSTRING:
        if (inpam2.maxval != inpam1.maxval)
            pm_error("For a bit string operation, the maxvals of the "
                     "left and right image must be the same.  You have "
                     "left=%u and right=%u", 
                     (unsigned)inpam1.maxval, (unsigned)inpam2.maxval);

        if (pm_bitstomaxval(pm_maxvaltobits(inpam1.maxval)) != inpam1.maxval)
            pm_error("For a bit string operation, the maxvals of the inputs "
                     "must be a full binary count, i.e. a power of two "
                     "minus one such as 0xff.  You have 0x%x",
                     (unsigned)inpam1.maxval);

        outpamP->maxval = inpam1.maxval;
        break;
    case CATEGORY_SHIFT:
        if (pm_bitstomaxval(pm_maxvaltobits(inpam1.maxval)) != inpam1.maxval)
            pm_error("For a bit shift operation, the maxval of the left "
                     "input image "
                     "must be a full binary count, i.e. a power of two "
                     "minus one such as 0xff.  You have 0x%x",
                     (unsigned)inpam1.maxval);
        outpamP->maxval = inpam1.maxval;
    }
    outpamP->bytes_per_sample = (pm_maxvaltobits(outpamP->maxval)+7)/8;
    strcpy(outpamP->tuple_type, inpam1.tuple_type);
}



static samplen
applyNormalizedFunction(enum function const function,
                        samplen       const leftArg,
                        samplen       const rightArg) {

    samplen result;

    switch (function) {
    case FN_ADD:
        result = MIN(1., leftArg + rightArg);
        break;
    case FN_SUBTRACT:
        result = MAX(0., leftArg - rightArg);
        break;
    case FN_MULTIPLY:
        result = leftArg * rightArg;
        break;
    case FN_DIVIDE:
        result = (rightArg > leftArg) ?
        leftArg / rightArg : 1.;
        break;
    case FN_DIFFERENCE:
        result = leftArg > rightArg ? 
            leftArg - rightArg : rightArg - leftArg;
        break;
    case FN_MINIMUM:
        result = MIN(leftArg, rightArg);
        break;
    case FN_MAXIMUM:
        result = MAX(leftArg, rightArg);
        break;
    case FN_MEAN:
        result = (leftArg + rightArg) / 2.0;
        break;
    case FN_COMPARE:
        result = 
            leftArg > rightArg ? 1. : leftArg < rightArg ? 0. : .5;
        break;
    default:
        pm_error("Internal error.  applyNormalizedFunction() called "
                 "for a function it doesn't know how to do: %u", function);
    }

    return result;
}



static void
doNormalizedArith(struct pam *  const inpam1P,
                  struct pam *  const inpam2P,
                  struct pam *  const outpamP,
                  enum function const function) {

    tuplen * tuplerown1;
    tuplen * tuplerown2;
    tuplen * tuplerownOut;
    unsigned int row;

    tuplerown1   = pnm_allocpamrown(inpam1P);
    tuplerown2   = pnm_allocpamrown(inpam2P);
    tuplerownOut = pnm_allocpamrown(outpamP);

    for (row = 0; row < outpamP->height; ++row) {
        unsigned int col;
        pnm_readpamrown(inpam1P, tuplerown1);
        pnm_readpamrown(inpam2P, tuplerown2);
        
        for (col = 0; col < outpamP->width; ++col) {
            unsigned int outplane;
            
            for (outplane = 0; outplane < outpamP->depth; ++outplane) {
                unsigned int const plane1 = MIN(outplane, inpam1P->depth-1);
                unsigned int const plane2 = MIN(outplane, inpam2P->depth-1);

                tuplerownOut[col][outplane] = 
                    applyNormalizedFunction(function, 
                                            tuplerown1[col][plane1], 
                                            tuplerown2[col][plane2]);
                assert(tuplerownOut[col][outplane] >= 0.);
                assert(tuplerownOut[col][outplane] <= 1.);

            }
        }
        pnm_writepamrown(outpamP, tuplerownOut);
    }

    pnm_freepamrown(tuplerown1);
    pnm_freepamrown(tuplerown2);
    pnm_freepamrown(tuplerownOut);
}



static sample
applyUnNormalizedFunction(enum function const function,
                          sample        const leftArg,
                          sample        const rightArg,
                          sample        const maxval) {
/*----------------------------------------------------------------------------
   Apply dyadic function 'function' to the arguments 'leftArg' and
   'rightArg', assuming both are based on the same maxval 'maxval'.
   Return a value which is also a fraction of 'maxval'.

   Exception: for the shift operations, 'rightArg' is not based on any
   maxval.  It is an absolute bit count.
-----------------------------------------------------------------------------*/
    sample result;

    switch (function) {
    case FN_ADD:
        result = MIN(maxval, leftArg + rightArg);
        break;
    case FN_SUBTRACT:
        result = MAX(0, (int)leftArg - (int)rightArg);
        break;
    case FN_DIFFERENCE:
        result = leftArg > rightArg ? leftArg - rightArg : rightArg - leftArg;
        break;
    case FN_MINIMUM:
        result = MIN(leftArg, rightArg);
        break;
    case FN_MAXIMUM:
        result = MAX(leftArg, rightArg);
        break;
    case FN_MEAN:
        result = (leftArg + rightArg + 1) / 2;
        break;
    case FN_COMPARE:
        result = leftArg > rightArg ? 2 : leftArg < rightArg ? 0 : 1;
        break;
    case FN_MULTIPLY:
        result = (leftArg * rightArg + maxval/2) / maxval;
        break;
    case FN_DIVIDE:
        result = (rightArg > leftArg) ?
            (leftArg * maxval + rightArg/2) / rightArg : maxval;
        break;

    case FN_AND:
        result = leftArg & rightArg;
        break;
    case FN_OR:
        result = leftArg | rightArg;
        break;
    case FN_NAND:
        result = ~(leftArg & rightArg) & maxval;
        break;
    case FN_NOR:
        result = ~(leftArg | rightArg) & maxval;
        break;
    case FN_XOR:
        result = leftArg ^ rightArg;
        break;
    case FN_SHIFTLEFT:
        result = (leftArg << rightArg) & maxval;
        break;
    case FN_SHIFTRIGHT:
        result = leftArg >> rightArg;
        break;
    default:
        pm_error("Internal error.  applyUnNormalizedFunction() called "
                 "for a function it doesn't know how to do: %u", function);
    }

    return result;
}



static void
doUnNormalizedArith(struct pam *  const inpam1P,
                    struct pam *  const inpam2P,
                    struct pam *  const outpamP,
                    enum function const function) {
/*----------------------------------------------------------------------------
   Take advantage of the fact that both inputs and the output use the same
   maxval to do the computation without time-consuming normalization of
   sample values.
-----------------------------------------------------------------------------*/
    sample const maxval = outpamP->maxval;

    tuple * tuplerow1;
    tuple * tuplerow2;
    tuple * tuplerowOut;
    unsigned int row;

    /* Input conditions: */
    assert(inpam1P->maxval == maxval);
    assert(inpam2P->maxval == maxval);
    assert(outpamP->maxval == maxval);

    tuplerow1   = pnm_allocpamrow(inpam1P);
    tuplerow2   = pnm_allocpamrow(inpam2P);
    tuplerowOut = pnm_allocpamrow(outpamP);

    for (row = 0; row < outpamP->height; ++row) {
        unsigned int col;
        pnm_readpamrow(inpam1P, tuplerow1);
        pnm_readpamrow(inpam2P, tuplerow2);
        
        for (col = 0; col < outpamP->width; ++col) {
            unsigned int outplane;
            
            for (outplane = 0; outplane < outpamP->depth; ++outplane) {
                unsigned int const plane1 = MIN(outplane, inpam1P->depth-1);
                unsigned int const plane2 = MIN(outplane, inpam2P->depth-1);

                tuplerowOut[col][outplane] = 
                    applyUnNormalizedFunction(function, 
                                              tuplerow1[col][plane1], 
                                              tuplerow2[col][plane2],
                                              maxval);

                assert(tuplerowOut[col][outplane] >= 0);
                assert(tuplerowOut[col][outplane] <= outpamP->maxval);
            }
        }
        pnm_writepamrow(outpamP, tuplerowOut);
    }

    pnm_freepamrow(tuplerow1);
    pnm_freepamrow(tuplerow2);
    pnm_freepamrow(tuplerowOut);
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    struct pam inpam1;
    struct pam inpam2;
    struct pam outpam;
    FILE * if1P;
    FILE * if2P;
    
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    if1P = pm_openr(cmdline.input1Filespec);
    if2P = pm_openr(cmdline.input2Filespec);

    pnm_readpaminit(if1P, &inpam1, PAM_STRUCT_SIZE(tuple_type));
    pnm_readpaminit(if2P, &inpam2, PAM_STRUCT_SIZE(tuple_type));

    if (inpam1.width != inpam2.width || inpam1.height != inpam2.height)
        pm_error("The two images must be the same width and height.  "
                 "The first is %ux%ux%u, but the second is %ux%ux%u",
                 inpam1.width, inpam1.height, inpam1.depth,
                 inpam2.width, inpam2.height, inpam2.depth);

    if (inpam1.depth != 1 && inpam2.depth != 1 && inpam1.depth != inpam2.depth)
        pm_error("The two images must have the same depth or one of them "
                 "must have depth 1.  The first has depth %u and the second "
                 "has depth %u", inpam1.depth, inpam2.depth);

    computeOutputType(&outpam, inpam1, inpam2, cmdline.function);

    pnm_writepaminit(&outpam);

    switch (functionCategory(cmdline.function)) {    
    case CATEGORY_FRACTIONAL_ARITH:
        if (inpam1.maxval == inpam2.maxval)
            doUnNormalizedArith(&inpam1, &inpam2, &outpam, cmdline.function);
        else
            doNormalizedArith(&inpam1, &inpam2, &outpam, cmdline.function);
        break;
    case CATEGORY_BITSTRING:
    case CATEGORY_SHIFT:
        doUnNormalizedArith(&inpam1, &inpam2, &outpam, cmdline.function);
        break;
    }

    pm_close(if1P);
    pm_close(if2P);
    
    return 0;
}
