/* pbm.h
 * Copyright (c) 1998 Tim Norman.  See LICENSE for details
 * 2-25-98
 *
 * Mar 18, 1998  Jim Peterson  <jspeter@birch.ee.vt.edu>
 *
 *     Restructured to encapsulate more of the PBM handling.
 */
#ifndef _PBM_H
#define _PBM_H

#include <stdio.h>

typedef struct
{
  FILE* fptr;
  enum { none, P1, P4 } version;
  int width, height;
  int current_line;
  void *revdata;
  int unread;
} pbm_stat;

int make_pbm_stat(pbm_stat*,FILE*);
int pbm_readline(pbm_stat*,unsigned char*); 
  /* reads a single line into char* */
void pbm_unreadline(pbm_stat*,void*); /* pushes a single line back */

#endif
