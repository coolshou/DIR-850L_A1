/*===========================================================================*
 * psearch.c                                     
 *                                       
 *  Procedures concerned with the P-frame motion search          
 *                                       
 *===========================================================================*/

/*==============*
 * HEADER FILES *
 *==============*/

#include "all.h"
#include "mtypes.h"
#include "frames.h"
#include "motion_search.h"
#include "prototypes.h"
#include "fsize.h"
#include "param.h"


/*==================*
 * STATIC VARIABLES *
 *==================*/

/* none */


/*==================*
 * GLOBAL VARIABLES *
 *==================*/

int **pmvHistogram = NULL;  /* histogram of P-frame motion vectors */
int **bbmvHistogram = NULL; /* histogram of B-frame bkwd motion vectors */
int **bfmvHistogram = NULL; /* histogram of B-frame fwd motion vectors */
int pixelFullSearch;
int searchRangeP,searchRangeB;
/* The range, in half pixels in each direction, that we are to search
       when detecting motion.  Specified by RANGE statement in parameter file.
    */
int psearchAlg;
/* specified by parameter file. */

/*===============================*
 * INTERNAL PROCEDURE prototypes *
 *===============================*/


/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 *  Compute the best P-frame motion vector we can.  If it's better than
 *  *motionP, update *motionP to it.
 *
 * PRECONDITIONS:   The relevant block in 'current' is valid (it has not
 *          been dct'd).  Thus, the data in 'current' can be
 *          accesed through y_blocks, cr_blocks, and cb_blocks.
 *          This is not the case for the blocks in 'prev.'
 *          Therefore, references into 'prev' should be done
 *          through the struct items ref_y, ref_cr, ref_cb
 *
 * POSTCONDITIONS:  current, prev unchanged.
 *          Some computation could be saved by requiring
 *          the dct'd difference to be put into current's block
 *          elements here, depending on the search technique.
 *          However, it was decided that it mucks up the code
 *          organization a little, and the saving in computation
 *          would be relatively little (if any).
 *
 * NOTES:   the search procedure need not check the (0,0) motion vector
 *      the calling procedure has a preference toward (0,0) and it
 *      will check it itself
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
PMotionSearch(const LumBlock * const currentBlockP, 
              MpegFrame *      const prev, 
              int              const by,
              int              const bx, 
              vector *         const motionP) {

    /* CALL SEARCH PROCEDURE */

    switch(psearchAlg) {
    case PSEARCH_SUBSAMPLE:
        PSubSampleSearch(currentBlockP, prev, by, bx, motionP, searchRangeP);
        break;
    case PSEARCH_EXHAUSTIVE:
        PLocalSearch(currentBlockP, prev, by, bx, 
                     motionP, INT_MAX, searchRangeP);
        break;
    case PSEARCH_LOGARITHMIC:
        PLogarithmicSearch(currentBlockP, prev, by, bx, motionP, searchRangeP);
        break;
    case PSEARCH_TWOLEVEL:
        PTwoLevelSearch(currentBlockP, prev, by, bx, 
                        motionP, INT_MAX, searchRangeP);
        break;
    default:
        pm_error("IMPOSSIBLE PSEARCH ALG:  %d", psearchAlg);
    }
}



/*===========================================================================*
 *
 * SetPixelSearch
 *
 *  set the pixel search type (half or full)
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    pixelFullSearch
 *
 *===========================================================================*/
void
SetPixelSearch(const char * const searchType) {
    if ( (strcmp(searchType, "FULL") == 0 ) || 
         ( strcmp(searchType, "WHOLE") == 0 )) {
        pixelFullSearch = TRUE;
    } else if ( strcmp(searchType, "HALF") == 0 ) {
        pixelFullSearch = FALSE;
    } else {
        fprintf(stderr, "ERROR:  Invalid pixel search type:  %s\n",
                searchType);
        exit(1);
    }
}


/*===========================================================================*
 *
 * SetPSearchAlg
 *
 *  set the P-search algorithm
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    psearchAlg
 *
 *===========================================================================*/
void
SetPSearchAlg(const char * const alg)
{
    if ( strcmp(alg, "EXHAUSTIVE") == 0 ) {
        psearchAlg = PSEARCH_EXHAUSTIVE;
    } else if (strcmp(alg, "SUBSAMPLE") == 0 ) {
        psearchAlg = PSEARCH_SUBSAMPLE;
    } else if ( strcmp(alg, "LOGARITHMIC") == 0 ) {
        psearchAlg = PSEARCH_LOGARITHMIC;
    } else if ( strcmp(alg, "TWOLEVEL") == 0 ) {
        psearchAlg = PSEARCH_TWOLEVEL;
    } else {
        fprintf(stderr, "ERROR:  Invalid psearch algorithm:  %s\n", alg);
        exit(1);
    }
}


/*===========================================================================*
 *
 * PSearchName
 *
 *  returns a string containing the name of the search algorithm
 *
 * RETURNS: pointer to the string
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
const char *
PSearchName(void)
{
    const char *retval;

    switch(psearchAlg) {
    case PSEARCH_EXHAUSTIVE:
        retval = "EXHAUSTIVE";break;
    case PSEARCH_SUBSAMPLE:
        retval = "SUBSAMPLE";break;
    case PSEARCH_LOGARITHMIC:
        retval = "LOGARITHMIC";break;
    case PSEARCH_TWOLEVEL:
        retval = "TWOLEVEL";break;
    default:
        fprintf(stderr, "ERROR:  Illegal PSEARCH ALG:  %d\n", psearchAlg);
        exit(1);
        break;
    }
    return retval;
}


/*===========================================================================*
 *
 * SetSearchRange
 *
 *  sets the range of the search to the given number of pixels,
 *  allocate histogram storage
 *
 *===========================================================================*/
void
SetSearchRange(int const pixelsP, int const pixelsB) {

    searchRangeP = 2*pixelsP;   /* +/- 'pixels' pixels */
    searchRangeB = 2*pixelsB;

    if ( computeMVHist ) {
        int const max_search = max(searchRangeP, searchRangeB);

        int index;
    
        pmvHistogram = (int **) malloc((2*searchRangeP+3)*sizeof(int *));
        bbmvHistogram = (int **) malloc((2*searchRangeB+3)*sizeof(int *));
        bfmvHistogram = (int **) malloc((2*searchRangeB+3)*sizeof(int *));
        for ( index = 0; index < 2*max_search+3; index++ ) {
            pmvHistogram[index] = 
                (int *) calloc(2*searchRangeP+3, sizeof(int));
            bbmvHistogram[index] = 
                (int *) calloc(2*searchRangeB+3, sizeof(int));
            bfmvHistogram[index] = 
                (int *) calloc(2*searchRangeB+3, sizeof(int));
        }
    }
}


/*===========================================================================*
 *
 *              USER-MODIFIABLE
 *
 * MotionSearchPreComputation
 *
 *  do whatever you want here; this is called once per frame, directly
 *  after reading
 *
 * RETURNS: whatever
 *
 * SIDE EFFECTS:    whatever
 *
 *===========================================================================*/
void
MotionSearchPreComputation(MpegFrame * const frameP) {
    /* do nothing */
}


/*===========================================================================*
 *
 * PSubSampleSearch
 *
 *  uses the subsampling algorithm to compute the P-frame vector
 *
 * RETURNS: motion vector
 *
 * SIDE EFFECTS:    none
 *
 * REFERENCE:  Liu and Zaccarin:  New Fast Algorithms for the Estimation
 *      of Block Motion Vectors, IEEE Transactions on Circuits
 *      and Systems for Video Technology, Vol. 3, No. 2, 1993.
 *
 *===========================================================================*/
int
PSubSampleSearch(const LumBlock * const currentBlockP,
                 MpegFrame *      const prev, 
                 int              const by,
                 int              const bx,
                 vector *         const motionP,
                 int              const searchRange) {
    
    int my, mx;
    int bestBestDiff;
    int stepSize;
    int x;
    int bestMY[4], bestMX[4], bestDiff[4];
    int leftMY, leftMX;
    int rightMY, rightMX;

    stepSize = (pixelFullSearch ? 2 : 1);

    COMPUTE_MOTION_BOUNDARY(by,bx,stepSize,leftMY,leftMX,rightMY,rightMX);

    if ( searchRange < rightMY ) {
        rightMY = searchRange;
    }

    if ( searchRange < rightMX ) {
        rightMX = searchRange;
    }

    for ( x = 0; x < 4; x++ ) {
        bestMY[x] = 0;
        bestMX[x] = 0;
        bestDiff[x] = INT_MAX;
    }

    /* do A pattern */
    for (my = -searchRange; my < rightMY; my += 2*stepSize) {
        if (my >= leftMY) {
            for ( mx = -searchRange; mx < rightMX; mx += 2*stepSize ) {
                if (mx >= leftMX) {
                    int diff;
                    vector m;
                    m.y = my; m.x = mx;
                    diff = LumMotionErrorA(currentBlockP, prev, by, bx, m,
                                           bestDiff[0]);
                    
                    if (diff < bestDiff[0]) {
                        bestMY[0] = my;
                        bestMX[0] = mx;
                        bestDiff[0] = diff;
                    }
                }
            }
        }
    }

    /* do B pattern */
    for (my = stepSize-searchRange; my < rightMY; my += 2*stepSize) {
        if (my >= leftMY) {
            for (mx = -searchRange; mx < rightMX; mx += 2*stepSize) {
                if (mx >= leftMX) {
                    int diff;
                    vector m;
                    m.y = my; m.x = mx;
                    diff = LumMotionErrorB(currentBlockP, prev, by, bx, m, 
                                           bestDiff[1]);
                    
                    if (diff < bestDiff[1]) {
                        bestMY[1] = my;
                        bestMX[1] = mx;
                        bestDiff[1] = diff;
                    }
                }
            }
        }
    }

    /* do C pattern */
    for (my = stepSize-searchRange; my < rightMY; my += 2*stepSize) {
        if (my >= leftMY) {
            for ( mx = stepSize-searchRange; mx < rightMX; mx += 2*stepSize ) {
                if (mx >= leftMX) {
                    int diff;
                    vector m;
                    m.y = my; m.x = mx;
                    diff = LumMotionErrorC(currentBlockP, prev, by, bx, m,
                                           bestDiff[2]);
                    
                    if (diff < bestDiff[2]) {
                        bestMY[2] = my;
                        bestMX[2] = mx;
                        bestDiff[2] = diff;
                    }
                }
            }
        }
    }

    /* do D pattern */
    for (my = -searchRange; my < rightMY; my += 2*stepSize) {
        if (my >= leftMY) {
            for (mx = stepSize-searchRange; mx < rightMX; mx += 2*stepSize) {
                if (mx >= leftMX) {
                    int diff;
                    vector m;
                    m.y = my; m.x = mx;
                    diff = LumMotionErrorD(currentBlockP, prev, by, bx, m,
                                           bestDiff[3]);
                    
                    if (diff < bestDiff[3]) {
                        bestMY[3] = my;
                        bestMX[3] = mx;
                        bestDiff[3] = diff;
                    }
                }
            }
        }
    }

    /* first check old motion */
    if ((motionP->y >= leftMY) && (motionP->y < rightMY) &&
        (motionP->x >= leftMX) && (motionP->x < rightMX)) {
        bestBestDiff = LumMotionError(currentBlockP, prev, by, bx, 
                                      *motionP, INT_MAX);
    } else
        bestBestDiff = INT_MAX;

    /* look at Error of 4 different motion vectors */
    for (x = 0; x < 4; ++x) {
        vector m;
        m.y = bestMY[x];
        m.x = bestMX[x];
        bestDiff[x] = LumMotionError(currentBlockP, prev, by, bx, m,
                                     bestBestDiff);

        if (bestDiff[x] < bestBestDiff) {
            bestBestDiff = bestDiff[x];
            *motionP     = m;
        }
    }
    return bestBestDiff;
}



static void
findBestSpaced(int              const minMY,
               int              const minMX,
               int              const maxMY,
               int              const maxMX,
               int              const spacing,
               const LumBlock * const currentBlockP, 
               MpegFrame *      const prev,
               int              const by,
               int              const bx,
               int *            const bestDiffP, 
               vector *         const centerP) {
/*----------------------------------------------------------------------------
   Examine every 'spacing'th half-pixel within the rectangle 
   ('minBoundX', 'minBoundY', 'maxBoundX', 'maxBoundY'), 

   If one of the half-pixels examined has a lower "LumMotionError" value
   than *bestDiffP, update *bestDiffP to that value and update
   *centerP to the location of that half-pixel.
-----------------------------------------------------------------------------*/
    int const minBoundY = MAX(minMY, centerP->y - spacing);
    int const minBoundX = MAX(minMX, centerP->x - spacing);
    int const maxBoundY = MIN(maxMY, centerP->y + spacing + 1);
    int const maxBoundX = MIN(maxMX, centerP->x + spacing + 1);

    int my;

    for (my = minBoundY; my < maxBoundY; my += spacing) {
        int mx;

        for (mx = minBoundX; mx < maxBoundX; mx += spacing) {
            int diff;
            vector m;

            m.y = my; m.x = mx;
            
            diff = LumMotionError(currentBlockP, prev, by, bx, m, *bestDiffP);
            
            if (diff < *bestDiffP) {
                *centerP   = m;
                *bestDiffP = diff;
            }
        }
    }
}



/*===========================================================================*
 *
 * PLogarithmicSearch
 *
 *  uses logarithmic search to compute the P-frame vector
 *
 * RETURNS: motion vector
 *
 * SIDE EFFECTS:    none
 *
 * REFERENCE:  MPEG-I specification, pages 32-33
 *
 *===========================================================================*/
int
PLogarithmicSearch(const LumBlock * const currentBlockP,
                   MpegFrame *      const prev, 
                   int              const by,
                   int              const bx, 
                   vector *         const motionP,
                   int              const searchRange) {

    int const stepSize = (pixelFullSearch ? 2 : 1);

    int minMY, minMX, maxMY, maxMX;
    int spacing;  /* grid spacing */
    vector motion;
        /* Distance from (bx,by) (in half-pixels) of the block that is most
           like the current block among those that we have examined so far.
           (0,0) means we haven't examined any.
        */
    int bestDiff;
        /* The difference between the current block and the block offset
           'motion' from it.
        */
    
    COMPUTE_MOTION_BOUNDARY(by, bx, stepSize, minMY, minMX, maxMY, maxMX);
    minMX = max(minMX, - searchRange);
    minMY = max(minMY, - searchRange);
    maxMX = min(maxMX, + searchRange);
    maxMY = min(maxMY, + searchRange);

    /* Note: The clipping to 'searchRange' above may seem superfluous because
       the basic algorithm would never want to look more than 'searchRange'
       pixels away, but with rounding error, it can.
    */

    motion.x = motion.y = 0;
    bestDiff = INT_MAX;

    for (spacing = searchRange; spacing >= stepSize;) {
        if (stepSize == 2) {  /* make sure spacing is even */
            if (spacing == 2) 
                spacing = 0;
            else {
                spacing = (spacing+1)/2;
                if (spacing % 2 != 0) 
                    --spacing;
            }
        } else {
            if (spacing == 1) {
                spacing = 0;
            } else 
                spacing = (spacing + 1) / 2;
        }
        if (spacing >= stepSize) 
            findBestSpaced(minMY, minMX, maxMY, maxMX,
                           spacing, currentBlockP, prev, by, bx,
                           &bestDiff, &motion);
    }

    {
        int diff;
        /* check old motion -- see if it's better */
        if ((motionP->y >= minMY) && (motionP->y < maxMY) &&
            (motionP->x >= minMX) && (motionP->x < maxMX)) {
            diff = LumMotionError(currentBlockP, prev, by, bx, 
                                  *motionP, bestDiff);
        } else 
            diff = INT_MAX;
        
        if (bestDiff < diff)
            *motionP = motion;
        else 
            bestDiff = diff;
    }

    return bestDiff;
}



/*===========================================================================*
 *
 * PLocalSearch
 *
 *  uses local exhaustive search to compute the P-frame vector
 *
 * RETURNS: motion vector
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
int
PLocalSearch(const LumBlock * const currentBlockP,
             MpegFrame *      const prev, 
             int              const by,
             int              const bx,
             vector *         const motionP,
             int              const bestSoFar,
             int              const searchRange) {

    int mx, my;
    int bestDiff;
    int stepSize;
    int leftMY, leftMX;
    int rightMY, rightMX;
    int distance;
    int tempRightMY, tempRightMX;

    stepSize = (pixelFullSearch ? 2 : 1);

    COMPUTE_MOTION_BOUNDARY(by,bx,stepSize,leftMY,leftMX,rightMY,rightMX);

    /* try old motion vector first */
    if (VALID_MOTION(*motionP)) {
        bestDiff = LumMotionError(currentBlockP, prev, by, bx, 
                                  *motionP, bestSoFar);

        if (bestSoFar < bestDiff)
            bestDiff = bestSoFar;
    } else {
        motionP->y = motionP->x = 0;
        bestDiff = bestSoFar;
    }

    /* try a spiral pattern */    
    for (distance = stepSize; distance <= searchRange; distance += stepSize) {
        tempRightMY = MIN(distance, rightMY);
        tempRightMX = MIN(distance, rightMX);

        /* do top, bottom */
        for (my = -distance; my < tempRightMY;
             my += max(tempRightMY+distance-stepSize, stepSize)) {
            if (my >= leftMY) {
                for ( mx = -distance; mx < tempRightMX; mx += stepSize ) {
                    if (mx >= leftMX) {
                        int diff;
                        vector m;
                        
                        m.y = my; m.x = mx;
                        diff = LumMotionError(currentBlockP, prev, by, bx, m, 
                                              bestDiff);
                        
                        if (diff < bestDiff) {
                            *motionP = m;
                            bestDiff = diff;
                        }
                    }
                }
            }
        }

        /* do left, right */
        for (mx = -distance; mx < tempRightMX;
             mx += max(tempRightMX+distance-stepSize, stepSize)) {

            if (mx >= leftMX) {
                for (my = -distance+stepSize; my < tempRightMY-stepSize;
                     my += stepSize) {
                    if (my >= leftMY) {
                        int diff;
                        vector m;

                        m.y = my; m.x = mx;
                        diff = LumMotionError(currentBlockP, prev, by, bx, m,
                                              bestDiff);
                        
                        if (diff < bestDiff) {
                            *motionP = m;
                            bestDiff = diff;
                        }
                    }
                }
            }
        }
    }
    return bestDiff;
}


/*===========================================================================*
 *
 * PTwoLevelSearch
 *
 *  uses two-level search to compute the P-frame vector
 *  first does exhaustive full-pixel search, then looks at neighboring
 *  half-pixel motion vectors
 *
 * RETURNS: motion vector
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
int
PTwoLevelSearch(const LumBlock * const currentBlockP,
                MpegFrame *      const prev, 
                int              const by,
                int              const bx, 
                vector *         const motionP,
                int              const bestSoFar,
                int              const searchRange) {
    int mx, my;
    int loopInc;
    int diff, bestDiff;
    int leftMY, leftMX;
    int rightMY, rightMX;
    int distance;
    int tempRightMY, tempRightMX;
    int xOffset, yOffset;

    /* exhaustive full-pixel search first */

    COMPUTE_MOTION_BOUNDARY(by,bx,2,leftMY,leftMX,rightMY,rightMX);

    rightMY--;
    rightMX--;

    /* convert vector into full-pixel vector */
    if (motionP->y > 0) {
        if ((motionP->y % 2) == 1) {
            --motionP->y;
        }
    } else if (((-motionP->y) % 2) == 1)
        ++motionP->y;

    if (motionP->x > 0) {
        if ((motionP->x % 2) == 1)
            --motionP->x;
    } else if ((-motionP->x % 2) == 1)
        ++motionP->x;

    /* try old motion vector first */
    if (VALID_MOTION(*motionP)) {
        bestDiff = LumMotionError(currentBlockP, prev, by, bx, 
                                  *motionP, bestSoFar);

        if ( bestSoFar < bestDiff ) {
            bestDiff = bestSoFar;
        }
    } else {
        motionP->y = motionP->x = 0;
        bestDiff = bestSoFar;
    }

    ++rightMY;
    ++rightMX;

    /* try a spiral pattern */    
    for ( distance = 2; distance <= searchRange; distance += 2 ) {
        tempRightMY = MIN(distance, rightMY);
        tempRightMX = MIN(distance, rightMX);

        /* do top, bottom */
        loopInc = max(tempRightMY + distance - 2, 2);
        for (my = -distance; my < tempRightMY; my += loopInc) {
            if (my >= leftMY) {
                for (mx = -distance; mx < tempRightMX; mx += 2) {
                    if (mx >= leftMX) {
                        vector m;
                        m.y = my; m.x = mx;
                        diff = LumMotionError(currentBlockP, prev, by, bx, m, 
                                              bestDiff);
                        
                        if (diff < bestDiff) {
                            *motionP = m;
                            bestDiff = diff;
                        }
                    }
                }
            }
        }

        /* do left, right */
        loopInc = max(tempRightMX+distance-2, 2);
        for (mx = -distance; mx < tempRightMX; mx += loopInc) {
            if (mx >= leftMX) {
                for ( my = -distance+2; my < tempRightMY-2; my += 2 ) {
                    if (my >= leftMY) {
                        int diff;
                        vector m;
                        m.y = my; m.x = mx;
                        diff = LumMotionError(currentBlockP, prev, by, bx, m, 
                                              bestDiff);
                        
                        if ( diff < bestDiff ) {
                            *motionP = m;
                            bestDiff = diff;
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
                m.y = my+yOffset; m.x = mx+xOffset;
                if (VALID_MOTION(m)) {
                    int diff;
                    diff = LumMotionError(currentBlockP, prev, by, bx,
                                          m, bestDiff);
                    if (diff < bestDiff) {
                        *motionP = m;
                        bestDiff = diff;
                    }
                }
            }
        }
    }
    return bestDiff;
}



void
ShowPMVHistogram(fpointer)
    FILE *fpointer;
{
    register int x, y;
    int *columnTotals;
    int rowTotal;

    columnTotals = (int *) calloc(2*searchRangeP+3, sizeof(int));

#ifdef COMPLETE_DISPLAY
    fprintf(fpointer, "    ");
    for ( y = 0; y < 2*searchRange+3; y++ ) {
        fprintf(fpointer, "%3d ", y-searchRangeP-1);
    }
    fprintf(fpointer, "\n");
#endif

    for ( x = 0; x < 2*searchRangeP+3; x++ ) {
#ifdef COMPLETE_DISPLAY
        fprintf(fpointer, "%3d ", x-searchRangeP-1);
#endif
        rowTotal = 0;
        for ( y = 0; y < 2*searchRangeP+3; y++ ) {
            fprintf(fpointer, "%3d ", pmvHistogram[x][y]);
            rowTotal += pmvHistogram[x][y];
            columnTotals[y] += pmvHistogram[x][y];
        }
#ifdef COMPLETE_DISPLAY
        fprintf(fpointer, "%4d\n", rowTotal);
#else
        fprintf(fpointer, "\n");
#endif
    }

#ifdef COMPLETE_DISPLAY
    fprintf(fpointer, "Tot ");
    for ( y = 0; y < 2*searchRangeP+3; y++ ) {
        fprintf(fpointer, "%3d ", columnTotals[y]);
    }
#endif
    fprintf(fpointer, "\n");
}


void
ShowBBMVHistogram(fpointer)
    FILE *fpointer;
{
    register int x, y;
    int *columnTotals;
    int rowTotal;

    fprintf(fpointer, "B-frame Backwards:\n");

    columnTotals = (int *) calloc(2*searchRangeB+3, sizeof(int));

#ifdef COMPLETE_DISPLAY
    fprintf(fpointer, "    ");
    for ( y = 0; y < 2*searchRangeB+3; y++ ) {
        fprintf(fpointer, "%3d ", y-searchRangeB-1);
    }
    fprintf(fpointer, "\n");
#endif

    for ( x = 0; x < 2*searchRangeB+3; x++ ) {
#ifdef COMPLETE_DISPLAY
        fprintf(fpointer, "%3d ", x-searchRangeB-1);
#endif
        rowTotal = 0;
        for ( y = 0; y < 2*searchRangeB+3; y++ ) {
            fprintf(fpointer, "%3d ", bbmvHistogram[x][y]);
            rowTotal += bbmvHistogram[x][y];
            columnTotals[y] += bbmvHistogram[x][y];
        }
#ifdef COMPLETE_DISPLAY
        fprintf(fpointer, "%4d\n", rowTotal);
#else
        fprintf(fpointer, "\n");
#endif
    }

#ifdef COMPLETE_DISPLAY
    fprintf(fpointer, "Tot ");
    for ( y = 0; y < 2*searchRangeB+3; y++ ) {
        fprintf(fpointer, "%3d ", columnTotals[y]);
    }
#endif
    fprintf(fpointer, "\n");
}


void
ShowBFMVHistogram(fpointer)
    FILE *fpointer;
{
    register int x, y;
    int *columnTotals;
    int rowTotal;

    fprintf(fpointer, "B-frame Forwards:\n");

    columnTotals = (int *) calloc(2*searchRangeB+3, sizeof(int));

#ifdef COMPLETE_DISPLAY
    fprintf(fpointer, "    ");
    for ( y = 0; y < 2*searchRangeB+3; y++ ) {
        fprintf(fpointer, "%3d ", y-searchRangeB-1);
    }
    fprintf(fpointer, "\n");
#endif

    for ( x = 0; x < 2*searchRangeB+3; x++ ) {
#ifdef COMPLETE_DISPLAY
        fprintf(fpointer, "%3d ", x-searchRangeB-1);
#endif
        rowTotal = 0;
        for ( y = 0; y < 2*searchRangeB+3; y++ ) {
            fprintf(fpointer, "%3d ", bfmvHistogram[x][y]);
            rowTotal += bfmvHistogram[x][y];
            columnTotals[y] += bfmvHistogram[x][y];
        }
#ifdef COMPLETE_DISPLAY
        fprintf(fpointer, "%4d\n", rowTotal);
#else
        fprintf(fpointer, "\n");
#endif
    }

#ifdef COMPLETE_DISPLAY
    fprintf(fpointer, "Tot ");
    for ( y = 0; y < 2*searchRangeB+3; y++ ) {
        fprintf(fpointer, "%3d ", columnTotals[y]);
    }
#endif
    fprintf(fpointer, "\n");
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

/*  
 *  $Header: /u/smoot/md/mpeg_encode/RCS/psearch.c,v 1.9 1995/01/19 23:09:12 eyhung Exp $
 *  $Log: psearch.c,v $
 * Revision 1.9  1995/01/19  23:09:12  eyhung
 * Changed copyrights
 *
 * Revision 1.9  1995/01/19  23:09:12  eyhung
 * Changed copyrights
 *
 * Revision 1.8  1994/12/07  00:40:36  smoot
 * Added seperate P and B search ranges
 *
 * Revision 1.7  1994/11/12  02:09:45  eyhung
 * full pixel bug
 * fixed on lines 512 and 563
 *
 * Revision 1.6  1994/03/15  00:27:11  keving
 * nothing
 *
 * Revision 1.5  1993/12/22  19:19:01  keving
 * nothing
 *
 * Revision 1.4  1993/07/22  22:23:43  keving
 * nothing
 *
 * Revision 1.3  1993/06/30  20:06:09  keving
 * nothing
 *
 * Revision 1.2  1993/06/03  21:08:08  keving
 * nothing
 *
 * Revision 1.1  1993/03/02  18:27:05  keving
 * nothing
 *
 */


