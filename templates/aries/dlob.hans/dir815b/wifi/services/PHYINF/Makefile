# vi: set sw=4 ts=4:

-include ../../../path.mk
-include ../../../arch.mk
-include $(TOPDIR)/.config

###########################################################################

MYNAME	:= "WRGND10/Aries/services"

FILES	:= phywifi
DIRS	:=

all:

rootfs:

install:
	@echo -e "\033[32mInstalling services for $(MYNAME) ...\033[0m"
	$(Q)if [ ! -d $(TARGET)/etc/services/PHYINF ]; then \
		mkdir $(TARGET)/etc/services/PHYINF;\
		fi;
	$(Q)for i in $(FILES); do cp $$i.php $(TARGET)/etc/services/PHYINF/.; done
	$(Q)for i in $(DIRS); do \
		if [ ! -d $(TARGET)/etc/service/$$i ]; then \
			mkdir -p $(TARGET)/etc/services/$$i; \
		fi ; \
		cp $$i/*.php $(TARGET)/etc/services/$$i/.; \
	done

clean:

.PHONY: all rootfs install clean
