/* infotopam:  A program to convert Amiga Info icon files to PAM files
 * Copyright (C) 2004  Richard Griswold - griswold@acm.org
 *
 * Thanks to the following people on comp.sys.amiga.programmer for tips
 * and pointers on decoding the info file format:
 *
 *   Ben Hutchings
 *   Thomas Richter
 *   Kjetil Svalastog Matheussen
 *   Anders Melchiorsen
 *   Dirk Stoecker
 *   Ronald V.D.
 *
 * The format of the Amiga info file is as follows:
 *
 *   DiskObject header            (78 bytes)
 *   Optional DrawerData header   (56 bytes)
 *   First icon header            (20 bytes)
 *   First icon data
 *   Second icon header           (20 bytes)
 *   Second icon data
 *
 * The DiskObject header contains, among other things, the magic number
 * (0xE310), the object width and height (inside the embedded Gadget header),
 * and the version.
 *
 * Each icon header contains the icon width and height, which can be smaller
 * than the object width and height, and the number of bit-planes.
 *
 * The icon data has the following format:
 *
 *   BIT-PLANE planes, each with HEIGHT rows of (WIDTH +15) / 16 * 2 bytes
 *   length.
 *
 * So if you have a 9x3x2 icon, the icon data will look like this:
 *
 *   aaaa aaaa a000 0000
 *   aaaa aaaa a000 0000
 *   aaaa aaaa a000 0000
 *   bbbb bbbb b000 0000
 *   bbbb bbbb b000 0000
 *   bbbb bbbb b000 0000
 *
 * Where 'a' is a bit for the first bit-plane, 'b' is a bit for the second
 * bit-plane, and '0' is padding.  Thanks again to Ben Hutchings for his
 * very helpful post!
 *
 * This program uses code from "sidplay" and an older "infotoxpm" program I
 * wrote, both of which are released under GPL.
 *
 *-------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Struct to hold miscellaneous icon information */
typedef struct IconInfo_ {
    const char  *name;        /* Icon file name */
    FILE        *fp;          /* Input file pointer */

    bool         forceColor;  /* Convert 1 bitplane icon to color icon */
    unsigned int numColors;   /* Number of colors to override */
    bool         selected;    /* Converting selected (second) icon */

    bool         drawerData;  /* Icon has drawer data */
    unsigned int version;     /* Icon version */
    unsigned int width;       /* Width in pixels */
    unsigned int height;      /* Height in pixels */
    unsigned int depth;       /* Bits of color per pixel */
    pixel        colors[4];   /* Colors to use for converted icons */
    unsigned char *icon;      /* Completed icon */

} IconInfo;

/* Header for each icon image */
typedef struct IconHeader_ { /* 20 bytes */
    unsigned char pad0[4];        /* Padding (always seems to be zero) */
    unsigned char iconWidth[2];   /* Width (usually equal to Gadget width) */
    unsigned char iconHeight[2];  
    /* Height (usually equal to Gadget height -1) */
    unsigned char bpp[2];         /* Bits per pixel */
    unsigned char pad1[10];       /* ??? */
} IconHeader;

/*
 * Gadget and DiskObject structs come from the libsidplay 1.36.57 info_.h file
 * http://www.geocities.com/SiliconValley/Lakes/5147/sidplay/linux.html
 */
typedef struct DiskObject_ { /* 78 bytes (including Gadget struct) */
    unsigned char magic[2];         /* Magic number at the start of the file */
    unsigned char version[2];       /* Object version number */
    unsigned char gadget[44];       /* Copy of in memory gadget (44 by */
    unsigned char type;             /* ??? */
    unsigned char pad;              /* Pad it out to the next word boundry */
    unsigned char pDefaultTool[4];  /* Pointer  to default tool */
    unsigned char ppToolTypes[4];   /* Pointer pointer to tool types */
    unsigned char currentX[4];      /* Current X position (?) */
    unsigned char currentY[4];      /* Current Y position (?) */
    unsigned char pDrawerData[4];   /* Pointer to drawer data */
    unsigned char pToolWindow[4];   /* Ptr to tool window - only for tools */
    unsigned char stackSize[4];     /* Stack size - only for tools */
} DiskObject;



static void
parseCommandLine( int              argc,
                  char *           argv[],
                  IconInfo * const infoP ) {

    unsigned int numColorArgs,  /* Number of arguments for overriding colors */
        colorIdx,      /* Color index */
        i;             /* Argument index */
    const char  * const colors[4] = { 
        /* Pixel colors based on original Amiga colors */
        "#0055AA",    /*   Blue      0,  85, 170 */
        "#FFFFFF",    /*   White   255, 255, 255 */
        "#000020",    /*   Black     0,   0,  32 */
        "#FF8A00"     /*   Orange  255, 138,   0 */
    };

    /* Option entry variables */
    optEntry     *option_def;
    optStruct3    opt;
    unsigned int  option_def_index;
    unsigned int numColorsSpec, forceColorSpec, selectedSpec;
  
    MALLOCARRAY_NOFAIL(option_def, 100);

    /* Set command line options */
    option_def_index = 0;   /* Incremented by OPTENT3 */
    OPTENT3(0, "forcecolor", OPT_FLAG, NULL, &forceColorSpec, 0);
    OPTENT3(0, "numcolors",  OPT_UINT, &infoP->numColors, &numColorsSpec, 0);
    OPTENT3(0, "selected",   OPT_FLAG, NULL, &selectedSpec,   0);

    /* Initialize the iconInfo struct */
    infoP->name = NULL;
    infoP->fp = NULL;
    infoP->drawerData = FALSE;
    infoP->version = 0;
    infoP->width = 0;
    infoP->height = 0;
    infoP->depth = 0;
    infoP->icon = NULL;
    for ( colorIdx = 0; colorIdx < 4; colorIdx++ )
        infoP->colors[colorIdx] = 
            ppm_parsecolor( (char*) colors[colorIdx], 0xFF );

    /* Initialize option structure */
    opt.opt_table     = option_def;
    opt.short_allowed = FALSE;  /* No short (old-fashioned) options */
    opt.allowNegNum   = FALSE;  /* No negative number parameters */

    /* Parse the command line */
    optParseOptions3( &argc, argv, opt, sizeof( opt ), 0 );

    infoP->forceColor = forceColorSpec;
    infoP->selected = selectedSpec;
    if (!numColorsSpec)
        infoP->numColors = 0;

    /* Get colors and file name */
    numColorArgs = infoP->numColors * 2;
    if ( ( argc - 1 != numColorArgs ) && ( argc - 1 != numColorArgs + 1 ) ) {
        pm_error( "Wrong number of arguments for number of colors.  "
                  "For %u colors, you need %u color arguments, "
                  "with possibly one more argument for the input file name.",
                  infoP->numColors, numColorArgs );
    }

    /* Convert color arguments */
    for ( i = 1; i < numColorArgs; i += 2 ) {
        char *       endptr;        /* End pointer for strtol() */
        unsigned int colorIdx;

        /* Get color index from argument */
        colorIdx = strtoul( argv[i], &endptr, 0 );

        if ( *endptr != '\0' ) {
            pm_error( "'%s' is not a valid color index", argv[i] );
        }

        /* Check color index range (current 0 to 3) */
        if ( ( colorIdx < 0 ) || ( colorIdx > 3 ) ) {
            pm_error( "%u is not a valid color index (minimum 0, maximum 3)",
                      colorIdx );
        }

        /* Convert the color for this color index */
        infoP->colors[colorIdx] = ppm_parsecolor( argv[i+1], 0xFF );
    }

    /* Set file name */
    if ( i > argc-1 )
        infoP->name = "-";  /* Read from standard input */
    else
        infoP->name = argv[i];
}



static void
getDiskObject( IconInfo * const infoP ) {
/*-------------------------------------------------------------------------
 * Get fields from disk object portion of info file
 *-------------------------------------------------------------------------*/
    DiskObject  dobj;      /* Disk object structure */
    size_t      bytesRead;

    /* Read the disk object header */
    bytesRead = fread( &dobj, 1, sizeof(dobj), infoP->fp );
    if (ferror(infoP->fp))
        pm_error( "Cannot read disk object header for file '%s'.  "
                  "fread() errno = %d (%s)",
                  infoP->name, errno, strerror( errno ) );
    else if ( bytesRead != sizeof(dobj) )
        pm_error( "Cannot read entire disk object header for file '%s'.  "
                  "Only read 0x%X of 0x%X bytes",
                  infoP->name, bytesRead, sizeof(dobj) );

    /* Check magic number */
    if ( ( dobj.magic[0] != 0xE3 ) && ( dobj.magic[1] != 0x10 ) )
        pm_error( "Wrong magic number for file '%s'.  "
                  "Expected 0xE310, but got 0x%X%X",
                  infoP->name, dobj.magic[0], dobj.magic[1] );

    /* Set version info and have drawer data flag */
    infoP->version     = ( dobj.version[0]     <<  8 ) +
        ( dobj.version[1]           );
    infoP->drawerData  = ( dobj.pDrawerData[0] << 24 ) +
        ( dobj.pDrawerData[1] << 16 ) +
        ( dobj.pDrawerData[2] <<  8 ) +
        ( dobj.pDrawerData[3]       ) ? TRUE : FALSE;
}



static void 
getIconHeader( IconInfo * const infoP ) {
/*-------------------------------------------------------------------------
 * Get fields from icon header portion of info file
 *-------------------------------------------------------------------------*/
    IconHeader  ihead;      /* Icon header structure */
    size_t      bytesRead;

    /* Read icon header */
    bytesRead = fread( &ihead, 1, sizeof(ihead), infoP->fp );
    if ( ferror(infoP->fp ) )
         pm_error( "Cannot read icon header for file '%s'.  "
                   "fread() errno = %d (%s)",
                   infoP->name, errno, strerror( errno ) );
    else if ( bytesRead != sizeof(ihead) )
        pm_error( "Cannot read the entire icon header for file '%s'.  "
                  "Only read 0x%X of 0x%X bytes",
                  infoP->name, bytesRead, sizeof(ihead) );

    /* Get icon width, heigh, and bitplanes */
    infoP->width  = ( ihead.iconWidth[0]  << 8 ) + ihead.iconWidth[1];
    infoP->height = ( ihead.iconHeight[0] << 8 ) + ihead.iconHeight[1];
    infoP->depth  = ( ihead.bpp[0]        << 8 ) + ihead.bpp[1];

    /* Check number of bit planes */
    if ( ( infoP->depth > 2 ) || ( infoP->depth < 1 ) )
        pm_error( "We don't know how to interpret %u bitplanes file '%s'.  ",
                  infoP->depth, infoP->name );
}



static void
addBitplane(unsigned char * const icon,
            unsigned int    const bpsize,
            unsigned char * const buff) {
/*----------------------------------------------------------------------------
   Add bitplane to existing icon image
-----------------------------------------------------------------------------*/
    unsigned int i;
    unsigned int j;
    
    for (i = j = 0; i < bpsize; ++i, j += 8) {
        icon[j+0] = (icon[j+0] << 1) | ((buff[i] >> 0) & 0x01);
        icon[j+1] = (icon[j+1] << 1) | ((buff[i] >> 1) & 0x01);
        icon[j+2] = (icon[j+2] << 1) | ((buff[i] >> 2) & 0x01);
        icon[j+3] = (icon[j+3] << 1) | ((buff[i] >> 3) & 0x01);
        icon[j+4] = (icon[j+4] << 1) | ((buff[i] >> 4) & 0x01);
        icon[j+5] = (icon[j+5] << 1) | ((buff[i] >> 5) & 0x01);
        icon[j+6] = (icon[j+6] << 1) | ((buff[i] >> 6) & 0x01);
        icon[j+7] = (icon[j+7] << 1) | ((buff[i] >> 7) & 0x01);
    }
}



static void
readIconData(FILE *           const fileP,
             unsigned int     const width,
             unsigned int     const height, 
             unsigned int     const depth,
             unsigned char ** const iconP) {
/*-------------------------------------------------------------------------
 * Read icon data from file
 *-------------------------------------------------------------------------*/
    int             bitplane; /* Bitplane index */
    unsigned char * buff;     /* Buffer to hold bits for 1 bitplane */
    unsigned char * icon;

    unsigned int const bpsize = height * (((width + 15) / 16) * 2);
        /* Bitplane size in bytes, with padding */

  
    MALLOCARRAY(buff, bpsize);
    if ( buff == NULL )
        pm_error( "Cannot allocate memory to hold icon pixels" );

    MALLOCARRAY(icon, bpsize * 8);
    if (icon == NULL)
        pm_error( "Cannot allocate memory to hold icon" );

    /* Initialize to zero */
    memset(buff, 0, bpsize);
    memset(icon, 0, bpsize * 8);

    /* Each bitplane is stored independently in the icon file.  This
     * loop reads one bitplane at a time into buff.  Since fread() may
     * not read all of the bitplane on the first call, the inner loop
     * continues until all bytes are read.  The buffer pointer, bp,
     * points to the next byte in buff to fill in.  When the inner
     * loop is done, bp points to the end of buff.
     *
     * After reading in the entire bitplane, the second inner loop splits the 
     * eight pixels in each byte of the bitplane into eight separate bytes in 
     * the icon buffer.  The existing contents of each byte in icon are left 
     * shifted by one to make room for the next bit. 
     *
     * Each byte in the completed icon contains a value from 0 to
     * 2^depth (0 to 1 for depth of 1 and 0 to 3 for a depth of 3).
     * This is an index into the colors array in the info struct.  */

    for (bitplane = 0; bitplane < depth; ++bitplane) {
        /* Read bitplane into buffer */
        int toread;   /* Number of bytes left to read */
        unsigned char * buffp;    /* Buffer point for reading data */
       
        toread = bpsize; buffp = &buff[0];

        while (toread > 0) {
            size_t bytesRead;

            bytesRead = fread(buffp, 1, toread, fileP);
            if (ferror(fileP))
                pm_error("Cannot read from file info file.  "
                         "fread() errno = %d (%s)",
                         errno, strerror(errno));
            else if (bytesRead == 0)
                pm_error("Premature end-of-file.  "
                         "Still have 0x%X bytes to read",
                         toread );
            
            toread -= bytesRead;
            buffp  += bytesRead;
        }
        addBitplane(icon, bpsize, buff);
    }
    *iconP = icon;

    free(buff);
}



static void
writeIconData( IconInfo *   const infoP,
               struct pam * const pamP ) {
/*-------------------------------------------------------------------------
 * Write icon data to file
 *-------------------------------------------------------------------------*/
    unsigned int const bpwidth = ( ( infoP->width + 15 ) / 16 ) * 16;
        /* Bitplane width; Width of each row in icon, including padding */

    tuple * row;      /* Output row */

    /* Allocate row */
    row = pnm_allocpamrow( pamP );

    /* Write icon image to output file */
    /* Put if check outside for loop to reduce number of times check is made */
    if ( infoP->depth == 1 ) {
        if ( infoP->forceColor ) {
            /* Convert 1 bitplane icon into color PAM */
            unsigned int i;
            for ( i = 0; i < infoP->height; ++i ) {
                unsigned int j;
                for ( j = 0; j < infoP->width; ++j ) {
                    /* 1 is black and 0 is white */
                    unsigned int colorIdx =
                        infoP->icon[ i * bpwidth + j ] ? 2 : 1;
                    row[j][PAM_RED_PLANE] =
                        PPM_GETR( infoP->colors[colorIdx] );
                    row[j][PAM_GRN_PLANE] =
                        PPM_GETG( infoP->colors[colorIdx] );
                    row[j][PAM_BLU_PLANE] =
                        PPM_GETB( infoP->colors[colorIdx] );
                }
                pnm_writepamrow( pamP, row );
            }
        } else {
            /* Convert 1 bitplane icon into bitmap PAM */
            unsigned int i;
            for ( i = 0; i < infoP->height; ++i ) {
                unsigned int j;
                for ( j = 0; j < infoP->width; j++ ) {
                    /* 1 is black and 0 is white */
                    row[j][0] = infoP->icon[ i * bpwidth + j ] ? 0 : 1;
                }
                pnm_writepamrow( pamP, row );
            }
        }
    } else {
        /* Convert color icon into color PAM */
        unsigned int i;
        for ( i = 0; i < infoP->height; ++i ) {
            unsigned int j;
            for ( j = 0; j < infoP->width; ++j ) {
                unsigned int const colorIdx = infoP->icon[ i * bpwidth + j ];
                row[j][PAM_RED_PLANE] = PPM_GETR( infoP->colors[colorIdx] );
                row[j][PAM_GRN_PLANE] = PPM_GETG( infoP->colors[colorIdx] );
                row[j][PAM_BLU_PLANE] = PPM_GETB( infoP->colors[colorIdx] );
            }
            pnm_writepamrow( pamP, row );
        }
    }

    /* Clean up allocated memory */
    pnm_freepamrow( row );
}



int
main( int argc,
      char *argv[] ) {

    IconInfo    info;    /* Miscellaneous icon information */
    struct pam  pam;     /* PAM header */
    int         skip;    /* Bytes to skip to read next icon header */

    /* Init PNM library */
    pnm_init( &argc, argv );

    /* Parse command line arguments */
    parseCommandLine( argc, argv, &info );

    /* Open input file */
    info.fp = pm_openr( info.name );

    /* Read disk object header */
    getDiskObject( &info );

    /* Skip drawer data, if any */
    if ( info.drawerData ) {
        skip = 56;   /* Draw data size */
        if ( fseek( info.fp, skip, SEEK_CUR ) < 0 )
            pm_error( "Cannot skip header information in file '%s'.  "
                      "fseek() errno = %d (%s)",
                      info.name, errno, strerror( errno ) );
    }

    /* Get dimensions for first icon */
    getIconHeader( &info );

    /* Skip ahead to next header if converting second icon */
    if ( info.selected ) {
        skip = info.height * ( ( ( info.width + 15 ) / 16 ) * 2 ) * info.depth;

        if ( fseek( info.fp, skip, SEEK_CUR ) < 0 )
            pm_error( "Cannot skip to next icon in file '%s'.  "
                      "fseek() errno = %d (%s)",
                      info.name, errno, strerror( errno ) );

        /* Get dimensions for second icon */
        getIconHeader( &info );
    }

    /* Read icon data */
    readIconData( info.fp, info.width, info.height, info.depth, &info.icon );

    /* Print icon info */
    pm_message( "converting %s, version %d, %s icon: %d X %d X %d",
                info.name, info.version, info.selected ? "second" : "first",
                info.width, info.height, info.depth );

    /* Write PAM header */
    pam.size   = sizeof( pam );
    pam.len    = PAM_STRUCT_SIZE( tuple_type );
    pam.file   = stdout;
    pam.height = info.height;
    pam.width  = info.width;
    pam.format = PAM_FORMAT;

    if ( ( info.depth == 1 ) && ( info.forceColor == FALSE ) ) {
        pam.depth  = 1;
        pam.maxval = 1;
        strcpy( pam.tuple_type, "BLACKANDWHITE" );
    } else {
        pam.depth  = 3;
        pam.maxval = 0xFF;
        strcpy( pam.tuple_type, "RGB" );
    }
    pnm_writepaminit( &pam );

    /* Write icon data */
    writeIconData( &info, &pam );

    free( info.icon );

    /* Close input file and return */
    pm_close( pam.file );
    pm_close( info.fp );

    return 0;
}
