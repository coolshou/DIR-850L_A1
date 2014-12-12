#!/bin/sh

echo "CC=$CC" > netpbm-10.35.81/Makefile.config
echo "AR=$AR" >> netpbm-10.35.81/Makefile.config
echo "RANLIB=$RANLIB" >> netpbm-10.35.81/Makefile.config
echo "TIFFHDR_DIR=$TIFFHDR_DIR" >> netpbm-10.35.81/Makefile.config
echo "TIFFLIB=$TIFFLIB" >> netpbm-10.35.81/Makefile.config
echo "JPEGHDR_DIR=$JPEGHDR_DIR" >> netpbm-10.35.81/Makefile.config
echo "JPEGLIB=$JPEGLIB" >> netpbm-10.35.81/Makefile.config
echo "PNGHDR_DIR=$PNGHDR_DIR" >> netpbm-10.35.81/Makefile.config
echo "PNGLIB=$PNGLIB" >> netpbm-10.35.81/Makefile.config
echo "ZHDR_DIR=$ZHDR_DIR" >> netpbm-10.35.81/Makefile.config
echo "ZLIB=$ZLIB" >> netpbm-10.35.81/Makefile.config

cat >> "netpbm-10.35.81/Makefile.config" << "EOF"
DEFAULT_TARGET = nonmerge
BUILD_FIASCO = Y
LD = $(CC)
INTTYPES_H = <inttypes.h>
HAVE_INT64 = Y
CC_FOR_BUILD = cc
LD_FOR_BUILD = cc
CFLAGS_FOR_BUILD = $(CFLAGS)
INSTALL = $(SRCDIR)/buildtools/install.sh
STRIPFLAG = -s
SYMLINK = ln -s
MANPAGE_FORMAT = nroff
LEX = flex
EXE =
  
LDSHLIB = -shared -Wl,-soname,$(SONAME)
SHLIB_CLIB =
NEED_RUNTIME_PATH = N
RPATHOPTNAME = -rpath
NETPBMLIB_RUNTIME_PATH = 
TIFFLIB_NEEDS_JPEG = Y
TIFFLIB_NEEDS_Z = Y
PNGVER = 
JBIGLIB = $(BUILDDIR)/converter/other/jbig/libjbig.a
JBIGHDR_DIR = $(SRCDIR)/converter/other/jbig
JASPERLIB = $(INTERNAL_JASPERLIB)
JASPERHDR_DIR = $(INTERNAL_JASPERHDR_DIR)
JASPERDEPLIBS =
URTLIB = $(BUILDDIR)/urt/librle.a
URTHDR_DIR = $(SRCDIR)/urt
X11LIB = NONE
X11HDR_DIR =
LINUXSVGALIB = NONE
LINUXSVGAHDR_DIR = 
OMIT_NETWORK =
NETWORKLD = 
VMS = 
DONT_HAVE_PROCESS_MGMT = N
PKGDIR_DEFAULT = /tmp/netpbm
PKGMANDIR = man
INSTALL_PERM_BIN =  755       # u=rwx,go=rx
INSTALL_PERM_LIBD = 755       # u=rwx,go=rx
INSTALL_PERM_LIBS = 644       # u=rw,go=r
INSTALL_PERM_HDR =  644       # u=rw,go=r
INSTALL_PERM_MAN =  644       # u=rw,go=r
INSTALL_PERM_DATA = 644       # u=rw,go=r
SUFFIXMANUALS1 = 1
SUFFIXMANUALS3 = 3
SUFFIXMANUALS5 = 5
STATICLIBSUFFIX = a
SHLIBPREFIXLIST = lib
NETPBMSHLIBPREFIX = $(firstword $(SHLIBPREFIXLIST))
DLLVER =
DEFAULT_TARGET = nonmerge
NETPBMLIBTYPE=unixshared
NETPBMLIBSUFFIX=so
STATICLIB_TOO=y
CFLAGS = -O3 -ffast-math  -pedantic -fno-common -Wall -Wno-uninitialized -Wmissing-declarations -Wimplicit -Wwrite-strings -Wmissing-prototypes -Wundef
CFLAGS_MERGE = -Wno-missing-declarations -Wno-missing-prototypes
LDRELOC = ld --reloc
LINKER_CAN_DO_EXPLICIT_LIBRARY=Y
LINKERISCOMPILER = Y
CFLAGS_SHLIB += -fPIC
NETPBM_DOCURL = http://netpbm.sourceforge.net/doc/
EOF

