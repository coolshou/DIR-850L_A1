# vi: set sw=4 ts=4:
#
# MIPS32 little endian.
# Toolchain: rsdk-1.5.5-5281-EB-2.6.30-0.9.30.3-110714
#############################################################################

Q?=@
ifeq ($(strip $(V)),y)
Q:=
endif

ifdef TPATH_UC
CC_PATH := $(TPATH_UC)
else
CC_PATH :=
endif

CROSS_COMPILE	:= mips-linux-
HOST_TYPE		:= mips-linux
HOST_CPU		:= mips
#TARGET_ABI      := 32

CC		= $(CROSS_COMPILE)gcc
CXX		= $(CROSS_COMPILE)g++
AS		= $(CROSS_COMPILE)as
AR		= $(CROSS_COMPILE)ar
LD		= $(CROSS_COMPILE)ld
RANLIB	= $(CROSS_COMPILE)ranlib
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY	= $(CROSS_COMPILE)objcopy

# DO NOT add -DLOGNUM=1 in arch.mk anymore.
# The following ELBOX_TEMPLATE_GW_WIFI case is for backward compatible.
# This option should be add in elbox_config.h & .config by menuconfig.
CFLAGS += -Os -Wall -D__UCLIBC_HAS_MMU__ -D__UCLIBC_ -D__BIG_ENDIAN_BITFIELD -DCONFIG_CPU_BIG_ENDIAN
ifeq ($(strip $(ELBOX_TEMPLATE_GW_WIFI)),y)
CFLAGS += -DLOGNUM=1
endif
LDFLAGS:=
CPU_BIG_ENDIAN:=y

ifdef TPATH_KC
KCC_PATH:=$(TPATH_KC)
else
KCC_PATH:=
endif
KCC = mips-linux-gcc
KLD = mips-linux-ld
#KCFLAGS = -D__KERNEL__ -Wall -Wstrict-prototypes -Wno-trigraphs -O2 -fno-strict-aliasing -fno-common -fomit-frame-pointer -G 0 -mno-abicalls -fno-pic -pipe  -finline-limit=100000 -mabi=32 -Wa,--trap -DMODULE -mlong-calls -nostdinc -iwithprefix
