ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/cameratopam
VPATH=.:$(SRCDIR)/$(SUBDIR)

ifneq ($(JPEGLIB),NONE)
  ifneq ($(JPEGHDR_DIR)x,x)
    INCLUDES += -I$(JPEGHDR_DIR)
    CFLAGS += -DHAVE_JPEG
  endif
endif

include $(BUILDDIR)/Makefile.config


.PHONY: all
all: cameratopam

OBJECTS = util.o identify.o cameratopam.o camera.o foveon.o decode.o \
	canon.o ljpeg.o dng.o

MERGE_OBJECTS =

BINARIES = cameratopam
MERGEBINARIES = 
SCRIPTS = 

include $(SRCDIR)/Makefile.common

cameratopam: $(OBJECTS) $(NETPBMLIB) $(LIBOPT)
	$(LD) -o $@ \
          $(OBJECTS) $(shell $(LIBOPT) $(NETPBMLIB) $(LIBOPTR)) \
	  $(MATHLIB) $(LDFLAGS) $(LDLIBS) \
	  $(RPATH) $(LADD)

