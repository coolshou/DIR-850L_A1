/* tgatoppm.c - read a TrueVision Targa file and write a portable pixmap
**
** Partially based on tga2rast, version 1.0, by Ian MacPhedran.
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

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <string.h>
#include "ppm.h"
#include "tga.h"
#include "shhopt.h"
#include "nstring.h"

#define MAXCOLORS 16384

static int mapped, rlencoded;

static pixel ColorMap[MAXCOLORS];
static gray AlphaMap[MAXCOLORS];
static int RLE_count = 0, RLE_flag = 0;

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filename;
    unsigned int headerdump;
    const char *alpha_filename;
    unsigned int alpha_stdout;
};



static void
parseCommandLine(int argc, char ** argv,
                   struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that many of the strings that this function returns in the
   *cmdlineP structure are actually in the supplied argv array.  And
   sometimes, one of these strings is actually just a suffix of an entry
   in argv!
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int alpha_spec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "headerdump", OPT_FLAG,   NULL, &cmdlineP->headerdump,   0);
    OPTENT3(0, "debug",      OPT_FLAG,   NULL, &cmdlineP->headerdump,   0);
    OPTENT3(0, "alphaout",   OPT_STRING, &cmdlineP->alpha_filename,
            &alpha_spec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc - 1 == 0)
        cmdlineP->input_filename = "-";  /* he wants stdin */
    else if (argc - 1 == 1)
        cmdlineP->input_filename = strdup(argv[1]);
    else 
        pm_error("Too many arguments.  The only argument accepted "
                 "is the input file specification");

    if (alpha_spec &&
        STREQ(cmdlineP->alpha_filename, "-"))
        cmdlineP->alpha_stdout = 1;
    else 
        cmdlineP->alpha_stdout = 0;

    if (!alpha_spec)
        cmdlineP->alpha_filename = NULL;
}




static unsigned char
getbyte(FILE * const ifP) {

    unsigned char c;

    if ( fread( (char*) &c, 1, 1, ifP ) != 1 )
        pm_error( "EOF / read error" );
    
    return c;
}



static void
get_pixel(FILE * const ifP, pixel * dest, int Size, gray *alpha_p) {

    static pixval Red, Grn, Blu;
    static pixval Alpha;
    unsigned char j, k;
    static unsigned int l;

    /* Check if run length encoded. */
    if ( rlencoded )
	{
	if ( RLE_count == 0 )
	    { /* Have to restart run. */
	    unsigned char i;
	    i = getbyte( ifP );
	    RLE_flag = ( i & 0x80 );
	    if ( RLE_flag == 0 )
		/* Stream of unencoded pixels. */
		RLE_count = i + 1;
	    else
		/* Single pixel replicated. */
		RLE_count = i - 127;
	    /* Decrement count & get pixel. */
	    --RLE_count;
	    }
	else
	    { /* Have already read count & (at least) first pixel. */
	    --RLE_count;
	    if ( RLE_flag != 0 )
		/* Replicated pixels. */
		goto PixEncode;
	    }
	}
    /* Read appropriate number of bytes, break into RGB. */
    switch ( Size )
	{
	case 8:				/* Grayscale, read and triplicate. */
	Red = Grn = Blu = l = getbyte( ifP );
    Alpha = 0;
	break;

	case 16:			/* 5 bits each of red green and blue. */
	case 15:			/* Watch byte order. */
	j = getbyte( ifP );
	k = getbyte( ifP );
	l = ( (unsigned int) k << 8 ) + j;
	Red = ( k & 0x7C ) >> 2;
	Grn = ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 );
	Blu = j & 0x1F;
    Alpha = 0;
	break;

	case 32:            /* 8 bits each of blue, green, red, and alpha */
	case 24:			/* 8 bits each of blue, green, and red. */
	Blu = getbyte( ifP );
	Grn = getbyte( ifP );
	Red = getbyte( ifP );
	if ( Size == 32 )
	    Alpha = getbyte( ifP );
    else
        Alpha = 0;
	l = 0;
	break;

	default:
	pm_error( "unknown pixel size (#2) - %d", Size );
	}

PixEncode:
    if ( mapped ) {
        *dest = ColorMap[l];
        *alpha_p = AlphaMap[l];
    } else {
        PPM_ASSIGN( *dest, Red, Grn, Blu );
        *alpha_p = Alpha;
    }
    }



static void
readtga(FILE * const ifP, struct ImageHeader * tgaP) {

    unsigned char flags;
    ImageIDField junk;

    tgaP->IdLength = getbyte( ifP );
    tgaP->CoMapType = getbyte( ifP );
    tgaP->ImgType = getbyte( ifP );
    tgaP->Index_lo = getbyte( ifP );
    tgaP->Index_hi = getbyte( ifP );
    tgaP->Length_lo = getbyte( ifP );
    tgaP->Length_hi = getbyte( ifP );
    tgaP->CoSize = getbyte( ifP );
    tgaP->X_org_lo = getbyte( ifP );
    tgaP->X_org_hi = getbyte( ifP );
    tgaP->Y_org_lo = getbyte( ifP );
    tgaP->Y_org_hi = getbyte( ifP );
    tgaP->Width_lo = getbyte( ifP );
    tgaP->Width_hi = getbyte( ifP );
    tgaP->Height_lo = getbyte( ifP );
    tgaP->Height_hi = getbyte( ifP );
    tgaP->PixelSize = getbyte( ifP );
    flags = getbyte( ifP );
    tgaP->AttBits = flags & 0xf;
    tgaP->Rsrvd = ( flags & 0x10 ) >> 4;
    tgaP->OrgBit = ( flags & 0x20 ) >> 5;
    tgaP->IntrLve = ( flags & 0xc0 ) >> 6;
    
    if ( tgaP->IdLength != 0 )
        fread( junk, 1, (int) tgaP->IdLength, ifP );
}



static void
get_map_entry(FILE * const ifP, pixel * Value, int Size, gray * Alpha) {

    unsigned char j, k, r, g, b, a;

    /* Read appropriate number of bytes, break into rgb & put in map. */
    switch ( Size )
	{
	case 8:				/* Grayscale, read and triplicate. */
        r = g = b = getbyte( ifP );
        a = 0;
        break;
        
	case 16:			/* 5 bits each of red green and blue. */
	case 15:			/* Watch for byte order. */
        j = getbyte( ifP );
        k = getbyte( ifP );
        r = ( k & 0x7C ) >> 2;
        g = ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 );
        b = j & 0x1F;
        a = 0;
        break;
        
	case 32:            /* 8 bits each of blue, green, red, and alpha */
	case 24:			/* 8 bits each of blue green and red. */
        b = getbyte( ifP );
        g = getbyte( ifP );
        r = getbyte( ifP );
        if ( Size == 32 )
            a = getbyte( ifP );
        else 
            a = 0;
        break;
        
	default:
        pm_error( "unknown colormap pixel size (#2) - %d", Size );
	}
    PPM_ASSIGN( *Value, r, g, b );
    *Alpha = a;
}



static void
dumpHeader(struct ImageHeader const tga_head) {
    const char * imgTypeName;
    switch(tga_head.ImgType) {
    case TGA_Map:      imgTypeName = "TGA_Map";      break;
    case TGA_RGB:      imgTypeName = "TGA_RGB";      break;
    case TGA_Mono:     imgTypeName = "TGA_Mono";     break;
    case TGA_RLEMap:   imgTypeName = "TGA_RLEMap";   break;
    case TGA_RLERGB:   imgTypeName = "TGA_RLERGB";   break;
    case TGA_RLEMono:  imgTypeName = "TGA_RLEMono";  break;
    case TGA_CompMap:  imgTypeName = "TGA_CompMap";  break;
    case TGA_CompMap4: imgTypeName = "TGA_CompMap4"; break;
    default:           imgTypeName = "unknown";
    }
    pm_message( "IdLength = %d", (int) tga_head.IdLength );
    pm_message( "CoMapType = %d", (int) tga_head.CoMapType );
    pm_message( "ImgType = %d (%s)", (int) tga_head.ImgType, imgTypeName );
    pm_message( "Index_lo = %d", (int) tga_head.Index_lo );
    pm_message( "Index_hi = %d", (int) tga_head.Index_hi );
    pm_message( "Length_lo = %d", (int) tga_head.Length_lo );
    pm_message( "Length_hi = %d", (int) tga_head.Length_hi );
    pm_message( "CoSize = %d", (int) tga_head.CoSize );
    pm_message( "X_org_lo = %d", (int) tga_head.X_org_lo );
    pm_message( "X_org_hi = %d", (int) tga_head.X_org_hi );
    pm_message( "Y_org_lo = %d", (int) tga_head.Y_org_lo );
    pm_message( "Y_org_hi = %d", (int) tga_head.Y_org_hi );
    pm_message( "Width_lo = %d", (int) tga_head.Width_lo );
    pm_message( "Width_hi = %d", (int) tga_head.Width_hi );
    pm_message( "Height_lo = %d", (int) tga_head.Height_lo );
    pm_message( "Height_hi = %d", (int) tga_head.Height_hi );
    pm_message( "PixelSize = %d", (int) tga_head.PixelSize );
    pm_message( "AttBits = %d", (int) tga_head.AttBits );
    pm_message( "Rsrvd = %d", (int) tga_head.Rsrvd );
    pm_message( "OrgBit = %d", (int) tga_head.OrgBit );
    pm_message( "IntrLve = %d", (int) tga_head.IntrLve );
}



int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    struct ImageHeader tga_head;
    FILE* ifP;
    FILE *imageout_file, *alpha_file;
    int rows, cols, row, realrow, truerow, baserow;
    int maxval;
    pixel** pixels;   /* The image array in ppm format */
    gray** alpha;     /* The alpha channel array in pgm format */

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.input_filename);

    if (cmdline.alpha_stdout)
        alpha_file = stdout;
    else if (cmdline.alpha_filename == NULL) 
        alpha_file = NULL;
    else
        alpha_file = pm_openw(cmdline.alpha_filename);
    
    if (cmdline.alpha_stdout) 
        imageout_file = NULL;
    else
        imageout_file = stdout;
    
    /* Read the Targa file header. */
    readtga(ifP, &tga_head);

    if (cmdline.headerdump) 
        dumpHeader(tga_head);

    rows = ((int) tga_head.Height_lo) + ((int) tga_head.Height_hi) * 256;
    cols = ((int) tga_head.Width_lo)  + ((int) tga_head.Width_hi)  * 256;

    switch (tga_head.ImgType) {
	case TGA_Map:
	case TGA_RGB:
	case TGA_Mono:
	case TGA_RLEMap:
	case TGA_RLERGB:
	case TGA_RLEMono:
        break;
	default:
        pm_error("unknown Targa image type %d", tga_head.ImgType);
	}
    
    if (tga_head.ImgType == TGA_Map ||
        tga_head.ImgType == TGA_RLEMap ||
        tga_head.ImgType == TGA_CompMap ||
        tga_head.ImgType == TGA_CompMap4)
	{ /* Color-mapped image */
        if (tga_head.CoMapType != 1)
            pm_error( 
                "mapped image (type %d) with color map type != 1",
                tga_head.ImgType );
        mapped = true;
        /* Figure maxval from CoSize. */
        switch (tga_head.CoSize) {
	    case 8:
	    case 24:
	    case 32:
            maxval = 255;
            break;

	    case 15:
	    case 16:
            maxval = 31;
            break;

	    default:
	    pm_error(
		"unknown colormap pixel size - %d", tga_head.CoSize );
	    }
	} else { 
        /* Not colormap, so figure maxval from PixelSize. */
        mapped = false;
        switch ( tga_head.PixelSize ) {
	    case 8:
	    case 24:
	    case 32:
            maxval = 255;
            break;
            
	    case 15:
	    case 16:
            maxval = 31;
            break;
            
	    default:
            pm_error("unknown pixel size - %d", tga_head.PixelSize);
	    }
	}
    
    /* If required, read the color map information. */
    if ( tga_head.CoMapType != 0 ) {
        unsigned int i;
        unsigned int temp1, temp2;

        temp1 = tga_head.Index_lo + tga_head.Index_hi * 256;
        temp2 = tga_head.Length_lo + tga_head.Length_hi * 256;
        if ((temp1 + temp2 + 1) >= MAXCOLORS)
            pm_error("too many colors - %d", (temp1 + temp2 + 1));
        for (i = temp1; i < (temp1 + temp2); ++i)
            get_map_entry(ifP, &ColorMap[i], (int) tga_head.CoSize,
                          &AlphaMap[i]);
	}

    /* Check run-length encoding. */
    if (tga_head.ImgType == TGA_RLEMap ||
        tga_head.ImgType == TGA_RLERGB ||
        tga_head.ImgType == TGA_RLEMono)
        rlencoded = 1;
    else
        rlencoded = 0;
    
    /* Read the Targa file body and convert to portable format. */
    pixels = ppm_allocarray( cols, rows );
    alpha = pgm_allocarray( cols, rows );
    truerow = 0;
    baserow = 0;
    for (row = 0; row < rows; ++row) {
        unsigned int col;

        realrow = truerow;
        if (tga_head.OrgBit == 0)
            realrow = rows - realrow - 1;
        
        for (col = 0; col < cols; ++col)
            get_pixel(ifP, &(pixels[realrow][col]), (int) tga_head.PixelSize,
                      &(alpha[realrow][col]));
        if (tga_head.IntrLve == TGA_IL_Four)
            truerow += 4;
        else if (tga_head.IntrLve == TGA_IL_Two)
            truerow += 2;
        else
            ++truerow;
        if (truerow >= rows)
            truerow = ++baserow;
	}
    pm_close(ifP);
    
    if (imageout_file) 
        ppm_writeppm(imageout_file, pixels, cols, rows, (pixval) maxval, 0);
    if (alpha_file)
        pgm_writepgm(alpha_file, alpha, cols, rows, (pixval) maxval, 0);
    if (imageout_file) 
        pm_close(imageout_file);
    if (alpha_file)
        pm_close(alpha_file);

    return 0;
}
