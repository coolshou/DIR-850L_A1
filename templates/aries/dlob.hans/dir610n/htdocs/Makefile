# vi: set sw=4 ts=4:

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/Dlob/dir610/htdocs"
HTDOCS	:= webinc web phplib
SUBDIRS	:=

###########################################################################

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/htdocs ] || mkdir -p $(TARGET)/htdocs

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(HTDOCS); do cp -r --remove-destination $$i $(TARGET)/htdocs/. ; done
	$(Q)for i in $(SUBDIRS); do cp -r --remove-destination $$i $(TARGET)/htdocs/. ; done
	$(Q)find $(TARGET)/htdocs -name .svn -type d | xargs rm -rf
	$(Q)cp lld2d.ico $(TARGET)/etc/config/.

clean:

.PHONY: all rootfs install clean
