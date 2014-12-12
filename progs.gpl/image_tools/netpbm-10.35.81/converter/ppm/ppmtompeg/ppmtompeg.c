/*===========================================================================*
 * main.c
 *
 * Main procedure
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

#define _BSD_SOURCE   /* Make sure strdup() is in string.h */

#include <assert.h>
#include <sys/utsname.h>

#include "all.h"
#include "mtypes.h"
#include "mpeg.h"
#include "motion_search.h"
#include "prototypes.h"
#include "param.h"
#include "parallel.h"
#include "readframe.h"
#include "combine.h"
#include "frames.h"
#include "jpeg.h"
#include "specifics.h"
#include "opts.h"
#include "frametype.h"
#include "input.h"
#include "gethostname.h"

#include "pm_c_util.h"
#include "ppm.h"
#include "nstring.h"

#include <time.h>

int main _ANSI_ARGS_((int argc, char **argv));


/*==================*
 * GLOBAL VARIABLES *
 *==================*/

boolean showBitRatePerFrame;
boolean frameSummary;

extern time_t IOtime;
int whichGOP = -1;
boolean ioServer = FALSE;
boolean outputServer = FALSE;
boolean decodeServer = FALSE;
int quietTime = 0;
boolean realQuiet = FALSE;
boolean noFrameSummaryOption = FALSE;
boolean debugSockets = FALSE;
boolean debugMachines = FALSE;
boolean bitRateInfoOption = FALSE;
boolean computeMVHist = FALSE;
int     baseFormat;
extern  boolean specificsOn;
extern  FrameSpecList *fsl;
boolean pureDCT=FALSE;
char    encoder_name[1024];
const char * hostname;


/*================================*
 * External PROCEDURE prototypes  *
 *================================*/

void init_idctref _ANSI_ARGS_((void));
void init_fdct _ANSI_ARGS_((void));


struct cmdlineInfo {
    bool         childProcess;
    int          function;
    const char * masterHostname;
    int          masterPortNumber;
    unsigned int outputFrames;
    int          maxMachines;
    const char * paramFileName;
    bool         specificFrames;
    unsigned int frameStart;
    unsigned int frameEnd;
};



static void
parseArgs(int     const argc,
          char ** const argv,
          struct cmdlineInfo * const cmdlineP) {

    int idx;

    if (argc-1 < 1)
        pm_error("You must specify at least one argument: the parameter "
                 "file name");

    cmdlineP->function = ENCODE_FRAMES;
    cmdlineP->childProcess = FALSE;  /* initial assumption */
    cmdlineP->outputFrames = 0;
    cmdlineP->maxMachines = MAXINT;
    cmdlineP->specificFrames = FALSE;
    
    /* parse the arguments */
    idx = 1;
    while ( idx < argc-1 ) {
        if ( argv[idx][0] != '-' )
            pm_error("argument '%s', which must be an option because "
                     "it is not the last argument, "
                     "does not start with '-'", argv[idx]);

        if ( strcmp(argv[idx], "-stat") == 0 ) {
            if ( idx+1 < argc-1 ) {
                SetStatFileName(argv[idx+1]);
                idx += 2;
            } else {
                pm_error("Invalid -stat option");
            }
        } else if ( strcmp(argv[idx], "-gop") == 0 ) {
            if ((cmdlineP->function != ENCODE_FRAMES) || 
                (cmdlineP->specificFrames))
                pm_error("Invalid -gop option");
            
            if ( idx+1 < argc-1 ) {
                whichGOP = atoi(argv[idx+1]);
                idx += 2;
            } else {
                pm_error("Invalid -gop option");
            }
        } else if ( strcmp(argv[idx], "-frames") == 0 ) {
            if ( (cmdlineP->function != ENCODE_FRAMES) || (whichGOP != -1) ) {
                pm_error("invalid -frames option");
            }

            if ( idx+2 < argc-1 ) {
                int const frameStart = atoi(argv[idx+1]);
                int const frameEnd = atoi(argv[idx+2]);

                if (frameStart > frameEnd)
                    pm_error("Start frame number %d is greater than end "
                             "frame number %d", frameStart, frameEnd);
                if (frameStart < 0)
                    pm_error("Start frame number %d is less than zero",
                             frameStart);

                cmdlineP->specificFrames = TRUE;
                cmdlineP->frameStart = frameStart;
                cmdlineP->frameEnd   = frameEnd;
                
                idx += 3;
            } else
                pm_error("-frames needs to be followed by two values");
        } else if (strcmp(argv[idx], "-combine_gops") == 0) {
            if ((cmdlineP->function != ENCODE_FRAMES) || (whichGOP != -1) || 
                (cmdlineP->specificFrames)) {
                pm_error("Invalid -combine_gops option");
            }

            cmdlineP->function = COMBINE_GOPS;
            idx++;
        } else if (strcmp(argv[idx], "-combine_frames") == 0) {
            if ((cmdlineP->function != ENCODE_FRAMES) || (whichGOP != -1) ||
                (cmdlineP->specificFrames))
                pm_error("Invalid -combine_frames option");

            cmdlineP->function = COMBINE_FRAMES;
            idx++;
        } else if ( strcmp(argv[idx], "-child") == 0 ) {
            if ( idx+7 < argc-1 ) {
                int combinePortNumber;
                    /* This used to be important information, when the child
                       notified the combine server.  Now the master notifies
                       the combine server after the child notifies the master
                       it is done.  So this value is unused.
                    */
                cmdlineP->masterHostname = argv[idx+1];
                cmdlineP->masterPortNumber = atoi(argv[idx+2]);
                ioPortNumber = atoi(argv[idx+3]);
                combinePortNumber = atoi(argv[idx+4]);
                decodePortNumber = atoi(argv[idx+5]);
                machineNumber = atoi(argv[idx+6]);
                remoteIO = atoi(argv[idx+7]);

                IOhostName = cmdlineP->masterHostname;
            } else
                pm_error("Not enough option values for -child option.  "
                         "Need 7.");

            cmdlineP->childProcess = TRUE;
            idx += 8;
        } else if ( strcmp(argv[idx], "-io_server") == 0 ) {
            if ( idx+2 < argc-1 ) {
                cmdlineP->masterHostname = argv[idx+1];
                cmdlineP->masterPortNumber = atoi(argv[idx+2]);
            } else {
                pm_error("Invalid -io_server option");
            }

            ioServer = TRUE;
            idx += 3;
        } else if ( strcmp(argv[idx], "-output_server") == 0 ) {
            if ( idx+3 < argc-1 ) {
                cmdlineP->masterHostname = argv[idx+1];
                cmdlineP->masterPortNumber = atoi(argv[idx+2]);
                cmdlineP->outputFrames = atoi(argv[idx+3]);
            } else {
                pm_error("-output_server option requires 3 option values.  "
                         "You specified %d", argc-1 - idx);
            }

            outputServer = TRUE;
            idx += 4;
        } else if ( strcmp(argv[idx], "-decode_server") == 0 ) {
            if ( idx+3 < argc-1 ) {
                cmdlineP->masterHostname = argv[idx+1];
                cmdlineP->masterPortNumber = atoi(argv[idx+2]);
                cmdlineP->outputFrames = atoi(argv[idx+3]);
            } else {
                pm_error("Invalid -decode_server option");
            }

            cmdlineP->function = COMBINE_FRAMES;
            decodeServer = TRUE;
            idx += 4;
        } else if ( strcmp(argv[idx], "-nice") == 0 ) {
            niceProcesses = TRUE;
            idx++;
        } else if ( strcmp(argv[idx], "-max_machines") == 0 ) {
            if ( idx+1 < argc-1 ) {
                cmdlineP->maxMachines = atoi(argv[idx+1]);
            } else {
                pm_error("Invalid -max_machines option");
            }

            idx += 2;
        } else if ( strcmp(argv[idx], "-quiet") == 0 ) {
            if ( idx+1 < argc-1 ) {
                quietTime = atoi(argv[idx+1]);
            } else {
                pm_error("Invalid -quiet option");
            }

            idx += 2;
        } else if ( strcmp(argv[idx], "-realquiet") == 0 ) {
            realQuiet = TRUE;
            idx++;
        } else if (( strcmp(argv[idx], "-float_dct") == 0 ) ||
                   ( strcmp(argv[idx], "-float-dct") == 0 )) {
            pureDCT = TRUE;
            init_idctref();
            init_fdct();
            idx++;
        } else if ( strcmp(argv[idx], "-no_frame_summary") == 0 ) {
            if ( idx < argc-1 ) {
                noFrameSummaryOption = TRUE;
            } else {
                pm_error("Invalid -no_frame_summary option");
            }

            idx++;
        } else if ( strcmp(argv[idx], "-snr") == 0 ) {
            printSNR = TRUE;
            idx++;
        } else if ( strcmp(argv[idx], "-mse") == 0 ) {
            printSNR =  printMSE = TRUE;
            idx++;
        } else if ( strcmp(argv[idx], "-debug_sockets") == 0 ) {
            debugSockets = TRUE;
            idx++;
        } else if ( strcmp(argv[idx], "-debug_machines") == 0 ) {
            debugMachines = TRUE;
            idx++;
        } else if ( strcmp(argv[idx], "-bit_rate_info") == 0 ) {
            if ( idx+1 < argc-1 ) {
                bitRateInfoOption = TRUE;
                SetBitRateFileName(argv[idx+1]);
                idx += 2;
            } else {
                pm_error("Invalid -bit_rate_info option");
            }
        } else if ( strcmp(argv[idx], "-mv_histogram") == 0 ) {
            computeMVHist = TRUE;
            idx++;
        } else {
            pm_error("Unrecognized option: '%s'", argv[idx]);
        }
    }

    cmdlineP->paramFileName = argv[argc-1];
}



static void
compileTests() {

    /* There was code here (assert() calls) that verified that uint16
       is 16 bits, etc.  It caused compiler warnings that said, "Of
       course it is!"  (actually, "statement has no effect").  There
       isn't enough chance that uint16, etc. are defined incorrectly
       to warrant those asserts.  2000.05.07
    */

    if ( (-8 >> 3) != -1 ) {
        fprintf(stderr, "ERROR:  Right shifts are NOT arithmetic!!!\n");
        fprintf(stderr, "Change >> to multiplies by powers of 2\n");
        exit(1);
    }
}



static void
announceJob(enum frameContext const context,
            bool              const childProcess,
            unsigned int      const frameStart,
            unsigned int      const frameEnd,
            const char *      const outputFileName) {

    if (!realQuiet) {
        const char * outputDest;
        const char * combineDest;

        if (context == CONTEXT_JUSTFRAMES)
            asprintfN(&outputDest, "to individual frame files");
        else
            asprintfN(&outputDest, "to file '%s'", outputFileName);

        if (childProcess)
            combineDest = strdup("for delivery to combine server");
        else
            combineDest = strdup("");
    
        pm_message("%s:  ENCODING FRAMES %u-%u to %s %s",
                   hostname, frameStart, frameEnd, outputDest, combineDest);

        strfree(combineDest);
        strfree(outputDest);
    }
}



static void
encodeSomeFrames(struct inputSource * const inputSourceP,
                 boolean              const childProcess,
                 enum frameContext    const context, 
                 unsigned int         const frameStart,
                 unsigned int         const frameEnd,
                 int32                const qtable[],
                 int32                const niqtable[],
                 FILE *               const ofp,
                 const char *         const outputFileName,
                 bool                 const wantVbvUnderflowWarning,
                 bool                 const wantVbvOverflowWarning,
                 boolean              const printStats,
                 unsigned int *       const encodeTimeP) {
    
    time_t framesTimeStart, framesTimeEnd;
    unsigned int inputFrameBits;
    unsigned int totalBits;

    announceJob(context, childProcess, frameStart, frameEnd, outputFileName);

    time(&framesTimeStart);
    if (printStats)
        PrintStartStats(framesTimeStart, context == CONTEXT_JUSTFRAMES,
                        frameStart, frameEnd, inputSourceP);

    GenMPEGStream(inputSourceP, context, frameStart, frameEnd,
                  qtable, niqtable, childProcess, ofp, outputFileName, 
                  wantVbvUnderflowWarning, wantVbvOverflowWarning,
                  &inputFrameBits, &totalBits);

    time(&framesTimeEnd);

    *encodeTimeP = (unsigned int)(framesTimeEnd - framesTimeStart);

    if (!realQuiet)
        pm_message("%s:  COMPLETED FRAMES %u-%u (%u seconds)",
                   hostname, frameStart, frameEnd, *encodeTimeP);

    if (printStats)
        PrintEndStats(framesTimeStart, framesTimeEnd, 
                      inputFrameBits, totalBits);
}




static void
encodeFrames(struct inputSource * const inputSourceP,
             boolean              const childProcess,
             const char *         const masterHostname,
             int                  const masterPortNumber,
             int                  const whichGOP,
             bool                 const specificFrames,
             unsigned int         const whichFrameStart,
             unsigned int         const whichFrameEnd,
             int32                const qtable[],
             int32                const niqtable[],
             FILE *               const ofp,
             const char *         const outputFileName,
             bool                 const wantVbvUnderflowWarning,
             bool                 const wantVbvOverflowWarning) {
/*----------------------------------------------------------------------------
  Encode the stream.  If 'specificFrames' is true, then encode frames
  'whichFrameStart' through 'whichFrameEnd' individually.  Otherwise,
  encode the entire input stream as a complete MPEG stream.
  
  'childProcess' means to do it as a child process that is under the
  supervision of a master process and is possibly doing only part of
  a larger batch.
  
  (If we had proper modularity, we wouldn't care, but parallel operation
  was glued on to this program after it was complete).
  
  One thing we don't do when running as a child process is print
  statistics; our master will do that for the whole job.
----------------------------------------------------------------------------*/
    unsigned int frameStart, frameEnd;
    enum frameContext context;
    unsigned int      lastEncodeTime;
        /* How long it took the encoder to do the last set of frames */
    boolean           printStats;
        /* We want the encoder to print start & end stats */

    if (whichGOP != -1) {
        /* He wants just one particular GOP from the middle of the movie. */
        ComputeGOPFrames(whichGOP, &frameStart, &frameEnd, 
                         inputSourceP->numInputFiles);
        context = CONTEXT_GOP;
    } else if (specificFrames) {
        /* He wants some pure frames from the middle of the movie */
        if (whichFrameStart > whichFrameEnd)
            pm_error("You specified a starting frame number (%d) that is "
                     "greater than the ending frame number (%d) "
                     "you specified.", whichFrameStart, whichFrameEnd);
        if (whichFrameEnd + 1 > inputSourceP->numInputFiles)
            pm_error("You specified ending frame number %d, which is "
                     "beyond the number of input files you supplied (%u)",
                     whichFrameEnd, inputSourceP->numInputFiles);

        frameStart = whichFrameStart;
        frameEnd   = whichFrameEnd;
        context = CONTEXT_JUSTFRAMES;
    } else {
        /* He wants the whole movie */
        frameStart = 0;
        frameEnd   = inputSourceP->numInputFiles - 1;
        context = CONTEXT_WHOLESTREAM;
    }
    
    printStats = !childProcess;
    
    encodeSomeFrames(inputSourceP, childProcess, context, frameStart, frameEnd,
                     customQtable, customNIQtable,
                     ofp, outputFileName, 
                     wantVbvUnderflowWarning, wantVbvOverflowWarning,
                     printStats,
                     &lastEncodeTime);

    if (childProcess) {
        boolean moreWorkToDo;
        
        /* A child is not capable of generating GOP or stream headers */
        assert(context == CONTEXT_JUSTFRAMES);
        
        moreWorkToDo = TRUE;  /* initial assumption */
        while (moreWorkToDo) {
            int nextFrameStart, nextFrameEnd;

            NotifyMasterDone(masterHostname, masterPortNumber, machineNumber, 
                             lastEncodeTime, &moreWorkToDo,
                             &nextFrameStart, &nextFrameEnd);
            if (moreWorkToDo)
                encodeSomeFrames(inputSourceP, childProcess, 
                                 CONTEXT_JUSTFRAMES, 
                                 nextFrameStart, nextFrameEnd,
                                 customQtable, customNIQtable,
                                 NULL, outputFileName, 
                                 wantVbvUnderflowWarning, 
                                 wantVbvOverflowWarning,
                                 FALSE,
                                 &lastEncodeTime);
        }
        if (!realQuiet)
            pm_message("%s: Child exiting.  Master has no more work.",
                       hostname);
    }
}




static void
runMaster(struct inputSource * const inputSourceP,
          const char *         const paramFileName,
          const char *         const outputFileName) {

    if (paramFileName[0] != '/' && paramFileName[0] != '~')
        pm_error("For parallel mode, you must "
                 "use an absolute path for parameter file.  "
                 "You specified '%s'", paramFileName);
    else
        MasterServer(inputSourceP, paramFileName, outputFileName);
}



static void
getUserFrameFile(void *       const handle,
                 unsigned int const frameNumber,
                 FILE **      const ifPP) {
    
    struct inputSource * const inputSourceP = (struct inputSource *) handle;

    if (inputSourceP->stdinUsed)
        pm_error("You cannot combine frames from Standard Input.");
            /* Because Caller detects end of frames by EOF */
    
    if (frameNumber >= inputSourceP->numInputFiles)
        *ifPP = NULL;
    else {
        const char * fileName;
        const char * inputFileName;

        GetNthInputFileName(inputSourceP, frameNumber, &inputFileName);
        
        asprintfN(&fileName, "%s/%s", currentFramePath, inputFileName);
        
        *ifPP = fopen(fileName, "rb");
        if (*ifPP == NULL)
            pm_error("Unable to open file '%s'.  Errno = %d (%s)",
                     fileName, errno, strerror(errno));
        
        strfree(inputFileName);
        strfree(fileName);
    }
}



static void
nullDisposeFile(void *       const handle,
                unsigned int const frameNumber) {


}



static unsigned int
framePoolSize(bool const sequentialInput) {
/*----------------------------------------------------------------------------
   Return the number of frames that our frame memory pool needs to have.

   'sequentialInput' means we'll be reading from input that comes in frame
   number order and we can't go back, so we'll have to have a bigger buffer
   of frames than otherwise.
-----------------------------------------------------------------------------*/
    unsigned int numOfFrames;

    numOfFrames = 0;  /* initial value */

    if (sequentialInput) {
        unsigned int idx;
        unsigned int bcount;

        for ( idx = 0, bcount = 0; idx < strlen(framePattern); idx++) {

            /* counts the maximum number of B frames between two reference
             * frames. 
             */
            
            switch( framePattern[idx] ) {
            case 'b': 
                bcount++;
                break;
            case 'i':
            case 'p':
                if (bcount > numOfFrames)
                    numOfFrames = bcount;
                bcount = 0;
                break;
            }
            
            /* add 2 to hold the forward and past reference frames in addition
             * to the maximum number of B's 
             */
        }
        
        numOfFrames += 2;

    } else {
        /* non-interactive, only 3 frames needed */
        numOfFrames = 3;
    }
    return numOfFrames;
}



int
main(int argc, char **argv) {
    FILE *ofP;
    time_t  initTimeStart;
    struct cmdlineInfo cmdline;
    struct params params;

    ppm_init(&argc, argv);

    strcpy(encoder_name, argv[0]);

    compileTests();

    time(&initTimeStart);

    SetStatFileName("");

    hostname = GetHostName();

    parseArgs(argc, argv, &cmdline);

    ReadParamFile(cmdline.paramFileName, cmdline.function, &params);

    /* Jim Boucher's stuff:
       if we are using a movie format then break up into frames
    */
    if ((!cmdline.childProcess) && (baseFormat == JMOVIE_FILE_TYPE))
        JM2JPEG(params.inputSourceP);

    if (printSNR || (referenceFrame == DECODED_FRAME))
        decodeRefFrames = TRUE;

    showBitRatePerFrame = (bitRateInfoOption && !cmdline.childProcess);
    frameSummary = (!noFrameSummaryOption && !cmdline.childProcess);

    numMachines = min(numMachines, cmdline.maxMachines);

    Tune_Init();
    Frame_Init(framePoolSize(params.inputSourceP->stdinUsed));

    if (specificsOn) 
        Specifics_Init();

    ComputeFrameTable(params.inputSourceP->stdinUsed ? 
                      0 : params.inputSourceP->numInputFiles);

    if (ioServer) {
        IoServer(params.inputSourceP, cmdline.masterHostname, 
                 cmdline.masterPortNumber);
        return 0;
    } else if (outputServer) {
        CombineServer(cmdline.outputFrames, 
                      cmdline.masterHostname, cmdline.masterPortNumber,
                      outputFileName);
    } else if (decodeServer) {
        DecodeServer(cmdline.outputFrames, outputFileName, 
                     cmdline.masterHostname, cmdline.masterPortNumber);
    } else {
        if ((!cmdline.specificFrames) &&
            ((numMachines == 0) || (cmdline.function != ENCODE_FRAMES)) ) {
            ofP = fopen(outputFileName, "wb");
            if (ofP == NULL)
                pm_error("Could not open output file!");
        } else
            ofP = NULL;
        
        if (cmdline.function == ENCODE_FRAMES) {
            if ((numMachines == 0) || (cmdline.specificFrames)) {
                encodeFrames(params.inputSourceP,
                             cmdline.childProcess, 
                             cmdline.masterHostname, cmdline.masterPortNumber,
                             whichGOP, cmdline.specificFrames,
                             cmdline.frameStart, cmdline.frameEnd,
                             customQtable, customNIQtable,
                             ofP, outputFileName,
                             params.warnUnderflow, params.warnOverflow);
                
            } else
                runMaster(params.inputSourceP,
                          cmdline.paramFileName, outputFileName);
        } else if (cmdline.function == COMBINE_GOPS)
            GOPsToMPEG(params.inputSourceP, outputFileName, ofP);
        else if (cmdline.function == COMBINE_FRAMES)
            FramesToMPEG(ofP, params.inputSourceP,
                         &getUserFrameFile, &nullDisposeFile);
    } 
    Frame_Exit();
        
    strfree(hostname);

    return 0;
}
