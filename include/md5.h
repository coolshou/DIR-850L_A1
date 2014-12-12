/* vi: set sw=4 ts=4: */
/* MD5.H - header file for MD5C.C */
/*  Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
    rights reserved.

    License to copy and use this software is granted provided that it
    is identified as the &quot;RSA Data Security, Inc. MD5 Message-Digest
    Algorithm&quot; in all material mentioning or referencing this software
    or this function.

    License is also granted to make and use derivative works provided
    that such works are identified as &quot;derived from the RSA Data
    Security, Inc. MD5 Message-Digest Algorithm&quot; in all material
    mentioning or referencing the derived work.

    RSA Data Security, Inc. makes no representations concerning either
    the merchantability of this software or the suitability of this
    software for any particular purpose. It is provided &quot;as is&quot;
    without express or implied warranty of any kind.

    These notices must be retained in any copies of any part of this
    documentation and/or software.
*/

#ifndef __MD5_HEADER_H__

/*For 64-bit CPU, Builder, 2008/06/12*/
/*+++*/
#include <stdint.h>
/*+++*/

/*For 64-bit CPU, Builder, 2008/06/12*/
/*+++*/
#if 0
typedef unsigned char *		POINTER;
typedef unsigned short int	UINT2;
typedef unsigned long int	UINT4;
#else
typedef uint8_t *	POINTER;
typedef uint16_t	UINT2;
typedef uint32_t	UINT4;
#endif
/*+++*/

#ifdef __cplusplus
extern "C" {
#endif

/* MD5 context. */
typedef struct
{
	UINT4 state[4];				/* state (ABCD) */
	UINT4 count[2];				/* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64];	/* input buffer */
}
MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(unsigned char [16], MD5_CTX *);

#ifdef __cplusplus
}
#endif

#endif	/* endof #ifndef __MD5_HEADER_H__ */
