.EXPORT_ALL_VARIABLES:
# vi: set sw=4 ts=4:

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

#####################################################################

MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
#SUBDIRS := htdocs defnodes
#SUBDIRS := $(TOPDIR)/progs.brand/dir845 defnodes htdocs udev.rules wifi rc services scripts events
SUBDIRS := $(TOPDIR)/progs.brand/dir845 defnodes htdocs udev.rules wifi
LANFILES :=
ifneq ($(strip $(ELBOX_CREATE_LANGUAGE_PACK)),y)
#SUBDIRS += locale
endif

all:
#	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i; done
	$(Q)for i in $(SUBDIRS); do \
	if [ -f $$i/Makefile ]; then \
		$(MYMAKE) -C $$i all; \
	fi \
	done

rootfs:
#	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i rootfs; done
	$(Q)for i in $(SUBDIRS); do \
	if [ -f $$i/Makefile ]; then \
		$(MYMAKE) -C $$i rootfs; \
	fi \
	done

install:
#	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i install; done
#	$(Q)for i in $(LANFILES); do \
#	[ -d $(TARGET)/etc/sealpac ] || mkdir -p $(TARGET)/etc/sealpac ;\
#	cp -r --remove-destination $$i $(TARGET)/etc/sealpac/$$i ;\
#	done
	$(Q)for i in $(SUBDIRS); do \
	if [ -f $$i/Makefile ]; then \
		$(MYMAKE) -C $$i install; \
	else \
		[ -d $(TARGET)/$$i ] || mkdir -p $(TARGET)/$$i ;\
		cp -r --remove-destination $$i $(TARGET)/$$i/. ;\
	fi \
	done

	$(Q)for i in $(LANGDIRS); do \
	[ -d $(TARGET)/etc/sealpac ] || mkdir -p $(TARGET)/etc/sealpac ;\
	cp -r --remove-destination $$i $(TARGET)/etc/sealpac/$$i ;\
	done
	
clean:
#	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i clean; done
	$(Q)for i in $(SUBDIRS); do \
	if [ -f $$i/Makefile ]; then \
		$(MYMAKE) -C $$i all; \
	fi \
	done

.PHONY: all rootfs install clean
