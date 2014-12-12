-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

all:
install:
ifeq ($(strip $(CONFIG_NEAPS_ARIES_INTERNET)),y)
	$(Q)cp --remove-destination NEAP.11.* $(TARGET)/htdocs/neap
endif
ifeq ($(strip $(CONFIG_NEAPS_ARIES_WIRELESS)),y)
	$(Q)cp --remove-destination NEAP.12.* $(TARGET)/htdocs/neap
endif
ifeq ($(strip $(CONFIG_NEAPS_ARIES_RESIDENTIAL_GATEWAY)),y)
	$(Q)cp --remove-destination NEAP.10.* NEAP.13.* $(TARGET)/htdocs/neap
endif	
clean:
	$(Q)rm -rf $(TARGET)/htdocs/neap/*

.PHONY: all install clean
