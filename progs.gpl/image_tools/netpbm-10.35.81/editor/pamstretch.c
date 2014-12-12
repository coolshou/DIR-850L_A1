/* pamstretch - scale up portable anymap by interpolating between pixels.
 * 
 * This program is based on 'pnminterp' by Russell Marks, rename
 * pnmstretch for inclusion in Netpbm, then rewritten and renamed to
 * pamstretch by Bryan Henderson in December 2001.
 *
 * Copyright (C) 1998,2000 Russell Marks.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pam.h"
#include "shhopt.h"

enum an_edge_mode {
    EDGE_DROP,
        /* drop one (source) pixel at right/bottom edges. */
    EDGE_INTERP_TO_BLACK,
        /* interpolate right/bottom edge pixels to black. */
    EDGE_NON_INTERP
        /* don't interpolate right/bottom edge pixels 
           (default, and what zgv does). */
};


struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespecs of input files */
    enum an_edge_mode edge_mode;
    unsigned int xscale;
    unsigned int yscale;
};



tuple blackTuple;  
   /* A "black" tuple.  Unless our input image is PBM, PGM, or PPM, we
      don't really know what "black" means, so this is just something
      arbitrary in that case.
      */


static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdline_p) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optStruct3 opt;  /* set by OPTENT3 */
    optEntry *option_def = malloc(100*sizeof(optEntry));
    unsigned int option_def_index;

    unsigned int blackedge;
    unsigned int dropedge;
    unsigned int xscale_spec;
    unsigned int yscale_spec;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3('b', "blackedge",    OPT_FLAG, NULL, &blackedge,            0);
    OPTENT3('d', "dropedge",     OPT_FLAG, NULL, &dropedge,             0);
    OPTENT3(0,   "xscale",       OPT_UINT, 
            &cmdline_p->xscale, &xscale_spec, 0);
    OPTENT3(0,   "yscale",       OPT_UINT, 
            &cmdline_p->yscale, &yscale_spec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE; /* We have some short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */

    if (blackedge && dropedge) 
        pm_error("Can't specify both -blackedge and -dropedge options.");
    else if (blackedge)
        cmdline_p->edge_mode = EDGE_INTERP_TO_BLACK;
    else if (dropedge)
        cmdline_p->edge_mode = EDGE_DROP;
    else
        cmdline_p->edge_mode = EDGE_NON_INTERP;

    if (xscale_spec && cmdline_p->xscale == 0)
        pm_error("You specified zero for the X scale factor.");
    if (yscale_spec && cmdline_p->yscale == 0)
        pm_error("You specified zero for the Y scale factor.");

    if (xscale_spec && !yscale_spec)
        cmdline_p->yscale = 1;
    if (yscale_spec && !xscale_spec)
        cmdline_p->xscale = 1;

    if (!(xscale_spec || yscale_spec)) {
        /* scale must be specified in an argument */
        if ((argc-1) != 1 && (argc-1) != 2)
            pm_error("Wrong number of arguments (%d).  Without scale options, "
                     "you must supply 1 or 2 arguments:  scale and "
                     "optional file specification", argc-1);
        
        {
            char *endptr;   /* ptr to 1st invalid character in scale arg */
            unsigned int scale;
            
            scale = strtol(argv[1], &endptr, 10);
            if (*argv[1] == '\0') 
                pm_error("Scale argument is a null string.  "
                         "Must be a number.");
            else if (*endptr != '\0')
                pm_error("Scale argument contains non-numeric character '%c'.",
                         *endptr);
            else if (scale < 2)
                pm_error("Scale argument must be at least 2.  "
                         "You specified %d", scale);
            cmdline_p->xscale = scale;
            cmdline_p->yscale = scale;
        }
        if (argc-1 > 1) 
            cmdline_p->input_filespec = argv[2];
        else
            cmdline_p->input_filespec = "-";
    } else {
        /* No scale argument allowed */
        if ((argc-1) > 1)
            pm_error("Too many arguments (%d).  With a scale option, "
                     "the only argument is the "
                     "optional file specification", argc-1);
        if (argc-1 > 0) 
            cmdline_p->input_filespec = argv[1];
        else
            cmdline_p->input_filespec = "-";
    }
}



static void
stretch_line(struct pam * const inpamP, 
             const tuple * const line, const tuple * const line_stretched, 
             unsigned int const scale, enum an_edge_mode const edge_mode) {
/*----------------------------------------------------------------------------
   Stretch the line of tuples 'line' into the output buffer 'line_stretched',
   by factor 'scale'.
-----------------------------------------------------------------------------*/
    int scaleincr;
    int sisize;   
        /* normalizing factor to make fractions representable as integers.
           E.g. if sisize = 100, one half is represented as 50.
        */
    unsigned int col;
    unsigned int outcol;
    
    sisize=0;
    while (sisize<256) 
        sisize += scale;
    scaleincr = sisize/scale;  /* (1/scale, normalized) */

    outcol = 0;  /* initial value */

    for (col = 0; col < inpamP->width; ++col) {
        unsigned int pos;
            /* The fraction of the way we are from curline to nextline,
               normalized by sisize.
            */
        if (col >= inpamP->width-1) {
            /* We're at the edge.  There is no column to the right with which
               to interpolate.
            */
            switch(edge_mode) {
            case EDGE_DROP:
                /* No output column needed for this input column */
                break;
            case EDGE_INTERP_TO_BLACK: {
                unsigned int pos;
                for (pos = 0; pos < sisize; pos += scaleincr) {
                    unsigned int plane;
                    for (plane = 0; plane < inpamP->depth; ++plane)
                        line_stretched[outcol][plane] = 
                            (line[col][plane] * (sisize-pos)) / sisize;
                    ++outcol;
                }
            }
            break;
            case EDGE_NON_INTERP: {
                unsigned int pos;
                for (pos = 0; pos < sisize; pos += scaleincr) {
                    unsigned int plane;
                    for (plane = 0; plane < inpamP->depth; ++plane)
                        line_stretched[outcol][plane] = line[col][plane];
                    ++outcol;
                }
            }
            break;
            default: 
                pm_error("INTERNAL ERROR: invalid value for edge_mode");
            }
        } else {
            /* Interpolate with the next input column to the right */
            for (pos = 0; pos < sisize; pos += scaleincr) {
                unsigned int plane;
                for (plane = 0; plane < inpamP->depth; ++plane)
                    line_stretched[outcol][plane] = 
                        (line[col][plane] * (sisize-pos) 
                         +  line[col+1][plane] * pos) / sisize;
                ++outcol;
            }
        }
    }
}



static void 
write_interp_rows(struct pam *      const outpamP,
                  const tuple *     const curline,
                  const tuple *     const nextline, 
                  tuple *           const outbuf,
                  int               const scale) {
/*----------------------------------------------------------------------------
   Write out 'scale' rows, being 'curline' followed by rows that are 
   interpolated between 'curline' and 'nextline'.
-----------------------------------------------------------------------------*/
    unsigned int scaleincr;
    unsigned int sisize;
    unsigned int pos;

    sisize=0;
    while(sisize<256) sisize+=scale;
    scaleincr=sisize/scale;

    for (pos = 0; pos < sisize; pos += scaleincr) {
        unsigned int col;
        for (col = 0; col < outpamP->width; ++col) {
            unsigned int plane;
            for (plane = 0; plane < outpamP->depth; ++plane) 
                outbuf[col][plane] = (curline[col][plane] * (sisize-pos)
                    + nextline[col][plane] * pos) / sisize;
        }
        pnm_writepamrow(outpamP, outbuf);
    }
}



static void
swap_buffers(tuple ** const buffer1P, tuple ** const buffer2P) {
    /* Advance "next" line to "current" line by switching
       line buffers 
    */
    tuple *tmp;

    tmp = *buffer1P;
    *buffer1P = *buffer2P;
    *buffer2P = tmp;
}


static void 
stretch(struct pam * const inpamP, struct pam * const outpamP,
        int const xscale, int const yscale,
        enum an_edge_mode const edge_mode) {

    tuple *linebuf1, *linebuf2;  /* Input buffers for two rows at a time */
    tuple *curline, *nextline;   /* Pointers to one of the two above buffers */
    /* And the stretched versions: */
    tuple *stretched_linebuf1, *stretched_linebuf2;
    tuple *curline_stretched, *nextline_stretched;

    tuple *outbuf;   /* One-row output buffer */
    unsigned int row;
    unsigned int rowsToStretch;
    
    linebuf1 =           pnm_allocpamrow(inpamP);
    linebuf2 =           pnm_allocpamrow(inpamP);
    stretched_linebuf1 = pnm_allocpamrow(outpamP);
    stretched_linebuf2 = pnm_allocpamrow(outpamP);
    outbuf =             pnm_allocpamrow(outpamP);

    curline = linebuf1;
    curline_stretched = stretched_linebuf1;
    nextline = linebuf2;
    nextline_stretched = stretched_linebuf2;

    pnm_readpamrow(inpamP, curline);
    stretch_line(inpamP, curline, curline_stretched, xscale, edge_mode);

    if (edge_mode == EDGE_DROP) 
        rowsToStretch = inpamP->height - 1;
    else
        rowsToStretch = inpamP->height;
    
    for (row = 0; row < rowsToStretch; row++) {
        if (row == inpamP->height-1) {
            /* last line is about to be output. there is no further
             * `next line'.  if EDGE_DROP, we stop here, with output
             * of rows-1 rows.  if EDGE_INTERP_TO_BLACK we make next
             * line black.  if EDGE_NON_INTERP (default) we make it a
             * copy of the current line.  
             */
            switch (edge_mode) {
            case EDGE_INTERP_TO_BLACK: {
                int col;
                for (col = 0; col < outpamP->width; col++)
                    nextline_stretched[col] = blackTuple;
            } 
            break;
            case EDGE_NON_INTERP: {
                /* EDGE_NON_INTERP */
                int col;
                for (col = 0; col < outpamP->width; col++)
                    nextline_stretched[col] = curline_stretched[col];
            }
            break;
            case EDGE_DROP: 
                pm_error("INTERNAL ERROR: processing last row, but "
                         "edge_mode is EDGE_DROP.");
            }
        } else {
            pnm_readpamrow(inpamP, nextline);
            stretch_line(inpamP, nextline, nextline_stretched, xscale,
                         edge_mode);
        }
        
        /* interpolate curline towards nextline into outbuf */
        write_interp_rows(outpamP, curline_stretched, nextline_stretched,
                          outbuf, yscale);

        swap_buffers(&curline, &nextline);
        swap_buffers(&curline_stretched, &nextline_stretched);
    }
    pnm_freerow(outbuf);
    pnm_freerow(stretched_linebuf2);
    pnm_freerow(stretched_linebuf1);
    pnm_freerow(linebuf2);
    pnm_freerow(linebuf1);
}



int 
main(int argc,char *argv[]) {

    FILE *ifp;

    struct cmdline_info cmdline; 
    struct pam inpam, outpam;
    
    pnm_init(&argc, argv);

    parse_command_line(argc, argv, &cmdline);

    ifp = pm_openr(cmdline.input_filespec);

    pnm_readpaminit(ifp, &inpam, PAM_STRUCT_SIZE(tuple_type));

    if (inpam.width < 2)
        pm_error("Image is too narrow.  Must be at least 2 columns.");
    if (inpam.height < 2)
        pm_error("Image is too short.  Must be at least 2 lines.");


    outpam = inpam;  /* initial value */
    outpam.file = stdout;

    if (PNM_FORMAT_TYPE(inpam.format) == PBM_TYPE) {
        outpam.format = PGM_TYPE;
        /* usual filter message when reading PBM but writing PGM: */
        pm_message("promoting from PBM to PGM");
    } else {
        outpam.format = inpam.format;
    }
    {
        unsigned int const dropped = cmdline.edge_mode == EDGE_DROP ? 1 : 0;

        outpam.width = (inpam.width - dropped) * cmdline.xscale;
        outpam.height = (inpam.height - dropped) * cmdline.yscale;

        pnm_writepaminit(&outpam);
    }

    pnm_createBlackTuple(&outpam, &blackTuple);

    stretch(&inpam, &outpam, 
            cmdline.xscale, cmdline.yscale, cmdline.edge_mode);

    pm_close(ifp);

    exit(0);
}



