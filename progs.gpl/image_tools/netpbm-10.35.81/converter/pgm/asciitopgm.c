/* asciitopgm.c - read an ASCII graphics file and produce a portable graymap
**
** Copyright (C) 1989 by Wilson H. Bent, Jr
**
** - Based on fstopgm.c and other works which bear the following notice:
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

#include "pm_c_util.h"
#include "pgm.h"
#include "mallocvar.h"
#include "nstring.h"

static char gmap [128] = {
/*00 nul-bel*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*08 bs -si */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*10 dle-etb*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*18 can-us */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*20 sp - ' */  0x00, 0x21, 0x1b, 0x7f, 0x70, 0x25, 0x20, 0x0a,
/*28  ( - / */  0x11, 0x11, 0x2a, 0x2b, 0x0b, 0x13, 0x04, 0x10,
/*30  0 - 7 */  0x30, 0x28, 0x32, 0x68, 0x39, 0x35, 0x39, 0x16,
/*38  8 - ? */  0x38, 0x39, 0x14, 0x15, 0x11, 0x1c, 0x11, 0x3f,
/*40  @ - G */  0x40, 0x49, 0x52, 0x18, 0x44, 0x3c, 0x38, 0x38,
/*48  H - O */  0x55, 0x28, 0x2a, 0x70, 0x16, 0x7f, 0x70, 0x14,
/*50  P - W */  0x60, 0x20, 0x62, 0x53, 0x1a, 0x55, 0x36, 0x57,
/*58  X - _ */  0x50, 0x4c, 0x5a, 0x24, 0x10, 0x24, 0x5e, 0x13,
/*60  ` - g */  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
/*68  h - o */  0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x2a,
/*70  p - w */  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
/*78  x -del*/  0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
};

static gray maxval = 127;

int
main( argc, argv )
    int argc;
    char *argv[];
{
    FILE *ifd;
    register gray **grays;
    int argn, row;
    register int c, i;
    int rows = 0, cols = 0;
    int divisor = 1;
    bool warned;
    int *obuf;
    const char * const usage = "[-d <val>] height width [asciifile]";
    char trunc;

    pgm_init( &argc, argv );

    warned = FALSE;

    argn = 1;

    if ( argc < 3 || argc > 6 )
        pm_usage( usage );

    if ( argv[argn][0] == '-' )
    {
        if ( STREQ( argv[argn], "-d" ) )
        {
            if ( argc == argn + 1 )
                pm_usage( usage );
            if ( sscanf( argv[argn+1], "%d", &divisor ) != 1 )
                pm_usage( usage );
            argn += 2;
        }
        else
            pm_usage( usage );
    }

    if ( sscanf( argv[argn++], "%d", &rows ) != 1 )
        pm_usage( usage );
    if ( sscanf( argv[argn++], "%d", &cols ) != 1 )
        pm_usage( usage );
    if ( rows < 1 )
        pm_error( "height is less than 1" );
    if ( cols < 1 )
        pm_error( "width is less than 1" );

    if ( argc > argn + 1 )
        pm_usage( usage );

    if ( argc == argn + 1 )
        ifd = pm_openr( argv[argn] );
    else
        ifd = stdin;

    /* Here's where the work is done:
     * - Usually, the graymap value of the input char is summed into grays
     * - For a 'normal' newline, the current row is adjusted by the divisor
     *   and the current row is incremented
     * - If the first char in the input line is a '+', then the current row
     *   stays the same to allow 'overstriking'
     * NOTE that we assume the user specified a sufficiently large width!
     */
    MALLOCARRAY( obuf, cols );
    if ( obuf == NULL )
        pm_error( "Unable to allocate memory for %d columns.", cols);
    else {
        unsigned int col;
        for (col = 0; col < cols; ++col) obuf[col] = 0;
    }
    grays = pgm_allocarray( cols, rows );
    row = i = trunc = 0;
    while ( row < rows )
    {
        switch (c = getc (ifd))
        {
        case EOF:
            goto line_done;
        case '\n':
	newline:
	    trunc = 0;
            if ((c = getc (ifd)) == EOF)
                goto line_done;
            if (c == '+')
                i = 0;
            else
            {
            line_done:
                for (i = 0; i < cols; ++i)
                    grays[row][i] = maxval - (obuf[i] / divisor);
                {
                    unsigned int col;
                    for (col = 0; col < cols; ++col) obuf[col] = 0;
                }
                i = 0;
                ++row;
                if ( row >= rows )
                    break;
		if (c == '\n')
		    goto newline;
                else if (c != EOF)
                    obuf[i++] += gmap[c];
            }
            break;
        default:
	    if (i == cols)
	    {
		if (! trunc)
		{
		    pm_message("Warning: row %d being truncated at %d columns",
			       row+1, cols);
		    trunc = 1;
		}
		continue;
	    }
            if (c > 0x7f)       /* !isascii(c) */
            {
                if (!warned)
                {
                    pm_message("Warning: non-ASCII char(s) in input");
                    warned = TRUE;
                }
                c &= 0x7f;      /* toascii(c) */
            }
            obuf[i++] += gmap[c];
            break;
        }
    }
    pm_close( ifd );

    pgm_writepgm( stdout, grays, cols, rows, maxval, 0 );

    return 0;
}
