EXTRA_CFLAGS += -I$(src)/../cedar-ve/codec/lib/include \
				-I$(src)/../cedar-ve/include \
				-I$(src)/../cedar-ve/codec/vencoder \
				-I$(src)/../cedar-ve/codec/base \
				-I$(src)/../cedar-ve/codec/memory \
				-I$(src)/../cedar-ve/codec/ve \
				-I$(src)/../platform/sunxi-vin/vin-video

obj-$(CONFIG_VIDEO_RT_MEDIA) += rt-media.o

rt-media-objs := rt_media.o
rt-media-objs += component/rt_message.o
rt-media-objs += component/rt_component.o
rt-media-objs += component/rt_venc_component.o
rt-media-objs += component/rt_vi_component.o
rt-media-objs += ../cedar-ve/codec/lib/libvencoder
rt-media-objs += ../cedar-ve/codec/lib/libbase
rt-media-objs += ../cedar-ve/codec/lib/libve
rt-media-objs += ../cedar-ve/codec/lib/libgcc

