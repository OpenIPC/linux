#
# arch/arm/boot/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1995-2002 Russell King
#

OBJCOPYFLAGS	:=-O binary -R .comment -S

ifneq ($(CONFIG_ARM_PATCH_PHYS_VIRT),)
LOADADDR = 0x20008000
else
LOADADDR = $(shell /bin/bash -c 'printf "0x%08x" $$[$(CONFIG_PHYS_OFFSET) + 0x8000]')
endif

MKIMAGE_BIN = $(srctree)/scripts/mkimage
MZ_BIN = $(srctree)/scripts/mz
KERNEL_RELEASE_FILE = include/config/kernel.release
ROOTFS = $(obj)/ramdisk_linaro4.8_minit.img.xz

_BUILTIN_DTB_NAME=$(strip $(shell echo ${CONFIG_SS_DTB_NAME}))
ifneq ($(_BUILTIN_DTB_NAME),)
ifneq ($(wildcard $(srctree)/arch/arm/boot/dts/$(_BUILTIN_DTB_NAME).dts),)
SS_DTB_NAME=arch/arm/boot/dts/$(_BUILTIN_DTB_NAME).dtb
endif
endif

CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

ifeq ($(findstring fpga, $(SS_DTB_NAME)), fpga)
SS_FPGA_DTB_NAME = $(SS_DTB_NAME)
else
SS_FPGA_DTB_NAME = arch/arm/boot/dts/$(CHIP_NAME)-fpga.dtb
endif

ifneq ($(MACHINE),)
include $(MACHINE)/Makefile.boot
endif

# Note: the following conditions must always be true:
#   ZRELADDR == virt_to_phys(PAGE_OFFSET + TEXT_OFFSET)
#   PARAMS_PHYS must be within 4MB of ZRELADDR
#   INITRD_PHYS must be in RAM
ZRELADDR    := $(zreladdr-y)
PARAMS_PHYS := $(params_phys-y)
INITRD_PHYS := $(initrd_phys-y)

export ZRELADDR INITRD_PHYS PARAMS_PHYS

targets := Image zImage xipImage bootpImage uImage

ifeq ($(CONFIG_XIP_KERNEL),y)

cmd_deflate_xip_data = $(CONFIG_SHELL) -c \
	'$(srctree)/$(src)/deflate_xip_data.sh $< $@'

ifeq ($(CONFIG_XIP_DEFLATED_DATA),y)
quiet_cmd_mkxip = XIPZ    $@
cmd_mkxip = $(cmd_objcopy) && $(cmd_deflate_xip_data)
else
quiet_cmd_mkxip = $(quiet_cmd_objcopy)
cmd_mkxip = $(cmd_objcopy)
endif

$(obj)/xipImage: vmlinux FORCE
	$(call if_changed,mkxip)
	@$(kecho) '  Physical Address of xipImage: $(CONFIG_XIP_PHYS_ADDR)'

$(obj)/Image $(obj)/zImage: FORCE
	@echo 'Kernel configured for XIP (CONFIG_XIP_KERNEL=y)'
	@echo 'Only the xipImage target is available in this case'
	@false

else

$(obj)/xipImage: FORCE
	@echo 'Kernel not configured for XIP (CONFIG_XIP_KERNEL!=y)'
	@false

$(obj)/Image: vmlinux FORCE
	$(call if_changed,objcopy)

$(obj)/compressed/vmlinux: $(obj)/Image FORCE
	$(Q)$(MAKE) $(build)=$(obj)/compressed $@

$(obj)/zImage:	$(obj)/compressed/vmlinux FORCE
	$(call if_changed,objcopy)
endif

ifneq ($(LOADADDR),)
  UIMAGE_LOADADDR=$(LOADADDR)
else
  ifeq ($(CONFIG_ZBOOT_ROM),y)
    UIMAGE_LOADADDR=$(CONFIG_ZBOOT_ROM_TEXT)
  else
    UIMAGE_LOADADDR=$(ZRELADDR)
  endif
endif

check_for_multiple_loadaddr = \
if [ $(words $(UIMAGE_LOADADDR)) -ne 1 ]; then \
	echo 'multiple (or no) load addresses: $(UIMAGE_LOADADDR)'; \
	echo 'This is incompatible with uImages'; \
	echo 'Specify LOADADDR on the commandline to build an uImage'; \
	false; \
fi

clean-files += uImage.xz uImage.mz Image.xz Image.mz Image-fpga
ifeq ($(CONFIG_ARCH_SSTAR),)
$(obj)/uImage:	$(obj)/zImage FORCE
	@$(check_for_multiple_loadaddr)
	$(call if_changed,uimage)
else
$(obj)/uImage:	$(obj)/Image $(SS_DTB_NAME) $(SS_FPGA_DTB_NAME) FORCE
ifeq ($(CONFIG_SS_BUILTIN_DTB), y)
		#update builtin DTB
		@test "${SS_DTB_NAME}" = "" || \
		if [ -e ${SS_DTB_NAME} ]; then \
			echo "  IMAGE   $(obj)/Image"; \
			echo "  BNDTB ${SS_DTB_NAME}"; \
			python3 $(srctree)/scripts/ms_builtin_dtb_update.py $(obj)/Image ${SS_DTB_NAME}; \
			echo; \
		fi;
		#update Image-fpga DTB
		@if [ -e ${SS_FPGA_DTB_NAME} ]; then \
			echo "  IMAGE   $(obj)/Image-fpga"; \
			cp -f $(obj)/Image $(obj)/Image-fpga; \
			echo "  BNDTB   ${SS_FPGA_DTB_NAME}"; \
			python3 $(srctree)/scripts/ms_builtin_dtb_update.py $(obj)/Image-fpga ${SS_FPGA_DTB_NAME}; \
			echo; \
		fi;
endif

ifeq ($(CONFIG_SS_BUILTIN_UNFDT), y)
		#update builtin UNFDT
		@test "${SS_DTB_NAME}" = "" || \
		if [ -e ${SS_DTB_NAME} ]; then \
			echo "  BNDTB ${SS_DTB_NAME}"; \
			scripts/dtc/dtb2unfdt/dtb2unfdt ${SS_DTB_NAME}  $(obj)/unfdt.bin; \
			python $(srctree)/scripts/ms_builtin_unfdt_update.py $(obj)/Image $(obj)/unfdt.bin; \
			echo; \
		fi;
endif

ifneq ($(wildcard $(srctree)/scripts/ss_padmux_check.py),)
		@$(srctree)/scripts/dtc/dtx_diff ${SS_DTB_NAME} > $(CONFIG_SS_DTB_NAME).dts
ifneq ($(wildcard drivers/sstar/gpio/$(CHIP_NAME)/mhal_pinmux.o),)
		@$(CROSS_COMPILE)objdump -D -S -j .rodata drivers/sstar/gpio/$(CHIP_NAME)/mhal_pinmux.o > pinmux_table
		@python3 $(srctree)/scripts/ss_padmux_check.py $(CONFIG_SS_DTB_NAME).dts pinmux_table
		@rm pinmux_table
endif
		@rm $(CONFIG_SS_DTB_NAME).dts
endif

ifneq ($(wildcard $(srctree)/scripts/ss_puse_check.py),)
		@python3 scripts/ss_puse_check.py drivers/sstar/include/mdrv_puse.h
endif

	#update the image size into Image
	@python3 $(srctree)/scripts/ms_bin_option_update_int.py  $(obj)/Image '#IMG_SZ#' $$(stat -c %s $(obj)/Image)

	#build uImage
	@xz -z -k -f $(obj)/Image
	@${MKIMAGE_BIN} -A arm -O linux -C lzma -a ${LOADADDR} -e ${LOADADDR} -n ${UIMAGE_NAME} -d $(obj)/Image.xz $(obj)/uImage
endif

$(obj)/bootp/bootp: $(obj)/zImage initrd FORCE
	$(Q)$(MAKE) $(build)=$(obj)/bootp $@

$(obj)/bootpImage: $(obj)/bootp/bootp FORCE
	$(call if_changed,objcopy)

PHONY += initrd install zinstall uinstall
initrd:
	@test "$(INITRD_PHYS)" != "" || \
	(echo This machine does not support INITRD; exit -1)
	@test "$(INITRD)" != "" || \
	(echo You must specify INITRD; exit -1)

install:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh "$(KERNELRELEASE)" \
	$(obj)/Image System.map "$(INSTALL_PATH)"

zinstall:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh "$(KERNELRELEASE)" \
	$(obj)/zImage System.map "$(INSTALL_PATH)"

uinstall:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh "$(KERNELRELEASE)" \
	$(obj)/uImage System.map "$(INSTALL_PATH)"

subdir-	    := bootp compressed dts
