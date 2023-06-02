#
# Makefile for MStar MFE driver on SOC INFINITY.
#

# general options
EXTRA_CFLAGS += -Idrivers/mstar/include
EXTRA_CFLAGS += -Idrivers/mstar/mhve
EXTRA_CFLAGS += -Idrivers/mstar/mfev5
EXTRA_CFLAGS += -Idrivers/mstar/mfev5/mfe6

# make LINUX_KERNEL version of 'ops/ios'.
EXTRA_CFLAGS += -DLINUX_KERNEL

# files
obj-$(CONFIG_MS_MFEV5) += mdrv_mfe6.o

mdrv_mfe6-y :=		\
	mfe6_ops.o	    \
	mfe6_ios.o		\
	h264_enc.o		\
	mfe6_rqc.o		\
	msb2_rqc.o		\
