#
#	Makefile for SAMBA
#

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config

####################################################################################

MYNAME		:= samba
VERSION		:= 3.0.24
CURRDIR		:= $(shell pwd)
BUILD		:= build
BUILDDIR	:= $(CURRDIR)/$(BUILD)
PROGDIR		:= $(CURRDIR)/$(BUILD)/$(MYNAME)-$(VERSION)/source
PUREDIR		:= original
RUNDIR		:= /var/etc/samba
SRCDIR		:= $(CURRDIR)/sources
SRCTARBALL	:= samba-$(VERSION).tar.gz
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

ifeq ($(ELBOX_PROGS_GPL_SAMBA_3_0_24_LITE), y)

all_builds: $(PROGDIR)/Makefile
#	@echo -e "kkk" ${ELBOX_PROGS_GPL_SAMBA_3_0_24_LITE}
	@echo -e "\033[32mBuilding only $(MYNAME) utilities ...\033[0m"
	$(Q)cp sources/proto.h $(PROGDIR)/include 
	$(Q)make bin/smbtree -C $(PROGDIR)
	$(Q)make bin/nmblookup -C $(PROGDIR)

install:
	@echo -e "\033[32mInstalling $(MYNAME) utilities ...\033[0m"
	$(Q)[ -d $(TARGET)/sbin ] || mkdir -p $(TARGET)/sbin
	$(Q)cp -u $(PROGDIR)/bin/smbtree $(TARGET)/sbin/
	$(Q)$(STRIP) $(TARGET)/sbin/smbtree
	$(Q)cp -u $(PROGDIR)/bin/nmblookup $(TARGET)/sbin/
	$(Q)$(STRIP) $(TARGET)/sbin/nmblookup

else

all_builds: $(PROGDIR)/Makefile
	@echo -e "\033[32mBuilding $(MYNAME) ...\033[0m"
	$(Q)make -C $(PROGDIR)

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/sbin ] || mkdir -p $(TARGET)/sbin
	$(Q)cp -u $(PROGDIR)/bin/smbd $(TARGET)/sbin/
	$(Q)$(STRIP) $(TARGET)/sbin/smbd
#sam_pan add add link smbd, nmbd, smbpasswd together	
	$(Q)cd $(TARGET)/sbin && ln -s -f ./smbd ./smbpasswd
	$(Q)cd $(TARGET)/sbin && ln -s -f ./smbd ./nmbd

endif


clean_builds:
	$(Q)-rm $(CONFIG_SITE)
	$(Q)-make -C $(PROGDIR) distclean
	$(Q)find $(PROGDIR) -name '*.bak' | xargs rm -f

clean:
	@echo -e "\033[32mCleaning $(MYNAME) ...\033[0m"
	$(Q)rm -rf $(BUILDDIR)

.PHONY: all all_builds install clean_builds clean

####################################################################################

prepare_makefile $(PROGDIR)/Makefile:
	@echo -e "\033[32mGenerating Makefile of $(MYNAME) ...\033[0m"
	$(Q)echo "samba_cv_HAVE_GETTIMEOFDAY_TZ=yes" > $(CONFIG_SITE)
	$(Q)echo "samba_cv_USE_SETRESUID=yes" >> $(CONFIG_SITE)
	$(Q)echo "samba_cv_HAVE_SECURE_MKSTEMP=yes" >> $(CONFIG_SITE)
	$(Q)cd $(PROGDIR); \
			./configure \
				--target=$(HOST_TYPE) \
				--host=$(HOST_TYPE) \
				--prefix=$(TARGET) \
				--localstatedir=$(RUNDIR) \
				--with-privatedir=$(RUNDIR) \
				--with-lockdir=$(RUNDIR)/locks \
				--with-piddir=$(RUNDIR) \
				--with-configdir=$(RUNDIR) \
				--with-logfilebase=$(RUNDIR) \
				--with-sendfile-support \
				--with-included-popt \
				--with-included-iniparser \
				--with-aio-support \
				--without-automount \
				--without-winbind \
				--without-ldap \
				--without-sys-quotas \
				--without-ads \
				--disable-cups \
				--disable-pie \
				--enable-static=yes \
				--enable-shared=no
#	@cp $(SRCDIR)/config.h $(PROGDIR)/include
	sed -i 's/\/\* #undef HAVE_EXPLICIT_LARGEFILE_SUPPORT \*\//#define HAVE_EXPLICIT_LARGEFILE_SUPPORT 1/g' $(PROGDIR)/include/config.h
	sed -i 's/\/\* #undef HAVE_IFACE_IFCONF \*\//#define HAVE_IFACE_IFCONF 1/g' $(PROGDIR)/include/config.h
	sed -i 's/\/\* #undef HAVE_LONGLONG \*\//#define HAVE_LONGLONG 1/g' $(PROGDIR)/include/config.h
	sed -i 's/\/\* #undef HAVE_OFF64_T \*\//#define HAVE_OFF64_T 1/g' $(PROGDIR)/include/config.h
	sed -i 's/#define HAVE_SIGSET 1/\/\* #undef HAVE_SIGSET \*\//g' $(PROGDIR)/include/config.h
	sed -i 's/\/\* #undef _FILE_OFFSET_BITS \*\//#define _FILE_OFFSET_BITS 64/g' $(PROGDIR)/include/config.h
	sed -i 's/\/\* #undef _LARGEFILE64_SOURCE \*\//#define _LARGEFILE64_SOURCE 1/g' $(PROGDIR)/include/config.h
prepare_source:
	@echo -e "\033[32mPreparing the source of $(MYNAME) ...\033[0m"
	$(Q)mkdir -p $(BUILDDIR)
	$(Q)cd $(BUILDDIR); $(UNTAR) $(SRCDIR)/$(SRCTARBALL)
	$(Q)chmod +w -R $(BUILDDIR)
	$(Q)patch -p0 < $(SRCDIR)/$(SRCTARBALL).alpha.diff

mkdiff: clean_builds
	@echo -e "\033[32mGenerating the diff file for $(MYNAME)...\033[0m"
	$(Q)rm -rf $(PUREDIR)
	$(Q)mkdir -p $(PUREDIR)
	$(Q)cd $(PUREDIR); $(UNTAR) $(SRCDIR)/$(SRCTARBALL)
	$(Q)$(SRCDIR)/mkdiff $(PUREDIR) $(BUILD) $(SRCDIR)/$(DIFF).tmp
	$(Q)$(SRCDIR)/stripdate.pl < $(SRCDIR)/$(DIFF).tmp > $(SRCDIR)/$(DIFF)
	$(Q)rm -rf original $(SRCDIR)/$(DIFF).tmp
	
.PHONY: generate_makefile prepare_source mkdiff
