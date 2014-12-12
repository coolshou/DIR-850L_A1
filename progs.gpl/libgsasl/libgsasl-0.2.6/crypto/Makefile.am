## Process this file with automake to produce Makefile.in
# Copyright (C) 2002, 2003, 2004 Simon Josefsson.
#
# This file is part of GNU SASL Library.
#
# GNU SASL Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1 of
# the License, or (at your option) any later version.
#
# GNU SASL Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with GNU SASL Library; if not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.

EXTRA_DIST = README

AM_CPPFLAGS = @LIBGCRYPT_CFLAGS@ -I$(srcdir)/../gl -I../gl

BUILT_SOURCES = nettle-types.h
DISTCLEANFILES = nettle-types.h

noinst_LTLIBRARIES = libgc.la

libgc_la_SOURCES = gc.h
libgc_la_LIBADD = @LIBGCRYPT_LIBS@

if NETTLE
libgc_la_SOURCES += gc-nettle.c \
	macros.h nettle-meta.h nettle-internal.h cbc.h \
	memxor.c memxor.h \
	md5.h md5.c md5-meta.c \
	sha.h \
	hmac.c hmac.h hmac-md5.c
endif

if LIBGCRYPT
libgc_la_SOURCES += gc-libgcrypt.c
endif
