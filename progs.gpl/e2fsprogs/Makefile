#
#	Makefile for e2fsprogs
#

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/Rules.mk

####################################################################################

MYNAME		:= e2fsprogs
ifeq ($(strip $(ELBOX_PROGS_GPL_E2FSPROGS_1_41_12)),y)
VERSION		:= 1.41.12
endif
CURRDIR		:= $(shell pwd)
BUILD		:= build
BUILDDIR	:= $(CURRDIR)/$(BUILD)
PROGDIR		:= $(CURRDIR)/$(BUILD)/$(MYNAME)-$(VERSION)
PUREDIR		:= original
SRCDIR		:= $(CURRDIR)/sources
SRCTARBALL	:= $(MYNAME)-$(VERSION).tar.gz
CONFIG_SITE	:= $(PROGDIR)/config.site
DIFF		:= $(SRCTARBALL).alpha.diff
UNTAR		:= tar zxf

####################################################################################
.EXPORT_ALL_VARIABLES:

ifeq ($(BUILD), $(wildcard $(BUILD)))
all: all_builds
else
all: prepare_source all_builds
endif

all_builds: $(PROGDIR)/Makefile
	@echo -e "\033[32mBuilding $(MYNAME) ...\033[0m"
	$(Q)make -C $(PROGDIR)

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)ln -fs /proc/mounts $(TARGET)/etc/mtab
	$(Q)make -C $(PROGDIR)/misc install-strip

clean_builds:
	$(Q)-rm $(CONFIG_SITE)
	$(Q)-make -C $(PROGDIR) distclean

clean:
	@echo -e "\033[32mCleaning $(MYNAME) ...\033[0m"
	$(Q)if [ -d $(BUILDDIR) ]; then rm -rf $(BUILDDIR); fi
	$(Q)if [ -f .depend ]; then rm -f .depend; fi

.PHONY: all all_builds install clean_builds clean

####################################################################################

prepare_makefile $(PROGDIR)/Makefile:
	@echo -e "\033[32mGenerating Makefile of $(MYNAME) ...\033[0m"
	$(Q)echo "enable_symlink_install=yes" > $(CONFIG_SITE)
	$(Q)echo "enable_symlink_build=yes" >> $(CONFIG_SITE)
	$(Q)cd $(PROGDIR); \
			./configure \
				--target=$(HOST_TYPE) \
				--host=$(HOST_TYPE) \
				--prefix=$(TARGET) \
				--enable-libuuid= \
				--enable-libblkid= \
				--disable-testio-debug \
				--disable-debugfs \
				--disable-imager \
				--disable-resizer \
				--disable-tls \
				--disable-uuidd

prepare_source:
	@echo -e "\033[32mPreparing the source of $(MYNAME) ...\033[0m"
	$(Q)mkdir -p $(BUILDDIR)
	$(Q)cd $(BUILDDIR); $(UNTAR) $(SRCDIR)/$(SRCTARBALL)
	$(Q)chmod +w -R $(BUILDDIR)
	$(Q)if [ -e $(SRCDIR)/$(DIFF) ]; then \
		patch -p0 < $(SRCDIR)/$(DIFF); \
	fi

mkdiff: clean_builds
	@echo -e "\033[32mGenerating the diff file for $(MYNAME)...\033[0m"
	$(Q)rm -rf original
	$(Q)mkdir -p original
	$(Q)cd original; $(UNTAR) $(SRCDIR)/$(SRCTARBALL)
	$(Q)$(SRCDIR)/mkdiff original $(BUILD) $(SRCDIR)/$(DIFF).tmp
	$(Q)$(SRCDIR)/stripdate.pl < $(SRCDIR)/$(DIFF).tmp > $(SRCDIR)/$(DIFF)
	$(Q)rm -rf original $(SRCDIR)/$(DIFF).tmp

.PHONY: prepare_makefile prepare_source mkdiff
