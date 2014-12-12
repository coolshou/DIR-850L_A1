
# Copyright (C) 2006-2008 Junjiro Okajima
# Copyright (C) 2006-2008 Tomas Matejicek, slax.org
#
# LICENSE follows the described ones in lzma and squashfs.

# $Id: Makefile,v 1.30 2008-03-12 16:24:54 jro Exp $

# paths
Sqlzma = ${CURDIR}

LzmaVer = lzma-457
Lzma = ${Sqlzma}/${LzmaVer}
SqVer = squashfs3.4
Squashfs = ${Sqlzma}/${SqVer}
#KVer = linux-2.6.27.4
#SqFs = ${Squashfs}/kernel-patches/${KVer}/fs/squashfs
#KDir = /lib/modules/$(shell uname -r)/build
#KDir = ${SqFs}/../..

ifeq (${LzmaVer}, lzma443)
LzmaC = ${Lzma}/C/7zip/Compress/LZMA_C
LzmaAlone = ${Lzma}/C/7zip/Compress/LZMA_Alone
else
LzmaC = ${Lzma}/C/Compress/Lzma
LzmaAlone = ${Lzma}/CPP/7zip/Compress/LZMA_Alone
endif
SqTools = ${Squashfs}/squashfs-tools

# enable it if you want to add -g option when compiling
#UseDebugFlags = 1
#MyDebugFlags = -DSQUASHFS_TRACE
# disable it if you don't want to compile squashfs kernel module here
BuildSquashfs = 0

export

all:
	${MAKE} -C ${LzmaC} -f sqlzma.mk $@
	${MAKE} -C ${LzmaAlone} -f sqlzma.mk $@
	${MAKE} -C ${SqTools} $@
	mv ${SqTools}/mksquashfs .
	mv ${SqTools}/unsquashfs .

clean:
	@${MAKE} -C ${LzmaC} -f sqlzma.mk $@
	@${MAKE} -C ${LzmaAlone} -f sqlzma.mk $@
	@${MAKE} -C ${SqTools} $@
	@${RM} *~ ./mksquashfs
	@${RM} *~ ./unsquashfs

########################################

-include priv.mk

########################################

load:
	for i in ${LzmaC}/kmod/unlzma.ko ${LzmaC}/kmod/sqlzma.ko \
		${SqFs}/squashfs.ko; \
	do sudo insmod $$i; done

unload:
	-sudo rmmod squashfs sqlzma unlzma
