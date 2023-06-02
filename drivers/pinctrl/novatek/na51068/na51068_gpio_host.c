/*
    GPIO controller

    Sets the GPIO control of each pin.

    @file       na51068_gpio_host.c
    @ingroup    mIDrvSys_PAD
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include <linux/spinlock.h>
#include "na51068_pinmux.h"

static DEFINE_SPINLOCK(gpio_lock);
#define loc_cpu(lock, flags) spin_lock_irqsave(lock, flags)
#define unl_cpu(lock, flags) spin_unlock_irqrestore(lock, flags)

void __iomem *gpio_vaddr;
#define GPIO_SETREG(ofs,value)     OUTW((gpio_vaddr+(ofs)),(value))
#define GPIO_GETREG(ofs)           INW(gpio_vaddr+(ofs))
#define GPIO_DBG(fmt, ...)         pr_info(fmt, ##__VA_ARGS__)
#define GPIO_DBG_ERR(fmt, ...)     pr_err(fmt, ##__VA_ARGS__)

#define NVT_GPIO_PIN_DIR           0x08
#define NVT_GPIO_DATA_SET          0x10
#define NVT_GPIO_DATA_CLEAR        0x14
#define NVT_GPIO_NR_PIN_PER_GROUP  32

static unsigned int group_base_offset[] = {
	0x0, 0x20000, 0x40000, 0x240000,
};

void gpio_init(struct nvt_gpio_info *gpio, int nr_gpio, struct nvt_pinctrl_info *info)
{
	u32 reg_data, offset, group, val;
	int cnt = 0;
	unsigned long flags = 0;

	gpio_vaddr = info->gpio_base;

	loc_cpu(&gpio_lock, flags);

	for (cnt = 0; cnt < nr_gpio; cnt++) {
		offset = gpio[cnt].gpio_pin;
		group = (offset >> 5);
		offset &= (NVT_GPIO_NR_PIN_PER_GROUP - 1);
		val = (1 << offset);

		/* Set output */
		reg_data = GPIO_GETREG(group_base_offset[group] + NVT_GPIO_PIN_DIR);
		reg_data |= val;
		GPIO_SETREG(group_base_offset[group] + NVT_GPIO_PIN_DIR, reg_data);

		/* Set output value */
		if (gpio[cnt].direction)
			GPIO_SETREG(group_base_offset[group] + NVT_GPIO_DATA_SET, val);
		else
			GPIO_SETREG(group_base_offset[group] + NVT_GPIO_DATA_CLEAR, val);
	}

	unl_cpu(&gpio_lock, flags);
}
