/*
 * squashfs with lzma compression
 *
 * Copyright (C) 2010, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: sqlzma.c 241381 2011-02-18 03:28:19Z stakita $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "LzmaDec.h"
#include "LzmaEnc.h"
#include "sqlzma.h"

static void *SzAlloc(void *p, size_t size) { p = p; return malloc(size); }
static void SzFree(void *p, void *address) { p = p; free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

int LzmaUncompress(char *dst, unsigned long * dstlen, char *src, int srclen)
{
	int res;
	SizeT inSizePure;
	ELzmaStatus status;

	if (srclen < LZMA_PROPS_SIZE)
	{
		memcpy(dst, src, srclen);
		return srclen;
	}
	inSizePure = srclen - LZMA_PROPS_SIZE;
	res = LzmaDecode(dst, dstlen, src + LZMA_PROPS_SIZE, &inSizePure,
	                 src, LZMA_PROPS_SIZE, LZMA_FINISH_ANY, &status, &g_Alloc);
	srclen = inSizePure ;

	if ((res == SZ_OK) ||
		((res == SZ_ERROR_INPUT_EOF) && (srclen == inSizePure)))
		res = 0;
	if (res != SZ_OK)
		printf("LzmaUncompress: error (%d)\n", res);
	return res;
}


int LzmaCompress(char *in_data, int in_size, char *out_data, int out_size, unsigned long *total_out)
{
	CLzmaEncProps props;
	size_t headerSize = LZMA_PROPS_SIZE;
	int ret;
	SizeT outProcess;

	LzmaEncProps_Init(&props);
	props.algo = 1;
	outProcess = out_size - LZMA_PROPS_SIZE;
	ret = LzmaEncode(out_data+LZMA_PROPS_SIZE, &outProcess, in_data, in_size, &props, out_data, 
						&headerSize, 0, NULL, &g_Alloc, &g_Alloc);
	*total_out = outProcess + LZMA_PROPS_SIZE;
	return ret;
}
