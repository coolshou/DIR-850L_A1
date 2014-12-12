/*===========================================================================*
 * frame.h								     *
 *									     *
 *	basic frames procedures						     *
 *									     *
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


#ifndef FRAME_INCLUDED
#define FRAME_INCLUDED

/*==============*
 * HEADER FILES *
 *==============*/

#include "general.h"
#include "ansi.h"
#include "mtypes.h"

/*===========*
 * CONSTANTS *
 *===========*/
#define TYPE_IFRAME	2
#define TYPE_PFRAME	3
#define TYPE_BFRAME	4


/*=======================*
 * STRUCTURE DEFINITIONS *
 *=======================*/

typedef struct mpegFrame {
    int type;
    char    inputFileName[256];
    int id;           /* the frame number -- starts at 0 */
    boolean inUse;	/* TRUE iff this frame is currently being used */
			/* FALSE means any data here can be thrashed */

    /*  
     *  now, the YCrCb data.  All pixel information is stored in unsigned
     *  8-bit pieces.  We separate y, cr, and cb because cr and cb are
     *  subsampled by a factor of 2.
     *
     *  if orig_y is NULL, then orig_cr, orig_cb are undefined
     */
    uint8 **orig_y, **orig_cr, **orig_cb;

    /* now, the decoded data -- relevant only if
     *	    referenceFrame == DECODED_FRAME
     *
     * if decoded_y is NULL, then decoded_cr, decoded_cb are undefined 
     */
    uint8 **decoded_y, **decoded_cr, **decoded_cb;

    /* reference data */
    uint8 **ref_y, **ref_cr, **ref_cb;

    /*  
     *  these are the Blocks which will ultimately compose MacroBlocks.
     *  A Block is in a format that mp_fwddct() can crunch.
     *  if y_blocks is NULL, then cr_blocks, cb_blocks are undefined
     */
    Block **y_blocks, **cr_blocks, **cb_blocks;

    /*
     *  this is the half-pixel luminance data (for reference frames)
     */
    uint8 **halfX, **halfY, **halfBoth;

    boolean   halfComputed;        /* TRUE iff half-pixels already computed */

    struct mpegFrame *next;  /* points to the next B-frame to be encoded, if
		       * stdin is used as the input. 
		       */
} MpegFrame;


void
Frame_Init(unsigned int const numOfFramesRequested);

void
Frame_Exit(void);

void
Frame_Free(MpegFrame * const frameP);

MpegFrame *
Frame_New(int const id,
          int const type);

void
Frame_AllocBlocks(MpegFrame * const frameP);

void
Frame_AllocYCC(MpegFrame * const frameP);

void
Frame_AllocHalf(MpegFrame * const frameP);

void
Frame_AllocDecoded(MpegFrame * const frameP,
                   boolean     const makeReference);

void
Frame_Resize(MpegFrame * const omf,
             MpegFrame * const mf,
             int         const insize_x,
             int         const insize_y,
             int         const outsize_x,
             int         const outsize_y);


extern void	  Frame_Free _ANSI_ARGS_((MpegFrame * const frame));
extern void	  Frame_Exit _ANSI_ARGS_((void));
extern void	  Frame_AllocPPM _ANSI_ARGS_((MpegFrame * frame));
extern void	  Frame_AllocYCC _ANSI_ARGS_((MpegFrame * const mf));
extern void	  Frame_AllocDecoded _ANSI_ARGS_((MpegFrame * const frame,
						  boolean const makeReference));
extern void	  Frame_AllocHalf _ANSI_ARGS_((MpegFrame * const frame));
extern void	  Frame_AllocBlocks _ANSI_ARGS_((MpegFrame * const mf));
extern void	  Frame_Resize _ANSI_ARGS_((MpegFrame * const omf, MpegFrame * const mf,
					    int const insize_x, int const insize_y,
					    int const outsize_x, int const outsize_y));


#endif /* FRAME_INCLUDED */
