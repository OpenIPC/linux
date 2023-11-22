#
# arch/arm64/boot/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2012, ARM Ltd.
# Author: Will Deacon <will.deacon@arm.com>
#
# Based on the ia64 boot/Makefile.
#

OBJCOPYFLAGS_Image :=-O binary -R .note -R .note.gnu.build-id -R .comment -S


ifeq ($(CONFIG_ARCH_SSTAR),)
targets := Image Image.bz2 Image.gz Image.lz4 Image.lzma Image.lzo
$(obj)/Image: vmlinux FORCE
	$(call if_changed,objcopy)

else
targets := Image Image.bz2 Image.gz Image.lz4 Image.lzma Image.lzo Image.xz uImage uImage.xz
MKIMAGE_BIN = $(srctree)/scripts/mkimage_deal_64bit_entry
LOADADDR = $(shell /bin/bash -c 'printf "0x%08x" $$[$(CONFIG_UIMAGE_LOAD_ADDR)]')
ENTRYADDR = $(shell /bin/bash -c 'printf "0x%08x" $$[$(CONFIG_UIMAGE_LOAD_ADDR) - 0x20000000 + 0x1000000000]')
SS_DTB_NAME = arch/arm64/boot/dts/sstar/${CONFIG_SS_DTB_NAME}.dtb

CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

$(obj)/Image: vmlinux
	$(call if_changed,objcopy)

ifeq ($(CONFIG_SS_BUILTIN_DTB), y)
	#update builtin DTB
	@if [ -e ${SS_DTB_NAME} ]; then \
		echo "  IMAGE $(obj)/Image"; \
		echo "  BNDTB ${SS_DTB_NAME}"; \
		python $(srctree)/scripts/ms_builtin_dtb_update.py $(obj)/Image ${SS_DTB_NAME}; \
		echo; \
	else \
		echo -e "\033[41;33m No dtb file available!!!\033[0m"; \
	fi;
endif

ifneq ($(wildcard scripts/ss_padmux_check.py),)
		@scripts/dtc/dtx_diff ${SS_DTB_NAME} > $(CONFIG_SS_DTB_NAME).dts
ifneq ($(wildcard drivers/sstar/gpio/$(CHIP_NAME)/mhal_pinmux.o),)
		@$(CROSS_COMPILE)objdump -D -S -j .rodata drivers/sstar/gpio/$(CHIP_NAME)/mhal_pinmux.o > pinmux_table
		@python scripts/ss_padmux_check.py $(CONFIG_SS_DTB_NAME).dts pinmux_table
		@rm pinmux_table
endif
		@rm $(CONFIG_SS_DTB_NAME).dts
endif

	#update the image size into Image
	@python $(srctree)/scripts/ms_bin_option_update_int.py  $(obj)/Image '#IMG_SZ#' $$(stat -c %s $(obj)/Image)

$(obj)/Image.xz: $(obj)/Image
#	$(call if_changed,xzmisc)
	@echo "  XZ   $@"
	@xz -z -k -f $(obj)/Image;

$(obj)/uImage: $(obj)/Image
#	$(call if_changed,uimage)
	@echo "  UIMAGE   $@"
	@IMGNAME=$(shell /usr/bin/strings -a -T binary $(obj)/Image | grep 'MVX' | grep 'LX'  | \
	sed 's/\\*MVX/MVX/g' | cut -c 1-32 ); \
	if [ -e ${MKIMAGE_BIN} ]; then \
		${MKIMAGE_BIN} -A $(CONFIG_UIMAGE_ARCH) -O linux -T kernel -C none -a ${LOADADDR} \
		-e ${ENTRYADDR} -n $${IMGNAME} -d $< $@; \
		echo '$(IMGNAME)'; \
	else \
		echo ">> Can't find $${MKIMAGE_BIN}. <<"; \
	fi;

$(obj)/uImage.xz: $(obj)/Image.xz
#	$(call if_changed,uimage)
	@echo "  UIMAGE   $@"
	@IMGNAME=$(shell /usr/bin/strings -a -T binary $(obj)/Image | grep 'MVX' | grep 'LX'  | \
	sed 's/\\*MVX/MVX/g' | cut -c 1-32 ); \
	if [ -e ${MKIMAGE_BIN} ]; then \
		${MKIMAGE_BIN} -A $(CONFIG_UIMAGE_ARCH) -O linux -T kernel -C lzma -a ${LOADADDR} \
		-e ${ENTRYADDR} -n $${IMGNAME} -d $< $@; \
		echo '$(IMGNAME)'; \
	else \
		echo ">> Can't find $${MKIMAGE_BIN}. <<"; \
	fi;

endif

$(obj)/Image.bz2: $(obj)/Image FORCE
	$(call if_changed,bzip2)

$(obj)/Image.gz: $(obj)/Image FORCE
	$(call if_changed,gzip)

$(obj)/Image.lz4: $(obj)/Image FORCE
	$(call if_changed,lz4)

$(obj)/Image.lzma: $(obj)/Image FORCE
	$(call if_changed,lzma)

$(obj)/Image.lzo: $(obj)/Image FORCE
	$(call if_changed,lzo)

install:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image System.map "$(INSTALL_PATH)"

zinstall:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image.gz System.map "$(INSTALL_PATH)"
