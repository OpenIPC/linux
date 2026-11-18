// SPDX-License-Identifier: GPL-2.0
/*
 * /proc/ax_proc and the Axera module log.
 *
 * Axera's kernels create /proc/ax_proc at boot; the closed media modules
 * (ax_sys, ax_ivps, ax_venc, ...) put their entries under it and fail to
 * load without it. They log through _ax_printk() into a buffer of their
 * own rather than the kernel log: here a ring of text lines, read from
 * /proc/ax_proc/kmsg. Lines at or above ax_proc.console_level (default
 * error) are also sent to the kernel log.
 */
#include <linux/ax_printk.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched/clock.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>

#define AX_LOG_BUF_LEN	(64 * 1024)
#define AX_LOG_LINE_MAX	512

static char ax_log_buf[AX_LOG_BUF_LEN];
static size_t ax_log_head;	/* next write position */
static bool ax_log_wrapped;
static DEFINE_SPINLOCK(ax_log_lock);

static int console_level = AX_KERN_ERR;
module_param(console_level, int, 0644);
MODULE_PARM_DESC(console_level, "ax_printk levels <= this also go to the kernel log (-1: none)");

static int log_level = AX_KERN_INFO;
module_param(log_level, int, 0644);
MODULE_PARM_DESC(log_level, "ax_printk levels <= this are kept in /proc/ax_proc/kmsg");

static void ax_log_store(const char *s, size_t len)
{
	unsigned long flags;
	size_t n;

	spin_lock_irqsave(&ax_log_lock, flags);
	while (len) {
		n = min(len, AX_LOG_BUF_LEN - ax_log_head);
		memcpy(ax_log_buf + ax_log_head, s, n);
		ax_log_head += n;
		if (ax_log_head == AX_LOG_BUF_LEN) {
			ax_log_head = 0;
			ax_log_wrapped = true;
		}
		s += n;
		len -= n;
	}
	spin_unlock_irqrestore(&ax_log_lock, flags);
}

int ax_vprintk(const int id, const char *tag, const int level,
	       const char *fmt, va_list args)
{
	static const char * const kern_lvl[] = {
		KERN_EMERG, KERN_ALERT, KERN_CRIT, KERN_ERR,
		KERN_WARNING, KERN_NOTICE, KERN_INFO, KERN_DEBUG,
	};
	char line[AX_LOG_LINE_MAX];
	u64 ts = local_clock();
	unsigned long rem = do_div(ts, NSEC_PER_SEC);
	int lvl = clamp(level, AX_KERN_EMERG, AX_KERN_DEBUG);
	int hdr, len;

	if (lvl > log_level && lvl > console_level)
		return 0;

	hdr = scnprintf(line, sizeof(line), "[%5lu.%06lu][%d][%s] ",
			(unsigned long)ts, rem / 1000, lvl, tag ? tag : "AX");
	len = hdr + vscnprintf(line + hdr, sizeof(line) - hdr, fmt, args);
	if (len && line[len - 1] != '\n') {
		if (len == sizeof(line) - 1)
			len--;
		line[len++] = '\n';
		line[len] = '\0';
	}

	if (lvl <= log_level)
		ax_log_store(line, len);
	if (lvl <= console_level)
		printk("%s[%s] %s", kern_lvl[lvl], tag ? tag : "AX", line + hdr);
	return len;
}
EXPORT_SYMBOL(ax_vprintk);

int _ax_printk(const int id, const char *tag, const int level,
	       const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = ax_vprintk(id, tag, level, fmt, args);
	va_end(args);
	return r;
}
EXPORT_SYMBOL(_ax_printk);

static ssize_t ax_kmsg_read(struct file *file, char __user *buf,
			    size_t count, loff_t *ppos)
{
	unsigned long flags;
	size_t head, size, start, off, n;
	char *snap;
	ssize_t ret;

	snap = kvmalloc(AX_LOG_BUF_LEN, GFP_KERNEL);
	if (!snap)
		return -ENOMEM;

	spin_lock_irqsave(&ax_log_lock, flags);
	head = ax_log_head;
	size = ax_log_wrapped ? AX_LOG_BUF_LEN : head;
	start = ax_log_wrapped ? head : 0;
	n = AX_LOG_BUF_LEN - start;
	memcpy(snap, ax_log_buf + start, min(n, size));
	if (size > n)
		memcpy(snap + n, ax_log_buf, size - n);
	spin_unlock_irqrestore(&ax_log_lock, flags);

	off = *ppos;
	ret = 0;
	if (off < size) {
		n = min(count, size - off);
		if (copy_to_user(buf, snap + off, n))
			ret = -EFAULT;
		else {
			*ppos += n;
			ret = n;
		}
	}
	kvfree(snap);
	return ret;
}

static const struct proc_ops ax_kmsg_ops = {
	.proc_read	= ax_kmsg_read,
	.proc_lseek	= default_llseek,
};

static int ax_version_show(struct seq_file *m, void *v)
{
	seq_printf(m, "Ax_Version OpenIPC %s\n", utsname()->release);
	return 0;
}

static int __init ax_proc_init(void)
{
	if (!proc_mkdir("ax_proc", NULL))
		return -ENOMEM;
	proc_create_single("ax_proc/version", 0444, NULL, ax_version_show);
	proc_create("ax_proc/kmsg", 0400, NULL, &ax_kmsg_ops);
	return 0;
}
fs_initcall(ax_proc_init);
