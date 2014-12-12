# vi: set sw=4 ts=4:

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

#####################################################################
MYNAME	:= "aries/dlob.hans/mydlink"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
SUBDIRS := htdocs scripts services events
#####################################################################

all:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i; done
	
rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i rootfs; done
	
install: install_utilities install_files install_api
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i install; done
		
install_utilities:
	echo -e "\033[32mInstall mdb ...\033[0m"
	$(Q)cp mdb $(TARGET)/usr/sbin/.
	chmod +x $(TARGET)/usr/sbin/mdb
	$(Q)cp factory_reset $(TARGET)/usr/sbin/.
	chmod +x $(TARGET)/usr/sbin/factory_reset
	$(Q)cp fw_upgrade $(TARGET)/usr/sbin/.
	chmod +x $(TARGET)/usr/sbin/fw_upgrade
	
install_files:
	$(Q)cp S22mydlink.sh $(TARGET)/etc/init.d/.
	chmod +x $(TARGET)/etc/init.d/S22mydlink.sh
	$(Q)cp S43mydlinkevent.sh $(TARGET)/etc/init0.d/.
	chmod +x $(TARGET)/etc/init0.d/S43mydlinkevent.sh
	
	$(Q)cp profile $(TARGET)/etc/
	[ -d $(TARGET)/etc/lang ] & rm -rf $(TARGET)/etc/lang
	$(Q)cp -r lang $(TARGET)/etc/
	$(Q)cd $(TARGET)/etc/lang;\
	ln -s tw zhtw;\
	ln -s cn zhcn;\
	ln -s cz cs
	
install_api:
	$(Q)[ -d $(TARGET)/htdocs/mydlink ] || mkdir -p $(TARGET)/htdocs/mydlink
	$(Q)cp --remove-destination api/* $(TARGET)/htdocs/mydlink/.
	
clean:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i clean; done
	
.PHONY: all rootfs install install_mdb install_files install_api clean
