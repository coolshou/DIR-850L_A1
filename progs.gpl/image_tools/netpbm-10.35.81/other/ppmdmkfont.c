#include <stdio.h>
#include <assert.h>

#include "ppm.h"
#include "mallocvar.h"
#include "nstring.h"
#include "ppmdfont.h"


/*        Stroke character definitions

   The  following  character  definitions are derived from the (public
   domain) Hershey plotter  font  database,  using  the  single-stroke
   Roman font.

   Each  character  definition  begins  with 3 bytes which specify the
   number of X, Y plot pairs which follow, the negative  of  the  skip
   before  starting  to  draw  the  characters, and the skip after the
   character.  The first plot pair moves the pen to that location  and
   subsequent  pairs  draw  to  the  location given.  A pair of 192, 0
   raises the pen, moves to the location given by the following  pair,
   and resumes drawing with the pair after that.

   The  values  in  the  definition  tables are 8-bit two's complement
   signed numbers.  We  declare  the  table  as  "unsigned  char"  and
   manually  sign-extend  the  values because C compilers differ as to
   whether the type "char" is signed or unsigned, and  some  compilers
   don't  accept the qualifier "signed" which we would like to use for
   these items.  We specify negative numbers as their  unsigned  two's
   complements  to  avoid  complaints  from compilers which don't like
   initialising unsigned data with signed values.  Ahhh,  portability.
*/

static unsigned char char32[] =
{ 0, 0, 21 };

static unsigned char char33[] =
{ 8, 251, 5,
  0, 244, 0, 2, 192, 0, 0, 7, 255, 8, 0, 9, 1, 8, 0, 7 };

static unsigned char char34[] =
{ 17, 253, 15,
  2, 244, 1, 245, 0, 244, 1, 243, 2, 244, 2, 246, 1,
  248, 0, 249, 192, 0, 10, 244, 9, 245, 8, 244, 9, 243, 10, 244,
  10, 246, 9, 248, 8, 249, };

static unsigned char char35[] =
{ 11, 246, 11,
  1, 240, 250, 16, 192, 0, 7, 240, 0, 16, 192, 0, 250,
  253, 8, 253, 192, 0, 249, 3, 7, 3 };

static unsigned char char36[] =
{ 26, 246, 10,
  254, 240, 254, 13, 192, 0, 2, 240, 2, 13, 192, 0, 7,
  247, 5, 245, 2, 244, 254, 244, 251, 245, 249, 247, 249, 249, 250,
  251, 251, 252, 253, 253, 3, 255, 5, 0, 6, 1, 7, 3, 7, 6, 5, 8, 2,
  9, 254, 9, 251, 8, 249, 6 };

static unsigned char char37[] =
{ 31, 244, 12,
  9, 244, 247, 9, 192, 0, 252, 244, 254, 246, 254,
  248, 253, 250, 251, 251, 249, 251, 247, 249, 247, 247, 248, 245,
  250, 244, 252, 244, 254, 245, 1, 246, 4, 246, 7, 245, 9, 244,
  192, 0, 5, 2, 3, 3, 2, 5, 2, 7, 4, 9, 6, 9, 8, 8, 9, 6, 9, 4, 7,
  2, 5, 2 };
  
static unsigned char char38[] =
{ 34, 243, 13,
  10, 253, 10, 252, 9, 251, 8, 251, 7, 252, 6, 254, 4,
  3, 2, 6, 0, 8, 254, 9, 250, 9, 248, 8, 247, 7, 246, 5, 246, 3,
  247, 1, 248, 0, 255, 252, 0, 251, 1, 249, 1, 247, 0, 245, 254,
  244, 252, 245, 251, 247, 251, 249, 252, 252, 254, 255, 3, 6, 5,
  8, 7, 9, 9, 9, 10, 8, 10, 7 };

static unsigned char char39[] =
{ 7, 251, 5,
  0, 246, 255, 245, 0, 244, 1, 245, 1, 247, 0, 249, 255,
  250 };

static unsigned char char40[] =
{ 10, 249, 7,
  4, 240, 2, 242, 0, 245, 254, 249, 253, 254, 253, 2,
  254, 7, 0, 11, 2, 14, 4, 16 };

static unsigned char char41[] =
{ 10, 249, 7,
  252, 240, 254, 242, 0, 245, 2, 249, 3, 254, 3, 2, 2,
  7, 0, 11, 254, 14, 252, 16 };

static unsigned char char42[] =
{ 8, 248, 8,
  0, 250, 0, 6, 192, 0, 251, 253, 5, 3, 192, 0, 5, 253,
  251, 3 };

static unsigned char char43[] =
{ 5, 243, 13,
  0, 247, 0, 9, 192, 0, 247, 0, 9, 0 };

static unsigned char char44[] =
{ 8, 251, 5,
  1, 8, 0, 9, 255, 8, 0, 7, 1, 8, 1, 10, 0, 12, 255, 13
};

static unsigned char char45[] =
{ 2, 243, 13,
  247, 0, 9, 0 };

static unsigned char char46[] =
{ 5, 251, 5,
  0, 7, 255, 8, 0, 9, 1, 8, 0, 7 };

static unsigned char char47[] =
{ 2, 245, 11,
  9, 240, 247, 16 };

static unsigned char char48[] =
{ 17, 246, 10,
  255, 244, 252, 245, 250, 248, 249, 253, 249, 0, 250,
  5, 252, 8, 255, 9, 1, 9, 4, 8, 6, 5, 7, 0, 7, 253, 6, 248, 4,
  245, 1, 244, 255, 244 };

static unsigned char char49[] =
{ 4, 246, 10,
  252, 248, 254, 247, 1, 244, 1, 9 };

static unsigned char char50[] =
{ 14, 246, 10,
  250, 249, 250, 248, 251, 246, 252, 245, 254, 244, 2,
  244, 4, 245, 5, 246, 6, 248, 6, 250, 5, 252, 3, 255, 249, 9, 7, 9
};

static unsigned char char51[] =
{ 15, 246, 10,
  251, 244, 6, 244, 0, 252, 3, 252, 5, 253, 6, 254, 7,
  1, 7, 3, 6, 6, 4, 8, 1, 9, 254, 9, 251, 8, 250, 7, 249, 5 };


static unsigned char char52[] =
{ 6, 246, 10,
  3, 244, 249, 2, 8, 2, 192, 0, 3, 244, 3, 9 };

static unsigned char char53[] =
{ 17, 246, 10,
  5, 244, 251, 244, 250, 253, 251, 252, 254, 251, 1,
  251, 4, 252, 6, 254, 7, 1, 7, 3, 6, 6, 4, 8, 1, 9, 254, 9, 251,
  8, 250, 7, 249, 5 };

static unsigned char char54[] =
{ 23, 246, 10,
  6, 247, 5, 245, 2, 244, 0, 244, 253, 245, 251, 248,
  250, 253, 250, 2, 251, 6, 253, 8, 0, 9, 1, 9, 4, 8, 6, 6, 7, 3,
  7, 2, 6, 255, 4, 253, 1, 252, 0, 252, 253, 253, 251, 255, 250, 2
};

static unsigned char char55[] =
{ 5, 246, 10,
  7, 244, 253, 9, 192, 0, 249, 244, 7, 244 };
    
static unsigned char char56[] =
{ 29, 246, 10,
  254, 244, 251, 245, 250, 247, 250, 249, 251, 251,
  253, 252, 1, 253, 4, 254, 6, 0, 7, 2, 7, 5, 6, 7, 5, 8, 2, 9,
  254, 9, 251, 8, 250, 7, 249, 5, 249, 2, 250, 0, 252, 254, 255,
  253, 3, 252, 5, 251, 6, 249, 6, 247, 5, 245, 2, 244, 254, 244 };

static unsigned char char57[] =
{ 23, 246, 10,
  6, 251, 5, 254, 3, 0, 0, 1, 255, 1, 252, 0, 250,
  254, 249, 251, 249, 250, 250, 247, 252, 245, 255, 244, 0, 244, 3,
  245, 5, 247, 6, 251, 6, 0, 5, 5, 3, 8, 0, 9, 254, 9, 251, 8, 250,
  6 };
      
static unsigned char char58[] =
{ 11, 251, 5,
  0, 251, 255, 252, 0, 253, 1, 252, 0, 251, 192, 0, 0,
  7, 255, 8, 0, 9, 1, 8, 0, 7 };
                                
static unsigned char char59[] =
{ 14, 251, 5,
  0, 251, 255, 252, 0, 253, 1, 252, 0, 251, 192, 0, 1,
  8, 0, 9, 255, 8, 0, 7, 1, 8, 1, 10, 0, 12, 255, 13 };

static unsigned char char60[] =
{ 3, 244, 12,
  8, 247, 248, 0, 8, 9 };

static unsigned char char61[] =
{ 5, 243, 13,
  247, 253, 9, 253, 192, 0, 247, 3, 9, 3 };

static unsigned char char62[] =
{ 3, 244, 12,
  248, 247, 8, 0, 248, 9 };

static unsigned char char63[] =
{ 20, 247, 9,
  250, 249, 250, 248, 251, 246, 252, 245, 254, 244, 2,
  244, 4, 245, 5, 246, 6, 248, 6, 250, 5, 252, 4, 253, 0, 255, 0,
  2, 192, 0, 0, 7, 255, 8, 0, 9, 1, 8, 0, 7 };

static unsigned char char64[] =
{ 55, 243, 14,
  5, 252, 4, 250, 2, 249, 255, 249, 253, 250, 252,
  251, 251, 254, 251, 1, 252, 3, 254, 4, 1, 4, 3, 3, 4, 1, 192, 0,
  255, 249, 253, 251, 252, 254, 252, 1, 253, 3, 254, 4, 192, 0, 5,
  249, 4, 1, 4, 3, 6, 4, 8, 4, 10, 2, 11, 255, 11, 253, 10, 250, 9,
  248, 7, 246, 5, 245, 2, 244, 255, 244, 252, 245, 250, 246, 248,
  248, 247, 250, 246, 253, 246, 0, 247, 3, 248, 5, 250, 7, 252, 8,
  255, 9, 2, 9, 5, 8, 7, 7, 8, 6, 192, 0, 6, 249, 5, 1, 5, 3, 6, 4
};
  
static unsigned char char65[] =
{ 8, 247, 9,
  0, 244, 248, 9, 192, 0, 0, 244, 8, 9, 192, 0, 251, 2,
  5, 2 };

static unsigned char char66[] =
{ 23, 245, 10,
  249, 244, 249, 9, 192, 0, 249, 244, 2, 244, 5, 245,
  6, 246, 7, 248, 7, 250, 6, 252, 5, 253, 2, 254, 192, 0, 249, 254,
  2, 254, 5, 255, 6, 0, 7, 2, 7, 5, 6, 7, 5, 8, 2, 9, 249, 9 };

static unsigned char char67[] =
{ 18, 246, 11,
  8, 249, 7, 247, 5, 245, 3, 244, 255, 244, 253, 245,
  251, 247, 250, 249, 249, 252, 249, 1, 250, 4, 251, 6, 253, 8,
  255, 9, 3, 9, 5, 8, 7, 6, 8, 4 };

static unsigned char char68[] =
{ 15, 245, 10,
  249, 244, 249, 9, 192, 0, 249, 244, 0, 244, 3, 245,
  5, 247, 6, 249, 7, 252, 7, 1, 6, 4, 5, 6, 3, 8, 0, 9, 249, 9 };

static unsigned char char69[] =
{ 11, 246, 9,
  250, 244, 250, 9, 192, 0, 250, 244, 7, 244, 192, 0,
  250, 254, 2, 254, 192, 0, 250, 9, 7, 9 };

static unsigned char char70[] =
{ 8, 246, 8,
  250, 244, 250, 9, 192, 0, 250, 244, 7, 244, 192, 0,
  250, 254, 2, 254 };

static unsigned char char71[] =
{ 22, 246, 11,
  8, 249, 7, 247, 5, 245, 3, 244, 255, 244, 253, 245,
  251, 247, 250, 249, 249, 252, 249, 1, 250, 4, 251, 6, 253, 8,
  255, 9, 3, 9, 5, 8, 7, 6, 8, 4, 8, 1, 192, 0, 3, 1, 8, 1 };

static unsigned char char72[] =
{ 8, 245, 11,
  249, 244, 249, 9, 192, 0, 7, 244, 7, 9, 192, 0, 249,
  254, 7, 254 };

static unsigned char char73[] =
{ 2, 252, 4,
  0, 244, 0, 9 };

static unsigned char char74[] =
{ 10, 248, 8,
  4, 244, 4, 4, 3, 7, 2, 8, 0, 9, 254, 9, 252, 8, 251,
  7, 250, 4, 250, 2 };

static unsigned char char75[] =
{ 8, 245, 10,
  249, 244, 249, 9, 192, 0, 7, 244, 249, 2, 192, 0,
  254, 253, 7, 9 };

static unsigned char char76[] = 
{ 3, 246, 7,
  250, 244, 250, 9, 6, 9 };

static unsigned char char77[] =
{ 11, 244, 12,
  248, 244, 248, 9, 192, 0, 248, 244, 0, 9, 192, 0, 8,
  244, 0, 9, 192, 0, 8, 244, 8, 9 };
                                               
static unsigned char char78[] =
{ 8, 245, 11,
  249, 244, 249, 9, 192, 0, 249, 244, 7, 9, 192, 0, 7,
  244, 7, 9 };
                         
static unsigned char char79[] =
{ 21, 245, 11,
  254, 244, 252, 245, 250, 247, 249, 249, 248, 252,
  248, 1, 249, 4, 250, 6, 252, 8, 254, 9, 2, 9, 4, 8, 6, 6, 7, 4,
  8, 1, 8, 252, 7, 249, 6, 247, 4, 245, 2, 244, 254, 244 };

static unsigned char char80[] =
{ 13, 245, 10,
  249, 244, 249, 9, 192, 0, 249, 244, 2, 244, 5, 245,
  6, 246, 7, 248, 7, 251, 6, 253, 5, 254, 2, 255, 249, 255 };

static unsigned char char81[] =
{ 24, 245, 11,
  254, 244, 252, 245, 250, 247, 249, 249, 248, 252,
  248, 1, 249, 4, 250, 6, 252, 8, 254, 9, 2, 9, 4, 8, 6, 6, 7, 4,
  8, 1, 8, 252, 7, 249, 6, 247, 4, 245, 2, 244, 254, 244, 192, 0,
  1, 5, 7, 11 };
                           
static unsigned char char82[] =
{ 16, 245, 10,
  249, 244, 249, 9, 192, 0, 249, 244, 2, 244, 5, 245,
  6, 246, 7, 248, 7, 250, 6, 252, 5, 253, 2, 254, 249, 254, 192, 0,
  0, 254, 7, 9 };

static unsigned char char83[] =
{ 20, 246, 10,
  7, 247, 5, 245, 2, 244, 254, 244, 251, 245, 249,
  247, 249, 249, 250, 251, 251, 252, 253, 253, 3, 255, 5, 0, 6, 1,
  7, 3, 7, 6, 5, 8, 2, 9, 254, 9, 251, 8, 249, 6 };

static unsigned char char84[] =
{ 5, 248, 8,
  0, 244, 0, 9, 192, 0, 249, 244, 7, 244 };

static unsigned char char85[] =
{ 10, 245, 11,
  249, 244, 249, 3, 250, 6, 252, 8, 255, 9, 1, 9, 4,
  8, 6, 6, 7, 3, 7, 244 };

static unsigned char char86[] =
{ 5, 247, 9,
  248, 244, 0, 9, 192, 0, 8, 244, 0, 9 };

static unsigned char char87[] =
{ 11, 244, 12,
  246, 244, 251, 9, 192, 0, 0, 244, 251, 9, 192, 0, 0,
  244, 5, 9, 192, 0, 10, 244, 5, 9 };

static unsigned char char88[] =
{ 5, 246, 10,
  249, 244, 7, 9, 192, 0, 7, 244, 249, 9 };

static unsigned char char89[] =
{ 6, 247, 9,
  248, 244, 0, 254, 0, 9, 192, 0, 8, 244, 0, 254 };

static unsigned char char90[] =
{ 8, 246, 10,
  7, 244, 249, 9, 192, 0, 249, 244, 7, 244, 192, 0,
  249, 9, 7, 9 };

static unsigned char char91[] =
{ 11, 249, 7,
  253, 240, 253, 16, 192, 0, 254, 240, 254, 16, 192, 0,
  253, 240, 4, 240, 192, 0, 253, 16, 4, 16 };

static unsigned char char92[] =
{ 2, 245, 11,
  9, 16, 247, 240 };

static unsigned char char93[] =
{ 11, 249, 7,
  2, 240, 2, 16, 192, 0, 3, 240, 3, 16, 192, 0, 252,
  240, 3, 240, 192, 0, 252, 16, 3, 16 };

static unsigned char char94[] =
{ 7, 245, 11,
  248, 2, 0, 253, 8, 2, 192, 0, 248, 2, 0, 254, 8, 2 };

static unsigned char char95[] =
{ 2, 253, 22,
  0, 9, 20, 9 };

static unsigned char char96[] =
{ 7, 251, 5,
  1, 244, 0, 245, 255, 247, 255, 249, 0, 250, 1, 249, 0, 248 };

static unsigned char char97[] =
{ 17, 247, 10,
  6, 251, 6, 9, 192, 0, 6, 254, 4, 252, 2, 251, 255,
  251, 253, 252, 251, 254, 250, 1, 250, 3, 251, 6, 253, 8, 255, 9,
  2, 9, 4, 8, 6, 6 };
                                
static unsigned char char98[] =
{ 17, 246, 9,
  250, 244, 250, 9, 192, 0, 250, 254, 252, 252, 254,
  251, 1, 251, 3, 252, 5, 254, 6, 1, 6, 3, 5, 6, 3, 8, 1, 9, 254,
  9, 252, 8, 250, 6 };
                                 
static unsigned char char99[] =
{ 14, 247, 9,
  6, 254, 4, 252, 2, 251, 255, 251, 253, 252, 251, 254,
  250, 1, 250, 3, 251, 6, 253, 8, 255, 9, 2, 9, 4, 8, 6, 6 };

static unsigned char char100[] =
{ 17, 247, 10,
  6, 244, 6, 9, 192, 0, 6, 254, 4, 252, 2, 251, 255,
  251, 253, 252, 251, 254, 250, 1, 250, 3, 251, 6, 253, 8, 255, 9,
  2, 9, 4, 8, 6, 6 };
                                 
static unsigned char char101[] =
{ 17, 247, 9,
  250, 1, 6, 1, 6, 255, 5, 253, 4, 252, 2, 251, 255,
  251, 253, 252, 251, 254, 250, 1, 250, 3, 251, 6, 253, 8, 255, 9,
  2, 9, 4, 8, 6, 6 };

static unsigned char char102[] =
{ 8, 251, 7,
  5, 244, 3, 244, 1, 245, 0, 248, 0, 9, 192, 0, 253,
  251, 4, 251 };

static unsigned char char103[] =
{ 22, 247, 10,
  6, 251, 6, 11, 5, 14, 4, 15, 2, 16, 255, 16, 253,
  15, 192, 0, 6, 254, 4, 252, 2, 251, 255, 251, 253, 252, 251,
  254, 250, 1, 250, 3, 251, 6, 253, 8, 255, 9, 2, 9, 4, 8, 6, 6 };

static unsigned char char104[] =
{ 10, 247, 10,
  251, 244, 251, 9, 192, 0, 251, 255, 254, 252, 0,
  251, 3, 251, 5, 252, 6, 255, 6, 9 };

static unsigned char char105[] =
{ 8, 252, 4,
  255, 244, 0, 245, 1, 244, 0, 243, 255, 244, 192, 0,
  0, 251, 0, 9 };

static unsigned char char106[] =
{ 11, 251, 5,
  0, 244, 1, 245, 2, 244, 1, 243, 0, 244, 192, 0, 1,
  251, 1, 12, 0, 15, 254, 16, 252, 16 };

static unsigned char char107[] =
{ 8, 247, 8,
  251, 244, 251, 9, 192, 0, 5, 251, 251, 5, 192, 0, 255, 1, 6, 9 };

static unsigned char char108[] =
{ 2, 252, 4,
  0, 244, 0, 9 };
                                        
static unsigned char char109[] =
{ 18, 241, 15,
  245, 251, 245, 9, 192, 0, 245, 255, 248, 252, 250,
  251, 253, 251, 255, 252, 0, 255, 0, 9, 192, 0, 0, 255, 3, 252,
  5, 251, 8, 251, 10, 252, 11, 255, 11, 9 };

static unsigned char char110[] =
{ 10, 247, 10,
  251, 251, 251, 9, 192, 0, 251, 255, 254, 252, 0,
  251, 3, 251, 5, 252, 6, 255, 6, 9 };

static unsigned char char111[] =
{ 17, 247, 10,
 255, 251, 253, 252, 251, 254, 250, 1, 250, 3, 251,
  6, 253, 8, 255, 9, 2, 9, 4, 8, 6, 6, 7, 3, 7, 1, 6, 254, 4, 252,
  2, 251, 255, 251 };

static unsigned char char112[] =
{ 17, 246, 9,
  250, 251, 250, 16, 192, 0, 250, 254, 252, 252, 254,
  251, 1, 251, 3, 252, 5, 254, 6, 1, 6, 3, 5, 6, 3, 8, 1, 9, 254,
  9, 252, 8, 250, 6 };

static unsigned char char113[] =
{ 17, 247, 10,
  6, 251, 6, 16, 192, 0, 6, 254, 4, 252, 2, 251, 255,
  251, 253, 252, 251, 254, 250, 1, 250, 3, 251, 6, 253, 8, 255, 9,
  2, 9, 4, 8, 6, 6 };
                                 
static unsigned char char114[] =
{ 8, 249, 6,
  253, 251, 253, 9, 192, 0, 253, 1, 254, 254, 0, 252,
  2, 251, 5, 251 };
                               
static unsigned char char115[] =
{ 17, 248, 9,
  6, 254, 5, 252, 2, 251, 255, 251, 252, 252, 251,
  254, 252, 0, 254, 1, 3, 2, 5, 3, 6, 5, 6, 6, 5, 8, 2, 9, 255, 9,
  252, 8, 251, 6 };

static unsigned char char116[] =
{ 8, 251, 7,
  0, 244, 0, 5, 1, 8, 3, 9, 5, 9, 192, 0, 253, 251, 4, 251 };

static unsigned char char117[] =
{ 10, 247, 10,
  251, 251, 251, 5, 252, 8, 254, 9, 1, 9, 3, 8, 6, 5,
  192, 0, 6, 251, 6, 9 };

static unsigned char char118[] =
{ 5, 248, 8,
  250, 251, 0, 9, 192, 0, 6, 251, 0, 9 };
                                                                
static unsigned char char119[] =
{ 11, 245, 11,
  248, 251, 252, 9, 192, 0, 0, 251, 252, 9, 192, 0,
  0, 251, 4, 9, 192, 0, 8, 251, 4, 9 };

static unsigned char char120[] =
{ 5, 248, 9,
  251, 251, 6, 9, 192, 0, 6, 251, 251, 9 };

static unsigned char char121[] =
{ 9, 248, 8,
  250, 251, 0, 9, 192, 0, 6, 251, 0, 9, 254, 13, 252,
  15, 250, 16, 249, 16 };
                                     
static unsigned char char122[] =
{ 8, 248, 9,
  6, 251, 251, 9, 192, 0, 251, 251, 6, 251, 192, 0,
  251, 9, 6, 9 };

static unsigned char char123[] =
{ 39, 249, 7,
  2, 240, 0, 241, 255, 242, 254, 244, 254, 246, 255,
  248, 0, 249, 1, 251, 1, 253, 255, 255, 192, 0, 0, 241, 255, 243,
  255, 245, 0, 247, 1, 248, 2, 250, 2, 252, 1, 254, 253, 0, 1, 2,
  2, 4, 2, 6, 1, 8, 0, 9, 255, 11, 255, 13, 0, 15, 192, 0, 255, 1,
  1, 3, 1, 5, 0, 7, 255, 8, 254, 10, 254, 12, 255, 14, 0, 15, 2, 16 };

static unsigned char char124[] =
{ 2, 252, 4,
  0, 240, 0, 16 };

static unsigned char char125[] =
{ 39, 249, 7,
  254, 240, 0, 241, 1, 242, 2, 244, 2, 246, 1, 248, 0,
  249, 255, 251, 255, 253, 1, 255, 192, 0, 0, 241, 1, 243, 1, 245,
  0, 247, 255, 248, 254, 250, 254, 252, 255, 254, 3, 0, 255, 2,
  254, 4, 254, 6, 255, 8, 0, 9, 1, 11, 1, 13, 0, 15, 192, 0, 1, 1,
  255, 3, 255, 5, 0, 7, 1, 8, 2, 10, 2, 12, 1, 14, 0, 15, 254, 16 };

static unsigned char char126[] =
{ 23, 255, 21,
  2, 1, 0, 255, 1, 253, 3, 251, 5, 251, 7, 252,
  11, 255, 13, 0, 15, 0, 17, 255, 18, 254, 192, 0, 2, 0, 1,
  254, 3, 253, 5, 253, 7, 254, 11, 1, 13, 2, 15, 2, 17, 1, 18,
  255, 18, 252 };

/* Pointers to character definition tables. */

static unsigned char * fontData[] = {
    char32, char33, char34, char35, char36, char37, char38, char39, char40,
    char41, char42, char43, char44, char45, char46, char47, char48, char49,
    char50, char51, char52, char53, char54, char55, char56, char57, char58,
    char59, char60, char61, char62, char63, char64, char65, char66, char67,
    char68, char69, char70, char71, char72, char73, char74, char75, char76,
    char77, char78, char79, char80, char81, char82, char83, char84, char85,
    char86, char87, char88, char89, char90, char91, char92, char93, char94,
    char95, char96, char97, char98, char99, char100, char101, char102,
    char103, char104, char105, char106, char107, char108, char109, char110,
    char111, char112, char113, char114, char115, char116, char117, char118,
    char119, char120, char121, char122, char123, char124, char125, char126
};



static void
writeGlyphCommand(FILE *                   const ofP,
                  struct ppmd_glyphCommand const glyphCommand) {

    fputc(glyphCommand.verb, ofP);
    fputc(glyphCommand.x, ofP);
    fputc(glyphCommand.y, ofP);
}    



static void
writeMovePen(FILE *                const ofP,
             const unsigned char * const glyphData) {

    struct ppmd_glyphCommand glyphCommand;
            
    glyphCommand.verb = CMD_MOVEPEN;
    glyphCommand.x = glyphData[0];
    glyphCommand.y = glyphData[1];
    
    writeGlyphCommand(ofP, glyphCommand);
}



static void
writeMovePenNoop(FILE *                const ofP,
                 const unsigned char * const glyphData) {

    struct ppmd_glyphCommand glyphCommand;
            
    glyphCommand.verb = CMD_MOVEPEN;
    glyphCommand.x = glyphData[0];
    glyphCommand.y = glyphData[1];
    
    writeGlyphCommand(ofP, glyphCommand);
                
    glyphCommand.verb = CMD_NOOP;
    glyphCommand.x = 0;
    glyphCommand.y = 0;

    writeGlyphCommand(ofP, glyphCommand);
}



static void
writeDrawLine(FILE *                const ofP,
              const unsigned char * const glyphData) {

    struct ppmd_glyphCommand glyphCommand;

    glyphCommand.verb = CMD_DRAWLINE;
    glyphCommand.x = glyphData[0];
    glyphCommand.y = glyphData[1];
    
    writeGlyphCommand(ofP, glyphCommand);
}
            


static void
writeGlyphHeader(FILE *                  const ofP,
                 struct ppmd_glyphHeader const glyphHeader) {

    fputc(glyphHeader.commandCount, ofP);
    fputc(glyphHeader.skipBefore, ofP);
    fputc(glyphHeader.skipAfter, ofP);
}    



static void
writeBuiltinCharacter(FILE *       const ofP,
                      unsigned int const relativeCodePoint) {

    const unsigned char * const glyphData = fontData[relativeCodePoint];

    struct ppmd_glyphHeader glyphHeader;
    unsigned int commandNum;

    glyphHeader.commandCount = glyphData[0];
    glyphHeader.skipBefore   = glyphData[1];
    glyphHeader.skipAfter    = glyphData[2];

    writeGlyphHeader(ofP, glyphHeader);

    commandNum = 0;

    while (commandNum < glyphHeader.commandCount) {
            
        if (commandNum == 0) {
            writeMovePen(ofP, &glyphData[3 + commandNum * 2]);
            commandNum += 1;
        } else if (glyphData[3 + commandNum*2] == 192) {

            assert(commandNum + 1 < glyphHeader.commandCount);

            writeMovePenNoop(ofP, &glyphData[3 + (commandNum + 1) * 2]);

            commandNum += 2;
        } else {
            writeDrawLine(ofP, &glyphData[3 + commandNum * 2]);
            commandNum += 1;
        }
    }
}



static void
writeFontHeader(FILE *                 const ofP,
                struct ppmd_fontHeader const fontHeader) {

    fwrite(fontHeader.signature, 1, sizeof(fontHeader.signature), ofP);
    fputc(fontHeader.format, ofP);
    fputc(fontHeader.characterCount, ofP);
    fputc(fontHeader.firstCodePoint, ofP);
}



static void
writeBuiltinFont(FILE * const ofP) {

    unsigned int relativeCodePoint;

    struct ppmd_fontHeader fontHeader;

    memcpy(fontHeader.signature, "ppmdfont", sizeof(fontHeader.signature));
    fontHeader.format         = 0x01;
    fontHeader.characterCount = 95;
    fontHeader.firstCodePoint = 32;

    writeFontHeader(ofP, fontHeader);

    for (relativeCodePoint = 0;
         relativeCodePoint < fontHeader.characterCount;
         ++relativeCodePoint) {

        writeBuiltinCharacter(ofP,relativeCodePoint);
    }
}



int
main(int argc, char **argv) {

    ppm_init(&argc, argv);

    writeBuiltinFont(stdout);
    
    return 0;
}
