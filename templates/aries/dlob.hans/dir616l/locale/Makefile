#
# Makefile for installing locale files.
#

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/Dlob/locale"
LANGUAGE_FILES := $(wildcard *.slp)
###########################################################################
all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/sealpac/wizard ] || mkdir -p $(TARGET)/etc/sealpac/wizard

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(LANGUAGE_FILES); do \
	[ -d $(TARGET)/etc/sealpac/wizard ] || mkdir -p $(TARGET)/etc/sealpac/wizard ;\
	cp -r --remove-destination $$i $(TARGET)/etc/sealpac/wizard/$$i ;\
	done
clean:

.PHONY: all rootfs install clean
