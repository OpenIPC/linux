KDRV_DIR=$(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR=$(NVT_HDAL_DIR)/drivers/k_flow
SUBDIRS := $(dir $(shell find . -name Makefile))
SUBDIRS := $(shell echo $(SUBDIRS) | sed -e 's/\.\///g';)
# The current directory is passed to sub-makes as argument
PWD := $(shell pwd)
.PHONY: modules modules_install clean $(SUBDIRS)
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile these then includes this
# Makefile once again.
# This conditional selects whether we are being included from the
# kernel Makefile or not.
ifeq ($(KERNELRELEASE),)
export KBUILD_EXTRA_SYMBOLS = $(shell find $(VOS_DRIVER_DIR) -name Module.symvers) $(shell find $(KDRV_DIR) -name Module.symvers) $(shell find $(KFLOW_DIR) -name Module.symvers)

modules:
	@$(MAKE) -C $(KERNELDIR) M=$(PWD) $(NVT_KGCOV) modules $(NVT_MULTI_CORES_FLAG);

modules_install:
	@if [ -z $(NVT_MOD_INSTALL) ]; then \
		$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=./_install_modules/ INSTALL_MOD_DIR=hdal modules_install; \
	else \
		$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=$(NVT_MOD_INSTALL) INSTALL_MOD_DIR=hdal modules_install; \
	fi

clean:
	@rm -rf Module.symvers modules.order .tmp_versions
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n clean $(NVT_MULTI_CORES_FLAG); \
	done

codesize:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n codesize $(NVT_MULTI_CORES_FLAG); \
	done
else
# called from kernel build system: just declare what our modules are
obj-m += \
	isp/ \
	ae/ \
	awb/ \
	af/ \
	iq/ \
	samples/sample_ae/ \
	samples/sample_af/ \
	samples/sample_awb/ \
	samples/sample_iq/

endif

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)

modules: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@  modules $(NVT_MULTI_CORES_FLAG)

modules_install:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n modules_install; \
	done

clean:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n clean; \
	done
	@rm ../output -rf
endif
