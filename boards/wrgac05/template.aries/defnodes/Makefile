# vi: set sw=4 ts=4:
###########################################################################
-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk
###########################################################################

MYNAME	:= "WRGAC05/ARIES/DEFNODES"
XMLFILES:= $(wildcard *.xml)
PHPFILES:= S12flashspeed.php S14setchlist.php
SHFILES	:= $(wildcard *.sh)

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/defnodes ] || mkdir -p $(TARGET)/etc/defnodes

install:
	@echo -e "\033[32mInstalling applets for $(MYNAME) ...\033[0m"
	$(Q)for i in $(XMLFILES); do \
		sed -e 's|SiGnAtUrE|$(ELBOX_SIGNATURE)|' $$i > $(TARGET)/etc/defnodes/$$i ; done
	$(Q)for i in $(PHPFILES); do cp $$i $(TARGET)/etc/defnodes/$$i; done
	$(Q)for i in $(SHFILES);  do cp $$i $(TARGET)/etc/defnodes/$$i; done

clean:

.PHONY: all rootfs install clean
