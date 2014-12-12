ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/jpeg2000/libjasper/base
VPATH=.:$(SRCDIR)/$(SUBDIR)

JASPERSRCDIR=$(SRCDIR)/$(SUBDIR)/..

include $(BUILDDIR)/Makefile.config

LIB_OBJECTS = jas_debug.o jas_getopt.o jas_image.o jas_init.o \
	      jas_malloc.o jas_seq.o jas_stream.o jas_string.o \
	      jas_tvp.o jas_version.o

MERGE_OBJECTS =

all: partlist $(LIB_OBJECTS)

include $(JASPERSRCDIR)/Makefile.common

