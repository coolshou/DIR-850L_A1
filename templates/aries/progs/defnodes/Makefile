# vi: set sw=4 ts=4:

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/defnodes"
XMLFILES:= S20device.xml
PHPFILES:= S90sessions.php S22timezone.php
SHFILES :=

###########################################################################

ifeq ($(ELBOX_ARIES_FEATURE_ROUTER_GATEWAY),y)
PHPFILES += S90upnpigd.php S90upnpwfa.php
endif

ifeq ($(ELBOX_ARIES_FEATURE_BRIDGE_AP),y)
PHPFILES += S90upnpwfa.php
endif

ifeq ($(ELBOX_PROGS_GPL_DNSMASQ_ADVANCE_DNS_SERVICE_ALPHA),y)
PHPFILES += S90opendns.php
endif

ifeq ($(ELBOX_USE_IPV6),y)
PHPFILES += S91upnpigd2.php
endif
###########################################################################

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/defnodes ] || mkdir -p $(TARGET)/etc/defnodes

install:
	@echo -e "\033[32mInstalling applets for $(MYNAME) ...\033[0m"
	$(Q)for i in $(XMLFILES); do \
		sed -e 's|SiGnAtUrE|$(ELBOX_CONFIG_SIGNATURE)|' $$i > $(TARGET)/etc/defnodes/$$i ; done
	$(Q)for i in $(PHPFILES); do cp $$i $(TARGET)/etc/defnodes/$$i; done
	$(Q)for i in $(SHFILES); do \
		   	cp --remove-destination $$i $(TARGET)/etc/defnodes/$$i; \
			chmod +x $(TARGET)/etc/defnodes/$$i; \
		done

clean:

.PHONY: all rootfs install clean
