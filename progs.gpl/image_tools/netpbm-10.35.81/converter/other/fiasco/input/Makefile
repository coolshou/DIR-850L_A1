ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../../..
  BUILDDIR = $(SRCDIR)
endif
FIASCOSUBDIR = converter/other/fiasco
SUBDIR = $(FIASCOSUBDIR)/input
BUILDDIR = ../../../..
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

OBJECTS =  basis.o matrices.o mc.o nd.o read.o tree.o weights.o

MERGE_OBJECTS = $(OBJECTS)

INCLUDES = -I$(SRCDIR)/$(FIASCOSUBDIR) -I$(SRCDIR)/$(FIASCOSUBDIR)/lib \
	   -I$(SRCDIR)/$(FIASCOSUBDIR)/codec

all: libfiasco_input.a

include $(SRCDIR)/Makefile.common

libfiasco_input.a: $(OBJECTS)
	$(AR) -rc $@ $(OBJECTS)
	$(RANLIB) $@

