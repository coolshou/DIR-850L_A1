
-include ../arch.mk
-include ../../path.mk
-include ../../.config

# udhcp makefile

prefix=$(TARGET)/usr
SBINDIR=$(TARGET)/sbin
USRSBINDIR=${prefix}/sbin
USRBINDIR=${prefix}/bin
USRSHAREDIR=${prefix}/share
LDFLAGS += -lm

# Uncomment this to get a shared binary. Call as udhcpd for the server,
# and udhcpc for the client
COMBINED_BINARY=1

# Uncomment this for extra output and to compile with debugging symbols
#DEBUG=1

# Uncomment this to output messages to syslog, otherwise, messages go to stdout
#CFLAGS += -DSYSLOG

#CROSS_COMPILE=arm-uclibc-
#CC = $(CROSS_COMPILE)gcc
#LD = $(CROSS_COMPILE)gcc
INSTALL = install

VER := 0.9.8

CFLAGS += -I$(TOPDIR)/include

OBJS_SHARED = options.o socket.o packet.o pidfile.o
DHCPD_OBJS = dhcpd.o arpping.o files.o leases.o serverpacket.o
DHCPC_OBJS = dhcpc.o clientpacket.o script.o

#add henan,china dhcpplus .by tsrites 2008.11.7
ifdef ELBOX_PROGS_PRIV_DHCPPLUS
CFLAGS += -DDHCPPLUS
endif

ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
CFLAGS += -DELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
DHCPD_OBJS += wireless_netlink.o
endif

ifdef ELBOX_PROGS_GPL_UDHCP_REQUEST_BROADCAST 
CFLAGS += -DELBOX_PROGS_GPL_UDHCP_REQUEST_BROADCAST
endif

ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
CFLAGS += -DELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
endif

ifdef ELBOX_PROGS_PRIV_LOGD_AP
CFLAGS += -DELBOX_PROGS_PRIV_LOGD_AP
endif

ifdef ELBOX_PROGS_GPL_UDHCP_MS_RFC3442
CFLAGS += -DMSCLSLESS_STATIC_ROUTE_OPTION
endif
ifdef ELBOX_PROGS_GPL_UDHCP_RFC3442
CFLAGS += -DCLASSLESS_STATIC_ROUTE_OPTION
endif
ifdef ELBOX_PROGS_GPL_UDHCP_RFC2132
CFLAGS += -DSTATIC_ROUTE_OPTION
endif

ifdef ELBOX_USE_IPV6
CFLAGS += -DSIX_RD_OPTION
endif

ifdef COMBINED_BINARY
EXEC1 = udhcpd
OBJS1 = $(DHCPD_OBJS) $(DHCPC_OBJS) $(OBJS_SHARED) frontend.o
CFLAGS += -DCOMBINED_BINARY
else
EXEC1 = udhcpd
OBJS1 = $(DHCPD_OBJS) $(OBJS_SHARED)

EXEC2 = udhcpc
OBJS2 = $(DHCPC_OBJS) $(OBJS_SHARED)
endif

EXEC3 = dumpleases
OBJS3 = dumpleases.o

BOOT_PROGRAMS = udhcpc
DAEMONS = udhcpd
COMMANDS = dumpleases

ifdef SYSLOG
CFLAGS += -DSYSLOG
endif

CFLAGS += -W -Wall -Wstrict-prototypes -DVERSION='"$(VER)"'

ifdef DEBUG
CFLAGS += -g -DDEBUG
STRIP=true
else
CFLAGS += -O2 -fomit-frame-pointer
#STRIP=$(CROSS_COMPILE)strip
endif

all: $(EXEC1) $(EXEC2) $(EXEC3)
	@echo -e "\033[32mBuilding udhcpd ...\033[0m"
#	$(STRIP) --remove-section=.note --remove-section=.comment $(EXEC1) $(EXEC2) $(EXEC3)

$(OBJS1) $(OBJS2) $(OBJS3): *.h Makefile
$(EXEC1) $(EXEC2) $(EXEC3): Makefile

.c.o:
	$(Q)$(CC) -c $(CFLAGS) $<

$(EXEC1): $(OBJS1)
	$(Q)$(CC) $(LDFLAGS) $(OBJS1) -o $(EXEC1)
	$(Q)$(STRIP) $(EXEC1)

$(EXEC2): $(OBJS2)
	$(Q)$(CC) $(LDFLAGS) $(OBJS2) -o $(EXEC2)
	$(Q)$(STRIP) $(EXEC2)

$(EXEC3): $(OBJS3)
	$(Q)$(CC) $(LDFLAGS) $(OBJS3) -o $(EXEC3)
	$(Q)$(STRIP) $(EXEC3)

install: all
	@echo -e "\033[32mInstalling udhcpd ...\033[0m"
	$(Q)[ -d $(USRSBINDIR) ] || mkdir -p $(USRSBINDIR)
	$(Q)$(INSTALL) $(DAEMONS) $(USRSBINDIR)
	$(Q)chmod +x $(USRSBINDIR)/$(DAEMONS)
#ifndef RGAPPS_PROGS_PRIV_COMMON_SCRIPTS
#	@echo -e "\033[32mInstalling scripts for udhcp ...\033[0m"
#	[ -d $(TARGET)/etc/scripts/udhcp ] || mkdir -p $(TARGET)/etc/scripts/udhcp
#	cp -a ./scripts/*.sh $(TARGET)/etc/scripts/udhcp/.
#	chmod +x $(TARGET)/etc/scripts/udhcp/*.sh
#endif
#	$(INSTALL) $(COMMANDS) $(USRBINDIR)

ifdef COMBINED_BINARY
	$(Q)ln -s ./$(DAEMONS) $(USRSBINDIR)/$(BOOT_PROGRAMS)
else
	$(Q)$(INSTALL) $(BOOT_PROGRAMS) $(SBINDIR)
endif

clean:
	@echo -e "\033[32mCleaning udhcpd ...\033[0m"
	$(Q)rm -f udhcpd udhcpc dumpleases *.o core 

.PHONY: all intall clean

