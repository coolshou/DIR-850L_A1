#####################################################################

-include ../arch.mk
-include ../../path.mk
-include ../../.config

APPLET:=telnetd
PREFIX:=/usr/sbin

#####################################################################


CFLAGS+= -DELINUX -DHAVE_DEVPTS_FS -I../../include

ifeq ($(strip $(ELBOX_PROGS_GPL_SYSLOGD)),y)
CFLAGS+= -DSYSLOG
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_TELNETD_CLIENT_CONNECTION_TIMEOUT)),y)
CFLAGS+= -DCONNECTION_TIMEOUT
CFLAGS+= -DMAX_CONNECTIONS=$(ELBOX_PROGS_GPL_TELNETD_CLIENT_NCONNECTIONS)
endif


all:	$(APPLET)

$(APPLET): telnetd.c
	@echo -e "\033[32mBuilding $(APPLET) ...\033[0m"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(Q)[ -z "$(STRIP)" ] || $(STRIP) $(APPLET)

install:
	@echo -e "\033[32mInstalling $(APPLET) ...\033[0m"
	$(Q)[ -d $(TARGET)$(PREFIX) ] || mkdir -p $(TARGET)$(PREFIX)
	$(Q)install ./$(APPLET) $(TARGET)$(PREFIX)
	$(Q)chmod +x $(TARGET)$(PREFIX)/$(APPLET)

clean:
	@echo -e "\033[32mCleaning $(APPLET) ...\033[0m"
	$(Q)rm -rf *~ *.o *.gdb core *.elf $(APPLET)

.PHONY:	all install clean
