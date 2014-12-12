# vi: set sw=4 ts=4:
#############################################################################
-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk
#############################################################################

MYNAME	:= "WRGND10/ARIES/RC"
#INIT	:= S10init.sh S19devfs.sh S20interfaces.sh S45rtstad.sh S21rdm.sh
INIT	:= S10init.sh S19devfs.sh S20interfaces.sh S21rdm.sh

ifeq ($(strip $(ELBOX_PROGS_PRIV_KDBG_AGENT)),y)
INIT +=  S11kdbg_agent.sh
endif

#INIT0	:= S20phyinf.sh S21layout.sh S21wlan.sh S40gpioevent.sh S80telnetd.sh S41event.sh
INIT0	:= S21layout.sh S51wlan.sh S52wlan.sh S40gpioevent.sh S80telnetd.sh S41event.sh

# It is workaround for ralink wlan issue, It seems 5g depends on 2.4g's somethings.
# If 2.4g never up, 5g will not work. When device booting, we need to up 2.4g and
# down it immediately. After this issue is fixed, remove this script.
INIT0	+= S53wlan_ra_issue.sh

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
