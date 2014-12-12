/* pbmpscale.c - pixel scaling with jagged edge smoothing.
 * AJCD 13/8/90
 */

#include <stdio.h>
#include "pbm.h"
#include "mallocvar.h"

/* prototypes */
void nextrow_pscale ARGS((FILE *ifd, int row));
int corner ARGS((int pat));

/* input bitmap size and storage */
int rows, columns, format ;
bit *inrow[3] ;

#define thisrow (1)

/* compass directions from west clockwise */
int xd_pscale[] = { -1, -1,  0,  1, 1, 1, 0, -1 } ;
int yd_pscale[] = {  0, -1, -1, -1, 0, 1, 1,  1 } ;

/* starting positions for corners */
#define NE(f) ((f) & 3)
#define SE(f) (((f) >> 2) & 3)
#define SW(f) (((f) >> 4) & 3)
#define NW(f) (((f) >> 6) & 3)

typedef unsigned short sixteenbits ;

/* list of corner patterns; bit 7 is current color, bits 0-6 are squares
 * around (excluding square behind), going clockwise.
 * The high byte of the patterns is a mask, which determines which bits are
 * not ignored.
 */

sixteenbits patterns[] = { 0x0000, 0xd555,         /* no corner */
                           0x0001, 0xffc1, 0xd514, /* normal corner */
                           0x0002, 0xd554, 0xd515, /* reduced corners */
                           0xbea2, 0xdfc0, 0xfd81,
                           0xfd80, 0xdf80,
                           0x0003, 0xbfa1, 0xfec2 /* reduced if > 1 */
                           };

/* search for corner patterns, return type of corner found:
 *  0 = no corner,
 *  1 = normal corner,
 *  2 = reduced corner,
 *  3 = reduced if cutoff > 1
 */

int corner(pat)
     int pat;
{
   register int i, r=0;
   for (i = 0; i < sizeof(patterns)/sizeof(sixteenbits); i++)
      if (patterns[i] < 0x100)
         r = patterns[i];
      else if ((pat & (patterns[i] >> 8)) ==
               (patterns[i] & (patterns[i] >> 8)))
         return r;
   return 0;
}

/* get a new row
 */

void nextrow_pscale(ifd, row)
     FILE *ifd;
     int row;
{
   bit *shuffle = inrow[0] ;
   inrow[0] = inrow[1];
   inrow[1] = inrow[2];
   inrow[2] = shuffle ;
   if (row < rows) {
      if (shuffle == NULL)
         inrow[2] = shuffle = pbm_allocrow(columns);
      pbm_readpbmrow(ifd, inrow[2], columns, format) ;
   } else inrow[2] = NULL; /* discard storage */

}

int
main(argc, argv)
     int argc;
     char *argv[];
{
   FILE *ifd;
   register bit *outrow;
   register int row, col, i, k;
   int scale, cutoff, ucutoff ;
   unsigned char *flags;

   pbm_init( &argc, argv );

   if (argc < 2)
      pm_usage("scale [pbmfile]");

   scale = atoi(argv[1]);
   if (scale < 1)
      pm_perror("bad scale (< 1)");

   if (argc == 3)
      ifd = pm_openr(argv[2]);
   else
      ifd = stdin ;

   inrow[0] = inrow[1] = inrow[2] = NULL;
   pbm_readpbminit(ifd, &columns, &rows, &format) ;

   outrow = pbm_allocrow(columns*scale) ;
   MALLOCARRAY(flags, columns);
   if (flags == NULL) 
       pm_error("out of memory") ;

   pbm_writepbminit(stdout, columns*scale, rows*scale, 0) ;

   cutoff = scale / 2;
   ucutoff = scale - 1 - cutoff;
   nextrow_pscale(ifd, 0);
   for (row = 0; row < rows; row++) {
      nextrow_pscale(ifd, row+1);
      for (col = 0; col < columns; col++) {
         flags[col] = 0 ;
         for (i = 0; i != 8; i += 2) {
            int vec = inrow[thisrow][col] != PBM_WHITE;
            for (k = 0; k < 7; k++) {
               int x = col + xd_pscale[(k+i)&7] ;
               int y = thisrow + yd_pscale[(k+i)&7] ;
               vec <<= 1;
               if (x >=0 && x < columns && inrow[y])
                  vec |= (inrow[y][x] != PBM_WHITE) ;
            }
            flags[col] |= corner(vec)<<i ;
         }
      }
      for (i = 0; i < scale; i++) {
         bit *ptr = outrow ;
         int zone = (i > ucutoff) - (i < cutoff) ;
         int cut = (zone < 0) ? (cutoff - i) :
                   (zone > 0) ? (i - ucutoff) : 0 ;

         for (col = 0; col < columns; col++) {
            int pix = inrow[thisrow][col] ;
            int flag = flags[col] ;
            int cutl, cutr ;

            switch (zone) {
            case -1:
               switch (NW(flag)) {
               case 0: cutl = 0; break;
               case 1: cutl = cut; break;
               case 2: cutl = cut ? cut-1 : 0; break;
               case 3: cutl = (cut && cutoff > 1) ? cut-1 : cut; break;
               default: cutl = 0;  /* Should never reach here */
               }
               switch (NE(flag)) {
               case 0: cutr = 0; break;
               case 1: cutr = cut; break;
               case 2: cutr = cut ? cut-1 : 0; break;
               case 3: cutr = (cut && cutoff > 1) ? cut-1 : cut; break;
               default: cutr = 0;  /* Should never reach here */
              }
               break;
            case 0:
               cutl = cutr = 0;
               break ;
            case 1:
               switch (SW(flag)) {
               case 0: cutl = 0; break;
               case 1: cutl = cut; break;
               case 2: cutl = cut ? cut-1 : 0; break;
               case 3: cutl = (cut && cutoff > 1) ? cut-1 : cut; break;
               default: cutl = 0;  /* should never reach here */
               }
               switch (SE(flag)) {
               case 0: cutr = 0; break;
               case 1: cutr = cut; break;
               case 2: cutr = cut ? cut-1 : 0; break;
               case 3: cutr = (cut && cutoff > 1) ? cut-1 : cut; break;
               default: cutr = 0;  /* should never reach here */
               }
               break;
             default: cutl = 0; cutr = 0;  /* Should never reach here */
            }
            for (k = 0; k < cutl; k++) /* left part */
               *ptr++ = !pix ;
            for (k = 0; k < scale-cutl-cutr; k++)  /* centre part */
               *ptr++ = pix ;
            for (k = 0; k < cutr; k++) /* right part */
               *ptr++ = !pix ;
         }
         pbm_writepbmrow(stdout, outrow, scale*columns, 0) ;
      }
   }
   pm_close(ifd);
   exit(0);
}
