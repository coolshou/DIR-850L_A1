# Makefile for iptables-1.4.7

include ../../path.mk
include $(TOPDIR)/arch.mk
include $(TOPDIR)/.config
include $(KERNELDIR)/.config

IPTABLESPATH := src
INSTALL_ROOT=$(shell pwd)/install

ifeq ($(strip $(ELBOX_USE_IPV6)),y)
IPV6_PARAM := 
else
IPV6_PARAM := --disable-ipv6 
endif

ifndef KERNEL_DIR
KERNEL_DIR=$(KERNELDIR)
endif

CFLAGS += -DIPTABLES_VERSION=$(IPTABLES_VERSION) -I$(TOPDIR)/include
export CFLAGS
export LDFLAGS

CONFIGCMD = ./configure --verbose --target=$(HOST_TYPE) --host=$(HOST_TYPE) $(IPV6_PARAM) \
--prefix=$(INSTALL_ROOT)/usr --exec-prefix=$(INSTALL_ROOT)/usr --libdir=$(INSTALL_ROOT)/lib \
--with-xtlibdir=$(INSTALL_ROOT)/lib/xtables --disable-devel --enable-shared --disable-libipq --with-kernel=$(KERNELDIR)

all:
	@if [ ! -f ./$(IPTABLESPATH)/Makefile ]; then \
		echo -e "\033[32miptables-1.4.7 hasn't be config before, configuring...\033[0m"; \
		cd $(IPTABLESPATH); echo -e "\033[32m$(CONFIGCMD)\033[0m"; $(CONFIGCMD); \
	fi
		
	@echo -e "\033[32mCompiling iptables-1.4.7 ...\033[0m"
	@make -C $(IPTABLESPATH)

install:
	@if [ -f ./$(IPTABLESPATH)/Makefile ]; then \
		echo -e "\033[32mInstall for iptables-1.4.7...\033[0m"; \
		[ -d $(INSTALL_ROOT) ] || mkdir -p $(INSTALL_ROOT); \
		make -C $(IPTABLESPATH) install; \
		[ -d $(TARGET)/usr/sbin ] || mkdir -p $(TARGET)/usr/sbin; \
	fi

	# Copy necessary files to rootfs
	cp -af $(INSTALL_ROOT)/lib/libiptc.so* $(TARGET)/lib/.
	cp -af $(INSTALL_ROOT)/lib/libip4tc.so* $(TARGET)/lib/.
ifeq ($(strip $(ELBOX_USE_IPV6)),y)
	cp -af $(INSTALL_ROOT)/lib/libip6tc.so* $(TARGET)/lib/.
endif
	cp -af $(INSTALL_ROOT)/lib/libxtables.* $(TARGET)/lib/.
	cp -arf $(INSTALL_ROOT)/lib/xtables $(TARGET)/lib/.
	$(Q)$(STRIP) $(INSTALL_ROOT)/usr/sbin/ip*
	cp -af $(INSTALL_ROOT)/usr/sbin/ip* $(TARGET)/usr/sbin/.
	
ifeq ($(strip $(ELBOX_IPTABLES_L7_EXT)),y)	
	@echo Nothing to compile, just run \'make install\'
	@echo \(This simply copies this directory into $(TARGET)/etc/l7-protocols \)
	$(Q)mkdir -p $(TARGET)/etc/l7-protocols
	$(Q)cp -R ./layer7_support/l7-protocols-2010-10-19/* $(TARGET)/etc/l7-protocols
	$(Q)find $(TARGET)/etc/l7-protocols -name .svn -type d | xargs rm -rf
endif

clean:
	@if [ -f ./$(IPTABLESPATH)/Makefile ]; then \
		make -C $(IPTABLESPATH) distclean; \
		rm -f ./$(IPTABLESPATH)/Makefile; \
	fi
	rm -rf $(INSTALL_ROOT);

.PHONY: all install clean
