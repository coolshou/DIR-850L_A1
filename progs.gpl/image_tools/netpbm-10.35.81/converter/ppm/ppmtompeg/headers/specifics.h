#include "ansi.h"


/*===========*
 * TYPES     *
 *===========*/

typedef struct bs_def {
  int num;
  boolean relative;
  char qscale;
  BlockMV *mv;  /* defined in mtypes.h */
  struct bs_def *next;
} Block_Specifics;

typedef struct detail_def {
  int num;
  char qscale;
  struct detail_def *next;
}  Slice_Specifics;

typedef struct fsl_def {
  int framenum; 
  int frametype;
  char qscale;
  Slice_Specifics *slc;
  Block_Specifics *bs;
  struct fsl_def *next;
} FrameSpecList;


void	Specifics_Init _ANSI_ARGS_((void));
int     SpecLookup _ANSI_ARGS_((int fn, int typ, int num, 
				BlockMV **info, int start_qs));
int SpecTypeLookup _ANSI_ARGS_((int fn));

