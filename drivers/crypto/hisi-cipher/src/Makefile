export CIPHER_SRC_BASE=$(CURDIR)/drivers/crypto/hisi-cipher/src

include $(CIPHER_SRC_BASE)/drv/cipher_v1.0/build.mak

EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/../include
EXTRA_CFLAGS += $(CIPHER_INS)
EXTRA_CFLAGS += $(CIPHER_CFLAGS)

CIPHER_OBJS += drv/cipher_initdevice.o

obj-y += hi_cipher.o
hi_cipher-y += $(CIPHER_OBJS)

ifeq ($(findstring y, $(CONFIG_ARCH_HI3516CV500) $(CONFIG_ARCH_HI3516DV300) \
                      $(CONFIG_ARCH_HI3516AV300) $(CONFIG_ARCH_HI3559V200) \
                      $(CONFIG_ARCH_HI3562V100) $(CONFIG_ARCH_HI3566V100)), y)
INTER_DRV := hi3516cv500
else ifeq ($(findstring y, $(CONFIG_ARCH_HI3559AV100) $(CONFIG_ARCH_HI3569V100)), y)
INTER_DRV := hi3559av100
else ifeq ($(findstring y, $(CONFIG_ARCH_HI3519AV100) $(CONFIG_ARCH_HI3556AV100)), y)
INTER_DRV := hi3519av100
else ifeq ($(findstring y, $(CONFIG_ARCH_HI3516EV200) $(CONFIG_ARCH_HI3516EV300) \
                           $(CONFIG_ARCH_HI3516DV200) $(CONFIG_ARCH_HI3518EV300)), y)
INTER_DRV := hi3516ev200
endif

EXTRA_CFLAGS += -DCHIP_TYPE_$(INTER_DRV)
