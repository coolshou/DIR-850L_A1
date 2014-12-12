# vi: set sw=4 ts=4:

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/htdocs"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
HTDOCS	:= web 
SUBDIRS	:= phplib webinc
EXTDIRS	:= ext.phplib ext.web ext.webinc

###########################################################################

ifeq ($(ELBOX_ARIES_FEATURE_ROUTER_GATEWAY),y)
HAS_UPNP	:=y
HAS_UPNP_IGD:=y
HAS_UPNP_WFA:=y
endif

ifeq ($(ELBOX_ARIES_FEATURE_BRIDGE_AP),y)
HAS_UPNP	:=y
HAS_UPNP_WFA:=y
endif

ifeq ($(CONFIG_CGIBIN_ARIES_HNAP),y)
HAS_HNAP	:=y
EXTDIRS		+= ext.hnap
endif

ifeq ($(ELBOX_PROGS_PRIV_WIDGET),y)
HAS_WIDGET    :=y
endif

ifeq ($(ELBOX_PROGS_PRIV_NEAPS_ARIES),y)
HAS_NEAP    :=y
endif
#To avoid the smart404 web page present in some condition.
#ifeq ($(ELBOX_PROGS_PRIV_SMART404),y)
#SUBDIRS  += smart404
#endif
###########################################################################

all:
	$(Q)for i in $(EXTDIRS); do $(MYMAKE) -C $$i ; done

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)for i in $(HTDOCS) ; do \
		[ -d $(TARGET)/htdocs/$$i ] || mkdir -p $(TARGET)/htdocs/$$i ; \
		ln -sf /var/htdocs/$$i $(TARGET)/htdocs/$$i/docs ; \
		done
	$(Q)for i in $(EXTDIRS); do $(MYMAKE) -C $$i rootfs; done

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(HTDOCS); do cp -r --remove-destination $$i $(TARGET)/htdocs/. ; done
	$(Q)for i in $(SUBDIRS); do cp -r --remove-destination $$i $(TARGET)/htdocs/. ; done
	$(Q)make upnpd
	$(Q)make hnap
	$(Q)make widget
	$(Q)make neap
	$(Q)for i in $(EXTDIRS); do $(MYMAKE) -C $$i install; done

###########################################################################
upnpd:
ifeq ($(HAS_UPNP),y)
	$(Q)[ -d $(TARGET)/htdocs/upnp ] || mkdir -p $(TARGET)/htdocs/upnp
	$(Q)cp -r --remove-destination upnpinc $(TARGET)/htdocs/.
	$(Q)ln -sf /var/htdocs/upnp $(TARGET)/htdocs/upnp/docs
	$(Q)cp --remove-destination upnp/*.php $(TARGET)/htdocs/upnp/.
	$(Q)make upnpigd
	$(Q)make upnpwfa
	$(Q)chmod +x $(TARGET)/htdocs/upnp/*.sh
endif

upnpigd:
ifeq ($(HAS_UPNP_IGD),y)
	@echo -e "\033[32mInstalling upnpigd ...\033[0m"
	$(Q)cp --remove-destination upnp/igd/*/*								$(TARGET)/htdocs/upnp/.
	$(Q)cp --remove-destination upnpinc/igd/*/*								$(TARGET)/htdocs/upnpinc/.
	$(Q)[ -d $(TARGET)/htdocs/upnpdevdesc ] || mkdir -p $(TARGET)/htdocs/upnpdevdesc
	$(Q)cp --remove-destination upnpdevdesc/InternetGatewayDevice.xml.php	$(TARGET)/htdocs/upnpdevdesc/.
	$(Q)cp --remove-destination upnpdevdesc/Layer3Forwarding.xml.php		$(TARGET)/htdocs/upnpdevdesc/.
	$(Q)cp --remove-destination upnpdevdesc/OSInfo.xml.php					$(TARGET)/htdocs/upnpdevdesc/.
	$(Q)cp --remove-destination upnpdevdesc/WANCommonInterfaceConfig.xml.php $(TARGET)/htdocs/upnpdevdesc/.
	$(Q)cp --remove-destination upnpdevdesc/WANEthernetLinkConfig.xml.php	$(TARGET)/htdocs/upnpdevdesc/.
	$(Q)cp --remove-destination upnpdevdesc/WANIPConnection.xml.php			$(TARGET)/htdocs/upnpdevdesc/.
endif

upnpwfa:
ifeq ($(HAS_UPNP_WFA),y)
	@echo -e "\033[32mInstalling upnpwfa ...\033[0m"
	$(Q)cp --remove-destination upnp/wfa/*/*						$(TARGET)/htdocs/upnp/.
	$(Q)[ -d $(TARGET)/htdocs/upnpdevdesc ] || mkdir -p $(TARGET)/htdocs/upnpdevdesc
	$(Q)cp --remove-destination upnpdevdesc/WFADevice.xml.php		$(TARGET)/htdocs/upnpdevdesc/.
	$(Q)cp --remove-destination upnpdevdesc/WFAWLANConfig.xml.php	$(TARGET)/htdocs/upnpdevdesc/.
endif

hnap:
ifeq ($(HAS_HNAP),y)
	@echo -e "\033[32mInstalling hnap ...\033[0m"
	$(Q)[ -d $(TARGET)/htdocs/HNAP1 ] || mkdir -p $(TARGET)/htdocs/HNAP1
	$(Q)[ -d $(TARGET)/etc/templates/hnap ] || mkdir -p $(TARGET)/etc/templates/hnap
	$(Q)cp -rf --remove-destination hnap/ $(TARGET)/etc/templates/
	$(Q)touch $(TARGET)/htdocs/HNAP1/index.hnap
endif

widget:
ifeq ($(HAS_WIDGET),y)
	@echo -e "\033[32mInstalling widget ...\033[0m"
	$(Q)[ -d $(TARGET)/htdocs/widget ] || mkdir -p $(TARGET)/htdocs/widget
	$(Q)cp -rf --remove-destination widget/ $(TARGET)/htdocs
endif

neap:
ifeq ($(HAS_NEAP),y)
	@echo -e "\033[32mInstalling neap ...\033[0m"
	$(Q)[ -d $(TARGET)/htdocs/neap ] || mkdir -p $(TARGET)/htdocs/neap
	$(Q)cp --remove-destination neap/NEAP_* $(TARGET)/htdocs/neap
	$(Q)make -C ./neap install 
endif
clean:

gpl:
	mv Makefile.gpl Makefile

.PHONY: all install upnpigd upnpwfa clean hnap neap widget gpl
