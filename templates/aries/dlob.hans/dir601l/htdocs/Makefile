# vi: set sw=4 ts=4:

include ../../path.mk
include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/Dlob/dir601l/htdocs"
HTDOCS	:= webinc web phplib
SUBDIRS	:=
DONT_NEED_FILES := adv_dlna.php adv_gzone.php adv_itunes.php adv_wlan_wifitest.php bsc_media_server.php bsc_sms_inbox.php bsc_sms_inbox_rlt.php bsc_sms.php bsc_sms_send.php bsc_wlan_wifitest.php dlna_refresh.php dlnastate.php shareport.php st_missed_call.php 

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
	$(Q)for i in $(DONT_NEED_FILES); do find $(TARGET)/htdocs -name $$i -type f | xargs rm -f; done

clean:

.PHONY: all rootfs install clean
