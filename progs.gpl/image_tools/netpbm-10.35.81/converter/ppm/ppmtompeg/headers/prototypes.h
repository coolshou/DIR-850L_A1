/*===========================================================================*
 * prototypes.h                                  *
 *                                       *
 *  miscellaneous prototypes                         *
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

#include "general.h"
#include "ansi.h"
#include "frame.h"


/*===============================*
 * EXTERNAL PROCEDURE prototypes *
 *===============================*/

int GetBQScale _ANSI_ARGS_((void));
int GetPQScale _ANSI_ARGS_((void));
void    ResetBFrameStats _ANSI_ARGS_((void));
void    ResetPFrameStats _ANSI_ARGS_((void));
void SetSearchRange (int const pixelsP,
                     int const pixelsB);
void    ResetIFrameStats _ANSI_ARGS_((void));
void
SetPixelSearch(const char * const searchType);
void    SetIQScale _ANSI_ARGS_((int const qI));
void    SetPQScale _ANSI_ARGS_((int qP));
void    SetBQScale _ANSI_ARGS_((int qB));
float   EstimateSecondsPerIFrame _ANSI_ARGS_((void));
float   EstimateSecondsPerPFrame _ANSI_ARGS_((void));
float   EstimateSecondsPerBFrame _ANSI_ARGS_((void));
void    SetGOPSize _ANSI_ARGS_((int size));
void
SetStatFileName(const char * const fileName);
void    SetSlicesPerFrame _ANSI_ARGS_((int const number));
void    SetBlocksPerSlice _ANSI_ARGS_((void));


void DCTFrame _ANSI_ARGS_((MpegFrame * mf));

void PPMtoYCC _ANSI_ARGS_((MpegFrame * mf));

void    MotionSearchPreComputation _ANSI_ARGS_((MpegFrame * const frame));

void    ComputeHalfPixelData _ANSI_ARGS_((MpegFrame *frame));
void mp_validate_size _ANSI_ARGS_((int *x, int *y));

extern void SetFCode _ANSI_ARGS_((void));


/* psearch.c */
void    ShowPMVHistogram _ANSI_ARGS_((FILE *fpointer));
void    ShowBBMVHistogram _ANSI_ARGS_((FILE *fpointer));
void    ShowBFMVHistogram _ANSI_ARGS_((FILE *fpointer));
