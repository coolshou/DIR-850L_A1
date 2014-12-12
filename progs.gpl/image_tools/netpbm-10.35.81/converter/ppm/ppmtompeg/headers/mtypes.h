/*===========================================================================*
 * mtypes.h
 *
 *	MPEG data types
 *
 *===========================================================================*/

/* Copyright information is at end of file */

#ifndef MTYPES_INCLUDED
#define MTYPES_INCLUDED


/*==============*
 * HEADER FILES *
 *==============*/

#include "general.h"
#include "dct.h"


/*===========*
 * CONSTANTS *
 *===========*/

#define TYPE_BOGUS	0   /* for the header of the circular list */
#define TYPE_VIRGIN	1

#define STATUS_EMPTY	0
#define STATUS_LOADED	1
#define STATUS_WRITTEN	2


typedef struct vector {
    int y;
    int x;
} vector;

typedef struct motion {
    vector fwd;
    vector bwd;
} motion;

/*==================*
 * TYPE DEFINITIONS *
 *==================*/

/*  
 *  your basic Block type
 */
typedef int16 Block[DCTSIZE][DCTSIZE];
typedef int16 FlatBlock[DCTSIZE_SQ];
typedef	struct {
    int32 l[2*DCTSIZE][2*DCTSIZE];
} LumBlock;
typedef	int32 ChromBlock[DCTSIZE][DCTSIZE];

/*========*
 * MACROS *
 *========*/

#ifdef ABS
#undef ABS
#endif

#define ABS(x) (((x)<0)?-(x):(x))

#ifdef HEINOUS_DEBUG_MODE
#define DBG_PRINT(x) {printf x; fflush(stdout);}
#else
#define DBG_PRINT(x)
#endif

#define ERRCHK(bool, str) {if(!(bool)) {perror(str); exit(1);}}

/* For Specifics */
typedef struct detalmv_def {
  int typ,fx,fy,bx,by;
} BlockMV;
#define TYP_SKIP 0
#define TYP_FORW 1
#define TYP_BACK 2
#define TYP_BOTH 3


#endif /* MTYPES_INCLUDED */


/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

