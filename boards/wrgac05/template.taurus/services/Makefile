# vi: set sw=4 ts=4:
###########################################################################
-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
###########################################################################

MYNAME   := $(ELBOX_BOARD_NAME)/$(ELBOX_TEMPLATE_NAME)/services
SERVICES :=
DIRS     :=

define INSPROFILE
	$(shell echo $(1) | awk '{if ($$1 >= $(2)) print "yes"}')
endef

ifeq ($(strip $(ELBOX_NETWORK)),y)
SERVICES += LAYOUT ENLAN
endif
ifeq ($(shell echo $(ELBOX_NETWORK_PHYINF_PROFILES) | awk '{if ($$1 >= 1) print "yes"}'),yes)
SERVICES += PHYINF.ETH-1 
DIRS     += PHYINF
endif
ifeq ($(shell echo $(ELBOX_NETWORK_PHYINF_PROFILES) | awk '{if ($$1 >= 2) print "yes"}'),yes)
SERVICES += PHYINF.ETH-2 
endif
ifeq ($(shell echo $(ELBOX_NETWORK_PHYINF_PROFILES) | awk '{if ($$1 >= 3) print "yes"}'),yes)
SERVICES += PHYINF.ETH-3 
endif
ifeq ($(shell echo $(ELBOX_NETWORK_PHYINF_PROFILES) | awk '{if ($$1 >= 4) print "yes"}'),yes)
SERVICES += PHYINF.ETH-4 
endif
ifeq ($(strip $(ELBOX_NETWORK_WIFI)),y)
SERVICES += PHYINF.WIFI WIFI
DIRS     += WIFI
ifeq ($(strip $(ELBOX_NETWORK_WIFI_2.4G)),y)
SERVICES += PHYINF.BAND24G-1.1 WIFI_5G
endif
ifeq ($(strip $(ELBOX_NETWORK_WIFI_5G)),y)
SERVICES += PHYINF.BAND5G-1.1 WIFI_5G
endif
endif

ifeq ($(strip $(ELBOX_NETWORK_ADVANCED_LAYOUT)),y)
SERVICES += PHYINF.ETH-3 PHYINF.ETH-4
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
