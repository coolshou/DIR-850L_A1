# vi: set sw=4 ts=4:
#####################################################################
-include ../path.mk
-include ../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk
#####################################################################

MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
SUBDIRS	:= rc services scripts defnodes events

#####################################################################
all:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i; done

rootfs:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i rootfs; done

install: install_mfc
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i install; done

clean:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i clean; done

#####################################################################

install_mfc:
	$(Q)cp mfc $(TARGET)/usr/sbin/.
.PHONY: all rootfs install clean install_mfc

