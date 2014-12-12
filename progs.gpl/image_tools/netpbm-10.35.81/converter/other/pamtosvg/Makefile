ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/pamtosvg
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

BINARIES = pamtosvg

PAMTOSVG_OBJECTS = \
	pamtosvg.o \
	output-svg.o \
	fit.o \
	spline.o \
	curve.o \
	vector.o \
	epsilon-equal.o \
	autotrace.o \
	pxl-outline.o \
	bitmap.o \
	thin-image.o \
	logreport.o \
	exception.o \
	image-proc.o \

MERGE_OBJECTS = \
	pamtosvg.o2 \
	output-svg.o \
	fit.o \
	spline.o \
	curve.o \
	vector.o \
	epsilon-equal.o \
	autotrace.o \
	pxl-outline.o \
	bitmap.o \
	thin-image.o \
	logreport.o \
	exception.o \
	image-proc.o \

OBJECTS = $(PAMTOSVG_OBJECTS)

MERGEBINARIES = $(BINARIES)

all: $(BINARIES)

include $(SRCDIR)/Makefile.common

pamtosvg: $(PAMTOSVG_OBJECTS) $(NETPBMLIB) $(LIBOPT)
	$(LD) -o $@ $(PAMTOSVG_OBJECTS) \
	  $(shell $(LIBOPT) $(NETPBMLIB)) \
	  $(MATHLIB) $(LDFLAGS) $(LDLIBS) $(RPATH) $(LADD)
