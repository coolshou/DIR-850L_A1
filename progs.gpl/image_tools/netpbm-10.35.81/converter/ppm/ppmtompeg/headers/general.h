/*===========================================================================*
 * general.h								     *
 *									     *
 *	very general stuff						     *
 *									     *
 *===========================================================================*/

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

/*  
 *  $Header: /n/picasso/project/mpeg/mpeg_dist/mpeg_encode/headers/RCS/general.h,v 1.7 1995/08/04 23:34:13 smoot Exp $
 *  $Log: general.h,v $
 *  Revision 1.7  1995/08/04 23:34:13  smoot
 *  jpeg5 changed the silly HAVE_BOOLEAN define....
 *
 *  Revision 1.6  1995/01/19 23:54:49  eyhung
 *  Changed copyrights
 *
 * Revision 1.5  1994/11/12  02:12:48  keving
 * nothing
 *
 * Revision 1.4  1993/07/22  22:24:23  keving
 * nothing
 *
 * Revision 1.3  1993/07/09  00:17:23  keving
 * nothing
 *
 * Revision 1.2  1993/06/03  21:08:53  keving
 * nothing
 *
 * Revision 1.1  1993/02/22  22:39:19  keving
 * nothing
 *
 */


#ifndef GENERAL_INCLUDED
#define GENERAL_INCLUDED


/* prototypes for library procedures
 *
 * if your /usr/include headers do not have these, then pass -DMISSING_PROTOS
 * to your compiler
 *
 */ 
#ifdef MISSING_PROTOS
int fprintf();
int fwrite();
int fread();
int fflush();
int fclose();

int sscanf();
int bzero();
int bcopy();
int system();
int time();
int perror();

int socket();
int bind();
int listen();
int accept();
int connect();
int close();
int read();
int write();

int pclose();

#endif


/*===========*
 * CONSTANTS *
 *===========*/

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define SPACE ' '
#define TAB '\t'
#define SEMICOLON ';'
#define NULL_CHAR '\0'
#define NEWLINE '\n'


/*==================*
 * TYPE DEFINITIONS *
 *==================*/

#ifndef HAVE_BOOLEAN
typedef int boolean;
#define HAVE_BOOLEAN
/* JPEG library also defines boolean */
#endif

/* In the following, we need the "signed" in order to make these typedefs
   match those in AIX system header files.  Otherwise, compile fails on 
   AIX.  2000.09.11.
*/
typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;

/* The 32 bit integer types are probably obsolete.

   parallel.c used to use them as buffers for socket I/O, because in the
   protocol on the socket, an integer is represented by 4 octets.  But
   the proper type for that is unsigned char[4], so we changed parallel.c
   to that in September 2004.

   A user of TRU64 4.0f in May 2000 used a -DLONG_32 option, but did
   not indicate that it was really necessary.  After that, Configure
   added -DLONG_32 for all TRU64 builds.  A user of TRU64 5.1A in July
   2003 demonstrated that int is in fact 4 bytes on his machine (and
   long is 8 bytes).  So we removed the -DLONG_32 from Configure.  This
   whole issue may too be obsolete because of the fixing of parallel.c
   mentioned above.
*/

    /* LONG_32 should only be defined iff
     *	    1) long's are 32 bits and
     *	    2) int's are not
     */
#ifdef LONG_32		
typedef unsigned long uint32;
typedef long int32;
#else
typedef unsigned int uint32;
typedef signed int int32;
#endif


/*========*
 * MACROS *
 *========*/

#undef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#undef abs
#define abs(a) ((a) >= 0 ? (a) : -(a))


#endif
