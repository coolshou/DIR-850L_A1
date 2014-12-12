.EXPORT_ALL_VARIABLES:

# vi: set sw=4 ts=4:
#####################################################################
-include ../path.mk
-include ../arch.mk
include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk
#####################################################################

MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
SUBDIRS	:= services scripts

all:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i; done

rootfs:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i rootfs; done

install:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i install; done

clean:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i clean; done

#####################################################################

.PHONY: all rootfs install clean
