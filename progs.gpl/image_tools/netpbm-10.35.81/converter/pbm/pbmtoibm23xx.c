/*
 * pbmtoibm23xx -- print pbm file on IBM 23XX printers
 * Copyright (C) 2004  Jorrit Fahlke <jorrit@jorrit.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/*
 * This prgram is primarily based on the description of Brothers PPDS
 * emulation (see
 * http://www.brother.de/download/send_file.cfm?file_name=guide_ibmpro.pdf).
 * However, there are some differences.  Their document states that
 * ESC J does linefeed in terms of 1/216" -- my printer clearly does
 * it in terms of 1/240".  Also, the quick and the slow mode for
 * double density printing really makes a difference on my printer,
 * the result of printing tiger.ps in quick double density mode was
 * worse than printing it in single density mode.
 *
 * If anyone Knows of any better documentation of the language used by
 * the IBM 23XX or PPDS in general, please send a mail to
 * Jö Fahlke <jorrit@jorrit.de>.
 *
 * All the graphics modes of the printer differ only in the resolution
 * in x they provide (and how quick they do their job).  They print a
 * line of 8 pixels height and variable widths.  The bitlines within
 * the line are 1/60" apart, so that is the resolution you can
 * normally achieve in y.  But the printer is able to do line feeds in
 * terms of 1/240", so the trick to print in higher resolutions is to
 * print in several interleaved passes, and do a line feed of 1/240"
 * or 1/120" inbetween.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pbm.h"
#include "shhopt.h"
#include "mallocvar.h"

struct cmdlineInfo {
    unsigned char graph_mode;
    unsigned int passes;
    unsigned int nFiles;
    const char ** inputFile;
};


bool sent_xon; /* We have send x-on to enable the printer already */




static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {

    optStruct3 opt;
    optEntry option_def[100];

    unsigned int option_def_index = 0;
    unsigned int xresSpec, yresSpec;
    unsigned int xres, yres;
    unsigned int slowMode;

    OPTENT3(0, "xres", OPT_UINT, &xres, &xresSpec, 0);
    OPTENT3(0, "yres", OPT_UINT, &yres, &yresSpec, 0);
    OPTENT3(0, "slow", OPT_FLAG, NULL,  &slowMode, 0);

    opt.opt_table = option_def;
    opt.short_allowed = 0;
    opt.allowNegNum = 0;

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!xresSpec)
        pm_error("You must specify the -xres option");
    if (!yresSpec)
        pm_error("You must specify the -yres option");

    switch (xres) {
    case  60: cmdlineP->graph_mode = 'K';                  break;
    case 120: cmdlineP->graph_mode = slowMode ? 'L' : 'Y'; break;
    case 240: cmdlineP->graph_mode = 'Z';                  break;
    default:
        pm_error("Please specify 60, 120, or 240 for -xres");
    }

    if (yres != 60 && yres != 120 && yres != 240)
        pm_error("Please specify 60, 120, or 240 for -yres");

    cmdlineP->passes = yres / 60;

    cmdlineP->nFiles = MAX(argc-1, 1);
    MALLOCARRAY_NOFAIL(cmdlineP->inputFile, cmdlineP->nFiles);

    if (argc-1 < 1)
        cmdlineP->inputFile[0] = "-";
    else {
        unsigned int i;
        for (i = 0; i < argc-1; ++i)
            cmdlineP->inputFile[i] = argv[i+1];
    }
}



/* Read all pbm images from a filehandle and print them */
static void 
process_handle(FILE *        const fh,
               unsigned char const graph_mode,
               unsigned int  const passes) {
    int eof;

    while(pbm_nextimage(fh, &eof), eof == 0) {
        /* pbm header dats */
        int cols, rows, format;
        /* iteration variables */
        unsigned int x, y;
        unsigned int bitline; /* pixel line within a sigle printing line */
        unsigned int pass;
        /* here we build the to-be-printed data */
        unsigned char *output;  /* for reading one row from the file */
        bit *row;

        /* Enable printer in case it is disabled, do it only once */
        if(!sent_xon) {
            putchar(0x11);
            sent_xon = TRUE;
        }

        pbm_readpbminit(fh, &cols, &rows, &format);

        output = malloc(sizeof(*output) * cols * passes);
        if(output == NULL)
            pm_error("Out of memory");
        row = pbm_allocrow(cols);

        for(y = 0; y < rows; y += 8 * passes) {
            memset(output, 0, sizeof(*output) * cols * passes);
            for(bitline = 0; bitline < 8; ++bitline)
                for(pass = 0; pass < passes; ++pass)
                    /* don't read beyond the end of the image if
                       height is not a multiple of passes 
                    */
                    if(y + bitline * passes + pass < rows) {
                        pbm_readpbmrow(fh, row, cols, format);
                        for(x = 0; x < cols; ++x)
                            if(row[x] == PBM_BLACK)
                                output[cols * pass + x] |= 1 << (7 - bitline);
                    }
            for(pass = 0; pass < passes; ++pass){
                /* write graphics data */
                putchar(0x1b); putchar(graph_mode);
                putchar(cols & 0xff); putchar((cols >> 8) & 0xff);
                fwrite(output + pass * cols, sizeof(*output), cols, stdout);

                /* Carriage return */
                putchar('\r');

                /* move one pixel down */
                putchar(0x1b); putchar('J'); putchar(4 / passes);
            }

            /* move one line - passes pixel down */
            putchar(0x1b); putchar('J'); putchar(24 - 4);
        }
        putchar(0x0c); /* Form-feed */

        pbm_freerow(row);
        free(output);
    }
}



int 
main(int argc,char **argv) {

  struct cmdlineInfo cmdline;
  unsigned int i;

  pbm_init(&argc, argv);

  parseCommandLine(argc, argv, &cmdline);

  sent_xon = FALSE;

  for (i = 0; i < cmdline.nFiles; ++i) {
      FILE *ifP;
      pm_message("opening '%s'", cmdline.inputFile[i]);
      ifP = pm_openr(cmdline.inputFile[i]);
      process_handle(ifP, cmdline.graph_mode, cmdline.passes);
      pm_close(ifP);
  }

  free(cmdline.inputFile);

  return 0;
}
