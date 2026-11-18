/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Axera module log (ax_printk): the vendor media modules log through
 * _ax_printk(id, tag, level, fmt, ...) into a buffer of their own, apart
 * from the kernel log. See drivers/soc/axera/ax_proc.c.
 */
#ifndef _AX_PRINTK_H
#define _AX_PRINTK_H

#include <linux/compiler.h>
#include <linux/stdarg.h>

enum {
	AX_KERN_EMERG	= 0,
	AX_KERN_ALERT	= 1,
	AX_KERN_CRIT	= 2,
	AX_KERN_ERR	= 3,
	AX_KERN_WARNING	= 4,
	AX_KERN_NOTICE	= 5,
	AX_KERN_INFO	= 6,
	AX_KERN_DEBUG	= 7,
};

#ifdef CONFIG_AXERA_OSAL
__printf(4, 0)
int ax_vprintk(const int id, const char *tag, const int level,
	       const char *fmt, va_list args);
__printf(4, 5) __cold
int _ax_printk(const int id, const char *tag, const int level,
	       const char *fmt, ...);
#else
static inline __printf(4, 0)
int ax_vprintk(const int id, const char *tag, const int level,
	       const char *fmt, va_list args) { return 0; }
static inline __printf(4, 5)
int _ax_printk(const int id, const char *tag, const int level,
	       const char *fmt, ...) { return 0; }
#endif

#define ax_printk(id, tag, level, fmt, ...) \
	_ax_printk(id, tag, level, fmt, ##__VA_ARGS__)

#define ax_pr_emerg(id, tag, fmt, ...)	ax_printk(id, tag, AX_KERN_EMERG, fmt, ##__VA_ARGS__)
#define ax_pr_alert(id, tag, fmt, ...)	ax_printk(id, tag, AX_KERN_ALERT, fmt, ##__VA_ARGS__)
#define ax_pr_crit(id, tag, fmt, ...)	ax_printk(id, tag, AX_KERN_CRIT, fmt, ##__VA_ARGS__)
#define ax_pr_err(id, tag, fmt, ...)	ax_printk(id, tag, AX_KERN_ERR, fmt, ##__VA_ARGS__)
#define ax_pr_warn(id, tag, fmt, ...)	ax_printk(id, tag, AX_KERN_WARNING, fmt, ##__VA_ARGS__)
#define ax_pr_notice(id, tag, fmt, ...)	ax_printk(id, tag, AX_KERN_NOTICE, fmt, ##__VA_ARGS__)
#define ax_pr_info(id, tag, fmt, ...)	ax_printk(id, tag, AX_KERN_INFO, fmt, ##__VA_ARGS__)
#define ax_pr_debug(id, tag, fmt, ...)	ax_printk(id, tag, AX_KERN_DEBUG, fmt, ##__VA_ARGS__)

#endif
