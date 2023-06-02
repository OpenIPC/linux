MS_ARCH_NAME=infinity
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/include/infinity

#
# Makefile for MStar HVSP drivers.
#

# general options
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/inc
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/inc
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv

EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/msos

EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/vip/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/vip


EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/cmdq
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/cmdq

EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/pnl/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/pnl
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/pnl/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/pnl

EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/sclirq/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/sclirq
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/sclirq/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/sclirq

EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/vip/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/vip
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/vip/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/vip

EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/hvsp/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/hvsp
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/hvsp/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/hvsp

EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/scldma/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/drv/scldma
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/scldma/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/hal/scldma

EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/pq
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/pq/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/pq/hal/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/pq/hal/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/scl/$(MS_ARCH_NAME)/src/mxlib/pq/hal/include/sc

EXTRA_CFLAGS += -Werror

# specific options
#
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX_KERNEL

ccflags-$(CONFIG_ARCH_CEDRIC)		+= -DC3_SIM


obj-$(CONFIG_MS_SCL_INFINITY)   += mdrv_scl.o

mdrv_scl-y := 	  src/mxlib/msos/MsOS.o\
                  src/mdrv_multiinst.o \
                  mdrv_hvsp1_io.o\
                  mdrv_hvsp2_io.o\
                  mdrv_hvsp3_io.o\
                  src/mdrv_hvsp.o\
                  src/mxlib/hal/hvsp/halhvsp.o\
                  src/mxlib/drv/hvsp/drvhvsp.o\
                  mdrv_scldma1_io.o\
                  mdrv_scldma2_io.o\
                  mdrv_scldma3_io.o\
                  mdrv_scldma4_io.o\
                  src/mdrv_scldma.o\
                  src/mxlib/hal/sclirq/halsclirq.o\
                  src/mxlib/drv/sclirq/drvsclirq.o\
                  src/mxlib/hal/scldma/halscldma.o\
                  src/mxlib/drv/scldma/drvscldma.o\
                  mdrv_pnl_io.o\
                  src/mdrv_pnl.o\
                  src/mxlib/hal/pnl/halpnl.o\
                  src/mxlib/drv/pnl/drvpnl.o\
                  mdrv_vip_io.o\
                  src/mdrv_vip.o\
                  src/mxlib/hal/vip/halvip.o\
                  src/mxlib/drv/vip/drvvip.o\
                  src/mxlib/drv/cmdq/drvCMDQ.o\
									src/mxlib/hal/cmdq/halCMDQ.o\
                  src/mxlib/pq/drvPQ.o\
                  src/mxlib/pq/drvPQ_Bin.o\
                  src/mxlib/pq/hal/mhal_pq.o\
                  src/mxlib/pq/hal/QualityMap_Main.o\




