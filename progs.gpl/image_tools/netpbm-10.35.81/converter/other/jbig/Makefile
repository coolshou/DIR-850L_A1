ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/jbig
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

LIBJBIG_OBJECTS = jbig.o jbig_tab.o

INCLUDES =
ifneq ($(JBIGHDR_DIR),NONE)
  INCLUDES += -I$(JBIGHDR_DIR)
endif

ifneq ($(JBIGHDR_DIR),NONE)
  ifneq ($(JBIGLIB),NONE)
    BINARIES = jbigtopnm pnmtojbig
  endif
endif

SCRIPTS =

ifeq ($(JBIGLIB),$(BUILDDIR)/$(SUBDIR)/libjbig.a)
  JBIGLIB_DEP = $(JBIGLIB)
else
  # It's not our internal version; user's on his own to make sure it's built
endif

OBJECTS = $(BINARIES:%=%.o) $(LIBJBIG_OBJECTS)
MERGE_OBJECTS = $(BINARIES:%=%.o2) $(LIBJBIG_OBJECTS)

all: $(BINARIES)

include $(SRCDIR)/Makefile.common

$(BINARIES): %: %.o $(JBIGLIB_DEP) $(NETPBMLIB) $(LIBOPT)
	$(LD) -o $@ $< \
	  $(shell $(LIBOPT) $(NETPBMLIB) $(JBIGLIB)) $(MATHLIB) \
	  $(LDFLAGS) $(LDLIBS) $(RPATH) $(LADD)


$(BUILDDIR)/$(SUBDIR)/libjbig.a: $(LIBJBIG_OBJECTS)
	$(AR) -rc $@ $^
	$(RANLIB) $@

