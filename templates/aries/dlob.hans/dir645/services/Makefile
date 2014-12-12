# vi: set sw=4 ts=4:
#
# Makefile for services of Aries.
#

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/Dlob/services"
SERVICES := HTTP.WAN-1
DIRS    := HTTP

###########################################################################

ifeq ($(ELBOX_PROGS_PRIV_MIIICASA) , y)
SERVICES+= MIIICASA.WAN-1 miiicasa_proxyd miiicasa_agent
DIRS    := HTTP INFSVCS IPTABLES
endif

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/services ] || mkdir -p $(TARGET)/etc/services
	$(Q)[ -d $(TARGET)/etc/config ] || mkdir -p $(TARGET)/etc/config

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(SERVICES); do cp $$i.php $(TARGET)/etc/services/.; done
	$(Q)for i in $(DIRS); do \
		[ -d $(TARGET)/etc/services/$$i ] || mkdir -p $(TARGET)/etc/services/$$i ; \
		cp $$i/*.php $(TARGET)/etc/services/$$i/.; \
	done
clean:

.PHONY: all rootfs install clean
