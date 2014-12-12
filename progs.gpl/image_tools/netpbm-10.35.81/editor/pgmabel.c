/* pgmabel.c - read a portable graymap and making the deconvolution
**
**      Deconvolution of an axial-symmetric image of an rotation symmetrical
**      process by solving the linear equation system with y-Axis as
**      symmetry-line
**
** Copyright (C) 1997-2006 by German Aerospace Research establishment
**
** Author: Volker Schmidt
**         lefti@voyager.boerde.de
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** $HISTORY:
**
** 24 Jan 2002 : 001.009 :  some optimzization
** 22 Jan 2002 : 001.008 :  some stupid calculations changed
** 08 Aug 2001 : 001.007 :  new usage (netpbm-conform)
** 27 Jul 1998 : 001.006 :  First try of error correction
** 26 Mar 1998 : 001.005 :  Calculating the dl's before transformation
** 06 Feb 1998 : 001.004 :  Include of a logo in the upper left edge
** 26 Nov 1997 : 001.003 :  Some bug fixes and reading only lines
** 25 Nov 1997 : 001.002 :  include of pixsize for getting scale invariant
** 03 Sep 1997 : 001.001 :  only define for PID2
** 03 Sep 1997 : 001.000 :  First public release
** 21 Aug 1997 : 000.909 :  Recalculate the streching-factor
** 20 Aug 1997 : 000.908 :  -left and -right for calculating only one side
** 20 Aug 1997 : 000.906 :  correction of divisor, include of -factor
** 15 Aug 1997 : 000.905 :  Include of -help and -axis
*/

static const char* const version="$VER: pgmabel 1.009 (24 Jan 2002)";

#include <math.h>
#include <stdlib.h>   /* for calloc */
#include "pgm.h"
#include "mallocvar.h"

#ifndef PID2          /*  PI/2 (on AMIGA always defined) */
#define PID2    1.57079632679489661923  
#endif

#define TRUE 1
#define FALSE 0

/* some global variables */
static double *aldl, *ardl;                /* pointer for weighting factors */

/* ----------------------------------------------------------------------------
** procedure for calculating the sum of the calculated surfaces with the
** weight of the surface
**      n     <-  index of end point of the summation
**      N     <-  width of the calculated row
**      xr    <-  array of the calculated elements of the row
**      adl   <-  pre-calculated surface coefficient for each segment
*/
static double 
Sum ( int n, double *xr, int N, double *adl)
{
    int k;
    double result=0.0;

    if (n==0) return(0.0);             /* outer ring  is 0 per definition    */
    for (k=0 ; k<=(n-1) ; k++)
    {
         result += xr[k] * ( adl[k*N+n] - adl[(k+1)*N+n]);
/*       result += xr[k] * ( dr(k,n+0.5,N) - dr(k+1,n+0.5,N));   */
    }
    return(result);
}

/* ----------------------------------------------------------------------------
** procedure for calculating the surface coefficient for the Integration
**      R, N  <-  indizes of the coefficient
**      r     <-  radial position of the center of the surface
*/
static double 
dr ( int R, double r,  int N)
{
    double a;
    double b;
    a=(double) N-R ;
    b=(double) N-r ;
    return(sqrt(a*a-b*b));
}

/* ----------------------------------------------------------------------------
** procedure for making the Abel integration for deconvolution of the image
**        y    <-> array with values for deconvolution and results
**        N    <-  width of the array
**        adl  <-  array with pre-calculated weighting factors
*/
static void 
abel ( float *y, int N, double *adl)
{
    register int n;
    double *rho, *rhop;       /* results and new index                       */
    float  *yp;               /* new indizes for the y-array                 */

    MALLOCARRAY(rho, N);
    if( !rho )
        pm_error( "out of memory" );
    rhop = rho;
    yp  = y;

    for (n=0 ; n<N ; n++)
    {
        *(rhop++) = ((*yp++) - Sum(n,rho,N,adl))/(adl[n*N+n]);
/*    *(rhop++) = ((*yp++) - Sum(n,rho,N))/(dr(n,n+0.5,N));  old version */
        if ( *rhop < 0.0 ) *rhop = 0.0;         /*  error correction !       */
/*   if (n > 2) rhop[n-1] = (rho[n-2]+rho[n-1]+rho[n])/3.0;  stabilization*/
    }
    for (n=0 ; n<N ; n++)
        {
            if (( n>=1 )&&( n<N-1 ))
	       (*y++) = ((rho[n-1]*0.5+rho[n]+rho[n+1]*0.5)/2.0);/*1D median filter*/
            else (*y++) = rho[n];
        }
    free(rho);
}

/* ----------------------------------------------------------------------------
** printing a help message if Option -h(elp) is chosen
*/
static void 
help()
{
    pm_message("-----------------------------------------------------------------");
    pm_message("| pgmabel                                                       |");
    pm_message("| make a deconvolution with vertical axis as symmetry-line      |");
    pm_message("| usage:                                                        |");
    pm_message("| pgmabel [-help] [-axis N] [-factor N] [-left|-right]          |");
    pm_message("|         [-pixsize] [-verbose] [pgmfile]                       |");
    pm_message("|   axis    : horizontal position of the axis                   |");
    pm_message("|   factor  : user defines stretch-factor for the gray levels   |");
    pm_message("|   pixsize : size of one pixel in mm (default = 0.1)           |");
    pm_message("|   left    : calculating only the left (or right) side         |");
    pm_message("|   verbose : output of useful data                             |");
    pm_message("|   pgmfile : Name of a pgmfile (optional)                      |");
    pm_message("|                                                               |");
    pm_message("| for further information please contact the manpage            |"); 
    pm_message("-----------------------------------------------------------------");
    pm_message("%s",version);     /* telling the version      */
    exit(-1);                     /* retur-code for no result */
}





/* ----------------------------------------------------------------------------
** main program
*/
int main( argc, argv )
    int    argc;
    char*  argv[];
{
    FILE*  ifp;
    gray maxval;                            /* maximum gray-level            */
    gray* grayorig;
    gray* grayrow;                          /* one line in the image         */
    int argn, rows, cols, row, format;
    int col, midcol=0, temp, tc;
    float *trow;                          /* temporary row for deconvolution */
    float l_div, r_div, fac=1.0, cfac=4.0;  /* factor for scaling gray-level */
    float pixsize=0.1;
    /* no verbose, calculating both sides                                */
    int verb = FALSE, left = TRUE, right = TRUE;
    int nologo = FALSE;
    const char* const usage = "[-help] [-axis N] [-factor N] [-pixsize N] [-left|-right] [-verbose] [pgmfile]";

    pgm_init( &argc, argv );
    argn = 1;

    /* Check for flags. */
    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
        {
        if ( pm_keymatch( argv[argn], "-help", 1 ) ) help();
        else if ( pm_keymatch( argv[argn], "-axis", 1 ) )
            {
            ++argn;
            if ( argn == argc || sscanf( argv[argn], "%i", &midcol ) !=1 )
                pm_usage( usage );
            }
        else if ( pm_keymatch( argv[argn], "-factor", 1 ) )
            {
            ++argn;
            if ( argn == argc || sscanf( argv[argn], "%f", &fac ) !=1 )
                pm_usage( usage );
            }
        else if ( pm_keymatch( argv[argn], "-pixsize", 1 ) )
            {
            ++argn;
            if ( argn == argc || sscanf( argv[argn], "%f", &pixsize ) !=1 )
                pm_usage( usage );
            }
        else if ( pm_keymatch( argv[argn], "-verbose", 1 ) )
            {
                verb = TRUE;
            }
        else if ( pm_keymatch( argv[argn], "-left", 1 ) )
            {
                if ( left ) right = FALSE;
                else pm_usage( usage );
            }
        else if ( pm_keymatch( argv[argn], "-right", 1 ) )
           {
                if ( right ) left = FALSE;
                else pm_usage( usage );
            }
        else if ( pm_keymatch( argv[argn], "-nologo", 4 ) )
            {
                nologo = TRUE;
            }
        else
            pm_usage( usage );
        ++ argn;
        }
    if ( argn < argc )
        {
        ifp = pm_openr( argv[argn] );                    /* open the picture */
        ++argn;
        }
    else
        ifp = stdin;                                /* or reading from STDIN */
    if ( argn != argc )
        pm_usage( usage );

    pgm_readpgminit( ifp, &cols, &rows, &maxval, &format );  /* read picture  */
    pgm_writepgminit( stdout, cols, rows, maxval, 0 );  /* write the header  */
    grayorig = pgm_allocrow(cols);
    grayrow = pgm_allocrow( cols );                     /* allocate a row    */

    if (midcol == 0) midcol = cols/2;     /* if no axis set take the center */
    if (left ) l_div = (float)(PID2*pixsize)/(cfac*fac);
    else l_div=1.0;                              /* weighting the left side  */
    if (right) r_div = (float)(PID2*pixsize)/(cfac*fac);
    else r_div=1.0;                              /* weighting the right side */

    if (verb)
    {
        pm_message("%s",version);
        pm_message("Calculating a portable graymap with %i rows and %i cols",rows,cols);
        pm_message("  resuming a pixelsize of %f mm",pixsize);
        if ( !right ) pm_message("     only the left side!");
        if ( !left ) pm_message("     only the right side!");
        pm_message("  axis = %i, stretching factor = %f",midcol,cfac*fac);
        if ( left ) pm_message("  left side weighting = %f",l_div);
        if ( right ) pm_message(" right side weighting = %f",r_div);
    }

    /* allocating the memory for the arrays aldl and ardl                    */
    aldl = calloc ( midcol*midcol, sizeof(double));
    if( !aldl )
        pm_error( "out of memory" );
    ardl = calloc ( (cols-midcol)*(cols-midcol), sizeof(double));
    if( !ardl )
        pm_error( "out of memory" );

    MALLOCARRAY(trow, cols);
    if( !trow )
        pm_error( "out of memory" );

    /* now precalculating the weighting-factors for the abel-transformation  */
    for (col = 0; col < midcol; ++col)             /* factors for left side  */
    {
        for (tc = 0; tc < midcol; ++tc) aldl[col*midcol+tc] = dr(col,tc+0.5,midcol);
    }
    for (col = 0; col < (cols-midcol); ++col)      /* factors for right side */
    {
        for (tc = 0; tc < (cols-midcol); ++tc) 
            ardl[col*(cols-midcol)+tc] = dr(col,tc+0.5,cols-midcol);
    }

    /* abel-transformation for each row splitted in right and left side      */
    for ( row = 0; row < rows ; ++row )
    {
        pgm_readpgmrow( ifp, grayorig, cols, maxval, format );
        for ( col = 0; col < midcol; ++col)          /* left side            */
        {
            trow[col] = (float) (grayorig[col]);
        }
        if (left ) abel(trow, midcol, aldl);         /* deconvolution        */
        for ( col = 0; col < midcol; ++col)          /* writing left side    */
        {
            temp = (int)(trow[col]/l_div);
            grayrow[col] = (temp>0?temp:0);
        }
        for ( col = midcol; col < cols; ++col )      /* right side           */
        {
            trow[cols-col-1] = (float) (grayorig[col]);
        }
        if ( right ) abel(trow,(cols-midcol),ardl);  /* deconvolution        */
        for ( col = midcol; col < cols; ++col)       /* writing right side   */
        {
            temp = (int)(trow[cols-col-1]/r_div);
            temp = (temp>0?temp:0);
            grayrow[col] = temp;
        }
        pgm_writepgmrow( stdout, grayrow, cols, maxval, 0 );  /* saving row  */
    }
    pm_close( ifp );
    pm_close( stdout );               /* closing output                      */
    free( trow );                     /* deconvolution is done, clear memory */
    pgm_freerow( grayorig );
    pgm_freerow( grayrow );
    free(aldl);
    free(ardl);                      /* all used memory freed (i hope)       */
    exit( 0 );                       /* end of procedure                     */
}

