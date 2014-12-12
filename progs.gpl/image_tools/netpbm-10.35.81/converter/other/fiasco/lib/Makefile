ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../../..
  BUILDDIR = $(SRCDIR)
endif
FIASCOSUBDIR = converter/other/fiasco
SUBDIR = $(FIASCOSUBDIR)/lib
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

OBJECTS = \
  arith.o \
  bit-io.o \
  dither.o \
  error.o \
  image.o \
  list.o \
  misc.o \
  rpf.o \
  mvcode.o \

MERGE_OBJECTS = $(OBJECTS)

INCLUDES = -I$(SRCDIR)/$(FIASCOSUBDIR)

all: libfiasco_lib.a

include $(SRCDIR)/Makefile.common

libfiasco_lib.a: $(OBJECTS)
	$(AR) -rc $@ $(OBJECTS)
	$(RANLIB) $@

