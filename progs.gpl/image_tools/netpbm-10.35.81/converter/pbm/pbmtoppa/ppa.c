/* ppa.c
 * functions to handle PPA communications
 * Copyright (c) 1998 Tim Norman.  See LICENSE for details.
 * 2-25-98
 *
 * Mar 3, 1998  Jim Peterson  <jspeter@birch.ee.vt.edu>
 *
 *     Restructured to accommodate both the HP820/1000, and HP720 series.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pm_config.h"
  /* pm_config.h is necessary for __inline__ for those compilers that don't
     define __inline__ themselves
     */
#include "ppa.h"

/*
  VLink packet structure:

    Bytes  Description
    --------------------------------------------
      1    Packet-start marker (always '$')
      1    Channel (0: image data, 1: commands/responses(*), 2: autostatus(*), 128: pacing(*))
      2    Data length (N)
      N    [remaining data (e.g., SCP or SCP2 packet when Channel=1)]

  (*): responses, autostatus, and pacing are communicated from the printer to
       the computer, and may be safely ignored.
*/
static void 
vlink_put(FILE *fptr, int channel, int length, void *data)
{
  fputc ('$', fptr);
  fputc (channel, fptr);
  fputc (length>>8, fptr);     fputc (length&0xFF, fptr);
  fwrite (data, length, 1, fptr);
}

/*
  SCP packet structure:

    Bytes  Description
    --------------------------------------------
      2    Command specifier
      2    Command reference number
      1    Priority
      1    padding? (always zero)
      2    Data length (N)
      N    [remaining data]

    ComSpec  ComRef     Command
    -------------------------------
       35       1     Initialize1
      101       2     Initialize2
       21       1     Initialize3
       19       1     Handle Media
       18       1     Print Sweep
*/
static void 
scp_put(FILE *fptr, int comspec, int comref, int priority,
	     int length, void *data)
{
  /* encapsulate the vlink_put call in here, to avoid a memcpy */
  fputc ('$', fptr);
  fputc (1, fptr);
  fputc ((length+8)>>8, fptr);     fputc ((length+8)&0xFF, fptr);

  fputc (comspec>>8, fptr);  fputc (comspec&0xFF, fptr);
  fputc (comref>>8, fptr);   fputc (comref&0xFF, fptr);
  fputc (priority, fptr);
  fputc (0, fptr);
  fputc (length>>8, fptr);   fputc (length&0xFF, fptr);

  fwrite (data, length, 1, fptr);
}


/*
  SCP2 packet structure:

    Bytes  Description
    --------------------------------------------
      2    Command specifier
      2    Packet length (N)
      1    Priority
      1    padding? (always zero)
      2    Command reference number
      4    Channel 0 buffer increment
      4    version number? (always 00 02 00 00)
     N-16  [remaining data]

    ComSpec  ComRef     Command
    -------------------------------
     0x186      1     Initialize1
     0x18f      2     Initialize2
     0x183      1     Initialize3
     0x181      1     Handle Media
     0x180      1     Print Sweep
*/
static void 
scp2_put(FILE *fptr,unsigned short comspec,unsigned short pkt_len_s16,
	      unsigned char priority,unsigned short comref,unsigned data_len,
	      void *data)
{
  /* encapsulate the vlink_put call in here, to avoid a memcpy */
  fputc ('$', fptr);
  fputc (1, fptr);
  fputc ((pkt_len_s16+16)>>8, fptr);  fputc ((pkt_len_s16+16), fptr);

  fputc (comspec>>8, fptr);  fputc (comspec, fptr);
  fputc ((pkt_len_s16+16)>>8, fptr);  fputc ((pkt_len_s16+16), fptr);
  fputc (priority, fptr);
  fputc (0, fptr);
  fputc (comref>>8, fptr);
  fputc (comref, fptr);
  fputc (data_len>>24, fptr);   fputc (data_len>>16, fptr);
  fputc (data_len>>8, fptr);    fputc (data_len, fptr);
  fputc (0, fptr);
  fputc (2, fptr);
  fputc (0, fptr);
  fputc (0, fptr);

  fwrite (data, pkt_len_s16, 1, fptr);
}


/*
  SCP3 packet structure:

    Bytes  Description
    --------------------------------------------
      2    Command specifier
      2    Packet length (N)
      1    Priority
      1    padding? (always zero)
      2    Command reference number
      4    Channel 0 buffer increment
      4    version number? (always 01 04 00 00)
     N-16  [remaining data]

    ComSpec  ComRef     Command
    -------------------------------
     0x186      1     Initialize1
     0x18C     16     Initialize Printer name
     0x1A1      1     Initialize4?
     0x18f      2     Initialize2
     0x183      1     Initialize3
     0x181      1     Handle Media
     0x180      1     Print Sweep
*/
static void 
scp3_put(FILE *fptr,unsigned short comspec,unsigned short pkt_len_s16,
	      unsigned char priority,unsigned short comref,unsigned data_len,
	      void *data)
{
  /* encapsulate the vlink_put call in here, to avoid a memcpy */
  fputc ('$', fptr);
  fputc (1, fptr);
  fputc ((pkt_len_s16+16)>>8, fptr);  fputc ((pkt_len_s16+16), fptr);

  fputc (comspec>>8, fptr);  fputc (comspec, fptr);
  fputc ((pkt_len_s16+16)>>8, fptr);  fputc ((pkt_len_s16+16), fptr);
  fputc (priority, fptr);
  fputc (0, fptr);
  fputc (comref>>8, fptr);
  fputc (comref, fptr);
  fputc (data_len>>24, fptr);   fputc (data_len>>16, fptr);
  fputc (data_len>>8, fptr);    fputc (data_len, fptr);
  fputc (1, fptr);
  fputc (4, fptr);
  fputc (0, fptr);
  fputc (0, fptr);

  fwrite (data, pkt_len_s16, 1, fptr);
}


void ppa_init_job(ppa_stat* prn)
{
  unsigned char init1[8] = { 0x00, 0x00, 0x01, 0xf4, 0x01, 0x00, 0x00, 0x00 };
  unsigned char init2[4] = { 0xde, 0xad, 0xbe, 0xef };
  unsigned char init3[8] = { 0xde, 0xad, 0xbe, 0xef, 0x02, 0x00, 0x00, 0x00 };
  unsigned char init4[60] = "!!TAZ            \x81*HP DeskJet 1000C Prin (Copy 2)*FILE!!\x00\x00\x00"; /* plus 0 terminator */
  unsigned char init5[4] = { 0x01, 0x01, 0x00, 0x00 };

  switch(prn->version)
  {
  case HP820:
    scp_put (prn->fptr, 35, 1, 7, sizeof(init1), init1);
    vlink_put (prn->fptr, 0, sizeof(init2), init2);
    scp_put (prn->fptr, 101, 2, 7, sizeof(init3), init3);
    break;
  case HP720:
    scp2_put (prn->fptr, 0x0186, sizeof(init1), 7, 1, 0, init1);
    vlink_put(prn->fptr, 0, sizeof(init2), init2);
    scp2_put (prn->fptr, 0x018f, sizeof(init3), 7, 2, 4, init3);
    break;
  case HP1000:
    scp3_put (prn->fptr, 0x0186, sizeof(init1), 7, 16, 0, init1);
    scp3_put (prn->fptr, 0x018C, sizeof(init4), 7, 1, 0, init4);
    scp3_put (prn->fptr, 0x01A1, sizeof(init5), 7, 1, 0, init5);
    vlink_put (prn->fptr, 0, sizeof(init2), init2);
    scp3_put (prn->fptr, 0x018f, sizeof(init3), 7, 2, 4, init3);
    break;
  default:
    fprintf(stderr,"ppa_init_job(): unknown printer verson\n");
  }
}

void ppa_end_print(ppa_stat* prn)
{
  unsigned char pageA[4] = { 0x05, 0x01, 0x03, 0x84 };

  if (prn->version == HP1000)
    scp3_put (prn->fptr, 0x0181, sizeof(pageA), 7, 2, 0, pageA);
}

void ppa_init_page(ppa_stat* prn)
{
  unsigned char pageA[16] = {0x28, 0x2d, 0x00, 0x41, 0x29, 0x2e, 0x00, 0x42,
			     0x29, 0x2e, 0x00, 0x42, 0x29, 0x2e, 0x00, 0x42 };
  unsigned char pageB[16] = {0x28, 0x2d, 0x00, 0x41, 0x2d, 0x32, 0x00, 0x46,
			     0x2d, 0x32, 0x00, 0x46, 0x2d, 0x32, 0x00, 0x46 };

  switch(prn->version)
  {
  case HP820:
    scp_put (prn->fptr, 21, 1, 5, sizeof(pageA), pageA);
    break;
  case HP720:
    scp2_put (prn->fptr, 0x0183, sizeof(pageB), 5, 1, 0, pageB);
    break;
  case HP1000:
    scp3_put (prn->fptr, 0x0183, sizeof(pageA), 5, 1, 0, pageA);
    break;
  default:
    fprintf(stderr,"ppa_init_page(): unknown printer verson\n");
  }
}

void ppa_load_page(ppa_stat* prn)
{
  unsigned char loadA[4] = {0x01, 0x01, 0x09, 0x60 };
  unsigned char loadB[4] = {0x01, 0x01, 0x12, 0xc0 };
  unsigned char loadC[4] = {0x01, 0x01, 0x07, 0x08 };

  switch(prn->version)
  {
  case HP820:
    scp_put (prn->fptr, 19, 1, 7, sizeof(loadA), loadA);
    break;
  case HP720:
    scp2_put (prn->fptr, 0x0181, sizeof(loadB), 7, 1, 0, loadB);
    break;
  case HP1000:
    scp3_put (prn->fptr, 0x0181, sizeof(loadC), 7, 1, 0, loadC);
    break;
  default:
    fprintf(stderr,"ppa_load_page(): unknown printer verson\n");
  }
}

void ppa_eject_page(ppa_stat* prn)
{
  unsigned char loadA[4] = {0x02, 0x01, 0x09, 0x60 };
  unsigned char loadB[4] = {0x02, 0x01, 0x12, 0xc0 };
  unsigned char loadC[4] = {0x02, 0x01, 0x07, 0x08 };

  switch(prn->version)
  {
  case HP820:
    scp_put (prn->fptr, 19, 1, 7, sizeof(loadA), loadA);
    break;
  case HP720:
    scp2_put (prn->fptr, 0x0181, sizeof(loadB), 7, 1, 0, loadB);
    break;
  case HP1000:
    scp3_put (prn->fptr, 0x0181, sizeof(loadC), 7, 1, 0, loadC);
    break;
  default:
    fprintf(stderr,"ppa_eject_page(): unknown printer verson\n");
  }
}



static int 
compress(unsigned char *in, int num_lines_d2, int final_len, 
             unsigned char *iout)
{
  unsigned char* out = iout;
  int I,len=num_lines_d2;
  for(I=0; I<final_len; I+=num_lines_d2, in+=num_lines_d2)
  {
    int i = 0;
    while (i < len) {
      /* Find the size of duplicate values */
      int dup_len = 0;
      while ((i + dup_len < len)
	     && (in[i + dup_len] == in[i])) {
	dup_len++;
      }
      /* See if we have enough zeros to be worth compressing. */
      /* I figure one is enough. */
      if ((dup_len >= 1) && (in[i] == 0)) {
	/* Output run of zeros. */
	while (dup_len >= 128) {
	  /* Max is 128 */
	  *out++ = 0x00;
	  i += 128;
	  dup_len -= 128;
	}
	if (dup_len >= 1)
	{
	  *out++ = dup_len;
	  i += dup_len;
	}
	/* See if we have enough non-zeros to be worth compressing. */
	/* Here two should be enough. */
      }
      else if (dup_len >= 2)
      {
	/* Output run of duplicates. */
	while (dup_len >= 64) {
	  /* Max is 64 */
	  *out++ = 0x80;
	  *out++ = in[i];
	  i += 64;
	  dup_len -= 64;
	}
	if (dup_len >= 2)
	{
	  *out++ = dup_len + 0x80;
	  *out++ = in[i];
	  i += dup_len;
	}
      }
      else
      {
	/* Look for two zeros, or three duplicates to end literal run. */
	/* Note this is one more than the number to start a run. */
	int lit_len = -1;
	int add_more = 1;
	while (add_more) {
	  lit_len++;
	  if (i + lit_len == len) add_more = 0;
	  /* Always add more if we are near the very end. */
	  if (i + lit_len < len - 3) {
	    char a = in[i + lit_len + 0];
	    char b = in[i + lit_len + 1];
	    char c = in[i + lit_len + 2];
	    /* See if there are enough zeros */
	    if ((a == b) && (b == 0)) add_more = 0;
	    /* See if there are enough duplicates */
	    if ((a == b) && (b == c)) add_more = 0;
	  }
	}
	/* Output run of literals. */
	while (lit_len >= 64) {
	  /* Max is 64 */
	  int j;
	  *out++ = 0xc0;
	  for (j = i; j < i + 64; j++) {
	    *out++ = in[j];
	  }
	  i += 64;
	  lit_len -= 64;
	} 
	if (lit_len) {
	  int j;
	  *out++ = lit_len + 0xc0;
	  for (j = i; j < i + lit_len; j++) {
	    *out++ = in[j];
	  }
	  i += lit_len;
	}
      }
    }
  }
  return out-iout;
}

static void __inline__ place_2bytes(int x,unsigned char* y)
{ y[0]=x>>8; y[1]=x; }
static void __inline__ place_4bytes(int x,unsigned char* y)
{ place_2bytes(x>>16,y); place_2bytes(x,y+2); }

#define do_compress_data (1)
void ppa_print_sweep(ppa_stat* prn,ppa_sweep_data* data)
{
  unsigned char* pc, *tpc;
  unsigned i,datasize=data->data_size;
  unsigned char sweep_packet[144];
  int sweep_packet_size;
  unsigned short HP720constants[] = { 0x8ca0, 0x4650, 0x12c0 };
  unsigned short HP820constants[] = { 0x4650, 0x1c20, 0x0960 };
  unsigned short HP1000constants[] = { 0x4650, 0x2328, 0x0708 };
  unsigned short* constants;
  int nozzle_data_size;
  int MF; /* Multiplicative Factor -- quick hack */

  pc=data->image_data;

  if(do_compress_data)
  {
    if(!(pc=malloc((datasize/64+1)*65)))
    {
      fprintf(stderr,"ppa_print_sweep(): could not malloc storage for compressed data\n");
      exit(-1);
    }
    datasize=compress(data->image_data,data->nozzle_data->pins_used_d2,datasize,pc);
  }

  /* send image data 16k at a time */
  for(i=0, tpc=pc; i<datasize; tpc+=16384, i+=16384)
    vlink_put(prn->fptr, 0, datasize-i > 16384 ? 16384 : datasize-i, tpc);

  /* memory leak fix courtesy of John McKown */
  if (do_compress_data)
    free (pc);

  /* construct sweep packet */
  switch(prn->version)
  {
  case HP820:
    constants=HP820constants;
    MF=1;
    break;
  case HP720:
    constants=HP720constants;
    MF=2;
    break;
  case HP1000:
    constants=HP1000constants;
    MF=1;
    break;
  default:
    fprintf(stderr,"ppa_print_sweep(): unknown printer verson\n");
    return;
  }

  sweep_packet[0]=0;
  sweep_packet[1]= do_compress_data;
  sweep_packet[2]= data->direction==right_to_left ? 1 : 2;
  sweep_packet[3]= data->in_color ? 14 : 1;

  place_4bytes(datasize,sweep_packet+4);

  memset(sweep_packet+8,0,8);

  place_4bytes(MF*(data->vertical_pos+prn->y_offset),sweep_packet+16);
  place_2bytes(constants[0],sweep_packet+20);
  place_2bytes(MF*(data->left_margin+prn->x_offset),sweep_packet+22);
  place_2bytes(MF*(data->right_margin+prn->x_offset),sweep_packet+24);
  place_2bytes(constants[1],sweep_packet+26);
  place_2bytes(constants[2],sweep_packet+28);
  place_2bytes(0x100,sweep_packet+30);

  if(data->next)
  {
    sweep_packet[32]= data->next->direction==right_to_left ? 1 : 2;
    sweep_packet[33]= data->next->in_color ? 14 : 1;
    place_4bytes(MF*(data->next->vertical_pos+prn->y_offset),sweep_packet+34);
    place_2bytes(MF*(data->next->left_margin+prn->x_offset),sweep_packet+38);
    place_2bytes(MF*(data->next->right_margin+prn->x_offset),sweep_packet+40);
    place_2bytes(constants[1],sweep_packet+42);
    place_2bytes(constants[2],sweep_packet+44);
  }
  else
    memset(sweep_packet+32,0,14);
  sweep_packet[46]=8;

  nozzle_data_size=data->nozzle_data_size;
  if(nozzle_data_size>6)
  {
    fprintf(stderr,"ppa_print_sweep(): truncating nozzle data from %d rows to 6 rows\n",nozzle_data_size);
    nozzle_data_size=6;
  }
  sweep_packet[47]=nozzle_data_size;

  for(i=0; i<nozzle_data_size; ++i)
  {
    unsigned char * const pc = sweep_packet + 48 + i*16;

    place_2bytes(data->nozzle_data[i].DPI,pc);
    place_2bytes(data->nozzle_data[i].pins_used_d2,pc+2);
    place_2bytes(data->nozzle_data[i].unused_pins_p1,pc+4);
    place_2bytes(data->nozzle_data[i].first_pin,pc+6);
    place_2bytes(data->nozzle_data[i].pins_used_d2,pc+8);
    place_2bytes(MF*(data->nozzle_data[i].left_margin+prn->x_offset),pc+10);
    place_2bytes(MF*(data->nozzle_data[i].right_margin+prn->x_offset),pc+12);
    pc[14]=data->nozzle_data[i].nozzle_delay;
    pc[15]=0;
  }

  sweep_packet_size = data->in_color ? 144 : 80;

  /* send sweep packet */
  switch(prn->version)
  {
  case HP820:
    scp_put (prn->fptr, 18, 1, 7, sweep_packet_size, sweep_packet);
    break;
  case HP720:
    scp2_put (prn->fptr, 0x0180, sweep_packet_size, 7, 1, datasize, sweep_packet);
    break;
  case HP1000:
    scp3_put (prn->fptr, 0x0180, sweep_packet_size, 7, 1, datasize, sweep_packet);
    break;
  default:
    fprintf(stderr,"ppa_print_sweep(): unknown printer version\n");
    return;
  }
}


void ppa_print_sweeps(ppa_stat* prn,ppa_sweep_data* data)
{
  ppa_sweep_data* current_sweep;
  for(current_sweep=data; current_sweep; current_sweep=current_sweep->next)
    ppa_print_sweep(prn,current_sweep);
}
