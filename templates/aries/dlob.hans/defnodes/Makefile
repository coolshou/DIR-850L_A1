# vi: set sw=4 ts=4:

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME	:= "Aries/Dlob/defnodes"
XMLFILES:= $(wildcard *.xml)
PHPFILES:= S30device.php S31locale.php S33wifi.php default_wifi.php S14setchlist.php S13dfs.php defaultvalue.php

ifeq ($(ELBOX_PROGS_GPL_PPTPD),y)
	PHPFILES += S92vpn.php
endif

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/defnodes ] || mkdir -p $(TARGET)/etc/defnodes

install:
	@echo -e "\033[32mInstalling applets for $(MYNAME) ...\033[0m"
	$(Q)for i in $(XMLFILES); do \
		sed -e 's|SiGnAtUrE|$(ELBOX_CONFIG_SIGNATURE)|' $$i > $(TARGET)/etc/defnodes/$$i ; done
	$(Q)for i in $(PHPFILES); do cp $$i $(TARGET)/etc/defnodes/$$i; done

clean:

.PHONY: all rootfs install clean
