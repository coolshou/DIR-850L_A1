# vi: set sw=4 ts=4:

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/scripts"

SCRIPTFILES	:=	dbload.sh dbsave.sh dlcfg_hlper.sh \
				prefwupdate.sh settime.sh setdate.sh killpid.sh \
				conntrack_flush.sh delpathbytarget.sh delpathbytarget.php \
				IPV4.INET.php dlcfg_hlper.sh dlcfg_hlper.php IPV4.DSLITE.php \
				SETVPNSRRT.php vpnroute.php

DIRS		:=	libs
SCRIPTDIRS	:=

###########################################################################
ifeq ($(strip $(ELBOX_PROGS_PRIV_WCN_UFD_v_2_0)),y)
SCRIPTDIRS  +=  wifi
endif
ifeq ($(ELBOX_ARIES_FEATURE_ROUTER_GATEWAY),y)
SCRIPTFILES	+=	ip-up ip-down ppp-status dns-helper.sh \
                igmpproxy_helper.sh getmodem.sh adapter_cmd.php \
				adapter_cmd.sh ddnsd_helper.sh
SCRIPTDIRS	+=	upnp

ifeq ($(ELBOX_USE_IPV6),y)
SCRIPTFILES	+=	ipv6-up ipv6-down IPV6.INET.php RA-WAIT.php V6IN4-TUNNEL.php IPV6.CHILD.php stopchild.php dhcp6s_helper.php IP-WAIT.php mldproxy_helper.sh ENULA.php CABLE-RA-WAIT.php ppp6-status
endif
endif

ifeq ($(ELBOX_ARIES_FEATURE_BRIDGE_AP),y)
SCRIPTFILES	+=
SCRIPTDIRS	+=	upnp
endif

ifeq ($(ELBOX_ARIES_USE_BACKUP_CONNECTION),y)
SCRIPTFILES	+= backupconn.sh backupconn.php
endif

ifeq ($(ELBOX_PROGS_PRIV_STARSPEED),y)
SCRIPTFILES += starspeed.sh
endif

ifeq ($(ELBOX_PROGS_PRIV_SMART404),y)
SCRIPTFILES += control_smart404.php
endif

ifeq ($(ELBOX_PROGS_PRIV_USBMOUNT),y)
SCRIPTFILES += usbmount_helper.php usbmount_helper.sh usbmount_fsid.sh
endif

ifeq ($(ELBOX_PROGS_GPL_QUAGGA),y)
SCRIPTFILES += route_dynamic_helper.php route_dynamic_helper.sh
endif

###########################################################################
INSSH	:= $(TARGET)/etc/scripts/iptables_insmod.sh
RMSH	:= $(TARGET)/etc/scripts/iptables_rmmod.sh
###########################################################################

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/scripts ] || mkdir -p $(TARGET)/etc/scripts

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
ifeq ($(ELBOX_TEMPLATE_ARIES_PHPCONTROL),y)
	$(Q)[ -d $(TARGET)/etc/scripts/php_cmd/cmd ] || mkdir -p $(TARGET)/etc/scripts/php_cmd/cmd
	$(Q)cp -f $(TOPDIR)/templates/aries/progs/scripts/php_cmd/cmd/*.php $(TARGET)/etc/scripts/php_cmd/cmd
	$(Q)cp -f $(TOPDIR)/templates/aries/progs/scripts/php_cmd/*.php $(TARGET)/etc/scripts/php_cmd/
endif
	$(Q)for i in $(SCRIPTFILES); do \
			cp $$i --remove-destination $(TARGET)/etc/scripts/$$i; \
			chmod +x $(TARGET)/etc/scripts/$$i; \
		done
	$(Q)for i in $(DIRS); do \
		[ -d $(TARGET)/etc/scripts/$$i ] || mkdir -p $(TARGET)/etc/scripts/$$i ; \
		cp --remove-destination $$i/* $(TARGET)/etc/scripts/$$i/. ; \
	done
	$(Q)for i in $(SCRIPTDIRS); do \
		[ -d $(TARGET)/etc/scripts/$$i ] || mkdir -p $(TARGET)/etc/scripts/$$i ; \
		cp --remove-destination $$i/* $(TARGET)/etc/scripts/$$i/. ; \
		chmod +x $(TARGET)/etc/scripts/$$i/*.sh ; \
	done
ifeq ($(ELBOX_ARIES_FEATURE_ROUTER_GATEWAY),y)
	$(Q)make iptables_modules V=$(V)
endif
#if have config version control we change the dbload.sh
ifeq ($(strip $(ELBOX_XMLDB_VERSION_CONTROL)),y)
	cp dbload.ver.sh --remove-destination $(TARGET)/etc/scripts/dbload.sh
endif		

ifeq ($(ELBOX_PROGS_PRIV_SMART404),y)
	cp dbsave.smart404.sh --remove-destination $(TARGET)/etc/scripts/dbsave.sh
endif



iptables_modules:
	@echo -e "\033[32mCreating iptables modules scripts for $(MYNAME) ...\033[0m"
	$(Q)rm -f $(INSSH) $(RMSH)
	$(Q)echo "#!/bin/sh"	> $(INSSH)
	$(Q)echo "#!/bin/sh"	> $(RMSH)
ifeq ($(strip $(ELBOX_STUN_NAT)),y)
	$(Q)echo "insmod /lib/modules/ip_stun_func.ko"	>> $(INSSH)
	$(Q)echo "rmmod ip_stun_func"					>> $(RMSH)
endif
ifeq ($(strip $(ELBOX_TURBONAT)),y)
	$(Q)echo "insmod /lib/modules/sw_tcpip.ko"		>> $(INSSH)
	$(Q)echo "rmmod sw_tcpip"						>> $(RMSH)
endif
ifeq ($(strip $(ELBOX_IFRESETCNT)),y)
	$(Q)echo "insmod /lib/modules/ifresetcnt.ko"	>> $(INSSH)
	$(Q)echo "rmmod ifresetcnt"						>> $(RMSH)
endif
	$(Q)chmod +x $(INSSH) $(RMSH)

clean:

.PHONY: all rootfs install clean
