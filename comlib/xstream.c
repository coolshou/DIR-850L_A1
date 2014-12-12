/* vi: set sw=4 ts=4: */
/*
 *	xstream.c
 *
 *	extensible stream module.
 *
 *	Created by David Hsieh <david_hsieh@alphanetworks.com>
 *	Copyright (C) 2007-2009 by Alpha Networks, Inc.
 *
 *	This file is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either'
 *	version 2.1 of the License, or (at your option) any later version.
 *
 *	The GNU C Library is distributed in the hope that it will be useful,'
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	Lesser General Public License for more details.
 *
 *	You should have received a copy of the GNU Lesser General Public
 *	License along with the GNU C Library; if not, write to the Free
 *	Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *	02111-1307 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <dtrace.h>
#include <mem_helper.h>

#include "xstream.h"

#define MAX_XS_UNDO		512	/* the maximum number of the unget() function can be accept. */
#define XSTREAM_T(x)	(struct xstream *)(x)

struct xsbuff
{
	unsigned char * start;
	size_t curr, size;
};

struct fdopen
{
	FILE * file;
	int bytesleft;
};

struct xstream
{
	xstype_t			type;
	union
	{
		FILE *			file;
		struct fdopen	fdopen;
		struct xsbuff	buff;
	}					fd;
	unsigned char		undo[MAX_XS_UNDO];
	size_t				udsize;
};

/* open file type stream */
xstream_t xs_fopen(const char * file, const char * mode)
{
	FILE * fd;
	struct xstream * xs;

	fd = fopen(file, mode);
	if (fd)
	{
		xs = xmalloc(sizeof(struct xstream));
		if (xs)
		{
			memset(xs, 0, sizeof(struct xstream));
			xs->type = XSTYPE_FILE;
			xs->fd.file = fd;
			return xs;
		}
		fclose(fd);
	}
	return NULL;
}

xstream_t xs_fdopen(int fd, const char * mode, int max)
{
	struct xstream * xs;

	xs = xmalloc(sizeof(struct xstream));
	if (xs)
	{
		memset(xs, 0, sizeof(struct xstream));
		xs->fd.fdopen.file = fdopen(fd, mode);
		if (xs->fd.fdopen.file)
		{
			xs->type = XSTYPE_FDOPEN;
			xs->fd.fdopen.bytesleft = max;	/* how many bytes left to read */
		}
		else
		{
			xfree(xs);
			xs = NULL;
		}
	}
	return xs;
}

/* open buffer type stream */
xstream_t xs_bopen(void * buff, size_t size)
{
	struct xstream * xs;
	if (buff && size)
	{
		xs = xmalloc(sizeof(struct xstream));
		if (xs)
		{
			memset(xs, 0, sizeof(struct xstream));
			xs->type = XSTYPE_BUFFER;
			xs->fd.buff.start = (unsigned char *)buff;
			xs->fd.buff.curr = 0;
			xs->fd.buff.size = size;
			return xs;
		}
	}
	return NULL;
}

int xs_close(xstream_t fd)
{
	struct xstream * xs = XSTREAM_T(fd);
	if (!xs) return -1;
	switch (xs->type)
	{
	case XSTYPE_FILE: fclose(xs->fd.file); break;
	case XSTYPE_FDOPEN:	if (xs->fd.fdopen.file) fclose(xs->fd.fdopen.file); break;
	case XSTYPE_BUFFER:	break;
	case XSTYPE_UNKNOWN: break;
	}
	xfree(xs);
	return 0;
}

int xs_getc(xstream_t fd)
{
	struct xstream * xs = XSTREAM_T(fd);
	int c = EOF;

	if (!xs) return c;

	/* read the undo buffer first. */
	if (xs->udsize > 0)
	{
		xs->udsize--;
		c = (int)(xs->undo[xs->udsize]);
		return c;
	}

	/* undo buffer is empty, lets really read something. */
	switch (xs->type)
	{
	case XSTYPE_FILE:	c = fgetc(xs->fd.file);	break;
	case XSTYPE_FDOPEN:
		if (xs->fd.fdopen.bytesleft > 0)
		{
			c = fgetc(xs->fd.fdopen.file);
			if (c == EOF) xs->fd.fdopen.bytesleft = 0;
			else xs->fd.fdopen.bytesleft--;
		}
		else c = EOF;
		break;
	case XSTYPE_BUFFER:
		if (xs->fd.buff.curr < xs->fd.buff.size)
		{
			c = (int)xs->fd.buff.start[xs->fd.buff.curr];
			xs->fd.buff.curr++;
		}
		break;

	default:
		c = EOF;
		break;
	}
	return c;
}

int xs_ungetc(int c, xstream_t fd)
{
	struct xstream * xs = XSTREAM_T(fd);

	if (!xs) return EOF;
	if (c==EOF) return EOF;

	/* put it to the undo buffer. */
	if (xs->udsize < MAX_XS_UNDO)
	{
		xs->undo[xs->udsize] = (unsigned char)c;
		xs->udsize++;
		return c;
	}
	return EOF;
}

int xs_ungets(const char * s, xstream_t fd)
{
	int c;
	size_t len;

	if (!s) return 0;
	len = strlen(s);
	while (len>0)
	{
		c = s[--len];
		if (xs_ungetc(c, fd)==EOF) return EOF;
	}
	return 0;
}
