/*===========================================================================*
 * subsample.c                                   *
 *                                       *
 *  Procedures concerned with subsampling                    *
 *                                       *
 * EXPORTED PROCEDURES:                              *
 *  LumMotionErrorA                              *
 *  LumMotionErrorB                              *
 *  LumMotionErrorC                              *
 *  LumMotionErrorD                              *
 *                                       *
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

#include "pm_c_util.h"
#include "all.h"
#include "mtypes.h"
#include "frames.h"
#include "bitio.h"
#include "prototypes.h"



static void
computePrevFyFx(MpegFrame * const prevFrame,
                int         const by,
                int         const bx,
                vector      const m,
                uint8 ***   const prevP,
                int *       const fyP,
                int *       const fxP) {

    boolean const xHalf = (ABS(m.x) % 2 == 1);
    boolean const yHalf = (ABS(m.y) % 2 == 1);

    MotionToFrameCoord(by, bx, m.y/2, m.x/2, fyP, fxP);

    if (xHalf) {
        if (m.x < 0)
            --*fxP;

        if (yHalf) {
            if (m.y < 0)
                --*fyP;
        
            *prevP = prevFrame->halfBoth;
        } else
            *prevP = prevFrame->halfX;
    } else if (yHalf) {
        if (m.y < 0)
            --*fyP;
        
        *prevP = prevFrame->halfY;
    } else
        *prevP = prevFrame->ref_y;
}



static int32
evenColDiff(const uint8 * const macross,
            const int32 * const currentRow) {

    return 0
        + ABS(macross[ 0] - currentRow[ 0])
        + ABS(macross[ 2] - currentRow[ 2])
        + ABS(macross[ 4] - currentRow[ 4])
        + ABS(macross[ 6] - currentRow[ 6])
        + ABS(macross[ 8] - currentRow[ 8])
        + ABS(macross[10] - currentRow[10])
        + ABS(macross[12] - currentRow[12])
        + ABS(macross[14] - currentRow[14]);
}



static int32
oddColDiff(const uint8 * const macross,
           const int32 * const currentRow) {

    return 0
        + ABS(macross[ 1] - currentRow[ 1])
        + ABS(macross[ 3] - currentRow[ 3])
        + ABS(macross[ 5] - currentRow[ 5])
        + ABS(macross[ 7] - currentRow[ 7])
        + ABS(macross[ 9] - currentRow[ 9])
        + ABS(macross[11] - currentRow[11])
        + ABS(macross[13] - currentRow[13])
        + ABS(macross[15] - currentRow[15]);
}



/*===========================================================================*
 *
 * LumMotionErrorA
 *
 *  compute the motion error for the A subsampling pattern
 *
 * RETURNS: the error, or some number greater if it is worse
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
int32
LumMotionErrorA(const LumBlock * const currentBlockP,
                MpegFrame *      const prevFrame,
                int              const by,
                int              const bx,
                vector           const m,
                int32            const bestSoFar) {

    int32 diff; /* max value of diff is 255*256 = 65280 */
    uint8 ** prev;
    int fy, fx;
    unsigned int rowNumber;

    computePrevFyFx(prevFrame, by, bx, m, &prev, &fy, &fx);

    diff = 0;  /* initial value */

    for (rowNumber = 0; rowNumber < 16; rowNumber +=2) {
        uint8 *       const macross    = &(prev[fy + rowNumber][fx]);
        const int32 * const currentRow = currentBlockP->l[rowNumber];

        diff += evenColDiff(macross, currentRow);

        if (diff > bestSoFar)
            return diff;
    }
    return diff;
}



/*===========================================================================*
 *
 * LumMotionErrorB
 *
 *  compute the motion error for the B subsampling pattern
 *
 * RETURNS: the error, or some number greater if it is worse
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
int32
LumMotionErrorB(const LumBlock * const currentBlockP,
                MpegFrame *      const prevFrame,
                int              const by,
                int              const bx,
                vector           const m,
                int32            const bestSoFar) {

    int32 diff;  /* max value of diff is 255*256 = 65280 */
    uint8 **prev;
    int fy, fx;
    unsigned int rowNumber;

    computePrevFyFx(prevFrame, by, bx, m, &prev, &fy, &fx);

    diff = 0;  /* initial value */
    
    for (rowNumber = 0; rowNumber < 16; rowNumber +=2) {
        uint8 *       const macross    = &(prev[fy + rowNumber][fx]);
        const int32 * const currentRow = currentBlockP->l[rowNumber];

        diff += oddColDiff(macross, currentRow);

        if (diff > bestSoFar)
            return diff;
    }
    return diff;
}


/*===========================================================================*
 *
 * LumMotionErrorC
 *
 *  compute the motion error for the C subsampling pattern
 *
 * RETURNS: the error, or some number greater if it is worse
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
int32
LumMotionErrorC(const LumBlock * const currentBlockP,
                MpegFrame *      const prevFrame,
                int              const by,
                int              const bx,
                vector           const m,
                int32            const bestSoFar) {

    int32 diff;        /* max value of diff is 255*256 = 65280 */
    uint8 **prev;
    int fy, fx;
    unsigned int rowNumber;

    computePrevFyFx(prevFrame, by, bx, m, &prev, &fy, &fx);

    diff = 0;  /* initial value */
    
    for (rowNumber = 1; rowNumber < 16; rowNumber +=2) {
        uint8 *       const macross    = &(prev[fy + rowNumber][fx]);
        const int32 * const currentRow = currentBlockP->l[rowNumber];

        diff += evenColDiff(macross, currentRow);

        if (diff > bestSoFar)
            return diff;
    }
    return diff;
}


/*===========================================================================*
 *
 * LumMotionErrorD
 *
 *  compute the motion error for the D subsampling pattern
 *
 * RETURNS: the error, or some number greater if it is worse
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
int32
LumMotionErrorD(const LumBlock * const currentBlockP,
                MpegFrame *      const prevFrame,
                int              const by,
                int              const bx,
                vector           const m,
                int32            const bestSoFar) {

    int32 diff;     /* max value of diff is 255*256 = 65280 */
    uint8 ** prev;
    int fy, fx;
    unsigned int rowNumber;

    computePrevFyFx(prevFrame, by, bx, m, &prev, &fy, &fx);

    diff = 0;  /* initial value */

    for (rowNumber = 1; rowNumber < 16; rowNumber +=2) {
        uint8 *       const macross    = &(prev[fy + rowNumber][fx]);
        const int32 * const currentRow = currentBlockP->l[rowNumber];

        diff += oddColDiff(macross, currentRow);

        if (diff > bestSoFar)
            return diff;
    }
    return diff;
}
