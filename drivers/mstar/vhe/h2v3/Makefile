#
# Makefile for MStar MFE driver on SOC INFINITY3E.
#
H2BR := h2br

# general options
EXTRA_CFLAGS += -Idrivers/mstar/include
EXTRA_CFLAGS += -Idrivers/mstar/include/infinity3
EXTRA_CFLAGS += -Idrivers/mstar/mhve
EXTRA_CFLAGS += -Idrivers/mstar/vhe
EXTRA_CFLAGS += -Idrivers/mstar/vhe/h2v3
EXTRA_CFLAGS += -Idrivers/mstar/vhe/h2v3/$(H2BR)
EXTRA_CFLAGS += -DCTBRC_STRENGTH
#EXTRA_CFLAGS += -DROIMAP
#EXTRA_CFLAGS += -DTRACEREG
#EXTRA_CFLAGS += -DRCTRACE

# make LINUX_KERNEL version of 'ops/ios'.
EXTRA_CFLAGS += -DLINUX_KERNEL

# files
obj-$(CONFIG_MS_VHE) += mdrv_h2v3.o
obj-$(CONFIG_MS_VHE) += $(H2BR)/

mdrv_h2v3-y :=	\
	h2v3_ops.o	\
	h2v3_ios.o	\
	h2v3_rqc.o	\
	h2v3_reg.o	\
	h265_enc.o	\
	vmbr_rqc.o	\
	h2br_rqc.o	\
