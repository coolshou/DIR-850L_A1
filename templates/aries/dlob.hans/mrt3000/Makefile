.EXPORT_ALL_VARIABLES:

# vi: set sw=4 ts=4:

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

#####################################################################

MYNAME	:= "Aries/Dlob/mrt3000"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
SUBDIRS := defnodes htdocs udev.rules wifi services scripts events rc
DLOB_WEB_FILES := $(shell ls $(TOPDIR)/templates/aries/dlob.hans/htdocs/web)
DLOB_WEBINC_BODY := $(shell ls $(TOPDIR)/templates/aries/dlob.hans/htdocs/webinc/body)
DLOB_WEBINC_JS := $(shell ls $(TOPDIR)/templates/aries/dlob.hans/htdocs/webinc/js)
ifneq ($(strip $(ELBOX_CREATE_LANGUAGE_PACK)),y)
#SUBDIRS += locale
endif

all:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i; done

rootfs:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i rootfs; done

install:
	$(Q)for i in $(DLOB_WEB_FILES); do rm $(TOPDIR)/rootfs/htdocs/web/$$i; done
	$(Q)for i in $(DLOB_WEBINC_BODY); do rm $(TOPDIR)/rootfs/htdocs/webinc/body/$$i; done
	$(Q)for i in $(DLOB_WEBINC_JS); do rm $(TOPDIR)/rootfs/htdocs/webinc/js/$$i; done
	$(Q)rm -rf $(TOPDIR)/rootfs/htdocs/webinc/help
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i install; done
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/webinc/getcfg/SCHEDULE.xml.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/setcfg/SCHEDULE.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/fatlady/SCHEDULE.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/webinc/getcfg/DDNS6.WAN-1.xml.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/setcfg/DDNS6.WAN-1.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/fatlady/DDNS6.WAN-1.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/DDNS6.WAN-1.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/web/wiz_wps.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/webinc/js/wiz_wps.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/webinc/body/wiz_wps.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/OPENDNS4.MAP.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/OPENDNS4.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/fatlady/OPENDNS4.MAP.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/fatlady/OPENDNS4.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/setcfg/OPENDNS4.MAP.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/setcfg/OPENDNS4.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/webinc/getcfg/OPENDNS4.xml.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/webinc/getcfg/OPENDNS4.MAP.xml.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/fatlady/QOS.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/setcfg/QOS.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/fatlady/BWC.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/setcfg/BWC.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/getcfg/BWC.xml.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/fatlady/SCHEDULE.php
	$(Q)rm -f $(TOPDIR)/rootfs/htdocs/phplib/setcfg/SCHEDULE.php
	$(Q)rm -rf $(TOPDIR)/rootfs/htdocs/parentalcontrols
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/BWC.LAN-1.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/BWC.LAN-2.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/BWC.LAN-3.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/BWC.LAN-4.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/BWC.WAN-1.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/BWC.WAN-2.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/BWC.WAN-3.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/BWC.WAN-4.php
	$(Q)rm -f $(TOPDIR)/rootfs/etc/services/BWC.php
	$(Q)rm -rf $(TOPDIR)/rootfs/htdocs/phplib/fatlady/BWC

clean:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i clean; done

test:
	echo $(DLOB_WEBINC_FILES)

.PHONY: all rootfs install clean
