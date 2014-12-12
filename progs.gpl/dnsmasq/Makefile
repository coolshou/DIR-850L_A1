-include ../../arch.mk
-include ../../path.mk
-include ../../.config

APPLET=dnsmasq
PREFIX=/usr/sbin
############################################################################

SRC	= dnsmasq-2.45/src
VPATH = ./$(SRC)
OBJS = cache.o rfc1035.o util.o option.o forward.o isc.o network.o \
       dnsmasq.o dhcp.o lease.o rfc2131.o netlink.o dbus.o bpf.o \
       helper.o tftp.o log.o

ifeq ($(strip $(ELBOX_PROGS_GPL_DNSMASQ_HAVE_BROKEN_RTC)),y)
COPTS += -DHAVE_BROKEN_RTC
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_DNSMASQ_NO_TFTP)),y)
COPTS += -DNO_TFTP
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_DNSMASQ_NOUSER_ALPHA)),y)
COPTS += -DALPHA_NO_USER_SUPPORT
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_DNSMASQ_NOPRCTL_ALPHA)),y)
COPTS += -DALPHA_NO_PRCTL_SUPPORT
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_DNSMASQ_HELPER_SUPPORT_ALPHA)),y)
COPTS += -DALPHA_DNS_HELPER_SUPPORT
endif

#sam_pan add
ifeq ($(strip $(ELBOX_PROGS_GPL_DNSMASQ_ADVANCE_DNS_SERVICE_ALPHA)),y)
CFLAGS += -DADV_DNS_SERVICE -I$(TOPDIR)/include
endif
CFLAGS += -I$(TOPDIR)/include
all :$(APPLET)
	@echo -e "\033[32mBuilding dnsmasq ...\033[0m"
.c.o:
	$(Q)$(CC) $(CFLAGS) $(COPTS) -c $<
$(APPLET): $(OBJS)
	$(Q)$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(Q)$(STRIP) ./dnsmasq

install: all
	@echo -e "\033[32mInstalling dnsmasq ...\033[0m"
	$(Q)cp -f ./dnsmasq $(TARGET)$(PREFIX) 
	$(Q)chmod +x $(TARGET)$(PREFIX)/dnsmasq

clean:
	@echo -e "\033[32mCleaning dnsmasq ...\033[0m"
	$(Q)rm -f *.o dnsmasq
	$(Q)rm -f *~ ./$(SRC)/*.mo contrib/*/*~ */*~ ./$(SRC)/*.pot
	$(Q)rm -f ./$(SRC)/*.o ./$(SRC)/dnsmasq.a ./$(SRC)/dnsmasq core */core
