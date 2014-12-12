/*=============================================================================

  This file is the part of wordaccess.h for use under these
  conditions:

  * GCC (>=3.4), GLIBC
  * 32 bit Little-Endian machines (intel MPUs 80386, Pentium, etc.)
  * Other non-Big-Endian machines (very rare)
  
=============================================================================*/

typedef uint32_t wordint;
typedef unsigned char wordintBytes[sizeof(wordint)];

#include <sys/types.h>
#include <netinet/in.h>

/*
  Here we use the more widely used functions htonl and ntohl instead of
  bswap_32.  This makes possible the handling of weird byte ordering
  (neither Big-Endian nor Little-Endian) schemes, if any.
*/

static __inline__ wordint
bytesToWordint(wordintBytes const bytes) {
    return (wordint) ntohl(*(wordint *)bytes);
}



static __inline__ void
wordintToBytes(wordintBytes * const bytesP,
               wordint        const wordInt) {

    *(wordint *)bytesP = htonl(wordInt);
}



static __inline__ unsigned int
wordintClz(wordint const x) {

    /* Find the data type closest to 32 bits, and file off any extra.  */

    if (x == 0)
        return sizeof(wordint) * 8;
    else if (sizeof(int) >= 4)
        return __builtin_clz((int)x << (sizeof(int) - 4) * 8);
    else if (sizeof(long int) >= 4)
        return __builtin_clzl((long int)x << (sizeof(long int) - 4) * 8);
    else
        pm_error("Long int is less than 32 bits on this machine"); 
}

