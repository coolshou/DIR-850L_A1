# vi: set sw=4 ts=4:

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "WRGAC05/Aries/rc"
INIT    := S40event.sh S41inf.sh S92fastroute.sh
INIT0    := S12usb_storage.sh
IDIR    := $(TARGET)/etc/init.d
###########################################################################
all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(INIT); do \
	cp --remove-destination init0.$$i $(TARGET)/etc/init0.d/$$i; \
	chmod +x $(TARGET)/etc/init0.d/$$i; \
	done
	for i in $(INIT0); do cp init.$$i $(IDIR)/$$i; chmod +x $(IDIR)/$$i; done

clean:

.PHONY: all rootfs install clean
