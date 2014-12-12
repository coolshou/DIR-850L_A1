# search file from current folder to parent folders
# cutoff at 7th parent folder
search_cutoff := ../../../../../../../
define search-file
	$(if $(filter $(search_cutoff)%,$1),,$(if $(wildcard $1),$1,$(call search-file,../$1)))
endef

include $(call search-file,path.mk)
include $(TOPDIR)/lib.mk

targets :=

current_dir := $(shell pwd)
init_folder := $(TARGET)/etc/init.d
init0_folder := $(TARGET)/etc/init0.d

targets += all
all:
	$(call color_print,Building objects in $(current_dir) ...,green)

targets += rootfs
rootfs:
	$(call color_print,Constructing rootfs folders in $(current_dir)  ...,green)
	$(call create_install_paths,$(init_folder) $(init0_folder))

targets += install
# For init.S10init.sh, we need to do special action for it.
# It has substring "init." in its name.
# So we remove it first and put it back later.
init_scripts := $(subst init.,,$(subst init.S10init.sh,,$(wildcard init.*.sh)))
init_scripts += $(if $(wildcard init.S10init.sh),S10init.sh)

init0_scripts := $(subst init0.,,$(wildcard init0.*.sh))

# function: copy_init_files
# $1: initial script filename prefix
# $2: destination path
# $3: filenames
# usage: $(call copy_init_files,init0,path_a,script_1 script_2 ...)
define copy_init_files
	@for file in $3; do \
		cp --remove-destination $1.$$file $2/$$file; \
		chmod +x $2/$$file; \
	done
endef

install:
	$(call color_print,Installing objects in $(current_dir) ...,green)
	$(call copy_init_files,init,$(init_folder),$(init_scripts))
	$(call copy_init_files,init0,$(init0_folder),$(init0_scripts))

targets += clean
clean:
	$(call color_print,Cleaning objects in $(current_dir) ...,green)

.PHONY: $(targets)
