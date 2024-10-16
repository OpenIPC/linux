CONFIG_MODULE_NAME:=xr806

CONFIG_PLATFORM_LINUX:=y

xr806-y := \
	main.o \
	txrx.o \
	up_cmd.o \
	low_cmd.o \
	hwio.o \
	queue.o \
	debug.o \
	checksum.o \
	platform/spi.o \
	test/hwio_test.o \
	test/data_test.o \
	os_dep/os_intf.o \
	os_dep/os_net.o

EXTRA_CFLAGS += -Wno-error=date-time
EXTRA_CFLAGS += -Wno-date-time
EXTRA_CFLAGS += -I$(src)/include -I$(src)/os_dep -I$(src)/platform -I$(src)/test

ifeq ($(CONFIG_PLATFORM_LINUX), y)
xr806-y += os_dep/linux/iface.o
EXTRA_CFLAGS += -DCONFIG_PLATFORM_LINUX -I$(src)/os_dep/linux
endif

$(CONFIG_MODULE_NAME)-y := $(xr806-y)

obj-m += $(CONFIG_MODULE_NAME).o
