#
# Makefile for rules of udevd.
#

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk
-include $(TOPDIR)/kernel/.config

###########################################################################

MYNAME	:= "Aries/udev.rules"
RULES	:= 20-names.rules
FILES   :=

ifeq ($(strip $(ELBOX_ARIES_USE_UDEV_USB_STORAGE_SUPPORT)),y)
RULES	+= 60-usb-storage.rules
endif

ifeq ($(strip $(CONFIG_TUN)),y)
RULES	+= 20-tun.rules
endif

ifeq ($(strip $(ELBOX_ARIES_USE_UDEV)),y)
FILES   += udev.conf
endif
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
	cp $(FILES) $(TARGET)/etc/udev/.
clean:

.PHONY: all rootfs install clean
