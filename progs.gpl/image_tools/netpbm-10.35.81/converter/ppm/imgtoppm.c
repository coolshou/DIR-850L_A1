/* imgtoppm.c - read an Img-whatnot file and produce a portable pixmap
**
** Based on a simple conversion program posted to comp.graphics by Ed Falk.
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <string.h>
#include "ppm.h"



int
main(int argc, char ** argv) {

    FILE* ifp;
    pixel* pixelrow;
    pixel colormap[256];
    register pixel* pP;
    unsigned int cols;
    unsigned int rows;
    int argn, row, i;
    int col;
    pixval maxval;
    unsigned int cmaplen;
    int len, gotAT, gotCM, gotPD;
    unsigned char buf[4096];
    unsigned char* bP;


    ppm_init( &argc, argv );

    argn = 1;

    if ( argn < argc )
    {
        ifp = pm_openr( argv[argn] );
        argn++;
    }
    else
        ifp = stdin;

    if ( argn != argc )
        pm_usage( "[imgfile]" );

    /* Get signature. */
    fread( buf, 8, 1, ifp );
    buf[8] = '\0';

    /* Get entries. */
    gotAT = 0;
    gotCM = 0;
    gotPD = 0;
    while ( fread( buf, 2, 1, ifp ) == 1 )
    {
        if ( strncmp( (char*) buf, "AT", 2 ) == 0 )
        {
            if ( fread( buf, 8, 1, ifp ) != 1 )
                pm_error( "bad attributes header" );
            buf[8] = '\0';
            len = atoi( (char*) buf );
            if ( fread( buf, len, 1, ifp ) != 1 )
                pm_error( "bad attributes buf" );
            buf[len] = '\0';
            sscanf( (char*) buf, "%4u%4u%4u", &cols, &rows, &cmaplen );
            maxval = 255;
            gotAT = 1;
        }

        else if ( strncmp( (char*) buf, "CM", 2 ) == 0 )
        {
            if ( ! gotAT )
                pm_error( "missing attributes header" );
            if ( fread( buf, 8, 1, ifp ) != 1 )
                pm_error( "bad colormap header" );
            buf[8] = '\0';
            len = atoi((char*) buf );
            if ( fread( buf, len, 1, ifp ) != 1 )
                pm_error( "bad colormap buf" );
            if ( cmaplen * 3 != len )
            {
                pm_message(
                    "cmaplen (%d) and colormap buf length (%d) do not match",
                    cmaplen, len );
                if ( cmaplen * 3 < len )
                    len = cmaplen * 3;
                else if ( cmaplen * 3 > len )
                    cmaplen = len / 3;
            }
            for ( i = 0; i < len; i += 3 )
                PPM_ASSIGN( colormap[i / 3], buf[i], buf[i + 1], buf[i + 2] );
            gotCM = 1;
        }

        else if ( strncmp( (char*) buf, "PD", 2 ) == 0 )
        {
            if ( fread( buf, 8, 1, ifp ) != 1 )
                pm_error( "bad pixel data header" );
            buf[8] = '\0';
            len = atoi((char*) buf );
            if ( len != cols * rows )
                pm_message(
                    "pixel data length (%d) does not match image size (%d)",
                    len, cols * rows );

            ppm_writeppminit( stdout, cols, rows, maxval, 0 );
            pixelrow = ppm_allocrow( cols );

            for ( row = 0; row < rows; row++ )
            {
                if ( fread( buf, 1, cols, ifp ) != cols )
                    pm_error( "EOF / read error" );
                for ( col = 0, pP = pixelrow, bP = buf;
                      col < cols; col++, pP++, bP++ )
                {
                    if ( gotCM )
                        *pP = colormap[*bP];
                    else
                        PPM_ASSIGN( *pP, *bP, *bP, *bP );
                }
                ppm_writeppmrow( stdout, pixelrow, cols, maxval, 0 );
            }
            gotPD = 1;

        }
    }
    if ( ! gotPD )
        pm_error( "missing pixel data header" );

    pm_close( ifp );
    /* If the program failed, it previously aborted with nonzero completion
       code, via various function calls.
    */
    return 0;
}
