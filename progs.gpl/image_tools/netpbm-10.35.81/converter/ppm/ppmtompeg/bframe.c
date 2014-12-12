/*===========================================================================*
 * bframe.c
 *
 *  Procedures concerned with the B-frame encoding
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
#include <sys/param.h>
#include <assert.h>

#include "ppm.h"

#include "mtypes.h"
#include "bitio.h"
#include "frames.h"
#include "prototypes.h"
#include "block.h"
#include "fsize.h"
#include "param.h"
#include "mheaders.h"
#include "postdct.h"
#include "rate.h"
#include "opts.h"
#include "specifics.h"

extern int **bfmvHistogram;
extern int **bbmvHistogram;

/*==================*
 * STATIC VARIABLES *
 *==================*/

static int32 totalTime = 0;
static int qscaleB;
static float    totalSNR = 0.0;
static float    totalPSNR = 0.0;

static struct bframeStats {
    unsigned int BSkipped;
    unsigned int BIBits;
    unsigned int BBBits;
    unsigned int Frames;
    unsigned int FrameBits;
    unsigned int BIBlocks;
    unsigned int BBBlocks;
    unsigned int BFOBlocks;    /* forward only */
    unsigned int BBABlocks;    /* backward only */
    unsigned int BINBlocks;    /* interpolate */
    unsigned int BFOBits;
    unsigned int BBABits;
    unsigned int BINBits;
} bframeStats = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


/*====================*
 * EXTERNAL VARIABLES *
 *====================*/

extern Block **dct, **dctr, **dctb;
extern dct_data_type **dct_data;
#define NO_MOTION 0
#define MOTION 1
#define SKIP 2  /* used in useMotion in dct_data */

/*=====================*
 * INTERNAL PROCEDURES *
 *=====================*/

static void
zeroMotion(motion * const motionP) {
    
    motionP->fwd.y = motionP->fwd.x = motionP->bwd.y = motionP->bwd.x = 0;
}



static motion
halfMotion(motion const motion) {
    struct motion half;

    half.fwd.y = motion.fwd.y / 2;
    half.fwd.x = motion.fwd.x / 2;
    half.bwd.y = motion.bwd.y / 2;
    half.bwd.x = motion.bwd.x / 2;

    return half;
}




/*===========================================================================*
 *
 *  compute the block resulting from motion compensation
 *
 * RETURNS: motionBlock is modified
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITION:    the motion vectors must be valid!
 *
 *===========================================================================*/
static void
ComputeBMotionBlock(MpegFrame * const prev,
                    MpegFrame * const next,
                    int         const by,
                    int         const bx,
                    int         const mode,
                    motion      const motion,
                    Block *     const motionBlockP,
                    int         const type) {

    Block prevBlock, nextBlock;

    switch(mode) {
    case MOTION_FORWARD:
        switch (type) {
        case LUM_BLOCK:
            ComputeMotionBlock(prev->ref_y, by, bx, motion.fwd, motionBlockP);
            break;
        case CB_BLOCK:
            ComputeMotionBlock(prev->ref_cb, by, bx, motion.fwd, motionBlockP);
            break;
        case CR_BLOCK:
            ComputeMotionBlock(prev->ref_cr, by, bx, motion.fwd, motionBlockP);
        }
        break;
    case MOTION_BACKWARD:
        switch (type) {
        case LUM_BLOCK:
            ComputeMotionBlock(next->ref_y, by, bx, motion.bwd, motionBlockP);
            break;
        case CB_BLOCK:
            ComputeMotionBlock(next->ref_cb, by, bx, motion.bwd, motionBlockP);
            break;
        case CR_BLOCK:
            ComputeMotionBlock(next->ref_cr, by, bx, motion.bwd, motionBlockP);
            break;
        }
        break;
    case MOTION_INTERPOLATE:
        switch (type) {
        case LUM_BLOCK:
            ComputeMotionBlock(prev->ref_y, by, bx, motion.fwd, &prevBlock);
            ComputeMotionBlock(next->ref_y, by, bx, motion.bwd, &nextBlock);
            break;
        case CB_BLOCK:
            ComputeMotionBlock(prev->ref_cb, by, bx, motion.fwd, &prevBlock);
            ComputeMotionBlock(next->ref_cb, by, bx, motion.bwd, &nextBlock);
            break;
        case CR_BLOCK:
            ComputeMotionBlock(prev->ref_cr, by, bx, motion.fwd, &prevBlock);
            ComputeMotionBlock(next->ref_cr, by, bx, motion.bwd, &nextBlock);
            break;
        }
        {
            unsigned int y;
            for (y = 0; y < 8; ++y) {
                int16 * const blockRow = (*motionBlockP)[y];
                int16 * const prevRow  = prevBlock[y];
                int16 * const nextRow  = nextBlock[y];
                unsigned int x;
                for (x = 0; x < 8; ++x)
                    blockRow[x] = (prevRow[x] + nextRow[x] + 1) / 2;
            }
        }
        break;
    }
}



/*===========================================================================*
 *
 *  compute the DCT of the error term
 *
 * RETURNS: appropriate blocks of current will contain the DCTs
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITION:    the motion vectors must be valid!
 *
 *===========================================================================*/
static void
ComputeBDiffDCTs(MpegFrame * const current,
                 MpegFrame * const prev,
                 MpegFrame * const next,
                 int         const by,
                 int         const bx,
                 int         const mode,
                 motion      const motion,
                 int *       const patternP) {

    Block motionBlock;
    
    if (*patternP & 0x20) {
        boolean significantDiff;
        ComputeBMotionBlock(prev, next, by, bx, mode, motion,
                            &motionBlock, LUM_BLOCK);
        ComputeDiffDCTBlock(current->y_blocks[by][bx], dct[by][bx],
                            motionBlock, &significantDiff);
        if (!significantDiff) 
            *patternP ^=  0x20;
    }

    if (*patternP & 0x10) {
        boolean significantDiff;
        ComputeBMotionBlock(prev, next, by, bx+1, mode, motion,
                            &motionBlock, LUM_BLOCK);
        ComputeDiffDCTBlock(current->y_blocks[by][bx+1], dct[by][bx+1],
                            motionBlock, &significantDiff);
        if (!significantDiff) 
            *patternP ^=  0x10;
    }

    if (*patternP & 0x8) {
        boolean significantDiff;
        ComputeBMotionBlock(prev, next, by+1, bx, mode, motion,
                            &motionBlock, LUM_BLOCK);
        ComputeDiffDCTBlock(current->y_blocks[by+1][bx], dct[by+1][bx],
                            motionBlock, &significantDiff);
        if (!significantDiff)
            *patternP ^= 0x8;
    }

    if (*patternP & 0x4) {
        boolean significantDiff;
        ComputeBMotionBlock(prev, next, by+1, bx+1, mode, motion,
                            &motionBlock, LUM_BLOCK);
        ComputeDiffDCTBlock(current->y_blocks[by+1][bx+1], dct[by+1][bx+1],
                            motionBlock, &significantDiff);
        if (!significantDiff)
            *patternP ^= 0x4;
    }

    if (*patternP & 0x2) {
        boolean significantDiff;
        ComputeBMotionBlock(prev, next, by/2, bx/2, mode,
                            halfMotion(motion), &motionBlock, CB_BLOCK);
        ComputeDiffDCTBlock(current->cb_blocks[by/2][bx/2],
                            dctb[by/2][bx/2], motionBlock,
                            &significantDiff);
        if (!significantDiff)
            *patternP ^= 0x2;
    }

    if (*patternP & 0x1) {
        boolean significantDiff;
        ComputeBMotionBlock(prev, next, by/2, bx/2, mode,
                            halfMotion(motion),
                            &motionBlock, CR_BLOCK);
        ComputeDiffDCTBlock(current->cr_blocks[by/2][bx/2],
                            dctr[by/2][bx/2], motionBlock,
                            &significantDiff);
        if (!significantDiff)
            *patternP ^= 0x1;
    }
}



/*===========================================================================*
 *
 *  decides if this block should be coded as intra-block
 *
 * RETURNS: TRUE if intra-coding should be used; FALSE otherwise
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITION:    the motion vectors must be valid!
 *
 *===========================================================================*/
static boolean
DoBIntraCode(MpegFrame * const current,
             MpegFrame * const prev,
             MpegFrame * const next,
             int         const by,
             int         const bx,
             int         const mode,
             motion      const motion) {

    boolean retval;
    unsigned int y;
    int32 sum = 0, vard = 0, varc = 0;
    LumBlock motionBlock;
    int fy, fx;

    ComputeBMotionLumBlock(prev, next, by, bx, mode, motion, &motionBlock);

    MotionToFrameCoord(by, bx, 0, 0, &fy, &fx);

    for (y = 0; y < 16; ++y) {
        unsigned int x;
        for (x = 0; x < 16; ++x) {
            int32 const currPixel = current->orig_y[fy+y][fx+x];
            int32 const prevPixel = motionBlock.l[y][x];
            int32 const dif = currPixel - prevPixel;

            sum += currPixel;
            varc += SQR(currPixel);
            vard += SQR(dif);
        }
    }

    vard >>= 8;     /* divide by 256; assumes mean is close to zero */
    varc = (varc>>8) - (sum>>8)*(sum>>8);

    if (vard <= 64)
        retval = FALSE;
    else if (vard < varc)
        retval = FALSE;
    else
        retval = TRUE;
    return retval;
}



static int
ComputeBlockColorDiff(Block current,
                      Block motionBlock) {

    unsigned int y;
    int diffTotal;
    
    diffTotal = 0;

    for (y = 0; y < 8; ++y) {
        unsigned int x;
        for (x = 0; x < 8; ++x) {
            int const diffTmp = current[y][x] - motionBlock[y][x];
            diffTotal += ABS(diffTmp);
        }
    }
    return diffTotal;
}



/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/


/*===========================================================================*
 * MotionSufficient
 *
 *  decides if this motion vector is sufficient without DCT coding
 *
 * RETURNS: TRUE if no DCT is needed; FALSE otherwise
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITION:    the motion vectors must be valid!
 *
 *===========================================================================*/
static boolean
MotionSufficient(MpegFrame *      const curr,
                 const LumBlock * const currBlockP,
                 MpegFrame *      const prev,
                 MpegFrame *      const next,
                 int              const by,
                 int              const bx,
                 int              const mode,
                 motion           const motion) {

    LumBlock mLumBlock;
    Block mColorBlock;
    int lumErr, colorErr;

    /* check bounds */
    if ( mode != MOTION_BACKWARD ) {
        if ( (by*DCTSIZE+(motion.fwd.y-1)/2 < 0) ||
             ((by+2)*DCTSIZE+(motion.fwd.y+1)/2-1 >= Fsize_y) ) {
            return FALSE;
        }
        if ( (bx*DCTSIZE+(motion.fwd.x-1)/2 < 0) ||
             ((bx+2)*DCTSIZE+(motion.fwd.x+1)/2-1 >= Fsize_x) ) {
            return FALSE;
        }
    }

    if ( mode != MOTION_FORWARD ) {
        if ( (by*DCTSIZE+(motion.bwd.y-1)/2 < 0) ||
             ((by+2)*DCTSIZE+(motion.bwd.y+1)/2-1 >= Fsize_y) ) {
            return FALSE;
        }
        if ( (bx*DCTSIZE+(motion.bwd.x-1)/2 < 0) ||
             ((bx+2)*DCTSIZE+(motion.bwd.x+1)/2-1 >= Fsize_x) ) {
            return FALSE;
        }
    }

    /* check Lum */
    ComputeBMotionLumBlock(prev, next, by, bx, mode, motion, &mLumBlock);
    lumErr =  LumBlockMAD(currBlockP, &mLumBlock, 0x7fffffff);
    if (lumErr > 512) {
        return FALSE;
    }

    /* check color */
    ComputeBMotionBlock(prev, next, by/2, bx/2, mode,
                        halfMotion(motion), &mColorBlock, CR_BLOCK);
    colorErr = ComputeBlockColorDiff(curr->cr_blocks[by/2][bx/2],
                                     mColorBlock);
    ComputeBMotionBlock(prev, next, by/2, bx/2, mode, halfMotion(motion),
                        &mColorBlock, CB_BLOCK);
    colorErr += ComputeBlockColorDiff(curr->cr_blocks[by/2][bx/2],
                                      mColorBlock);
    
    return (colorErr < 256); /* lumErr checked above */
}




struct stats {
    int IBlocks;
    int BBlocks;
    int Skipped;
    int totalBits;
    int IBits;
    int BBits;
};



static void
initializeStats(struct stats * const statsP) {
    statsP->IBlocks = 0;
    statsP->BBlocks = 0;
    statsP->Skipped = 0;
    statsP->totalBits  = 0;
    statsP->IBits   = 0;
    statsP->BBits   = 0;
}



static void
checkSpecifics(MpegFrame *      const curr, 
               int              const mbAddress,
               int              const QScale,
               boolean *        const skipItP,
               boolean *        const doBsearchP,
               int *            const modeP,
               motion *         const motionP) {
/*----------------------------------------------------------------------------
   We return *modeP iff we return *doBsearchP == FALSE.

   We return *motionP iff we return *skipItP == FALSE.
-----------------------------------------------------------------------------*/
    BlockMV * info;

    SpecLookup(curr->id, 2, mbAddress, &info, QScale);
    if (info == NULL) {
        *doBsearchP = TRUE;
        *skipItP = FALSE;
    } else {
        *doBsearchP = FALSE;

        switch (info->typ) {
        case TYP_SKIP:
            *skipItP = TRUE;
            break;
        case TYP_FORW:
            *skipItP = FALSE;
            motionP->fwd.y = info->fy;
            motionP->fwd.x = info->fx;
            *modeP = MOTION_FORWARD;
            break;
        case TYP_BACK:
            *skipItP = FALSE;
            motionP->bwd.y = info->by;
            motionP->bwd.x = info->bx;
            *modeP = MOTION_BACKWARD;
            break;
        case TYP_BOTH:
            *skipItP = FALSE;
            motionP->fwd.y = info->fy;
            motionP->fwd.x = info->fx;
            motionP->bwd.y = info->by;
            motionP->bwd.x = info->bx;
            *modeP = MOTION_INTERPOLATE;
            break;
        default:
            pm_error("Unreachable code in GenBFrame!");
        }
    }
}



static void
makeNonSkipBlock(int              const y,
                 int              const x, 
                 MpegFrame *      const curr, 
                 MpegFrame *      const prev, 
                 MpegFrame *      const next,
                 boolean          const specificsOn,
                 int              const mbAddress,
                 int              const QScale,
                 const LumBlock * const currentBlockP,
                 int *            const modeP,
                 int *            const oldModeP,
                 boolean          const IntrPBAllowed,
                 boolean *        const lastIntraP,
                 motion *         const motionP,
                 motion *         const oldMotionP,
                 struct stats *   const statsP) {

    motion motion;
    int mode;
    boolean skipIt;
    boolean doBsearch;

    if (specificsOn)
        checkSpecifics(curr, mbAddress, QScale, &skipIt, &doBsearch,
                       &mode, &motion);
    else {
        skipIt = FALSE;
        doBsearch = TRUE;
    }
    if (skipIt)
        dct_data[y][x].useMotion = SKIP;
    else {
        if (doBsearch) {
            motion = *motionP;  /* start with old motion */
            mode = BMotionSearch(currentBlockP, prev, next, y, x,
                                 &motion, mode);
        }
        /* STEP 2:  INTRA OR NON-INTRA CODING */
        if (IntraPBAllowed && 
            DoBIntraCode(curr, prev, next, y, x, mode, motion)) {
            /* output I-block inside a B-frame */
            ++statsP->IBlocks;
            zeroMotion(oldMotionP);
            *lastIntraP = TRUE;
            dct_data[y][x].useMotion = NO_MOTION;
            *oldModeP = MOTION_FORWARD;
            /* calculate forward dct's */
            if (collect_quant && (collect_quant_detailed & 1)) 
                fprintf(collect_quant_fp, "l\n");
            mp_fwd_dct_block2(curr->y_blocks[y][x], dct[y][x]);
            mp_fwd_dct_block2(curr->y_blocks[y][x+1], dct[y][x+1]);
            mp_fwd_dct_block2(curr->y_blocks[y+1][x], dct[y+1][x]);
            mp_fwd_dct_block2(curr->y_blocks[y+1][x+1], dct[y+1][x+1]);
            if (collect_quant && (collect_quant_detailed & 1)) {
                fprintf(collect_quant_fp, "c\n");
            }
            mp_fwd_dct_block2(curr->cb_blocks[y>>1][x>>1], 
                              dctb[y>>1][x>>1]);
            mp_fwd_dct_block2(curr->cr_blocks[y>>1][x>>1], 
                              dctr[y>>1][x>>1]);

        } else { /* dct P/Bi/B block */
            int pattern;

            pattern = 63;
            *lastIntraP = FALSE;
            ++statsP->BBlocks;
            dct_data[y][x].mode = mode;
            *oldModeP = mode;
            dct_data[y][x].fmotionY = motion.fwd.y;
            dct_data[y][x].fmotionX = motion.fwd.x;
            dct_data[y][x].bmotionY = motion.bwd.y;
            dct_data[y][x].bmotionX = motion.bwd.x;

            switch (mode) {
            case MOTION_FORWARD:
                ++bframeStats.BFOBlocks;
                oldMotionP->fwd = motion.fwd;
                break;
            case MOTION_BACKWARD:
                ++bframeStats.BBABlocks;
                oldMotionP->bwd = motion.bwd;
                break;
            case MOTION_INTERPOLATE:
                ++bframeStats.BINBlocks;
                *oldMotionP = motion;
                break;
            default:
                pm_error("INTERNAL ERROR:  Illegal mode: %d", mode);
            }
        
            ComputeBDiffDCTs(curr, prev, next, y, x, mode, motion, &pattern);
        
            dct_data[y][x].pattern = pattern;
            dct_data[y][x].useMotion = MOTION;
            if ( computeMVHist ) {
                assert(motion.fwd.x + searchRangeB + 1 >= 0);
                assert(motion.fwd.y + searchRangeB + 1 >= 0);
                assert(motion.fwd.x + searchRangeB + 1 <= 2*searchRangeB + 2);
                assert(motion.fwd.y + searchRangeB + 1 <= 2*searchRangeB + 2);
                assert(motion.bwd.x + searchRangeB + 1 >= 0);
                assert(motion.bwd.y + searchRangeB + 1 >= 0);
                assert(motion.bwd.x + searchRangeB + 1 <= 2*searchRangeB + 2);
                assert(motion.bwd.y + searchRangeB + 1 <= 2*searchRangeB + 2);
                
                ++bfmvHistogram[motion.fwd.x + searchRangeB + 1]
                    [motion.fwd.y + searchRangeB + 1];
                ++bbmvHistogram[motion.bwd.x + searchRangeB + 1]
                    [motion.bwd.y + searchRangeB + 1];
            }
        } /* motion-block */
    }
    *motionP = motion;
    *modeP   = mode;
}



/*===========================================================================*
 *
 * GenBFrame
 *
 *  generate a B-frame from previous and next frames, adding the result
 *  to the given bit bucket
 *
 * RETURNS: frame appended to bb
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
GenBFrame(BitBucket * const bb, 
          MpegFrame * const curr, 
          MpegFrame * const prev, 
          MpegFrame * const next) {

    FlatBlock fba[6], fb[6];
    Block     dec[6];
    int32 y_dc_pred, cr_dc_pred, cb_dc_pred;
    int x, y;
    struct motion motion;
    struct motion oldMotion;
    int oldMode = MOTION_FORWARD;
    int mode = MOTION_FORWARD;
    int offsetX, offsetY;
    struct motion motionRem;
    struct motion motionQuot;
    struct stats stats;
    boolean lastIntra = TRUE;
    boolean    motionForward, motionBackward;
    int     totalFrameBits;
    int32    startTime, endTime;
    int lastX, lastY;
    int lastBlockX, lastBlockY;
    int ix, iy;
    LumBlock currentBlock;
    int         fy, fx;
    boolean make_skip_block;
    int mbAddrInc = 1;
    int mbAddress;
    int     slicePos;
    float   snr[3], psnr[3];
    int     idx;
    int     QScale;
    BlockMV *info;
    int     bitstreamMode, newQScale;
    int     rc_blockStart=0;
    boolean overflowChange=FALSE;
    int overflowValue = 0;

    assert(prev != NULL);
    assert(next != NULL);

    initializeStats(&stats);

    if (collect_quant) {fprintf(collect_quant_fp, "# B\n");}
    if (dct == NULL) AllocDctBlocks();
    ++bframeStats.Frames;
    totalFrameBits = bb->cumulativeBits;
    startTime = time_elapsed();

    /*   Rate Control */
    bitstreamMode = getRateMode();
    if (bitstreamMode == FIXED_RATE) {
        targetRateControl(curr);
    }
 
    QScale = GetBQScale();
    Mhead_GenPictureHeader(bb, B_FRAME, curr->id, fCodeB);
    /* Check for Qscale change */
    if (specificsOn) {
        newQScale = SpecLookup(curr->id, 0, 0 /* junk */, &info, QScale);
        if (newQScale != -1) {
            QScale = newQScale;
        }
        /* check for slice */
        newQScale = SpecLookup(curr->id, 1, 1, &info, QScale);
        if (newQScale != -1) {
            QScale = newQScale;
        }
    }

    Mhead_GenSliceHeader(bb, 1, QScale, NULL, 0);

    Frame_AllocBlocks(curr);
    BlockifyFrame(curr);

    if ( printSNR ) {
        Frame_AllocDecoded(curr, FALSE);
    }

    /* for I-blocks */
    y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

    stats.totalBits = bb->cumulativeBits;

    if ( ! pixelFullSearch ) {
        if ( ! prev->halfComputed ) {
            ComputeHalfPixelData(prev);
        }

        if ( ! next->halfComputed ) {
            ComputeHalfPixelData(next);
        }
    }

    lastBlockX = Fsize_x / 8;
    lastBlockY = Fsize_y / 8;
    lastX = lastBlockX - 2;
    lastY = lastBlockY - 2;
    mbAddress = 0;

    /* Start with zero motion assumption */
    zeroMotion(&motion); 
    zeroMotion(&oldMotion);
    zeroMotion(&motionRem);
    zeroMotion(&motionQuot);

    /* find motion vectors and do dcts */
    /* In this first loop, all MVs are in half-pixel scope, (if FULL
       is set then they will be multiples of 2).  This is not true in
       the second loop. 
    */
    for (y = 0;  y < lastBlockY;  y += 2) {
        for (x = 0;  x < lastBlockX;  x += 2) {
            slicePos = (mbAddress % blocksPerSlice);

            /* compute currentBlock */
            BLOCK_TO_FRAME_COORD(y, x, fy, fx);
            for ( iy = 0; iy < 16; iy++ ) {
                for ( ix = 0; ix < 16; ix++ ) {
                    currentBlock.l[iy][ix] = (int16)curr->orig_y[fy+iy][fx+ix];
                }
            }
        
            if (slicePos == 0) {
                zeroMotion(&oldMotion);
                oldMode = MOTION_FORWARD;
                lastIntra = TRUE;
            }

            /* STEP 1:  Select Forward, Backward, or Interpolated motion 
               vectors */
            /* see if old motion is good enough */
            /* but force last block to be non-skipped */
            /* can only skip if:
             *     1)  not the last block in frame
             *     2)  not the last block in slice
             *     3)  not the first block in slice
             *     4)  previous block was not intra-coded
             */
            if ( ((y < lastY) || (x < lastX)) &&
                 (slicePos+1 != blocksPerSlice) &&
                 (slicePos != 0) &&
                 (! lastIntra) &&
                 (BSkipBlocks) ) {
                make_skip_block =
                    MotionSufficient(curr, &currentBlock, 
                                     prev, next, y, x, oldMode, oldMotion);
            } else
                make_skip_block = FALSE;

            if (make_skip_block) {
                /* skipped macro block */
                dct_data[y][x].useMotion = SKIP;
            } else
                makeNonSkipBlock(y, x, curr, prev, next, specificsOn,
                                 mbAddress,
                                 QScale, &currentBlock,
                                 &mode, &oldMode,
                                 IntraPBAllowed,
                                 &lastIntra, &motion, &oldMotion, &stats);

            ++mbAddress;
        }
    }

    /* reset everything */
    zeroMotion(&oldMotion);
    oldMode = MOTION_FORWARD;
    lastIntra = TRUE;
    y_dc_pred = cr_dc_pred = cb_dc_pred = 128;
    mbAddress = 0;

    /* Now generate the frame */
    for (y = 0; y < lastBlockY; y += 2) {
      for (x = 0; x < lastBlockX; x += 2) {
    slicePos = (mbAddress % blocksPerSlice);

    if ( (slicePos == 0) && (mbAddress != 0) ) {
      if (specificsOn) {
        /* Make sure no slice Qscale change */
        newQScale = 
            SpecLookup(curr->id,1,mbAddress/blocksPerSlice, &info, QScale);
        if (newQScale != -1) QScale = newQScale;
      }
      Mhead_GenSliceEnder(bb);
      Mhead_GenSliceHeader(bb, 1+(y>>1), QScale, NULL, 0);

      /* reset everything */
      zeroMotion(&oldMotion);
      oldMode = MOTION_FORWARD;
      lastIntra = TRUE;
      y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

      mbAddrInc = 1+(x>>1);
    }

    /*  Determine if new Qscale needed for Rate Control purposes */
    if (bitstreamMode == FIXED_RATE) {
      rc_blockStart =  bb->cumulativeBits;
      newQScale = needQScaleChange(QScale,
                       curr->y_blocks[y][x],
                       curr->y_blocks[y][x+1],
                       curr->y_blocks[y+1][x],
                       curr->y_blocks[y+1][x+1]);
      if (newQScale > 0) {
        QScale = newQScale;
      }
    }
 
    if (specificsOn) {
      newQScale = SpecLookup(curr->id, 2, mbAddress, &info, QScale);
      if (newQScale != -1) {
        QScale = newQScale;
      }}

    if (dct_data[y][x].useMotion == NO_MOTION) {

      GEN_I_BLOCK(B_FRAME, curr, bb, mbAddrInc, QScale);
      mbAddrInc = 1;
      stats.IBits += (bb->cumulativeBits - stats.totalBits);
      stats.totalBits = bb->cumulativeBits;
          
      /* reset because intra-coded */
      zeroMotion(&oldMotion);
      oldMode = MOTION_FORWARD;
      lastIntra = TRUE;
          
      if ( printSNR ) {
        /* need to decode block we just encoded */
        /* and reverse the DCT transform */
        for ( idx = 0; idx < 6; idx++ ) {
          Mpost_UnQuantZigBlock(fb[idx], dec[idx], QScale, TRUE);
          mpeg_jrevdct((int16 *)dec[idx]);
        }

        /* now, unblockify */
        BlockToData(curr->decoded_y, dec[0], y, x);
        BlockToData(curr->decoded_y, dec[1], y, x+1);
        BlockToData(curr->decoded_y, dec[2], y+1, x);
        BlockToData(curr->decoded_y, dec[3], y+1, x+1);
        BlockToData(curr->decoded_cb, dec[4], y>>1, x>>1);
        BlockToData(curr->decoded_cr, dec[5], y>>1, x>>1);
      }
    } else if (dct_data[y][x].useMotion == SKIP) {
      ++stats.Skipped;
      mbAddrInc++;
          
      /* decode skipped block */
      if (printSNR) {
          struct motion motion;
        
          for (idx = 0; idx < 6; ++idx)
              memset((char *)dec[idx], 0, sizeof(Block)); 
        
          if (pixelFullSearch) {
              motion.fwd.y = 2 * oldMotion.fwd.y;
              motion.fwd.x = 2 * oldMotion.fwd.x;
              motion.bwd.y = 2 * oldMotion.bwd.y;
              motion.bwd.x = 2 * oldMotion.bwd.x;
          } else
              motion = oldMotion;
          
          /* now add the motion block */
          AddBMotionBlock(dec[0], prev->decoded_y,
                          next->decoded_y, y, x, mode, motion);
          AddBMotionBlock(dec[1], prev->decoded_y,
                          next->decoded_y, y, x+1, mode, motion);
          AddBMotionBlock(dec[2], prev->decoded_y,
                          next->decoded_y, y+1, x, mode, motion);
          AddBMotionBlock(dec[3], prev->decoded_y,
                          next->decoded_y, y+1, x+1, mode, motion);
          AddBMotionBlock(dec[4], prev->decoded_cb,
                          next->decoded_cb, y/2, x/2, mode,
                          halfMotion(motion));
          AddBMotionBlock(dec[5], prev->decoded_cr,
                          next->decoded_cr, y/2, x/2, mode,
                          halfMotion(motion));
        
          /* now, unblockify */
          BlockToData(curr->decoded_y, dec[0], y, x);
          BlockToData(curr->decoded_y, dec[1], y, x+1);
          BlockToData(curr->decoded_y, dec[2], y+1, x);
          BlockToData(curr->decoded_y, dec[3], y+1, x+1);
          BlockToData(curr->decoded_cb, dec[4], y/2, x/2);
          BlockToData(curr->decoded_cr, dec[5], y/2, x/2);
      }
    } else   /* B block */ {
        int const fCode = fCodeB;   
        int pattern;
        
        pattern = dct_data[y][x].pattern;
        motion.fwd.y = dct_data[y][x].fmotionY;
        motion.fwd.x = dct_data[y][x].fmotionX;
        motion.bwd.y = dct_data[y][x].bmotionY;
        motion.bwd.x = dct_data[y][x].bmotionX;

        if (pixelFullSearch)
            motion = halfMotion(motion);
          
        /* create flat blocks and update pattern if necessary */
    calc_blocks:
        /* Note DoQuant references QScale, overflowChange, overflowValue,
           pattern, and the calc_blocks label                 */
        DoQuant(0x20, dct[y][x], fba[0]);
        DoQuant(0x10, dct[y][x+1], fba[1]);
        DoQuant(0x08, dct[y+1][x], fba[2]);
        DoQuant(0x04, dct[y+1][x+1], fba[3]);
        DoQuant(0x02, dctb[y/2][x/2], fba[4]);
        DoQuant(0x01, dctr[y/2][x/2], fba[5]);

        motionForward  = (dct_data[y][x].mode != MOTION_BACKWARD);
        motionBackward = (dct_data[y][x].mode != MOTION_FORWARD);
        
        /* Encode Vectors */
        if (motionForward) {
            /* transform the fMotion vector into the appropriate values */
            offsetY = motion.fwd.y - oldMotion.fwd.y;
            offsetX = motion.fwd.x - oldMotion.fwd.x;

            encodeMotionVector(offsetX, offsetY,
                               &motionQuot.fwd, &motionRem.fwd,
                               FORW_F, fCode);
            oldMotion.fwd = motion.fwd;
        }
          
        if (motionBackward) {
            /* transform the bMotion vector into the appropriate values */
            offsetY = motion.bwd.y - oldMotion.bwd.y;
            offsetX = motion.bwd.x - oldMotion.bwd.x;
            encodeMotionVector(offsetX, offsetY,
                               &motionQuot.bwd, &motionRem.bwd,
                               BACK_F, fCode);
            oldMotion.bwd = motion.bwd;
        }
          
        oldMode = dct_data[y][x].mode;
          
        if (printSNR) { /* Need to decode */
            if (pixelFullSearch) {
                motion.fwd.x *= 2; motion.fwd.y *= 2;
                motion.bwd.x *= 2; motion.bwd.y *= 2;
            }
            for ( idx = 0; idx < 6; idx++ ) {
                if ( pattern & (1 << (5-idx)) ) {
                    Mpost_UnQuantZigBlock(fba[idx], dec[idx], QScale, FALSE);
                    mpeg_jrevdct((int16 *)dec[idx]);
                } else {
                    memset((char *)dec[idx], 0, sizeof(Block));
                }
            }

            /* now add the motion block */
            AddBMotionBlock(dec[0], prev->decoded_y,
                            next->decoded_y, y, x, mode, motion);
            AddBMotionBlock(dec[1], prev->decoded_y,
                            next->decoded_y, y, x+1, mode, motion);
            AddBMotionBlock(dec[2], prev->decoded_y,
                            next->decoded_y, y+1, x, mode, motion);
            AddBMotionBlock(dec[3], prev->decoded_y,
                            next->decoded_y, y+1, x+1, mode, motion);
            AddBMotionBlock(dec[4], prev->decoded_cb,
                            next->decoded_cb, y/2, x/2, mode,
                            halfMotion(motion));
            AddBMotionBlock(dec[5], prev->decoded_cr,
                            next->decoded_cr, y/2, x/2, mode,
                            halfMotion(motion));

            /* now, unblockify */
            BlockToData(curr->decoded_y,  dec[0], y,   x);
            BlockToData(curr->decoded_y,  dec[1], y,   x+1);
            BlockToData(curr->decoded_y,  dec[2], y+1, x);
            BlockToData(curr->decoded_y,  dec[3], y+1, x+1);
            BlockToData(curr->decoded_cb, dec[4], y/2, x/2);
            BlockToData(curr->decoded_cr, dec[5], y/2, x/2);
        }

        /* reset because non-intra-coded */
        y_dc_pred = cr_dc_pred = cb_dc_pred = 128;
        lastIntra = FALSE;
        mode = dct_data[y][x].mode;
        
        /*      DBG_PRINT(("MB Header(%d,%d)\n", x, y));  */
        Mhead_GenMBHeader(
            bb, 3 /* pict_code_type */, mbAddrInc /* addr_incr */,
            QScale /* q_scale */,
            fCodeB /* forw_f_code */, fCodeB /* back_f_code */,
            motionRem.fwd.x /* horiz_forw_r */,
            motionRem.fwd.y /* vert_forw_r */,
            motionRem.bwd.x /* horiz_back_r */,
            motionRem.bwd.y /* vert_back_r */,
            motionForward /* motion_forw */,
            motionQuot.fwd.x /* m_horiz_forw */,
            motionQuot.fwd.y /* m_vert_forw */,
            motionBackward /* motion_back */,
            motionQuot.bwd.x /* m_horiz_back */,
            motionQuot.bwd.y /* m_vert_back */,
            pattern /* mb_pattern */, FALSE /* mb_intra */);

        mbAddrInc = 1;
          
        /* now output the difference */
        {
            unsigned int x;
            for (x = 0; x < 6; ++x) {
                if (GET_ITH_BIT(pattern, 5-x))
                    Mpost_RLEHuffPBlock(fba[x], bb);
            }
        }
          
        switch (mode) {
        case MOTION_FORWARD:
            bframeStats.BFOBits += (bb->cumulativeBits - stats.totalBits);
            break;
        case MOTION_BACKWARD:
            bframeStats.BBABits += (bb->cumulativeBits - stats.totalBits);
            break;
        case MOTION_INTERPOLATE:
            bframeStats.BINBits += (bb->cumulativeBits - stats.totalBits);
            break;
        default:
            pm_error("PROGRAMMER ERROR:  Illegal mode: %d", mode);
      }
      
      stats.BBits += (bb->cumulativeBits - stats.totalBits);
      stats.totalBits = bb->cumulativeBits;
    
      if (overflowChange) {
        /* undo an overflow-caused Qscale change */
        overflowChange = FALSE;
        QScale -= overflowValue;
        overflowValue = 0;
      }
    } /* if I-block, skip, or B */

    mbAddress++;
    /*   Rate Control  */
    if (bitstreamMode == FIXED_RATE) {
      incMacroBlockBits( bb->cumulativeBits - rc_blockStart);
      rc_blockStart = bb->cumulativeBits;
      MB_RateOut(TYPE_BFRAME);
    }
    
      }
    }

    if ( printSNR ) {
      BlockComputeSNR(curr,snr,psnr);
      totalSNR += snr[0];
      totalPSNR += psnr[0];
    }
    
    Mhead_GenSliceEnder(bb);
    /*   Rate Control  */
    if (bitstreamMode == FIXED_RATE) {
      updateRateControl(TYPE_BFRAME);
    }
    
    endTime = time_elapsed();
    totalTime += (endTime-startTime);
    
    if ( showBitRatePerFrame ) {
      /* ASSUMES 30 FRAMES PER SECOND */
      fprintf(bitRateFile, "%5d\t%8d\n", curr->id,
          30*(bb->cumulativeBits-totalFrameBits));
    }
    
    if ( frameSummary && !realQuiet) {
      fprintf(stdout, "FRAME %d (B):  "
              "I BLOCKS: %5d;  B BLOCKS: %5d   SKIPPED: %5d (%ld seconds)\n",
              curr->id, stats.IBlocks, stats.BBlocks, stats.Skipped,
              (long)((endTime-startTime)/TIME_RATE));
      if ( printSNR )
    fprintf(stdout, "FRAME %d:  "
            "SNR:  %.1f\t%.1f\t%.1f\tPSNR:  %.1f\t%.1f\t%.1f\n",
            curr->id, snr[0], snr[1], snr[2],
            psnr[0], psnr[1], psnr[2]);
    }
    
    bframeStats.FrameBits += (bb->cumulativeBits-totalFrameBits);
    bframeStats.BIBlocks += stats.IBlocks;
    bframeStats.BBBlocks += stats.BBlocks;
    bframeStats.BSkipped += stats.Skipped;
    bframeStats.BIBits   += stats.IBits;
    bframeStats.BBBits   += stats.BBits;
  }


/*===========================================================================*
 *
 * SetBQScale
 *
 *  set the B-frame Q-scale
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    qscaleB
 *
 *===========================================================================*/
void
SetBQScale(qB)
    int qB;
{
    qscaleB = qB;
}


/*===========================================================================*
 *
 * GetBQScale
 *
 *  get the B-frame Q-scale
 *
 * RETURNS: the Q-scale
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
int
GetBQScale()
{
    return qscaleB;
}


/*===========================================================================*
 *
 * ResetBFrameStats
 *
 *  reset the B-frame stats
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
ResetBFrameStats() {

    bframeStats.BIBlocks = 0;
    bframeStats.BBBlocks = 0;
    bframeStats.BSkipped = 0;
    bframeStats.BIBits = 0;
    bframeStats.BBBits = 0;
    bframeStats.Frames = 0;
    bframeStats.FrameBits = 0;
    totalTime = 0;
}



float
BFrameTotalTime(void) {
    return (float)totalTime/(float)TIME_RATE;
}



void
ShowBFrameSummary(unsigned int const inputFrameBits, 
                  unsigned int const totalBits, 
                  FILE *       const fpointer) {

    if (bframeStats.Frames > 0) {
        fprintf(fpointer, "-------------------------\n");
        fprintf(fpointer, "*****B FRAME SUMMARY*****\n");
        fprintf(fpointer, "-------------------------\n");

        if (bframeStats.BIBlocks > 0) {
            fprintf(fpointer,
                    "  I Blocks:  %5d     (%6d bits)     (%5d bpb)\n",
                    bframeStats.BIBlocks, bframeStats.BIBits,
                    bframeStats.BIBits/bframeStats.BIBlocks);
        } else
            fprintf(fpointer, "  I Blocks:  %5d\n", 0);

        if (bframeStats.BBBlocks > 0) {
            fprintf(fpointer,
                    "  B Blocks:  %5d     (%6d bits)     (%5d bpb)\n",
                    bframeStats.BBBlocks, bframeStats.BBBits,
                    bframeStats.BBBits/bframeStats.BBBlocks);
            fprintf(fpointer,
                    "  B types:   %5d     (%4d bpb) "
                    "forw  %5d (%4d bpb) back   %5d (%4d bpb) bi\n",
                    bframeStats.BFOBlocks,
                    (bframeStats.BFOBlocks==0) ? 
                        0 : bframeStats.BFOBits/bframeStats.BFOBlocks,
                    bframeStats.BBABlocks,
                    (bframeStats.BBABlocks==0) ? 
                        0 : bframeStats.BBABits/bframeStats.BBABlocks,
                    bframeStats.BINBlocks,
                    (bframeStats.BINBlocks==0) ? 
                        0 : bframeStats.BINBits/bframeStats.BINBlocks);
        } else
            fprintf(fpointer, "  B Blocks:  %5d\n", 0);

        fprintf(fpointer, "  Skipped:   %5d\n", bframeStats.BSkipped);

        fprintf(fpointer, "  Frames:    %5d     (%6d bits)     "
                "(%5d bpf)     (%2.1f%% of total)\n",
                bframeStats.Frames, bframeStats.FrameBits,
                bframeStats.FrameBits/bframeStats.Frames,
                100.0*(float)bframeStats.FrameBits/(float)totalBits);        
        fprintf(fpointer, "  Compression:  %3d:1     (%9.4f bpp)\n",
                bframeStats.Frames*inputFrameBits/bframeStats.FrameBits,
                24.0*(float)bframeStats.FrameBits/
                    (float)(bframeStats.Frames*inputFrameBits));
        if (printSNR)
            fprintf(fpointer, "  Avg Y SNR/PSNR:  %.1f     %.1f\n",
                    totalSNR/(float)bframeStats.Frames,
                    totalPSNR/(float)bframeStats.Frames);
        if (totalTime == 0) {
            fprintf(fpointer, "  Seconds:  NONE\n");
        } else {
            fprintf(fpointer, "  Seconds:  %9ld     (%9.4f fps)  "
                    "(%9ld pps)  (%9ld mps)\n",
                    (long)(totalTime/TIME_RATE),
                    (float)((float)(TIME_RATE*bframeStats.Frames)/
                            (float)totalTime),
                    (long)((float)TIME_RATE*(float)bframeStats.Frames *
                           (float)inputFrameBits/(24.0*(float)totalTime)),
                    (long)((float)TIME_RATE*(float)bframeStats.Frames *
                           (float)inputFrameBits/(256.0*24.0 *
                                                  (float)totalTime)));
        }
    }
}



/*===========================================================================*
 *
 *  compute the luminance block resulting from motion compensation
 *
 * RETURNS: motionBlock modified
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITION:    the motion vectors must be valid!
 *
 *===========================================================================*/
void
ComputeBMotionLumBlock(MpegFrame * const prev,
                       MpegFrame * const next,
                       int         const by,
                       int         const bx,
                       int         const mode,
                       motion      const motion,
                       LumBlock *  const motionBlockP) {

    switch(mode) {
    case MOTION_FORWARD:
        ComputeMotionLumBlock(prev, by, bx, motion.fwd, motionBlockP);
        break;
    case MOTION_BACKWARD:
        ComputeMotionLumBlock(next, by, bx, motion.bwd, motionBlockP);
        break;
    case MOTION_INTERPOLATE: {
        LumBlock prevBlock, nextBlock;
        unsigned int y;

        ComputeMotionLumBlock(prev, by, bx, motion.fwd, &prevBlock);
        ComputeMotionLumBlock(next, by, bx, motion.bwd, &nextBlock);
        
        for (y = 0; y < 16; ++y) {
            unsigned int x;
            for (x = 0; x < 16; ++x)
                motionBlockP->l[y][x] =
                    (prevBlock.l[y][x] + nextBlock.l[y][x] + 1) / 2;
        }
    } break;
    default:
        pm_error("Bad mode!  Programmer error!");
    }  /* switch */
}


/*===========================================================================*
 *
 *  estimate the seconds to compute a B-frame
 *
 * RETURNS: the time, in seconds
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
float
EstimateSecondsPerBFrame() {
    if (bframeStats.Frames == 0)
        return 20.0;
    else
        return (float)totalTime/((float)TIME_RATE*(float)bframeStats.Frames);
}


