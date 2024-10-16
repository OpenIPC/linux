################################################################################
#
# Common Variables that already set:
#     LICHEE_KDIR
#     LICHEE_MOD_DIR
#     CROSS_COMPILE
#     ARCH
#
#################################################################################

ifneq ($(LICHEE_KDIR),)
SOC=$(shell \
	sed -n 's/^CONFIG_ARCH_\(SUN.*P1\)=y/\1/p' $(LICHEE_KDIR)/.config 2>/dev/null | sed 's/[A-Z]/\l&/g')
else
SOC=$(shell \
	sed -n 's/^CONFIG_ARCH_\(SUN.*P1\)=y/\1/p' .config 2>/dev/null | sed 's/[A-Z]/\l&/g')
endif

# The nand driver of sun8iw15p1 is differnet between Tina and Android.
# The Android is using driver on directory A50 for large capacity while
# Tina uses driver on directory MR133 for small capacity.
ifeq ($(TARGET_BUILD_VARIANT), tina)
ifeq ($(SOC), sun8iw15p1)
SOC := $(SOC)-small
endif
endif

obj-$(CONFIG_SUNXI_NAND) += $(SOC)/

.PHONY: install build clean build_rr

install:
	@if cat $(LICHEE_KDIR)/.config|grep "CONFIG_SUNXI_NAND=m" >/dev/null 2>&1;then \
		cp $(SOC)/nand.ko $(LICHEE_MOD_DIR)/; \
	else \
		echo "[NAND] NOT buid as ko";\
	fi
clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~

build:
	@make -C ${SOC}/lib clean 2>/dev/null
	@make -C ${SOC}/lib install

build_rr:
	@make -C ${SOC}/phy-nand/physic_v2/rr_lib clean 2>/dev/null
	@make -C ${SOC}/phy-nand/physic_v2/rr_lib

build_3d:
	@make -C ${SOC}/lib-3d clean 2>/dev/null
	@make -C ${SOC}/lib-3d install
