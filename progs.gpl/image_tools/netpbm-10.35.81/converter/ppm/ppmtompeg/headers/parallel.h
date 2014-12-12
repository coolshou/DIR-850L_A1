/*===========================================================================*
 * parallel.h          
 *                     
 *  parallel encoding  
 *                     
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

#include "ansi.h"
#include "bitio.h"
#include "frame.h"


struct inputSource;

/*===============================*
 * EXTERNAL PROCEDURE prototypes *
 *===============================*/

void
MasterServer(struct inputSource * const inputSourceP,
             const char *         const paramFileName, 
             const char *         const outputFileName);

void
NotifyMasterDone(const char * const hostName, 
                 int          const portNum, 
                 int          const machineNumber, 
                 unsigned int const seconds, 
                 boolean *    const moreWorkToDoP,
                 int *        const frameStartP,
                 int *        const frameEndP);

void
IoServer(struct inputSource * const inputSourceP,
         const char *         const parallelHostName, 
         int                  const portNum);

void
CombineServer(int          const numInputFiles, 
              const char * const masterHostName, 
              int          const masterPortNum,
              const char*  const outputFileName);

void
DecodeServer(int          const numInputFiles, 
             const char * const decodeFileName, 
             const char * const parallelHostName, 
             int          const portNum);

void
WaitForOutputFile(int number);

void
GetRemoteFrame(MpegFrame * const frameP,
               int         const frameNumber);

void
SendRemoteFrame(int         const frameNumber,
                BitBucket * const bbP);

void
NoteFrameDone(int frameStart, int frameEnd);

void
SetIOConvert(boolean const separate);

void
SetRemoteShell(const char * const shell);

void 
NotifyDecodeServerReady(int const id);

void 
WaitForDecodedFrame(int id);

void 
SendDecodedFrame(MpegFrame * const frameP);

void 
GetRemoteDecodedRefFrame(MpegFrame * const frameP,
                         int         const frameNumber);

void 
SetParallelPerfect(boolean val);


/*==================*
 * GLOBAL VARIABLES *
 *==================*/

extern int parallelTestFrames;
extern int parallelTimeChunks;

extern const char *IOhostName;
extern int ioPortNumber;
extern int decodePortNumber;

extern boolean  ioServer;
extern boolean  niceProcesses;
extern boolean  forceIalign;
extern int    machineNumber;
extern boolean remoteIO;
extern boolean  separateConversion;







