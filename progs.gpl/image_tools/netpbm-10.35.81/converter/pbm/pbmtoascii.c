/* pbmtoascii.c - read a portable bitmap and produce ASCII graphics
**
** Copyright (C) 1988, 1992 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "pbm.h"

#define SQQ '\''
#define BSQ '\\'

/* Bit-map for 1x2 mode:
**     1
**     2
*/
static char carr1x2[4] = {
/*   0    1    2    3   */
    ' ', '"', 'o', 'M' };

/* Bit-map for 2x4 mode (hex):
**     1  2
**     4  8
**     10 20
**     40 80
** The idea here is first to preserve geometry, then to show density.
*/
#define D08 'M'
#define D07 'H'
#define D06 '&'
#define D05 '$'
#define D04 '?'
static char carr2x4[256] = {
/*0  1    2   3    4   5    6   7    8   9    A   B    C   D    E   F  */
' ',SQQ, '`','"', '-',SQQ, SQQ,SQQ, '-','`', '`','`', '-','^', '^','"',/*00-0F*/
'.',':', ':',':', '|','|', '/',D04, '/','>', '/','>', '~','+', '/','*',/*10-1F*/
'.',':', ':',':', BSQ,BSQ, '<','<', '|',BSQ, '|',D04, '~',BSQ, '+','*',/*20-2F*/
'-',':', ':',':', '~',D04, '<','<', '~','>', D04,'>', '=','b', 'd','#',/*30-3F*/
'.',':', ':',':', ':','!', '/',D04, ':',':', '/',D04, ':',D04, D04,'P',/*40-4F*/
',','i', '/',D04, '|','|', '|','T', '/',D04, '/','7', 'r','}', '/','P',/*50-5F*/
',',':', ';',D04, '>',D04, 'S','S', '/',')', '|','7', '>',D05, D05,D06,/*60-6F*/
'v',D04, D04,D05, '+','}', D05,'F', '/',D05, '/',D06, 'p','D', D06,D07,/*70-7F*/
'.',':', ':',':', ':',BSQ, ':',D04, ':',BSQ, '!',D04, ':',D04, D04,D05,/*80-8F*/
BSQ,BSQ, ':',D04, BSQ,'|', '(',D05, '<','%', D04,'Z', '<',D05, D05,D06,/*90-9F*/
',',BSQ, 'i',D04, BSQ,BSQ, D04,BSQ, '|','|', '|','T', D04,BSQ, '4','9',/*A0-AF*/
'v',D04, D04,D05, BSQ,BSQ, D05,D06, '+',D05, '{',D06, 'q',D06, D06,D07,/*B0-BF*/
'_',':', ':',D04, ':',D04, D04,D05, ':',D04, D04,D05, ':',D05, D05,D06,/*C0-CF*/
BSQ,D04, D04,D05, D04,'L', D05,'[', '<','Z', '/','Z', 'c','k', D06,'R',/*D0-DF*/
',',D04, D04,D05, '>',BSQ, 'S','S', D04,D05, 'J',']', '>',D06, '1','9',/*E0-EF*/
'o','b', 'd',D06, 'b','b', D06,'6', 'd',D06, 'd',D07, '#',D07, D07,D08 /*F0-FF*/
    };



int
main( argc, argv )
int argc;
char* argv[];
    {
    FILE* ifp;
    int argn, gridx, gridy, rows, cols, format;
    int ccols, lastcol, row, subrow, subcol;
    register int col, b;
    bit* bitrow;
    register bit* bP;
    int* sig;
    register int* sP;
    char* line;
    register char* lP;
    char* carr;
    const char* usage = "[-1x2|-2x4] [pbmfile]";

    pbm_init( &argc, argv );

    /* Set up default parameters. */
    argn = 1;
    gridx = 1;
    gridy = 2;
    carr = carr1x2;

    /* Check for flags. */
    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
	{
	if ( pm_keymatch( argv[argn], "-1x2", 2 ) )
	    {
	    gridx = 1;
	    gridy = 2;
	    carr = carr1x2;
	    }
	else if ( pm_keymatch( argv[argn], "-2x4", 2 ) )
	    {
	    gridx = 2;
	    gridy = 4;
	    carr = carr2x4;
	    }
	else
	    pm_usage( usage );
	++argn;
	}

    if ( argn < argc )
	{
	ifp = pm_openr( argv[argn] );
	++argn;
	}
    else
	ifp = stdin;
    
    if ( argn != argc )
        pm_usage( usage );

    pbm_readpbminit( ifp, &cols, &rows, &format );
    ccols = ( cols + gridx - 1 ) / gridx;
    bitrow = pbm_allocrow( cols );
    sig = (int*) pm_allocrow( ccols, sizeof(int) );
    line = (char*) pm_allocrow( ccols + 1, sizeof(char) );

    for ( row = 0; row < rows; row += gridy )
	{
	/* Get a character-row's worth of sigs. */
	for ( col = 0; col < ccols; ++col )
	    sig[col] = 0;
	b = 1;
	for ( subrow = 0; subrow < gridy; ++subrow )
	    {
	    if ( row + subrow < rows )
		{
		pbm_readpbmrow( ifp, bitrow, cols, format );
		for ( subcol = 0; subcol < gridx; ++subcol )
		    {
		    for ( col = subcol, bP = &(bitrow[subcol]), sP = sig;
			  col < cols;
			  col += gridx, bP += gridx, ++sP )
			if ( *bP == PBM_BLACK )
			    *sP |= b;
		    b <<= 1;
		    }
		}
	    }
	/* Ok, now remove trailing blanks.  */
	for ( lastcol = ccols - 1; lastcol >= 0; --lastcol )
	    if ( carr[sig[lastcol]] != ' ' )
		break;
	/* Copy chars to an array and print. */
	for ( col = 0, sP = sig, lP = line; col <= lastcol; ++col, ++sP, ++lP )
	    *lP = carr[*sP];
	*lP++ = '\0';
	puts( line );
	}

    pm_close( ifp );
    pbm_freerow( bitrow );
    pm_freerow( (char*) sig );
    pm_freerow( (char*) line );

    /* If the program failed, it previously aborted with nonzero completion
       code, via various function calls.
    */
    return 0;
    }
