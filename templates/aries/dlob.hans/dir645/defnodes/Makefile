# vi: set sw=4 ts=4:

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME  := "Aries/Dlob/DIR645/defnodes"
XMLFILES:= $(wildcard *.xml)
PHPFILES:= $(wildcard *.php)
SCRIPTFILES:= $(wildcard *.sh)
OTHER_DEFAULT_FILES :=
RU_FW= $(shell echo $(ELBOX_FIRMWARE_VERSION) | grep RU )

ifeq ($(ELBOX_PROGS_PRIV_MIIICASA) , y)
OTHER_DEFAULT_FILES += defaultvalue_miiicasa.txt
endif

all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TARGET)/etc/defnodes ] || mkdir -p $(TARGET)/etc/defnodes

#for defaultvalue.xml we need to do something special
#some default value blocks will be inserted when config flag is enabled
#defaultvalue_appender.pl will do this, just give block text file to it
install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(XMLFILES); do \
		sed -e 's|SiGnAtUrE|$(ELBOX_CONFIG_SIGNATURE)|' $$i > $(TARGET)/etc/defnodes/$$i ; done
	$(Q)for i in $(PHPFILES); do cp $$i $(TARGET)/etc/defnodes/$$i; done 
	$(Q)for i in $(SCRIPTFILES); do cp $$i $(TARGET)/etc/defnodes/$$i; done
	$(Q)./defaultvalue_appender.pl defaultvalue.xml $(OTHER_DEFAULT_FILES) | sed -e 's|SiGnAtUrE|$(ELBOX_CONFIG_SIGNATURE)|' > $(TARGET)/etc/defnodes/defaultvalue.xml
ifneq ($(strip $(RU_FW)),)
	@echo -e "\033[32m....install RU country....\033[0m"
	sed -e 's|<countrycode get="devdata get -e countrycode" />|<countrycode>RU</countrycode>|' $(TARGET)/etc/defnodes/S11devdata.xml > $(TARGET)/etc/defnodes/S11devdata.xml.tmp
	mv $(TARGET)/etc/defnodes/S11devdata.xml.tmp $(TARGET)/etc/defnodes/S11devdata.xml
endif	
clean:


.PHONY: all rootfs install clean
