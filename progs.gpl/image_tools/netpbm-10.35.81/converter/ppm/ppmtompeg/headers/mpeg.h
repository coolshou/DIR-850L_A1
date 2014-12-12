/*===========================================================================*
 * mpeg.h								     *
 *									     *
 *	no comment							     *
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

/*==============*
 * HEADER FILES *
 *==============*/

#include <time.h>

#include "pm_c_util.h"
#include "ppm.h"
#include "ansi.h"
#include "mtypes.h"
#include "frame.h"

struct inputSource;

/*===============================*
 * EXTERNAL PROCEDURE prototypes *
 *===============================*/

enum frameContext {CONTEXT_WHOLESTREAM, CONTEXT_GOP, CONTEXT_JUSTFRAMES};

void
GenMPEGStream(struct inputSource * const inputSourceP,
              enum frameContext    const context, 
              unsigned int         const frameStart, 
              unsigned int         const frameEnd, 
              int32                const qtable[], 
              int32                const niqtable[], 
              bool                 const childProcess,
              FILE *               const ofp, 
              const char *         const outputFileName,
              bool                 const wantVbvUnderflowWarning,
              bool                 const wantVbvOverflowWarning,
              unsigned int *       const inputFrameBitsP,
              unsigned int *       const totalBitsP);

void
PrintStartStats(time_t               const startTime, 
                bool                 const specificFrames,
                unsigned int         const firstFrame, 
                unsigned int         const lastFrame,
                struct inputSource * const inputSourceP);

void
PrintEndStats(time_t       const startTime,
              time_t       const endTime,
              unsigned int const inputFrameBits, 
              unsigned int const totalBits);

void
ComputeGOPFrames(int            const whichGOP, 
                 unsigned int * const firstFrameP, 
                 unsigned int * const lastFrameP, 
                 unsigned int   const numFrames);

extern void	IncrementTCTime _ANSI_ARGS_((void));
void SetReferenceFrameType(const char * const type);

boolean
NonLocalRefFrame(int     const id);

void
ReadDecodedRefFrame(MpegFrame *  const frameP, 
                    unsigned int const frameNumber);

extern void	WriteDecodedFrame _ANSI_ARGS_((MpegFrame * const frame));
extern void	SetBitRateFileName _ANSI_ARGS_((char *fileName));
extern void	SetFrameRate _ANSI_ARGS_((void));


/*==================*
 * GLOBAL VARIABLES *
 *==================*/

extern MpegFrame *frameMemory[3];
extern int32	  tc_hrs, tc_min, tc_sec, tc_pict, tc_extra;
extern int	  totalFramesSent;
extern int	  gopSize;
extern char	 *framePattern;
extern int	  framePatternLen;
extern int32 qtable[];
extern int32 niqtable[];
extern int32 *customQtable;
extern int32 *customNIQtable;
extern int  aspectRatio;
extern int  frameRate;
extern int     frameRateRounded;
extern boolean    frameRateInteger;

