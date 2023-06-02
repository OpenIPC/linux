CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

obj-y += mdrv_RNG.o

EXTRA_CFLAGS      +=  -Idrivers/sstar/include
EXTRA_CFLAGS      +=  -Idrivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/sstar/crypto
EXTRA_CFLAGS      +=  -Idrivers/sstar/crypto/hal/$(CONFIG_SSTAR_CHIP_NAME)

mdrv_RNG-objs := sstar-rng.o 
