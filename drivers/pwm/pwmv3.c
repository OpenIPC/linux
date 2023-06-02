#include <linux/io.h>
#include "pwm-fullhan-common.h"

unsigned int _pwm_set_stoptime_bit(int chn)
{
	return (0x1 << (16 + chn));
}

void _pwm_interrupt_finishall_enable(void *base, int chn)
{
	unsigned int reg;

	reg = readl(base);
	reg |= (0x1 << chn);
	writel(reg, base);
}

void _pwm_interrupt_finishall_disable(void *base, int chn)
{
	unsigned int reg;

	reg = readl(base);
	reg &= ~(0x1 << chn);
	writel(reg, base);
}

void _pwm_interrupt_finishonce_enable(void *base, int chn)
{
	unsigned int reg;

	reg = readl(base);
	reg |= (0x1 << (16 + chn));
	writel(reg, base);
}

void _pwm_interrupt_finishonce_disable(void *base, int chn)
{
	unsigned int reg;

	reg = readl(base);
	reg &= ~(0x1 << (16 + chn));
	writel(reg, base);
}

void _pwm_interrupt_finishonce_clear(void *base, int chn)
{
	unsigned int reg = 0x0;

	reg = readl(base);
	reg &= ~(0x1 << (16 + chn));
	return writel(reg, base);
}

void _pwm_interrupt_finishall_clear(void *base, int chn)
{
	unsigned int reg = 0x0;

	reg = readl(base);
	reg &= ~(0x1 << chn);
	return writel(reg, base);
}

int _pwm_need_inverse(void)
{
	return 1;
}

void _pwm_interrupt_process(unsigned int status, int *chn_once, int *chn_all)
{
	unsigned int stat_once, stat_all;

	status    &= 0xffffffff;
	stat_once = (status >> 16) & 0xffff;
	stat_all  = status & 0xffff;
	if (stat_once)
		*chn_once = fls(stat_once) - 1;
	if (stat_all)
		*chn_all  = fls(stat_all) - 1;
}
