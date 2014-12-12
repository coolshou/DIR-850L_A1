ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = other
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

SUBDIRS = pamx

ifneq ($(LINUXSVGALIB),NONE)
  ifneq ($(LINUXSVGAHDR_DIR),)
    INCLUDES += -I$(LINUXSVGAHDR_DIR)
  endif
endif

# We tend to separate out the build targets so that we don't have
# any more dependencies for a given target than it really needs.
# That way, if there is a problem with a dependency, we can still
# successfully build all the stuff that doesn't depend upon it.
# This package is so big, it's useful even when some parts won't 
# build.

PORTBINARIES = pamarith pambayer pamchannel pamdepth \
	pamendian pamlookup pampick pamsplit \
	pamstack pamsummcol pnmcolormap \
	ppmdcfont ppmddumpfont ppmdmkfont 

BINARIES = $(PORTBINARIES)

ifneq ($(LINUXSVGALIB),NONE)
  BINARIES += ppmsvgalib
endif

SCRIPTS = ppmtomap

OBJECTS = $(BINARIES:%=%.o)

# We don't include programs that have special library dependencies in the
# merge scheme, because we don't want those dependencies to prevent us
# from building all the other programs.

MERGEBINARIES = $(BINARIES)
MERGE_OBJECTS = $(MERGEBINARIES:%=%.o2)

.PHONY: all
all: $(BINARIES) $(SUBDIRS:%=%/all)

include $(SRCDIR)/Makefile.common

ppmsvgalib: %: %.o $(NETPBMLIB) $(LIBOPT)
	$(LD) -o $@ $< \
	  $(shell $(LIBOPT) $(NETPBMLIB) $(LINUXSVGALIB)) \
	  $(MATHLIB) $(LDFLAGS) $(LDLIBS) $(LADD) 

install.bin: install.bin.local
.PHONY: install.bin.local
install.bin.local: $(PKGDIR)/bin
# Remember that $(SYMLINK) might just be a copy command.
# In July 2002, pamarith replaced pnmarith
	cd $(PKGDIR)/bin ; \
	  rm -f pnmarith ; \
	  $(SYMLINK) pamarith$(EXE) pnmarith
# In December 2005, pamsplit replaced pnmsplit
	cd $(PKGDIR)/bin ; \
	  rm -f pnmsplit ; \
	  $(SYMLINK) pamsplit$(EXE) pnmsplit
# In February 2006, pamdepth replaced pnmdepth
	cd $(PKGDIR)/bin ; \
	  rm -f pnmdepth ; \
	  $(SYMLINK) pamdepth$(EXE) pnmdepth

FORCE:
