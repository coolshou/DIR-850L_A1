# vi: set sw=4 ts=4:
###########################################################################
-include ../../../../path.mk
-include ../../../../arch.mk
-include $(TOPDIR)/.config
###########################################################################
MYNAME	:= "WRGN70/ARIES/SCRIPTS/WIFI/WPS"
FILES	:= wps.php
DIRS	:=
###########################################################################

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/scripts ] || mkdir -p $(TARGET)/etc/scripts
	$(Q)[ -d $(TARGET)/etc/scripts/wps ] || mkdir -p $(TARGET)/etc/scripts/wps

install:
	@echo -e "\033[32mInstalling scripts for $(MYNAME) ...\033[0m"
	$(Q)for i in $(FILES); do \
		cp --remove-destination $$i $(TARGET)/etc/scripts/wps/.; \
		chmod +x $(TARGET)/etc/scripts/wps/$$i; \
	done
	$(Q)for i in $(DIRS); do \
		if [ ! -d $(TARGET)/etc/scripts/$$i ]; then \
			mkdir -p $(TARGET)/etc/scripts/$$i; \
		fi ; \
		cp -r --remove-destination $$i $(TARGET)/etc/scripts/. ; \
	done

clean:

.PHONY: all rootfs install clean
