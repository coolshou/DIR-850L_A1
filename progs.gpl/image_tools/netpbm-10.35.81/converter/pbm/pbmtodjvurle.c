/*
   Convert a PBM image into the DjVu Bitonal RLE format
   described in the csepdjvu(1) documentation
  
   Copyright (c) 2004 Scott Pakin <scott+pbm@pakin.org>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. The name of the author may not be used to endorse or promote products
      derived from this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "pbm.h"


/* Write a byte to a file and check for errors. */
static void
writebyte(FILE *        const ofP,
          unsigned char const c) {

    if (fputc (c, ofP) == EOF)
        pm_error ("failed to write to the RLE file.  Errno=%d (%s)",
                  errno, strerror(errno));
}


/* Write a run length to the RLE file. */
static void 
write_rle (FILE *rlefile, uint32n tally)
{
  do {
    /* Output a single run. */
    if (tally < 192) {
      /* Single-byte runs */
      writebyte (rlefile, tally);
      tally >>= 8;
    }
    else {
      /* Two-byte runs */
      writebyte (rlefile, ((tally>>8)&0x3F) + 0xC0);
      writebyte (rlefile, tally&0xFF);
      tally >>= 14;
    }

    /* Very large runs need to be split into smaller runs.  We
     * therefore need to toggle back to the same color we had for the
     * previous smaller run. */
    if (tally > 0)
      writebyte (rlefile, 0);
  }
  while (tally > 0);
}



int 
main (int argc, char *argv[])
{
  FILE * const rlefile = stdout;    /* Generated Bitonal RLE file */

  FILE *pbmfile;             /* PBM file to convert */
  int numcols, numrows;      /* Width and height in pixels of the PBM file */
  int format;                /* Original image type before conversion to PBM */
  bit *pbmrow;               /* One row of the PBM file */
  uint32n pixeltally = 0;    /* Run length of the current color */
  int row, col;              /* Row and column loop variables */
  const char * pbmfilename;  /* Name of input file */

  /* Parse the command line. */
  pbm_init (&argc, argv);

  if (argc-1 < 1)
      pbmfilename = "-";
  else if (argc-1 == 1)
      pbmfilename = argv[1];
  else
      pm_error("Program takes at most 1 argument -- the input file name.  "
               "You specified %d", argc-1);

  pbmfile = pm_openr(pbmfilename);

  /* Write an RLE header. */
  pbm_readpbminit (pbmfile, &numcols, &numrows, &format);
  fprintf (rlefile, "R4\n");
  fprintf (rlefile, "%d %d\n", numcols, numrows);

  /* Write the RLE data. */
  pbmrow = pbm_allocrow (numcols);
  for (row=0; row<numrows; row++) {
    bit prevpixel;        /* Previous pixel seen */

    pbm_readpbmrow (pbmfile, pbmrow, numcols, format);
    prevpixel = PBM_WHITE;   /* Bitonal RLE rows always start with white */
    pixeltally = 0;
    for (col=0; col<numcols; col++) {
      bit newpixel = pbmrow[col];      /* Current pixel color */

      if (newpixel == prevpixel)
        pixeltally++;
      else {
        write_rle (rlefile, pixeltally);
        pixeltally = 1;
        prevpixel = newpixel;
      }
    }
    write_rle (rlefile, pixeltally);
  }

  /* Finish up cleanly. */
  pbm_freerow (pbmrow);
  if (rlefile != stdout)
    pm_close (rlefile);
  if (pbmfile != stdin)
    pm_close (pbmfile);
  exit (0);
}
