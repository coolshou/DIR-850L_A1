/* ppa.h
 * Copyright (c) 1998 Tim Norman.  See LICENSE for details.
 * 2-25-98
 *
 * Mar 3, 1998  Jim Peterson  <jspeter@birch.ee.vt.edu>
 *
 *     Restructured to accommodate both the HP820/1000, and HP720 series.
 */
#ifndef _PPA_H
#define _PPA_H
#include <stdio.h>

typedef struct
{
  FILE* fptr;
  enum { HP820, HP1000, HP720, HP710 } version;
  /* 300 , 600 dpi */
  int DPI;
  /* nozzle delay */
  int right_to_left_delay[2];
  int left_to_right_delay[2];
  /* direction of printing */
  enum { RIGHT_ONLY, LEFT_ONLY, BOTH_DIR } direction;
  int x_offset;
  int y_offset;
  int marg_diff;
  int top_margin;
  int left_margin;
  int right_margin;
  int bottom_margin;
  int bufsize;
} ppa_stat;

typedef struct
{ 
  int DPI;
  int right;
  int left;
} printer_delay;

typedef struct
{
  unsigned short DPI;
  unsigned short pins_used_d2;
  unsigned short unused_pins_p1;
  unsigned short first_pin;
  unsigned short left_margin;
  unsigned short right_margin;
  unsigned char nozzle_delay;
} ppa_nozzle_data;

typedef struct ppa_sweep_data_s
{
  unsigned char* image_data;
  unsigned data_size;
  enum { False=0, True=1 } in_color;
  enum { right_to_left, left_to_right } direction;
  int vertical_pos;
  unsigned short left_margin;
  unsigned short right_margin;
  unsigned char nozzle_data_size;
  ppa_nozzle_data* nozzle_data;
  struct ppa_sweep_data_s* next; /* NULL indicates last print sweep */
} ppa_sweep_data;

void ppa_init_job(ppa_stat*);
void ppa_init_page(ppa_stat*);
void ppa_load_page(ppa_stat*);
void ppa_eject_page(ppa_stat*);
void ppa_end_print(ppa_stat*);
void ppa_print_sweep(ppa_stat*,ppa_sweep_data*);  /* prints one sweep */
void ppa_print_sweeps(ppa_stat*,ppa_sweep_data*); /* prints a linked-list of sweeps */

#endif
