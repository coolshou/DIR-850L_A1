ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/jpeg2000/libjasper/jp2
VPATH=.:$(SRCDIR)/$(SUBDIR)

JASPERSRCDIR=$(SRCDIR)/$(SUBDIR)/..

include $(BUILDDIR)/Makefile.config

LIB_OBJECTS = jp2_cod.o jp2_dec.o jp2_enc.o

MERGE_OBJECTS =

all: partlist $(LIB_OBJECTS)

include $(JASPERSRCDIR)/Makefile.common

