# vi: set sw=4 ts=4:
###########################################################################
-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
###########################################################################

MYNAME := $(ELBOX_BOARD_NAME)/$(ELBOX_TEMPLATE_NAME)/scripts
DIRS   :=
FILES  :=

ifeq ($(strip $(ELBOX_NETWORK_WIFI)),y)
DIRS  += wifi
endif

all:

rootfs:
	@echo -e "\033[32mCreating scripts for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/scripts ] || mkdir -p $(TARGET)/etc/scripts
	$(Q)for i in $(DIRS); do make -C $$i rootfs; done

install:
	@echo -e "\033[32mInstalling scripts for $(MYNAME) ...\033[0m"
	$(Q)for i in $(FILES); do \
		cp --remove-destination $$i $(TARGET)/etc/scripts/.; \
		chmod +x $(TARGET)/etc/scripts/$$i; \
	done
	$(Q)for i in $(DIRS); do make -C $$i install; done

clean:

.PHONY: all rootfs install clean
