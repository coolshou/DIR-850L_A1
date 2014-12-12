# 

-include ../../../path.mk
-include ../../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME  := "ARIES/HTDOCS/PHPLIB/GETCFG"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
TDIR	:= $(TARGET)/htdocs/webinc/getcfg
SUBDIRS	:= 
FILES	:=
REMOVES	:=

ifeq ($(ELBOX_TEMPLATE_ARIES_NET_SNMP_5_5),y)
FILES += SNMPD
endif 

ifeq ($(ELBOX_TEMPLATE_ARIES_ADVANCED_LAYOUT),y)
FILES += VLAN PHYINF.INF
endif

ifeq ($(ELBOX_USE_IPV6),y)
ifneq ($(ELBOX_PROGS_GPL_QUAGGA_UNIFIED),y)
FILES += ROUTE6.DYNAMIC
endif
FILES += DHCPS6.INF ROUTE6.STATIC \
		DHCPS6.BRIDGE-1 DHCPS6.LAN-1 DHCPS6.LAN-2 DHCPS6.LAN-3 DHCPS6.LAN-4 DEVICE.RDNSS RUNTIME.ROUTE.DYNAMIC
endif

ifeq ($(ELBOX_TEMPLATE_ARIES_UNIFIED_DNS),y)
FILES	+= DNS.INF
REMOVES	+= DNS4.INF
endif

ifeq ($(ELBOX_PROGS_PRIV_FILTER),y)
FILES += FILTER.BRIDGE-1
endif

ifeq ($(ELBOX_TEMPLATE_ARIES_STORAGE_SUPPORT),y)
FILES += DEVICE.ACCOUNT RUNTIME.STORAGE
endif

ifeq ($(ELBOX_PROGS_GPL_QUAGGA_UNIFIED),y)
FILES += RUNTIME.ROUTE.DYNAMIC ROUTE.DYNAMIC 
endif

ifeq ($(ELBOX_PROGS_PRIV_CAPWAP),y)
FILES += CAPWAP
endif
###########################################################################
all:
	@echo -e "\033[32mBuilding applets for $(MYNAME) ...\033[0m"
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i; done

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TDIR) ] || mkdir -p $(TDIR)
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i rootfs; done

install:
	@echo -e "\033[32mInstalling applets for $(MYNAME) ...\033[0m"
	$(Q)for i in $(FILES); do cp -r --remove-destination $$i.xml.php $(TDIR)/. ; done
	$(Q)for i in $(REMOVES); do rm -f $(TDIR)/$$i.xml.php ; done

.PHONY: all rootfs install
###########################################################################
