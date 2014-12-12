/* pnmmalias.c - antialias a portable anymap.
**
** Copyright (C) 1992 by Alberto Accomazzi, Smithsonian Astrophysical
** Observatory.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "pnm.h"

int
main(int argc, char * argv[] ) {
    FILE* ifp;
    xel* xelrow[3];
    xel* newxelrow;
    pixel bgcolorppm, fgcolorppm;
    register xel* xpP;
    register xel* xP;
    register xel* xnP;
    register xel* nxP;
    xel bgcolor, fgcolor;
    int argn, rows, cols, format, newformat, bgonly, fgonly;
    int bgalias, fgalias;
    int row;
    double fmask[9], weight;
    xelval maxval;
    xelval newmaxval;
    const char* const usage = "[-bgcolor <color>] [-fgcolor <color>] [-bonly] [-fonly] [-balias] [-falias] [-weight <w>] [pnmfile]";

    pnm_init( &argc, argv );

    bgonly = fgonly = 0;
    bgalias = fgalias = 0;
    weight = 1./3.;
    argn = 1;
    PPM_ASSIGN( bgcolorppm, 0, 0, 0);
    PPM_ASSIGN( fgcolorppm, 0, 0, 0);

    while ( argn < argc && argv[argn][0] == '-' )
        {
        if ( pm_keymatch( argv[argn], "-fgcolor", 3 ) ) 
        {
        if ( ++argn >= argc ) 
        pm_usage( usage );
        else 
        fgcolorppm = ppm_parsecolor( argv[argn], PPM_MAXMAXVAL );
        }
        else if ( pm_keymatch( argv[argn], "-bgcolor", 3 ) ) 
        {
        if ( ++argn >= argc ) 
        pm_usage( usage );
        else 
        bgcolorppm = ppm_parsecolor( argv[argn], PPM_MAXMAXVAL );
        }
        else if ( pm_keymatch( argv[argn], "-weight", 2 ) ) 
        {
        if ( ++argn >= argc ) 
        pm_usage( usage );
        else if ( sscanf( argv[argn], "%lf", &weight ) != 1 )
            pm_usage( usage );
        else if ( weight >= 1. || weight <= 0. )
        {
        pm_message( "weight factor w must be 0.0 < w < 1.0" );
        pm_usage( usage );
        }
        }
    else if ( pm_keymatch( argv[argn], "-bonly", 3 ) )
        bgonly = 1;
    else if ( pm_keymatch( argv[argn], "-fonly", 3 ) )
        fgonly = 1;
    else if ( pm_keymatch( argv[argn], "-balias", 3 ) )
        bgalias = 1;
    else if ( pm_keymatch( argv[argn], "-falias", 3 ) )
        fgalias = 1;
    else if ( pm_keymatch( argv[argn], "-bfalias", 3 ) )
        bgalias = fgalias = 0;
    else if ( pm_keymatch( argv[argn], "-fbalias", 3 ) )
        bgalias = fgalias = 0;
        else
            pm_usage( usage );
        ++argn;
        }

    if ( argn != argc )
    {
    ifp = pm_openr( argv[argn] );
    ++argn;
    }
    else
    ifp = stdin;

    if ( argn != argc )
    pm_usage( usage );

    /* normalize mask elements */
    fmask[4] = weight;
    fmask[0] = fmask[1] = fmask[2] = fmask[3] = ( 1.0 - weight ) / 8.0;
    fmask[5] = fmask[6] = fmask[7] = fmask[8] = ( 1.0 - weight ) / 8.0;

    pnm_readpnminit( ifp, &cols, &rows, &maxval, &format );
   
    xelrow[0] = pnm_allocrow( cols );
    xelrow[1] = pnm_allocrow( cols );
    xelrow[2] = pnm_allocrow( cols );
    newxelrow = pnm_allocrow( cols );

    /* Promote PBM files to PGM. */
    if ( PNM_FORMAT_TYPE(format) == PBM_TYPE ) {
        newformat = PGM_TYPE;
        newmaxval = PGM_MAXMAXVAL;
        pm_message( "promoting from PBM to PGM" );
    } else {
        newformat = format;
        newmaxval = maxval;
    }

    /* Figure out foreground pixel value if none was given */
    if (PPM_GETR(fgcolorppm) == 0 && PPM_GETG(fgcolorppm) == 0 && 
        PPM_GETB(fgcolorppm) == 0 ) {
        if ( PNM_FORMAT_TYPE(newformat) == PGM_TYPE )
            PNM_ASSIGN1( fgcolor, newmaxval );
        else 
            PPM_ASSIGN( fgcolor, newmaxval, newmaxval, newmaxval );
    } else {
        if ( PNM_FORMAT_TYPE(newformat) == PGM_TYPE )
            PNM_ASSIGN1( fgcolor, PPM_GETR( fgcolorppm ) );
        else 
            fgcolor = fgcolorppm;
    }

    if (PPM_GETR(bgcolorppm) != 0 || PPM_GETG(bgcolorppm) != 0 || 
        PPM_GETB(bgcolorppm) != 0 ) {
        if ( PNM_FORMAT_TYPE(newformat) == PGM_TYPE )
            PNM_ASSIGN1( bgcolor, PPM_GETR( bgcolorppm) );
        else 
            bgcolor = bgcolorppm;
    } else {
        if ( PNM_FORMAT_TYPE(newformat) == PGM_TYPE )
            PNM_ASSIGN1( bgcolor, 0 );
        else 
            PPM_ASSIGN( bgcolor, 0, 0, 0 );
    }


    pnm_readpnmrow( ifp, xelrow[0], cols, newmaxval, format );
    pnm_readpnmrow( ifp, xelrow[1], cols, newmaxval, format );
    pnm_writepnminit( stdout, cols, rows, newmaxval, newformat, 0 );
    pnm_writepnmrow( stdout, xelrow[0], cols, newmaxval, newformat, 0 );

    for ( row = 1; row < rows - 1; ++row ) {
        int col;
        int value, valuer, valueg, valueb;

        pnm_readpnmrow( ifp, xelrow[(row+1)%3], cols, newmaxval, format );
        newxelrow[0] = xelrow[row%3][0];
        
        for ( col = 1, xpP = (xelrow[(row-1)%3] + 1), xP = (xelrow[row%3] + 1),
                  xnP = (xelrow[(row+1)%3] + 1), nxP = (newxelrow+1); 
              col < cols - 1; ++col, ++xpP, ++xP, ++xnP, ++nxP ) {

            int fgflag, bgflag;

            /* Reset flags if anti-aliasing is to be done on foreground
             * or background pixels only */
            if ( ( bgonly && PNM_EQUAL( *xP, fgcolor ) ) ||
                 ( fgonly && PNM_EQUAL( *xP, bgcolor ) ) ) 
                bgflag = fgflag = 0;
            else {
                /* Do anti-aliasing here: see if pixel is at the border of a
                 * background or foreground stepwise side */
                bgflag = 
                    (PNM_EQUAL(*xpP,bgcolor) && PNM_EQUAL(*(xP+1),bgcolor)) ||
                    (PNM_EQUAL(*(xP+1),bgcolor) && PNM_EQUAL(*xnP,bgcolor)) ||
                    (PNM_EQUAL(*xnP,bgcolor) && PNM_EQUAL(*(xP-1),bgcolor)) ||
                    (PNM_EQUAL(*(xP-1),bgcolor) && PNM_EQUAL(*xpP,bgcolor));
                fgflag = 
                    (PNM_EQUAL(*xpP,fgcolor) && PNM_EQUAL(*(xP+1),fgcolor)) ||
                    (PNM_EQUAL(*(xP+1),fgcolor) && PNM_EQUAL(*xnP,fgcolor)) ||
                    (PNM_EQUAL(*xnP,fgcolor) && PNM_EQUAL(*(xP-1),fgcolor)) ||
                    (PNM_EQUAL(*(xP-1),fgcolor) && PNM_EQUAL(*xpP,fgcolor)); 
            }
            if ( ( bgflag && bgalias ) || ( fgflag && fgalias ) || 
                 ( bgflag && fgflag ) )
                switch( PNM_FORMAT_TYPE( newformat ) ) {   
                case PGM_TYPE:
                    value = PNM_GET1(*(xpP-1)) * fmask[0] +
                        PNM_GET1(*(xpP  )) * fmask[1] + 
                        PNM_GET1(*(xpP+1)) * fmask[2] +
                        PNM_GET1(*(xP -1)) * fmask[3] +
                        PNM_GET1(*(xP   )) * fmask[4] +
                        PNM_GET1(*(xP +1)) * fmask[5] +
                        PNM_GET1(*(xnP-1)) * fmask[6] +
                        PNM_GET1(*(xnP  )) * fmask[7] +
                        PNM_GET1(*(xnP+1)) * fmask[8] +
                        0.5;
                    PNM_ASSIGN1( *nxP, value );
                    break;
                default:
                    valuer= PPM_GETR(*(xpP-1)) * fmask[0] +
                        PPM_GETR(*(xpP  )) * fmask[1] + 
                        PPM_GETR(*(xpP+1)) * fmask[2] +
                        PPM_GETR(*(xP -1)) * fmask[3] +
                        PPM_GETR(*(xP   )) * fmask[4] +
                        PPM_GETR(*(xP +1)) * fmask[5] +
                        PPM_GETR(*(xnP-1)) * fmask[6] +
                        PPM_GETR(*(xnP  )) * fmask[7] +
                        PPM_GETR(*(xnP+1)) * fmask[8] +
                        0.5;
                    valueg= PPM_GETG(*(xpP-1)) * fmask[0] +
                        PPM_GETG(*(xpP  )) * fmask[1] + 
                        PPM_GETG(*(xpP+1)) * fmask[2] +
                        PPM_GETG(*(xP -1)) * fmask[3] +
                        PPM_GETG(*(xP   )) * fmask[4] +
                        PPM_GETG(*(xP +1)) * fmask[5] +
                        PPM_GETG(*(xnP-1)) * fmask[6] +
                        PPM_GETG(*(xnP  )) * fmask[7] +
                        PPM_GETG(*(xnP+1)) * fmask[8] +
                        0.5;
                    valueb= PPM_GETB(*(xpP-1)) * fmask[0] +
                        PPM_GETB(*(xpP  )) * fmask[1] + 
                        PPM_GETB(*(xpP+1)) * fmask[2] +
                        PPM_GETB(*(xP -1)) * fmask[3] +
                        PPM_GETB(*(xP   )) * fmask[4] +
                        PPM_GETB(*(xP +1)) * fmask[5] +
                        PPM_GETB(*(xnP-1)) * fmask[6] +
                        PPM_GETB(*(xnP  )) * fmask[7] +
                        PPM_GETB(*(xnP+1)) * fmask[8] +
                        0.5;
                    PPM_ASSIGN( *nxP, valuer, valueg, valueb );
                    break;
                }
            else
                *nxP = *xP;
        }

        newxelrow[cols-1] = xelrow[row%3][cols-1];
        pnm_writepnmrow( stdout, newxelrow, cols, newmaxval, newformat, 0 );
    }
        
    pnm_writepnmrow( stdout, xelrow[row%3], cols, newmaxval, newformat, 0 );
    
    pm_close( ifp );
    exit ( 0 );
}

