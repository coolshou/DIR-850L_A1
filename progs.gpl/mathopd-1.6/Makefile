#
#	Makefile for mathopd <david_hsieh@alphanetworks.com>
#

-include ../../path.mk
-include ../arch.mk

####################################################################################

MYNAME		:= mathopd-1.6
CURRDIR		:= $(shell pwd)
BUILD		:= build
BUILDDIR	:= $(CURRDIR)/$(BUILD)
SRCDIR		:= $(CURRDIR)/sources
SRCTARBALL	:= mathopd-1.6b9.tar.gz
UNTAR		:= tar zxvf
DIFF		:= $(SRCTARBALL).alpha.diff

####################################################################################

ifeq ($(BUILD), $(wildcard $(BUILD)))
all: all_builds
else
all: prepare_source all_builds
endif

all_builds:
	@echo -e "\033[32mbuilding $(MYNAME) ...\033[0m"
	$(Q)make -C $(BUILDDIR)/mathopd-1.6b9/src V=$(V) DEBUG=$(DEBUG)

install:
	@echo -e "\033[32minstalling $(MYNAME) ...\033[0m"
	$(Q)make -C $(BUILDDIR)/mathopd-1.6b9/src install V=$(V) DEBUG=$(DEBUG)

clean_builds:
	$(Q)make -C $(BUILDDIR)/mathopd-1.6b9/src clean V=$(V) DEBUG=$(DEBUG)
	$(Q)find  $(BUILDDIR)/mathopd-1.6b9/src -name '*.bak' | xargs rm -f

clean:
	@echo -e "\033[32mcleaning $(MYNAME) ...\033[0m"
	$(Q)rm -rf $(BUILDDIR)

.PHONY: all all_builds install clean

####################################################################################

prepare_source:
	@echo -e "\033[32mpreparing the source of $(MYNAME) ...\033[0m"
	$(Q)mkdir -p $(BUILDDIR)
	$(Q)cd $(BUILDDIR); $(UNTAR) $(SRCDIR)/$(SRCTARBALL)
	$(Q)chmod +w -R $(BUILDDIR)
	$(Q)patch -p0 < $(SRCDIR)/$(DIFF)
	# for igd ipv6 patch
	#$(Q)patch -p0 < $(SRCDIR)/patch/main.c.igdipv6.patch
	#$(Q)patch -p0 < $(SRCDIR)/patch/Makefile.igdipv6.patch

mkdiff: clean_builds
	@echo -e "\033[32mGenerating the diff file for $(MYNAME)...\033[0m"
	$(Q)rm -rf original
	$(Q)mkdir -p original
	$(Q)cd original; $(UNTAR) $(SRCDIR)/$(SRCTARBALL)
	$(Q)$(SRCDIR)/mkdiff original $(BUILD) $(SRCDIR)/$(DIFF).tmp
	$(Q)$(SRCDIR)/stripdate.pl < $(SRCDIR)/$(DIFF).tmp > $(SRCDIR)/$(DIFF)
	$(Q)rm -rf original $(SRCDIR)/$(DIFF).tmp

.PHONY: prepare_source mkdiff
