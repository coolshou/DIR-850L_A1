# vi: set sw=4 ts=4:
#
# Makefile for services of Aries.
#

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/services"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
SERVICES:= INFSVCS.BRIDGE-2 UPNP.BRIDGE-2 HTTP.BRIDGE-2 INET.BRIDGE-2 \
		   NAMERESOLV.BRIDGE-1 NAMERESOLV.BRIDGE-2 \
		   HW_NAT SAMBA DEVICE.PASSTHROUGH
DIRS	:= INET NAMERESOLV DDNS HTTP IPTABLES

###########################################################################

ifeq ($(CONFIG_SHAREPORT_WITH_SAMBA),y)
SERVICES += SHAREPORT
endif

ifeq ($(ELBOX_PROGS_GPL_ITUNES_SERVER),y)
SERVICES += ITUNES
endif

ifeq ($(ELBOX_ARIES_FEATURE_ROUTER_GATEWAY),y)
SERVICES += MULTICAST
endif

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/services ] || mkdir -p $(TARGET)/etc/services
	$(Q)[ -d $(TARGET)/etc/config ] || mkdir -p $(TARGET)/etc/config

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(SERVICES); do cp $$i.php $(TARGET)/etc/services/.; done
	$(Q)cp SAMBA_loop.sh $(TARGET)/etc/services/.;
	$(Q)for i in $(DIRS); do \
	if [ -f $$i/Makefile ]; then \
		$(MYMAKE) -C $$i install; \
	else \
		[ -d $(TARGET)/etc/services/$$i ] || mkdir -p $(TARGET)/etc/services/$$i ; \
		cp $$i/*.php $(TARGET)/etc/services/$$i/.; \
	fi \
	done
ifeq ($(strip $(ELBOX_MINIDLNA)),y)
	cp  UPNPAV_minidlna.php $(TARGET)/etc/services/UPNPAV.php
endif

clean:

.PHONY: all rootfs install clean
