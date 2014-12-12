ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/ppm/hpcdtoppm
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

all: hpcdtoppm

SCRIPTS = hpcdtoppm pcdovtoppm
MERGE_OBJECTS =

include $(SRCDIR)/Makefile.common

install: install.bin.local
.PHONY: install.bin.local
install.bin.local: $(PKGDIR)/bin
# In June 2002, pcdovtoppm replaced pcdindex
	cd $(PKGDIR)/bin ; \
	$(SYMLINK) pcdindex$(EXE) pcdovtoppm


FORCE:
