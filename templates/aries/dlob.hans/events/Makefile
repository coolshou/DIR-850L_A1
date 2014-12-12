# vi: set sw=4 ts=4:

include ../path.mk
include ../arch.mk
include $(TOPDIR)/.config
include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/Dlob/events"
FILES	:= checkfw.sh UPDATERESOLV.php SENDMAIL.php WAN-DETECT.php \
			WAN_ppp_dis.php WAN_dhcp_dis.php WAN_dhcp_dis.sh wan_detect_chk.sh wan_detect_chk.php UPDATELEASES.php \
			FWUPDATER.sh
			

###########################################################################

###########################################################################
all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/events ] || mkdir -p $(TARGET)/etc/events

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(FILES); do \
			cp --remove-destination $$i $(TARGET)/etc/events/$$i; \
			chmod +x $(TARGET)/etc/events/$$i; \
		done
ifeq ($(ELBOX_PROGS_GPL_EMAILCLIENT),y)
	$(Q)cp --remove-destination ./SENDMAIL_GPL.php $(TARGET)/etc/events/SENDMAIL.php;
endif

clean:

.PHONY: all rootfs install clean
