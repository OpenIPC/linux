#
# Copyright (c) [2019~2022] SigmaStar Technology.
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

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/crypto/infinity
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/bdma/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/movedma/

obj-$(CONFIG_MS_MSYS) = ms_msys.o
obj-$(CONFIG_MS_MSYS) += ms_msys_dma_wrapper.o
obj-$(CONFIG_MSYS_PERF_TEST) += ms_msys_perf_test.o
obj-$(CONFIG_MS_MSYS) += $(CONFIG_SSTAR_CHIP_NAME)/platform_msys.o
obj-$(CONFIG_MS_MSYS_LOG) += ms_msys_log.o
obj-$(CONFIG_MSYS_BENCH_MEMORY_FUNC) += ms_msys_memory_bench.o
obj-$(CONFIG_MSYS_MIU_PROTECT) += ms_msys_miu_protect.o
obj-$(CONFIG_SSTAR_IRQ_DEBUG_TRACE) += sstar_irq_debug_trace.o
obj-$(CONFIG_SS_PROFILING_TIME) += prof.o

ifneq ($(CONFIG_THUMB2_KERNEL),y)
    ifneq ($(CONFIG_ARM64),y)
        obj-$(CONFIG_MS_MSYS) += memcpy.o
    else
        obj-$(CONFIG_MS_MSYS) += memcpy_64.o
    endif
endif
