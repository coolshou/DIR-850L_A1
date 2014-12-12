/* pgmmorphconv.c - morphological convolutions on a graymap: dilation and 
** erosion
**
** Copyright (C) 2000 by Luuk van Dijk/Mind over Matter
**
** Based on 
** pnmconvol.c - general MxN convolution on a portable anymap
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "pm_c_util.h"
#include "pgm.h"


/************************************************************
 * Dilate 
 ************************************************************/

static int 
dilate( bit** template, int trowso2, int tcolso2, 
        gray** in_image, gray** out_image, 
        int rows, int cols ){

  int c, r, tc, tr;
  int templatecount;
  gray source;

  for( c=0; c<cols; ++c)
    for( r=0; r<rows; ++r )
      out_image[r][c] = 0;   /* only difference with erode is here and below */
  
  /* 
   *  for each non-black pixel of the template
   *  add in to out
   */

  templatecount=0;

  for( tr=-trowso2; tr<=trowso2; ++tr ){
    for( tc=-tcolso2; tc<=tcolso2; ++tc ){

      if( template[trowso2+tr][tcolso2+tc] == PBM_BLACK ) continue;

      ++templatecount;

      for( r= ((tr>0)?0:-tr) ; r< ((tr>0)?(rows-tr):rows) ; ++r ){
        for( c= ((tc>0)?0:-tc) ; c< ((tc>0)?(cols-tc):cols) ; ++c ){
          source = in_image[r+tr][c+tc];
          out_image[r][c] = MAX(source, out_image[r][c]);
        } /* for c */
      } /* for r */
    } /* for tr */
  } /* for tc */

  return templatecount;

} /* dilate */



/************************************************************
 * Erode: same as dilate except !!!!
 ************************************************************/

static int 
erode( bit** template, int trowso2, int tcolso2, 
       gray** in_image, gray** out_image, 
       int rows, int cols ){

  int c, r, tc, tr;
  int templatecount;
  gray source;

  for( c=0; c<cols; ++c)
    for( r=0; r<rows; ++r )
      out_image[r][c] = PGM_MAXMAXVAL; /* !!!! */
  
  /* 
   *  for each non-black pixel of the template
   *  add in to out
   */

  templatecount=0;

  for( tr=-trowso2; tr<=trowso2; ++tr ){
    for( tc=-tcolso2; tc<=tcolso2; ++tc ){

      if( template[trowso2+tr][tcolso2+tc] == PBM_BLACK ) continue;

      ++templatecount;

      for( r= ((tr>0)?0:-tr) ; r< ((tr>0)?(rows-tr):rows) ; ++r ){
    for( c= ((tc>0)?0:-tc) ; c< ((tc>0)?(cols-tc):cols) ; ++c ){

      source = in_image[r+tr][c+tc];
      out_image[r][c] = MIN(source, out_image[r][c]);
      
    } /* for c */
      } /* for r */



    } /* for tr */
  } /* for tc */

  return templatecount;

} /* erode */



/************************************************************
 *  Main
 ************************************************************/


int main( int argc, char* argv[] ){

  int argn;
  char operation;
  const char* usage = "-dilate|-erode|-open|-close <templatefile> [pgmfile]";

  FILE* tifp;   /* template */
  int tcols, trows;
  int tcolso2, trowso2;
  bit** template;


  FILE*  ifp;   /* input image */
  int cols, rows;
  gray maxval;

  gray** in_image;
  gray** out_image;

  int templatecount=0;

  pgm_init( &argc, argv );

  /*
   *  parse arguments
   */ 
  
  ifp = stdin;
  operation = 'd';

  argn=1;
  
  if( argn == argc ) pm_usage( usage );
  
  if( pm_keymatch( argv[argn], "-erode", 2  )) { operation='e'; argn++; }
  else
  if( pm_keymatch( argv[argn], "-dilate", 2 )) { operation='d'; argn++; }
  else
  if( pm_keymatch( argv[argn], "-open", 2   )) { operation='o'; argn++; }
  else
  if( pm_keymatch( argv[argn], "-close", 2  )) { operation='c'; argn++; }
  
  if( argn == argc ) pm_usage( usage );
  
  tifp = pm_openr( argv[argn++] );
  
  if( argn != argc ) ifp = pm_openr( argv[argn++] );

  if( argn != argc ) pm_usage( usage );

  
  /* 
   * Read in the template matrix.
   */

  template = pbm_readpbm( tifp, &tcols, &trows );
  pm_close( tifp );

  if( tcols % 2 != 1 || trows % 2 != 1 )
    pm_error("the template matrix must have an odd number of "
             "rows and columns" );

  /* the reason is that we want the middle pixel to be the origin */
  tcolso2 = tcols / 2; /* template coords run from -tcols/2 .. 0 .. +tcols/2 */
  trowso2 = trows / 2;

#if 0
  fprintf(stderr, "template: %d  x %d\n", trows, tcols);
  fprintf(stderr, "half: %d  x %d\n", trowso2, tcolso2);
#endif

  /*
   * Read in the image
   */
  
  in_image = pgm_readpgm( ifp, &cols, &rows, &maxval);

  if( cols < tcols || rows < trows )
    pm_error("the image is smaller than the convolution matrix" );
  
#if 0
  fprintf(stderr, "image: %d  x %d (%d)\n", rows, cols, maxval);
#endif

  /* 
   * Allocate  output buffer and initialize with min or max value 
   */

  out_image = pgm_allocarray( cols, rows );
  
  if( operation == 'd' ){
    templatecount = dilate(template, trowso2, tcolso2, 
               in_image, out_image, rows, cols);
  } 
  else if( operation == 'e' ){
    templatecount = erode(template, trowso2, tcolso2, 
              in_image, out_image, rows, cols);
  }
  else if( operation == 'o' ){
    gray ** eroded_image;
    eroded_image = pgm_allocarray( cols, rows );
    templatecount = erode(template, trowso2, tcolso2, 
                          in_image, eroded_image, rows, cols);
    templatecount = dilate(template, trowso2, tcolso2, 
                           eroded_image, out_image, rows, cols);
    pgm_freearray( eroded_image, rows );
  }
  else if( operation == 'c' ){
    gray ** dilated_image;
    dilated_image = pgm_allocarray( cols, rows );
    templatecount = dilate(template, trowso2, tcolso2, 
                           in_image, dilated_image, rows, cols);
    templatecount = erode(template, trowso2, tcolso2, 
                          dilated_image, out_image, rows, cols);
    pgm_freearray( dilated_image, rows );
  }
  
  if(templatecount == 0 ) pm_error( "The template was empty!" );

  pgm_writepgm( stdout, out_image, cols, rows, maxval, 1 );

  pgm_freearray( out_image, rows );
  pgm_freearray( in_image, rows );
  pm_close( ifp );

  exit( 0 );

} /* main */

