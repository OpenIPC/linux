libertas_tf-objs := main.o cmd.o

libertas_tf_usb-objs += if_usb.o
libertas_tf_sdio-objs += if_sdio.o

obj-$(CONFIG_LIBERTAS_THINFIRM) += libertas_tf.o
obj-$(CONFIG_LIBERTAS_THINFIRM_USB) += libertas_tf_usb.o
obj-$(CONFIG_LIBERTAS_THINFIRM_SDIO) += libertas_tf_sdio.o
