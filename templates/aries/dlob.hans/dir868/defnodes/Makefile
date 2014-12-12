# search file from current folder to parent folders
# cutoff at 7th parent folder
search_cutoff := ../../../../../../../
define search-file
	$(if $(filter $(search_cutoff)%,$1),,$(if $(wildcard $1),$1,$(call search-file,../$1)))
endef

include $(call search-file,path.mk)
include $(TOPDIR)/lib.mk
include $(TOPDIR)/.config
include $(TOPDIR)/Vars.mk

targets :=
current_dir := $(shell pwd)

targets += all
all:

targets += rootfs
rootfs:
	$(call color_print,Constructing rootfs folders in $(current_dir) ...,green)
	$(call create_install_paths,$(TARGET)/etc/defnodes)

targets += install
xml_files := $(wildcard *.xml)
script_files := $(wildcard *.sh)
php_files := $(wildcard *.php)

# function: replace_sign_and_install
# $1: source path
# $2: destination path
# $3: filenames
# usage: $(call replace_sign_and_install,path_a,path_b,file_1 file_2 ...)
define replace_sign_and_install
	@for file in $3; do \
		sed -e 's|SiGnAtUrE|$(ELBOX_CONFIG_SIGNATURE)|' $1/$$file > $2/$$file; \
	done
endef

install:
	$(call color_print,Installing objects in $(current_dir) ...,green)
	$(call replace_sign_and_install,.,$(TARGET)/etc/defnodes,$(xml_files))
	$(call install_files,.,$(TARGET)/etc/defnodes,$(php_files))
	$(call install_scripts,.,$(TARGET)/etc/defnodes,$(script_files))

targets += clean
clean:

.PHONY: $(targets)
