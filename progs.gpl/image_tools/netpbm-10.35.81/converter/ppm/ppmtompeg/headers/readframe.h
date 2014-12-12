/*===========================================================================*
 * readframe.h								     *
 *									     *
 *	stuff dealing with reading frames				     *
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

#include "pm_c_util.h"

/*===========*
 * CONSTANTS *
 *===========*/

#define	PPM_FILE_TYPE	    0
#define YUV_FILE_TYPE	    2
#define ANY_FILE_TYPE	    3
#define BASE_FILE_TYPE	    4
#define PNM_FILE_TYPE	    5
#define SUB4_FILE_TYPE	    6
#define JPEG_FILE_TYPE	    7
#define JMOVIE_FILE_TYPE    8
#define Y_FILE_TYPE	    9


struct inputSource;

void
ReadFrameFile(MpegFrame *  const frameP,
              FILE *       const ifP,
              const char * const conversion,
              bool *       const eofP);

void
ReadFrame(MpegFrame *          const frameP, 
          struct inputSource * const inputSourceP,
          unsigned int         const frameNumber,
          const char *         const conversion,
          bool *               const endOfStreamP);

FILE *
ReadIOConvert(struct inputSource * const inputSourceP,
              unsigned int         const frameNumber);

extern void	SetFileType(const char * const conversion);
extern void	SetFileFormat(const char * const format);
extern void	SetResize(bool const set);

extern int	baseFormat;
