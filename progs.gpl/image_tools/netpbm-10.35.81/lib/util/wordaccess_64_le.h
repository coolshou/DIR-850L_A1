/*=============================================================================
  This file is the part of wordaccess.h for use under these
  conditions:

  * GCC (>=3.4), GLIBC
  * 64 bit Little-Endian machines (IA64, X86-64, AMD64)
=============================================================================*/
   
/*  
    64 bit hton and ntoh do not exist.  Here we use bswap_64.
    
    While bswap_64 works on 64 bit data, __builtin_clzl works on "long" which
    may or may not be 64 bits.  Code provided to find the right data type and
    file off any extra when necessary.
*/

#include <byteswap.h>  /* See note above on bswap_64 */
 
typedef uint64_t wordint;
typedef unsigned char wordintBytes[sizeof(wordint)];

static __inline__ wordint
bytesToWordint(wordintBytes bytes) {
    return ((wordint) bswap_64(*(wordint *)bytes));
}



static __inline__ void
wordintToBytes(wordintBytes * const bytesP,
               wordint        const wordInt) {
    *(wordint *)bytesP = bswap_64(wordInt);
}



static __inline__ unsigned int
wordintClz(wordint const x){

    unsigned int s;

    if (x == 0)
        return sizeof(wordint) * 8;

    /* Find the data type closest to 64 bits, and file off any extra. */
    else if ((s=sizeof(long int)) >= 8)
        return (__builtin_clzl((long int)x << (s - 8) * 8));
    else if ((s=sizeof(long long int)) >= 8)
        return (__builtin_clzll((long long int)x << (s - 8) * 8));
    else
        pm_error("Long long int is less than 64 bits on this machine"); 
}
