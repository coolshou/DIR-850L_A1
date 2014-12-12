#
# Makefile for rules of udevd.
#

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/udev.rules"
RULES	:= 60-usb-storage.rules 70-net.rules
FILES   :=

###########################################################################

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/udev/rules.d ] || mkdir -p $(TARGET)/etc/udev/rules.d

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(RULES); do \
		cp $$i $(TARGET)/etc/udev/rules.d/.; \
	done
	
clean:

.PHONY: all rootfs install clean
