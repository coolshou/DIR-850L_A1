/* pgmminkowsky.c - read a portable graymap and calculate the Minkowski 
** Integrals as a function of the threshold.
**
** Copyright (C) 2000 by Luuk van Dijk/Mind over Matter
**
** Based on pgmhist.c, 
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "pgm.h"
#include "mallocvar.h"


#define MAX2(a,b) ( ( (a)>(b) ) ? (a) : (b) )
#define MAX4(a,b,c,d) MAX2( MAX2((a),(b)), MAX2((c),(d)) )

int main( int argc, char** argv ){
  
  FILE *ifp;

  gray maxval;
  int cols, rows, format;

  gray* prevrow;
  gray* thisrow;
  gray* tmprow;
  
  int* countTile;   
  int* countEdgeX;  
  int* countEdgeY; 
  int* countVertex; 

  int i, col, row;

  int maxtiles, maxedgex, maxedgey, maxvertex;
  int area, perimeter, eulerchi;

  double l2inv, linv;

  /*
   * parse arg and initialize
   */ 

  pgm_init( &argc, argv );

  if ( argc > 2 ) pm_usage( "[pgmfile]" );
  
  if ( argc == 2 )
    ifp = pm_openr( argv[1] );
  else
    ifp = stdin;

  /*
   * initialize
   */

  pgm_readpgminit( ifp, &cols, &rows, &maxval, &format );
  
  prevrow = pgm_allocrow( cols );
  thisrow = pgm_allocrow( cols );
  
  MALLOCARRAY(countTile   , maxval + 1 );
  MALLOCARRAY(countEdgeX  , maxval + 1 );
  MALLOCARRAY(countEdgeY  , maxval + 1 );
  MALLOCARRAY(countVertex , maxval + 1 );
 
  if (countTile == NULL || countEdgeX == NULL || countEdgeY == NULL ||
      countVertex == NULL)
      pm_error( "out of memory" );
  
  for ( i = 0; i <= maxval; i++ ) countTile[i]   = 0;
  for ( i = 0; i <= maxval; i++ ) countEdgeX[i]  = 0;
  for ( i = 0; i <= maxval; i++ ) countEdgeY[i]  = 0;
  for ( i = 0; i <= maxval; i++ ) countVertex[i] = 0;




  /* first row */

  pgm_readpgmrow( ifp, thisrow, cols, maxval, format );

  /* tiles */

  for ( col = 0; col < cols; ++col ) ++countTile[thisrow[col]]; 
  
  /* y-edges */

  for ( col = 0; col < cols; ++col ) ++countEdgeY[thisrow[col]]; 

  /* x-edges */

  ++countEdgeX[thisrow[0]];

  for ( col = 0; col < cols-1; ++col ) 
    ++countEdgeX[ MAX2(thisrow[col], thisrow[col+1]) ];
  
  ++countEdgeX[thisrow[cols-1]];
  
  /* shortcut: for the first row, countVertex == countEdgeX */
  
  ++countVertex[thisrow[0]];

  for ( col = 0; col < cols-1; ++col ) 
    ++countVertex[ MAX2(thisrow[col], thisrow[col+1]) ];

  ++countVertex[thisrow[cols-1]];

  

  for ( row = 1; row < rows; ++row ){  
    
    tmprow = prevrow; 
    prevrow = thisrow;
    thisrow = tmprow;
 
    pgm_readpgmrow( ifp, thisrow, cols, maxval, format );
  
    /* tiles */

    for ( col = 0; col < cols; ++col ) ++countTile[thisrow[col]]; 
    
    /* y-edges */
    
    for ( col = 0; col < cols; ++col ) 
      ++countEdgeY[ MAX2(thisrow[col], prevrow[col]) ];
    /* x-edges */
    
    ++countEdgeX[thisrow[0]];
    
    for ( col = 0; col < cols-1; ++col ) 
      ++countEdgeX[ MAX2(thisrow[col], thisrow[col+1]) ];
    
    ++countEdgeX[thisrow[cols-1]];
    
    /* vertices */

    ++countVertex[ MAX2(thisrow[0],prevrow[0]) ];

    for ( col = 0; col < cols-1; ++col ) 
      ++countVertex[
        MAX4(thisrow[col], thisrow[col+1], prevrow[col], prevrow[col+1])
      ];
    
    ++countVertex[ MAX2(thisrow[cols-1],prevrow[cols-1]) ];
    
  } /* for row */
  
  /* now thisrow contains the top row*/

  /* tiles and x-edges have been counted, now upper
     y-edges and top vertices remain */
  
  /* y-edges */

  for ( col = 0; col < cols; ++col ) ++countEdgeY[ thisrow[col] ];

  /* vertices */
  
  ++countVertex[thisrow[0]];

  for ( col = 0; col < cols-1; ++col ) 
    ++countVertex[ MAX2(thisrow[col],thisrow[col+1]) ];

  ++countVertex[ thisrow[cols-1] ];


  /* cleanup */

  maxtiles =  rows    * cols;
  maxedgex =  rows    * (cols+1);
  maxedgey = (rows+1) *  cols;
  maxvertex= (rows+1) * (cols+1);
  
  l2inv = 1.0/maxtiles;
  linv  = 0.5/(rows+cols);

  /* And print it. */
  printf( "#threshold\t tiles\tx-edges\ty-edges\tvertices\n" );
  printf( "#---------\t -----\t-------\t-------\t--------\n" );
  for ( i = 0; i <= maxval; i++ ){

    if( !(countTile[i] || countEdgeX[i] || countEdgeY[i] || countVertex[i] ) ) 
      continue; /* skip empty slots */

    area      = maxtiles;
    perimeter = 2*maxedgex + 2*maxedgey - 4*maxtiles;
    eulerchi  = maxtiles - maxedgex - maxedgey + maxvertex;

    printf( "%f\t%6d\t%7d\t%7d\t%8d\t%g\t%g\t%6d\n", (float) i/(1.0*maxval), 
        maxtiles, maxedgex, maxedgey, maxvertex,
        area*l2inv, perimeter*linv, eulerchi
        );


    maxtiles -= countTile[i];
    maxedgex -= countEdgeX[i];
    maxedgey -= countEdgeY[i];
    maxvertex-= countVertex[i];

    /*  i, countTile[i], countEdgeX[i], countEdgeY[i], countVertex[i] */

  }

  /* these should be zero: */
  printf( "#  check:\t%6d\t%7d\t%7d\t%8d\n", 
          maxtiles, maxedgex, maxedgey, maxvertex );

  pm_close( ifp );
  
  exit( 0 );
  
} /*main*/


