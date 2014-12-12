/* pbmtoxbm.c - read a portable bitmap and produce an X11 bitmap file
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

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <string.h>

#include "pbm.h"
#include "nstring.h"


static void
generateName(char const filenameArg[], const char ** const nameP) {
/*----------------------------------------------------------------------------
   Generate a name for the image to put in the bitmap file.  Derive it from
   the filename argument filenameArg[] and return it as a null-terminated
   string in newly malloc'ed space at *nameP.

   We take the part of the name after the rightmost slash
   (i.e. filename without the directory path part), stopping before
   any period.  We convert any punctuation to underscores.

   If the argument is "-", meaning standard input, we return the name
   "noname".  Also, if the argument is null or ends in a slash, we
   return "noname".
-----------------------------------------------------------------------------*/
    if (STREQ(filenameArg, "-"))
        *nameP = strdup("noname");
    else {
        int nameIndex, argIndex;
        /* indices into the input and output buffers */

        /* Start just after the rightmost slash, or at beginning if no slash */
        if (strrchr(filenameArg, '/') == 0) 
            argIndex = 0;
        else argIndex = strrchr(filenameArg, '/') - filenameArg + 1;

        if (filenameArg[argIndex] == '\0') 
            *nameP = strdup("noname");
        else {
            char * name;
            nameIndex = 0;  /* Start at beginning of name buffer */

            name = malloc(strlen(filenameArg));
    
            while (filenameArg[argIndex] != '\0' 
                   && filenameArg[argIndex] != '.') {
                const char filenameChar = filenameArg[argIndex++];
                name[nameIndex++] = 
                    ISALNUM(filenameChar) ? filenameChar : '_';
            }
            name[nameIndex] = '\0';
            *nameP = name;
        }
    }
}



int
main(int argc, char * argv[]) {

    FILE* ifp;
    bit* bitrow;
    int rows, cols, format;
    int padright;
    int row;
    const char * inputFilename;
    const char *name;
    int itemsperline;
    int bitsperitem;
    int item;
    int firstitem;
    const char hexchar[] = "0123456789abcdef";

    pbm_init(&argc, argv);

    if (argc-1 > 1)
        pm_error("Too many arguments (%d).  The only valid argument is an "
                 "input file name.", argc-1);
    else if (argc-1 == 1) 
        inputFilename = argv[1];
    else
        inputFilename = "-";

    generateName(inputFilename, &name);
    ifp = pm_openr(inputFilename);
    
    pbm_readpbminit(ifp, &cols, &rows, &format);
    bitrow = pbm_allocrow(cols);
    
    /* Compute padding to round cols up to the nearest multiple of 8. */
    padright = ((cols + 7)/8) * 8 - cols;

    printf("#define %s_width %d\n", name, cols);
    printf("#define %s_height %d\n", name, rows);
    printf("static char %s_bits[] = {\n", name);

    itemsperline = 0;
    bitsperitem = 0;
    item = 0;
    firstitem = 1;

#define PUTITEM \
    { \
    if ( firstitem ) \
        firstitem = 0; \
    else \
        putchar( ',' ); \
    if ( itemsperline == 15 ) \
        { \
        putchar( '\n' ); \
        itemsperline = 0; \
        } \
    if ( itemsperline == 0 ) \
        putchar( ' ' ); \
    ++itemsperline; \
    putchar('0'); \
    putchar('x'); \
    putchar(hexchar[item >> 4]); \
    putchar(hexchar[item & 15]); \
    bitsperitem = 0; \
    item = 0; \
    }

#define PUTBIT(b) \
    { \
    if ( bitsperitem == 8 ) \
        PUTITEM; \
    if ( (b) == PBM_BLACK ) \
        item += 1 << bitsperitem; \
    ++bitsperitem; \
    }

    for (row = 0; row < rows; ++row) {
        int col;
        pbm_readpbmrow(ifp, bitrow, cols, format);
        for (col = 0; col < cols; ++col)
            PUTBIT(bitrow[col]);
        for (col = 0; col < padright; ++col)
            PUTBIT(0);
    }

    pm_close(ifp);

    if (bitsperitem > 0)
        PUTITEM;
    printf("};\n");

    pbm_freerow(bitrow);

    strfree(name);

    exit(0);
}
