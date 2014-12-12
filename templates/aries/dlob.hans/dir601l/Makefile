.EXPORT_ALL_VARIABLES:
# vi: set sw=4 ts=4:

-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

#####################################################################

MYNAME	:= "Aries/Dlob/dir601l"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
#SUBDIRS := htdocs defnodes
SUBDIRS := defnodes htdocs wifi rc events scripts services
ifneq ($(strip $(ELBOX_CREATE_LANGUAGE_PACK)),y)
#SUBDIRS += locale
endif

all:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i; done

rootfs:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i rootfs; done

install:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i install; done

clean:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i clean; done

.PHONY: all rootfs install clean
