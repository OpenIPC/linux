#ifndef __FH_GPIO_PLAT_H__
#define __FH_GPIO_PLAT_H__

#define GPIO_NAME "FH_GPIO"

#include <asm-generic/gpio.h>

enum trigger_type {
	SOFTWARE,
	HARDWARE,
};

struct gpio_irq_info {
	int irq_gpio;
	int irq_line;
	int irq_type;
	int irq_gpio_val;
	int irq_gpio_mode;
};

struct fh_gpio_chip {
	struct gpio_chip chip;
	void __iomem *base;
	struct irq_domain *irq_domain;
	struct platform_device *pdev;
	int irq;
	spinlock_t lock;

	enum trigger_type type;
	u32 gpio_wakeups;
	u32 gpio_backups;
};
#endif
