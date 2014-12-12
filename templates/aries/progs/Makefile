# vi: set sw=4 ts=4:
#
# Makefile for elbox's template.
#

-include path.mk
-include arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

#####################################################################
# targets for main Makefile

MYNAME	:= "Aries"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
MYSUBDIRS:= rc htdocs scripts services events defnodes

ifeq ($(ELBOX_ARIES_USE_UDEV),y)
MYSUBDIRS+= udev.rules
endif

LIBCSRC := $(CC_PATH)

#####################################################################
define CheckOption
$(Q)$(if $(strip $(1)),$(if $(strip $(2)),$(MYMAKE) $(strip $(2)),),$(if $(strip $(3)),$(MYMAKE) $(strip $(3)),))
endef


all:
	@echo -e "\033[32mBuilding applets for $(MYNAME) ...\033[0m"
	$(Q)for i in $(MYSUBDIRS); do $(MYMAKE) -C $$i; done

prepare:
	@echo -e "\033[32mPreparing configuration for $(MYNAME) ...\033[0m"
	$(Q)$(if $(ELBOX_PROGS_GPL_BUSYBOX_1_00),	$(MYMAKE) bb_setup,)
	$(Q)$(if $(ELBOX_PROGS_GPL_BUSYBOX_1_11_1),	$(MYMAKE) bb_setup_1_11_1,)
	$(Q)$(if $(ELBOX_PROGS_GPL_BUSYBOX_1_14_1),	$(MYMAKE) bb_setup_1_14_1,)
	$(Q)$(if $(ELBOX_PROGS_GPL_BUSYBOX_1_18_1),	$(MYMAKE) bb_setup_1_18_1,)
	$(Q)$(MYMAKE) patch_cc_prefix
	$(call CheckOption,$(ELBOX_BUSYBOX_DYNAMIC_LINKING),				disable_static,)
	$(call CheckOption,$(ELBOX_BUSYBOX_HAVE_MDEV),						enable_mdev,)
	$(call CheckOption,$(ELBOX_BUSYBOX_USE_FANCY_PING),					enable_fancy_ping,)
	$(call CheckOption,$(ELBOX_TEMPLATE_ARIES_ENABLE_USER_MANAGEMENT),	enable_user_management,)
	$(call CheckOption,$(ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG),	enable_tftp,)
	$(call CheckOption,$(ELBOX_PROGS_GPL_BUSYBOX_NOBZIP2),				disable_bzip2,)
	$(call CheckOption,$(ELBOX_PROGS_GPL_BUSYBOX_NO_2_4_MODULES),		disable_2_4_modules,)
	$(call CheckOption,$(ELBOX_TEMPLATE_ARIES_STORAGE_SUPPORT),			enable_storage_support,)
	$(call CheckOption,$(ELBOX_PROGS_GPL_BUSYBOX_WITHOUT_IPROUTE2),		disable_iproute2,)
	$(call CheckOption,$(ELBOX_PROGS_GPL_BUSYBOX_WITHOUT_BZIP2),		disable_bzip2,)
	$(call CheckOption,$(ELBOX_PROGS_GPL_BUSYBOX_WITHOUT_2_4_MODULES),	disable_2_4_modules,)
	$(call CheckOption,$(ELBOX_TEMPLATE_ARIES_MYDLINK_SUPPORT),			enable_wget,)
	$(Q)$(MYMAKE) bb_oldconfig

rootfs:
	@echo -e "\033[32mCreate rootfs for $(MYNAME) ...\033[0m"
	$(Q)for i in $(MYSUBDIRS); do $(MYMAKE) -C $$i rootfs; done

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
ifndef TPATH_LIBTGZ
	$(Q)$(MYMAKE) $(if $(strip $(ELBOX_BSP_GNUCLIB)),gnuclib,uclibc)
endif
	$(Q)for i in $(MYSUBDIRS); do $(MYMAKE) -C $$i install; done

clean:
	@echo -e "\033[32mCleaning $(MYNAME) ...\033[0m"
	$(Q)for i in $(MYSUBDIRS); do $(MYMAKE) -C $$i clean; done

.PHONY: all install clean prepare rootfs

#####################################################################

ifeq ($(strip $(ELBOX_PROGS_GPL_BUSYBOX_1_00)),y)
BBBUILDDIR := busybox-1.00
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_BUSYBOX_1_11_1)),y)
BBBUILDDIR := busybox-1.11.1
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_BUSYBOX_1_14_1)),y)
BBBUILDDIR := busybox-1.14.1
endif
ifeq ($(ELBOX_PROGS_GPL_BUSYBOX_1_18_1),y)
BBBUILDDIR := busybox/build/busybox-1.18.1
endif
BBCONF = $(TOPDIR)/progs.gpl/$(BBBUILDDIR)/.config

#####################################################################

define TurnOn
	$(Q)sed -e 's|^# $(1) is not set|$(1)=y|' $(BBCONF) -i $(BBCONF)
endef
define TurnOff
	$(Q)sed -e 's|^$(1)=y|# $(1) is not set|' $(BBCONF) -i $(BBCONF)
endef

patch_cc_prefix:
ifeq ($(strip $(BUSYBOX_CC_PREFIX)),)
	$(Q)sed -e 's|@@CROSS_COMPILER@@|$(TPATH_UC)/bin/$(CROSS_COMPILE)|' $(BBCONF) -i $(BBCONF)
else
	$(Q)sed -e 's|@@CROSS_COMPILER@@|$(BUSYBOX_CC_PREFIX)|' $(BBCONF) -i $(BBCONF)
endif
	$(Q)sed -e 's|@@TARGET@@|$(TARGET)|' $(BBCONF) -i $(BBCONF)

disable_static:
	$(call TurnOff,CONFIG_STATIC)

enable_mdev:
	$(call TurnOn,CONFIG_MDEV)

enable_fancy_ping:
	$(call TurnOn,CONFIG_FEATURE_FANCY_PING)

enable_user_management:
	$(call TurnOn,CONFIG_FEATURE_SHADOWPASSWDS)
	$(call TurnOn,CONFIG_USE_BB_PWD_GRP)
	$(call TurnOn,CONFIG_USE_BB_SHADOW)
	$(call TurnOn,CONFIG_USE_BB_CRYPT)
	$(call TurnOn,CONFIG_USE_BB_CRYPT_SHA)
	$(call TurnOn,CONFIG_ADDGROUP)
	$(call TurnOn,CONFIG_FEATURE_ADDUSER_TO_GROUP)
	$(call TurnOn,CONFIG_DELGROUP)
	$(call TurnOn,CONFIG_FEATURE_DEL_USER_FROM_GROUP)
	$(call TurnOn,CONFIG_FEATURE_CHECK_NAMES)
	$(call TurnOn,CONFIG_ADDUSER)
	$(call TurnOn,CONFIG_DELUSER)
	$(call TurnOn,CONFIG_PASSWD)
	$(call TurnOn,CONFIG_CRYPTPW)
	$(call TurnOn,CONFIG_CHPASSWD)

disable_iproute2:
	$(call TurnOff,CONFIG_CMP)
	$(call TurnOff,CONFIG_IP)
	$(call TurnOff,CONFIG_FEATURE_IP_ADDRESS)
	$(call TurnOff,CONFIG_FEATURE_IP_LINK)
	$(call TurnOff,CONFIG_FEATURE_IP_ROUTE)
	$(call TurnOff,CONFIG_FEATURE_IP_TUNNEL)
	$(call TurnOff,CONFIG_FEATURE_IP_RULE)
	$(call TurnOff,CONFIG_FEATURE_IP_SHORT_FORMS)
	$(call TurnOff,CONFIG_IPADDR)
	$(call TurnOff,CONFIG_IPLINK)
	$(call TurnOff,CONFIG_IPROUTE)
	$(call TurnOff,CONFIG_IPTUNNEL)
	$(call TurnOff,CONFIG_IPRULE)

enable_tftp:
	$(call TurnOn,CONFIG_TFTP)
	$(call TurnOn,CONFIG_FEATURE_TFTP_GET)
	$(call TurnOn,CONFIG_FEATURE_TFTP_PUT)

disable_bzip2:
	$(call TurnOff,CONFIG_BUNZIP2)
	$(call TurnOff,CONFIG_BZIP2)

disable_2_4_modules:
	$(call TurnOff,CONFIG_FEATURE_2_4_MODULES)

enable_storage_support:
	$(call TurnOn,CONFIG_DD)
	$(call TurnOn,CONFIG_FEATURE_DD_SIGNAL_HANDLING)
	$(call TurnOn,CONFIG_DF)
	$(call TurnOn,CONFIG_DU)
	$(call TurnOn,CONFIG_FEATURE_DU_DEFAULT_BLOCKSIZE_1K)
	$(call TurnOn,CONFIG_WGET)
	$(call TurnOn,CONFIG_FEATURE_WGET_AUTHENTICATION)
	$(call TurnOn,CONFIG_GETOPT_LONG)
# to use CONFIG_FEATURE_WGET_LONG_OPTIONS, need enable CONFIG_GETOPT_LONG
	$(call TurnOn,CONFIG_FEATURE_WGET_LONG_OPTIONS)
	$(call TurnOn,CONFIG_FDISK)
	$(call TurnOn,CONFIG_FDISK_SUPPORT_LARGE_DISKS)
	$(call TurnOn,CONFIG_FEATURE_FDISK_WRITABLE)
	$(call TurnOn,CONFIG_FEATURE_FDISK_ADVANCED)
# Use external fdisk and e2fs tools.
#	$(call TurnOn,CONFIG_MKFS_EXT2)
#	$(call TurnOn,CONFIG_MKFS_VFAT)

enable_wget:
	$(call TurnOn,CONFIG_WGET)
	$(call TurnOn,CONFIG_FEATURE_WGET_AUTHENTICATION)
	$(call TurnOn,CONFIG_FEATURE_WGET_LONG_OPTIONS)
	$(call TurnOn,CONFIG_GETOPT_LONG)
	

.PHONY: patch_cc_prefix disable_static enable_mdev enable_fancy_ping enable_user_management
.PHONY: disable_iproute2 enable_tftp disable_bzip2 disable_2_4_modules enable_storage_support enable_wget
#####################################################################

# If model have private config file for busybox, then use it. 
# Else, use common config file(progs.tempates/busybox-xxx).
define PickConfig
	if [ -f $(TOPDIR)/progs.brand/$(ELBOX_MODEL_NAME)/$(1) ]; then\
		echo -e "\033[32mSetup $(1) - use model's private config file ...\033[0m"; \
		cp $(TOPDIR)/progs.brand/$(ELBOX_MODEL_NAME)/$(1) $(BBCONF); \
	else \
		cp $(1) $(BBCONF); \
	fi
endef

bb_oldconfig:
	$(Q)make -C $(TOPDIR)/progs.gpl/$(BBBUILDDIR) oldconfig > /dev/null

bb_setup:
	@echo -e "\033[32mSetup busybox v1.00 configuration ...\033[0m"
	$(Q)$(call PickConfig,busybox-1.00.config)

bb_setup_1_11_1:
	@echo -e "\033[32mSetup busybox v1.11.1 configuration ...\033[0m"
	$(Q)$(call PickConfig,busybox-1.11.1.config)
		
bb_setup_1_14_1:
	@echo -e "\033[32mSetup busybox v1.14.1 configuration ...\033[0m"
	$(Q)$(call PickConfig,busybox-1.14.1.config)
	# IPROUTE2 is default enabled in busybox 1.14.1, so we have an option to disable it.
	$(Q)$(if $(strip $(ELBOX_PROGS_GPL_BUSYBOX_1_14_1_NOIP)),$(MYMAKE) disable_iproute2,)
	$(Q)sed -e 's|@@CFLAGS@@|$(CFLAGS)|' $(BBCONF) -i $(BBCONF)

bb_setup_1_18_1:
	@echo -e "\033[32mSetup busybox v1.18.1 configuration ...\033[0m"
	$(Q)make -C $(TOPDIR)/progs.gpl/busybox prepare_source V=$(V) DEBUG=$(DEBUG)
	$(Q)$(call PickConfig,busybox-1.18.1.config)
	$(Q)sed -e 's|@@CFLAGS@@|$(CFLAGS) -I$(TOPDIR)/include|' $(BBCONF) -i $(BBCONF)

.PHONY: bb_oldconfig bb_setup bb_setup_1_11_1 bb_setup_1_14_1 bb_setup_1_18_1
#####################################################################
LIBPTHREAD		:= $(notdir $(wildcard $(LIBCSRC)/lib/libpthread-*.so))
LIBPTHREADLINK	:= $(notdir $(wildcard $(LIBCSRC)/lib/libpthread.so*))

uclibc:
	@echo -e "\033[32mInstalling uClibc ...\033[0m"
	$(Q)[ -d $(TARGET)/lib ] || mkdir -p $(TARGET)/lib
	@# ld-uClibc.so.0
	$(Q)cp -f $(LIBCSRC)/lib/ld-uClibc.so.0	$(TARGET)/lib/.
	@# libc.so libc.so.0
	$(Q)cp -f $(LIBCSRC)/lib/libc.so.0		$(TARGET)/lib/.
	$(Q)[ ! -f $(TARGET)/lib/libc.so ] || rm -f $(TARGET)/lib/libc.so
	$(Q)ln -s libc.so.0						$(TARGET)/lib/libc.so
	@# libcrypt.so libcrypt.so.0
	$(Q)cp -f $(LIBCSRC)/lib/libcrypt.so.0	$(TARGET)/lib/.
	$(Q)[ ! -f $(TARGET)/lib/libcrypt.so ] || rm -f $(TARGET)/lib/libcrypt.so
	$(Q)ln -s libcrypt.so.0					$(TARGET)/lib/libcrypt.so
	@# libdl.so libdl.so.0
	$(Q)cp -f $(LIBCSRC)/lib/libdl.so.0		$(TARGET)/lib/.
	$(Q)[ ! -f $(TARGET)/lib/libdl.so ] || rm -f $(TARGET)/lib/libdl.so
	$(Q)ln -s libdl.so.0					$(TARGET)/lib/libdl.so
	@# libm.so libm.so.0
	$(Q)cp -f $(LIBCSRC)/lib/libm.so.0		$(TARGET)/lib/.
	$(Q)[ ! -f $(TARGET)/lib/libm.so ] || rm -f $(TARGET)/lib/libm.so
	$(Q)ln -s libm.so.0						$(TARGET)/lib/libm.so
	@# libnsl.so libnsl.so.0
	$(Q)cp -f $(LIBCSRC)/lib/libnsl.so.0	$(TARGET)/lib/.
	$(Q)[ ! -f $(TARGET)/lib/libnsl.so ] || rm -f $(TARGET)/lib/libnsl.so
	$(Q)ln -s libnsl.so.0					$(TARGET)/lib/libnsl.so
	@# libresolv.so libresolv.so.0
	$(Q)cp -f $(LIBCSRC)/lib/libresolv.so.0	$(TARGET)/lib/.
	$(Q)[ ! -f $(TARGET)/lib/libresolv.so ] || rm -f $(TARGET)/lib/libresolv.so
	$(Q)ln -s libresolv.so.0				$(TARGET)/lib/libresolv.so
ifeq ($(strip $(ELBOX_INSTALL_LIBGCC_A_SO)),y)
	@# libgcc_s.so libgcc_s.so.1
	$(Q)cp -f $(LIBCSRC)/lib/libgcc_s.so.1	$(TARGET)/lib/.
	$(Q)[ ! -f $(TARGET)/lib/libgcc_s.so ] || rm -f $(TARGET)/lib/libgcc_s.so
	$(Q)ln -s libgcc_s.so.1					$(TARGET)/lib/libgcc_s.so
endif
ifeq ($(strip $(ELBOX_INSTALL_LIBUTIL_SO)),y)
	@# libutil.so libutil.so.0
	$(Q)cp -f $(LIBCSRC)/lib/libutil.so.0	$(TARGET)/lib/.
	$(Q)[ ! -f $(TARGET)/lib/libutil.so ] || rm -f $(TARGET)/lib/libutil.so
	$(Q)ln -s libutil.so.0					$(TARGET)/lib/libutil.so
endif
ifeq ($(strip $(ELBOX_INSTALL_LIBRT_SO)),y)
	@# librt.so librt.so.0
	$(Q)cp -f $(LIBCSRC)/lib/librt.so.0 $(TARGET)/lib/.
	$(Q)[ ! -f $(TARGET)/lib/librt.so ] || rm -f $(TARGET)/lib/librt.so
	$(Q)ln -s librt.so.0                    $(TARGET)/lib/librt.so
endif
ifeq ($(strip $(ELBOX_INSTALL_LIBPTHREAD_SO)),y)
	@# libpthread.so.0 libpthread.so.0 libpthread-0.9.30.so
	$(Q)cp -f $(LIBCSRC)/lib/$(LIBPTHREAD) $(TARGET)/lib/.
	$(Q)for i in $(LIBPTHREADLINK); do \
		ln -fs $(LIBPTHREAD) $(TARGET)/lib/$$i; \
	done
	$(Q)$(STRIP) $(TARGET)/lib/$(LIBPTHREAD)
endif
ifeq ($(strip $(ELBOX_INSTALL_LIBGCC_S_4181_SO)),y)
	$(Q)cp -f $(LIBCSRC)/lib/libgcc_s_4181.so.1 $(TARGET)/lib/.
	$(Q)[ ! -f $(TARGET)/lib/libgcc_s_4181.so ] || rm -f $(TARGET)/lib/libgcc_s_4181.so
	$(Q)ln -s libgcc_s_4181.so.1			$(TARGET)/lib/libgcc_s_4181.so
endif

gnuclib:
	@echo -e "\033[32mInstalling GNU C Lib ...\033[0m"
ifeq ($(TARGET_ABI),64)
	$(Q)[ -d $(TARGET)/lib64 ] || mkdir -p $(TARGET)/lib64
	$(Q)cp -Rp $(LIBCSRC)/lib64/ld*              $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libanl*          $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libBrokenLocale* $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libc*            $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libdl*           $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libm*            $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libnsl*          $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libnss**         $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libpthread*      $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libpcprofile*    $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libresolv*       $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/librt*           $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libSegFault*     $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libthread_db*    $(TARGET)/lib64/.
	$(Q)cp -Rp $(LIBCSRC)/lib64/libutil*         $(TARGET)/lib64/.
	$(Q)find $(TARGET)/lib64/ -name *.so | xargs $(STRIP)
endif

gpl:
	make -C htdocs gpl
	mv Makefile.gpl Makefile

.PHONY: uclibc gnuclib gpl
