include ../../path.mk
include ../../arch.mk
include $(TOPDIR)/.config
include $(TOPDIR)/lib.mk

SCRIPTS := $(wildcard *.sh)

all:

clean:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(shell pwd) ...\033[0m"
	$(call create_install_paths,$(TARGET)/etc/events)

install:
	@echo -e "\033[32mInstalling $(shell pwd) ...\033[0m"
	$(call install_scripts,$(shell pwd),$(TARGET)/etc/events,$(SCRIPTS))

.PHONY: all rootfs install clean
