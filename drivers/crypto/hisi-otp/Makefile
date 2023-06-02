
cflags-y += -I$(srctree)/drivers/crypto/hisi-cipher/src/drv/cipher_v1.0/osal/include
cflags-y += -I$(srctree)/drivers/crypto/hisi-cipher/include
cflags-y += -I$(srctree)/drivers/crypto/hisi-cipher/src/drv/cipher_v1.0/drivers/core/include
cflags-y += -I$(srctree)/drivers/crypto/hisi-cipher/src/drv/cipher_v1.0/compat/

ifeq ($(CONFIG_ARCH_HI3519AV100),y)
cflags-y += -DCHIP_TYPE_hi3519av100
endif

ifeq ($(CONFIG_ARCH_HI3556AV100),y)
cflags-y += -DCHIP_TYPE_hi3556av100
endif

ifeq ($(CONFIG_ARCH_HI3559AV100),y)
cflags-y += -DCHIP_TYPE_hi3559av100
endif

ifeq ($(CONFIG_ARCH_HI3516CV500),y)
cflags-y += -DCHIP_TYPE_hi3516cv500
endif

ifeq ($(CONFIG_ARCH_HI3516DV300),y)
cflags-y += -DCHIP_TYPE_hi3516dv300
endif

ifeq ($(CONFIG_ARCH_HI3556V200),y)
cflags-y += -DCHIP_TYPE_hi3556v200
endif

ifeq ($(CONFIG_ARCH_HI3559V200),y)
cflags-y += -DCHIP_TYPE_hi3559v200
endif

ifeq ($(CONFIG_ARCH_HI3516AV300),y)
cflags-y += -DCHIP_TYPE_hi3516av300
endif

ifeq ($(CONFIG_ARCH_HI3516EV200),y)
cflags-y += -DCHIP_TYPE_hi3516ev200
endif

ifeq ($(CONFIG_ARCH_HI3516EV300),y)
cflags-y += -DCHIP_TYPE_hi3516ev300
endif

ifeq ($(CONFIG_ARCH_HI3518EV300),y)
cflags-y += -DCHIP_TYPE_hi3518ev300
endif

ifeq ($(CONFIG_ARCH_HI3516DV200),y)
cflags-y += -DCHIP_TYPE_hi3516dv200
endif

ccflags-y  += $(cflags-y)
HOSTCFLAGS += $(cflags-y)
CPPFLAGS   += $(cflags-y)

obj-y    += hal_otp.o
