/*=============================================================================
  This file is the part of wordaccess.h for use under these
  conditions:

  * GCC (>=3.4), GLIBC
  * Big-Endian machines
  
  __builtin_clz is available on GCC 3.4 and above
     
  Note that the clz scheme does not work and requires adjustment
  if long type does not make use of all bits for data storage.
  
  This is unlikely.  According to GNU MP (http://www.swox.com/gmp/),
  in rare cases such as Cray, there are smaller data types that take up
  the same space as long, but leave the higher bits silent.  Currently,
  there are no known such cases for data type long.
*===========================================================================*/

typedef unsigned long int wordint;
typedef unsigned char wordintBytes[sizeof(wordint)];

static __inline__ wordint
bytesToWordint(wordintBytes bytes) {
    return *((wordint *)bytes);
}



static __inline__ void
wordintToBytes(wordintBytes * const bytesP,
               wordint        const wordInt) {
    *(wordint *)bytesP = wordInt;
}



static __inline__ unsigned int
wordintClz(wordint const x) {
    return (x==0 ? sizeof(wordint)*8 : __builtin_clzl(x));
}
