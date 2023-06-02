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


MKIMAGE_BIN = scripts/mkimage
MZ_BIN = scripts/mz
KERNEL_RELEASE_FILE = include/config/kernel.release
ROOTFS = $(obj)/ramdisk_linaro4.8_minit.img.xz

MS_BUILTIN_DTB=arch/arm/boot/dts/infinity3-BGA128M.dtb
_BUILTIN_DTB_NAME=$(strip $(shell echo ${CONFIG_MS_BUILTIN_DTB}))
ifneq ($(_BUILTIN_DTB_NAME),)
ifneq ($(wildcard arch/arm/boot/dts/$(_BUILTIN_DTB_NAME).dts),)
MS_BUILTIN_DTB=arch/arm/boot/dts/$(_BUILTIN_DTB_NAME).dtb
endif
endif

MS_EXTRA_DTBS=
ifneq ($(CONFIG_ARCH_INFINITY),)
MS_EXTRA_DTBS=arch/arm/boot/dts/infinity-fpga.dtb arch/arm/boot/dts/infinity-zebu.dtb
endif

#ifneq ($(CONFIG_ARCH_INFINITY3),)
#MS_EXTRA_DTBS=arch/arm/boot/dts/infinity3-fpga.dtb arch/arm/boot/dts/infinity3-fpgaxl.dtb
#endif

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/Makefile.boot
endif

# Note: the following conditions must always be true:
#   ZRELADDR == virt_to_phys(PAGE_OFFSET + TEXT_OFFSET)
#   PARAMS_PHYS must be within 4MB of ZRELADDR
#   INITRD_PHYS must be in RAM
LOADADDR    := 0x40008000
ZRELADDR    := $(zreladdr-y)
PARAMS_PHYS := $(params_phys-y)
INITRD_PHYS := $(initrd_phys-y)

export ZRELADDR INITRD_PHYS PARAMS_PHYS

targets := Image zImage xipImage bootpImage uImage

ifeq ($(CONFIG_XIP_KERNEL),y)

$(obj)/xipImage: vmlinux FORCE $(MS_BUILTIN_DTB) $(MS_EXTRA_DTBS)
	$(call if_changed,objcopy)
	@$(kecho) '  Kernel: $@ is ready (physical address: $(CONFIG_XIP_PHYS_ADDR))'
	@test "${MS_BUILTIN_DTB}" = "" || \
	if [ -e ${MS_BUILTIN_DTB} ]; then \
	    echo "  BNDTB ${MS_BUILTIN_DTB}"; \
	    python scripts/ms_builtin_dtb_update.py $@ ${MS_BUILTIN_DTB}; \
	    echo; \
	fi;
	@python scripts/ms_bin_option_update_int.py  $@ '#IMG_SZ#' $$(stat -c %s $@)
	@python scripts/ms_bin_option_update_int.py  $@ '#XIP_PA#' $(CONFIG_XIP_PHYS_ADDR)
	@echo "  xipImage size: $(shell stat -c %s $@) bytes"


$(obj)/Image $(obj)/zImage: FORCE
	@echo 'Kernel configured for XIP (CONFIG_XIP_KERNEL=y)'
	@echo 'Only the xipImage target is available in this case'
	@false

else

$(obj)/xipImage: FORCE
	@echo 'Kernel not configured for XIP (CONFIG_XIP_KERNEL!=y)'
	@false

$(obj)/Image: vmlinux FORCE $(MS_BUILTIN_DTB) $(MS_EXTRA_DTBS)
	$(call if_changed,objcopy)
	
	#update builtin DTB
	@test "${MS_BUILTIN_DTB}" = "" || \
	if [ -e ${MS_BUILTIN_DTB} ]; then \
	    echo "  BNDTB ${MS_BUILTIN_DTB}"; \
	    python scripts/ms_builtin_dtb_update.py $@ ${MS_BUILTIN_DTB}; \
	    echo; \
	fi;

    #update the image size into Image	
	@python scripts/ms_bin_option_update_int.py  $@ '#IMG_SZ#' $$(stat -c %s $@)

	#build Image     
	@LD_ADDR=40008000; \
	IMGNAME=Linux-`cat $(KERNEL_RELEASE_FILE)`; \
#	if [ -e ${MKIMAGE_BIN} ]; then \
#	    ${MKIMAGE_BIN} -A arm -O linux -T kernel -C none -a $${LD_ADDR} -e $${LD_ADDR} -n $${IMGNAME} -d $@ $(obj)/uImage; \
#	    echo; \
#	else \
#	    echo ">> Can't find $${MKIMAGE}. Please check the u-boot path or build u-boot <<"; \
#	fi; \
#    if [ -e $(ROOTFS) ]; then \
#        ${MKIMAGE_BIN} -A arm -O linux -T multi -C none -a $${LD_ADDR} -e $${LD_ADDR} -n $${IMGNAME} -d  $@:$(ROOTFS) $(obj)/kernel.img; \
#        echo; \
#        if [ -e ${MZ_BIN} ]; then \
#	        ${MZ_BIN} c  $@  $@.mz ; \
#	        echo; \
#	        ${MKIMAGE_BIN} -A arm -O linux -T multi -C mz -a $${LD_ADDR} -e $${LD_ADDR} -n $${IMGNAME} -d $@.mz:$(ROOTFS) $(obj)/kernel.mz.img; \
#	        echo; \
#        fi; \
#	fi;
	@$(kecho) '  Kernel: $@ is ready'	
	@echo
ifneq ($(CONFIG_ARCH_INFINITY),)
	@echo 'Building extra images wifh different DTB for iNfinity:'
	@echo
#	@if [ -e "arch/arm/boot/dts/infinity-zebu.dtb" ]; then \
#        echo "  IMAGE   $(obj)/Image-zebu"; \
#        cp -f $(obj)/Image $(obj)/Image-zebu; \
#        echo "  BNDTB   infinity-zebu.dtb"; \
#        python scripts/ms_builtin_dtb_update.py $(obj)/Image-zebu "arch/arm/boot/dts/infinity-zebu.dtb"; \
#	    echo; \
#	fi;
	
	@if [ -e "arch/arm/boot/dts/infinity-fpga.dtb" ]; then \
	    echo "  IMAGE   $(obj)/Image-fpga"; \
	    cp -f $(obj)/Image $(obj)/Image-fpga; \
	    echo "  BNDTB   infinity-fpga.dtb"; \
	    python scripts/ms_builtin_dtb_update.py $(obj)/Image-fpga "arch/arm/boot/dts/infinity-fpga.dtb"; \
	    echo; \
	fi;
endif	
ifneq ($(CONFIG_ARCH_INFINITY3),)
#	@echo 'Building extra images wifh different DTB for iNfinity3:'
#	@echo
#	@if [ -e "arch/arm/boot/dts/inf3-zebu.dtb" ]; then \
#        echo "  IMAGE   $(obj)/Image-zebu"; \
#        cp -f $(obj)/Image $(obj)/Image-zebu; \
#        echo "  BNDTB   inf3-zebu.dtb"; \
#        python scripts/ms_builtin_dtb_update.py $(obj)/Image-zebu "arch/arm/boot/dts/inf3-zebu.dtb"; \
#	    echo; \
#	fi;
	
#	@if [ -e "arch/arm/boot/dts/infinity3-fpga.dtb" ]; then \
#	    echo "  IMAGE   $(obj)/Image-fpga"; \
#	    cp -f $(obj)/Image $(obj)/Image-fpga; \
#	    echo "  BNDTB   infinity3-fpga.dtb"; \
#	    python scripts/ms_builtin_dtb_update.py $(obj)/Image-fpga "arch/arm/boot/dts/infinity3-fpga.dtb"; \
#	    echo; \
#	fi;
## Run Kernel file system on norflash with infinity3-fpgaxl.dts
#	@if [ -e "arch/arm/boot/dts/infinity3-fpgaxl.dtb" ]; then \
#	    echo "  IMAGE   $(obj)/Image-fpgaxl"; \
#	    cp -f $(obj)/Image $(obj)/Image-fpgaxl; \
#	    echo "  BNDTB   infinity3-fpgaxl.dtb"; \
#	    python scripts/ms_builtin_dtb_update.py $(obj)/Image-fpgaxl "arch/arm/boot/dts/infinity3-fpgaxl.dtb"; \
#	    echo; \
#	fi;
endif	

$(obj)/compressed/vmlinux: $(obj)/Image FORCE
	$(Q)$(MAKE) $(build)=$(obj)/compressed $@

$(obj)/zImage:	$(obj)/compressed/vmlinux FORCE
	$(call if_changed,objcopy)
	@$(kecho) '  Kernel: $@ is ready'

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

$(obj)/uImage:	$(obj)/zImage FORCE
	@$(check_for_multiple_loadaddr)
	$(call if_changed,uimage)
	@$(kecho) '  Image $@ is ready'

$(obj)/bootp/bootp: $(obj)/zImage initrd FORCE
	$(Q)$(MAKE) $(build)=$(obj)/bootp $@
	@:

$(obj)/bootpImage: $(obj)/bootp/bootp FORCE
	$(call if_changed,objcopy)
	@$(kecho) '  Kernel: $@ is ready'

PHONY += initrd FORCE
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

zi:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh "$(KERNELRELEASE)" \
	$(obj)/zImage System.map "$(INSTALL_PATH)"

i:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh "$(KERNELRELEASE)" \
	$(obj)/Image System.map "$(INSTALL_PATH)"

subdir-	    := bootp compressed dts
