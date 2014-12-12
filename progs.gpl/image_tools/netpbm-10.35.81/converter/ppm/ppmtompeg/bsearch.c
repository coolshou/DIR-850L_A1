/*===========================================================================*
 * bsearch.c
 *
 *  Procedures concerned with the B-frame motion search
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

/*  
 *  $Header: /n/picasso/project/mpeg/mpeg_dist/mpeg_encode/RCS/bsearch.c,v 1.10 1995/08/07 21:49:01 smoot Exp $
 *  $Log: bsearch.c,v $
 *  Revision 1.10  1995/08/07 21:49:01  smoot
 *  fixed bug in initial-B-frame searches
 *
 *  Revision 1.9  1995/06/26 21:36:07  smoot
 *  added new ordering constraints
 *  (B frames which are backward P's at the start of a sequence)
 *
 *  Revision 1.8  1995/03/27 19:17:43  smoot
 *  killed useless type error messge (int32 defiend as int)
 *
 * Revision 1.7  1995/01/19  23:07:20  eyhung
 * Changed copyrights
 *
 * Revision 1.6  1994/12/07  00:40:36  smoot
 * Added seperate P and B search ranges
 *
 * Revision 1.5  1994/03/15  00:27:11  keving
 * nothing
 *
 * Revision 1.4  1993/12/22  19:19:01  keving
 * nothing
 *
 * Revision 1.3  1993/07/22  22:23:43  keving
 * nothing
 *
 * Revision 1.2  1993/06/30  20:06:09  keving
 * nothing
 *
 * Revision 1.1  1993/06/03  21:08:08  keving
 * nothing
 *
 * Revision 1.1  1993/03/02  18:27:05  keving
 * nothing
 *
 */


/*==============*
 * HEADER FILES *
 *==============*/

#include "pm_c_util.h"
#include "pm.h"
#include "all.h"
#include "mtypes.h"
#include "frames.h"
#include "motion_search.h"
#include "fsize.h"


/*==================*
 * STATIC VARIABLES *
 *==================*/

static int  bsearchAlg;


/*===========================*
 * INITIALIZATION PROCEDURES *
 *===========================*/


/*===========================================================================*
 *
 * SetBSearchAlg
 *
 *  set the B-search algorithm
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    bsearchAlg modified
 *
 *===========================================================================*/
void
SetBSearchAlg(const char * const alg) {
    if (strcmp(alg, "SIMPLE") == 0)
        bsearchAlg = BSEARCH_SIMPLE;
    else if (strcmp(alg, "CROSS2") == 0)
        bsearchAlg = BSEARCH_CROSS2;
    else if (strcmp(alg, "EXHAUSTIVE") == 0)
        bsearchAlg = BSEARCH_EXHAUSTIVE;
    else
        pm_error("ERROR:  Impossible bsearch alg:  %s", alg);
}


/*===========================================================================*
 *
 * BSearchName
 *
 *  return the text of the B-search algorithm
 *
 * RETURNS: a pointer to the string
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
const char *
BSearchName(void)
{
    const char *retval;

    switch(bsearchAlg) {
    case BSEARCH_SIMPLE:
        retval = "SIMPLE";break;
    case BSEARCH_CROSS2:
        retval = "CROSS2";break;
    case BSEARCH_EXHAUSTIVE:
        retval = "EXHAUSTIVE";break;
    default:
        pm_error("ERROR:  Impossible BSEARCH ALG:  %d", psearchAlg);
    }
    return retval;
}



/*===========================================================================*
 *
 * FindBestMatchExhaust
 *
 *  tries to find matching motion vector
 *  see FindBestMatch for generic description
 *
 * DESCRIPTION:  uses an exhaustive search
 *
 *===========================================================================*/
static int32
FindBestMatchExhaust(const LumBlock * const blockP,
                     const LumBlock * const currentBlockP,
                     MpegFrame *      const prev,
                     int              const by,
                     int              const bx,
                     vector *         const motionP,
                     int32            const bestSoFar,
                     int              const searchRange) {

    register int mx, my;
    int32 bestDiff;
    int   stepSize;
    int   leftMY, leftMX;
    int   rightMY, rightMX;
    int   distance;
    int   tempRightMY, tempRightMX;
    boolean changed = FALSE;

    stepSize = (pixelFullSearch ? 2 : 1);

    COMPUTE_MOTION_BOUNDARY(by,bx,stepSize,leftMY,leftMX,rightMY,rightMX);

    /* try old motion vector first */
    if (VALID_MOTION(*motionP)) {
        bestDiff = LumAddMotionError(currentBlockP, blockP, prev, by, bx,
                                     *motionP, bestSoFar);

        if (bestSoFar < bestDiff)
            bestDiff = bestSoFar;
    } else {
        motionP->y = motionP->x = 0;
        bestDiff = bestSoFar;
    }

    /* maybe should try spiral pattern centered around  prev motion vector? */

    /* try a spiral pattern */    
    for (distance = stepSize;
         distance <= searchRange;
         distance += stepSize) {
        tempRightMY = MIN(distance, rightMY);
        tempRightMX = MIN(distance, rightMX);

        /* do top, bottom */
        for (my = -distance; my < tempRightMY;
             my += max(tempRightMY+distance-stepSize, stepSize)) {
            if (my >= leftMY) {
                for (mx = -distance; mx < tempRightMX; mx += stepSize) {
                    if (mx >= leftMX) {
                        int diff;
                        vector m;
                        m.y = my; m.x = mx;
                        diff = LumAddMotionError(currentBlockP, blockP, prev,
                                                 by, bx, m, bestDiff);
                        
                        if (diff < bestDiff) {
                            *motionP = m;
                            bestDiff = diff;
                        }
                    }
                }
            }
        }

        /* do left, right */
        for (mx = -distance;
             mx < tempRightMX;
             mx += max(tempRightMX+distance-stepSize, stepSize)) {
            if (mx >= leftMX) {
                for (my = -distance+stepSize;
                     my < tempRightMY-stepSize;
                     my += stepSize) {
                    if (my >= leftMY) {
                        int diff;
                        vector m;
                        m.y = my; m.x = mx;
                        diff = LumAddMotionError(currentBlockP, blockP, prev,
                                                 by, bx,
                                                 m, bestDiff);
                        
                        if (diff < bestDiff) {
                            *motionP = m;
                            bestDiff = diff;
                            changed = TRUE;
                        }
                    }
                }
            }
        }
    }

    if (!changed)
        ++bestDiff;

    return bestDiff;
}


/*===========================================================================*
 *
 * FindBestMatchTwoLevel
 *
 *  tries to find matching motion vector
 *  see FindBestMatch for generic description
 *
 * DESCRIPTION:  uses an exhaustive full-pixel search, then looks at
 *       neighboring half-pixels
 *
 *===========================================================================*/
static int32
FindBestMatchTwoLevel(const LumBlock * const blockP,
                      const LumBlock * const currentBlockP,
                      MpegFrame *      const prev,
                      int              const by,
                      int              const bx,
                      vector *         const motionP,
                      int32            const bestSoFar,
                      int              const searchRange) {

    int mx, my;
    int32 bestDiff;
    int leftMY, leftMX;
    int rightMY, rightMX;
    int distance;
    int tempRightMY, tempRightMX;
    boolean changed = FALSE;
    int yOffset, xOffset;

    /* exhaustive full-pixel search first */

    COMPUTE_MOTION_BOUNDARY(by,bx,2,leftMY,leftMX,rightMY,rightMX);

    --rightMY;
    --rightMX;

    /* convert vector into full-pixel vector */
    if (motionP->y > 0 ) {
        if ((motionP->y % 2) == 1)
            --motionP->y;
    } else if ((-motionP->y % 2) == 1)
        ++motionP->y;

    if (motionP->x > 0 ) {
        if ((motionP->x % 2) == 1 )
            --motionP->x;
    } else if ((-motionP->x % 2) == 1)
        ++motionP->x;

    /* try old motion vector first */
    if (VALID_MOTION(*motionP)) {
        bestDiff = LumAddMotionError(currentBlockP, blockP, prev, by, bx,
                                     *motionP, bestSoFar);
        
        if (bestSoFar < bestDiff)
            bestDiff = bestSoFar;
    } else {
        motionP->y = motionP->x = 0;
        bestDiff = bestSoFar;
    }

    ++rightMY;
    ++rightMX;

    /* maybe should try spiral pattern centered around  prev motion vector? */

    /* try a spiral pattern */    
    for ( distance = 2; distance <= searchRange; distance += 2 ) {
        tempRightMY = MIN(distance, rightMY);
        tempRightMX = MIN(distance, rightMX);

        /* do top, bottom */
        for (my = -distance; my < tempRightMY;
             my += max(tempRightMY+distance-2, 2)) {
            if (my >= leftMY) {
                for ( mx = -distance; mx < tempRightMX; mx += 2 ) {
                    if (mx >= leftMX) {
                        int diff;
                        vector m;
                        m.y = my; m.x = mx;
                        diff = LumAddMotionError(currentBlockP, blockP, prev,
                                                 by, bx, m, bestDiff);
                        
                        if (diff < bestDiff) {
                            *motionP = m;
                            bestDiff = diff;
                        }
                    }
                }
            }
        }

        /* do left, right */
        for (mx = -distance;
             mx < tempRightMX;
             mx += max(tempRightMX+distance-2, 2)) {
            if (mx >= leftMX) {
                for (my = -distance+2; my < tempRightMY-2; my += 2) {
                    if (my >= leftMY) {
                        int diff;
                        vector m;
                        m.y = my; m.x = mx;
                        diff = LumAddMotionError(currentBlockP, blockP, prev,
                                                 by, bx, m, bestDiff);
                        
                        if (diff < bestDiff) {
                            *motionP = m;
                            bestDiff = diff;
                            changed = TRUE;
                        }
                    }
                }
            }
        }
    }

    /* now look at neighboring half-pixels */
    my = motionP->y;
    mx = motionP->x;

    --rightMY;
    --rightMX;

    for (yOffset = -1; yOffset <= 1; ++yOffset) {
        for (xOffset = -1; xOffset <= 1; ++xOffset) {
            if ((yOffset != 0) || (xOffset != 0)) {
                vector m;
                m.y = my + yOffset;
                m.x = mx + xOffset;
                if (VALID_MOTION(m)) {
                    int diff;
                    diff = LumAddMotionError(currentBlockP, blockP,
                                             prev, by, bx, m, bestDiff);
                    if (diff < bestDiff) {
                        *motionP = m;
                        bestDiff = diff;
                        changed = TRUE;
                    }
                }
            }
        }
    }

    if (!changed)
        ++bestDiff;

    return bestDiff;
}



static void
trySpacing(int              const spacing,
           vector           const center,
           int              const bestDiffSoFar,
           vector *         const newCenterP,
           int32 *          const newBestDiffP,
           int              const leftMY,
           int              const leftMX,
           int              const rightMY,
           int              const rightMX,
           const LumBlock * const currentBlockP,
           const LumBlock * const blockP,
           MpegFrame *      const prev,
           int              const by,
           int              const bx) {
           
    int tempRightMY, tempRightMX;
    int my;
    int bestDiff;
    vector newCenter;

    /* Initial values */
    newCenter = center;
    bestDiff   = bestDiffSoFar;

    tempRightMY = MIN(rightMY, center.y + spacing + 1);
    tempRightMX = MIN(rightMX, center.x + spacing + 1);
    
    for (my = center.y - spacing; my < tempRightMY; my += spacing) {
        if (my >= leftMY) {
            int mx;
            for (mx = center.x - spacing; mx < tempRightMX; mx += spacing) {
                if (mx >= leftMX) {
                    int32 diff;
                    vector m;
                    m.y = my; m.x = mx;
                    diff = LumAddMotionError(currentBlockP, blockP, prev,
                                             by, bx, m, bestDiff);
                    
                    if (diff < bestDiff) {
                        /* We have a new best */
                        newCenter = m;
                        bestDiff   = diff;
                    }
                }
            }
        }
    }
    *newCenterP  = newCenter;
    *newBestDiffP = bestDiff;
}



static void
chooseNewSpacing(int   const oldSpacing,
                 int   const stepSize,
                 int * const newSpacingP) {
        
    if (stepSize == 2) {  /* make sure spacing is even */
        if (oldSpacing == 2)
            *newSpacingP = 0;
        else {
            int const trialSpacing = (oldSpacing + 1) / 2;
            if ((trialSpacing % 2) != 0)
                *newSpacingP = trialSpacing + 1;
            else
                *newSpacingP = trialSpacing;
        }
    } else {
        if (oldSpacing == 1)
            *newSpacingP = 0;
        else
            *newSpacingP = (oldSpacing + 1) / 2;
    }
}



/*===========================================================================*
 *
 * FindBestMatchLogarithmic
 *
 *  tries to find matching motion vector
 *  see FindBestMatch for generic description
 *
 * DESCRIPTION:  uses a logarithmic search
 *
 *===========================================================================*/
static int32
FindBestMatchLogarithmic(const LumBlock * const blockP,
                         const LumBlock * const currentBlockP,
                         MpegFrame *      const prev,
                         int              const by,
                         int              const bx,
                         vector *         const motionP,
                         int32            const bestSoFar,
                         int              const searchRange) {

    int const stepSize = (pixelFullSearch ? 2 : 1);

    int32 diff, bestDiff;
    int leftMY, leftMX;
    int rightMY, rightMX;
    int spacing;
    vector center;

    COMPUTE_MOTION_BOUNDARY(by, bx, stepSize,
                            leftMY, leftMX, rightMY, rightMX);

    bestDiff = 0x7fffffff;

    /* grid spacing */
    if (stepSize == 2) {  /* make sure spacing is even */
        spacing = (searchRange + 1) / 2;
        if ((spacing % 2) != 0)
            ++spacing;
    } else
        spacing = (searchRange + 1) / 2;

    /* Start at (0,0) */
    center.y = center.x = 0;
    
    while (spacing >= stepSize) {
        trySpacing(spacing, center, bestDiff,
                   &center, &bestDiff,
                   leftMY, leftMX, rightMY, rightMX,
                   currentBlockP, blockP, prev, by, bx);

        chooseNewSpacing(spacing, stepSize, &spacing);
    }

    /* check old motion -- see if it's better */
    if ((motionP->y >= leftMY) && (motionP->y < rightMY) &&
        (motionP->x >= leftMX) && (motionP->x < rightMX)) {
        diff = LumAddMotionError(currentBlockP, blockP, prev, by, bx,
                                 *motionP, bestDiff);
    } else
        diff = 0x7fffffff;

    if (bestDiff < diff)
        *motionP = center;
    else
        bestDiff = diff;

    return bestDiff;
}



/*===========================================================================*
 *
 * FindBestMatchSubSample
 *
 *  tries to find matching motion vector
 *  see FindBestMatch for generic description
 *
 * DESCRIPTION:  should use subsampling method, but too lazy to write all
 *       the code for it (so instead just calls FindBestMatchExhaust)
 *
 *===========================================================================*/
static int32
FindBestMatchSubSample(const LumBlock * const blockP,
                       const LumBlock * const currentBlockP,
                       MpegFrame *      const prev,
                       int              const by,
                       int              const bx,
                       vector *         const motionP,
                       int32            const bestSoFar,
                       int              const searchRange) {

    /* too lazy to write the code for this... */
    
    return FindBestMatchExhaust(blockP, currentBlockP, prev,
                                by, bx, motionP, bestSoFar,
                                searchRange);
}


/*===========================================================================*
 *
 * FindBestMatch
 *
 *  given a motion-compensated block in one direction, tries to find
 *  the best motion vector in the opposite direction to match it
 *
 * RETURNS: the best vector (*motionY, *motionX), and the corresponding
 *      error is returned if it is better than bestSoFar.  If not,
 *      then a number greater than bestSoFar is returned and
 *      (*motionY, *motionX) has no meaning.
 *
 * SIDE EFFECTS:  none
 *
 *===========================================================================*/
static int32
FindBestMatch(const LumBlock * const blockP,
              const LumBlock * const currentBlockP,
              MpegFrame *      const prev,
              int              const by,
              int              const bx,
              vector *         const motionP,
              int32            const bestSoFar,
              int              const searchRange) {

    int32 result;

    switch(psearchAlg) {
    case PSEARCH_SUBSAMPLE:
        result = FindBestMatchSubSample(
            blockP, currentBlockP, prev, by, bx,
            motionP, bestSoFar, searchRange);
        break;
    case PSEARCH_EXHAUSTIVE:
        result = FindBestMatchExhaust(
            blockP, currentBlockP, prev, by, bx,
            motionP, bestSoFar, searchRange);
        break;
    case PSEARCH_LOGARITHMIC:
        result = FindBestMatchLogarithmic(
            blockP, currentBlockP, prev, by, bx,
            motionP, bestSoFar, searchRange);
        break;
    case PSEARCH_TWOLEVEL:
        result = FindBestMatchTwoLevel(
            blockP, currentBlockP, prev, by, bx,
            motionP, bestSoFar, searchRange);
        break;
    default:
        pm_error("ERROR:  Impossible P-search alg %d", psearchAlg);
    }
    return result;
}



/*===========================================================================*
 *
 *  finds the best backward and forward motion vectors
 *  if backNeeded == FALSE, then won't find best backward vector if it
 *  is worse than the best forward vector
 *
 * *motionP is input as well as output.  We do not update it
 * if it would make the error worse than the existing value.
 *
 * RETURNS: *motionP and associated errors *forwardErrP and *backErrP.
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
BMotionSearchNoInterp(const LumBlock * const currentBlockP,
                      MpegFrame *      const prev,
                      MpegFrame *      const next,
                      int              const by,
                      int              const bx,
                      motion *         const motionP,
                      int32 *          const forwardErrP,
                      int32 *          const backErrP,
                      boolean          const backNeeded) {

    /* CALL SEARCH PROCEDURE */
    switch(psearchAlg) {
    case PSEARCH_SUBSAMPLE:
        *forwardErrP = PSubSampleSearch(currentBlockP, prev, by, bx, 
                                        &motionP->fwd,searchRangeB);
        *backErrP = PSubSampleSearch(currentBlockP, next, by, bx, 
                                     &motionP->bwd, searchRangeB);
        break;
    case PSEARCH_EXHAUSTIVE:
        *forwardErrP = PLocalSearch(currentBlockP, prev, by, bx,
                                    &motionP->fwd,
                                    0x7fffffff, searchRangeB);
        if (backNeeded)
            *backErrP = PLocalSearch(currentBlockP, next, by, bx,
                                     &motionP->bwd,
                                     0x7fffffff, searchRangeB);
        else
            *backErrP = PLocalSearch(currentBlockP, next, by, bx,
                                     &motionP->bwd,
                                     *forwardErrP, searchRangeB);
        break;
    case PSEARCH_LOGARITHMIC:
        *forwardErrP = PLogarithmicSearch(currentBlockP, prev, by, bx, 
                                          &motionP->fwd, searchRangeB);
        *backErrP = PLogarithmicSearch(currentBlockP, next, by, bx, 
                                       &motionP->bwd, searchRangeB);
        break;
    case PSEARCH_TWOLEVEL:
        *forwardErrP =
            PTwoLevelSearch(currentBlockP, prev, by, bx,
                            &motionP->fwd, 0x7fffffff, searchRangeB);
        if ( backNeeded ) {
            *backErrP =
                PTwoLevelSearch(currentBlockP, next, by, bx,
                                &motionP->bwd, 0x7fffffff, searchRangeB);
        } else {
            *backErrP =
                PTwoLevelSearch(currentBlockP, next, by, bx,
                                &motionP->bwd, *forwardErrP, searchRangeB);
        }
        break;
    default:
        pm_error("ERROR:  Impossible PSEARCH ALG:  %d", psearchAlg);
    }
}



/*===========================================================================*
 *
 * BMotionSearchSimple
 *
 *  does a simple search for B-frame motion vectors
 *  see BMotionSearch for generic description
 *
 * DESCRIPTION:
 *  1)  find best backward and forward vectors
 *  2)  compute interpolated error using those two vectors
 *  3)  return the best of the three choices
 *
 * *fmyP,fmxP,bmyP,bmxP are inputs as well as outputs.  We do not update
 * them if it would make the error worse than the existing values.  Otherwise,
 * we update them to the vectors we find to be best.
 * 
 *===========================================================================*/
static int
BMotionSearchSimple(const LumBlock * const currentBlockP,
                    MpegFrame *      const prev,
                    MpegFrame *      const next,
                    int              const by,
                    int              const bx,
                    motion *         const motionP,
                    int              const oldMode) {

    int retval;
    int32 forwardErr, backErr, interpErr;
    LumBlock interpBlock;
    int32 bestSoFar;

    /* STEP 1 */
    BMotionSearchNoInterp(currentBlockP, prev, next, by, bx, motionP,
                          &forwardErr, &backErr, TRUE);
              
    /* STEP 2 */

    ComputeBMotionLumBlock(prev, next, by, bx, MOTION_INTERPOLATE,
                           *motionP, &interpBlock);
    bestSoFar = min(backErr, forwardErr);
    interpErr =
        LumBlockMAD(currentBlockP, &interpBlock, bestSoFar);

    /* STEP 3 */

    if (interpErr <= forwardErr) {
        if (interpErr <= backErr)
            retval = MOTION_INTERPOLATE;
        else
            retval = MOTION_BACKWARD;
    } else if (forwardErr <= backErr)
        retval = MOTION_FORWARD;
    else
        retval = MOTION_BACKWARD;

    return retval;
}


/*===========================================================================*
 *
 * BMotionSearchCross2
 *
 *  does a cross-2 search for B-frame motion vectors
 *  see BMotionSearch for generic description
 *
 * DESCRIPTION:
 *  1)  find best backward and forward vectors
 *  2)  find best matching interpolating vectors
 *  3)  return the best of the 4 choices
 *
 * *fmyP,fmxP,bmyP,bmxP are inputs as well as outputs.  We do not update
 * them if it would make the error worse than the existing values.  Otherwise,
 * we update them to the vectors we find to be best.
 *===========================================================================*/
static int
BMotionSearchCross2(const LumBlock * const currentBlockP,
                    MpegFrame *      const prev,
                    MpegFrame *      const next,
                    int              const by,
                    int              const bx,
                    motion *         const motionP,
                    int              const oldMode) {
    
    int retval;
    LumBlock forwardBlock, backBlock;
    int32   forwardErr, backErr;
    motion newMotion;
    int32   interpErrF, interpErrB, interpErr;
    int32   bestErr;

    /* STEP 1 */

    BMotionSearchNoInterp(currentBlockP, prev, next, by, bx, motionP,
                          &forwardErr, &backErr, TRUE);

    bestErr = min(forwardErr, backErr);

    {
        /* STEP 2 */
        
        struct motion motion;
        motion.fwd = motionP->fwd;
        motion.bwd.y = motion.bwd.x = 0;
        ComputeBMotionLumBlock(prev, next, by, bx, MOTION_FORWARD, motion,
                               &forwardBlock);
        
        motion.fwd.y = motion.fwd.x = 0;
        motion.bwd = motionP->bwd;
        ComputeBMotionLumBlock(prev, next, by, bx, MOTION_BACKWARD, motion,
                               &backBlock);
    }
    /* try a cross-search; total of 4 local searches */    
    newMotion = *motionP;

    interpErrF = FindBestMatch(&forwardBlock, currentBlockP,
                               next, by, bx, &newMotion.bwd,
                               bestErr, searchRangeB);
    bestErr = min(bestErr, interpErr);
    interpErrB = FindBestMatch(&backBlock, currentBlockP,
                               prev, by, bx, &newMotion.fwd,
                               bestErr, searchRangeB);

    /* STEP 3 */

    if (interpErrF <= interpErrB) {
        newMotion.fwd = motionP->fwd;
        interpErr = interpErrF;
    } else {
        newMotion.bwd = motionP->bwd;
        interpErr = interpErrB;
    }

    if (interpErr <= forwardErr) {
        if (interpErr <= backErr) {
            *motionP = newMotion;
            retval = MOTION_INTERPOLATE;
        } else
            retval = MOTION_BACKWARD;
    } else if (forwardErr <= backErr)
        retval = MOTION_FORWARD;
    else
        retval = MOTION_BACKWARD;

    return retval;
}


/*===========================================================================*
 *
 * BMotionSearchExhaust
 *
 *  does an exhaustive search for B-frame motion vectors
 *  see BMotionSearch for generic description
 *
 * DESCRIPTION:
 *  1)  find best backward and forward vectors
 *  2)  use exhaustive search to find best interpolating vectors
 *  3)  return the best of the 3 choices
 *
 * *fmyP,fmxP,bmyP,bmxP are inputs as well as outputs.  We do not update
 * them if it would make the error worse than the existing values.  Otherwise,
 * we update them to the vectors we find to be best.
 *===========================================================================*/
static int
BMotionSearchExhaust(const LumBlock * const currentBlockP,
                     MpegFrame *      const prev,
                     MpegFrame *      const next,
                     int              const by,
                     int              const bx,
                     motion *         const motionP,
                     int              const oldMode) {

    int mx, my;
    int32 bestDiff;
    int     stepSize;
    LumBlock    forwardBlock;
    int32   forwardErr, backErr;
    int     leftMY, leftMX;
    int     rightMY, rightMX;
    boolean result;

    /* STEP 1 */

    BMotionSearchNoInterp(currentBlockP, prev, next, by, bx, motionP,
                          &forwardErr, &backErr, FALSE);

    if (forwardErr <= backErr) {
        bestDiff = forwardErr;
        result = MOTION_FORWARD;
    } else {
        bestDiff = backErr;
        result = MOTION_BACKWARD;
    }

    /* STEP 2 */

    stepSize = (pixelFullSearch ? 2 : 1);

    COMPUTE_MOTION_BOUNDARY(by,bx,stepSize,leftMY,leftMX,rightMY,rightMX);

    if (searchRangeB < rightMY)
        rightMY = searchRangeB;
    if (searchRangeB < rightMX)
        rightMX = searchRangeB;

    for (my = -searchRangeB; my < rightMY; my += stepSize) {
        if (my >= leftMY) {
            for (mx = -searchRangeB; mx < rightMX; mx += stepSize) {
                if (mx >= leftMX) {
                    struct motion motion;
                    vector newMotion;
                    int32 diff;
                    motion.fwd.y = my; motion.fwd.x = mx;
                    motion.bwd.y = 0;  motion.bwd.x = 0;
                    ComputeBMotionLumBlock(prev, next, by, bx, MOTION_FORWARD,
                                           motion, &forwardBlock);

                    newMotion = motion.fwd;
                    
                    diff = FindBestMatch(&forwardBlock,
                                         currentBlockP, next, by, bx,
                                         &newMotion, bestDiff, searchRangeB);
                    
                    if (diff < bestDiff) {
                        motionP->fwd = motion.fwd;
                        motionP->bwd = newMotion;
                        bestDiff = diff;
                        result = MOTION_INTERPOLATE;
                    }
                }
            }
        }
    }
    return result;
}



/*===========================================================================*
 *
 *  search for the best B-frame motion vectors
 *
 * RETURNS: MOTION_FORWARD      forward motion should be used
 *      MOTION_BACKWARD     backward motion should be used
 *      MOTION_INTERPOLATE  both should be used and interpolated
 *
 * OUTPUTS: *motionP  =   TWICE the forward motion vectors
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:   The relevant block in 'current' is valid (it has not
 *          been dct'd).  Thus, the data in 'current' can be
 *          accesed through y_blocks, cr_blocks, and cb_blocks.
 *          This is not the case for the blocks in 'prev' and
 *          'next.'  Therefore, references into 'prev' and 'next'
 *          should be done
 *          through the struct items ref_y, ref_cr, ref_cb
 *
 * POSTCONDITIONS:  current, prev, next should be unchanged.
 *          Some computation could be saved by requiring
 *          the dct'd difference to be put into current's block
 *          elements here, depending on the search technique.
 *          However, it was decided that it mucks up the code
 *          organization a little, and the saving in computation
 *          would be relatively little (if any).
 *
 * NOTES:   the search procedure MAY return (0,0) motion vectors
 *
 *===========================================================================*/
int
BMotionSearch(const LumBlock * const currentBlockP,
              MpegFrame *      const prev,
              MpegFrame *      const next,
              int              const by,
              int              const bx,
              motion *         const motionP,
              int              const oldMode) {

    int retval;

    /* If we are an initial B frame, no possibility of forward motion */
    if (prev == (MpegFrame *) NULL) {
        PMotionSearch(currentBlockP, next, by, bx, &motionP->bwd);
        return MOTION_BACKWARD;
    }
  
    /* otherwise simply call the appropriate algorithm, based on user
       preference
    */

    switch(bsearchAlg) {
    case BSEARCH_SIMPLE:
        retval = BMotionSearchSimple(currentBlockP, prev, next, by, bx,
                                     motionP, oldMode);
        break;
    case BSEARCH_CROSS2:
        retval = BMotionSearchCross2(currentBlockP, prev, next, by, bx,
                                     motionP, oldMode);
        break;
    case BSEARCH_EXHAUSTIVE:
        retval = BMotionSearchExhaust(currentBlockP, prev, next, by, bx,
                                      motionP, oldMode);
        break;
    default:
        pm_error("Impossible B-frame motion search algorithm:  %d",
                 bsearchAlg);
    }
    return retval;
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

/*===========================================================================*
 *
 * ValidBMotion
 *
 *  decides if the given B-frame motion is valid
 *
 * RETURNS: TRUE if the motion is valid, FALSE otherwise
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
boolean
ValidBMotion(by, bx, mode, fmy, fmx, bmy, bmx)
    int by;
    int bx;
    int mode;
    int fmy;
    int fmx;
    int bmy;
    int bmx;
{
    if ( mode != MOTION_BACKWARD ) {
    /* check forward motion for bounds */
    if ( (by*DCTSIZE+(fmy-1)/2 < 0) || ((by+2)*DCTSIZE+(fmy+1)/2-1 >= Fsize_y) ) {
        return FALSE;
    }
    if ( (bx*DCTSIZE+(fmx-1)/2 < 0) || ((bx+2)*DCTSIZE+(fmx+1)/2-1 >= Fsize_x) ) {
        return FALSE;
    }
    }

    if ( mode != MOTION_FORWARD ) {
    /* check backward motion for bounds */
    if ( (by*DCTSIZE+(bmy-1)/2 < 0) || ((by+2)*DCTSIZE+(bmy+1)/2-1 >= Fsize_y) ) {
        return FALSE;
    }
    if ( (bx*DCTSIZE+(bmx-1)/2 < 0) || ((bx+2)*DCTSIZE+(bmx+1)/2-1 >= Fsize_x) ) {
        return FALSE;
    }
    }

    return TRUE;
}


#endif /* UNUSED_PROCEDURES */
