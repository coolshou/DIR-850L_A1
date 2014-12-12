#ifndef WORDACCESS_H_INCLUDED
#define WORDACCESS_H_INCLUDED

/* These are facilities for accessing data in C programs in ways that
   exploit the way the machine defines words in order to squeeze out
   speed and CPU efficiency.

   In particular, routines in this file exploit the endianness of the
   machine and use explicit machine instructions to access C
   variables.

   A word is the amount of data that fits in a register; the amount of
   data that a single machine instruction can process.  For example,
   on IA32, a word is 32 bits because a single load or store
   instruction moves that many bits and a single add instruction
   operates on that many bits.


   These facilities revolve around two data types:  wordInt and
   wordIntBytes.

   wordint is an unsigned integer with precision (size) of one word.
   It is just the number -- nothing is implied about how it is
   represented in memory.

   wordintBytes is an array of bytes that represent a word-sized
   unsigned integer.  x[0] is the high order 8 digits of the binary
   coding of the integer, x[1] the next highest 8 digits, etc.
   Note that it has big-endian form, regardless of what endianness the
   underlying machine uses.

   The actual size of word differs by machine.  Usually it is 32 or 64
   bits.  Logically it can be as small as one byte.  Fixed bit sequences
   in each program impose a lower limit of word width.  For example, the
   longest bit sequence in pbmtog3 has 13 bits, so an 8-bit word won't
   work with that.

   We also assume that a char is 8 bits.
*/
#if (!defined(WORDACCESS_GENERIC) \
     && defined(__GNUC__) && defined(__GLIBC__) \
     && (__GNUC__ * 100 + __GNUC_MINOR__ >= 304) )

    #if BYTE_ORDER==BIG_ENDIAN    /* defined by GCC */

        #include "wordaccess_gcc3_be.h"

    #elif defined(__ia64__) || defined(__amd64__) || defined(__x86_64__)
         /* all these macros are defined by GCC */

        #include "wordaccess_64_le.h"

    #else

        #include "wordaccess_gcc3_le.h"

    #endif

#else

    #include "wordaccess_generic.h"

#endif

#endif
