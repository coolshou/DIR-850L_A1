/* cutswath.c
 * functions to cut a swath of a PBM file for PPA printers
 * Copyright (c) 1998 Tim Norman.  See LICENSE for details.
 * 3-15-98
 *
 * Mar 15, 1998  Jim Peterson  <jspeter@birch.ee.vt.edu>
 *
 *     Structured to accommodate both the HP820/1000, and HP720 series.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppa.h"
#include "ppapbm.h"
#include "cutswath.h"

extern int Width;
extern int Height;
extern int Pwidth;

/* sweep_data->direction must be set already */
/* Upon successful completion, sweep_data->image_data and
   sweep_data->nozzle_data have been set to pointers which this routine
   malloc()'d. */
/* Upon successful completion, all members of *sweep_data have been set
   except direction, vertical_pos, and next. */
/* Returns: 0 if unsuccessful
            1 if successful, but with non-printing result (end of page)
            2 if successful, with printing result */
int 
cut_pbm_swath(pbm_stat* pbm,ppa_stat* prn,int maxlines,ppa_sweep_data* sweep_data)
{
  unsigned char *data, *ppa, *place, *maxplace;
  int p_width, width8, p_width8;
  int i, j, left, right, got_nonblank, numlines;
  int horzpos, hp2;
  int shift;
  ppa_nozzle_data nozzles[2];

  /* shift = 6 if DPI==300  */
  /* shift = 12 if DPI==600 */ 
  shift = ( prn->DPI == 300 ? 6:12 ) ;
  
  /* safeguard against the user freeing these */
  sweep_data->image_data=NULL;
  sweep_data->nozzle_data=NULL;

  /* read the data from the input file */
  width8 = (pbm->width + 7) / 8;
 
/* 
  fprintf(stderr,"cutswath(): width=%u\n",pbm->width);
  fprintf(stderr,"cutswath(): height=%u\n",pbm->height);
*/

  if ((data=malloc(width8*maxlines)) == NULL)
  {
    fprintf(stderr,"cutswath(): could not malloc data storage\n");
    return 0;
  }

  /* ignore lines that are above the upper margin */
  while(pbm->current_line < prn->top_margin)
    if(!pbm_readline(pbm,data))
    {
      fprintf(stderr,"cutswath(): A-could not read top margin\n");
      free(data);
      return 0;
    }

  /* eat all lines that are below the lower margin */
  if(pbm->current_line >= Height - prn->bottom_margin)
  {
    while(pbm->current_line < pbm->height)
      if(!pbm_readline(pbm,data))
      {
	fprintf(stderr,"cutswath(): could not clear bottom margin\n");
	free(data);
	return 0;
      }
    free(data);
    return 1;
  }

  left = Pwidth-prn->right_margin/8;
  right = prn->left_margin/8;

  /* eat all beginning blank lines and then up to maxlines or lower margin */
  got_nonblank=numlines=0;
  while( (pbm->current_line < Height-prn->bottom_margin) &&
	 (numlines < maxlines) )
  {
    if(!pbm_readline(pbm,data+width8*numlines))
    {
      fprintf(stderr,"cutswath(): B-could not read next line\n");
      free(data);
      return 0;
    }
    if(!got_nonblank)
      for(j=prn->left_margin/8; j<left; j++)
	if(data[j])
	{
	  left = j;
	  got_nonblank=1;
	  break;
	}
    if(got_nonblank)
      {
	int newleft = left, newright = right;

	/* find left-most nonblank */
	for (i = prn->left_margin/8; i < left; i++)
	  if (data[width8*numlines+i])
	    {
	      newleft = i;
	      break;
	    }
	/* find right-most nonblank */
	for (i = Pwidth-prn->right_margin/8-1; i >= right; i--)
	  if (data[width8*numlines+i])
	    {
	      newright = i;
	      break;
	    }
	numlines++;

	if (newright < newleft)
	  {
	    fprintf (stderr, "Ack! newleft=%d, newright=%d, left=%d, right=%d\n",
		     newleft, newright, left, right);
	    free (data);
	    return 0;
	  }

	/* if the next line might push us over the buffer size, stop here! */
	/* ignore this test for the 720 right now.  Will add better */
	/* size-guessing for compressed data in the near future! */
	if (numlines % 2 == 1 && prn->version != HP720)
	  {
	    int l = newleft, r = newright, w;
	    
	    l--;
	    r+=2;
	    l*=8;
	    r*=8;
	    w = r-l;
	    w = (w+7)/8;
	    
	    if ((w+2*shift)*numlines > prn->bufsize)
	      {
		numlines--;
		pbm_unreadline (pbm, data+width8*numlines);
		break;
	      }
	    else
	      {
		left = newleft;
		right = newright;
	      }
	  }
	else
	  {
	    left = newleft;
	    right = newright;
	  }
      }
  }

  if(!got_nonblank)
  {
    /* eat all lines that are below the lower margin */
    if(pbm->current_line >= Height - prn->bottom_margin)
    {
      while(pbm->current_line < pbm->height)
	if(!pbm_readline(pbm,data))
	{
	  fprintf(stderr,"cutswath(): could not clear bottom margin\n");
	  free(data);
	  return 0;
	}
      free(data);
      return 1;
    }
    free(data);
    return 0; /* error, since didn't get to lower margin, yet blank */
  }

  /* make sure numlines is even and >= 2 (b/c we have to pass the printer 
     HALF of the number of pins used */
  if (numlines == 1)
    {
      /* there's no way that we only have 1 line and not enough memory, so
	 we're safe to increase numlines here.  Also, the bottom margin should
	 be > 0 so we have some lines to read */
      if(!pbm_readline(pbm,data+width8*numlines))
	{
	  fprintf(stderr,"cutswath(): C-could not read next line\n");
	  free(data);
	  return 0;
	}
      numlines++;
    }
  if (numlines % 2 == 1)
    {
      /* decrease instead of increasing so we don't max out the buffer */
      numlines--;
      pbm_unreadline (pbm, data+width8*numlines);
    }

  /* calculate vertical position */
  sweep_data->vertical_pos = pbm->current_line;

  /* change sweep params */
  left--;
  right+=2;
  left *= 8;
  right *= 8;

  /* construct the sweep data */
  p_width = right - left;
  p_width8 = (p_width + 7) / 8;

  if ((ppa = malloc ((p_width8+2*shift) * numlines)) == NULL)
    {
      fprintf(stderr,"cutswath(): could not malloc ppa storage\n");
      free (data);
      return 0;
    }

  place = ppa;

  /* place 0's in the first 12 columns */
  memset (place, 0, numlines/2 * shift);
  place += numlines/2 * shift;


  if(sweep_data->direction == right_to_left)  /* right-to-left */
  {
    for (i = p_width8+shift-1; i >= 0; i--)
    {
      if (i >= shift)
      {
	for (j = 0; j < numlines/2; j++)
	  *place++ = data[j*2*width8 + i + left/8-shift];
      }
      else
      {
	memset (place, 0, numlines/2);
	place += numlines/2;
      }

      if (i < p_width8)
      {
	for (j = 0; j < numlines/2; j++)
	  *place++ = data[(j*2+1)*width8 + i + left/8];
      }
      else
      {
	memset (place, 0, numlines/2);
	place += numlines/2;
      }
    }
  }
  else /* sweep_data->direction == left_to_right */
  {
    for (i = 0; i < p_width8+shift; i++)
    {
      if (i < p_width8)
      {
	for (j = 0; j < numlines/2; j++)
	  *place++ = data[(j*2+1)*width8 + i + left/8];
      }
      else
      {
	memset (place, 0, numlines/2);
	place += numlines/2;
      }

      if (i >= shift)
      {
	for (j = 0; j < numlines/2; j++)
	  *place++ = data[j*2*width8 + i + left/8 - shift];
      }
      else
      {
	memset (place, 0, numlines/2);
	place += numlines/2;
      }
    }
  }

  /* done with data */
  free(data);

  /* place 0's in the last 12 columns */
  memset (place, 0, numlines/2 * shift);
  place += numlines/2 * shift;
  maxplace = place;

  /* create sweep data */
  sweep_data->image_data = ppa;
  sweep_data->data_size = maxplace-ppa;
  sweep_data->in_color = False;

  /*
  horzpos = left*600/prn->DPI + (sweep_data->direction==left_to_right ? 0*600/prn->DPI : 0);
  */
  horzpos = left * 600/prn->DPI;

  hp2 = horzpos + ( p_width8 + 2*shift )*8 * 600/prn->DPI;
  
 
  sweep_data->left_margin = horzpos;
  sweep_data->right_margin = hp2 + prn->marg_diff;

  
  for (i = 0; i < 2; i++)
  {
    nozzles[i].DPI = prn->DPI;
        
    nozzles[i].pins_used_d2 = numlines/2;
    nozzles[i].unused_pins_p1 = 301-numlines;
    nozzles[i].first_pin = 1;
    if (i == 0)
    {
      nozzles[i].left_margin = horzpos + prn->marg_diff;
      nozzles[i].right_margin = hp2 + prn->marg_diff;
      if(sweep_data->direction == right_to_left)
       /* 0 */
	nozzles[i].nozzle_delay=prn->right_to_left_delay[0];
      else
       /* 6 */
	nozzles[i].nozzle_delay=prn->left_to_right_delay[0];
    }
    else
    {
      nozzles[i].left_margin = horzpos;
      nozzles[i].right_margin = hp2;
      if(sweep_data->direction == right_to_left)
       /* 2 */
	nozzles[i].nozzle_delay=prn->right_to_left_delay[1];
      else
       /* 0 */
	nozzles[i].nozzle_delay=prn->left_to_right_delay[1];

    }
  }

  sweep_data->nozzle_data_size = 2;
  sweep_data->nozzle_data = malloc(sizeof(nozzles));
  if(sweep_data->nozzle_data == NULL)
    return 0;
  memcpy(sweep_data->nozzle_data,nozzles,sizeof(nozzles));

  return 2;
}


