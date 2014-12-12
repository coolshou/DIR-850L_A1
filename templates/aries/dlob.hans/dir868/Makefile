# search file from current folder to parent folders
# cutoff at 7th parent folder
search_cutoff := ../../../../../../../
define search-file
	$(if $(filter $(search_cutoff)%,$1),,$(if $(wildcard $1),$1,$(call search-file,../$1)))
endef

include $(call search-file,path.mk)
include $(TOPDIR)/lib.mk
include $(TOPDIR)/.config

targets :=

subdirs := $(subst /,,$(shell ls -d */ 2> /dev/null))

targets += all
all:
	$(call color_print,Building objects for brand $(ELBOX_MODEL_NAME) ...,green)
	$(call do_make_for_each,,,$(subdirs))

targets += rootfs
rootfs:
	$(call color_print,Constructing rootfs folders for brand $(ELBOX_MODEL_NAME) ...,green)
	$(call do_make_for_each,,rootfs,$(subdirs))

targets += install
install:
	$(call color_print,Installing objects for brand $(ELBOX_MODEL_NAME) ...,green)
	$(call do_make_for_each,,install,$(subdirs))
	$(call color_print,Seaching script files to set execution bit ...,green)
	find $(TARGET)/etc -name *.sh | xargs chmod +x

targets += clean
clean:
	$(call color_print,Cleaning objects for brand $(ELBOX_MODEL_NAME) ...,green)
	$(call do_make_for_each,,clean,$(subdirs))

.PHONY: $(targets)
