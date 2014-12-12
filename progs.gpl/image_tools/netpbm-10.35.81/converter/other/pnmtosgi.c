/* pnmtosgi.c - convert portable anymap to SGI image
**
** Copyright (C) 1994 by Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
**
** Based on the SGI image description v0.9 by Paul Haeberli (paul@sgi.comp)
** Available via ftp from sgi.com:graphics/SGIIMAGESPEC
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** 29Jan94: first version
*/
#include "pnm.h"
#include "sgi.h"
#include "mallocvar.h"

/*#define DEBUG*/

typedef short       ScanElem;
typedef struct {
    ScanElem *  data;
    long        length;
} ScanLine;

/* prototypes */
static void put_big_short ARGS((short s));
static void put_big_long ARGS((long l));
#define put_byte(b)     (void)(putc((unsigned char)(b), stdout))
static void put_short_as_byte ARGS((short s));
static void write_table  ARGS((long *table, int tabsize));
static void write_channels ARGS((int cols, int rows, int channels, void (*put) ARGS((short)) ));
static long * build_channels ARGS((FILE *ifp, int cols, int rows, xelval maxval, int format, int bpc, int channels));
static ScanElem *compress ARGS((ScanElem *temp, int row, int rows, int cols, int chan_no, long *table, int bpc));
static int rle_compress ARGS((ScanElem *inbuf, int cols));

#define WORSTCOMPR(x)   (2*(x) + 2)


#define MAXVAL_BYTE     255
#define MAXVAL_WORD     65535

static char storage = STORAGE_RLE;
static ScanLine * channel[3];
static ScanElem * rletemp;
static xel * pnmrow;


static void
write_header(int const cols, 
             int const rows, 
             xelval const maxval, 
             int const bpc, 
             int const dimensions, 
             int const channels, 
             const char * const imagename)
{
    int i;

#ifdef DEBUG
    pm_message("writing header");
#endif

    put_big_short(SGI_MAGIC);
    put_byte(storage);
    put_byte((char)bpc);
    put_big_short(dimensions);
    put_big_short(cols);
    put_big_short(rows);
    put_big_short(channels);
    put_big_long(0);                /* PIXMIN */
    put_big_long(maxval);           /* PIXMAX */
    for( i = 0; i < 4; i++ )
        put_byte(0);
    for( i = 0; i < 79 && imagename[i] != '\0'; i++ )
        put_byte(imagename[i]);
    for(; i < 80; i++ )
        put_byte(0);
    put_big_long(CMAP_NORMAL);
    for( i = 0; i < 404; i++ )
        put_byte(0);
}



int
main(argc, argv)
    int argc;
    char *argv[];
{
    FILE *ifp;
    int argn;
    const char * const usage = "[-verbatim|-rle] [-imagename <name>] [pnmfile]";
    int cols, rows, format;
    xelval maxval, newmaxval;
    const char *imagename = "no name";
    int bpc, dimensions, channels;
    long *table = NULL;

    pnm_init(&argc, argv);

    argn = 1;
    while( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' ) {
        if( pm_keymatch(argv[argn], "-verbatim", 2) )
            storage = STORAGE_VERBATIM;
        else
        if( pm_keymatch(argv[argn], "-rle", 2) )
            storage = STORAGE_RLE;
        else
        if( pm_keymatch(argv[argn], "-imagename", 2) ) {
            if( ++argn >= argc )
                pm_usage(usage);
            imagename = argv[argn];
        }
        else
            pm_usage(usage);
        ++argn;
    }

    if( argn < argc ) {
        ifp = pm_openr( argv[argn] );
        argn++;
    }
    else
        ifp = stdin;

    if( argn != argc )
        pm_usage(usage);

    pnm_readpnminit(ifp, &cols, &rows, &maxval, &format);
    pnmrow = pnm_allocrow(cols);

    switch( PNM_FORMAT_TYPE(format) ) {
        case PBM_TYPE:
            newmaxval = PGM_MAXMAXVAL;
            pm_message("promoting PBM to PGM");
        case PGM_TYPE:
            newmaxval = maxval;
            dimensions = 2; channels = 1;
            break;
        case PPM_TYPE:
            newmaxval = maxval;
            dimensions = 3; channels = 3;
            break;
        default:
            pm_error("can\'t happen");
    }
    if( newmaxval <= MAXVAL_BYTE )
        bpc = 1;
    else if( newmaxval <= MAXVAL_WORD )
        bpc = 2;
    else
        pm_error("maxval too large - try using \"pnmdepth %d\"", MAXVAL_WORD);

    table = build_channels(ifp, cols, rows, newmaxval, format, bpc, channels);
    pnm_freerow(pnmrow);
    pm_close(ifp);

    write_header(cols, rows, newmaxval, bpc, dimensions, channels, imagename);
    if( table )
        write_table(table, rows * channels);
    if( bpc == 1 )
        write_channels(cols, rows, channels, put_short_as_byte);
    else
        write_channels(cols, rows, channels, put_big_short);

    exit(0);
}


static void
write_table(table, tabsize)
    long *table;
    int tabsize;
{
    int i;
    long offset;

#ifdef DEBUG
    pm_message("writing table");
#endif

    offset = HeaderSize + tabsize * 8;
    for( i = 0; i < tabsize; i++ ) {
        put_big_long(offset);
        offset += table[i];
    }
    for( i = 0; i < tabsize; i++ )
        put_big_long(table[i]);
}


static void
write_channels(cols, rows, channels, put)
    int cols, rows, channels;
    void (*put) ARGS((short));
{
    int i, row, col;

#ifdef DEBUG
    pm_message("writing image data");
#endif

    for( i = 0; i < channels; i++ ) {
        for( row = 0; row < rows; row++ ) {
            for( col = 0; col < channel[i][row].length; col++ ) {
                (*put)(channel[i][row].data[col]);
            }
        }
    }
}

static void
put_big_short(short s)
{
    if ( pm_writebigshort( stdout, s ) == -1 )
        pm_error( "write error" );
}


static void
put_big_long(l)
    long l;
{
    if ( pm_writebiglong( stdout, l ) == -1 )
        pm_error( "write error" );
}


static void
put_short_as_byte(short s)
{
    put_byte((unsigned char)s);
}


static long *
build_channels(FILE *ifp, int cols, int rows, xelval maxval, 
               int format, int bpc, int channels)
{
    int i, row, col, sgirow;
    long *table = NULL;
    ScanElem *temp;

#ifdef DEBUG
    pm_message("building channels");
#endif

    if( storage != STORAGE_VERBATIM ) {
        MALLOCARRAY_NOFAIL(table, channels * rows);
        MALLOCARRAY_NOFAIL(rletemp, WORSTCOMPR(cols));
    }
    MALLOCARRAY_NOFAIL(temp, cols);

    for( i = 0; i < channels; i++ )
        MALLOCARRAY_NOFAIL(channel[i], rows);

    for( row = 0, sgirow = rows-1; row < rows; row++, sgirow-- ) {
        pnm_readpnmrow(ifp, pnmrow, cols, maxval, format);
        if( channels == 1 ) {
            for( col = 0; col < cols; col++ )
                temp[col] = (ScanElem)PNM_GET1(pnmrow[col]);
            temp = compress(temp, sgirow, rows, cols, 0, table, bpc);
        }
        else {
            for( col = 0; col < cols; col++ )
                temp[col] = (ScanElem)PPM_GETR(pnmrow[col]);
            temp = compress(temp, sgirow, rows, cols, 0, table, bpc);
            for( col = 0; col < cols; col++ )
                temp[col] = (ScanElem)PPM_GETG(pnmrow[col]);
            temp = compress(temp, sgirow, rows, cols, 1, table, bpc);
            for( col = 0; col < cols; col++ )
                temp[col] = (ScanElem)PPM_GETB(pnmrow[col]);
            temp = compress(temp, sgirow, rows, cols, 2, table, bpc);
        }
    }

    free(temp);
    if( table )
        free(rletemp);
    return table;
}


static ScanElem *
compress(temp, row, rows, cols, chan_no, table, bpc)
    ScanElem *temp;
    int row, rows, cols, chan_no;
    long *table;
    int bpc;
{
    int len, i, tabrow;
    ScanElem *p;

    switch( storage ) {
        case STORAGE_VERBATIM:
            channel[chan_no][row].length = cols;
            channel[chan_no][row].data = temp;
            MALLOCARRAY_NOFAIL(temp, cols);
            break;
        case STORAGE_RLE:
            tabrow = chan_no * rows + row;
            len = rle_compress(temp, cols);    /* writes result into rletemp */
            channel[chan_no][row].length = len;
            MALLOCARRAY(p, len);
            channel[chan_no][row].data = p;
            for( i = 0; i < len; i++, p++ )
                *p = rletemp[i];
            table[tabrow] = len * bpc;
            break;
        default:
            pm_error("unknown storage type - can\'t happen");
    }
    return temp;
}


/*
slightly modified RLE algorithm from ppmtoilbm.c
written by Robert A. Knop (rknop@mop.caltech.edu)
*/
static int
rle_compress(inbuf, size)
    ScanElem *inbuf;
    int size;
{
    int in, out, hold, count;
    ScanElem *outbuf = rletemp;

    in=out=0;
    while( in<size ) {
        if( (in<size-1) && (inbuf[in]==inbuf[in+1]) ) {     /*Begin replicate run*/
            for( count=0,hold=in; in<size && inbuf[in]==inbuf[hold] && count<127; in++,count++)
                ;
            outbuf[out++]=(ScanElem)(count);
            outbuf[out++]=inbuf[hold];
        }
        else {  /*Do a literal run*/
            hold=out; out++; count=0;
            while( ((in>=size-2)&&(in<size)) || ((in<size-2) && ((inbuf[in]!=inbuf[in+1])||(inbuf[in]!=inbuf[in+2]))) ) {
                outbuf[out++]=inbuf[in++];
                if( ++count>=127 )
                    break;
            }
            outbuf[hold]=(ScanElem)(count | 0x80);
        }
    }
    outbuf[out++] = (ScanElem)0;     /* terminator */
    return(out);
}

