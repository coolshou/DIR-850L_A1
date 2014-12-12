# vi: set sw=4 ts=4:
#############################################################################
-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk
#############################################################################

MYNAME	:= "WRGN78/ARIES/RC"
INIT	:= S10init.sh S16ipv6.sh S19devfs.sh
INIT0	:= S21layout.sh S40gpioevent.sh S51wlan.sh S80telnetd.sh
IDIR	:= $(TARGET)/etc/init.d
I0DIR	:= $(TARGET)/etc/init0.d
#############################################################################

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(IDIR) ] || mkdir -p $(IDIR)
	$(Q)[ -d $(I0DIR) ] || mkdir -p $(I0DIR)

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	for i in $(INIT); do cp init.$$i $(IDIR)/$$i; chmod +x $(IDIR)/$$i; done
	for i in $(INIT0); do cp init0.$$i $(I0DIR)/$$i; chmod +x $(I0DIR)/$$i; done

clean:

.PHONY: all rootfs install clean
