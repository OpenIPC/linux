#linux/arch/arm/mach-ak39/Makefile
#
# Copyright 2013 Anyka Microelectronics
#
# Licensed under GPLv2

obj-y               :=
obj-m               :=
obj-n               :=
obj-                :=

# Core support for all Anyka SoCs
obj-y			+= cpu.o time.o irq.o clock.o	\
				   l2cache.o devices.o	\
				   ak39-gpio.o gpio.o	\
				   adc.o				\
				   reset.o reboot.o		\
				   timer.o		\
				   ispdrv_interface.o


# chip extra interface
obj-$(CONFIG_AK39_PWM_TIMER)		+= pwm_timer.o

# Power Management support
obj-$(CONFIG_AK39_PM)       += pm.o

# Machin support
obj-$(CONFIG_ARCH_CLOUD39EV2_AK3916E_MAINBD)                   += mach-cloud39ev2_ak3916e_mainbd.o
obj-$(CONFIG_ARCH_CLOUD39EV3_AK3916E128PIN_COREBD)             += mach-cloud39ev3_ak3916e128pin_mnbd.o
obj-$(CONFIG_ARCH_CLOUD39EV3_AK3918E80PIN_COREBD)              += mach-cloud39ev3_ak3918e80pin_mnbd.o
obj-$(CONFIG_ARCH_SKY39EV2_AK3918E80PIN_COREBD)                += mach-sky39e_ak3918e80pin_mnbd.o
obj-$(CONFIG_ARCH_CLOUD39EV3_AK3916EV300_MAINBD)               += mach-cloud39ev3_ak3916ev300_mnbd.o
obj-$(CONFIG_ARCH_CLOUD39EV3_AK3918EV300_MAINBD)               += mach-cloud39ev3_ak3918ev300_mnbd.o
obj-$(CONFIG_ARCH_CLOUD39EV3_AK3918EV300_YTJ_MAINBD)           += mach-cloud39ev3_ak3918ev300_YTJ_mnbd.o
obj-$(CONFIG_ARCH_CLOUD39EV3_AK3918EV300_SDIO_WIFI_MAINBD)     += mach-cloud39ev3_ak3918ev300_sdio_wifi_mndb.o
obj-$(CONFIG_ARCH_CLOUD39EV3_AK3919EV300_MAINBD)               += mach-cloud39ev3_ak3919ev300_mnbd.o

obj-$(CONFIG_ARCH_CLOUD39EV3_AK3919EV300_SQ38_SC2232_V1_0_0)   += mach-cloud39ev3_ak3919ev300_sq38_sc2232_V1.0.0.o
obj-$(CONFIG_ARCH_CLOUD39EV3_AK3919EV300_SQ38_SC2232_V1_0_1)   += mach-cloud39ev3_ak3919ev300_sq38_sc2232_V1.0.1.o
obj-$(CONFIG_ARCH_CLOUD39EV3_CO38_SC2235)                      += mach-cloud39ev3_co38_sc2235.o
obj-$(CONFIG_ARCH_CLOUD39EV3_SQ38_QFN64_F22_V100)              += mach-cloud39ev3_sq38_qfn64_f22_v100.o
obj-$(CONFIG_ARCH_AK3918EV300_C80DX)                           += mach-ak3918ev300_c80dx.o
obj-$(CONFIG_ARCH_AK3918EV300_SC10008_V100)                    += mach-ak3918ev300_sc10008_v100.o
obj-$(CONFIG_ARCH_AK3918EV300_HL_XIAOFANG)                     += mach-ak3918ev300_hl_xiaofang.o
obj-$(CONFIG_ARCH_CLOUD39EV3_AK3918EV300_YR_IPC100A_MAINBD)    += mach-cloud39ev3_ak3918ev300_YR_IPC100A_mnbd.o
obj-$(CONFIG_ARCH_CLOUD39EV3_SQ38X4_COREBD)                    += mach-cloud39ev3_sq38x4_mnbd.o
obj-$(CONFIG_ARCH_AK3918EV300_JW_F37_V10)                      += mach-ak3918ev300_jw_f37_v10.o
