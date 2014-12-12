ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = lib/util
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

INCLUDES = -I $(BUILDDIR) -I $(SRCDIR)/$(SUBDIR)/..

# nstring is required for asprintf(), etc.  Also some systems don't have
# snprintf(), e.g. Solaris 2.5.1.  2002.03.29.
UTILOBJECTS = shhopt.o nstring.o filename.o

MERGE_OBJECTS =

all: $(UTILOBJECTS)

include $(SRCDIR)/Makefile.common

$(UTILOBJECTS):%.o:%.c importinc
	$(CC) -c $(INCLUDES) -DNDEBUG $(CFLAGS) $(CFLAGS_SHLIB) \
	  $(CFLAGS_PERSONAL) $(CADD) -o $@ $<

testnstring: test.c nstring.h nstring.o
	$(CC) $(CFLAGS) $(CADD) -o $@ nstring.o $<

include Makefile.depend
