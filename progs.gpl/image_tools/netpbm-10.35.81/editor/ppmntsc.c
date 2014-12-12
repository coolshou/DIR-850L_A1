/* This is ppmntsc.c, a program to adjust saturation values in an image
   so they are legal for NTSC or PAL.

   It is derived from the program rlelegal.c, dated June 5, 1995,
   which is described below and propagates that program's copyright.
   The derivation was done by Bryan Henderson on 2000.04.21 to convert
   it from operating on the RLE format to operating on the PPM format
   and to rewrite it in a cleaner style, taking advantage of modern C
   compiler technology.  
*/


/*
 * This software is copyrighted as noted below.  It may be freely copied,
 * modified, and redistributed, provided that the copyright notice is 
 * preserved on all copies.
 * 
 * There is no warranty or other guarantee of fitness for this software,
 * it is provided solely "as is".  Bug reports or fixes may be sent
 * to the author, who may or may not act on them as he desires.
 *
 * You may not include this software in a program or other software product
 * without supplying the source, or without informing the end-user that the 
 * source is available for no extra charge.
 *
 * If you modify this software, you should include a notice giving the
 * name of the person performing the modification, the date of modification,
 * and the reason for such modification.
 */

/* 
 * rlelegal.c - Make RGB colors legal in the YIQ or YUV color systems.
 * 
 * Author:	Wes Barris
 * 		Minnesota Supercomputer Center, Inc.
 * Date:	Fri Oct 15, 1993
 * @Copyright, Research Equipment Inc., d/b/a Minnesota Supercomputer
 * Center, Inc., 1993

 */

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "ppm.h"
#include "mallocvar.h"
#include "shhopt.h"

#define TRUE 1
#define FALSE 0

enum legalize {RAISE_SAT, LOWER_SAT, ALREADY_LEGAL};
   /* The actions that make a legal pixel */

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilename;
    unsigned int verbose;
    unsigned int debug;
    unsigned int pal;
    enum {ALL, LEGAL_ONLY, ILLEGAL_ONLY, CORRECTED_ONLY} output;
};




static void 
rgbtoyiq(const int r, const int g, const int b, 
         double * const y_p, 
         double * const i_p, 
         double * const q_p) {
    
    *y_p = .299*(r/255.0) + .587*(g/255.0) + .114*(b/255.0);
    *i_p = .596*(r/255.0) - .274*(g/255.0) - .322*(b/255.0);
    *q_p = .211*(r/255.0) - .523*(g/255.0) + .312*(b/255.0);
}



static void 
yiqtorgb(const double y, const double i, const double q, 
         int * const r_p, int * const g_p, int * const b_p) {
    *r_p = 255.0*(1.00*y + .9562*i + .6214*q);
    *g_p = 255.0*(1.00*y - .2727*i - .6468*q);
    *b_p = 255.0*(1.00*y -1.1037*i +1.7006*q);
}



static void 
rgbtoyuv(const int r, const int g, const int b, 
         double * const y_p, 
         double * const u_p, 
         double * const v_p) {
    *y_p =  .299*(r/255.0) + .587*(g/255.0) + .114*(b/255.0);
    *u_p = -.147*(r/255.0) - .289*(g/255.0) + .437*(b/255.0);
    *v_p =  .615*(r/255.0) - .515*(g/255.0) - .100*(b/255.0);
}



static void 
yuvtorgb(const double y, const double u, const double v, 
         int * const r_p, int * const g_p, int * const b_p) {
    
    *r_p = 255.0*(1.00*y + .0000*u +1.1398*v);
    *g_p = 255.0*(1.00*y - .3938*u - .5805*v);
    *b_p = 255.0*(1.00*y +2.0279*u + .0000*v);
}



static void
make_legal_yiq(const double y, const double i, const double q, 
               double * const y_new_p, 
               double * const i_new_p, 
               double * const q_new_p,
               enum legalize * const action_p
    ) {
    
    double sat_old, sat_new;
    /*
     * I and Q are legs of a right triangle.  Saturation is the hypotenuse.
     */
    sat_old = sqrt(i*i + q*q);
    if (y+sat_old > 1.0) {
        const double diff = 0.5*((y+sat_old) - 1.0);
        *y_new_p = y - diff;
        sat_new = 1.0 - *y_new_p;
        *i_new_p = i*(sat_new/sat_old);
        *q_new_p = q*(sat_new/sat_old);
        *action_p = LOWER_SAT;
    } else if (y-sat_old <= -0.251) {
        const double diff = 0.5*((sat_old-y) - 0.251);
        *y_new_p = y + diff;
        sat_new = 0.250 + *y_new_p;
        *i_new_p = i*(sat_new/sat_old);
        *q_new_p = q*(sat_new/sat_old);
        *action_p = RAISE_SAT;
    } else {
        *y_new_p = y;
        *i_new_p = i;
        *q_new_p = q;
        *action_p = ALREADY_LEGAL;
    }
    return;
}



static void
make_legal_yuv(const double y, const double u, const double v, 
               double * const y_new_p, 
               double * const u_new_p, 
               double * const v_new_p,
               enum legalize * const action_p
    ) {
    
    double sat_old, sat_new;
    /*
     * U and V are legs of a right triangle.  Saturation is the hypotenuse.
     */
    sat_old = sqrt(u*u + v*v);
    if (y+sat_old >= 1.334) {
        const double diff = 0.5*((y+sat_old) - 1.334);
        *y_new_p = y - diff;
        sat_new = 1.333 - *y_new_p;
        *u_new_p = u*(sat_new/sat_old);
        *v_new_p = v*(sat_new/sat_old);
        *action_p = LOWER_SAT;
    } else if (y-sat_old <= -0.339) {
        const double diff = 0.5*((sat_old-y) - 0.339);
        *y_new_p = y + diff;
        sat_new = 0.338 + *y_new_p;
        *u_new_p = u*(sat_new/sat_old);
        *v_new_p = v*(sat_new/sat_old);
        *action_p = RAISE_SAT;
    } else {
        *u_new_p = u;
        *v_new_p = v;
        *action_p = ALREADY_LEGAL;
    }
    return;
}



static void
make_legal_yiq_i(const int r_in, const int g_in, const int b_in, 
                 int * const r_out_p, 
                 int * const g_out_p, 
                 int * const b_out_p,
                 enum legalize * const action_p
    ) {
    
    double y, i, q;
    double y_new, i_new, q_new;
    /*
     * Convert to YIQ and compute the new saturation.
     */
    rgbtoyiq(r_in, g_in, b_in, &y, &i, &q);
    make_legal_yiq(y, i, q, &y_new, &i_new, &q_new, action_p);
    if (*action_p != ALREADY_LEGAL)
        /*
         * Given the new I and Q, compute new RGB values.
        */
        yiqtorgb(y_new, i_new, q_new, r_out_p, g_out_p, b_out_p);
    else {
        *r_out_p = r_in;
        *g_out_p = g_in;
        *b_out_p = b_in;
      }
    return;
}



static void
make_legal_yuv_i(const int r_in, const int g_in, const int b_in, 
                 int * const r_out_p, 
                 int * const g_out_p, 
                 int * const b_out_p,
                 enum legalize * const action_p
    ){
    
    double y, u, v;
    double y_new, u_new, v_new;  
    /*
     * Convert to YUV and compute the new saturation.
     */
    rgbtoyuv(r_in, g_in, b_in, &y, &u, &v);
    make_legal_yuv(y, u, v, &y_new, &u_new, &v_new, action_p);
    if (*action_p != ALREADY_LEGAL)
        /*
         * Given the new U and V, compute new RGB values.
         */
        yuvtorgb(y_new, u_new, v_new, r_out_p, g_out_p, b_out_p);
    else {
        *r_out_p = r_in;
        *g_out_p = g_in;
        *b_out_p = b_in;
    }
    return;
}



static void 
make_legal_yiq_b(const pixel input,
                 pixel * const output_p,
                 enum legalize * const action_p) {


    int ir_in, ig_in, ib_in;
    int ir_out, ig_out, ib_out;
    
    ir_in = (int)PPM_GETR(input);
    ig_in = (int)PPM_GETG(input);
    ib_in = (int)PPM_GETB(input);

    make_legal_yiq_i(ir_in, ig_in, ib_in, &ir_out, &ig_out, &ib_out, action_p);

    PPM_ASSIGN(*output_p, ir_out, ig_out, ib_out);

    return;
}



static void 
make_legal_yuv_b(const pixel input,
                 pixel * const output_p,
                 enum legalize * const action_p) {

    int ir_in, ig_in, ib_in;
    int ir_out, ig_out, ib_out;
    
    ir_in = (int)PPM_GETR(input);
    ig_in = (int)PPM_GETG(input);
    ib_in = (int)PPM_GETB(input);
    make_legal_yuv_i(ir_in, ig_in, ib_in, &ir_out, &ig_out, &ib_out, action_p);

    PPM_ASSIGN(*output_p, ir_out, ig_out, ib_out);

    return;
}



static void 
report_mapping(const pixel old_pixel, const pixel new_pixel) {
/*----------------------------------------------------------------------------
  Assuming old_pixel and new_pixel are input and output pixels,
  tell the user that we changed a pixel to make it legal, if in fact we
  did and it isn't the same change that we just reported.
-----------------------------------------------------------------------------*/
    static pixel last_changed_pixel;
    static int first_time = TRUE;

    if (!PPM_EQUAL(old_pixel, new_pixel) && 
        (first_time || PPM_EQUAL(old_pixel, last_changed_pixel))) {
        pm_message("Mapping %d %d %d -> %d %d %d\n",
                   PPM_GETR(old_pixel),
                   PPM_GETG(old_pixel),
                   PPM_GETB(old_pixel),
                   PPM_GETR(new_pixel),
                   PPM_GETG(new_pixel),
                   PPM_GETB(new_pixel)
            );

        last_changed_pixel = old_pixel;
        first_time = FALSE;
    }    
}



static void
convert_one_image(FILE * const ifp, struct cmdlineInfo const cmdline, 
                  bool * const eofP, 
                  int * const hicountP, int * const locountP) {

    /* Parameters of input image: */
    int rows, cols;
    pixval maxval;
    int format;

    ppm_readppminit(ifp, &cols, &rows, &maxval, &format);
    ppm_writeppminit(stdout, cols, rows, maxval, FALSE);
    {
        pixel* const input_row = ppm_allocrow(cols);
        pixel* const output_row = ppm_allocrow(cols);
        pixel last_illegal_pixel;
        /* Value of the illegal pixel we most recently processed */
        pixel black;
        /* A constant - black pixel */

        PPM_ASSIGN(black, 0, 0, 0);

        PPM_ASSIGN(last_illegal_pixel, 0, 0, 0);  /* initial value */
        {
            int row;

            *hicountP = 0; *locountP = 0;  /* initial values */

            for (row = 0; row < rows; ++row) {
                int col;
                ppm_readppmrow(ifp, input_row, cols, maxval, format);
                for (col = 0; col < cols; ++col) {
                    pixel corrected;
                    /* Corrected or would-be corrected value for pixel */
                    enum legalize action;
                    /* What action was used to make pixel legal */
                    if (cmdline.pal)
                        make_legal_yuv_b(input_row[col],
                                         &corrected,
                                         &action);
                    else
                        make_legal_yiq_b(input_row[col],
                                         &corrected,
                                         &action);
                        
                    if (action == LOWER_SAT) 
                        (*hicountP)++;
                    if (action == RAISE_SAT)
                        (*locountP)++;
                    if (cmdline.debug) report_mapping(input_row[col],
                                                      corrected);
                    switch (cmdline.output) {
                    case ALL:
                        output_row[col] = corrected;
                        break;
                    case LEGAL_ONLY:
                        output_row[col] = (action == ALREADY_LEGAL) ?
                            input_row[col] : black;
                        break;
                    case ILLEGAL_ONLY:
                        output_row[col] = (action != ALREADY_LEGAL) ?
                            input_row[col] : black;
                        break;
                    case CORRECTED_ONLY:
                        output_row[col] = (action != ALREADY_LEGAL) ?
                            corrected : black;
                        break;
                    }
                }
                ppm_writeppmrow(stdout, output_row, cols, maxval, FALSE);
            }
        }
        ppm_freerow(output_row);
        ppm_freerow(input_row);
    }
}


static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that many of the strings that this function returns in the
   *cmdlineP structure are actually in the supplied argv array.  And
   sometimes, one of these strings is actually just a suffix of an entry
   in argv!
-----------------------------------------------------------------------------*/
    optStruct3 opt;
    optEntry *option_def;
        /* Instructions to OptParseOptions on how to parse our options.
         */
    unsigned int option_def_index;
    unsigned int legalonly, illegalonly, correctedonly;

    MALLOCARRAY(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3('v', "verbose",        OPT_FLAG, NULL,  &cmdlineP->verbose,  0);
    OPTENT3('V', "debug",          OPT_FLAG, NULL,  &cmdlineP->debug,    0);
    OPTENT3('p', "pal",            OPT_FLAG, NULL,  &cmdlineP->pal,      0);
    OPTENT3('l', "legalonly",      OPT_FLAG, NULL,  &legalonly,           0);
    OPTENT3('i', "illegalonly",    OPT_FLAG, NULL,  &illegalonly,         0);
    OPTENT3('c', "correctedonly",  OPT_FLAG, NULL,  &correctedonly,       0);

    opt.opt_table = option_def;
    opt.short_allowed = TRUE;
    opt.allowNegNum = FALSE;

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

    if (argc - 1 == 0)
        cmdlineP->inputFilename = "-";  /* he wants stdin */
    else if (argc - 1 == 1)
        cmdlineP->inputFilename = argv[1];
    else 
        pm_error("Too many arguments.  The only arguments accepted "
                 "are the mask color and optional input file specification");

    if (legalonly + illegalonly + correctedonly > 1)
        pm_error("--legalonly, --illegalonly, and --correctedonly are "
                 "conflicting options.  Specify at most one of these.");
        
    if (legalonly) 
        cmdlineP->output = LEGAL_ONLY;
    else if (illegalonly) 
        cmdlineP->output = ILLEGAL_ONLY;
    else if (correctedonly) 
        cmdlineP->output = CORRECTED_ONLY;
    else 
        cmdlineP->output = ALL;
}



int
main(int argc, char **argv) {
    
    struct cmdlineInfo cmdline;
    FILE * ifP;
    int total_hicount, total_locount;
    int image_count;

    bool eof;

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilename);

    image_count = 0;    /* initial value */
    total_hicount = 0;  /* initial value */
    total_locount = 0;  /* initial value */

    eof = FALSE;
    while (!eof) {
        int hicount, locount;
        convert_one_image(ifP, cmdline, &eof, &hicount, &locount);
        image_count++;
        total_hicount += hicount;
        total_locount += locount;
        ppm_nextimage(ifP, &eof);
    }


	if (cmdline.verbose) {
        pm_message("%d images processed.", image_count);
        pm_message("%d pixels were above the saturation limit.", 
                   total_hicount);
        pm_message("%d pixels were below the saturation limit.", 
                   total_locount);
    }
    
    pm_close(ifP);

    return 0;
}
