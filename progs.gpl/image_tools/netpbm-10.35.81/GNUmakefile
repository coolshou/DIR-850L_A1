# Makefile for Netpbm
 
# Configuration should normally be done in the included file Makefile.config.

# Targets in this file:
#
#   nonmerge:     Build everything, in the source directory.
#   merge:        Build everything as merged executables, in the source dir
#   package:      Make a package of Netpbm files ready to install
#   
#   The default target is either "merge" or "nonmerge", as determined by
#   the DEFAULT_TARGET variable set by Makefile.config.

# About the "merge" target: Normally the Makefiles build separate
# executables for each program.  However, on some systems (especially
# those without shared libraries) this can mean a lot of space.  In
# this case you might try building a "merge" instead.  The idea here
# is to link all the programs together into one huge executable, along
# with a tiny dispatch program that runs one of the programs based on
# the command name with which it was invoked.  You install the merged
# executable with a file system link for the name of each program it
# includes.  This is much more important when you're statically
# linking than when you're using shared libraries.  On a Sun3 under
# SunOS 3.5, where shared libraries are not available, the space for
# executables went from 2970K to 370K in an older Netpbm.  On a
# GNU/Linux IA32 system with shared libraries in 2002, it went from
# 2949K to 1663K.

# To build a "merge" system, just set DEFAULT_TARGET to "merge" instead
# of "nomerge" in Makefile.config.  In that case, you should probably also
# set NETPBMLIBTYPE to "unixstatic", since a shared library doesn't do you 
# much good.

# The CURDIR variable presents a problem because it was introduced in
# GNU Make 3.77.  We need the CURDIR variable in order for our 'make
# -C xxx -f xxx' commands to work.  If we used the obvious alternative
# ".", that wouldn't work because it would refer to the directory
# named in -C, not the directory the make file you are reading is
# running in.  The -f option is necessary in order to have separate
# source and object directories.

ifeq ($(CURDIR)x,x)
all package install:
	@echo "YOU NEED AT LEAST VERSION 3.77 OF GNU MAKE TO BUILD NETPBM."
	@echo "Netpbm's makefiles need the CURDIR variable that was "
	@echo "introduced in 3.77.  Your version does not have CURDIR."
	@echo
	@echo "You can get a current GNU Make via http://www.gnu.org/software"
	@echo 
	@echo "If upgrading is impossible, try modifying GNUMakefile and "
	@echo "Makefile.common to replace \$(CURDIR) with \$(shell /bin/pwd) "
else


include Makefile.srcdir
BUILDDIR = $(CURDIR)
SUBDIR = 
VPATH=.:$(SRCDIR)

include $(BUILDDIR)/Makefile.config

PROG_SUBDIRS = converter analyzer editor generator other
PRODUCT_SUBDIRS = lib $(PROG_SUBDIRS)
SUPPORT_SUBDIRS = urt buildtools

SUBDIRS = $(PRODUCT_SUBDIRS) $(SUPPORT_SUBDIRS)

SCRIPTS = manweb
MANUALS1 = netpbm
NOMERGEBINARIES = netpbm

OBJECTS = netpbm.o

default: $(DEFAULT_TARGET)
	echo "EXISTENCE OF THIS FILE MEANS NETPBM HAS BEEN BUILT." \
	  >build_complete
	@echo ""
	@echo "Netpbm is built.  The next step is normally to package it "
	@echo "for installation by running "
	@echo ""
	@echo "    make package pkgdir=DIR"
	@echo ""
	@echo "to copy all the Netpbm files you need to install into the "
	@echo "directory DIR.  Then you can proceed to install."

all: nonmerge

.PHONY: nonmerge
nonmerge: $(PRODUCT_SUBDIRS:%=%/all)

# Parallel make (make --jobs) is not smart enough to coordinate builds
# between submakes, so a naive parallel make would cause certain
# targets to get built multiple times simultaneously.  That is usually
# unacceptable.  So we introduce extra dependencies here just to make
# sure such targets are already up to date before the submake starts,
# for the benefit of parallel make.  Note that we ensure that parallel
# make works for 'make all' in the top directory, but it may still fail
# for the aforementioned reason for other invocations.

$(SUBDIRS:%=%/all): pm_config.h inttypes_netpbm.h version.h
$(PROG_SUBDIRS:%=%/all): lib/all $(SUPPORT_SUBDIRS:%=%/all)

OMIT_CONFIG_RULE = 1
OMIT_VERSION_H_RULE = 1
OMIT_INTTYPES_RULE = 1
include $(SRCDIR)/Makefile.common

$(BUILDDIR)/Makefile.config: $(SRCDIR)/Makefile.config.in
	$(SRCDIR)/configure $(SRCDIR)/Makefile.config.in


# typegen is a utility program used by the make file below.
TYPEGEN = $(BUILDDIR)/buildtools/typegen

# endiangen is a utility program used by the make file below.
ENDIANGEN = $(BUILDDIR)/buildtools/endiangen

$(TYPEGEN) $(ENDIANGEN): $(BUILDDIR)/buildtools
	$(MAKE) -C $(dir $@) -f $(SRCDIR)/buildtools/Makefile \
	    SRCDIR=$(SRCDIR) BUILDDIR=$(BUILDDIR) $(notdir $@) 

DELETEIT = (rm -f $@ || false)

inttypes_netpbm.h: $(TYPEGEN)
	$(TYPEGEN) >$@ || $(DELETEIT)

# We run a couple of programs on the build machine in computing the
# contents of pm_config.h.  We need to give the user a way not to do
# that or to override the results, because it doesn't work if he's
# cross compiling.

pm_config.h: \
  $(SRCDIR)/pm_config.in.h Makefile.config inttypes_netpbm.h $(ENDIANGEN)
	echo '/* pm_config.h GENERATED BY A MAKE RULE */' >$@ || $(DELETEIT)
	echo '#ifndef PM_CONFIG_H' >>$@ || $(DELETEIT)
	echo '#define PM_CONFIG_H' >>$@ || $(DELETEIT)
ifeq ($(INTTYPES_H)x,x)
	echo '/* Don't need to #include any inttypes.h-type thing */
else
  ifeq ($(INTTYPES_H),"inttypes_netpbm.h")
	cat inttypes_netpbm.h >>$@ || $(DELETEIT)
  else
	echo '#include $(INTTYPES_H)' >>$@ || $(DELETEIT)
  endif
endif
ifeq ($(HAVE_INT64),Y)
	echo "#define HAVE_INT64 1" >>$@ || $(DELETEIT)
else
	echo "#define HAVE_INT64 0" >>$@ || $(DELETEIT)
endif	
	echo '/* pm_config.h.in FOLLOWS ... */' >>$@ || $(DELETEIT)
	cat $(SRCDIR)/pm_config.in.h >>$@ || $(DELETEIT)
	$(ENDIANGEN) >>$@ || $(DELETEIT)
	echo '#endif' >>$@ || $(DELETEIT)


MAJOR := $(NETPBM_MAJOR_RELEASE)
MINOR := $(NETPBM_MINOR_RELEASE)
POINT := $(NETPBM_POINT_RELEASE)
version.h:
	@rm -f $@
	@echo "/* Generated by make file rule */" >$@
	@echo "#define NETPBM_VERSION" \
	  \"Netpbm $(MAJOR).$(MINOR).$(POINT)"\"" >$@


.PHONY: install
install:
	@echo "After doing a 'make', do "
	@echo ""
	@echo "  make package pkgdir=DIR"
	@echo ""
	@echo "to copy all the Netpbm files you need to install into the "
	@echo "directory DIR."
	@echo ""
	@echo "Then, do "
	@echo ""
	@echo "  ./installnetpbm"
	@echo
	@echo "to install from there to your system via an interactive.  "
	@echo "dialog.  Or do it manually using simple copy commands and "
	@echo "following instructions in the file DIR/README"

.PHONY: package package_build init_package advise_installnetpbm
package: build_complete package_build advise_installnetpbm

build_complete:
# The regular build creates this file as its last act, so if it doesn't exist,
# that means either the user skipping the build step, or the build failed.
	@echo "You must build Netpbm before you can package Netpbm. "
	@echo "The usual way to do this is to type 'make' with no arguments."
	@echo "If you did that, then the build apparently failed.  There "
	@echo "should have been error messages indicating why.  If you "
	@echo "can't fix the build problem, you can do 'make --keep-going' "
	@echo "to force the build to continue with other parts that "
	@echo "it may be able to build successfully, then do "
	@echo "'make package --keep-going' to package whatever was "
	@echo "successfully built."
	@echo
	@false;

package_build: init_package install-run install-dev 

MAJOR=$(NETPBM_MAJOR_RELEASE)
MINOR=$(NETPBM_MINOR_RELEASE)
POINT=$(NETPBM_POINT_RELEASE)

init_package:
	@if [ -d $(PKGDIR) ]; then \
	  echo "Directory $(PKGDIR) already exists.  Please specify a "; \
	  echo "directory that can be created fresh, like this: "; \
	  echo "  make package PKGDIR=/tmp/newnetpbm "; \
	  false; \
	  fi
	mkdir $(PKGDIR)
	echo "Netpbm install package made by 'make package'" \
	    >$(PKGDIR)/pkginfo
	date >>$(PKGDIR)/pkginfo
	echo Netpbm $(MAJOR).$(MINOR).$(POINT) >$(PKGDIR)/VERSION
	$(INSTALL) -c -m 664 $(SRCDIR)/buildtools/README.pkg $(PKGDIR)/README
	$(INSTALL) -c -m 664 $(SRCDIR)/buildtools/config_template \
	  $(PKGDIR)/config_template

advise_installnetpbm:
	@echo
	@echo "Netpbm has been successfully packaged under directory"
	@echo "$(PKGDIR).  Run 'installnetpbm' to install it on your system."

.PHONY: install-run
ifeq ($(DEFAULT_TARGET),merge)
install-run: install-merge
else
install-run: install-nonmerge 
endif

.PHONY: install-merge install-nonmerge
install-merge: install.merge install.lib install.data \
	install.manweb install.man

install-nonmerge: install.bin install.lib install.data \
	install.manweb install.man

.PHONY: merge
merge: lib/all netpbm

MERGELIBS = 
ifneq ($(ZLIB),NONE)
  MERGELIBS += $(ZLIB)
endif
ifneq ($(JPEGLIB),NONE)
  MERGELIBS += $(JPEGLIB)
endif
ifneq ($(TIFFLIB),NONE)
  MERGELIBS += $(TIFFLIB)
endif
ifneq ($(URTLIB),NONE)
  MERGELIBS += $(URTLIB)
endif
ifneq ($(LINUXSVGALIB),NONE)
  MERGELIBS += $(LINUXSVGALIB)
endif
ifneq ($(X11LIB),NONE)
  MERGELIBS += $(X11LIB)
endif

ifeq ($(shell libpng-config --version),)
  PNGLD = $(shell $(LIBOPT) $(LIBOPTR) $(PNGLIB) $(ZLIB))
else
  PNGLD = $(shell libpng-config --ldflags)
endif

ifeq ($(shell xml2-config --version),)
  XML2LD=
else
  XML2LD=$(shell xml2-config --libs)
endif


# If URTLIB is BUNDLED_URTLIB, then we're responsible for building it, which
# means it needs to be a dependency:
ifeq ($(URTLIB),$(BUNDLED_URTLIB))
  URTLIBDEP = $(URTLIB)
endif

# We have two different ways to do the merge build:
#  
#   1) Each directory produces an object file merge.o containing all the code
#      in that directory and its descendants that needs to go into the 'netpbm'
#      program.  The make files do this recursively, via a link command that
#      combines multiple relocateable object files into one.  All we do here
#      at the top level is make merge.o and link it with netpbm.o and the
#      libraries.
#
#      This is the clean way, and we use it whenever we can.  But we don't
#      know how to do the link on every platform.
#
#   2) Each directory produces a list of all the object files in that 
#      directory and its descendants that need to go into the 'netpbm'
#      program.  This list is in a file called 'mergelist'.  The make files
#      do this recursively.  Here at the top level, we make mergelist and
#      then do one large link of everything listed in it, plus netpbm.o and
#      the libraries.
#
#      This doesn't require any special link command like (1), but is
#      not very clean.  The dependencies don't work right.  And at least
#      one linker (on DJGPP) can't handle that many input files.

ifeq ($(LDRELOC),NONE)
  OBJECT_DEP = mergelist
  OBJECT_LIST = `cat mergelist`
else
  OBJECT_DEP = merge.o
  OBJECT_LIST = merge.o
endif

netpbm:%:%.o $(OBJECT_DEP) $(NETPBMLIB) $(URTLIBDEP) $(LIBOPT)
# Note that LDFLAGS might contain -L options, so order is important.
	$(LD) -o $@ $< $(OBJECT_LIST) \
          $(LDFLAGS) $(shell $(LIBOPT) $(NETPBMLIB) $(MERGELIBS)) \
	  $(PNGLD) $(XML2LD) $(MATHLIB) $(NETWORKLD) $(LADD)

netpbm.o: mergetrylist

install.merge: local.install.merge
.PHONY: local.install.merge
local.install.merge:
	cd $(PKGDIR)/bin; $(SYMLINKEXE) netpbm pnmnoraw
	cd $(PKGDIR)/bin; $(SYMLINKEXE) netpbm gemtopbm
	cd $(PKGDIR)/bin; $(SYMLINKEXE) netpbm pnminterp
	cd $(PKGDIR)/bin; $(SYMLINKEXE) netpbm pgmoil
	cd $(PKGDIR)/bin; $(SYMLINKEXE) netpbm ppmtojpeg
	cd $(PKGDIR)/bin; $(SYMLINKEXE) netpbm bmptoppm
	cd $(PKGDIR)/bin; $(SYMLINKEXE) netpbm pgmnorm
	cd $(PKGDIR)/bin; $(SYMLINKEXE) netpbm pnmfile

ifneq ($(NETPBMLIBTYPE),unixstatic)
install.lib: lib/install.lib
else
install.lib:
endif

.PHONY: install.manweb
install.manweb: $(PKGDIR)/man/web/netpbm.url $(PKGDIR)/bin/doc.url

$(PKGDIR)/man/web/netpbm.url: $(PKGDIR)/man/web
	echo "$(NETPBM_DOCURL)" > $@
	chmod $(INSTALL_PERM_MAN) $@

$(PKGDIR)/bin/doc.url: $(PKGDIR)/bin
	echo "$(NETPBM_DOCURL)" > $@
	chmod $(INSTALL_PERM_MAN) $@

.PHONY: install-dev
# Note that you might install the development package and NOT the runtime
# package.  If you have a special system for building stuff, maybe for 
# multiple platforms, that's what you'd do.  Ergo, install.lib is here even
# though it is also part of the runtime install.
install-dev: install.hdr install.staticlib install.lib install.sharedlibstub

.PHONY: install.hdr
install.hdr: $(PKGDIR)/include
	$(MAKE) -C lib -f $(SRCDIR)/lib/Makefile \
	    SRCDIR=$(SRCDIR) BUILDDIR=$(BUILDDIR) install.hdr
	$(INSTALL) -c -m $(INSTALL_PERM_HDR) \
	    $(BUILDDIR)/pm_config.h $(PKGDIR)/include

ifeq ($(STATICLIB_TOO),y)
BUILD_STATIC = y
else
  ifeq ($(NETPBMLIBTYPE),unixstatic)
    BUILD_STATIC = y
  else
    BUILD_STATIC = n
  endif
endif

.PHONY: install.staticlib
install.staticlib: 
ifeq ($(BUILD_STATIC),y)
	$(MAKE) -C lib -f $(SRCDIR)/lib/Makefile \
	SRCDIR=$(SRCDIR) BUILDDIR=$(BUILDDIR) install.staticlib 
endif

.PHONY: install.sharedlibstub
install.sharedlibstub:
	$(MAKE) -C lib -f $(SRCDIR)/lib/Makefile \
	    SRCDIR=$(SRCDIR) BUILDDIR=$(BUILDDIR) install.sharedlibstub 

clean: localclean

.PHONY: localclean
localclean:
	rm -f netpbm build_started build_complete
	rm -f pm_config.h inttypes_netpbm.h version.h

# Note that removing Makefile.config must be the last thing we do,
# because no other makes will work after that is done.
distclean: localdistclean
.PHONY: localdistclean
localdistclean: localclean
	-rm -f `find -type l`
	-rm -f Makefile.config

# The following endif is for the else block that contains virtually the
# whole file, for the test of the existence of CURDIR.
endif
