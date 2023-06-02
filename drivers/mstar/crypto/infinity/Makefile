obj-$(CONFIG_MS_CRYPTO) += mdrv_crypto.o
obj-$(CONFIG_MS_CRYPTO) += mdrv_cryptodev.o
#obj-$(CONFIG_MS_CRYPTO) += mdrv_rsaioctl.o
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/include/$(CONFIG_MSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/mstar/crypto
EXTRA_CFLAGS      +=  -Idrivers/mstar/crypto/infinity/cryptodev
mdrv_crypto-objs := mdrv_aes.o \
               mdrv_sha.o \
               mdrv_rsa.o \
               halAESDMA.o

mdrv_cryptodev-objs := \
	    cryptodev/mdrv_cryptioctl.o \
	    cryptodev/mdrv_cryptmain.o \
	    cryptodev/mdrv_cryptlib.o \
	    cryptodev/mdrv_cryptzc.o \
	    cryptodev/mdrv_cryptutil.o
	    
#mdrv_rsaioctl-objs := \
#		  mdrv_rsa.o \
#		  halAESDMA.o
