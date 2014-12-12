# vi: set sw=4 ts=4:
###########################################################################
-include ../../../path.mk
-include ../../../arch.mk
-include $(TOPDIR)/.config
###########################################################################

MYNAME	:= "WRGND12/ARIES/SERVICES"
SERVICES:= PHYINF.WIFI PHYINF.BAND24G-1.1 PHYINF.BAND24G-1.2 PHYINF.BAND5G-1.1 PHYINF.BAND5G-1.2

DIRS := WIFI PHYINF

###########################################################################

rootfs:
	@echo -e "\033[32mCreating services for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/services ] || mkdir -p $(TARGET)/etc/services

install: 
	@echo -e "\033[32mInstalling services for $(MYNAME) ...\033[0m"
	$(Q)for i in $(SERVICES); do cp $$i.php $(TARGET)/etc/services/.; done
	$(Q)for i in $(DIRS); do \
		if [ ! -d $(TARGET)/etc/service/$$i ]; then \
			mkdir -p $(TARGET)/etc/services/$$i; \
		fi ; \
		cp $$i/*.php $(TARGET)/etc/services/$$i/.; \
	done

clean:

.PHONY: all rootfs install clean
