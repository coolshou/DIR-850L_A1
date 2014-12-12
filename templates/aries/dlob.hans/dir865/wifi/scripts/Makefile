# vi: set sw=4 ts=4:
###########################################################################
include ../../../path.mk
include ../../../arch.mk
include $(TOPDIR)/.config
include $(TOPDIR)/lib.mk
###########################################################################

SCRIPTS := $(wildcard *.sh *.php)
DIRS := wifi wps

###########################################################################

#all:

rootfs:
	@echo -e "\033[32mCreating scripts for $(shell pwd) ...\033[0m"
	$(call create_install_paths,$(TARGET)/etc/scripts)

install: 
	@echo -e "\033[32mInstalling scripts for $(shell pwd) ...\033[0m"
	$(call install_scripts,$(shell pwd),$(TARGET)/etc/scripts,$(SCRIPTS))
	$(call install_dirs,$(shell pwd),$(TARGET)/etc/scripts,$(DIRS))
	@chmod +x $(TARGET)/etc/scripts/*.sh;
	
clean:

.PHONY: all rootfs install clean
