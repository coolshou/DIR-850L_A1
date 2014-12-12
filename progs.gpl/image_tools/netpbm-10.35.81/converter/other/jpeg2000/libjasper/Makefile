ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/jpeg2000/libjasper
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

SUBDIRS = base jp2 jpc
LIB_OBJECTS =
JASPERSRCDIR = $(SRCDIR)/$(SUBDIR)

# NOTE: The library source code uses 64 bit types, so you cannot
# build it (compile will fail) if you don't have 64 bit types defined
# by pm_config.h (normally by including <inttypes.h>)

all: libjasper.a

include $(SRCDIR)/$(SUBDIR)/Makefile.common

# We cheat a bit here -- the real dependencies are all the .o files listed
# in the part list, but since we don't know what those are, we just do a
# 'make all' in each subdirectory to get them built.  That means it always
# looks like dependencies were rebuilt and libjasper.a gets rebuilt every
# time.
libjasper.a: $(SUBDIRS:%=%/all) partlist 
	$(AR) rc $@ $(shell cat partlist)
	$(RANLIB) $@

