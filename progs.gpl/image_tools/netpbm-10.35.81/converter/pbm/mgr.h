/* mgr.h - the following defs are taken from the MGR header file lib/dump.h
*/

#ifndef MGR_H_INCLUDED
#define MGR_H_INCLUDED

struct old_b_header {
   char magic[2];
   char h_wide;
   char l_wide;
   char h_high;
   char l_high;
   };

struct b_header {
   char magic[2];
   char h_wide;
   char l_wide;
   char h_high;
   char l_high;
   char depth;
   char _reserved;
   };

#endif
