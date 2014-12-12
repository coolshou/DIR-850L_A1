#
# Makefile for progs.gpl
#

.EXPORT_ALL_VARIABLES:

-include arch.mk

all:
	@echo -e "\033[32mBuilding progs.gpl ...\033[0m"
	@for i in $(PROGS_GPL_SUBDIRS); do make -C $$i || exit $? ; done

install:
	@echo -e "\033[32mInstalling progs.gpl ...\033[0m"
	@for i in $(PROGS_GPL_SUBDIRS); do make -C $$i install || exit $? ; done

clean:
	@echo -e "\033[32mCleaning progs.gpl\033[0m"
	@for i in $(PROGS_GPL_SUBDIRS); do make -C $$i clean || exit $? ; done

gpl:
	rm -rf telnetd
	mv Makefile.gpl Makefile

.PHONY: all install clean gpl
