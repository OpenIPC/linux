// SPDX-License-Identifier: GPL-2.0
/*
 * Symbols the AX620U vendor kernel exports to its closed media modules
 * (ax_osal & co, ax_rtc, ax_hwinfo) that the AX620E SDK kernel does not
 * carry, or carries with another signature.
 * Signatures read off the call sites in those modules.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/printk.h>

/*
 * ax_printk(fmt, ...) / ax_vprintk(fmt, va_list): the AX620U kernel's
 * printk wrappers (r0 = format at every call site in ax_osal & co).
 */
#undef ax_printk
#undef ax_vprintk

asmlinkage __visible int ax_vprintk(const char *fmt, va_list args)
{
	return vprintk(fmt, args);
}
EXPORT_SYMBOL(ax_vprintk);

asmlinkage __visible int ax_printk(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintk(fmt, args);
	va_end(args);
	return r;
}
EXPORT_SYMBOL(ax_printk);

static DEFINE_SPINLOCK(ax_special_rtc_spinlock);
static unsigned long ax_special_rtc_flags;

void ax_special_rtc_lock(void)
{
	spin_lock_irqsave(&ax_special_rtc_spinlock, ax_special_rtc_flags);
}
EXPORT_SYMBOL(ax_special_rtc_lock);

void ax_special_rtc_unlock(void)
{
	spin_unlock_irqrestore(&ax_special_rtc_spinlock, ax_special_rtc_flags);
}
EXPORT_SYMBOL(ax_special_rtc_unlock);

/* Board-ID ADC read used by ax_hwinfo; AX620U has no ADC driver here. */
int read_adc_value(int chan)
{
	return 0;
}
EXPORT_SYMBOL(read_adc_value);
