export CIPHER_SRC_BASE=$(CURDIR)/drivers/crypto/hisi-cipher/src

ifdef CONFIG_ARCH_HI3519V101
INTERDRVVER := hi3519v101
endif

ifdef CONFIG_ARCH_HI3556
INTERDRVVER := hi3519v101
endif

ifdef CONFIG_ARCH_HI3559
INTERDRVVER := hi3519v101
endif

ifdef CONFIG_ARCH_HI3516AV200
INTERDRVVER := hi3519v101
endif

ifdef CONFIG_ARCH_HI3516CV300
INTERDRVVER := hi3516cv300
endif

ifdef CONFIG_ARCH_HI3516EV100
INTERDRVVER := hi3516cv300
endif

ifdef CONFIG_ARCH_HI3519
INTERDRVVER := hi3519
endif

CONFIG_CIPHER_MULTICIPHER_SUPPORT=y
CONFIG_CIPHER_HASH_SUPPORT=y
CONFIG_CIPHER_RSA_SUPPORT=y
CFG_HI_CIPHER_RNG_SUPPORT=y

EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)
EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/drv
EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/drv/cipher
EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/drv/hash
EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/drv/efuse
EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/include
EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/../include
EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/../arch/$(INTERDRVVER)
EXTRA_CFLAGS += -I$(CIPHER_SRC_BASE)/drv/osal/include

DRV_OBJ := drv/efuse/hal_efuse.o
DRV_OBJ += drv/cipher/drv_cipher.o drv/cipher/hal_cipher.o
DRV_OBJ += drv/hash/drv_hash_intf.o

DRV_OBJ   += drv/osal/drv_osal_init_linux.o
DRV_OBJ   += drv/osal/drv_osal_sys_linux.o

EXTRA_CFLAGS += -DCHIP_TYPE_$(INTERDRVVER)

obj-y := hi_cipher.o
hi_cipher-y += $(DRV_OBJ)
