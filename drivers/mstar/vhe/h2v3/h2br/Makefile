#
# VSi rate controller
#
EXTRA_CFLAGS += -Idrivers/mstar/vhe/h2v3
EXTRA_CFLAGS += -DCTBRC_STRENGTH
EXTRA_CFLAGS += -DLINUX_KERNEL

obj-$(CONFIG_MS_VHE) += rqc_h2br.o

rqc_h2br-y :=	\
	rate_control_picture.o  \
