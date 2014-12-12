/*
  By Jim Frost 1989.10.02.

  Copyright 1989 Jim Frost.
  See COPYRIGHT file for copyright information.
*/
#ifndef VALTOMEM_H_INCLUDED
#define VALTOMEM_H_INCLUDED

/* inline these functions for speed.  these only work for {len : 1,2,3,4}.
 */

#define memToVal(PTR,LEN) \
  ((LEN) == 1 ? ((unsigned long)(*((unsigned char *)PTR))) : \
   ((LEN) == 3 ? ((unsigned long) \
          (*(unsigned char *)(PTR) << 16) | \
          (*((unsigned char *)(PTR) + 1) << 8) | \
          (*((unsigned char *)(PTR) + 2))) : \
    ((LEN) == 2 ? ((unsigned long) \
           (*(unsigned char *)(PTR) << 8) | \
           (*((unsigned char *)(PTR) + 1))) : \
     ((unsigned long)((*(unsigned char *)(PTR) << 24) | \
              (*((unsigned char *)(PTR) + 1) << 16) | \
              (*((unsigned char *)(PTR) + 2) << 8) | \
              (*((unsigned char *)(PTR) + 3)))))))

#define memToValLSB(PTR,LEN) \
  ((LEN) == 1 ? ((unsigned long)(*(unsigned char *)(PTR))) : \
   ((LEN) == 3 ? ((unsigned long) \
          (*(unsigned char *)(PTR)) | \
          (*((unsigned char *)(PTR) + 1) << 8) | \
          (*((unsigned char *)(PTR) + 2) << 16)) : \
    ((LEN) == 2 ? ((unsigned long) \
           (*(unsigned char *)(PTR)) | (*((unsigned char *)(PTR) + 1) << 8)) : \
     ((unsigned long)((*(unsigned char *)(PTR)) | \
              (*((unsigned char *)(PTR) + 1) << 8) | \
              (*((unsigned char *)(PTR) + 2) << 16) | \
              (*((unsigned char *)(PTR) + 3) << 24))))))

#define valToMem(VAL,PTR,LEN) \
  ((LEN) == 1 ? (*(unsigned char *)(PTR) = ((unsigned int)(VAL) & 0xff)) : \
   ((LEN) == 3 ? (((*(unsigned char *)(PTR)) = ((unsigned int)(VAL) & 0xff0000) >> 16), \
          ((*((unsigned char *)(PTR) + 1)) = ((unsigned int)(VAL) & 0xff00) >> 8), \
          ((*((unsigned char *)(PTR) + 2)) = ((unsigned int)(VAL) & 0xff))) : \
    ((LEN) == 2 ? (((*(unsigned char *)(PTR)) = ((unsigned int)(VAL) & 0xff00) >> 8), \
           ((*((unsigned char *)(PTR) + 1)) = ((unsigned int)(VAL) & 0xff))) : \
     (((*(unsigned char *)(PTR)) = ((unsigned int)(VAL) & 0xff000000) >> 24), \
      ((*((unsigned char *)(PTR) + 1)) = ((unsigned int)(VAL) & 0xff0000) >> 16), \
      ((*((unsigned char *)(PTR) + 2)) = ((unsigned int)(VAL) & 0xff00) >> 8), \
      ((*((unsigned char *)(PTR) + 3)) = ((unsigned int)(VAL) & 0xff))))))

#define valToMemLSB(VAL,PTR,LEN) \
  ((LEN) == 1 ? (*(unsigned char *)(PTR) = ((unsigned int)(VAL) & 0xff)) : \
   ((LEN) == 3 ? (((*(unsigned char *)(PTR) + 2) = ((unsigned int)(VAL) & 0xff0000) >> 16), \
          ((*((unsigned char *)(PTR) + 1)) = ((unsigned int)(VAL) & 0xff00) >> 8), \
          ((*(unsigned char *)(PTR)) = ((unsigned int)(VAL) & 0xff))) : \
    ((LEN) == 2 ? (((*((unsigned char *)(PTR) + 1) = ((unsigned int)(VAL) & 0xff00) >> 8), \
            ((*(unsigned char *)(PTR)) = ((unsigned int)(VAL) & 0xff)))) : \
     (((*((unsigned char *)(PTR) + 3)) = ((unsigned int)(VAL) & 0xff000000) >> 24), \
      ((*((unsigned char *)(PTR) + 2)) = ((unsigned int)(VAL) & 0xff0000) >> 16), \
      ((*((unsigned char *)(PTR) + 1)) = ((unsigned int)(VAL) & 0xff00) >> 8), \
      ((*(unsigned char *)(PTR)) = ((unsigned int)(VAL) & 0xff))))))


#endif
