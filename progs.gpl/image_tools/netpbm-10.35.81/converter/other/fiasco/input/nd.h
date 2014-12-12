/*
 *  nd.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO (�F�ractal �I�mage �A�nd �S�equence �CO�dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:50:13 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef _ND_H
#define _ND_H

#include "wfa.h"
#include "rpf.h"
#include "bit-io.h"

void
read_nd (wfa_t *wfa, bitfile_t *input);

#endif /* not _ND_H */

