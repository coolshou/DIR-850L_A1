# vi: set sw=4 ts=4:

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME  := "Aries/Dlob/scripts"
FILES	:= iptables_insmod.sh IPV4.INET.php
#We don't need following files
DONT_NEED_FILES := cpuload.sh adapter_cmd.php adapter_cmd.sh getmodem.sh shareport_setgw.sh silex_usbmount.sh upwifistatshlper_A_band.sh 
DIRS	:= 

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
	$(Q)for i in $(DONT_NEED_FILES); do \
		rm -f $(TARGET)/etc/scripts/$$i; \
	done	

clean:
.PHONY: all rootfs install clean

