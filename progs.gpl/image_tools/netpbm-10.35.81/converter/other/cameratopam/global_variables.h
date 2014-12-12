/* These are unfortunately global variables used by various modules.

   It would be nice to clean up this code and get rid of all of these.
*/
#include <stdio.h>
#include <time.h>
#include "pm_c_util.h"

extern FILE * ifp;
extern int flip;
extern int data_offset;
extern int raw_width;
extern int raw_height;
extern int height;
extern int width;
extern char * meta_data;
extern int meta_offset;
extern int meta_length;
extern char make[64];
extern char model[70];
extern char model2[64];
extern time_t timestamp;
extern int is_foveon;
extern int is_dng;
extern int is_canon;
extern unsigned short (*image)[4];
extern int maximum;
extern int clip_max;
extern short order;
extern int zero_after_ff;
extern int shrink;
extern int iwidth;
extern unsigned int filters;
extern int black;
extern unsigned short white[8][8];
extern int top_margin;
extern int left_margin;
extern int fuji_width;
extern int tiff_samples;
extern int tiff_data_compression;
extern int kodak_data_compression;
extern int fuji_secondary;
extern int use_coeff;
extern int use_gamma;
extern int xmag;
extern int ymag;
extern float cam_mul[4];
#define camera_red  cam_mul[0]
#define camera_blue cam_mul[2]
extern float pre_mul[4];
extern int colors;
extern unsigned short curve[0x1000];
extern float coeff[3][4];
extern int use_secondary;
extern bool verbose;
