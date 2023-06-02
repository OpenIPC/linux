#
# Makefile for MStar MFE (Multi-Format Encoder) device driver.

# general options
EXTRA_CFLAGS += -Idrivers/mstar/mfe/inc
EXTRA_CFLAGS += -Idrivers/mstar/mfe
EXTRA_CFLAGS += -Idrivers/mstar/include

# files
obj-$(CONFIG_MS_MFE) += kdrv_mmfe.o

kdrv_mmfe-y :=		\
	mmfe_module.o	\
	mmfe_ops.o		\
	src/mfe_asic.o	\
	src/mfe_hdlr.o	\
	src/mfe_avc.o	\
	src/mfe_rc.o	\
