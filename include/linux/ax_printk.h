#ifndef _AX_PRINTK_H
#define _AX_PRINTK_H

#include <stdarg.h>
#include <linux/linkage.h>
#include <linux/types.h>

enum{
    AX_KERN_EMERG	= 0,
    AX_KERN_ALERT	= 1,
    AX_KERN_CRIT	= 2,
    AX_KERN_ERR     = 3,
    AX_KERN_WARNING	= 4,
    AX_KERN_NOTICE	= 5,
    AX_KERN_INFO	= 6,
    AX_KERN_DEBUG	= 7,
};

asmlinkage __printf(6, 0)
int ax_vprintk_emit(const int id, const char* tag, int level,
		 const char *dict, size_t dictlen,
		 const char *fmt, va_list args);

asmlinkage __printf(4, 0)
int ax_vprintk(const int id, const char* tag, const int level, const char *fmt, va_list args);

asmlinkage __printf(6, 7) __cold
int ax_printk_emit(const int id, const char* tag, int level,
		const char *dict, size_t dictlen,
		const char *fmt, ...);

asmlinkage __printf(4, 5) __cold
int ax_printk(const int id, const char* tag, const int level, const char *fmt, ...);

__printf(6, 0)
int ax_vprintk_store(const int id, const char* tag, int level,
                  const char *dict, size_t dictlen,
                  const char *fmt, va_list args);
__printf(4, 0) int ax_vprintk_default(const int id, const char* tag, const int level, const char *fmt, va_list args);
__printf(4, 0) int ax_vprintk_deferred(const int id, const char* tag, const int level, const char *fmt, va_list args);
__printf(4, 0) int ax_vprintk_func(const int id, const char* tag, const int level, const char *fmt, va_list args);
__printf(4, 5) __cold int ax_printk_deferred(const int id, const char* tag, const int level, const char *fmt, ...);

void __ax_printk_safe_enter(void);
void __ax_printk_safe_exit(void);
void wake_up_axklogd(void);
void ax_defer_console_output(void);
void ax_printk_safe_init(void);
void ax_printk_safe_flush(void);
void ax_printk_safe_flush_on_panic(void);
int do_axsyslog(int type, char __user *buf, int len, int source);

#define ax_no_printk(id, tag, level, fmt, ...)				\
	({							\
		if (0)						\
			ax_printk(id, tag, level, fmt, ##__VA_ARGS__); 	\
		0;						\
	})

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define ax_pr_emerg(id, tag, fmt, ...) \
    ax_printk(id, tag, AX_KERN_EMERG, pr_fmt(fmt), ##__VA_ARGS__)
#define ax_pr_alert(id, tag, fmt, ...) \
    ax_printk(id, tag, AX_KERN_ALERT, pr_fmt(fmt), ##__VA_ARGS__)
#define ax_pr_crit(id, tag, fmt, ...) \
    ax_printk(id, tag, AX_KERN_CRIT, pr_fmt(fmt), ##__VA_ARGS__)
#define ax_pr_err(id, tag, fmt, ...) \
    ax_printk(id, tag, AX_KERN_ERR, pr_fmt(fmt), ##__VA_ARGS__)
#define ax_pr_warn(id, tag, fmt, ...) \
    ax_printk(id, tag, AX_KERN_WARNING, pr_fmt(fmt), ##__VA_ARGS__)
#define ax_pr_notice(id, tag, fmt, ...) \
    ax_printk(id, tag, AX_KERN_NOTICE, pr_fmt(fmt), ##__VA_ARGS__)
#define ax_pr_info(id, tag, fmt, ...) \
    ax_printk(id, tag, AX_KERN_INFO, pr_fmt(fmt), ##__VA_ARGS__)
#define ax_pr_debug(id, tag, fmt, ...) \
    ax_printk(id, tag, AX_KERN_DEBUG, pr_fmt(fmt), ##__VA_ARGS__)

#define ax_printk_default ax_pr_warn

#endif
