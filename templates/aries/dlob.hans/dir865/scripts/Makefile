# vi: set sw=4 ts=4:

# function: search-file
# $1: filename
#
# search file from current folder to parent folders
# cutoff at 7th parent folder
search_cutoff := ../../../../../../../
define search-file
        $(if $(filter $(search_cutoff)%,$1),,$(if $(wildcard $1),$1,$(call search-file,../$1)))
endef

include $(call search-file,path.mk)
include $(call search-file,arch.mk)
include $(TOPDIR)/lib.mk

###########################################################################

SCRIPTS := $(wildcard *.sh)

###########################################################################

all:

clean:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(shell pwd) ...\033[0m"
	$(call create_install_paths,$(TARGET)/etc/scripts)

install:
	@echo -e "\033[32mInstalling $(shell pwd) ...\033[0m"
	$(call install_scripts,$(shell pwd),$(TARGET)/etc/scripts,$(SCRIPTS))

.PHONY: rootfs install all clean
