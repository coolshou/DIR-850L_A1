# vi: set sw=4 ts=4:

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/Dlob/mrt3000/htdocs"
HTDOCS	:= webinc phplib web
SUBDIRS	:=

###########################################################################

ifeq ($(ELBOX_PROGS_PRIV_MIIICASA) , y)
HTDOCS += miiicasa web
endif 

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/htdocs ] || mkdir -p $(TARGET)/htdocs

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(HTDOCS); do cp -r --remove-destination $$i $(TARGET)/htdocs/. ; done
	$(Q)for i in $(SUBDIRS); do cp -r --remove-destination $$i $(TARGET)/htdocs/. ; done
	$(Q)find $(TARGET)/htdocs -name .svn -type d | xargs rm -rf
	$(Q)cp ./pic/lld2d.ico $(TARGET)/etc/config/.
	$(Q)rm -rf $(TARGET)/htdocs/web/fileaccess

	$(Q)rm -f $(TARGET)/htdocs/web/adv_wps.php
	$(Q)rm -f $(TARGET)/htdocs/web/adv_qos.php
	$(Q)rm -f $(TARGET)/htdocs/web/adv_web_filter.php
	$(Q)rm -f $(TARGET)/htdocs/webinc/js/adv_wps.php
	$(Q)rm -f $(TARGET)/htdocs/webinc/js/adv_qos.php
	$(Q)rm -f $(TARGET)/htdocs/webinc/js/adv_web_filter.php
	$(Q)rm -f $(TARGET)/htdocs/webinc/body/adv_wps.php
	$(Q)rm -f $(TARGET)/htdocs/webinc/body/adv_qos.php
	$(Q)rm -f $(TARGET)/htdocs/webinc/body/adv_web_filter.php

clean:

.PHONY: all rootfs install clean
