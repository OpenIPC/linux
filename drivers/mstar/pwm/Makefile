CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))

# specific options
#EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

# general options
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/include/$(CONFIG_MSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/mstar/pwm/$(CONFIG_MSTAR_CHIP_NAME)

# files
obj-$(CONFIG_MS_PWM) += ms_pwm.o
ms_pwm-y             += mdrv_pwm.o $(CONFIG_MSTAR_CHIP_NAME)/mhal_pwm.o

