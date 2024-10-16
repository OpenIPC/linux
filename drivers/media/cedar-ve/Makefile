EXTRA_CFLAGS += -I$(src)/include

#EXTRA_CFLAGS += -g -O0 -v

obj-$(CONFIG_VIDEO_ENCODER_DECODER_SUNXI) := sunxi-ve.o

sunxi-ve-objs := cedar_ve.o

ifeq (${CONFIG_VIDEO_KERNEL_DEC_SUNXI},y)
include $(src)/codec/Makefile
endif

ifeq (${CONFIG_VIDEO_KERNEL_ENC_SUNXI},y)
include $(src)/codec/Makefile
sunxi-ve-objs += ${codec_obj}
endif
