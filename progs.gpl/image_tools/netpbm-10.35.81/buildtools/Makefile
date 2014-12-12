ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = buildtools
VPATH = .:$(SRCDIR)/$(SUBDIR)
include $(BUILDDIR)/Makefile.config

MERGE_OBJECTS =

# These are programs that are used by the make files:
PROGS = libopt typegen endiangen

all: $(PROGS)

BINARIES =
SCRIPTS =

OMIT_BUILDTOOL_RULE = 1
include $(SRCDIR)/Makefile.common

ifdef DLLVER
STRIP_DLL_VERSION=-DDLLVERSTR="\"$(DLLVER)\""
endif

ifeq ($(LINKER_CAN_DO_EXPLICIT_LIBRARY),Y)
EXPLICIT=-DEXPLICIT
endif

libopt.o: libopt.c
	$(CC_FOR_BUILD) -c $(CFLAGS_FOR_BUILD) \
	  -DSHLIBPREFIXLIST="\"$(SHLIBPREFIXLIST)\"" \
	  $(STRIP_DLL_VERSION) $(EXPLICIT) $(CFLAGS_PERSONAL) $(CADD) \
	  -o $@ $<

typegen.o endiangen.o:%.o:%.c
	$(CC_FOR_BUILD) -c $(CFLAGS_FOR_BUILD) -o $@ $<

$(PROGS):%:%.o
	$(LD_FOR_BUILD) -o $@ $<

distclean clean: cleanlocal
.PHONY: cleanlocal
cleanlocal:
	rm -f $(PROGS)
