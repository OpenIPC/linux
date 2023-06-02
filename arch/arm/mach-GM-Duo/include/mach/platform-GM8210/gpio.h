#ifndef __GM8126_GPIO_H__
#define __GM8126_GPIO_H__

#define GM_GPIO010_NR_PORT		3
#define GM_GPIO_NR_PIN_PER_PORT		32
#define ARCH_NR_GPIOS			(GM_GPIO_NR_PIN_PER_PORT*GM_GPIO010_NR_PORT)

static inline int gm_gpio_pin_index(unsigned port, unsigned pin)
{
	int ret = port*GM_GPIO_NR_PIN_PER_PORT + pin;
	
	return ret < ARCH_NR_GPIOS ? ret : -1;
} 

#endif//end of __GM8126_GPIO_H__
