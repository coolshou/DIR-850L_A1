/*=============================================================================

  This file is the part of wordaccess.h for use under these
  conditions:

  * Compilers other than GCC
  * GCC before version 3.4
  * c libraries other than Glibc
  * Specified by the user with WORDACCESS_GENERIC
=============================================================================*/

typedef uint32_t wordint;
typedef unsigned char wordintBytes[sizeof(wordint)];
    
static __inline__ wordint
bytesToWordint(wordintBytes const bytes) {
    wordint retval;
    unsigned int i;

    /* Note that 'bytes' is a pointer, due to C array degeneration.
       That means sizeof(bytes) isn't what you think it is.
    */
    
    for (i = 1, retval = bytes[0]; i < sizeof(wordint); ++i) {
        retval = (retval << 8) + bytes[i];
    }
    return retval;
}



static __inline__ void
wordintToBytes(wordintBytes * const bytesP,
               wordint        const wordInt) {

    wordint buffer;
    int i;

    for (i = sizeof(*bytesP)-1, buffer = wordInt; i >= 0; --i) {
        (*bytesP)[i] = buffer & 0xFF;
        buffer >>= 8;
    }
}
    
static unsigned char const clz8[256]= {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
};



static __inline__ unsigned int
clz16(wordint const x) {
    if (x >> 8  != 0)
        return clz8[x >> 8];
    else
        return clz8[x] + 8;
}



static __inline__  unsigned int
clz32(wordint const x) {
    if (x >> 16  != 0)
        return clz16(x >> 16);
    else
        return clz16(x) +16;
}



static __inline__  unsigned int
wordintClz(wordint const x) {
    return clz32(x);
}
