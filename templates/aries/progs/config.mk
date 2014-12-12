# Template config.mk file.

$(TOPDIR)/tools/sealpac/sealpac:
	$(Q)make -C $(TOPDIR)/tools/sealpac

sealpac_template: $(TOPDIR)/tools/sealpac/sealpac
	@echo -e "\033[32m$(MYNAME): creating sealpac template file ...\033[34m"
	$(Q)rm -f sealpac.slt
	$(Q)$(TOPDIR)/tools/sealpac/sealpac -d rootfs/htdocs -r -t php -f sealpac.slt
	@echo -ne "\033[0m"

.PHONY: sealpac_template
