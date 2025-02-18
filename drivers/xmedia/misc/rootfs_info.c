/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/io.h>
#include <linux/export.h>

static struct proc_dir_entry *rootfs_info_entry;


static int rootfs_info_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t rootfs_info_write(struct file *file, const char __user *ubuf, size_t count,  loff_t *ppos)
{
	return -1;
}

static char *_rootfs_snprintf(char *buf, size_t size, size_t *remaining, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = snprintf(buf, size, fmt, args);
	va_end(args);

	if (i >= size) {
		return NULL;
	}

	if (remaining) {
		*remaining = size - i;
	}

	return &buf[i];
}

static char *add_root_name(char *str, size_t size, size_t *rest)
{
	char *p = str;
	size_t remaining = size;
	int len = 0;
	char *root_name = get_root_name(&len);

	p = _rootfs_snprintf(p, remaining, &remaining, "Root Name: ");
	if (p == NULL) {
		goto err_out;
	}

	p = _rootfs_snprintf(p, remaining, &remaining, root_name);
	if (p == NULL) {
		goto err_out;
	}

	p = _rootfs_snprintf(p, remaining, &remaining, "\n");
	if (p == NULL) {
		goto err_out;
	}

	if (rest) {
		*rest = size -  remaining;
	}

	return p;
err_out:
	return NULL;
}

static char *add_rootfs_type(char *str, size_t size, size_t *rest)
{
	char *p = str;
	size_t remaining = size;
	int len = 0;
	char *rootfs_type = get_rootfs_type(&len);

	p = _rootfs_snprintf(p, remaining, &remaining, "Rootfs Type: ");
	if (p == NULL) {
		goto err_out;
	}

	p = _rootfs_snprintf(p, remaining, &remaining, rootfs_type);
	if (p == NULL) {
		goto err_out;
	}

	p = _rootfs_snprintf(p, remaining, &remaining, "\n");
	if (p == NULL) {
		goto err_out;
	}

	if (rest) {
		*rest = size -  remaining;
	}

	return p;
err_out:
	return NULL;
}

static ssize_t rootfs_info_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	char info[256] = {0};
	char *p = info;
	size_t remaining = sizeof(info);
	size_t read = 0;

	if (*ppos > 0 || count < sizeof(info))
		return 0;

	p = add_root_name(p, remaining, &remaining);
	if (p == NULL) {
		goto err_out;
	}

	p = add_rootfs_type(p, remaining, &remaining);
	if (p == NULL) {
		goto err_out;
	}

	read = sizeof(info) -  remaining;

	if (copy_to_user(ubuf, info, read))
		return -EFAULT;

	*ppos = read;

	return read;
err_out:
	return 0;
}

static struct proc_ops rootfs_info_ops = {
	.proc_open = rootfs_info_open,
	.proc_read  = rootfs_info_read,
	.proc_write = rootfs_info_write,
};

static int rootfs_info_init(void)
{
	rootfs_info_entry = proc_create("rootfs_info", 0644, NULL, &rootfs_info_ops);
	if (rootfs_info_entry == NULL) {
		pr_err("Fail to create rootfs_info proc file\n");
		return -1;
	}

	return 0;
}

static void rootfs_info_exit(void)
{
	proc_remove(rootfs_info_entry);
}

module_init(rootfs_info_init);
module_exit(rootfs_info_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xmedia");

