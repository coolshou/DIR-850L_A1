# vi: set sw=4 ts=4:
###########################################################################
-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
###########################################################################

MYNAME	:= "WRGND10/ARIES/EVENTS"
FILES	:=

###########################################################################
all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/events ] || mkdir -p $(TARGET)/etc/events

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(FILES); do cp $$i $(TARGET)/etc/events/.; done

clean:

.PHONY: all rootfs install clean
