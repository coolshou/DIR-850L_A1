ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/fiasco
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

INCLUDES = \
	-I$(SRCDIR)/$(SUBDIR)/codec -I$(SRCDIR)/$(SUBDIR)/input \
	-I$(SRCDIR)/$(SUBDIR)/output -I$(SRCDIR)/$(SUBDIR)/lib \

BINARIES = pnmtofiasco fiascotopnm

MERGEBINARIES = $(BINARIES)

SCRIPTS =

all: $(BINARIES)

FIASCOLIBS = codec/libfiasco_codec.a \
	     input/libfiasco_input.a \
	     output/libfiasco_output.a \
	     lib/libfiasco_lib.a 

COMMON_OBJECTS = binerror.o getopt.o getopt1.o params.o

OBJECTS = $(BINARIES:%=%.o) $(COMMON_OBJECTS)

MERGE_OBJECTS = $(BINARIES:%=%.o2) $(COMMON_OBJECTS)  $(FIASCOLIBS)

SUBDIRS = codec input output lib

include $(SRCDIR)/Makefile.common

$(BINARIES):%:%.o $(COMMON_OBJECTS) $(FIASCOLIBS) $(NETPBMLIB) \
   $(LIBOPT)
	$(LD) -o $@ $< $(COMMON_OBJECTS) \
	$(shell $(LIBOPT) $(FIASCOLIBS) $(NETPBMLIB)) $(MATHLIB) \
	$(LDFLAGS) $(LDLIBS) $(RPATH) $(LADD)

codec/libfiasco_codec.a: $(BUILDDIR)/$(SUBDIR)/codec FORCE
	$(MAKE) -C codec -f $(SRCDIR)/$(SUBDIR)/codec/Makefile \
		libfiasco_codec.a SRCDIR=$(SRCDIR) BUILDDIR=$(BUILDDIR)

input/libfiasco_input.a: $(BUILDDIR)/$(SUBDIR)/input FORCE
	$(MAKE) -C input -f $(SRCDIR)/$(SUBDIR)/input/Makefile \
		libfiasco_input.a SRCDIR=$(SRCDIR) BUILDDIR=$(BUILDDIR)

output/libfiasco_output.a: $(BUILDDIR)/$(SUBDIR)/output FORCE
	$(MAKE) -C output -f $(SRCDIR)/$(SUBDIR)/output/Makefile \
		libfiasco_output.a SRCDIR=$(SRCDIR) BUILDDIR=$(BUILDDIR)

lib/libfiasco_lib.a: $(BUILDDIR)/$(SUBDIR)/lib FORCE
	$(MAKE) -C lib -f $(SRCDIR)/$(SUBDIR)/lib/Makefile \
		libfiasco_lib.a SRCDIR=$(SRCDIR) BUILDDIR=$(BUILDDIR)


