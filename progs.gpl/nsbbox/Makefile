####################################################################
#
#   Makefile for nsbbox
#
####################################################################

-include ../arch.mk
-include ../../path.mk
-include ../../.config

###################################################################
# module configurations
###################################################################
ifeq (config.nsbbox,$(wildcard config.nsbbox))
-include config.nsbbox
endif

####################################################################

PREBUILD	:=
APPLET		:= nsbbox
LOCAL_SRCS	:= main.c
CMLIB_SRCS	:= libxmldbc.c
CPLUS_SRCS	:=

# main object file
CMDS	=
DIRS	=
CFLAGS	+= -DNSBBOX -I$(TOPDIR)/include -I.

####################################################################
# Configuration for each module

# brctl
ifeq ($(CONFIG_NSBBOX_BRCTL),y)
REQ_LBRIDGE	:=y
LOCAL_SRCS	+= src.brctl/brctl.c src.brctl/brctl_cmd.c src.brctl/brctl_disp.c
CMDS		+= brctl
CFLAGS		+= -I./src.brctl/libbridge
LDFLAGS		+= -L./src.brctl/libbridge -lbridge
endif

# ntpclient
ifeq ($(CONFIG_NSBBOX_NTPCLIENT),y)
LOCAL_SRCS	+= src.ntpclient/ntpclient.c src.ntpclient/phaselock.c
CMDS		+= ntpclient
endif

# base64: b64enc b64enc b64dec
ifeq ($(CONFIG_NSBBOX_BASE64),y)
LOCAL_SRCS	+= src.base64/base64.c
CMDS		+= b64enc b64dec
endif

# smtpclient: smtpclient
ifeq ($(CONFIG_NSBBOX_SMTPCLIENT),y)
LOCAL_SRCS	+= src.smtpclient/smtpclient_main.c
CMDS		+= smtpclient
endif

# vconfig
ifeq ($(CONFIG_NSBBOX_VCONFIG),y)
LOCAL_SRCS	+= src.vconfig/vconfig.c
CMDS		+= vconfig
endif

#####################################################################
ifeq ($(REQ_LBRIDGE),y)
PREBUILD	+= libbridge
endif
#####################################################################
# modules in comlib.
ifeq ($(REQ_SLOOP),y)
CMLIB_SRCS	+= sloop.c
endif
ifeq ($(REQ_USOCK),y)
CMLIB_SRCS	+= unixsock.c
endif
-include $(TOPDIR)/Rules.mk
#####################################################################

USBIN=$(TARGET)/usr/sbin

install:
	@echo -e "\033[32mInstalling $(APPLET) ...\033[0m"
	$(Q)$(STRIP) $(APPLET)
	$(Q)[ -d $(USBIN) ] || mkdir -p $(USBIN)
	$(Q)install $(APPLET) $(USBIN)/.
	$(Q)for i in $(CMDS) ; do [ ! -f $(USBIN)/$$i ] || rm -f $(USBIN)/$$i ; ln -s ./$(APPLET) $(USBIN)/$$i ; done

clean: clean_objs
	@echo -e "\033[32mCleaning $(APPLET) ...\033[0m"
ifeq ($(REQ_LBRIDGE),y)
	$(Q)make -C src.brctl clean
endif

.PHONY: install clean

libbridge:
	@echo -e "\033[32mBuilding libbridge.a ...\033[0m"
	$(Q)make -C src.brctl libbridge/libbridge.a

.PHONY: libbridge
