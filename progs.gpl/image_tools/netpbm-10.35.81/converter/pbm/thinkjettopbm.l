%pointer
/*
 * A user reported in January 2005 a problem building Thinkjettopbm
 * in which the opening comment delimiter above for some reason did
 * not make it into the Lex output in the Netpbm build of this.
 * Needless to say, that would not compile.  This user was using
 * 'lex -t' on Tru64.  We did not find it worthwhile to debug it.
 *
 * Simple FLEX scanner to convert HP ThinkJet graphics image
 * to PBM format.
 *
 * Implements a small subset of ThinkJet commands.
 *
 * Copyright (C) 2001 by W. Eric Norum
 *
 * Department of Electrical Engineering
 * University of Saskatchewan
 * Saskatoon, Saskatchewan, CANADA
 * eric.norum@usask.ca
 *
 *  Permission to use, copy, modify, and distribute this software and
 *  its documentation for any purpose and without fee is hereby granted,
 *  provided that the above copyright notice appear in all copies and
 *  that both that copyright notice and this permission notice appear in
 *  supporting documentation.  This software is provided "as is" without
 *  express or implied warranty.
 *
 *  Modified 2001.04.05 by Bryan Henderson for inclusion in the Netpbm
 *  package.  Now uses Netpbm libraries and, for consistency with other
 *  Netpbm programs, does not have PGM output option.
 */

%{

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "pbm.h"
#include "shhopt.h"

/* The following macro definitions tell Lex what sort of code to generate.
   GNU Flex does #if XXX for some of these, as opposed to #ifdef XXX, which
   means that they properly have to be set to zero instead of just left
   undefined.  (Simply leaving them undefined typically works anyway, but it
   is a problem if you use compiler options that say to fail when someone
   uses a macro he failed to define).
*/
#define YY_NO_UNPUT
#define YY_STACK_USED 0
#define YY_ALWAYS_INTERACTIVE 0
#define YY_NEVER_INTERACTIVE 0
#define YY_MAIN 0
    /* Don't include the main() function.  We have our own */

static int yylex(void);
static int yywrap(void);
/* This works, but generates a warning 
static void yyrestart(FILE*);
*/

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */

    unsigned int debug;
};



struct RowInfo {
    int     length;    /* length, in bytes */
    char    *bits;     /* Bitmap */
};

static int maxRowLength;
static int rowCount;
static int rowCapacity;
static struct RowInfo *rows;

static int column;

int debugFlag;
static void debug (const char *format, ...);

%}

DIG             [0-9]

%x RASTERMODE ROWMODE

%%

<ROWMODE>[\0-\377]      {
                        rows[rowCount].bits[column++] = yytext[0]; 
                        if (column >= rows[rowCount].length) {
                            rowCount++;
                            debug ("Done %d-byte row %d.\n", column, rowCount);
                            BEGIN (RASTERMODE);
                        }
                        }

<RASTERMODE>\033\*b{DIG}+W  {
                            int l;
                            if (rowCount >= rowCapacity) {
                                rowCapacity += 100;
                                rows = realloc (rows, rowCapacity * sizeof *rows);
                                if (rows == NULL)
                                    pm_error ("Out of memory.");
                            }
                            l = atoi (yytext+3);
                            rows[rowCount].length = l;
                            rows[rowCount].bits = malloc (l);
                            if (rows[rowCount].bits == NULL)
                                pm_error ("Out of memory.");
                            if (l > maxRowLength)
                                maxRowLength = l;
                            debug ("Start %d-byte row.\n", l);
                            column = 0;
                            BEGIN (ROWMODE);
                            }

<RASTERMODE>\033\*rB   {
                       debug ("Match <esc>*rB\n");
                       BEGIN (0);
                       }

<RASTERMODE>[.\0\n]    { pm_error ("Unexpected character (%#x) in raster mode.\n", yytext[0]); }

\033\&l{DIG}+.         { debug ("Match <esc>&l\n"); }
\033\*r{DIG}+S         { debug ("Match <esc>*r#S\n"); }
\033\*b{DIG}+W         { debug ("Match <esc>*r#w\n"); }
\033\*rA               {
                       debug ("Match <esc>*rA\n");
                       BEGIN (RASTERMODE);
                       }

[\0-\377]               { /* Silently consume all other characters */ }

%%

static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0,   "debug",      OPT_FLAG, NULL, &cmdlineP->debug,    0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 > 1)
        pm_error("Too many parameters.  There is at most one: the input "
                 "file specification.  You specified %d", argc-1);
    else
        cmdlineP->inputFilespec = argv[1];
}



/*
 * Application entry point
 */
int
main (int argc, char **argv)
{
    struct cmdlineInfo cmdline;

    pbm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    if (strlen(cmdline.inputFilespec) > 0) {
        FILE * ifP;
        ifP = freopen(cmdline.inputFilespec, "rb", stdin);
        if (ifP == NULL)
            pm_error("Unable to open file '%s' as stdin.  errno=%d (%s)", 
                     cmdline.inputFilespec, errno, strerror(errno));
    }
    debugFlag = cmdline.debug;
    yylex ();
    return 0;
}

/*
 * Finish at end of file
 */
static int 
yywrap (void)
{
    int row;
    unsigned char * packed_bitrow;

    debug ("Got %d rows, %d columns\n", rowCount, maxRowLength);

    /*
     * Quite simple since ThinkJet bit arrangement matches PBM
     */
    pbm_writepbminit(stdout, maxRowLength*8, rowCount, 0);

    packed_bitrow = malloc(maxRowLength);
    if (packed_bitrow == NULL) pm_error("Out of memory");

    for (row = 0 ; row < rowCount ; row++) {
        int col;
        for (col = 0 ; col < rows[row].length ; col++) 
            packed_bitrow[col] = rows[row].bits[col];
        for (        ; col < maxRowLength;      col++)
            packed_bitrow[col] = 0;
        pbm_writepbmrow_packed(stdout, packed_bitrow, maxRowLength*8, 0);
    }
    free(packed_bitrow);
    return 1;
}

/*
 * Print debugging message
 */
static void
debug (const char *format, ...)
{
    va_list args;

    if (debugFlag) {
        fprintf (stderr, "thinkjettopbm: ");
        va_start (args, format);
        vfprintf (stderr, format, args);
        va_end (args);
    }
}

