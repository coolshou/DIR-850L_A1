# vi: set sw=4 ts=4:
###########################################################################
-include ../../../path.mk
-include ../../../arch.mk
-include $(TOPDIR)/.config
###########################################################################

MYNAME := "WRGAC05/TAURUS/SCRIPTS/WIFI"
FILES  :=
DIRS   :=
ifeq ($(strip $(ELBOX_NETWORK_WIFI_2.4G)),y)
FILES  += wifirnodes.php
endif
###########################################################################
# script for hostapd wps
ifeq ($(strip $(ELBOX_PROGS_PRIV_HOSTAPD_WPS)),y)
FILES  += wps.sh wpsevents.php restartap_hostapd.sh
DIRS   += wps 
endif
###########################################################################
# helper scripts for rtstad
ifeq ($(strip $(ELBOX_INSTALL_REALTEK_UDPSERVER)),y)
FILES  += mp.sh 
endif
ifeq ($(strip $(ELBOX_NETWORK_WIFI_2.4G)),y)
FILES  += upwifistatshlper_G_band.sh
endif
ifeq ($(strip $(ELBOX_NETWORK_WIFI_2.4G)),y)
FILES  += upwifistatshlper_A_band.sh
endif
###########################################################################

all:

rootfs:
	@echo -e "\033[32mCreating scripts for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/scripts ] || mkdir -p $(TARGET)/etc/scripts
	$(Q)for i in $(DIRS); do make -C $$i rootfs; done

install:
	@echo -e "\033[32mInstalling scripts for $(MYNAME) ...\033[0m"
	$(Q)for i in $(FILES); do \
		cp --remove-destination $$i $(TARGET)/etc/scripts/.; \
		chmod +x $(TARGET)/etc/scripts/$$i; \
	done
	$(Q)cp wlanapp.sh $(TARGET)/bin/.
	$(Q)for i in $(DIRS); do make -C $$i install; done

clean:

.PHONY: all rootfs install clean
