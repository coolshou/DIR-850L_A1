/* pbmminkowsky.c - read a portable bitmap and calculate the Minkowski Integrals
**
** Copyright (C) 2000 by Luuk van Dijk/Mind over Matter
**
** Based on pbmlife.c,
** Copyright (C) 1988,1 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "pbm.h"

#define ISWHITE(x) ( (x) == PBM_WHITE )


int main( int argc, char** argv ){

  FILE* ifp;

  bit* prevrow;
  bit* thisrow;
  bit* tmprow;
  
  int row;
  int col; 

  int countTile=0;
  int countEdgeX=0;
  int countEdgeY=0;
  int countVertex=0;
  
  int rows;
  int cols;
  int format;

  int area, perimeter, eulerchi;


  /*
   * parse arg and initialize
   */ 

  pbm_init( &argc, argv );
  
  if ( argc > 2 )
    pm_usage( "[pbmfile]" );
  
  if ( argc == 2 )
    ifp = pm_openr( argv[1] );
  else
    ifp = stdin;
  
  pbm_readpbminit( ifp, &cols, &rows, &format );

  prevrow = pbm_allocrow( cols );
  thisrow = pbm_allocrow( cols );


  /* first row */

  pbm_readpbmrow( ifp, thisrow, cols, format );

  /* tiles */

  for ( col = 0; col < cols; ++col ) 
    if( ISWHITE(thisrow[col]) ) ++countTile;
  
  /* shortcut: for the first row, edgeY == countTile */
  countEdgeY = countTile;

  /* x-edges */

  if( ISWHITE(thisrow[0]) ) ++countEdgeX;

  for ( col = 0; col < cols-1; ++col ) 
    if( ISWHITE(thisrow[col]) || ISWHITE(thisrow[col+1]) ) ++countEdgeX;

  if( ISWHITE(thisrow[cols-1]) ) ++countEdgeX;

  /* shortcut: for the first row, countVertex == countEdgeX */
  
  countVertex = countEdgeX;
  

  for ( row = 1; row < rows; ++row ){  
    
    tmprow = prevrow; 
    prevrow = thisrow;
    thisrow = tmprow;
 
    pbm_readpbmrow( ifp, thisrow, cols, format );
  
    /* tiles */

    for ( col = 0; col < cols; ++col ) 
      if( ISWHITE(thisrow[col]) ) ++countTile;
    
    /* y-edges */

    for ( col = 0; col < cols; ++col ) 
      if( ISWHITE(thisrow[col]) || ISWHITE( prevrow[col] )) ++countEdgeY;
    
    /* x-edges */

    if( ISWHITE(thisrow[0]) ) ++countEdgeX;

    for ( col = 0; col < cols-1; ++col ) 
      if( ISWHITE(thisrow[col]) || ISWHITE(thisrow[col+1]) ) ++countEdgeX;
    
    if( ISWHITE(thisrow[cols-1]) ) ++countEdgeX;
    
    /* vertices */

    if( ISWHITE(thisrow[0]) || ISWHITE(prevrow[0]) ) ++countVertex;

    for ( col = 0; col < cols-1; ++col ) 
      if(    ISWHITE(thisrow[col]) || ISWHITE(thisrow[col+1]) 
	  || ISWHITE(prevrow[col]) || ISWHITE(prevrow[col+1]) ) ++countVertex;

    if( ISWHITE(thisrow[cols-1]) || ISWHITE(prevrow[cols-1]) ) ++countVertex;

	  
  } /* for row */

  /* now thisrow contains the top row*/
  /* tiles and x-edges have been counted, now y-edges and top vertices remain */

  
  /* y-edges */

  for ( col = 0; col < cols; ++col ) 
    if( ISWHITE(thisrow[col]) ) ++countEdgeY;

  /* vertices */
  
  if( ISWHITE(thisrow[0]) ) ++countVertex;

  for ( col = 0; col < cols-1; ++col ) 
    if( ISWHITE(thisrow[col]) || ISWHITE(thisrow[col+1]) ) ++countVertex;

  if( ISWHITE(thisrow[cols-1]) ) ++countVertex;


  /* cleanup */

  pm_close( ifp );

  /* print results */

  printf( "   tiles:\t%d\n x-edges:\t%d\n y-edges:\t%d\nvertices:\t%d\n",
	  countTile, countEdgeX, countEdgeY,countVertex );

  area      = countTile;
  perimeter = 2*countEdgeX + 2*countEdgeY - 4*countTile;
  eulerchi  = countTile - countEdgeX - countEdgeY + countVertex;

  printf( "    area:\t%d\nperimeter:\t%d\n eulerchi:\t%d\n",
	  area, perimeter, eulerchi );
  
  exit( 0 );

} /* main */

