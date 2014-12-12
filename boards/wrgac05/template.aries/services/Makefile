# vi: set sw=4 ts=4:
###########################################################################
-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
###########################################################################

MYNAME	:= "WRGAC05/ARIES/SERVICES"
SERVICES:= LAYOUT ENLAN PHYINF.ETH-1 PHYINF.ETH-2 PHYINF.ETH-3 PHYINF.BAND24G-1.1 PHYINF.BAND24G-1.2\
		   PHYINF.BAND5G-1.1 PHYINF.BAND5G-1.2 PHYINF.WIFI PHYINF.WIFI-REPEATER
DIRS	:= PHYINF WIFI

ifeq ($(strip $(ELBOX_TEMPLATE_ARIES_ADVANCED_LAYOUT)),y)
SERVICES+= PHYINF.ETH-3 PHYINF.ETH-4
endif

###########################################################################
all:

rootfs:
	@echo -e "\033[32mCreating services for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/services ] || mkdir -p $(TARGET)/etc/services

install:
	@echo -e "\033[32mInstalling services for $(MYNAME) ...\033[0m"
	$(Q)for i in $(SERVICES); do cp $$i.php $(TARGET)/etc/services/.; done
	$(Q)for i in $(DIRS); do \
	if [ -f $$i/Makefile ]; then \
		$(MYMAKE) -C $$i install; \
	else \
		[ -d $(TARGET)/etc/services/$$i ] || mkdir -p $(TARGET)/etc/services/$$i ; \
		cp $$i/*.php $(TARGET)/etc/services/$$i/.; \
	fi \
	done

clean:

.PHONY: all rootfs install clean
