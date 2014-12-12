############################################################################
# Board dependent Makefile for WRG-AC05
############################################################################

MYNAME	:= $(ELBOX_BOARD_NAME)
MKSQFS  := ./tools/squashfs-tools-4.0_realtek/mksquashfs
SEAMA	:= ./tools/seama/seama
PIMGS	:= ./tools/buildimg/packimgs
LZMA	:= ./tools/lzma/lzma
CVIMG	:= ./tools/realtek/cvimg
MYMAKE	:= $(Q)make V=$(V) DEBUG=$(DEBUG)
FWDEV	:= /dev/mtdblock/1
#LANGDEV := /dev/mtdblock/5


FIRMWARENAME = $(shell echo $(ELBOX_MODEL_NAME) | tr '[:lower:]' '[:upper:]' | cut -d- --output-delimiter=\"\" -f1,2)
FIRMWAREREV= $(shell echo $(ELBOX_FIRMWARE_VERSION) | cut -d. --output-delimiter=\"\" -f1,2)
BUILDNO :=$(shell cat buildno)
RELIMAGE:=""
ifneq ($(strip $(ELBOX_FIRMWARE_SIGNATURE)), "N/A")
ifeq ($(strip $(ELBOX_FIRMWARE_SIGNATURE)), "wrgac05_dlob.hans_dir850l_SN")
RELIMAGE:=$(shell echo $(FIRMWARENAME)_FW$(FIRMWAREREV)SN$(ELBOX_FIRMWARE_REVISION)_$(BUILDNO))
endif
endif
ifeq ($(strip $(RELIMAGE)), "")
RELIMAGE:=$(shell echo $(FIRMWARENAME)_FW$(FIRMWAREREV)$(ELBOX_FIRMWARE_REVISION)_$(BUILDNO))
endif

ifeq ($(strip $(ELBOX_USE_IPV6)),y)
KERNELCONFIG := kernel.aries.ipv6.config
else
KERNELCONFIG := kernel.aries.config
endif

#############################################################################
# This one will be make in fakeroot.
fakeroot_rootfs_image:
	@rm -f fakeroot.rootfs.img
	@./progs.board/template.aries/makedevnodes rootfs
#	@./tools/sqlzma/sqlzma-3.2-443-r2/mksquashfs rootfs fakeroot.rootfs.img -be
	@./tools/squashfs-tools-4.0_realtek/mksquashfs rootfs fakeroot.rootfs.img -comp lzma -always-use-fragments
.PHONY: rootfs_image

#############################################################################
# The real image files

$(ROOTFS_IMG): ./tools/squashfs-tools-4.0_realtek/mksquashfs
	@echo -e "\033[32m$(MYNAME): building squashfs (LZMA)!\033[0m"
	$(Q)make clean_CVS
	$(Q)fakeroot make -f progs.board/template.aries/config.mk fakeroot_rootfs_image
	$(Q)mv fakeroot.rootfs.img $(ROOTFS_IMG)
	$(Q)chmod 664 $(ROOTFS_IMG)

$(KERNEL_IMG): ./tools/lzma/lzma ./tools/realtek/cvimg $(KERNELDIR)/vmlinux
	@echo -e "\033[32m$(MYNAME): building kernel image (LZMA)...\033[0m"
	$(Q)rm -f vmlinux.bin $(KERNEL_IMG)
	$(Q)mips-linux-objcopy -O binary -R .note -R .comment -S $(KERNELDIR)/vmlinux vmlinux.bin
	$(Q)$(LZMA) -9 -f -S .lzma vmlinux.bin
	#$(Q)mv vmlinux.bin.lzma $(KERNEL_IMG)
	$(Q)cp ./kernel/rtkload/linux.bin $(KERNEL_IMG)
$(KERNELDIR )/vmlinux:
	$(MYMAKE) kernel

./tools/squashfs-tools-4.0_realtek/mksquashfs:
	$(Q)make -C ./tools/squashfs-tools-4.0_realtek

./tools/seama/seama:
	$(Q)make -C ./tools/seama

./tools/buildimg/packimgs:
	$(Q)make -C ./tools/buildimg

./tools/lzma/lzma:
	$(Q)make -C ./tools/lzma

./tools/realtek/cvimg:
	$(Q)make -C ./tools/realtek
##########################################################################

kernel_image:
	@echo -e "\033[32m$(MYNAME): creating kernel image\033[0m"
	$(Q)rm -f $(KERNEL_IMG)
	$(MYMAKE) $(KERNEL_IMG)

rootfs_image:
	@echo -e "\033[32m$(MYNAME): creating rootfs image ...\033[0m"
	$(Q)rm -f $(ROOTFS_IMG)
	$(MYMAKE) $(ROOTFS_IMG)

.PHONY: rootfs_image kernel_image

##########################################################################
#
#	Major targets: kernel, kernel_clean, release & tftpimage
#
##########################################################################

squashfs_clean:
	@echo -e "\033[32m$(MYNAME): cleaning squashfs ...\033[0m"
	$(Q)make -C ./tools/squashfs-tools-4.0_realtek  clean

kernel_target:
	$(Q)rm -f $(KERNELDIR)/arch/rlx/target 2>/dev/null
	$(Q)ln -s ./boards/$(ELBOX_BSP_RTLSDK_3_2_TARGET) $(KERNELDIR)/arch/rlx/target

kernel_clean: squashfs_clean kernel_target
	@echo -e "\033[32m$(MYNAME): cleaning kernel ...\033[0m"
	$(Q)cp progs.board/$(KERNELCONFIG) kernel/.config
	$(Q)make -C kernel mrproper

kernel: kernel_clean
	@echo -e "\033[32m$(MYNAME) Building kernel ...\033[0m"
	$(Q)cp progs.board/$(KERNELCONFIG) kernel/.config
	$(Q)make -C kernel oldconfig
	$(Q)make -C kernel dep
	$(Q)make -C kernel

ifeq (buildno, $(wildcard buildno))
BUILDNO	:=$(shell cat buildno)

release: kernel_image rootfs_image ./tools/buildimg/packimgs ./tools/seama/seama
	@echo -e "\033[32m"; \
	echo "=====================================";	\
	echo "You are going to build release image.";	\
	echo "=====================================";	\
	echo -e "\033[32m$(MYNAME) make release image... \033[0m"
	$(Q)[ -d images ] || mkdir -p images
	@echo -e "\033[32m$(MYNAME) prepare image...\033[0m"
	$(Q)$(PIMGS) -o raw.img -i $(KERNEL_IMG) -i $(ROOTFS_IMG)
	$(Q)$(SEAMA) -i raw.img -m dev=$(FWDEV) -m type=firmware 
	$(Q)$(SEAMA) -s web.img -i raw.img.seama -m signature=$(ELBOX_SIGNATURE)
	$(Q)$(SEAMA) -d web.img
	$(Q)rm -f raw.img raw.img.seama
	$(Q)./tools/release.sh web.img $(RELIMAGE).bin
	$(Q)cp $(TOPDIR)/images/$(RELIMAGE).bin $(TOPDIR)/$(TFTPIMG)
	$(Q)make sealpac_template
	$(Q)if [ -f sealpac.slt ]; then ./tools/release.sh sealpac.slt $(RELIMAGE).slt; fi
	$(Q)echo -e "\033[32mcopy images/$(RELIMAGE).bin to $(TFTPIMG)... \033[0m"
	$(Q)cp $(TOPDIR)/images/$(RELIMAGE).bin $(TOPDIR)/$(TFTPIMG)

magic_release: kernel_image rootfs_image ./tools/buildimg/packimgs ./tools/seama/seama
	@echo -e "\033[32m"; \
	echo "===========================================";	\
	echo "You are going to build magic release image.";	\
	echo "===========================================";	\
	echo -e "\033[32m$(MYNAME) make magic release image... \033[0m"
	$(Q)[ -d images ] || mkdir -p images
	@echo -e "\033[32m$(MYNAME) prepare image...\033[0m"
	$(Q)$(PIMGS) -o raw.img -i $(KERNEL_IMG) -i $(ROOTFS_IMG)
	$(Q)$(SEAMA) -i raw.img -m dev=$(FWDEV) -m type=firmware 
	$(Q)$(SEAMA) -s web.img -i raw.img.seama -m signature=$(ELBOX_BOARD_NAME)_aLpHa
	$(Q)$(SEAMA) -d web.img
	$(Q)rm -f raw.img raw.img.seama
	$(Q)./tools/release.sh web.img $(RELIMAGE).magic.bin

tftpimage: kernel_image rootfs_image ./tools/buildimg/packimgs ./tools/seama/seama
	@echo -e "\033[32mThe tftpimage of $(MYNAME) is identical to the release image!\033[0m"
	$(Q)$(PIMGS) -o raw.img -i $(KERNEL_IMG) -i $(ROOTFS_IMG)
	$(Q)$(SEAMA) -i raw.img -m dev=$(FWDEV) -m type=firmware
	$(Q)rm -f raw.img; mv raw.img.seama raw.img
	$(Q)$(SEAMA) -d raw.img
	$(Q)./tools/tftpimage.sh $(TFTPIMG)

else
release tftpimage:
	@echo -e "\033[32m$(MYNAME): Can not build image, ROOTFS is not created yet !\033[0m"
endif

.PHONY: kernel release tftpimage kernel_clean magic_release
