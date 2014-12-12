/* pbmtopsg3

   Reads a series of PBM images and writes a Postscript program
   containing these images as individual pages with Fax-G3 
   (CCITT-Fiter) compression. (Useful for combining scanned pages into
   a comfortably printable document.)

   Copyright (C) 2001 Kristof Koehler 
       <kristof@fachschaft.physik.uni-karlsruhe.de>

   Netpbm adaptation by Bryan Henderson June 2001.
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "pbm.h"
#include "shhopt.h"

struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilespec;  /* Filespec of input file */
    float        dpi;            /* requested resolution, dpi */
    char *       title;          /* -title option.  NULL for none */
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdline_info *cmdlineP) {

    optStruct3 opt;
    unsigned int option_def_index = 0;
    optEntry *option_def = malloc(100*sizeof(optEntry));

    unsigned int dpiSpec, titleSpec;
    float dpiOpt;
    char * titleOpt;

    OPTENT3(0, "dpi",      OPT_FLOAT,  &dpiOpt,   &dpiSpec,   0);
    OPTENT3(0, "title",    OPT_STRING, &titleOpt, &titleSpec, 0);
    
    opt.opt_table = option_def;
    opt.short_allowed = FALSE;
    opt.allowNegNum = FALSE;

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

    if (argc-1 == 0)
        cmdlineP->inputFilespec = "-";
    else {
        cmdlineP->inputFilespec = argv[1];
        if (argc-1 > 1)
            pm_error("Too many arguments.  The only argument is the input "
                     "file specification");
    }

    cmdlineP->dpi = dpiSpec ? dpiOpt : 72.0;
    cmdlineP->title = titleSpec ? titleOpt : NULL;
}

    

static void 
write85 ( unsigned int bits, int *col )
{
	char buf[5] ;
	if ( bits == 0 ) {
		fputc ( 'z', stdout ) ;
		*col += 1 ;
	} else {
		buf[4] = bits % 85 + '!' ;
		bits /= 85 ;
		buf[3] = bits % 85 + '!' ;
		bits /= 85 ;
		buf[2] = bits % 85 + '!' ;
		bits /= 85 ;
		buf[1] = bits % 85 + '!' ;
		bits /= 85 ;
		buf[0] = bits % 85 + '!' ;
		fwrite ( buf, 1, 5, stdout ) ;
		*col += 5 ;
	}
	if ( *col > 70 ) {
		printf ( "\n" ) ;
		*col = 0 ;
	}
}


static void 
writebits ( unsigned int *outbits, int *outbitsidx, int *col,
            unsigned int bits, int n )
{
	int k, m ;
	unsigned int usedbits ;
	while ( n > 0 ) {
		if ( *outbitsidx == 0 )
			*outbits = 0 ;
		k = 32 - *outbitsidx ;
		m = n > k ? k : n ;
		usedbits = (bits >> (n-m)) & ((1<<m)-1) ;
		*outbits |= usedbits << (k-m) ;
		*outbitsidx += m ;
		n -= m ;
		if ( *outbitsidx == 32 ) {
			write85 ( *outbits, col ) ;
			*outbitsidx = 0 ;
		}
	}
}


static void 
flushbits ( unsigned int *outbits, int *outbitsidx, int *col )
{
	if ( *outbitsidx > 0 ) {
		write85 ( *outbits, col ) ;
		*outbitsidx = 0 ;
	}
}

struct { unsigned int b, l ; } makeup[40][2] = {
    { { 0x001b, 5 } /*         11011 */ , { 0x000f,10 } /*    0000001111 */  },
    { { 0x0012, 5 } /*         10010 */ , { 0x00c8,12 } /*  000011001000 */  },
    { { 0x0017, 6 } /*        010111 */ , { 0x00c9,12 } /*  000011001001 */  },
    { { 0x0037, 7 } /*       0110111 */ , { 0x005b,12 } /*  000001011011 */  },
    { { 0x0036, 8 } /*      00110110 */ , { 0x0033,12 } /*  000000110011 */  },
    { { 0x0037, 8 } /*      00110111 */ , { 0x0034,12 } /*  000000110100 */  },
    { { 0x0064, 8 } /*      01100100 */ , { 0x0035,12 } /*  000000110101 */  },
    { { 0x0065, 8 } /*      01100101 */ , { 0x006c,13 } /* 0000001101100 */  },
    { { 0x0068, 8 } /*      01101000 */ , { 0x006d,13 } /* 0000001101101 */  },
    { { 0x0067, 8 } /*      01100111 */ , { 0x004a,13 } /* 0000001001010 */  },
    { { 0x00cc, 9 } /*     011001100 */ , { 0x004b,13 } /* 0000001001011 */  },
    { { 0x00cd, 9 } /*     011001101 */ , { 0x004c,13 } /* 0000001001100 */  },
    { { 0x00d2, 9 } /*     011010010 */ , { 0x004d,13 } /* 0000001001101 */  },
    { { 0x00d3, 9 } /*     011010011 */ , { 0x0072,13 } /* 0000001110010 */  },
    { { 0x00d4, 9 } /*     011010100 */ , { 0x0073,13 } /* 0000001110011 */  },
    { { 0x00d5, 9 } /*     011010101 */ , { 0x0074,13 } /* 0000001110100 */  },
    { { 0x00d6, 9 } /*     011010110 */ , { 0x0075,13 } /* 0000001110101 */  },
    { { 0x00d7, 9 } /*     011010111 */ , { 0x0076,13 } /* 0000001110110 */  },
    { { 0x00d8, 9 } /*     011011000 */ , { 0x0077,13 } /* 0000001110111 */  },
    { { 0x00d9, 9 } /*     011011001 */ , { 0x0052,13 } /* 0000001010010 */  },
    { { 0x00da, 9 } /*     011011010 */ , { 0x0053,13 } /* 0000001010011 */  },
    { { 0x00db, 9 } /*     011011011 */ , { 0x0054,13 } /* 0000001010100 */  },
    { { 0x0098, 9 } /*     010011000 */ , { 0x0055,13 } /* 0000001010101 */  },
    { { 0x0099, 9 } /*     010011001 */ , { 0x005a,13 } /* 0000001011010 */  },
    { { 0x009a, 9 } /*     010011010 */ , { 0x005b,13 } /* 0000001011011 */  },
    { { 0x0018, 6 } /*        011000 */ , { 0x0064,13 } /* 0000001100100 */  },
    { { 0x009b, 9 } /*     010011011 */ , { 0x0065,13 } /* 0000001100101 */  },
    { { 0x0008,11 } /*   00000001000 */ , { 0x0008,11 } /*   00000001000 */  },
    { { 0x000c,11 } /*   00000001100 */ , { 0x000c,11 } /*   00000001100 */  },
    { { 0x000d,11 } /*   00000001101 */ , { 0x000d,11 } /*   00000001101 */  },
    { { 0x0012,12 } /*  000000010010 */ , { 0x0012,12 } /*  000000010010 */  },
    { { 0x0013,12 } /*  000000010011 */ , { 0x0013,12 } /*  000000010011 */  },
    { { 0x0014,12 } /*  000000010100 */ , { 0x0014,12 } /*  000000010100 */  },
    { { 0x0015,12 } /*  000000010101 */ , { 0x0015,12 } /*  000000010101 */  },
    { { 0x0016,12 } /*  000000010110 */ , { 0x0016,12 } /*  000000010110 */  },
    { { 0x0017,12 } /*  000000010111 */ , { 0x0017,12 } /*  000000010111 */  },
    { { 0x001c,12 } /*  000000011100 */ , { 0x001c,12 } /*  000000011100 */  },
    { { 0x001d,12 } /*  000000011101 */ , { 0x001d,12 } /*  000000011101 */  },
    { { 0x001e,12 } /*  000000011110 */ , { 0x001e,12 } /*  000000011110 */  },
    { { 0x001f,12 } /*  000000011111 */ , { 0x001f,12 } /*  000000011111 */  }
} ;

struct { unsigned int b, l ; } term[64][2] = {
    { { 0x0035, 8 } /*      00110101 */ , { 0x0037,10 } /*    0000110111 */  },
    { { 0x0007, 6 } /*        000111 */ , { 0x0002, 3 } /*           010 */  },
    { { 0x0007, 4 } /*          0111 */ , { 0x0003, 2 } /*            11 */  },
    { { 0x0008, 4 } /*          1000 */ , { 0x0002, 2 } /*            10 */  },
    { { 0x000b, 4 } /*          1011 */ , { 0x0003, 3 } /*           011 */  },
    { { 0x000c, 4 } /*          1100 */ , { 0x0003, 4 } /*          0011 */  },
    { { 0x000e, 4 } /*          1110 */ , { 0x0002, 4 } /*          0010 */  },
    { { 0x000f, 4 } /*          1111 */ , { 0x0003, 5 } /*         00011 */  },
    { { 0x0013, 5 } /*         10011 */ , { 0x0005, 6 } /*        000101 */  },
    { { 0x0014, 5 } /*         10100 */ , { 0x0004, 6 } /*        000100 */  },
    { { 0x0007, 5 } /*         00111 */ , { 0x0004, 7 } /*       0000100 */  },
    { { 0x0008, 5 } /*         01000 */ , { 0x0005, 7 } /*       0000101 */  },
    { { 0x0008, 6 } /*        001000 */ , { 0x0007, 7 } /*       0000111 */  },
    { { 0x0003, 6 } /*        000011 */ , { 0x0004, 8 } /*      00000100 */  },
    { { 0x0034, 6 } /*        110100 */ , { 0x0007, 8 } /*      00000111 */  },
    { { 0x0035, 6 } /*        110101 */ , { 0x0018, 9 } /*     000011000 */  },
    { { 0x002a, 6 } /*        101010 */ , { 0x0017,10 } /*    0000010111 */  },
    { { 0x002b, 6 } /*        101011 */ , { 0x0018,10 } /*    0000011000 */  },
    { { 0x0027, 7 } /*       0100111 */ , { 0x0008,10 } /*    0000001000 */  },
    { { 0x000c, 7 } /*       0001100 */ , { 0x0067,11 } /*   00001100111 */  },
    { { 0x0008, 7 } /*       0001000 */ , { 0x0068,11 } /*   00001101000 */  },
    { { 0x0017, 7 } /*       0010111 */ , { 0x006c,11 } /*   00001101100 */  },
    { { 0x0003, 7 } /*       0000011 */ , { 0x0037,11 } /*   00000110111 */  },
    { { 0x0004, 7 } /*       0000100 */ , { 0x0028,11 } /*   00000101000 */  },
    { { 0x0028, 7 } /*       0101000 */ , { 0x0017,11 } /*   00000010111 */  },
    { { 0x002b, 7 } /*       0101011 */ , { 0x0018,11 } /*   00000011000 */  },
    { { 0x0013, 7 } /*       0010011 */ , { 0x00ca,12 } /*  000011001010 */  },
    { { 0x0024, 7 } /*       0100100 */ , { 0x00cb,12 } /*  000011001011 */  },
    { { 0x0018, 7 } /*       0011000 */ , { 0x00cc,12 } /*  000011001100 */  },
    { { 0x0002, 8 } /*      00000010 */ , { 0x00cd,12 } /*  000011001101 */  },
    { { 0x0003, 8 } /*      00000011 */ , { 0x0068,12 } /*  000001101000 */  },
    { { 0x001a, 8 } /*      00011010 */ , { 0x0069,12 } /*  000001101001 */  },
    { { 0x001b, 8 } /*      00011011 */ , { 0x006a,12 } /*  000001101010 */  },
    { { 0x0012, 8 } /*      00010010 */ , { 0x006b,12 } /*  000001101011 */  },
    { { 0x0013, 8 } /*      00010011 */ , { 0x00d2,12 } /*  000011010010 */  },
    { { 0x0014, 8 } /*      00010100 */ , { 0x00d3,12 } /*  000011010011 */  },
    { { 0x0015, 8 } /*      00010101 */ , { 0x00d4,12 } /*  000011010100 */  },
    { { 0x0016, 8 } /*      00010110 */ , { 0x00d5,12 } /*  000011010101 */  },
    { { 0x0017, 8 } /*      00010111 */ , { 0x00d6,12 } /*  000011010110 */  },
    { { 0x0028, 8 } /*      00101000 */ , { 0x00d7,12 } /*  000011010111 */  },
    { { 0x0029, 8 } /*      00101001 */ , { 0x006c,12 } /*  000001101100 */  },
    { { 0x002a, 8 } /*      00101010 */ , { 0x006d,12 } /*  000001101101 */  },
    { { 0x002b, 8 } /*      00101011 */ , { 0x00da,12 } /*  000011011010 */  },
    { { 0x002c, 8 } /*      00101100 */ , { 0x00db,12 } /*  000011011011 */  },
    { { 0x002d, 8 } /*      00101101 */ , { 0x0054,12 } /*  000001010100 */  },
    { { 0x0004, 8 } /*      00000100 */ , { 0x0055,12 } /*  000001010101 */  },
    { { 0x0005, 8 } /*      00000101 */ , { 0x0056,12 } /*  000001010110 */  },
    { { 0x000a, 8 } /*      00001010 */ , { 0x0057,12 } /*  000001010111 */  },
    { { 0x000b, 8 } /*      00001011 */ , { 0x0064,12 } /*  000001100100 */  },
    { { 0x0052, 8 } /*      01010010 */ , { 0x0065,12 } /*  000001100101 */  },
    { { 0x0053, 8 } /*      01010011 */ , { 0x0052,12 } /*  000001010010 */  },
    { { 0x0054, 8 } /*      01010100 */ , { 0x0053,12 } /*  000001010011 */  },
    { { 0x0055, 8 } /*      01010101 */ , { 0x0024,12 } /*  000000100100 */  },
    { { 0x0024, 8 } /*      00100100 */ , { 0x0037,12 } /*  000000110111 */  },
    { { 0x0025, 8 } /*      00100101 */ , { 0x0038,12 } /*  000000111000 */  },
    { { 0x0058, 8 } /*      01011000 */ , { 0x0027,12 } /*  000000100111 */  },
    { { 0x0059, 8 } /*      01011001 */ , { 0x0028,12 } /*  000000101000 */  },
    { { 0x005a, 8 } /*      01011010 */ , { 0x0058,12 } /*  000001011000 */  },
    { { 0x005b, 8 } /*      01011011 */ , { 0x0059,12 } /*  000001011001 */  },
    { { 0x004a, 8 } /*      01001010 */ , { 0x002b,12 } /*  000000101011 */  },
    { { 0x004b, 8 } /*      01001011 */ , { 0x002c,12 } /*  000000101100 */  },
    { { 0x0032, 8 } /*      00110010 */ , { 0x005a,12 } /*  000001011010 */  },
    { { 0x0033, 8 } /*      00110011 */ , { 0x0066,12 } /*  000001100110 */  },
    { { 0x0034, 8 } /*      00110100 */ , { 0x0067,12 } /*  000001100111 */  } 
} ;


static void
writelength ( unsigned int *outbits, int *outbitsidx, int *col,
              int bit, int length )
{
	while ( length >= 64 ) {
		int m = length / 64 ;
		if ( m > 40 )
			m = 40 ;
		writebits ( outbits, outbitsidx, col,
			    makeup[m-1][bit].b, makeup[m-1][bit].l ) ;
		length -= 64*m ;
	}
	writebits ( outbits, outbitsidx, col,
		    term[length][bit].b, term[length][bit].l ) ;
}



static void
doPage(FILE *       const ifP,
       unsigned int const pageNum,
       double       const dpi) {

    int cols, rows, format;
    bit * bitrow;
    unsigned int row;
    unsigned int outbits ;
    int outbitsidx, col ;

    pbm_readpbminit(ifP, &cols, &rows, &format);
        
    bitrow = pbm_allocrow(cols);
        
    pm_message("[%u]\n", pageNum);

    printf ("%%%%Page: %u %u\n", pageNum, pageNum);
    printf ("%u %u 1 [ %f 0 0 %f 0 %u ]\n"
            "{ currentfile /ASCII85Decode filter\n"
            "  << /Columns %u /Rows %u /EndOfBlock false >> "
                "/CCITTFaxDecode filter\n"
            "  image } exec\n",
            cols, rows, dpi/72.0, -dpi/72.0, rows, 
            cols, rows) ;
        
    outbitsidx = col = 0 ;
    for (row = 0 ; row < rows; ++row) {
        int lastbit, cnt ;
        unsigned int j;

        pbm_readpbmrow(ifP, bitrow, cols, format);
            
        lastbit = cnt = 0 ;
        for (j = 0; j < cols; ++j) {
            if (bitrow[j] != lastbit) {
                writelength(&outbits, &outbitsidx, &col, lastbit, cnt) ;
                lastbit = 1 ^ lastbit ;
                cnt = 0 ;
            }
            ++cnt;
        }
        writelength(&outbits, &outbitsidx, &col, lastbit, cnt);
    }
        
    flushbits(&outbits, &outbitsidx, &col) ;
    printf("~>\nshowpage\n") ;

    pbm_freerow(bitrow);
}



static void 
doPages(FILE *         const ifP,
        unsigned int * const pagesP,
        double         const dpi) {

    bool eof;
    unsigned int pagesDone;

    eof = FALSE;
    pagesDone = 0;

    while (!eof) {
        doPage(ifP, pagesDone + 1, dpi);
        ++pagesDone;
        pbm_nextimage(ifP, &eof);
    }
    *pagesP = pagesDone;
}



int
main(int    argc,
     char * argv[]) {

    FILE *ifP;
    unsigned int pages;
    
    struct cmdline_info cmdline;

    pbm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    printf ("%%!PS-Adobe-3.0\n");
    if (cmdline.title)
        printf("%%%%Title: %s\n", cmdline.title) ;
    printf ("%%%%Creator: pbmtopsg3, Copyright (C) 2001 Kristof Koehler\n"
            "%%%%Pages: (atend)\n"
            "%%%%EndComments\n") ;
    
    doPages(ifP, &pages, cmdline.dpi);

    printf ("%%%%Trailer\n"
            "%%%%Pages: %u\n"
            "%%%%EOF\n",
            pages);

    pm_close(ifP);
    pm_close(stdout);
    
    return 0;
}
