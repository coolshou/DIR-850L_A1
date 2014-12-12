/* pnmshear.c - read a portable anymap and shear it by some angle
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#define _XOPEN_SOURCE   /* get M_PI in math.h */

#include <math.h>
#include <string.h>

#include "pnm.h"
#include "shhopt.h"

#define SCALE 4096
#define HALFSCALE 2048

struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *       input_filespec;  /* Filespec of input file */
    double       angle;           /* requested shear angle, in radians */
    unsigned int noantialias;     /* -noantialias option */
};



static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdlineP) {

    optStruct3 opt;
    unsigned int option_def_index = 0;
    optEntry *option_def = malloc(100*sizeof(optEntry));

    OPTENT3(0, "noantialias",      OPT_FLAG,  NULL, &cmdlineP->noantialias, 0);
    
    opt.opt_table = option_def;
    opt.short_allowed = FALSE;
    opt.allowNegNum = TRUE;

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
    
    if (argc-1 < 1)
        pm_error("Need an argument:  the shear angle.\n");
    else {
        char *endptr;
        cmdlineP->angle = strtod(argv[1], &endptr) * M_PI / 180;
        if (*endptr != '\0' || strlen(argv[1]) == 0)
            pm_error("Angle argument is not a valid floating point number: "
                     "'%s'", argv[1]);
        if (argc-1 < 2)
            cmdlineP->input_filespec = "-";
        else {
            cmdlineP->input_filespec = argv[2];
            if (argc-1 > 2)
                pm_error("too many arguments (%d).  "
                         "The only arguments are shear angle and filespec.",
                         argc-1);
        }
    }
}


static void
makeNewXel(xel * const outputXelP, xel const curXel, xel const prevXel,
           double const fracnew0, double const omfracnew0, int const format) {
/*----------------------------------------------------------------------------
   Create an output xel as *outputXel, which is part curXel and part
   prevXel, the part given by the fractions omfracnew0 and fracnew0,
   respectively.  These fraction values are the numerator of a fraction
   whose denominator is SCALE.

   The format of the pixel is 'format'.
-----------------------------------------------------------------------------*/

    switch ( PNM_FORMAT_TYPE(format) ) {
    case PPM_TYPE:
        PPM_ASSIGN( *outputXelP,
                    ( fracnew0 * PPM_GETR(prevXel) 
                      + omfracnew0 * PPM_GETR(curXel) 
                      + HALFSCALE ) / SCALE,
                    ( fracnew0 * PPM_GETG(prevXel) 
                      + omfracnew0 * PPM_GETG(curXel) 
                      + HALFSCALE ) / SCALE,
                    ( fracnew0 * PPM_GETB(prevXel) 
                      + omfracnew0 * PPM_GETB(curXel) 
                      + HALFSCALE ) / SCALE );
        break;
        
    default:
        PNM_ASSIGN1( *outputXelP,
                     ( fracnew0 * PNM_GET1(prevXel) 
                       + omfracnew0 * PNM_GET1(curXel) 
                       + HALFSCALE ) / SCALE );
        break;
    }
}


static void
shear_row(xel * const xelrow, int const cols, 
          xel * const newxelrow, int const newcols, 
          double const shearCols,
          int const format, xel const bgxel, bool const antialias) {
/*----------------------------------------------------------------------------
   Shear the row 'xelrow' by 'shearCols' columns, and return the result as
   'newxelrow'.  They are 'cols' and 'newcols' columns wide, respectively.
   
   Fill in the part of the output row that doesn't contain image data with
   'bgxel'.

   Use antialiasing iff 'antialias'.

   The format of the input xels (which implies something about the
   output xels too) is 'format'.
-----------------------------------------------------------------------------*/
    int const intShearCols = (int) shearCols;
        
    if ( antialias ) {
        const long fracnew0 = ( shearCols - intShearCols ) * SCALE;
        const long omfracnew0 = SCALE - fracnew0;

        int col;
        xel prevXel;
            
        for ( col = 0; col < newcols; ++col )
            newxelrow[col] = bgxel;

        prevXel = bgxel;
        for ( col = 0; col < cols; ++col){
            makeNewXel(&newxelrow[intShearCols + col],
                       xelrow[col], prevXel, fracnew0, omfracnew0,
                       format);
            prevXel = xelrow[col];
        }
        if ( fracnew0 > 0 ) 
            /* Need to add a column for what's left over */
            makeNewXel(&newxelrow[intShearCols + cols],
                       bgxel, prevXel, fracnew0, omfracnew0, format);
    } else {
        int col;
        for ( col = 0; col < intShearCols; ++col )
            newxelrow[col] = bgxel;
        for ( col = 0; col < cols; ++col )
            newxelrow[intShearCols+col] = xelrow[col];
        for ( col = intShearCols + cols; col < newcols; ++col )
            newxelrow[col] = bgxel;
    }
}



int
main(int argc, char * argv[]) {
    FILE* ifp;
    xel* xelrow;
    xel* newxelrow;
    xel bgxel;
    int rows, cols, format; 
    int newformat, newcols; 
    int row;
    xelval maxval, newmaxval;
    double shearfac;

    struct cmdline_info cmdline;

    pnm_init( &argc, argv );

    parse_command_line( argc, argv, &cmdline );

    ifp = pm_openr( cmdline.input_filespec );

    pnm_readpnminit( ifp, &cols, &rows, &maxval, &format );
    xelrow = pnm_allocrow( cols );

    /* Promote PBM files to PGM. */
    if ( !cmdline.noantialias && PNM_FORMAT_TYPE(format) == PBM_TYPE ) {
        newformat = PGM_TYPE;
        newmaxval = PGM_MAXMAXVAL;
        pm_message( "promoting from PBM to PGM - "
                    "use -noantialias to avoid this" );
    } else {
        newformat = format;
        newmaxval = maxval;
    }

    shearfac = tan( cmdline.angle );
    if ( shearfac < 0.0 )
        shearfac = -shearfac;

    newcols = rows * shearfac + cols + 0.999999;

    pnm_writepnminit( stdout, newcols, rows, newmaxval, newformat, 0 );
    newxelrow = pnm_allocrow( newcols );

    bgxel = pnm_backgroundxelrow( xelrow, cols, newmaxval, format );

    for ( row = 0; row < rows; ++row ) {
        double shearCols;

        pnm_readpnmrow( ifp, xelrow, cols, newmaxval, format );

        if ( cmdline.angle > 0.0 )
            shearCols = row * shearfac;
        else
            shearCols = ( rows - row ) * shearfac;

        shear_row(xelrow, cols, newxelrow, newcols, 
                  shearCols, format, bgxel, !cmdline.noantialias);

        pnm_writepnmrow( stdout, newxelrow, newcols, newmaxval, newformat, 0 );
    }

    pm_close( ifp );
    pm_close( stdout );

    exit( 0 );
}

