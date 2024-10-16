lib_dir := codec/lib

EXTRA_CFLAGS += -I$(src)/$(lib_dir)/include

lib_obj += $(lib_dir)/libbase
lib_obj += $(lib_dir)/libve
lib_obj += $(lib_dir)/libmem

ifeq (${CONFIG_VIDEO_KERNEL_DEC_SUNXI},y)
lib_obj += $(lib_dir)/libvdecoder
endif

ifeq (${CONFIG_VIDEO_KERNEL_ENC_SUNXI},y)
lib_obj += $(lib_dir)/libvencoder
endif

