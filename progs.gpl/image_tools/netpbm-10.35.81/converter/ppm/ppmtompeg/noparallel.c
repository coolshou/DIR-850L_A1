/*===========================================================================*
 *  noparallel.c
 *
 *  Would be procedures to make encoder to run in parallel -- except
 *  this machine doesn't have sockets, so we can only run sequentially
 *  so this file has dummy procedures which lets it compile
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

#include <time.h>

#include <pm.h>

#include "all.h"
#include "mtypes.h"
#include "parallel.h"
#include "frame.h"

/*==================*
 * GLOBAL VARIABLES *
 *==================*/

int parallelTestFrames = 10;
int parallelTimeChunks = 60;
const char *IOhostName;
int ioPortNumber;
int combinePortNumber;
int decodePortNumber;
boolean niceProcesses = FALSE;
boolean forceIalign = FALSE;
int     machineNumber = -1;
boolean remoteIO = FALSE;
boolean separateConversion;
time_t  IOtime = 0;


/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/

/*=================*
 * IO SERVER STUFF *
 *=================*/


void
IoServer(struct inputSource * const inputSourceP,
         const char *         const parallelHostName, 
         int                  const portNum) {

    pm_error("This version of Ppmtompeg cannot run an I/O server because "
             "it does not have socket capability.");
}



void
SetIOConvert(boolean const separate) {
    /* do nothing -- this may be called during non-parallel execution */
}



void
SetParallelPerfect(boolean const val) {
    /* do nothing -- this may be called during non-parallel execution */
}


void
SetRemoteShell(const char * const shell) {
    /* do nothing -- this may be called during non-parallel execution */
}



void
NoteFrameDone(int const frameStart,
              int const frameEnd) {
    fprintf(stdout, 
            "ERROR:  (NoteFrameDone) "
            "This machine can NOT run parallel version\n");
    exit(1);
}



/* SendRemoteFrame
 */
void
SendRemoteFrame(int         const frameNumber,
                BitBucket * const bb) {
    fprintf(stdout, "ERROR:  (SendRemoteFrame) "
            "This machine can NOT run parallel version\n");
    exit(1);
}



/* GetRemoteFrame
 */
void
GetRemoteFrame(MpegFrame * const frame,
               int         const frameNumber) {

    fprintf(stdout, "ERROR:  (GetRemoteFrame) "
            "This machine can NOT run parallel version\n");
    exit(1);
}



void
WaitForOutputFile(int const number) {
    fprintf(stdout, "ERROR:  (WaitForOutputFile) "
            "This machine can NOT run parallel version\n");
    exit(1);
}



/*=======================*
 * PARALLEL SERVER STUFF *
 *=======================*/


void
MasterServer(struct inputSource * const inputSourceP,
             const char *         const paramFileName, 
             const char *         const outputFileName) {

    pm_error("This version of Ppmtompeg cannot run a master server because "
             "it does not have socket capability.");
}



void
CombineServer(int          const numFrames, 
              const char * const masterHostName, 
              int          const masterPortNum,
              const char * const outputFileName) {

    pm_error("This version of Ppmtompeg cannot run combine server because "
             "it does not have socket capability.");
}



void
DecodeServer(int          const numInputFiles, 
             const char * const decodeFileName, 
             const char * const masterHostName, 
             int          const masterPortNum) {

    pm_error("This version of Ppmtompeg cannot run a decode server because "
             "it does not have socket capability.");
}



void
NotifyMasterDone(const char * const hostName, 
                 int          const portNum, 
                 int          const machineNumber, 
                 unsigned int const seconds,
                 boolean *    const moreWorkToDoP,
                 int *        const frameStartP,
                 int *        const frameEndP) {
    pm_error("This version of Ppmtompeg cannot run parallel mode because "
             "it does not have socket capability.");
}



void
NotifyDecodeServerReady(int const id) {
    pm_error("This version of Ppmtompeg cannot run parallel mode because "
             "it does not have socket capability.");
}



void
WaitForDecodedFrame(int const id) {
    pm_error("This version of Ppmtompeg cannot run parallel mode because "
             "it does not have socket capability.");
}



void
SendDecodedFrame(MpegFrame * const frame) {
    pm_error("This version of Ppmtompeg cannot run parallel mode because "
             "it does not have socket capability.");
}



void
GetRemoteDecodedRefFrame(MpegFrame * const frame,
                         int         const frameNumber) {
    pm_error("This version of Ppmtompeg cannot run parallel mode because "
             "it does not have socket capability.");
}
