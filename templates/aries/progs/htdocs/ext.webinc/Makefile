# 

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME  := "ARIES/HTDOCS/WEBINC"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
TDIR	:= $(TARGET)/htdocs/webinc
FILES	:=
SUBDIRS	:= getcfg

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
