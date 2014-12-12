# vi: set sw=4 ts=4:

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################
MYNAME  		:= "aries/dlob.hans/mydlink/scripts"
SHELL_FILES		:= userlookup.sh dns_log.sh fw_mail_check.sh mdb_selftest.sh
PHP_FILES		:= mdb.php useradd.php MYDLINKMAIL.php
DIRS			:= 
###########################################################################

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/scripts/mydlink ] || mkdir -p $(TARGET)/etc/scripts/mydlink

install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(SHELL_FILES); do \
		cp --remove-destination $$i $(TARGET)/etc/scripts/mydlink/.; \
		chmod +x $(TARGET)/etc/scripts/mydlink/$$i; \
	done
	$(Q)for i in $(PHP_FILES); do \
		cp --remove-destination $$i $(TARGET)/etc/scripts/mydlink/.; \
	done
	$(Q)for i in $(DIRS); do \
		[ -d $(TARGET)/etc/scripts/mydlink/$$i ] || mkdir -p $(TARGET)/etc/scripts/mydlink/$$i ; \
		cp --remove-destination $$i/* $(TARGET)/etc/scripts/mydlink/$$i/. ; \
	done	

clean:
.PHONY: all rootfs install clean

