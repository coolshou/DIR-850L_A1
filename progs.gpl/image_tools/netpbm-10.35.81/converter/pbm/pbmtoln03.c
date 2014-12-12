/*
From tim@deakin.edu.au Fri May  7 00:18:57 1993
From: Tim Cook <tim@deakin.edu.au>
Date: Fri, 7 May 1993 15:18:34 -0500
X-Mailer: Mail User's Shell (7.2.4 2/2/92)
To: dyson@sunfish.Physics.UIowa.Edu
Subject: Re: DEC LN03+ printer (not postscript) under SunOS (on SS10) ?
Content-Length: 5893

In a message dated 6 May,  9:32, dyson@sunfish.Physics.UIowa.Edu
(Richard L. Dyson) wrote:
> > Just in case anyone is interested, I have a pbmtoln03 utility I wrote
> > when I was mucking about with an LN03+.  If you are interested in
> > printing your bitmaps at 300x300dpi, ask me for the source.
>
> I would be interested.  We still only have LN03+ printers on our VMS
> machines here...

Ok, here goes.  Note that you will need the source from the pbmplus
utilities, because my pbmtoln03 utility uses the library routines that
are a part of pbmplus to read a PBM file (I linked it with libpbm.a).
I have not tested this utility on VMS, but it looks like it should
work.
*/

/* pbmtoln03.c -        Converts a PBM bitmap to DEC LN03 SIXEL bitmap
 *
 * SYNOPSIS
 *      pbmtoln03 [pbm-file]
 *
 * OPTIONS
 *      -l nn   Use "nn" as value for left margin (default 0).
 *      -r nn   Use "nn" as value for right margin (default 2400).
 *      -t nn   Use "nn" as value for top margin (default 0).
 *      -b nn   Use "nn" as value for bottom margin (default 3400).
 *      -f nn   Use "nn" as value for form length (default 3400).
 *
 * Tim Cook, 26 Feb 1992
 * changed option parsing to PBM standards  - Ingo Wilken, 13 Oct 1993
 */

#include <stdio.h>
#include "pbm.h"

FILE *input ;

#ifndef print
#define print(s)        fputs (s, stdout)
#define fprint(f, s)    fputs (s, f)
#endif


static void 
output_sixel_record (unsigned char * record, int width) {

   int i, j, k ;
   unsigned char last_char ;
   int start_repeat = 0 ;
   int repeated ;
   char repeated_str[16] ;
   char *p ;

   /* Do RLE */
   last_char = record[0] ;
   j = 0 ;

   /* This will make the following loop complete */
   record[width] = '\0' ;

   for (i = 1 ; i <= width ; i++) {

      repeated = i - start_repeat ;

      if (record[i] != last_char || repeated >= 32766) {

         /* Repeat has ended */

         if (repeated > 3) {

            /* Do an encoding */
            record[j++] = '!' ;
            sprintf (repeated_str, "%d", i - start_repeat) ;
            for (p = repeated_str ; *p ; p++)
               record[j++] = *p ;
               record[j++] = last_char ; }

         else {
            for (k = 0 ; k < repeated ; k++)
               record[j++] = last_char ; }

         start_repeat = i ;
         last_char = record[i] ; }
      }

   fwrite ((char *) record, j, 1, stdout) ;
   putchar ('-') ;      /* DECGNL (graphics new-line) */
   putchar ('\n') ;
   }


static void 
convert (int width, int height, int format) {

   int i ;
   unsigned char *sixel ;               /* A row of sixels */
   int sixel_row ;

   bit *row = pbm_allocrow (width) ;

   sixel = (unsigned char *) malloc (width + 2) ;

   sixel_row = 0 ;
   while (height--) {
      pbm_readpbmrow (input, row, width, format) ;
      switch (sixel_row) {
         case 0 :
           for (i = 0 ; i < width ; i++)
              sixel[i] = row[i] ;
           break ;
         case 1 :
           for (i = 0 ; i < width ; i++)
              sixel[i] += row[i] << 1 ;
           break ;
         case 2 :
           for (i = 0 ; i < width ; i++)
              sixel[i] += row[i] << 2 ;
           break ;
         case 3 :
           for (i = 0 ; i < width ; i++)
              sixel[i] += row[i] << 3 ;
           break ;
         case 4 :
           for (i = 0 ; i < width ; i++)
              sixel[i] += row[i] << 4 ;
           break ;
         case 5 :
           for (i = 0 ; i < width ; i++)
              sixel[i] += (row[i] << 5) + 077 ;
           output_sixel_record (sixel, width) ;
           break ; }
      if (sixel_row == 5)
         sixel_row = 0 ;
      else
         sixel_row++ ;
      }

   if (sixel_row > 0) {
      /* Incomplete sixel record needs to be output */
      for (i = 0 ; i < width ; i++)
         sixel[i] += 077 ;
      output_sixel_record (sixel, width) ; }
   }



int
main (int argc, char **argv) {
   int argc_copy = argc ;
   char **argv_copy = argv ;
   int argn;
   const char * const usage = "[-left <nn>] [-right <nn>] [-top <nn>] "
       "[-bottom <nn>] [-formlength <nn>] [pbmfile]";

   /* Options */
   /* These defaults are for a DEC LN03 with A4 paper (2400x3400 pixels) */
   const char *opt_left_margin = "0";
   const char *opt_top_margin = opt_left_margin;
   const char *opt_right_margin = "2400";
   const char *opt_bottom_margin = "3400";
   const char *opt_form_length = opt_bottom_margin;

   int width, height, format ;

   pbm_init (&argc_copy, argv_copy) ;

   argn = 1;
   while( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' ) {
      if( pm_keymatch(argv[argn], "-left", 2) ) {
         if( ++argn >= argc )
            pm_usage(usage);
         opt_left_margin = argv[argn];
      }
      else
      if( pm_keymatch(argv[argn], "-right", 2) ) {
         if( ++argn >= argc )
            pm_usage(usage);
         opt_right_margin = argv[argn];
      }
      else
      if( pm_keymatch(argv[argn], "-top", 2) ) {
         if( ++argn >= argc )
            pm_usage(usage);
         opt_top_margin = argv[argn];
      }
      else
      if( pm_keymatch(argv[argn], "-bottom", 2) ) {
         if( ++argn >= argc )
            pm_usage(usage);
         opt_bottom_margin = argv[argn];
      }
      else
      if( pm_keymatch(argv[argn], "-formlength", 2) ) {
         if( ++argn >= argc )
            pm_usage(usage);
         opt_form_length = argv[argn];
      }
      else
         pm_usage(usage);
      ++argn;
   }

   if( argn < argc ) {
      input = pm_openr( argv[argn] );
      argn++;
   }
   else
      input = stdin;

   if( argn != argc )
      pm_usage(usage);


   /* Initialize pbm file */
   pbm_readpbminit (input, &width, &height, &format) ;

   if (format != PBM_FORMAT && format != RPBM_FORMAT)
      pm_error ("input not in PBM format") ;

/*
 * In explanation of the sequence below:
 *      <ESC>[!p        DECSTR  soft terminal reset
 *      <ESC>[11h       PUM     select unit of measurement
 *      <ESC>[7 I       SSU     select pixel as size unit
 *      <ESC>[?52l      DECOPM  origin is corner of printable area
 *      <ESC>[%s;%ss    DECSLRM left and right margins
 *      <ESC>[%s;%sr    DECSTBM top and bottom margins
 *      <ESC>[%st       DECSLPP form length
 *      <ESC>P0;0;1q            select sixel graphics mode
 *      "1;1            DECGRA  aspect ratio (1:1)
 */

   /* Initialize sixel file */
   printf ("\033[!p\033[11h\033[7 I\033[?52l\033[%s;%ss\033"
           "[%s;%sr\033[%st\033P0;0;1q\"1;1",
      opt_left_margin, opt_right_margin, opt_top_margin, opt_bottom_margin,
      opt_form_length);

   /* Convert data */
   convert (width, height, format) ;

   /* Terminate sixel data */
   print ("\033\\\n") ;

   /* If the program failed, it previously aborted with nonzero completion
      code, via various function calls.
   */
   return 0;
}


