/* ppmtomitsu.c - read a portable pixmap and produce output for the
**                Mitsubishi S340-10 Thermo-Sublimation Printer
**                (or the S3410-30 parallel interface)
**
** Copyright (C) 1992,93 by S.Petra Zeidler
** Minor modifications by Ingo Wilken:
x**  - mymalloc() and check_and_rotate() functions for often used
**    code fragments.  Reduces code size by a few KB.
**  - use pm_error() instead of fprintf(stderr)
**  - localized allocation of colorhastable
**
** This software was written for the Max Planck Institut fuer Radioastronomie,
** Bonn, Germany, Optical Interferometry group
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
#include "ppm.h"
#include "nstring.h"
#include "mallocvar.h"

#include "mitsu.h"


#include <stdio.h>

#define HASHSIZE 2048
#define myhash(x) ((PPM_GETR(x)*3 + PPM_GETG(x)*5 + PPM_GETB(x)*7) % HASHSIZE)

typedef struct hashinfo {
        pixel     color;
        long      flag;
        struct hashinfo *next;
} hashinfo;

#ifdef __STDC__
static void lineputinit(int cols, int rows, int sharpness, int enlarge, int
                        copy, struct mediasize medias);
static void frametransferinit(int cols, int rows, int sharpness, int enlarge,
                              int copy, struct mediasize medias);
static void lookuptableinit(int sharpness, int enlarge, int copy,
                            struct mediasize medias);
static void lookuptabledata(int cols, int rows, int enlarge,
                            struct mediasize medias);
static void check_and_rotate(int cols, int rows, int enlarge,
                             struct mediasize medias);
#define CONST const
#else /*__STDC__*/
static int lineputinit();
static int lookuptableinit();
static int lookuptabledata();
static int frametransferinit();
static int check_and_rotate();
#define CONST
#endif

#define cmd(arg)           fputc((arg), stdout)
#define datum(arg)         fputc((char)(arg), stdout)
#define data(arg,num)      fwrite((arg), sizeof(char), (num), stdout)


#ifdef __STDC__
int main(int argc, char *argv[] )
#else
int main( argc, argv )
    int argc;
    char* argv[];
#endif
    {
    FILE             *ifp;
    /*hashinfo         colorhashtable[HASHSIZE];*/
    struct hashinfo  *hashrun;
    pixel            *xP;
    int              argn;
    bool             dpi300;
    int              cols, rows, format, col, row;
    int              sharpness, enlarge, copy, tiny;
    pixval           maxval;
    struct mediasize medias;
    char             media[16];
    const char * const usage = "[-sharpness <1-4>] [-enlarge <1-3>] [-media <a,a4,as,a4s>] [-copy <1-9>] [-tiny] [-dpi300] [ppmfile]";

    ppm_init(&argc, argv);

    dpi300 = FALSE;
    argn = 1;
    sharpness = 32;
    enlarge   = 1;
    copy      = 1;
    memset(media, '\0', 16);
    tiny      = FALSE;

    /* check for flags */
    while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0') {
    if (pm_keymatch(argv[argn], "-sharpness", 2)) {
        ++argn;
        if (argn == argc || sscanf(argv[argn], "%d", &sharpness) != 1)
            pm_usage(usage);
        else if (sharpness < 1 || sharpness > 4)
            pm_usage(usage);
        }
    else if (pm_keymatch(argv[argn], "-enlarge", 2)) {
        ++argn;
        if (argn == argc || sscanf(argv[argn], "%d", &enlarge) != 1)
            pm_usage(usage);
        else if (enlarge < 1 || enlarge > 3)
            pm_usage(usage);
        }
    else if (pm_keymatch(argv[argn], "-media", 2)) {
        ++argn;
        if (argn == argc || sscanf(argv[argn], "%15s", media) < 1)
            pm_usage(usage);
        else if (TOUPPER(media[0]) != 'A')
            pm_usage(usage);
    }
    else if (pm_keymatch(argv[argn], "-copy", 2)) {
        ++argn;
        if (argn == argc || sscanf(argv[argn], "%d", &copy) != 1)
            pm_usage(usage);
        else if (copy < 1 || copy > 9)
            pm_usage(usage);
        }
    else if (pm_keymatch(argv[argn], "-dpi300", 2))
        dpi300 = TRUE;
    else if (pm_keymatch(argv[argn], "-tiny", 2))
        tiny = TRUE;
    else
        pm_usage(usage);
        ++argn;
    }

    if (argn < argc) {
        ifp = pm_openr(argv[argn]);
        ++argn;
    }
    else
        ifp = stdin;

    if (argn != argc)
        pm_usage(usage);

    if (TOUPPER(media[0]) == 'A')
        switch (TOUPPER(media[1])) {
        case 'S':
            medias = MSize_AS;
            break;
        case '4':
            if(TOUPPER(media[2]) == 'S')
                medias = MSize_A4S;
            else {
                medias = MSize_A4;
            }
            break;
        default:
            medias = MSize_A;
        }
    else
        medias = MSize_User;

        if (dpi300) {
                medias.maxcols *= 2;
                medias.maxrows *= 2;
        }

    if (tiny) {
        pixel            *pixelrow;
        char             *redrow, *greenrow, *bluerow;

        ppm_readppminit(ifp, &cols, &rows, &maxval, &format);
        pixelrow = (pixel *) ppm_allocrow(cols);
        MALLOCARRAY_NOFAIL(redrow, cols);
        MALLOCARRAY_NOFAIL(greenrow, cols);
        MALLOCARRAY_NOFAIL(bluerow, cols);
        lineputinit(cols, rows, sharpness, enlarge, copy, medias);

        for ( row = 0; row < rows; ++row ) {
            ppm_readppmrow(ifp, pixelrow, cols, maxval, format);
            switch(PPM_FORMAT_TYPE(format)) {
            /* color */
            case PPM_TYPE:
                for (col = 0, xP = pixelrow; col < cols; col++, xP++) {
                    /* First red. */
                    redrow[col] = PPM_GETR(*xP);
                    /* Then green. */
                    greenrow[col] = PPM_GETG(*xP);
                    /* And blue. */
                    bluerow[col] = PPM_GETB(*xP);
                }
                data(redrow,   cols);
                data(greenrow, cols);
                data(bluerow,  cols);
                break;
            /* grayscale */
            default:
                for (col = 0, xP = pixelrow; col < cols; col++, xP++)
                    bluerow[col] = PPM_GETB(*xP);
                data(bluerow, cols);
                data(bluerow, cols);
                data(bluerow, cols);
                break;
            }
        }
        pm_close(ifp);
    }
    else {
        pixel            **pixelpic;
        int              colanz, colval;
        int                 i;
        colorhist_vector table;

        ppm_readppminit( ifp, &cols, &rows, &maxval, &format );
        pixelpic = ppm_allocarray( cols, rows );
        for (row = 0; row < rows; row++)
            ppm_readppmrow( ifp, pixelpic[row], cols, maxval, format );
        pm_close(ifp);

        /* first check wether we can use the lut transfer */

        table = ppm_computecolorhist(pixelpic, cols, rows, MAXLUTCOL+1, 
                                     &colanz);
        if (table != NULL) {
            hashinfo *colorhashtable;

            MALLOCARRAY_NOFAIL(colorhashtable, HASHSIZE);
            for (i=0; i<HASHSIZE; i++) {
                colorhashtable[i].flag = -1;
                colorhashtable[i].next = NULL;
            }

            /* we can use the lookuptable */
            pm_message("found %d colors - using the lookuptable-method",
                       colanz);
            lookuptableinit(sharpness, enlarge, copy, medias);
            switch(PPM_FORMAT_TYPE(format)) {
            /* color */
            case PPM_TYPE:
                for (colval=0; colval<colanz; colval++) {
                    cmd('$');
                    datum(colval);
                    datum(PPM_GETR((table[colval]).color));
                    datum(PPM_GETG((table[colval]).color));
                    datum(PPM_GETB((table[colval]).color));

                    hashrun = &colorhashtable[myhash((table[colval]).color)];
                    if (hashrun->flag == -1) {
                        hashrun->color = (table[colval]).color;
                        hashrun->flag  = colval;
                    }
                    else {
                        while (hashrun->next != NULL)
                            hashrun = hashrun->next;
                        MALLOCVAR_NOFAIL(hashrun->next);
                        hashrun = hashrun->next;
                        hashrun->color = (table[colval]).color;
                        hashrun->flag  = colval;
                        hashrun->next  = NULL;
                    }
                }
                break;
            /* other */
            default:
                for (colval=0; colval<colanz; colval++) {
                    cmd('$');
                    datum(colval);
                    datum(PPM_GETB((table[colval]).color));
                    datum(PPM_GETB((table[colval]).color));
                    datum(PPM_GETB((table[colval]).color));

                    hashrun = &colorhashtable[myhash((table[colval]).color)];
                    if (hashrun->flag == -1) {
                        hashrun->color = (table[colval]).color;
                        hashrun->flag  = colval;
                    }
                    else {
                        while (hashrun->next != NULL)
                            hashrun = hashrun->next;
                        MALLOCVAR_NOFAIL(hashrun->next);
                        hashrun = hashrun->next;
                        hashrun->color = (table[colval]).color;
                        hashrun->flag  = colval;
                        hashrun->next  = NULL;
                    }
                }
            }
            lookuptabledata(cols, rows, enlarge, medias);
            for (row=0; row<rows; row++) {
                xP = pixelpic[row];
                for (col=0; col<cols; col++, xP++) {
                    hashrun = &colorhashtable[myhash(*xP)];
                    while (!PPM_EQUAL((hashrun->color), *xP))
                        if (hashrun->next != NULL)
                            hashrun = hashrun->next;
                        else {
                            pm_error("you just found a lethal bug.");
                        }
                    datum(hashrun->flag);
                }
            }
            free(colorhashtable);
        }
        else {
        /* $#%@^!& no lut possible, so send the pic as 24bit */
            pm_message("found too many colors for fast lookuptable mode");
            frametransferinit(cols, rows, sharpness, enlarge, copy, medias);
            switch(PPM_FORMAT_TYPE(format)) {
            /* color */
            case PPM_TYPE:
                COLORDES(RED);
                DATASTART;                    /* red coming */
                for (row=0; row<rows; row++) {
                    xP = pixelpic[row];
                    for (col=0; col<cols; col++, xP++)
                        datum(PPM_GETR(*xP));
                }
                COLORDES(GREEN);
                DATASTART;                    /* green coming */
                for (row=0; row<rows; row++) {
                    xP = pixelpic[row];
                    for (col=0; col<cols; col++, xP++)
                        datum(PPM_GETG(*xP));
                }
                COLORDES(BLUE);
                DATASTART;                    /* blue coming */
                for (row=0; row<rows; row++) {
                    xP = pixelpic[row];
                    for (col=0; col<cols; col++, xP++)
                        datum(PPM_GETB(*xP));
                }
                break;
            /* grayscale */
            default:
                COLORDES(RED);
                DATASTART;                    /* red coming */
                for (row=0; row<rows; row++) {
                    xP = pixelpic[row];
                    for (col=0; col<cols; col++, xP++)
                        datum(PPM_GETB(*xP));
                }
                COLORDES(GREEN);
                DATASTART;                    /* green coming */
                for (row=0; row<rows; row++) {
                    xP = pixelpic[row];
                    for (col=0; col<cols; col++, xP++)
                        datum(PPM_GETB(*xP));
                }
                COLORDES(BLUE);
                DATASTART;                    /* blue coming */
                for (row=0; row<rows; row++) {
                    xP = pixelpic[row];
                    for (col=0; col<cols; col++, xP++)
                        datum(PPM_GETB(*xP));
                }
            }
        }
    }
    PRINTIT;
    exit(0);
}

#ifdef __STDC__
static void lineputinit(int cols, int rows,
                        int sharpness, int enlarge, int copy,
                        struct mediasize medias)
#else /*__STDC__*/
static int lineputinit(cols, rows, sharpness, enlarge, copy, medias)
    int cols, rows;
    int sharpness, enlarge, copy;
    struct mediasize medias;
#endif /*__STDC__*/
{
    ONLINE;
    CLRMEM;
    MEDIASIZE(medias);

    switch (enlarge) {
    case 2:
        HENLARGE(ENLARGEx2); /* enlarge horizontal */
        VENLARGE(ENLARGEx2); /* enlarge vertical */
        break;
    case 3:
        HENLARGE(ENLARGEx3); /* enlarge horizontal */
        VENLARGE(ENLARGEx3); /* enlarge vertical */
        break;
    default:
        HENLARGE(NOENLARGE); /* enlarge horizontal */
        VENLARGE(NOENLARGE); /* enlarge vertical */
    }

    COLREVERSION(DONTREVERTCOLOR);
    NUMCOPY(copy);

    HOFFINCH('\000');
    VOFFINCH('\000');
    CENTERING(DONTCENTER);

    TRANSFERFORMAT(LINEORDER);
    COLORSYSTEM(RGB);
    GRAYSCALELVL(BIT_8);

    switch (sharpness) {          /* sharpness :-) */
    case 0:
        SHARPNESS(SP_NONE);
        break;
    case 1:
        SHARPNESS(SP_LOW);
        break;
    case 2:
        SHARPNESS(SP_MIDLOW);
        break;
    case 3:
        SHARPNESS(SP_MIDHIGH);
        break;
    case 4:
        SHARPNESS(SP_HIGH);
        break;
    default:
        SHARPNESS(SP_USER);
    }
    check_and_rotate(cols, rows, enlarge, medias);
    DATASTART;
    return;
}

#ifdef __STDC__
static void lookuptableinit(int sharpness, int enlarge, int copy,
                            struct mediasize medias)
#else /*__STDC__*/
static int lookuptableinit(sharpness, enlarge, copy, medias)
    int sharpness, enlarge, copy;
    struct mediasize medias;
#endif /*__STDC__*/
{
    ONLINE;
    CLRMEM;
    MEDIASIZE(medias);

    switch (enlarge) {
    case 2:
        HENLARGE(ENLARGEx2); /* enlarge horizontal */
        VENLARGE(ENLARGEx2); /* enlarge vertical */
        break;
    case 3:
        HENLARGE(ENLARGEx3); /* enlarge horizontal */
        VENLARGE(ENLARGEx3); /* enlarge vertical */
        break;
    default:
        HENLARGE(NOENLARGE); /* enlarge horizontal */
        VENLARGE(NOENLARGE); /* enlarge vertical */
    }

    COLREVERSION(DONTREVERTCOLOR);
    NUMCOPY(copy);

    HOFFINCH('\000');
    VOFFINCH('\000');
    CENTERING(DONTCENTER);

    TRANSFERFORMAT(LOOKUPTABLE);

    switch (sharpness) {          /* sharpness :-) */
    case 0:
        SHARPNESS(SP_NONE);
        break;
    case 1:
        SHARPNESS(SP_LOW);
        break;
    case 2:
        SHARPNESS(SP_MIDLOW);
        break;
    case 3:
        SHARPNESS(SP_MIDHIGH);
        break;
    case 4:
        SHARPNESS(SP_HIGH);
        break;
    default:
        SHARPNESS(SP_USER);
    }

    LOADLOOKUPTABLE;
    return;
}

#ifdef __STDC__
static void lookuptabledata(int cols, int rows, int enlarge,
                                                        struct mediasize medias)
#else /*__STDC__*/
static int lookuptabledata(cols, rows, enlarge, medias)
    int   rows, cols;
    int   enlarge;
    struct mediasize medias;
#endif /*__STDC__*/
{
    DONELOOKUPTABLE;
    check_and_rotate(cols, rows, enlarge, medias);
    DATASTART;
    return;
}

#ifdef __STDC__
static void frametransferinit(int cols, int rows, int sharpness,
                              int enlarge, int copy, struct mediasize medias)
#else
static int frametransferinit(cols, rows, sharpness, enlarge, copy, medias)

    int     rows, cols;
    int     sharpness, enlarge, copy;
    struct mediasize medias;
#endif
{
    ONLINE;
    CLRMEM;
    MEDIASIZE(medias);

    switch (enlarge) {
    case 2:
        HENLARGE(ENLARGEx2); /* enlarge horizontal */
        VENLARGE(ENLARGEx2); /* enlarge vertical */
        break;
    case 3:
        HENLARGE(ENLARGEx3); /* enlarge horizontal */
        VENLARGE(ENLARGEx3); /* enlarge vertical */
        break;
    default:
        HENLARGE(NOENLARGE); /* enlarge horizontal */
        VENLARGE(NOENLARGE); /* enlarge vertical */
    }

    COLREVERSION(DONTREVERTCOLOR);
    NUMCOPY(copy);

    HOFFINCH('\000');
    VOFFINCH('\000');
    CENTERING(DONTCENTER);

    TRANSFERFORMAT(FRAMEORDER);
    COLORSYSTEM(RGB);
    GRAYSCALELVL(BIT_8);

    switch (sharpness) {          /* sharpness :-) */
    case 0:
        SHARPNESS(SP_NONE);
        break;
    case 1:
        SHARPNESS(SP_LOW);
        break;
    case 2:
        SHARPNESS(SP_MIDLOW);
        break;
    case 3:
        SHARPNESS(SP_MIDHIGH);
        break;
    case 4:
        SHARPNESS(SP_HIGH);
        break;
    default:
        SHARPNESS(SP_USER);
    }
    check_and_rotate(cols, rows, enlarge, medias);
    return;
}


#ifdef __STDC__
static void
check_and_rotate(int cols, int rows, int enlarge, struct mediasize medias)
#else
static int
check_and_rotate(cols, rows, enlarge, medias)
    int cols, rows, enlarge;
    struct mediasize medias;
#endif
{
    if (cols > rows) {
        ROTATEIMG(DOROTATE);                        /* rotate image */
        if (enlarge*rows > medias.maxcols || enlarge*cols > medias.maxrows) {
            pm_error("Image too large, MaxPixels = %d x %d", medias.maxrows, medias.maxcols);
        }
        HPIXELS(cols);
        VPIXELS(rows);
        HPIXELSOFF((medias.maxcols/enlarge - rows)/2);
        VPIXELSOFF((medias.maxrows/enlarge - cols)/2);
        pm_message("rotating image for output");
    }
    else {
        ROTATEIMG(DONTROTATE);
        if (enlarge*rows > medias.maxrows || enlarge*cols > medias.maxcols) {
            pm_error("Image too large, MaxPixels = %d x %d", medias.maxrows, medias.maxcols);
        }
        HPIXELS(cols);
        VPIXELS(rows);
        HPIXELSOFF((medias.maxcols/enlarge - cols)/2);
        VPIXELSOFF((medias.maxrows/enlarge - rows)/2);
    }
}

