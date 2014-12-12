/* pbm.c
 * code for reading the header of an ASCII PBM file
 * Copyright (c) 1998 Tim Norman.  See LICENSE for details
 * 2-25-98
 *
 * Mar 18, 1998  Jim Peterson  <jspeter@birch.ee.vt.edu>
 *
 *     Restructured to encapsulate more of the PBM handling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppapbm.h"

int make_pbm_stat(pbm_stat* pbm,FILE* fptr)
{
  char line[1024];

  pbm->fptr=fptr;
  pbm->version=none;
  pbm->current_line=0;
  pbm->unread = 0;

  if (fgets (line, 1024, fptr) == NULL)
    return 0;
  line[strlen(line)-1] = 0;

  if(!strcmp(line,"P1")) pbm->version=P1;
  if(!strcmp(line,"P4")) pbm->version=P4;
  if(pbm->version == none)
  {
    fprintf(stderr,"pbm_readheader(): unknown PBM magic '%s'\n",line);
    return 0;
  }

  do
    if (fgets (line, 1024, fptr) == NULL)
      return 0;
  while (line[0] == '#');

  if (2 != sscanf (line, "%d %d", &pbm->width, &pbm->height))
    return 0;

  return 1;
}

static int getbytes(FILE *fptr,int width,unsigned char* data)
{
  unsigned char mask,acc,*place;
  int num;

  if(!width) return 0;
  for(mask=0x80, acc=0, num=0, place=data; num<width; )
  {
    switch(getc(fptr))
    {
    case EOF:      
      return 0;
    case '1':
      acc|=mask;
      /* fall through */
    case '0':
      mask>>=1;
      num++;
      if(!mask) /* if(num%8 == 0) */
      {
	*place++ = acc;
	acc=0;
	mask=0x80;
      }
    }
  }
  if(width%8)
    *place=acc;
  return 1;
}

/* Reads a single line into data which must be at least (pbm->width+7)/8
   bytes of storage */
int pbm_readline(pbm_stat* pbm,unsigned char* data)
{
  int tmp,tmp2;

  if(pbm->current_line >= pbm->height) return 0;

  if (pbm->unread)
    {
      memcpy (data, pbm->revdata, (pbm->width+7)/8);
      pbm->current_line++;
      pbm->unread = 0;
      free (pbm->revdata);
      return 1;
    }

  switch(pbm->version)
  {
  case P1:
    if(getbytes(pbm->fptr,pbm->width,data))
    {
      pbm->current_line++;
      return 1;
    }
    return 0;

  case P4:
    tmp=(pbm->width+7)/8;
    tmp2=fread(data,1,tmp,pbm->fptr);
    if(tmp2 == tmp)
    {
      pbm->current_line++;
      return 1;
    }
    fprintf(stderr,"pbm_readline(): error reading line data (%d)\n",tmp2);
    return 0;

  default:
    fprintf(stderr,"pbm_readline(): unknown PBM version\n");
    return 0;
  }
}

/* push a line back into the buffer; we read too much! */
void pbm_unreadline (pbm_stat *pbm, void *data)
{
  /* can only store one line in the unread buffer */
  if (pbm->unread)
    return;

  pbm->unread = 1;
  pbm->revdata = malloc ((pbm->width+7)/8);
  memcpy (pbm->revdata, data, (pbm->width+7)/8);
  pbm->current_line--;
}
