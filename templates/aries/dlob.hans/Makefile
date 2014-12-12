# vi: set sw=4 ts=4:

include path.mk
include arch.mk
include $(TOPDIR)/.config
include $(TOPDIR)/Vars.mk

#####################################################################
# targets for main Makefile

MYNAME	:= "Aries/Dlob"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
SUBDIRS	:= defnodes htdocs scripts services events init0.d locale

ifeq ($(strip $(ELBOX_TEMPLATE_ARIES_MYDLINK_SUPPORT)),y)
SUBDIRS += mydlink
endif

ifeq ($(strip $(ELBOX_CREATE_LANGUAGE_PACK)),y)
LTARGET	:= $(TOPDIR)/language
else
LTARGET	:= $(TARGET)/www/locale
endif

ifneq ($(strip $(ELBOX_PROGS_GPL_DNSMASQ_ADVANCE_DNS_SERVICE_ALPHA)),y)
NO_ADV_DNS :=y
endif

ifneq ($(strip $(ELBOX_PROGS_PRIV_UPNP_AV)),y)
ifneq ($(strip $(ELBOX_PROGS_GPL_ITUNES_SERVER)),y)
NO_UPNP_AV_ITUNES_SERVER :=y
endif
endif

ifneq ($(strip $(ELBOX_PROGS_PRIV_YAHOO_WIDGET)),y)
ifneq ($(strip $(ELBOX_PROGS_PRIV_WIDGET)),y)
NO_WIDGET :=y
endif
endif
#####################################################################
SUBDIRS	+= $(ELBOX_MODEL_NAME)

all:
	@echo -e "\033[32mBuilding applets for $(MYNAME) ...\033[0m"
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i; done

prepare:
	@echo -e "\033[32mPreparing configuration for $(MYNAME) ...\033[0m"

rootfs:
	@echo -e "\033[32mCreate rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(LTARGET) ] || mkdir -p $(LTARGET)
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i rootfs; done

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i install; done
	$(Q)$(MYMAKE) inst_language
	$(Q)$(MYMAKE) remove_useless_files

clean:
	@echo -e "\033[32mCleaning board related programs for $(MYNAME) ...\033[0m"
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i clean; done

gpl:
	find . -name dir\* | grep -v $(ELBOX_MODEL_NAME) | xargs rm -rf
	mv Makefile.gpl Makefile

.PHONY: all install clean prepare rootfs gpl

#####################################################################

inst_language:
	$(Q)for i in $(LANGUAGE); do \
		echo -e "\033[32mInstall language $$i ...\033[0m" ; \
		cp -r --remove-destination locale/$$i $(LTARGET)/. ; \
		if [ -d $(ELBOX_MODEL_NAME)/locale/$$i ]; then \
			cp -r --remove-destination $(ELBOX_MODEL_NAME)/locale/$$i $(LANG_TARGET)/. ; \
		fi; \
		echo $$i > $(LTARGET)/$$i/langcode ; \
		done
	$(Q)find $(TARGET) -name .svn -type d | xargs rm -rf
	$(Q)find $(TARGET) -name CVS -type d | xargs rm -rf


remove_useless_files: no_parentalcontrols no_upnpav_itunesserver no_webfile_access no_file_access no_ppp_server no_widget

no_parentalcontrols:
ifeq ($(strip $(NO_ADV_DNS)),y)
	@echo -e "\033[32mRemove useless function of parentalcontrols ...\033[0m"
	rm $(TARGET)/htdocs/web/adv_parent_ctrl.php
	rm $(TARGET)/htdocs/web/adv_parent_ctrl_map.php
	rm $(TARGET)/htdocs/web/open_dns.php
	rm $(TARGET)/htdocs/webinc/body/adv_parent_ctrl.php
	rm $(TARGET)/htdocs/webinc/body/adv_parent_ctrl_map.php
	rm $(TARGET)/htdocs/webinc/js/adv_parent_ctrl.php
	rm $(TARGET)/htdocs/webinc/js/adv_parent_ctrl_map.php
	rm $(TARGET)/htdocs/webinc/help/adv_parent_ctrl.php	
	rm $(TARGET)/htdocs/webinc/getcfg/OPENDNS4.xml.php
	rm $(TARGET)/htdocs/webinc/getcfg/OPENDNS4.MAP.xml.php
	rm $(TARGET)/htdocs/phplib/fatlady/OPENDNS4.php
	rm $(TARGET)/htdocs/phplib/fatlady/OPENDNS4.MAP.php
	rm $(TARGET)/htdocs/phplib/setcfg/OPENDNS4.php
	rm $(TARGET)/htdocs/phplib/setcfg/OPENDNS4.MAP.php
	rm $(TARGET)/etc/services/OPENDNS4.php
	rm $(TARGET)/etc/services/OPENDNS4.MAP.php
	rm -rf $(TARGET)/htdocs/parentalcontrols
	rm $(TARGET)/htdocs/web/pic/dlink_utility.jpg
endif

no_upnpav_itunesserver:
ifeq ($(strip $(NO_UPNP_AV_ITUNES_SERVER)),y)
	@echo -e "\033[32mRemove useless folder of web/portal ...\033[0m"
	rm -rf $(TARGET)/htdocs/web/portal
endif

no_webfile_access:
ifneq ($(strip $(ELBOX_PROGS_PRIV_WEB_FILE_ACCESS)),y)
	@echo -e "\033[32mRemove useless folder of web/webaccess ...\033[0m"
	rm -rf $(TARGET)/htdocs/web/webaccess
	rm -f $(TARGET)/htdocs/web/bsc_web_access.php
	rm -f $(TARGET)/htdocs/webinc/body/bsc_web_access.php
	rm -f $(TARGET)/htdocs/webinc/js/bsc_web_access.php
	rm -f $(TARGET)/htdocs/webinc/help/bsc_web_access.php
	rm -f $(TARGET)/htdocs/webinc/getcfg/WEBACCESS.xml.php
	rm -f $(TARGET)/htdocs/phplib/fatlady/WEBACCESS.php
	rm -f $(TARGET)/htdocs/phplib/setcfg/WEBACCESS.php
	rm -f $(TARGET)/etc/services/WEBACCESS.php
	rm -f $(TARGET)/etc/scripts/webaccess_map.php
endif

no_file_access:
ifneq ($(strip $(ELBOX_PROGS_PRIV_FILE_ACCESS)),y)
	@echo -e "\033[32mRemove useless folder of web/fileaccess ...\033[0m"
	rm -rf $(TARGET)/htdocs/web/fileaccess
endif

no_ppp_server:
ifneq ($(strip $(ELBOX_PROGS_GPL_PPTPD)),y)
	@echo -e "\033[32mRemove useless files for pptp or l2tp server ...\033[0m"
	rm -f $(TARGET)/htdocs/web/adv_vpn.php
	rm -f $(TARGET)/htdocs/webinc/body/adv_vpn.php
	rm -f $(TARGET)/htdocs/webinc/js/adv_vpn.php
	rm -f $(TARGET)/htdocs/webinc/help/adv_vpn.php
	rm -f $(TARGET)/htdocs/webinc/getcfg/VPN.xml.php
	rm -f $(TARGET)/htdocs/phplib/fatlady/VPN.php
	rm -f $(TARGET)/htdocs/phplib/setcfg/VPN.php
endif

no_widget:
ifeq ($(strip $(NO_WIDGET)),y)
	@echo -e "\033[32mRemove useless folder and files for widget ...\033[0m"
	rm -rf $(TARGET)/htdocs/widget
	rm -f $(TARGET)/htdocs/web/router_info.xml
	rm -f $(TARGET)/htdocs/web/post_login.xml
endif

.PHONY: inst_language remove_useless_files no_parentalcontrols no_upnpav_itunesserver no_webfile_access no_file_access no_ppp_server no_widget
