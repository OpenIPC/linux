/*
 * TI DaVinci GPIO Support
 *
 * Copyright (c) 2006 David Brownell
 * Copyright (c) 2007, MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __FH_GPIO_H
#define __FH_GPIO_H

#include <linux/io.h>
#include <linux/spinlock.h>

#include <asm-generic/gpio.h>

#include <mach/irqs.h>
#include <mach/fh_predefined.h>

/*
 * GPIO Direction
 */
#define GPIO_DIR_INPUT          0
#define GPIO_DIR_OUTPUT         1

/*
 * GPIO interrupt type
 */
#define GPIO_INT_TYPE_LEVEL     0
#define GPIO_INT_TYPE_EDGE      1

/*
 * GPIO interrupt polarity
 */
#define GPIO_INT_POL_LOW        0
#define GPIO_INT_POL_HIGH       1

#define OFFSET_GPIO_SWPORTA_DR         (0x0000)
#define OFFSET_GPIO_SWPORTA_DDR        (0x0004)
#define OFFSET_GPIO_PORTA_CTL          (0x0008)
#define OFFSET_GPIO_SWPORTB_DR         (0x000C)
#define OFFSET_GPIO_SWPORTB_DDR        (0x0010)
#define OFFSET_GPIO_PORTB_CTL          (0x0014)
#define OFFSET_GPIO_INTEN              (0x0030)
#define OFFSET_GPIO_INTMASK            (0x0034)
#define OFFSET_GPIO_INTTYPE_LEVEL      (0x0038)
#define OFFSET_GPIO_INT_POLARITY       (0x003C)
#define OFFSET_GPIO_INTSTATUS          (0x0040)
#define OFFSET_GPIO_RAWINTSTATUS       (0x0044)
#define OFFSET_GPIO_DEBOUNCE           (0x0048)
#define OFFSET_GPIO_PORTA_EOI          (0x004C)
#define OFFSET_GPIO_EXT_PORTA          (0x0050)
#define OFFSET_GPIO_EXT_PORTB          (0x0054)

static inline void FH_GPIO_SetValue(unsigned int base, int bit, int val)
{
	unsigned int reg;

	reg = GET_REG(base + OFFSET_GPIO_SWPORTA_DR);
	reg = val ? (reg | (1 << bit)) : (reg & ~(1 << bit));
	SET_REG(base + OFFSET_GPIO_SWPORTA_DR, reg);
}

static inline int FH_GPIO_GetValue(unsigned int base, int bit)
{
	return (GET_REG(base + OFFSET_GPIO_EXT_PORTA) >> bit) & 0x1;
}

static inline void FH_GPIO_SetDirection(unsigned int base, int bit, int dir)
{
	unsigned int reg;

	reg = GET_REG(base + OFFSET_GPIO_SWPORTA_DDR);
	reg = dir ? reg | (1 << bit) : reg & ~(1 << bit);
	SET_REG(base + OFFSET_GPIO_SWPORTA_DDR, reg);
}

static inline int FH_GPIO_GetDirection(unsigned int base, int bit)
{
	return (GET_REG(base + OFFSET_GPIO_SWPORTA_DDR) >> bit) & 0x1;
}

static inline void FH_GPIOB_SetValue(unsigned int base, int bit, int val)
{
	unsigned int reg;

	reg = GET_REG(base + OFFSET_GPIO_SWPORTB_DR);
	reg = val ? (reg | (1 << bit)) : (reg & ~(1 << bit));
	SET_REG(base + OFFSET_GPIO_SWPORTB_DR, reg);
}

static inline int FH_GPIOB_GetValue(unsigned int base, int bit)
{
	return (GET_REG(base + OFFSET_GPIO_EXT_PORTB) >> bit) & 0x1;
}

static inline void FH_GPIOB_SetDirection(unsigned int base, int bit, int dir)
{
	unsigned int reg;

	reg = GET_REG(base + OFFSET_GPIO_SWPORTB_DDR);
	reg = dir ? reg | (1 << bit) : reg & ~(1 << bit);
	SET_REG(base + OFFSET_GPIO_SWPORTB_DDR, reg);
}

static inline int FH_GPIOB_GetDirection(unsigned int base, int bit)
{
	return (GET_REG(base + OFFSET_GPIO_SWPORTB_DDR) >> bit) & 0x1;
}

static inline void FH_GPIO_EnableDebounce(unsigned int base, int bit, int bool)
{
	unsigned int reg;

	reg = GET_REG(base + OFFSET_GPIO_DEBOUNCE);
	reg = bool ? reg | (1 << bit) : reg & ~(1 << bit);
	SET_REG(base + OFFSET_GPIO_DEBOUNCE, reg);
}

static inline void FH_GPIO_SetInterruptType(unsigned int base, int bit,
		int type)
{
	unsigned int reg;

	reg = GET_REG(base + OFFSET_GPIO_INTTYPE_LEVEL);
	reg = type ? reg | (1 << bit) : reg & ~(1 << bit);
	SET_REG(base + OFFSET_GPIO_INTTYPE_LEVEL, reg);
}

static inline void FH_GPIO_SetInterruptPolarity(unsigned int base, int bit,
		int pol)
{
	unsigned int reg;

	reg = GET_REG(base + OFFSET_GPIO_INT_POLARITY);
	reg = pol ? reg | (1 << bit) : reg & ~(1 << bit);
	SET_REG(base + OFFSET_GPIO_INT_POLARITY, reg);
}

static inline void FH_GPIO_EnableInterruptMask(unsigned int base, int bit,
		int bool)
{
	unsigned int reg;

	reg = GET_REG(base + OFFSET_GPIO_INTMASK);
	reg = bool ? reg | (1 << bit) : reg & ~(1 << bit);
	SET_REG(base + OFFSET_GPIO_INTMASK, reg);
}

static inline void FH_GPIO_EnableInterrupt(unsigned int base, int bit, int bool)
{
	unsigned int reg;

	reg = GET_REG(base + OFFSET_GPIO_INTEN);
	reg = bool ? reg | (1 << bit) : reg & ~(1 << bit);
	SET_REG(base + OFFSET_GPIO_INTEN, reg);
}

static inline void FH_GPIO_SetEnableInterrupts(unsigned int base,
		unsigned int val)
{
	SET_REG(base + OFFSET_GPIO_INTEN, val);
}

static inline unsigned int FH_GPIO_GetEnableInterrupts(unsigned int base)
{
	return GET_REG(base + OFFSET_GPIO_INTEN);
}

static inline unsigned int FH_GPIO_GetInterruptStatus(unsigned int base)
{
	return GET_REG(base + OFFSET_GPIO_INTSTATUS);
}

static inline void FH_GPIO_ClearInterrupt(unsigned int base, int bit)
{
	unsigned int reg;

	reg = GET_REG(base + OFFSET_GPIO_PORTA_EOI);
	reg |= (1 << bit);
	SET_REG(base + OFFSET_GPIO_PORTA_EOI, reg);
}

#define GPIO_NAME "FH_GPIO"


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

	struct platform_device *pdev;
	int irq;
	spinlock_t lock;

	u32 gpio_wakeups;
	u32 gpio_backups;
};

/*
 * The get/set/clear functions will inline when called with constant
 * parameters referencing built-in GPIOs, for low-overhead bitbanging.
 *
 * gpio_set_value() will inline only on traditional Davinci style controllers
 * with distinct set/clear registers.
 *
 * Otherwise, calls with variable parameters or referencing external
 * GPIOs (e.g. on GPIO expander chips) use outlined functions.
 */
static inline void gpio_set_value(unsigned gpio, int value)
{
    __gpio_set_value(gpio, value);
}

/* Returns zero or nonzero; works for gpios configured as inputs OR
 * as outputs, at least for built-in GPIOs.
 *
 * NOTE: for built-in GPIOs, changes in reported values are synchronized
 * to the GPIO clock.  This is easily seen after calling gpio_set_value()
 * and then immediately gpio_get_value(), where the gpio_get_value() will
 * return the old value until the GPIO clock ticks and the new value gets
 * latched.
 */
static inline int gpio_get_value(unsigned gpio)
{
    return __gpio_get_value(gpio);
}

static inline int gpio_cansleep(unsigned gpio)
{
    return 0;
}

static inline int gpio_to_irq(unsigned gpio)
{
    return __gpio_to_irq(gpio);
}

static inline int irq_to_gpio(unsigned irq)
{
    return 0;
}

void fh_gpio_irq_suspend(void);
void fh_gpio_irq_resume(void);

#endif

