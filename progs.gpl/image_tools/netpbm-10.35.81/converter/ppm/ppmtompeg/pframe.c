/*===========================================================================*
 * pframe.c                                  
 *                                       
 *  Procedures concerned with generation of P-frames             
 *                                       
 * EXPORTED PROCEDURES:                              
 *  GenPFrame                                
 *  ResetPFrameStats                             
 *  ShowPFrameSummary                            
 *  EstimateSecondsPerPFrame                         
 *  ComputeHalfPixelData                             
 *  SetPQScale                               
 *  GetPQScale                               
 *                                                                           
 * NOTE:  when motion vectors are passed as arguments, they are passed as    
 *        twice their value.  In other words, a motion vector of (3,4) will  
 *        be passed as (6,8).  This allows half-pixel motion vectors to be   
 *        passed as integers.  This is true throughout the program.          
 *                                       
 *===========================================================================*/

/*==============*
 * HEADER FILES *
 *==============*/

#include <assert.h>
#include <sys/param.h>
#include "pm.h"
#include "pm_c_util.h"
#include "all.h"
#include "mtypes.h"
#include "bitio.h"
#include "frames.h"
#include "motion_search.h"
#include "prototypes.h"
#include "block.h"
#include "param.h"
#include "mheaders.h"
#include "fsize.h"
#include "postdct.h"
#include "mpeg.h"
#include "parallel.h"
#include "rate.h"
#include "opts.h"
#include "specifics.h"

/*==================*
 * STATIC VARIABLES *
 *==================*/

static int32    zeroDiff;
static int      numPIBlocks = 0;
static int      numPPBlocks = 0;
static int      numPSkipped = 0;
static int      numPIBits = 0;
static int      numPPBits = 0;
static int      numFrames = 0;
static int      numFrameBits = 0;
static int32    totalTime = 0;
static int      qscaleP;
static float    totalSNR = 0.0;
static float    totalPSNR = 0.0;
extern Block    **dct, **dctr, **dctb;
extern dct_data_type   **dct_data;

/*=====================*
 * INTERNAL PROCEDURES *
 *=====================*/

static vector
halfVector(vector const vector) {
    struct vector half;

    half.y = vector.y/2;
    half.x = vector.x/2;

    return half;
}

/*===========================================================================*
 *
 *  decide if (0,0) motion is better than the given motion vector
 *
 * RETURNS: TRUE if (0,0) is better, FALSE if (my,mx) is better
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:   The relevant block in 'current' is valid (it has not
 *          been dct'd).  'zeroDiff' has already been computed
 *          as the LumMotionError() with (0,0) motion
 *
 * NOTES:   This procedure follows the algorithm described on
 *      page D-48 of the MPEG-1 specification
 *
 *===========================================================================*/
static boolean
ZeroMotionBetter(const LumBlock * const currentBlockP,
                 MpegFrame *      const prev,
                 int              const by,
                 int              const bx,
                 vector           const m) {

    int bestDiff;
    int CompareMode;

    /* Junk needed to adapt for TUNEing */ 
    CompareMode = SearchCompareMode;
    SearchCompareMode = DEFAULT_SEARCH;
    bestDiff = LumMotionError(currentBlockP, prev, by, bx, m, 0x7fffffff);
    SearchCompareMode = CompareMode;

    if ( zeroDiff < 256*3 ) {
    if ( 2*bestDiff >= zeroDiff ) {
        return TRUE;
    }
    } else {
    if ( 11*bestDiff >= 10*zeroDiff ) {
        return TRUE;
    }
    }
    return FALSE;
}


/*===========================================================================*
 *
 *                USER-MODIFIABLE
 *
 * DoIntraCode
 *
 *  decide if intra coding is necessary
 *
 * RETURNS: TRUE if intra-block coding is better; FALSE if not
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:   The relevant block in 'current' is valid (it has not
 *          been dct'd).
 *
 * NOTES:   This procedure follows the algorithm described on
 *      page D-49 of the MPEG-1 specification
 *
 *===========================================================================*/
static boolean
DoIntraCode(const LumBlock * const currentBlockP,
            MpegFrame *      const prev,
            int              const by,
            int              const bx,
            vector           const motion) {

    unsigned int y;
    int32 sum = 0, vard = 0, varc = 0;
    int32 currPixel, prevPixel;
    LumBlock motionBlock;

    ComputeMotionLumBlock(prev, by, bx, motion, &motionBlock);

    for (y = 0; y < 16; ++y) {
        unsigned int x;
        for (x = 0; x < 16; ++x) {
            currPixel = currentBlockP->l[y][x];
            prevPixel = motionBlock.l[y][x];
            
            sum += currPixel;
            varc += currPixel*currPixel;
            
            vard += SQR(currPixel - prevPixel);
        }
    }
    
    vard /= 256;     /* divide by 256; assumes mean is close to zero */
    varc = (varc/256) - (sum/256) * (sum/256);

    if (vard <= 64)
        return FALSE;
    else if (vard < varc)
        return FALSE;
    else
        return TRUE;
}



/*===========================================================================*
 *
 *                USER-MODIFIABLE
 *
 * ZeroMotionSufficient
 *
 *  decide if zero motion is sufficient without DCT correction
 *
 * RETURNS: TRUE no DCT required; FALSE otherwise
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:   The relevant block in 'current' is raw YCC data
 *
 *===========================================================================*/
static boolean
ZeroMotionSufficient(const LumBlock * const currentBlockP,
                     MpegFrame *      const prev,
                     int              const by,
                     int              const bx) {

    LumBlock motionBlock;
    int fy, fx;
    unsigned int y;

    fy = by * DCTSIZE;
    fx = bx * DCTSIZE;
    for (y = 0; y < 16; ++y) {
        unsigned int x;
        for (x = 0; x < 16; ++x) {
            motionBlock.l[y][x] = prev->ref_y[fy+y][fx+x];
        }
    }

    zeroDiff = LumBlockMAD(currentBlockP, &motionBlock, 0x7fffffff);

    return (zeroDiff <= 256);
}
                 


static void 
computeCurrentBlock(MpegFrame * const current, 
                    int         const y,
                    int         const x,
                    LumBlock *  const currentBlockP) {
    int fy, fx;
    int iy;

    BLOCK_TO_FRAME_COORD(y, x, fy, fx);
    for ( iy = 0; iy < 16; iy++ ) {
        int ix;
        for ( ix = 0; ix < 16; ix++ ) {
            currentBlockP->l[iy][ix] = 
                (int16)current->orig_y[fy+iy][fx+ix];
        }
    }
}



static void
computeMotionVectors(bool             const specificsOn, 
                     bool             const IntraPBAllowed,
                     MpegFrame *      const current, 
                     MpegFrame *      const prev,
                     int              const mbAddress,
                     BlockMV **       const infoP,
                     int              const QScale, 
                     const LumBlock * const currentBlockP,
                     int              const y, 
                     int              const x,
                     bool *           const useMotionP, 
                     vector *         const motionP) {

    bool useCached;
    BlockMV * info;

    /* See if we have a cached answer */
    if (specificsOn) {
        SpecLookup(current->id, 2, mbAddress, &info, QScale);
        if (info != (BlockMV*)NULL) 
            useCached = TRUE;
        else
            useCached = FALSE;
    } else
        useCached = FALSE;

    if (useCached) {
        if (info->typ == TYP_SKIP) {
            motionP->x = motionP->y = 0;
            *useMotionP = TRUE;
        } else {        /* assume P, since we're a P frame.... */
            motionP->x = info->fx;
            motionP->y = info->fy;
            *useMotionP = TRUE;
        }
    } else {
        /* see if we should use motion vectors, and if so, what those
         * vectors should be
         */
        if (ZeroMotionSufficient(currentBlockP, prev, y, x)) {
            motionP->x = 0;
            motionP->y = 0;
            *useMotionP = TRUE;
        } else {
            vector motion;
            motion.y = motion.x = 0;  /* initial values */
            PMotionSearch(currentBlockP, prev, y, x, &motion);
            if (ZeroMotionBetter(currentBlockP, prev, y, x, motion)) {
                motionP->y = 0;
                motionP->x = 0;
            } else
                *motionP = motion;
            if (IntraPBAllowed) 
                *useMotionP = !DoIntraCode(currentBlockP, prev, y, x, motion);
            else
                *useMotionP = TRUE;
        }
    }
    *infoP = info;
}



static void
calculateForwardDcts(MpegFrame * const current, 
                     int const y, int const x,
                     Block ** const dct) {

    /* calculate forward dct's */
    if (collect_quant && (collect_quant_detailed & 1)) 
        fprintf(collect_quant_fp, "l\n");

    mp_fwd_dct_block2(current->y_blocks[y][x], dct[y][x]);
    mp_fwd_dct_block2(current->y_blocks[y][x+1], dct[y][x+1]);
    mp_fwd_dct_block2(current->y_blocks[y+1][x], dct[y+1][x]);
    mp_fwd_dct_block2(current->y_blocks[y+1][x+1], dct[y+1][x+1]);

    if (collect_quant && (collect_quant_detailed & 1)) 
        fprintf(collect_quant_fp, "c\n");

    mp_fwd_dct_block2(current->cb_blocks[y/2][x/2], dctb[y/2][x/2]);

    mp_fwd_dct_block2(current->cr_blocks[y/2][x/2], dctr[y/2][x/2]);
}



static void
computeMotionAndDct(int         const lastBlockY,
                    int         const lastBlockX,
                    bool        const specificsOn,
                    bool        const IntraPBAllowed,
                    MpegFrame * const current,
                    MpegFrame * const prev,
                    BlockMV **  const infoP,
                    int         const QScale,
                    int         const searchRangeP,
                    Block **    const dct,
                    int *       const numPBlocksP,
                    int *       const numIBlocksP,
                    int **      const pmvHistogram) {
/*----------------------------------------------------------------------------
   Loop through the frame finding motion/not and DCTing
-----------------------------------------------------------------------------*/
    int mbAddress;
    int y;

    mbAddress = 0;                        

    for (y = 0; y < lastBlockY; y += 2) {
        int x;
        for (x = 0; x < lastBlockX; x += 2) {
            LumBlock currentBlock;
            vector motion;
            bool useMotion;

            computeCurrentBlock(current, y, x, &currentBlock);

            computeMotionVectors(specificsOn,  IntraPBAllowed,
                                 current, prev, mbAddress, infoP,
                                 QScale, &currentBlock, y, x,
                                 &useMotion, &motion);

            dct_data[y][x].useMotion = useMotion;

            if (useMotion) {
                int pattern;

                (*numPBlocksP)++;

                pattern = 63;
                ComputeDiffDCTs(current, prev, y, x, motion, &pattern);

                assert(motion.x + searchRangeP + 1 >= 0);
                assert(motion.y + searchRangeP + 1 >= 0);

                if (computeMVHist) {
                    assert(motion.x + searchRangeP + 1 <= 2*searchRangeP + 2);
                    assert(motion.y + searchRangeP + 1 <= 2*searchRangeP + 2);
                    ++pmvHistogram[motion.x + searchRangeP + 1]
                        [motion.y + searchRangeP + 1];
                }
                /* Save specs for next loops */
                dct_data[y][x].pattern  = pattern;
                dct_data[y][x].fmotionX = motion.x;
                dct_data[y][x].fmotionY = motion.y;
            } else {
                /* output I-block inside a P-frame */
                ++*numIBlocksP;

                calculateForwardDcts(current, y, x, dct);
            }
            ++mbAddress;
        }
    }
}



/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 * GenPFrame
 *
 *  generate a P-frame from previous frame, adding the result to the
 *  given bit bucket
 *
 * RETURNS: frame appended to bb
 *
 *===========================================================================*/
void
GenPFrame(BitBucket * const bb,
          MpegFrame * const current, 
          MpegFrame * const prev) {

    extern int **pmvHistogram;
    FlatBlock fba[6], fb[6];
    Block   dec[6];
    int32 y_dc_pred, cr_dc_pred, cb_dc_pred;
    int x, y;
    vector motion;
    vector oldMotion;
    int offsetX, offsetY;
    vector motionRem;
    vector motionQuot;
    int pattern;
    int mbAddrInc = 1;
    int numIBlocks = 0;
    int numPBlocks = 0;
    int numSkipped = 0;
    int numIBits = 0;
    int numPBits = 0;
    int totalBits;
    int totalFrameBits;
    int32    startTime, endTime;
    int lastBlockX, lastBlockY;
    int lastX, lastY;
    int mbAddress;
    int slicePos;
    register int index;
    float   snr[3], psnr[3];
    int QScale;
    BlockMV *info;
    int bitstreamMode, newQScale;
    int rc_blockStart = 0;
    boolean overflowChange = FALSE;
    int     overflowValue  = 0;


    oldMotion.x = oldMotion.y = 0;

    if (collect_quant) {fprintf(collect_quant_fp, "# P\n");}
    if (dct==NULL) AllocDctBlocks();
    numFrames++;
    totalFrameBits = bb->cumulativeBits;
    startTime = time_elapsed();

    DBG_PRINT(("Generating pframe\n"));

    QScale = GetPQScale();
    /*   bit allocation for rate control purposes */
    bitstreamMode = getRateMode();
    if (bitstreamMode == FIXED_RATE) {
        targetRateControl(current);
    }
 
    Mhead_GenPictureHeader(bb, P_FRAME, current->id, fCodeP);
    /* Check for Qscale change */  
    if (specificsOn) {
        /* Set a Qscale for this frame? */
        newQScale = 
            SpecLookup(current->id, 0, 0 /* junk */, &info /*junk*/, QScale);
        if (newQScale != -1) {
            QScale = newQScale;
        }
        /* Set for slice? */
        newQScale = SpecLookup(current->id, 1, 1, &info /*junk*/, QScale);
        if (newQScale != -1) {
            QScale = newQScale;
        }
    }

    DBG_PRINT(("Slice Header\n"));
    Mhead_GenSliceHeader(bb, 1, QScale, NULL, 0);

    if ( referenceFrame == DECODED_FRAME ) {
        Frame_AllocDecoded(current, TRUE);
    } else if ( printSNR ) {
        Frame_AllocDecoded(current, FALSE);
    }

    /* don't do dct on blocks yet */
    Frame_AllocBlocks(current);
    BlockifyFrame(current);

    /* for I-blocks */
    y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

    totalBits = bb->cumulativeBits;

    if ( (! pixelFullSearch) && (! prev->halfComputed) ) {
        ComputeHalfPixelData(prev);
    }

    lastBlockX = Fsize_x>>3;
    lastBlockY = Fsize_y>>3;
    lastX = lastBlockX-2;
    lastY = lastBlockY-2;

    computeMotionAndDct(lastBlockY, lastBlockX,
                        specificsOn, IntraPBAllowed, current, prev,
                        &info, QScale, searchRangeP, dct, 
                        &numPBlocks, &numIBlocks, pmvHistogram);
    
    mbAddress = 0;
    for (y = 0; y < lastBlockY; y += 2) {
        for (x = 0; x < lastBlockX; x += 2) {
            slicePos = (mbAddress % blocksPerSlice);

            if ( (slicePos == 0) && (mbAddress != 0) ) {
                if (specificsOn) {
                    /* Make sure no slice Qscale change */
                    newQScale = 
                        SpecLookup(current->id, 1, mbAddress/blocksPerSlice,
                                   &info /*junk*/, QScale);
                    if (newQScale != -1) QScale = newQScale;
                }

                Mhead_GenSliceEnder(bb);
                Mhead_GenSliceHeader(bb, 1+(y>>1), QScale, NULL, 0);

                /* reset everything */
                oldMotion.x = oldMotion.y = 0;
                y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

                mbAddrInc = 1+(x>>1);
            }
        
            /*  Determine if new Qscale needed for Rate Control purposes  */
            if (bitstreamMode == FIXED_RATE) {
                rc_blockStart =  bb->cumulativeBits;
                newQScale = needQScaleChange(qscaleP,
                                             current->y_blocks[y][x],
                                             current->y_blocks[y][x+1],
                                             current->y_blocks[y+1][x],
                                             current->y_blocks[y+1][x+1]);
                if (newQScale > 0) {
                    QScale = newQScale;
                }
            }
        
            /* Check for Qscale change */
            if (specificsOn) {
                newQScale = 
                    SpecLookup(current->id, 2, mbAddress, &info, QScale);
                if (newQScale != -1) {
                    QScale = newQScale;
                }
            }

            if (! dct_data[y][x].useMotion) {
                GEN_I_BLOCK(P_FRAME, current, bb, mbAddrInc, QScale);
                mbAddrInc = 1;

                numIBits += (bb->cumulativeBits-totalBits);
                totalBits = bb->cumulativeBits;

                /* reset because intra-coded */
                oldMotion.x = oldMotion.y = 0;

                if ( decodeRefFrames ) {
                    /* need to decode block we just encoded */
                    Mpost_UnQuantZigBlock(fb[0], dec[0], QScale, TRUE);
                    Mpost_UnQuantZigBlock(fb[1], dec[1], QScale, TRUE);
                    Mpost_UnQuantZigBlock(fb[2], dec[2], QScale, TRUE);
                    Mpost_UnQuantZigBlock(fb[3], dec[3], QScale, TRUE);
                    Mpost_UnQuantZigBlock(fb[4], dec[4], QScale, TRUE);
                    Mpost_UnQuantZigBlock(fb[5], dec[5], QScale, TRUE);

                    /* now, reverse the DCT transform */
                    for ( index = 0; index < 6; index++ ) {
                        mpeg_jrevdct((int16 *)dec[index]);
                    }

                    /* now, unblockify */
                    BlockToData(current->decoded_y, dec[0], y, x);
                    BlockToData(current->decoded_y, dec[1], y, x+1);
                    BlockToData(current->decoded_y, dec[2], y+1, x);
                    BlockToData(current->decoded_y, dec[3], y+1, x+1);
                    BlockToData(current->decoded_cb, dec[4], y>>1, x>>1);
                    BlockToData(current->decoded_cr, dec[5], y>>1, x>>1);
                }
            } else {
                int fCode = fCodeP;

                /* reset because non-intra-coded */
                y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

                pattern = dct_data[y][x].pattern;
                motion.x = dct_data[y][x].fmotionX;
                motion.y = dct_data[y][x].fmotionY;

#ifdef BLEAH
                ComputeAndPrintPframeMAD(currentBlock, prev, y, x, motion, 
                                         mbAddress);
#endif

                if ( pixelFullSearch ) { /* should be even */
                    motion.y /= 2;
                    motion.x /= 2;
                }

                /* transform the motion vector into the appropriate values */
                offsetX = motion.x - oldMotion.x;
                offsetY = motion.y - oldMotion.y;
                /*  if ((offsetX+(8*x)) >= (Fsize_x-8)) log(10.0); */
                encodeMotionVector(offsetX, offsetY, &motionQuot, &motionRem,
                                   FORW_F, fCode);

#ifdef BLEAH
                if ( (motion.x != 0) || (motion.y != 0) ) {
                    fprintf(stdout, "FRAME (y, x)  %d, %d (block %d)\n", 
                            y, x, mbAddress);
                    fprintf(stdout, "motion.x = %d, motion.y = %d\n", 
                            motion.x, motion.y);
                    fprintf(stdout, 
                            "    mxq, mxr = %d, %d    myq, myr = %d, %d\n",
                            motionQuot.x, motionRem.x,
                            motionQuot.y, motionRem.y);
                }
#endif

                oldMotion = motion;

                if ( pixelFullSearch ) { /* reset for use with PMotionSearch */
                    motion.y *= 2;
                    motion.x *= 2;
                }
            calc_blocks:
                /* create flat blocks and update pattern if necessary */
                /* Note DoQuant references QScale, overflowChange,
                   overflowValue, pattern, and the calc_blocks label */
                DoQuant(0x20, dct[y][x], fba[0]);
                DoQuant(0x10, dct[y][x+1], fba[1]);
                DoQuant(0x08, dct[y+1][x], fba[2]);
                DoQuant(0x04, dct[y+1][x+1], fba[3]);
                DoQuant(0x02, dctb[y/2][x/2], fba[4]);
                DoQuant(0x01, dctr[y/2][x/2], fba[5]);

                if ( decodeRefFrames) {
                    for ( index = 0; index < 6; index++ ) {
                        if ( pattern & (1 << (5-index))) {
                            Mpost_UnQuantZigBlock(fba[index], dec[index], 
                                                  QScale, FALSE);
                            mpeg_jrevdct((int16 *)dec[index]);
                        } else {
                            memset((char *)dec[index], 0, sizeof(Block));
                        }
                    }

                    /* now add the motion block */
                    AddMotionBlock(dec[0], prev->decoded_y, y, x, motion);
                    AddMotionBlock(dec[1], prev->decoded_y, y, x+1, motion);
                    AddMotionBlock(dec[2], prev->decoded_y, y+1, x, motion);
                    AddMotionBlock(dec[3], prev->decoded_y, y+1, x+1, motion);
                    AddMotionBlock(dec[4], prev->decoded_cb, y/2, x/2,
                                   halfVector(motion));
                    AddMotionBlock(dec[5], prev->decoded_cr, y/2, x/2, 
                                   halfVector(motion));

                    /* now, unblockify */
                    BlockToData(current->decoded_y, dec[0], y, x);
                    BlockToData(current->decoded_y, dec[1], y, x+1);
                    BlockToData(current->decoded_y, dec[2], y+1, x);
                    BlockToData(current->decoded_y, dec[3], y+1, x+1);
                    BlockToData(current->decoded_cb, dec[4], y/2, x/2);
                    BlockToData(current->decoded_cr, dec[5], y/2, x/2);
                } 

                if ((motion.x == 0) && (motion.y == 0)) {
                    if ( pattern == 0 ) {
                        /* can only skip if:
                         *     1)  not the last block in frame
                         *     2)  not the last block in slice
                         *     3)  not the first block in slice
                         */

                        if ( ((y < lastY) || (x < lastX)) &&
                             (slicePos+1 != blocksPerSlice) &&
                             (slicePos != 0) ) {
                            mbAddrInc++;    /* skipped macroblock */
                            numSkipped++;
                            numPBlocks--;
                        } else {        /* first/last macroblock */
                            Mhead_GenMBHeader(bb, 2 /* pict_code_type */, 
                                              mbAddrInc /* addr_incr */,
                                              QScale /* q_scale */,
                                              fCode /* forw_f_code */, 
                                              1 /* back_f_code */,
                                              motionRem.x /* horiz_forw_r */, 
                                              motionRem.y /* vert_forw_r */,
                                              0 /* horiz_back_r */, 
                                              0 /* vert_back_r */,
                                              1 /* motion_forw */, 
                                              motionQuot.x /* m_horiz_forw */,
                                              motionQuot.y /* m_vert_forw */, 
                                              0 /* motion_back */,
                                              0 /* m_horiz_back */, 
                                              0 /* m_vert_back */,
                                              0 /* mb_pattern */, 
                                              0 /* mb_intra */);
                            mbAddrInc = 1;
                        }
                    } else {
                        DBG_PRINT(("MB Header(%d,%d)\n", x, y));
                        Mhead_GenMBHeader(bb, 2 /* pict_code_type */, 
                                          mbAddrInc /* addr_incr */,
                                          QScale /* q_scale */,
                                          fCode /* forw_f_code */, 
                                          1 /* back_f_code */,
                                          0 /* horiz_forw_r */, 
                                          0 /* vert_forw_r */,
                                          0 /* horiz_back_r */, 
                                          0 /* vert_back_r */,
                                          0 /* motion_forw */, 
                                          0 /* m_horiz_forw */,
                                          0 /* m_vert_forw */, 
                                          0 /* motion_back */,
                                          0 /* m_horiz_back */, 
                                          0 /* m_vert_back */,
                                          pattern /* mb_pattern */, 
                                          0 /* mb_intra */);
                        mbAddrInc = 1;
                    }
                } else {
                    /*      DBG_PRINT(("MB Header(%d,%d)\n", x, y));  */
          
                    Mhead_GenMBHeader(bb, 2 /* pict_code_type */, 
                                      mbAddrInc /* addr_incr */,
                                      QScale /* q_scale */,
                                      fCode /* forw_f_code */, 
                                      1 /* back_f_code */,
                                      motionRem.x /* horiz_forw_r */, 
                                      motionRem.y /* vert_forw_r */,
                                      0 /* horiz_back_r */, 
                                      0 /* vert_back_r */,
                                      1 /* motion_forw */, 
                                      motionQuot.x /* m_horiz_forw */,
                                      motionQuot.y /* m_vert_forw */, 
                                      0 /* motion_back */,
                                      0 /* m_horiz_back */, 
                                      0 /* m_vert_back */,
                                      pattern /* mb_pattern */, 
                                      0 /* mb_intra */);
                    mbAddrInc = 1;
                }

                /* now output the difference */
                {
                    unsigned int x;
                    for (x = 0; x < 6; ++x) {
                        if (GET_ITH_BIT(pattern, 5-x))
                            Mpost_RLEHuffPBlock(fba[x], bb);
                    }
                }
                numPBits += (bb->cumulativeBits-totalBits);
                totalBits = bb->cumulativeBits;
            }

            if (overflowChange) {
                /* undo an overflow-caused Qscale change */
                overflowChange = FALSE;
                QScale -= overflowValue;
                overflowValue = 0;
            }

            mbAddress++;
            /*   Rate Control  */
            if (bitstreamMode == FIXED_RATE) {
                incMacroBlockBits( bb->cumulativeBits- rc_blockStart);
                rc_blockStart = bb->cumulativeBits;
                MB_RateOut(TYPE_PFRAME);
            }
        }
    }

    if ( printSNR ) {
        BlockComputeSNR(current,snr,psnr);
        totalSNR += snr[0];
        totalPSNR += psnr[0];
    }

#ifdef BLEAHBLEAH
    {
        FILE *filePtr;

        filePtr = fopen("PFRAME.yuv", "wb");

        for ( y = 0; y < Fsize_y; y++ )
        {
            for ( x = 0; x < Fsize_x; x++ )
                fprintf(filePtr, "%d ", current->decoded_y[y][x]);
            fprintf(filePtr, "\n");
        }

        fclose(filePtr);
    }
#endif

    Mhead_GenSliceEnder(bb);
    /*   Rate Control */
    if (bitstreamMode == FIXED_RATE) {
        updateRateControl(TYPE_PFRAME);
    }

    /* UPDATE STATISTICS */
    endTime = time_elapsed();
    totalTime += (endTime-startTime);

    if ( showBitRatePerFrame ) {
        /* ASSUMES 30 FRAMES PER SECOND */
        fprintf(bitRateFile, "%5d\t%8d\n", current->id,
                30*(bb->cumulativeBits-totalFrameBits));
    }

    if ( frameSummary && (! realQuiet) ) {
        fprintf(stdout, "FRAME %d (P):  I BLOCKS:  %d;  "
                "P BLOCKS:  %d   SKIPPED:  %d  (%ld seconds)\n",
                current->id, numIBlocks, numPBlocks, numSkipped, 
                (long)(endTime-startTime)/TIME_RATE);
        if ( printSNR ) {
            fprintf(stdout, "FRAME %d:  SNR:  %.1f\t%.1f\t%.1f\t"
                    "PSNR:  %.1f\t%.1f\t%.1f\n",
                    current->id, snr[0], snr[1], snr[2],
                    psnr[0], psnr[1], psnr[2]);
        }
    }

    numFrameBits += (bb->cumulativeBits-totalFrameBits);
    numPIBlocks += numIBlocks;
    numPPBlocks += numPBlocks;
    numPSkipped += numSkipped;
    numPIBits += numIBits;
    numPPBits += numPBits;
}


/*===========================================================================*
 *
 * ResetPFrameStats
 *
 *  reset the P-frame statistics
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
  ResetPFrameStats()
{
  numPIBlocks = 0;
  numPPBlocks = 0;
  numPSkipped = 0;
  numPIBits = 0;
  numPPBits = 0;
  numFrames = 0;
  numFrameBits = 0;
  totalTime = 0;
}


/*===========================================================================*
 *
 * SetPQScale
 *
 *  set the P-frame Q-scale
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    qscaleP
 *
 *===========================================================================*/
void
  SetPQScale(qP)
int qP;
{
  qscaleP = qP;
}


/*===========================================================================*
 *
 * GetPQScale
 *
 *  return the P-frame Q-scale
 *
 * RETURNS: the P-frame Q-scale
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
int
  GetPQScale()
{
  return qscaleP;
}


float
PFrameTotalTime(void) {
    return (float)totalTime/(float)TIME_RATE;
}



void
ShowPFrameSummary(unsigned int const inputFrameBits, 
                  unsigned int const totalBits, 
                  FILE *       const fpointer) {

    if (numFrames > 0) {

        fprintf(fpointer, "-------------------------\n");
        fprintf(fpointer, "*****P FRAME SUMMARY*****\n");
        fprintf(fpointer, "-------------------------\n");

        if ( numPIBlocks != 0 ) {
            fprintf(fpointer, "  I Blocks:  %5d     (%6d bits)     (%5d bpb)\n",
                    numPIBlocks, numPIBits, numPIBits/numPIBlocks);
        } else {
            fprintf(fpointer, "  I Blocks:  %5d\n", 0);
        }

        if ( numPPBlocks != 0 ) {
            fprintf(fpointer, "  P Blocks:  %5d     (%6d bits)     (%5d bpb)\n",
                    numPPBlocks, numPPBits, numPPBits/numPPBlocks);
        } else {
            fprintf(fpointer, "  P Blocks:  %5d\n", 0);
        }

        fprintf(fpointer, "  Skipped:   %5d\n", numPSkipped);

        fprintf(fpointer, "  Frames:    %5d     (%6d bits)     (%5d bpf)     (%2.1f%% of total)\n",
                numFrames, numFrameBits, numFrameBits/numFrames,
                100.0*(float)numFrameBits/(float)totalBits);
        fprintf(fpointer, "  Compression:  %3d:1     (%9.4f bpp)\n",
                numFrames*inputFrameBits/numFrameBits,
                24.0*(float)numFrameBits/(float)(numFrames*inputFrameBits));
        if ( printSNR )
            fprintf(fpointer, "  Avg Y SNR/PSNR:  %.1f     %.1f\n",
                    totalSNR/(float)numFrames, totalPSNR/(float)numFrames);
        if ( totalTime == 0 ) {
            fprintf(fpointer, "  Seconds:  NONE\n");
        } else {
            fprintf(fpointer, "  Seconds:  %9ld     (%9.4f fps)  (%9ld pps)  (%9ld mps)\n",
                    (long)(totalTime/TIME_RATE),
                    (float)((float)(TIME_RATE*numFrames)/(float)totalTime),
                    (long)((float)TIME_RATE*(float)numFrames*(float)inputFrameBits/(24.0*(float)totalTime)),
                    (long)((float)TIME_RATE*(float)numFrames*(float)inputFrameBits/(256.0*24.0*(float)totalTime)));
        }
    }
}



/*===========================================================================*
 *
 * EstimateSecondsPerPFrame
 *
 *  compute an estimate of the number of seconds required per P-frame
 *
 * RETURNS: the estimate, in seconds
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
float
  EstimateSecondsPerPFrame()
{
  if ( numFrames == 0 ) {
    return 10.0;
  } else {
    return (float)totalTime/((float)TIME_RATE*(float)numFrames);
  }
}


/*===========================================================================*
 *
 * ComputeHalfPixelData
 *
 *  compute all half-pixel data required for half-pixel motion vector
 *  search (luminance only)
 *
 * RETURNS: frame->halfX, ->halfY, and ->halfBoth modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
  ComputeHalfPixelData(frame)
MpegFrame *frame;
{
  register int x, y;

  /* we add 1 before dividing by 2 because .5 is supposed to be rounded up
   * (see MPEG-1, page D-31)
   */

  if ( frame->halfX == NULL ) { /* need to allocate memory */
    Frame_AllocHalf(frame);
  }

  /* compute halfX */
  for ( y = 0; y < Fsize_y; y++ ) {
    for ( x = 0; x < Fsize_x-1; x++ ) {
      frame->halfX[y][x] = (frame->ref_y[y][x]+
                frame->ref_y[y][x+1]+1)>>1;
    }
  }

  /* compute halfY */
  for ( y = 0; y < Fsize_y-1; y++ ) {
    for ( x = 0; x < Fsize_x; x++ ) {
      frame->halfY[y][x] = (frame->ref_y[y][x]+
                frame->ref_y[y+1][x]+1)>>1;
    }
  }

  /* compute halfBoth */
  for ( y = 0; y < Fsize_y-1; y++ ) {
    for ( x = 0; x < Fsize_x-1; x++ ) {
      frame->halfBoth[y][x] = (frame->ref_y[y][x]+
                   frame->ref_y[y][x+1]+
                   frame->ref_y[y+1][x]+
                   frame->ref_y[y+1][x+1]+2)>>2;
    }
  }

  frame->halfComputed = TRUE;
}


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
