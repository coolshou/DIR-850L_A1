/*===========================================================================*
 * block.c
 *
 *  Block routines
 *
 * NOTES:   MAD =   Mean Absolute Difference
 *===========================================================================*/

/* Copyright information is at end of file */

#include <assert.h>

#include "pm_c_util.h"
#include "all.h"
#include "mtypes.h"
#include "frames.h"
#include "bitio.h"
#include "prototypes.h"
#include "fsize.h"
#include "opts.h"
#include "postdct.h"

#include "block.h"

#define TRUNCATE_UINT8(x)   ((x < 0) ? 0 : ((x > 255) ? 255 : x))

/*==================*
 * GLOBAL VARIABLES *
 *==================*/


extern Block **dct, **dctb, **dctr;


static vector
halfVector(vector const v) {
    vector half;

    half.y = v.y/2;
    half.x = v.x/2;

    return half;
}

/*===========================*
 * COMPUTE DCT OF DIFFERENCE *
 *===========================*/

/*===========================================================================*
 *
 *  compute current-motionBlock, take the DCT, and put the difference
 *  back into current
 *
 * RETURNS: current block modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
ComputeDiffDCTBlock(Block           current,
                    Block           dest,
                    Block           motionBlock,
                    boolean * const significantDifferenceP) {

    unsigned int y;
    int diff;

    diff = 0;  /* initial value */

    for (y = 0; y < 8; ++y) {
        unsigned int x;
        for (x = 0; x < 8; ++x) {
            current[y][x] -= motionBlock[y][x];
            diff += ABS(current[y][x]);
        }
    }
    /* Kill the block if change is too small     */
    /* (block_bound defaults to 128, see opts.c) */
    if (diff < block_bound)
        *significantDifferenceP = FALSE;
    else {
        mp_fwd_dct_block2(current, dest);
        *significantDifferenceP = TRUE;
    }
}



/*===========================================================================*
 *
 *  appropriate (according to pattern, the coded block pattern) blocks
 *  of 'current' are diff'ed and DCT'd.
 *
 * RETURNS: current blocks modified
 *
 * SIDE EFFECTS:    Can remove too-small difference blocks from pattern
 *
 * PRECONDITIONS:   appropriate blocks of 'current' have not yet been
 *          modified
 *
 *===========================================================================*/
void
ComputeDiffDCTs(MpegFrame * const current,
                MpegFrame * const prev,
                int         const by,
                int         const bx,
                vector      const m,
                int *       const patternP) {
    
    Block motionBlock;

    if (collect_quant && (collect_quant_detailed & 1))
        fprintf(collect_quant_fp, "l\n");
    if (*patternP & 0x20) {
        boolean significantDiff;
        ComputeMotionBlock(prev->ref_y, by, bx, m, &motionBlock);
        ComputeDiffDCTBlock(current->y_blocks[by][bx], dct[by][bx],
                            motionBlock, &significantDiff);
        if (!significantDiff)
            *patternP ^= 0x20;
    }

    if (*patternP & 0x10) {
        boolean significantDiff;
        ComputeMotionBlock(prev->ref_y, by, bx+1, m, &motionBlock);
        ComputeDiffDCTBlock(current->y_blocks[by][bx+1], dct[by][bx+1],
                            motionBlock, &significantDiff);
        if (!significantDiff)
            *patternP ^= 0x10;
    }

    if (*patternP & 0x8) {
        boolean significantDiff;
        ComputeMotionBlock(prev->ref_y, by+1, bx, m, &motionBlock);
        ComputeDiffDCTBlock(current->y_blocks[by+1][bx], dct[by+1][bx],
                            motionBlock, &significantDiff);
        if (!significantDiff)
            *patternP ^= 0x8;
    }

    if (*patternP & 0x4) {
        boolean significantDiff;
        ComputeMotionBlock(prev->ref_y, by+1, bx+1, m, &motionBlock);
        ComputeDiffDCTBlock(current->y_blocks[by+1][bx+1], dct[by+1][bx+1],
                            motionBlock, &significantDiff);
        if (!significantDiff)
            *patternP ^= 0x4;
    }

    if (collect_quant && (collect_quant_detailed & 1))
        fprintf(collect_quant_fp, "c\n");

    if (*patternP & 0x2) {
        boolean significantDiff;
        ComputeMotionBlock(prev->ref_cb, by/2, bx/2, halfVector(m),
                           &motionBlock);
        ComputeDiffDCTBlock(current->cb_blocks[by/2][bx/2],
                            dctb[by/2][bx/2], motionBlock,
                            &significantDiff);
        if (!significantDiff)
            *patternP ^= 0x2;
    }

    if (*patternP & 0x1) {
        boolean significantDiff;
        ComputeMotionBlock(prev->ref_cr, by/2, bx/2, halfVector(m),
                           &motionBlock);
        ComputeDiffDCTBlock(current->cr_blocks[by/2][bx/2],
                            dctr[by/2][bx/2], motionBlock,
                            &significantDiff);
        if (!significantDiff)
            *patternP ^= 0x1;
    }
}



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

    assert(*fyP >= 0); assert(*fxP >= 0);

    /* C integer arithmetic rounds toward zero.  But what we need is a
       "floor" -- i.e. round down.  So we adjust now for where the dividend
       in the above divide by two was negative.
    */

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



/*======================*
 * COMPUTE MOTION BLOCK *
 *======================*/

/*===========================================================================*
 *
 *  compute the motion-compensated block
 *
 * RETURNS: motionBlock
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:   motion vector MUST be valid
 *
 * NOTE:  could try to speed this up using halfX, halfY, halfBoth,
 *    but then would have to compute for chrominance, and it's just
 *    not worth the trouble (this procedure is not called relatively
 *    often -- a constant number of times per macroblock)
 *
 *===========================================================================*/
void
ComputeMotionBlock(uint8 ** const prev,
                   int      const by,
                   int      const bx,
                   vector   const m,
                   Block *  const motionBlockP) {

    int fy, fx;
    boolean xHalf, yHalf;

    xHalf = (ABS(m.x) % 2 == 1);
    yHalf = (ABS(m.y) % 2 == 1);

    MotionToFrameCoord(by, bx, m.y/2, m.x/2, &fy, &fx);

    if (xHalf && yHalf) {
        unsigned int y;
        /* really should be fy+y-1 and fy+y so do (fy-1)+y = fy+y-1 and
           (fy-1)+y+1 = fy+y
        */
        if (m.y < 0)
            --fy;

        if (m.x < 0)
            --fx;
    
        for (y = 0; y < 8; ++y) {
            int16 * const destPtr = (*motionBlockP)[y];
            uint8 * const srcPtr  = &(prev[fy+y][fx]);
            uint8 * const srcPtr2 = &(prev[fy+y+1][fx]);
            unsigned int x;

            for (x = 0; x < 8; ++x)
                destPtr[x] =
                    (srcPtr[x]+srcPtr[x+1]+srcPtr2[x]+srcPtr2[x+1]+2) >> 2;
        }
    } else if (xHalf) {
        unsigned int y;
        if (m.x < 0)
            --fx;
        
        for (y = 0; y < 8; ++y) {
            int16 * const destPtr = (*motionBlockP)[y];
            uint8 * const srcPtr  = &(prev[fy+y][fx]);
            unsigned int x;

            for (x = 0; x < 8; ++x)
                destPtr[x] = (srcPtr[x]+srcPtr[x+1]+1) >> 1;
        }
    } else if (yHalf) {
        unsigned int y;
        if ( m.y < 0 )
            fy--;

        for (y = 0; y < 8; ++y) {
            int16 * const destPtr = (*motionBlockP)[y];
            uint8 * const srcPtr  = &(prev[fy+y][fx]);
            uint8 * const srcPtr2 = &(prev[fy+y+1][fx]);
            unsigned int x;

            for (x = 0; x < 8; ++x)
                destPtr[x] = (srcPtr[x]+srcPtr2[x]+1) >> 1;
        }
    } else {
        unsigned int y;
        for (y = 0; y < 8; ++y) {
            int16 * const destPtr = (*motionBlockP)[y];
            uint8 * const srcPtr  = &(prev[fy+y][fx]);
            unsigned int x;

            for (x = 0; x < 8; ++x)
                destPtr[x] = srcPtr[x];
        }
    }
}



/*===========================================================================*
 *
 *  compute the motion-compensated luminance block
 *
 * RETURNS: motionBlock
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:   motion vector MUST be valid
 *
 * NOTE:  see ComputeMotionBlock
 *
 *===========================================================================*/
void
ComputeMotionLumBlock(MpegFrame * const prevFrame,
                      int         const by,
                      int         const bx,
                      vector      const m,
                      LumBlock *  const motionBlockP) {

    unsigned int y;
    uint8 ** prev;
    int fy, fx;

    computePrevFyFx(prevFrame, by, bx, m, &prev, &fy, &fx);

    for (y = 0; y < 16; ++y) {
        uint8 * const across  = &(prev[fy+y][fx]);
        int32 * const macross = motionBlockP->l[y];
        unsigned int x;

        for (x = 0; x < 16; ++x)
            macross[x] = across[x];
    }

    /* this is what's really happening, in slow motion:
     *
     *  for (y = 0; y < 16; ++y, ++py)
     *      for (x = 0; x < 16; ++x, ++px)
     *          motionBlock[y][x] = prev[fy+y][fx+x];
     *
     */
}


/*=======================*
 * BASIC ERROR FUNCTIONS *
 *=======================*/


/*===========================================================================*
 *
 *  return the MAD of two luminance blocks
 *
 * RETURNS: the MAD, if less than bestSoFar, or some number bigger if not
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
int32
LumBlockMAD(const LumBlock * const currentBlockP,
            const LumBlock * const motionBlockP,
            int32            const bestSoFar) {

    int32 diff;    /* max value of diff is 255*256 = 65280 */
    unsigned int y;

    diff = 0;  /* initial value */

    for (y = 0; y < 16; ++y) {
        const int32 * const currentRow = currentBlockP->l[y];
        const int32 * const motionRow  = motionBlockP->l[y];
        unsigned int x;
        for (x = 0; x < 16; ++x)
            diff += ABS(currentRow[x] - motionRow[x]);

        if (diff > bestSoFar)
            /* We already know the MAD won't be less than bestSoFar;
               Caller doesn't care by how much we missed, so just return
               this.
            */
            return diff;
    }
    /* Return the actual MAD */
    return diff;
}



/*===========================================================================*
 *
 *  return the MAD of the currentBlock and the motion-compensated block
 *      (without TUNEing)
 *
 *  (by, bx) is the location of the block in the frame 
 *  (block number coordinates).  'm' is the motion of the block in pixels.
 *  The moved block must be wholly within the frame.
 *
 * RETURNS: the MAD, if less than bestSoFar, or
 *      some number bigger if not
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:  motion vector MUST be valid
 *
 * NOTES:  this is the procedure that is called the most, and should therefore
 *         be the most optimized!!!
 *
 *===========================================================================*/
int32
LumMotionError(const LumBlock * const currentBlockP,
               MpegFrame *      const prevFrame,
               int              const by,
               int              const bx,
               vector           const m,
               int32            const bestSoFar) {

    int32 adiff;
    int32 diff;    /* max value of diff is 255*256 = 65280 */
    int y;
    uint8 **prev;
    int fy, fx;

    computePrevFyFx(prevFrame, by, bx, m, &prev, &fy, &fx);

    assert(fy >= 0); assert(fx >= 0);

    adiff = 0;  /* initial value */
    diff = 0; /* initial value */

    switch (SearchCompareMode) {
    case DEFAULT_SEARCH: /* Default. */
        for (y = 0; y < 16; ++y) {
            const int32 * const cacross = currentBlockP->l[y];
            uint8 *       const across  = &prev[fy+y][fx];
            unsigned int x;
            
            for (x = 0; x < 16; ++x) {
                int32 const localDiff = across[x]-cacross[x];
                diff += ABS(localDiff);
            }
            if (diff > bestSoFar)
                return diff;
        }
        break;
      
    case LOCAL_DCT: {
        Block     dctdiff[4], dctquant[4];
        FlatBlock quant;
        int x, i, tmp;
        int distortion=0, datarate=0;
        int pq = GetPQScale();
      
        for (y = 0;  y < 16;  ++y) {
            const int32 * const cacross = currentBlockP->l[y];
            uint8 * const across = &(prev[fy+y][fx]);
            for (x = 0;  x < 16;  ++x) {
                dctdiff[(x>7)+2*(y>7)][y%8][x%8] = cacross[x]-across[x];
            }}

        /* Calculate rate */
        for (i = 0;  i < 4;  ++i) {
            mp_fwd_dct_block2(dctdiff[i], dctdiff[i]);
            if (Mpost_QuantZigBlock(dctdiff[i], quant, pq, FALSE) ==
                MPOST_ZERO) {
                /* no sense in continuing */
                memset((char *)dctquant[i], 0, sizeof(Block));
            } else {
                Mpost_UnQuantZigBlock(quant, dctquant[i], pq, FALSE);
                mpeg_jrevdct((int16 *)dctquant[i]);
                datarate += CalcRLEHuffLength(quant);
            }
        }
      
        /* Calculate distortion */
        for (y = 0;  y < 16;  ++y) {
            const int32 * const cacross = currentBlockP->l[y];
            uint8 * const across = &(prev[fy+y][fx]);
            for (x = 0;  x < 16;  ++x) {
                tmp = across[x] - cacross[x] +
                    dctquant[(x>7)+2*(y>7)][y%8][x%8];
                distortion += tmp*tmp;
            }}
        distortion /= 256;
        distortion *= LocalDCTDistortScale;
        datarate *= LocalDCTRateScale;
        diff = (int) sqrt(distortion*distortion + datarate*datarate);
        break;
    }

    case NO_DC_SEARCH: {
        extern int32 niqtable[];
        int pq = niqtable[0]*GetPQScale();
        unsigned int y;
        
        for (y = 0; y < 16; ++y) {
            const int32 * const cacross = currentBlockP->l[y];
            uint8 * const across = &(prev[fy+y][fx]);
            unsigned int x;
            
            for (x = 0; x < 16; ++x) {
                int32 const localDiff = across[x]-cacross[x];
                diff += localDiff;
                adiff += ABS(localDiff);
            }
        }

        diff /= 64*pq;  /* diff is now the DC difference (with QSCALE 1) */
        adiff -= 64*pq*ABS(diff);
        diff = adiff;
    }
    break;

    case DO_Mean_Squared_Distortion:
        for (y = 0; y < 16; ++y) {
            const int32 * const cacross = currentBlockP->l[y];
            uint8 * const across = &(prev[fy+y][fx]);
            unsigned int x;

            for (x = 0; x < 16; ++x) {
                int32 const localDiff = across[x] - cacross[x];
                diff += localDiff * localDiff;
            }
            if (diff > bestSoFar)
                return diff;
        }
        break;
    } /* End of Switch */

    return diff;
}



/*===========================================================================*
 *
 *  return the MAD of the currentBlock and the average of the blockSoFar
 *  and the motion-compensated block (this is used for B-frame searches)
 *
 * RETURNS: the MAD, if less than bestSoFar, or
 *      some number bigger if not
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:  motion vector MUST be valid
 *
 *===========================================================================*/
int32
LumAddMotionError(const LumBlock * const currentBlockP,
                  const LumBlock * const blockSoFarP,
                  MpegFrame *      const prevFrame,
                  int              const by,
                  int              const bx,
                  vector           const m,
                  int32            const bestSoFar) {

    int32 diff;    /* max value of diff is 255*256 = 65280 */
    int y;
    uint8 **prev;
    int fy, fx;

    computePrevFyFx(prevFrame, by, bx, m, &prev, &fy, &fx);

    diff = 0; /* initial value */

    /* do we add 1 before dividing by two?  Yes -- see MPEG-1 doc page 46 */

    for (y = 0; y < 16; ++y) {
        unsigned int x;
        const uint8 * const across  = &prev[fy+y][fx];
        const int32 * const bacross = blockSoFarP->l[y];
        const int32 * const cacross = currentBlockP->l[y];

        for (x = 0; x < 16; ++x) {
            int32 const localDiff =
                ((across[x] + bacross[x] + 1) / 2) - cacross[x];
            diff += ABS(localDiff);
        }

        if (diff > bestSoFar)
            return diff;
    }
    
    /* This is what's happening:
     *
     *  ComputeMotionLumBlock(prevFrame, by, bx, my, mx, lumMotionBlock);
     *
     *  for (y = 0; y < 16; ++y)
     *      for (x = 0; x < 16; ++x) {
     *          localDiff = currentBlock[y][x] - lumMotionBlock[y][x];
     *          diff += ABS(localDiff);
     *      }
     */

    return diff;
}



/*===========================================================================*
 *
 *  adds the motion-compensated block to the given block
 *
 * RETURNS: block modified
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:  motion vector MUST be valid
 *
 *===========================================================================*/
void
AddMotionBlock(Block          block,
               uint8 ** const prev,
               int      const by,
               int      const bx,
               vector   const m) {

    int     fy, fx;
    boolean xHalf, yHalf;

    xHalf = (ABS(m.x) % 2 == 1);
    yHalf = (ABS(m.y) % 2 == 1);

    MotionToFrameCoord(by, bx, (m.y/2), (m.x/2), &fy, &fx);

    if (xHalf && yHalf) {
        unsigned int y;
        /* really should be fy+y-1 and fy+y so do (fy-1)+y = fy+y-1 and
           (fy-1)+y+1 = fy+y
        */
        if (m.y < 0)
            --fy;
        if (m.x < 0)
            --fx;

        for (y = 0; y < 8; ++y) {
            unsigned int x;
            for (x = 0; x < 8; ++x)
                block[y][x] += (prev[fy+y][fx+x]+prev[fy+y][fx+x+1]+
                                prev[fy+y+1][fx+x]+prev[fy+y+1][fx+x+1]+2)>>2;
        }
    } else if (xHalf) {
        unsigned int y;
        if (m.x < 0)
            --fx;

        for (y = 0; y < 8; ++y) {
            unsigned int x;
            for (x = 0; x < 8; ++x) {
                block[y][x] += (prev[fy+y][fx+x]+prev[fy+y][fx+x+1]+1)>>1;
            }
        }
    } else if ( yHalf ) {
        unsigned int y;
        if (m.y < 0)
            --fy;

        for (y = 0; y < 8; ++y) {
            unsigned int x;
            for (x = 0; x < 8; ++x) {
                block[y][x] += (prev[fy+y][fx+x]+prev[fy+y+1][fx+x]+1)>>1;
            }
        }
    } else {
        unsigned int y;
        for (y = 0; y < 8; ++y) {
            unsigned int x;
            for (x = 0; x < 8; ++x) {
                block[y][x] += (int16)prev[fy+y][fx+x];
            }
        }
    }
}


/*===========================================================================*
 *
 *  adds the motion-compensated B-frame block to the given block
 *
 * RETURNS: block modified
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:  motion vectors MUST be valid
 *
 *===========================================================================*/
void
AddBMotionBlock(Block          block,
                uint8 ** const prev,
                uint8 ** const next,
                int      const by,
                int      const bx,
                int      const mode,
                motion   const motion) {

    unsigned int y;
    Block prevBlock, nextBlock;

    switch (mode) {
    case MOTION_FORWARD:
        AddMotionBlock(block, prev, by, bx, motion.fwd);
        break;
    case MOTION_BACKWARD:
        AddMotionBlock(block, next, by, bx, motion.bwd);
        break;
    default:
        ComputeMotionBlock(prev, by, bx, motion.fwd, &prevBlock);
        ComputeMotionBlock(next, by, bx, motion.bwd, &nextBlock);
    }
    for (y = 0; y < 8; ++y) {
        unsigned int x;
        for (x = 0; x < 8; ++x) {
            block[y][x] += (prevBlock[y][x] + nextBlock[y][x] + 1) / 2;
        }
    }
}


/*===========================================================================*
 *
 *  copies the given block into the appropriate data area
 *
 * RETURNS: data modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
BlockToData(uint8 ** const data,
            Block          block,
            int      const by,
            int      const bx) {

    int x, y;
    int fy, fx;
    int16    blockItem;

    BLOCK_TO_FRAME_COORD(by, bx, fy, fx);

    for ( y = 0; y < 8; y++ ) {
    for ( x = 0; x < 8; x++ ) {
        blockItem = block[y][x];
        data[fy+y][fx+x] = TRUNCATE_UINT8(blockItem);
    }
    }
}


/*===========================================================================*
 *
 *  copies data into appropriate blocks
 *
 * RETURNS: mf modified
 *
 * SIDE EFFECTS:    none
 *
 * NOTES:  probably shouldn't be in this file
 *
 *===========================================================================*/
void
BlockifyFrame(MpegFrame * const frameP) {

    int dctx, dcty;
    int x, y;
    int bx, by;
    int fy, fx;
    int16 *destPtr;
    uint8 *srcPtr;
    int16 *destPtr2;
    uint8 *srcPtr2;
    Block *blockPtr;
    Block *blockPtr2;

    dctx = Fsize_x / DCTSIZE;
    dcty = Fsize_y / DCTSIZE;

    /*
     * copy y data into y_blocks
     */
    for (by = 0; by < dcty; by++) {
    fy = by*DCTSIZE;
    for (bx = 0; bx < dctx; bx++) {
        fx = bx*DCTSIZE;
        blockPtr = (Block *) &(frameP->y_blocks[by][bx][0][0]);
        for (y = 0; y < DCTSIZE; y++) {
        destPtr = &((*blockPtr)[y][0]);
        srcPtr = &(frameP->orig_y[fy+y][fx]);
        for (x = 0; x < DCTSIZE; x++) {
            destPtr[x] = srcPtr[x];
        }
        }
    }
    }

    /*
     * copy cr/cb data into cr/cb_blocks
     */
    for (by = 0; by < (dcty >> 1); by++) {
    fy = by*DCTSIZE;
    for (bx = 0; bx < (dctx >> 1); bx++) {
        fx = bx*DCTSIZE;
        blockPtr = (Block *) &(frameP->cr_blocks[by][bx][0][0]);
        blockPtr2 = (Block *) &(frameP->cb_blocks[by][bx][0][0]);
        for (y = 0; y < DCTSIZE; y++) {
        destPtr = &((*blockPtr)[y][0]);
        srcPtr = &(frameP->orig_cr[fy+y][fx]);
        destPtr2 = &((*blockPtr2)[y][0]);
        srcPtr2 = &(frameP->orig_cb[fy+y][fx]);
        for (x = 0; x < DCTSIZE; x++) {
            destPtr[x] = srcPtr[x];
            destPtr2[x] = srcPtr2[x];
        }
        }
    }
    }
}


/*===========================================================================*
 *                                       *
 * UNUSED PROCEDURES                                 *
 *                                       *
 *  The following procedures are all unused by the encoder           *
 *                                       *
 *  They are listed here for your convenience.  You might want to use    *
 *  them if you experiment with different search techniques          *
 *                                       *
 *===========================================================================*/

#ifdef UNUSED_PROCEDURES

/* this procedure calculates the subsampled motion block (obviously)
 *
 * for speed, this procedure is probably not called anywhere (it is
 * incorporated directly into LumDiffA, LumDiffB, etc.
 *
 * but leave it here anyway for clarity
 *
 * (startY, startX) = (0,0) for A....(0,1) for B...(1,0) for C...(1,1) for D
 *  
 */
void
ComputeSubSampledMotionLumBlock(MpegFrame * const prevFrame,
                                int         const by,
                                int         const bx,
                                int         const my,
                                int         const mx,
                                LumBlock    const motionBlock,
                                int         const startY,
                                int         const startX) {

    uint8 *across;
    int32 *macross;
    int32 *lastx;
    int y;
    uint8 **prev;
    int    fy, fx;
    boolean xHalf, yHalf;

    xHalf = (ABS(mx) % 2 == 1);
    yHalf = (ABS(my) % 2 == 1);

    MotionToFrameCoord(by, bx, my/2, mx/2, &fy, &fx);

    if ( xHalf ) {
    if ( mx < 0 ) {
        fx--;
    }

    if ( yHalf ) {
        if ( my < 0 ) {
        fy--;
        }
        
        prev = prevFrame->halfBoth;
    } else {
        prev = prevFrame->halfX;
    }
    } else if ( yHalf ) {
    if ( my < 0 ) {
        fy--;
    }

    prev = prevFrame->halfY;
    } else {
    prev = prevFrame->ref_y;
    }

    for ( y = startY; y < 16; y += 2 ) {
    across = &(prev[fy+y][fx+startX]);
    macross = &(motionBlock[y][startX]);
    lastx = &(motionBlock[y][16]);
    while ( macross < lastx ) {
        (*macross) = (*across);
        across += 2;
        macross += 2;
    }
    }

    /* this is what's really going on in slow motion:
     *
     *  for ( y = startY; y < 16; y += 2 )
     *      for ( x = startX; x < 16; x += 2 )
     *      motionBlock[y][x] = prev[fy+y][fx+x];
     *
     */
}


/*===========================================================================*
 *
 *  return the MAD of the currentBlock and the motion-compensated block,
 *  subsampled 4:1 with given starting coordinates (startY, startX)
 *
 * RETURNS: the MAD
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:  motion vector MUST be valid
 *
 * NOTES:  this procedure is never called.  Instead, see subsample.c.  This
 *         procedure is provided only for possible use in extensions
 *
 *===========================================================================*/
int32
LumMotionErrorSubSampled(LumBlock    const currentBlock,
                         MpegFrame * const prevFrame,
                         int         const by,
                         int         const bx,
                         int         const my,
                         int         const mx,
                         int         const startY,
                         int         const startX) {

    int32 diff;     /* max value of diff is 255*256 = 65280 */
    int32 localDiff;
    int32 *cacross;
    uint8 *macross;
    int32 *lastx;
    int y;
    uint8 **prev;
    int    fy, fx;
    boolean xHalf, yHalf;

    xHalf = (ABS(mx) % 2 == 1);
    yHalf = (ABS(my) % 2 == 1);

    motionToFrameCoord(by, bx, my/2, mx/2, &fy, &fx);

    if ( xHalf ) {
    if ( mx < 0 ) {
        fx--;
    }

    if ( yHalf ) {
        if ( my < 0 ) {
        fy--;
        }
        
        prev = prevFrame->halfBoth;
    } else {
        prev = prevFrame->halfX;
    }
    } else if ( yHalf ) {
    if ( my < 0 ) {
        fy--;
    }

    prev = prevFrame->halfY;
    } else {
    prev = prevFrame->ref_y;
    }

    diff = 0; /* initial value */

    for ( y = startY; y < 16; y += 2 ) {
    macross = &(prev[fy+y][fx+startX]);
    cacross = &(currentBlock[y][startX]);
    lastx = &(currentBlock[y][16]);
    while ( cacross < lastx ) {
        localDiff = (*cacross)-(*macross);
        diff += ABS(localDiff);
        macross += 2;
        cacross += 2;
    }
    }

    /* this is what's really happening:
     *
     *  ComputeSubSampledMotionLumBlock(prevFrame, by, bx, my, mx,
     *                  lumMotionBlock, startY, startX);
     *
     *  for ( y = startY; y < 16; y += 2 )
     *      for ( x = startX; x < 16; x += 2 )
     *      {
     *          localDiff = currentBlock[y][x] - lumMotionBlock[y][x];
     *      diff += ABS(localDiff);
     *      }
     *
     */

    return (int32)diff;
}


#endif /* UNUSED_PROCEDURES */
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

