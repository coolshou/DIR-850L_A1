# 

-include ../../../../path.mk
-include ../../../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

###########################################################################

MYNAME  := "ARIES/HTDOCS/PHPLIB/SETCFG/LIBS"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
TDIR	:= $(TARGET)/htdocs/phplib/setcfg/libs
FILES	:=

###########################################################################

ifeq ($(ELBOX_TEMPLATE_ARIES_NEW_WIFI_UID),y)
FILES	+= wifi
endif

ifeq ($(ELBOX_USE_IPV6),y)
FILES += dhcps6
endif

ifeq ($(ELBOX_PROGS_PRIV_FILTER),y)
FILES += filter
endif

###########################################################################
all:

rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)[ -d $(TDIR) ] || mkdir -p $(TDIR)

install:
	@echo -e "\033[32mInstalling applets for $(MYNAME) ...\033[0m"
	$(Q)for i in $(FILES); do cp -r --remove-destination $$i.php $(TDIR)/. ; done

.PHONY: all rootfs install
###########################################################################
