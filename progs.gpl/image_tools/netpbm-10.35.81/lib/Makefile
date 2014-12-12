ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = lib
VPATH=.:$(SRCDIR)/$(SUBDIR)
DLLTOOL=dlltool
include $(BUILDDIR)/Makefile.config

ifeq ($(NETPBMLIBTYPE),unixstatic)
LIBNETPBM = libnetpbm.$(STATICLIBSUFFIX)
else
LIBNETPBM = $(NETPBMSHLIBPREFIX)netpbm$(DLLVER).$(NETPBMLIBSUFFIX)
endif

ifeq ($(STATICLIB_TOO),y)
EXTRA_STATICLIB = libnetpbm.$(STATICLIBSUFFIX)
else
EXTRA_STATICLIB =
endif

ifeq ($(DONT_HAVE_PROCESS_MGMT),Y)
  LIBSYSTEM = libsystem_dummy.o
else
  LIBSYSTEM = libsystem.o
endif

LIBOBJECTS = libpm.o fileio.o bitio.o colorname.o \
	libpbm1.o libpbm2.o libpbm3.o libpbmfont.o \
	libpgm1.o libpgm2.o \
	libppm1.o libppm2.o libppmcmap.o libppmcolor.o libppmfuzzy.o \
	libppmd.o ppmdfont.o standardppmdfont.o path.o \
	libppmfloyd.o \
	libpnm1.o libpnm2.o libpnm3.o \
	libpam.o libpamread.o libpamwrite.o \
	libpamn.o libpammap.o libpamcolor.o \
	$(LIBSYSTEM) \

ifneq (${VMS}x,x)
LIBOBJECTS += libpbmvms.o
endif
# Library objects to be linked but not built by Makefile.common:
LIBOBJECTS_X = util/shhopt.o util/nstring.o util/filename.o

MANUALS3 = libnetpbm
MANUALS5 = pbm pgm ppm pnm pam

INTERFACE_HEADERS =  pm.h pbm.h bitio.h pbmfont.h \
	pgm.h ppm.h ppm.h ppmcmap.h ppmfloyd.h colorname.h \
	pnm.h pam.h pammap.h util/shhopt.h util/nstring.h util/mallocvar.h \
	pm_system.h pm_gamma.h

DATAFILES = rgb.txt

.PHONY: all
all: libnetpbm extra_staticlib

INCLUDES = -I$(SRCDIR)/$(SUBDIR) -I. -Iimportinc

SUBDIRS = util
SCRIPTS = 
BINARIES = 

OMIT_LIBRARY_RULE = 1
include $(SRCDIR)/Makefile.common

# The following must go after Makefile.common because $(LIBNETPBM) may 
# contain a reference to $(NETPBM_MAJOR_RELEASE).
.PHONY: libnetpbm
libnetpbm: $(LIBNETPBM)

.PHONY: extra_staticlib
extra_staticlib: $(EXTRA_STATICLIB)

#----------------------------------------------------------------------------
# Following are rules for building shared libraries.
# Note that the user may specify a shared library as his "main" library
# type, but request a static library in addition.
#----------------------------------------------------------------------------

$(LIBOBJECTS): %.o: %.c importinc
# Note that the user may have configured -I options into CFLAGS.
	$(CC) -c $(INCLUDES) -DNDEBUG $(CFLAGS) $(CFLAGS_SHLIB) \
	  $(CFLAGS_PERSONAL) $(CADD) -o $@ $<

MAJ = $(NETPBM_MAJOR_RELEASE)
MIN = $(NETPBM_MINOR_RELEASE)

SONAME = libnetpbm.$(NETPBMLIBSUFFIX).$(MAJ)

ifeq ($(NETPBMLIBTYPE),irixshared)
# The libxxx.so link is needed to link the executables.
libnetpbm.$(NETPBMLIBSUFFIX): $(SONAME)
	rm -f $@
	$(SYMLINK) $< $@

PERLPROG = print "sgi$(MAJ)." . join(":sgi$(MAJ) . ", (0..$(MIN))) . "\n"

$(SONAME): \
    $(LIBOBJECTS) $(LIBOBJECTS_X) 
	$(LD) $(LDSHLIB) -o $@ $(LIBOBJECTS) $(LIBOBJECTS_X) \
          -lc \
	  -soname libnetpbm.$(NETPBMLIBSUFFIX) \
	  -set_version $(shell perl -e '$(PERLPROG)') \
	  $(LADD)
endif

ifeq ($(NETPBMLIBTYPE),unixshared)
# The libxxx.so link is needed to link the executables.
libnetpbm.$(NETPBMLIBSUFFIX): $(SONAME)
	rm -f $@
	$(SYMLINK) $< $@
# The $(SONAME) link is needed only to test the programs without
# installing the libraries (in that case, you also need to direct the 
# dynamic linker to the source directories, e.g. set LD_LIBRARY_PATH).
$(SONAME): libnetpbm.$(NETPBMLIBSUFFIX).$(MAJ).$(MIN)
	rm -f $@
	$(SYMLINK) $< $@
libnetpbm.$(NETPBMLIBSUFFIX).$(MAJ).$(MIN): $(LIBOBJECTS) $(LIBOBJECTS_X)
	$(LD) $(LDSHLIB) -o $@ $(LIBOBJECTS) $(LIBOBJECTS_X) \
          $(SHLIB_CLIB) -lm $(LADD)
endif

ifeq ($(NETPBMLIBTYPE),dll)
ifeq ($(STATICLIB_TOO),y)
$(NETPBMSHLIBPREFIX)netpbm$(DLLVER).dll: $(LIBOBJECTS) $(LIBOBJECTS_X) libnetpbm.$(STATICLIBSUFFIX)
else
$(NETPBMSHLIBPREFIX)netpbm$(DLLVER).dll: $(LIBOBJECTS) $(LIBOBJECTS_X)
endif
	$(LD) $(LDSHLIB) -Wl,--export-all-symbols \
            -Wl,-soname,$(NETPBMSHLIBPREFIX)netpbm$(DLLVER).dll \
	    -Wl,--output-def,$(NETPBMSHLIBPREFIX)netpbm$(DLLVER).def \
            -Wl,--out-implib,libnetpbm.dll.a -o $@ $(LDFLAGS) \
            $(LIBOBJECTS) $(LIBOBJECTS_X) $(LDLIBS) $(LADD) 
endif

ifeq ($(NETPBMLIBTYPE),dylib)
libnetpbm.dylib: libnetpbm.$(MAJ).dylib
	rm -f $@
	$(SYMLINK) $< $@

libnetpbm.$(MAJ).dylib: libnetpbm.$(MAJ).$(MIN).dylib
	rm -f $@
	$(SYMLINK) $< $@

libnetpbm.$(MAJ).$(MIN).dylib: $(LIBOBJECTS) $(LIBOBJECTS_X) 
	$(LD) $(LDSHLIB) -o $@ $(LIBOBJECTS) $(LIBOBJECTS_X) \
           -lc $(LADD)
endif

#--------------------------------------------------------------------------
# The rule for building a static library is below (if needed).  This is
# tricky because the user can be building the static library as his main
# library or in addition to some other kind of main library.  In fact,
# he may have specified it both as the main library type and an 
# additional library type.  In that case, NETPBMLIBSUFFIX and 
# STATICLIBSUFFIX are redundant -- we hope they're the same.
# 
# We must not include a rule for static libraries if he doesn't want us
# to build any.  The library name we come up with might conflict with 
# the name of the library he actually is building.  In fact, in that case
# STATICLIB_SUFFIX may just be arbitrary.
#-----------------------------------------------------------------------------
ifeq ($(NETPBMLIBTYPE),unixstatic)
  BUILD_STATICLIB = y
else
  ifeq ($(STATICLIB_TOO),y)
    BUILD_STATICLIB = y
  else
    BUILD_STATICLIB = n
  endif
endif

ifeq ($(BUILD_STATICLIB),y)
libnetpbm.$(STATICLIBSUFFIX): $(LIBOBJECTS) $(LIBOBJECTS_X)
	-rm -f $@
	$(AR) rc $@ $(LIBOBJECTS) $(LIBOBJECTS_X)
	-$(RANLIB) $@
endif


# To avoid major hassles with having ppmdcfont available here, we just
# ship a pre-made standardppmfont.c, so this rule will not normally be
# used.
standardppmdfont.c:standard.ppmdfont
	ppmdcfont <$< >$@ || (rm $@ && false)

# Note that we create a new compile.h only for the first make after a
# make clean.  This is good enough.  We used to do stamp-date for
# every build of "all" from the Netpbm top directory, but nowhere
# else, so it was really sloppy.

compile.h:
	$(SRCDIR)/buildtools/stamp-date >$@ || rm $@

$(LIBOBJECTS_X): FORCE
	@if [ ! -d $(dir $@) ] ; then mkdir $(dir $@) ; fi
	$(MAKE) -C $(dir $@) -f $(SRCDIR)/$(SUBDIR)/$(dir $@)Makefile \
		SRCDIR=$(SRCDIR) BUILDDIR=$(BUILDDIR) $(notdir $@) 

libpm.o: compile.h

# Install a shared library
#
.PHONY: install.lib
ifeq ($(NETPBMLIBTYPE),unixshared)
# install a Unix-style shared library
install.lib: $(PKGDIR)/lib $(PKGDIR)/link
	cd $(PKGDIR)/lib ; rm -f libnetpbm.$(NETPBMLIBSUFFIX).$(MAJ).*
	$(INSTALL) -c -m $(INSTALL_PERM_LIBD) \
	  libnetpbm.$(NETPBMLIBSUFFIX).$(MAJ).$(MIN)  $(PKGDIR)/lib/
	cd $(PKGDIR)/lib/ ; \
          rm -f libnetpbm.$(NETPBMLIBSUFFIX).$(MAJ); \
          $(SYMLINK) libnetpbm.$(NETPBMLIBSUFFIX).$(MAJ).$(MIN) $(SONAME)
endif
ifeq ($(NETPBMLIBTYPE),dll)
#install a Windows DLL shared library
#Note that unlike Unix libraries, a Windows DLL must go into a directory
#that is in the PATH, so we use bin/ instead of lib/
install.lib: $(PKGDIR)/bin
	( cd $(PKGDIR)/bin ; rm -f $(NETPBMSHLIBPREFIX)netpbm$(DLLVER).dll )
	$(INSTALL) -c $(STRIPFLAG) -m $(INSTALL_PERM_LIBD) \
          $(NETPBMSHLIBPREFIX)netpbm$(DLLVER).dll $(PKGDIR)/bin/
endif
ifeq ($(NETPBMLIBTYPE),dylib)
# install a Darwin-style shared library
install.lib: $(PKGDIR)/lib
	cd $(PKGDIR)/lib ; rm -f libnetpbm.*.dylib
	$(INSTALL) -c -m $(INSTALL_PERM_LIBD) libnetpbm.$(MAJ).$(MIN).dylib \
	   $(PKGDIR)/lib
	cd $(PKGDIR)/lib ; \
          rm -f libnetpbm.$(MAJ).dylib; \
          $(SYMLINK) libnetpbm.$(MAJ).$(MIN).dylib libnetpbm.$(MAJ).dylib
endif

.PHONY: install.hdr
install.hdr: $(INTERFACE_HEADERS:%=%_installhdr)
# You need to install the interface header files only if you are going to
# compile programs that use the Netpbm libraries.  Alternatively, you may
# prefer not to "install" them, but just to access the Netpbm source
# directory when you compile your programs.

%_installhdr: $(PKGDIR)/include
	$(INSTALL) -c -m $(INSTALL_PERM_HDR) \
	  $(SRCDIR)/lib/$(@:%_installhdr=%) $(PKGDIR)/include/;

.PHONY: install.staticlib
install.staticlib: $(PKGDIR)/link
	$(INSTALL) -c -m $(INSTALL_PERM_LIBS) libnetpbm.$(STATICLIBSUFFIX) \
	  $(PKGDIR)/link

# Install a shared library stub -- the ".so" file used at link time to
# prepare a program for dynamically linking a library at run time 
.PHONY: install.sharedlibstub
install.sharedlibstub: $(PKGDIR)/link
ifeq ($(NETPBMLIBTYPE),unixshared)
# install the link-time (.so) links to the runtime libraries
	cd $(PKGDIR)/link ; \
          rm -f libnetpbm.$(NETPBMLIBSUFFIX); \
          $(SYMLINK) ../lib/libnetpbm.$(NETPBMLIBSUFFIX).$(MAJ) \
            libnetpbm.$(NETPBMLIBSUFFIX)
endif
ifeq ($(NETPBMLIBTYPE),dll)
	$(INSTALL) -c -m $(INSTALL_PERM_LIBS) libnetpbm.dll.a $(PKGDIR)/link
endif
ifeq ($(NETPBMLIBTYPE),dylib)
	cd $(PKGDIR)/link/ ; \
          rm -f libnetpbm.dylib; \
	$(SYMLINK) ../lib/libnetpbm.$(MAJ).$(MIN).dylib libnetpbm.dylib
endif

clean: localclean

.PHONY: localclean
localclean:
	rm -f compile.h

FORCE:
