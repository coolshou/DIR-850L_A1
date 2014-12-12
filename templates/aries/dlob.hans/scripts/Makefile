# vi: set sw=4 ts=4:

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME  := "Aries/Dlob/scripts"
FILES	:= upwifistatshlper.sh ping.sh iptables_insmod.sh silex_usbmount.sh shareport_setgw.sh \
			IPV4.INET.php webaccess_map.php cpuload.sh wfa_igd_handle.php
DIRS	:= libs

ifeq ($(ELBOX_PROGS_GPL_UPNPC_IGD_FORWARDER),y)
FILES += upnpc.sh
endif
###########################################################################

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/scripts ] || mkdir -p $(TARGET)/etc/scripts

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(FILES); do \
		cp --remove-destination $$i $(TARGET)/etc/scripts/.; \
		chmod +x $(TARGET)/etc/scripts/$$i; \
	done
	$(Q)for i in $(DIRS); do \
		[ -d $(TARGET)/etc/scripts/$$i ] || mkdir -p $(TARGET)/etc/scripts/$$i ; \
		cp --remove-destination $$i/* $(TARGET)/etc/scripts/$$i/. ; \
	done	
	$(Q)cp fw_upgrade $(TARGET)/usr/sbin/.
	chmod +x $(TARGET)/usr/sbin/fw_upgrade

ifeq ($(strip $(ELBOX_PROGS_PRIV_WEB_FILE_ACCESS)),y)
	cp usbmount_helper.sh $(TARGET)/etc/scripts/
endif

clean:
.PHONY: all rootfs install clean

