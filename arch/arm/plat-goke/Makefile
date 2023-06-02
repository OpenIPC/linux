obj-y	+= gk_gpio.o
obj-y += timer.o
obj-$(CONFIG_SERIAL_GOKE) += uart.o
obj-$(CONFIG_ETH_GOKE) += eth.o
obj-y += cache.o event.o highres_timer.o sync_proc.o 
obj-y += fb.o
obj-y += pmu.o
obj-y += sd.o
obj-y += hw_timer.o
obj-y += switch.o

obj-$(CONFIG_GK_GPIO_V1_00)	+= gk_gpio_v1_00.o
obj-$(CONFIG_GK_GPIO_V1_10)	+= gk_gpio_v1_10.o
obj-$(CONFIG_GK_GPIO_V1_20)	+= gk_gpio_v1_20.o

ccflags-y += -Iarch/arm/plat-goke/include/
