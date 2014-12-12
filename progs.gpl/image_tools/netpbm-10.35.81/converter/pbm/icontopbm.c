/* icontopbm.c - read a Sun icon file and produce a portable bitmap
**
** Copyright (C) 1988 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <string.h>

#include "nstring.h"
#include "pbm.h"

/* size in bytes of a bitmap */
#define BitmapSize(width, height) (((((width) + 15) >> 3) &~ 1) * (height))

static void
ReadIconFile(FILE *                const file, 
             int *                 const widthP, 
             int *                 const heightP, 
             short unsigned int ** const dataP) {

    char variable[80+1];
    int ch;
    int status, value, i, data_length, gotsome;

    gotsome = 0;
    *widthP = *heightP = -1;
    for ( ; ; )
    {
        while ( ( ch = getc( file ) ) == ',' || ch == '\n' || ch == '\t' ||
                ch == ' ' )
            ;
        for ( i = 0;
              ch != '=' && ch != ',' && ch != '\n' && ch != '\t' && 
                  ch != ' ' && (i < (sizeof(variable) - 1));
              i++ )
        {
            variable[i] = ch;
            if ((ch = getc( file )) == EOF)
                pm_error( "invalid input file -- premature EOF" );
        }
        variable[i] = '\0';

        if ( STREQ( variable, "*/" )&& gotsome )
            break;

        if ( fscanf( file, "%d", &value ) != 1 )
            continue;

        if ( STREQ( variable, "Width" ) )
        {
            *widthP = value;
            gotsome = 1;
        }
        else if ( STREQ( variable, "Height" ) )
        {
            *heightP = value;
            gotsome = 1;
        }
        else if ( STREQ( variable, "Depth" )  )
        {
            if ( value != 1 )
                pm_error( "invalid depth" );
            gotsome = 1;
        }
        else if ( STREQ( variable, "Format_version" ) )
        {
            if ( value != 1 )
                pm_error( "invalid Format_version" );
            gotsome = 1;
        }
        else if ( STREQ( variable, "Valid_bits_per_item" ) )
        {
            if ( value != 16 )
                pm_error( "invalid Valid_bits_per_item" );
            gotsome = 1;
        }
    }

    if ( *widthP <= 0 )
        pm_error( "invalid width (must be positive): %d", *widthP );
    if ( *heightP <= 0 )
        pm_error( "invalid height (must be positive): %d", *heightP );

    data_length = BitmapSize( *widthP, *heightP );
    *dataP = (short unsigned int *) malloc( data_length );
    if ( *dataP == NULL )
        pm_error( "out of memory" );
    data_length /= sizeof( short );
    
    for ( i = 0 ; i < data_length; i++ )
    {
        if ( i == 0 )
            status = fscanf( file, " 0x%4hx", *dataP );
        else
            status = fscanf( file, ", 0x%4hx", *dataP + i );
        if ( status != 1 )
            pm_error( "error 4 scanning bits item" );
    }
}



int
main(int  argc, char ** argv) {

    FILE* ifp;
    bit* bitrow;
    register bit* bP;
    int rows, cols, row, col, shortcount, mask;
    short unsigned int * data;


    pbm_init( &argc, argv );

    if ( argc > 2 )
        pm_usage( "[iconfile]" );

    if ( argc == 2 )
        ifp = pm_openr( argv[1] );
    else
        ifp = stdin;

    ReadIconFile( ifp, &cols, &rows, &data );

    pm_close( ifp );

    pbm_writepbminit( stdout, cols, rows, 0 );
    bitrow = pbm_allocrow( cols );

    for ( row = 0; row < rows; row++ )
    {
        shortcount = 0;
        mask = 0x8000;
        for ( col = 0, bP = bitrow; col < cols; col++, bP++ )
        {
            if ( shortcount >= 16 )
            {
                data++;
                shortcount = 0;
                mask = 0x8000;
            }
            *bP = ( *data & mask ) ? PBM_BLACK : PBM_WHITE;
            shortcount++;
            mask = mask >> 1;
        }
        data++;
        pbm_writepbmrow( stdout, bitrow, cols, 0 );
    }

    pm_close( stdout );
    exit( 0 );
}

