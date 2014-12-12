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
SERVICES:= INFSVCS.BRIDGE-2 UPNP.BRIDGE-2 HTTP.BRIDGE-2 INET.BRIDGE-2 NAMERESOLV.BRIDGE-1 NAMERESOLV.BRIDGE-2
DIRS	:= INET HTTP NAMERESOLV 

###########################################################################

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/services ] || mkdir -p $(TARGET)/etc/services
	$(Q)[ -d $(TARGET)/etc/config ] || mkdir -p $(TARGET)/etc/config

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
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
