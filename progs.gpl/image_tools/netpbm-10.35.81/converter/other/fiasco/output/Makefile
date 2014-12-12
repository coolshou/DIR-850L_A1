ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../../..
  BUILDDIR = $(SRCDIR)
endif
FIASCOSUBDIR = converter/other/fiasco
SUBDIR = $(FIASCOSUBDIR)/output
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

OBJECTS =  matrices.o mc.o nd.o tree.o weights.o write.o

MERGE_OBJECTS = $(OBJECTS)

INCLUDES = -I$(SRCDIR)/$(FIASCOSUBDIR) \
	   -I$(SRCDIR)/$(FIASCOSUBDIR)/lib \
	   -I$(SRCDIR)/$(FIASCOSUBDIR)/codec 

all: libfiasco_output.a

include $(SRCDIR)/Makefile.common

libfiasco_output.a: $(OBJECTS)
	$(AR) -rc $@ $(OBJECTS)
	$(RANLIB) $@

