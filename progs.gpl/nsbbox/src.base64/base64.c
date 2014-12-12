/*
 * base64.c -- Base64 Mime encoding
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: base64.c,v 1.1.1.1 2005/05/19 10:53:07 r01122 Exp $
 */

/******************************** Description *********************************/

/*
 *	The base64 command encodes and decodes a string in mime base64 format
 */

/********************************* Includes ***********************************/

#include	<stdio.h>
#include        <ctype.h>
#include        <stdlib.h>
#include        <string.h>
#include        <stdarg.h>

/******************************** Local Data **********************************/
/*
 *	Mapping of ANSI chars to base64 Mime encoding alphabet (see below)
 */
#define DEF_OUTLEN 64

static char	map64[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, 
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static char	alphabet64[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/',
};

/*********************************** Code *************************************/
/*
 *	Decode a buffer from "string" and into "outbuf"
 */

static int Decode64(char *outbuf, char *string, int outlen)
{
	unsigned long	shiftbuf;
	char		*cp, *op;
	int		c, i, j, shift;

	op = outbuf;
	*op = '\0';
	cp = string;
	while (*cp && *cp != '=') {
/*
 *		Map 4 (6bit) input bytes and store in a single long (shiftbuf)
 */
		shiftbuf = 0;
		shift = 18;
		for (i = 0; i < 4 && *cp && *cp != '='; i++, cp++) {
			c = map64[*cp & 0xff];
			if (c == -1) {
				printf("Bad string: %s at %c index %d\n", string, c, i);
				return -1;
			} 
			shiftbuf = shiftbuf | (c << shift);
			shift -= 6;
		}
/*
 *		Interpret as 3 normal 8 bit bytes (fill in reverse order).
 *		Check for potential buffer overflow before filling.
 */
		--i;
		if ((op + i) >= &outbuf[outlen]) {
			printf("String too big\n");
			return -1;
		}
		for (j = 0; j < i; j++) {
			*op++ = (char) ((shiftbuf >> (8 * (2 - j))) & 0xff);
		}
		*op = '\0';
	}
	return 0;
}


/******************************************************************************/
/*
 *	Encode a buffer from "string" into "outbuf"
 */

static void Encode64(char *outbuf, char *string, int outlen)
{
	unsigned long	shiftbuf;
	char		*cp, *op;
	int		x, i, j, shift;

	op = outbuf;
	*op = '\0';
	cp = string;
	while (*cp) {
/*
 *		Take three characters and create a 24 bit number in shiftbuf
 */
		shiftbuf = 0;
		for (j = 2; j >= 0 && *cp; j--, cp++) {
			shiftbuf |= ((*cp & 0xff) << (j * 8));
		}
/*
 *		Now convert shiftbuf to 4 base64 letters.  The i,j magic calculates
 *		how many letters need to be output.
 */
		shift = 18;
		for (i = ++j; i < 4 && op < &outbuf[outlen] ; i++) {
			x = (shiftbuf >> shift) & 0x3f;
			*op++ = alphabet64[(shiftbuf >> shift) & 0x3f];
			shift -= 6;
		}
/*
 *		Pad at the end with '='
 */
		while (j-- > 0) {
			*op++ = '=';
		}
		*op = '\0';
	}
}
/******************************************************************************/

static void base64usage(char *pstrExecName)
{
	printf("Usage: %s message\n", pstrExecName);
}

#ifdef NSBBOX
int base64_main(int argc, char * argv[])
#else
int main(int argc, char * argv[])
#endif
{
	char *base;
	char buf[DEF_OUTLEN+1];
	int  ret=0;

	base = strrchr(argv[0], '/');
	if (base) base = base + 1;
	else base = argv[0];
	if(argc<2)
	{
		base64usage(base);
		return -1;
	}

	if (strcmp(base, "b64enc")==0)		Encode64(buf, argv[1], DEF_OUTLEN);
	else if (strcmp(base, "b64dec")==0)	ret = Decode64(buf, argv[1], DEF_OUTLEN);

	if (ret==0) printf(buf);
	return ret;
}
