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
	$(call color_print,Building objects in $(current_dir) ...,green)

targets += rootfs
rootfs:
	$(call color_print,Constructing rootfs folders in $(current_dir)  ...,green)
	$(call create_install_paths,$(TARGET)/htdocs)

targets += install
copied_dirs := $(subst /,,$(shell ls -d */ 2> /dev/null))

install:
	$(call color_print,Installing objects in $(current_dir) ...,green)
	$(call install_dirs,.,$(TARGET)/htdocs,$(copied_dirs))
	@find $(TARGET)/htdocs -name .svn -type d | xargs rm -rf
	$(call install_files,.,$(TARGET)/etc/config,lld2d.ico)

targets += clean
clean:
	$(call color_print,Cleaning objects in $(current_dir) ...,green)

.PHONY: $(targets)
