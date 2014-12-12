#
# Makefile for progs.priv
#

.EXPORT_ALL_VARIABLES:

-include arch.mk

SUBDIRS := alpha_gd cgibin.aries ddnsd ifresetcnt igmpproxy \
lld2d llmnresp logd neap.aries Netbios portt proxyd rgbin \
sendmail servd turbonat.26 ubcom updateleases updatewifistats\
widget wpstools xmldb3 rt2860v2

all:
	@echo -e "\033[32mBuilding progs.priv ...\033[0m"
	@for i in $(PROGS_PRIV_SUBDIRS); do make -C $$i || exit $? ; done

install:
	@echo -e "\033[32mInstalling progs.priv ...\033[0m"
	@for i in $(PROGS_PRIV_SUBDIRS); do make -C $$i install || exit $? ; done

clean:
	@echo -e "\033[32mCleaning progs.priv ...\033[0m"
	@for i in $(PROGS_PRIV_SUBDIRS); do make -C $$i clean || exit $? ; done

gpl:
	for i in $(PROGS_PRIV_SUBDIRS); do make -C $$i gpl;done
	mv Makefile.gpl Makefile

.PHONY: all install clean gpl
