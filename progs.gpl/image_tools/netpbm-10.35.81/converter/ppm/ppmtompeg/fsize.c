/*===========================================================================*
 * fsize.c								     *
 *									     *
 *	procedures to keep track of frame size				     *
 *									     *
 * EXPORTED PROCEDURES:							     *
 *	Fsize_Reset							     *
 *	Fsize_Note							     *
 *	Fsize_Validate							     *
 *									     *
 * EXPORTED VARIABLES:							     *
 *	Fsize_x								     *
 *	Fsize_y								     *
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


/*==============*
 * HEADER FILES *
 *==============*/

#include "all.h"
#include "fsize.h"
#include "dct.h"


/*==================*
 * GLOBAL VARIABLES *
 *==================*/
int Fsize_x = 0;
int Fsize_y = 0;


/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 * Fsize_Reset
 *
 *	reset the frame size to 0
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    Fsize_x, Fsize_y
 *
 *===========================================================================*/
void
Fsize_Reset(void) {
    Fsize_x = Fsize_y = 0;
}



/*===========================================================================*
 *
 * Fsize_Validate
 *
 *	make sure that the x, y values are 16-pixel aligned
 *
 * RETURNS:	modifies the x, y values to 16-pixel alignment
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
Fsize_Validate(int * const xP,
               int * const yP) {

    *xP &= ~(DCTSIZE * 2 - 1);  /* Zero the low-order bits */
    *yP &= ~(DCTSIZE * 2 - 1);  /* Zero the low-order bits */
}


/*===========================================================================*
 *
 * Fsize_Note
 *
 *	note the given frame size and modify the global values as appropriate
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    Fsize_x, Fsize_y
 *
 *===========================================================================*/
void
Fsize_Note(int          const id,
           unsigned int const width,
           unsigned int const height) {

    Fsize_x = width;
    Fsize_y = height;
    Fsize_Validate(&Fsize_x, &Fsize_y);

    if ((Fsize_x == 0) || (Fsize_y == 0)) {
        fprintf(stderr,"Frame %d:  size is less than the minimum: %d x %d!\n",
                id, DCTSIZE*2, DCTSIZE*2);
        exit(1);
    }

#ifdef BLEAH
    if (Fsize_x == 0) {
	Fsize_x = width;
	Fsize_y = height;
	Fsize_Validate(&Fsize_x, &Fsize_y);
    } else if (width < Fsize_x || height < Fsize_y) {
	fprintf(stderr, "Frame %d: wrong size: (%d,%d).  Should be greater or equal to: (%d,%d)\n",
		id, width, height, Fsize_x, Fsize_y);
	exit(1);
    }
#endif
}
