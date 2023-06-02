#
# Makefile for the ambarella video device drivers.
#

# Ambarella A8 CODEC driver
rpmsg_ambxc-y +=	ambxc_vdev.o \
			ambxc_ioctl.o \
			ambxc_ctrls.o \
			ambxc_ctl.o \
			ambxc_rpmsg.o

obj-$(CONFIG_RPMSG_AMBXC) += rpmsg_ambxc.o
