#include <linux/io.h>
#include "pwm-fullhan-common.h"

#define OFFSET_PWM_STOPTIME_BIT(n)	(8 + 16 * ((n) / 8) + ((n) % 8))
#define OFFSET_PWM_FINSHALL_BIT(n)	(0 + 16 * ((n) / 8) + ((n) % 8))
#define OFFSET_PWM_FINSHONCE_BIT(n)	(8 + 16 * ((n) / 8) + ((n) % 8))

unsigned int _pwm_set_stoptime_bit(int chn)
{
	return (0x1 << OFFSET_PWM_STOPTIME_BIT(chn));
}

void _pwm_interrupt_finishall_enable(void *base, int chn)
{
	unsigned int reg;

	reg = readl(base);
	reg |= (0x1 << OFFSET_PWM_FINSHALL_BIT(chn));
	writel(reg, base);
}

void _pwm_interrupt_finishall_disable(void *base, int chn)
{
	unsigned int reg;

	reg = readl(base);
	reg &= ~(0x1 << OFFSET_PWM_FINSHALL_BIT(chn));
	writel(reg, base);
}

void _pwm_interrupt_finishonce_enable(void *base, int chn)
{
	unsigned int reg;

	reg = readl(base);
	reg |= (0x1 << OFFSET_PWM_FINSHONCE_BIT(chn));
	writel(reg, base);
}

void _pwm_interrupt_finishonce_disable(void *base, int chn)
{
	unsigned int reg;

	reg = readl(base);
	reg &= ~(0x1 << OFFSET_PWM_FINSHONCE_BIT(chn));
	writel(reg, base);
}

void _pwm_interrupt_finishonce_clear(void *base, int chn)
{
	unsigned int reg = 0x0;

	reg = readl(base);
	reg |= (0x1 << OFFSET_PWM_FINSHONCE_BIT(chn));
	return writel(reg, base);
}

void _pwm_interrupt_finishall_clear(void *base, int chn)
{
	unsigned int reg = 0x0;

	reg = readl(base);
	reg |= (0x1 << OFFSET_PWM_FINSHALL_BIT(chn));
	return writel(reg, base);
}

int _pwm_need_inverse(void)
{
	return 0;
}

void _pwm_interrupt_process(unsigned int status, int *chn_once, int *chn_all)
{
	unsigned int stat_once_l, stat_all_l;
	unsigned int stat_once_h, stat_all_h;

	status    &= 0xffffffff;
	stat_once_l = (status >> 8) & 0xff;
	stat_all_l  = status & 0xff;
	stat_once_h = (status >> 24) & 0x3f;
	stat_all_h  = (status >> 16) & 0x3f;

	if (stat_once_l)
		*chn_once = fls(stat_once_l) - 1;
	if (stat_all_l)
		*chn_all  = fls(stat_all_l) - 1;
	/* Special Process: Data will be covered but not a bug
	 * Wait for next interrupt to process low interrupt.
	 */
	if (stat_once_h)
		*chn_once = fls(stat_once_h) + 8 - 1;
	if (stat_all_h)
		*chn_all  = fls(stat_all_h) + 8 - 1;
}
