#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

unsigned short
get2(FILE * const ifp);

int
get4(FILE * const ifp);

void
read_shorts (FILE * const ifp, unsigned short *pixel, int count);

unsigned int
getbits (FILE * const ifp, int nbits);

#endif
