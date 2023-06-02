#export CIPHER_SRC_BASE=$(CURDIR)/drivers/crypto/hisi-cipher/src
export CIPHER_SRC_BASE=$(src)

include $(CIPHER_SRC_BASE)/drv/cipher_v1.0/build.mak

ifeq ($(CONFIG_ARCH_HI3519AV100),y)
INTER_DRV := hi3519av100
endif

ifeq ($(CONFIG_ARCH_HI3556AV100),y)
INTER_DRV := hi3556av100
endif

ifeq ($(CONFIG_ARCH_HI3559AV100),y)
INTER_DRV := hi3559av100
endif

ifeq ($(CONFIG_ARCH_HI3516CV500),y)
INTER_DRV := hi3516cv500
endif

ifeq ($(CONFIG_ARCH_HI3516DV300),y)
INTER_DRV := hi3516dv300
endif

ifeq ($(CONFIG_ARCH_HI3556V200),y)
INTER_DRV := hi3556v200
endif

ifeq ($(CONFIG_ARCH_HI3559V200),y)
INTER_DRV := hi3559v200
endif

ifeq ($(CONFIG_ARCH_HI3516AV300),y)
INTER_DRV := hi3516av300
endif

ifeq ($(CONFIG_ARCH_HI3516EV200),y)
INTER_DRV := hi3516ev200
endif

ifeq ($(CONFIG_ARCH_HI3516EV300),y)
INTER_DRV := hi3516ev300
endif

ifeq ($(CONFIG_ARCH_HI3518EV300),y)
INTER_DRV := hi3518ev300
endif

ifeq ($(CONFIG_ARCH_HI3516DV200),y)
INTER_DRV := hi3516dv200
endif

EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/../include
EXTRA_CFLAGS += $(CIPHER_INS)
EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/../../hisi-otp

CIPHER_OBJS += drv/cipher_initdevice.o

EXTRA_CFLAGS += -DCHIP_TYPE_$(INTER_DRV)
EXTRA_CFLAGS += $(CIPHER_CFLAGS)

obj-y += hi_cipher.o
hi_cipher-y += $(CIPHER_OBJS)
