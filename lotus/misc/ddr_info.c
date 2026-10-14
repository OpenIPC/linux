// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/export.h>
#include <mach/common.h>


static struct proc_dir_entry *ddr_info_entry;
static struct proc_dir_entry *ddr_bandwidth_entry;



static char *_ddr_snprintf(char *buf, size_t size, size_t *unused_size, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, size, fmt, args);
	va_end(args);

	if (i >= size)
		return NULL;

	if (unused_size)
		*unused_size = size - i;

	return &buf[i];
}

static char *add_ddr_type(char *str, size_t size, size_t *rest)
{
	char *p = str;
	size_t unused_size = size;
	char *ddr_type = get_ddr_type_string(NULL);

	p = _ddr_snprintf(p, unused_size, &unused_size, "%-16s", "DDR Type: ");
	if (p == NULL)
		goto err_out;

	p = _ddr_snprintf(p, unused_size, &unused_size, "%10s", ddr_type);
	if (p == NULL)
		goto err_out;

	p = _ddr_snprintf(p, unused_size, &unused_size, "\n");
	if (p == NULL)
		goto err_out;

	if (rest)
		*rest = unused_size;

	return p;
err_out:
	return NULL;
}

static char *add_ddr_bandwidth_util(char *str, size_t size, size_t *rest)
{
	char *p = str;
	size_t unused_size = size;
	int bandwidth_util_fract = 0;
	int bandwidth_util = 0;

	get_ddr_bandwidth_util(30, &bandwidth_util, &bandwidth_util_fract);

	p = _ddr_snprintf(p, unused_size, &unused_size, "%-16s", "Bandwidth Util: ");
	if (p == NULL)
		goto err_out;

	p = _ddr_snprintf(p, unused_size, &unused_size, "%6d.%02d%%", bandwidth_util, bandwidth_util_fract);
	if (p == NULL)
		goto err_out;

	p = _ddr_snprintf(p, unused_size, &unused_size, "\n");
	if (p == NULL)
		goto err_out;

	if (rest)
		*rest = unused_size;

	return p;
err_out:
	return NULL;
}

static int ddr_info_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t ddr_info_write(struct file *file, const char __user *ubuf, size_t count,  loff_t *ppos)
{
	return -1;
}

static ssize_t ddr_info_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	char info[256] = {0};
	char *p = info;
	size_t unused_size = sizeof(info);
	size_t read = 0;

	if (*ppos > 0 || count < sizeof(info))
		return 0;

	p = add_ddr_type(p, unused_size, &unused_size);
	if (p == NULL)
		goto err_out;

	read = sizeof(info) -  unused_size;

	if (copy_to_user(ubuf, info, read))
		return -EFAULT;

	*ppos = read;

	return read;
err_out:
	return 0;
}

static int ddr_bandwidth_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t ddr_bandwidth_write(struct file *file, const char __user *ubuf, size_t count,  loff_t *ppos)
{
	return -1;
}

static ssize_t ddr_bandwidth_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	char info[256] = {0};
	char *p = info;
	size_t unused_size = sizeof(info);
	size_t read = 0;

	if (*ppos > 0 || count < sizeof(info))
		return 0;

	p = add_ddr_bandwidth_util(p, unused_size, &unused_size);
	if (p == NULL)
		goto err_out;

	read = sizeof(info) -  unused_size;

	if (copy_to_user(ubuf, info, read))
		return -EFAULT;

	*ppos = read;

	return read;
err_out:
	return 0;
}

static struct proc_ops ddr_info_ops = {
	.proc_open = ddr_info_open,
	.proc_read  = ddr_info_read,
	.proc_write = ddr_info_write,
};

static struct proc_ops ddr_bandwidth_ops = {
	.proc_open = ddr_bandwidth_open,
	.proc_read  = ddr_bandwidth_read,
	.proc_write = ddr_bandwidth_write,
};

static int ddr_info_init(void)
{
	int ret = -1;

	ret = ddrc_init();
	if (ret < 0) {
		pr_err("Fail to init ddrc\n");
		return ret;
	}

	ddr_info_entry = proc_create("ddr_info", 0644, NULL, &ddr_info_ops);
	if (ddr_info_entry == NULL) {
		pr_err("Fail to create ddr_info proc file\n");
		return -1;
	}

	ddr_bandwidth_entry = proc_create("ddr_bandwidth", 0644, NULL, &ddr_bandwidth_ops);
	if (ddr_bandwidth_entry == NULL) {
		pr_err("Fail to create ddr_bandwidth proc file\n");
		return -1;
	}

	return 0;
}

static void ddr_info_exit(void)
{
	proc_remove(ddr_info_entry);
	proc_remove(ddr_bandwidth_entry);
	ddrc_exit();
}

module_init(ddr_info_init);
module_exit(ddr_info_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lotus");

