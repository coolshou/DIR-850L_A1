# vi: set sw=4 ts=4:

-include ../../path.mk
-include ../../arch.mk
-include $(TOPDIR)/.config
-include $(TOPDIR)/Vars.mk

#####################################################################
MYNAME	:= "aries/dlob.hans/dir850l/mydlink"
MYMAKE	:= make V=$(V) DEBUG=$(DEBUG)
SUBDIRS := scripts
#####################################################################

all:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i; done
	
rootfs:
	@echo -e "\033[32mCreating rootfs for $(MYNAME) ...\033[0m"
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i rootfs; done
	
install:
	@echo -e "\033[32mInstalling $(MYNAME) ...\033[0m"
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i install; done
clean:
	$(Q)for i in $(SUBDIRS); do $(MYMAKE) -C $$i clean; done
	
.PHONY: all rootfs install clean
