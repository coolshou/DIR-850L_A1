/*===========================================================================*
 * mpeg.c
 *
 *  Procedures to generate the MPEG sequence
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

#define _BSD_SOURCE   /* Make sure strdup() is in string.h */

#include "all.h"
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#ifdef MIPS
#include <sys/types.h>
#endif
#include <sys/stat.h>

#include "ppm.h"
#include "nstring.h"

#include "mtypes.h"
#include "frames.h"
#include "motion_search.h"
#include "prototypes.h"
#include "parallel.h"
#include "param.h"
#include "readframe.h"
#include "fsize.h"
#include "mheaders.h"
#include "rate.h"
#include "input.h"
#include "frametype.h"
#include "mpeg.h"


/*===========*
 *  VERSION  *
 *===========*/

#define VERSION "1.5b"


/*===========*
 * CONSTANTS *
 *===========*/

#define FPS_30  0x5   /* from MPEG standard sect. 2.4.3.2 */
#define ASPECT_1    0x1 /* aspect ratio, from MPEG standard sect. 2.4.3.2 */


/*==================*
 * STATIC VARIABLES *
 *==================*/

static unsigned int framesOutput;
static int      realStart, realEnd;
static int  currentGOP;
static int      timeMask;
static int      numI, numP, numB;
static boolean  frameCountsUnknown;


/*==================*
 * GLOBAL VARIABLES *   
 *==================*/

/* important -- don't initialize anything here */
/* must be re-initted anyway in GenMPEGStream */

extern int  IOtime;
extern boolean  resizeFrame;
extern int outputWidth, outputHeight;
int     gopSize = 100;  /* default */
int32       tc_hrs, tc_min, tc_sec, tc_pict, tc_extra;
int     totalFramesSent;
int     yuvWidth, yuvHeight;
int     realWidth, realHeight;
char        currentPath[MAXPATHLEN];
char        statFileName[256];
char        bitRateFileName[256];
time_t      timeStart, timeEnd;
FILE       *statFile;
FILE       *bitRateFile = NULL;
char       *framePattern;
int     framePatternLen;
int     referenceFrame;
int     frameRate = FPS_30;
int     frameRateRounded = 30;
boolean     frameRateInteger = TRUE;
int     aspectRatio = ASPECT_1;
extern char userDataFileName[];
extern int mult_seq_headers;

int32 bit_rate, buf_size;

/*===============================*
 * INTERNAL PROCEDURE prototypes *
 *===============================*/

static void ComputeDHMSTime _ANSI_ARGS_((int32 someTime, char *timeText));
static void OpenBitRateFile _ANSI_ARGS_((void));
static void CloseBitRateFile _ANSI_ARGS_((void));


static void
ShowRemainingTime(boolean const childProcess) {
/*----------------------------------------------------------------------------
   Print out an estimate of the time left to encode
-----------------------------------------------------------------------------*/

    if (childProcess) {
        /* nothing */;
    } else if ( numI + numP + numB == 0 ) {
        /* no time left */
    } else if ( timeMask != 0 ) {   
        /* haven't encoded all types yet */
    } else {
        static int  lastTime = 0;
        float   total;
        time_t  nowTime;
        float   secondsPerFrame;
        
        time(&nowTime);
        secondsPerFrame = (nowTime-timeStart)/(float)framesOutput;
        total = secondsPerFrame*(float)(numI+numP+numB);

        if ((quietTime >= 0) && (!realQuiet) && (!frameCountsUnknown) &&
            ((lastTime < (int)total) || ((lastTime-(int)total) >= quietTime) ||
             (lastTime == 0) || (quietTime == 0))) {
            if (total > 270.0)
                pm_message("ESTIMATED TIME OF COMPLETION:  %d minutes",
                           ((int)total+30)/60);
            else
                pm_message("ESTIMATED TIME OF COMPLETION:  %d seconds",
                           (int)total);
        }
        lastTime = (int)total;
    }
}



static void
initTCTime(unsigned int const firstFrameNumber) {

    unsigned int frameNumber;
    
    tc_hrs = 0; tc_min = 0; tc_sec = 0; tc_pict = 0; tc_extra = 0;
    for (frameNumber = 0; frameNumber < firstFrameNumber; ++frameNumber)
        IncrementTCTime();
}



/*===========================================================================*
 *
 * IncrementTCTime
 *
 *  increment the tc time by one second (and update min, hrs if necessary)
 *  also increments totalFramesSent
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    totalFramesSent, tc_pict, tc_sec, tc_min, tc_hrs, tc_extra
 *
 *===========================================================================*/
void
IncrementTCTime() {
    /* if fps = an integer, then tc_extra = 0 and is ignored

       otherwise, it is the number of extra 1/1001 frames we've passed by

       so far; for example, if fps = 24000/1001, then 24 frames = 24024/24000
       seconds = 1 second + 24/24000 seconds = 1 + 1/1000 seconds; similary,
       if fps = 30000/1001, then 30 frames = 30030/30000 = 1 + 1/1000 seconds
       and if fps = 60000/1001, then 60 frames = 1 + 1/1000 seconds

       if fps = 24000/1001, then 1/1000 seconds = 24/1001 frames
       if fps = 30000/1001, then 1/1000 seconds = 30/1001 frames
       if fps = 60000/1001, then 1/1000 seconds = 60/1001 frames     
     */

    totalFramesSent++;
    tc_pict++;
    if ( tc_pict >= frameRateRounded ) {
        tc_pict = 0;
        tc_sec++;
        if ( tc_sec == 60 ) {
            tc_sec = 0;
            tc_min++;
            if ( tc_min == 60 ) {
                tc_min = 0;
                tc_hrs++;
            }
        }
        if ( ! frameRateInteger ) {
            tc_extra += frameRateRounded;
            if ( tc_extra >= 1001 ) {   /* a frame's worth */
                tc_pict++;
                tc_extra -= 1001;
            }
        }
    }
}



static void
initializeRateControl(bool const wantUnderflowWarning,
                      bool const wantOverflowWarning) {
/*----------------------------------------------------------------------------
   Initialize rate control
-----------------------------------------------------------------------------*/
    int32 const bitstreamMode = getRateMode();

    if (bitstreamMode == FIXED_RATE) {
        initRateControl(wantUnderflowWarning, wantOverflowWarning);
        /*
          SetFrameRate();
        */
    }
}
    


/*===========================================================================*
 *
 * SetReferenceFrameType
 *
 *  set the reference frame type to be original or decoded
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    referenceFrame
 *
 *===========================================================================*/
void
SetReferenceFrameType(const char * const type) {

    if (strcmp(type, "ORIGINAL") == 0)
        referenceFrame = ORIGINAL_FRAME;
    else if ( strcmp(type, "DECODED") == 0 )
        referenceFrame = DECODED_FRAME;
    else
        pm_error("INTERNAL ERROR: Illegal reference frame type: '%s'", type);
}



void
SetBitRateFileName(fileName)
    char *fileName;
{
    strcpy(bitRateFileName, fileName);
}




static void
finishFrameOutput(MpegFrame * const frameP,
                  BitBucket * const bbP,
                  boolean     const separateFiles,
                  int         const referenceFrame,
                  boolean     const childProcess,
                  boolean     const remoteIO) {

    if ((referenceFrame == DECODED_FRAME) && 
        childProcess && NonLocalRefFrame(frameP->id)) {
        if (remoteIO)
            SendDecodedFrame(frameP);
        else
            WriteDecodedFrame(frameP);
            
        NotifyDecodeServerReady(frameP->id);
    }
    
    if (separateFiles) {
        if (remoteIO)
            SendRemoteFrame(frameP->id, bbP);
        else {
            Bitio_Flush(bbP);
            Bitio_Close(bbP);
        }
    }
}

    


static void
outputIFrame(MpegFrame * const frameP,
             BitBucket * const bb,
             int         const realStart,
             int         const realEnd,
             MpegFrame * const pastRefFrameP,
             boolean     const separateFiles) {
      
    /* only start a new GOP with I */
    /* don't start GOP if only doing frames */
    if ((!separateFiles) && (currentGOP >= gopSize)) {
        boolean const closed = 
            (totalFramesSent == frameP->id || pastRefFrameP == NULL);

        static int num_gop = 0;
    
        /* first, check to see if closed GOP */
    
        /* new GOP */
        if (num_gop != 0 && mult_seq_headers && 
            num_gop % mult_seq_headers == 0) {
            if (!realQuiet) {
                fprintf(stdout, 
                        "Creating new Sequence before GOP %d\n", num_gop);
                fflush(stdout);
            }
      
            Mhead_GenSequenceHeader(
                bb, Fsize_x, Fsize_y,
                /* pratio */    aspectRatio,
                /* pict_rate */ frameRate, /* bit_rate */ bit_rate,
                /* buf_size */  buf_size,  /* c_param_flag */ 1,
                /* iq_matrix */ customQtable, 
                /* niq_matrix */ customNIQtable,
                /* ext_data */ NULL,  /* ext_data_size */ 0,
                /* user_data */ NULL, /* user_data_size */ 0);
        }
    
        if (!realQuiet)
            pm_message("Creating new GOP (closed = %s) before frame %d\n",
                       closed ? "YES" : "NO", frameP->id);
    
        ++num_gop;
        Mhead_GenGOPHeader(bb,  /* drop_frame_flag */ 0,
                           tc_hrs, tc_min, tc_sec, tc_pict,
                           closed, /* broken_link */ 0,
                           /* ext_data */ NULL, /* ext_data_size */ 0,
                           /* user_data */ NULL, /* user_data_size */ 0);
        currentGOP -= gopSize;
        if (pastRefFrameP == NULL)
            SetGOPStartTime(0);
        else
            SetGOPStartTime(pastRefFrameP->id+1);
    }
      
    if ((frameP->id >= realStart) && (frameP->id <= realEnd))
        GenIFrame(bb, frameP);
      
    numI--;
    timeMask &= 0x6;
      
    currentGOP++;
    IncrementTCTime();
}



static void
outputPFrame(MpegFrame * const frameP,
             BitBucket * const bbP,
             int         const realStart,
             int         const realEnd,
             MpegFrame * const pastRefFrameP) {

    if ((frameP->id >= realStart) && (frameP->id <= realEnd))
        GenPFrame(bbP, frameP, pastRefFrameP);

    numP--;
    timeMask &= 0x5;
    
    currentGOP++;
    IncrementTCTime();
}



static BitBucket *
bitioNew(const char * const outputFileName,
         unsigned int const frameNumber,
         boolean      const remoteIO) {

    BitBucket * bbP;

    if (remoteIO)
        bbP = Bitio_New(NULL);
    else {
        const char * fileName;

        asprintfN(&fileName, "%s.frame.%d", outputFileName, frameNumber);

        bbP = Bitio_New_Filename(fileName);

        strfree(fileName);
    }
    return bbP;
}



static void
getBFrame(int                  const frameNum,
          struct inputSource * const inputSourceP,
          MpegFrame *          const pastRefFrameP,
          boolean              const childProcess,
          boolean              const remoteIO,
          MpegFrame **         const bFramePP,
          int *                const IOtimeP,
          unsigned int *       const framesReadP) {
/*----------------------------------------------------------------------------
   Get Frame 'frameNum', which is a B frame related to previous reference
   frame 'pastRefFrameP'.  Return it as *bFramePP.

   We have various ways of getting the frame, corresponding to the
   multitude of modes in which Ppmtompeg works.
-----------------------------------------------------------------------------*/
    if (!inputSourceP->stdinUsed) {
        time_t tempTimeStart, tempTimeEnd;
        MpegFrame * bFrameP;
        bool endOfStream;

        bFrameP = Frame_New(frameNum, 'b');

        time(&tempTimeStart);

        ReadNthFrame(inputSourceP, frameNum, remoteIO, childProcess,
                     separateConversion, slaveConversion, inputConversion,
                     bFrameP, &endOfStream);

        assert(!endOfStream);  /* Because it's not a stream */

        time(&tempTimeEnd);
        *IOtimeP += (tempTimeEnd-tempTimeStart);

        ++(*framesReadP);
        *bFramePP = bFrameP;
    } else {
        /* As the frame input is serial, we can't read the B frame now.
           Rather, Caller has already read it and chained it to 
           the previous reference frame.  So we get that copy now.
        */
        *bFramePP = pastRefFrameP->next;
        pastRefFrameP->next = (*bFramePP)->next;  /* unlink from list */
    }
}



static void
processBFrames(MpegFrame *          const pastRefFrameP,
               MpegFrame *          const futureRefFrameP,
               int                  const realStart,
               int                  const realEnd,
               struct inputSource * const inputSourceP,
               boolean              const remoteIo,
               boolean              const childProcess,
               int *                const IOtimeP,
               BitBucket *          const wholeStreamBbP,
               const char *         const outputFileName,
               unsigned int *       const framesReadP,
               unsigned int *       const framesOutputP,
               int *                const currentGopP) {
/*----------------------------------------------------------------------------
   Process the B frames that go between 'pastRefFrameP' and
   'futureRefFrame' in the movie (but go after 'futureRefFrameP' in the
   MPEG stream, so reader doesn't have to read ahead).

   Remember that a B frame is one which is described by data in the
   MPEG stream that describes the frame with respect to a frame somewhere
   before it, and a frame somewhere after it (i.e. reference frames).

   But do only those B frames whose frame numbers are within the range
   'realStart' through 'realEnd'.
-----------------------------------------------------------------------------*/
    boolean const separateFiles = (wholeStreamBbP == NULL);
    unsigned int const firstBFrameNum = pastRefFrameP->id + 1;

    int frameNum;

    assert(pastRefFrameP != NULL);
    assert(futureRefFrameP != NULL);
    
    for (frameNum = MAX(realStart, firstBFrameNum); 
         frameNum < MIN(realEnd, futureRefFrameP->id); 
         ++frameNum) {

        MpegFrame * bFrame;
        BitBucket * bbP;

        getBFrame(frameNum, inputSourceP, pastRefFrameP, childProcess, 
                  remoteIO,
                  &bFrame, IOtimeP, framesReadP);

        if (separateFiles)
            bbP = bitioNew(outputFileName, bFrame->id, remoteIO);
        else
            bbP = wholeStreamBbP;

        GenBFrame(bbP, bFrame, pastRefFrameP, futureRefFrameP);
        ++(*framesOutputP);

        if (separateFiles) {
            if (remoteIO)
                SendRemoteFrame(bFrame->id, bbP);
            else {
                Bitio_Flush(bbP);
                Bitio_Close(bbP);
            }
        }

        /* free this B frame right away */
        Frame_Free(bFrame);

        numB--;
        timeMask &= 0x3;
        ShowRemainingTime(childProcess);

        ++(*currentGopP);
        IncrementTCTime();
    }
}



static void
processRefFrame(MpegFrame *    const frameP, 
                BitBucket *    const bb_arg,
                int            const realStart,
                int            const realEnd,
                MpegFrame *    const pastRefFrameP,
                boolean        const childProcess,
                const char *   const outputFileName,
                unsigned int * const framesReadP,
                unsigned int * const framesOutputP) {
/*----------------------------------------------------------------------------
   Process an I or P frame.  Encode and output it.

   But only if its frame number is within the range 'realStart'
   through 'realEnd'.
-----------------------------------------------------------------------------*/
    if (frameP->id >= realStart && frameP->id <= realEnd) {
        boolean separateFiles;
        BitBucket * bb;
  
        separateFiles = (bb_arg == NULL);
  
        if ( separateFiles )
            bb = bitioNew(outputFileName, frameP->id, remoteIO);
        else
            bb = bb_arg;
  
        /* first, output this reference frame */
        switch (frameP->type) {
        case TYPE_IFRAME:
            outputIFrame(frameP, bb, realStart, realEnd, pastRefFrameP, 
                         separateFiles);
            break;
        case TYPE_PFRAME:
            outputPFrame(frameP, bb, realStart, realEnd, pastRefFrameP);
            ShowRemainingTime(childProcess);
            break;
        default:
            pm_error("INTERNAL ERROR: non-reference frame passed to "
                     "ProcessRefFrame()");
        }  
        
        ++(*framesOutputP);
        
        finishFrameOutput(frameP, bb, separateFiles, referenceFrame,
                          childProcess, remoteIO);
    }
}



static void
countFrames(unsigned int const firstFrame,
            unsigned int const lastFrame,
            boolean      const stdinUsed,
            int *        const numIP,
            int *        const numPP,
            int *        const numBP,
            int *        const timeMaskP,
            boolean *    const frameCountsUnknownP) {
/*----------------------------------------------------------------------------
  Count number of I, P, and B frames
-----------------------------------------------------------------------------*/
    unsigned int numI, numP, numB;
    unsigned int timeMask;
            
    numI = 0; numP = 0; numB = 0;
    timeMask = 0;
    if (stdinUsed) {
        numI = numP = numB = MAXINT/4;
        *frameCountsUnknownP = TRUE;
    } else {
        unsigned int i;
        for (i = firstFrame; i <= lastFrame; ++i) {
            char const frameType = FType_Type(i);
            switch(frameType) {
            case 'i':        numI++;            timeMask |= 0x1;    break;
            case 'p':        numP++;            timeMask |= 0x2;    break;
            case 'b':        numB++;            timeMask |= 0x4;    break;
            }
        }
        *frameCountsUnknownP = FALSE;
    }

    *numIP     = numI;
    *numPP     = numP;
    *numBP     = numB;
    *timeMaskP = timeMask;
    *frameCountsUnknownP = frameCountsUnknown;
}



static void
readAndSaveFrame(struct inputSource * const inputSourceP,
                 unsigned int         const frameNumber,
                 char                 const frameType,
                 const char *         const inputConversion,
                 MpegFrame *          const pastRefFrameP,
                 unsigned int *       const framesReadP,
                 int *                const ioTimeP,
                 bool *               const endOfStreamP) {
/*----------------------------------------------------------------------------
   Read the next frame from Standard Input and add it to the linked list
   at *pastRefFrameP.  Assume it is Frame Number 'frameNumber' and is of
   type 'frameType'.

   Increment *framesReadP.
   
   Add the time it took to read it, in seconds, to *iotimeP.

   Iff we can't read because we hit end of file, return
   *endOfStreamP == TRUE and *framesReadP and *iotimeP untouched.
-----------------------------------------------------------------------------*/
    /* This really should be part of ReadNthFrame.  The frame should be chained
       to the input object, not the past reference frame.
    */
       
    MpegFrame * p;
    MpegFrame * frameP;
    time_t ioTimeStart, ioTimeEnd;
    
    time(&ioTimeStart);

    frameP = Frame_New(frameNumber, frameType);
    ReadFrame(frameP, inputSourceP, frameNumber, inputConversion,
              endOfStreamP);

    if (*endOfStreamP)
        Frame_Free(frameP);
    else {
        ++(*framesReadP);
    
        time(&ioTimeEnd);
        *ioTimeP += (ioTimeEnd - ioTimeStart);

        /* Add the B frame to the end of the queue of B-frames 
           for later encoding.
        */
        assert(pastRefFrameP != NULL);
        
        p = pastRefFrameP;
        while (p->next != NULL)
            p = p->next;
        p->next = frameP;
    }
}



static void
doFirstFrameStuff(enum frameContext const context,
                  const char *      const userDataFileName,
                  BitBucket *       const bb,
                  int               const fsize_x,
                  int               const fsize_y,
                  int               const aspectRatio,
                  int               const frameRate,
                  int32             const qtable[],
                  int32             const niqtable[],
                  unsigned int *    const inputFrameBitsP) {
/*----------------------------------------------------------------------------
   Do stuff we have to do after reading the first frame in a sequence
   of frames requested of GenMPEGStream().
-----------------------------------------------------------------------------*/
    *inputFrameBitsP = 24*Fsize_x*Fsize_y;
    SetBlocksPerSlice();
          
    if (context == CONTEXT_WHOLESTREAM) {
        int32 const bitstreamMode = getRateMode();
        char * userData;
        unsigned int userDataSize;

        assert(bb != NULL);

        DBG_PRINT(("Generating sequence header\n"));
        if (bitstreamMode == FIXED_RATE) {
            bit_rate = getBitRate();
            buf_size = getBufferSize();
        } else {
            bit_rate = -1;
            buf_size = -1;
        }
        
        if (strlen(userDataFileName) != 0) {
            struct stat statbuf;
            FILE *fp;
          
            stat(userDataFileName,&statbuf);
            userDataSize = statbuf.st_size;
            userData = malloc(userDataSize);
            fp = fopen(userDataFileName,"rb");
            if (fp == NULL) {
                pm_message("Could not open userdata file '%s'.",
                           userDataFileName);
                userData = NULL;
                userDataSize = 0;
            } else {
                size_t bytesRead;

                bytesRead = fread(userData,1,userDataSize,fp);
                if (bytesRead != userDataSize) {
                    pm_message("Could not read %d bytes from "
                               "userdata file '%s'.",
                               userDataSize,userDataFileName);
                    userData = NULL;
                    userDataSize = 0;
                }
            }
        } else { /* Put in our UserData Header */
            const char * userDataString;
            time_t now;
                    
            time(&now);
            asprintfN(&userDataString,"MPEG stream encoded by UCB Encoder "
                      "(mpeg_encode) v%s on %s.",
                      VERSION, ctime(&now));
            userData = strdup(userDataString);
            userDataSize = strlen(userData);
            strfree(userDataString);
        }
        Mhead_GenSequenceHeader(bb, Fsize_x, Fsize_y,
                                /* pratio */ aspectRatio,
                                /* pict_rate */ frameRate, 
                                /* bit_rate */ bit_rate,
                                /* buf_size */ buf_size,
                                /*c_param_flag */ 1,
                                /* iq_matrix */ qtable, 
                                /* niq_matrix */ niqtable,
                                /* ext_data */ NULL,
                                /* ext_data_size */ 0,
                                /* user_data */ (uint8*) userData,
                                /* user_data_size */ userDataSize);
    }
}



static void
getPreviousFrame(unsigned int         const frameStart,
                 int                  const referenceFrame,
                 struct inputSource * const inputSourceP,
                 boolean              const childProcess,
                 const char *         const slaveConversion,
                 const char *         const inputConversion,
                 MpegFrame **         const framePP,
                 unsigned int *       const framesReadP,
                 int *                const ioTimeP) {

    /* This needs to be modularized.  It shouldn't issue messages about
       encoding GOPs and B frames, since it knows nothing about those.
       It should work for Standard Input too, through a generic Standard
       Input reader that buffers stuff for backward reading.
    */

    MpegFrame * frameP;
    time_t ioTimeStart, ioTimeEnd;

    /* can't find the previous frame interactively */
    if (inputSourceP->stdinUsed)
        pm_error("Cannot encode GOP from stdin when "
                 "first frame is a B-frame.");

    if (frameStart < 1)
        pm_error("Cannot encode GOP when first frame is a B-frame "
                 "and is not preceded by anything.");

    /* need to load in previous frame; call it an I frame */
    frameP = Frame_New(frameStart-1, 'i');

    time(&ioTimeStart);

    if ((referenceFrame == DECODED_FRAME) && childProcess) {
        WaitForDecodedFrame(frameStart);

        if (remoteIO)
            GetRemoteDecodedRefFrame(frameP, frameStart - 1);
        else
            ReadDecodedRefFrame(frameP, frameStart - 1);
    } else {
        bool endOfStream;
        ReadNthFrame(inputSourceP, frameStart - 1, remoteIO, childProcess,
                     separateConversion, slaveConversion, inputConversion,
                     frameP, &endOfStream);
        assert(!endOfStream);  /* Because Stdin causes failure above */
    }            
    ++(*framesReadP);
    
    time(&ioTimeEnd);
    *ioTimeP += (ioTimeEnd-ioTimeStart);

    *framePP = frameP;
}



static void
computeFrameRange(unsigned int         const frameStart,
                  unsigned int         const frameEnd,
                  enum frameContext    const context, 
                  struct inputSource * const inputSourceP,
                  unsigned int *       const firstFrameP,
                  unsigned int *       const lastFrameP) {

    switch (context) {
    case CONTEXT_GOP:
        *firstFrameP = frameStart;
        *lastFrameP  = frameEnd;
        break;
    case CONTEXT_JUSTFRAMES: {
        /* if first frame is P or B, need to read in P or I frame before it */
        if (FType_Type(frameStart) != 'i') {
            /* can't find the previous frame interactively */
            if (inputSourceP->stdinUsed)
                pm_error("Cannot encode frames from Standard Input "
                         "when first frame is not an I-frame.");

            *firstFrameP = FType_PastRef(frameStart);
        } else
            *firstFrameP = frameStart;

        /* if last frame is B, need to read in P or I frame after it */
        if ((FType_Type(frameEnd) == 'b') && 
            (frameEnd != inputSourceP->numInputFiles-1)) {
            /* can't find the next reference frame interactively */
            if (inputSourceP->stdinUsed)
                pm_error("Cannot encode frames from Standard Input "
                         "when last frame is a B-frame.");
            
            *lastFrameP = FType_FutureRef(frameEnd);
        } else
            *lastFrameP = frameEnd;
    }
    break;
    case CONTEXT_WHOLESTREAM:
        *firstFrameP = frameStart;
        *lastFrameP  = frameEnd;
    }
}



static void
getFrame(MpegFrame **         const framePP,
         struct inputSource * const inputSourceP,
         unsigned int         const frameNumber,
         char                 const frameType,
         unsigned int         const realStart,
         unsigned int         const realEnd,
         int                  const referenceFrame,
         boolean              const childProcess,
         boolean              const remoteIo,
         boolean              const separateConversion,
         const char *         const slaveConversion,
         const char *         const inputConversion,
         unsigned int *       const framesReadP,
         int *                const ioTimeP) {
/*----------------------------------------------------------------------------
   Get frame with number 'frameNumber' as *frameP.

   Increment *framesReadP.

   Add to *ioTimeP the time in seconds we spent reading it.

   Iff we fail to get the frame because the stream ends, return
   *frameP == NULL, don't increment *framesReadP, and leave
   *ioTimeP unchanged.
-----------------------------------------------------------------------------*/
    time_t ioTimeStart, ioTimeEnd;
    MpegFrame * frameP;
    bool endOfStream;
    
    time(&ioTimeStart);

    frameP = Frame_New(frameNumber, frameType);
            
    if ((referenceFrame == DECODED_FRAME) &&
        ((frameNumber < realStart) || (frameNumber > realEnd)) ) {
        WaitForDecodedFrame(frameNumber);

        if (remoteIo)
            GetRemoteDecodedRefFrame(frameP, frameNumber);
        else
            ReadDecodedRefFrame(frameP, frameNumber);

        /* I don't know what this block of code does, so I don't know
           what endOfStream should be.  Here's a guess:
        */
        endOfStream = FALSE;
    } else
        ReadNthFrame(inputSourceP, frameNumber, remoteIO, childProcess,
                     separateConversion, slaveConversion, inputConversion,
                     frameP, &endOfStream);
    
    if (endOfStream) {
        Frame_Free(frameP);
        *framePP = NULL;
    } else {
        ++(*framesReadP);
            
        time(&ioTimeEnd);
        *ioTimeP += (ioTimeEnd - ioTimeStart);

        *framePP = frameP;
    }
}



static void
handleBitRate(unsigned int const realEnd,
              unsigned int const numBits,
              boolean      const childProcess,
              boolean      const showBitRatePerFrame) {

    extern void PrintItoIBitRate (int numBits, int frameNum);

    if (FType_Type(realEnd) != 'i')
        PrintItoIBitRate(numBits, realEnd+1);

    if ((!childProcess) && showBitRatePerFrame)
        CloseBitRateFile();
}



static void
doAFrame(unsigned int         const frameNumber,
         struct inputSource * const inputSourceP,
         enum frameContext    const context, 
         unsigned int         const frameStart, 
         unsigned int         const frameEnd, 
         unsigned int         const realStart,
         unsigned int         const realEnd,
         bool                 const childProcess,
         const char *         const outputFileName,
         MpegFrame *          const pastRefFrameP,
         MpegFrame **         const newPastRefFramePP,
         unsigned int *       const framesReadP,
         unsigned int *       const framesOutputP,
         bool *               const firstFrameDoneP,
         BitBucket *          const bbP,
         unsigned int *       const inputFrameBitsP,
         bool *               const endOfStreamP) {
/*----------------------------------------------------------------------------
   *endOfStreamP returned means we were unable to do a frame because
   the input stream has ended.  In that case, none of the other outputs
   are valid.
-----------------------------------------------------------------------------*/
    char const frameType = FType_Type(frameNumber);
    
    *endOfStreamP = FALSE;  /* initial assumption */

    if (frameType == 'b') {
        /* We'll process this non-reference frame later.  If reading
           from stdin, we read it now and save it.  Otherwise, we can
           just read it later.
        */
        *newPastRefFramePP = pastRefFrameP;
        if (inputSourceP->stdinUsed) 
            readAndSaveFrame(inputSourceP,
                             frameNumber, frameType, inputConversion,
                             pastRefFrameP, framesReadP, &IOtime,
                             endOfStreamP);
    } else {
        MpegFrame * frameP;
        
        getFrame(&frameP, inputSourceP, frameNumber, frameType,
                 realStart, realEnd, referenceFrame, childProcess,
                 remoteIO,
                 separateConversion, slaveConversion, inputConversion,
                 framesReadP, &IOtime);
        
        if (frameP) {
            *endOfStreamP = FALSE;

            if (!*firstFrameDoneP) {
                doFirstFrameStuff(context, userDataFileName,
                                  bbP, Fsize_x, Fsize_y, aspectRatio,
                                  frameRate, qtable, niqtable, 
                                  inputFrameBitsP);
            
                *firstFrameDoneP = TRUE;
            }
            processRefFrame(frameP, bbP, frameStart, frameEnd,
                            pastRefFrameP, childProcess, outputFileName, 
                            framesReadP, framesOutputP);
                
            if (pastRefFrameP) {
                processBFrames(pastRefFrameP, frameP, realStart, realEnd,
                               inputSourceP, remoteIO, childProcess, 
                               &IOtime, bbP, outputFileName,
                               framesReadP, framesOutputP, &currentGOP);
            }
            if (pastRefFrameP != NULL)
                Frame_Free(pastRefFrameP);
        
            *newPastRefFramePP = frameP;
        } else
            *endOfStreamP = TRUE;
    }
}



void
GenMPEGStream(struct inputSource * const inputSourceP,
              enum frameContext    const context, 
              unsigned int         const frameStart, 
              unsigned int         const frameEnd, 
              int32                const qtable[], 
              int32                const niqtable[], 
              bool                 const childProcess,
              FILE *               const ofP, 
              const char *         const outputFileName,
              bool                 const wantVbvUnderflowWarning,
              bool                 const wantVbvOverflowWarning,
              unsigned int *       const inputFrameBitsP,
              unsigned int *       const totalBitsP) {
/*----------------------------------------------------------------------------
   Encode a bunch of frames into an MPEG sequence stream or a part thereof.

   'context' tells what in addition to the frames themselves must go into
   the stream:

      CONTEXT_JUSTFRAMES:  Nothing but the indicated frames
      CONTEXT_GOP:         GOP header/trailer stuff to make a single GOP
                           that contains the indicated frames
      CONTEXT_WHOLESTREAM: A whole stream consisting of the indicated
                           frames -- a sequence of whole GOPS, with stream
                           header/trailer stuff as well.

   'frameStart' and 'frameEnd' are the numbers of the first and last
   frames we are to encode, except that if the input source is a stream,
   we stop where the stream ends if that is before 'frameEnd'.

-----------------------------------------------------------------------------*/
    BitBucket * bbP;
    unsigned int frameNumber;
    bool endOfStream;
    bool firstFrameDone;
    int numBits;
    unsigned int firstFrame, lastFrame;
    /* Frame numbers of the first and last frames we look at.  This
       could be more than the the frames we actually encode because
       we may need context (i.e. to encode a B frame, we need the subsequent
       I or P frame).
    */
    unsigned int framesRead;
        /* Number of frames we have read; for statistical purposes */
    MpegFrame * pastRefFrameP;
        /* The frame that will be the past reference frame for the next
           B or P frame that we put into the stream
        */
    if (frameEnd + 1 > inputSourceP->numInputFiles)
        pm_error("Last frame (number %u) is beyond the end of the stream "
                 "(%u frames)", frameEnd, inputSourceP->numInputFiles);

    if (context == CONTEXT_WHOLESTREAM &&
        !inputSourceP->stdinUsed && 
        FType_Type(inputSourceP->numInputFiles-1) == 'b')
        pm_message("WARNING:  "
                   "One or more B-frames at end will not be encoded.  "
                   "See FORCE_ENCODE_LAST_FRAME parameter file statement.");

    time(&timeStart);

    framesRead = 0;

    ResetIFrameStats();
    ResetPFrameStats();
    ResetBFrameStats();

    Fsize_Reset();

    framesOutput = 0;

    if (childProcess && separateConversion)
        SetFileType(slaveConversion);
    else
        SetFileType(inputConversion);

    realStart = frameStart;
    realEnd   = frameEnd;

    computeFrameRange(frameStart, frameEnd, context, inputSourceP,
                      &firstFrame, &lastFrame);

    if (context == CONTEXT_GOP && FType_Type(frameStart) == 'b')
        getPreviousFrame(frameStart, referenceFrame, inputSourceP,
                         childProcess, slaveConversion, inputConversion,
                         &pastRefFrameP, &framesRead, &IOtime);
    else
        pastRefFrameP = NULL;

    countFrames(firstFrame, lastFrame, inputSourceP->stdinUsed,
                &numI, &numP, &numB, &timeMask, &frameCountsUnknown);

    if (showBitRatePerFrame)
        OpenBitRateFile();  /* May modify showBitRatePerFrame */

    if (context == CONTEXT_WHOLESTREAM || context == CONTEXT_GOP)
        bbP = Bitio_New(ofP);
    else
        bbP = NULL;

    initTCTime(firstFrame);

    totalFramesSent = firstFrame;
    currentGOP = gopSize;        /* so first I-frame generates GOP Header */

    initializeRateControl(wantVbvUnderflowWarning, wantVbvOverflowWarning);

    firstFrameDone = FALSE;
    for (frameNumber = firstFrame, endOfStream = FALSE;
         frameNumber <= lastFrame && !endOfStream;
         ++frameNumber) {

        doAFrame(frameNumber, inputSourceP, context, 
                 frameStart, frameEnd, realStart, realEnd,
                 childProcess, outputFileName,
                 pastRefFrameP, &pastRefFrameP,
                 &framesRead, &framesOutput, &firstFrameDone, bbP,
                 inputFrameBitsP, &endOfStream);
    }
    
    if (pastRefFrameP != NULL)
        Frame_Free(pastRefFrameP);
    
    /* SEQUENCE END CODE */
    if (context == CONTEXT_WHOLESTREAM)
        Mhead_GenSequenceEnder(bbP);
    
    if (context == CONTEXT_WHOLESTREAM)
        numBits = bbP->cumulativeBits;
    else {
        /* What should the correct value be?  Most likely 1.  "numBits" is
           used below, so we need to make sure it's properly initialized 
           to somthing (anything).  
        */
        numBits = 1;
    }

    if (context != CONTEXT_JUSTFRAMES) {
        Bitio_Flush(bbP);
        bbP = NULL;
        fclose(ofP);
    }
    handleBitRate(realEnd, numBits, childProcess, showBitRatePerFrame);

    *totalBitsP  = numBits;
}



/*===========================================================================*
 *
 * SetStatFileName
 *
 *  set the statistics file name
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    statFileName
 *
 *===========================================================================*/
void
SetStatFileName(const char * const fileName) {
    strcpy(statFileName, fileName);
}


/*===========================================================================*
 *
 * SetGOPSize
 *
 *  set the GOP size (frames per GOP)
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    gopSize
 *
 *===========================================================================*/
void
SetGOPSize(size)
    int size;
{
    gopSize = size;
}


/*===========================================================================*
 *
 * PrintStartStats
 *
 *  print out the starting statistics (stuff from the param file)
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
PrintStartStats(time_t               const startTime, 
                bool                 const specificFrames,
                unsigned int         const firstFrame, 
                unsigned int         const lastFrame,
                struct inputSource * const inputSourceP) {

    FILE *fpointer;
    int i;

    if (statFileName[0] == '\0') {
        statFile = NULL;
    } else {
        statFile = fopen(statFileName, "a");    /* open for appending */
        if (statFile == NULL) {
            fprintf(stderr, "ERROR:  Could not open stat file:  %s\n",
                    statFileName);
            fprintf(stderr, "        Sending statistics to stdout only.\n");
            fprintf(stderr, "\n\n");
        } else if (! realQuiet) {
            fprintf(stdout, "Appending statistics to file:  %s\n",
                    statFileName);
            fprintf(stdout, "\n\n");
        }
    }
    
    for (i = 0; i < 2; ++i) {
        if ( ( i == 0 ) && (! realQuiet) ) {
            fpointer = stdout;
        } else if ( statFile != NULL ) {
            fpointer = statFile;
        } else {
            continue;
        }

        fprintf(fpointer, "MPEG ENCODER STATS (%s)\n",VERSION);
        fprintf(fpointer, "------------------------\n");
        fprintf(fpointer, "TIME STARTED:  %s", ctime(&startTime));
        if (getenv("HOST") != NULL)
            fprintf(fpointer, "MACHINE:  %s\n", getenv("HOST"));
        else
            fprintf(fpointer, "MACHINE:  unknown\n");

        if (inputSourceP->stdinUsed)
            fprintf(fpointer, "INPUT:  stdin\n");
        else {
            const char * inputFileName;

            fprintf(fpointer, "INPUT FROM FILES:\n");

            GetNthInputFileName(inputSourceP, 0, &inputFileName);
            fprintf(fpointer, "FIRST FILE:  %s/%s\n", 
                    currentPath, inputFileName);
            strfree(inputFileName);
            GetNthInputFileName(inputSourceP, inputSourceP->numInputFiles-1, 
                                &inputFileName);
            fprintf(fpointer, "LAST FILE:  %s/%s\n", 
                    currentPath, inputFileName);
            strfree(inputFileName);
        }    
        fprintf(fpointer, "OUTPUT:  %s\n", outputFileName);

        if (resizeFrame)
            fprintf(fpointer, "RESIZED TO:  %dx%d\n",
                    outputWidth, outputHeight);
        fprintf(fpointer, "PATTERN:  %s\n", framePattern);
        fprintf(fpointer, "GOP_SIZE:  %d\n", gopSize);
        fprintf(fpointer, "SLICES PER FRAME:  %d\n", slicesPerFrame);
        if (searchRangeP==searchRangeB)
            fprintf(fpointer, "RANGE:  +/-%d\n", searchRangeP/2);
        else fprintf(fpointer, "RANGES:  +/-%d %d\n", 
                     searchRangeP/2,searchRangeB/2);
        fprintf(fpointer, "PIXEL SEARCH:  %s\n", 
                pixelFullSearch ? "FULL" : "HALF");
        fprintf(fpointer, "PSEARCH:  %s\n", PSearchName());
        fprintf(fpointer, "BSEARCH:  %s\n", BSearchName());
        fprintf(fpointer, "QSCALE:  %d %d %d\n", qscaleI, 
                GetPQScale(), GetBQScale());
        if (specificsOn) 
            fprintf(fpointer, "(Except as modified by Specifics file)\n");
        if ( referenceFrame == DECODED_FRAME ) {
            fprintf(fpointer, "REFERENCE FRAME:  DECODED\n");
        } else if ( referenceFrame == ORIGINAL_FRAME ) {
            fprintf(fpointer, "REFERENCE FRAME:  ORIGINAL\n");
        } else
            pm_error("Illegal referenceFrame!!!");

        /*  For new Rate control parameters */
        if (getRateMode() == FIXED_RATE) {
            fprintf(fpointer, "PICTURE RATE:  %d\n", frameRateRounded);
            if (getBitRate() != -1) {
                fprintf(fpointer, "\nBIT RATE:  %d\n", getBitRate());
            }
            if (getBufferSize() != -1) {
                fprintf(fpointer, "BUFFER SIZE:  %d\n", getBufferSize());
            }
        }
    }
    if (!realQuiet)
        fprintf(stdout, "\n\n");
}



boolean
NonLocalRefFrame(int const id) {
/*----------------------------------------------------------------------------
   Return TRUE if frame number 'id' might be referenced from a non-local
   process.  This is a conservative estimate.  We return FALSE iff there
   is no way based on the information we have that the frame could be
   referenced by a non-local process.
-----------------------------------------------------------------------------*/
    boolean retval;

    int const lastIPid = FType_PastRef(id);
    
    /* might be accessed by B-frame */
    
    if (lastIPid+1 < realStart)
        retval = TRUE;
    else {
        unsigned int const nextIPid = FType_FutureRef(id);
        
        /* if B-frame is out of range, then current frame can be
           ref'd by it 
        */
        
        /* might be accessed by B-frame */
        if (nextIPid > realEnd+1)
            retval = TRUE;
        
        /* might be accessed by P-frame */
        if ((nextIPid > realEnd) && (FType_Type(nextIPid) == 'p'))
            retval = TRUE;
    }
    return retval;
}


 
/*===========================================================================*
 *
 * SetFrameRate
 *
 *  sets global frame rate variables.  value passed is MPEG frame rate code.
 *
 * RETURNS: TRUE or FALSE
 *
 * SIDE EFFECTS:    frameRateRounded, frameRateInteger
 *
 *===========================================================================*/
void
SetFrameRate()
{
    switch(frameRate) {
    case 1:
        frameRateRounded = 24;
        frameRateInteger = FALSE;
        break;
    case 2:
        frameRateRounded = 24;
        frameRateInteger = TRUE;
        break;
    case 3:
        frameRateRounded = 25;
        frameRateInteger = TRUE;
        break;
    case 4:
        frameRateRounded = 30;
        frameRateInteger = FALSE;
        break;
    case 5:
        frameRateRounded = 30;
        frameRateInteger = TRUE;
        break;
    case 6:
        frameRateRounded = 50;
        frameRateInteger = TRUE;
        break;
    case 7:
        frameRateRounded = 60;
        frameRateInteger = FALSE;
        break;
    case 8:
        frameRateRounded = 60;
        frameRateInteger = TRUE;
        break;
    }
    printf("frame rate(%d) set to %d\n", frameRate, frameRateRounded);
}


/*=====================*
 * INTERNAL PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 * ComputeDHMSTime
 *
 *  turn some number of seconds (someTime) into a string which
 *  summarizes that time according to scale (days, hours, minutes, or
 *  seconds)
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
ComputeDHMSTime(someTime, timeText)
    int32 someTime;
    char *timeText;
{
    int     days, hours, mins, secs;

    days = someTime / (24*60*60);
    someTime -= days*24*60*60;
    hours = someTime / (60*60);
    someTime -= hours*60*60;
    mins = someTime / 60;
    secs = someTime - mins*60;

    if ( days > 0 ) {
        sprintf(timeText, "Total time:  %d days and %d hours", days, hours);
    } else if ( hours > 0 ) {
        sprintf(timeText, "Total time:  %d hours and %d minutes", hours, mins);
    } else if ( mins > 0 ) {
        sprintf(timeText, "Total time:  %d minutes and %d seconds", mins, secs);
    } else {
    sprintf(timeText, "Total time:  %d seconds", secs);
    }
}



void
ComputeGOPFrames(int            const whichGOP, 
                 unsigned int * const firstFrameP, 
                 unsigned int * const lastFrameP, 
                 unsigned int   const numFrames) {
/*----------------------------------------------------------------------------
   Figure out which frames are in GOP number 'whichGOP'.
-----------------------------------------------------------------------------*/
    unsigned int passedB;
    unsigned int currGOP;
    unsigned int gopNum;
    unsigned int frameNum;
    unsigned int firstFrame, lastFrame;
    bool foundGop;

    /* calculate first, last frames of whichGOP GOP */

    gopNum = 0;
    frameNum = 0;
    passedB = 0;
    currGOP = 0;
    foundGop = FALSE;

    while (!foundGop) {
        if (frameNum >= numFrames)
            pm_error("There aren't that many GOPs!");

        if (gopNum == whichGOP) {
            foundGop = TRUE;
            firstFrame = frameNum;
        }           

        /* go past one gop */
        /* must go past at least one frame */
        do {
            currGOP += (1 + passedB);

            ++frameNum;

            passedB = 0;
            while ((frameNum < numFrames) && (FType_Type(frameNum) == 'b')) {
                ++frameNum;
                ++passedB;
            }
        } while ((frameNum < numFrames) && 
                 ((FType_Type(frameNum) != 'i') || (currGOP < gopSize)));

        currGOP -= gopSize;

        if (gopNum == whichGOP)
            lastFrame = (frameNum - passedB - 1);

        ++gopNum;
    }
    *firstFrameP = firstFrame;
    *lastFrameP  = lastFrame;
}



static void
doEndStats(FILE *       const fpointer,
           time_t       const startTime,
           time_t       const endTime,
           unsigned int const inputFrameBits,
           unsigned int const totalBits,
           float        const totalCPU) {

    int32 const diffTime = endTime - startTime;

    char    timeText[256];

    ComputeDHMSTime(diffTime, timeText);

    fprintf(fpointer, "TIME COMPLETED:  %s", ctime(&endTime));
    fprintf(fpointer, "%s\n\n", timeText);
        
    ShowIFrameSummary(inputFrameBits, totalBits, fpointer);
    ShowPFrameSummary(inputFrameBits, totalBits, fpointer);
    ShowBFrameSummary(inputFrameBits, totalBits, fpointer);

    fprintf(fpointer, "---------------------------------------------\n");
    fprintf(fpointer, "Total Compression:  %3d:1     (%9.4f bpp)\n",
            framesOutput*inputFrameBits/totalBits,
            24.0*(float)(totalBits)/(float)(framesOutput*inputFrameBits));
    if (diffTime > 0) {
        fprintf(fpointer, "Total Frames Per Sec Elapsed:  %f (%ld mps)\n",
                (float)framesOutput/(float)diffTime,
                (long)((float)framesOutput * 
                       (float)inputFrameBits /
                       (256.0*24.0*(float)diffTime)));
    } else {
        fprintf(fpointer, "Total Frames Per Sec Elapsed:  Infinite!\n");
    }
    if ( totalCPU == 0.0 ) {
        fprintf(fpointer, "CPU Time:  NONE!\n");
    } else {
        fprintf(fpointer, "Total Frames Per Sec CPU    :  %f (%ld mps)\n",
                (float)framesOutput/totalCPU,
                (long)((float)framesOutput *
                       (float)inputFrameBits/(256.0*24.0*totalCPU)));
    }
    fprintf(fpointer, "Total Output Bit Rate (%d fps):  %d bits/sec\n",
            frameRateRounded, frameRateRounded*totalBits/framesOutput);
    fprintf(fpointer, "MPEG file created in :  %s\n", outputFileName);
    fprintf(fpointer, "\n\n");
        
    if ( computeMVHist ) {
        ShowPMVHistogram(fpointer);
        ShowBBMVHistogram(fpointer);
        ShowBFMVHistogram(fpointer);
    }
}



/*===========================================================================*
 *
 * PrintEndStats
 *
 *  print end statistics (summary, time information)
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
PrintEndStats(time_t       const startTime,
              time_t       const endTime,
              unsigned int const inputFrameBits, 
              unsigned int const totalBits) {

    float   totalCPU;

    totalCPU = 0.0;
    totalCPU += IFrameTotalTime();
    totalCPU += PFrameTotalTime();
    totalCPU += BFrameTotalTime();

    if (!realQuiet) {
        fprintf(stdout, "\n\n");
        doEndStats(stdout, startTime, endTime, inputFrameBits,
                   totalBits, totalCPU);
    }
    
    if (statFile) {
        doEndStats(statFile, startTime, endTime, inputFrameBits,
                   totalBits, totalCPU);

        fclose(statFile);
    }
}



void
ReadDecodedRefFrame(MpegFrame *  const frameP, 
                    unsigned int const frameNumber) {

    FILE    *fpointer;
    char    fileName[256];
    int width, height;
    register int y;

    width = Fsize_x;
    height = Fsize_y;

    sprintf(fileName, "%s.decoded.%u", outputFileName, frameNumber);
    if (! realQuiet) {
        fprintf(stdout, "reading %s\n", fileName);
        fflush(stdout);
    }

    if ((fpointer = fopen(fileName, "rb")) == NULL) {
        sleep(1);
        if ((fpointer = fopen(fileName, "rb")) == NULL) {
            fprintf(stderr, "Cannot open %s\n", fileName);
            exit(1);
        }}

    Frame_AllocDecoded(frameP, TRUE);
    
    for ( y = 0; y < height; y++ ) {
        size_t bytesRead;

        bytesRead = fread(frameP->decoded_y[y], 1, width, fpointer);
        if (bytesRead != width)
            pm_error("Could not read enough bytes from '%s;", fileName);
    }
    
    for (y = 0; y < (height >> 1); y++) {           /* U */
        size_t const bytesToRead = width/2;
        size_t bytesRead;

        bytesRead = fread(frameP->decoded_cb[y], 1, bytesToRead, fpointer);
        if (bytesRead != bytesToRead)
            pm_message("Could not read enough bytes from '%s'", fileName);
    }
    
    for (y = 0; y < (height >> 1); y++) {           /* V */
        size_t const bytesToRead = width/2;
        size_t bytesRead;
        bytesRead = fread(frameP->decoded_cr[y], 1, bytesToRead, fpointer);
        if (bytesRead != bytesToRead)
            pm_message("Could not read enough bytes from '%s'", fileName);
    }
    fclose(fpointer);
}



static void
OpenBitRateFile() {
    bitRateFile = fopen(bitRateFileName, "w");
    if ( bitRateFile == NULL ) {
        pm_message("ERROR:  Could not open bit rate file:  '%s'", 
                   bitRateFileName);
        showBitRatePerFrame = FALSE;
    }
}



static void
CloseBitRateFile() {
    fclose(bitRateFile);
}
