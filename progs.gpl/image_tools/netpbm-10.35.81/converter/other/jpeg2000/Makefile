ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/jpeg2000
VPATH=.:$(SRCDIR)/$(SUBDIR)

SUBDIRS = libjasper

include $(BUILDDIR)/Makefile.config

INCLUDES =
ifneq ($(JASPERHDR_DIR),NONE)
  INCLUDES += -I$(JASPERHDR_DIR)
endif


# INTERNAL_JASPERLIB must be relative to the current directory, because it
# may end up in MERGE_OBJECTS, which must be relative.
INTERNAL_JASPERLIB = libjasper/libjasper.a
INTERNAL_JASPERHDR_DIR = $(SRCDIR)/$(SUBDIR)/libjasper/include

ifeq ($(JASPERLIB),$(INTERNAL_JASPERLIB))
  ifeq ($(HAVE_INT64),Y)
    JASPERLIB_DEP = $(JASPERLIB)
    JASPERLIB_USE = $(JASPERLIB)
  else
    # He wants the internal library, but doesn't have a 64 bit compiler,
    # so we have no way to build it.  Ergo, he doesn't have a Jasper
    # library.
    JASPERLIB_USE = NONE
  endif
else
  # It's not our internal version; user's on his own to make sure it's built
  JASPERLIB_USE = $(JASPERLIB)
endif


ifneq ($(JASPERHDR_DIR),NONE)
  ifneq ($(JASPERLIB_USE),NONE)
    BINARIES = pamtojpeg2k jpeg2ktopam
  endif
endif


OBJECTS = $(BINARIES:%=%.o)
MERGE_OBJECTS = $(BINARIES:%=%.o2) 
ifeq ($(JASPERLIB),$(INTERNAL_JASPERLIB))
  # MERGE_OBJECTS contains relative paths, so $(INTERNAL_JASPERLIB) had better
  # be relative to the current relative to the current directory.
  MERGE_OBJECTS += $(JASPERLIB)
endif
MERGEBINARIES = $(BINARIES)

.PHONY: all
all: $(BINARIES)

include $(SRCDIR)/Makefile.common

LIBOPTS = $(shell $(LIBOPT) $(NETPBMLIB) $(JASPERLIB_USE))

$(BINARIES): %: %.o $(JASPERLIB_DEP) $(NETPBMLIB) $(LIBOPT)
	$(LD) -o $@ $< \
	  $(LIBOPTS) $(JASPERDEPLIBS) $(MATHLIB) $(RPATH) \
	  $(LDFLAGS) $(LDLIBS) $(LADD)

$(INTERNAL_JASPERLIB): $(BUILDDIR)/$(SUBDIR)/libjasper FORCE
	$(MAKE) -f $(SRCDIR)/$(SUBDIR)/libjasper/Makefile \
	   -C $(dir $@) $(notdir $@)

.PHONY: FORCE
FORCE:
