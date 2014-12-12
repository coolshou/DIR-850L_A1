/*

  Bayer matrix conversion tool

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 2 of the License,
  or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
  USA

  Copyright Alexandre Becoulet <diaxen AT free DOT fr>
  
  Completely rewritten for Netpbm by Bryan Henderson August 2005.
*/

#include <unistd.h>
#include <stdio.h>

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"


enum bayerType {
    BAYER1,
    BAYER2,
    BAYER3,
    BAYER4
};

struct cmdlineInfo {
    const char * inputFilespec;
    enum bayerType bayerType;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    unsigned int typeSpec;
    unsigned int type;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "type",     OPT_UINT, &type,
            &typeSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 > 1)
        pm_error("There is at most one argument -- the input file.  "
                 "You specified %u", argc-1);
    else
        cmdlineP->inputFilespec = argv[1];

    if (!typeSpec)
        pm_error("You must specify the -type option");
    else {
        switch (type) {
        case 1: cmdlineP->bayerType = BAYER1; break;
        case 2: cmdlineP->bayerType = BAYER2; break;
        case 3: cmdlineP->bayerType = BAYER3; break;
        case 4: cmdlineP->bayerType = BAYER4; break;
        }
    }
}



static void
calc_4(const struct pam * const pamP,
       tuple **           const intuples,
       tuple **           const outtuples,
       unsigned int       const plane,
       unsigned int       const xoffset,
       unsigned int       const yoffset) {
/*----------------------------------------------------------------------------
    X . X
    . . .
    X . X
-----------------------------------------------------------------------------*/
    unsigned int y;
    
    for (y = yoffset; y < pamP->height; y += 2) {
        unsigned int x;
        for (x = xoffset; x + 2 < pamP->width; x += 2) {
            outtuples[y][x][plane] = intuples[y][x][0];
            outtuples[y][x + 1][plane] =
                (intuples[y][x][0] + intuples[y][x + 2][0]) / 2;
        }
    }
    for (y = yoffset; y + 2 < pamP->height; y += 2) {
        unsigned int x;
        for (x = xoffset; x < pamP->width; ++x)
            outtuples[y + 1][x][plane] =
                (outtuples[y][x][plane] + outtuples[y + 2][x][plane]) / 2;
    }
}



static void
calc_5(const struct pam * const pamP,
       tuple **           const intuples,
       tuple **           const outtuples,
       unsigned int       const plane,
       unsigned int       const xoffset,
       unsigned int       const yoffset) {
/*----------------------------------------------------------------------------
   . X .
   X . X
   . X .
-----------------------------------------------------------------------------*/
    unsigned int y;
    unsigned int j;

    j = 0;  /* initial value */

    for (y = yoffset; y + 2 < pamP->height; ++y) {
        unsigned int x;
        for (x = xoffset + j; x + 2 < pamP->width; x += 2) {
            outtuples[y][x + 1][plane] = intuples[y][x + 1][0];
            outtuples[y + 1][x + 1][plane] = 
                (intuples[y][x + 1][0] + intuples[y + 1][x][0] +
                 intuples[y + 2][x + 1][0] + intuples[y + 1][x + 2][0]) / 4;
        }
        j = 1 - j;
    }
}



struct compAction {
    unsigned int xoffset;
    unsigned int yoffset;
    void (*calc)(const struct pam * const pamP,
                 tuple **           const intuples,
                 tuple **           const outtuples,
                 unsigned int       const plane,
                 unsigned int       const xoffset,
                 unsigned int       const yoffset);
};



static struct compAction const comp_1[3] = {
/*----------------------------------------------------------------------------
  G B G B
  R G R G
  G B G B
  R G R G
-----------------------------------------------------------------------------*/

    { 0, 1, calc_4 },
    { 0, 1, calc_5 },
    { 1, 0, calc_4 }
};

static struct compAction const comp_2[3] = {
/*----------------------------------------------------------------------------
  R G R G
  G B G B
  R G R G
  G B G B
-----------------------------------------------------------------------------*/
    { 0, 0, calc_4 },
    { 0, 0, calc_5 },
    { 1, 1, calc_4 }
};

static struct compAction const comp_3[3] = {
/*----------------------------------------------------------------------------
  B G B G
  G R G R
  B G B G
  G R G R
-----------------------------------------------------------------------------*/
    { 1, 1, calc_4 },
    { 0, 0, calc_5 },
    { 0, 0, calc_4 }
};

static struct compAction const comp_4[3] = {
/*----------------------------------------------------------------------------
  G R G R
  B G B G
  G R G R
  B G B G
-----------------------------------------------------------------------------*/
    { 1, 0, calc_4 },
    { 0, 1, calc_5 },
    { 0, 1, calc_4 }
};



static void
makeOutputPam(const struct pam * const inpamP,
              struct pam *       const outpamP) {

    outpamP->size   = sizeof(*outpamP);
    outpamP->len    = PAM_STRUCT_SIZE(tuple_type);
    outpamP->file   = stdout;
    outpamP->format = PAM_FORMAT;
    outpamP->plainformat = 0;
    outpamP->width  = inpamP->width;
    outpamP->height = inpamP->height;
    outpamP->depth  = 3;
    outpamP->maxval = inpamP->maxval;
    outpamP->bytes_per_sample = inpamP->bytes_per_sample;
    STRSCPY(outpamP->tuple_type, "RGB");
}



static const struct compAction *
actionTableForType(enum bayerType const bayerType) {

    const struct compAction * retval;

    switch (bayerType) {
    case BAYER1: retval = comp_1; break;
    case BAYER2: retval = comp_2; break;
    case BAYER3: retval = comp_3; break;
    case BAYER4: retval = comp_4; break;
    }
    return retval;
}



int
main(int argc, char **argv) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    struct pam inpam;
    struct pam outpam;
    tuple ** intuples;
    tuple ** outtuples;
    const struct compAction * compActionTable;
    unsigned int plane;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
    
    ifP = pm_openr(cmdline.inputFilespec);
    
    intuples = pnm_readpam(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));

    compActionTable = actionTableForType(cmdline.bayerType);

    makeOutputPam(&inpam, &outpam);

    outtuples = pnm_allocpamarray(&outpam);

    for (plane = 0; plane < 3; ++plane) {
        struct compAction const compAction = compActionTable[plane];

        compAction.calc(&inpam, intuples, outtuples, plane,
                        compAction.xoffset, compAction.yoffset);
    }
    pnm_writepam(&outpam, outtuples);

    pnm_freepamarray(outtuples, &outpam);
    pnm_freepamarray(intuples, &inpam);

    return 0;
}
