/*
    GPIO controller

    Sets the GPIO control of each pin.

    @file       na51055_gpio_host.c
    @ingroup    mIDrvSys_PAD
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "na51055_pinmux.h"

static DEFINE_SPINLOCK(gpio_lock);
#define loc_cpu(lock, flags) spin_lock_irqsave(lock, flags)
#define unl_cpu(lock, flags) spin_unlock_irqrestore(lock, flags)

void gpio_init(struct nvt_gpio_info *gpio, int nr_gpio, struct nvt_pinctrl_info *info)
{
	u32 reg_data, tmp, ofs, offset;
	int cnt = 0;
	unsigned long flags = 0;

	loc_cpu(&gpio_lock, flags);

	for (cnt = 0; cnt < nr_gpio; cnt++) {
		offset = gpio[cnt].gpio_pin;
		ofs = (offset >> 5) << 2;
		offset &= (32-1);
		tmp = (1 << offset);
		reg_data = GPIO_GETREG(info, NVT_GPIO_STG_DIR_0 + ofs);
		reg_data |= (1<<offset);    /*output*/
		GPIO_SETREG(info, NVT_GPIO_STG_DIR_0 + ofs, reg_data);

		if (gpio[cnt].direction)
			GPIO_SETREG(info, NVT_GPIO_STG_SET_0 + ofs, tmp);
		else
			GPIO_SETREG(info, NVT_GPIO_STG_CLR_0 + ofs, tmp);
	}

	unl_cpu(&gpio_lock, flags);
}
