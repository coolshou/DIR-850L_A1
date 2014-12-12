# vi: set sw=4 ts=4:

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/Dlob/htdocs"
HTDOCS	:= web webinc
SUBDIRS	:= phplib neap widget parentalcontrols

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
ifeq ($(strip $(CONFIG_RGBIN_USB3G_KIT)),y)
	$(Q)cp -r --remove-destination isplst.php $(TARGET)/htdocs/$(SUBDIRS)/.
endif
ifeq ($(ELBOX_PROGS_GPL_UPNPC_IGD_FORWARDER),y)
	$(Q)cp -f upnpinc/igd/WANIPConn1/*	$(TARGET)/htdocs/upnpinc/.
#	$(Q)cp -f upnpinc/js/*				$(TARGET)/htdocs/webinc/js/.
endif
clean:

.PHONY: all rootfs install clean
