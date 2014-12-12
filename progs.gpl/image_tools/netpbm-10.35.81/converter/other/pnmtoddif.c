/*
 * $Log: pnmtoddif.c,v $
 * Revision 1.6  1993/01/25  08:14:06  neideck
 * Placed into public use form.
 *
 * Revision 1.5  1992/12/02  08:15:18  neideck
 *  Added RCS id.
 *
 */

/*
 * Author:      Burkhard Neidecker-Lutz
 *              Digital CEC Karlsruhe
 *      neideck@nestvx.enet.dec.com 

 Copyright (c) Digital Equipment Corporation, 1992

 Permission to use, copy, modify, and distribute this software and its
 documentation for any purpose and without fee is hereby granted,
 provided that the above copyright notice appear in all copies and that
 both that copyright notice and this permission notice appear in
 supporting documentation, and that the name of Digital Equipment
 Corporation not be used in advertising or publicity pertaining to
 distribution of the software without specific, written prior
 permission. Digital Equipment Corporation makes no representations
 about the suitability of this software for any purpose. It is provided
 "as is" without express or implied warranty.

 Version: 1.0           30.07.92

*/

#include <string.h>
#include "pnm.h"

/* The structure we use to convey all sorts of "magic" data to the DDIF */
/* header write procedure.                      */

typedef struct {
    int width;
    int height;
    int h_res;            /* Resolutions in dpi for bounding box */
    int v_res;
    int bits_per_pixel;
    int bytes_per_line;
    int spectral;         /* 2 == monochrome, 5 == rgb        */
    int components;
    int bits_per_component;
    int polarity;         /* zeromin == 2 , zeromax == 1      */
} imageparams;



/* ASN.1 basic encoding rules magic number */
#define UNIVERSAL 0
#define APPLICATION 1
#define CONTEXT 2
#define PRIVATE 3

#define PRIM 0
#define CONS 1

/* "tag": Emit an ASN tag of the specified class and tag number.    */
/* This is used in conjuntion with the                  */
/* wr_xxx routines that follow to construct the various ASN.1 entities. */
/* Writing each entity is a two-step process, where first the tag is    */
/* written and then the length and value.               */
/* All of these routines take a pointer to a pointer into an output */
/* buffer in the first argument and update it accordingly.      */

static void tag(unsigned char ** buffer, int cl, int constructed,
                unsigned int t)
{
    int tag_first;
    unsigned int stack[10];
    int sp;
    unsigned char *p = *buffer;

    tag_first = (cl << 6) | constructed << 5;
    if (t < 31) {         /* Short tag form   */
        *p++ = tag_first | t;
    } else {          /* Long tag form */
        *p++ = tag_first | 31;
        sp = 0;
        while (t > 0) {
            stack[sp++] = t & 0x7f; 
            t >>= 7;
        }
        while (--sp > 0) {  /* Tag values with continuation bits */
            *p++ = stack[sp] | 0x80;
        }
        *p++ = stack[0];    /* Last tag portion without continuation bit */
    }
    *buffer = p;      /* Update buffer pointer */
}



/* Emit indefinite length encoding */
static void 
ind(unsigned char **buffer)
{
    unsigned char *p = *buffer;

    *p++ = 0x80;
    *buffer = p;
}



/* Emit ASN.1 NULL */
static void 
wr_null(unsigned char **buffer)
{
    unsigned char *p = *buffer;

    *p++ = 0;
    *buffer = p;
}



/* Emit ASN.1 length only into buffer, no data */
static void 
wr_length(unsigned char ** buffer, int amount)
{
    int length;
    unsigned int mask;
    unsigned char *p = *buffer;

    length = 4;
    mask = 0xff000000;

    if (amount < 128) {
        *p++ = amount;
    } else {          /* > 127 */
        while (!(amount & mask)) {  /* Search for first non-zero byte */
            mask >>= 8;
            --length;
        }

        *p++ = length | 0x80;       /* Number of length bytes */

        while (--length >= 0) {     /* Put length bytes   */
            *p++ =(amount >> (8*length)) & 0xff;
        }

    }
    *buffer = p;
}



/* BER encode an integer and write it's length and value */
static void 
wr_int(unsigned char ** buffer, int val)
{
    int length;
    int sign;
    unsigned int mask;
    unsigned char *p = *buffer;

    if (val == 0) {
        *p++ = 1;               /* length */
        *p++ = 0;               /* value  */
    } else {
        sign = val < 0 ? 0xff : 0x00;   /* Sign bits */
        length = 4;
#ifdef __STDC__
        mask  = 0xffu << 24;
#else
        mask  = 0xff << 24;
#endif
        while ((val & mask) == sign) {  /* Find the smallest representation */
            length--;
            mask >>= 8;
        }
        sign = (0x80 << ((length-1)*8)) & val; /* New sign bit */
        if (((val < 0) && !sign) || ((val > 0) && sign)) { /* Sign error */
            length++;
        }
        *p++ = length;          /* length */
        while (--length >= 0) {
            *p++ = (val >> (8*length)) & 0xff;
        }
    }
    *buffer = p;
}



/* Emit and End Of Coding sequence  */
static void 
eoc(unsigned char ** buffer)
{
    unsigned char *p = *buffer;

    *p++ = 0;
    *p++ = 0;
    *buffer = p;
}



/* Emit a simple string */
static 
void wr_string(unsigned char ** const buffer, const char * const val)
{
    int length;
    unsigned char *p = *buffer;

    length  = strlen(val);        
    if (length > 127) {
        fprintf(stderr,"Can't encode length > 127 yet (%d)\n",length);
        exit(1);
    }
    *p++ = length;
    {
        const char * valCursor;
        for (valCursor = val; *valCursor; ++valCursor)
            *p++ = *valCursor;
    }
    *buffer = p;
}



/* Emit a ISOLATIN-1 string */
static void 
emit_isolatin1(unsigned char ** const buffer, const char * const val)
{
    int length;
    unsigned char *p = *buffer;

    length  = strlen(val) + 1;        /* One NULL byte and charset leader */
    if (length > 127) {
        fprintf(stderr,"Can't encode length > 127 yet (%d)\n",length);
        exit(1);
    }
    *p++ = length;
    *p++ = 1;             /* ISO LATIN-1 */
    {
        const char * valCursor;
        for (valCursor = val; *valCursor; ++valCursor)
            *p++ = *valCursor;
    }
    *buffer = p;
}



/* Write the DDIF grammar onto "file" up to the actual starting location */
/* of the image data. The "ip" structure needs to be set to the right    */
/* values. A lot of the values here are hardcoded to be just right for   */
/* the bit grammars that the PBMPLUS formats want.           */

static int 
write_header(FILE *file, imageparams *ip)
{
    unsigned char buffer[300];            /* Be careful with the size ! */
    unsigned char *p = buffer;
    int headersize;
    int bounding_x;
    int bounding_y;
    int i;

    /* Calculate the bounding box from the resolutions    */
    bounding_x = ((int) (1200 * ((double) (ip->width) / ip->h_res)));
    bounding_y = ((int) (1200 * ((double) (ip->height) / ip->v_res)));

    /* This is gross. The entire DDIF grammar is constructed by   */
    /* hand. The indentation is meant to indicate DDIF document structure */

    tag(&p,PRIVATE,CONS,16383); ind(&p);      /* DDIF Document */
    tag(&p,CONTEXT,CONS, 0); ind(&p);        /* Document Descriptor */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,1);  /* Major Version */
    tag(&p,CONTEXT,PRIM, 1); wr_int(&p,3);  /* Minor Version */
    tag(&p,CONTEXT,PRIM, 2); wr_string(&p,"PBM+"); /* Product Indentifier */
    tag(&p,CONTEXT,CONS, 3); ind(&p);       /* Product Name */
    tag(&p,PRIVATE,PRIM, 9); emit_isolatin1(&p,"PBMPLUS Writer V1.0");
    eoc(&p);
    eoc(&p);                 /* Document Descriptor */
    tag(&p,CONTEXT,CONS, 1); ind(&p);        /* Document Header     */
    tag(&p,CONTEXT,CONS, 3); ind(&p);       /* Version */
    tag(&p,PRIVATE,PRIM, 9); emit_isolatin1(&p,"1.0");
    eoc(&p);
    eoc(&p);                 /* Document Header */
    tag(&p,CONTEXT,CONS, 2); ind(&p);        /* Document Content    */
    tag(&p,APPLICATION,CONS,2); ind(&p);    /* Segment Primitive    */
    eoc(&p);
    tag(&p,APPLICATION,CONS,2); ind(&p);    /* Segment  */
    tag(&p,CONTEXT,CONS, 3); ind(&p);      /* Segment Specific Attributes */
    tag(&p,CONTEXT,PRIM, 2); wr_string(&p,"$I");  /* Category */
    tag(&p,CONTEXT,CONS,22); ind(&p);     /* Image Attributes */
    tag(&p,CONTEXT,CONS, 0); ind(&p);    /* Image Presentation Attributes */
    tag(&p,CONTEXT,PRIM, 1); wr_int(&p,0);  /* Pixel Path */
    tag(&p,CONTEXT,PRIM, 2); wr_int(&p,270); /* Line Progression */
    tag(&p,CONTEXT,CONS, 3); ind(&p);   /* Pixel Aspect Ratio */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,1); /* PP Pixel Dist */
    tag(&p,CONTEXT,PRIM, 1); wr_int(&p,1); /* LP Pixel Dist */
    eoc(&p);                /* Pixel Aspect Ratio */
    tag(&p,CONTEXT,PRIM, 4); wr_int(&p,ip->polarity);  
        /* Brightness Polarity */
    tag(&p,CONTEXT,PRIM, 5); wr_int(&p,1);  /* Grid Type    */
    tag(&p,CONTEXT,PRIM, 7); wr_int(&p,ip->spectral);  /* Spectral Mapping */
    tag(&p,CONTEXT,CONS,10); ind(&p);   /* Pixel Group Info */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,1); /* Pixel Group Size */
    tag(&p,CONTEXT,PRIM, 1); wr_int(&p,1); /* Pixel Group Order */
    eoc(&p);                /* Pixel Group Info */
    eoc(&p);                     /* Image Presentation Attributes */
    tag(&p,CONTEXT,CONS, 1); ind(&p);    /* Component Space Attributes */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,1);  /* Component Space Organization */
    tag(&p,CONTEXT,PRIM, 1); wr_int(&p,1);  /* Planes per Pixel */
    tag(&p,CONTEXT,PRIM, 2); wr_int(&p,1);  /* Plane Significance   */
    tag(&p,CONTEXT,PRIM, 3); wr_int(&p,ip->components);  
        /* Number of Components    */
    tag(&p,CONTEXT,CONS, 4); ind(&p);   /* Bits per Component   */
    for (i = 0; i < ip->components; i++) {
        tag(&p,UNIVERSAL,PRIM,2); wr_int(&p,ip->bits_per_component);
    }
    eoc(&p);                /* Bits per Component   */
    tag(&p,CONTEXT,CONS, 5); ind(&p);   /* Component Quantization Levels */
    for (i = 0; i < ip->components; i++) {
        tag(&p,UNIVERSAL,PRIM,2); wr_int(&p,1 << ip->bits_per_component);
    }
    eoc(&p);                /* Component Quantization Levels */
    eoc(&p);                 /* Component Space Attributes */
    eoc(&p);                  /* Image Attributes */
    tag(&p,CONTEXT,CONS,23); ind(&p);     /* Frame Parameters */
    tag(&p,CONTEXT,CONS, 1); ind(&p);    /* Bounding Box */
    tag(&p,CONTEXT,CONS, 0); ind(&p);   /* lower-left   */
    tag(&p,CONTEXT,CONS, 0); ind(&p);  /* XCoordinate  */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,0);
    eoc(&p);                           /* XCoordinate  */
    tag(&p,CONTEXT,CONS, 1); ind(&p);  /* YCoordinate  */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,0);
    eoc(&p);                               /* YCoordinate  */
    eoc(&p);                /* lower left */
    tag(&p,CONTEXT,CONS, 1); ind(&p);       /* upper right */
    tag(&p,CONTEXT,CONS, 0); ind(&p);      /* XCoordinate  */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,bounding_x);
    eoc(&p);               /* XCoordinate  */
    tag(&p,CONTEXT,CONS, 1); ind(&p);      /* YCoordinate  */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,bounding_y);
    eoc(&p);                   /* YCoordinate  */
    eoc(&p);                            /* upper right */
    eoc(&p);                 /* Bounding Box */
    tag(&p,CONTEXT,CONS, 4); ind(&p);    /* Frame Position */
    tag(&p,CONTEXT,CONS, 0); ind(&p);   /* XCoordinate  */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,0);
    eoc(&p);                /* XCoordinate  */
    tag(&p,CONTEXT,CONS, 1); ind(&p);   /* YCoordinate  */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,0);
    eoc(&p);                    /* YCoordinate  */
    eoc(&p);                 /* Frame Position */
    eoc(&p);                  /* Frame Parameters */
    eoc(&p);                        /* Segment Specific Attributes */
    eoc(&p);                    /* Segment */
    tag(&p,APPLICATION,CONS,17); ind(&p);   /* Image Data Descriptor */
    tag(&p,UNIVERSAL,CONS,16); ind(&p);    /* Sequence */
    tag(&p,CONTEXT,CONS, 0); ind(&p);     /* Image Coding Attributes */
    tag(&p,CONTEXT,PRIM, 1); wr_int(&p,ip->width); /* Pixels per Line    */
    tag(&p,CONTEXT,PRIM, 2); wr_int(&p,ip->height);  /* Number of Lines  */
    tag(&p,CONTEXT,PRIM, 3); wr_int(&p,2);   /* Compression Type */
    tag(&p,CONTEXT,PRIM, 5); wr_int(&p,0);   /* Data Offset  */
    tag(&p,CONTEXT,PRIM, 6); wr_int(&p,ip->bits_per_pixel);  /* Pixel Stride */
    tag(&p,CONTEXT,PRIM, 7); wr_int(&p,ip->bytes_per_line * 8);
        /* Scanline Stride    */
    tag(&p,CONTEXT,PRIM, 8); wr_int(&p,1);   /* Bit Order        */
    tag(&p,CONTEXT,PRIM, 9); wr_int(&p,ip->bits_per_pixel);  
        /* Planebits per Pixel */
    tag(&p,CONTEXT,CONS,10); ind(&p);    /* Byteorder Info   */
    tag(&p,CONTEXT,PRIM, 0); wr_int(&p,1);  /* Byte Unit        */
    tag(&p,CONTEXT,PRIM, 1); wr_int(&p,1);  /* Byte Order   */
    eoc(&p);                 /* Byteorder Info   */
    tag(&p,CONTEXT,PRIM,11); wr_int(&p,3);   /* Data Type        */
    eoc(&p);                              /* Image Coding Attributes */
    tag(&p,CONTEXT,PRIM, 1); wr_length(&p,ip->bytes_per_line*ip->height);  
        /* Component Plane Data */
    /* End of DDIF document Indentation */
    headersize = p - buffer;
    if (headersize >= 300)  {
        fprintf(stderr,"Overran buffer area %d >= 300\n",headersize);
        exit(1);
    }

    return (fwrite(buffer, 1, headersize, file) == headersize);
}



/* Write all the closing brackets of the DDIF grammar that are missing */
/* The strange indentation reflects exactly the same indentation that  */
/* we left off in the write_header procedure.                  */
static int 
write_trailer(FILE * file)
{
    unsigned char buffer[30];
    unsigned char *p = buffer;
    int trailersize;

    /* Indentation below gives DDIF document structure */
    eoc(&p);                        /* Sequence */
    eoc(&p);                     /* Image Data Descriptor */
    tag(&p,APPLICATION,PRIM,1); wr_null(&p);     /* End Segment */
    tag(&p,APPLICATION,PRIM,1); wr_null(&p);     /* End Segment */
    eoc(&p);                  /* Document Content */
    eoc(&p);                   /* DDIF Document */
    /* End of DDIF document Indentation */
    trailersize = p - buffer;
    if (trailersize >= 30)  {
        fprintf(stderr,"Overran buffer area %d >= 30\n",trailersize);
        exit(1);
    }

    return(fwrite(buffer, 1, trailersize, file) == trailersize);
}



int main(int argc, char *argv[])
{
    FILE           *ifd;
    FILE       *ofd;
    int             rows, cols;
    xelval          maxval;
    int             format;
    const char     * const usage = "[-resolution x y] [pnmfile [ddiffile]]";
    int             i, j;
    char           *outfile;
    int       argn;
    int hor_resolution = 75;
    int ver_resolution = 75;
    imageparams ip;
    unsigned char  *data, *p;

    pnm_init(&argc, argv);

    for (argn = 1;argn < argc && argv[argn][0] == '-';argn++) {
        int arglen = strlen(argv[argn]);

        if (!strncmp (argv[argn],"-resolution", arglen)) {
            if (argn + 2 < argc) {
                hor_resolution = atoi(argv[argn+1]);
                ver_resolution = atoi(argv[argn+2]);
                argn += 2;
                continue;
            } else {
                pm_usage(usage);
            }
        } else {
            pm_usage(usage);
        }
    }

    if (hor_resolution <= 0 || ver_resolution <= 0) {
        fprintf(stderr,"Unreasonable resolution values: %d x %d\n",
                hor_resolution,ver_resolution);
        exit(1);
    }

    if (argn == argc - 2) {
        ifd = pm_openr(argv[argn]);
        outfile = argv[argn+1];
        if (!(ofd = fopen(outfile,"wb"))) {
            perror(outfile);
            exit(1);
        }
    } else if (argn == argc - 1) {
        ifd = pm_openr(argv[argn]);
        ofd = stdout;
    } else {
        ifd = stdin;
        ofd = stdout;
    }

    pnm_readpnminit(ifd, &cols, &rows, &maxval, &format);

    ip.width = cols;
    ip.height = rows;
    ip.h_res = hor_resolution;
    ip.v_res = ver_resolution;

    switch (PNM_FORMAT_TYPE(format)) {
    case PBM_TYPE:
        ip.bits_per_pixel = 1;
        ip.bytes_per_line = (cols + 7) / 8;
        ip.spectral = 2;
        ip.components = 1;
        ip.bits_per_component = 1;
        ip.polarity = 1;
        break;
    case PGM_TYPE:
        ip.bytes_per_line = cols;
        ip.bits_per_pixel = 8;
        ip.spectral = 2;
        ip.components = 1;
        ip.bits_per_component = 8;
        ip.polarity = 2;
        break;
    case PPM_TYPE:
        ip.bytes_per_line = 3 * cols;
        ip.bits_per_pixel = 24;
        ip.spectral = 5;
        ip.components = 3;
        ip.bits_per_component = 8;
        ip.polarity = 2;
        break;
    default:
        fprintf(stderr, "Unrecognized PBMPLUS format %d\n", format);
        exit(1);
    }

    if (!write_header(ofd,&ip)) {
        perror("Writing header");
        exit(1);
    }

    if (!(p = data = (unsigned char*)  malloc(ip.bytes_per_line))) {
        perror("allocating line buffer");
        exit(1);
    }

    switch (PNM_FORMAT_TYPE(format)) {
    case PBM_TYPE:
    {
        bit            *pixels;
        int             mask;
        int             k;

        pixels = pbm_allocrow(cols);

        for (i = 0; i < rows; i++) {
            pbm_readpbmrow(ifd, pixels, cols, format);
            mask = 0;
            p = data;
            for (j = 0, k = 0; j < cols; j++) {
                if (pixels[j] == PBM_BLACK) {
                    mask |= 1 << k;
                }
                if (k == 7) {
                    *p++ = mask;
                    mask = 0;
                    k = 0;
                } else {
                    k++;
                }
            }
            if (k != 7) {       /* Flush the rest of the column */
                *p = mask;
            }
            if (fwrite(data,1,ip.bytes_per_line,ofd) != ip.bytes_per_line) {
                perror("Writing image data\n");
                exit(1);
            }
        }
    }
    break;
    case PGM_TYPE:
    {
        gray          *pixels = pgm_allocrow(cols);

        for (i = 0; i < rows; i++) {
            p = data;
            pgm_readpgmrow(ifd, pixels, cols, maxval, format);
            for (j = 0; j < cols; j++) {
                *p++ = (unsigned char) pixels[j];
            }
            if (fwrite(data,1,ip.bytes_per_line,ofd) != ip.bytes_per_line) {
                perror("Writing image data\n");
                exit(1);
            }
        }
        pgm_freerow(pixels);
    }
    break;
    case PPM_TYPE:
    {
        pixel          *pixels = ppm_allocrow(cols);

        for (i = 0; i < rows; i++) {
            p = data;
            ppm_readppmrow(ifd, pixels, cols, maxval, format);
            for (j = 0; j < cols; j++) {
                *p++ = PPM_GETR(pixels[j]);
                *p++ = PPM_GETG(pixels[j]);
                *p++ = PPM_GETB(pixels[j]);
            }
            if (fwrite(data,1,ip.bytes_per_line,ofd) != ip.bytes_per_line) {
                perror("Writing image data\n");
                exit(1);
            }
        }
        ppm_freerow(pixels);
    }
    break;
    }

    pm_close(ifd);

    free(data);

    if (!write_trailer(ofd)) {
        perror("Writing trailer");
        exit(1);
    }

    if (fclose(ofd) == EOF) {
        perror("Closing output file");
        exit(1);
    };

    return(0);
}
