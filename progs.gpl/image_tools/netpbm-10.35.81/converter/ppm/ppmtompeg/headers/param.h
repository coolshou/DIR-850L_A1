/*===========================================================================*
 * param.h								     *
 *									     *
 *	reading the parameter file					     *
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

#include "pm_c_util.h"
#include "ansi.h"
#include "input.h"


/*===========*
 * CONSTANTS *
 *===========*/

#define MAX_MACHINES	    256
#ifndef MAXPATHLEN
#define MAXPATHLEN  1024
#endif

#define	ENCODE_FRAMES	0
#define COMBINE_GOPS	1
#define COMBINE_FRAMES	2


struct params {
    struct inputSource * inputSourceP;
    bool warnUnderflow;
    bool warnOverflow;
};


void
ReadParamFile(const char *    const fileName, 
              int             const function,
              struct params * const paramP);

/*==================*
 * GLOBAL VARIABLES *
 *==================*/

/* All this stuff ought to be in a struct param instead */

extern char	outputFileName[256];
extern int	whichGOP;
extern int numMachines;
extern char	machineName[MAX_MACHINES][256];
extern char	userName[MAX_MACHINES][256];
extern char	executable[MAX_MACHINES][1024];
extern char	remoteParamFile[MAX_MACHINES][1024];
extern boolean	remote[MAX_MACHINES];
extern char	currentPath[MAXPATHLEN];
extern char	currentFramePath[MAXPATHLEN];
extern char	currentGOPPath[MAXPATHLEN];
extern char inputConversion[1024];
extern char yuvConversion[256];
extern int  yuvWidth, yuvHeight;
extern int  realWidth, realHeight;
extern char ioConversion[1024];
extern char slaveConversion[1024];
extern FILE *bitRateFile;
extern boolean showBitRatePerFrame;
extern boolean computeMVHist;
extern const double VidRateNum[9];
extern boolean keepTempFiles;
