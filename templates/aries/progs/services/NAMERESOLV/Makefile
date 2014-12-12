# vi: set sw=4 ts=4:

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

MYNAME	:= "NAMERESOLV"


install:
	@echo -e "\033[32mInstalling progs/services/$(MYNAME) ...\033[0m"
	[ -d $(TARGET)/etc/services/$(MYNAME) ] || mkdir -p $(TARGET)/etc/services/$(MYNAME) ;
	cp ./nameresolv.php $(TARGET)/etc/services/$(MYNAME)/.
ifeq ($(ELBOX_PROGS_PRIV_NAMERESOLV),y)
		cp ./nameresolv_v2.php $(TARGET)/etc/services/$(MYNAME)/nameresolv.php
		cp ./nameresolv_del.php $(TARGET)/etc/services/$(MYNAME)/.
endif

.PHONY: install 

