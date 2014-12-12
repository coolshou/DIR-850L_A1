-include ../../arch.mk
-include ../../path.mk
-include ../../.config

APPLET=rdisc6
PREFIX=/usr/sbin
localedir=/usr/local/share/locale
############################################################################

SRC	= ndisc6-0.9.9/src
VPATH = ./$(SRC)
OBJS =	ndisc.o

#ifeq ($(strip $(ELBOX_PROGS_GPL_DNSMASQ_HELPER_SUPPORT_ALPHA)),y)
#COPTS += -DALPHA_DNS_HELPER_SUPPORT
#endif
COPTS  += -Os -Wall -D__UCLIBC_HAS_MMU__ -D__UCLIBC__ -DHAVE_CONFIG_H -DRDISC -std=c99
#COPTS  += -Os -Wall -D__UCLIBC_HAS_MMU__ -D__UCLIBC__ -DHAVE_CONFIG_H -DRDISC -std=c99 -DUSE_QUEUE
CFLAGS += -I./ndisc6-0.9.9/include -I./ndisc6-0.9.9 -DLOCALEDIR=\"$(localedir)\"

all :$(APPLET)
	@echo -e "\033[32mBuilding ndisc6 ...\033[0m"
.c.o:
	$(Q)$(CC) $(CFLAGS) $(COPTS) -c $<
$(APPLET): $(OBJS)
	$(Q)$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(Q)$(STRIP) ./rdisc6

install: all
	@echo -e "\033[32mInstalling ndisc6 ...\033[0m"
	$(Q)cp -f ./rdisc6 $(TARGET)$(PREFIX) 
	$(Q)chmod +x $(TARGET)$(PREFIX)/rdisc6

clean:
	@echo -e "\033[32mCleaning ndisc6 ...\033[0m"
	$(Q)rm -f *.o rdisc6
#	$(Q)rm -f *~ ./$(SRC)/*.mo contrib/*/*~ */*~ ./$(SRC)/*.pot
#	$(Q)rm -f ./$(SRC)/*.o ./$(SRC)/dnsmasq.a ./$(SRC)/dnsmasq core */core
