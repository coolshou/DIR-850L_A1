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

targets += all
all:

targets += rootfs
rootfs:
	$(call color_print,Constructing rootfs folders in $(current_dir) ...,green)
	$(call create_install_paths,$(TARGET)/etc/services)

targets += install
php_files := $(wildcard *.php)
dirs := $(subst /,,$(shell ls -d */ 2> /dev/null))

install:
	$(call color_print,Installing objects in $(current_dir) ...,green)
	$(call install_files,.,$(TARGET)/etc/services,$(php_files))
	$(call install_dirs,.,$(TARGET)/etc/services,$(dirs))
	@find $(TARGET)/etc/services -name .svn -type d | xargs rm -rf

targets += clean
clean:

.PHONY: $(targets)
