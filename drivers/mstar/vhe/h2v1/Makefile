#
# Makefile for MStar MFE driver on SOC INFINITY.
#

# general options
EXTRA_CFLAGS += -Idrivers/mstar/include
EXTRA_CFLAGS += -Idrivers/mstar/include/infinity
EXTRA_CFLAGS += -Idrivers/mstar/mhve
EXTRA_CFLAGS += -Idrivers/mstar/vhe
EXTRA_CFLAGS += -Idrivers/mstar/vhe/h2v1

# files
obj-$(CONFIG_MS_VHE) += mdrv_h2v1.o

mdrv_h2v1-y :=	\
	vhe.o		\
	vhe_regs.o	\
	vhe_asic.o	\
	vhe_hevc.o	\
	vhe_rctl.o	\
	vhe_vcbr.o	\
	vhe_vsbr.o	\
	vhe_vsb3.o	\
	vhe_vcb2.o	\
