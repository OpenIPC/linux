CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

obj-y += mdrv_RNG.o

EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/crypto
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/crypto/hal/$(CONFIG_SSTAR_CHIP_NAME)

mdrv_RNG-objs := sstar-rng.o
