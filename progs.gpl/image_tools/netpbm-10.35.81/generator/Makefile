ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = generator
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

# We tend to separate out the build targets so that we don't have
# any more dependencies for a given target than it really needs.
# That way, if there is a problem with a dependency, we can still
# successfully build all the stuff that doesn't depend upon it.
# This package is so big, it's useful even when some parts won't 
# build.

PORTBINARIES = pamgauss pamgradient pamseq pamstereogram \
	       pbmpage pbmmake pbmtext pbmtextps pbmupc \
	       pgmcrater pgmkernel pgmmake pgmnoise pgmramp \
	       ppmcie ppmcolors ppmforge ppmmake ppmpat ppmrough ppmwheel \

# We don't include programs that have special library dependencies in the
# merge scheme, because we don't want those dependencies to prevent us
# from building all the other programs.

NOMERGEBINARIES = 
MERGEBINARIES = $(PORTBINARIES)


BINARIES = $(MERGEBINARIES) $(NOMERGEBINARIES)
SCRIPTS = ppmrainbow

OBJECTS = $(BINARIES:%=%.o)

MERGE_OBJECTS = $(MERGEBINARIES:%=%.o2)

.PHONY: all
all: $(BINARIES)

include $(SRCDIR)/Makefile.common
