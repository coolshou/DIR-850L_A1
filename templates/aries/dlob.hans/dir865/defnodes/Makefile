# vi: set sw=4 ts=4:

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME  := "defnodes"
XMLFILES:= $(wildcard *.xml)
PHPFILES:= $(wildcard *.php)
SCRIPTFILES:= $(wildcard *.sh)

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/defnodes ] || mkdir -p $(TARGET)/etc/defnodes

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(XMLFILES); do \
		sed -e 's|SiGnAtUrE|$(ELBOX_CONFIG_SIGNATURE)|' $$i > $(TARGET)/etc/defnodes/$$i ; done
	$(Q)for i in $(PHPFILES); do cp $$i $(TARGET)/etc/defnodes/$$i; done 
	$(Q)for i in $(SCRIPTFILES); do cp $$i $(TARGET)/etc/defnodes/$$i; done

clean:

.PHONY: all rootfs install clean
