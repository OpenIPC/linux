#
# Makefile for MStar MFE driver on SOC INFINITY.
#

# general options
EXTRA_CFLAGS += -Idrivers/mstar/include
EXTRA_CFLAGS += -Idrivers/mstar/mhve
EXTRA_CFLAGS += -Idrivers/mstar/mfev5
EXTRA_CFLAGS += -Idrivers/mstar/mfev5/mfe5

# files
obj-$(CONFIG_MS_MFEV5) += mdrv_mfe5.o

mdrv_mfe5-y :=		\
	mfe.o			\
	mfe_asic.o		\
	mfe_h264.o		\
	mfe_rctl.o		\
	mfe_cvbr.o		\
	mfe_msbr.o		\
	mfe_msb2.o		\
	mfe_msb3.o		\
