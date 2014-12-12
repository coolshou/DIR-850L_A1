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
include $(call searcf-file,arch.mk)

include $(TOPDIR)/.config
include $(TOPDIR)/Vars.mk
include $(TOPDIR)/lib.mk

###########################################################################

MYNAME	:= "$(ELBOX_TEMPLATE_NAME).$(ELBOX_BRAND_NAME).$(ELBOX_MODEL_NAME)"
HTDOCS	:= webinc web
SUBDIRS	:=

###########################################################################

all:

rootfs:
	$(call color_print,Creating rootfs for $(MYNAME) ...,green)
	$(call create_install_paths,$(TARGET)/htdocs)

install:
	$(call color_print,Installing $(MYNAME) ...,green)
	$(call install_dirs,./,$(TARGET)/htdocs,$(HTDOCS))
	$(call install_dirs,./,$(TARGET)/htdocs,$(SUBDIRS))
	$(Q)find $(TARGET)/htdocs -name .svn -type d | xargs rm -rf
	$(call install_files,./,$(TARGET)/etc/config,lld2d.ico)

clean:

.PHONY: all rootfs install clean
