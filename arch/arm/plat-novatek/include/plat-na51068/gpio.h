#ifndef __NVT_GPIO_H__
#define __NVT_GPIO_H__

#define NVT_GPIO010_NR_PORT		4
#define NVT_GPIO_NR_PIN_PER_PORT		32

#define ARCH_NR_GPIOS			(NVT_GPIO_NR_PIN_PER_PORT*NVT_GPIO010_NR_PORT)

static inline int nvt_gpio_pin_index(unsigned port, unsigned pin)
{
	int ret = port*NVT_GPIO_NR_PIN_PER_PORT + pin;
	
	return ret < ARCH_NR_GPIOS ? ret : -1;
} 

#endif//end of __NVT_GPIO_H__
