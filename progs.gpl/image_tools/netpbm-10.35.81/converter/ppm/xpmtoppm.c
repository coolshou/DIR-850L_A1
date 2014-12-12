/* xpmtoppm.c - read an X11 pixmap file and produce a portable pixmap
**
** Copyright (C) 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Upgraded to handle XPM version 3 by
**   Arnaud Le Hors (lehors@mirsa.inria.fr)
**   Tue Apr 9 1991
**
** Rainer Sinkwitz sinkwitz@ifi.unizh.ch - 21 Nov 91:
**  - Bug fix, no advance of read ptr, would not read 
**    colors like "ac c black" because it would find 
**    the "c" of "ac" and then had problems with "c"
**    as color.
**    
**  - Now understands multiword X11 color names
**  
**  - Now reads multiple color keys. Takes the color
**    of the hightest available key. Lines no longer need
**    to begin with key 'c'.
**    
**  - expanded line buffer to from 500 to 2048 for bigger files
*/

#define _BSD_SOURCE   /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <string.h>

#include "ppm.h"
#include "shhopt.h"
#include "nstring.h"
#include "mallocvar.h"

#define MAX_LINE (8 * 1024)
  /* The maximum size XPM input line we can handle. */

/* number of xpmColorKeys */
#define NKEYS 5

const char *xpmColorKeys[] =
{
 "s",					/* key #1: symbol */
 "m",					/* key #2: mono visual */
 "g4",					/* key #3: 4 grays visual */
 "g",					/* key #4: gray visual */
 "c",					/* key #5: color visual */
};

struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    char *input_filespec;  /* Filespecs of input files */
    char *alpha_filename;
    int alpha_stdout;
    int verbose;
};


static int verbose;


static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdline_p) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optStruct *option_def = malloc(100*sizeof(optStruct));
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct2 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENTRY(0,   "alphaout",   OPT_STRING, &cmdline_p->alpha_filename, 0);
    OPTENTRY(0,   "verbose",    OPT_FLAG,   &cmdline_p->verbose,        0);

    cmdline_p->alpha_filename = NULL;
    cmdline_p->verbose = FALSE;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = TRUE;  /* We may have parms that are negative numbers */

    optParseOptions2(&argc, argv, opt, 0);
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (argc - 1 == 0)
        cmdline_p->input_filespec = NULL;  /* he wants stdin */
    else if (argc - 1 == 1)
        cmdline_p->input_filespec = strdup(argv[1]);
    else 
        pm_error("Too many arguments.  The only argument accepted\n"
                 "is the input file specification");

    if (cmdline_p->alpha_filename && 
        STREQ(cmdline_p->alpha_filename, "-"))
        cmdline_p->alpha_stdout = TRUE;
    else 
        cmdline_p->alpha_stdout = FALSE;

}


static char lastInputLine[MAX_LINE+1];
    /* contents of line most recently read from input */
static bool backup;
    /* TRUE means next read should be a reread of the most recently read
       line, i.e. lastInputLine, instead of a read from the input file.
    */


static void
getline(char * const line,
        size_t const size,
        FILE * const stream) {
/*----------------------------------------------------------------------------
   Read the next line from the input file 'stream', through the one-line
   buffer lastInputLine[].

   If 'backup' is true, the "next line" is the previously read line, i.e.
   the one in that one-line buffer.  Otherwise, the "next line" is the next
   line from the real file.  After reading the backed up line, we reset 
   'backup' to false.

   Return the line as a null terminated string in *line, which is an
   array of 'size' bytes.

   Exit program if the line doesn't fit in the buffer.
-----------------------------------------------------------------------------*/
    if (size > sizeof(lastInputLine))
        pm_error("INTERNAL ERROR: getline() received 'size' parameter "
                 "which is out of bounds");

    if (backup) {
        strncpy(line, lastInputLine, size); 
        backup = FALSE;
    } else {
        if (fgets(line, size, stream) == NULL)
            pm_error("EOF or read error on input file");
        if (strlen(line) == size - 1)
            pm_error("Input file has line that is too long (longer than "
                     "%u bytes).", (unsigned)size - 1);
        STRSCPY(lastInputLine, line);
    }
}



static unsigned int
getNumber(char * const p, unsigned int const size) {

    unsigned int retval;
    unsigned char * q;
    
    retval = 0;
    for (q = p; q < p+size; ++q)
        retval = (retval << 8) + *q;

    return retval;
}



static void
getword(char * const output, char ** const cursorP) {

    char *t1;
    char *t2;

    for (t1=*cursorP; ISSPACE(*t1); t1++); /* skip white space */
    for (t2 = t1; !ISSPACE(*t2) && *t2 != '"' && *t2 != '\0'; t2++);
        /* Move to next white space, ", or eol */
    if (t2 > t1)
        strncpy(output, t1, t2 - t1);
    output[t2 - t1] = '\0';
    *cursorP = t2;
}    



static void
addToColorMap(unsigned int const seqNum,
              unsigned int const colorNumber, 
              pixel * const colors, int * const ptab, 
              char colorspec[], int const isTransparent,
              int * const transparentP) {
/*----------------------------------------------------------------------------
   Add the color named by colorspec[] to the colormap contained in
   'colors' and 'ptab', as the color associated with XPM color number
   'colorNumber', which is the seqNum'th color in the XPM color map.

   Iff 'transparent', set *transparentP to the colormap index that 
   corresponds to this color.
-----------------------------------------------------------------------------*/
    if (ptab == NULL) {
        /* Index into table. */
        colors[colorNumber] = ppm_parsecolor(colorspec,
                                             (pixval) PPM_MAXMAXVAL);
        if (isTransparent) 
            *transparentP = colorNumber;
    } else {
        /* Set up linear search table. */
        colors[seqNum] = ppm_parsecolor(colorspec,
                                        (pixval) PPM_MAXMAXVAL);
        ptab[seqNum] = colorNumber;
        if (isTransparent)
            *transparentP = seqNum;
    }
}



static void
interpretXpm3ColorTableLine(char line[], int const seqNum, 
                            int const chars_per_pixel,
                            pixel * const colors, int * const ptab,
                            int * const transparentP) {
/*----------------------------------------------------------------------------
   Interpret one line of the color table in the XPM header.  'line' is
   the line from the XPM file.  It is the seqNum'th color table entry in
   the file.  The file uses 'chars_per_pixel' characters per pixel.

   Add the information from this color table entry to the color table
   'colors' and, if it isn't NULL, the corresponding lookup shadow table
   'ptab' (see readXpm3ColorTable for a description of these data 
   structures).

   The line may include values for multiple kinds of color (grayscale,
   color, etc.).  We take the highest of these (e.g. color over grayscale).

   If a color table entry indicates transparency, set *transparentP
   to the colormap index that corresponds to the indicated color.
-----------------------------------------------------------------------------*/
    /* Note: this code seems to allow for multi-word color specifications,
       but I'm not aware that such are legal.  Ultimately, ppm_parsecolor()
       interprets the name, and I believe it only takes single word 
       color specifications.  -Bryan 2001.05.06.
    */
    char str2[MAX_LINE+1];    
    char *t1;
    char *t2;
    int endOfEntry;   /* boolean */
    
    unsigned int curkey, key, highkey;	/* current color key */
    unsigned int lastwaskey;	
        /* The last token we processes was a key, and we have processed
           at least one token.
        */
    char curbuf[BUFSIZ];		/* current buffer */
    int isTransparent;
    
    int colorNumber;
        /* A color number that appears in the raster */
    /* read the chars */
    t1 = strchr(line, '"');
    if (t1 == NULL)
        pm_error("A line that is supposed to be an entry in the color "
                 "table does not start with a quote.  The line is '%s'.  "
                 "It is the %dth entry in the color table.", 
                 line, seqNum);
    else
        t1++;  /* Points now to first color number character */

    colorNumber = getNumber(t1, chars_per_pixel);
    t1 += chars_per_pixel;

    /*
     * read color keys and values 
     */
    curkey = 0; 
    highkey = 1;
    lastwaskey = FALSE;
    t2 = t1;
    endOfEntry = FALSE;
    while ( !endOfEntry ) {
        int isKey;   /* boolean */
        getword(str2, &t2);
        if (strlen(str2) == 0)
            endOfEntry = TRUE;
        else {
            /* See if the word we got is a valid key (and get its key
               number if so)
            */
            for (key = 1; 
                 key <= NKEYS && !STREQ(xpmColorKeys[key - 1], str2); 
                 key++);
            isKey = (key <= NKEYS);

            if (lastwaskey || !isKey) {
                /* This word is a color specification (or "none" for
                   transparent).
                */
                if (!curkey) 
                    pm_error("Missing color key token in color table line "
                             "'%s' before '%s'.", line, str2);
                if (!lastwaskey) 
                    strcat(curbuf, " ");		/* append space */
                if ( (strncmp(str2, "None", 4) == 0) 
                     || (strncmp(str2, "none", 4) == 0) ) {
                    /* This entry identifies the transparent color number */
                    strcat(curbuf, "#000000");  /* Make it black */
                    isTransparent = TRUE;
                } else 
                    strcat(curbuf, str2);		/* append buf */
                lastwaskey = 0;
            } else { 
                /* This word is a key.  So we've seen the last of the 
                   info for the previous key, and we must either put it
                   in the color map or ignore it if we already have a higher
                   color form in the colormap for this colormap entry.
                */
                if (curkey > highkey) {	/* flush string */
                    addToColorMap(seqNum, colorNumber, colors, ptab, curbuf,
                                  isTransparent, transparentP);
                    highkey = curkey;
                }
                curkey = key;			/* set new key  */
                curbuf[0] = '\0';		/* reset curbuf */
                isTransparent = FALSE;
                lastwaskey = 1;
            }
            if (*t2 == '"') break;
        }
    }
    /* Put the info for the last key in the line into the colormap (or
       ignore it if there's already a higher color form for this colormap
       entry in it)
    */
    if (curkey > highkey) {
        addToColorMap(seqNum, colorNumber, colors, ptab, curbuf,
                      isTransparent, transparentP);
        highkey = curkey;
    }
    if (highkey == 1) 
        pm_error("C error scanning color table");
}



static void
readXpm3Header(FILE * const stream, int * const widthP, int * const heightP, 
               int * const chars_per_pixelP, int * const ncolorsP,
               pixel ** const colorsP, int ** const ptabP,
               int * const transparentP) {
/*----------------------------------------------------------------------------
  Read the header of the XPM file on stream 'stream'.  Assume the
  getline() stream is presently positioned to the beginning of the
  file and it is a Version 3 XPM file.  Leave the stream positioned
  after the header.

  We have two ways to return the colormap, depending on the number of
  characters per pixel in the XPM:  
  
  If it is 1 or 2 characters per pixel, we return the colormap as a
  Netpbm 'pixel' array *colorsP (in newly malloc'ed storage), such
  that if a color in the raster is identified by index N, then
  (*colorsP)[N] is that color.  So this array is either 256 or 64K
  pixels.  In this case, we return *ptabP = NULL.

  If it is more than 2 characters per pixel, we return the colormap as
  both a Netpbm 'pixel' array *colorsP and a lookup table *ptabP (both
  in newly malloc'ed storage).

  If a color in the raster is identified by index N, then for some I,
  (*ptabP)[I] is N and (*colorsP)[I] is the color in question.  So 
  you iterate through *ptabP looking for N and then look at the 
  corresponding entry in *colorsP to get the color.

  Return as *transColorNumberP the value of the XPM color number that
  represents a transparent pixel, or -1 if no color number does.
-----------------------------------------------------------------------------*/
    char line[MAX_LINE+1];
    const char * xpm3_signature = "/* XPM */";
    
    *widthP = *heightP = *ncolorsP = *chars_per_pixelP = -1;

    /* Read the XPM signature comment */
    getline(line, sizeof(line), stream);
    if (strncmp(line, xpm3_signature, strlen(xpm3_signature)) != 0) 
        pm_error("Apparent XPM 3 file does not start with '/* XPM */'.  "
                 "First line is '%s'", xpm3_signature);

    /* Read the assignment line */
    getline(line, sizeof(line), stream);
    if (strncmp(line, "static char", 11) != 0)
        pm_error("Cannot find data structure declaration.  Expected a "
                 "line starting with 'static char', but found the line "
                 "'%s'.", line);

	/* Read the hints line */
    getline(line, sizeof(line), stream);
    /* skip the comment line if any */
    if (!strncmp(line, "/*", 2)) {
        while (!strstr(line, "*/"))
            getline(line, sizeof(line), stream);
        getline(line, sizeof(line), stream);
    }
    if (sscanf(line, "\"%d %d %d %d\",", widthP, heightP,
               ncolorsP, chars_per_pixelP) != 4)
        pm_error("error scanning hints line");

    if (verbose == 1) 
    {
        pm_message("Width x Height:  %d x %d", *widthP, *heightP);
        pm_message("no. of colors:  %d", *ncolorsP);
        pm_message("chars per pixel:  %d", *chars_per_pixelP);
    }

    /* Allocate space for color table. */
    if (*chars_per_pixelP <= 2) {
        /* Set up direct index (see above) */
        *colorsP = ppm_allocrow(*chars_per_pixelP == 1 ? 256 : 256*256);
        *ptabP = NULL;
    } else {
        /* Set up lookup table (see above) */
        *colorsP = ppm_allocrow(*ncolorsP);
        MALLOCARRAY(*ptabP, *ncolorsP);
        if (*ptabP == NULL)
            pm_error("Unable to allocate memory for %d colors", *ncolorsP);
    }
    
    { 
        /* Read the color table */
        int seqNum;
            /* Sequence number of entry within color table in XPM header */

        *transparentP = -1;  /* initial value */

        for (seqNum = 0; seqNum < *ncolorsP; seqNum++) {
            getline(line, sizeof(line), stream);
            /* skip the comment line if any */
            if (!strncmp(line, "/*", 2))
                getline(line, sizeof(line), stream);
            
            interpretXpm3ColorTableLine(line, seqNum, *chars_per_pixelP, 
                                        *colorsP, *ptabP, transparentP);
        }
    }
}


static void
readXpm1Header(FILE * const stream, int * const widthP, int * const heightP, 
               int * const chars_per_pixelP, int * const ncolorsP, 
               pixel ** const colorsP, int ** const ptabP) {
/*----------------------------------------------------------------------------
  Read the header of the XPM file on stream 'stream'.  Assume the
  getline() stream is presently positioned to the beginning of the
  file and it is a Version 1 XPM file.  Leave the stream positioned
  after the header.
  
  Return the information from the header the same as for readXpm3Header.
-----------------------------------------------------------------------------*/
    char line[MAX_LINE+1], str1[MAX_LINE+1], str2[MAX_LINE+1];
    char *t1;
    char *t2;
    int format;
    unsigned int v;
    int i, j;
    bool processedStaticChar;  
        /* We have read up to and interpreted the "static char..." line */

    *widthP = *heightP = *ncolorsP = *chars_per_pixelP = format = -1;

    /* Read the initial defines. */
    processedStaticChar = FALSE;
    while (!processedStaticChar) {
        getline(line, sizeof(line), stream);

        if (sscanf(line, "#define %s %d", str1, &v) == 2) {
            char *t1;
            if ((t1 = strrchr(str1, '_')) == NULL)
                t1 = str1;
            else
                ++t1;
            if (STREQ(t1, "format"))
                format = v;
            else if (STREQ(t1, "width"))
                *widthP = v;
            else if (STREQ(t1, "height"))
                *heightP = v;
            else if (STREQ(t1, "ncolors"))
                *ncolorsP = v;
            else if (STREQ(t1, "pixel"))
                *chars_per_pixelP = v;
        } else if (!strncmp(line, "static char", 11)) {
            if ((t1 = strrchr(line, '_')) == NULL)
                t1 = line;
            else
                ++t1;
            processedStaticChar = TRUE;
        }
    }
    /* File is positioned to "static char" line, which is in line[] and
       t1 points to position of last "_" in the line, or the beginning of
       the line if there is no "_"
    */
    if (format == -1)
        pm_error("missing or invalid format");
    if (format != 1)
        pm_error("can't handle XPM version %d", format);
    if (*widthP == -1)
        pm_error("missing or invalid width");
    if (*heightP == -1)
        pm_error("missing or invalid height");
    if (*ncolorsP == -1)
        pm_error("missing or invalid ncolors");
    if (*chars_per_pixelP == -1)
        pm_error("missing or invalid *chars_per_pixelP");
    if (*chars_per_pixelP > 2)
        pm_message("WARNING: *chars_per_pixelP > 2 uses a lot of memory");

    /* If there's a monochrome color table, skip it. */
    if (!strncmp(t1, "mono", 4)) {
        for (;;) {
            getline(line, sizeof(line), stream);
            if (!strncmp(line, "static char", 11))
                break;
        }
    }
    /* Allocate space for color table. */
    if (*chars_per_pixelP <= 2) {
        /* Up to two chars per pixel, we can use an indexed table. */
        v = 1;
        for (i = 0; i < *chars_per_pixelP; ++i)
            v *= 256;
        *colorsP = ppm_allocrow(v);
        *ptabP = NULL;
    } else {
        /* Over two chars per pixel, we fall back on linear search. */
        *colorsP = ppm_allocrow(*ncolorsP);
        MALLOCARRAY(*ptabP, *ncolorsP);
        if (*ptabP == NULL)
            pm_error("Unable to allocate memory for %d colors", *ncolorsP);
    }

    /* Read color table. */
    for (i = 0; i < *ncolorsP; ++i) {
        getline(line, sizeof(line), stream);

        if ((t1 = strchr(line, '"')) == NULL)
            pm_error("D error scanning color table");
        if ((t2 = strchr(t1 + 1, '"')) == NULL)
            pm_error("E error scanning color table");
        if (t2 - t1 - 1 != *chars_per_pixelP)
            pm_error("wrong number of chars per pixel in color table");
        strncpy(str1, t1 + 1, t2 - t1 - 1);
        str1[t2 - t1 - 1] = '\0';

        if ((t1 = strchr(t2 + 1, '"')) == NULL)
            pm_error("F error scanning color table");
        if ((t2 = strchr(t1 + 1, '"')) == NULL)
            pm_error("G error scanning color table");
        strncpy(str2, t1 + 1, t2 - t1 - 1);
        str2[t2 - t1 - 1] = '\0';

        v = 0;
        for (j = 0; j < *chars_per_pixelP; ++j)
            v = (v << 8) + str1[j];
        if (*chars_per_pixelP <= 2)
            /* Index into table. */
            (*colorsP)[v] = ppm_parsecolor(str2,
                                           (pixval) PPM_MAXMAXVAL);
        else {
            /* Set up linear search table. */
            (*colorsP)[i] = ppm_parsecolor(str2,
                                           (pixval) PPM_MAXMAXVAL);
            (*ptabP)[i] = v;
        }
    }
    /* Position to first line of raster (which is the line after
       "static char ...").
    */
    for (;;) {
        getline(line, sizeof(line), stream);
        if (strncmp(line, "static char", 11) == 0)
            break;
    }
}



static void
interpretXpmLine(char   const line[],
                 int    const chars_per_pixel,
                 int    const ncolors,
                 int *  const ptab, 
                 int ** const cursorP,
                 int *  const maxCursor) {
/*----------------------------------------------------------------------------
   Interpret one line of XPM input.  The line is in 'line', and its
   format is 'chars_per_pixel' characters per pixel.  'ptab' is the
   color table that applies to the line, which table has 'ncolors'
   colors.

   Put the colormap indexes for the pixels represented in 'line' at
   *cursorP, lined up in the order they are in 'line', and return
   *cursorP positioned just after the last one.

   If the line doesn't start with a quote (e.g. it is empty), we issue
   a warning and just treat the line as one that describes no pixels.

   Stop processing the line either at the end of the line or when
   the output would go beyond maxCursor, whichever comes first.
-----------------------------------------------------------------------------*/
    char * lineCursor;

    lineCursor = strchr(line, '"');  /* position to 1st quote in line */
    if (lineCursor == NULL) {
        /* We've seen a purported XPM that had a blank line in it.  Just
           ignoring it was the right thing to do.  05.05.27.
        */
        pm_message("WARNING:  No opening quotation mark in XPM input "
                   "line which is supposed to be a line of raster data: "
                   "'%s'.  Ignoring this line.", line);
    } else {
        ++lineCursor; /* Skip to first character after quote */

        /* Handle pixels until a close quote, eol, or we've returned all
           the pixels Caller wants.
        */
        while (*lineCursor && *lineCursor != '"' && *cursorP <= maxCursor) {
            int colorNumber;
            int i;
            colorNumber = 0;  /* initial value */
            for (i = 0; i < chars_per_pixel; ++i)
                colorNumber = (colorNumber << 8) + *(lineCursor++);
            if (ptab == NULL)
                /* colormap is indexed directly by XPM color number */
                *(*cursorP)++ = colorNumber;
            else {
                /* colormap shadows ptab[].  Find this color # in ptab[] */
                int i;
                for (i = 0; i < ncolors && ptab[i] != colorNumber; ++i);
                if (i < ncolors)
                    *(*cursorP)++ = i;
                else
                    pm_error("Color number %d is in raster, but not in "
                             "colormap.  Line it's in: '%s'",
                             colorNumber, line);
            }
        }
    }
}



static void
ReadXPMFile(FILE * const stream, int * const widthP, int * const heightP, 
            pixel ** const colorsP, int ** const dataP, 
            int * const transparentP) {
/*----------------------------------------------------------------------------
   Read the XPM file from stream 'stream'.

   Return the dimensions of the image as *widthP and *heightP.
   Return the color map as *colorsP, which is an array of *ncolorsP
   colors.

   Return the raster in newly malloced storage, an array of *widthP by
   *heightP integers, each of which is an index into the colormap
   *colorsP (and therefore less than *ncolorsP).  Return the address
   of the array as *dataP.

   In the colormap, put black for the transparent color, if the XPM 
   image contains one.
-----------------------------------------------------------------------------*/
    char line[MAX_LINE+1], str1[MAX_LINE+1];
    int totalpixels;
    int *cursor;  /* cursor into *dataP */
    int *maxcursor;  /* value of above cursor for last pixel in image */
    int *ptab;   /* colormap - malloc'ed */
    int rc;
    int ncolors;
    int chars_per_pixel;

    backup = FALSE;

    /* Read the header line */
    getline(line, sizeof(line), stream);
    backup = TRUE;  /* back up so next read reads this line again */
    
    rc = sscanf(line, "/* %s */", str1);
    if (rc == 1 && strncmp(str1, "XPM", 3) == 0) {
        /* It's an XPM version 3 file */
        readXpm3Header(stream, widthP, heightP, &chars_per_pixel,
                       &ncolors, colorsP, &ptab, transparentP);
    } else {				/* try as an XPM version 1 file */
        /* Assume it's an XPM version 1 file */
        readXpm1Header(stream, widthP, heightP, &chars_per_pixel, 
                       &ncolors, colorsP, &ptab);
        *transparentP = -1;  /* No transparency in version 1 */
    }
    totalpixels = *widthP * *heightP;
    MALLOCARRAY(*dataP, totalpixels);
    if (*dataP == NULL)
        pm_error("Could not get %d bytes of memory for image", totalpixels);
    cursor = *dataP;
    maxcursor = *dataP + totalpixels - 1;
	getline(line, sizeof(line), stream); 
        /* read next line (first line may not always start with comment) */
    while (cursor <= maxcursor) {
        if (strncmp(line, "/*", 2) == 0) {
            /* It's a comment.  Ignore it. */
        } else {
            interpretXpmLine(line, chars_per_pixel, 
                             ncolors, ptab, &cursor, maxcursor);
        }
        if (cursor <= maxcursor)
            getline(line, sizeof(line), stream);
    }
    if (ptab) free(ptab);
}
 


static void
writeOutput(FILE * const imageout_file,
            FILE * const alpha_file,
            int const cols, int const rows, 
            pixel * const colors, int * const data,
            int transparent) {
/*----------------------------------------------------------------------------
   Write the image in 'data' to open PPM file stream 'imageout_file',
   and the alpha mask for it to open PBM file stream 'alpha_file',
   except if either is NULL, skip it.

   'data' is an array of cols * rows integers, each one being an index
   into the colormap 'colors'.

   Where the index 'transparent' occurs in 'data', the pixel is supposed
   to be transparent.  If 'transparent' < 0, no pixels are transparent.
-----------------------------------------------------------------------------*/
    int row;
    pixel *pixrow;
    bit * alpharow;

    if (imageout_file)
        ppm_writeppminit(imageout_file, cols, rows, PPM_MAXMAXVAL, 0);
    if (alpha_file)
        pbm_writepbminit(alpha_file, cols, rows, 0);

    pixrow = ppm_allocrow(cols);
    alpharow = pbm_allocrow(cols);

    for (row = 0; row < rows; ++row ) {
        int col;
        int * const datarow = data+(row*cols);

        for (col = 0; col < cols; ++col) {
            pixrow[col] = colors[datarow[col]];
            if (datarow[col] == transparent)
                alpharow[col] = PBM_BLACK;
            else
                alpharow[col] = PBM_WHITE;
        }
        if (imageout_file)
            ppm_writeppmrow(imageout_file, 
                            pixrow, cols, (pixval) PPM_MAXMAXVAL, 0);
        if (alpha_file)
            pbm_writepbmrow(alpha_file, alpharow, cols, 0);
    }
    ppm_freerow(pixrow);
    pbm_freerow(alpharow);

    if (imageout_file)
        pm_close(imageout_file);
    if (alpha_file)
        pm_close(alpha_file);
}    



int
main(int argc, char *argv[]) {

    FILE *ifp;
    FILE *alpha_file, *imageout_file;
    pixel *colormap;
    int cols, rows;
    int transparent;  /* value of 'data' that means transparent */
    int *data;
        /* The image as an array of width * height integers, each one
           being an index int colormap[].
        */

    struct cmdline_info cmdline;

    ppm_init(&argc, argv);

    parse_command_line(argc, argv, &cmdline);

    verbose = cmdline.verbose;

    if ( cmdline.input_filespec != NULL ) 
        ifp = pm_openr( cmdline.input_filespec);
    else
        ifp = stdin;

    if (cmdline.alpha_stdout)
        alpha_file = stdout;
    else if (cmdline.alpha_filename == NULL) 
        alpha_file = NULL;
    else {
        alpha_file = pm_openw(cmdline.alpha_filename);
    }

    if (cmdline.alpha_stdout) 
        imageout_file = NULL;
    else
        imageout_file = stdout;

    ReadXPMFile(ifp, &cols, &rows, &colormap, &data, &transparent);
    
    pm_close(ifp);

    writeOutput(imageout_file, alpha_file, cols, rows, colormap, data,
                transparent);

    free(colormap);
    
    return 0;
}
