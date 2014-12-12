/*===========================================================================*
 * frame.c                                   *
 *                                       *
 *  basic frame procedures                           *
 *                                       *
 * EXPORTED PROCEDURES:                              *
 *  Frame_Init                               *
 *  Frame_Exit                               *
 *  Frame_New                                *
 *  Frame_Free                               *
 *  Frame_AllocBlocks                            *
 *  Frame_AllocYCC                               *
 *  Frame_AllocDecoded                           *
 *  Frame_AllocHalf                                  *
 *  Frame_Resize                                     * 
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


#include "mallocvar.h"

#include "all.h"
#include "mtypes.h"
#include "frames.h"
#include "frame.h"
#include "fsize.h"
#include "dct.h"

/*===========*
 * CONSTANTS *
 *===========*/

/* The maximum number of B-Frames allowed between reference frames. */
#define  B_FRAME_RUN  16    

/*==================*
 * GLOBAL VARIABLES *
 *==================*/

static MpegFrame * frameMemory[B_FRAME_RUN+2];
static unsigned int numOfFrames;


/*====================================================
* Resize_Array_Width
*    
*   This function will resize any array width up
* or down in size.  The algorithm is based on the
* least common multiple approach more commonly
* used in audio frequency adjustments.
*=====================================================*/
static void 
Resize_Array_Width(uint8 ** const inarray,
                   int      const in_x,
                   int      const in_y,
                   uint8 ** const outarray,
                   int      const out_x) {

    unsigned int i;
    int in_total;
    int out_total;
    uint8 *inptr;
    uint8 *outptr;
    uint8 pointA,pointB;
    /* double slope,diff; */
    
    for (i = 0; i < in_y; ++i) {     /* For each row */
        unsigned int j;
        inptr = &inarray[i][0];
        outptr = &outarray[i][0];
        in_total = 0;
        out_total = 0;
        for (j=0; j < out_x; ++j) {      /* For each output value */
            if (in_total == out_total) {
                *outptr = *inptr;
                outptr++;
                out_total=out_total+in_x;
                while(in_total < out_total){
                    in_total = in_total + out_x;
                    ++inptr;
                }
                if (in_total > out_total) {
                    in_total = in_total - out_x;
                    --inptr;
                }
            } else {  
                pointA = *inptr;
                ++inptr;
                pointB = *inptr;
                --inptr;
#if 0
                /*Interpolative solution */
                slope = ((double)(pointB -pointA))/((double)(out_x));
                diff = (((double)(out_total - in_total)));
                if (diff < (out_x/2)){
                    *outptr = (pointA + (uint8)(slope*diff));
                } else {
                    *outptr = (pointB -
                               (uint8)(slope*(((float)(out_x)) - diff)));
                } 
#endif
                /* Non-Interpolative solution */
                *outptr = *inptr;  

                ++outptr;
                out_total = out_total + in_x;
                while(in_total < out_total) {
                    in_total = in_total + out_x;
                    ++inptr;
                }
                if (in_total > out_total) {
                    in_total = in_total - out_x;
                    --inptr;
                }
            }  /* end if */
        }  /* end for each output value */
    }  /* end for each row */
}  /* end main */



/*==============================
* Resize_Array_Height
*
*    Resize any array height larger or smaller.
* Same as Resize_array_Width except pointer
* manipulation must change.
*===============================*/
static void 
Resize_Array_Height(uint8 ** const inarray,
                    int      const in_x,
                    int      const in_y,
                    uint8 ** const outarray,
                    int      const out_y) {

    unsigned int i;

    for(i=0; i < in_x; ++i){    /* for each column */
        int in_total;
        int out_total;
        uint8 pointA, pointB;
        double slope, diff;
        unsigned int j;
        int k;

        in_total = 0;
        out_total = 0;
        k = 0;
        for(j=0; j < out_y; ++j){  /* for each output value */
            if (in_total == out_total) {  
                outarray[j][i] = inarray[k][i];
                out_total=out_total+in_y;
                while(in_total < out_total){
                    in_total = in_total + out_y;
                    ++k;
                }
                if (in_total > out_total) {
                    in_total = in_total - out_y;
                    --k;
                }
            } else {  
                pointA = inarray[k][i];
                if (k != (in_y -1)) {
                    pointB = inarray[k+1][i];
                } else
                    pointB = pointA;
                /* Interpolative case */
                slope = ((double)(pointB -pointA))/(double)(out_y);
                diff = (double)(out_total - in_total);
                /*  outarray[j][i] = (inarray[k][i] + (uint8)(slope*diff)); */
                /* Non-Interpolative case */
                outarray[j][i] = inarray[k][i];
                out_total = out_total + in_y;
                while (in_total < out_total) {
                    in_total = in_total + out_y;
                    ++k;
                }
                if (in_total > out_total){
                    in_total = in_total - out_y;
                    --k;
                }
            } 
        }
    }
}



/*========================================================
* Resize_Width
*======================================================*/
static void  
Resize_Width(MpegFrame * const omfrw,
             MpegFrame * const mfrw,
             int         const in_x,
             int         const in_y,
             int         const out_x) {

    int y;

    omfrw->orig_y = NULL;
    Fsize_x = out_x;

    /* Allocate new frame memory */
    MALLOCARRAY(omfrw->orig_y, Fsize_y);
    ERRCHK(omfrw->orig_y, "malloc");
    for (y = 0; y < Fsize_y; ++y) {
        MALLOCARRAY(omfrw->orig_y[y], out_x);
        ERRCHK(omfrw->orig_y[y], "malloc");
    }

    MALLOCARRAY(omfrw->orig_cr, Fsize_y / 2);
    ERRCHK(omfrw->orig_cr, "malloc");
    for (y = 0; y < Fsize_y / 2; ++y) {
        MALLOCARRAY(omfrw->orig_cr[y], out_x / 2);
        ERRCHK(omfrw->orig_cr[y], "malloc");
    }

    MALLOCARRAY(omfrw->orig_cb, Fsize_y / 2);
    ERRCHK(omfrw->orig_cb, "malloc");
    for (y = 0; y < Fsize_y / 2; ++y) {
        MALLOCARRAY(omfrw->orig_cb[y], out_x / 2);
        ERRCHK(omfrw->orig_cb[y], "malloc");
    }

    if (referenceFrame == ORIGINAL_FRAME) {
        omfrw->ref_y = omfrw->orig_y;
        omfrw->ref_cr = omfrw->orig_cr;
        omfrw->ref_cb = omfrw->orig_cb;
    }

    /* resize each component array separately */
    Resize_Array_Width(mfrw->orig_y, in_x, in_y, omfrw->orig_y, out_x);
    Resize_Array_Width(mfrw->orig_cr, (in_x/2), (in_y/2), omfrw->orig_cr,
                       (out_x/2));
    Resize_Array_Width(mfrw->orig_cb, (in_x/2), (in_y/2), omfrw->orig_cb,
                       (out_x/2));

    /* Free old frame memory */
    if (mfrw->orig_y) {
        unsigned int i;
        for (i = 0; i < in_y; ++i) {
            free(mfrw->orig_y[i]);
        }
        free(mfrw->orig_y);
        
        for (i = 0; i < in_y / 2; ++i) {
            free(mfrw->orig_cr[i]);
        }
        free(mfrw->orig_cr);
        
        for (i = 0; i < in_y / 2; ++i) {
            free(mfrw->orig_cb[i]);
        }
        free(mfrw->orig_cb);
    }
}



/*=======================================================
* Resize_Height
*
*   Resize Frame height up or down
*=======================================================*/
static  void
Resize_Height(MpegFrame * const omfrh,
              MpegFrame * const mfrh,
              int         const in_x,
              int         const in_y,
              int         const out_y) {
    
    unsigned int y; 
    
    Fsize_y = out_y;

    /* Allocate new frame memory */
    MALLOCARRAY(omfrh->orig_y, out_y);
    ERRCHK(omfrh->orig_y, "malloc");
    for (y = 0; y < out_y; ++y) {
        MALLOCARRAY(omfrh->orig_y[y], Fsize_x);
        ERRCHK(omfrh->orig_y[y], "malloc");
    }

    MALLOCARRAY(omfrh->orig_cr, out_y / 2);
    ERRCHK(omfrh->orig_cr, "malloc");
    for (y = 0; y < out_y / 2; ++y) {
        MALLOCARRAY(omfrh->orig_cr[y], Fsize_x / 2);
        ERRCHK(omfrh->orig_cr[y], "malloc");
    }

    MALLOCARRAY(omfrh->orig_cb, out_y / 2);
    ERRCHK(omfrh->orig_cb, "malloc");
    for (y = 0; y < out_y / 2; ++y) {
        MALLOCARRAY(omfrh->orig_cb[y], Fsize_x / 2);
        ERRCHK(omfrh->orig_cb[y], "malloc");
    }

    if (referenceFrame == ORIGINAL_FRAME) {
        omfrh->ref_y = omfrh->orig_y;
        omfrh->ref_cr = omfrh->orig_cr;
        omfrh->ref_cb = omfrh->orig_cb;
    }

    /* resize component arrays separately */
    Resize_Array_Height(mfrh->orig_y, in_x, in_y, omfrh->orig_y, out_y);
    Resize_Array_Height(mfrh->orig_cr, (in_x/2), (in_y/2), omfrh->orig_cr,
                        (out_y/2));
    Resize_Array_Height(mfrh->orig_cb, (in_x/2), (in_y/2), omfrh->orig_cb,
                        (out_y/2));

    /* Free old frame memory */
    if (mfrh->orig_y) {
        unsigned int i;
        for (i = 0; i < in_y; ++i) {
            free(mfrh->orig_y[i]);
        }
        free(mfrh->orig_y);
        
    for (i = 0; i < in_y / 2; ++i) {
        free(mfrh->orig_cr[i]);
    }
    free(mfrh->orig_cr);
    
    for (i = 0; i < in_y / 2; ++i) {
        free(mfrh->orig_cb[i]);
    }
    free(mfrh->orig_cb);
    }
}



/*===========================================================================*
 *
 * Frame_Init
 *
 *  initializes the memory associated with all frames ever
 *  If the input is not coming in from stdin, only 3 frames are needed ;
 *      else, the program must create frames equal to the greatest distance
 *      between two reference frames to hold the B frames while it is parsing
 *      the input from stdin.
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    frameMemory, numOfFrames
 *
 *===========================================================================*/
void
Frame_Init(unsigned int const numOfFramesRequested) {
    int idx;

    numOfFrames = numOfFramesRequested;

    for (idx = 0; idx < numOfFrames; ++idx) {
        MALLOCVAR(frameMemory[idx]);
        frameMemory[idx]->inUse = FALSE;
        frameMemory[idx]->orig_y = NULL;    
        frameMemory[idx]->y_blocks = NULL; 
        frameMemory[idx]->decoded_y = NULL;
        frameMemory[idx]->halfX = NULL;
        frameMemory[idx]->next = NULL;
    }
}


/*===========================================================================*
 *
 * FreeFrame
 *
 *  frees the memory associated with the given frame
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
FreeFrame(MpegFrame * const frameP) {

    if (frameP) {
        if (frameP->orig_y) {
            unsigned int i;
            for (i = 0; i < Fsize_y; ++i)
                free(frameP->orig_y[i]);
            free(frameP->orig_y);

            for (i = 0; i < (Fsize_y / 2); ++i)
                free(frameP->orig_cr[i]);
            free(frameP->orig_cr);

            for (i = 0; i < (Fsize_y / 2); ++i)
                free(frameP->orig_cb[i]);
            free(frameP->orig_cb);
        }
        if (frameP->decoded_y) {
            unsigned int i;
            for (i = 0; i < Fsize_y; ++i)
                free(frameP->decoded_y[i]);
            free(frameP->decoded_y);

            for (i = 0; i < (Fsize_y / 2); ++i)
                free(frameP->decoded_cr[i]);
            free(frameP->decoded_cr);

            for (i = 0; i < (Fsize_y / 2); ++i)
                free(frameP->decoded_cb[i]);
            free(frameP->decoded_cb);
        }

        if (frameP->y_blocks) {
            unsigned int i;
            for (i = 0; i < Fsize_y / DCTSIZE; ++i)
                free(frameP->y_blocks[i]);
            free(frameP->y_blocks);

            for (i = 0; i < Fsize_y / (2 * DCTSIZE); ++i)
                free(frameP->cr_blocks[i]);
            free(frameP->cr_blocks);

            for (i = 0; i < Fsize_y / (2 * DCTSIZE); ++i)
                free(frameP->cb_blocks[i]);
            free(frameP->cb_blocks);
        }
        if (frameP->halfX) {
            unsigned int i;
            for ( i = 0; i < Fsize_y; ++i )
                free(frameP->halfX[i]);
            free(frameP->halfX);
            
            for (i = 0; i < Fsize_y-1; ++i)
                free(frameP->halfY[i]);
            free(frameP->halfY);
            
            for (i = 0; i < Fsize_y-1; ++i)
                free(frameP->halfBoth[i]);
            free(frameP->halfBoth);
        }
        free(frameP);
    }
}



/*===========================================================================*
 *
 * Frame_Exit
 *
 *  frees the memory associated with frames
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    frameMemory
 *
 *===========================================================================*/
void
Frame_Exit(void) {

    int idx;

    for (idx = 0; idx < numOfFrames; ++idx) {
        FreeFrame(frameMemory[idx]);
    }
}


/*===========================================================================*
 *
 * Frame_Free
 *
 *  frees the given frame -- allows it to be re-used
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
Frame_Free(MpegFrame * const frameP) {
    frameP->inUse = FALSE;
}



/*===========================================================================*
 *
 * GetUnusedFrame
 *
 *  return an unused frame
 *
 * RETURNS: the frame
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static MpegFrame *
GetUnusedFrame() {
    unsigned int idx;

    for (idx = 0; idx < numOfFrames; ++idx) {
        if (!frameMemory[idx]->inUse) {
            frameMemory[idx]->inUse = TRUE;
            break;
        }
    }
    if (idx >= numOfFrames) {
        fprintf(stderr, "ERROR:  No unused frames!!!\n");
        fprintf(stderr, "        If you are using stdin for input, "
                "it is likely that you have too many\n");
        fprintf(stderr, "        B-frames between two reference frames.  "
                "See the man page for help.\n");
        exit(1);
    }
    return frameMemory[idx]; 
}



/*===========================================================================*
 *
 * ResetFrame
 *
 *  reset a frame to the given id and type
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
ResetFrame(int         const id,
           int         const type,
           MpegFrame * const frame) {

    switch (type) {
    case 'i':
        frame->type = TYPE_IFRAME;
        break;
    case 'p':
        frame->type = TYPE_PFRAME;
        break;
    case 'b':
        frame->type = TYPE_BFRAME;
        break;
    default:
        fprintf(stderr, "Invalid MPEG frame type %c\n", type);
        exit(1);
    }

    frame->id = id;
    frame->halfComputed = FALSE;
    frame->next = NULL;
}



/*===========================================================================*
 *
 * Frame_New
 *
 *  finds a frame that isn't currently being used and resets it
 *
 * RETURNS: the frame
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
MpegFrame *
Frame_New(int const id,
          int const type) {

    MpegFrame *frame;

    frame = GetUnusedFrame();
    ResetFrame(id, type, frame);

    return frame;
}



/*===========================================================================*
 *
 * Frame_AllocBlocks
 *
 *  allocate memory for blocks for the given frame, if required
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
Frame_AllocBlocks(MpegFrame * const frameP) {

    if (frameP->y_blocks != NULL) {
        /* already allocated */
    } else {
        int const dctx = Fsize_x / DCTSIZE;
        int const dcty = Fsize_y / DCTSIZE;

        unsigned int i;

        MALLOCARRAY(frameP->y_blocks, dcty);
        ERRCHK(frameP->y_blocks, "malloc");
        for (i = 0; i < dcty; ++i) {
            MALLOCARRAY(frameP->y_blocks[i], dctx);
            ERRCHK(frameP->y_blocks[i], "malloc");
        }
    
        MALLOCARRAY(frameP->cr_blocks, dcty / 2);
        ERRCHK(frameP->cr_blocks, "malloc");
        MALLOCARRAY(frameP->cb_blocks, dcty / 2);
        ERRCHK(frameP->cb_blocks, "malloc");
        for (i = 0; i < (dcty / 2); ++i) {
            MALLOCARRAY(frameP->cr_blocks[i], dctx / 2);
            ERRCHK(frameP->cr_blocks[i], "malloc");
            MALLOCARRAY(frameP->cb_blocks[i], dctx / 2);
            ERRCHK(frameP->cb_blocks[i], "malloc");
        }
    }
}



/*===========================================================================*
 *
 * Frame_AllocYCC
 *
 *  allocate memory for YCC info for the given frame, if required
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
Frame_AllocYCC(MpegFrame * const frameP) {

    if (frameP->orig_y != NULL) {
        /* already allocated */
    } else {
        unsigned int y;
    
        DBG_PRINT(("ycc_calc:\n"));
        /*
         * first, allocate tons of memory
         */
        MALLOCARRAY(frameP->orig_y, Fsize_y);
        ERRCHK(frameP->orig_y, "malloc");
        for (y = 0; y < Fsize_y; ++y) {
            MALLOCARRAY(frameP->orig_y[y], Fsize_x);
            ERRCHK(frameP->orig_y[y], "malloc");
        }

        MALLOCARRAY(frameP->orig_cr, Fsize_y / 2);
        ERRCHK(frameP->orig_cr, "malloc");
        for (y = 0; y < (Fsize_y / 2); ++y) {
            MALLOCARRAY(frameP->orig_cr[y], Fsize_x / 2);
            ERRCHK(frameP->orig_cr[y], "malloc");
        }

        MALLOCARRAY(frameP->orig_cb, Fsize_y / 2);
        ERRCHK(frameP->orig_cb, "malloc");
        for (y = 0; y < (Fsize_y / 2); ++y) {
            MALLOCARRAY(frameP->orig_cb[y], Fsize_x / 2);
            ERRCHK(frameP->orig_cb[y], "malloc");
        }

        if (referenceFrame == ORIGINAL_FRAME) {
            frameP->ref_y  = frameP->orig_y;
            frameP->ref_cr = frameP->orig_cr;
            frameP->ref_cb = frameP->orig_cb;
        }
    }
}



/*===========================================================================*
 *
 * Frame_AllocHalf
 *
 *  allocate memory for half-pixel values for the given frame, if required
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
Frame_AllocHalf(MpegFrame * const frameP) {

    if (frameP->halfX != NULL) {
    } else {
        unsigned int y;

        MALLOCARRAY(frameP->halfX, Fsize_y);
        ERRCHK(frameP->halfX, "malloc");
        for (y = 0; y < Fsize_y; ++y) {
            MALLOCARRAY(frameP->halfX[y], Fsize_x - 1);
            ERRCHK(frameP->halfX[y], "malloc");
        }
        MALLOCARRAY(frameP->halfY, Fsize_y - 1);
        ERRCHK(frameP->halfY, "malloc");
        for (y = 0; y < Fsize_y - 1; ++y) {
            MALLOCARRAY(frameP->halfY[y], Fsize_x);
            ERRCHK(frameP->halfY[y], "malloc");
        }
        MALLOCARRAY(frameP->halfBoth, Fsize_y - 1);
        ERRCHK(frameP->halfBoth, "malloc");
        for (y = 0; y < Fsize_y - 1; ++y) {
            MALLOCARRAY(frameP->halfBoth[y], Fsize_x - 1);
            ERRCHK(frameP->halfBoth[y], "malloc");
        }
    }
}



/*===========================================================================*
 *
 * Frame_AllocDecoded
 *
 *  allocate memory for decoded frame for the given frame, if required
 *  if makeReference == TRUE, then makes it reference frame
 * 
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
Frame_AllocDecoded(MpegFrame * const frameP,
                   boolean     const makeReference) {

    if (frameP->decoded_y != NULL) {
        /* already allocated */
    } else {
        unsigned int y;

        /* allocate memory for decoded image */
        /* can probably reuse original image memory, but may decide to use
           it for some reason, so do it this way at least for now -- more
           flexible
        */
        MALLOCARRAY(frameP->decoded_y, Fsize_y);
        ERRCHK(frameP->decoded_y, "malloc");
        for (y = 0; y < Fsize_y; ++y) {
            MALLOCARRAY(frameP->decoded_y[y], Fsize_x);
            ERRCHK(frameP->decoded_y[y], "malloc");
        }
        
        MALLOCARRAY(frameP->decoded_cr, Fsize_y / 2);
        ERRCHK(frameP->decoded_cr, "malloc");
        for (y = 0; y < (Fsize_y / 2); ++y) {
            MALLOCARRAY(frameP->decoded_cr[y], Fsize_x / 2);
            ERRCHK(frameP->decoded_cr[y], "malloc");
        }
        
        MALLOCARRAY(frameP->decoded_cb, Fsize_y / 2);
        ERRCHK(frameP->decoded_cb, "malloc");
        for (y = 0; y < (Fsize_y / 2); ++y) {
            MALLOCARRAY(frameP->decoded_cb[y], Fsize_x / 2);
            ERRCHK(frameP->decoded_cb[y], "malloc");
        }

        if (makeReference) {
            frameP->ref_y  = frameP->decoded_y;
            frameP->ref_cr = frameP->decoded_cr;
            frameP->ref_cb = frameP->decoded_cb;
        }
    }
}



/*===============================================================
 *
 * Frame_Resize                  by James Boucher
 *                Boston University Multimedia Communications Lab
 *  
 *     This function takes the mf input frame, read in READFrame(),
 * and resizes all the input component arrays to the output
 * dimensions specified in the parameter file as OUT_SIZE.
 * The new frame is returned with the omf pointer.  As well,
 * the values of Fsize_x and Fsize_y are adjusted.
 ***************************************************************/
void
Frame_Resize(MpegFrame * const omf,
             MpegFrame * const mf,
             int         const insize_x,
             int         const insize_y,
             int         const outsize_x,
             int         const outsize_y) {

    MpegFrame * frameAP;  /* intermediate frame */

    MALLOCVAR_NOFAIL(frameAP);
    
    if (insize_x != outsize_x && insize_y != outsize_y) {
        Resize_Width(frameAP, mf, insize_x, insize_y, outsize_x);
        Resize_Height(omf, frameAP, outsize_x, insize_y, outsize_y);
    } else 
        if (insize_x ==outsize_x && insize_y != outsize_y) {
            Resize_Height(omf, mf, insize_x, insize_y, outsize_y);
        } else
            if (insize_x !=outsize_x && insize_y == outsize_y) {
                Resize_Width(omf, mf, insize_x, insize_y, outsize_x);
            } else
                exit(1);

    free(frameAP);
    free(mf);
}
