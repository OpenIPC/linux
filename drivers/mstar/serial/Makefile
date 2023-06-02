CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))

# general options
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/include/$(CONFIG_MSTAR_CHIP_NAME)

obj-$(CONFIG_MS_SERIAL) += ms_uart.o
