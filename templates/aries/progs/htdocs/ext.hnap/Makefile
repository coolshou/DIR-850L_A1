# 

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME  := "ARIES/HTDOCS/HNAP"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
TDIR	:= $(TARGET)/etc/templates/hnap
FILES	:=
SUBDIRS	:=

ifeq ($(ELBOX_TEMPLATE_ARIES_NEW_WIFI_UID),y)
FILES  += Config.php
endif

###########################################################################
all:
	@echo -e "\033[32mBuilding applets for $(MYNAME) ...\033[0m"
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i; done

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TDIR) ] || mkdir -p $(TDIR)
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i rootfs; done

install:
	@echo -e "\033[32mInstalling applets for $(MYNAME) ...\033[0m"
	$(Q)for i in $(FILES); do cp -r --remove-destination $$i $(TDIR)/. ; done
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i install; done

.PHONY: all rootfs install
###########################################################################
