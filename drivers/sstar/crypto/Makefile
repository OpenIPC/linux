CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

obj-$(CONFIG_MS_CRYPTO) += mdrv_crypto.o
obj-$(CONFIG_MS_CRYPTO) += mdrv_cryptodev.o

EXTRA_CFLAGS      +=  -Idrivers/sstar/include
EXTRA_CFLAGS      +=  -Idrivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/sstar/crypto
EXTRA_CFLAGS      +=  -Idrivers/sstar/crypto/hal/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/sstar/crypto/cryptodev
mdrv_crypto-objs := mdrv_aes.o \
               mdrv_sha.o \
               mdrv_rsa.o \
               mdrv_cipher.o \
               halAESDMA.o

mdrv_cryptodev-objs := \
	    cryptodev/mdrv_cryptioctl.o \
	    cryptodev/mdrv_cryptmain.o \
	    cryptodev/mdrv_cryptlib.o \
	    cryptodev/mdrv_cryptzc.o \
	    cryptodev/mdrv_cryptutil.o

