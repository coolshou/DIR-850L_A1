/* Spottopgm: Convert a SPOT satellite image to PGM format.
 *
 * Usage: spottopgm [-1|2|3] [Firstcol Firstline Lastcol Lastline] inputfile
 *
 * Author: Warren Toomey, 1992.
 */

#include <stdio.h>
#include "pgm.h"

/* Global variables */

static FILE *spotfile;          /* The input file */
static char linebuf[12000];        /* The line buffer */

static long Firstline = 0,     /* The rectangle the user wants */
    Lastline = 3000,   /* cut out of the image */
    Firstcol = 0,
    Lastcol = 0;
static long Diff = 0;          /* Firstcol - Lastcol */
static char *Bufptr;           /* Pointer into the input image */
static int Color = 1;          /* Either 1, 2 or 3 */
static int Colbool = 0;        /* 1 if color */


/* Get_image extracts the pixel data from one line
 * (i.e one record) in the SPOT input file. A SPOT image
 * record has a header, data and trailer. The data lengths
 * are fixed at 3960, 5400, 8640 or 10980 bytes.
 *
 * When we arrive here we have read in 12 bytes of the record.
 * We then read in the rest of the record. We find the trailer
 * and from that determine the number of pixels on the line.
 *
 * If the image is really color i.e interleaved 3 colors, we
 * convert a line if its spectral sequence is the same as the one
 * requested by the user (i.e 1, 2 or 3). I could create a ppm file
 * but I couldn't be bothered with the rearranging of the data.
 */
static int 
get_image(long length)
{
    int cnt;
    struct Linehdr        /* Each line begins with the 12 bytes */
    {             /* we have already, plus these 20 bytes */
        long linenum;       /* The line number of the record */
        short recseq;       /* The record sequence number */
        short spectseq;     /* The spectral number of the line */
        long linetime;      /* Time it was recorded (in ms). */
        long leftpixmar;        /* The pixel number of the 1st pixel */
        long rightpixmar;       /* The pixel number of the last pixel */
    } linehdr;
    long numpixels;       /* Number of pixels on the line */

    /* Get the details of this line */
    if (pm_readbiglong (spotfile, &linehdr.linenum) == -1
        || pm_readbigshort (spotfile, &linehdr.recseq) == -1
        || pm_readbigshort (spotfile, &linehdr.spectseq) == -1
        || pm_readbiglong (spotfile, &linehdr.linetime) == -1
        || pm_readbiglong (spotfile, &linehdr.leftpixmar) == -1
        || pm_readbiglong (spotfile, &linehdr.rightpixmar) == -1)
        pm_error ("EOF / read error reading line header");

    /* Now read in the line data */
    cnt = length - 20 - 88;
    cnt = fread(linebuf, 1, cnt, spotfile);

    if (!Diff)
    {
        cnt += 28;
        if (fseek (spotfile, 24, 1) == EOF)
            pm_error ("seek error");
        if (pm_readbiglong (spotfile, &numpixels) == -1)
            pm_error ("EOF / read error reading line ender");
    
        /* Determine the picture size */
        Bufptr = &linebuf[Firstcol];
        if (Lastcol == 0 || Lastcol > numpixels)
            Lastcol = numpixels;
        Diff = Lastcol - Firstcol;
        /* Print out the header */
        printf("P5\n%ld %ld\n255\n", Diff, Lastline - Firstline);
        /* Inform about the image size */
        if (Colbool) fprintf(stderr, "Color image, ");
        fprintf(stderr, "%ld pixels wide\n", numpixels);
    }

    /* Output the line */
    if (linehdr.linenum >= Firstline && linehdr.linenum <= Lastline
        && linehdr.spectseq == Color)
        fwrite(Bufptr, 1, Diff, stdout);
    if (linehdr.linenum > Lastline) exit(0);

#ifdef DEBUG
    fprintf(stderr,
            "Line %4d, %3d, %3d, time %4d, l/r "
            "pixmar %4d %4d len %d pixnum %d\n",
            linehdr.linenum, linehdr.recseq, 
            linehdr.spectseq, linehdr.linetime,
            linehdr.leftpixmar, linehdr.rightpixmar, length, numpixels);
#endif
    /* And return the amount to seek */
    return (length - 20 - cnt);
}



/* The image header tells us if the image is in monochrome or color, and
 * if the latter, if the input colors are interleaved. If interleaved
 * color, lines are interleaved R, G, B, R, G, B etc. Technically, some
 * interleaving of infra-red, visible and ultra-violet.
 *
 * In the description field below,
 *  element 0 == P --> monochrome
 *  element 0 == X --> color
 *  element 9 == S --> sequential (i.e only one color here)
 *  element 9 == I --> interleaved (1 or more colors)
 */
static int 
get_imghdr(int length)
{
    struct Imghdr
    {
        long linewidth;
        char dummy1[36];
        char description[16];   /* Type of image */
    } header;

    if (pm_readbiglong (spotfile, &header.linewidth) == -1)
        pm_error ("EOF / read error reading header");
#ifdef DEBUG
    if (fread (header.dummy1, 1, 36, spotfile) != 36)
        pm_error ("EOF / read error reading header");
#else
    if (fseek (spotfile, 36, 1) == EOF)
        pm_error ("seek error");
#endif
    if (fread (header.description, 1, 16, spotfile) != 16)
        pm_error ("EOF / read error reading header");

    /* Determine mono or color */
    if (header.description[0] == 'X' && header.description[9] == 'S')
        Colbool = 1;
    else Colbool = 0;

#ifdef DEBUG
    fprintf(stderr, "Dummy str is >%s<\n", header.dummy1);
    fprintf(stderr, "Imghdr str is >%s<, col %d\n", 
            header.description, Colbool);
#endif
    /* Return the amount to fseek */
    return (length - 56);
}


static void
usage()
{
    fprintf(stderr,
            "Usage: spottopgm [-1|2|3] [Firstcol Firstline Lastcol Lastline] "
            "input_file\n");
    exit(1);
}

int
main(int argc, char *argv[])
{
    struct Record             /* A SPOT image is broken up into */
    {                 /* records with the following fields */
        long record;            /* The record number (1, 2, 3...) */
        unsigned char sub1;         /* Record sub type 1 */
        unsigned char type;         /* The record type */
        unsigned char sub2;         /* Record sub type 2 */
        unsigned char sub3;         /* Record sub type 3 */
        long length;            /* Record length in bytes */
    } arecord;

    pgm_init( &argc, argv );
   
    switch (argc)
    {
    case 7:
        Color= -(atoi(argv[1]));      /* Get the color to extract */
        argv++;
    case 6:
        Firstcol = atoi(argv[1]);     /* Get the rectangle to extract */
        Firstline = atoi(argv[2]);
        Lastcol = atoi(argv[3]);
        Lastline = atoi(argv[4]);
        argv += 4;
        goto openfile;            /* Yuk, a goto! */
    case 3:
        Color= -(atoi(argv[1]));      /* Get the color to extract */
        argv++;
    case 2:
    openfile:
    spotfile = fopen(argv[1], "rb");  /* Open the input file */
    if (spotfile == NULL) { perror("fopen"); exit(1); }
    break;
    default:
        usage();
    }

    while (1)             /* Get a record */
    {
        if (pm_readbiglong (spotfile, &arecord.record) == -1)
            break;
        arecord.sub1 = fgetc (spotfile);
        arecord.type = fgetc (spotfile);
        arecord.sub2 = fgetc (spotfile);
        arecord.sub3 = fgetc (spotfile);
        if (pm_readbiglong (spotfile, &arecord.length) == -1)
            pm_error ("EOF / read error reading a record");

        arecord.length -= 12;   /* Subtract header size as well */
        if (arecord.type == 0355 && arecord.sub1 == 0355)
            arecord.length = get_image(arecord.length);
        else if (arecord.type == 0300 && arecord.sub1 == 077)
            arecord.length = get_imghdr(arecord.length);
#ifdef DEBUG
        else
            fprintf(stderr, 
                    "Rcrd %3d, type %03o, stype %03o %03o %03o, length %d\n",
                    arecord.record, arecord.type, arecord.sub1, arecord.sub2,
                    (int) arecord.sub3 & 0xff, arecord.length);
#endif
        /* Seek to next record */
        fseek(spotfile, arecord.length, 1);
    }
    exit (0);
}
