# vi: set sw=4 ts=4:
#
# Makefile for RC scripts of Aries.
#

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/rc"
INIT	:= S19init.sh S20init.sh S45gpiod.sh
INIT0	:= 

###########################################################################

ifeq ($(ELBOX_ARIES_USE_UDEV),y)
INIT	+= S15udevd.sh S23udevd.sh
else
INIT	+= S15pts.sh
endif

ifeq ($(ELBOX_USE_IPV6),y)
INIT	+= S16ipv6.sh
endif

ifeq ($(ELBOX_ARIES_FEATURE_ROUTER_GATEWAY),y)
INIT0	+= S40event.sh S41inf.sh S42pthrough.sh S65ddnsd.sh S41autowan.sh S65logd.sh
ifeq ($(ELBOX_USE_IPV6),y)
INIT0	+= S41autowanv6.sh
endif
endif

ifeq ($(ELBOX_ARIES_FEATURE_BRIDGE_AP),y)
INIT0	+= S40event.br.sh S41inf.br.sh
endif

ifeq ($(ELBOX_PROGS_PRIV_SMART404),y)
INIT0	+= S41smart404.sh
endif

ifeq ($(ELBOX_TEMPLATE_ARIES_NET_SNMP_5_5),y)
INIT0   += S65snmp.sh
endif

ifeq ($(ELBOX_TEMPLATE_ARIES_ENABLE_USER_MANAGEMENT),y)
INIT0	+= S65user.sh
endif

ifeq ($(ELBOX_PROGS_PRIV_UPNP_AV),y)
INIT0   += S42event.sh S90upnpav.sh
endif

ifeq ($(ELBOX_MINIDLNA),y)
INIT0   += S42event.sh S90upnpav.sh
endif

ifeq ($(ELBOX_PROGS_PRIV_WEB_FILE_ACCESS),y)
INIT0   += S90upnpav.sh
endif

ifeq ($(ELBOX_PROGS_PRIV_SHAREPORT_PLUS),y)
INIT0   += S60shareport.sh
endif

#ifeq ($(ELBOX_TEMPLATE_ARIES_MYDLINK_SUPPORT),y)
#INIT0	+= S69mydlink.sh
#endif
###########################################################################

define Install/S21usbmount.sh
	@echo -e "\033[32mCreating S21usbmount.sh ...\033[0m"
	@echo "#!/bin/sh" > $(TARGET)/etc/init.d/S21usbmount.sh
	@echo "mkdir -p $(ELBOX_PROGS_PRIV_USBMOUNT_MNT_ROOT)" >> $(TARGET)/etc/init.d/S21usbmount.sh
	@chmod +x $(TARGET)/etc/init.d/S21usbmount.sh
	@echo "$(ELBOX_PROGS_PRIV_USBMOUNT_MNT_ROOT)" > $(TARGET)/etc/config/usbmount
endef

###########################################################################

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/init.d ] || mkdir -p $(TARGET)/etc/init.d
	$(Q)[ -d $(TARGET)/etc/init0.d ] || mkdir -p $(TARGET)/etc/init0.d
	$(Q)[ -d $(TARGET)/usr/sbin ] || mkdir -p $(TARGET)/usr/sbin
	$(Q)cp --remove-destination rcS	$(TARGET)/etc/init.d/rcS
	$(Q)cp --remove-destination rcS0 $(TARGET)/etc/init0.d/rcS
	$(Q)cp --remove-destination phpsh $(TARGET)/usr/sbin/.
	$(Q)ln -sf /var/etc/iproute2 $(TARGET)/etc/iproute2

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(INIT); do \
		cp --remove-destination init.$$i $(TARGET)/etc/init.d/$$i; \
		chmod +x $(TARGET)/etc/init.d/$$i; \
		done
	$(Q)for i in $(INIT0); do \
		cp --remove-destination init0.$$i $(TARGET)/etc/init0.d/$$i; \
		chmod +x $(TARGET)/etc/init0.d/$$i; \
		done
	$(if $(ELBOX_PROGS_PRIV_USBMOUNT),$(call Install/S21usbmount.sh),)

clean:

.PHONY: all rootfs install clean
