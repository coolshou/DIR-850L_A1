# vi: set sw=4 ts=4:
#############################################################################
-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk
#############################################################################

MYNAME := $(ELBOX_BOARD_NAME)/$(ELBOX_TEMPLATE_NAME)/rc
INIT   := S20gpio.sh
INIT0  :=
ifeq ($(strip $(ELBOX_NETWORK)),y)
INIT0  += S21layout.sh
endif
ifeq ($(strip $(ELBOX_PROGS_PRIV_SERVD)),y)
INIT0  += S40gpioevent.sh
endif
ifeq ($(strip $(ELBOX_NETWORK_WIFI)),y)
INIT0  += S51wlan.sh
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_TELNETD)),y)
INIT0  += S80telnetd.sh
endif
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
	$(Q)for i in $(INIT); do cp init.$$i $(IDIR)/$$i; chmod +x $(IDIR)/$$i; done
	$(Q)for i in $(INIT0); do cp init0.$$i $(I0DIR)/$$i; chmod +x $(I0DIR)/$$i; done

clean:

.PHONY: all rootfs install clean
