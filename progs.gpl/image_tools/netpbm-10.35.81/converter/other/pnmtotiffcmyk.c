/*

CMYKTiff - pnmtotiffcmyk - conversion from a pnm file to a CMYK
encoded tiff file.

Copyright (C) 1999 Andrew Cooke  (Jara Software)

Comments to jara@andrewcooke.free-online.co.uk

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA

These conditions also apply to any other files distributed with the
program that describe / document the CMYKTiff package.


Much of the code uses ideas from other pnm programs, written by Jef
Poskanzer (thanks go to him and libtiff maintainer Sam Leffler).  A
small section of the code - some of the tiff tag settings - is derived
directly from pnmtotiff, by Jef Poskanzer, which, in turn,
acknowledges Patrick Naughton with the following text:

** Derived by Jef Poskanzer from ras2tif.c, which is:
**
** Copyright (c) 1990 by Sun Microsystems, Inc.
**
** Author: Patrick J. Naughton
** naughton@wind.sun.com
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted,
** provided that the above copyright notice appear in all copies and that
** both that copyright notice and this permission notice appear in
** supporting documentation.
**
** This file is provided AS IS with no warranties of any kind.  The author
** shall have no liability with respect to the infringement of copyrights,
** trade secrets or any patents by this file or any part thereof.  In no
** event will the author be liable for any lost revenue or profits or
** other special, indirect and consequential damages.

Software copyrights will soon need family trees... :-)

*/

#define _XOPEN_SOURCE

#include <string.h>
#include <math.h>
/* float.h used to be included only if __osf__ was defined.  On some
   platforms, float.h is also included automatically by math.h.  But at
   least on Solaris, it isn't.  00.04.25.
*/
#include <float.h>
#include <limits.h>
#include <fcntl.h>
/* See warning about tiffio.h in pamtotiff.c */
#include <tiffio.h>

#include "pm_c_util.h"
#include "nstring.h"
#include "pnm.h"

/* first release after testing */
/* second release after testing - small mods to netpbm package */
#define VERSION 1.01

/* only support 8 bit values */
#define MAXTIFFBITS 8
#define MAXTIFFVAL 255 


/* definitions for error values */

typedef int Err ;

#define ERR_OK       0  /* no error */
#define ERR_PNM      1  /* thrown by pnm library */
#define ERR_MEMORY   2  /* could not allocate memory */
#define ERR_ARG      3  /* unexpected argument */
#define ERR_TIFF     4  /* failure in tiff library */
#define ERR_HELP     5  /* terminate with -help */

/* definitions for command line options */

#define UNSET 2 /* Simple->remove value when gammap not set */
#define K_NORMAL 0
#define K_REMOVE 1
#define K_ONLY 2


/* definitions for conversion calculations */

/* catch rounding errors to outside 0-1 range */
#define LIMIT01(x) MAX( 0.0, MIN( 1.0, (x) ) )
/* convert from 0-1 to 0-maxOut */
#define TOMAX(x) MAX( 0, MIN( rt->maxOut, (int)(rt->maxOut * (x)) ) )
#define TINY FLT_EPSILON
#ifndef PI
#define PI 3.1415926
#endif
#define ONETWENTY ( 2.0 * PI / 3.0 ) 
#define TWOFORTY ( 4.0 * PI / 3.0 )
#define THREESIXTY ( 2.0 * PI ) 
/* expand from 0-90 to 0-120 degrees */
#define EXPAND(x) ( 4.0 * ( x ) / 3.0 )
/* contract from 0-120 to 0-90 degrees */
#define CONTRACT(x) ( 3.0 * ( x ) / 4.0 )



/* --- main structure for the process ----------------- */

struct tagIn ;
struct tagConv ;
struct tagOut ;

typedef struct {
  struct tagIn *in ;
  struct tagConv *conv ;
  struct tagOut *out ;
  int nCols ;
  int nRows ;
  int maxOut ;
  const char *name ;
} Root ;



/* --- utils ------------------------------------------ */


/* parse an arg with a float value.  name should be the full key name,
   preceded by '-' */
static Err 
floatArg( float *arg, const char *name, int size, float lo, float hi,
          int *argn, int argc, char **argv ) {

  char extra ;
  int count ;

  if ( pm_keymatch( argv[*argn], name, size ) ) {
    if ( ++(*argn) == argc ) {
      fprintf( stderr, "no value for %s\n", name ) ;
      return ERR_ARG ;
    }
    if ( ! (count = sscanf( argv[*argn], "%f%1c", arg, &extra )) ) {
      fprintf( stderr, "cannot parse %s for %s\n", argv[*argn], name ) ;
      return ERR_ARG ;
    }
    if ( count > 1 ) {
      fprintf( stderr, "warning: ignored %c... in value for %s\n",
               extra, name ) ;
    }
    if ( *arg > hi || *arg < lo ) {
      fprintf( stderr, "%s (%f) must be in range %f to %f\n", 
               name, *arg, lo, hi ) ;
      return ERR_ARG ;
    }
    ++(*argn) ;
  }

  return ERR_OK ;
}


/* parse an arg with a long value.  name should be the full key name,
   preceded by '-' */
static Err 
longArg( long *arg, const char *name, int size, long lo, long hi,
         int *argn, int argc, char **argv ) {

  char extra ;
  int count ;

  if ( pm_keymatch( argv[*argn], name, size ) ) {
    if ( ++(*argn) == argc ) {
      fprintf( stderr, "no value for %s\n", name ) ;
      return ERR_ARG ;
    }
    if ( ! (count = sscanf( argv[*argn], "%ld%1c", arg, &extra )) ) {
      fprintf( stderr, "cannot parse %s for %s\n", argv[*argn], name ) ;
      return ERR_ARG ;
    }
    if ( count > 1 ) {
      fprintf( stderr, "warning: ignored %c... in value for %s\n",
               extra, name ) ;
    }
    if ( *arg > hi || *arg < lo ) {
      fprintf( stderr, "%s (%ld) must be in range %ld to %ld\n", 
               name, *arg, lo, hi ) ;
      return ERR_ARG ;
    }
    ++(*argn) ;
  }

  return ERR_OK ;
}


/* print usage.  for simplicity this routine is *not* split amongst
   the various components - when you add a component (eg a new
   conversion algorithm, or maybe new input or output code), you must
   also change this routine.  by keeping all the options in one place
   it is also easier to calculate the minimum key name length (passed
   to pnm_keymatch) */
static void 
printUsage( ) {
  fprintf( stderr, "\nusage: pnmtocmyk [Compargs] [Tiffargs] [Convargs] [pnmfile]\n" ) ;
  fprintf( stderr, " Compargs: [-none|-packbits|-lzw [-predictor 1|-predictor 2]]\n" ) ;
  fprintf( stderr, " Tiffargs: [-msb2lsb|-lsb2msb] [-rowsperstrip n]\n" ) ;
  fprintf( stderr, "           [-lowdotrange lo] [-highdotrange hi] [-knormal|-konly|-kremove]\n" ) ;
  fprintf( stderr, " Convargs: [[-default] [Defargs]|-negative]\n" ) ;
  fprintf( stderr, " Defargs:  [-theta deg] [-gamma g] [-gammap -1|-gammap g]\n" ) ;
  fprintf( stderr, "where 0 <= lo < hi <= 255; -360 < deg < 360; 0.1 < g < 10; 0 < n < INT_MAX\n\n" ) ;
  fprintf( stderr, "returns: 0 OK; 1 pnm library error ; 2 memory error ; 3 unexpected arg ;\n" ) ;
  fprintf( stderr, "         4 tiff library error ; 5 -help key used\n\n" ) ;
  fprintf( stderr, "Convert a pnm file to a CMYK tiff file.  Version %5.2f\n", VERSION ) ;
  fprintf( stderr, "CMY under K will be removed unless -gammap -1 is used.\n" ) ;
  fprintf( stderr, "(c) 1999 Andrew Cooke - Beta version, not for public use.\n" ) ;
  fprintf( stderr, "No warranty.\n\n" ) ;
}

/* list of key args and number of significant letters required
-default      2
-gamma        6
-gammap       7
-highdotrange 2
-knormal      3
-konly        3
-kremove      3
-lowdotrange  3
-lsb2msb      3
-lzw          3
-msb2lsb      2
-negative     3
-none         3
-packbits     3
-predictor    3
-rowsperstrip 2
-theta        2
*/



/* --- reading the input ------------------------------ */


/* encapsulate the file reader - uses pnm library at the moment, but
   could be changed if we move to libtiff */

typedef Err OptIn( struct tagIn *conv, Root *r, 
                   int *argn, int argc, char **argv ) ;
typedef int HasMore( struct tagIn *in ) ;
typedef Err Next( struct tagIn *in, float *r, float *g, float *b ) ;
typedef Err OpenIn( struct tagIn *in, Root *r ) ; 
typedef void CloseIn( struct tagIn *in ) ;

typedef struct tagIn {
  OptIn *opt ;
  HasMore *hasMore ;
  Next *next ;
  OpenIn *open ;
  CloseIn *close ;
  void *private ;
} In ;


/* implementation for pnm files */

typedef struct {
  FILE *in ;
  int type ;
  xelval maxVal ;
  int maxPix ;
  int iPix ;
  xel *row ;
  int maxRow ;
  int iRow ;
} PnmIn ;


/* the only output option is the filename, which will be the last
   argument, if it doesn't start with '-' */
static Err 
pnmOpt( In *in, Root *r, int *argn, int argc, char **argv ) {
  PnmIn *p = (PnmIn*)in->private ;
  if ( *argn + 1 == argc && argv[*argn][0] != '\0' &&
       argv[*argn][0] != '-' ) {
    r->name = argv[(*argn)++] ;
    p->in = pm_openr( r->name ) ;
  }
  return ERR_OK ;
}


/* free the row buffer when closing the input */
static void 
pnmClose( In *in ) {
  if ( in ) {
    PnmIn *p = (PnmIn*)in->private ;
    if ( p ) {
      if ( p->row ) pnm_freerow( p->row ) ;
      if ( p->in ) pm_close( p->in ) ;
      free( p ) ;
    }
  }
}


/* open the file, storing dimensions both locally and in the global
   root structure */
static Err 
pnmOpen( In *in, Root *r ) {

  PnmIn *p = (PnmIn*)in->private ;

  if ( ! p->in ) p->in = stdin ;

  pnm_readpnminit( p->in, &(r->nCols), &(r->nRows), &(p->maxVal), 
                   &(p->type) ) ;
  p->maxPix = r->nCols * r->nRows ;
  p->iPix = 0 ;
  p->maxRow = r->nCols ;
  p->iRow = 0 ;
  p->row = pnm_allocrow( p->maxRow ) ;

  return ERR_OK ;
}


/* more data available? */
static int 
pnmHasMore( In *in ) {
  PnmIn *p = (PnmIn*)in->private ;
  return p->iPix < p->maxPix ;
}


/* read next pixel - buffered by row.  return values in range 0 to 1 */
static Err 
pnmNext( In *in, float *r, float *g, float *b ) {

  PnmIn *p = (PnmIn*)in->private ;
  float m = (float)p->maxVal ;

  if ( p->iPix == 0 || p->iRow == p->maxRow ) {
    p->iRow = 0 ;
    pnm_readpnmrow( p->in, p->row, p->maxRow, p->maxVal, p->type ) ;
  }
  if ( PNM_FORMAT_TYPE( p->type ) == PPM_TYPE ) {
    *r = (float)PPM_GETR( p->row[p->iRow] ) / m ;
    *g = (float)PPM_GETG( p->row[p->iRow] ) / m ;
    *b = (float)PPM_GETB( p->row[p->iRow] ) / m ;
  } else {
    *r = *g = *b = (float)PNM_GET1( p->row[p->iRow] ) / m ;
  }
  ++(p->iRow) ;
  ++(p->iPix) ;

  return ERR_OK ;
}


/* build the input struct */
static Err 
newPnmInput( In **in ) {
  if ( ! (*in = (In*)calloc( 1, sizeof( In ) )) ) {
    fprintf( stderr, "cannot allocate memory\n" ) ;
    return ERR_MEMORY ;
  }
  (*in)->private = calloc( 1, sizeof( PnmIn ) ) ;
  (*in)->opt = pnmOpt ;
  (*in)->open = pnmOpen ;
  (*in)->hasMore = pnmHasMore ;
  (*in)->next = pnmNext ;
  (*in)->close = pnmClose ;
  return ERR_OK ;
}



/* --- writing the output ----------------------------- */


/* encapsulate file writing (will probably always use libtiff, but may
   as well be consistent) */

typedef Err OptOut( struct tagOut *conv, Root *r,
                    int *argn, int argc, char **argv ) ;
typedef Err Write( struct tagOut *out, int c, int m, int y, int k ) ;
typedef Err OpenOut( struct tagOut *out, Root *r ) ;
typedef void CloseOut( struct tagOut *out ) ;

typedef struct tagOut {
  OptOut *opt ;
  Write *write ;
  OpenOut *open ;
  CloseOut *close ;
  void *private ;
} Out ;


/* implementation for tiff files */

typedef struct {
  tdata_t buffer ;
  tsize_t maxBuffer ;
  tsize_t iBuffer ;
  uint32 iRow ;
  TIFF *tiff ;
  uint32 rowsperstrip ;
  uint16 compression ;
  uint16 fillorder ;
  uint16 predictor ;
  uint16 lowdotrange ;
  uint16 highdotrange ;
  int kFlag ;
} TiffOut ;


/* these options come from either the tiff 6.0 spec or the pnmtotiff
   code */
static Err 
tiffOpt( Out *out, Root* rt, int *argn, int argc, char **argv ) {

  Err err ;
  int oldn ;
  long lVal ;
  TiffOut *t = (TiffOut*)out->private ;

  if ( pm_keymatch( argv[*argn], "-none", 3 ) ) {
    t->compression = COMPRESSION_NONE ;
    ++(*argn) ;
  } else if ( pm_keymatch( argv[*argn], "-packbits", 3 ) ) {
    t->compression = COMPRESSION_PACKBITS ;
    ++(*argn) ;
  } else if ( pm_keymatch( argv[*argn], "-lzw", 3 ) ) {
    t->compression = COMPRESSION_LZW ;
    ++(*argn) ;
  } else if ( pm_keymatch( argv[*argn], "-msb2lsb", 2 ) ) {
    t->fillorder = FILLORDER_MSB2LSB ;
    ++(*argn) ;
  } else if ( pm_keymatch( argv[*argn], "-lsb2msb", 3 ) ) {
    t->fillorder = FILLORDER_LSB2MSB ;
    ++(*argn) ;
  } else if ( pm_keymatch( argv[*argn], "-knormal", 3 ) ) {
    t->kFlag = K_NORMAL ;
    ++(*argn) ;
  } else if ( pm_keymatch( argv[*argn], "-kremove", 3 ) ) {
    t->kFlag = K_REMOVE ;
    ++(*argn) ;
  } else if ( pm_keymatch( argv[*argn], "-konly", 3 ) ) {
    t->kFlag = K_ONLY ;
    ++(*argn) ;
  } else {
    oldn = *argn ;
    if ( (err = longArg( &lVal, "-predictor", 3, 1, 2,
                         argn, argc, argv )) ) return err ;
    if ( oldn != *argn ) {
      t->predictor = (uint16)lVal ;
    } else {
      if ( (err = longArg( &lVal, "-rowsperstrip", 2, 1, INT_MAX,
                           argn, argc, argv )) ) return err ;
    }
    if ( oldn != *argn ) {
      t->rowsperstrip = (uint32)lVal ;
    } else {
      if ( (err = longArg( &lVal, "-lowdotrange", 3, 0, 
                           (int)(t->highdotrange - 1),
                           argn, argc, argv )) ) return err ;
    }
    if ( oldn != *argn ) {
      t->lowdotrange = (uint16)lVal ;
    } else {
      if ( (err = longArg( &lVal, "-highdotrange", 2, 
                           (int)(t->lowdotrange + 1), MAXTIFFVAL,
                           argn, argc, argv )) ) return err ;
    }
    if ( oldn != *argn ) {
      t->highdotrange = (uint16)lVal ;
    }
  }

  return ERR_OK ;
}


/* helper routine - writes individual bytes */
static Err 
tiffWriteByte( TiffOut *t, int b ) {
  ((unsigned char*)(t->buffer))[t->iBuffer] = (unsigned char)b ;
  if ( ++(t->iBuffer) == t->maxBuffer ) {
    if ( TIFFWriteScanline( t->tiff, t->buffer, t->iRow, 0 ) == -1 ) {
      return ERR_TIFF ;
    }
    ++(t->iRow) ;
    t->iBuffer = 0 ;
  }
  return ERR_OK ;
}


/* write the pixel to the tiff file */
static Err 
tiffWrite( Out *out, int c, int m, int y, int k ) {
  Err err ;
  TiffOut *t = (TiffOut*)out->private ;
  if ( t->kFlag == K_ONLY ) {
    c = m = y = k ;
  } else if ( t->kFlag == K_REMOVE ) {
    k = 0 ;
  }
  if ( (err = tiffWriteByte( t, c )) ) return err ;
  if ( (err = tiffWriteByte( t, m )) ) return err ;
  if ( (err = tiffWriteByte( t, y )) ) return err ;
  if ( (err = tiffWriteByte( t, k )) ) return err ;
  return ERR_OK ;
}


/* open output to stdout - see warning below */
static Err 
tiffOpen( Out* out, Root *r ) {

  TiffOut *t = (TiffOut*)out->private ;

  short samplesperpixel = 4 ; /* cmyk has four values */
  uint16 bitspersample = MAXTIFFBITS ;
  short photometric = PHOTOMETRIC_SEPARATED ; /* ie cmyk */
  int bytesperrow = r->nCols ;

  /* if i don't set stdout non-blocking on my machine then the read
     that is called inside TIFFFdOpen hangs until the users types ^D.
     this is also true for pnmtotiff */
  fcntl( 1, F_SETFL, O_NONBLOCK ) ;
  t->tiff = TIFFFdOpen( 1, "Standard Output", "w" ) ;
  if ( ! t->tiff ) {
    fprintf( stderr, "cannot open tiff stream to standard output\n" ) ;
    return ERR_TIFF ;
  }

  /* from pnmtotiff - default is to have 8kb strips */
  if ( ! t->rowsperstrip ) {
    t->rowsperstrip = ( 8 * 1024 ) / bytesperrow ;
  }

  TIFFSetField( t->tiff, TIFFTAG_DOTRANGE, t->lowdotrange, t->highdotrange ) ;
  TIFFSetField( t->tiff, TIFFTAG_IMAGEWIDTH, (uint32)r->nCols ) ;
  TIFFSetField( t->tiff, TIFFTAG_IMAGELENGTH, (uint32)r->nRows ) ;
  TIFFSetField( t->tiff, TIFFTAG_BITSPERSAMPLE, bitspersample ) ;
  TIFFSetField( t->tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT ) ;
  TIFFSetField( t->tiff, TIFFTAG_COMPRESSION, t->compression ) ;
  if ( t->compression == COMPRESSION_LZW && t->predictor ) {
	TIFFSetField( t->tiff, TIFFTAG_PREDICTOR, t->predictor ) ;
  }
  TIFFSetField( t->tiff, TIFFTAG_PHOTOMETRIC, photometric ) ;
  TIFFSetField( t->tiff, TIFFTAG_FILLORDER, t->fillorder ) ;
  TIFFSetField( t->tiff, TIFFTAG_DOCUMENTNAME, r->name ) ;
  TIFFSetField( t->tiff, TIFFTAG_IMAGEDESCRIPTION, "PNM -> CMYK tiff" ) ;
  TIFFSetField( t->tiff, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel ) ;
  TIFFSetField( t->tiff, TIFFTAG_ROWSPERSTRIP, t->rowsperstrip ) ;
  TIFFSetField( t->tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG ) ;

  t->maxBuffer = TIFFScanlineSize( t->tiff ) ;
  t->buffer = _TIFFmalloc( t->maxBuffer ) ;
  t->iBuffer = 0 ;
  t->iRow = 0 ;
  if ( ! t->buffer ) {
    fprintf( stderr, "cannot allocate memory\n" ) ;
    return ERR_MEMORY ;
  }
  
  return ERR_OK ;
}


/* close file and tidy memory */
static void 
tiffClose( Out *out ) {
  if ( out ) {
    TiffOut *t = (TiffOut*)out->private ;
    if ( t ) {
      if ( t->buffer ) _TIFFfree( t->buffer ) ;
      if ( t->tiff ) TIFFClose( t->tiff ) ;
      free( t ) ;
    }
    free( out ) ;
  }
}


/* assemble the routines above into a single struct/object */
static Err 
newTiffOutput( Out **out ) {

  TiffOut *t ;

  if ( ! (*out = (Out*)calloc( 1, sizeof( Out ) )) ) goto error ;
  if ( ! (t = (TiffOut*)calloc( 1, sizeof( TiffOut ) )) ) goto error ;
  t->compression = COMPRESSION_LZW ;
  t->fillorder = FILLORDER_MSB2LSB ;
  t->predictor = 0 ;
  t->rowsperstrip = 0 ;
  t->lowdotrange = 0 ;
  t->highdotrange = MAXTIFFVAL ;
  t->kFlag = K_NORMAL ;
  (*out)->private = t ;
  (*out)->opt = tiffOpt ;
  (*out)->open = tiffOpen ;
  (*out)->write = tiffWrite ;
  (*out)->close = tiffClose ;
  return ERR_OK ;

 error:
  fprintf( stderr, "cannot allocate memory\n" ) ;
  return ERR_MEMORY ;
}



/* --- conversion ------------------------------------- */


/* encapsulate conversion - allow easy extension with other conversion
   algorithms.  if selection of conversion routine is by command line
   flag then that flag must precede conversion parameters (for
   simplicity in argument parsing) */

typedef Err Opt( struct tagConv *conv, Root *r,
                 int *argn, int argc, char **argv ) ;
typedef Err Convert( struct tagConv *conv, Root *rt,
                     float r, float g, float b, 
                     int *c, int *m, int *y, int *k ) ;
typedef void Close( struct tagConv *conv ) ;

typedef struct tagConv {
  Opt *opt ;
  Convert *convert ;
  Close *close ;
  void *private ;
} Conv ;


/* include two conversion routines to show how the code can be
   extended.  first, the standard converter, as outlined in the
   proposal, then a simple replacement that produces colour 
   negatives */


/* implementation with hue rotation, black gamma correction and
   removal of cmy under k */

typedef struct {
  int initialized ;
  float theta ;
  float gamma ;
  float gammap ;
  int remove ;
} Standard ;


/* represent an rgb colour as a hue (angle), white level and colour
   strength */
static void 
rgbToHueWhiteColour( float r, float g, float b, 
                     double *phi, float *white, float *colour ) {
  *white = MIN( r, MIN( g, b ) ) ;
  r -= *white ;
  g -= *white ;
  b -= *white ;
  *colour = sqrt( r * r + g * g + b * b ) ;
  if ( r > TINY || g > TINY || b > TINY ) {
    if ( b < r && b <= g ) { 
      *phi = EXPAND( atan2( g, r ) ) ;
    } else if ( r < g && r <= b ) {
      *phi = ONETWENTY + EXPAND( atan2( b, g ) ) ;
    } else {
      *phi = TWOFORTY + EXPAND( atan2( r, b ) ) ;
    }
  } 
}


/* represent hue, white and colour values as rgb */
static void 
hueWhiteColourToRgb( double phi, float white, float colour,
                     float *r, float *g, float *b ) {
  while ( phi < 0 ) { phi += THREESIXTY ; }
  while ( phi > THREESIXTY ) { phi -= THREESIXTY ; }
  if ( phi < ONETWENTY ) {
    phi = CONTRACT( phi ) ;
    *r = colour * cos( phi ) ;
    *g = colour * sin( phi ) ;
    *b = 0.0 ;
  } else if ( phi < TWOFORTY ) {
    phi = CONTRACT( phi - ONETWENTY ) ;
    *r = 0.0 ;
    *g = colour * cos( phi ) ;
    *b = colour * sin( phi ) ;
  } else {
    phi = CONTRACT( phi - TWOFORTY ) ;
    *r = colour * sin( phi ) ;
    *g = 0.0 ;
    *b = colour * cos( phi ) ;
  }
  *r += white ;
  *g += white ;
  *b += white ;
}


/* for details, see the proposal.  it's pretty simple - a rotation
   before conversion, with colour removal */
static Err 
standardConvert( Conv *conv, Root *rt, float r, float g, float b, 
                 int *c, int *m, int *y, int *k ) {

  float c0, m0, y0, k0 ; /* CMYK before colour removal */
  float gray, white, colour ;
  double phi ;

  Standard *s ; /* private conversion data */

  s = (Standard*)(conv->private) ;

  if ( ! s->initialized ) {
    s->theta *= 2.0 * PI / 360.0 ; /* to radians */
    /* if gammap never specified in options, set to gamma now */
    if ( s->remove == UNSET ) {
      s->remove = 1 ;
      s->gammap = s->gamma ;
    }
    s->initialized = 1 ;
  }

  /* rotate in rgb */
  if ( fabs( s->theta ) > TINY ) {
    rgbToHueWhiteColour( r, g, b, &phi, &white, &colour ) ;
    hueWhiteColourToRgb( phi + s->theta, white, colour, &r, &g, &b ) ;
  }

  c0 = LIMIT01( 1.0 - r ) ;
  m0 = LIMIT01( 1.0 - g ) ;
  y0 = LIMIT01( 1.0 - b ) ;
  k0 = MIN( c0, MIN( y0, m0 ) ) ;

  /* apply gamma corrections to modify black levels */
  *k = TOMAX( pow( k0, s->gamma ) ) ;

  /* remove colour under black and convert to integer range 0 - rt->maxOut */
  if ( s->remove ) {
    gray = pow( k0, s->gammap ) ;
  } else {
    gray = 0.0 ;
  }
  *c = TOMAX( c0 - gray ) ;
  *m = TOMAX( m0 - gray ) ;
  *y = TOMAX( y0 - gray ) ;

  return ERR_OK ;
}


/* parse options for this conversion - note the ugly use of -1 for
   gammap to indicate no removal of cmy under k */
static Err 
standardOpt( Conv *conv, Root *r, int *argn, int argc, char **argv ) {

  Err err ;
  int oldn ;
  Standard *p = (Standard*)conv->private ;

  oldn = *argn ;
  if ( (err = floatArg( &(p->theta), "-theta", 2, -360.0, 360.0,
                        argn, argc, argv )) ) return err ;
  if ( oldn == *argn ) {
    if ( (err = floatArg( &(p->gamma), "-gamma", 6, 0.1, 10.0,
                          argn, argc, argv )) ) return err ;
    /* gammap traces gamma unless set separately */
    if ( oldn != *argn && p->remove == UNSET ) p->gammap = p->gamma ;
  }
  /* handle the special case of -1 (no removal) */
  if ( oldn == *argn && pm_keymatch( argv[*argn], "-gammap", 7 ) &&
       *argn + 1 < argc && STREQ(argv[*argn + 1], "-1") ) {
    p->remove = 0 ;
    *argn = (*argn) + 2 ;
  } 
  if ( oldn == *argn ) {
    if ( (err = floatArg( &(p->gammap), "-gammap", 7, 0.1, 10.0,
                               argn, argc, argv )) ) return err ;
    if ( oldn != *argn ) p->remove = 1 ;
  }

  return ERR_OK ;
}


/* free conversion structure */
static void 
standardClose( Conv *conv ) {
  if ( conv ) {
    if ( conv->private ) { free( conv->private ) ; }
    free( conv ) ; 
  }
}


/* build new conversion structure */
static Err 
newStandardMap( Conv **conv ) {

  Standard *s ;

  if ( ! (*conv = (Conv*)calloc( 1, sizeof( Conv ) )) ) goto error ;
  if ( ! (s = (Standard*)calloc( 1, sizeof( Standard ) )) ) goto error ;
  s->initialized = 0 ;
  s->remove = UNSET ; /* gammap unset */
  s->gamma = 1.0 ;
  (*conv)->private = s ;
  (*conv)->opt = standardOpt ;
  (*conv)->convert = standardConvert ;
  (*conv)->close = standardClose ;

  return ERR_OK ;

 error:
  fprintf( stderr, "cannot allocate memory\n" ) ;
  return ERR_MEMORY ;
}



/* next a simple demonstration of how to implement other conversion
   algorithms - in this case, something that produces a "colour
   negative" (just to be different) */


/* the conversion routine must match the Convert typedef */
static Err 
negativeConvert( Conv *conv, Root *rt, float r, float g, float b,
                 int *c, int *m, int *y, int *k ) {

  /* the simple conversion is c=1-r, but for negatives we will use
     c=r, so only need to convert from rgb 0-1 floats to cmyk
     0-MAXTIFFVAL ints */
  *c = TOMAX( r ) ;
  *m = TOMAX( g ) ;
  *y = TOMAX( b ) ;
  *k = MIN( *c, MIN( *m, *y ) ) ;

  return ERR_OK ;
}


/* since this simple conversion takes no parameters, we don't need to
   parse them - this routine must match the Opt typedef */
static Err 
negativeOpt( Conv *conv, Root *r, int *argn, int argc, char **argv ) {
  return ERR_OK ;
}


/* with no parameters we haven't needed the private data structure, so
   closing is trivial - this routine must match the Close typedef */
static void 
negativeClose( Conv *conv ) { }


/* and that's it, apart from assembling the routines above into a
   single struct/object (and adding code in parseOpts to select the
   algorithm and printUsage to help the user) */
static Err 
newNegativeMap( Conv **conv ) {

  if ( ! (*conv = (Conv*)calloc( 1, sizeof( Conv ) )) ) goto error ;
  (*conv)->opt = negativeOpt ;
  (*conv)->convert = negativeConvert ;
  (*conv)->close = negativeClose ;

  return ERR_OK ;

 error:
  fprintf( stderr, "cannot allocate memory\n" ) ;
  return ERR_MEMORY ;
}



/* --- general routines ------------------------------- */


/* run through args, passing to sub components */
static Err 
parseOpts( int argc, char **argv, Root *r ) {

  int argn = 1 ;
  int oldn ;
  Err err ;

  /* set default reader, writer and converter */
  if ( (err = newPnmInput( &(r->in) )) ) return err ;
  if ( (err = newTiffOutput( &(r->out) )) ) return err ;
  if ( (err = newStandardMap( &(r->conv) )) ) return err ;

  /* minimum number of chars from inspection - this is standard netpbm
     approach - so must check when adding more options */
  while ( argn < argc ) {

    /* first, high-level options that select components - currently
       just the default or negative converter */
    if ( pm_keymatch( argv[argn], "-default", 2 ) ) {
      if ( r->conv ) { r->conv->close( r->conv ) ; }
      if ( (err = newStandardMap( &(r->conv) )) ) return err ;
      ++argn ;
    } else if ( pm_keymatch( argv[argn], "-negative", 3 ) ) {
      if ( r->conv ) { r->conv->close( r->conv ) ; }
      if ( (err = newNegativeMap( &(r->conv) )) ) return err ;
      ++argn ;
    } else if ( pm_keymatch( argv[argn], "-help", 2 ) ) {
      printUsage( ) ;
      return ERR_HELP ;
    } else {
      /* next, try passing the option to each subcomponent */
      oldn = argn ;
      if ( (err = r->in->opt( r->in, r, &argn, argc, argv )) ) {
        return err ;
      } else if ( oldn == argn &&
                  (err = r->out->opt( r->out, r, &argn, argc, argv )) ) {
        return err ;
      } else if ( oldn == argn &&
                  (err = r->conv->opt( r->conv, r, &argn, argc, argv )) ) {
        return err ;
      } else if ( oldn == argn ) {
        fprintf( stderr, "unexpected arg: %s\n", argv[argn] ) ;
        return ERR_ARG ;
      }
    }
  }

  return ERR_OK ;
}


/* drive the reading, conversion, and writing */
int main( int argc, char **argv ) {

  Root *rt ;
  float r, g, b ;
  int c, m, y, k ;
  Err err = ERR_OK ;

  pnm_init(&argc, argv);

  if ( ! (rt = (Root*)calloc( 1, sizeof( Root ) )) ) {
    err = ERR_MEMORY ;
    fprintf( stderr, "cannot allocate memory\n" ) ;
    goto exit ;
  }
  rt->name = "Standard input" ;
  rt->maxOut = MAXTIFFVAL ;

  if ( (err = parseOpts( argc, argv, rt )) ) goto exit ;
  
  if ( (err = rt->in->open( rt->in, rt )) ) goto exit ;
  if ( (err = rt->out->open( rt->out, rt )) ) goto exit ;

  while ( rt->in->hasMore( rt->in ) ) {
    if ( (err = rt->in->next( rt->in, &r, &g, &b )) ) goto exit ;
    if ( (err = rt->conv->convert( rt->conv, rt, r, g, b, &c, &m, &y, &k )) )
      goto exit ;
    if ( (err = rt->out->write( rt->out, c, m, y, k )) ) goto exit ;
  }

 exit:

  if ( rt && rt->out && rt->out->close ) rt->out->close( rt->out ) ;
  if ( rt && rt->conv && rt->conv->close ) rt->conv->close( rt->conv ) ;
  if ( rt && rt->in && rt->in->close ) rt->in->close( rt->in ) ;
  if ( rt ) free( rt ) ;

  if ( err == ERR_ARG ) printUsage( ) ;

  return err ;
}



