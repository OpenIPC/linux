#
# Copyright (c) [2019~2020] SigmaStar Technology.
#
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 2 for more details.
#

# general options
subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/include/infinity7
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/dsp/xrp-kernel
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/dsp/xrp-hal/infinity7
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/dsp/xrp-drv/pub
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/dsp/xrp-drv/inc
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/dsp/xrp-drv/ut
SRCDIR=./xrp-kernel
HALDIR=./xrp-hal/infinity7
DRVDIR=./xrp-drv/src

ccflags-$(CONFIG_SSTAR_DSP) += -DMSOS_TYPE_LINUX_KERNEL -DDEBUG

# files
obj-$(CONFIG_SSTAR_DSP) += xrp.o

xrp-y += $(SRCDIR)/xvp_main.o $(SRCDIR)/xrp_address_map.o $(SRCDIR)/xrp_alloc.o $(SRCDIR)/xrp_hw_simple.o
xrp-y += $(HALDIR)/hal_dsp.o
xrp-y += $(HALDIR)/hal_dsp_dbg.o
xrp-y += $(DRVDIR)/mdrv_dsp.o
xrp-y += $(DRVDIR)/mdrv_dsp_virtio.o
xrp-$(CONFIG_SSTAR_DSP_UT) += $(DRVDIR)/../ut/dsp_ut.o
xrp-$(CONFIG_SSTAR_DSP_UT) += $(DRVDIR)/../ut/dsp_fw.o

xrp-$(CONFIG_OF) += $(SRCDIR)/xrp_firmware.o
xrp-$(CONFIG_CMA) += $(SRCDIR)/xrp_cma_alloc.o
