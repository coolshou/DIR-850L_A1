-include ../arch.mk
-include ../../path.mk
-include ../../.config

APPLET			= iputils
IPUTILS_PATH	= iputils_s20071127
PREFIX			= /usr/sbin
VPATH			= ./$(IPUTILS_PATH)

CFLAGS += -D_GNU_SOURCE -Wstrict-prototypes

IPV4_TARGETS =
IPV6_TARGETS =

ifeq ($(strip $(ELBOX_PROGS_GPL_IPUTILS_TRACEPATH)),y)
IPV4_TARGETS += tracepath
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_IPUTILS_PING)),y)
IPV4_TARGETS += ping
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_IPUTILS_CLOCKDIFF)),y)
IPV4_TARGETS += clockdiff
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_IPUTILS_RDISC)),y)
IPV4_TARGETS += rdisc
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_IPUTILS_ARPING)),y)
IPV4_TARGETS += arping
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_IPUTILS_TFTPD)),y)
IPV4_TARGETS += tftpd
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_IPUTILS_TRACEPATH6)),y)
IPV6_TARGETS += tracepath6
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_IPUTILS_TRACEROUTE6)),y)
IPV6_TARGETS += traceroute6
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_IPUTILS_PING6)),y)
IPV6_TARGETS += ping6
endif

TARGETS=$(IPV4_TARGETS) $(IPV6_TARGETS)
###############################################################################

all: $(TARGETS)
	@echo -e "\033[32mBuilding $(APPLET) ...\033[0m"

tftpd: tftpd.o tftpsubs.o
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $^

ping: ping.o ping_common.o
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $^

ping6: ping6.o ping_common.o
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $^

ping.o ping6.o ping_common.o: ping_common.h
tftpd.o tftpsubs.o: tftp.h

#rdisc_srv: rdisc_srv.o

#rdisc_srv.o: rdisc.c
#	$(CC) $(CFLAGS) -DRDISC_SERVER -o rdisc_srv.o rdisc.c
install:
	@echo -e "\033[32mInstalling $(APPLET) ...\033[0m"
	$(Q)$(STRIP) $(TARGETS)
	$(Q)[ -d $(TARGET)$(PREFIX) ] || mkdir -p $(TARGET)$(PREFIX)
	$(Q)for i in $(TARGETS); do \
		cp -f ./$$i $(TARGET)$(PREFIX)/iputils-$$i; \
		chmod +x $(TARGET)$(PREFIX)/iputils-$$i; \
	done

clean:
	@echo -e "\033[32mCleaning *.o and $(APPLET) utilities ...\033[0m"
	$(Q)rm -f *.o $(TARGETS)

.PHONY: all tftpd ping ping6 clean

