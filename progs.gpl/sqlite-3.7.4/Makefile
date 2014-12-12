##############################################################################
# Makefile for SQLite
# Created by Yi-hung <yi_hung@alphanetworks.com>
##############################################################################
.EXPORT_ALL_VARIABLES:
include ../arch.mk
include ../../path.mk
include ../../.config

VERBOSE := > /dev/null
MYTOP = $(shell pwd)
MYLIBS = sqlite
PKS = $(MYLIBS)
SOURCES = $(MYTOP)
BUILDDIR = build
ROOTFS = $(CURDIR)/rootfs
ORIGDIR = orig
###############################################################################################
define Banner
$(Q)echo -e "\033[32m$(1)\033[0m";
endef

define SHOWVALUE
$(Q)echo '$1 : $($1)'
endef

define CheckDir
$(Q)test -d $(1) || mkdir -p $(1)
endef

define RemoveFileOrDir
$(call Banner,Remove $(1))
$(Q)if [ -d $(1) ]; then rm -rf $(1); fi
$(Q)if [ -f $(1) ]; then rm -f $(1); fi
endef

define MakeTarget
#
$(Q)make $(1)_$(2)
endef

# 1: tarball 2: untar place 3: patch 4: real source place
define PrepareSource
$(Q)if [ ! -d "$(4)" ]; then \
tar axf $(1) -C $(2); \
if [ -f "$(3)" ]; then cd $(4) && cat $(3) | patch -p2; fi \
fi
endef

define DoConfig
$(Q)if [ ! -f "$(1)/Makefile" -o ! -f "$(1)/config.log" ]; then cd $(1) && $(1)/configure $(2) $(VERBOSE); fi
endef

define GenLibtool
$(Q)export PATH=$(TPATH_UC)/bin:$(PATH) && cd $(1) && ./ltconfig --no-verify --with-gnu-ld ltmain.sh $(HOST_TYPE)
endef

define BuildApplet
$(Q)make -C $(1) $(2) $(VERBOSE);
endef

# 1: orig 2: new 3: patch file
define MKDIFF
$(Q)if test -f $(3); then cp -f $(3) $(3).bak; fi
$(CURDIR)/mkdiff "$(1)" "$(2)" "$(3).tmp"
$(CURDIR)/stripdate.pl < "$(3).tmp" > "$(3)"
$(Q)if test -f $(3).tmp; then rm -f $(3).tmp; fi
endef
###############################################################################################
all:
	$(call Banner,Build Libraries)
	$(Q)make build_libs

$(foreach mk,$(PKS),$(eval include ./Makefile.$(mk)))

build_libs:
	$(call CheckDir,$(BUILDDIR))
	$(foreach lib,$(MYLIBS),$(call MakeTarget,build,$(lib)))

clear_libs:
	$(foreach lib,$(MYLIBS),$(call MakeTarget,clear,$(lib)))

clean:
	$(call RemoveFileOrDir,$(BUILDDIR))
	$(call RemoveFileOrDir,$(ROOTFS))
	$(call RemoveFileOrDir,$(ORIGDIR))

strip_target:
	$(call Banner,Strip target)
	$(call RemoveFileOrDir,$(ROOTFS)/share)
	$(call RemoveFileOrDir,$(ROOTFS)/man)
	$(call RemoveFileOrDir,$(ROOTFS)/include)
	$(call RemoveFileOrDir,$(ROOTFS)/etc)
	$(call RemoveFileOrDir,$(ROOTFS)/lib/pkgconfig)
	$(call RemoveFileOrDir,$(ROOTFS)/lib/charset.alias)
	$(Q)rm -f $(ROOTFS)/lib/*.la
	$(Q)rm -f $(ROOTFS)/lib/*.a
	-$(Q)$(STRIP) $(ROOTFS)/bin/*
	$(Q)$(STRIP) $(ROOTFS)/lib/*

install: strip_target
	$(call Banner,Install target)
	cp -Rp $(ROOTFS)/* $(TARGET)/.

showinfo:
	$(call SHOWVALUE,CURDIR)
	$(call SHOWVALUE,CFLAGS)
	$(call SHOWVALUE,MYLIBS)
