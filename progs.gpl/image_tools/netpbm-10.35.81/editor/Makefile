ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = editor
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

# We tend to separate out the build targets so that we don't have
# any more dependencies for a given target than it really needs.
# That way, if there is a problem with a dependency, we can still
# successfully build all the stuff that doesn't depend upon it.
# This package is so big, it's useful even when some parts won't 
# build.

PORTBINARIES = pamaddnoise pamcomp pamcut \
	       pamdeinterlace pamdice pamditherbw pamedge \
	       pamenlarge \
	       pamflip pamfunc pammasksharpen pammixinterlace \
	       pamoil pamperspective pampop9 \
	       pamscale pamstretch pamthreshold \
	       pbmclean pbmlife pbmmask pbmpscale pbmreduce \
	       pgmabel pgmbentley pgmdeshadow pgmenhance \
	       pgmmedian pgmmorphconv \
	       pnmalias pnmcat pnmcomp pnmconvol pnmcrop pnmcut \
	       pnmgamma \
	       pnmhisteq pnmindex pnminvert pnmmontage \
	       pnmnlfilt pnmnorm pnmpad pnmpaste \
	       pnmremap pnmrotate \
	       pnmscale pnmscalefixed pnmshear pnmsmooth pnmstitch pnmtile \
	       ppm3d ppmbrighten ppmchange ppmcolormask \
	       ppmdim ppmdist ppmdither ppmdraw \
	       ppmflash ppmglobe ppmlabel ppmmix \
	       ppmntsc ppmrelief ppmshift ppmspread ppmtv

# We don't include programs that have special library dependencies in the
# merge scheme, because we don't want those dependencies to prevent us
# from building all the other programs.

NOMERGEBINARIES = 
MERGEBINARIES = $(PORTBINARIES)


BINARIES = $(MERGEBINARIES) $(NOMERGEBINARIES)
SCRIPTS = pnmflip ppmfade ppmquant ppmquantall ppmshadow \
	  pamstretch-gen pnmmargin pnmquant 

OBJECTS = $(BINARIES:%=%.o)

MERGE_OBJECTS = $(MERGEBINARIES:%=%.o2)

.PHONY: all
all: $(BINARIES)

include $(SRCDIR)/Makefile.common

install.bin: install.bin.local

.PHONY: install.bin.local
install.bin.local: $(PKGDIR)/bin
# Remember that $(SYMLINK) might just be a copy command.
# backward compatibility: program used to be named pnmnoraw
# backward compatibility: program used to be pnminterp
	cd $(PKGDIR)/bin ; \
	rm -f pnminterp; \
	$(SYMLINK) pamstretch$(EXE) pnminterp
# pamoil replaced pgmoil in June 2001.
	cd $(PKGDIR)/bin ; \
	rm -f pgmoil ; \
	$(SYMLINK) pamoil$(EXE) pgmoil
# In March 2002, pnmnorm replaced ppmnorm and pgmnorm
	cd $(PKGDIR)/bin ; \
	rm -f ppmnorm ; \
	$(SYMLINK) pnmnorm$(EXE) ppmnorm 
	cd $(PKGDIR)/bin ; \
	rm -f pgmnorm ; \
	$(SYMLINK) pnmnorm$(EXE) pgmnorm
# In March 2003, pamedge replaced pgmedge
	cd $(PKGDIR)/bin ; \
	rm -f pgmedge ; \
	$(SYMLINK) pamedge$(EXE) pgmedge
# In October 2004, pamenlarge replaced pnmenlarge
	cd $(PKGDIR)/bin ; \
	rm -f pnmenlarge ; \
	$(SYMLINK) pamenlarge$(EXE) pnmenlarge
