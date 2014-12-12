/*===========================================================================*
 * combine.c
 *
 *  Procedures to combine frames or GOPS into an MPEG sequence
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

#include "all.h"
#include <time.h>
#include <errno.h>
#include <unistd.h>

#include "ppm.h"
#include "nstring.h"

#include "mtypes.h"
#include "frames.h"
#include "frametype.h"
#include "motion_search.h"
#include "mpeg.h"
#include "prototypes.h"
#include "parallel.h"
#include "param.h"
#include "readframe.h"
#include "mheaders.h"
#include "fsize.h"
#include "input.h"
#include "combine.h"


#define READ_ATTEMPTS 5 /* number of times (seconds) to retry an input file */

/*==================*
 * GLOBAL VARIABLES *
 *==================*/
extern int  yuvWidth, yuvHeight;


/*===========================================================================*
 *
 * AppendFile
 *
 *  appends the output file with the contents of the given input file
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
AppendFile(FILE * const ofP,
           FILE * const ifP) {

    uint8 data[9999];
    unsigned int readItems;

    readItems = 9999;
    while (readItems == 9999) {
        readItems = fread(data, sizeof(uint8), 9999, ifP);
        if (readItems > 0)
            fwrite(data, sizeof(uint8), readItems, ofP);
    }
    fclose(ifP);
}



static void
appendSpecifiedGopFiles(struct inputSource * const inputSourceP,
                        const char *         const currentGopPath,
                        FILE *               const ofP) {

    unsigned int fileSeq;

    for (fileSeq = 0; fileSeq < inputSourceP->numInputFiles; ++fileSeq) {
        const char * inputFileName;
        const char * fileName;
        unsigned int nAttempts;
        FILE * ifP;

        GetNthInputFileName(inputSourceP, fileSeq, &inputFileName);
        asprintfN(&fileName, "%s/%s", currentGOPPath, inputFileName);

        for (nAttempts = 0, ifP = NULL;
             nAttempts < READ_ATTEMPTS && !ifP;
             ++nAttempts) {

            ifP = fopen(fileName, "rb");
            if (ifP == NULL) {
                pm_message("ERROR:  Couldn't read file '%s'.  retry %u", 
                           fileName, nAttempts);
                sleep(1);
            }
        }
        if (ifP) {
            if (!realQuiet)
                pm_message("Appending file %u '%s'", fileSeq, fileName);
            AppendFile(ofP, ifP);
        } else
            pm_error("Unable to read file '%s' after %u attempts.",
                     fileName, READ_ATTEMPTS);
        strfree(fileName);
        strfree(inputFileName);
    }
} 



static void
appendDefaultGopFiles(const char * const outputFileName,
                      FILE *       const ofP) {

    unsigned int fileSeq;
    bool endOfFiles;
    
    for (fileSeq = 0, endOfFiles = FALSE; !endOfFiles; ++fileSeq) {
        const char * fileName;
        FILE * ifP;

        asprintfN(&fileName, "%s.gop.%u", outputFileName, fileSeq);
        
        ifP = fopen(fileName, "rb");
        if (ifP == NULL)
            endOfFiles = TRUE;
        else {
            if (!realQuiet)
                pm_message("appending file:  %s", fileName);
            
            AppendFile(ofP, ifP);
        }
        strfree(fileName);
    }
}



void
GOPsToMPEG(struct inputSource * const inputSourceP,
           const char *         const outputFileName, 
           FILE *               const ofP) {
/*----------------------------------------------------------------------------
   Combine individual GOPs (one per file) into a single MPEG sequence file.
-----------------------------------------------------------------------------*/
    BitBucket * bb;

    {
        /* Why is this reset called? */
        int x=Fsize_x, y=Fsize_y;
        Fsize_Reset();
        Fsize_Note(0, yuvWidth, yuvHeight);
        if (Fsize_x == 0 || Fsize_y == 0)
            Fsize_Note(0, x, y);
    }
    
    bb = Bitio_New(ofP);

    Mhead_GenSequenceHeader(bb, Fsize_x, Fsize_y, /* pratio */ aspectRatio,
                            /* pict_rate */ frameRate, /* bit_rate */ -1,
                            /* buf_size */ -1, /*c_param_flag */ 1,
                            /* iq_matrix */ customQtable, 
                            /* niq_matrix */ customNIQtable,
                            /* ext_data */ NULL, /* ext_data_size */ 0,
                            /* user_data */ NULL, /* user_data_size */ 0);

    /* it's byte-padded, so we can dump it now */
    Bitio_Flush(bb);

    if (inputSourceP->stdinUsed)
        AppendFile(ofP, stdin);
    else {
        if (inputSourceP->numInputFiles > 0)
            appendSpecifiedGopFiles(inputSourceP, currentGOPPath, ofP);
        else
            appendDefaultGopFiles(outputFileName, ofP);
    }
    bb = Bitio_New(ofP);

    /* SEQUENCE END CODE */
    Mhead_GenSequenceEnder(bb);

    Bitio_Flush(bb);

    fclose(ofP);
}



static void
makeGOPHeader(FILE *       const outputFileP,
              unsigned int const totalFramesSent,
              unsigned int const frameNumber,
              unsigned int const seqWithinGop) {

    boolean closed = (totalFramesSent == frameNumber);

    BitBucket * bbP;

    if (!realQuiet)
        fprintf(stdout, "Creating new GOP (closed = %d) after %d frames\n",
                closed, seqWithinGop);

    /* new GOP */
    bbP = Bitio_New(outputFileP);
    Mhead_GenGOPHeader(bbP, /* drop_frame_flag */ 0,
                       tc_hrs, tc_min, tc_sec, tc_pict,
                       closed, /* broken_link */ 0,
                       /* ext_data */ NULL, /* ext_data_size */ 0,
                       /* user_data */ NULL, 
                       /* user_data_size */ 0);
    Bitio_Flush(bbP);
    SetGOPStartTime(frameNumber);
}        



void
FramesToMPEG(FILE *               const outputFile, 
             void *               const inputHandle,
             fileAcquisitionFn          acquireInputFile,
             fileDispositionFn          disposeInputFile) {
/*----------------------------------------------------------------------------
   Combine a bunch of frames, one per file, into a single MPEG
   sequence file.

   acquireInputFile() opens a file that contains an encoded frame,
   identified by frame number.  It returns NULL when the frame number
   is beyond the frames available.
-----------------------------------------------------------------------------*/
    unsigned int frameNumber;
    BitBucket *bb;
    FILE *inputFile;
    int pastRefNum = -1;
    int futureRefNum = -1;
    boolean inputLeft;
    unsigned int seqWithinGop;
        /* The sequence of the current frame within its GOP.  0 is the
           first frame of a GOP, etc.
        */

    tc_hrs = 0; tc_min = 0; tc_sec = 0; tc_pict = 0; tc_extra = 0;

    {
        /* Why is this reset called? */
        int x=Fsize_x, y=Fsize_y;
        Fsize_Reset();
        Fsize_Note(0, yuvWidth, yuvHeight);
        if (Fsize_x == 0 || Fsize_y == 0)
            Fsize_Note(0, x, y);
    }
    SetBlocksPerSlice();

    bb = Bitio_New(outputFile);
    Mhead_GenSequenceHeader(bb, Fsize_x, Fsize_y, /* pratio */ aspectRatio,
                            /* pict_rate */ frameRate, /* bit_rate */ -1,
                            /* buf_size */ -1, /*c_param_flag */ 1,
                            /* iq_matrix */ qtable, /* niq_matrix */ niqtable,
                            /* ext_data */ NULL, /* ext_data_size */ 0,
                            /* user_data */ NULL, /* user_data_size */ 0);
    /* it's byte-padded, so we can dump it now */
    Bitio_Flush(bb);

    /* need to do these in the right order!!! */
    /* also need to add GOP headers */

    seqWithinGop = 0;
    totalFramesSent = 0;

    inputLeft = TRUE;
    frameNumber = 0;

    makeGOPHeader(outputFile, totalFramesSent, frameNumber, seqWithinGop);
    
    while (inputLeft) {
        if (FType_Type(frameNumber) != 'b') {
            pastRefNum = futureRefNum;
            futureRefNum = frameNumber;

            if ((FType_Type(frameNumber) == 'i') && seqWithinGop >= gopSize) {
                makeGOPHeader(outputFile,
                              totalFramesSent, frameNumber, seqWithinGop);
                seqWithinGop -= gopSize;
            }

            acquireInputFile(inputHandle, frameNumber, &inputFile);
            if (inputFile == NULL)
                inputLeft = FALSE;
            else {
                AppendFile(outputFile, inputFile);

                disposeInputFile(inputHandle, frameNumber);

                ++seqWithinGop;
                IncrementTCTime();

                /* now, output the B-frames */
                if (pastRefNum != -1) {
                    unsigned int bNum;
                    
                    for (bNum = pastRefNum+1; bNum < futureRefNum; ++bNum) {
                        acquireInputFile(inputHandle, bNum, &inputFile);

                        if (inputFile) {
                            AppendFile(outputFile, inputFile);

                            disposeInputFile(inputHandle, bNum);
            
                            ++seqWithinGop;
                            IncrementTCTime();
                        }
                    }
                }
            }
        }
        ++frameNumber;
    }

    if (!realQuiet)
        fprintf(stdout, "Wrote %d frames\n", totalFramesSent);

    bb = Bitio_New(outputFile);

    /* SEQUENCE END CODE */
    Mhead_GenSequenceEnder(bb);

    Bitio_Flush(bb);

    fclose(outputFile);
}



