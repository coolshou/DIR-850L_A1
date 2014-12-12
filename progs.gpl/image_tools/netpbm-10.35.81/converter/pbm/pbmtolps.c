/*
 * pbmtolps -- convert a Portable BitMap into Postscript.  The
 * output Postscript uses lines instead of the image operator to
 * generate a (device dependent) picture which will be imaged
 * much faster.
 *
 * The Postscript path length is constrained to be less that 1000
 * points so that no limits are overrun on the Apple Laserwriter
 * and (presumably) no other printers.
 *
 * To do:
 *      make sure encapsulated format is correct
 *      repitition of black-white strips
 *      make it more device independent (is this possible?)
 *
 * Author:
 *      George Phillips <phillips@cs.ubc.ca>
 *      Department of Computer Science
 *      University of British Columbia
 */

#include <string.h>
#include <stdio.h>

#include "nstring.h"
#include "pbm.h"


static int prev_white = -1;
static int prev_black = -1;
static char cmd = '\0';
static int pointcount = 2;

#ifdef RUN
static int run = 1;
#endif

static char 
morepoints(char cmd, int howmany_pbmtolps) {
    pointcount += 2;
    if (pointcount > 1000) {
        pointcount = 2;
        cmd += 'm' - 'a';
    }
    return(cmd);
}



static void 
addstrip(int const white, 
         int const black) {

    if (cmd) {
#ifdef RUN
        if (white == prev_white && black == prev_black)
            run++;
        else {
            if (run == 1)
#endif
                printf("%d %d %c ", 
                       prev_black, prev_white, morepoints(cmd, 2));
#ifdef RUN
            else
                                /* of course, we need to give a new command */
                printf("%d %d %d %c ",
                       prev_white, prev_black, run,
                       morepoints(cmd + 'f' - 'a', 2 * run));
            run = 1;
        }
#endif
    }

    prev_white = white;
    prev_black = black;
    cmd = 'a';
}



static void 
nextline(void) {
    /* need to check run, should have an outcommand */
    if (cmd)
        printf("%d %d %c\n", prev_black, prev_white, morepoints('c', 3));
    else
        printf("%c\n", morepoints('b', 1));
    cmd = '\0';
}



int
main(int argc, char ** argv) {
    FILE*   fp;
    bit*    bits;
    int             row;
    int             col;
    int         rows;
    int             cols;
    int             format;
    int             white;
    int             black;
    const char*   name;
    float   dpi = 300.0;
    float   sc_rows;
    float   sc_cols;
    int             i;
    const char*   const usage = "[ -dpi n ] [ pbmfile ]";


	pbm_init(&argc, argv);

    i = 1;
    if (i < argc && STREQ(argv[i], "-dpi")) {
        if (i == argc - 1)
            pm_usage(usage);
        sscanf(argv[i + 1], "%f", &dpi);
        i += 2;
    }

    if (i < argc - 1)
        pm_usage(usage);

    if (i == argc) {
        name = "noname";
        fp = stdin;
    } else {
        name = argv[i];
        fp = pm_openr(name);
    }
    pbm_readpbminit(fp, &cols, &rows, &format);
    bits = pbm_allocrow(cols);

    sc_rows = (float)rows / dpi * 72.0;
    sc_cols = (float)cols / dpi * 72.0;

    puts("%!PS-Adobe-2.0 EPSF-2.0");
    puts("%%Creator: pbmtolps");
    printf("%%%%Title: %s\n", name);
    printf("%%%%BoundingBox: %d %d %d %d\n",
           (int)(305.5 - sc_cols / 2.0),
           (int)(395.5 - sc_rows / 2.0),
           (int)(306.5 + sc_cols / 2.0),
           (int)(396.5 + sc_rows / 2.0));
    puts("%%EndComments");
    puts("%%EndProlog");
    puts("gsave");

    printf("%f %f translate\n", 306.0 - sc_cols / 2.0, 396.0 + sc_rows / 2.0);
    printf("72 %f div dup neg scale\n", dpi);
    puts("/a { 0 rmoveto 0 rlineto } def");
    puts("/b { 0 row 1 add dup /row exch def moveto } def");
    puts("/c { a b } def");
    puts("/m { currentpoint stroke newpath moveto a } def");
    puts("/n { currentpoint stroke newpath moveto b } def");
    puts("/o { currentpoint stroke newpath moveto c } def");
    puts("/row 0 def");
    puts("newpath 0 0 moveto");

    for (row = 0; row < rows; row++) {
        pbm_readpbmrow(fp, bits, cols, format);
        /* output white-strip+black-strip sequences */
        for (col = 0; col < cols; ) {
            for (white = 0; col < cols && bits[col] == PBM_WHITE; col++)
                white++;
            for (black = 0; col < cols && bits[col] == PBM_BLACK; col++)
                black++;

            if (black != 0)
                addstrip(white, black);
        }
        nextline();
    }
    puts("stroke grestore showpage");
    puts("%%Trailer");

    pm_close(fp);

    exit(0);
}
